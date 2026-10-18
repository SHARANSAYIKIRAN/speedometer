/**
 * @file GaugeWidget.cpp
 * Swept Rally Tachometer Widget Implementation for LVGL 8.4
 * Matches the reference rally instrument cluster
 */

#include "GaugeWidget.h"
#include <stdio.h>
#include <math.h>

static void getRpmPoint(float rpm, lv_point_t* pt) {
    if (rpm <= 0.0f) {
        pt->x = 12;
        pt->y = 142;
        return;
    }
    if (rpm <= 2000.0f) {
        float t = rpm / 2000.0f;
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;

        float p0x = 12.0f, p0y = 142.0f;
        float p1x = 35.0f, p1y = 140.0f;
        float p2x = 65.0f, p2y = 92.0f;
        float p3x = 86.0f, p3y = 92.0f;

        pt->x = (lv_coord_t)(uuu * p0x + 3.0f * uu * t * p1x + 3.0f * u * tt * p2x + ttt * p3x + 0.5f);
        pt->y = (lv_coord_t)(uuu * p0y + 3.0f * uu * t * p1y + 3.0f * u * tt * p2y + ttt * p3y + 0.5f);
        return;
    }
    float px = 86.0f + ((rpm - 2000.0f) / 1000.0f) * 38.0f;
    if (px > 314.0f) px = 314.0f;
    pt->x = (lv_coord_t)(px + 0.5f);
    pt->y = 92;
}

GaugeWidget::GaugeWidget() 
    : _parent(nullptr), _tacho_obj(nullptr),
      _rpmMin(0), _rpmMax(8000), _rpmRedline(6000), _rpmWarning(5500),
      _currentDisplayedRpm(0.0f), _isStale(false) {
    for (int i = 0; i < 8; i++) {
        _marker_labels[i] = nullptr;
    }
}

void GaugeWidget::draw_event_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_DRAW_MAIN) return;

    GaugeWidget* widget = (GaugeWidget*)lv_event_get_user_data(e);
    if (!widget) return;

    lv_draw_ctx_t* draw_ctx = lv_event_get_draw_ctx(e);

    /* Points along the path */
    static const float rpmSteps[] = {
        0.0f, 200.0f, 400.0f, 600.0f, 800.0f, 1000.0f, 1200.0f, 1400.0f, 1600.0f, 1800.0f, 2000.0f,
        2500.0f, 3000.0f, 3500.0f, 4000.0f, 4500.0f, 5000.0f, 5500.0f, 6000.0f, 6500.0f, 7000.0f, 7500.0f, 8000.0f
    };
    const int numSteps = sizeof(rpmSteps) / sizeof(rpmSteps[0]);

    lv_point_t pts[numSteps];
    for (int i = 0; i < numSteps; i++) {
        getRpmPoint(rpmSteps[i], &pts[i]);
    }

    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.width = 11;
    line_dsc.round_start = 1;
    line_dsc.round_end = 1;

    /* 1. Draw Background Track */
    for (int i = 0; i < numSteps - 1; i++) {
        if (rpmSteps[i] >= widget->_rpmRedline) {
            line_dsc.color = lv_color_make(0x5A, 0x12, 0x1C); // Dark crimson redline zone
        } else {
            line_dsc.color = lv_color_make(0x13, 0x25, 0x3F); // Dark slate navy
        }
        line_dsc.round_start = (i == 0) ? 1 : 0;
        line_dsc.round_end = (i == numSteps - 2) ? 1 : 0;
        lv_draw_line(draw_ctx, &line_dsc, &pts[i], &pts[i + 1]);
    }

    /* 2. Draw Active RPM Fill */
    float curRpm = widget->_currentDisplayedRpm;
    if (curRpm > 50.0f) {
        for (int i = 0; i < numSteps - 1; i++) {
            if (curRpm <= rpmSteps[i]) break;

            line_dsc.round_start = (i == 0) ? 1 : 0;

            if (curRpm >= rpmSteps[i + 1]) {
                // Entire segment is filled
                if (widget->_isStale) {
                    line_dsc.color = lv_color_make(0x50, 0x60, 0x70);
                } else if (rpmSteps[i] >= widget->_rpmRedline) {
                    line_dsc.color = lv_color_make(0xFF, 0x22, 0x33); // Neon Red
                } else {
                    line_dsc.color = lv_color_make(0xFF, 0xFF, 0xFF); // Crisp White
                }
                line_dsc.round_end = (curRpm == rpmSteps[i + 1]) ? 1 : 0;
                lv_draw_line(draw_ctx, &line_dsc, &pts[i], &pts[i + 1]);
            } else {
                // Partial segment up to curRpm
                lv_point_t endPt;
                getRpmPoint(curRpm, &endPt);

                if (widget->_isStale) {
                    line_dsc.color = lv_color_make(0x50, 0x60, 0x70);
                } else if (rpmSteps[i] >= widget->_rpmRedline) {
                    line_dsc.color = lv_color_make(0xFF, 0x22, 0x33);
                } else {
                    line_dsc.color = lv_color_make(0xFF, 0xFF, 0xFF);
                }
                line_dsc.round_end = 1;
                lv_draw_line(draw_ctx, &line_dsc, &pts[i], &endPt);
                break;
            }
        }
    }

    /* 3. Draw Ticks below each 1000 RPM mark */
    lv_draw_line_dsc_t tick_dsc;
    lv_draw_line_dsc_init(&tick_dsc);
    tick_dsc.width = 2;
    tick_dsc.round_start = 1;
    tick_dsc.round_end = 1;

    // Tick 1 (1000 RPM)
    tick_dsc.color = lv_color_make(0x60, 0x94, 0xC0);
    lv_point_t t1_start = { 49, 122 };
    lv_point_t t1_end   = { 47, 127 };
    lv_draw_line(draw_ctx, &tick_dsc, &t1_start, &t1_end);

    // Ticks 2-7
    const int tickX[] = { 86, 124, 162, 200, 238, 276 };
    for (int i = 0; i < 6; i++) {
        int markRpm = (i + 2) * 1000;
        if (markRpm >= widget->_rpmRedline) {
            tick_dsc.color = lv_color_make(0xFF, 0x44, 0x44); // Redline tick
        } else {
            tick_dsc.color = lv_color_make(0x60, 0x94, 0xC0); // Cyan-steel tick
        }
        lv_point_t p_start = { (lv_coord_t)tickX[i], 98 };
        lv_point_t p_end   = { (lv_coord_t)tickX[i], 103 };
        lv_draw_line(draw_ctx, &tick_dsc, &p_start, &p_end);
    }
}

