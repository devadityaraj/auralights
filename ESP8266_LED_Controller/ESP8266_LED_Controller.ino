#include "config.h"
#include "secrets.h"
#include "device_state.h"
#include "wifi_manager.h"
#include "firebase_manager.h"
#include "led_controller.h"

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
  Serial.print(F("WiFi SSID: "));
  Serial.println(WIFI_SSID);
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  printBanner();

  LEDController::begin();

  WiFiManager::begin();
  bool wifiUp = WiFiManager::waitForConnection(WIFI_BOOT_TIMEOUT_MS);

  if (wifiUp) {
    FirebaseManager::begin();
  } else {
    Serial.println(F("[Setup] Continuing offline - WiFi and Firebase will connect in background"));
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
}
