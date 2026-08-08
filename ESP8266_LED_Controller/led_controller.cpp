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

  uint16_t meteorTick = 0;
  uint8_t  chasePos   = 0;
  uint8_t  chaseHue   = 0;
  uint8_t  partyHue   = 0;
  uint8_t  bouncePos  = 0;
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
    candleBase = (candleBase * 3 + random8(170, 255)) / 4;
    CRGB flameCol = blend(CRGB(255, 60, 0), CRGB(255, 190, 30), candleBase);
    fill_solid(buf, n, flameCol);
    for (int i = 0; i < n; i++) {
      if (random8() < 25) buf[i] = blend(buf[i], CRGB(255, 230, 90), random8(20, 60));
    }
  }

  void renderFire(CRGB* buf, uint8_t* heat, int n) {
    uint8_t sparking = 120;
    for (int i = 0; i < n; i++)
      heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / n) + 2));
    for (int k = n - 1; k >= 2; k--)
      heat[k] = (heat[k-1] + heat[k-2] + heat[k-2]) / 3;
    if (random8() < sparking) {
      int y = random8(7);
      if (y < n) heat[y] = qadd8(heat[y], random8(160, 255));
    }
    for (int j = 0; j < n; j++) buf[j] = HeatColor(heat[j]);
  }

  void renderCyberwave(CRGB* buf, int n) {
    static const CRGB cyberCyan = CRGB(0, 235, 255);
    static const CRGB hotPink   = CRGB(255, 10, 150);
    for (int i = 0; i < n; i++) {
      uint8_t blendAmt = sin8((i * 255 / max(1, n)) + wavePos);
      buf[i] = blend(cyberCyan, hotPink, blendAmt);
    }
  }

  void renderStrobe(CRGB* buf, int n) {
    uint32_t now = millis();
    if (now - lastStrobeMs >= SPEED_STROBE_MS) {
      lastStrobeMs = now;
      strobeState = !strobeState;
    }
    fill_solid(buf, n, strobeState ? CRGB(240, 250, 255) : CRGB::Black);
  }

  void renderRain(CRGB* buf, int n) {
    for (int i = n - 1; i > 0; i--) {
      buf[i] = buf[i - 1];
      buf[i].fadeToBlackBy(35);
    }
    if (random8() < 35) {
      buf[0] = (random8() < 50) ? CRGB(110, 210, 255) : CRGB(40, 150, 255);
    } else {
      buf[0] = CRGB::Black;
    }
  }

  void renderMeteor(CRGB* buf, int n) {
    for (int i = 0; i < n; i++) buf[i].fadeToBlackBy(45);
    int head = (meteorTick / 2) % (2 * n);
    if (head >= n) head = 2 * n - 1 - head;
    for (int t = 0; t < 7 && head - t >= 0; t++) {
      uint8_t amt = 255 - t * 38;
      buf[head - t] = blend(CRGB(10, 110, 255), CRGB(235, 250, 255), amt);
    }
  }

  void renderTwinkle(CRGB* buf, int n) {
    for (int i = 0; i < n; i++) buf[i].fadeToBlackBy(16);
    if (random8() < 40) {
      int idx = random8(n);
      buf[idx] = (random8() < 60) ? CRGB(255, 235, 180) : CRGB(180, 225, 255);
    }
  }

  void renderChase(CRGB* buf, int n) {
    for (int i = 0; i < n; i++) buf[i].fadeToBlackBy(55);
    int pos = chasePos % n;
    buf[pos] = CHSV(chaseHue, 255, 255);
    if (pos > 0) buf[pos - 1] = CHSV(chaseHue - 8, 240, 160);
    if (pos > 1) buf[pos - 2] = CHSV(chaseHue - 16, 240, 70);
  }

  void renderParty(CRGB* buf, int n) {
    for (int i = 0; i < n; i++) buf[i].fadeToBlackBy(30);
    if (random8() < 120) buf[random8(n)] = CHSV(partyHue, 255, 255);
    if (random8() < 120) buf[random8(n)] = CHSV(partyHue + 85, 255, 255);
  }

  void renderBounce(CRGB* buf, int n) {
    for (int i = 0; i < n; i++) buf[i].fadeToBlackBy(45);
    int pos = bouncePos;
    buf[pos] = CHSV(bounceHue, 255, 255);
    if (bounceDir > 0 && pos > 0) buf[pos - 1] = CHSV(bounceHue - 8, 230, 140);
    if (bounceDir < 0 && pos < n - 1) buf[pos + 1] = CHSV(bounceHue - 8, 230, 140);
  }

  void applyEffect(CRGB* buf, uint8_t* heat, int n, const DeviceState& s) {
    switch (s.effect) {
      case EffectType::STATIC:    renderStatic(buf, n, s);        break;
      case EffectType::RAINBOW:   renderRainbow(buf, n);          break;
      case EffectType::BREATHING: renderBreathe(buf, n);          break;
      case EffectType::PULSE:     renderPulse(buf, n);            break;
      case EffectType::AURORA:    renderAurora(buf, n);           break;
      case EffectType::CANDLE:    renderCandle(buf, n);           break;
      case EffectType::FIRE:      renderFire(buf, heat, n);       break;
      case EffectType::CYBERWAVE: renderCyberwave(buf, n);        break;
      case EffectType::STROBE:    renderStrobe(buf, n);           break;
      case EffectType::RAIN:      renderRain(buf, n);             break;
      case EffectType::METEOR:    renderMeteor(buf, n);           break;
      case EffectType::TWINKLE:   renderTwinkle(buf, n);          break;
      case EffectType::CHASE:     renderChase(buf, n);            break;
      case EffectType::PARTY:     renderParty(buf, n);            break;
      case EffectType::BOUNCE:    renderBounce(buf, n);           break;
    }
  }

  void advancePhase(EffectType e) {
    switch (e) {
      case EffectType::RAINBOW:
        rainbowHue++;
        break;
      case EffectType::AURORA:
        auroraPos += 1;
        break;
      case EffectType::BREATHING:
        breathPhase += 1;
        if (++breathTick >= 4) { breathTick = 0; breathHue++; }
        break;
      case EffectType::PULSE:
        pulsePhase += 3;
        if (++pulseTick >= 3) { pulseTick = 0; pulseHue++; }
        break;
      case EffectType::CYBERWAVE:
        wavePos += 2;
        break;
      case EffectType::METEOR:
        meteorTick++;
        break;
      case EffectType::CHASE:
        chasePos++;
        chaseHue++;
        break;
      case EffectType::PARTY:
        partyHue += 2;
        break;
      case EffectType::BOUNCE:
        bounceHue++;
        bouncePos += bounceDir;
        if (bounceDir > 0 && bouncePos >= LED_COUNT_1 - 1) bounceDir = -1;
        if (bounceDir < 0 && bouncePos == 0)               bounceDir =  1;
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
  memset(heat2, 0, sizeof(heat2));
#endif
  memset(heat1, 0, sizeof(heat1));
  FastLED.clear(true);
  FastLED.show();
}

void update(const DeviceState& state) {
  if (!state.power) {
    if (!offApplied) {
      fill_solid(leds1, LED_COUNT_1, CRGB::Black);
#if DUAL_STRIP_ENABLED
      fill_solid(leds2, LED_COUNT_2, CRGB::Black);
#endif
      FastLED.setBrightness(0);
      FastLED.show();
      delay(2);
      FastLED.show();
      offApplied = true;
    }
    transitionState      = Transition::IDLE;
    transitionBrightness = 255;
    activeEffect         = state.effect;
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

  applyEffect(leds1, heat1, LED_COUNT_1, rs);
#if DUAL_STRIP_ENABLED
  applyEffect(leds2, heat2, LED_COUNT_2, rs);
#endif

  advancePhase(activeEffect);

  uint8_t tb = map(clampPercent(state.brightness), 0, 100, 0, 255);
  FastLED.setBrightness(scale8(tb, transitionBrightness));
  FastLED.show();
}

}
