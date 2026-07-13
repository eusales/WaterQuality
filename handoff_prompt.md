# Handoff Prompt — Monitor Portátil de Qualidade da Água (ESP32)

## Who I am
Undergraduate student in Engenharia de Controle e Automação at UFMT (Universidade Federal de Mato Grosso), Cuiabá, Brazil. This project is for the course **Instrumentação Industrial II**.

---

## Project Goal
Build a **standalone portable water quality monitor** using an ESP32 DevKit V3, capable of reading pH, TDS, turbidity, and temperature, displaying results on a TFT screen, and logging data to an SD card — all offline, no internet required in Phase 1.

---

## Hardware (already purchased)

| Component | Model | Interface | Notes |
|---|---|---|---|
| Microcontroller | ESP32 DevKit V3 (WROOM-32) | — | 3.3V logic |
| ADC | ADS1115 16-bit | I²C @ 0x48 | Fed at **5V** |
| pH sensor | PH4502C | Analog out | Needs exactly 5V supply |
| TDS sensor | Gravity Analog TDS | Analog out | Max output 2.3V |
| Turbidity sensor | TS-300B | Analog out | Supply 5V |
| Temperature | DS18B20 | 1-Wire | 3.3V, needs 4.7kΩ pull-up |
| Display | TFT 2.0" ST7789 | SPI | 3.3V |
| SD card reader | Micro SD SPI | SPI | 3.3V, FAT32 |
| Level shifter | TXS0108E (4-ch bidirectional) | Digital only | Handles I²C between ADS and ESP32 |

---

## Critical Architecture Decision (SETTLED — do not reopen)

**Signal chain for analog sensors:**
```
Sensor (5V supply, analog out) → ADS1115 (VDD = 5V) → [I²C at 5V] → TXS0108E → [I²C at 3.3V] → ESP32
```

**Why:**
- ADS1115 fed at 5V tolerates analog inputs up to ~5.5V — solves the voltage level problem on the analog side with no resistors needed
- TXS0108E is **digital only** — it sits exclusively on the I²C bus between ADS1115 and ESP32, converting 5V↔3.3V digital signals
- **Never place TXS0108E in the analog signal path** — it distorts analog signals
- Resistor dividers were tested and worked but were discarded (too many components)

---

## Confirmed GPIO Pinout — ESP32 DevKit V3

### SPI (VSPI) — TFT ST7789 + SD Card (shared bus, dedicated CS)
| GPIO | Function |
|---|---|
| 18 | SCK |
| 23 | MOSI |
| 19 | MISO (SD only) |
| 5 | CS — TFT ST7789 |
| 15 | CS — SD Card |
| 2 | DC — TFT (has onboard LED, OK for DC) |
| 4 | RST — TFT |

### I²C — ADS1115 (via TXS0108E)
| GPIO | Function |
|---|---|
| 22 | SCL → TXS0108E A1 → B1 → ADS1115 SCL |
| 21 | SDA → TXS0108E A2 → B2 → ADS1115 SDA |

### 1-Wire — DS18B20
| GPIO | Function |
|---|---|
| 13 | DATA + 4.7kΩ pull-up to 3.3V |

### TXS0108E wiring
| Pin | Connected to |
|---|---|
| VCCA | 3.3V |
| VCCB | 5V (Vin ESP32) |
| GND | Common GND |
| OE | 3.3V (always enabled) |
| A1 | GPIO 22 (SCL ESP32) |
| B1 | SCL ADS1115 |
| A2 | GPIO 21 (SDA ESP32) |
| B2 | SDA ADS1115 |

### ADS1115 channel map
| Channel | Sensor |
|---|---|
| AIN0 | Turbidity TS-300B |
| AIN1 | TDS Gravity |
| AIN2 | pH PH4502C |
| AIN3 | Reserved |

### GPIOs to NEVER use
- **GPIO 6–11**: connected to internal flash — will crash ESP32
- **GPIO 34–39**: input only, no pull-up
- **GPIO 0**: boot button
- **GPIO 1/3**: TX0/RX0 (USB serial)

---

## Calibration (defined, not yet implemented in firmware)

### pH — 3-point calibration
```cpp
slope_acid  = (7.0 - 4.0)  / (V7 - V4);
slope_basic = (7.0 - 10.0) / (V7 - V10);
pH = (voltage < V7) ? slope_acid*(voltage-offset)+7 : slope_basic*(voltage-offset)+7;
```

### TDS — 1-point + temperature compensation
```cpp
EC25    = EC_raw / (1.0 + 0.02 * (temp - 25.0));
k       = 1413.0 / EC25;                          // calibration factor
TDS_ppm = (EC_raw * k / (1.0 + 0.02*(temp-25.0))) / 2.0;
```

### Turbidity — empirical polynomial regression
```cpp
NTU = a*V*V + b*V + c;   // coefficients from lab measurement
```

---

## Software Plan — Phase 1 MVP (offline, standalone)

Four states navigated via TFT menu:
1. **Monitoring** — real-time readings every 2s, visual alerts (red) when out of range
2. **Manual log** — snapshot to SD as CSV with timestamp and sample label
3. **Calibration** — step-by-step guided flow per sensor, coefficients saved to NVS
4. **Settings** — alert limits, read interval, log filename

**Storage:**
- `calibration.json` → NVS/SPIFFS (survives SD card swap)
- `water_log.csv` → SD card, append mode: `timestamp, label, pH, TDS_ppm, turbidity_NTU, temperature_C`

**Phase 2 (later):** activate ESP32 Wi-Fi, run local HTTP server, `GET /readings` returns JSON, `POST /log` triggers snapshot — no cloud, LAN only.

---

## What was already delivered (documents)

1. **`contexto_projeto_qualidade_agua.md`** — full technical context document
2. **`relatorio_testes_sensores.docx`** — complete sensor test report including:
   - Individual test for each sensor (turbidity first, then TDS, then pH)
   - Integrated test (all 3 simultaneously on ADS1115)
   - Pinout tables, calibration procedures, result tables to fill in
   - Full Arduino/ESP32 test code for each test (Serial Monitor only, no display)
   - Required libraries: Adafruit ADS1X15 ≥2.5.0, OneWire ≥2.3.7, DallasTemperature ≥3.9.0

---

## What to avoid
- Do NOT suggest placing TXS0108E in the analog signal path
- Do NOT suggest using GPIO 6–11
- Do NOT suggest connecting analog sensor outputs directly to ESP32 ADC pins (noisy and non-linear)
- Do NOT suggest cloud connectivity in Phase 1
- Do NOT reopen the voltage architecture discussion — it is settled

---

## Very Next Step

**Write the Phase 1 MVP firmware** structured as follows:

1. Initialize all peripherals (SPI: TFT + SD; I²C: ADS1115; 1-Wire: DS18B20)
2. Load calibration coefficients from NVS on boot
3. Main loop with 2s read cycle: ADS1115 → apply calibration → update display
4. Menu navigation on TFT ST7789 (library: TFT_eSPI)
5. Manual log trigger → write CSV row to SD
6. Calibration flow per sensor with NVS save
7. Visual alerts (red values) when readings exceed configured limits

**Start with step 1** — peripheral initialization and a bare-bones "all sensors reading" sketch that prints to Serial Monitor, then add TFT and menu on top.
