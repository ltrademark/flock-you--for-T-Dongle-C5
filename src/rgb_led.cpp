// Compile for either LilyGO dongle (both carry a single APA102 RGB LED)
#if defined(BOARD_T_DONGLE_S3) || defined(BOARD_T_DONGLE_C5)

#include "rgb_led.h"

// ----------------------------------------------------------------------------
// Board-specific LED backend
//
// Both dongles use a single APA102 (data + clock) RGB LED, but on different
// pins and via different libraries:
//   * T-Dongle S3 -> FastLED (has an APA102 controller for the Xtensa core)
//   * T-Dongle C5 -> Pololu APA102 library (bit-banged; FastLED has no ESP32-C5
//     pin map yet). Vendored in lib/APA102/.
//
// The rest of this file drives the LED through three primitives so the
// animation logic below is identical on both boards:
//   led_set_rgb(r,g,b)      -- set the pending color
//   led_set_brightness(b)   -- set global brightness (0-255)
//   led_show()              -- push the pending color/brightness to the LED
// ----------------------------------------------------------------------------

#if defined(BOARD_T_DONGLE_C5)

#include <APA102.h>
using namespace Pololu;

// T-Dongle C5 APA102 pins (from LilyGO pin_config.h): DI=5 (data), CI=4 (clock).
#define LED_DATA_PIN   5
#define LED_CLOCK_PIN  4

// APA102 template takes <dataPin, clockPin>.
static APA102<LED_DATA_PIN, LED_CLOCK_PIN> ledStrip;

static uint8_t cur_r = 0, cur_g = 0, cur_b = 0;
static uint8_t cur_brightness = 50;  // 0-255 (matches FastLED semantics)

static inline void led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    cur_r = r; cur_g = g; cur_b = b;
}
static inline void led_set_brightness(uint8_t b) {
    cur_brightness = b;
}
static inline void led_show() {
    // APA102 has a separate 5-bit (0-31) brightness register; scale 0-255 into it.
    uint8_t b5 = (uint16_t)cur_brightness * 31 / 255;
    if (b5 == 0 && (cur_r || cur_g || cur_b)) b5 = 1;  // keep a lit color visible
    ledStrip.startFrame();
    ledStrip.sendColor(cur_r, cur_g, cur_b, b5);
    ledStrip.endFrame(1);
}

static inline void led_backend_init() {
    // The Pololu APA102 library bit-bangs with digitalWrite but does NOT set the
    // pin direction. On the ESP32-C5 core, digitalWrite() on a pin that was never
    // configured as OUTPUT is rejected ("IO N is not set as GPIO"), so the LED
    // stays dark. Configure both pins here before the first frame.
    pinMode(LED_DATA_PIN, OUTPUT);
    pinMode(LED_CLOCK_PIN, OUTPUT);
    led_set_brightness(50);
    led_set_rgb(0, 0, 0);
    led_show();
}

#else  // BOARD_T_DONGLE_S3

#include <FastLED.h>

// T-Dongle S3 APA102 LED configuration
#define DATA_PIN    40
#define CLOCK_PIN   39
#define NUM_LEDS    1
#define LED_TYPE    APA102
#define COLOR_ORDER BGR

static CRGB leds[NUM_LEDS];

static inline void led_set_rgb(uint8_t r, uint8_t g, uint8_t b) { leds[0] = CRGB(r, g, b); }
static inline void led_set_brightness(uint8_t b) { FastLED.setBrightness(b); }
static inline void led_show() { FastLED.show(); }

static inline void led_backend_init() {
    FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
}

#endif

// ----------------------------------------------------------------------------
// Animation state (shared by both boards)
// ----------------------------------------------------------------------------

static bool detection_active = false;
static bool heartbeat_active = false;
static unsigned long last_update = 0;
static uint8_t flash_count = 0;
static bool flash_red = true;

void rgb_init() {
    led_backend_init();
    rgb_off();
}

void rgb_boot_sequence() {
    // Quick RGB cycle to show it's working
    led_set_brightness(100);

    // Red
    led_set_rgb(255, 0, 0);
    led_show();
    delay(100);

    // Blue
    led_set_rgb(0, 0, 255);
    led_show();
    delay(100);

    // Cyan
    led_set_rgb(0, 255, 255);
    led_show();
    delay(100);

    // Off
    led_set_brightness(50);
    rgb_off();
}

void rgb_detection_flash() {
    // Start rapid red/blue strobe (replaces beeper)
    detection_active = true;
    heartbeat_active = false;
    flash_count = 0;
    flash_red = true;
    last_update = millis();

    // Immediate red flash
    led_set_brightness(255);
    led_set_rgb(255, 0, 0);
    led_show();
}

void rgb_heartbeat_pulse() {
    if (!detection_active) {
        heartbeat_active = true;
    }
}

void rgb_set_color(uint8_t r, uint8_t g, uint8_t b) {
    detection_active = false;
    heartbeat_active = false;
    led_set_rgb(r, g, b);
    led_show();
}

void rgb_off() {
    detection_active = false;
    heartbeat_active = false;
    led_set_rgb(0, 0, 0);
    led_show();
}

void rgb_update() {
    unsigned long now = millis();

    if (detection_active) {
        // Fast red/blue strobe - like police lights (replaces beeper)
        if (now - last_update >= 50) {  // 50ms = very fast strobe
            flash_red = !flash_red;

            if (flash_red) {
                led_set_rgb(255, 0, 0);   // Red
            } else {
                led_set_rgb(0, 0, 255);   // Blue
            }
            led_set_brightness(255);
            led_show();
            last_update = now;
            flash_count++;

            // Stop strobing after ~3 seconds (60 flashes), go to heartbeat
            if (flash_count > 60) {
                detection_active = false;
                heartbeat_active = true;
                led_set_brightness(50);
            }
        }
    } else if (heartbeat_active) {
        // Slow orange pulse for heartbeat (device still in range)
        if (now - last_update >= 500) {
            static bool pulse_on = false;
            pulse_on = !pulse_on;

            if (pulse_on) {
                led_set_rgb(255, 100, 0);  // Orange
                led_set_brightness(80);
            } else {
                led_set_rgb(255, 50, 0);   // Dim orange
                led_set_brightness(30);
            }
            led_show();
            last_update = now;
        }
    }
}

#endif // BOARD_T_DONGLE_S3 || BOARD_T_DONGLE_C5
