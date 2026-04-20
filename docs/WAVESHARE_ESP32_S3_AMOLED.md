# Waveshare ESP32-S3-Touch-AMOLED-1.75 Hardware Documentation

**Updated with Schematic Information**

## Board Overview

| Specification | Value |
|--------------|-------|
| **Display** | 1.75" AMOLED capacitive touch |
| **Resolution** | 466×466 pixels, 16.7M colors |
| **Processor** | ESP32-S3R8 dual-core 240MHz |
| **Memory** | 512KB SRAM, 384KB ROM, 8MB PSRAM, 16MB Flash |
| **Wireless** | WiFi 802.11 b/g/n (2.4GHz), Bluetooth 5.0 BLE |
| **Power** | 3.7V lithium battery (MX1.25 connector) |

---

## Onboard Components

| Component | Function | Interface |
|-----------|----------|-----------|
| ESP32-S3R8 | Main WiFi/BT SoC with 8MB PSRAM | - |
| TCA9554 (U13) | IO expansion chip | I2C (0x18) |
| PCF85063 (U14) | RTC clock chip | I2C (0x51) |
| QMI8658 (U15) | 6-axis IMU (3-axis gyro + 3-axis accel) | I2C (0x6B) |
| AXP2101 (U16) | Power management chip | I2C (0x34) |
| ES7210 (U11) | Echo cancellation for microphones | - |
| ES8311 (U12) | Audio codec | I2C (0x30), I2S |
| CST9217 (U10) | Touch controller | I2C (0x5A) |
| CO5300 | Display driver | QSPI |

---

## Complete Pin Assignments

### Display (QSPI Interface)

| Signal | GPIO | Notes |
|--------|------|-------|
| LCD_CS | GPIO48 | Chip select |
| LCD_SCLK | GPIO9 | Clock |
| LCD_SDIO0 | GPIO3 | Data 0 |
| LCD_SDIO1 | GPIO46 | Data 1 |
| LCD_SDIO2 | GPIO14 | Data 2 |
| LCD_SDIO3 | GPIO13 | Data 3 |
| LCD_RESET | GPIO21 | Reset |
| LCD_PWR | TCA9554 P2 | Power enable |

### I2S Audio (ES8311 Codec)

| Signal | GPIO | Notes |
|--------|------|-------|
| BCLK | GPIO4 | Bit clock |
| WCLK | GPIO15 | Word clock (LRCK) |
| DOUT | GPIO5 | Data out (to codec) |
| DIN | GPIO6 | Data in (from codec) |
| MCLK | GPIO7 | Master clock |

### I2C Bus

| Signal | GPIO | Notes |
|--------|------|-------|
| SDA | GPIO8 | Data |
| SCL | GPIO10 | Clock |

### TF Card (SPI/SDMMC)

| Signal | GPIO | Notes |
|--------|------|-------|
| CMD | GPIO11 | Command |
| CLK | GPIO13 | Clock |
| DATA0 | GPIO2 | Data |
| DATA1 | GPIO1 | Data |
| DATA2 | GPIO17 | Data |
| DATA3 | GPIO18 | Data |

### Buttons

| Signal | GPIO | Notes |
|--------|------|-------|
| PWR_BTN | GPIO0 | Power button (bootable) |
| BOOT_BTN | GPIO9 | Boot button |

### Power & Other

| Signal | GPIO | Notes |
|--------|------|-------|
| PA_EN | GPIO41 | Power amplifier enable |
| Battery | J1 | 3.7V lithium |
| Speaker | J2 | Speaker output |

### TCA9554 IO Expander (U13)

| Pin | Function | Notes |
|-----|----------|-------|
| P0 | TP_RST | Touch controller reset |
| P1 | LCD_RST | Display reset |
| P2 | LCD_PWR | Display power enable |
| P3 | TE | Tearing effect signal |

**Note:** TCA9554 is used for display/touch control, NOT available for general-purpose IO.

### J3 - 8-Pin Header (2.54mm Pitch)

**This is the key connector for external buttons!**

| Pin | Signal | GPIO | Notes |
|-----|--------|------|-------|
| 1 | IO1 | GPIO4 | Available (used by I2S BCLK) |
| 2 | IO2 | GPIO5 | Available (used by I2S DOUT) |
| 3 | IO3 | GPIO6 | Available (used by I2S DIN) |
| 4 | GND | - | Ground |
| 5 | TX | GPIO7 | UART TX |
| 6 | RX | GPIO8 | UART RX |
| 7 | 3V3 | - | 3.3V power |
| 8 | GND | - | Ground |

