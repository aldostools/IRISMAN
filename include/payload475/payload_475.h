#ifndef _PAYLOAD_475_H
#define _PAYLOAD_475_H


#include <unistd.h>

#define SKY10_PAYLOAD 1
#define ZERO_PAYLOAD 0

#define SYS36_PAYLOAD -1
#define WANIN_PAYLOAD -2

extern int is_firm_475(void);
extern int is_firm_476(void);
extern int is_firm_478(void);
extern int is_firm_481(void);
extern int is_firm_482(void);
extern int is_firm_483(void);
extern int is_firm_484(void);
extern int is_firm_485(void);
extern int is_firm_486(void);
extern int is_firm_487(void);
extern int is_firm_488(void);
extern int is_firm_489(void);
extern int is_firm_490(void);
extern int is_firm_491(void);
extern void set_bdvdemu_475(int current_payload);
extern void load_payload_475(int mode);
extern int is_payload_loaded_475(void);

//#define CONFIG_USE_SYS8PERMH4 //disabled by default (testing, maybe not added on final release - if not usefull)
//#define CONFIG_USE_SYS8CONFIG //disabled, not working yet

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
