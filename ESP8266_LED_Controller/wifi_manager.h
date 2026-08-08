#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

namespace WiFiManager {

  void begin();
  bool waitForConnection(uint32_t timeoutMs);
  void handle();
  bool isConnected();

}

#endif
