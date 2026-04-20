# Tombogo Collab Gadget - Full Wiring Diagram

## System Overview

```mermaid
flowchart TB
    subgraph ESP32["Waveshare ESP32-S3 AMOLED"]
        direction TB
        I2C[I2C Bus<br/>GPIO8 SDA, GPIO10 SCL]
        I2S[I2S Audio<br/>GPIO4-7, 15]
        PWR[Power<br/>3.3V, GND]
    end

    subgraph Buttons["Button Circuit"]
        PCF[Adafruit PCF8574]
        B1[Button 1<br/>Record]
        B2[Button 2<br/>Stop]
        B3[Button 3<br/>Loop]
        B4[Button 4<br/>Effect]
    end

    subgraph AudioOut["Audio Output"]
        WM[WM8960<br/>Audio Board]
        SPK[Speaker<br/>8Ω 1W]
        HP[Headphones<br/>3.5mm jack]
    end

    subgraph AudioIn["Audio Input"]
        MIC[Onboard<br/>Dual Mics]
    end

    Buttons -->|I2C| I2C
    AudioOut -->|I2S| I2S
    AudioOut -->|I2C| I2C
    AudioOut -->|Power| PWR
```

---

## Button Wiring

```mermaid
flowchart LR
    subgraph ESP32["Waveshare ESP32-S3"]
        GPIO8[SDA<br/>GPIO8]
        GPIO10[SCL<br/>GPIO10]
        V33[3V3]
        GND[GND]
    end

    subgraph PCF8574["Adafruit PCF8574"]
        SDA[SDA]
        SCL[SCL]
        VCC[VCC]
        GND[GND]
        P0[P0]
        P1[P1]
        P2[P2]
        P3[P3]
    end

    subgraph Buttons["Buttons (x4)"]
        B1[Record]
        B2[Stop]
        B3[Loop]
        B4[Effect]
    end

    ESP32 -->|"Jumper Wires"| PCF8574
    PCF8574 --> Buttons
```

### Button Connections

| PCF8574 Pin | Function | Wire Color (suggested) |
|-------------|----------|----------------------|
| P0 | Record Button | Red |
| P1 | Stop Button | Orange |
| P2 | Loop Select | Yellow |
| P3 | Effect Bypass | Green |
| VCC | 3.3V | White |
| GND | Ground | Black |

### Button Circuit (Internal)

```mermaid
flowchart TB
    subgraph PCF8574["PCF8574"]
        P0
        P1
        P2
        P3
        PU[(Internal<br/>Pull-ups)]
    end

    subgraph ButtonCircuit["Button Circuit"]
        B1[Button 1]
        B2[Button 2]
        B3[Button 3]
        B4[Button 4]
    end

    P0 -->|"one leg"| B1
    P1 -->|"one leg"| B2
    P2 -->|"one leg"| B3
    P3 -->|"one leg"| B4

    B1 -->|"other leg"| GND
    B2 -->|"other leg"| GND
    B3 -->|"other leg"| GND
    B4 -->|"other leg"| GND
```

---

## I2C Connections

```mermaid
flowchart LR
    subgraph Bus["I2C Bus"]
        direction TB
        SDA[SDA - Data]
        SCL[SCL - Clock]
    end

    subgraph Devices["I2C Devices"]
        direction TB
        ES8311[ES8311<br/>Audio Codec<br/>0x30]
        QMI8658[QMI8658<br/>IMU<br/>0x6B]
        CST9217[CST9217<br/>Touch<br/>0x5A]
        TCA9554[TCA9554<br/>IO Expander<br/>0x18]
        PCF85063[PCF85063<br/>RTC<br/>0x51]
        AXP2101[AXP2101<br/>PMIC<br/>0x34]
        PCF8574[PCF8574<br/>Buttons<br/>0x20]
    end

    Bus --> Devices
```

### Complete I2C Wiring

| ESP32 Pin | Signal | Device | Address |
|-----------|--------|--------|---------|
| GPIO8 | SDA | ES8311 | 0x30 |
| GPIO8 | SDA | QMI8658 | 0x6B |
| GPIO8 | SDA | CST9217 | 0x5A |
| GPIO8 | SDA | TCA9554 | 0x18 |
| GPIO8 | SDA | PCF85063 | 0x51 |
| GPIO8 | SDA | AXP2101 | 0x34 |
| GPIO8 | SDA | **PCF8574** | **0x20** |
| GPIO10 | SCL | *(all above)* | - |

**Note:** All devices share the same SDA/SCL lines - this is how I2C works!

---

## Audio Board Wiring (WM8960)

