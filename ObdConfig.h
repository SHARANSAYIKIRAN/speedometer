/**
 * @file ObdConfig.h
 * ELM327 OBD-II parameters, PID commands, and connection configuration
 */

#ifndef OBD_CONFIG_H
#define OBD_CONFIG_H

#include <stdint.h>

/* Bluetooth Connection Configuration */
#define DEFAULT_OBD_BT_NAME     "OBDII"     // Common ELM327 names: "OBDII", "ELM327", "OBD2"
#define DEFAULT_OBD_BT_PIN      "1234"      // Typical PINs: "1234" or "0000"

/* Standard Mode 01 PIDs */
#define OBD_PID_STATUS          "0101"      // Monitor status / Check Engine Light
#define OBD_PID_ENGINE_LOAD     "0104"      // Calculated engine load
#define OBD_PID_COOLANT_TEMP    "0105"      // Engine coolant temp
#define OBD_PID_MAP             "010B"      // Intake manifold absolute pressure (kPa)
#define OBD_PID_RPM             "010C"      // Engine RPM
#define OBD_PID_SPEED           "010D"      // Vehicle speed (km/h)
#define OBD_PID_IAT             "010F"      // Intake air temp
#define OBD_PID_RUNTIME         "011F"      // Run time since engine start
#define OBD_PID_FUEL_LEVEL      "012F"      // Fuel tank level input (%)
#define OBD_PID_OIL_TEMP        "015C"      // Engine oil temp (°C)

/* ELM327 AT Commands */
#define ELM_CMD_RESET           "ATZ"       // Reset adapter
#define ELM_CMD_ECHO_OFF        "ATE0"      // Echo off
#define ELM_CMD_LINEFEED_OFF    "ATL0"      // Linefeeds off
#define ELM_CMD_SPACES_OFF      "ATS0"      // Spaces off
#define ELM_CMD_AUTO_PROTOCOL   "ATSP0"     // Auto-detect protocol (ISO 9141-2, KWP, etc.)
#define ELM_CMD_GET_PROTOCOL    "ATDP"      // Display protocol description
#define ELM_CMD_VOLTAGE         "ATRV"      // Read battery voltage from ELM327 ADC

/* Polling Intervals in milliseconds */
#define OBD_TIER1_INTERVAL_MS   120         // RPM & Speed (High priority)
#define OBD_TIER2_INTERVAL_MS   1500        // Boost, Load, Battery
#define OBD_TIER3_INTERVAL_MS   4000        // Coolant, IAT, Fuel, Oil

/* Communication Timeouts */
#define OBD_CONNECT_TIMEOUT_MS  12000       // Max time to wait for Bluetooth pairing
#define OBD_RESPONSE_TIMEOUT_MS 1500        // Max response time per query (slow K-Line support)

#endif /* OBD_CONFIG_H */
