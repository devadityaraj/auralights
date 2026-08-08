#include "config.h"
#include "secrets.h"
#include "device_state.h"
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "led_controller.h"
#include "ota_manager.h"

DeviceState deviceState;

void printBanner() {
  Serial.println();
  Serial.println(F("================================"));
  Serial.println(F("ESP8266 LED CONTROLLER"));
  Serial.println(F("================================"));
  Serial.println();
  Serial.print(F("Firmware version: "));
  Serial.println(FIRMWARE_VERSION);
  Serial.print(F("Device ID: "));
  Serial.println(DEVICE_ID);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  printBanner();

  LEDController::begin();

  WiFiManager::begin();
  bool wifiUp = WiFiManager::waitForConnection(WIFI_BOOT_TIMEOUT_MS);

  if (wifiUp && OTA_CHECK_ON_BOOT) {
    OTAManager::checkForUpdates();
  }

  if (wifiUp) {
    Serial.println();
    Serial.println(F("Connecting to Firebase..."));
    FirebaseManager::begin();

    if (FirebaseManager::isReady()) {
      if (!FirebaseManager::readInitialState(deviceState)) {
        Serial.println(F("[Setup] Using default LED state (device node not found yet)"));
      }
      LEDController::update(deviceState);
      FirebaseManager::startStream();
    }
  } else {
    Serial.println(F("[Setup] Continuing offline - WiFi will keep retrying in the background"));
  }

  Serial.println();
  Serial.println(F("LED controller ready"));
  Serial.println();
}

void loop() {
  WiFiManager::handle();

  bool stateChangedByFirebase = false;
  FirebaseManager::handle(deviceState, stateChangedByFirebase);

  LEDController::update(deviceState);

#if OTA_PERIODIC_CHECK_INTERVAL_MS > 0
  static uint32_t lastPeriodicOtaCheck = 0;
  if (WiFiManager::isConnected() &&
      millis() - lastPeriodicOtaCheck >= OTA_PERIODIC_CHECK_INTERVAL_MS) {
    lastPeriodicOtaCheck = millis();
    OTAManager::checkForUpdates();
  }
#endif
}
