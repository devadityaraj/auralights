#include "led_controller.h"
#include <Arduino.h>
#include <string.h>
#include <FastLED.h>
#include "config.h"

namespace {
  CRGB leds1[LED_COUNT_1];
  uint8_t heat1[LED_COUNT_1];
#if DUAL_STRIP_ENABLED
  CRGB leds2[LED_COUNT_2];
  uint8_t heat2[LED_COUNT_2];
#endif

  uint32_t lastFrame  = 0;
  bool     offApplied = false;

  uint8_t  rainbowHue   = 0;
  uint16_t auroraPos    = 0;
  uint8_t  breathPhase  = 0;
  uint8_t  breathHue    = 140;
  uint8_t  breathTick   = 0;
  uint8_t  pulsePhase   = 0;
  uint8_t  pulseHue     = 180;
  uint8_t  pulseTick    = 0;
  uint16_t wavePos      = 0;
  uint8_t  candleBase   = 200;
  uint32_t lastStrobeMs = 0;
  bool     strobeState  = false;

  struct MeteorPalette { CRGB head; CRGB tail; };
  static const MeteorPalette meteorPalettes[] = {
    { CRGB(0,   120, 255), CRGB(0,   220, 255) },  // Blue     -> Cyan
    { CRGB(255, 60,  0  ), CRGB(255, 200, 0  ) },  // Orange-red -> Yellow
    { CRGB(0,   220, 180), CRGB(255, 255, 255) },  // Cyan     -> White
    { CRGB(255, 10,  0  ), CRGB(255, 120, 0  ) },  // Red      -> Orange
    { CRGB(160, 0,   255), CRGB(0,   80,  255) },  // Purple   -> Blue
    { CRGB(255, 220, 0  ), CRGB(255, 55,  0  ) },  // Yellow   -> Orange-Red
  };
  static const uint8_t METEOR_PALETTE_COUNT = 6;

  uint16_t meteorTick      = 0;
  uint8_t  meteorColorIdx  = 0;
  int      meteorLastPos1  = -1;
  int      meteorLastPos2  = -1;
  uint8_t  chasePos   = 0;
  uint8_t  chaseHue   = 0;
  uint8_t  partyHue   = 0;
  int16_t  bouncePos  = 0;
  int8_t   bounceDir  = 1;
  uint8_t  bounceHue  = 0;

  enum class Transition { IDLE, FADE_OUT, FADE_IN };
  EffectType activeEffect         = EffectType::RAINBOW;
  EffectType pendingEffect        = EffectType::RAINBOW;
  Transition transitionState      = Transition::IDLE;
  uint8_t    transitionBrightness = 255;
  uint32_t   lastTransitionStep   = 0;
  static const uint16_t FADE_STEP_MS  = 8;
  static const uint8_t  FADE_STEP_AMT = 32;

  uint16_t getEffectIntervalMs(EffectType effect) {
    switch (effect) {
      case EffectType::STATIC:    return SPEED_STATIC_MS;
      case EffectType::RAINBOW:   return SPEED_RAINBOW_MS;
      case EffectType::BREATHING: return SPEED_BREATHE_MS;
      case EffectType::PULSE:     return SPEED_PULSE_MS;
      case EffectType::AURORA:    return SPEED_AURORA_MS;
      case EffectType::CANDLE:    return SPEED_CANDLE_MS;
      case EffectType::FIRE:      return SPEED_FIRE_MS;
      case EffectType::CYBERWAVE: return SPEED_CYBERWAVE_MS;
      case EffectType::STROBE:    return SPEED_STROBE_MS;
      case EffectType::RAIN:      return SPEED_RAIN_MS;
      case EffectType::METEOR:    return SPEED_METEOR_MS;
      case EffectType::TWINKLE:   return SPEED_TWINKLE_MS;
      case EffectType::CHASE:     return SPEED_CHASE_MS;
      case EffectType::PARTY:     return SPEED_PARTY_MS;
      case EffectType::BOUNCE:    return SPEED_BOUNCE_MS;
      default:                    return 20;
    }
  }

