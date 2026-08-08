#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Arduino.h>
#include "device_state.h"

namespace FirebaseManager {

  void begin();
  bool isReady();
  String getDevicePath();
  bool readInitialState(DeviceState& state);
  void startStream();
  void handle(DeviceState& state, bool& stateChangedByFirebase);

}

#endif
