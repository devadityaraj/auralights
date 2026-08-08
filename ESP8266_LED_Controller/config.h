#ifndef CONFIG_H
#define CONFIG_H

#define FIRMWARE_VERSION "1.0.0"

#define LED_PIN_1           D2
#define LED_COUNT_1          24 //No of leds
#define LED_CHIPSET          WS2812B
#define LED_COLOR_ORDER      GRB

#define DUAL_STRIP_ENABLED  false //change to true if using 2 strips
#define LED_PIN_2           D3
#define LED_COUNT_2          30 //no of leds

#define SPEED_STATIC_MS         100
#define SPEED_RAINBOW_MS         15
#define SPEED_BREATHE_MS         20
#define SPEED_PULSE_MS           18
#define SPEED_AURORA_MS          25
#define SPEED_CANDLE_MS          22
#define SPEED_FIRE_MS            20
#define SPEED_CYBERWAVE_MS       20
#define SPEED_STROBE_MS          60
#define SPEED_RAIN_MS            35
#define SPEED_METEOR_MS          20
#define SPEED_TWINKLE_MS         25
#define SPEED_CHASE_MS           25
#define SPEED_PARTY_MS           30
#define SPEED_BOUNCE_MS          22

#define WIFI_RECONNECT_INTERVAL_MS      5000UL
#define STREAM_RECONNECT_INTERVAL_MS    5000UL
#define WIFI_BOOT_TIMEOUT_MS           15000UL

#endif
