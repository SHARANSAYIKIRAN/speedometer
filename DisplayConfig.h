/**
 * @file DisplayConfig.h
 * Display hardware abstraction and LovyanGFX configuration
 * Target: ESP32-WROOM-32 with 2.8" 320x240 SPI TFT (ILI9341 / ST7789)
 */

#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lvgl.h>

/* Display Dimensions */
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

/* Standard SPI Pin Configuration for ESP32-WROOM-32 */
#ifndef TFT_MOSI
#define TFT_MOSI 23
#endif

#ifndef TFT_MISO
#define TFT_MISO 19
#endif

#ifndef TFT_SCLK
#define TFT_SCLK 18
#endif

#ifndef TFT_CS
#define TFT_CS   5
#endif

#ifndef TFT_DC
#define TFT_DC   2
#endif

#ifndef TFT_RST
#define TFT_RST  4
#endif

#ifndef TFT_BL
#define TFT_BL   21  /* Backlight PWM Pin */
#endif

/* Display Controller Selection: ILI9341 (default) or ST7789 */
#define USE_ILI9341 1
//#define USE_ST7789  1

class LGFX_RallyDash : public lgfx::LGFX_Device {
    lgfx::Panel_Device* _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;

public:
    LGFX_RallyDash();
};

extern LGFX_RallyDash tft;

/**
 * Initializes the display hardware, backlight PWM, and LVGL display driver buffers.
 */
void initDisplay();

/**
 * Sets display brightness (0 - 255)
 */
void setDisplayBrightness(uint8_t brightness);

#endif /* DISPLAY_CONFIG_H */
