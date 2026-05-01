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

> **READ THIS SECTION FIRST.** The "ES7210 Init Order" finding directly below
> is the **single most load-bearing piece** of getting microphone capture
> working on this board. Earlier findings in this document describe partial
> fixes, dead-ends, and one outright wrong claim ("ESP32-S3 has no APLL" — it
> does). Where the older findings conflict with the **Definitive Working
> Configuration** below, the newer section wins.

---

### ⭐ DEFINITIVE: ES7210 Init Order Determines Whether Mic Capture Sounds Like Audio Or Noise

**Status:** AUTHORITATIVE — verified working 2026-04-30. **Treat this as the
reference; do not regress.**

**Symptom of the broken state:** Microphone capture played back through the
speaker sounds like *bit-crushed digital noise / EM-sniffer-style aliasing /
high-frequency garbage*. Test tone (DAC-only path) sounds perfect. The
diagnostic shows non-zero, internally-correlated samples on the I2S RX path
(`FD23F700`-style values), so the chip is producing *something* — but the
something doesn't resemble the acoustic input at all. Speech is unrecognizable.

#### What actually fixed it: ES7210 init order matches Espressif `esp-bsp` exactly

The previous init wrote registers in this order:

1. Reset
2. **`REG01 = 0x3F` — gate all clocks**  ← *the wrapping mistake*
3. Timing, HPF
4. Mode config (`REG08 = 0x00`)
5. Analog power (`REG40 = 0xC3`)
6. Mic bias
7. **Clock config** (`REG02/07/04/05`) — *while clocks gated*
8. **I2S format** (`REG11/12`) — *while clocks gated*
9. Mic gain
10. Mic power
11. DLL power-down (`REG06 = 0x04`)
12. ADC + PGA power (`REG4B/4C = 0x0F`)
13. **`REG01 = 0x00` — un-gate clocks**  ← only here did the digital domain come up
14. Enable (`REG00 = 0x71` then `0x41`)

**The bug:** The ES7210's I2S format register (`REG11`) and clock config
(`REG02/04/05/07`) were written while the digital clock domain was gated by
`REG01 = 0x3F`. The register *bytes* updated over I2C and read back correctly
in our diagnostic, but **the format setting never propagated into the
chip's internal data path** because the digital pipeline wasn't clocked. When
clocks finally came up at step 13, the chip ran with whatever default/stale
format state the data path happened to be in. The result was audio data that
*looked* coherent in the I2C register dump but was *structurally wrong* on the
I2S serial output — exactly the "EM-sniffer / bit-crushed" symptom.

**The fix:** Match the Espressif `esp-bsp/components/es7210/es7210.c` order
exactly. Critically:

- **Do not write `REG01` at all** — leave clock gating at the post-reset default.
- **Write the I2S format register (`REG11/12`) FIRST**, immediately after the
  HPF coefficients and before any analog/clock setup.
- **Write the clock config (`REG07` OSR, `REG02` MAINCLK, `REG04/05` LRCK) LAST**,
  after analog power, mic bias, gain, and individual mic power are all set.
- Do not touch `REG08` (mode config) or `REG03` (master clock select) — Espressif
  leaves both at their reset defaults.

The verified-working order, exactly mirroring `es7210_config_codec()` in
`esp-bsp`:

