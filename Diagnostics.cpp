/**
 * @file Diagnostics.cpp
 * System diagnostics and interactive testing shell implementation
 */

#include "Diagnostics.h"
#include "VehicleData.h"
#include "Settings.h"
#include "ObdClient.h"

Diagnostics g_diagnostics;

Diagnostics::Diagnostics()
    : _lastPrintTime(0), _frameCount(0), _fps(0.0f) {}

void Diagnostics::begin() {
    Serial.println("\n==========================================");
    Serial.println("  RALLYDASH — ESP32 AUTOMOTIVE CLUSTER    ");
    Serial.println("==========================================");
    Serial.println("Interactive Serial Commands:");
    Serial.println("  [d] - Toggle DEMO Mode / LIVE Mode");
    Serial.println("  [u] - Toggle Units (KM/H <-> MPH)");
    Serial.println("  [b] - Reconnect / Disconnect Bluetooth ELM327");
    Serial.println("  [w] - Toggle Simulated Warnings (CEL, Overheat)");
    Serial.println("  [s] - Print System Health (Heap, FPS, Stack)");
    Serial.println("==========================================\n");
}

void Diagnostics::notifyFrameRendered() {
    _frameCount++;
}

void Diagnostics::loop() {
    uint32_t now = millis();
    if (now - _lastPrintTime >= 2000) {
        _fps = (float)_frameCount * 1000.0f / (float)(now - _lastPrintTime);
        _frameCount = 0;
        _lastPrintTime = now;

        uint32_t freeHeap = esp_get_free_internal_heap_size();
        uint32_t minHeap = esp_get_minimum_free_heap_size();

        // Print heartbeat every 10 seconds or when requested
        static uint8_t count = 0;
        if (++count >= 5) {
            count = 0;
            Serial.printf("[SYSTEM] FPS: %4.1f | Free Heap: %6u B | Min Heap: %6u B\n", 
                          _fps, freeHeap, minHeap);
        }
    }

    processSerialCommands();
}

void Diagnostics::processSerialCommands() {
    while (Serial.available()) {
        char c = (char)Serial.read();
        switch (c) {
            case 'd':
            case 'D':
                g_vehicleStore.modify([](VehicleData& d) {
                    d.demoMode = !d.demoMode;
                    Serial.printf("[SHELL] Switched to %s mode\n", d.demoMode ? "DEMO" : "LIVE");
                });
                break;

            case 'u':
            case 'U':
                g_settings.speedUnit = (g_settings.speedUnit == SpeedUnit::KMH) ? 
                    SpeedUnit::MPH : SpeedUnit::KMH;
                Serial.printf("[SHELL] Speed unit changed to %s\n", 
                              g_settings.speedUnit == SpeedUnit::KMH ? "KM/H" : "MPH");
                break;

            case 'b':
            case 'B':
                if (g_obdClient.isConnected()) {
                    Serial.println("[SHELL] Disconnecting OBD...");
                    g_obdClient.disconnect();
                } else {
                    Serial.println("[SHELL] Connecting OBD...");
                    g_obdClient.connect();
                }
                break;

            case 'w':
            case 'W':
                g_vehicleStore.modify([](VehicleData& d) {
                    d.status.cel = !d.status.cel;
                    d.status.coolantWarn = !d.status.coolantWarn;
                    Serial.printf("[SHELL] Warning toggled: CEL=%d, CLT_WARN=%d\n", 
                                  d.status.cel, d.status.coolantWarn);
                });
                break;

            case 's':
            case 'S':
                Serial.println("\n--- SYSTEM HEALTH REPORT ---");
                Serial.printf("Current FPS:          %5.1f\n", _fps);
                Serial.printf("Free Internal Heap:   %u bytes\n", esp_get_free_internal_heap_size());
                Serial.printf("Minimum Ever Heap:    %u bytes\n", esp_get_minimum_free_heap_size());
                Serial.printf("OBD State:            %u\n", (uint8_t)g_obdClient.getState());
                Serial.println("----------------------------\n");
                break;

            default:
                break;
        }
    }
}
