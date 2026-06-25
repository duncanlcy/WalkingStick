import asyncio
import json
import time
from contextlib import asynccontextmanager
from pathlib import Path

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

from backend.alerts.engine import AlertEngine
from backend.sensors.models import SensorBatch, SensorReading, WalkingBehavior

FRONTEND_DIR = Path(__file__).resolve().parents[1] / "frontend"


class AnalyzeRequest(BaseModel):
    readings: list[dict]


class ConnectionManager:
    def __init__(self):
        self.active: list[WebSocket] = []

    async def connect(self, websocket: WebSocket):
        await websocket.accept()
        self.active.append(websocket)

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active:
            self.active.remove(websocket)

    async def broadcast(self, message: dict):
        dead = []
        for ws in self.active:
            try:
                await ws.send_json(message)
            except Exception:
                dead.append(ws)
        for ws in dead:
            self.disconnect(ws)


manager = ConnectionManager()
engines: dict[str, AlertEngine] = {}


def get_engine(session_id: str) -> AlertEngine:
    if session_id not in engines:
        engines[session_id] = AlertEngine()
    return engines[session_id]


@asynccontextmanager
async def lifespan(app: FastAPI):
    yield
    engines.clear()


app = FastAPI(
    title="WalkingStick Gait Monitor",
    description="Detects unstable walking from sensor data and issues visual/voice warnings",
    version="0.1.0",
    lifespan=lifespan,
)

app.mount("/static", StaticFiles(directory=FRONTEND_DIR), name="static")


@app.get("/")
async def root():
    return FileResponse(FRONTEND_DIR / "index.html")


@app.get("/api/health")
async def health():
    return {"status": "ok", "service": "walkingstick-gait-monitor"}


@app.get("/api/thresholds")
async def get_thresholds():
    path = Path(__file__).resolve().parents[1] / "shared" / "gait_thresholds.json"
    with open(path) as f:
        return json.load(f)


@app.post("/api/analyze")
async def analyze_batch(request: AnalyzeRequest):
    readings = [SensorReading.from_dict(r) for r in request.readings]
    engine = AlertEngine()
    result, alert = engine.process_batch(readings)
    response = {"analysis": result.to_dict(), "alert": alert.to_dict() if alert else None}
    return response


@app.post("/api/reset/{session_id}")
async def reset_session(session_id: str):
    if session_id in engines:
        engines[session_id].reset()
    return {"status": "reset", "session_id": session_id}


@app.websocket("/ws/{session_id}")
async def websocket_endpoint(websocket: WebSocket, session_id: str):
    await manager.connect(websocket)
    engine = get_engine(session_id)

    try:
        await websocket.send_json({"type": "connected", "session_id": session_id})

        while True:
            data = await websocket.receive_json()
            msg_type = data.get("type", "sensor")

            if msg_type == "reset":
                engine.reset()
                await websocket.send_json({"type": "reset_ack"})
                continue

            if msg_type == "sensor":
                reading = SensorReading.from_dict(data["reading"])
                result, alert = engine.process_reading(reading)

                payload = {
                    "type": "update",
                    "reading": reading.to_dict(),
                    "analysis": result.to_dict() if result else None,
                    "alert": alert.to_dict() if alert else None,
                }
                await websocket.send_json(payload)

                if alert:
                    await manager.broadcast({"type": "alert", "alert": alert.to_dict(), "session_id": session_id})

            elif msg_type == "batch":
                batch = SensorBatch.from_dict(data)
                for reading in batch.readings:
                    result, alert = engine.process_reading(reading)
                    payload = {
                        "type": "update",
                        "reading": reading.to_dict(),
                        "analysis": result.to_dict() if result else None,
                        "alert": alert.to_dict() if alert else None,
                    }
                    await websocket.send_json(payload)
                    if alert:
                        await manager.broadcast({"type": "alert", "alert": alert.to_dict(), "session_id": session_id})
                    await asyncio.sleep(0.02)

    except WebSocketDisconnect:
        manager.disconnect(websocket)


def generate_sample_readings(behavior: str = "stable", duration_sec: float = 10.0, sample_rate: float = 20.0) -> list[dict]:
    """Generate synthetic IMU data for testing."""
    import math
    import random

    readings = []
    n = int(duration_sec * sample_rate)
    t0 = time.time()

    for i in range(n):
        t = t0 + i / sample_rate
        phase = i / sample_rate * 2.0

        if behavior == "stable":
            accel_z = 9.8 + 0.3 * math.sin(phase * 4 * math.pi)
            accel_x = 0.1 * math.sin(phase * 4 * math.pi)
            accel_y = 0.05 * math.cos(phase * 2 * math.pi)
            gyro_x, gyro_y, gyro_z = 0.02, 0.01, 0.01
        elif behavior == "unstable":
            wobble = 0.8 * math.sin(phase * 1.2)
            step_freq = 3.5 + wobble
            accel_z = 9.8 + 1.4 * math.sin(phase * step_freq * math.pi) + random.gauss(0, 0.35)
            accel_x = 1.2 * math.sin(phase * step_freq * math.pi + 0.3) + random.gauss(0, 0.45)
            accel_y = 0.9 * math.cos(phase * (step_freq * 0.7) * math.pi) + random.gauss(0, 0.35)
            if i % 7 < 3:
                accel_x *= 1.6
            gyro_x = 0.35 * math.sin(phase * 2.5)
            gyro_y = 0.25 * math.cos(phase * 1.8)
            gyro_z = random.gauss(0, 0.12)
        elif behavior == "near_fall":
            accel_z = 9.8 + 0.5 * math.sin(phase * 3 * math.pi)
            accel_x = 0.3 * math.sin(phase * 2 * math.pi)
            accel_y = 0.2 * math.cos(phase * 2 * math.pi)
            if i > n * 0.7:
                spike = (i - n * 0.7) / (n * 0.3)
                accel_x += spike * 3.0
                accel_y += spike * 2.5
                accel_z -= spike * 4.0
            gyro_x, gyro_y, gyro_z = 0.3, 0.4, 0.2
        else:
            accel_z, accel_x, accel_y = 9.8, 0.0, 0.0
            gyro_x = gyro_y = gyro_z = 0.0

        readings.append(
            {
                "timestamp": t,
                "accel_x": accel_x,
                "accel_y": accel_y,
                "accel_z": accel_z,
                "gyro_x": gyro_x,
                "gyro_y": gyro_y,
                "gyro_z": gyro_z,
                "behavior": behavior,
            }
        )
    return readings


@app.get("/api/sample/{behavior}")
async def get_sample_data(behavior: str):
    if behavior not in ("stable", "unstable", "near_fall"):
        behavior = "stable"
    return {"readings": generate_sample_readings(behavior)}
