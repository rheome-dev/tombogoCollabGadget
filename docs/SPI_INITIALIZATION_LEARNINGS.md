# SPI Initialization Learnings - Tombogo Collab Gadget

## Problem Summary

The ESP32-S3-Touch-AMOLED-1.75 display failed to initialize with:
```
E (287) spi: spi_bus_initialize(756): SPI bus already initialized.
ESP_ERROR_CHECK failed: esp_err_t 0x103 (ESP_ERR_INVALID_STATE)
```

## Root Cause

The ESP32 Arduino core or a preincluded library (likely LVGL or SensorLib) initializes the SPI3 (HSPI) bus before user code runs. When the GFX Library's `Arduino_ESP32QSPI::begin()` is called, it attempts to initialize the bus again without checking if it already exists.

## Waveshare Reference Analysis

Examined the [official Waveshare ESP32-S3-Touch-AMOLED-1.75 repository](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75):

- **GFX Library Version**: v1.6.4 (our project uses v1.5.0)
- **Key Observation**: Waveshare examples call `gfx->begin()` directly without any SPI bus pre-initialization
- **Difference**: Their library version is newer and may handle this case better

### What Waveshare Does (Working)

```cpp
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
  LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);

Arduino_CO5300 *gfx = new Arduino_CO5300(
  bus, LCD_RESET, 0, LCD_WIDTH, LCD_HEIGHT, 6, 0, 0, 0);

void setup() {
  // ... other init ...
  gfx->begin();  // Just works in Waveshare's environment
}
```

## The Solution We Implemented

The fix is to **free the SPI bus BEFORE creating the GFX object**, then let the GFX library's `begin()` initialize it fresh:

```cpp
// CRITICAL: Free SPI bus BEFORE creating GFX object
Serial.println("ESP32: Freeing SPI bus (if pre-initialized)...");
esp_err_t freeResult = spi_bus_free(targetHost);
Serial.printf("ESP32: spi_bus_free result: 0x%x\n", freeResult);
delay(10);

// Create the GFX object
qspiBus = new Arduino_ESP32QSPI(...);

// Then begin() - should work now that bus is freed
bool busBegun = qspiBus->begin(speed, dataMode);
```

## Key Learnings

1. **Order Matters**: Call `spi_bus_free()` BEFORE `new Arduino_ESP32QSPI()`, not after
   - Freeing after creates timing issues where GFX library's begin() still sees initialized bus

2. **Error Codes to Expect**:
   - `0x103` (ESP_ERR_INVALID_STATE): Bus was never initialized - safe to ignore
   - `0x0` (ESP_OK): Bus was initialized by something else, now successfully freed

3. **Library Version Matters**: GFX Library v1.6.4 (used by Waveshare) may have fixes not present in v1.5.0
   - Consider upgrading to v1.6.4 as alternative solution

4. **What's Pre-initializing SPI?**: Likely candidates:
   - LVGL's SPI driver
   - SensorLib
   - ESP32 Arduino core default initialization

5. **Why This Works**: By freeing the bus first, we give the GFX library a clean slate. It then successfully calls `spi_bus_initialize()` as if it's the first to use the bus.

## Alternative Solutions (Not Implemented)

1. **Upgrade GFX Library**: Use v1.6.4 which may have better handling
2. **Use Different SPI Host**: Try SPI2 (FSPI) instead of SPI3 (HSPI)
3. **Patch GFX Library**: Modify `Arduino_ESP32QSPI::begin()` to check for existing bus
4. **Find Culprit**: Add early Serial.print in startup to identify what's initializing SPI

## File References

- Working implementation: `firmware/platforms/esp32/display_esp32.cpp`
- Test file: `firmware/src/dead_front_test.cpp`
- Previous doc: `docs/SPI_DOUBLE_INIT_HANDOVER.md`

## References

- [Waveshare ESP32-S3-Touch-AMOLED-1.75 Repo](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75)
- [Arduino GFX Library](https://github.com/moononournation/Arduino_GFX)
- [ESP-IDF SPI Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html)

---

*Documented: 2026-02-27*
*Project: Tombogo x Rheome Collab Gadget*
