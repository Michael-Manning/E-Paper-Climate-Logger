# E-Paper Climate Logger Firmware

## Overview

The firmware runs on an ESP32-S3 microcontroller and is built using PlatformIO with the Arduino framework. It reads temperature and humidity from an SHT45 sensor, timestamps each reading with a DS3231 real-time clock, stores records in a 24LC512 EEPROM, monitors battery status via a BQ27441 fuel gauge, and displays information on an e-paper screen. The system is designed for low-power operation, entering deep sleep between measurement intervals and waking via the RTC alarm or user buttons.

## Getting Started

### Prerequisites

- PlatformIO Core (installed via pip or standalone installer)
- USB-C cable for programming and power
- Assembled E-Paper Climate Logger hardware

### Dependencies

All required libraries are specified in `platformio.ini` and will be fetched automatically by PlatformIO. The main dependencies include:

- GxEPD2 (e-paper display driver)

The Arduino framework and ESP32-S3 board definition are also handled automatically.

### Building and Flashing

1. Clone the repository and navigate to the firmware directory.
2. Connect the device via USB-C.
3. Build and upload:

   ```
   pio run --target upload
   ```

4. Monitor serial output (optional):

   ```
   pio device monitor
   ```

The `platformio.ini` file already configures the correct upload port (`/dev/ttyACM0` by default) and baud rate (115200). Adjust `monitor_port` and `upload_port` if necessary.

> `extra_script.py` runs before each build; it can be used to inject the compilation timestamp into firmware for RTC initialisation when no battery backup is present.

## Folder Structure

```
firmware/
├── src/
│   ├── drivers/               # Low-level hardware drivers
│   │   ├── BQ27441.cpp/h      # Battery fuel gauge
│   │   ├── BQ27441_Definitions.h
│   │   ├── DS3231.cpp/h       # Real-time clock
│   │   ├── EEPROM.cpp/h       # 24LC512 external EEPROM
│   │   ├── SHT45.cpp/h        # Temperature/humidity sensor
│   │   └── display.cpp/h      # E-paper display (GxEPD2 wrapper)
│   ├── Constants.h            # System-wide constants (sleep intervals, etc.)
│   ├── MAssert.h              # Debug assertions
│   ├── app.cpp/h              # Application state machine and main logic
│   ├── bitmaps.h              # Image data for the display
│   ├── buttons.cpp/h          # Debounced button handling
│   ├── debugLog.h             # Logging macros
│   ├── main.cpp               # Entry point, setup() and loop()
│   ├── pinout.h               # GPIO pin mapping
│   ├── platform.cpp/h         # Hardware abstraction (initialization)
│   ├── power.cpp/h            # Sleep management and wake sources
│   └── timer_setter.h         # Alarm configuration for DS3231
├── extra_script.py            # PlatformIO pre-build script
├── platformio.ini             # Build configuration
└── README.md
```

## Hardware Connections

The following diagram illustrates the connections between the ESP32-S3 and peripherals. All I2C devices share the same bus (SDA/SCL); SPI is used only for the e-paper display.

![simple-schematic](../img/simple-schematic.png)

Pin mappings are defined in `src/pinout.h`.

## General Operation

The firmware executes a simple state machine:

1. **Initialization**  
   - All peripherals (I2C, SPI, GPIO) are configured.  
   - The e-paper display shows a startup screen.  
   - The RTC is read; if the time is invalid, compilation timestamp is used as fallback.

2. **Main Loop**  
   - After initialisation, the device enters deep sleep with a wake-up timer set via the DS3231 alarm.  
   - The power button (via LTC2954) also generates a system wake event when the device is in deep sleep, not just when fully off.
   - Wake-up sources: RTC alarm or button press (menu or power).  
   - Upon wake, the following occurs:  
        - Sensor readings (SHT45) are taken.  
        - Battery level is read from BQ27441.  
        - The reading is stored in the external EEPROM (24LC512) with a timestamp.  
        - The e-paper display is refreshed with the latest data.  
   - The device then returns to deep sleep.