```mermaid
flowchart TB
    subgraph ESP32["Waveshare ESP32-S3"]
        direction TB
        I2S_PWR["I2S Pins<br/>GPIO4-7, 15"]
        I2C_PWR["I2C Pins<br/>GPIO8, 10"]
        PWR_3V3["3.3V"]
        GND_PWR["GND"]
    end

    subgraph WM8960["WM8960 Audio Board"]
        direction TB
        I2S_CONN["I2S Connector"]
        I2C_CONN["I2C Connector"]
        PWR_CONN["Power Connector"]
        SPK_L["Speaker L"]
        SPK_R["Speaker R"]
        HP_JACK["Headphone Jack"]
    end

    ESP32 -->|"I2S"| WM8960
    ESP32 -->|"I2C"| WM8960
    ESP32 -->|"Power"| WM8960
```

### WM8960 Connections

#### I2S Audio (Digital Audio)

| ESP32 Signal | ESP32 GPIO | WM8960 Pin | Notes |
|--------------|------------|------------|-------|
| BCLK | GPIO4 | BCLK | Bit clock |
| WCLK (LRCK) | GPIO15 | WS | Word select |
| DOUT | GPIO5 | DIN | Data to codec |
| DIN | GPIO6 | DOUT | Data from codec |
| MCLK | GPIO7 | MCLK | Master clock |

#### I2C Control

| ESP32 Signal | ESP32 GPIO | WM8960 Pin |
|--------------|------------|------------|
| SDA | GPIO8 | SDA |
| SCL | GPIO10 | SCL |

**WM8960 I2C Address:** 0x1A (different from ES8311 at 0x30!)

#### Power

| ESP32 | WM8960 Board |
|-------|--------------|
| 3V3 | VCC |
| GND | GND |

#### Speaker Output

| WM8960 Board | Speaker |
|---------------|---------|
| SPK_L | Speaker + |
| SPK_R | Speaker - |

**Recommended:** 8Ω 1W speaker

---

## Complete External Wiring Summary

```mermaid
flowchart TB
    subgraph External["External Connections"]
        direction TB

        subgraph Power["Power & Programming"]
            USB[USB-C<br/>Programming]
            BAT[3.7V Battery<br/>MX1.25]
        end

        subgraph I2C["I2C Bus (GPIO8, GPIO10)"]
            direction LR
            A1[PCF8574<br/>Buttons]
            A2[WM8960<br/>Audio]
        end

        subgraph I2S["I2S Audio (GPIO4-7, 15)"]
            I2S_Audio[→ WM8960]
        end

        subgraph Buttons["Buttons"]
            Btn[4x Tactile<br/>→ PCF8574]
        end

        subgraph AudioOut["Audio Output"]
            direction LR
            Spkr[Speaker<br/>8Ω]
            Hp[Headphones<br/>3.5mm]
        end
    end
```

---

## Pin Reference Table

### ESP32 GPIO Summary

| GPIO | Function | Used By |
|------|----------|---------|
| GPIO0 | PWR Button | Onboard |
| GPIO4 | I2S BCLK | ES8311 / WM8960 |
| GPIO5 | I2S DOUT | ES8311 / WM8960 |
| GPIO6 | I2S DIN | ES8311 / WM8960 |
| GPIO7 | I2S MCLK | ES8311 / WM8960 |
| GPIO8 | I2C SDA | **All I2C devices** |
| GPIO10 | I2C SCL | **All I2C devices** |
| GPIO15 | I2S WCLK | ES8311 / WM8960 |
| GPIO21 | LCD Reset | Display |
| GPIO41 | PA Enable | Amplifier |

### J3 Header (8-pin)

| Pin | Signal | GPIO | Use |
|-----|--------|------|-----|
| 1 | IO1 | GPIO4 | I2S (shared) |
| 2 | IO2 | GPIO5 | I2S (shared) |
| 3 | IO3 | GPIO6 | I2S (shared) |
| 4 | GND | - | Ground |
| 5 | TX | GPIO7 | UART/I2S MCLK |
| 6 | RX | GPIO8 | UART/I2C SDA |
| 7 | 3V3 | - | 3.3V power |
| 8 | GND | - | Ground |

---

## Quick Connect Checklist

- [ ] **I2C:** GPIO8 → PCF8574 SDA, GPIO10 → PCF8574 SCL, 3V3 → VCC, GND → GND
- [ ] **I2S:** GPIO4 → WM8960 BCLK, GPIO5 → WM8960 DIN, GPIO6 → WM8960 DOUT, GPIO7 → WM8960 MCLK, GPIO15 → WM8960 WS
- [ ] **I2C (WM8960):** GPIO8 → WM8960 SDA, GPIO10 → WM8960 SCL
- [ ] **Power (WM8960):** 3V3 → VCC, GND → GND
- [ ] **Buttons:** PCF8574 P0-P3 → 4 buttons → GND
- [ ] **Speaker:** WM8960 SPK_L/R → 8Ω speaker
