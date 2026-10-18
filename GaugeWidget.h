/**
 * @file GaugeWidget.h
 * Swept Rally Tachometer Widget with smooth curve ramp and plateau
 * Matches the reference rally instrument cluster
 */

#ifndef GAUGE_WIDGET_H
#define GAUGE_WIDGET_H

#include <lvgl.h>
#include <stdint.h>
#include <stdbool.h>

class GaugeWidget {
private:
    lv_obj_t* _parent;
    lv_obj_t* _tacho_obj;
    lv_obj_t* _marker_labels[8]; // 1, 2, 3, 4, 5, 6, 7

    uint16_t _rpmMin;
    uint16_t _rpmMax;
    uint16_t _rpmRedline;
    uint16_t _rpmWarning;
    float    _currentDisplayedRpm;
    bool     _isStale;

    static void draw_event_cb(lv_event_t* e);

public:
    GaugeWidget();

    /**
     * Initializes and renders the swept tachometer on the target LVGL parent
     */
    void create(lv_obj_t* parent, uint16_t minRpm = 0, uint16_t maxRpm = 8000, 
                uint16_t redlineRpm = 6000, uint16_t warningRpm = 5500);

    /**
     * Updates tachometer position with smoothing
     */
    void setRpm(uint16_t targetRpm, bool isStale = false);

    /**
     * Reconfigures redline threshold dynamically
     */
    void setRedline(uint16_t redlineRpm);

    float getCurrentRpm() const { return _currentDisplayedRpm; }
    uint16_t getRedline() const { return _rpmRedline; }
    bool isStale() const { return _isStale; }
};

#endif /* GAUGE_WIDGET_H */
