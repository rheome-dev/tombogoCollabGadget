# Audio Architecture - Tombogo x Rheome Collab Gadget

**Last Updated:** 2026-02-17

## Hardware Overview

The Waveshare ESP32-S3-Touch-AMOLED-1.75 has **three audio ICs** onboard:

| IC | Type | I2C Address | Function | Status |
|----|------|-------------|----------|--------|
| **ES8311** | Codec (DAC) | 0x30 | Speaker output via I2S | Implemented |
| **ES7210** | 4-ch ADC | 0x40 | Microphone array input | Pending |
| **NS4150B** | Power Amp | N/A (GPIO) | Speaker amplifier | Implemented |

**External module:**

| IC | Board | I2C Address | Function | Status |
|----|-------|-------------|----------|--------|
| **WM8960** | Waveshare Audio Board | 0x1A | Headphone output | Pending |

---

## Onboard Audio Architecture (from Schematic)

```
                    ┌─────────────────────────────────────────────────────────┐
                    │              ESP32-S3-Touch-AMOLED-1.75                  │
                    │                                                          │
  ┌──────────┐      │   ┌──────────┐      I2S       ┌──────────┐             │
  │  MIC 1   │──────┼──►│          │  ◄─────────►  │          │──► SPEAKER  │
  │  MIC 2   │──────┼──►│  ES7210  │    SDOUT      │  ES8311  │    (via     │
  │ (array)  │      │   │  4-ch ADC│               │  Codec   │   NS4150B)  │
  └──────────┘      │   └────┬─────┘               └────┬─────┘             │
                    │        │ I2C (0x40)               │ I2C (0x30)        │
                    │        └──────────┬───────────────┘                   │
                    │                   │                                    │
                    │              I2C Bus (SDA/SCL)                         │
                    └───────────────────┼────────────────────────────────────┘
                                        │
                                        ▼
                              ┌──────────────────┐
                              │     WM8960       │
                              │   Audio Board    │──► HEADPHONES
                              │   (I2C 0x1A)     │
                              └──────────────────┘
```

---

## Pin Configuration (Verified from Schematic 2026-02-17)

### I2S Bus (Shared by ES8311 and ES7210)

```
ESP32-S3 Pin  →  Function
-----------------------------------------
GPIO 8        →  I2S_DSDIN  (data TO ES8311 for speaker output)
GPIO 9        →  I2S_SCLK   (bit clock)
GPIO 10       →  I2S_ASDOUT (data FROM ES7210 for mic input)
GPIO 42       →  I2S_MCLK   (master clock - 12.288 MHz via LEDC)
GPIO 45       →  I2S_LRCK   (word select / left-right clock)
```

### I2C Bus (Shared by all I2C devices)

```
GPIO 14       →  ESP32_SCL (shared: touch, RTC, ES8311, ES7210, QMI8658)
GPIO 15       →  ESP32_SDA (shared: touch, RTC, ES8311, ES7210, QMI8658)
```

### Power Amplifier

```
GPIO 46       →  PA_CTRL (NS4150B enable - HIGH to enable)
```

### SD Card (SPI Mode)

```
GPIO 1        →  SD_MOSI
GPIO 2        →  SD_SCK
GPIO 3        →  SD_MISO
GPIO 41       →  SD_CS
```

**Status:** Pin configuration verified against official schematic pinout table.

---

## ES7210 4-Channel ADC

The ES7210 provides high-quality microphone input:
- **SNR:** 102 dB
- **THD+N:** -85 dB
- **Bit Depth:** 24-bit
- **Sample Rates:** 8 kHz to 100 kHz
- **Interface:** I2S/PCM (TDM supported)
- **Channels:** 4 microphone inputs
- **I2C Address:** 0x40 (default)

### Microphone Connections (from Schematic)

The board has provisions for multiple microphones:
- MIC1_P, MIC1_N (differential pair 1)
- MIC2_P, MIC2_N (differential pair 2)
- MIC3, MIC4 (additional inputs)

### AEC (Acoustic Echo Cancellation)

The schematic shows AEC circuitry connected to the ES7210, enabling echo cancellation for voice applications.

---

## ES8311 Codec (DAC)

The ES8311 handles speaker output:
- **I2C Address:** 0x30
- **Function:** DAC for speaker output
- **Output:** Connected to NS4150B power amplifier
- **MCLK:** Requires master clock input

**Note:** ES8311 also has an internal ADC, but the ES7210 is the primary ADC on this board.

---

## NS4150B Power Amplifier

- **Control:** GPIO46 (PA_CTRL)
- **Output:** Drives onboard speaker
- **Power:** 3W mono Class D amplifier

---

## WM8960 Audio Board (Pending Implementation)

### Specifications

| Parameter | Value |
|-----------|-------|
| I2C Address | 0x1A |
| DAC SNR | 98 dB |
| ADC SNR | 94 dB |
| Sample Rates | 8, 11.025, 22.05, 44.1, 48 kHz |
| Headphone Output | 40 mW @ 16Ω |
| Speaker Output | 1W per channel @ 8Ω BTL |
| Crystal | 24 MHz (onboard) |