```
1.  REG00 = 0xFF                     // software reset
    REG00 = 0x32                     // exit reset to intermediate state
2.  REG09 = 0x30, REG0A = 0x30       // power-up timing
3.  REG23 = 0x2A, REG22 = 0x0A       // ADC1/2 HPF
    REG21 = 0x2A, REG20 = 0x0A       // ADC3/4 HPF
4.  REG11 = (bit_width | i2s_format) // I2S format — MUST come before clock config
    REG12 = 0x00 (or TDM value)      // SDP interface 2
5.  REG40 = 0xC3                     // analog power & VMID
6.  REG41 = 0x70, REG42 = 0x70       // MIC bias (2.87V for MEMS)
7.  REG43..46 = (gain | 0x10)        // PGA gain — 0x10 is the PGA enable bit
8.  REG47..4A = 0x08                 // individual mic power
9.  REG07 = 0x20                     // OSR (LAST among clock regs)
    REG02 = 0xC3                     // MAINCLK = adc_div(3) | doubler<<6 | dll<<7
    REG04 = 0x03, REG05 = 0x00       // LRCK divider for 16kHz from 12.288MHz
10. REG06 = 0x04                     // power down DLL (per Espressif sequence)
11. REG4B = 0x0F, REG4C = 0x0F       // MIC1/2 + MIC3/4 ADC + PGA power
12. REG00 = 0x71, then REG00 = 0x41  // enable device
```

**For other sample rate / MCLK combinations**, the values for `REG02/07/04/05`
come from the lookup table in `esp-bsp` `es7210_coeff_div[]`. Build `REG02`
with `(adc_div) | (doubler << 6) | (dll << 7)`. The local copy of this table
in our `es7210.cpp` had **wrong field values** for `{12.288MHz, 16kHz}`
(`adc_div=0x01, dll=0, doubler=0, osr=0x06` instead of `adc_div=0x03, dll=1,
doubler=1, osr=0x20`). Don't trust that table; trust the upstream Espressif
header values.

#### Things we tried first that did NOT fix the bit-crushed output

These are documented so future debugging doesn't re-tour them. **None** of them
restored intelligible audio on their own. Some may still be necessary (✅) for
the working state; some were red herrings (❌).

| Attempt | Result | Should-Keep? |
|---|---|---|
| Reduce software gain `* 16 → * 4` (chasing "clipping" hypothesis) | No change in quality. Real symptom was structural corruption, not amplitude clipping. | ✅ Kept at `* 4` — sane default for ambient + speech levels at 30dB PGA. |
| Re-enable APLL (`use_apll = true` + `fixed_mclk = 12288000`) | Diagnostic showed cleaner MCLK duty cycle (1854/8146 → 4228/5772) but mic playback still garbled. | ✅ **Required.** ESP32-S3 *does* have APLL — earlier comment claiming otherwise was wrong. Without APLL the MCLK is ~0.16% off (PLL_160M / 13.02), which the DAC tolerates but the ADC's sigma-delta + decimation chain does not. |
| Revert I2S clock pin drive strength `CAP_2 → CAP_1` | No audible improvement, but theoretically reduces EMI coupling into the adjacent analog mic traces. Also matches the previously-confirmed working state. | ✅ Kept at `CAP_1` for MCLK/BCLK/WS/DOUT. The crackling-fix commit's `CAP_2` change was unnecessary. |
| Disable `MIC3`/`MIC4` (only enable `MIC1 \| MIC2` in `es7210_cfg.mics`) | No audible improvement on its own. | ✅ Kept. Schematic confirms MIC3/MIC4 are wired to the AEC feedback path (filtered speaker output via R37/R38/R39/R40/C75–C83), not real microphones. Enabling channels we don't read is wasteful and arguably can confuse the decimation pipeline. |
| Increase sample rate / bit depth | Not attempted — analyzed and rejected. | ❌ Wouldn't have helped. The corruption happens at the chip's internal pipeline, before any storage truncation. No standard sample rate gives an integer MCLK divider on ESP32-S3 anyway; APLL is the only path to precision regardless of rate. |
| Switch to ES8311 ADC for mic input | Aborted after schematic check. | ❌ **Not viable on this board.** ES8311 pins 17/18 (`MIC1N`, `MIC1P/DMIC_SDA`) are unrouted. Both onboard mics (`MIC1`, `MIC2`) connect *only* to ES7210 pins 15/16 and 19/20. The factory firmware that "sounded fine" must also have been using ES7210. |

#### Definitive working configuration: full file/setting checklist

If the audio path ever regresses, **verify all of these**. Any single one can
re-introduce the bit-crushed symptom.

