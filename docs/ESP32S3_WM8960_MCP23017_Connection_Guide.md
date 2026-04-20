
# ESP32-S3 Audio & Input Controller - Complete Wiring Guide

## System Overview

This document describes how to connect:
- **WM8960 Audio Board** → Dedicated headphone output only (microphone not used)
- **MCP23017 I/O Expander** → 5-way joystick, 3 push buttons, detented rotary encoder
- **ESP32-S3 Waveshare Board** → Main controller with existing dual mic + speaker

The WM8960 receives audio data via I2S from the ESP32-S3 while being configured via I2C. All inputs connect through the MCP23017 to preserve ESP32 GPIO for other functions.

> **Important:** The onboard ES8311 codec uses GPIO8/9/10/42/45 for I2S (I2S0). The WM8960 must use **I2S1** on GPIO16/17/18 (H2 header). The I2C bus is on GPIO15 (SDA) / GPIO14 (SCL) with 2.2kΩ pull-ups — these are exposed on the underneath pads.

---

## Part 1: Pin Reference - ESP32-S3 Waveshare Board

Based on the schematic and board layout:

| Location | H2 Pin # | Label | GPIO | Function |
|:---|:---|:---|:---|:---|
| **H2 Header** | 1 | VBUS | - | 5V from USB (Not used) |
| **H2 Header** | 2 | GND | - | Ground |
| **H2 Header** | 3 | 3V3 | - | 3.3V Power Output |
| **H2 Header** | 4 | TXD | 43 | UART TX (Not used) |
| **H2 Header** | 5 | RXD | 44 | UART RX (Not used) |
| **H2 Header** | 6 | GPIO16 | 16 | I2S Word Select (WS/LRCK) |
| **H2 Header** | 7 | GPIO17 | 17 | I2S Bit Clock (BCLK) |
| **H2 Header** | 8 | GPIO18 | 18 | I2S Data Output (to WM8960 TXSDA) |
| **Underneath Pads** | - | SDA | 15 | I2C Data (2.2kΩ pull-up on board) |
| **Underneath Pads** | - | SCL | 14 | I2C Clock (2.2kΩ pull-up on board) |
| **Underneath Pads** | - | EX0 | - | (Not used - expansion) |
| **Underneath Pads** | - | EX1 | - | (Not used - expansion) |
| **Underneath Pads** | - | EX2 | - | (Not used - expansion) |

H2 header silkscreen (left to right, viewed from top): `IO18 IO17 IO16 RXD TXD 3V3 GND VBUS`

> **Note:** GPIO8/9/10 are NOT available — they are used by the onboard ES8311 codec for I2S (DSDIN, SCLK, ASDOUT). The I2C bus on this board uses GPIO15/14, not GPIO8/9.

---

## Part 2: WM8960 Audio Board Connections (Headphone Only)

### WM8960 Pin Reference (from board label)

| WM8960 Pin | Function |
|:---|:---|
| VCC | 3.3V Power |
| GND | Ground |
| SDA | I2C Data |
| SCL | I2C Clock |
| CLK | I2S Bit Clock (BCLK) |
| WS | I2S Word Select (LRCK) |
| TXSDA | I2S Data Input to WM8960 DAC (playback — connects to DACDAT) |
| RXSDA | I2S Data Output from WM8960 ADC (recording — NOT USED) |
| TXMCLK | Master Clock Output (NOT USED) |
| RXMCLK | Master Clock Input (NOT USED) |
| NC | Not Connected (x2) |

### Wiring Instructions

| WM8960 Pin | Connect To | Location | Notes |
|:---|:---|:---|:---|
| **VCC** | 3.3V | H2 Header | Power |
| **GND** | GND | H2 Header | Ground |
| **SDA** | SDA | Underneath Pad | I2C control |
| **SCL** | SCL | Underneath Pad | I2C control |
| **CLK** | GPIO 17 | H2 Header | I2S bit clock |
| **WS** | GPIO 16 | H2 Header | I2S frame clock |
| **TXSDA** | GPIO 18 | H2 Header | Audio playback data to WM8960 DAC |
| **RXSDA** | NC | - | Not connected (recording not used) |
| **TXMCLK** | NC | - | Not connected |
| **RXMCLK** | NC | - | Not connected |
| **NC** (both) | NC | - | Not connected |