### Pin Requirements

The WM8960 needs connection to:
- **I2C:** SDA, SCL (can share with existing I2C bus)
- **I2S:** BCLK, LRCLK, DIN, DOUT

---

## Architecture Options

### Option A: Dual Codec (Current Plan)

```
                    ┌─────────────────┐
  Microphone ──────►│    ES8311      │──────► Onboard Speaker
  (onboard)         │  (I2S_NUM_0)   │        (via PA_EN)
                    └────────┬───────┘
                             │
                         I2C Bus
                             │
                    ┌────────┴───────┐
                    │    WM8960      │──────► Headphones
  Line In ─────────►│  (I2S_NUM_1)   │──────► (Optional) External Speaker
  (external)        └─────────────────┘
```

**Pros:**
- No hardware changes required
- Quick to implement for prototyping
- Fallback if one codec fails

**Cons:**
- Two codecs to initialize and manage
- Two I2S ports consumed
- More complex audio routing
- Potential clock synchronization issues

**Implementation:**
- ES8311 stays on I2S_NUM_0 (current)
- WM8960 on I2S_NUM_1 (needs GPIO allocation)
- Both share I2C bus (different addresses)

### Option B: WM8960 Only (Recommended for Simplicity)

```
                    ┌─────────────────┐
  Microphone ──────►│    WM8960      │──────► Headphones
  (WM8960 board)    │  (I2S_NUM_0)   │──────► Speaker (WM8960 SPK out)
  Line In ─────────►│                │
                    └─────────────────┘
```

**Pros:**
- Single codec = simpler code
- Better audio quality (98dB SNR vs ES8311's ~90dB)
- WM8960 speaker output (1W) is more powerful than ES8311
- One I2S port, one driver
- Built-in headphone amplifier

**Cons:**
- Requires rewiring speaker (cut MX1.25 connector)
- ES8311 goes unused
- Single point of failure

**Implementation:**
- Disable ES8311 initialization
- Wire WM8960 to I2S_NUM_0 pins
- Wire speaker to WM8960 SPK_OUT pins
- Use WM8960's onboard mic input or wire external mic

### Option C: Hybrid (Best Audio Quality)

```
                    ┌─────────────────┐
  Dual Mics ───────►│    ES8311      │──────► (Mic input only)
  (onboard)         │   ADC only     │
                    └────────┬───────┘
                             │
                         I2S Bus (shared)
                             │
                    ┌────────┴───────┐
                    │    WM8960      │──────► Headphones
                    │   DAC only     │──────► Speaker
                    └─────────────────┘
```

**Pros:**
- Best of both worlds
- ES8311's proximity to onboard mics may give better input
- WM8960's superior DAC for output

**Cons:**
- Most complex implementation
- Requires careful I2S bus sharing
- Needs external clock synchronization

---

## Recommendation

**For Prototype Phase:** Use **Option A (Dual Codec)**
- Keep ES8311 for speaker (already working)
- Add WM8960 for headphones only
- Minimal hardware changes
- Test both paths

**For Production/Custom PCB:** Consider **Option B (WM8960 Only)**
- Simpler BOM (one codec)
- Better audio specs
- Easier software maintenance

---

## GPIO Availability for WM8960 (I2S_NUM_1)

ESP32-S3 I2S1 needs 4 pins. Available GPIOs:

| GPIO | Current Use | Available? |
|------|-------------|------------|
| 12 | Unused | Yes |
| 16 | Unused | Yes |
| 17 | TF_DATA2 | Conflict |
| 18 | TF_DATA3 | Conflict |
| 19 | Unused | Yes |
| 20 | Unused | Yes |

**Proposed WM8960 I2S_NUM_1 Configuration:**
```c
#define WM8960_BCLK_PIN     12
#define WM8960_WS_PIN       16
#define WM8960_DOUT_PIN     19  // ESP32 → WM8960 (playback)
#define WM8960_DIN_PIN      20  // WM8960 → ESP32 (recording)
```

---

## Implementation Plan

### Phase 1: Verify ES8311 (Current)
- [x] MCLK generation via LEDC
- [x] ES8311 codec initialization
- [x] I2S driver setup
- [x] PA_EN control
- [ ] Test audio playback
- [ ] Test mic input

### Phase 2: Add WM8960 Driver
- [ ] Create WM8960 driver (wm8960.cpp)
- [ ] Initialize I2S_NUM_1
- [ ] Configure WM8960 for headphone output
- [ ] Test headphone playback
- [ ] Add volume control

### Phase 3: Audio Routing
- [ ] Extend AudioHAL for dual output
- [ ] Add output selection (speaker/headphones/both)
- [ ] Implement audio mixing if needed

---

## References

- [Waveshare ESP32-S3-Touch-AMOLED-1.75 Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75)
- [Waveshare WM8960 Audio Board Wiki](https://www.waveshare.com/wiki/WM8960_Audio_Board)
- [WM8960 Datasheet](https://www.cirrus.com/products/wm8960/)
- [ES8311 Datasheet](https://www.everest-semi.com/pdf/ES8311%20PB.pdf)
