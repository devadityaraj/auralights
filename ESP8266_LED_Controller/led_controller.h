#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include "device_state.h"

namespace LEDController {

  void begin();
  void update(const DeviceState& state);

}

#endif
