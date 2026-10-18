/**
 * @file DashboardUI.cpp
 * Complete RallyDash UI Implementation
 * Reproduces the rally cluster reference screenshot with exact layout & styling
 */

#include "DashboardUI.h"
#include <stdio.h>

DashboardUI g_dashboardUI;

DashboardUI::DashboardUI()
    : _screen(nullptr),
      _gearLabel(nullptr), _warnCont(nullptr),
      _lblCel(nullptr), _lblOil(nullptr), _lblClt(nullptr),
      _lblGas(nullptr), _lblBat(nullptr), _lblAfr(nullptr),
      _shiftCont(nullptr),
      _speedLabel(nullptr), _speedUnitLabel(nullptr),
      _rpmNumLabel(nullptr), _rpmUnitLabel(nullptr),
      _panelBoost(nullptr), _valBoost(nullptr), _unitBoost(nullptr),
      _panelAfr(nullptr), _valAfr(nullptr), _subAfr(nullptr),
      _panelRuntime(nullptr), _valRuntime(nullptr), _subRuntime(nullptr),
      _cardClt(nullptr), _valClt(nullptr),
      _cardOil(nullptr), _valOil(nullptr),
      _cardIat(nullptr), _valIat(nullptr),
      _cardFuel(nullptr), _valFuel(nullptr),
      _cardBat(nullptr), _valBat(nullptr),
      _cardEcu(nullptr), _badgeEcu(nullptr), _badgeEcuLabel(nullptr), _lblEcu(nullptr),
      _footerBar(nullptr), _footerLeft(nullptr), _footerRight(nullptr) {
    for (int i = 0; i < 10; i++) {
        _shiftLeds[i] = nullptr;
    }
}