**CRITICAL:** The J3 header GPIO pins (GPIO4, GPIO5, GPIO6) are SHARED with I2S audio! Using them for buttons will conflict with audio.

---

## I2C Addresses

| Device | Address | Notes |
|--------|---------|-------|
| Touch (CST9217) | 0x5A | |
| IMU (QMI8658) | 0x6B | |
| RTC (PCF85063) | 0x51 | |
| Power (AXP2101) | 0x34 | |
| IO Expander (TCA9554) | 0x18 | Used for display/touch |
| Audio Codec (ES8311) | 0x30 | |

---

## GPIO Status Summary

### Allocated (Cannot Use)

| GPIO | Function | Notes |
|------|----------|-------|
| GPIO0 | PWR_BTN | Power button (required) |
| GPIO1 | SDMMC DATA1 | TF card |
| GPIO2 | SDMMC DATA0 | TF card |
| GPIO3 | QSPI LCD | Display |
| GPIO4 | I2S BCLK | Audio (or J3 pin 1) |
| GPIO5 | I2S DOUT | Audio (or J3 pin 2) |
| GPIO6 | I2S DIN | Audio (or J3 pin 3) |
| GPIO7 | I2S MCLK | Audio (or J3 pin 5 TX) |
| GPIO8 | I2C SDA / UART RX | Sensors (or J3 pin 6 RX) |
| GPIO9 | BOOT_BTN / I2C SCL | Boot (or J3 - shared!) |
| GPIO10 | I2C SCL | Sensors |
| GPIO11 | SDMMC CMD | TF card |
| GPIO13 | QSPI LCD / SDMMC CLK | Display & TF card |
| GPIO14 | QSPI LCD | Display |
| GPIO15 | I2S WCLK | Audio |
| GPIO17 | SDMMC DATA2 | TF card |
| GPIO18 | SDMMC DATA3 | TF card |
| GPIO21 | LCD_RESET | Display |
| GPIO41 | PA_EN | Amplifier enable |
| GPIO46 | QSPI LCD | Display |
| GPIO48 | LCD_CS | Display |

### Potentially Available

| GPIO | Notes |
|------|-------|
| GPIO12 | May be strapping pin (JTAG) |
| GPIO16 | May be available |
| GPIO19 | May be available (USB) |
| GPIO20 | May be available (USB) |
| GPIO38-48 | Extended range - check |

---

## True GPIO Availability

**Unfortunately, nearly all GPIO are allocated:**

1. **Display:** Uses GPIO3, 9, 13, 14, 21, 46, 48 (7 pins)
2. **I2S Audio:** Uses GPIO4, 5, 6, 7, 15 (5 pins)
3. **I2C:** Uses GPIO8, 10 (2 pins)
4. **TF Card:** Uses GPIO1, 2, 11, 13, 17, 18 (6 pins)
5. **Buttons:** Uses GPIO0, 9 (2 pins)

**Total: 22 GPIO used out of ~26 available**

---

## Button Solution Options

### Option A: J3 Header GPIO (If Audio Disabled)

If we disable I2S audio temporarily or use only one-way audio, GPIO4, 5, 6 on J3 header are available.

**Not recommended** - loses audio functionality.

### Option B: Analog Button Circuit (Recommended)

Use a single ADC pin with resistor divider for 4+ buttons.

**Required:**
- Use GPIO12 or GPIO16 (if available)
- Add external resistor network

### Option C: External GPIO Expander

Add a second I2C GPIO expander (PCF8574 or another TCA9554) on the I2C bus.

**Pros:**
- 8 additional IO pins
- Simple to implement

**Cons:**
- Additional hardware

### Option D: Use UART Pins

J3 pins 5-6 (GPIO7 TX, GPIO8 RX) could be used for buttons when UART not in use.

---

## Recommended Button Implementation

### For This Project: External I2C GPIO Expander

Add a **PCF8574** (8-bit GPIO expander) at address **0x20-0x27**:

```
ESP32 I2C (GPIO8 SDA, GPIO10 SCL) ──► PCF8574
                                          ├── P0: Record Button
                                          ├── P1: Stop Button
                                          ├── P2: Loop Select
                                          └── P3: Effect Bypass
```

**Parts needed:**
- PCF8574 module or chip ($1-2)
- 4 tactile buttons
- 4x 10kΩ pull-up resistors (optional - PCF8574 has internal)

---

## Resources

- Schematic: `ESP32-S3-Touch-AMOLED-1.75-schematic.pdf` (in docs folder)
- Product Page: https://www.waveshare.com/product/esp32-s3-touch-amoled-1.75.htm
- Wiki: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75
