/**
 * @file Settings.h
 * Configurable thresholds, unit preferences, and calibration constants
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

enum class SpeedUnit : uint8_t {
    KMH = 0,
    MPH = 1
};

enum class TempUnit : uint8_t {
    CELSIUS = 0,
    FAHRENHEIT = 1
};

enum class PressureUnit : uint8_t {
    PSI = 0,
    BAR = 1,
    KPA = 2
};

struct DashboardSettings {
    /* Units */
    SpeedUnit    speedUnit = SpeedUnit::KMH;
    TempUnit     tempUnit  = TempUnit::CELSIUS;
    PressureUnit boostUnit = PressureUnit::PSI;

    /* Tachometer Limits */
    uint16_t rpmMin     = 0;
    uint16_t rpmMax     = 8000;
    uint16_t rpmRedline = 6500;
    uint16_t rpmWarning = 6000;

    /* Warning Thresholds */
    int16_t  coolantWarnTemp = 105;   // °C
    int16_t  coolantDangerTemp = 115; // °C
    float    battWarnLow     = 11.8f; // V
    float    battWarnHigh    = 15.2f; // V
    uint8_t  fuelWarnPercent = 15;    // %
    float    afrLeanWarn     = 15.5f; // AFR during load

    /* Boost Calibration */
    float    barometricPressurePsi = 14.69f; // Standard atmospheric pressure at sea level

    /* Display */
    uint8_t  backlightBrightness = 230; // 0 - 255
};

extern DashboardSettings g_settings;

void loadSettings();
void saveSettings();

#endif /* SETTINGS_H */
