/**
 * Hardware Abstraction Layer - Storage (SD Card) Interface
 *
 * Defines the contract for storage operations.
 */

#ifndef STORAGE_HAL_H
#define STORAGE_HAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * File handle type (platform-specific)
 */
typedef void* FileHandle;

/**
 * Storage HAL initialization
 */
void StorageHAL_init(void);

/**
 * Check if storage is available
 */
bool StorageHAL_available(void);

/**
 * Get storage info
 * @param cardType Output: CARD_MMC, CARD_SD, CARD_SDHC, or CARD_UNKNOWN
 * @param cardSizeMB Output: Card size in megabytes
 */
void StorageHAL_getInfo(uint8_t* cardType, uint64_t* cardSizeMB);

/**
 * Open file for reading
 * @param path File path
 * @return FileHandle or NULL on failure
 */
FileHandle StorageHAL_openRead(const char* path);

/**
 * Open file for writing (creates/truncates)
 * @param path File path
 * @return FileHandle or NULL on failure
 */
FileHandle StorageHAL_openWrite(const char* path);

/**
 * Open file for appending
 * @param path File path
 * @return FileHandle or NULL on failure
 */
FileHandle StorageHAL_openAppend(const char* path);

/**
 * Close file
 * @param handle File handle
 * @return true on success
 */
bool StorageHAL_close(FileHandle handle);

/**
 * Read from file
 * @param handle File handle
 * @param buffer Buffer to read into
 * @param bytes Number of bytes to read
 * @return Number of bytes actually read
 */
uint32_t StorageHAL_read(FileHandle handle, void* buffer, uint32_t bytes);

/**
 * Write to file
 * @param handle File handle
 * @param buffer Buffer containing data
 * @param bytes Number of bytes to write
 * @return Number of bytes actually written
 */
uint32_t StorageHAL_write(FileHandle handle, const void* buffer, uint32_t bytes);

/**
 * Check if file exists
 */
bool StorageHAL_exists(const char* path);

/**
 * Delete file
 */
bool StorageHAL_remove(const char* path);

/**
 * List files in directory
 * @param path Directory path
 * @param callback Function to call for each file
 */
typedef void (*FileCallback)(const char* filename, bool isDir);
void StorageHAL_listDir(const char* path, FileCallback callback);

/**
 * Write WAV file header
 * @param handle File handle (must be open for writing)
 * @param sampleRate Sample rate (e.g., 16000, 44100)
 * @param bitsPerSample Bits per sample (8, 16, 24)
 * @param numChannels Number of channels (1=mono, 2=stereo)
 * @param dataSize Size of audio data in bytes
 * @return true on success
 */
bool StorageHAL_writeWavHeader(FileHandle handle, uint32_t sampleRate,
                               uint16_t bitsPerSample, uint16_t numChannels,
                               uint32_t dataSize);

/**
 * Create and open a new WAV file with header
 * @param path File path (e.g., "/loops/track001.wav")
 * @param sampleRate Sample rate
 * @param bitsPerSample Bits per sample
 * @param numChannels Channels
 * @return FileHandle or NULL on failure
 */
FileHandle StorageHAL_createWav(const char* path, uint32_t sampleRate,
                                uint16_t bitsPerSample, uint16_t numChannels);

#endif // STORAGE_HAL_H
