
# ESP32-S3 Headphone Jack & Input Controller - Revised Wiring Guide

## What Changed From Previous Version

- **WM8960 removed** — broken, no longer used
- **Headphone output** is now sourced directly from the onboard NS4150B amplifier via the existing JST speaker connector (wire cut and rerouted through the PJ307 switched jack)
- **MCP23017 uses a dedicated second I2C bus** on GPIO16/GPIO17 (H2 header pins, previously reserved for WM8960 I2S) — the internal I2C bus on GPIO14/GPIO15 is left completely untouched
- **GPIO18 is now spare**

---

## System Overview

| Component | Purpose |
|:---|:---|
| ESP32-S3 Waveshare AMOLED 1.75 | Main controller |
| PJ307 3.5mm Mono Switched Jack | Headphone output + auto-speaker switching |
| MCP23017 (SG-IO-E017-A) | I/O expander for all physical controls |
| ALPS SKQUCAA010 | 5-way joystick with center push |
| 3× Tactile push buttons (4-pin) | User buttons |
| 1× Push-button rotary encoder | Volume / scroll |

---

## Part 1: Available GPIO (H2 Header)

| H2 Label | GPIO | New Use |
|:---|:---|:---|
| IO16 | 16 | **MCP23017 SDA (Wire2)** |
| IO17 | 17 | **MCP23017 SCL (Wire2)** |
| IO18 | 18 | **MCP23017 INTA (interrupt input)** |
| 3V3 | — | Power for MCP23017 + pull-ups |
| GND | — | Ground rail |

> **Do not use GPIO14/GPIO15 (underneath pads).** Those run the internal I2C bus shared by ES8311, ES7210, QMI8658, TCA9554, and the display.

---

## Part 2: PJ307 Headphone Jack — Pin Reference & Wiring

### PJ307 Pin Functions (from datasheet schematic)

| Pin | Label | Function |
|:---|:---|:---|
| 1 | Tip | Audio signal — connects to plug tip when inserted |
| 2 | Sleeve A | Ground/sleeve (mechanically redundant pair) |
| 3 | Sleeve B | Ground/sleeve (mechanically redundant pair) |
| 4 | Switch fixed | One side of NC switch (connect to speaker+) |
| 5 | Switch spring | Moving side of NC switch (connect to amp output) |

Pins 4 & 5 are **normally closed** (connected when no plug is inserted). When a plug is inserted, the barrel pushes pin 5's spring contact away from pin 4, **breaking** the connection — this disconnects the speaker automatically.

### Speaker Auto-Switch Wiring

You are cutting the JST connector wires that currently run from the NS4150B amplifier output to the onboard speaker. The red wire is amplifier positive output; black is the other terminal (amplifier negative / return).

```
JST Red wire (Amp+) ──┬──► PJ307 Pin 1  (Tip — headphone audio out)
                      └──► PJ307 Pin 5  (Switch spring — feeds speaker when NC)

PJ307 Pin 4 (Switch fixed) ──► Speaker (+)

JST Black wire (Amp–) ──┬──► PJ307 Pin 2 or 3  (Sleeve — headphone ground return)
                        └──► Speaker (–)
```

### Behavior

| State | Pins 4–5 | Speaker | Headphone |
|:---|:---|:---|:---|
| No plug inserted | **Connected** | Receives audio ✓ | Nothing connected |
| Plug inserted | **Open** | Disconnected ✓ | Receives audio through Tip ✓ |

> **Note:** The NS4150B is a BTL (bridge-tied load) class-D amplifier. Its output is differential — both the red and black wires swing. For a simple prototype this wiring works, but headphone volume may be lower than expected. Adding a 33Ω–47Ω series resistor on the tip line protects against shorts and limits current draw.

---

## Part 3: MCP23017 on Dedicated Second I2C Bus

### Why a Separate Bus

GPIO14/GPIO15 (underneath pads) already carry the internal I2C bus with five devices on it. Adding the MCP23017 there risks bus contention and complicates debugging. GPIO16 and GPIO17 (H2 header, now free) become a second hardware I2C peripheral (`Wire2`).

