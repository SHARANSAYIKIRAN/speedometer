/**
 * @file DashboardUI.h
 * Complete RallyDash UI Component Hierarchy for LVGL 8.4
 * Exact match to the rally instrument cluster reference
 */

#ifndef DASHBOARD_UI_H
#define DASHBOARD_UI_H

#include <lvgl.h>
#include "VehicleData.h"
#include "GaugeWidget.h"
#include "Settings.h"

class DashboardUI {
private:
    lv_obj_t* _screen;

    /* Top-Left: Gear & Warnings */
    lv_obj_t* _gearLabel;
    lv_obj_t* _warnCont;
    lv_obj_t* _lblCel;
    lv_obj_t* _lblOil;
    lv_obj_t* _lblClt;
    lv_obj_t* _lblGas;
    lv_obj_t* _lblBat;
    lv_obj_t* _lblAfr;

    /* Top-Center: Shift Lights */
    lv_obj_t* _shiftCont;
    lv_obj_t* _shiftLeds[10];

    /* Top-Right: Speed & RPM Numerical */
    lv_obj_t* _speedLabel;
    lv_obj_t* _speedUnitLabel;
    lv_obj_t* _rpmNumLabel;
    lv_obj_t* _rpmUnitLabel;

    /* Middle: Swept Tachometer */
    GaugeWidget _tachometer;

    /* 3 Central Metric Panels */
    lv_obj_t* _panelBoost;
    lv_obj_t* _valBoost;
    lv_obj_t* _unitBoost;

    lv_obj_t* _panelAfr;
    lv_obj_t* _valAfr;
    lv_obj_t* _subAfr;

    lv_obj_t* _panelRuntime;
    lv_obj_t* _valRuntime;
    lv_obj_t* _subRuntime;

    /* 6 Bottom Metric Cards */
    lv_obj_t* _cardClt;
    lv_obj_t* _valClt;

    lv_obj_t* _cardOil;
    lv_obj_t* _valOil;

    lv_obj_t* _cardIat;
    lv_obj_t* _valIat;

    lv_obj_t* _cardFuel;
    lv_obj_t* _valFuel;

    lv_obj_t* _cardBat;
    lv_obj_t* _valBat;

    lv_obj_t* _cardEcu;
    lv_obj_t* _badgeEcu;
    lv_obj_t* _badgeEcuLabel;
    lv_obj_t* _lblEcu;

    /* Footer Bar */
    lv_obj_t* _footerBar;
    lv_obj_t* _footerLeft;
    lv_obj_t* _footerRight;

    /* Helper Methods */
    lv_obj_t* createCard(lv_obj_t* parent, int x, int y, int w, int h);
    void updateShiftLights(uint16_t rpm);
    void setWarningLabel(lv_obj_t* lbl, bool active, lv_color_t activeColor);

public:
    DashboardUI();

    /**
     * Builds the entire visual hierarchy on the active screen
     */
    void init();

    /**
     * Refreshes all UI widgets with fresh or stale vehicle telemetry
     */
    void update(const VehicleData& data);
};

extern DashboardUI g_dashboardUI;

#endif /* DASHBOARD_UI_H */
