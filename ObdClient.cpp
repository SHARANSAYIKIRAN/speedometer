/**
 * @file ObdClient.cpp
 * Non-blocking ELM327 OBD-II Driver and Protocol Engine
 */

#include "ObdClient.h"
#include <ctype.h>

ObdClient g_obdClient;

ObdClient::ObdClient()
    : _state(ObdState::DISCONNECTED),
      _targetBtName(DEFAULT_OBD_BT_NAME),
      _stateEntryTime(0),
      _lastQueryTime(0),
      _lastTier2Time(0),
      _lastTier3Time(0),
      _lastMsgCountTime(0),
      _msgCounter(0),
      _oilTempSupported(true),
      _fuelLevelSupported(true),
      _runtimeSupported(true),
      _rxIndex(0) {
}

void ObdClient::begin(const char* btDeviceName) {
    _targetBtName = btDeviceName ? btDeviceName : DEFAULT_OBD_BT_NAME;
    _serialBT.begin("RallyDash-ESP32", true); // true = Master mode
    _serialBT.setPin(DEFAULT_OBD_BT_PIN);
    _state = ObdState::DISCONNECTED;
    _stateEntryTime = millis();
}

void ObdClient::connect() {
    if (_state == ObdState::DISCONNECTED || _state == ObdState::CONNECTION_LOST) {
        Serial.printf("[OBD] Initiating Bluetooth connection to '%s'...\n", _targetBtName.c_str());
        _state = ObdState::CONNECTING_BT;
        _stateEntryTime = millis();
    }
}

void ObdClient::disconnect() {
    if (_serialBT.connected()) {
        _serialBT.disconnect();
    }
    _state = ObdState::DISCONNECTED;
    g_vehicleStore.modify([](VehicleData& d) {
        d.ecuConnected = false;
        d.checkStaleness(millis());
    });
}

void ObdClient::cleanResponse(char* raw) {
    char* src = raw;
    char* dst = raw;
    while (*src) {
        if (*src != ' ' && *src != '\r' && *src != '\n' && *src != '>') {
            *dst++ = toupper((unsigned char)*src);
        }
        src++;
    }
    *dst = '\0';
}

