# 🪐 KAWKAB — كوكب
### Planet Encyclopedia Embedded Card Reader

> An AI-enhanced educational embedded system with analog card sensing, multi-sensor acquisition, and real-time PC dashboard.  
> Built for **ECCE4227: Embedded Systems** · Sultan Qaboos University · Spring 2026

---

## ✨ What is KAWKAB?

KAWKAB (Arabic: كوكب, meaning *"planet"*) is a microcontroller-based interactive educational device. Insert a physical planet trading card → the system reads its unique analog signature → and responds with light, sound, text, and an AI-powered PC dashboard.

A child (or a curious adult) holds a card of Saturn, slides it in, and instantly gets its colour, its tone, its facts — and can ask an AI anything about it. In English or Arabic.

---

## 🔧 Hardware Overview

| Component | Role |
|---|---|
| **ATmega8** (DIP-28) | Main MCU @ 8 MHz internal oscillator |
| **Resistor Divider** (ADC0 / PC0) | Planet identification via unique card resistor |
| **LDR Sensor** (ADC1 / PC1) | Card insertion detection (light-blocking) |
| **16×2 LCD HD44780** | Displays planet name + key fact |
| **Passive Buzzer** (OC2 / PB3) | Unique PWM tone per planet |
| **RGB LEDs** (PB0–PB2) | Planet colour coding |
| **UART @ 9600 baud** (PD1) | Streams telemetry to PC dashboard |
| **USB-to-TTL converter** | Bridges MCU UART to PC |
| **9V battery + 7805** | Regulated 5V supply |

---

## 🃏 Planet Card Identification

Each card carries a unique fixed resistor bridging two copper-tape contacts. A 10 kΩ pull-up forms a voltage divider — the midpoint voltage is read by ADC0 and mapped to a planet.

| # | Planet | Resistor | VADC0 (V) | ADC Count | Threshold Range |
|---|--------|----------|-----------|-----------|-----------------|
| 1 | Mercury | 1.0 kΩ | 0.45 | ~92 | < 138 |
| 2 | Venus | 2.2 kΩ | 0.90 | ~184 | 138 – 255 |
| 3 | Earth | 4.7 kΩ | 1.60 | ~328 | 256 – 419 |
| 4 | Mars | 10 kΩ | 2.50 | ~511 | 420 – 607 |
| 5 | Jupiter | 22 kΩ | 3.44 | ~705 | 608 – 764 |
| 6 | Saturn | 47 kΩ | 4.03 | ~825 | 765 – 876 |
| 7 | Uranus | 100 kΩ | 4.55 | ~930 | 877 – 952 |
| 8 | Neptune | 220 kΩ | 4.77 | ~976 | 953 – 994 |

> Pull-up = 10 kΩ, VCC = 5 V, 10-bit ADC resolution (~4.9 mV/count).  
> Software averages 4 samples + 5-reading debounce (~50 ms) for stability.

---

## 🖥️ PC Dashboard

A single-file HTML/JS dashboard communicates via **Web Serial API** (Chrome) through a virtual COM port pair.

On receiving a planet ID over UART, the dashboard:
- Renders an animated **CSS 3D planet** (unique texture; Saturn has rings 🪐)
- Shows interactive **data cards** (distance, diameter, day length, fun facts)
- Opens a **Claude AI chat panel** pre-loaded with the planet's context — ask anything in English or Arabic

---

## 🗂️ Repository Structure

```
kawkab/ (working on progress)
│
├── firmware/                  # ATmega8 C source (avr-gcc)
│   ├── main.c                 # Entry point, state machine
│   ├── adc.c / adc.h          # ADC driver (ADC0 + ADC1)
│   ├── lcd.c / lcd.h          # 4-bit LCD HD44780 driver
│   ├── uart.c / uart.h        # UART TX driver @ 9600 baud
│   ├── buzzer.c / buzzer.h    # Timer2 PWM tone generator
│   ├── leds.c / leds.h        # RGB LED controller
│   ├── planets.c / planets.h  # Planet data table + lookup
│   └── Makefile               # avr-gcc build + avrdude flash
│
├── simulation/                # Proteus design files
│   └── kawkab.pdsprj
│
├── dashboard/                 # PC-side HTML dashboard
│   └── index.html             # Single-file app (Web Serial + Claude API)
│
├── cards/                     # Planet card assets
│   ├── artwork/               # Print-ready card designs (PDF/PNG)
│   └── resistor_map.md        # Resistor values per planet
│
├── docs/                      # Project documentation
│   ├── proposal.pdf           # Original project proposal
│   ├── schematic.pdf          # Circuit schematic
│   └── report.pdf             # Final report (added Week 14)
│
└── README.md
```

---

## 🚀 Getting Started

### Flash the Firmware
```bash
cd firmware
make
make flash   # requires avrdude + USBasp or compatible programmer
```

### Run the Dashboard
1. Open `dashboard/index.html` in **Google Chrome**
2. Connect the USB-to-TTL converter
3. Click **Connect** → select the COM port
4. Insert a planet card and watch the magic ✨

---

## 📅 Project Timeline

| Week | Milestone |
|------|-----------|
| 9 | Proteus schematic + ADC simulation |
| 10 | Firmware complete, all 8 planets verified in sim |
| 11 | Breadboard wiring + card fabrication |
| 12 | Full integration: card → LCD → UART → dashboard |
| 13 | Calibration, tone tuning, edge case handling |
| 14 | Draft report + screenshots + waveforms |
| 15 | Live demo + final report submission |



---

*"The cosmos is within us. We are made of star-stuff."* — Carl Sagan