  void renderStatic(CRGB* buf, int n, const DeviceState& s) {
    fill_solid(buf, n, s.color);
  }

  void renderRainbow(CRGB* buf, int n) {
    uint8_t dh = (n > 1) ? max(1, 255 / n) : 1;
    fill_rainbow(buf, n, rainbowHue, dh);
  }

  void renderBreathe(CRGB* buf, int n) {
    uint8_t wave = ease8InOutQuad(sin8(breathPhase));
    CRGB col = CHSV(breathHue, 220, wave);
    fill_solid(buf, n, col);
  }

  void renderPulse(CRGB* buf, int n) {
    uint8_t wave = ease8InOutQuad(sin8(pulsePhase));
    for (int i = 0; i < n; i++) {
      uint8_t h = pulseHue + (i * 20 / max(1, n));
      buf[i] = CHSV(h, 240, max((uint8_t)8, wave));
    }
  }

  void renderAurora(CRGB* buf, int n) {
    for (int i = 0; i < n; i++) {
      uint8_t wave = sin8((i * 120 / max(1, n)) + auroraPos);
      uint8_t h = map8(wave, 96, 205);
      buf[i] = CHSV(h, 230, 255);
    }
  }

  void renderCandle(CRGB* buf, int n) {
    uint8_t flicker = random8(40);
    uint8_t val = (candleBase > flicker) ? candleBase - flicker : 160;
    CRGB flame = CHSV(25, 240, val);
    fill_solid(buf, n, flame);
  }