bool ObdClient::readResponse(char* buffer, size_t maxLen, uint32_t timeoutMs) {
    uint32_t start = millis();
    size_t idx = 0;

    while (millis() - start < timeoutMs) {
        while (_serialBT.available()) {
            char c = (char)_serialBT.read();
            if (c == '>') {
                // ELM327 prompt indicates completion of command
                buffer[idx] = '\0';
                return true;
            }
            if (idx < maxLen - 1) {
                buffer[idx++] = c;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    buffer[idx] = '\0';
    return idx > 0;
}

bool ObdClient::sendCommand(const char* cmd, char* responseOut, size_t maxLen, uint32_t timeoutMs) {
    if (!_serialBT.connected()) {
        return false;
    }

    // Flush any pending junk bytes
    while (_serialBT.available()) {
        _serialBT.read();
    }

    _serialBT.print(cmd);
    _serialBT.print('\r');

    char tempBuf[128];
    bool ok = readResponse(tempBuf, sizeof(tempBuf), timeoutMs);
    if (!ok) {
        g_vehicleStore.modify([](VehicleData& d) { d.failedQueries++; });
        return false;
    }

    cleanResponse(tempBuf);
    strncpy(responseOut, tempBuf, maxLen - 1);
    responseOut[maxLen - 1] = '\0';

    _msgCounter++;
    g_vehicleStore.modify([](VehicleData& d) {
        d.totalQueries++;
        d.lastHeartbeatMs = millis();
    });

    return true;
}

bool ObdClient::parsePidResponse(const char* pid, const char* response, uint8_t* bytesOut, size_t* numBytes) {
    // Mode 01 PID response format: "41" + PID + "AABBCCDD..."
    // Example: PID "010C" -> responds with "410C1A2F"
    if (strstr(response, "NODATA") != nullptr || strstr(response, "UNABLE") != nullptr) {
        return false;
    }

    // Expected prefix: "41" + (pid + 2) e.g. "410C"
    char expectedPrefix[5];
    expectedPrefix[0] = '4';
    expectedPrefix[1] = '1';
    expectedPrefix[2] = pid[2];
    expectedPrefix[3] = pid[3];
    expectedPrefix[4] = '\0';

    const char* p = strstr(response, expectedPrefix);
    if (!p) {
        return false;
    }

    p += 4; // Move past "41XX"
    size_t count = 0;
    while (*p && *(p + 1) && count < *numBytes) {
        char hex[3] = { p[0], p[1], '\0' };
        bytesOut[count++] = (uint8_t)strtol(hex, nullptr, 16);
        p += 2;
    }

    *numBytes = count;
    return count > 0;
}

void ObdClient::processTier1() {
    uint32_t now = millis();
    char resp[64];
    uint8_t bytes[8];

    /* 1. Engine RPM (PID 010C) -> (256 * A + B) / 4 */
    if (sendCommand(OBD_PID_RPM, resp, sizeof(resp), 300)) {
        size_t len = 2;
        if (parsePidResponse(OBD_PID_RPM, resp, bytes, &len) && len >= 2) {
            uint16_t rpm = (uint16_t)((bytes[0] * 256 + bytes[1]) / 4);
            g_vehicleStore.modify([rpm, now](VehicleData& d) {
                d.rpm.set(rpm, now);
            });
        }
    }

    /* 2. Vehicle Speed (PID 010D) -> A km/h */
    if (sendCommand(OBD_PID_SPEED, resp, sizeof(resp), 300)) {
        size_t len = 1;
        if (parsePidResponse(OBD_PID_SPEED, resp, bytes, &len) && len >= 1) {
            uint16_t speed = bytes[0];
            g_vehicleStore.modify([speed, now](VehicleData& d) {
                d.speed.set(speed, now);
            });
        }
    }
}

void ObdClient::processTier2() {
    uint32_t now = millis();
    char resp[64];
    uint8_t bytes[8];

    /* 1. Intake Manifold Absolute Pressure / MAP (PID 010B) -> A kPa */
    if (sendCommand(OBD_PID_MAP, resp, sizeof(resp), 400)) {
        size_t len = 1;
        if (parsePidResponse(OBD_PID_MAP, resp, bytes, &len) && len >= 1) {
            float mapKpa = (float)bytes[0];
            float boostPsi = (mapKpa * 0.145038f) - g_settings.barometricPressurePsi;
            g_vehicleStore.modify([boostPsi, now](VehicleData& d) {
                d.boostPsi.set(boostPsi, now);
            });
        }
    }

    /* 2. Calculated Engine Load (PID 0104) -> A * 100 / 255 % */
    if (sendCommand(OBD_PID_ENGINE_LOAD, resp, sizeof(resp), 400)) {
        size_t len = 1;
        if (parsePidResponse(OBD_PID_ENGINE_LOAD, resp, bytes, &len) && len >= 1) {
            uint8_t load = (uint8_t)((bytes[0] * 100) / 255);
            g_vehicleStore.modify([load, now](VehicleData& d) {
                d.engineLoad.set(load, now);
            });
        }
    }

    /* 3. Battery Voltage via ELM327 ATRV command */
    if (sendCommand(ELM_CMD_VOLTAGE, resp, sizeof(resp), 300)) {
        float volts = atof(resp);
        if (volts > 5.0f && volts < 20.0f) {
            g_vehicleStore.modify([volts, now](VehicleData& d) {
                d.batteryVolts.set(volts, now);
                d.status.batteryWarn = (volts < g_settings.battWarnLow || volts > g_settings.battWarnHigh);
            });
        }
    }

    /* 4. Engine Runtime (PID 011F) if supported */
    if (_runtimeSupported && sendCommand(OBD_PID_RUNTIME, resp, sizeof(resp), 400)) {
        if (strstr(resp, "NODATA") != nullptr) {
            _runtimeSupported = false;
        } else {
            size_t len = 2;
            if (parsePidResponse(OBD_PID_RUNTIME, resp, bytes, &len) && len >= 2) {
                uint32_t rtime = (uint32_t)(bytes[0] * 256 + bytes[1]);
                g_vehicleStore.modify([rtime, now](VehicleData& d) {
                    d.runtimeSec.set(rtime, now);
                });
            }
        }
    }
}

void ObdClient::processTier3() {
    uint32_t now = millis();
    char resp[64];
    uint8_t bytes[8];

    /* 1. Coolant Temperature (PID 0105) -> A - 40 °C */
    if (sendCommand(OBD_PID_COOLANT_TEMP, resp, sizeof(resp), 500)) {
        size_t len = 1;
        if (parsePidResponse(OBD_PID_COOLANT_TEMP, resp, bytes, &len) && len >= 1) {
            int16_t clt = (int16_t)bytes[0] - 40;
            g_vehicleStore.modify([clt, now](VehicleData& d) {
                d.coolantTemp.set(clt, now);
                d.status.coolantWarn = (clt >= g_settings.coolantWarnTemp);
            });
        }
    }

    /* 2. Intake Air Temperature (PID 010F) -> A - 40 °C */
    if (sendCommand(OBD_PID_IAT, resp, sizeof(resp), 500)) {
        size_t len = 1;
        if (parsePidResponse(OBD_PID_IAT, resp, bytes, &len) && len >= 1) {
            int16_t iat = (int16_t)bytes[0] - 40;
            g_vehicleStore.modify([iat, now](VehicleData& d) {
                d.intakeTemp.set(iat, now);
            });
        }
    }

    /* 3. Fuel Tank Level (PID 012F) if supported */
    if (_fuelLevelSupported && sendCommand(OBD_PID_FUEL_LEVEL, resp, sizeof(resp), 500)) {
        if (strstr(resp, "NODATA") != nullptr) {
            _fuelLevelSupported = false;
            g_vehicleStore.modify([](VehicleData& d) { d.fuelLevel.quality = DataQuality::UNSUPPORTED; });
        } else {
            size_t len = 1;
            if (parsePidResponse(OBD_PID_FUEL_LEVEL, resp, bytes, &len) && len >= 1) {
                uint8_t fuel = (uint8_t)((bytes[0] * 100) / 255);
                g_vehicleStore.modify([fuel, now](VehicleData& d) {
                    d.fuelLevel.set(fuel, now);
                    d.status.fuelLowWarn = (fuel <= g_settings.fuelWarnPercent);
                });
            }
        }
    }

    /* 4. Engine Oil Temperature (PID 015C) if supported */
    if (_oilTempSupported && sendCommand(OBD_PID_OIL_TEMP, resp, sizeof(resp), 500)) {
        if (strstr(resp, "NODATA") != nullptr) {
            _oilTempSupported = false;
            g_vehicleStore.modify([](VehicleData& d) { d.oilTemp.quality = DataQuality::UNSUPPORTED; });
        } else {
            size_t len = 1;
            if (parsePidResponse(OBD_PID_OIL_TEMP, resp, bytes, &len) && len >= 1) {
                int16_t oil = (int16_t)bytes[0] - 40;
                g_vehicleStore.modify([oil, now](VehicleData& d) {
                    d.oilTemp.set(oil, now);
                });
            }
        }
    }

    /* 5. Check Engine Light status (PID 0101) */
    if (sendCommand(OBD_PID_STATUS, resp, sizeof(resp), 500)) {
        size_t len = 4;
        if (parsePidResponse(OBD_PID_STATUS, resp, bytes, &len) && len >= 1) {
            bool milActive = (bytes[0] & 0x80) != 0;
            g_vehicleStore.modify([milActive](VehicleData& d) {
                d.status.cel = milActive;
            });
        }
    }
}

void ObdClient::loop() {
    uint32_t now = millis();

    // Message frequency calculation
    if (now - _lastMsgCountTime >= 1000) {
        float rate = (float)_msgCounter * 1000.0f / (float)(now - _lastMsgCountTime);
        _lastMsgCountTime = now;
        _msgCounter = 0;
        g_vehicleStore.modify([rate](VehicleData& d) {
            d.messagesPerSec = rate;
        });
    }

    switch (_state) {
        case ObdState::DISCONNECTED:
            // Idle or auto-reconnect trigger
            break;

        case ObdState::CONNECTING_BT:
            if (_serialBT.connect(_targetBtName.c_str())) {
                Serial.println("[OBD] Bluetooth paired and connected!");
                _state = ObdState::INITIALIZING_ELM;
                _stateEntryTime = now;
            } else if (now - _stateEntryTime > OBD_CONNECT_TIMEOUT_MS) {
                Serial.println("[OBD] BT connection timeout. Retrying...");
                _state = ObdState::CONNECTION_LOST;
                _stateEntryTime = now;
            }
            break;

        case ObdState::INITIALIZING_ELM: {
            char resp[64];
            vTaskDelay(pdMS_TO_TICKS(500));
            sendCommand(ELM_CMD_RESET, resp, sizeof(resp), 1200);
            vTaskDelay(pdMS_TO_TICKS(500));
            sendCommand(ELM_CMD_ECHO_OFF, resp, sizeof(resp), 400);
            sendCommand(ELM_CMD_LINEFEED_OFF, resp, sizeof(resp), 400);
            sendCommand(ELM_CMD_SPACES_OFF, resp, sizeof(resp), 400);
            sendCommand(ELM_CMD_AUTO_PROTOCOL, resp, sizeof(resp), 600);

            _state = ObdState::DETECTING_PROTOCOL;
            _stateEntryTime = now;
            break;
        }

        case ObdState::DETECTING_PROTOCOL: {
            char resp[64];
            if (sendCommand(ELM_CMD_GET_PROTOCOL, resp, sizeof(resp), 1000)) {
                g_vehicleStore.modify([resp](VehicleData& d) {
                    strncpy(d.protocolName, resp, sizeof(d.protocolName) - 1);
                });
            }
            _state = ObdState::QUERYING_SUPPORTED_PIDS;
            _stateEntryTime = now;
            break;
        }

        case ObdState::QUERYING_SUPPORTED_PIDS: {
            char resp[64];
            // Send 0100 to query supported PIDs and verify vehicle ECU response
            if (sendCommand("0100", resp, sizeof(resp), 2000)) {
                if (strstr(resp, "4100") != nullptr) {
                    Serial.println("[OBD] Valid ECU response confirmed! Switching to LIVE mode.");
                    _state = ObdState::CONNECTED_POLLING;
                    g_vehicleStore.modify([](VehicleData& d) {
                        d.ecuConnected = true;
                        d.demoMode = false; // Transition seamlessly to verified LIVE mode
                    });
                } else {
                    Serial.println("[OBD] ECU did not respond to 0100. Retrying protocol search...");
                    _state = ObdState::INITIALIZING_ELM;
                }
            } else if (now - _stateEntryTime > 8000) {
                _state = ObdState::CONNECTION_LOST;
                _stateEntryTime = now;
            }
            break;
        }

        case ObdState::CONNECTED_POLLING: {
            if (!_serialBT.connected()) {
                Serial.println("[OBD] Bluetooth connection dropped!");
                _state = ObdState::CONNECTION_LOST;
                _stateEntryTime = now;
                g_vehicleStore.modify([](VehicleData& d) {
                    d.ecuConnected = false;
                });
                break;
            }

            // Prioritized scheduling
            if (now - _lastQueryTime >= OBD_TIER1_INTERVAL_MS) {
                _lastQueryTime = now;
                processTier1();
            }

            if (now - _lastTier2Time >= OBD_TIER2_INTERVAL_MS) {
                _lastTier2Time = now;
                processTier2();
            }

            if (now - _lastTier3Time >= OBD_TIER3_INTERVAL_MS) {
                _lastTier3Time = now;
                processTier3();
            }
            break;
        }

        case ObdState::CONNECTION_LOST:
            // Non-blocking backoff before attempting auto-reconnect
            if (now - _stateEntryTime > 4000) {
                Serial.println("[OBD] Attempting reconnection...");
                connect();
            }
            break;
    }
}
