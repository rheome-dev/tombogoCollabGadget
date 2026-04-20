# Waveshare WM8960 Audio Board Hardware Documentation

## Board Overview

| Specification | Value |
|--------------|-------|
| **CODEC** | WM8960 |
| **Operating Voltage** | 3.3V |
| **Control Interface** | I2C |
| **Audio Interface** | I2S |
| **Audio Format** | WAV |
| **DAC SNR** | 98dB |
| **ADC SNR** | 94dB |
| **Earphone Driver** | 40mW (16Ω @ 3.3V) |
| **Speaker Driver** | 1W per channel (8Ω BTL) |

---

## Audio Capabilities

- Stereo encoding/decoding
- Onboard MEMS silicon microphone for recording
- Standard 3.5mm 4-segment earphone jack (CTIA standard)
- Dual-channel speaker interface
- Audio sampling rates: 8, 11.025, 22.05, 44.1, 48 kHz
- Stereo 3D surround sound effects

---

## Pin Connections (Typical)

### Power

| Pin | Description |
|-----|-------------|
| VCC | 3.3V |
| GND | Ground |

### I2C Control (WM8960)

| Signal | Notes |
|--------|-------|
| SDA | I2C data |
| SCL | I2C clock |
| I2C Address | 0x1A (typical for WM8960) |

### I2S Audio

| Signal | Description |
|--------|-------------|
| BCK | Bit clock |
| WS | Word select (LRCK) |
| DIN | Data input (to codec) |
| DOUT | Data output (from codec) |
| MCLK | Master clock (24MHz crystal on board) |

### Audio Outputs

| Connector | Description |
|-----------|-------------|
| Speaker L | Left speaker (BTL) |
| Speaker R | Right speaker (BTL) |
| HP_OUT | 3.5mm headphone jack (4-pole) |

### Audio Input

| Connector | Description |
|-----------|-------------|
| MIC_IN | Microphone input |
| LIN | Line input L |
| RIN | Line input R |

---

## WM8960 I2C Address

The WM8960 typically uses I2C address:

- **0x1A** (7-bit address) - Default
- Can be changed via chip pins (see datasheet)

This is DIFFERENT from the onboard ES8311 (0x30) on the ESP32-S3 AMOLED board.

---

## I2S Configuration

### Recommended Settings

| Parameter | Value |
|-----------|-------|
| Sample Rate | 44100 Hz (or 48000 Hz) |
| Bits Per Sample | 16-bit or 24-bit |
| I2S Mode | I2S_MODE_STD |
| Slot Mode | I2S_SLOT_MODE_STEREO |

### Clock Requirements

- MCLK: 256× or 384× sample rate (from external 24MHz crystal)
- BCLK: 32×, 48×, or 64× sample rate

---

## Connection to ESP32-S3 AMOLED

### Option 1: Use Only WM8960 (Replace ES8311)

Connect WM8960 I2S to ESP32 I2S pins (different from display pins):

```
ESP32-S3 AMOLED     WM8960 Audio Board
-----------         -----------------
GPIO4 (BCLK)  ----> BCLK
GPIO15 (WCLK) ----> WS (LRCK)
GPIO6 (DOUT)  ----> DIN
GPIO7 (DIN)   ----> DOUT
                --> MCLK (from WM8960 24MHz crystal, may not connect)

GPIO8 (SDA)   ----> SDA
GPIO9 (SCL)   ----> SCL
3V3            ----> VCC
GND            ----> GND
```

### Option 2: Use Both (ES8311 + WM8960)

The onboard ES8311 can drive the small speaker, while WM8960 provides headphone output.

- ES8311: Onboard speaker
- WM8960: Headphone jack

This requires no additional I2S wiring (ES8311 already connected).

---

## Integration Notes

### For This Project (Headphone Output)

Given the gadget is a wearable audio device, using WM8960 makes sense for:
1. Better audio quality (98dB SNR vs ES8311)
2. Dedicated headphone output with volume control
3. More powerful speaker amp (1W vs ~200mW)

### Recommended Configuration

1. Keep ES8311 for onboard speaker output
2. Add WM8960 for headphone output
3. Use analog switching or software to route audio

### Required Libraries

- WM8960 Arduino library (or use generic I2C/I2S)
- Standard ESP32 I2S library

---

## Pin Availability Analysis

### ESP32-S3 GPIO Usage Summary

| GPIO | Used For | Status |
|------|----------|--------|
| 0 | PWR button | In use |
| 1 | TF card DI | In use |
| 2 | TF card SCK | In use |
| 3 | TF card DO | In use |
| 4 | I2S BCLK | In use (ES8311) |
| 5 | LCD_RESET / I2S MCLK | In use |
| 6 | LCD_SDIO0 / I2S DOUT | In use |
| 7 | LCD_SDIO1 / I2S DIN | In use |
| 8 | I2C SDA | In use |
| 9 | BOOT_BTN / I2C SCL | In use |
| 10 | ? | Available? |
| 11 | ? | Available? |
| 12 | ? | Available? |
| 13 | ? | Available? |
| 14 | ? | Available? |
| 15 | LCD_SDIO2 / I2S WCLK | In use |
| 16 | LCD_SDIO3 | In use |
| 17 | LCD_SDIO2 | In use |
| 18 | LCD_SDIO3 | In use |
| 21 | PA (amp enable) | In use |
| 38-48 | ? | ESP32-S3 has additional GPIO |

### For WM8960 Connection

Best option: Use **different I2S instance** or share existing I2S.

The ESP32-S3 has 2 I2S controllers. The current setup uses I2S0. For WM8960:
- Option A: Use I2S1 (requires different pins - typically 40-48 range)
- Option B: Share I2S0 (both codecs at once - possible with mux)

### For 4 Buttons

**Available options:**

1. **TCA9554 IO Expander (recommended)**
   - 3 GPIO available on 8-pin header
   - Add external expander for more pins
   - Requires ESP32_IO_Expander library

2. **Analog Button Matrix**
   - Single ADC pin with resistor divider
   - 4-8 buttons with 1 pin
   - Uses analog input (GPIO with ADC)

3. **Available ESP32 GPIO**
   - Check GPIO38-48 for availability
   - May need board revision confirmation

---

## Resources

- Product Page: https://www.waveshare.com/wm8960-audio-board.htm
- Wiki: https://www.waveshare.com/wiki/WM8960_Audio_Board
- WM8960 Datasheet: Available on wiki
- Schematic: Available on wiki

---

## Recommendations

1. **For buttons:** Use TCA9554 IO expander OR analog button circuit
2. **For audio:** Keep ES8311 for speaker, add WM8960 for headphones
3. **I2C conflict:** ES8311 (0x30) and WM8960 (0x1A) have different addresses - no conflict
4. **I2S:** Consider software switching between outputs, or use both simultaneously
