# Hardware Findings

Verified configuration details for the Waveshare ESP32-S3-Touch-AMOLED-1.75 board and peripherals. Each finding documents a non-obvious behavior discovered during development.

---

## Display (CO5300 AMOLED)

### CO5300 Requires 6-Pixel Column Offset

**Symptom:** A white/green band appears along the top edge of the round display. The black background starts a few pixels below the physical edge, as if the framebuffer origin doesn't align with the panel.

**Root Cause:** The CO5300's internal framebuffer is misaligned by 6 columns relative to the physical display area. Without correcting this, the last columns of pixel data wrap around and render as artifacts at the panel edge.

**Fix:** Pass `col_offset1 = 6` to the `Arduino_CO5300` constructor:

```cpp
gfx = new Arduino_CO5300(
    bus,
    LCD_RESET,      // RST
    0,              // Rotation
    false,          // IPS mode off for AMOLED
    LCD_WIDTH,      // 466
    LCD_HEIGHT,     // 466
    6,              // col_offset1 — required for CO5300 framebuffer alignment
    0,              // row_offset1
    0,              // col_offset2
    0               // row_offset2
);
```

**Source:** Waveshare's own example code (`01_Hello_world.ino`) includes this offset. Also confirmed by ESPHome issue [#15765](https://github.com/esphome/esphome/issues/15765) which documents the same artifact and fix (`esp_lcd_panel_set_gap(panel_handle, 6, 0)`).

**Note:** Waveshare bundles an older version of Arduino_GFX where the `Arduino_CO5300` constructor does not have the `ips` parameter. The upstream library (moononournation/Arduino_GFX) added `ips` as the 4th parameter, shifting all subsequent arguments. If upgrading the library, ensure the `ips` parameter is accounted for.

---

## Audio

### ES8311 SDPOUT Bus Contention on GPIO10

**Symptom:** ES7210 mic data is all zeros. GPIO10 reads as DRIVEN LOW even with internal pull-up enabled.

**Root Cause:** The ES8311 DAC and ES7210 ADC share GPIO10 — ES8311's SDPOUT (data output) and ES7210's SDOUT1 are both wired to the same pin on the Waveshare PCB. When the ES8311 is in CSM mode (REG00=0x80), it auto-configures REG0A to 0x0C, which leaves bit 6 (tri-state control) cleared. The ES8311 actively drives GPIO10 low, overpowering any signal from the ES7210.

**Fix:** Set ES8311 REG0A bit 6 to tri-state the SDPOUT pin, freeing GPIO10 for the ES7210. Because CSM mode auto-configures registers, the tri-state must be forced AFTER CSM stabilizes:

```cpp
// During ES8311 init:
es8311_write_reg(ES8311_SDPOUT_REG0A, 0x40);  // Tri-state SDPOUT

// After CSM stabilizes (~50ms):
delay(50);
es8311_write_reg(ES8311_SDPOUT_REG0A, 0x40);  // Re-force tri-state
uint8_t r0a = es8311_read_reg(ES8311_SDPOUT_REG0A);
if (!(r0a & 0x40)) {
    // CSM overrode — exit CSM and force
    es8311_update_reg(ES8311_RESET_REG00, 0x80, 0x00);
    delay(5);
    es8311_write_reg(ES8311_SDPOUT_REG0A, 0x40);
}
```

Additionally, disable the ES8311's ADC entirely since we use the ES7210 for mic input:
```cpp
es8311_write_reg(ES8311_SYSTEM_REG14, 0x00);  // Disable analog MIC
es8311_write_reg(ES8311_ADC_REG15, 0x00);     // ADC power off
```

**Verified:** After fix, REG0A reads back 0x40 (tri-state confirmed). GPIO10 float test shows the pin is no longer driven by the ES8311.

---

### I2S Must Use Port 1 (I2S_NUM_1), Not Port 0

**Symptom:** BCLK (GPIO9) and WS (GPIO45) do not toggle on the physical pins despite correct GPIO matrix routing. I2S reads return all zeros.

