# RallyDash — ESP32 Real-Time Automotive Digital Cluster

A high-performance, rally-inspired digital instrument cluster firmware for the ESP32 microcontroller, designed to render directly onto a physical SPI TFT display (ILI9341 / ST7789) and display real-time telemetry from an automotive ECU via an ELM327 Bluetooth OBD-II adapter.

---

## 1. Visual Hierarchy & Design

RallyDash matches the reference digital rally cluster layout:

```
+-------------------------------------------------------------------------+
|  N   CEL OIL       •••• ••• ••• (Shift)         0     1542              |
|      CLT GAS                                  km/h      RPM             |
|      BAT AFR        ____------------------------------------------+     |
|              _ - '   1    2    3    4    5    6    7 (Redline)    |     |
|          _ -'       +----------+ +----------+ +----------+       |     |
|                     |   -7.1   | |   13.9   | |   0:01   |       |     |
|                     | BOOST PSI| |   AFR    | | RUN TIME |       |     |
|                     +----------+ +----------+ +----------+       |     |
|                                                                         |
|  +----+  +----+  +----+  +----+  +----+  +----------+                   |
|  | 44 |  | 31 |  | 31 |  | 39 |  |14.2|  |   [OK]   |                   |
|  |COOL|  |OIL |  |INTK|  |FUEL%| |BAT V|  |   ECU    |                   |
|  +----+  +----+  +----+  +----+  +----+  +----------+                   |
|                                                                         |
| LIVE - 155 MSG/S                                              KM/H - °C |
+-------------------------------------------------------------------------+
```

### Color Palette
- **Electric Cobalt Racing Blue Gradient Background**: `#0C366E` to `#071C3D`
- **Gear Indicator**: Vivid Floating Neon Cyan (`#00E5FF`) in 40pt font
- **Shift Lights**: 10 LEDs (Green `#00E676`, Yellow `#FFD600`, Red `#FF1744`)
- **Primary Figures (Speed & RPM)**: Crisp Pure White (`#FFFFFF`) in 32pt font
- **Speed & RPM Units**: Warm Amber/Gold (`#FFC107`) in 10pt font
- **Swept Tachometer**: Pure White Progress Fill (`#FFFFFF`), Dark Slate Track (`#13253F`), Dark Crimson Redline (`#5A121C`)
- **Status Warnings (CEL, CLT, BAT, OIL, GAS, AFR)**: 2-column muted steel cyan (`#386E94`), Red/Amber on trigger
- **ECU Status Badge**: Bright Neon Green (`#00E676`) with dark `OK` text
- **Footer Bar**: Solid Vivid Electric Royal Blue (`#0055D4`) with `LIVE - 155 MSG/S` and `KM/H - °C`

---

## 2. Hardware Architecture & Pinout

### Confirmed Hardware Configuration
- **MCU**: ESP32-WROOM-32 (Standard Dual-Core Xtensa LX6 @ 240MHz, 520KB SRAM, hardware Bluetooth Classic SPP)
- **Display**: 2.8" 320x240 SPI TFT (ILI9341 or ST7789 controller)
- **Adapter**: Standard Bluetooth Classic ELM327 v1.5 / v2.1 Mini OBD-II adapter
- **Vehicle Protocol**: Older OBD-II (ISO 9141-2 / ISO 14230-4 KWP2000 / J1850 PWM/VPW) and modern CAN (ISO 15765-4)

### SPI TFT Wiring Diagram (ESP32-WROOM-32 to 2.8" SPI TFT)

| Display Pin (TFT) | ESP32 GPIO Pin | Function | Description |
| :--- | :--- | :--- | :--- |
| **VCC** | **3.3V** (or 5V if module has onboard 3.3V LDO) | Power | Module VCC supply |
| **GND** | **GND** | Ground | Common system ground |
| **CS** | **GPIO 5** | SPI Chip Select | Display select line |
| **RESET / RST** | **GPIO 4** | Display Reset | Hardware reset |
| **DC / RS** | **GPIO 2** | Data / Command | Register select |
| **MOSI / SDI** | **GPIO 23** | SPI MOSI (VSPI) | Hardware SPI Data Out |
| **SCK / CLK** | **GPIO 18** | SPI Clock (VSPI) | Hardware SPI Clock (40MHz DMA) |
| **LED / BL** | **GPIO 21** | Backlight PWM | Screen brightness control (0-255) |
| **MISO / SDO** | **GPIO 19** | SPI MISO (Optional)| Display read line |

