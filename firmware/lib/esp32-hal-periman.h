/**
 * Stub header for esp32-hal-periman.h
 * This is a placeholder to allow building with GFX library
 * Real peripheral management requires actual implementation
 */

#ifndef ESP32_HAL_PERIMAN_H
#define ESP32_HAL_PERIMAN_H

#include <stdint.h>

// Peripheral manager initialization (stub)
void perimanInit(void);
void perimanDeinit(void);

// Peripheral type definitions (stub)
typedef enum {
    PERIMAN_NONE = 0,
    // Add more types as needed
} periman_type_t;

// Peripheral management functions (stub)
bool perimanSetPinBus(uint8_t pin, periman_type_t type, int8_t bus, uint8_t idx);
bool perimanClearPinBus(uint8_t pin);
void* perimanGetPinBus(uint8_t pin, periman_type_t type, int8_t bus);

#endif // ESP32_HAL_PERIMAN_H
