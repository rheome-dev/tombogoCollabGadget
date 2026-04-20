/**
 * ESP32 Platform - Display HAL Implementation
 *
 * Uses Arduino_GFX_Library with CO5300 AMOLED display driver
 * LVGL integration for UI rendering
 * Reference: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75
 *
 * Note: TCA9554 must power on the display BEFORE this init is called!
 *
 * SPI INIT WORKAROUND (PATCHED GFX Library):
 * The ESP32 Arduino core pre-initializes SPI3 (HSPI) before our code runs.
 * We patched Arduino_ESP32QSPI::begin() to handle ESP_ERR_INVALID_STATE gracefully.
 * This file uses the default SPI3 host.
 */

#ifdef PLATFORM_ESP32

#include "../../hal/display_hal.h"
#include "pin_config.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// Display objects
static Arduino_GFX* gfx = nullptr;
static Arduino_DataBus* bus = nullptr;

// Display state
static bool displayInitialized = false;
static uint8_t currentBrightness = 255;  // AMOLED: max brightness default

void DisplayHAL_init(void) {
    Serial.println("ESP32: Initializing Display HAL...");
    Serial.println("ESP32: Note - Display power should already be enabled via TCA9554");

    // If already initialized, just return
    if (displayInitialized && gfx != nullptr) {
        Serial.println("ESP32: Display already initialized, skipping...");
        return;
    }

    // Initialize QSPI bus for display
    // Using standard QSPI pins for Waveshare ESP32-S3-Touch-AMOLED-1.75
    // Following Waveshare example approach: use base class pointer, single begin() call
    Serial.println("ESP32: Creating QSPI bus object...");

    // Use base class pointer like Waveshare example
    bus = new Arduino_ESP32QSPI(
        LCD_CS,         // CS pin (GPIO12)
        LCD_QSPI_SCL,   // SCK pin (GPIO38)
        LCD_QSPI_SIO0,  // D0 pin (GPIO4)
        LCD_QSPI_SIO1,  // D1 pin (GPIO5)
        LCD_QSPI_SIO2,  // D2 pin (GPIO6)
        LCD_QSPI_SIO3   // D3 pin (GPIO7)
    );

    // Create CO5300 display device (AMOLED driver IC)
    // Note: TCA9554 handles power and reset BEFORE this is called
    // Pass -1 for reset pin since TCA9554 already handled it
    gfx = new Arduino_CO5300(
        bus,
        -1,             // Reset already handled by TCA9554
        0,              // Rotation 0 (portrait)
        false,          // IPS mode off for AMOLED
        LCD_WIDTH,      // 466
        LCD_HEIGHT      // 466
    );

    // Single begin() call - GFX library handles bus init internally
    // The patched GFX library handles "SPI already initialized" gracefully
    Serial.println("ESP32: Starting display...");
    if (!gfx->begin()) {
        Serial.println("ESP32: Display begin failed!");
        delete gfx;
        gfx = nullptr;
        delete bus;
        return;
    }

    Serial.println("ESP32: Display started successfully");

    // Set default display parameters
    gfx->setRotation(0);  // Portrait orientation

    // Initialize with black screen
    gfx->fillScreen(BLACK);

    // Set initial brightness
    DisplayHAL_setBrightness(currentBrightness);

    displayInitialized = true;
    Serial.println("ESP32: Display initialized successfully");
    Serial.printf("  Resolution: %dx%d\n", LCD_WIDTH, LCD_HEIGHT);
}

void DisplayHAL_deinit(void) {
    if (gfx) {
        gfx->fillScreen(BLACK);
        delete gfx;
        gfx = nullptr;
    }
    if (bus) {
        delete bus;
        bus = nullptr;
    }
    displayInitialized = false;
    Serial.println("ESP32: Display deinitialized");
}

void DisplayHAL_setBrightness(uint8_t brightness) {
    currentBrightness = brightness;
    // Note: AMOLED displays typically don't have backlight
    // Brightness is handled via pixel values or PWM if available
    // For CO5300, we may need to use display-specific commands
    if (gfx && displayInitialized) {
        // Some displays support brightness via gamma or PWM
        // This is a placeholder - actual implementation depends on CO5300
    }
}

void DisplayHAL_fillScreen(uint16_t color) {
    if (gfx && displayInitialized) {
        gfx->fillScreen(color);
    }
}

void DisplayHAL_drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (gfx && displayInitialized) {
        gfx->drawPixel(x, y, color);
    }
}

void DisplayHAL_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (gfx && displayInitialized) {
        gfx->drawLine(x0, y0, x1, y1, color);
    }
}

void DisplayHAL_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (gfx && displayInitialized) {
        gfx->fillRect(x, y, w, h, color);
    }
}

void DisplayHAL_getDimensions(uint16_t* width, uint16_t* height) {
    if (gfx && displayInitialized) {
        *width = gfx->width();
        *height = gfx->height();
    } else {
        *width = LCD_WIDTH;
        *height = LCD_HEIGHT;
    }
}

bool DisplayHAL_isReady(void) {
    return displayInitialized && gfx != nullptr;
}

void DisplayHAL_flush(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t* color_p) {
    if (!displayInitialized || !gfx || !color_p) {
        return;
    }

    // Calculate dimensions
    int16_t w = x2 - x1 + 1;
    int16_t h = y2 - y1 + 1;

    // Use Arduino_GFX draw16bitRGBBitmap for efficient block transfer
    gfx->draw16bitRGBBitmap(x1, y1, color_p, w, h);
}

void* DisplayHAL_getDevice(void) {
    return (void*)gfx;
}

#endif // PLATFORM_ESP32
