#ifndef PAD_H
#define PAD_H

#include <io/pad.h>

#define BUTTON_LEFT       32768
#define BUTTON_DOWN       16384
#define BUTTON_RIGHT      8192
#define BUTTON_UP         4096
#define BUTTON_START      2048
#define BUTTON_R3         1024
#define BUTTON_L3         512
#define BUTTON_SELECT     256
            
#define BUTTON_SQUARE     128
#define BUTTON_CROSS      64
#define BUTTON_CIRCLE     32
#define BUTTON_TRIANGLE   16
#define BUTTON_R1         8
#define BUTTON_L1         4
#define BUTTON_R2         2
#define BUTTON_L2         1

extern padInfo padinfo;
extern padData paddata;

extern unsigned new_pad; // new pad buttons pressed (only can see one time, when it change from 0 to 1)
extern unsigned old_pad; // old pad buttons pressed (only can change when you release the button)

extern int pad_alive; // if 1 paddata is valid

extern int rumble1_on; // used for rumble
extern int rumble2_on;

unsigned ps3pad_read(); // read the first conected pad

extern u64 pad_last_time;

#endif
