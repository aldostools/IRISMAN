#ifndef _PAYLOAD_355DEX_H
#define _PAYLOAD_355DEX_H


#include <unistd.h>

#define SKY10_PAYLOAD 1
#define ZERO_PAYLOAD 0

#define SYS36_PAYLOAD -1
#define WANIN_PAYLOAD -2

extern int is_firm_355dex(void);
extern void set_bdvdemu_355dex(int current_payload);
extern void load_payload_355dex(int mode);
extern int is_payload_loaded_355dex(void);

void install_new_poke_355dex();
void remove_new_poke_355dex();
int map_lv1_355dex(void);
void unmap_lv1_355dex();
void patch_lv2_protection_355dex();

//#define CONFIG_USE_SYS8PERMH4 //disabled by default (testing, maybe not added on final release - if not usefull)
//#define CONFIG_USE_SYS8CONFIG //disabled, not working yet

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
