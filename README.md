# KAWKAB — Planet Encyclopedia Embedded Card Reader

> **كوكب** (*kawkab*) — Arabic for "planet"

Insert a planet trading card → the system identifies it and responds with a planet-specific LCD message, RGB color, musical tone, and a live PC dashboard with a 3D view and AI assistant.

🎬 **[Watch the demo on YouTube](https://www.youtube.com/watch?v=02pbbkqU8Yo)**

---

## The Idea

Each planet card has a unique resistor embedded in it. When inserted, it completes a voltage divider that the ATmega8 reads via ADC. The microcontroller then simultaneously drives four outputs: LCD text, an RGB LED color, a tone sequence, and a UART packet that feeds a browser-based dashboard.

The cards are printed on card stock with copper-tape contacts — designed to feel like collectibles rather than components.

---

## Hardware Overview

| Part | Role |
|------|------|
| ATmega8 @ 8 MHz | Main MCU |
| Resistor-coded card + 10 kΩ pull-up (ADC0) | Planet identification |
| LDR + 10 kΩ divider (ADC1) | Card insertion detection |
| 16×2 LCD — 4-bit mode | Planet name + fact |
| RGB LED (common cathode, 330 Ω/ch) | Planet color |
| Passive buzzer (PB3/OC2, 100 Ω series) | Musical tone signature |
| USB-to-TTL adapter | UART → PC |

### Card Resistor Map

| Planet  | Resistor | ADC Range |
|---------|----------|-----------|
| Mercury | 1.0 kΩ  | < 138     |
| Venus   | 2.2 kΩ  | 138–255   |
| Earth   | 4.7 kΩ  | 256–419   |
| Mars    | 10 kΩ   | 420–607   |
| Jupiter | 22 kΩ   | 608–764   |
| Saturn  | 47 kΩ   | 765–876   |
| Uranus  | 100 kΩ  | 877–952   |
| Neptune | 220 kΩ  | 953–994   |

---

## PC Dashboard

A Python bridge reads the UART stream and forwards it over WebSocket to a local HTML dashboard.

```bash
pip install pyserial websockets
python bridge/serial_bridge.py --port COM3 --baud 9600
```

Open `dashboard/index.html` in Chrome. Enter your Anthropic API key in the AI Assistant panel to enable planet chat.

**UART packet format:**
```
PLANET=MARS ADC0=509 ADC1=11 OK
CARD_REMOVED
```

---

## Build & Flash

```bash
avr-gcc -mmcu=atmega8 -DF_CPU=8000000UL -Os -o kawkab.elf firmware/main.c
avr-objcopy -O ihex kawkab.elf kawkab.hex
avrdude -c usbasp -p m8 -U flash:w:kawkab.hex
```

---

## Simulating in Proteus

The full circuit can be validated in **Proteus Design Suite** before physical assembly.

1. **Open the schematic** from `simulation/kawkab.pdsprj`
2. **Load the hex file** — double-click the ATmega8 component, set *Program File* to `kawkab.hex` and *Clock Frequency* to `8MHz`
3. **Simulate the LDR** with a potentiometer on ADC1:
   - High resistance (ADC > 700) → no card
   - Low resistance (ADC < 300) → card inserted
4. **Simulate planet cards** with a second potentiometer on ADC0 — dial to each target voltage from the resistor map above
5. **Check UART output** via the Proteus Virtual Terminal (9600 baud, 8N1) — expect `PLANET=MARS ADC0=... OK` per planet
6. **Watch the outputs** — the LCD, LED, and buzzer components respond in real time as you adjust the potentiometers

> The state machine requires 5 consecutive stable LDR readings (~50 ms) before transitioning, so hold the potentiometer steady for a moment after adjusting.

---

## Repository Structure

```
kawkab/
├── firmware/
│   └── main.c                 # ATmega8 firmware (avr-gcc C)
├── bridge/
│   └── serial_bridge.py       # pyserial → WebSocket bridge
├── dashboard/
│   └── index.html             # Three.js + Claude AI dashboard
├── simulation/
│   └── kawkab.pdsprj          # Proteus Design Suite project
└── README.md
```

---

## Team

Ayah Alshanfari · Shahad Al Rubkhi · Hawa Bahashwan

**ECCE4227 Embedded Systems — Spring 2026**  
Sultan Qaboos University, College of Engineering
