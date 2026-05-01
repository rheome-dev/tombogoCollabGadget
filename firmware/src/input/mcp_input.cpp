/**
 * MCP23017 Input Driver Implementation
 *
 * Fixes from audit:
 * - Correct press/release polarity (active low: 0=pressed)
 * - Edge detection for joystick (fire on transition only)
 * - Shift tap-toggle and double-tap latch
 * - EVT_BUTTON_RELEASE emitted
 */

#include "mcp_input.h"
#include "platforms/esp32/pin_config.h"
#include <Arduino.h>
#include <Wire.h>
#include <driver/gpio.h>

// MCP23017 registers
#define MCP_IODIRA    0x00
#define MCP_IODIRB    0x01
#define MCP_GPPUA     0x0C
#define MCP_GPPUB     0x0D
#define MCP_GPIOA     0x12
#define MCP_GPIOB     0x13
#define MCP_INTFA     0x0E
#define MCP_INTFB     0x0F
#define MCP_INTCONA   0x08
#define MCP_INTCONB   0x09
#define MCP_GPINTENA  0x04
#define MCP_GPINTENB  0x05

// Timing constants
#define DEBOUNCE_MS        50
#define DOUBLECLICK_MS     300
#define LONGPRESS_MS       500

// Event queue (ring buffer)
#define QUEUE_SIZE 32
static InputMsg eventQueue[QUEUE_SIZE];
static volatile uint8_t queueHead = 0;
static volatile uint8_t queueTail = 0;

// Previous GPIO state
static uint8_t lastGPIOA = 0xFF;
static uint8_t lastGPIOB = 0xFF;

// Per-button state for debounce / multi-click / long-press
typedef struct {
    uint32_t lastChangeMs;
    uint32_t pressStartMs;    // when current press began
    bool     isPressed;       // current debounced state
    bool     longFired;       // long-press already emitted
    uint8_t  clickCount;      // clicks within double-click window
    uint32_t firstClickMs;    // timestamp of first click in sequence
    bool     shiftPressFired; // shift+button fired immediately on press-down
} ButtonState;

static ButtonState buttonStates[6];  // enc_sw, enc_a, enc_b, btn1, btn2, btn3

// Joystick previous state for edge detection
static bool lastJoyState[5] = {false, false, false, false, false};

// Joystick center debouncing: 5-way tact switches inevitably trigger an
// adjacent direction when pressing center. We defer directional emissions
// by JOY_DEFER_MS — if center activates within that window, the deferred
// direction is dropped. Center always fires immediately on its own edge.
#define JOY_CENTER_IDX 3
#define JOY_DEFER_MS   30
static int8_t  pendingDir   = -1;
static uint32_t pendingDirMs = 0;

// Encoder quadrature
static int8_t encoderDelta = 0;
static uint8_t lastEncA = 1;

// Shift state machine
typedef enum {
    SHIFT_OFF,
    SHIFT_HELD,      // physically held down
    SHIFT_TOGGLED,   // tap-toggled on
    SHIFT_LATCHED    // double-tap latched
} ShiftMode;

static ShiftMode shiftMode = SHIFT_OFF;
static bool shiftPhysicallyHeld = false;
static uint32_t shiftReleaseMs = 0;  // when shift was last released

static const gpio_num_t INTA_GPIO = (gpio_num_t)MCP_INTA;
static TwoWire* wire2 = nullptr;
static bool initialized = false;

static void mcpWrite(uint8_t reg, uint8_t val) {
    wire2->beginTransmission(MCP23017_I2C_ADDR);
    wire2->write(reg);
    wire2->write(val);
    wire2->endTransmission();
}

// mcpRead is reserved for future use (e.g. interrupt capture)
static uint8_t __attribute__((unused)) mcpRead(uint8_t reg) {
    wire2->beginTransmission(MCP23017_I2C_ADDR);
    wire2->write(reg);
    wire2->endTransmission();
    wire2->requestFrom((uint8_t)MCP23017_I2C_ADDR, (uint8_t)1);
    return wire2->read();
}

static void readGPIO(uint8_t& gpioA, uint8_t& gpioB) {
    wire2->beginTransmission(MCP23017_I2C_ADDR);
    wire2->write(MCP_GPIOA);
    wire2->endTransmission(false);
    wire2->requestFrom((uint8_t)MCP23017_I2C_ADDR, (uint8_t)2);
    gpioA = wire2->read();
    gpioB = wire2->read();
}

