/**
 * @file DisplayConfig.cpp
 * Implementation of LovyanGFX and LVGL driver bridge for ESP32
 */

#include "DisplayConfig.h"
#include <Arduino.h>

LGFX_RallyDash tft;

LGFX_RallyDash::LGFX_RallyDash() {
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = VSPI_HOST;     // Standard VSPI on ESP32
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000;    // 40 MHz SPI write speed
        cfg.freq_read  = 16000000;
        cfg.spi_3wire  = false;
        cfg.use_lock   = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = TFT_SCLK;
        cfg.pin_mosi = TFT_MOSI;
        cfg.pin_miso = TFT_MISO;
        cfg.pin_dc   = TFT_DC;
        _bus_instance.config(cfg);
        _panel_instance = nullptr;
    }

    {
#if defined(USE_ST7789)
        auto p = new lgfx::Panel_ST7789();
        auto cfg = p->config();
        cfg.pin_cs           = TFT_CS;
        cfg.pin_rst          = TFT_RST;
        cfg.pin_busy         = -1;
        cfg.panel_width      = SCREEN_HEIGHT; // 240
        cfg.panel_height     = SCREEN_WIDTH;  // 320
        cfg.offset_x         = 0;
        cfg.offset_y         = 0;
        cfg.offset_rotation  = 1;             // Landscape
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits  = 1;
        cfg.readable         = false;
        cfg.invert           = false;
        cfg.rgb_order        = false;
        cfg.dlen_16bit       = false;
        cfg.bus_shared       = false;
        p->config(cfg);
        _panel_instance = p;
#else
        // Default: ILI9341
        auto p = new lgfx::Panel_ILI9341();
        auto cfg = p->config();
        cfg.pin_cs           = TFT_CS;
        cfg.pin_rst          = TFT_RST;
        cfg.pin_busy         = -1;
        cfg.panel_width      = SCREEN_HEIGHT; // 240
        cfg.panel_height     = SCREEN_WIDTH;  // 320
        cfg.offset_x         = 0;
        cfg.offset_y         = 0;
        cfg.offset_rotation  = 1;             // Landscape
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits  = 1;
        cfg.readable         = false;
        cfg.invert           = false;
        cfg.rgb_order        = false;
        cfg.dlen_16bit       = false;
        cfg.bus_shared       = false;
        p->config(cfg);
        _panel_instance = p;
#endif
        _panel_instance->setBus(&_bus_instance);
    }

    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = TFT_BL;
        cfg.invert = false;
        cfg.freq   = 44100;
        cfg.pwm_channel = 7;
        _light_instance.config(cfg);
        _panel_instance->setLight(&_light_instance);
    }

    setPanel(_panel_instance);
}

/* LVGL display buffers */
static const uint32_t BUFFER_LINES = 24;
static lv_color_t buf1[SCREEN_WIDTH * BUFFER_LINES];
static lv_color_t buf2[SCREEN_WIDTH * BUFFER_LINES];
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;

/* LVGL display flush callback with LovyanGFX DMA */
static void disp_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.writePixels((lgfx::rgb565_t *)&color_p->full, w * h);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void initDisplay() {
    // Initialize LovyanGFX hardware
    tft.init();
    tft.setRotation(1); // Landscape 320x240
    tft.setBrightness(220); // 86% brightness
    tft.fillScreen(TFT_BLACK);

    // Initialize LVGL core
    lv_init();

    // Setup dual draw buffer for partial line rendering
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, SCREEN_WIDTH * BUFFER_LINES);

    // Register display driver with LVGL
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
}

void setDisplayBrightness(uint8_t brightness) {
    tft.setBrightness(brightness);
}
