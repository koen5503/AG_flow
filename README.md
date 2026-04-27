# ESP32-C3 Water Flow & Bistable Valve Controller

Firmware voor een **ESP32-C3 Super Mini** die een waterflow-sensor uitleest via hardware-interrupts, het volume elke 15 minuten naar **Blynk IoT** stuurt, en een **12V bistable (latching) magneetklep** op afstand kan bedienen.

---

## Projectstructuur

```
AG_flow/
├── src/
│   └── main.cpp              # Volledige firmware (alle 4 phases)
├── test/
│   ├── test_math/
│   │   └── test_math.cpp      # Unit test: pulse-naar-volume berekening
│   └── test_timer/
│       └── test_timer.cpp     # Unit test: 30ms non-blocking valve logica
├── platformio.ini             # PlatformIO configuratie (ESP32-C3 + native tests)
├── inject_env.py              # Pre-build script: leest .env en injecteert als C++ macros
├── run_tests.py               # Autonoom test-script (Phase 5)
├── .env.example               # Template voor credentials
├── .env                       # Jouw credentials (NIET in git)
├── .gitignore
├── opdracht.md                # Originele opdracht / prompt
└── README.md                  # Dit bestand
```

---

## Hardware: Pinout ESP32-C3 Super Mini

| Component            | GPIO | Richting | Opmerkingen                        |
| -------------------- | ---- | -------- | ---------------------------------- |
| Flow Sensor (Hall)   | 3    | INPUT    | Interrupt op RISING edge           |
| Valve OPEN pulse     | 4    | OUTPUT   | 30ms HIGH pulse opent de klep      |
| Valve CLOSE pulse    | 5    | OUTPUT   | 30ms HIGH pulse sluit de klep      |

> **Let op:** GPIO 8 en 9 zijn strapping pins op de ESP32-C3 en worden bewust vermeden.

---

## Snel starten

### 1. Kloon en maak je `.env` bestand

```bash
cp .env.example .env
```

Vul de juiste waarden in:

```env
WIFI_SSID="JouwNetwerk"
WIFI_PASS="JouwWachtwoord"
BLYNK_AUTH_TOKEN="JouwBlynkToken"
CALIBRATION_FACTOR=5.5
BLYNK_TEMPLATE_ID="TMPLxxxxxxx"
BLYNK_TEMPLATE_NAME="Jouw Template Naam"
```

> De `BLYNK_TEMPLATE_ID` en `BLYNK_TEMPLATE_NAME` vind je in de Blynk Console bij je template. Deze zijn **verplicht** voor de Blynk library v1.3+.

### 2. Installeer PlatformIO

```bash
pip install platformio
```

### 3. Compileer en upload

```bash
# Alleen compileren (geen board nodig)
python -m platformio run -e seeed_xiao_esp32c3

# Compileren + uploaden naar het board op COM3 (Windows) of /dev/cu.usbmodem* (macOS)
python -m platformio run -e seeed_xiao_esp32c3 --target upload --upload-port COM3
```

### 4. Seriële monitor

```bash
python -m platformio device monitor --port COM3 --baud 115200
```

---

## Unit Tests draaien

De tests draaien **native** op je computer (niet op het ESP32-board). Hiervoor heb je een C/C++ compiler nodig.

### Vereisten per OS

| OS      | Installatie compiler                                                   |
| ------- | ---------------------------------------------------------------------- |
| Windows | `winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT`             |
| macOS   | `xcode-select --install`                                               |
| Linux   | `sudo apt install build-essential`                                     |

> **Windows-specifiek:** Na installatie van MinGW moet je een nieuw terminal-venster openen zodat `gcc` in het PATH staat.

### Tests uitvoeren

```bash
python -m platformio test -e native
```

Verwachte output:

```
test_math    PASSED
test_timer   PASSED
2 test cases: 2 succeeded
```

---

## Autonoom testscript (`run_tests.py`)

Dit script voert alles in één keer uit: unit tests, compilatie, upload en serial-log capture.

```bash
# Alleen tests + compileren
python run_tests.py

# Tests + compileren + uploaden + seriële log vangen
python run_tests.py --upload

# Alleen seriële monitor (30 seconden)
python run_tests.py --monitor

# Met aangepaste poort en duur
python run_tests.py --upload --port COM3 --duration 30
```

Alle output wordt opgeslagen in `debug.log`.

---

## Architectuur / Design Decisions

### Non-blocking design