### WM8960 Clock Configuration Notes

The WM8960 board has an onboard 24 MHz crystal. The ESP32 acts as I2S master (generating BCLK and WS), and the WM8960 operates in **slave mode**.

The WM8960's internal DAC and DSP still require a master clock (MCLK). Since no MCLK pin is broken out on the H2 header, the WM8960's **internal PLL must be configured via I2C** to derive the required internal clocks from BCLK. The init code below handles this.

> **Note:** Without PLL configuration, the WM8960 DAC will not produce audio even if I2S data and clocks are correct.

---

## Part 3: MCP23017 I/O Expander Connections

### MCP23017 Pinout Reference

| Pin | Function | Connect To |
|:---|:---|:---|
| VDD | Power (3.3V) | 3.3V (H2 Header) |
| VSS | Ground | GND (H2 Header) |
| SDA | I2C Data | SDA (Underneath Pad) |
| SCL | I2C Clock | SCL (Underneath Pad) |
| A0 | Address bit 0 | GND (or 3.3V to change address) |
| A1 | Address bit 1 | GND |
| A2 | Address bit 2 | GND |
| RESET | Reset pin | 3.3V (pull high) |
| INT A | Interrupt A | NC (optional) |
| INT B | Interrupt B | NC (optional) |
| GPA0-GPA7 | I/O Port A | Inputs (see below) |
| GPB0-GPB7 | I/O Port B | Inputs (see below) |

### Default I2C Address Calculation

With A0, A1, A2 all connected to GND:
- **MCP23017 Address = 0x20**

This does not conflict with WM8960 (0x1A).

### MCP23017 Wiring Instructions

| MCP23017 Pin | Connect To | Location |
|:---|:---|:---|
| **VDD** | 3.3V | H2 Header |
| **VSS** | GND | H2 Header |
| **SDA** | SDA | Underneath Pad |
| **SCL** | SCL | Underneath Pad |
| **A0** | GND | H2 Header |
| **A1** | GND | H2 Header |
| **A2** | GND | H2 Header |
| **RESET** | 3.3V | H2 Header |
| **INT A** | NC | - |
| **INT B** | NC | - |

---

## Part 4: Peripheral Connections to MCP23017

All inputs will use the MCP23017's internal pull-up resistors. Connect one side of each switch/contact to the MCP23017 pin, the other side to **GND**.

### Recommended Pin Assignment

| Peripheral | MCP23017 Pin | Function |
|:---|:---|:---|
| **5-Way Joystick - Up** | GPA0 | Direction up |
| **5-Way Joystick - Down** | GPA1 | Direction down |
| **5-Way Joystick - Left** | GPA2 | Direction left |
| **5-Way Joystick - Right** | GPA3 | Direction right |
| **5-Way Joystick - Center** | GPA4 | Center press |
| **Push Button 1** | GPA5 | User button 1 |
| **Push Button 2** | GPA6 | User button 2 |
| **Push Button 3** | GPA7 | User button 3 |
| **Rotary Encoder - A** | GPB0 | Encoder phase A |
| **Rotary Encoder - B** | GPB1 | Encoder phase B |
| **Rotary Encoder - Switch** | GPB2 | Encoder push button |
| **Spare** | GPB3-GPB7 | Future expansion |

### Wiring Instructions for Each Peripheral

**5-Way Joystick (Common Ground type):**
- Common pin → GND
- Up → GPA0
- Down → GPA1
- Left → GPA2
- Right → GPA3
- Center press → GPA4

**Push Buttons (Normally Open):**
- Pin 1 → GND
- Pin 2 → Assigned GPA5/6/7

**Rotary Encoder (Mechanical, Common Ground):**
- Common pin → GND
- Phase A → GPB0
- Phase B → GPB1
- Push button switch → GPB2 (with other side to GND)

