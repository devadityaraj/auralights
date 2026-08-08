#include "ota_manager.h"

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <ArduinoJson.h>

#include "config.h"

namespace {

bool looksLikeVersion(const String& v) {
  if (v.length() == 0) return false;
  bool sawDot = false;
  for (unsigned int i = 0; i < v.length(); i++) {
    char c = v[i];
    if (c == '.') { sawDot = true; continue; }
    if (c < '0' || c > '9') return false;
  }
  return sawDot;
}

String extractVersionFromFilename(String filename) {
  if (filename.endsWith(".bin")) {
    filename.remove(filename.length() - 4);
  }
  int lastSep = -1;
  for (unsigned int i = 0; i < filename.length(); i++) {
    if (filename[i] == '-' || filename[i] == '_') lastSep = i;
  }
  String candidate = (lastSep >= 0) ? filename.substring(lastSep + 1) : filename;
  candidate.trim();
  if (candidate.startsWith("v") || candidate.startsWith("V")) candidate.remove(0, 1);
  return candidate;
}

String stripLeadingV(String v) {
  if (v.startsWith("v") || v.startsWith("V")) v.remove(0, 1);
  return v;
}

int compareVersions(String a, String b) {
  a = stripLeadingV(a);
  b = stripLeadingV(b);

  int aParts[3] = {0, 0, 0};
  int bParts[3] = {0, 0, 0};

  int idx = 0, start = 0;
  for (unsigned int i = 0; i <= a.length() && idx < 3; i++) {
    if (i == a.length() || a[i] == '.') {
      aParts[idx++] = a.substring(start, i).toInt();
      start = i + 1;
    }
  }
  idx = 0; start = 0;
  for (unsigned int i = 0; i <= b.length() && idx < 3; i++) {
    if (i == b.length() || b[i] == '.') {
      bParts[idx++] = b.substring(start, i).toInt();
      start = i + 1;
    }
  }

  for (int i = 0; i < 3; i++) {
    if (aParts[i] != bParts[i]) return aParts[i] - bParts[i];
  }
  return 0;
}

bool fetchLatestRelease(String& outVersion, String& outAssetUrl) {
  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(1024, 512);

  HTTPClient https;
  String url = String("https://") + OTA_GITHUB_API_HOST +
               "/repos/" + OTA_GITHUB_OWNER + "/" + OTA_GITHUB_REPO + "/releases/latest";

  if (!https.begin(client, url)) {
    Serial.println(F("[OTA] Could not open HTTPS connection to GitHub"));
    return false;
  }

  https.addHeader("User-Agent", OTA_USER_AGENT);
  https.addHeader("Accept", "application/vnd.github+json");

  int httpCode = https.GET();

  if (httpCode == 404) {
    Serial.println(F("[OTA] No releases published yet for this repo - skipping"));
    https.end();
    return false;
  }
  if (httpCode != HTTP_CODE_OK) {
    Serial.print(F("[OTA] GitHub API request failed, HTTP code: "));
    Serial.println(httpCode);
    https.end();
    return false;
  }

  StaticJsonDocument<256> filter;
  filter["tag_name"] = true;
  JsonObject assetFilter = filter["assets"].createNestedObject();
  assetFilter["name"] = true;
  assetFilter["browser_download_url"] = true;

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(
      doc, https.getStream(), DeserializationOption::Filter(filter));
  https.end();

  if (err) {
    Serial.print(F("[OTA] Failed to parse GitHub response: "));
    Serial.println(err.c_str());
    return false;
  }

  const char* tag = doc["tag_name"] | "";

  String binUrl, binName;
  for (JsonObject asset : doc["assets"].as<JsonArray>()) {
    String name = asset["name"].as<String>();
    if (name.endsWith(".bin")) {
      binUrl = asset["browser_download_url"].as<String>();
      binName = name;
      break;
    }
  }

  if (binUrl.length() == 0) {
    Serial.println(F("[OTA] Latest release has no .bin asset attached"));
    return false;
  }

  String fromFilename = extractVersionFromFilename(binName);
  String fromTag = String(tag);

  if (looksLikeVersion(fromFilename)) {
    outVersion = fromFilename;
  } else if (looksLikeVersion(fromTag)) {
    outVersion = stripLeadingV(fromTag);
  } else {
    Serial.println(F("[OTA] Could not determine a version number from the release"));
    return false;
  }

  outAssetUrl = binUrl;
  return true;
}

void performUpdate(const String& assetUrl) {
  Serial.print(F("[OTA] Downloading and flashing: "));
  Serial.println(assetUrl);

  WiFiClientSecure client;
  client.setInsecure();
  client.setBufferSizes(1024, 512);

  ESPhttpUpdate.rebootOnUpdate(true);
  ESPhttpUpdate.followRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

  t_httpUpdate_return ret = ESPhttpUpdate.update(client, assetUrl);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.print(F("[OTA] Update failed ("));
      Serial.print(ESPhttpUpdate.getLastError());
      Serial.print(F("): "));
      Serial.println(ESPhttpUpdate.getLastErrorString());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println(F("[OTA] Server reported no update available"));
      break;
    case HTTP_UPDATE_OK:
      Serial.println(F("[OTA] Update applied - rebooting"));
      break;
  }
}

}

namespace OTAManager {

void checkForUpdates() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[OTA] Skipped: WiFi not connected"));
    return;
  }

  Serial.println(F("[OTA] Checking devadityaraj/auralights for updates..."));
  Serial.print(F("[OTA] Current version: "));
  Serial.println(FIRMWARE_VERSION);

  String latestVersion, assetUrl;
  if (!fetchLatestRelease(latestVersion, assetUrl)) {
    return;
  }

  Serial.print(F("[OTA] Latest published version: "));
  Serial.println(latestVersion);

  if (compareVersions(latestVersion, FIRMWARE_VERSION) > 0) {
    Serial.println(F("[OTA] Newer firmware available - updating now"));
    performUpdate(assetUrl);
  } else {
    Serial.println(F("[OTA] Already running the latest version"));
  }
}

}