> [!IMPORTANT]
> Some cheap 2.8" red-PCB TFT modules require VCC to be connected to 5V (due to an onboard AMS1117 3.3V regulator), while the logic pins (CS, DC, MOSI, SCK) must be 3.3V compatible. Always check your specific PCB silkscreen.

---

## 3. Firmware Architecture & Dual-Core FreeRTOS

```
                 +-----------------------------------------------+
                 |              ESP32 Dual-Core CPU              |
                 +-----------------------+-----------------------+
                                         |
                   CORE 1                |                CORE 0
          (User Interface & Display)     |      (Telemetry & Communication)
         +--------------------------+    |    +-----------------------------+
         |     LVGL 8.4 Engine      |    |    |      ObdClient (ELM327)     |
         |  - Display flush (DMA)   |    |    |  - Bluetooth Classic SPP    |
         |  - Arc sweep animation   |    |    |  - Prioritized PID Queue    |
         |  - DashboardUI widgets   |    |    |  - Auto-reconnect engine    |
         +-------------+------------+    |    +--------------+--------------+
                       |                 |                   |
                       +------------+    |    +--------------+
                                    |    |    |
                                    v    v    v
                           +------------------------+
                           |    VehicleDataStore    |
                           |   (Mutex-Protected)    |
                           +------------------------+
                                         |
                                         v
                         +-------------------------------+
                         |      DEMO Physics Engine      |
                         |  - Throttle & Boost Sweep     |
                         |  - Gear Shift Simulator       |
                         |  - Coolant Warmup Curves      |
                         +-------------------------------+
```

### Core Execution Isolation
- **Core 1 (`uiTask`)**: Dedicated solely to rendering the LVGL UI and flushing pixels via 40MHz DMA. Maintains a smooth 40–60 FPS refresh rate.
- **Core 0 (`backendTask`)**: Dedicated to Bluetooth communication with the ELM327, command transmission, response parsing, and error recovery. Even if an older ECU takes 300ms to respond over ISO 9141-2 K-Line, **the UI never stutters or drops frames**.
- **Data Safety**: All shared telemetry is mediated through a FreeRTOS mutex inside `VehicleDataStore`.

---

## 4. Multi-Tier Prioritized OBD-II Polling

Older OBD-II protocols (1996–2007) communicate at 10.4 kbps baud rates, limiting message throughput to 5–15 queries/second. RallyDash solves this using a **3-tier prioritized polling scheduler**:

| Priority Tier | Interval | Parameters | OBD PID | Response Formula |
| :--- | :--- | :--- | :--- | :--- |
| **Tier 1 (High Rate)** | ~120 ms | **Engine RPM**<br>**Vehicle Speed** | `010C`<br>`010D` | `(256 * A + B) / 4`<br>`A` km/h |
| **Tier 2 (Medium Rate)** | ~1500 ms | **Manifold Pressure (MAP)**<br>**Calculated Engine Load**<br>**Battery Voltage**<br>**Engine Runtime** | `010B`<br>`0104`<br>`ATRV`<br>`011F` | `Boost = (MAP * 0.145) - 14.7 PSI`<br>`A * 100 / 255 %`<br>`Direct ADC Float (V)`<br>`256 * A + B (sec)` |
| **Tier 3 (Low Rate)** | ~4000 ms | **Engine Coolant Temp**<br>**Intake Air Temp (IAT)**<br>**Fuel Tank Level**<br>**Engine Oil Temp**<br>**Check Engine Light (MIL)** | `0105`<br>`010F`<br>`012F`<br>`015C`<br>`0101` | `A - 40 °C`<br>`A - 40 °C`<br>`A * 100 / 255 %`<br>`A - 40 °C`<br>`Bit 7 of Byte A` |

> [!TIP]
> **Unsupported PID Pruning**: If an older vehicle's ECU does not support a specific PID (such as Oil Temp `015C` or Fuel Level `012F`), the ECU returns `NO DATA`. The `ObdClient` detects this, marks the metric as `DataQuality::UNSUPPORTED`, and **permanently removes it from the polling cycle**, maximizing bandwidth for RPM and Speed!

---

## 5. Getting Started & Build Instructions

### Option A: Building with PlatformIO (Recommended)

1. Open a terminal in the project root:
   ```bash
   cd /home/babayogi/Projects/speedometer
   ```
2. Build the firmware:
   ```bash
   pio run
   ```
3. Connect your ESP32 via USB and upload:
   ```bash
   pio run --target upload
   ```
4. Open the serial monitor:
   ```bash
   pio device monitor -b 115200
   ```

