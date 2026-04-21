# ESP32 AMOLED Native UI Techniques
## TombogoCollabGadget — Icon-Based + Dead Front Display

---

## Display Stack Overview

### Hardware
- **Module**: WaveShare ESP32-S3-Touch-AMOLED-1.75
- **Driver IC**: CO5300 (AMOLED display driver)
- **Interface**: QSPI (4-lane, 466×466 @ 16-bit color)
- **Touch**: CST9217 capacitive touch controller (I2C 0x5A)
- **Framework**: Arduino + FreeRTOS + LVGL v8.x + Arduino_GFX

### Software Stack
```
┌─────────────────────────────────────────────┐
│           Application (UI Manager)          │
│    Icon-based UI, gesture handling, screens   │
├─────────────────────────────────────────────┤
│                  LVGL v8.x                   │
│   Animations, styles, widgets, rendering     │
├─────────────────────────────────────────────┤
│              Arduino_GFX Library             │
│   CO5300 driver, pixel drawing, bitmaps      │
├─────────────────────────────────────────────┤
│          Arduino_ESP32QSPI (bus)             │
│   QSPI communication layer                   │
├─────────────────────────────────────────────┤
│              ESP32-S3R8 Hardware             │
└─────────────────────────────────────────────┘
```

---

## 1. CO5300 AMOLED Driver Specifics

### Connection (from `pin_config.h`)
```
GPIO4  → QSPI SIO0 (Data 0)
GPIO5  → QSPI SIO1 (Data 1)
GPIO6  → QSPI SIO2 (Data 2)
GPIO7  → QSPI SIO3 (Data 3)
GPIO38 → QSPI SCL  (Clock)
GPIO12 → LCD_CS    (Chip Select)
GPIO13 → LCD_TE    (Tearing Effect)
GPIO39 → LCD_RESET (via TCA9554)
```

### Key CO5300 Capabilities
- **QSPI 4-lane**: Up to 4 pixels per clock cycle — critical for smooth animations
- **466×466 square display**: Unusual aspect — design icons within square bounds
- **16-bit color (RGB565)**: 65K colors, lower memory than 24-bit
- **AMOLED true black**: Each pixel is self-emitting, so `BLACK = 0x0000` = completely off (no power)
- **No backlight**: Brightness controlled via pixel values or PWM duty cycle
- **Tearing Effect pin (TE/GPIO13)**: Use for vsync — prevents horizontal tearing during animations

### Display HAL Implementation
From `display_esp32.cpp`:
```cpp
bus = new Arduino_ESP32QSPI(LCD_CS, LCD_QSPI_SCL,
    LCD_QSPI_SIO0, LCD_QSPI_SIO1, LCD_QSPI_SIO2, LCD_QSPI_SIO3);

gfx = new Arduino_CO5300(bus, -1, 0, false, LCD_WIDTH, LCD_HEIGHT);
gfx->begin();
gfx->setRotation(0);  // Portrait
gfx->fillScreen(BLACK);
```

### Important: TCA9554 Must Power On Display First
The TCA9554 IO expander (I2C 0x18) controls display power and reset:
- P1 (EXIO1) → LCD_RESET
- P2 (EXIO2) → LCD_POWER_ENABLE

Display initialization will fail silently if power isn't enabled first.

---

## 2. Dead Front Display Technique

### Concept
AMOLED's true black allows pixels to be "invisible" when showing black. The dead front technique uses extremely low opacity values for idle-state icons — so dim they're imperceptible on a powered-off screen, but bright enough to hint at functionality when the display wakes.

### Implementation Status
Already implemented in `dead_front_test.cpp`. The test uses:
```cpp
lv_obj_set_style_bg_color(circle, lv_color_make(150, 220, 255), 0);
lv_obj_set_style_bg_opa(circle, LV_OPA_COVER, 0);  // Full opacity for testing
```

### Design Rules for Dead Front Icons
1. **Idle state**: Use very low opacity (2-8% of normal brightness). Test in a dark room.
2. **Active/selected state**: Full opacity or near-full (80-100%).
3. **Transition**: Fade between states over 300-500ms for smoothness.
4. **Color choice**: Since icons are near-invisible at idle, color is less important than shape/silhouette for recognition when dormant.
5. **Avoid pure white at full opacity**: AMOLED subpixels age at different rates. Slight color tint (warm white, cool blue) extends panel life.