static void pushEvent(InputEvent type, uint8_t id) {
    uint8_t next = (queueHead + 1) % QUEUE_SIZE;
    if (next != queueTail) {
        eventQueue[queueHead].type = type;
        eventQueue[queueHead].id = id;
        eventQueue[queueHead].timestamp = millis();
        queueHead = next;
    }
}

// Process encoder quadrature (PA2=A/CLK, PA1=B/DT)
// On A falling edge: B=0 → CW, B=1 → CCW
static void processEncoder(uint8_t gpioA) {
    uint8_t encA = (gpioA >> MCP_ENC_A) & 1;
    uint8_t encB = (gpioA >> MCP_ENC_B) & 1;

    if (encA != lastEncA) {
        if (encA == 0) {
            // Falling edge of A
            if (encB == 0) {
                encoderDelta++;
                pushEvent(EVT_ENCODER_CW, INPUT_ENCODER_A);
            } else {
                encoderDelta--;
                pushEvent(EVT_ENCODER_CCW, INPUT_ENCODER_A);
            }
        }
        lastEncA = encA;
    }
}

// Process a single button with debounce, multi-click, and long-press
// Active low: bit=0 means pressed, bit=1 means released
static void processButton(uint8_t btnIdx, bool pressed, uint32_t now) {
    ButtonState* bs = &buttonStates[btnIdx];

    // Debounce: ignore changes within DEBOUNCE_MS of last change
    if (pressed != bs->isPressed) {
        if (now - bs->lastChangeMs < DEBOUNCE_MS) return;

        bs->lastChangeMs = now;
        bs->isPressed = pressed;

        if (pressed) {
            // Button just pressed
            bs->pressStartMs = now;
            bs->longFired = false;
            bs->shiftPressFired = false;

            // Shift+button: fire press IMMEDIATELY at press-down so handlers
            // can read live shift state. Otherwise the click-resolution delay
            // (DOUBLECLICK_MS = 300ms) means shift is usually released by the
            // time the press event reaches the consumer.
            // Skip btnIdx 5 (BTN_3) — it IS the shift button.
            if (shiftPhysicallyHeld && btnIdx != 5) {
                pushEvent(EVT_BUTTON_PRESS, btnIdx);
                bs->shiftPressFired = true;
            }
        } else {
            // Button just released
            pushEvent(EVT_BUTTON_RELEASE, btnIdx);

            if (bs->longFired) {
                // Long press release — don't fire click
                bs->longFired = false;
                bs->clickCount = 0;
                return;
            }

            if (bs->shiftPressFired) {
                // Press already fired at down-edge — suppress click resolution
                bs->shiftPressFired = false;
                bs->clickCount = 0;
                return;
            }

            // Click counting for double-click detection
            if (bs->clickCount == 0) {
                bs->firstClickMs = now;
                bs->clickCount = 1;
            } else {
                if (now - bs->firstClickMs < DOUBLECLICK_MS) {
                    bs->clickCount++;
                } else {
                    // Window expired, start new sequence
                    bs->firstClickMs = now;
                    bs->clickCount = 1;
                }
            }
        }
    }

    // Check long press (fires once while held)
    if (bs->isPressed && !bs->longFired) {
        if (now - bs->pressStartMs >= LONGPRESS_MS) {
            bs->longFired = true;
            bs->clickCount = 0;
            pushEvent(EVT_BUTTON_LONG, btnIdx);
        }
    }

    // Emit pending clicks after double-click window expires
    if (!bs->isPressed && bs->clickCount > 0) {
        if (now - bs->firstClickMs >= DOUBLECLICK_MS) {
            if (bs->clickCount >= 2) {
                pushEvent(EVT_BUTTON_DOUBLE, btnIdx);
            } else {
                pushEvent(EVT_BUTTON_PRESS, btnIdx);
            }
            bs->clickCount = 0;
        }
    }
}

