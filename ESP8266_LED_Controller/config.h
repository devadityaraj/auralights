#ifndef CONFIG_H
#define CONFIG_H

#define FIRMWARE_VERSION "1.0.0"

#define LED_PIN_1           D2 //Connect Din of strip 1 to this pin
#define LED_COUNT_1          60 //No of leds in strip 1
#define LED_CHIPSET          WS2812B //LED
#define LED_COLOR_ORDER      GRB

#define DUAL_STRIP_ENABLED  true //change to false if using one strip only
#define LED_PIN_2           D3 //Connect Din of strip 2 to this pin
#define LED_COUNT_2          24 //no of leds in strip 2

#define SPEED_STATIC_MS         100
#define SPEED_RAINBOW_MS         15
#define SPEED_BREATHE_MS         50
#define SPEED_PULSE_MS           40
#define SPEED_AURORA_MS          70
#define SPEED_CANDLE_MS          100
#define SPEED_FIRE_MS            50
#define SPEED_CYBERWAVE_MS       80
#define SPEED_STROBE_MS          60
#define SPEED_RAIN_MS            50
#define SPEED_METEOR_MS          50
#define SPEED_TWINKLE_MS         75
#define SPEED_CHASE_MS           50
#define SPEED_PARTY_MS           50
#define SPEED_BOUNCE_MS          22

#define WIFI_RECONNECT_INTERVAL_MS      5000UL
#define STREAM_RECONNECT_INTERVAL_MS    5000UL
#define WIFI_BOOT_TIMEOUT_MS           15000UL

#endif
