/**
 * @file ObdClient.h
 * Non-blocking ELM327 OBD-II client using Bluetooth Classic SPP for ESP32
 */

#ifndef OBD_CLIENT_H
#define OBD_CLIENT_H

#include <Arduino.h>
#include <BluetoothSerial.h>
#include "VehicleData.h"
#include "ObdConfig.h"
#include "Settings.h"

enum class ObdState : uint8_t {
    DISCONNECTED,
    CONNECTING_BT,
    INITIALIZING_ELM,
    DETECTING_PROTOCOL,
    QUERYING_SUPPORTED_PIDS,
    CONNECTED_POLLING,
    CONNECTION_LOST
};

class ObdClient {
private:
    BluetoothSerial _serialBT;
    ObdState        _state;
    String          _targetBtName;
    uint32_t        _stateEntryTime;
    uint32_t        _lastQueryTime;
    uint32_t        _lastTier2Time;
    uint32_t        _lastTier3Time;
    uint32_t        _lastMsgCountTime;
    uint32_t        _msgCounter;

    /* PID Support Mask Flags */
    bool _oilTempSupported;
    bool _fuelLevelSupported;
    bool _runtimeSupported;

    /* Internal query buffer */
    char _rxBuffer[128];
    uint8_t _rxIndex;

    /* Internal Methods */
    bool sendCommand(const char* cmd, char* responseOut, size_t maxLen, uint32_t timeoutMs);
    bool readResponse(char* buffer, size_t maxLen, uint32_t timeoutMs);
    void cleanResponse(char* raw);
    bool parsePidResponse(const char* pid, const char* response, uint8_t* bytesOut, size_t* numBytes);

    void processTier1();
    void processTier2();
    void processTier3();

public:
    ObdClient();

    /**
     * Initializes Bluetooth Classic hardware stack
     */
    void begin(const char* btDeviceName = DEFAULT_OBD_BT_NAME);

    /**
     * Non-blocking state machine step called periodically
     */
    void loop();

    /**
     * Connect or disconnect
     */
    void connect();
    void disconnect();

    /**
     * Returns true if active and communicating with vehicle ECU
     */
    bool isConnected() const {
        return _state == ObdState::CONNECTED_POLLING;
    }

    ObdState getState() const {
        return _state;
    }
};

extern ObdClient g_obdClient;

#endif /* OBD_CLIENT_H */
