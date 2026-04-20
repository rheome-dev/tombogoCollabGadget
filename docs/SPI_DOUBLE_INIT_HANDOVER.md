# SPI Double-Init Issue - Handoff Document

## Problem Summary

The ESP32-S3-Touch-AMOLED-1.75 display fails to initialize with the error:
```
E (287) spi: spi_bus_initialize(756): SPI bus already initialized.
ESP_ERROR_CHECK failed: esp_err_t 0x103 (ESP_ERR_INVALID_STATE)
```

The GFX Library's `Arduino_ESP32QSPI::begin()` calls `spi_bus_initialize()` which fails because something else has already initialized SPI3 (HSPI) host.

## Root Cause

The ESP32 Arduino core or one of the preincluded libraries initializes the SPI3 bus before `DisplayHAL_init()` is called. The GFX Library's `begin()` method doesn't check if the bus is already initialized - it just tries to initialize and fails with `ESP_ERR_INVALID_STATE`.

## What We've Tried

### Attempt 1: Remove spi_bus_free workaround (Original Code)
- Code called `spi_bus_free()` after creating the GFX object
- **Result**: Still failed - the spi_bus_free was corrupting bus state

### Attempt 2: Remove spi_bus_free entirely
- Let the GFX library handle everything normally
- **Result**: Failed with same error - bus already initialized

### Attempt 3: Free SPI bus BEFORE creating GFX object (Implemented 2026-02-27)
- Call `spi_bus_free(targetHost)` BEFORE `new Arduino_ESP32QSPI()`
- Then let the GFX library's `begin()` initialize the bus fresh
- **Build Status**: ✅ Succeeds
- **Test Status**: Ready for hardware test

## Current Code Location

`firmware/platforms/esp32/display_esp32.cpp` - `DisplayHAL_init()`

```cpp
// CRITICAL: Free SPI bus BEFORE creating GFX object
Serial.println("ESP32: Freeing SPI bus (if pre-initialized)...");
esp_err_t freeResult = spi_bus_free(targetHost);
Serial.printf("ESP32: spi_bus_free result: 0x%x\n", freeResult);
 THENdelay(10);

// create the GFX object
qspiBus = new Arduino_ESP32QSPI(...);

// Then begin() - should work now that bus is freed
bool busBegun = qspiBus->begin(speed, dataMode);
```

## Hardware Configuration

- **Board**: Waveshare ESP32-S3-Touch-AMOLED-1.75
- **Display**: 1.75" AMOLED 466x466 (CO5300 driver)
- **SPI Host**: SPI3 (HSPI) - defined as `ESP32QSPI_SPI_HOST` in GFX Library
- **QSPI Pins**: CS=12, SCK=38, D0=4, D1=5, D2=6, D3=7

## Libraries Involved

1. **GFX Library for Arduino v1.5.0** - Display driver
   - `Arduino_ESP32QSPI` class handles QSPI bus
   - `Arduino_CO5300` class for AMOLED driver
   - `begin()` at line 63 calls `spi_bus_initialize()` without checking if already initialized

2. **SensorLib v0.3.4** - Touch controller (uses I2C, not SPI)
3. **LVGL v8.4.0** - UI framework (uses display buffer, not direct SPI)

## If Current Fix Fails

Alternative approaches to try:

1. **Upgrade GFX Library** - The Waveshare examples use v1.6.4 which may have the `GFX_SKIP_DATABUS_UNDERLAYING_BEGIN` flag
2. **Check what's initializing SPI** - Add early Serial.print in startup to identify culprit
3. **Use different SPI host** - If SPI3 is taken, try SPI2 (FSPI)
4. **Modify GFX Library locally** - Patch `Arduino_ESP32QSPI::begin()` to check `ESP_ERR_INVALID_STATE` and skip init if bus exists

## Test Procedure

1. Upload firmware: `pio run --target upload`
2. Monitor: `pio device monitor`
3. Expected output (if fix works):
   - "ESP32: spi_bus_free result: 0x103" (ESP_ERR_INVALID_STATE if bus wasn't initialized)
   - OR "ESP32: spi_bus_free result: 0x0" (ESP_OK if bus was initialized and freed)
   - Then "ESP32: Display initialized successfully"

## Contact

Original issue investigated by: PAI (Claude Code)
Date: 2026-02-27
Project: Tombogo x Rheome Collab Gadget
