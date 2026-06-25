#include <Arduino.h>

#if defined(ROLE_WAIST_HUB)
#include "waist_hub/hub_app.h"
#elif defined(ROLE_WALKING_STICK)
#include "walking_stick/stick_app.h"
#elif defined(ROLE_SHOE_PAD)
#include "shoe_pad/shoe_app.h"
#else
#error "No device role defined. Build with -DROLE_WAIST_HUB, -DROLE_WALKING_STICK, or -DROLE_SHOE_PAD."
#endif

void setup() {
  Serial.begin(115200);
  delay(200);

#if defined(ROLE_WAIST_HUB)
  walkingstick::waist_hub_setup();
#elif defined(ROLE_WALKING_STICK)
  walkingstick::walking_stick_setup();
#elif defined(ROLE_SHOE_PAD)
  walkingstick::shoe_pad_setup();
#endif
}

void loop() {
#if defined(ROLE_WAIST_HUB)
  walkingstick::waist_hub_loop();
#elif defined(ROLE_WALKING_STICK)
  walkingstick::walking_stick_loop();
#elif defined(ROLE_SHOE_PAD)
  walkingstick::shoe_pad_loop();
#endif
}
