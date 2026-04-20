/**
 * ESP32 Platform - Storage HAL Implementation
 *
 * SD Card storage using SD_MMC for Waveshare ESP32-S3-Touch-AMOLED-1.75
 * Uses 1-bit SD mode to reduce pin conflicts
 *
 * Note: On this board, SD card chip select may be controlled via TCA9554 P4
 */

#ifdef PLATFORM_ESP32

#include "../../hal/storage_hal.h"
#include "pin_config.h"
#include <Arduino.h>
#include <SD_MMC.h>
#include <FS.h>

static bool sdAvailable = false;

void StorageHAL_init(void) {
    Serial.println("ESP32: Initializing SD Card (SD_MMC 1-bit mode)...");

    // Configure SD_MMC pins for 1-bit mode
    // Using only CLK, CMD, and DATA0 to minimize pin conflicts
    // Note: Some pins may conflict with display QSPI - check carefully

    // Try 1-bit mode first (fewer pins, more compatible)
    if (!SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT)) {
        Serial.println("ESP32: SD_MMC mount failed (1-bit mode)");

        // Try again with different settings
        if (!SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_HIGHSPEED)) {
            Serial.println("ESP32: SD card not available");
            sdAvailable = false;
            return;
        }
    }

    // Check card type
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("ESP32: No SD card inserted");
        sdAvailable = false;
        return;
    }

    const char* cardTypeStr = "Unknown";
    switch (cardType) {
        case CARD_MMC:  cardTypeStr = "MMC"; break;
        case CARD_SD:   cardTypeStr = "SD"; break;
        case CARD_SDHC: cardTypeStr = "SDHC"; break;
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("ESP32: SD Card Type: %s, Size: %llu MB\n", cardTypeStr, cardSize);

    // Create required directories
    if (!SD_MMC.exists("/loops")) {
        SD_MMC.mkdir("/loops");
        Serial.println("ESP32: Created /loops directory");
    }
    if (!SD_MMC.exists("/presets")) {
        SD_MMC.mkdir("/presets");
        Serial.println("ESP32: Created /presets directory");
    }

    sdAvailable = true;
    Serial.println("ESP32: SD card initialized successfully");
}

bool StorageHAL_available(void) {
    return sdAvailable;
}

void StorageHAL_getInfo(uint8_t* cardType, uint64_t* cardSizeMB) {
    if (!sdAvailable) {
        if (cardType) *cardType = 0;
        if (cardSizeMB) *cardSizeMB = 0;
        return;
    }

    if (cardType) *cardType = SD_MMC.cardType();
    if (cardSizeMB) *cardSizeMB = SD_MMC.cardSize() / (1024 * 1024);
}

FileHandle StorageHAL_openRead(const char* path) {
    if (!sdAvailable || !path) return NULL;

    File* file = new File();
    *file = SD_MMC.open(path, FILE_READ);

    if (!*file) {
        delete file;
        return NULL;
    }

    return (FileHandle)file;
}

FileHandle StorageHAL_openWrite(const char* path) {
    if (!sdAvailable || !path) return NULL;

    File* file = new File();
    *file = SD_MMC.open(path, FILE_WRITE);

    if (!*file) {
        delete file;
        return NULL;
    }

    return (FileHandle)file;
}

FileHandle StorageHAL_openAppend(const char* path) {
    if (!sdAvailable || !path) return NULL;

    File* file = new File();
    *file = SD_MMC.open(path, FILE_APPEND);

    if (!*file) {
        delete file;
        return NULL;
    }

    return (FileHandle)file;
}

bool StorageHAL_close(FileHandle handle) {
    if (!handle) return false;

    File* file = (File*)handle;
    file->close();
    delete file;

    return true;
}

uint32_t StorageHAL_read(FileHandle handle, void* buffer, uint32_t bytes) {
    if (!handle || !buffer || bytes == 0) return 0;

    File* file = (File*)handle;
    return file->read((uint8_t*)buffer, bytes);
}

uint32_t StorageHAL_write(FileHandle handle, const void* buffer, uint32_t bytes) {
    if (!handle || !buffer || bytes == 0) return 0;

    File* file = (File*)handle;
    return file->write((const uint8_t*)buffer, bytes);
}

bool StorageHAL_exists(const char* path) {
    if (!sdAvailable || !path) return false;
    return SD_MMC.exists(path);
}

bool StorageHAL_remove(const char* path) {
    if (!sdAvailable || !path) return false;
    return SD_MMC.remove(path);
}

void StorageHAL_listDir(const char* path, FileCallback callback) {
    if (!sdAvailable || !path || !callback) return;

    File root = SD_MMC.open(path);
    if (!root || !root.isDirectory()) {
        return;
    }

    File file = root.openNextFile();
    while (file) {
        callback(file.name(), file.isDirectory());
        file = root.openNextFile();
    }
}

bool StorageHAL_writeWavHeader(FileHandle handle, uint32_t sampleRate,
                               uint16_t bitsPerSample, uint16_t numChannels,
                               uint32_t dataSize) {
    if (!handle) return false;

    File* file = (File*)handle;

    // WAV file header structure (44 bytes total)
    struct WavHeader {
        // RIFF chunk descriptor
        char riff[4] = {'R', 'I', 'F', 'F'};
        uint32_t chunkSize;      // File size - 8
        char wave[4] = {'W', 'A', 'V', 'E'};

        // fmt sub-chunk
        char fmt[4] = {'f', 'm', 't', ' '};
        uint32_t subchunk1Size = 16;  // PCM = 16
        uint16_t audioFormat = 1;      // PCM = 1
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;        // SampleRate * NumChannels * BitsPerSample/8
        uint16_t blockAlign;      // NumChannels * BitsPerSample/8
        uint16_t bitsPerSample;

        // data sub-chunk
        char data[4] = {'d', 'a', 't', 'a'};
        uint32_t subchunk2Size;  // NumSamples * NumChannels * BitsPerSample/8
    } header;

    header.numChannels = numChannels;
    header.sampleRate = sampleRate;
    header.bitsPerSample = bitsPerSample;
    header.byteRate = sampleRate * numChannels * bitsPerSample / 8;
    header.blockAlign = numChannels * bitsPerSample / 8;
    header.subchunk2Size = dataSize;
    header.chunkSize = 36 + dataSize;

    // Write header
    size_t written = file->write((const uint8_t*)&header, sizeof(header));
    return written == sizeof(header);
}

FileHandle StorageHAL_createWav(const char* path, uint32_t sampleRate,
                                uint16_t bitsPerSample, uint16_t numChannels) {
    // Open file for writing
    FileHandle handle = StorageHAL_openWrite(path);
    if (!handle) return NULL;

    // Write placeholder header (will be updated when file is finalized)
    // Using 0 for data size - should be updated before closing
    if (!StorageHAL_writeWavHeader(handle, sampleRate, bitsPerSample, numChannels, 0)) {
        StorageHAL_close(handle);
        return NULL;
    }

    return handle;
}

#endif // PLATFORM_ESP32