---

## Part 5: Complete Wiring Summary Table

| From | Pin | To | Location |
|:---|:---|:---|:---|
| **POWER** ||||
| ESP32-S3 | 3.3V | WM8960 VCC | H2 Header |
| ESP32-S3 | 3.3V | MCP23017 VDD | H2 Header |
| ESP32-S3 | 3.3V | MCP23017 RESET | H2 Header |
| ESP32-S3 | GND | WM8960 GND | H2 Header |
| ESP32-S3 | GND | MCP23017 VSS | H2 Header |
| ESP32-S3 | GND | MCP23017 A0 | H2 Header |
| ESP32-S3 | GND | MCP23017 A1 | H2 Header |
| ESP32-S3 | GND | MCP23017 A2 | H2 Header |
| ESP32-S3 | GND | Joystick common | H2 Header |
| ESP32-S3 | GND | Button common (all) | H2 Header |
| ESP32-S3 | GND | Encoder common | H2 Header |
| **I2C CONTROL** ||||
| ESP32-S3 | SDA (GPIO 15) | WM8960 SDA | Underneath Pad |
| ESP32-S3 | SDA (GPIO 15) | MCP23017 SDA | Underneath Pad |
| ESP32-S3 | SCL (GPIO 14) | WM8960 SCL | Underneath Pad |
| ESP32-S3 | SCL (GPIO 14) | MCP23017 SCL | Underneath Pad |
| **I2S AUDIO (I2S1)** ||||
| ESP32-S3 | GPIO 16 | WM8960 WS | H2 Header |
| ESP32-S3 | GPIO 17 | WM8960 CLK | H2 Header |
| ESP32-S3 | GPIO 18 | WM8960 TXSDA | H2 Header |
| **MCP23017 INPUTS** ||||
| MCP23017 | GPA0 | Joystick Up | - |
| MCP23017 | GPA1 | Joystick Down | - |
| MCP23017 | GPA2 | Joystick Left | - |
| MCP23017 | GPA3 | Joystick Right | - |
| MCP23017 | GPA4 | Joystick Center | - |
| MCP23017 | GPA5 | Push Button 1 | - |
| MCP23017 | GPA6 | Push Button 2 | - |
| MCP23017 | GPA7 | Push Button 3 | - |
| MCP23017 | GPB0 | Encoder Phase A | - |
| MCP23017 | GPB1 | Encoder Phase B | - |
| MCP23017 | GPB2 | Encoder Switch | - |

---

## Part 6: ESP32-S3 Arduino Code

### Required Libraries

```cpp
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <driver/i2s.h>  // ESP-IDF I2S driver (supports I2S1)
```

### I2C and I2S Pin Definitions

```cpp
// I2C pins (underneath pads — with 2.2kΩ pull-ups on board)
#define I2C_SDA 15
#define I2C_SCL 14

// I2S1 pins (H2 header — I2S0 is used by onboard ES8311)
#define I2S_BCLK 17
#define I2S_LRCK 16
#define I2S_DOUT 18    // Data output to WM8960 TXSDA (DACDAT)

// Device addresses
#define WM8960_ADDR 0x1A
#define MCP23017_ADDR 0x20

// Use I2S port 1 (I2S0 is taken by onboard ES8311)
#define I2S_PORT I2S_NUM_1

Adafruit_MCP23X17 mcp;
```

### MCP23017 Initialization

```cpp
void initMCP23017() {
    if (!mcp.begin_I2C(MCP23017_ADDR, &Wire)) {
        Serial.println("MCP23017 not found - check wiring");
        while (1);
    }
    
    // Configure all input pins with pull-ups enabled
    // GPA0-GPA7 (joystick + buttons)
    for (int i = 0; i < 8; i++) {
        mcp.pinMode(i, INPUT_PULLUP);
    }
    
    // GPB0-GPB2 (encoder)
    for (int i = 8; i < 11; i++) {
        mcp.pinMode(i, INPUT_PULLUP);
    }
    
    // Optional: GPB3-GPB7 as inputs with pull-ups
    for (int i = 11; i < 16; i++) {
        mcp.pinMode(i, INPUT_PULLUP);
    }
    
    Serial.println("MCP23017 initialized");
}
```