### Required External Pull-Up Resistors

The onboard 2.2kΩ pull-ups only exist on GPIO14/GPIO15. The new Wire2 bus on GPIO16/GPIO17 needs its own pull-ups:

- **4.7kΩ from GPIO16 (SDA2) → 3.3V**
- **4.7kΩ from GPIO17 (SCL2) → 3.3V**

### MCP23017 Board (SG-IO-E017-A) Connections

| MCP Board Label | Connect To | Location |
|:---|:---|:---|
| VCC | 3.3V | H2 Header |
| GND | GND | H2 Header |
| SDA | GPIO16 | H2 Header (IO16 pin) |
| SCL | GPIO17 | H2 Header (IO17 pin) |
| INTA | GPIO18 | H2 Header (IO18 pin) — see Part 3b |
| INTB | NC | — (mirrored to INTA internally) |

**I2C Address:** The SG-IO-E017-A board has address solder jumpers. With all three jumpers open (default), address = **0x20**. This does not conflict with any onboard device.

### Part 3b: MCP23017 Interrupt Output (INTA → GPIO18)

Connecting INTA to GPIO18 enables interrupt-driven input reading instead of polling — the ESP32 only wakes to service the MCP23017 when a pin actually changes, which saves CPU cycles and battery on a wearable device.

**How it works:**
- Set `IOCON.MIRROR = 1`: INTA and INTB are wired together internally, so either port (A or B) triggers the single INTA output line.
- INTA pulses LOW when any monitored pin changes state.
- The ESP32 ISR sets a flag. The main loop reads `readGPIOAB()` when the flag is set — this also clears the interrupt so it can fire again.
- **No I2C inside the ISR** — Wire calls are not ISR-safe on ESP32.

**Wiring:**
- MCP23017 INTA → GPIO18 (H2 IO18 pin)
- GPIO18 uses the ESP32's internal pull-up (INTA is active-low, driven by the MCP23017 directly)
- INTB: leave unconnected (mirrored to INTA in firmware)

---

## Part 3c: Ground Routing for Peripherals

All grounds in the system (ESP32 H2 GND, MCP23017 GND, speaker return, headphone sleeve) are the same electrical node — once connected together, it doesn't matter which physical path a return current takes.

**Practical rule: connect joystick, button, and encoder GND terminals to the MCP23017 board's GND pins**, not back to the ESP32 H2 header.

The SG-IO-E017-A board has GND pins on both its top and bottom headers. Since all these peripherals are already wired to the MCP23017, landing their GND legs there keeps wire runs short and avoids running separate ground wires across the assembly back to the ESP32. The MCP23017's GND is already tied to the ESP32's GND through the power wiring you run at setup.

> **Don't mix ground destinations** (some peripherals to ESP32, some to MCP23017) — it doesn't cause electrical harm but creates messy ground loops that are harder to debug.

---

## Part 4: ALPS SKQUCAA010 5-Way Joystick — Pin Reference & Wiring

### Pin Identification (Snap-in, Drawing No. 3)

From the ALPS circuit diagram for "With Center Push Type":

| Pin | Signal | Physical Direction |
|:---|:---|:---|
| 1 (A) | Direction A | Up |
| 2 (B) | Direction B | Left |
| 3 (C) | Direction C | Down |
| **4** | **Common** | **→ GND** |
| 5 (D) | Direction D | Right |
| 6 | Center | Center press |

> The 4 larger holes (ø1.2mm) on the footprint are mechanical snap-in retention legs — they are **not electrical**. The 2 smaller holes (ø1mm) and the 4 signal pins are the electrical contacts.
>
> Exact Up/Down/Left/Right assignment depends on physical mounting orientation. Test in software and remap as needed.

### Wiring to MCP23017

| Joystick Pin | MCP23017 Pin | Function |
|:---|:---|:---|
| Pin 4 (Common) | GND | Ground reference |
| Pin 1 (A) | GPA0 | Up |
| Pin 2 (B) | GPA2 | Left |
| Pin 3 (C) | GPA1 | Down |
| Pin 5 (D) | GPA3 | Right |
| Pin 6 (Center) | GPA4 | Center press |

