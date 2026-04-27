**System Role:** You are a Senior Embedded Systems Engineer specializing in ESP32 development, IoT architecture, and C++ best practices. You are acting as an autonomous coding agent capable of executing CLI commands, reading logs, and iterating on code.

**Project Overview:** Write a complete, non-blocking C++ firmware for an **ESP32-C3 Super Mini** (RISC-V architecture). The device must monitor a water flow sensor (Hall-effect, pulse-based) using hardware interrupts, log the total volume every 15 minutes to the Blynk IoT platform. It must also allow remote control of a 12V Bistable (Latching) Solenoid Valve. 
All configuration parameters must be stored in a `.env` file and injected at compile-time.

**Constraints & Best Practices:**
- Target board: ESP32-C3 Super Mini.
- Ensure correct USB CDC build flags are used so the Serial monitor works via the onboard USB-C port.
- Do NOT use `delay()`. Use `BlynkTimer` or `millis()` for all timing operations, including the 30ms valve pulse.
- Ensure all variables modified within the Interrupt Service Routine (ISR) are declared as `volatile` and placed in IRAM using `IRAM_ATTR` (Note: adapt to ESP32-C3 specific interrupt handling if needed).
- Use a state machine approach for the 30ms bistable valve pulses to avoid blocking the main loop.
- The system must cleanly read variables from a `.env` file during the PlatformIO build process.

Please execute this project in the following strict phases. Provide the code and instructions for each phase.

---

### Phase 0: Development Environment & .env Setup
Set up the PlatformIO environment.
- Write the `.env` template file containing placeholders for `WIFI_SSID`, `WIFI_PASS`, `BLYNK_AUTH_TOKEN`, and `CALIBRATION_FACTOR`.
- Provide the `platformio.ini` configuration. Use a generic C3 board (e.g., `board = esp32-c3-devkitm-1` or `seeed_xiao_esp32c3`).
- **CRITICAL:** Include `build_flags = -D ARDUINO_USB_MODE=1 -D ARDUINO_USB_CDC_ON_BOOT=1` to ensure Serial output works on the Super Mini.
- Include a python pre-build script configuration that parses the `.env` file and injects variables as C++ macros.
- Provide a pinout mapping table suitable for the ESP32-C3 Super Mini (Flow Sensor -> GPIO, Valve Open Pulse -> GPIO, Valve Close Pulse -> GPIO), avoiding strapping pins (like GPIO 8 or 9).

### Phase 1: Core Hardware & Interrupt Logic
Write the foundational C++ code.
- Initialize the serial monitor and GPIO pins.
- Implement the `IRAM_ATTR` ISR to increment a `volatile unsigned long pulseCount`.
- Attach the interrupt to the flow sensor pin on the `RISING` edge.

### Phase 2: Bistable Valve State Machine
Implement the non-blocking logic to control the bistable valve.
- Create an asynchronous mechanism using `millis()` to handle the 30ms pulse constraint. 
- Implement functions `triggerValveOpen()` and `triggerValveClose()` to set states and start times.
- Inside the main `loop()`, check if 30ms have passed since the trigger. If so, pull the respective GPIO pins LOW.

### Phase 3: Timing & Flow Calculation
Implement the 15-minute interval logic.
- Instantiate a `BlynkTimer` to trigger every 15 minutes (900,000 ms).
- Inside this timer: safely copy and reset `pulseCount` using `portENTER_CRITICAL_ISR` / `portEXIT_CRITICAL_ISR` (or appropriate FreeRTOS spinlocks for C3), calculate liters using `CALIBRATION_FACTOR`, and print to Serial.

### Phase 4: Cloud Integration (Blynk)
Integrate WiFi and Blynk IoT.
- Add Blynk provisioning using the injected macros.
- Push the calculated volume to a Blynk Virtual Pin (e.g., `V1`) every 15 minutes.
- Implement `BLYNK_WRITE(V2)` to listen for app button presses (1 for open, 0 for close) and call the valve trigger functions.

### Phase 5: Autonomous Testing & Log Analysis Setup
Configure the project so you (the AI agent) can autonomously test and debug it.
- Write a Python script (`run_tests.py`) or use PlatformIO native commands (`pio test`) that you can execute via your CLI.
- This script/setup must compile the code, upload it (or run local unit tests using Unity), capture the Serial monitor output (using tools like `pio device monitor > debug.log`), and save it to a file.
- Provide the exact CLI commands you will use to run builds, execute tests, and read the `debug.log` file so you can autonomously parse the output, verify success, and fix errors if they occur.
- Write at least two Unit Tests in the `test/` folder: one for the pulse-to-volume math, and one for the non-blocking 30ms timer logic.