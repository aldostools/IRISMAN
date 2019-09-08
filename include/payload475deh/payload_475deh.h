#ifndef _PAYLOAD_475DEH_H
#define _PAYLOAD_475DEH_H


#include <unistd.h>

#define SKY10_PAYLOAD 1
#define ZERO_PAYLOAD 0

#define SYS36_PAYLOAD -1
#define WANIN_PAYLOAD -2

extern int is_firm_475deh(void);
extern int is_firm_476deh(void);
extern int is_firm_478deh(void);
extern int is_firm_481deh(void);
extern int is_firm_482deh(void);
extern int is_firm_483deh(void);
extern int is_firm_484deh(void);
extern void set_bdvdemu_475deh(int current_payload);
extern void load_payload_475deh(int mode);
extern int is_payload_loaded_475deh(void);

//#define CONFIG_USE_SYS8PERMH4 //disabled by default (testing, maybe not added on final release - if not usefull)
//#define CONFIG_USE_SYS8CONFIG //disabled, not working yet

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
