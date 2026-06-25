$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Port = if ($env:PORT) { $env:PORT } else { "8080" }
$DemoIndex = Join-Path $Root "demo\index.html"

if (-not (Test-Path $DemoIndex)) {
  Write-Host "ERROR: demo not found at $DemoIndex" -ForegroundColor Red
  Write-Host "Run: git pull   (demo was added after the initial firmware-only commit)"
  exit 1
}

$Python = $null
foreach ($candidate in @("python", "py", "python3")) {
  if (Get-Command $candidate -ErrorAction SilentlyContinue) {
  $version = & $candidate -c "import sys; print(sys.version_info.major)" 2>$null
    if ($version -eq "3") {
      $Python = $candidate
      break
    }
  }
}

if (-not $Python) {
  Write-Host "ERROR: Python 3 is required." -ForegroundColor Red
  Write-Host "Install from https://www.python.org/downloads/ and check 'Add Python to PATH'."
  exit 1
}

Write-Host "Starting WalkingStick demo on http://localhost:$Port"
Set-Location $Root
& $Python demo/server.py --port $Port @args