### Icon Design Guidelines for 466×466 Square Display
- Design icons in a **square canvas** (not rectangular)
- **Touch target minimum**: 44×44 pixels (Android accessibility standard)
- **Icon grid**: 4×4 grid = 116×116 pixel cells (good for mode indicators)
- **Safe margins**: 10px from edges
- **Shape priority**: Bold silhouettes over fine detail (icons must read when tiny and dim)

---

## 3. LVGL Animations for Icon-Based UI

### Animation System (LVGL v8.x)
LVGL has a built-in animation system with `lv_anim_t` objects:

```cpp
#include <lvgl.h>

// Define an animation
static void anim_opacity_callback(void* obj, int32_t value) {
    lv_obj_set_style_bg_opa(obj, value, 0);
}

void create_icon_animation(lv_obj_t* icon) {
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, icon);
    lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)anim_opacity_callback);
    lv_anim_set_values(&anim, LV_OPA_20, LV_OPA_100);  // Dim → bright
    lv_anim_set_duration(&anim, 400);                  // 400ms
    lv_anim_set_easing(&anim, &lv_anim_timing_path_overshoot);  // Bouncy
    lv_anim_start(&anim);
}
```

### Built-in Animation Timing Paths
| Easing | Feel |
|-------|------|
| `lv_anim_timing_path_linear` | Constant speed |
| `lv_anim_timing_path_ease_in` | Slow start |
| `lv_anim_timing_path_ease_out` | Slow end |
| `lv_anim_timing_path_ease_in_out` | Slow start and end |
| `lv_anim_timing_path_overshoot` | Bouncy overshoot |
| `lv_anim_timing_path_bounce` | Bounces at end |

### Icon Press Animation (Touch Feedback)
```cpp
static void anim_scale_down(void* obj, int32_t value) {
    // Scale from 100% to 85% and back
    // Need custom exec_cb for this
}

void icon_on_press(lv_obj_t* icon) {
    // Scale down slightly (press feel)
    lv_anim_t scale;
    lv_anim_init(&scale);
    lv_anim_set_var(&scale, icon);
    lv_anim_set_values(&scale, 100, 85);
    lv_anim_set_duration(&scale, 100);
    lv_anim_set_exec_cb(&scale, scale_cb);
    lv_anim_start(&scale);
}
```

### Animation Variables for Icon Transitions
For a smooth icon-based UI, animate these properties:

| Property | Use Case |
|----------|----------|
| `bg_opa` | Dead front fade in/out |
| `transform_scale` | Press feedback |
| `transform_rotation` | Mode cycling |
| `transform_x/y` | Sliding screens |
| `arc_angle` | Knob/slider feedback |
| `color_filter_opa` | Color tinting |

### Animated Icon Arrays (Frame-by-Frame)
LVGL `lv_img` supports source switching for sprite-sheet-style animation:
```cpp
// Switch the image source on a timer
static uint8_t current_frame = 0;
static const void* frames[] = {&icon_frame0, &icon_frame1, &icon_frame2, &icon_frame3};

void frame_timer_callback(lv_timer_t* t) {
    current_frame = (current_frame + 1) % 4;
    lv_img_set_src(anim_icon, frames[current_frame]);
}

// Create timer
lv_timer_t* frame_timer = lv_timer_create(frame_timer_callback, 150, NULL);
lv_timer_set_repeat_count(frame_timer, -1);  // Infinite
```

---

## 4. Touch Control Integration

### Touch HAL (CST9217)
From `touch_esp32.cpp`:
```cpp
touch = new TouchDrvCSTXXX();
touch->begin(Wire, CST9217_ADDR, screenWidth, screenHeight);  // 0x5A

// Reading touch
int16_t x[5], y[5];
uint8_t count = touch->getPoint(x, y, 5);
```

### Touch-to-LVGL Integration
From `lvgl_driver.cpp`:
```cpp
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
```

### Touch Coordinate Transformation
The CST9217 touch coordinates are inverted relative to the display:
```cpp
// From dead_front_test.cpp
targetX = (float)(SCREEN_WIDTH - 1 - rawX);
targetY = (float)(SCREEN_HEIGHT - 1 - rawY);
```
**Important**: Include this transform in your touch HAL or LVGL indev driver to prevent mirrored touch response.

