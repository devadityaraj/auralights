#include "device_state.h"
#include <ESP8266WiFi.h>

EffectType effectFromString(const String& name) {
  if (name == "Solid"    || name == "static")      return EffectType::STATIC;
  if (name == "Rainbow"  || name == "rainbow")     return EffectType::RAINBOW;
  if (name == "Breathe"  || name == "breathing")   return EffectType::BREATHING;
  if (name == "Fire"     || name == "fire")        return EffectType::FIRE;
  if (name == "Aurora"   || name == "wave")        return EffectType::AURORA;
  if (name == "Candle")                            return EffectType::CANDLE;
  if (name == "Cyberwave")                         return EffectType::CYBERWAVE;
  if (name == "Strobe")                            return EffectType::STROBE;
  if (name == "Pulse"    || name == "color_cycle") return EffectType::PULSE;
  if (name == "Rain")                              return EffectType::RAIN;
  if (name == "Meteor")                            return EffectType::METEOR;
  if (name == "Twinkle")                           return EffectType::TWINKLE;
  if (name == "Chase")                             return EffectType::CHASE;
  if (name == "Party")                             return EffectType::PARTY;
  if (name == "Bounce")                            return EffectType::BOUNCE;

  Serial.print(F("[State] Unknown effect '"));
  Serial.print(name);
  Serial.println(F("' - falling back to Rainbow"));
  return EffectType::RAINBOW;
}

String effectToString(EffectType e) {
  switch (e) {
    case EffectType::STATIC:    return "Solid";
    case EffectType::RAINBOW:   return "Rainbow";
    case EffectType::BREATHING: return "Breathe";
    case EffectType::FIRE:      return "Fire";
    case EffectType::AURORA:    return "Aurora";
    case EffectType::CANDLE:    return "Candle";
    case EffectType::CYBERWAVE: return "Cyberwave";
    case EffectType::STROBE:    return "Strobe";
    case EffectType::PULSE:     return "Pulse";
    case EffectType::RAIN:      return "Rain";
    case EffectType::METEOR:    return "Meteor";
    case EffectType::TWINKLE:   return "Twinkle";
    case EffectType::CHASE:     return "Chase";
    case EffectType::PARTY:     return "Party";
    case EffectType::BOUNCE:    return "Bounce";
  }
  return "Rainbow";
}

uint8_t clampPercent(int v) {
  if (v < 0)   return 0;
  if (v > 100) return 100;
  return (uint8_t)v;
}

CRGB parseColor(const String& hexColor) {
  String h = hexColor;
  h.trim();
  if (h.startsWith("#")) h.remove(0, 1);
  if (h.length() != 6) return CRGB::White;
  for (unsigned int i = 0; i < h.length(); i++) {
    if (!isHexadecimalDigit(h[i])) return CRGB::White;
  }
  long v = strtol(h.c_str(), nullptr, 16);
  return CRGB((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

String getDefaultDeviceId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[24];
  snprintf(buf, sizeof(buf), "ESP8266_%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(buf);
}