**Root Cause:** The Waveshare BSP defaults to `I2S_NUM_1` (configurable via `BSP_I2S_NUM`). With `I2S_NUM_0`, GPIO9/GPIO45 do not produce clock signals even though the GPIO matrix registers show the correct I2S signal IDs routed to those pins. The exact mechanism is unclear (possibly a peripheral clock gate or pin conflict), but switching to `I2S_NUM_1` resolves it.

**Fix:**
```cpp
static const i2s_port_t I2S_PORT = I2S_NUM_1;  // Not I2S_NUM_0
```

**Verified:** After switching to I2S_NUM_1, GPIO10 (DIN) shows toggling data (highs=168/10000), and I2S reads return non-zero values (range [-1, 0]).

---

### ES7210 Init Sequence Must Match ESPHome Reference (SUPERSEDES Earlier Clock Fix)

**Symptom:** ES7210 outputs ADC idle noise (`min=-1 max=0`) but no real mic signal, despite correct I2C responses and GPIO10 toggling.

**Root Cause:** Multiple register value and ordering errors in our init sequence, discovered by comparing against [ESPHome's working ES7210 driver](https://github.com/esphome/esphome/tree/dev/esphome/components/es7210) and a [confirmed working ESPHome config for the same board](https://github.com/abramosphere/Home-Assistant-Voice-for-ESP32-S3-Touch-AMOLED-1.75).

| Register | Our value | ESPHome value | Impact |
|----------|-----------|---------------|--------|
| REG40 (Analog Power) | 0x43 | **0xC3** | **Bit 7 missing — likely disables analog reference/VMID** |
| REG02 (MAINCLK) | 0xC1 (adc_div=1) | **0xC3 (adc_div=3)** | Wrong clock divider for 12.288MHz/16kHz |
| REG04/05 (LRCK div) | Not written | **0x03/0x00** | LRCK divider unset |
| REG06 (Power Down) | 0x00 | **0x04** | DLL should be powered down |
| REG00 (after reset) | 0x41 | **0x32** | Different intermediate state |

Additionally, our init ordering was wrong — we wrote clock config before I2S format, and mic power before clock un-gate. ESPHome's sequence is:
1. Reset → HPF → slave mode → analog power (0xC3) → mic bias
2. I2S format (REG11/12) → clock config (REG02/07/04/05)
3. Gain config → mic power (REG47-4A) → DLL power down (REG06=0x04) → ADC+PGA power (REG4B/4C=0x0F)
4. Enable device (REG00=0x71 then 0x41)

**Earlier "skip clock config in slave mode" approach was wrong** — while the Espressif `esp_codec_dev` driver does this, ESPHome's working driver writes all clock registers. The esp_codec_dev approach may depend on specific I2S driver behavior that differs between legacy and new APIs.

**Fix:** Rewrote `ES7210_init()` to match ESPHome's register values and ordering exactly. Key changes:
```cpp
es7210_write_reg(ES7210_ANALOG_REG40, 0xC3);     // Was 0x43 — bit 7 critical
es7210_write_reg(ES7210_MAINCLK_REG02, 0xC3);     // Was 0xC1 — adc_div=3
es7210_write_reg(ES7210_LRCK_DIVH_REG04, 0x03);   // Was missing
es7210_write_reg(ES7210_LRCK_DIVL_REG05, 0x00);   // Was missing
es7210_write_reg(ES7210_POWER_DOWN_REG06, 0x04);   // Was 0x00
```

**Source:** ESPHome `es7210.cpp` (setup function), ESPHome `es7210_const.h` (coefficient table for {12288000, 16000}), confirmed working config by [abramosphere](https://github.com/abramosphere/Home-Assistant-Voice-for-ESP32-S3-Touch-AMOLED-1.75).

**Verified (2026-04-30):** After flashing the fixed init, mic levels jumped from `min=-1 max=0` to `min=-1951 max=1669` — real audio signal. Ambient noise produces ±200, speech peaks ±1500. Both L and R I2S channels carry signal. The REG40 bit 7 fix (0x43→0xC3) was the critical change that enabled the analog front-end.

---

### ES7210 Gain Registers Need PGA Enable Bit (0x10)

**Symptom:** Very low mic signal despite high gain setting.

**Root Cause:** The Espressif BSP driver writes gain registers (REG43-REG46) with `gain | 0x10`. Bit 4 (0x10) appears to be a PGA enable flag. Without it, the PGA may not be fully active.

**Fix:**
```cpp
es7210_write_reg(ES7210_MIC1_GAIN_REG43, gain | 0x10);
es7210_write_reg(ES7210_MIC2_GAIN_REG44, gain | 0x10);
// etc.
```

**Verified:** Register dump shows REG43-REG46 = 0x18 (gain=8 + PGA enable=0x10) after fix.

---

### Diagnostic gpio_set_direction() Destroys I2S Clock Routing

**Symptom:** After running the audio diagnostic, BCLK/WS stop reaching external codecs. Mic data goes to all zeros.

**Root Cause:** Calling `gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT)` on an I2S output pin internally calls `esp_rom_gpio_connect_out_signal(pin, SIG_GPIO_OUT_IDX, ...)`, which reconnects the pin to the GPIO output register instead of the I2S peripheral's output signal. The I2S peripheral continues running internally, but its BCLK/WS signals no longer reach the physical pins.

The MCLK diagnostic test (Test 1) survives because it explicitly reconnects the I2S signal with `esp_rom_gpio_connect_out_signal(pin, mck_out_sig, ...)`. The BCLK/WS test did not do this.

**Fix:** Never call `gpio_set_direction()` on I2S clock pins. Use `gpio_get_level()` directly — `i2s_set_pin()` already configures them as INPUT_OUTPUT. Always call `i2s_set_pin()` at the end of any diagnostic to restore routing.

---

### BCLK/WS Cannot Be Read by gpio_get_level() (False Negative)

**Symptom:** Diagnostic Test 5 reports BCLK and WS as "NOT TOGGLING" (highs=0, lows=10000), but I2S data transfer is clearly working (non-zero I2S reads, GPIO10 toggling with ES7210 data).

**Root Cause:** On ESP32-S3, `gpio_get_level()` may not be able to read the state of pins driven by the I2S peripheral, even when the GPIO matrix routing is verified correct (signal IDs match). The GPIO matrix output selection registers confirm the I2S signals are routed to the correct pins, and explicit `esp_rom_gpio_connect_out_signal()` reconnection doesn't help. This appears to be a limitation of reading I2S output pins via the GPIO input register on ESP32-S3.

**Impact:** Diagnostic only — does not affect actual audio functionality. BCLK/WS ARE reaching the codecs (proven by ES7210 outputting data on GPIO10).

**Workaround:** Do not rely on `gpio_get_level()` for BCLK/WS verification. Instead, verify I2S clock operation indirectly by checking if the ES7210 SDOUT1 (GPIO10) shows toggling data.

---

### GPIO16 Is NOT an MCLK Pin

**Symptom:** MCP23017 I2C bus (Wire2 on GPIO16/17) may conflict with MCLK routing.

**Root Cause:** Early code routed MCLK to GPIO16 in addition to GPIO42, based on a misreading of the schematic. GPIO16 is actually I2C2_SDA for the MCP23017 GPIO expander, not an MCLK output.

**Fix:** Remove GPIO16 MCLK routing. MCLK is on GPIO42 only (confirmed by Waveshare BSP).

**Source:** Waveshare BSP pin definitions and schematic verification.

---

### APLL Must Stay Enabled With Legacy I2S Driver

**Symptom:** With `use_apll = false`, loop playback sounds "extremely lo-fi, undiscernable" despite mic levels being healthy in serial logs.

**Root Cause:** Without APLL, the ESP32-S3 I2S peripheral generates MCLK from the APB clock (80MHz). For a target of 12.288MHz, 80M/12.288M = 6.51 — not an integer. The actual MCLK will be either 80/6=13.33MHz or 80/7=11.43MHz, an 8-9% error. Both the ES8311 and ES7210 derive their internal timing from MCLK, so the wrong frequency causes garbled audio output.

The ESPHome config uses `use_apll: false`, but ESPHome runs the **new** ESP-IDF I2S driver (`i2s_std.h`), which may handle MCLK generation differently (e.g., different clock sources or fractional dividers). The legacy driver (`driver/i2s.h`) that we use requires APLL for precise audio clock generation.

**Fix:** Keep APLL enabled:
```cpp
i2sConfig.use_apll = true;   // Required for exact 12.288MHz MCLK
i2sConfig.fixed_mclk = 12288000;
```

**Verified:** With APLL on + fixed ES7210 init, mic captures real audio. With APLL off, same mic levels in serial but playback is garbled.

---

### I2S Must Use 32-Bit Slots (Not 16-Bit)

**Symptom:** Audio playback sounds "robotic," with constant bit-grunge noise and extreme aliasing, despite correct mic levels and working DAC init.

**Root Cause:** The ES8311 coefficient table for {12288000, 16000} has `bclk_div=4`, which means internal_MCLK / BCLK = 4. With internal_MCLK = 4.096MHz (12.288MHz / pre_div=3 × pre_mult=1), the expected BCLK = 1.024MHz. This is 32-bit I2S slots (32 BCLK per channel × 2 channels × 16kHz = 1.024MHz).

With `I2S_BITS_PER_SAMPLE_16BIT` and default `bits_per_chan`, the ESP32 generates 16-bit slots (BCLK = 512kHz) — half what the codec expects. The ES8311's internal clock tree is fundamentally mismatched: it samples I2S data at wrong bit positions, producing garbled output.

ESPHome's new I2S driver (`i2s_std.h`) defaults to `I2S_SLOT_BIT_WIDTH_AUTO` = 32-bit slots for Philips I2S. The legacy driver (`driver/i2s.h`) defaults to slot width = data width = 16.

**Fix:** Set `bits_per_chan` to 32-bit while keeping `bits_per_sample` at 16-bit:
```cpp
i2sConfig.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
i2sConfig.bits_per_chan = I2S_BITS_PER_CHAN_32BIT;  // Philips I2S standard
```

The HAL functions then convert between 32-bit I2S DMA buffers and the 16-bit API:
```cpp
// Read: extract 16-bit data from MSB of 32-bit slot
buffer[i] = (int16_t)(dma32[i] >> 16);
// Write: pack 16-bit data into MSB of 32-bit slot
dma32[i] = (int32_t)buffer[i] << 16;
```

**Source:** ESPHome `i2s_audio_speaker.cpp` uses `I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG` which sets 32-bit slot width. ES8311 coefficient table `bclk_div=4` confirms codec expects 1.024MHz BCLK (32-bit slots at 16kHz).

---

### ES8311 Coefficient Table: ESPHome vs Espressif esp_codec_dev

**Symptom:** Lo-fi DAC output quality even with correct MCLK and init order.

**Root Cause:** The Espressif `esp_codec_dev` driver and ESPHome's ES8311 component have different coefficient tables for {12288000, 16000}:

| Field | Espressif | ESPHome | Impact |
|-------|-----------|---------|--------|
| pre_mult | 0x00 | **0x01** | PLL pre-multiplier wrong — internal clock frequency off |
| dac_osr | 0x10 | **0x20** | DAC oversampling halved — worse reconstruction filter |

**Fix:** Use ESPHome's coefficients:
```cpp
// ESPHome (correct): {0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x20}
// Espressif (wrong): {0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xFF, 0x04, 0x10, 0x10}
```

---

### ES8311 CSM Power-On (REG00=0x80) Must Be Last

**Symptom:** Codec registers get overwritten after init.

**Root Cause:** Writing `REG00=0x80` enters CSM (Codec State Machine) mode, which auto-configures several registers. If written before clock/format registers are set, CSM runs with defaults and may override subsequent writes. ESPHome writes 0x80 as the last step after all configuration.

**Fix:** Move `REG00=0x80` to after all register configuration. Then re-force SDPOUT tri-state since CSM may override REG0A.

---

### Waveshare BSP Uses New I2S Driver API

**Note for future work:** The official Waveshare BSP uses the new ESP-IDF I2S driver (`driver/i2s_std.h` with `i2s_new_channel()`) rather than the legacy driver (`driver/i2s.h` with `i2s_driver_install()`). The legacy driver still works but may have subtle behavior differences on ESP32-S3. If issues persist, migrating to the new driver should be considered.

The BSP also defaults to 22050 Hz sample rate with mono I2S slot configuration.

---

## Handoff: Current State & Open Questions

### What Works (as of 2026-04-30)
- **ES7210 ADC (microphones): WORKING** — Real audio signal captured. Ambient ±200 LSBs, speech peaks ±1500 LSBs. Both L and R I2S channels carry signal.
- ES8311 DAC: Initializes, SDPOUT tri-stated (REG0A=0x40), volume set
- I2S on I2S_NUM_1 with APLL: Driver installs, DMA runs, full-duplex TX+RX
- MCLK on GPIO42 via APLL: Exact 12.288MHz confirmed
- GPIO matrix: Correct signal routing verified for all I2S pins
- Loop capture: RetroactiveBuffer captures and plays back audio
- Speaker output: Produces sound during loop playback

### What Needs Work
- **Loop playback quality needs re-test**: Three critical ES8311 fixes applied in this session (32-bit I2S slots, coefficient table, CSM init order). These address the root causes of "robotic/lo-fi" output.
- **Speaker/DAC path not independently confirmed**: Test tone was disabled at user request. No isolated speaker test has been done — the only audio heard was loop playback, which involves the full mic→buffer→DAC chain.

### Fixes Applied (2026-04-30, Session 2)
1. **32-bit I2S slots** (`bits_per_chan = I2S_BITS_PER_CHAN_32BIT`) — The codec coefficient table expects BCLK=1.024MHz (32-bit slots). We were generating 512kHz (16-bit slots), causing the ES8311 to sample I2S data at wrong bit positions.
2. **ES8311 coefficient table** — Switched from Espressif `esp_codec_dev` coefficients to ESPHome's (pre_mult=0x01, dac_osr=0x20). The Espressif values had wrong PLL multiplier and halved DAC oversampling.
3. **ES8311 init order** — Moved `REG00=0x80` (CSM power-on) to end of init sequence, per ESPHome. Prevents CSM from auto-configuring registers with defaults.
4. **REG1C=0x6A** — Added ADC EQ bypass + DC offset cancel per ESPHome.

### If Quality Still Not Acceptable
1. **Enable test tone** to confirm DAC/PA/speaker path independently
2. **Try single-mic mode** — use only L channel to rule out L+R phase cancellation
3. **Migrate to new I2S driver** (`driver/i2s_std.h`) — requires upgrading to Arduino ESP32 core 3.x (ESP-IDF 5.x). Current framework is Arduino 2.0.17 (ESP-IDF 4.4) which only has the legacy driver.

### Files Modified
- `firmware/platforms/esp32/audio_esp32.cpp` — 32-bit I2S slots, ESPHome coefficients, CSM init order, REG1C, 32-bit DMA buffers
- `firmware/platforms/esp32/es7210.cpp` — ESPHome-aligned init (REG40=0xC3, REG02=0xC3, REG04/05, REG06=0x04, correct ordering)
- `firmware/src/main.cpp` — Diagnostic ordering (before audio task), test tone disabled
- `firmware/src/core/audio_engine.cpp` — Test tone duration (5s)

### ESPHome Reference
A confirmed working ESPHome config for this exact board: [abramosphere/Home-Assistant-Voice-for-ESP32-S3-Touch-AMOLED-1.75](https://github.com/abramosphere/Home-Assistant-Voice-for-ESP32-S3-Touch-AMOLED-1.75). Uses ES8311 for DAC + ES7210 for mic, same pin assignments as our code. Key config: 16kHz, 16-bit, `use_apll: false` (but this only works with the new I2S driver), `mic_gain: 30db`, `pdm: false`.

---

## I2C Bus

*(Findings to be added as they are discovered.)*

---

## IMU (QMI8658)

*(Findings to be added as they are discovered.)*

---

## Touch (CST9217)

*(Findings to be added as they are discovered.)*