### Gesture Recognition
LVGL supports basic gestures (tap, long press, swipe) via `lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE)`:
```cpp
// Enable gestures on an icon
lv_obj_add_flag(icon, LV_OBJ_FLAG_GESTURE_BUBBLE);

// Handle gesture events
static void gesture_handler(lv_event_t* e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    switch(dir) {
        case LV_DIR_LEFT:  /* swipe left */  break;
        case LV_DIR_RIGHT: /* swipe right */ break;
        case LV_DIR_TOP:   /* swipe up */    break;
        case LV_DIR_BOTTOM:/* swipe down */  break;
    }
}
lv_obj_add_event_cb(screen, gesture_handler, LV_EVENT_GESTURE, NULL);
```

### Multi-Touch for Pinch/Zoom
The CST9217 supports up to 5 touch points. For pinch-to-zoom or rotate gestures:
```cpp
uint8_t count = touch->getPoint(x, y, 5);
if (count >= 2) {
    int16_t dx = x[1] - x[0];
    int16_t dy = y[1] - y[0];
    float pinch_distance = sqrt(dx*dx + dy*dy);
    // Use pinch_distance to scale UI element
}
```

---

## 5. Smooth Update Strategies

### Current Buffering Approach
From `lvgl_driver.cpp`:
```cpp
// BUF_SIZE = SCREEN_WIDTH * LVGL_BUFFER_LINES (30 lines default)
// ~28KB per buffer in PSRAM
buf1 = (lv_color_t*)ps_malloc(BUF_SIZE * sizeof(lv_color_t));
buf2 = (lv_color_t*)ps_malloc(BUF_SIZE * sizeof(lv_color_t));
lv_disp_draw_buf_init(&draw_buf, buf1, buf2, BUF_SIZE);
```

### Optimization: Increase Buffer Lines
For smoother animations, increase `LVGL_BUFFER_LINES` in `lv_conf.h`:
```cpp
// More buffer = smoother animations but more memory
#define LVGL_BUFFER_LINES  60   // 60 lines = ~56KB buffer
```

### Optimization: Use RGB565 for PSRAM Storage
Color depth is already RGB565 (16-bit). PSRAM at 8MB means you can afford larger buffers:
```cpp
// If animation stutters, try full-screen buffer
static lv_color_t fullbuf[466 * 466];  // ~434KB — fits in PSRAM
```

### Optimization: Dirty Area Rendering
Only update regions that changed. LVGL's default behavior — but verify:
```cpp
// In lv_conf.h
#define LV_INVALIDATE_AREA  1
#define LV_REFR_AREA_CHECK  1
```

### Tearing Prevention (TE Pin)
The CO5300's TE (Tearing Effect) pin signals when the display is ready for new data:
```cpp
// Attach GPIO13 (LCD_TE) interrupt
volatile bool frame_ready = false;
void IRAM_ATTR te_isr() {
    frame_ready = true;
}
attachInterrupt(LCD_TE, te_isr, RISING);

// In render loop — wait for vsync before flushing
while (!frame_ready) { }
frame_ready = false;
DisplayHAL_flush(x1, y1, x2, y2, color_p);
```

### Render Loop Best Practices
From `dead_front_test.cpp`:
```cpp
uint32_t lastTick = millis();
while (true) {
    uint32_t now = millis();
    uint32_t elapsed = now - lastTick;
    lastTick = now;
    LVGL_driver_tick(elapsed);      // Feed LVGL time base
    updateCirclePosition();          // Update animation state
    uint32_t sleepTime = LVGL_driver_task();  // Process LVGL tasks
    delay(sleepTime > 16 ? 16 : sleepTime);   // Cap at ~60fps
}
```

---

## 6. Icon Storage & Loading

### Flash Storage for Icons
Since PSRAM is precious for audio buffers, store icons in Flash (16MB available):
```cpp
// Icons stored as const arrays in Flash (PROGMEM on ESP32)
#include <pgmspace.h>

static const uint16_t icon_play[] PROGMEM = {
    0x0000, 0x0000, 0x1CE7, 0xFFFF,  // ... (binary RGB565 data)
    // Use lv_img_dsc_t struct:
};

static const lv_img_dsc_t icon_play_dsc = {
    .header.always_zero = 0,
    .header.w = 48,
    .header.h = 48,
    .data_size = 48 * 48 * 2,
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .data = (uint8_t*)icon_play,
};

// Apply to LVGL img
lv_obj_t* img = lv_img_create(screen);
lv_img_set_src(img, &icon_play_dsc);
```

