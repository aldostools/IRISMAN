#ifndef OSK_INPUT_H
#define OSK_INPUT_H

#include <sysutil/osk.h>

void UTF16_to_UTF8(u16 *stw, u8 *stb);
void UTF8_to_UTF16(u8 *stb, u16 *stw);
void UTF8_to_Ansi(char *utf8, char *ansi, int len);
void UTF32_to_UTF8(u32 *stw, u8 *stb);

int Get_OSK_String(char *caption, char *str, int len);
int Get_OSK_String_no_lang(char *caption, char *str, int len);

#endif

