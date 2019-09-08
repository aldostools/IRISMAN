#ifndef __VSHTASK_H__
#define __VSHTASK_H__


extern int32_t vshtask_0F80B71F(const char *path);  // auth_module, checks sprx SCE header 

extern int32_t vshtask_166551C5(char *app_id);      // RTC Alarm Unregister

//extern int32_t vshtask_668E3C94();  // some RTC Alarm Register 	int vshtask_668E3C94(char *app_id?, uint8_t [0x4C]) 

extern int32_t vshtask_8D03E0FD(void);              // ? 

extern int32_t vshtask_A02D46E7(int32_t arg, const char *msg);  // vshtask_notification_msg()
#define vshtask_notify(msg) vshtask_A02D46E7(0, msg)

#endif // __VSHTASK_H__ 
