/**
 * @file main.cpp
 * RallyDash — ESP32 Real-Time Automotive Digital Cluster
 * Dual-core FreeRTOS firmware coordinating LovyanGFX, LVGL 8.4, and ELM327 OBD-II
 */

#include <Arduino.h>
#include "DisplayConfig.h"
#include "VehicleData.h"
#include "Settings.h"
#include "DashboardUI.h"
#include "ObdClient.h"
#include "Diagnostics.h"
#include <math.h>

/* Global vehicle data repository */
VehicleDataStore g_vehicleStore;

/* Simulation State Variables for DEMO Mode */
static uint32_t simLastTick = 0;
static float simRpm = 1542.0f;
static float simSpeed = 0.0f;
static char simGear = 'N';
static float simBoost = -7.1f;
static float simAfr = 13.9f;
static float simCoolant = 44.0f;
static float simFuel = 39.0f;
static float simBattery = 14.2f;
static uint32_t simStartTime = 0;
static uint8_t simState = 0; // 0=Idle, 1=1st, 2=2nd, 3=3rd, 4=Cruise, 5=Decel

static void updateDemoSimulation(uint32_t now) {
    if (simStartTime == 0) simStartTime = now;
    float dt = (float)(now - simLastTick) / 1000.0f;
    simLastTick = now;
    if (dt <= 0.0f || dt > 1.0f) dt = 0.03f;

    // Simulation sequence state machine
    static float stateTimer = 0.0f;
    stateTimer += dt;

    switch (simState) {
        case 0: // Idle in Neutral (3 seconds)
            simGear = 'N';
            simRpm = 1542.0f + 12.0f * sinf(now * 0.005f);
            simSpeed = 0.0f;
            simBoost = -7.1f + 0.1f * sinf(now * 0.003f);
            simAfr = 13.9f + 0.05f * sinf(now * 0.008f);
            if (stateTimer > 4.0f) {
                simState = 1;
                stateTimer = 0.0f;
            }
            break;

        case 1: // 1st Gear Acceleration (0 - 45 km/h, RPM: 1200 -> 5800)
            simGear = '1';
            simRpm += 1200.0f * dt;
            simSpeed += 12.0f * dt;
            simBoost = -1.0f + (simRpm / 5800.0f) * 12.5f; // Spool to +11.5 PSI
            simAfr = 12.4f - 0.5f * (simBoost / 15.0f);   // Rich under boost
            if (simRpm >= 5600.0f || stateTimer > 3.5f) {
                simState = 2; // Shift to 2nd
                stateTimer = 0.0f;
                simRpm = 3200.0f; // Drop RPM for shift
            }
            break;

        case 2: // 2nd Gear Acceleration (45 - 82 km/h, RPM: 3200 -> 6100)
            simGear = '2';
            simRpm += 1050.0f * dt;
            simSpeed += 13.5f * dt;
            simBoost = 10.0f + 4.5f * sinf(stateTimer * 2.0f); // Peak boost +14.5 PSI
            simAfr = 12.1f;
            if (simRpm >= 6000.0f || stateTimer > 3.0f) {
                simState = 3; // Shift to 3rd
                stateTimer = 0.0f;
                simRpm = 3600.0f;
            }
            break;

        case 3: // 3rd Gear Pull (82 - 124 km/h, RPM: 3600 -> 5400)
            simGear = '3';
            simRpm += 750.0f * dt;
            simSpeed += 16.0f * dt;
            simBoost = 13.8f + 1.2f * sinf(now * 0.004f); // Rock solid +14.2 PSI
            simAfr = 11.9f;
            if (simSpeed >= 124.0f || stateTimer > 3.0f) {
                simState = 4; // Shift to 4th Cruise
                stateTimer = 0.0f;
            }
            break;

        case 4: // 4th Gear Highway Cruise (124 km/h, RPM ~3500)
            simGear = '4';
            simSpeed = 124.0f + 1.5f * sinf(now * 0.002f);
            simRpm = 3520.0f + 60.0f * sinf(now * 0.003f);
            simBoost = -1.5f + 0.5f * sinf(now * 0.004f); // Mild vacuum/light throttle
            simAfr = 14.7f + 0.1f * sinf(now * 0.007f);
            if (stateTimer > 7.0f) {
                simState = 5; // Decelerate back to idle
                stateTimer = 0.0f;
            }
            break;

        case 5: // Braking and Downshifting back to 0 km/h
            simSpeed -= 25.0f * dt;
            if (simSpeed <= 0.0f) simSpeed = 0.0f;

            if (simSpeed > 80.0f) {
                simGear = '3';
                simRpm = 3400.0f;
            } else if (simSpeed > 40.0f) {
                simGear = '2';
                simRpm = 2800.0f;
            } else if (simSpeed > 10.0f) {
                simGear = '1';
                simRpm = 1800.0f;
            } else {
                simGear = 'N';
                simRpm = 850.0f;
            }
            simBoost = -12.0f; // High vacuum on overrun
            simAfr = 18.0f;    // Fuel cut on overrun deceleration

            if (simSpeed <= 0.0f && stateTimer > 5.0f) {
                simState = 0; // Restart cycle
                stateTimer = 0.0f;
            }
            break;
    }

    // Gradual realistic engine warm-up (68°C to 88°C)
    if (simCoolant < 88.0f) {
        simCoolant += 0.12f * dt;
    } else {
        simCoolant = 88.0f + 1.0f * sinf(now * 0.001f);
    }

    // Battery voltage fluctuation
    simBattery = 14.0f + 0.15f * sinf(now * 0.002f);

    // Update thread-safe VehicleDataStore
    g_vehicleStore.modify([now](VehicleData& d) {
        d.demoMode = true;
        d.ecuConnected = true; // Sim connection is active
        strncpy(d.protocolName, "OBD-II DEMO SIM", sizeof(d.protocolName) - 1);

        d.speed.set((uint16_t)simSpeed, now);
        d.rpm.set((uint16_t)simRpm, now);
        d.gear.set(simGear, now);
        d.boostPsi.set(simBoost, now);
        d.afr.set(simAfr, now);
        d.runtimeSec.set((now - simStartTime) / 1000, now);
        d.coolantTemp.set((int16_t)simCoolant, now);
        d.oilTemp.set(31, now);
        d.intakeTemp.set(31, now);
        d.fuelLevel.set((uint8_t)simFuel, now);
        d.batteryVolts.set(simBattery, now);
        d.engineLoad.set((uint8_t)((simRpm / 6000.0f) * 85.0f), now);
        d.messagesPerSec = 155.0f;

        d.status.cel = false;
        d.status.coolantWarn = (d.coolantTemp.value >= g_settings.coolantWarnTemp);
        d.status.fuelLowWarn = (d.fuelLevel.value <= g_settings.fuelWarnPercent);
        d.status.batteryWarn = (d.batteryVolts.value < g_settings.battWarnLow);
    });
}