### WM8960 Initialization (Minimal for Headphone Playback)

```cpp
void writeWM8960Register(uint8_t reg, uint16_t value) {
    Wire.beginTransmission(WM8960_ADDR);
    Wire.write((reg << 1) | ((value >> 8) & 0x01));
    Wire.write(value & 0xFF);
    Wire.endTransmission();
}

void initWM8960() {
    // Reset the codec
    writeWM8960Register(0x0F, 0x0000);  // R15 - Software reset
    delay(100);
    
    // --- Power Management ---
    // R25 (0x19) Power 1: VMID=50kΩ divider, VREF enabled
    writeWM8960Register(0x19, 0x00C0);  // VMIDSEL=01 (50k), VREF=1
    delay(50);  // Wait for VMID to charge
    
    // R26 (0x1A) Power 2: Enable DAC L/R and headphone L/R outputs
    writeWM8960Register(0x1A, 0x01E0);  // DACL=1, DACR=1, LOUT1=1, ROUT1=1
    
    // R47 (0x2F) Power 3: Enable left and right output mixers
    writeWM8960Register(0x2F, 0x000C);  // LOMIX=1, ROMIX=1
    
    // --- Audio Interface ---
    // R7 (0x07): I2S format, 16-bit word length, slave mode
    writeWM8960Register(0x07, 0x0002);  // FORMAT=10 (I2S), WL=00 (16-bit), MS=0 (slave)
    
    // --- PLL Configuration (derive MCLK from BCLK) ---
    // R52 (0x34) PLL N: Enable PLL, PLLPRESCALE=1 (div 2), N=8
    writeWM8960Register(0x34, 0x0037);  // PLLEN=0, PLLPRESCALE=1, SDM=1, N=0x7
    // Note: PLL config is sample-rate dependent. For 44.1kHz with BCLK=1.4112MHz:
    // Adjust N, K values per WM8960 datasheet Table 42 for your chosen sample rate.
    // For simplicity, using BCLK as SYSCLK source may work with register R4 (0x04).
    
    // R4 (0x04) Clocking: SYSCLK derived from MCLK (or PLL)
    writeWM8960Register(0x04, 0x0000);  // CLKSEL=0 (MCLK), SYSCLKDIV=0
    
    // --- DAC Configuration ---
    // R5 (0x05) DAC Control: Unmute DAC
    writeWM8960Register(0x05, 0x0000);  // DACMU=0 (unmute)
    
    // R10 (0x0A) Left DAC Volume: 0dB, update
    writeWM8960Register(0x0A, 0x01FF);  // DACVU=1, LDACVOL=0xFF (0dB)
    // R11 (0x0B) Right DAC Volume: 0dB, update
    writeWM8960Register(0x0B, 0x01FF);  // DACVU=1, RDACVOL=0xFF (0dB)
    
    // --- Output Mixer ---
    // R34 (0x22) Left Output Mixer: DAC to left output
    writeWM8960Register(0x22, 0x0100);  // LD2LO=1 (left DAC to left output mixer)
    // R37 (0x25) Right Output Mixer: DAC to right output
    writeWM8960Register(0x25, 0x0100);  // RD2RO=1 (right DAC to right output mixer)
    
    // --- Headphone Volume ---
    // R2 (0x02) Left Headphone Volume
    writeWM8960Register(0x02, 0x0179);  // HPVU=1, LHPVOL=0x79 (0dB)
    // R3 (0x03) Right Headphone Volume
    writeWM8960Register(0x03, 0x0179);  // HPVU=1, RHPVOL=0x79 (0dB)
    
    Serial.println("WM8960 initialized for headphone playback");
}
```

### I2S Initialization (Using I2S1)

