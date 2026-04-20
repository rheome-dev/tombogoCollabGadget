/**
 * LVGL Configuration File
 *
 * Minimal configuration for the Tombogo Collab Gadget
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*===================
   MEMORY SETTINGS
===================*/

#define LV_MEM_CUSTOM          0
#define LV_MEM_SIZE            (32U * 1024U)

#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR     0

/*===================
   FEATURE SETTINGS
===================*/

#define LV_USE_ANIMATION        1
#define LV_USE_SHADOW           1
#define LV_USE_OUTLINE          1
#define LV_USE_PATTERN          1
#define LV_USE_VSNPRINTF        1

/*===================
   DISPLAY SETTINGS
===================*/

#define LV_HOR_RES_MAX          466
#define LV_VER_RES_MAX          466

#define LV_COLOR_DEPTH         16

/*===================
   INPUT DEVICE SETTINGS
===================*/

#define LV_USE_INDEV            1
#define LV_INDEV_READ_TIMEOUT   50

/*===================
   FONT SETTINGS
===================*/

#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_14

/*===================
   WIDGET SETTINGS
===================*/

#define LV_USE_LABEL            1
#define LV_USE_BTN              1
#define LV_USE_SLIDER           1
#define LV_USE_ARC              1

#endif /*LV_CONF_H*/
