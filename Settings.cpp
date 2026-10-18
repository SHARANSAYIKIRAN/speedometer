/**
 * @file Settings.cpp
 * Settings management and defaults implementation
 */

#include "Settings.h"
#include <Arduino.h>

DashboardSettings g_settings;

void loadSettings() {
    // Standard defaults configured in struct initialization
    // Can be extended with ESP32 Preferences (NVS) for persistent user adjustments
}

void saveSettings() {
    // Future expansion: store adjusted settings in Preferences (NVS)
}