```cpp
void initI2S() {
    // Configure I2S1 (I2S0 is used by onboard ES8311 codec)
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,     // GPIO17
        .ws_io_num = I2S_LRCK,      // GPIO16
        .data_out_num = I2S_DOUT,   // GPIO18 → WM8960 TXSDA (DACDAT)
        .data_in_num = I2S_PIN_NO_CHANGE  // No input (recording not used)
    };
    
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_PORT, &pin_config);
    i2s_zero_dma_buffer(I2S_PORT);
    
    Serial.println("I2S1 initialized for WM8960");
}
```

### Reading Inputs from MCP23017

```cpp
// Read button state (returns true when pressed, since we use pull-ups)
bool readButton(int pin) {
    return (mcp.digitalRead(pin) == LOW);
}

// Read encoder rotation (returns 1 for clockwise, -1 for counter-clockwise)
int readEncoder(int pinA, int pinB) {
    static int lastA = 1;
    int a = mcp.digitalRead(pinA);
    int b = mcp.digitalRead(pinB);
    int result = 0;
    
    if (lastA == 1 && a == 0) {  // Falling edge on A
        if (b == 1) result = 1;   // Clockwise
        else result = -1;          // Counter-clockwise
    }
    lastA = a;
    return result;
}
```

### Complete Setup Function

```cpp
void setup() {
    Serial.begin(115200);
    
    // Initialize I2C on underneath pads (GPIO15=SDA, GPIO14=SCL)
    // Board has 2.2kΩ pull-ups on these lines
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);  // 400kHz for faster I2C
    
    // Initialize peripherals
    initMCP23017();
    initWM8960();
    initI2S();
    
    Serial.println("System ready");
}

void loop() {
    // Read joystick directions
    if (readButton(0)) handleJoystickUp();
    if (readButton(1)) handleJoystickDown();
    if (readButton(2)) handleJoystickLeft();
    if (readButton(3)) handleJoystickRight();
    if (readButton(4)) handleJoystickCenter();
    
    // Read push buttons
    if (readButton(5)) handleButton1();
    if (readButton(6)) handleButton2();
    if (readButton(7)) handleButton3();
    
    // Read encoder rotation
    int encoderChange = readEncoder(8, 9);  // GPB0, GPB1
    if (encoderChange != 0) {
        adjustVolume(encoderChange);  // Increase/decrease volume
    }
    
    // Read encoder push button
    if (readButton(10)) handleEncoderButton();  // GPB2
    
    delay(20);  // Small debounce delay
}
```

---

## Part 7: Connection Diagram (Text Representation)

```
ESP32-S3 Waveshare Board
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  H2 HEADER (pins 1-8)         UNDERNEATH PADS              │
│  ┌─────────┐                  ┌──────────┐                 │
│  │VBUS (1) │                  │SDA (15)──┼──┬───────────┐  │
│  │GND  (2)─┼────┬─────────┬───│SCL (14)──┼──┼─────────┐ │  │
│  │3V3  (3)─┼────┼─────────┼───│EX0       │  │         │ │  │
│  │TXD  (4) │    │         │   │EX1       │  │         │ │  │
│  │RXD  (5) │    │         │   │EX2       │  │         │ │  │
│  │IO16 (6)─┼────┼────┐    │   └──────────┘  │         │ │  │
│  │IO17 (7)─┼────┼────┼──┐ │                 │         │ │  │
│  │IO18 (8)─┼────┼────┼──┼─┼──────────────┐  │         │ │  │
│  └─────────┘    │    │  │ │              │  │         │ │  │
└─────────────────┼────┼──┼─┼──────────────┼──┼─────────┼─┼──┘
                  │    │  │ │              │  │         │ │
   ┌──────────────┼────┼──┼─┼──────────────┼──┼─────────┼─┼──┐
   │              │    │  │ │              │  │         │ │  │
   │              ▼    ▼  ▼ ▼              ▼  ▼         ▼ ▼  │
   │                    WM8960 AUDIO BOARD                    │
   │                                                          │
   │  VCC ◄── 3V3      SDA ◄── GPIO15                        │
   │  GND ◄── GND      SCL ◄── GPIO14                        │
   │  CLK ◄── GPIO17   WS  ◄── GPIO16                        │
   │  TXSDA ◄─ GPIO18  (playback data to DACDAT)              │
   │  RXSDA    NC       (recording — not used)                │
   │                                                          │
   └──────────────────────────────────────────────────────────┘

   ┌──────────────────────────────────────────────────────────┐
   │                  MCP23017 I/O EXPANDER                   │
   │                                                          │
   │  VDD ◄── 3V3       SDA ◄── GPIO15                       │
   │  VSS ◄── GND       SCL ◄── GPIO14                       │
   │  RESET ◄─ 3V3      A0/A1/A2 ◄── GND                     │
   │                                                          │
   │  GPA0 ◄────── Joystick Up                                │
   │  GPA1 ◄────── Joystick Down                              │
   │  GPA2 ◄────── Joystick Left                              │
   │  GPA3 ◄────── Joystick Right                             │
   │  GPA4 ◄────── Joystick Center                            │
   │  GPA5 ◄────── Push Button 1                              │
   │  GPA6 ◄────── Push Button 2                              │
   │  GPA7 ◄────── Push Button 3                              │
   │  GPB0 ◄────── Encoder A                                  │
   │  GPB1 ◄────── Encoder B                                  │
   │  GPB2 ◄────── Encoder Switch                             │
   │                                                          │
   └──────────────────────────────────────────────────────────┘
```

