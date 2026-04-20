/**
 * ESP32 Pin Configuration
 *
 * Pin mappings for Waveshare ESP32-S3-Touch-AMOLED-1.75
 * Source: Official schematic pinout table (verified 2026-02-17)
 * Reference: https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.75
 */

#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// ============================================================================
// Display (QSPI) - CO5300 AMOLED Driver
// ============================================================================
#define LCD_QSPI_SIO0    4       // QSPI Data 0
#define LCD_QSPI_SIO1    5       // QSPI Data 1
#define LCD_QSPI_SIO2    6       // QSPI Data 2
#define LCD_QSPI_SIO3    7       // QSPI Data 3
#define LCD_QSPI_SCL     38      // QSPI Clock
#define LCD_CS           12      // Chip Select
#define LCD_TE           13      // Tearing Effect
#define LCD_RESET        39      // Hardware Reset
#define LCD_WIDTH        466
#define LCD_HEIGHT       466

// ============================================================================
// Touch Panel (CST9217)
// ============================================================================
#define TP_INT           11      // Touch Interrupt
#define TP_RESET         40      // Touch Reset
#define TP_SCL           14      // Touch I2C Clock (shared with main I2C)
#define TP_SDA           15      // Touch I2C Data (shared with main I2C)

// ============================================================================
// I2S Audio (ES8311 DAC + ES7210 ADC share bus)
// ============================================================================
#define I2S_DSDIN        8       // Data IN to ES8311 (ESP32 → Codec)
#define I2S_SCLK         9       // Bit Clock (BCLK)
#define I2S_ASDOUT       10      // Data OUT from ES7210 (Codec → ESP32)
#define I2S_MCLK         42      // Master Clock (12.288 MHz via LEDC)
#define I2S_LRCK         45      // Left/Right Clock (Word Select)
#define PA_CTRL          46      // NS4150B Power Amplifier Enable

// Legacy aliases for compatibility with existing code
#define BCLK_PIN         I2S_SCLK
#define WS_PIN           I2S_LRCK
#define DOUT_PIN         I2S_DSDIN   // To codec (speaker)
#define DIN_PIN          I2S_ASDOUT  // From codec (microphone)
#define MCLK_PIN         I2S_MCLK
#define PA_EN            PA_CTRL

// ============================================================================
// I2C Bus (shared by all I2C devices)
// ============================================================================
#define I2C_SCL          14      // Shared: Touch, RTC, ES8311, ES7210, QMI8658
#define I2C_SDA          15      // Shared: Touch, RTC, ES8311, ES7210, QMI8658

// I2C Device Addresses
#define TCA9554_ADDR     0x18    // IO Expander (LCD power/reset control)
#define PCF8574_ADDR     0x20    // External button expander (optional)
#define ES8311_ADDR      0x30    // Audio DAC (speaker output)
#define ES7210_ADDR      0x40    // Audio ADC (4-ch microphone array)
#define RTC_ADDR         0x51    // PCF85063 RTC
#define CST9217_ADDR     0x5A    // Touch controller
#define QMI8658_ADDR     0x6B    // 6-axis IMU

// Legacy alias
#define TOUCH_ADDR       CST9217_ADDR

// ============================================================================
// TCA9554 IO Expander Pins (accent by I2C at 0x18)
// ============================================================================
#define TCA_TP_RST       0       // EXIO0: Touch panel reset
#define TCA_LCD_RST      1       // EXIO1: LCD reset
#define TCA_LCD_PWR      2       // EXIO2: LCD power enable
#define TCA_RTC_INT      3       // EXIO3: RTC interrupt
#define TCA_SYS_OUT      4       // EXIO4: System output
#define TCA_AXP_IRQ      5       // EXIO5: Power management IRQ
#define TCA_QMI_INT1     6       // EXIO6: IMU interrupt 1
#define TCA_GPS_RST      7       // EXIO7: GPS reset

// ============================================================================
// SD Card (SPI Mode)
// ============================================================================
#define SD_MOSI          1       // SPI Master Out
#define SD_SCK           2       // SPI Clock
#define SD_MISO          3       // SPI Master In
#define SD_CS            41      // Chip Select

// ============================================================================
// IMU (QMI8658)
// ============================================================================
#define QMI_INT2         21      // Direct GPIO interrupt

// ============================================================================
// USB
// ============================================================================
#define USB_DN           19      // USB D-
#define USB_DP           20      // USB D+

// ============================================================================
// UART (directly exposed on header)
// ============================================================================
#define UART0_TX         43      // U0TXD
#define UART0_RX         44      // U0RXD

// ============================================================================
// Optional External Pins (directly exposed)
// ============================================================================
#define EXT_GPIO16       16      // Available on header
#define EXT_GPIO17       17      // GPS RXD / NC
#define EXT_GPIO18       18      // GPS TXD / NC

// ============================================================================
// Boot/Power
// ============================================================================
#define BOOT_BTN         0       // Boot button (active low)

#endif // PIN_CONFIG_H