  void renderFire(CRGB* buf, uint8_t* heat, int n) {
    for (int i = 0; i < n; i++) {
      heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / n) + 2));
    }
    for (int k = n - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }
    if (random8() < 160) {
      int y = random8(min(7, n));
      heat[y] = qadd8(heat[y], random8(160, 255));
    }
    for (int j = 0; j < n; j++) {
      buf[j] = HeatColor(heat[j]);
    }
  }

  void renderCyberwave(CRGB* buf, int n) {
    for (int i = 0; i < n; i++) {
      uint8_t p = (i * 256 / max(1, n)) + (wavePos & 0xFF);
      uint8_t s = sin8(p);
      CRGB c1 = CRGB(255, 0, 128);
      CRGB c2 = CRGB(0, 220, 255);
      buf[i] = blend(c1, c2, s);
    }
  }

  void renderStrobe(CRGB* buf, int n) {
    uint32_t now = millis();
    if (now - lastStrobeMs >= SPEED_STROBE_MS) {
      lastStrobeMs = now;
      strobeState = !strobeState;
    }
    fill_solid(buf, n, strobeState ? CRGB::White : CRGB::Black);
  }

  void renderRain(CRGB* buf, int n) {
    fadeToBlackBy(buf, n, 30);
    if (random8(10) < 3) {
      int pos = random16(n);
      buf[pos] = CRGB(160, 200, 255);
    }
  }

  void renderMeteor(CRGB* buf, int n, int masterN, int& lastPos, bool advanceColor) {
    static const uint8_t TAIL_LEN = 8;

    fadeToBlackBy(buf, n, 40);

    int totalRange = masterN + TAIL_LEN;
    int pos = (meteorTick / 3) % totalRange;

    if (advanceColor && lastPos >= 0 && pos < lastPos) {
      meteorColorIdx = (meteorColorIdx + 1) % METEOR_PALETTE_COUNT;
    }
    lastPos = pos;

    CRGB headCol = meteorPalettes[meteorColorIdx].head;
    CRGB tailCol = meteorPalettes[meteorColorIdx].tail;

    for (int j = 0; j < TAIL_LEN; j++) {
      int idx = pos - j;
      if (idx >= 0 && idx < n) {
        uint8_t blendAmt = (uint8_t)((j * 255) / (TAIL_LEN - 1));
        CRGB c = blend(headCol, tailCol, blendAmt);
        uint8_t fade = (uint8_t)(j * j * 5);
        c.fadeToBlackBy(fade);
        buf[idx] = c;
      }
    }
  }

  void renderTwinkle(CRGB* buf, int n) {
    fadeToBlackBy(buf, n, 12);
    if (random8() < 40) {
      int pos = random16(n);
      buf[pos] = CHSV(random8(), 180, 255);
    }
  }

  void renderChase(CRGB* buf, int n) {
    fadeToBlackBy(buf, n, 50);
    for (int i = 0; i < 3; i++) {
      int pos = (chasePos + (i * (n / 3))) % max(1, n);
      buf[pos] = CHSV(chaseHue + (i * 40), 220, 255);
    }
  }

  void renderParty(CRGB* buf, int n) {
    for (int i = 0; i < n; i++) {
      buf[i] = CHSV(partyHue + (i * 255 / max(1, n)), 240, 255);
    }
  }

  void renderBounce(CRGB* buf, int n) {
    fadeToBlackBy(buf, n, 40);
    if (bouncePos < n) {
      buf[bouncePos] = CHSV(bounceHue, 220, 255);
    }
  }

  void applyEffect(CRGB* buf, uint8_t* heat, int n, int masterN, int& meteorLastPos, bool meteorAdvColor, const DeviceState& s) {
    switch (s.effect) {
      case EffectType::STATIC:    renderStatic(buf, n, s);                                break;
      case EffectType::RAINBOW:   renderRainbow(buf, n);                                  break;
      case EffectType::BREATHING: renderBreathe(buf, n);                                  break;
      case EffectType::PULSE:     renderPulse(buf, n);                                    break;
      case EffectType::AURORA:    renderAurora(buf, n);                                   break;
      case EffectType::CANDLE:    renderCandle(buf, n);                                   break;
      case EffectType::FIRE:      renderFire(buf, heat, n);                               break;
      case EffectType::CYBERWAVE: renderCyberwave(buf, n);                                break;
      case EffectType::STROBE:    renderStrobe(buf, n);                                   break;
      case EffectType::RAIN:      renderRain(buf, n);                                     break;
      case EffectType::METEOR:    renderMeteor(buf, n, masterN, meteorLastPos, meteorAdvColor); break;
      case EffectType::TWINKLE:   renderTwinkle(buf, n);                                  break;
      case EffectType::CHASE:     renderChase(buf, n);                                    break;
      case EffectType::PARTY:     renderParty(buf, n);                                    break;
      case EffectType::BOUNCE:    renderBounce(buf, n);                                   break;
    }
  }

  void advancePhase(EffectType effect, int n) {
    switch (effect) {
      case EffectType::RAINBOW:
        rainbowHue++;
        break;
      case EffectType::BREATHING:
        breathPhase += 2;
        if (breathPhase == 0) {
          breathTick++;
          if (breathTick % 3 == 0) breathHue += 40;
        }
        break;
      case EffectType::PULSE:
        pulsePhase += 3;
        if (pulsePhase == 0) {
          pulseTick++;
          if (pulseTick % 2 == 0) pulseHue += 45;
        }
        break;
      case EffectType::AURORA:
        auroraPos += 2;
        break;
      case EffectType::CYBERWAVE:
        wavePos += 4;
        break;
      case EffectType::METEOR:
        meteorTick++;
        break;
      case EffectType::CHASE:
        chasePos = (chasePos + 1) % max(1, n);
        chaseHue += 2;
        break;
      case EffectType::PARTY:
        partyHue += 3;
        break;
      case EffectType::BOUNCE:
        bouncePos += bounceDir;
        if (bouncePos >= n - 1) {
          bounceDir = -1;
          bounceHue += 32;
        } else if (bouncePos <= 0) {
          bounceDir = 1;
          bounceHue += 32;
        }
        break;
      default:
        break;
    }
  }
}