- **Geen `delay()` in `loop()`**: Alle timing gaat via `millis()` en `BlynkTimer`.
- **Bistable valve state machine**: De 30ms puls naar de magneetklep wordt asynchroon afgehandeld via een state machine (`VALVE_IDLE → VALVE_OPENING/CLOSING → VALVE_IDLE`).
- **WiFi/Blynk non-blocking**: `WiFi.begin()` + `Blynk.config()` + `Blynk.connect(timeout)` wordt gebruikt in plaats van het blokkerende `Blynk.begin()`. Dit voorkomt dat de firmware vastloopt als WiFi niet bereikbaar is.

### Interrupt-veiligheid

- De `pulseCount` variabele is `volatile` en wordt beschermd met `portENTER_CRITICAL_ISR`/`portEXIT_CRITICAL_ISR` (FreeRTOS spinlocks).
- De ISR is gemarkeerd met `IRAM_ATTR` zodat deze vanuit IRAM wordt uitgevoerd.

### .env injectie

Het pre-build script `inject_env.py` leest het `.env` bestand en voegt elke key-value pair toe als C++ `-D` macro via PlatformIO's `CPPDEFINES`. String-waarden worden correct ge-escaped met backslash-quotes.

### USB Serial op ESP32-C3

De ESP32-C3 Super Mini heeft **geen** aparte USB-UART chip (zoals CH340). Serial output gaat via de ingebouwde **USB-Serial/JTAG** controller. Dit werkt doordat:

1. Het board-bestand (`seeed_xiao_esp32c3`) de flags `ARDUINO_USB_MODE=1` en `ARDUINO_USB_CDC_ON_BOOT=1` al bevat.
2. Hierdoor wijst `Serial` naar het `HWCDC` object (hardware USB CDC).

**Bekend gedrag:** Na een upload of reset verdwijnt de COM-poort even (USB re-enumeratie). De seriële monitor vangt daardoor niet altijd de allereerste boot-berichten op. Het `run_tests.py` script lost dit op door een DTR-reset te sturen na het openen van de poort.

---

## Blynk Virtual Pins

| Pin | Richting  | Functie                                               |
| --- | --------- | ----------------------------------------------------- |
| V1  | Write     | Volume in liters (elke 15 minuten gepusht)            |
| V2  | Read      | Valve control: `1` = open, `0` = sluit                |

---

## Veelvoorkomende problemen

### `pio` / `platformio` niet gevonden
```bash
pip install platformio
# Gebruik daarna: python -m platformio <command>
```

### `gcc` niet gevonden (bij `pio test -e native`)
Installeer een C/C++ compiler — zie tabel hierboven onder "Unit Tests draaien".

### Geen seriële output na upload
- Koppel de USB-kabel los en weer aan (USB re-enumeratie na reset).
- Gebruik `run_tests.py --monitor` — dit script stuurt automatisch een DTR-reset.
- Controleer of de juiste COM-poort is geselecteerd (`python -m platformio device list`).

### `#error "Please specify your BLYNK_TEMPLATE_ID and BLYNK_TEMPLATE_NAME"`
Voeg `BLYNK_TEMPLATE_ID` en `BLYNK_TEMPLATE_NAME` toe aan je `.env` bestand. Deze zijn verplicht voor Blynk v1.3+.

### `.env` syntax fouten
- Strings **moeten** aanhalingstekens hebben: `WIFI_SSID="mijnnetwerk"`
- Getallen **geen** aanhalingstekens: `CALIBRATION_FACTOR=5.5`
- Geen spaties rond het `=` teken.
- Precies één waarde per regel.

---

## CLI-referentie (voor AI-agents)

```bash
# Detecteer aangesloten board
python -m platformio device list

# Unit tests draaien (vereist gcc)
python -m platformio test -e native

# Firmware compileren (geen board nodig)
python -m platformio run -e seeed_xiao_esp32c3

# Firmware uploaden
python -m platformio run -e seeed_xiao_esp32c3 --target upload --upload-port COM3

# Seriële monitor
python -m platformio device monitor --port COM3 --baud 115200

# Alles-in-één: test + build + upload + serial log
python run_tests.py --upload --port COM3 --duration 30

# Serial log opvangen via Python (wanneer pio monitor niet werkt)
python -c "
import serial, time
s = serial.Serial('COM3', 115200, timeout=1)
s.dtr = False; s.rts = True; time.sleep(0.1)
s.rts = False; time.sleep(0.1); s.dtr = True
time.sleep(0.5); s.reset_input_buffer()
start = time.time()
while time.time() - start < 30:
    data = s.readline()
    if data: print(data.decode('utf-8', errors='replace').strip())
s.close()
"
```
