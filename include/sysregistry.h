#ifndef _PS3SYSREGISTRY_
#define _PS3SYSREGISTRY_

#define LANG_GERMAN  0
#define LANG_ENGLISH 1
#define LANG_SPANISH 2
#define LANG_FRENCH  3
#define LANG_ITALIAN 4
#define LANG_DUTCH   5
#define LANG_PORTUGUESE 6
#define LANG_RUSSIAN 7
#define LANG_JAPANESE 8
#define LANG_KOREAN   9
#define LANG_CHINESE  10
#define LANG_CHINESES 11
#define LANG_FINNISH  12
#define LANG_SWEDISH  13
#define LANG_DANISH   14
#define LANG_NORWEGIAN 15
#define LANG_POLISH    16
#define LANG_PORTUGUESEB 17
#define LANG_ENGLISHUK 18

extern struct _PS3TimeZone {

    int ftime;
    char *name;

} PS3TimeZone[128];

extern int sys_language;
extern int sys_timezone;
extern int sys_dateformat;
extern int sys_summer;
extern int sys_parental_level;
extern int sys_button_layout;

int read_from_registry();

void PS3GetDateTime(u32 * hh, u32 * mm, u32 * ss, u32 * day, u32 * month, u32 * year);

#endif