Each direction pin reads LOW when pushed (pulled to GND through internal switch), HIGH at rest (via MCP23017 internal pull-up).

---

## Part 5: Tactile Push Buttons (3×) — Pin Reference & Wiring

Standard 4-pin PCB tactile switches have two internally-shorted pairs:

```
  [A1]──[A2]        ← internally connected (one terminal)
     │ switch │
  [B1]──[B2]        ← internally connected (other terminal)
```

Only one pin from each pair needs to be wired:

| Button | MCP23017 Pin | Other terminal |
|:---|:---|:---|
| Button 1 | GPA5 | GND |
| Button 2 | GPA6 | GND |
| Button 3 | GPA7 | GND |

Connect any **one** of the A-side legs to GND, and any **one** of the B-side legs to the assigned MCP GPIO. The other two legs can be left unconnected or also tied to GND/GPIO (they're the same node internally).

---

## Part 6: Rotary Encoder (Push-Button Type)

| Encoder Pin | MCP23017 Pin | Function |
|:---|:---|:---|
| GND / Common | GND | Ground reference |
| CLK (A) | GPB0 | Quadrature channel A |
| DT (B) | GPB1 | Quadrature channel B |
| SW (push) | GPB2 | Encoder button |

The encoder button switch is identical to the tactile buttons — one terminal to GPB2, other terminal to GND.

---

## Part 7: Complete Wiring Summary

### Power

| From | To | Via |
|:---|:---|:---|
| H2 3.3V | MCP23017 VCC | H2 header |
| H2 GND | MCP23017 GND | H2 header |
| H2 3.3V | 4.7kΩ → GPIO16 (SDA2) | Pull-up resistor |
| H2 3.3V | 4.7kΩ → GPIO17 (SCL2) | Pull-up resistor |
| H2 GND | Speaker (–) | Through PJ307 Pin 2/3 |

### Audio / Headphone Jack (PJ307)

| From | To | Notes |
|:---|:---|:---|
| JST Red (Amp+) | PJ307 Pin 1 (Tip) | Headphone signal |
| JST Red (Amp+) | PJ307 Pin 5 (Switch spring) | Speaker feed when NC |
| PJ307 Pin 4 (Switch fixed) | Speaker (+) | Auto-disconnects when plug in |
| JST Black (Amp–) | PJ307 Pin 2 or 3 (Sleeve) | Headphone ground return |
| JST Black (Amp–) | Speaker (–) | Speaker ground return |

### I2C + Interrupt (Wire2 — MCP23017 only)

| From | To |
|:---|:---|
| GPIO16 (H2 IO16) | MCP23017 SDA |
| GPIO17 (H2 IO17) | MCP23017 SCL |
| GPIO18 (H2 IO18) | MCP23017 INTA |

### MCP23017 → Peripherals

| MCP23017 Pin | Peripheral | Notes |
|:---|:---|:---|
| GPA0 | Joystick Up (Pin 1 / A) | Other end to MCP23017 GND |
| GPA1 | Joystick Down (Pin 3 / C) | Other end to MCP23017 GND |
| GPA2 | Joystick Left (Pin 2 / B) | Other end to MCP23017 GND |
| GPA3 | Joystick Right (Pin 5 / D) | Other end to MCP23017 GND |
| GPA4 | Joystick Center (Pin 6) | Other end to MCP23017 GND |
| GPA5 | Push Button 1 | Other end to MCP23017 GND |
| GPA6 | Push Button 2 | Other end to MCP23017 GND |
| GPA7 | Push Button 3 | Other end to MCP23017 GND |
| GPB0 | Encoder CLK (A) | Other end to MCP23017 GND |
| GPB1 | Encoder DT (B) | Other end to MCP23017 GND |
| GPB2 | Encoder SW (push) | Other end to MCP23017 GND |
| GPB3–GPB7 | Spare | — |
| GND rail | Joystick Pin 4 (Common) | Connect here, not to ESP32 H2 GND |
| GND rail | All button GND terminals | Connect here, not to ESP32 H2 GND |
| GND rail | Encoder GND | Connect here, not to ESP32 H2 GND |

---

## Part 8: Updated Arduino Code

### Pin Definitions

```cpp
// ── Internal I2C bus (DO NOT USE — belongs to ES8311/ES7210/QMI8658/TCA9554) ──
// #define I2C_SDA 15
// #define I2C_SCL 14

// ── Second I2C bus — MCP23017 only ──
#define I2C2_SDA 16   // H2 header IO16
#define I2C2_SCL 17   // H2 header IO17
#define MCP_INT  18   // H2 header IO18 — MCP23017 INTA (active low)

// ── MCP23017 address (A2/A1/A0 all low = 0x20) ──
#define MCP23017_ADDR 0x20

// ── Joystick pins (MCP23017 GPA0–GPA4) ──
#define JOY_UP     0  // GPA0
#define JOY_DOWN   1  // GPA1
#define JOY_LEFT   2  // GPA2
#define JOY_RIGHT  3  // GPA3
#define JOY_CENTER 4  // GPA4

// ── Push buttons (MCP23017 GPA5–GPA7) ──
#define BTN_1 5  // GPA5
#define BTN_2 6  // GPA6
#define BTN_3 7  // GPA7

// ── Rotary encoder (MCP23017 GPB0–GPB2) ──
#define ENC_A  8   // GPB0
#define ENC_B  9   // GPB1
#define ENC_SW 10  // GPB2
```

### Initialization

```cpp
#include <Wire.h>
#include <Adafruit_MCP23X17.h>

TwoWire Wire2 = TwoWire(1);  // Second hardware I2C peripheral
Adafruit_MCP23X17 mcp;

volatile bool mcpInterruptFired = false;

void IRAM_ATTR mcpISR() {
    mcpInterruptFired = true;  // flag only — no I2C inside ISR
}

void initMCP23017() {
    // Second I2C bus — external 4.7kΩ pull-ups required on GPIO16 and GPIO17
    Wire2.begin(I2C2_SDA, I2C2_SCL);
    Wire2.setClock(400000);

    if (!mcp.begin_I2C(MCP23017_ADDR, &Wire2)) {
        Serial.println("MCP23017 not found — check wiring and pull-up resistors");
        while (1);
    }

    // All 16 pins as inputs with internal pull-ups
    for (int i = 0; i < 16; i++) {
        mcp.pinMode(i, INPUT_PULLUP);
    }

    // Mirror INTA/INTB so either port triggers the single INTA wire.
    // Active-driver output, active-LOW polarity.
    mcp.setupInterrupts(true, false, LOW);

    // Interrupt on any change for all 16 input pins
    for (int i = 0; i < 16; i++) {
        mcp.setupInterruptPin(i, CHANGE);
    }

    // Attach ESP32 interrupt — GPIO18 uses internal pull-up; INTA is active-low
    pinMode(MCP_INT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(MCP_INT), mcpISR, FALLING);

    Serial.println("MCP23017 ready on Wire2 (GPIO16/17), interrupt on GPIO18");
}
```

### Reading Controls (Interrupt-Driven)

```cpp
// Helper: extract a single pin state from the 16-bit GPIO snapshot
// Pins 0–7 = Port A (GPA), pins 8–15 = Port B (GPB)
bool pinLow(uint16_t state, int pin) {
    return !(state & (1 << pin));
}

static int lastEncA = HIGH;

void processInputs(uint16_t state) {
    if (pinLow(state, JOY_UP))     { /* up action */ }
    if (pinLow(state, JOY_DOWN))   { /* down action */ }
    if (pinLow(state, JOY_LEFT))   { /* left action */ }
    if (pinLow(state, JOY_RIGHT))  { /* right action */ }
    if (pinLow(state, JOY_CENTER)) { /* center action */ }
    if (pinLow(state, BTN_1))      { /* button 1 action */ }
    if (pinLow(state, BTN_2))      { /* button 2 action */ }
    if (pinLow(state, BTN_3))      { /* button 3 action */ }
    if (pinLow(state, ENC_SW))     { /* encoder click action */ }

    // Quadrature decoder — detect falling edge on channel A
    int encA = (state >> ENC_A) & 1;
    int encB = (state >> ENC_B) & 1;
    if (lastEncA == HIGH && encA == LOW) {
        int delta = (encB == HIGH) ? 1 : -1;  // CW or CCW
        /* adjust volume / scroll by delta */
    }
    lastEncA = encA;
}

void loop() {
    if (mcpInterruptFired) {
        mcpInterruptFired = false;
        uint16_t state = mcp.readGPIOAB();  // reads both ports, clears the interrupt
        processInputs(state);
    }
    // rest of main loop runs freely — no polling delay needed
}
```

---

## Part 9: Pre-Solder Checklist

**Headphone Jack (PJ307):**
- [ ] JST Red wire split: goes to both Pin 1 (Tip) and Pin 5 (switch spring)
- [ ] PJ307 Pin 4 (switch fixed) goes to Speaker (+)
- [ ] JST Black wire split: goes to both Pin 2 or 3 (Sleeve) and Speaker (–)
- [ ] Plug a headphone in and verify no continuity between Pin 4 and Pin 5
- [ ] Unplug headphone and verify continuity between Pin 4 and Pin 5

**MCP23017 / Wire2:**
- [ ] 4.7kΩ pull-up resistor: GPIO16 → 3.3V
- [ ] 4.7kΩ pull-up resistor: GPIO17 → 3.3V
- [ ] MCP23017 SDA → GPIO16 (H2 IO16)
- [ ] MCP23017 SCL → GPIO17 (H2 IO17)
- [ ] MCP23017 INTA → GPIO18 (H2 IO18)
- [ ] MCP23017 INTB → NC (mirrored to INTA in firmware)
- [ ] MCP23017 VCC → 3.3V, GND → GND (H2 header)
- [ ] Address jumpers set: A2/A1/A0 all low → 0x20
- [ ] Confirm interrupt fires: press any button, verify `mcpInterruptFired` goes true in serial debug

**Joystick (SKQUCAA010):**
- [ ] Pin 4 (Common) → GND
- [ ] Pins 1, 2, 3, 5, 6 → GPA0–GPA4 respectively
- [ ] I2C scan confirms MCP23017 at 0x20 before connecting inputs

**Push Buttons:**
- [ ] Each button: one A-pair leg → GND, one B-pair leg → GPA5/6/7
- [ ] Press each button, confirm correct GPA pin reads LOW in serial

**Rotary Encoder:**
- [ ] GND → GND
- [ ] CLK → GPB0, DT → GPB1, SW → GPB2
- [ ] Turn encoder, confirm alternating LOW/HIGH on GPB0/GPB1

---

## Part 10: Troubleshooting

| Symptom | Likely Cause | Fix |
|:---|:---|:---|
| MCP23017 not found on I2C scan | Missing pull-ups | Add 4.7kΩ from GPIO16→3.3V and GPIO17→3.3V |
| MCP23017 not found | Wrong `Wire` instance | Use `Wire2.begin(16, 17)` and pass `&Wire2` to `mcp.begin_I2C()` |
| Audio in speaker even with headphone plugged in | Switch wired backward | Pin 5 must be spring (moves); Pin 4 must be fixed (to speaker) |
| No audio from headphone | Tip (Pin 1) not receiving signal | Confirm red JST wire goes to Pin 1 |
| Headphone audio very quiet | BTL amp output | Expected for direct coupling — acceptable for prototype |
| Joystick direction wrong | Mounting orientation | Swap GPA mappings in software |
| Encoder skips or double-fires | No debounce | Add 10ms debounce or use hardware debounce caps (100nF across each signal) |
| Interrupt fires once then stops | Interrupt not cleared | Ensure `mcp.readGPIOAB()` is called after every ISR — this clears the MCP23017 interrupt latch |
| Interrupt never fires | INTA not wired or polarity wrong | Confirm GPIO18 → INTA, `setupInterrupts(true, false, LOW)`, `attachInterrupt(..., FALLING)` |
| Internal I2C devices stopped responding | GPIO14/15 disturbed | Confirm MCP23017 is on Wire2 only, not Wire |