### Converting Images to RGB565 for LVGL
Tools:
- **lvgl 在线 converter**: https://lvgl.io/tools/imageconverter
- Set output format: `True color, RGB565`
- Check "Binary" for C array output

### Animated Sprite Sheets
For multi-frame animations, pack frames into a horizontal sprite sheet:
```
[ Frame 0 ][ Frame 1 ][ Frame 2 ][ Frame 3 ]  ← Single image, 192×48
```
```cpp
lv_img_set_src(anim_icon, sprite_sheet);
lv_img_set_offset_x(anim_icon, frame_index * FRAME_WIDTH);
```

---

## 7. LVGL Buttonless Icon Grid Implementation

### Pure Icon UI Pattern
Since this is an icon-only interface, use `lv_obj` with click events instead of `lv_btn`:
```cpp
static void icon_click_handler(lv_event_t* e) {
    lv_obj_t* icon = lv_event_get_target(e);
    uint32_t icon_id = (uint32_t)lv_obj_get_user_data(icon);
    
    switch(icon_id) {
        case 0: /* play/record */ break;
        case 1: /* loop */       break;
        case 2: /* effects */     break;
        case 3: /* settings */    break;
    }
}

void create_icon_grid() {
    static lv_style_t icon_style;
    lv_style_init(&icon_style);
    lv_style_set_radius(&icon_style, 12);
    lv_style_set_bg_opa(&icon_style, LV_OPA_30);  // Dim at idle
    lv_style_set_bg_color(&icon_style, lv_color_white());
    
    for (int i = 0; i < 4; i++) {
        lv_obj_t* icon = lv_obj_create(grid);
        lv_obj_set_size(icon, 80, 80);
        lv_obj_add_style(icon, &icon_style, 0);
        lv_obj_set_user_data(icon, (uint32_t)i);
        lv_obj_add_event_cb(icon, icon_click_handler, LV_EVENT_CLICKED, NULL);
    }
}
```

### Toggle State via Style Change
```cpp
static void icon_select(lv_obj_t* icon) {
    // Dim all others
    for (int i = 0; i < 4; i++) icons[i]...
    
    // Highlight selected
    lv_obj_set_style_bg_opa(icon, LV_OPA_100, 0);
    lv_obj_set_style_transform_scale(icon, 110, 0);  // Slightly larger
}
```

---

## 8. Key Files in TombogoCollabGadget

| File | Purpose |
|------|---------|
| `firmware/platforms/esp32/display_esp32.cpp` | CO5300 init, HAL drawing, QSPI bus |
| `firmware/platforms/esp32/touch_esp32.cpp` | CST9217 touch driver |
| `firmware/src/core/lvgl_driver.cpp` | LVGL ↔ HAL bridge, double buffer |
| `firmware/src/core/ui_manager.cpp` | UI state management |
| `firmware/src/dead_front_test.cpp` | Dead front demo with touch |
| `firmware/lv_conf.h` | LVGL configuration |
| `firmware/platforms/esp32/pin_config.h` | GPIO pin mappings |

---

## 9. Recommended Next Steps

1. **Design icon set** — Create 8-12 icons for core functions (record, play, loop, chop, reverb, delay, synth, drums, menu, back) at 48×48 and 80×80 sizes
2. **Build icon grid UI** — Replace `ui_manager.cpp` create_ui() with icon grid layout
3. **Add animation state machine** — Define states (idle, hover, active, disabled) per icon with transitions
4. **Implement dead front fade** — Use `lv_anim_t` to fade icons in/out based on wake gesture
5. **Gesture detection** — Add swipe handling for screen navigation between mode panels
6. **Profile frame times** — Use `lv_tick_get()` to measure actual render time, adjust buffer size
7. **Consider AXP2101 power management** — The board has an AXP2101 PMU at 0x34 for display brightness and battery

---

## 10. Reference Resources

- **Arduino_GFX library**: https://github.com/moononournation/Arduino_GFX
- **LVGL v8 docs**: https://docs.lvgl.io/8.3/
- **WaveShare wiki**: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75
- **CO5300 driver**: In Arduino_GFX under `src/display/Arduino_CO5300.cpp`
- **CST9217 touch lib**: https://github.com/BeanAV/Arduino_TouchDrv (used via Seeed_Arduino_LCD)
- **Dead front test**: `firmware/src/dead_front_test.cpp`