namespace LEDController {

void begin() {
  FastLED.addLeds<LED_CHIPSET, LED_PIN_1, LED_COLOR_ORDER>(leds1, LED_COUNT_1);
#if DUAL_STRIP_ENABLED
  FastLED.addLeds<LED_CHIPSET, LED_PIN_2, LED_COLOR_ORDER>(leds2, LED_COUNT_2);
#endif
  FastLED.setBrightness(0);
  fill_solid(leds1, LED_COUNT_1, CRGB::Black);
  memset(heat1, 0, sizeof(heat1));
#if DUAL_STRIP_ENABLED
  fill_solid(leds2, LED_COUNT_2, CRGB::Black);
  memset(heat2, 0, sizeof(heat2));
#endif
  FastLED.clear(true);
  for (int i = 0; i < 3; i++) {
    FastLED.show();
    delay(2);
  }
}

void update(const DeviceState& state) {
  if (!state.power) {
    if (!offApplied) {
      fill_solid(leds1, LED_COUNT_1, CRGB::Black);
      memset(heat1, 0, sizeof(heat1));
#if DUAL_STRIP_ENABLED
      fill_solid(leds2, LED_COUNT_2, CRGB::Black);
      memset(heat2, 0, sizeof(heat2));
#endif
      FastLED.setBrightness(0);
      FastLED.clear(true);
      for (int i = 0; i < 3; i++) {
        FastLED.show();
        delay(2);
      }
      offApplied = true;
      transitionState      = Transition::IDLE;
      transitionBrightness = 255;
      strobeState          = false;
      Serial.println(F("[LED] Power OFF applied - all LEDs set to (0,0,0)"));
    }
    return;
  }
  offApplied = false;

  uint32_t now = millis();

  if (state.effect != activeEffect &&
      state.effect != pendingEffect &&
      transitionState == Transition::IDLE) {
    pendingEffect      = state.effect;
    transitionState    = Transition::FADE_OUT;
    lastTransitionStep = now;
    Serial.print(F("[LED] Crossfade -> "));
    Serial.println(effectToString(state.effect));
  }

  if (transitionState != Transition::IDLE && now - lastTransitionStep >= FADE_STEP_MS) {
    lastTransitionStep = now;
    if (transitionState == Transition::FADE_OUT) {
      if (transitionBrightness <= FADE_STEP_AMT) {
        transitionBrightness = 0;
        activeEffect         = pendingEffect;
        transitionState      = Transition::FADE_IN;
      } else {
        transitionBrightness -= FADE_STEP_AMT;
      }
    } else {
      if (transitionBrightness >= 255 - FADE_STEP_AMT) {
        transitionBrightness = 255;
        transitionState      = Transition::IDLE;
      } else {
        transitionBrightness += FADE_STEP_AMT;
      }
    }
  }

  uint16_t interval = getEffectIntervalMs(activeEffect);
  if (now - lastFrame < interval) {
    if (transitionState != Transition::IDLE) {
      uint8_t tb = map(clampPercent(state.brightness), 0, 100, 0, 255);
      FastLED.setBrightness(scale8(tb, transitionBrightness));
      FastLED.show();
    }
    return;
  }
  lastFrame = now;

  DeviceState rs = state;
  rs.effect       = activeEffect;

  applyEffect(leds1, heat1, LED_COUNT_1, LED_COUNT_1, meteorLastPos1, true,  rs);
#if DUAL_STRIP_ENABLED
  applyEffect(leds2, heat2, LED_COUNT_2, LED_COUNT_1, meteorLastPos2, false, rs);
#endif

  advancePhase(activeEffect, LED_COUNT_1);

  uint8_t tb = map(clampPercent(state.brightness), 0, 100, 0, 255);
  FastLED.setBrightness(scale8(tb, transitionBrightness));
  FastLED.show();
}

}
