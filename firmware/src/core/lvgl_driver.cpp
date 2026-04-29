/**
 * LVGL Driver Implementation
 *
 * Connects LVGL to HAL display and touch interfaces
 * Uses double buffering with partial updates for performance
 */

#include "lvgl_driver.h"
#include "../config.h"
#include "../../hal/display_hal.h"
#include "../../hal/touch_hal.h"
#include <Arduino.h>

// LVGL includes
#include <lvgl.h>

// LVGL objects
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;
static lv_disp_t* disp = nullptr;
static lv_indev_t* indev = nullptr;

// Draw buffer (allocated in PSRAM if available)
static lv_color_t* buf1 = nullptr;
static lv_color_t* buf2 = nullptr;

// State
static bool initialized = false;

// Buffer size (partial screen update - optimized for memory vs speed)
// Using LVGL_BUFFER_LINES from config.h (default 30 lines)
static const uint32_t BUF_SIZE = SCREEN_WIDTH * LVGL_BUFFER_LINES;

/**
 * Rounder callback - CO5300 QSPI requires even-aligned coordinates
 * (from Waveshare example)
 */
static void disp_rounder_cb(lv_disp_drv_t* drv, lv_area_t* area) {
    (void)drv;
    if (area->x1 % 2 != 0) area->x1--;
    if (area->y1 % 2 != 0) area->y1--;
    if (area->x2 % 2 == 0) area->x2++;
    if (area->y2 % 2 == 0) area->y2++;
}

/**
 * Display flush callback - called by LVGL when area needs updating
 */
static void disp_flush_cb(lv_disp_drv_t* drv, const lv_area_t* area, lv_color_t* color_p) {
    // Call HAL to flush the area
    DisplayHAL_flush(
        area->x1,
        area->y1,
        area->x2,
        area->y2,
        (uint16_t*)color_p
    );

    // Inform LVGL that flushing is done
    lv_disp_flush_ready(drv);
}

/**
 * Touch input read callback - called by LVGL to get touch state
 */
static void touch_read_cb(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    TouchPoint point;

    if (TouchHAL_isReady() && TouchHAL_getPoint(0, &point) && point.pressed) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = point.x;
        data->point.y = point.y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

bool LVGL_driver_init(void) {
    Serial.println("LVGL: Initializing driver...");

    // Check if display is ready
    if (!DisplayHAL_isReady()) {
        Serial.println("LVGL: ERROR - Display not ready!");
        return false;
    }

    // Initialize LVGL library
    lv_init();
    Serial.println("LVGL: Library initialized");

    // Allocate draw buffers
    // Try PSRAM first for larger buffers
    #if PSRAM_AVAILABLE
    buf1 = (lv_color_t*)ps_malloc(BUF_SIZE * sizeof(lv_color_t));
    buf2 = (lv_color_t*)ps_malloc(BUF_SIZE * sizeof(lv_color_t));
    if (buf1 && buf2) {
        Serial.printf("LVGL: Allocated %d bytes x2 in PSRAM\n", BUF_SIZE * sizeof(lv_color_t));
    }
    #endif

    // Fallback to regular malloc if PSRAM allocation failed
    if (!buf1) {
        buf1 = (lv_color_t*)malloc(BUF_SIZE * sizeof(lv_color_t));
        buf2 = nullptr;  // Single buffer mode if memory tight
        Serial.printf("LVGL: Allocated %d bytes in DRAM (single buffer)\n", BUF_SIZE * sizeof(lv_color_t));
    }

    if (!buf1) {
        Serial.println("LVGL: ERROR - Failed to allocate draw buffer!");
        return false;
    }

    // Initialize draw buffer (double buffer if buf2 available)
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, BUF_SIZE);

    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.rounder_cb = disp_rounder_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.sw_rotate = 1;
    disp_drv.rotated = LV_DISP_ROT_270;

    // Register display driver
    disp = lv_disp_drv_register(&disp_drv);
    if (!disp) {
        Serial.println("LVGL: ERROR - Failed to register display driver!");
        return false;
    }
    Serial.printf("LVGL: Display registered (%dx%d)\n", SCREEN_WIDTH, SCREEN_HEIGHT);

    // Initialize input (touch) driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;

    // Register input driver
    indev = lv_indev_drv_register(&indev_drv);
    if (!indev) {
        Serial.println("LVGL: WARNING - Failed to register input driver (touch may not work)");
    } else {
        Serial.println("LVGL: Touch input registered");
    }

    initialized = true;
    Serial.println("LVGL: Driver initialization complete");

    return true;
}

void LVGL_driver_tick(uint32_t tick_ms) {
    if (initialized) {
        lv_tick_inc(tick_ms);
    }
}

uint32_t LVGL_driver_task(void) {
    if (!initialized) {
        return 100;  // Return default delay if not initialized
    }

    return lv_timer_handler();
}

bool LVGL_driver_isReady(void) {
    return initialized;
}

void* LVGL_driver_getDisplay(void) {
    return (void*)disp;
}

void* LVGL_driver_getScreen(void) {
    if (disp) {
        return (void*)lv_disp_get_scr_act(disp);
    }
    return nullptr;
}