lv_obj_t* DashboardUI::createCard(lv_obj_t* parent, int x, int y, int w, int h) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_bg_color(card, lv_color_make(0x09, 0x1E, 0x3A), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_make(0x16, 0x36, 0x5C), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 4, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

void DashboardUI::setWarningLabel(lv_obj_t* lbl, bool active, lv_color_t activeColor) {
    if (!lbl) return;
    if (active) {
        lv_obj_set_style_text_color(lbl, activeColor, 0);
    } else {
        lv_obj_set_style_text_color(lbl, lv_color_make(0x38, 0x6E, 0x94), 0); // Muted steel cyan
    }
}

void DashboardUI::updateShiftLights(uint16_t rpm) {
    static const uint16_t thresholds[10] = {
        3200, 4000, 4700, 5300,  // Green 0..3
        5700, 6000, 6300,        // Yellow 4..6
        6600, 6900, 7200         // Red 7..9
    };

    static const lv_color_t activeColors[10] = {
        lv_color_make(0x00, 0xE6, 0x76), lv_color_make(0x00, 0xE6, 0x76),
        lv_color_make(0x00, 0xE6, 0x76), lv_color_make(0x00, 0xE6, 0x76),
        lv_color_make(0xFF, 0xD6, 0x00), lv_color_make(0xFF, 0xD6, 0x00),
        lv_color_make(0xFF, 0xD6, 0x00),
        lv_color_make(0xFF, 0x17, 0x44), lv_color_make(0xFF, 0x17, 0x44),
        lv_color_make(0xFF, 0x17, 0x44)
    };

    static const lv_color_t inactiveColors[10] = {
        lv_color_make(0x08, 0x2A, 0x14), lv_color_make(0x08, 0x2A, 0x14),
        lv_color_make(0x08, 0x2A, 0x14), lv_color_make(0x08, 0x2A, 0x14),
        lv_color_make(0x32, 0x2A, 0x06), lv_color_make(0x32, 0x2A, 0x06),
        lv_color_make(0x32, 0x2A, 0x06),
        lv_color_make(0x35, 0x08, 0x0D), lv_color_make(0x35, 0x08, 0x0D),
        lv_color_make(0x35, 0x08, 0x0D)
    };

    for (int i = 0; i < 10; i++) {
        if (!_shiftLeds[i]) continue;
        if (rpm >= thresholds[i]) {
            lv_obj_set_style_bg_color(_shiftLeds[i], activeColors[i], 0);
        } else {
            lv_obj_set_style_bg_color(_shiftLeds[i], inactiveColors[i], 0);
        }
    }
}

void DashboardUI::init() {
    _screen = lv_scr_act();
    lv_obj_clean(_screen);

    /* 1. Deep Electric/Cobalt Racing Blue Gradient Background */
    lv_obj_set_style_bg_color(_screen, lv_color_make(0x0C, 0x36, 0x6E), 0);
    lv_obj_set_style_bg_grad_color(_screen, lv_color_make(0x07, 0x1C, 0x3D), 0);
    lv_obj_set_style_bg_grad_dir(_screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_pad_all(_screen, 0, 0);
    lv_obj_clear_flag(_screen, LV_OBJ_FLAG_SCROLLABLE);

    /*========================================================================
     * 2. TOP-LEFT: LARGE GEAR INDICATOR & 2-COLUMN WARNINGS
     *========================================================================*/

    /* Large Cyan Gear Indicator (Floating, no box border, exactly like reference) */
    _gearLabel = lv_label_create(_screen);
    lv_label_set_text(_gearLabel, "N");
    lv_obj_set_style_text_font(_gearLabel, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(_gearLabel, lv_color_make(0x00, 0xE5, 0xFF), 0); // Neon Cyan
    lv_obj_set_pos(_gearLabel, 10, 4);

    /* Warning Indicators (2 Columns x 3 Rows) */
    _warnCont = lv_obj_create(_screen);
    lv_obj_set_size(_warnCont, 60, 42);
    lv_obj_set_pos(_warnCont, 48, 6);
    lv_obj_set_style_bg_opa(_warnCont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_warnCont, 0, 0);
    lv_obj_set_style_pad_all(_warnCont, 0, 0);
    lv_obj_clear_flag(_warnCont, LV_OBJ_FLAG_SCROLLABLE);

    // Column 1 (x = 0): CEL, CLT, BAT
    _lblCel = lv_label_create(_warnCont);
    lv_label_set_text(_lblCel, "CEL");
    lv_obj_set_style_text_font(_lblCel, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(_lblCel, 2, 0);

    _lblClt = lv_label_create(_warnCont);
    lv_label_set_text(_lblClt, "CLT");
    lv_obj_set_style_text_font(_lblClt, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(_lblClt, 2, 14);

    _lblBat = lv_label_create(_warnCont);
    lv_label_set_text(_lblBat, "BAT");
    lv_obj_set_style_text_font(_lblBat, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(_lblBat, 2, 28);

    // Column 2 (x = 28): OIL, GAS, AFR
    _lblOil = lv_label_create(_warnCont);
    lv_label_set_text(_lblOil, "OIL");
    lv_obj_set_style_text_font(_lblOil, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(_lblOil, 30, 0);

    _lblGas = lv_label_create(_warnCont);
    lv_label_set_text(_lblGas, "GAS");
    lv_obj_set_style_text_font(_lblGas, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(_lblGas, 30, 14);

    _lblAfr = lv_label_create(_warnCont);
    lv_label_set_text(_lblAfr, "AFR");
    lv_obj_set_style_text_font(_lblAfr, &lv_font_montserrat_10, 0);
    lv_obj_set_pos(_lblAfr, 30, 28);

    setWarningLabel(_lblCel, false, lv_color_make(0xFF, 0x22, 0x33));
    setWarningLabel(_lblOil, false, lv_color_make(0xFF, 0x22, 0x33));
    setWarningLabel(_lblClt, false, lv_color_make(0xFF, 0x22, 0x33));
    setWarningLabel(_lblGas, false, lv_color_make(0xFF, 0xAA, 0x00));
    setWarningLabel(_lblBat, false, lv_color_make(0xFF, 0x22, 0x33));
    setWarningLabel(_lblAfr, false, lv_color_make(0xFF, 0x22, 0x33));

    /*========================================================================
     * 3. TOP-CENTER: SHIFT LIGHTS ARRAY (10 LEDs)
     *========================================================================*/
    _shiftCont = lv_obj_create(_screen);
    lv_obj_set_size(_shiftCont, 80, 8);
    lv_obj_set_pos(_shiftCont, 138, 3);
    lv_obj_set_style_bg_opa(_shiftCont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_shiftCont, 0, 0);
    lv_obj_set_style_pad_all(_shiftCont, 0, 0);
    lv_obj_clear_flag(_shiftCont, LV_OBJ_FLAG_SCROLLABLE);

    const int ledOffsets[10] = { 0, 7, 14, 21, 30, 37, 44, 53, 60, 67 };
    for (int i = 0; i < 10; i++) {
        _shiftLeds[i] = lv_obj_create(_shiftCont);
        lv_obj_set_size(_shiftLeds[i], 5, 3);
        lv_obj_set_pos(_shiftLeds[i], ledOffsets[i], 2);
        lv_obj_set_style_radius(_shiftLeds[i], 1, 0);
        lv_obj_set_style_border_width(_shiftLeds[i], 0, 0);
        lv_obj_set_style_bg_opa(_shiftLeds[i], LV_OPA_COVER, 0);
        lv_obj_clear_flag(_shiftLeds[i], LV_OBJ_FLAG_SCROLLABLE);
    }
    updateShiftLights(0);

    /*========================================================================
     * 4. TOP-RIGHT: SPEED & RPM NUMERICAL VALUES
     *========================================================================*/

    /* Digital Speed Value (Crisp Bold White) */
    _speedLabel = lv_label_create(_screen);
    lv_label_set_text(_speedLabel, "0");
    lv_obj_set_style_text_font(_speedLabel, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(_speedLabel, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_set_pos(_speedLabel, 162, 6);

    /* Speed Unit "km/h" in Warm Amber/Yellow directly under speed */
    _speedUnitLabel = lv_label_create(_screen);
    lv_label_set_text(_speedUnitLabel, "km/h");
    lv_obj_set_style_text_font(_speedUnitLabel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(_speedUnitLabel, lv_color_make(0xFF, 0xC1, 0x07), 0); // Amber Yellow
    lv_obj_set_pos(_speedUnitLabel, 158, 38);

    /* Digital RPM Value (Crisp Bold White) */
    _rpmNumLabel = lv_label_create(_screen);
    lv_label_set_text(_rpmNumLabel, "1542");
    lv_obj_set_style_text_font(_rpmNumLabel, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(_rpmNumLabel, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_set_pos(_rpmNumLabel, 215, 6);

    /* RPM Unit "RPM" in Warm Amber/Yellow under RPM value */
    _rpmUnitLabel = lv_label_create(_screen);
    lv_label_set_text(_rpmUnitLabel, "RPM");
    lv_obj_set_style_text_font(_rpmUnitLabel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(_rpmUnitLabel, lv_color_make(0xFF, 0xC1, 0x07), 0); // Amber Yellow
    lv_obj_set_pos(_rpmUnitLabel, 278, 38);

    /*========================================================================
     * 5. SWEPT RALLY TACHOMETER GAUGE
     *========================================================================*/
    _tachometer.create(_screen, 0, 8000, 6000, 5500);

    /*========================================================================
     * 6. THREE CENTRAL METRIC PANELS (BOOST PSI, AFR, RUNTIME)
     *========================================================================*/

    /* Panel 1: BOOST PSI (x: 86, y: 114, w: 72, h: 42) */
    _panelBoost = createCard(_screen, 86, 114, 72, 42);

    _valBoost = lv_label_create(_panelBoost);
    lv_label_set_text(_valBoost, "-7.1");
    lv_obj_set_style_text_font(_valBoost, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(_valBoost, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_valBoost, LV_ALIGN_TOP_MID, 0, 2);

    _unitBoost = lv_label_create(_panelBoost);
    lv_label_set_text(_unitBoost, "BOOST PSI");
    lv_obj_set_style_text_font(_unitBoost, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(_unitBoost, lv_color_make(0x00, 0xE5, 0xFF), 0); // Cyan
    lv_obj_align(_unitBoost, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* Panel 2: AFR (x: 163, y: 114, w: 72, h: 42) */
    _panelAfr = createCard(_screen, 163, 114, 72, 42);

    _valAfr = lv_label_create(_panelAfr);
    lv_label_set_text(_valAfr, "13.9");
    lv_obj_set_style_text_font(_valAfr, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(_valAfr, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_valAfr, LV_ALIGN_TOP_MID, 0, 2);

    _subAfr = lv_label_create(_panelAfr);
    lv_label_set_text(_subAfr, "AFR");
    lv_obj_set_style_text_font(_subAfr, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(_subAfr, lv_color_make(0x00, 0xE5, 0xFF), 0);
    lv_obj_align(_subAfr, LV_ALIGN_BOTTOM_MID, 0, -2);

    /* Panel 3: RUN TIME (x: 240, y: 114, w: 72, h: 42) */
    _panelRuntime = createCard(_screen, 240, 114, 72, 42);

    _valRuntime = lv_label_create(_panelRuntime);
    lv_label_set_text(_valRuntime, "0:01");
    lv_obj_set_style_text_font(_valRuntime, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(_valRuntime, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_valRuntime, LV_ALIGN_TOP_MID, 0, 2);

    _subRuntime = lv_label_create(_panelRuntime);
    lv_label_set_text(_subRuntime, "RUN TIME");
    lv_obj_set_style_text_font(_subRuntime, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(_subRuntime, lv_color_make(0x00, 0xE5, 0xFF), 0);
    lv_obj_align(_subRuntime, LV_ALIGN_BOTTOM_MID, 0, -2);

    /*========================================================================
     * 7. BOTTOM ROW OF SIX METRIC CARDS (y: 162, h: 48)
     *========================================================================*/

    /* Card 0: COOL (Coolant Temp, x: 6) */
    _cardClt = createCard(_screen, 6, 162, 48, 48);
    _valClt = lv_label_create(_cardClt);
    lv_label_set_text(_valClt, "44");
    lv_obj_set_style_text_font(_valClt, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(_valClt, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_valClt, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* lblClt = lv_label_create(_cardClt);
    lv_label_set_text(lblClt, "COOL");
    lv_obj_set_style_text_font(lblClt, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lblClt, lv_color_make(0x00, 0xE5, 0xFF), 0);
    lv_obj_align(lblClt, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Card 1: OIL T (Oil Temp, x: 58) */
    _cardOil = createCard(_screen, 58, 162, 48, 48);
    _valOil = lv_label_create(_cardOil);
    lv_label_set_text(_valOil, "31");
    lv_obj_set_style_text_font(_valOil, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(_valOil, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_valOil, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* lblOil = lv_label_create(_cardOil);
    lv_label_set_text(lblOil, "OIL T");
    lv_obj_set_style_text_font(lblOil, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lblOil, lv_color_make(0x00, 0xE5, 0xFF), 0);
    lv_obj_align(lblOil, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Card 2: INTAKE (Intake Air Temp, x: 110) */
    _cardIat = createCard(_screen, 110, 162, 48, 48);
    _valIat = lv_label_create(_cardIat);
    lv_label_set_text(_valIat, "31");
    lv_obj_set_style_text_font(_valIat, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(_valIat, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_valIat, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* lblIat = lv_label_create(_cardIat);
    lv_label_set_text(lblIat, "INTAKE");
    lv_obj_set_style_text_font(lblIat, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lblIat, lv_color_make(0x00, 0xE5, 0xFF), 0);
    lv_obj_align(lblIat, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Card 3: FUEL % (Fuel Tank Level, x: 162) */
    _cardFuel = createCard(_screen, 162, 162, 48, 48);
    _valFuel = lv_label_create(_cardFuel);
    lv_label_set_text(_valFuel, "39");
    lv_obj_set_style_text_font(_valFuel, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(_valFuel, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_valFuel, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t* lblFuel = lv_label_create(_cardFuel);
    lv_label_set_text(lblFuel, "FUEL %");
    lv_obj_set_style_text_font(lblFuel, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lblFuel, lv_color_make(0x00, 0xE5, 0xFF), 0);
    lv_obj_align(lblFuel, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Card 4: BATT V (Battery Voltage, x: 214) */
    _cardBat = createCard(_screen, 214, 162, 48, 48);
    _valBat = lv_label_create(_cardBat);
    lv_label_set_text(_valBat, "14.2");
    lv_obj_set_style_text_font(_valBat, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(_valBat, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_valBat, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t* lblBat = lv_label_create(_cardBat);
    lv_label_set_text(lblBat, "BATT V");
    lv_obj_set_style_text_font(lblBat, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lblBat, lv_color_make(0x00, 0xE5, 0xFF), 0);
    lv_obj_align(lblBat, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Card 5: ECU STATUS BADGE (x: 266) */
    _cardEcu = createCard(_screen, 266, 162, 48, 48);

    _badgeEcu = lv_obj_create(_cardEcu);
    lv_obj_set_size(_badgeEcu, 38, 18);
    lv_obj_align(_badgeEcu, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(_badgeEcu, lv_color_make(0x00, 0xE6, 0x76), 0); // Neon Green
    lv_obj_set_style_bg_opa(_badgeEcu, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(_badgeEcu, 3, 0);
    lv_obj_set_style_pad_all(_badgeEcu, 0, 0);
    lv_obj_set_style_border_width(_badgeEcu, 0, 0);
    lv_obj_clear_flag(_badgeEcu, LV_OBJ_FLAG_SCROLLABLE);

    _badgeEcuLabel = lv_label_create(_badgeEcu);
    lv_label_set_text(_badgeEcuLabel, "OK");
    lv_obj_set_style_text_font(_badgeEcuLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_badgeEcuLabel, lv_color_make(0x00, 0x1A, 0x08), 0); // Dark contrast text
    lv_obj_center(_badgeEcuLabel);

    _lblEcu = lv_label_create(_cardEcu);
    lv_label_set_text(_lblEcu, "ECU");
    lv_obj_set_style_text_font(_lblEcu, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(_lblEcu, lv_color_make(0x00, 0xE5, 0xFF), 0);
    lv_obj_align(_lblEcu, LV_ALIGN_BOTTOM_MID, 0, -4);

    /*========================================================================
     * 8. SOLID BRIGHT ELECTRIC ROYAL BLUE FOOTER BAR (y: 216 to 240)
     *========================================================================*/
    _footerBar = lv_obj_create(_screen);
    lv_obj_set_size(_footerBar, 320, 24);
    lv_obj_set_pos(_footerBar, 0, 216);
    lv_obj_set_style_bg_color(_footerBar, lv_color_make(0x00, 0x55, 0xD4), 0); // Vivid Electric Royal Blue
    lv_obj_set_style_bg_opa(_footerBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(_footerBar, 0, 0);
    lv_obj_set_style_radius(_footerBar, 0, 0);
    lv_obj_set_style_pad_all(_footerBar, 0, 0);
    lv_obj_clear_flag(_footerBar, LV_OBJ_FLAG_SCROLLABLE);

    /* Left: Telemetry Status (e.g. "LIVE - 155 MSG/S") */
    _footerLeft = lv_label_create(_footerBar);
    lv_label_set_text(_footerLeft, "LIVE - 155 MSG/S");
    lv_obj_set_style_text_font(_footerLeft, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_footerLeft, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    lv_obj_align(_footerLeft, LV_ALIGN_LEFT_MID, 8, 0);

    /* Right: Measurement Units (e.g. "KM/H - °C") */
    _footerRight = lv_label_create(_footerBar);
    lv_label_set_text(_footerRight, "KM/H - °C");
    lv_obj_set_style_text_font(_footerRight, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_footerRight, lv_color_make(0x80, 0xDE, 0xEA), 0); // Light cyan
    lv_obj_align(_footerRight, LV_ALIGN_RIGHT_MID, -8, 0);
}

void DashboardUI::update(const VehicleData& data) {
    char buf[32];

    /* 1. Gear Indicator */
    if (data.gear.isAvailable()) {
        buf[0] = data.gear.value;
        buf[1] = '\0';
        lv_label_set_text(_gearLabel, buf);
        if (data.gear.value == 'N') {
            lv_obj_set_style_text_color(_gearLabel, lv_color_make(0x00, 0xE5, 0xFF), 0); // Cyan
        } else if (data.gear.value == 'R') {
            lv_obj_set_style_text_color(_gearLabel, lv_color_make(0xFF, 0x33, 0x44), 0); // Red
        } else {
            lv_obj_set_style_text_color(_gearLabel, lv_color_make(0xFF, 0xFF, 0xFF), 0); // White
        }
    } else {
        lv_label_set_text(_gearLabel, "-");
        lv_obj_set_style_text_color(_gearLabel, lv_color_make(0x50, 0x65, 0x82), 0);
    }

    /* 2. Speed Display */
    if (data.speed.isAvailable()) {
        uint16_t dispSpeed = data.speed.value;
        if (g_settings.speedUnit == SpeedUnit::MPH) {
            dispSpeed = (uint16_t)(dispSpeed * 0.621371f + 0.5f);
            lv_label_set_text(_speedUnitLabel, "mph");
        } else {
            lv_label_set_text(_speedUnitLabel, "km/h");
        }
        snprintf(buf, sizeof(buf), "%u", dispSpeed);
        lv_label_set_text(_speedLabel, buf);
        lv_obj_set_style_text_color(_speedLabel, data.speed.isValid() ? 
            lv_color_make(0xFF, 0xFF, 0xFF) : lv_color_make(0x8A, 0x9B, 0xB0), 0);
    } else {
        lv_label_set_text(_speedLabel, "--");
        lv_obj_set_style_text_color(_speedLabel, lv_color_make(0x50, 0x65, 0x82), 0);
    }

    /* 3. Numerical RPM Display & Swept Gauge & Shift Lights */
    uint16_t currentRpm = 0;
    if (data.rpm.isAvailable()) {
        currentRpm = data.rpm.value;
        snprintf(buf, sizeof(buf), "%u", currentRpm);
        lv_label_set_text(_rpmNumLabel, buf);
        _tachometer.setRpm(currentRpm, !data.rpm.isValid());
        if (currentRpm >= g_settings.rpmRedline) {
            lv_obj_set_style_text_color(_rpmNumLabel, lv_color_make(0xFF, 0x22, 0x33), 0);
        } else {
            lv_obj_set_style_text_color(_rpmNumLabel, lv_color_make(0xFF, 0xFF, 0xFF), 0);
        }
    } else {
        lv_label_set_text(_rpmNumLabel, "--");
        lv_obj_set_style_text_color(_rpmNumLabel, lv_color_make(0x50, 0x65, 0x82), 0);
        _tachometer.setRpm(0, true);
    }
    updateShiftLights(currentRpm);

    /* 4. Status Warning Labels (CEL, OIL, CLT, GAS, BAT, AFR) */
    setWarningLabel(_lblCel, data.status.cel, lv_color_make(0xFF, 0x22, 0x33));
    setWarningLabel(_lblOil, data.status.oilPressureWarn, lv_color_make(0xFF, 0x22, 0x33));
    setWarningLabel(_lblClt, data.status.coolantWarn, lv_color_make(0xFF, 0x22, 0x33));
    setWarningLabel(_lblGas, data.status.fuelLowWarn, lv_color_make(0xFF, 0xAA, 0x00));
    setWarningLabel(_lblBat, data.status.batteryWarn, lv_color_make(0xFF, 0x22, 0x33));
    setWarningLabel(_lblAfr, data.status.afrWarn, lv_color_make(0xFF, 0x22, 0x33));

    /* 5. Central Metric Panels */
    // Boost PSI
    if (data.boostPsi.isAvailable()) {
        if (g_settings.boostUnit == PressureUnit::BAR) {
            float bar = data.boostPsi.value * 0.0689476f;
            snprintf(buf, sizeof(buf), "%+.2f", bar);
            lv_label_set_text(_unitBoost, "BOOST BAR");
        } else {
            snprintf(buf, sizeof(buf), "%+.1f", data.boostPsi.value);
            lv_label_set_text(_unitBoost, "BOOST PSI");
        }
        lv_label_set_text(_valBoost, buf);
        lv_obj_set_style_text_color(_valBoost, data.boostPsi.value > 1.0f ? 
            lv_color_make(0xFF, 0xA0, 0x00) : lv_color_make(0xFF, 0xFF, 0xFF), 0);
    } else {
        lv_label_set_text(_valBoost, "--.-");
        lv_obj_set_style_text_color(_valBoost, lv_color_make(0x50, 0x65, 0x82), 0);
    }

    // AFR
    if (data.afr.isAvailable()) {
        snprintf(buf, sizeof(buf), "%.1f", data.afr.value);
        lv_label_set_text(_valAfr, buf);
    } else {
        lv_label_set_text(_valAfr, "--.-");
    }

    // Runtime (Formatted as M:SS or H:MM:SS matching 0:01 in reference)
    if (data.runtimeSec.isAvailable()) {
        uint32_t s = data.runtimeSec.value;
        uint32_t hrs = s / 3600;
        uint32_t mins = (s % 3600) / 60;
        uint32_t secs = s % 60;
        if (hrs > 0) {
            snprintf(buf, sizeof(buf), "%u:%02u:%02u", hrs, mins, secs);
        } else {
            snprintf(buf, sizeof(buf), "%u:%02u", mins, secs);
        }
        lv_label_set_text(_valRuntime, buf);
    } else {
        lv_label_set_text(_valRuntime, "0:00");
    }

    /* 6. Six Bottom Metric Cards */
    // Coolant Temp
    if (data.coolantTemp.isAvailable()) {
        snprintf(buf, sizeof(buf), "%d", data.coolantTemp.value);
        lv_label_set_text(_valClt, buf);
        if (data.coolantTemp.value >= g_settings.coolantWarnTemp) {
            lv_obj_set_style_text_color(_valClt, lv_color_make(0xFF, 0x22, 0x33), 0);
        } else {
            lv_obj_set_style_text_color(_valClt, lv_color_make(0xFF, 0xFF, 0xFF), 0);
        }
    } else {
        lv_label_set_text(_valClt, "--");
    }

    // Oil Temp
    if (data.oilTemp.isAvailable()) {
        snprintf(buf, sizeof(buf), "%d", data.oilTemp.value);
        lv_label_set_text(_valOil, buf);
    } else {
        lv_label_set_text(_valOil, "--");
    }

    // Intake Air Temp
    if (data.intakeTemp.isAvailable()) {
        snprintf(buf, sizeof(buf), "%d", data.intakeTemp.value);
        lv_label_set_text(_valIat, buf);
    } else {
        lv_label_set_text(_valIat, "--");
    }

    // Fuel Level %
    if (data.fuelLevel.isAvailable()) {
        snprintf(buf, sizeof(buf), "%u", data.fuelLevel.value);
        lv_label_set_text(_valFuel, buf);
        if (data.fuelLevel.value <= g_settings.fuelWarnPercent) {
            lv_obj_set_style_text_color(_valFuel, lv_color_make(0xFF, 0xAA, 0x00), 0);
        } else {
            lv_obj_set_style_text_color(_valFuel, lv_color_make(0xFF, 0xFF, 0xFF), 0);
        }
    } else {
        lv_label_set_text(_valFuel, "--");
    }

    // Battery Voltage
    if (data.batteryVolts.isAvailable()) {
        snprintf(buf, sizeof(buf), "%.1f", data.batteryVolts.value);
        lv_label_set_text(_valBat, buf);
        if (data.batteryVolts.value < g_settings.battWarnLow || data.batteryVolts.value > g_settings.battWarnHigh) {
            lv_obj_set_style_text_color(_valBat, lv_color_make(0xFF, 0x22, 0x33), 0);
        } else {
            lv_obj_set_style_text_color(_valBat, lv_color_make(0xFF, 0xFF, 0xFF), 0);
        }
    } else {
        lv_label_set_text(_valBat, "--.-");
    }

    // ECU Connection Badge
    if (data.ecuConnected) {
        lv_obj_set_style_bg_color(_badgeEcu, lv_color_make(0x00, 0xE6, 0x76), 0); // Bright Neon Green
        lv_label_set_text(_badgeEcuLabel, "OK");
        lv_obj_set_style_text_color(_badgeEcuLabel, lv_color_make(0x00, 0x1A, 0x08), 0);
    } else if (data.demoMode) {
        lv_obj_set_style_bg_color(_badgeEcu, lv_color_make(0x00, 0xE6, 0x76), 0); // Green in demo mode
        lv_label_set_text(_badgeEcuLabel, "OK");
        lv_obj_set_style_text_color(_badgeEcuLabel, lv_color_make(0x00, 0x1A, 0x08), 0);
    } else if (data.failedQueries > 0 && !data.ecuConnected) {
        lv_obj_set_style_bg_color(_badgeEcu, lv_color_make(0xFF, 0x22, 0x33), 0); // Red
        lv_label_set_text(_badgeEcuLabel, "ERR");
        lv_obj_set_style_text_color(_badgeEcuLabel, lv_color_make(0xFF, 0xFF, 0xFF), 0);
    } else {
        lv_obj_set_style_bg_color(_badgeEcu, lv_color_make(0xFF, 0x99, 0x00), 0); // Amber
        lv_label_set_text(_badgeEcuLabel, "INIT");
        lv_obj_set_style_text_color(_badgeEcuLabel, lv_color_make(0x00, 0x00, 0x00), 0);
    }

    /* 7. Footer Bar */
    if (data.demoMode) {
        snprintf(buf, sizeof(buf), "LIVE - %.0f MSG/S", data.messagesPerSec > 0.0f ? data.messagesPerSec : 155.0f);
    } else {
        snprintf(buf, sizeof(buf), "LIVE - %.0f MSG/S", data.messagesPerSec);
    }
    lv_label_set_text(_footerLeft, buf);

    if (g_settings.speedUnit == SpeedUnit::MPH) {
        lv_label_set_text(_footerRight, "MPH - °F");
    } else {
        lv_label_set_text(_footerRight, "KM/H - °C");
    }
}