void GaugeWidget::create(lv_obj_t* parent, uint16_t minRpm, uint16_t maxRpm, 
                         uint16_t redlineRpm, uint16_t warningRpm) {
    _parent = parent;
    _rpmMin = minRpm;
    _rpmMax = maxRpm;
    _rpmRedline = redlineRpm;
    _rpmWarning = warningRpm;

    /* Full-screen transparent canvas object for tachometer drawing */
    _tacho_obj = lv_obj_create(_parent);
    lv_obj_set_size(_tacho_obj, 320, 240);
    lv_obj_set_pos(_tacho_obj, 0, 0);
    lv_obj_set_style_bg_opa(_tacho_obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_tacho_obj, 0, 0);
    lv_obj_set_style_pad_all(_tacho_obj, 0, 0);
    lv_obj_clear_flag(_tacho_obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    /* Hook drawing callback */
    lv_obj_add_event_cb(_tacho_obj, draw_event_cb, LV_EVENT_DRAW_MAIN, this);

    /* Numbers 1, 2, 3, 4, 5, 6, 7 */
    const struct { int x; int y; const char* txt; bool red; } markers[7] = {
        { 42, 128, "1", false },
        { 82, 104, "2", false },
        { 120, 104, "3", false },
        { 158, 104, "4", false },
        { 196, 104, "5", false },
        { 234, 104, "6", true  },
        { 272, 104, "7", true  }
    };

    for (int i = 0; i < 7; i++) {
        _marker_labels[i] = lv_label_create(_parent);
        lv_label_set_text(_marker_labels[i], markers[i].txt);
        lv_obj_set_style_text_font(_marker_labels[i], &lv_font_montserrat_12, 0);
        if (markers[i].red) {
            lv_obj_set_style_text_color(_marker_labels[i], lv_color_make(0xFF, 0x44, 0x44), 0);
        } else {
            lv_obj_set_style_text_color(_marker_labels[i], lv_color_make(0x8A, 0xB8, 0xDF), 0);
        }
        lv_obj_set_pos(_marker_labels[i], markers[i].x, markers[i].y);
    }
}

void GaugeWidget::setRpm(uint16_t targetRpm, bool isStale) {
    if (!_tacho_obj) return;

    if (targetRpm > _rpmMax) targetRpm = _rpmMax;
    if (targetRpm < _rpmMin) targetRpm = _rpmMin;

    _isStale = isStale;

    /* Smooth response filter */
    _currentDisplayedRpm = _currentDisplayedRpm * 0.70f + (float)targetRpm * 0.30f;

    /* Invalidate object to trigger redraw */
    lv_obj_invalidate(_tacho_obj);
}

void GaugeWidget::setRedline(uint16_t redlineRpm) {
    _rpmRedline = redlineRpm;
    if (_tacho_obj) {
        lv_obj_invalidate(_tacho_obj);
    }
}
