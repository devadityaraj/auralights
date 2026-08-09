#include "firebase_manager.h"
#include <FirebaseESP8266.h>
#include <time.h>
#include "config.h"
#include "secrets.h"
#include "wifi_manager.h"

namespace {
  FirebaseData fbdo;
  FirebaseData streamData;
  FirebaseAuth auth;
  FirebaseConfig fbConfig;

  String uid;
  String devicePath;
  bool firebaseInitialized = false;
  bool ready = false;
  bool streamActive = false;
  bool initialReadDone = false;

  uint32_t lastStreamRetry = 0;

  volatile bool streamEventPending = false;
  String pendingDataPath;
  String pendingDataType;
  bool   pendingBoolVal = false;
  int    pendingIntVal  = 0;
  String pendingStrVal  = "";
}

void streamCallback(StreamData data) {
  pendingDataPath = data.dataPath();
  pendingDataType = data.dataType();

  if (pendingDataType == "boolean") {
    pendingBoolVal = data.boolData();
  } else if (pendingDataType == "int") {
    pendingIntVal = data.intData();
  } else if (pendingDataType == "string" || pendingDataType == "json") {
    pendingStrVal = data.stringData();
  }

  streamEventPending = true;
}

void streamTimeoutCallback(bool timeout) {
  if (timeout) {
    Serial.println(F("[Firebase] Stream keep-alive timeout"));
  }
  if (!streamData.httpConnected()) {
    streamActive = false;
    Serial.print(F("[Firebase] Stream disconnected: "));
    Serial.println(streamData.errorReason());
  }
}