### Option B: Building with Arduino IDE 2.x

1. Install **ESP32 Board Support** in Arduino IDE (Boards Manager -> search `esp32` by Espressif Systems).
2. Install Required Libraries via Library Manager (`Ctrl+Shift+I`):
   - **`lvgl`** by kisvegabor (select version **8.4.0**)
   - **`LovyanGFX`** by lovyan03 (version **1.1.12** or higher)
3. Copy `lv_conf.h` to your Arduino libraries directory (usually `~/Arduino/libraries/lv_conf.h` or right inside `~/Arduino/libraries/lvgl/`).
4. In Arduino IDE, open `RallyDash/RallyDash.ino`.
5. Select:
   - **Board**: `ESP32 Dev Module`
   - **Partition Scheme**: `Minimal SPIFFS (Large APPS with OTA)` or `Huge APP`
   - **Upload Speed**: `921600`
   - **CPU Frequency**: `240MHz`
6. Click **Upload**.

---

## 6. Testing & Operational Verification

### DEMO Mode Desk Testing (No Vehicle Required)
By default, RallyDash boots immediately into **DEMO mode** so you can develop and test on your desk.

The simulation engine automatically replicates:
1. **Engine Idle in Neutral ('N')**: 850 RPM, 0 km/h, -10.5 PSI vacuum, 14.7 AFR.
2. **1st Gear Pull ('1')**: Revs to 5600 RPM, boost ramps to +11.5 PSI, speed reaches 45 km/h.
3. **2nd Gear Shift ('2')**: RPM drops to 3200, pulls to 6000 RPM, boost peaks at +14.5 PSI.
4. **3rd Gear Pull ('3')**: Accelerates to 124 km/h with 14.2 PSI boost (matching the visual reference).
5. **4th Gear Cruise ('4')**: Cruises at 124 km/h, boost returns to light vacuum, AFR stabilizes at 14.7.
6. **Braking / Deceleration**: Downshifts through gears, engine overrun vacuum, returns to Neutral.
7. **Coolant Warmup**: Realistic curve from 68°C to 88°C operating temperature.

### Interactive Serial Debug Shell
Connect the ESP32 to your PC via USB and send any of these single-character commands over 115200 baud Serial:

| Key | Action | Output Description |
| :---: | :--- | :--- |
| **`d`** | Toggle Mode | Switches between `[ DEMO MODE ]` and `[ LIVE ECU ]` |
| **`u`** | Toggle Units | Switches units between `KM/H` and `MPH` |
| **`b`** | Reconnect OBD | Triggers Bluetooth connection attempt to ELM327 |
| **`w`** | Toggle Warnings | Simulates Check Engine Light (CEL) and Overheating alerts |
| **`s`** | System Health | Prints FPS, Free Internal Heap, and Minimum Heap to serial |

---

## 7. Connecting to a Real Vehicle (LIVE Mode)

1. Plug your ELM327 Bluetooth adapter into your car's OBD-II port (located beneath the steering column).
2. Turn your vehicle's ignition to **ON** (engine running or position II).
3. The ELM327 will broadcast as `"OBDII"`, `"ELM327"`, or `"OBD2"`.
4. In `include/ObdConfig.h`, verify `DEFAULT_OBD_BT_NAME` matches your dongle name (default is `"OBDII"` with PIN `"1234"`).
5. Press `'b'` in the serial monitor or restart the ESP32.
6. The cluster will:
   - Pair via Bluetooth Classic SPP.
   - Send `ATZ`, `ATE0`, `ATS0`, `ATSP0`.
   - Automatically detect the vehicle's protocol (e.g., `ISO 9141-2`, `KWP2000`, `CAN`).
   - Query PID `0100` to verify live ECU communication.
   - As soon as the ECU responds, the footer switches to **`[ LIVE ECU ]`** in neon green, and real vehicle telemetry streams to the cluster!

---

## 8. Automotive Power & Safety Guidelines

- **Auxiliary Display Notice**: RallyDash is an auxiliary telemetry display and does not replace factory safety warnings or instrument clusters.
- **Read-Only OBD Commands**: The firmware strictly issues Mode 01 read queries. It never writes to ECU registers or clears DTC codes.
- **Power Supply**: Automotive electrical systems experience load-dump spikes up to 40V. Power the ESP32 using an automotive-rated 12V-to-5V step-down buck converter (e.g., based on LM2596HV or MP1584 with reverse-polarity diode and TVS diode protection). Never connect raw 12V vehicle power directly to the ESP32.
