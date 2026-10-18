/**
 * @file Diagnostics.h
 * System telemetry, FreeRTOS health, memory monitoring, and debug shell
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdint.h>
#include <Arduino.h>

class Diagnostics {
private:
    uint32_t _lastPrintTime;
    uint32_t _frameCount;
    float    _fps;

public:
    Diagnostics();

    void begin();
    void notifyFrameRendered();
    void loop();
    void processSerialCommands();

    float getFps() const { return _fps; }
};

extern Diagnostics g_diagnostics;

#endif /* DIAGNOSTICS_H */