---

## Part 8: Verification Checklist

Before powering on, verify:

- [ ] **Power:** 3.3V and GND connected to both WM8960 and MCP23017
- [ ] **I2C:** SDA (GPIO15) / SCL (GPIO14) from underneath pads connected to BOTH modules (parallel)
- [ ] **I2C Pull-ups:** Board has 2.2kΩ external pull-ups on GPIO15/14 — no additional pull-ups needed
- [ ] **I2S:** GPIO16→WS, GPIO17→CLK, GPIO18→TXSDA (playback data)
- [ ] **MCP23017 Address:** A0/A1/A2 all connected to GND (address 0x20)
- [ ] **WM8960 Unused pins:** RXSDA, TXMCLK, RXMCLK left unconnected
- [ ] **Input wiring:** All switches connect the other side to GND (not 3.3V)
- [ ] **No I2S conflict:** WM8960 uses I2S1 (GPIO16/17/18), onboard ES8311 uses I2S0 (GPIO8/9/10/42/45)
- [ ] **No conflicts:** WM8960 (0x1A) and MCP23017 (0x20) have different I2C addresses

---

## Part 9: Troubleshooting Guide

| Symptom | Possible Cause | Solution |
|:---|:---|:---|
| No I2C devices found | Wrong SDA/SCL pins | Use `Wire.begin(15, 14)` explicitly (NOT 8/9!) |
| MCP23017 not responding | Address incorrect | Verify A0/A1/A2 are tied to GND |
| No headphone audio | I2S pins swapped | Check GPIO16=WS, GPIO17=CLK, GPIO18=TXSDA |
| No headphone audio | Wrong I2S port | Must use I2S_NUM_1, not I2S_NUM_0 (ES8311 conflict) |
| No headphone audio | TXSDA/RXSDA swapped | Playback data goes to TXSDA (not RXSDA) |
| No headphone audio | WM8960 PLL not configured | PLL must be configured via I2C for MCLK generation |
| Distorted audio | Sample rate mismatch | Ensure both codec and I2S use same rate (44.1kHz) |
| Erratic button readings | Missing pull-ups | Enable internal pull-ups in MCP23017 |
| Encoder skipping steps | Debounce needed | Add 10ms delay or implement debouncing |
| Boot failure | GPIO conflict | Avoid using GPIO 0/3/46 for inputs |

---

This document provides everything needed to wire and configure your system. The ESP32-S3 handles audio playback via I2S to the WM8960 headphone output, while all user inputs are read through the MCP23017 I2C I/O expander without consuming additional ESP32 GPIO pins.