/* FreeRTOS Core 0 Task: OBD-II Bluetooth Client & Physics Simulation */
void backendTask(void* parameter) {
    simLastTick = millis();

    while (true) {
        uint32_t now = millis();

        VehicleData currentData;
        g_vehicleStore.read(currentData);

        if (currentData.demoMode) {
            updateDemoSimulation(now);
        } else {
            // Live OBD mode: run communication state machine
            g_obdClient.loop();
            currentData.checkStaleness(now);
        }

        g_diagnostics.loop();
        vTaskDelay(pdMS_TO_TICKS(20)); // 50 Hz backend cycle
    }
}

/* FreeRTOS Core 1 Task: LVGL UI Rendering & Display Driver */
void uiTask(void* parameter) {
    while (true) {
        // Read latest telemetry from store
        VehicleData data;
        g_vehicleStore.read(data);

        // Update all UI elements
        g_dashboardUI.update(data);

        // Let LVGL perform rendering and DMA flushing
        lv_timer_handler();

        g_diagnostics.notifyFrameRendered();

        vTaskDelay(pdMS_TO_TICKS(15)); // ~60 Hz UI loop
    }
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n[BOOT] Initializing RallyDash Automotive Instrument Cluster...");

    // 1. Load user settings
    loadSettings();

    // 2. Initialize LovyanGFX and LVGL driver bridge
    initDisplay();
    Serial.println("[BOOT] Display initialized.");

    // 3. Build the dashboard UI
    g_dashboardUI.init();
    Serial.println("[BOOT] Dashboard UI hierarchy mounted.");

    // 4. Initialize Diagnostics shell
    g_diagnostics.begin();

    // 5. Initialize OBD Bluetooth Classic stack
    g_obdClient.begin(DEFAULT_OBD_BT_NAME);

    // 6. Spawn FreeRTOS Tasks across dual cores
    xTaskCreatePinnedToCore(
        backendTask,
        "BackendTask",
        4096,
        NULL,
        2,
        NULL,
        0 // Core 0 for Bluetooth and simulation
    );

    xTaskCreatePinnedToCore(
        uiTask,
        "UiTask",
        6144,
        NULL,
        3,
        NULL,
        1 // Core 1 for LVGL UI rendering
    );

    Serial.println("[BOOT] Dual-core execution running. Ready!");
}

void loop() {
    // FreeRTOS tasks handle Core 0 and Core 1 independently.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
