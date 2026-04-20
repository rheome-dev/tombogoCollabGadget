/**
 * Hardware Abstraction Layer - Display Interface
 *
 * Defines the contract for display hardware.
 * Implement per-platform in platforms/esp32/ or platforms/custom_pcb/
 */

#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Display HAL initialization
 */
void DisplayHAL_init(void);

/**
 * Display HAL deinitialization
 */
void DisplayHAL_deinit(void);

/**
 * Set display brightness (0-255)
 */
void DisplayHAL_setBrightness(uint8_t brightness);

/**
 * Fill entire screen with color
 * @param color 16-bit RGB565 color
 */
void DisplayHAL_fillScreen(uint16_t color);

/**
 * Draw a pixel
 */
void DisplayHAL_drawPixel(int16_t x, int16_t y, uint16_t color);

/**
 * Draw a line
 */
void DisplayHAL_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/**
 * Draw a filled rectangle
 */
void DisplayHAL_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * Get screen dimensions
 */
void DisplayHAL_getDimensions(uint16_t* width, uint16_t* height);

/**
 * Check if display is initialized
 */
bool DisplayHAL_isReady(void);

/**
 * Flush a rectangular area to the display (for LVGL)
 * @param x1 Start X coordinate
 * @param y1 Start Y coordinate
 * @param x2 End X coordinate
 * @param y2 End Y coordinate
 * @param color_p Pointer to RGB565 color data
 */
void DisplayHAL_flush(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t* color_p);

/**
 * Get the underlying graphics device (platform-specific)
 * Used for LVGL direct rendering
 * @return Platform-specific graphics handle (void* for portability)
 */
void* DisplayHAL_getDevice(void);

#endif // DISPLAY_HAL_H