// Update shift state machine based on BTN_2 (PA4) events
static void updateShiftState(bool btn2Pressed, uint32_t now) {
    // shiftPhysicallyHeld is updated by the caller before this runs; here we
    // only maintain the toggle/latch state machine.
    static bool wasHeldLast = false;
    bool wasHeld = wasHeldLast;
    wasHeldLast = btn2Pressed;

    if (btn2Pressed && !wasHeld) {
        // Shift just pressed
        shiftMode = SHIFT_HELD;
    } else if (!btn2Pressed && wasHeld) {
        // Shift just released
        uint32_t sinceLast = now - shiftReleaseMs;
        shiftReleaseMs = now;

        if (sinceLast < DOUBLECLICK_MS && shiftMode == SHIFT_TOGGLED) {
            // Double-tap → latch
            shiftMode = SHIFT_LATCHED;
        } else if (shiftMode == SHIFT_LATCHED) {
            // Was latched, tap again → off
            shiftMode = SHIFT_OFF;
        } else if (shiftMode == SHIFT_HELD) {
            // Single tap → toggle
            shiftMode = SHIFT_TOGGLED;
        }
    }
}

// ─── Public API ──────────────────────────────────────────────────────────────

bool MCPInput_init(void) {
    Serial.println("MCP23017: Initializing input driver on Wire2 (GPIO 16/17)...");

    wire2 = new TwoWire(1);
    wire2->begin((uint8_t)I2C2_SDA, (uint8_t)I2C2_SCL);
    wire2->setClock(400000);

    wire2->beginTransmission(MCP23017_I2C_ADDR);
    if (wire2->endTransmission() != 0) {
        Serial.println("MCP23017: Not found on Wire2!");
        delete wire2;
        wire2 = nullptr;
        return false;
    }

    // All pins as inputs with pull-ups
    mcpWrite(MCP_IODIRA, 0xFF);
    mcpWrite(MCP_IODIRB, 0xFF);
    mcpWrite(MCP_GPPUA, 0xFF);
    mcpWrite(MCP_GPPUB, 0xFF);

    // Interrupt on change
    mcpWrite(MCP_GPINTENA, 0xFF);
    mcpWrite(MCP_GPINTENB, 0xFF);
    mcpWrite(MCP_INTCONA, 0x00);
    mcpWrite(MCP_INTCONB, 0x00);

    // INTA GPIO
    gpio_set_direction(INTA_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INTA_GPIO, GPIO_PULLUP_ONLY);

    // Read initial state
    uint8_t gpioA, gpioB;
    readGPIO(gpioA, gpioB);
    lastGPIOA = gpioA;
    lastGPIOB = gpioB;
    lastEncA = (gpioA >> MCP_ENC_A) & 1;

    // Initialize button states (all released)
    for (int i = 0; i < 6; i++) {
        buttonStates[i].lastChangeMs = 0;
        buttonStates[i].pressStartMs = 0;
        buttonStates[i].isPressed = false;
        buttonStates[i].longFired = false;
        buttonStates[i].clickCount = 0;
        buttonStates[i].firstClickMs = 0;
        buttonStates[i].shiftPressFired = false;
    }

    // Initialize joystick previous state
    for (int i = 0; i < 5; i++) {
        lastJoyState[i] = false;
    }

    queueHead = queueTail = 0;
    encoderDelta = 0;
    shiftMode = SHIFT_OFF;

    initialized = true;
    Serial.println("MCP23017: Input driver initialized");
    Serial.printf("  Address: 0x%02X, INTA: GPIO%d\n", MCP23017_I2C_ADDR, MCP_INTA);

    return true;
}

