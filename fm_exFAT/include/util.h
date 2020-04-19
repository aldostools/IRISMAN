//util.h
#ifndef __UTIL_H__
#define __UTIL_H__

#include <tiny3d.h>

// change to 2D context ( virtual size of the screen is 848.0 x 512.0)
//for 8x8 font, we can split the screen into 106x64 chars - 2 panes w53chars
//we leave 8 lines for progress/status/info below the panes
#define PANEL_W 53
#define PANEL_H 60
#define STATUS_H 4

#define CBSIZE  256     //char buffer size

extern char *na_string;
void do_flip ();

//dialog
int YesNoDialog (char * str);
//progress dialog
int ProgressBarActionGet ();
void DoubleProgressBarDialog(char *caption);
void ProgressBarUpdate(u32 cprc, const char *msg);
void ProgressBar2Update(u32 cprc, const char *msg);

int fps_update ();
void DrawRect2d (float x, float y, float z, float w, float h, u32 rgba);
int NPad (int btn);
int PPad (int btn);
int APad (int btn);

//osk_input requires these
void UTF8_to_UTF16(u8 *stb, u16 *stw);
void UTF16_to_UTF8(u16 *stw, u8 *stb);
void Draw_scene();
void cls();

#endif
