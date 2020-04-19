#ifndef CONSOLE_H
#define CONSOLE_H

extern int con_x;
extern int con_y;

void initConsole();

void DbgHeader(char *str);

void DbgMess(char *str);

void DbgDraw();

void DPrintf(char *format, ...);

void NPrintf(const char* fmt, ...);

#endif

