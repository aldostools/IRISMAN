#ifndef _PAYLOAD_481dex_H
#define _PAYLOAD_481dex_H


#include <unistd.h>

#define SKY10_PAYLOAD 1
#define ZERO_PAYLOAD 0

#define SYS36_PAYLOAD -1
#define WANIN_PAYLOAD -2

extern int is_firm_481dex(void);
extern int is_firm_482dex(void);
extern int is_firm_483dex(void);
extern int is_firm_484dex(void);
extern int is_firm_485dex(void);
extern int is_firm_486dex(void);
extern int is_firm_487dex(void);
extern int is_firm_488dex(void);
extern int is_firm_489dex(void);
extern int is_firm_490dex(void);
extern int is_firm_49Xdex(void);
extern void set_bdvdemu_481dex(int current_payload);
extern void load_payload_481dex(int mode);
extern int is_payload_loaded_481dex(void);

//#define CONFIG_USE_SYS8PERMH4 //disabled by default (testing, maybe not added on final release - if not usefull)
//#define CONFIG_USE_SYS8CONFIG //disabled, not working yet

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