**`firmware/platforms/esp32/audio_esp32.cpp` — I2S driver setup:**

| Setting | Required value | Why |
|---|---|---|
| `I2S_PORT` | `I2S_NUM_0` | Single full-duplex port. (Earlier "must be `I2S_NUM_1`" finding applied to a dual-port attempt that has since been reverted.) |
| `i2s_mode` | `I2S_MODE_MASTER \| I2S_MODE_TX \| I2S_MODE_RX` | Full-duplex; needed because ES7210 RX and ES8311 TX share BCLK/WS. |
| `bits_per_sample` | `I2S_BITS_PER_SAMPLE_32BIT` | 32-bit DMA slots. ES7210 outputs 24-bit data left-aligned in 32-bit slots; we extract via `>> 16` to get 16-bit. |
| `channel_format` | `I2S_CHANNEL_FMT_RIGHT_LEFT` | Standard stereo interleave. |
| `communication_format` | `I2S_COMM_FORMAT_STAND_I2S` | Philips I2S (1-bit delay). Must match ES7210 `REG11` format=0 and ES8311 SDPIN format. |
| `dma_buf_count` | `16` | Full-duplex needs more headroom. |
| `dma_buf_len` | `512` | Same reason. |
| `use_apll` | **`true`** | **Required for clean ADC.** Without APLL, MCLK is non-integer-divided from PLL_160M. DAC tolerates the error; ADC's sigma-delta does not. |
| `fixed_mclk` | **`12288000`** | Tells APLL to lock exactly to 12.288 MHz so codec coefficient tables are valid. |
| `tx_desc_auto_clear` | `true` | Plus an explicit `i2s_start()` after `i2s_zero_dma_buffer()` — this is what keeps BCLK/WS toggling continuously, fixing the earlier crackle issue. |
| GPIO drive strength on MCLK/BCLK/WS/DOUT | **All `GPIO_DRIVE_CAP_1`** | Stronger drive (`CAP_2`) couples high-frequency switching noise into the differential mic traces that run nearby. |

**`firmware/platforms/esp32/es7210.cpp` — chip init order:**

Match the Espressif sequence exactly (see "What actually fixed it" above).
**Do not write `REG01`, `REG03`, or `REG08` at all.** The init must end with
`REG00 = 0x71` then `REG00 = 0x41` and **must not** wrap any of the format /
clock / mic-power writes inside a clock-gating bracket.

**`firmware/platforms/esp32/audio_esp32.cpp` — ES7210 config struct:**

```cpp
es7210_cfg.mics       = ES7210_MIC1 | ES7210_MIC2;  // not ES7210_MIC_ALL
es7210_cfg.gain       = ES7210_GAIN_30DB;
es7210_cfg.bits       = ES7210_BITS_32;
es7210_cfg.i2s_format = ES7210_I2S_NORMAL;
es7210_cfg.enable_tdm = false;
es7210_cfg.mclk_freq  = 12288000;
es7210_cfg.sample_rate = (es7210_sample_rate_t)16000;
```

**ES8311 init (already in `es8311_init_codec()`):**

- Tri-state ES8311 SDPOUT (`REG0A` bit 6) **after** CSM stabilizes — see the
  earlier "ES8311 SDPOUT Bus Contention on GPIO10" finding.
- Disable ES8311 ADC blocks (`REG14/15/16/17 = 0x00`) so it doesn't drive
  GPIO10 against the ES7210.
- 32-bit I2S slots with 16-bit data (data in MSB) — write via
  `(int32_t)sample << 16`.

**`firmware/src/core/audio_engine.cpp` — capture path:**

- Software gain `((L+R)/2) * 4`. Tunable. `* 16` clips speech, `* 1` is too
  quiet. `* 4` keeps normal speech at –9 to –21 dBFS post-gain.
- Mono'd into the retroactive buffer (`int16_t`, PSRAM, 10 s @ 16 kHz).

#### Reference