const InputMsg* MCPInput_poll(void) {
    if (!initialized || !wire2) return nullptr;

    uint32_t now = millis();
    uint8_t gpioA, gpioB;
    readGPIO(gpioA, gpioB);

    // Process encoder quadrature
    processEncoder(gpioA);

    // Process encoder push (PA0) — active low
    // Handled separately from buttons: emits EVT_ENCODER_PUSH on press edge
    bool encPush = !(gpioA & (1 << MCP_ENC_SW));
    {
        ButtonState* bs = &buttonStates[0];
        if (encPush != bs->isPressed) {
            if (now - bs->lastChangeMs >= DEBOUNCE_MS) {
                bs->lastChangeMs = now;
                bs->isPressed = encPush;
                if (encPush) {
                    pushEvent(EVT_ENCODER_PUSH, INPUT_ENCODER_SW);
                }
            }
        }
    }

    // Process buttons (PA3-PA5) — active low
    bool btn1 = !(gpioA & (1 << MCP_BTN_1));
    bool btn2 = !(gpioA & (1 << MCP_BTN_2));
    bool btn3 = !(gpioA & (1 << MCP_BTN_3));

    // BTN_3 (bottom) is the shift button. Update shiftPhysicallyHeld BEFORE
    // processing other buttons so press-down sees the current shift state.
    shiftPhysicallyHeld = btn3;

    processButton(3, btn1, now);  // buttonStates[3] = BTN_1
    processButton(4, btn2, now);  // buttonStates[4] = BTN_2
    processButton(5, btn3, now);  // buttonStates[5] = BTN_3 (shift)

    // Update shift state machine (toggle/latch tracking) from BTN_3
    updateShiftState(btn3, now);

    // Process joystick (PB3-PB7) — active low, EDGE DETECTION
    bool joyStates[5] = {
        !(gpioB & (1 << (MCP_JOY_A - 8))),       // Up (PB3)
        !(gpioB & (1 << (MCP_JOY_B - 8))),       // Left (PB4)
        !(gpioB & (1 << (MCP_JOY_C - 8))),       // Down (PB5)
        !(gpioB & (1 << (MCP_JOY_CENTER - 8))),  // Center (PB6)
        !(gpioB & (1 << (MCP_JOY_D - 8)))        // Right (PB7)
    };

    static const InputEvent joyEvents[5] = {
        EVT_JOY_UP, EVT_JOY_LEFT, EVT_JOY_DOWN, EVT_JOY_CENTER, EVT_JOY_RIGHT
    };
    static const uint8_t joyIds[5] = {
        INPUT_JOY_UP, INPUT_JOY_LEFT, INPUT_JOY_DOWN, INPUT_JOY_CENTER, INPUT_JOY_RIGHT
    };

    // Detect press edges first
    bool joyEdges[5];
    for (int i = 0; i < 5; i++) {
        joyEdges[i] = joyStates[i] && !lastJoyState[i];
        lastJoyState[i] = joyStates[i];
    }

    // Center: fire immediately on edge, cancel any pending direction
    if (joyEdges[JOY_CENTER_IDX]) {
        pushEvent(EVT_JOY_CENTER, INPUT_JOY_CENTER);
        pendingDir = -1;
    }

    // Directions: defer emission. If center is currently held, drop the edge
    // entirely (center is the user's intent). Otherwise, latest direction
    // wins until the defer window expires.
    for (int i = 0; i < 5; i++) {
        if (i == JOY_CENTER_IDX) continue;
        if (joyEdges[i] && !joyStates[JOY_CENTER_IDX]) {
            pendingDir   = i;
            pendingDirMs = now;
        }
    }

    // Resolve pending direction once the defer window has elapsed
    if (pendingDir >= 0 && (now - pendingDirMs) >= JOY_DEFER_MS) {
        if (!joyStates[JOY_CENTER_IDX]) {
            pushEvent(joyEvents[pendingDir], joyIds[pendingDir]);
        }
        pendingDir = -1;
    }

    lastGPIOA = gpioA;
    lastGPIOB = gpioB;

    // Return next event if available
    if (queueTail != queueHead) {
        const InputMsg* msg = &eventQueue[queueTail];
        queueTail = (queueTail + 1) % QUEUE_SIZE;
        return msg;
    }

    return nullptr;
}

bool MCPInput_hasInterrupt(void) {
    return gpio_get_level(INTA_GPIO) == 0;
}

bool MCPInput_isShiftHeld(void) {
    // Report only physical hold state. The toggle/latch state machine is
    // retained internally but not exposed because a sticky-after-tap shift
    // makes shift+button combos unpredictable from the user's perspective.
    return shiftPhysicallyHeld;
}

int8_t MCPInput_getEncoderDelta(void) {
    int8_t delta = encoderDelta;
    encoderDelta = 0;
    return delta;
}

// Expose raw GPIO for debug printing
bool MCPInput_getRawGPIO(uint8_t* gpioA, uint8_t* gpioB) {
    if (!initialized || !wire2) return false;
    uint8_t a, b;
    readGPIO(a, b);
    if (gpioA) *gpioA = a;
    if (gpioB) *gpioB = b;
    return true;
}