namespace FirebaseManager {

void begin() {
  if (firebaseInitialized) return;

  fbConfig.api_key = FIREBASE_API_KEY;
  fbConfig.database_url = FIREBASE_DATABASE_URL;

  auth.user.email = FIREBASE_USER_EMAIL;
  auth.user.password = FIREBASE_USER_PASSWORD;

  fbdo.setResponseSize(2048);
  streamData.setResponseSize(2048);

  Firebase.begin(&fbConfig, &auth);
  Firebase.reconnectWiFi(true);

  firebaseInitialized = true;
  Serial.println(F("[Firebase] Initialized auth and configuration"));
}

bool isReady() { return ready; }
String getDevicePath() { return devicePath; }

bool readInitialState(DeviceState& state) {
  if (!Firebase.ready()) return false;

  if (!Firebase.getJSON(fbdo, devicePath)) {
    Serial.print(F("[Firebase] Could not read initial state: "));
    Serial.println(fbdo.errorReason());
    Serial.println(F("[Firebase] Device node may not exist yet - using defaults"));
    return false;
  }

  FirebaseJson& json = fbdo.jsonObject();
  FirebaseJsonData result;

  if (json.get(result, "power")) {
    if (result.typeNum == FirebaseJson::JSON_BOOL) {
      state.power = result.boolValue;
    } else if (result.typeNum == FirebaseJson::JSON_INT) {
      state.power = (result.intValue != 0);
    } else if (result.typeNum == FirebaseJson::JSON_STRING) {
      state.power = (result.stringValue == "true" || result.stringValue == "1");
    } else {
      state.power = result.boolValue;
    }
  }

  if (json.get(result, "brightness")) state.brightness = clampPercent(result.intValue);
  if (json.get(result, "speed"))      state.speed      = clampPercent(result.intValue);
  if (json.get(result, "effect"))     state.effect     = effectFromString(result.stringValue);
  if (json.get(result, "color"))      state.color      = parseColor(result.stringValue);

  if (json.get(result, "timerEnd")) {
    if (result.typeNum == FirebaseJson::JSON_DOUBLE || result.typeNum == FirebaseJson::JSON_INT) {
      state.timerEnd = (uint64_t)result.doubleValue;
    } else if (result.stringValue.length() > 0 && result.stringValue != "null") {
      state.timerEnd = strtoull(result.stringValue.c_str(), nullptr, 10);
    } else {
      state.timerEnd = 0;
    }
  } else {
    state.timerEnd = 0;
  }

  Serial.println(F("[Firebase] Initial device configuration loaded:"));
  Serial.print(F("  power = "));
  Serial.println(state.power ? "ON" : "OFF");
  Serial.print(F("  brightness = "));
  Serial.println(state.brightness);
  Serial.print(F("  effect = "));
  Serial.println(effectToString(state.effect));
  return true;
}

void startStream() {
  if (!ready || !Firebase.ready()) return;

  Serial.print(F("[Firebase] Starting stream on: "));
  Serial.println(devicePath);

  if (!Firebase.beginStream(streamData, devicePath)) {
    Serial.print(F("[Firebase] Could not begin stream: "));
    Serial.println(streamData.errorReason());
    streamActive = false;
    return;
  }

  Firebase.setStreamCallback(streamData, streamCallback, streamTimeoutCallback);
  streamActive = true;
  Serial.println(F("[Firebase] Realtime stream active"));
}

void handle(DeviceState& state, bool& stateChangedByFirebase) {
  stateChangedByFirebase = false;

  if (!WiFiManager::isConnected()) {
    streamActive = false;
    return;
  }

  if (!firebaseInitialized) {
    begin();
    return;
  }

  // Critical: Firebase.ready() manages automatic 1-hour token renewal and authentication state
  if (!Firebase.ready()) {
    return;
  }

  if (!ready) {
    uid = auth.token.uid.c_str();
    if (uid.length() == 0) {
      return;
    }

    devicePath = "/users/" + uid + "/devices/" + String(DEVICE_ID);
    ready = true;

    Serial.println(F("[Firebase] Authentication ready & token valid"));
    Serial.print(F("[Firebase] UID: "));
    Serial.println(uid);
    Serial.print(F("[Firebase] Device path: "));
    Serial.println(devicePath);

    if (!initialReadDone) {
      if (readInitialState(state)) {
        stateChangedByFirebase = true;
      }
      initialReadDone = true;
    }
    startStream();
  }

  if (ready) {
    if (!streamActive || !streamData.httpConnected()) {
      uint32_t now = millis();
      if (now - lastStreamRetry >= STREAM_RECONNECT_INTERVAL_MS) {
        lastStreamRetry = now;
        Serial.println(F("[Firebase] Stream inactive or disconnected - reconnecting..."));
        startStream();
      }
    }
  }

  if (streamEventPending) {
    streamEventPending = false;
    stateChangedByFirebase = true;

    if (pendingDataPath == "/power") {
      if (pendingDataType == "boolean") {
        state.power = pendingBoolVal;
      } else if (pendingDataType == "int") {
        state.power = (pendingIntVal != 0);
      } else if (pendingDataType == "string") {
        state.power = (pendingStrVal == "true" || pendingStrVal == "1");
      }
      Serial.print(F("[Stream] power -> "));
      Serial.println(state.power ? F("ON") : F("OFF"));
    } else if (pendingDataPath == "/brightness") {
      state.brightness = clampPercent(pendingIntVal);
      Serial.print(F("[Stream] brightness -> "));
      Serial.println(state.brightness);
    } else if (pendingDataPath == "/speed") {
      state.speed = clampPercent(pendingIntVal);
      Serial.print(F("[Stream] speed -> "));
      Serial.println(state.speed);
    } else if (pendingDataPath == "/effect") {
      state.effect = effectFromString(pendingStrVal);
      Serial.print(F("[Stream] effect -> "));
      Serial.println(effectToString(state.effect));
    } else if (pendingDataPath == "/color") {
      state.color = parseColor(pendingStrVal);
      Serial.print(F("[Stream] color -> "));
      Serial.println(pendingStrVal);
    } else if (pendingDataPath == "/timerEnd") {
      if (pendingDataType == "int" || pendingDataType == "float" || pendingDataType == "double") {
        state.timerEnd = (uint64_t)pendingIntVal;
      } else if (pendingStrVal.length() > 0 && pendingStrVal != "null") {
        state.timerEnd = strtoull(pendingStrVal.c_str(), nullptr, 10);
      } else {
        state.timerEnd = 0;
      }
      Serial.print(F("[Stream] timerEnd -> "));
      Serial.println((double)state.timerEnd, 0);
    } else if (pendingDataPath == "/" && pendingStrVal.length() > 0) {
      FirebaseJson json;
      json.setJsonData(pendingStrVal);
      FirebaseJsonData result;

      if (json.get(result, "power")) {
        if (result.typeNum == FirebaseJson::JSON_BOOL) {
          state.power = result.boolValue;
        } else if (result.typeNum == FirebaseJson::JSON_INT) {
          state.power = (result.intValue != 0);
        } else if (result.typeNum == FirebaseJson::JSON_STRING) {
          state.power = (result.stringValue == "true" || result.stringValue == "1");
        } else {
          state.power = result.boolValue;
        }
      }
      if (json.get(result, "brightness")) state.brightness = clampPercent(result.intValue);
      if (json.get(result, "speed"))      state.speed      = clampPercent(result.intValue);
      if (json.get(result, "effect"))     state.effect     = effectFromString(result.stringValue);
      if (json.get(result, "color"))      state.color      = parseColor(result.stringValue);
      if (json.get(result, "timerEnd")) {
        if (result.typeNum == FirebaseJson::JSON_DOUBLE || result.typeNum == FirebaseJson::JSON_INT) {
          state.timerEnd = (uint64_t)result.doubleValue;
        } else if (result.stringValue.length() > 0 && result.stringValue != "null") {
          state.timerEnd = strtoull(result.stringValue.c_str(), nullptr, 10);
        } else {
          state.timerEnd = 0;
        }
      }
      Serial.print(F("[Stream] full state sync -> power: "));
      Serial.println(state.power ? F("ON") : F("OFF"));
    }
  }

  if (state.timerEnd > 0) {
    time_t nowSec = time(nullptr);
    if (nowSec > 1700000000ULL) {
      uint64_t nowMs = (uint64_t)nowSec * 1000ULL;
      if (nowMs >= state.timerEnd) {
        Serial.println(F("[Timer] Target time reached! Turning off LEDs & clearing timer in Firebase..."));
        state.power = false;
        state.timerEnd = 0;
        stateChangedByFirebase = true;

        FirebaseJson updateJson;
        updateJson.set("power", false);
        updateJson.set("timerEnd", 0);
        updateJson.set("timerMinutes", 0);
        Firebase.updateNode(fbdo, devicePath, updateJson);
      }
    }
  }
}

}
