/**
 * System Configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// Platform selection - switch between ESP32 and custom PCB
// Note: PLATFORM_ESP32 is defined in platformio.ini build_flags
#ifndef PLATFORM_ESP32
#define PLATFORM_ESP32
#endif
// #define PLATFORM_CUSTOM_PCB

// Audio Configuration
#define SAMPLE_RATE             16000
#define AUDIO_BUFFER_SIZE       256
#define BIT_DEPTH               16
#define CHANNELS                2  // Stereo

// Retroactive Buffer
#define RETROACTIVE_BUFFER_SEC  10  // seconds
#define RETROACTIVE_BUF_SIZE    (SAMPLE_RATE * RETROACTIVE_BUFFER_SEC)

// Memory Configuration
#define PSRAM_AVAILABLE         1  // Set to 0 if no PSRAM
#define DMA_BUFFER_SIZE         256

// Display Configuration
#define SCREEN_WIDTH            466
#define SCREEN_HEIGHT           466
#define LVGL_BUFFER_LINES      30

// Task Configuration
#define AUDIO_TASK_STACK_SIZE  4096
#define AUDIO_TASK_PRIORITY    5
#define AUDIO_TASK_DELAY_MS    1

#define IMU_TASK_STACK_SIZE     2048
#define IMU_TASK_PRIORITY      5
#define IMU_TASK_DELAY_MS      10

#define UI_TASK_STACK_SIZE      4096
#define UI_TASK_PRIORITY       3
#define UI_TASK_DELAY_MS       5

#define MAIN_LOOP_DELAY_MS      5

// Scale Definitions
enum ScaleType {
    SCALE_PENTATONIC_MAJOR,
    SCALE_PENTATONIC_MINOR,
    SCALE_MAJOR,
    SCALE_MINOR,
    SCALE_BLUES,
    SCALE_DORIAN,
    SCALE_PHRYGIAN,
    SCALE_COUNT
};

extern const int8_t SCALES[][7];  // Defined in scales.cpp

// System State
struct SystemState {
    bool recording;
    bool playing;
    bool armed;  // Retroactive buffer armed
    ScaleType currentScale;
    uint8_t rootNote;  // 0-11 (C, C#, D, etc.)
    uint16_t tempo;
    float imuX;  // Normalized -1 to 1
    float imuY;
};

#endif // CONFIG_H
