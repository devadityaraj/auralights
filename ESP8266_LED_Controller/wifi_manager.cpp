#include "wifi_manager.h"
#include <ESP8266WiFi.h>
#include <time.h>
#include "config.h"
#include "secrets.h"

namespace {
  uint32_t lastReconnectAttempt = 0;
}

namespace WiFiManager {

void begin() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.setAutoReconnect(true);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("Connecting to WiFi: "));
  Serial.println(WIFI_SSID);
}

bool waitForConnection(uint32_t timeoutMs) {
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("WiFi connected"));
    Serial.print(F("IP: "));
    Serial.println(WiFi.localIP());

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println(F("[NTP] Time sync started"));

    return true;
  }

  Serial.println(F("WiFi connect timed out - continuing boot, will keep retrying"));
  return false;
}

bool isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void handle() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  uint32_t now = millis();
  if (now - lastReconnectAttempt >= WIFI_RECONNECT_INTERVAL_MS) {
    lastReconnectAttempt = now;
    Serial.println(F("[WiFi] Disconnected - attempting reconnect"));
    WiFi.reconnect();
  }
}

}