- **Espressif `esp-bsp` ES7210 driver** — the source of truth for register
  values and init sequence: <https://raw.githubusercontent.com/espressif/esp-bsp/master/components/es7210/es7210.c>
- **Schematic** confirms `MIC1`/`MIC2` (the two onboard MEMS mics) → ES7210
  pins 15/16, 19/20 (analog differential, AC-coupled through `C99/C100/C103/C107`).
  `MIC3`/`MIC4` → AEC filter network (`R37/R38/R39/R40/C75/C76/C79–C83`) →
  ES7210 pins 27/28, 31/32. ES8311 mic pins (17/18) are unrouted on this PCB.

---

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

### Audio Crackle Fix: Three Root Causes (2026-04-30, Session 3)

**Symptom:** Loop playback audio is extremely crackly and lo-fi, despite mic levels being healthy and all previous codec/I2S slot/coefficient fixes applied.

**Root Cause 1 — `vTaskDelay` in audio task causes DMA underruns (CRITICAL):**
The audio task loop called `vTaskDelay(pdMS_TO_TICKS(1))` between each `AudioEngine_process()` iteration. This yields the CPU for at minimum 1 FreeRTOS tick, during which no data feeds the I2S TX DMA. When other tasks (display, touch, IMU) momentarily delay the scheduler, the audio task sleeps 2-5ms instead of 1ms, causing audible gaps. The correct pattern for audio tasks is to let the blocking `i2s_read()`/`i2s_write()` calls provide pacing — they block until a DMA buffer is available, which is inherently synchronized to the audio sample clock.

**Fix:** Removed `vTaskDelay` from `audioTaskFunction()`.

**Root Cause 2 — `fixed_mclk` forces worst-case fractional divider (HIGH):**
The I2S config had `fixed_mclk = 12288000`. The ESP32-S3 has no APLL, so MCLK is derived from PLL_160M (160 MHz). `160M / 12.288M ≈ 13.02` — not an integer. When `fixed_mclk` is set, the driver forces this exact ratio, causing the divider to oscillate between /13 and /14 every ~50 cycles. This creates periodic clock jitter that both codecs (ES8311 and ES7210) propagate into the audio signal as crackle/artifacts.

**Fix:** Removed `fixed_mclk` from I2S config. Without `fixed_mclk`, the driver auto-calculates MCLK from `sample_rate × channel_count × bits_per_sample`, which may choose a different (potentially more favorable) divider ratio.

**Root Cause 3 — DMA buffers undersized for full-duplex (MODERATE):**
DMA was configured as 12 buffers × 256 samples. In full-duplex mode, both TX and RX share the same I2S peripheral, so effective headroom is halved. Increased to 16 buffers × 512 samples for more DMA runway.

**Additional fix — Clock GPIO drive strength too low:**
All I2S GPIOs (including MCLK at 12.288 MHz) were set to `GPIO_DRIVE_CAP_1` (weakest). At high frequencies, weak drive strength can cause signals to not reach valid logic thresholds, which the codecs see as jittery edges. Changed clock pins (MCLK, BCLK, WS) to `GPIO_DRIVE_CAP_2` (medium), kept data pin (DOUT) at `GPIO_DRIVE_CAP_1`.

**Source:** ESP32 community forums, ESP-IDF I2S documentation, and ESPHome driver analysis all confirm: (1) audio tasks should never use vTaskDelay, (2) ESP32-S3 has no APLL so 12.288 MHz is always approximate from 160 MHz, and (3) full-duplex DMA needs larger buffers.

**Files Modified:**
- `firmware/src/core/audio_engine.cpp` — Removed `vTaskDelay` from audio task loop
- `firmware/platforms/esp32/audio_esp32.cpp` — Removed `fixed_mclk`, increased DMA buffers (16×512), raised clock GPIO drive to CAP_2

---

## I2C Bus

*(Findings to be added as they are discovered.)*

---

## IMU (QMI8658)

*(Findings to be added as they are discovered.)*

---

## Touch (CST9217)

*(Findings to be added as they are discovered.)*
