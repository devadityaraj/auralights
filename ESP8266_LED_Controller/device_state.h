#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <Arduino.h>
#include <FastLED.h>

enum class EffectType {
  STATIC, RAINBOW, BREATHING, FIRE, AURORA, CANDLE,
  CYBERWAVE, STROBE, PULSE,
  RAIN, METEOR, TWINKLE, CHASE, PARTY, BOUNCE
};

struct DeviceState {
  bool       power      = true;
  uint8_t    brightness = 80;
  uint8_t    speed      = 50;
  EffectType effect     = EffectType::RAINBOW;
  CRGB       color      = CRGB(112, 232, 255);
  uint64_t   timerEnd   = 0;
};

EffectType effectFromString(const String& name);
String     effectToString(EffectType effect);
CRGB       parseColor(const String& hexColor);
uint8_t    clampPercent(int value);
String     getDefaultDeviceId();

#endif
