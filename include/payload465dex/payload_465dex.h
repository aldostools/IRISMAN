#ifndef _PAYLOAD_465DEX_H
#define _PAYLOAD_465DEX_H


#include <unistd.h>

#define SKY10_PAYLOAD 1
#define ZERO_PAYLOAD 0

#define SYS36_PAYLOAD -1
#define WANIN_PAYLOAD -2

extern int is_firm_465dex(void);
extern int is_firm_466dex(void);
extern void set_bdvdemu_465dex(int current_payload);
extern void load_payload_465dex(int mode);
extern int is_payload_loaded_465dex(void);

//#define CONFIG_USE_SYS8PERMH4 //disabled by default (testing, maybe not added on final release - if not usefull)
//#define CONFIG_USE_SYS8CONFIG //disabled, not working yet

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
