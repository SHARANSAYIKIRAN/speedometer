/**
 * @file VehicleData.h
 * Typed vehicle telemetry model with freshness and validity tracking
 */

#ifndef VEHICLE_DATA_H
#define VEHICLE_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * Data Quality and freshness state for each individual measurement
 */
enum class DataQuality : uint8_t {
    INVALID = 0,     // Initial state or communication error
    UNSUPPORTED,     // ECU responded with NO DATA or command rejection
    STALE,           // No fresh reading within timeout window
    VALID            // Fresh, verified reading from ECU
};

template <typename T>
struct Metric {
    T value;
    T raw;
    DataQuality quality = DataQuality::INVALID;
    uint32_t lastUpdateMs = 0;

    void set(T val, uint32_t now) {
        value = val;
        raw = val;
        quality = DataQuality::VALID;
        lastUpdateMs = now;
    }

    void markStale(uint32_t now, uint32_t thresholdMs = 3000) {
        if (quality == DataQuality::VALID && (now - lastUpdateMs > thresholdMs)) {
            quality = DataQuality::STALE;
        }
    }

    bool isValid() const {
        return quality == DataQuality::VALID;
    }

    bool isAvailable() const {
        return quality == DataQuality::VALID || quality == DataQuality::STALE;
    }
};

/**
 * Status warning indicators (CEL, OIL, CLT, GAS, BAT, AFR)
 */
struct StatusFlags {
    bool cel = false;              // Check Engine Light (MIL)
    bool oilPressureWarn = false;  // Low oil pressure / extreme temp
    bool coolantWarn = false;      // Overheating warning (>105°C)
    bool fuelLowWarn = false;      // Low fuel reserve (<15%)
    bool batteryWarn = false;      // Alternator / battery voltage fault (<11.8V or >15.0V)
    bool afrWarn = false;          // Lean spike warning under load
};

/**
 * Main Vehicle Telemetry Data Structure
 */
struct VehicleData {
    /* Primary Driving Dynamics */
    Metric<uint16_t> speed;        // Vehicle speed in km/h
    Metric<uint16_t> rpm;          // Engine speed in RPM
    Metric<char>     gear;         // 'N', '1'..'6', 'R', or '-' if unknown

    /* Central Secondary Metrics */
    Metric<float>    boostPsi;     // Manifold boost/vacuum in PSI (-14.7 to +30.0)
    Metric<float>    afr;          // Air-Fuel Ratio (e.g. 14.7)
    Metric<uint32_t> runtimeSec;   // Engine runtime in seconds

    /* Bottom Row Cards */
    Metric<int16_t>  coolantTemp;  // Engine Coolant Temperature in °C
    Metric<int16_t>  oilTemp;      // Engine Oil Temperature in °C (often unsupported in older ECUs)
    Metric<int16_t>  intakeTemp;   // Intake Air Temperature (IAT) in °C
    Metric<uint8_t>  fuelLevel;    // Fuel Tank Level in % (0 - 100)
    Metric<float>    batteryVolts; // System voltage from ELM327 ATRV or ECU PID
    Metric<uint8_t>  engineLoad;   // Calculated Engine Load % (0 - 100)

    /* Warning Indicators */
    StatusFlags      status;

    /* Connection and Telemetry Diagnostics */
    bool             ecuConnected = false;
    bool             demoMode = true;       // Starts in DEMO mode for desk development
    char             protocolName[24] = "DEMO SIM";
    float            messagesPerSec = 0.0f;
    uint32_t         totalQueries = 0;
    uint32_t         failedQueries = 0;
    uint32_t         lastHeartbeatMs = 0;

    /**
     * Updates staleness flags across all parameters
     */
    void checkStaleness(uint32_t now) {
        speed.markStale(now, 1000);
        rpm.markStale(now, 1000);
        boostPsi.markStale(now, 2000);
        afr.markStale(now, 2000);
        runtimeSec.markStale(now, 5000);
        coolantTemp.markStale(now, 5000);
        oilTemp.markStale(now, 5000);
        intakeTemp.markStale(now, 5000);
        fuelLevel.markStale(now, 10000);
        batteryVolts.markStale(now, 4000);
        engineLoad.markStale(now, 2000);

        if (now - lastHeartbeatMs > 4000) {
            ecuConnected = false;
        }
    }
};

/**
 * Thread-safe wrapper for VehicleData across FreeRTOS cores
 */
class VehicleDataStore {
private:
    VehicleData _data;
    SemaphoreHandle_t _mutex;

public:
    VehicleDataStore() {
        _mutex = xSemaphoreCreateMutex();
    }

    void read(VehicleData &out) {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            out = _data;
            xSemaphoreGive(_mutex);
        }
    }

    void update(const VehicleData &in) {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            _data = in;
            xSemaphoreGive(_mutex);
        }
    }

    template<typename F>
    void modify(F lambda) {
        if (xSemaphoreTake(_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            lambda(_data);
            xSemaphoreGive(_mutex);
        }
    }
};

extern VehicleDataStore g_vehicleStore;

#endif /* VEHICLE_DATA_H */