3. **Button Handling**  
   - Menu button (GPIO2): cycles through display views (current, history, battery).  
   - Power button (GPIO14): forces an immediate measurement and display update.

## Operational Notes

### Low-Power Design

- The ESP32-S3 enters deep sleep between logging intervals (default: every 1 minute). Controlled by the DS3231 alarm. This interval can be changed by editing Constants.h and recompiling. 
- Deep sleep current is minimised by disabling unused peripherals.  
- The DS3231 generates a periodic alarm on its INT pin (GPIO21) to wake the MCU.  
- Button wake is enabled via GPIO interrupts.  
- The display is updated only when new data is available or upon user request; no continuous screen refresh occurs.
- The device consumes the majority of its energy during the brief wake period (approximately 2 seconds at 50 mA). Deep sleep current is less than 20 µA. Therefore, reducing the wake interval (e.g., to 5 minutes) will proportionally extend battery life, while a shorter interval (e.g., 30 seconds) will reduce it.

### Logging and Storage

- Data records are stored as binary structs in the 24LC512 EEPROM.  
- The storage implements a circular buffer: when the EEPROM is full, the oldest record is overwritten.  
- A small header area stores the current write index and total record count.  
- Records can be retrieved via serial (debug) or displayed on the e-paper screen in history mode.
- Upon next power‑on, the firmware reads a flag to determine whether it is resuming from deep sleep or from a complete power‑off. This allows proper initialisation of the real‑time clock, display, and user interface state.

### Temperature Sensor Accuracy

The ESP32‑S3 generates heat during wake‑up and active operation. This heat can conduct through the PCB and affect the SHT45 reading, artificially raising the measured temperature. To mitigate this error, the firmware:

- Reads the sensor immediately upon waking, before any other processing.
- Completes the measurement within the first few milliseconds of wake‑up.
- Avoids back‑to‑back readings during user interaction (e.g., menu navigation), as holding the device or prolonged wake‑up will skew the ambient reading for several minutes.

For the most accurate ambient logging, avoid handling the device or placing it in direct sunlight while it is awake.

### Display Low‑Temperature Limit

The e‑paper display module is rated for operation only above 0°C. When the ambient temperature falls below freezing:

- The device continues to log temperature and humidity to EEPROM.
- The screen may become sluggish, fail to update, or show artefacts.
- Once the temperature rises above 0°C, normal display operation resumes.
- This is a hardware limitation of the display, not a firmware bug.

### Power Management Hardware

The system includes an LTC2954 soft power button controller and a BQ24075 power path IC. The LTC2954:

- Runs directly from the battery with < 1 µA quiescent current.
- Monitors the physical power button (connected to its `PB_IN`).
- Toggles the enable line of the power path IC to completely disconnect or restore battery power.
- Provides a button state output (GPIO14) to the ESP32.
- Accepts a `KILL` input (GPIO37) from the ESP32 to trigger a software shutdown.

## Building Custom Hardware

If you are building a variant of the hardware, update `pinout.h` to match your GPIO assignments. The power management logic (charge status, USB detect, power kill) can be adjusted in `power.cpp` and `platform.cpp`.

## Troubleshooting

- **Device does not wake:** Check the DS3231 alarm configuration and GPIO21 connection.  
- **E-paper shows garbage:** Verify SPI pins and reset sequence in `display.cpp`.  
- **I2C communication fails:** Confirm pull-up resistors and address settings in each driver file.  
- **Battery percentage inaccurate:** The BQ27441 requires a one-time capacity configuration; refer to the datasheet and adjust `BQ27441.cpp`.

---

This README focuses on the firmware as it exists in the repository, using PlatformIO and Arduino. For hardware schematics and PCB design, refer to the `PCB` folder at the root of the project.