/**
 * Scale Definitions
 *
 * Musical scale note intervals for scale-locked playing
 */

#include "config.h"

// Pentatonic major: 0, 2, 4, 7, 9
// Pentatonic minor: 0, 3, 5, 7, 10
// Major: 0, 2, 4, 5, 7, 9, 11
// Minor: 0, 2, 3, 5, 7, 8, 10
// Blues: 0, 3, 5, 6, 7, 10
// Dorian: 0, 2, 3, 5, 7, 9, 10
// Phrygian: 0, 1, 3, 5, 7, 8, 10

const int8_t SCALES[7][7] = {
    {0, 2, 4, 7, 9, 12, 14},  // Pentatonic major
    {0, 3, 5, 7, 10, 12, 15}, // Pentatonic minor
    {0, 2, 4, 5, 7, 9, 11},   // Major
    {0, 2, 3, 5, 7, 8, 10},   // Minor
    {0, 3, 5, 6, 7, 10, 12},  // Blues
    {0, 2, 3, 5, 7, 9, 10},   // Dorian
    {0, 1, 3, 5, 7, 8, 10}    // Phrygian
};
