#ifndef CONFIG_H
#define CONFIG_H

#define FIRMWARE_VERSION "1.0.0"

#define LED_PIN_1           D2
#define LED_COUNT_1          60 //No of leds
#define LED_CHIPSET          WS2812B
#define LED_COLOR_ORDER      GRB

#define DUAL_STRIP_ENABLED  true //change to true if using 2 strips
#define LED_PIN_2           D3
#define LED_COUNT_2          24 //no of leds

#define SPEED_STATIC_MS         100
#define SPEED_RAINBOW_MS         15
#define SPEED_BREATHE_MS         50 //20
#define SPEED_PULSE_MS           40 //18
#define SPEED_AURORA_MS          70 //25  //ch from 50
#define SPEED_CANDLE_MS          100 //22  //50 to 100
#define SPEED_FIRE_MS            50 //20
#define SPEED_CYBERWAVE_MS       80
#define SPEED_STROBE_MS          60
#define SPEED_RAIN_MS            50 //35
#define SPEED_METEOR_MS          50 //20
#define SPEED_TWINKLE_MS         75 //25
#define SPEED_CHASE_MS           50  //25
#define SPEED_PARTY_MS           50 //30
#define SPEED_BOUNCE_MS          22

#define WIFI_RECONNECT_INTERVAL_MS      5000UL
#define STREAM_RECONNECT_INTERVAL_MS    5000UL
#define WIFI_BOOT_TIMEOUT_MS           15000UL

#endif
