#ifndef _PAYLOAD_355_H
#define _PAYLOAD_355_H


#include <unistd.h>

#define SKY10_PAYLOAD 1
#define ZERO_PAYLOAD 0

#define SYS36_PAYLOAD -1
#define WANIN_PAYLOAD -2

extern uint64_t peekq(uint64_t addr);
extern void pokeq(uint64_t addr, uint64_t val);
extern void pokeq32(uint64_t addr, uint32_t val);

extern int is_firm_355(void);
extern void set_bdvdemu_355(int current_payload);
extern void load_payload_355(int mode);

extern int map_lv1(void);
extern void unmap_lv1(void);
extern void patch_lv2_protection(void);
extern void install_new_poke(void);
extern void remove_new_poke(void);
extern int is_payload_loaded_355(void);

//#define CONFIG_USE_SYS8PERMH4 //disabled by default (testing, maybe not added on final release - if not usefull)
//#define CONFIG_USE_SYS8CONFIG //disabled, not working yet

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
