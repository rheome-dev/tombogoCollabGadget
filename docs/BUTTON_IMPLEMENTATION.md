# Button Implementation Analysis

**Updated with Schematic Information**

## Summary

The Waveshare ESP32-S3-Touch-AMOLED-1.75 has **nearly all GPIO allocated**. The onboard TCA9554 is used for display/touch control, NOT available for buttons.

---

## GPIO Status (From Schematic)

### Allocated GPIO (Cannot Use)

| GPIO | Function | Component |
|------|----------|-----------|
| GPIO0 | PWR_BTN | Power button |
| GPIO1 | SDMMC DATA1 | TF card |
| GPIO2 | SDMMC DATA0 | TF card |
| GPIO3 | QSPI LCD | Display |
| GPIO4 | I2S BCLK | Audio (J3 pin 1) |
| GPIO5 | I2S DOUT | Audio (J3 pin 2) |
| GPIO6 | I2S DIN | Audio (J3 pin 3) |
| GPIO7 | I2S MCLK | Audio (J3 pin 5) |
| GPIO8 | I2C SDA | Sensors (J3 pin 6) |
| GPIO9 | BOOT_BTN/I2C SCL | Boot button |
| GPIO10 | I2C SCL | Sensors |
| GPIO11 | SDMMC CMD | TF card |
| GPIO13 | QSPI/SDMMC | Display & TF card |
| GPIO14 | QSPI LCD | Display |
| GPIO15 | I2S WCLK | Audio |
| GPIO17 | SDMMC DATA2 | TF card |
| GPIO18 | SDMMC DATA3 | TF card |
| GPIO21 | LCD_RESET | Display |
| GPIO41 | PA_EN | Amplifier |
| GPIO46 | QSPI LCD | Display |
| GPIO48 | LCD_CS | Display |

### Potentially Available

| GPIO | Notes |
|------|-------|
| GPIO12 | Strapping pin (JTAG disable required) |
| GPIO16 | May be available |
| GPIO19-20 | USB D+/D- (can be used as GPIO) |
| GPIO38-48 | Check availability |

---

## Button Implementation Options

### Option 1: External I2C GPIO Expander (Recommended)

Add a **PCF8574** or **TCA9554** module:

**Wiring:**
```
ESP32 I2C (GPIO8 SDA, GPIO10 SCL)
    │
    └──► PCF8574 @ 0x20-0x27
             ├── P0: Record Button
             ├── P1: Stop Button
             ├── P2: Loop Select
             └── P3: Effect Bypass
```

**Parts List:**
- PCF8574 module ($1-2 from Amazon/AliExpress)
- 4x tactile buttons
- Hookup wire

**Library:** Use `Wire.h` with standard I2C, or `PCF8574` Arduino library

**Code Example:**
```cpp
#include <Wire.h>
#include <PCF8574.h>

PCF8574 buttons(0x20);

void setup() {
    Wire.begin(8, 10);  // SDA, SCL
    for (int i = 0; i < 4; i++) {
        buttons.pinMode(i, INPUT_PULLUP);
    }
}

void loop() {
    bool recordBtn = !buttons.read(0);  // Active LOW
    bool stopBtn = !buttons.read(1);
    bool loopBtn = !buttons.read(2);
    bool effectBtn = !buttons.read(3);
}
```

### Option 2: Analog Button Matrix

Use one ADC pin with resistor divider for multiple buttons.

**Circuit:**
```
GPIO12 (ADC) ──┬── R1 10kΩ ── Button 1 ── GND
                ├── R2 6.8kΩ ── Button 2 ── GND
                ├── R3 4.7kΩ ── Button 3 ── GND
                └── R4 3.3kΩ ── Button 4 ── GND
```

**ADC Values:**
| Button | Approx ADC | Range |
|--------|------------|-------|
| None | 4095 | 3800-4095 |
| 1 | ~3100 | 2900-3300 |
| 2 | ~2200 | 2000-2400 |
| 3 | ~1400 | 1200-1600 |
| 4 | ~700 | 500-900 |

**Code Example:**
```cpp
const int BUTTON_ADC = GPIO12;

int readButtons() {
    int adc = analogRead(BUTTON_ADC);
    if (adc > 3800) return 0;  // None
    if (adc > 2900) return 1;  // Button 1
    if (adc > 2000) return 2;  // Button 2
    if (adc > 1200) return 3;  // Button 3
    return 4;                    // Button 4
}
```

### Option 3: Use J3 Header (Conflicts with Audio)

The J3 8-pin header has GPIO4, 5, 6 which could be used for buttons:
- Pin 1: GPIO4 (I2S BCLK)
- Pin 2: GPIO5 (I2S DOUT)
- Pin 3: GPIO6 (I2S DIN)

**WARNING:** Using these pins for buttons will break I2S audio!

---

## Recommended Solution

### For Prototype: PCF8574 GPIO Expander

**Hardware needed:**
1. PCF8574 module ($1.50)
2. 4x tactile buttons ($0.50)
3. JST connectors or wire

**No PCB changes needed** - just wire to existing I2C bus.

**I2C bus already has:**
- GPIO8: SDA
- GPIO10: SCL

**PCF8574 address:** 0x20 (avoid conflict with TCA9554 at 0x18)

---

## Action Items

1. [x] Analyze GPIO availability from schematic
2. [ ] Order PCF8574 module
3. [ ] Wire 4 buttons to PCF8574
4. [ ] Implement button reading in firmware

---

## Bill of Materials (Buttons)

| Part | Quantity | Est. Cost |
|------|----------|-----------|
| PCF8574 module | 1 | $1.50 |
| Tactile buttons (6x6mm) | 4 | $0.50 |
| JST connector (4-pin) | 1 | $0.50 |
| Wire | 1m | $0.50 |
| **Total** | | **$3.00** |
