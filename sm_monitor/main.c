#include <sdk_version.h>
#include <cellstatus.h>
#include <cell/cell_fs.h>
#include <cell/pad.h>

#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/memory.h>
#include <sys/ss_get_open_psid.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netex/net.h>
#include <netex/errno.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "vshtask.h"

extern int stdc_273B9711(char *str, const char *fmt, ...);                            // sprintf()
#define sprintf stdc_273B9711

static char temp_buf[512];

#define BUTTON_LEFT       32768
#define BUTTON_DOWN       16384
#define BUTTON_RIGHT      8192
#define BUTTON_UP         4096
#define BUTTON_START      2048
#define BUTTON_R3         1024
#define BUTTON_L3         512
#define BUTTON_SELECT     256

#define BUTTON_SQUARE     128
#define BUTTON_CROSS      64
#define BUTTON_CIRCLE     32
#define BUTTON_TRIANGLE   16
#define BUTTON_R1         8
#define BUTTON_L1         4
#define BUTTON_R2         2
#define BUTTON_L2         1

#define SC_GET_FREE_MEM   (352)

typedef struct {
	uint32_t total;
	uint32_t avail;
} _meminfo;

SYS_MODULE_INFO(SMPLUGIN, 0, 1, 0);
SYS_MODULE_START(sm_start);
SYS_MODULE_STOP(sm_stop);

#define THREAD_NAME "sm_thread"
#define STOP_THREAD_NAME "sm_stop_thread"

static sys_ppu_thread_t thread_id=-1;
static volatile int done = 0;
static sys_ppu_thread_t usbwakeup_id=-1;
static volatile int done2 = 0;

int sm_start(uint64_t arg);
int sm_stop(void);

extern int vshmain_87BB0001(int param);

static inline void _sys_ppu_thread_exit(uint64_t val)
{
	system_call_1(41, val);
}

static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
	system_call_1(461, (uint64_t)(uint32_t)addr);
	return (int)p1;
}

/***************************/

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define s32 int32_t

#define FS_S_IFMT 0170000

#define LV2_SM_CMD_ADDR 0x8000000000000450ULL

#define SC_RING_BUZZER  (392)

#define BEEP1 { system_call_3(SC_RING_BUZZER, 0x1004, 0x4,   0x6); }
#define BEEP2 { system_call_3(SC_RING_BUZZER, 0x1004, 0x7,  0x36); }
#define BEEP3 { system_call_3(SC_RING_BUZZER, 0x1004, 0xa, 0x1b6); }

static inline u64 lv2peek(u64 addr)
{
    system_call_1(6, (u64) addr);
   return (u64)p1;

}

static inline u64 lv2poke(u64 addr, u64 value)
{
    system_call_2(7, (u64) addr, (u64) value);
    return (u64)p1;
}

static u32 lv2peek32(u64 addr) {
    u32 ret = (u32) (lv2peek(addr) >> 32ULL);
    return ret;
}

static u64 lv2poke32(u64 addr, u32 value)
{
    return lv2poke(addr, (((u64) value) <<32) | (lv2peek(addr) & 0xffffffffULL));
}


static int sys_game_get_temperature(int sel, u32 *temperature)
{
    u32 temp;

    system_call_2(383, (u64) (u32) sel, (u64) (u32) &temp);
    *temperature = (temp >> 24);
    return (int)p1;
}

static int sys_sm_set_fan_policy(u8 arg0, u8 arg1, u8 arg2)
{
    system_call_3(389, (u64) arg0, (u64) arg1, (u64) arg2);
    return (int)p1;
}

static int sys_sm_get_fan_policy(u8 id, u8 *st, u8 *mode, u8 *speed, u8 *unknown)
{
    system_call_5(409, (u64) id, (u64) (u32) st, (u64) (u32) mode, (u64) (u32) speed, (u64) (u32) unknown);
    return (int)p1;
}

static int sys_set_leds(u64 color, u64 state)
{
    system_call_2(386,  (u64) color, (u64) state);
    return (int)p1;
}

static u64 PAYLOAD_BASE = 0x8000000000000f70ULL;

volatile int temp_enable = 1;
volatile int set_main_flag = 0;

int cleds = 0, leds = 0;
int cspeed = 0x33;

u32 speed_table[8] = {
    0x4d,     // < 62°C temp_control0 -> 30%
    0x54,     // temp_control0 => 62°C temp_control1 -> 33%
    0x60,     // temp_control0 <= 68°C temp_control1 -> 37%
    0x68,     // >= 68°C temp_control1 -> 41%
    0x70,     // >= 70°C temp_control2 -> 45%
    0x78,     // >= 72°C temp_control3 -> 47%
    0xA0,     // >= 75°C temp_control4 -> 63%
    0xff      // >= 80°C temp_control5 -> 100%

};

u32 temp_control[6] = {
    62,
    68,
    70,
    72,
    75,
    80
};


int speed_up = 0;
u64 payload_ctrl;
int stopped = 0;

static void fan_manager(void)
{
    int n;
    static int alive_cont = 5;


    static int cpu_temp = 0;
    static int rsx_temp = 0;


    static int step = 0;
    static int step2 = 0;

    step2++;
    if(step2 >=5)
    {
        // in Mode 1 copy the datas from the payload area (same method as PFAN)
        step2 = 0;

        if((temp_enable & 127)!=2 && lv2peek32(PAYLOAD_BASE) == 0x50534D58)
        {
            // copy datas from payload
            payload_ctrl = (PAYLOAD_BASE + (lv2peek32(PAYLOAD_BASE + 4ULL ))) - 8ULL;
            cspeed = lv2peek32(payload_ctrl) & 0xff;
            payload_ctrl+= 0x4ULL;
            temp_enable = lv2peek32(payload_ctrl)!=0;
            if(!temp_enable) {set_main_flag = 1;}
            payload_ctrl+= 0x8ULL;
            for(n = 0; n < 7; n++)
            {
                speed_table[n] = lv2peek32(payload_ctrl);
                payload_ctrl+= 0x4ULL;
            }
            for(n = 0; n < 6; n++)
            {
                temp_control[n] = lv2peek32(payload_ctrl);
                payload_ctrl+= 0x4ULL;
            }

        }
    }

    if(temp_enable && step2 == 1)
    {
        temp_enable^=128;
        stopped = 0;

        if(step == 0)
        {
            n= sys_game_get_temperature(0, (void *) &cpu_temp);
            if(n < 0) cpu_temp = 0;
        }
        else
        {
            n= sys_game_get_temperature(1, (void *) &rsx_temp);
            if(n < 0) rsx_temp =0;
        }

        step^=1;

        int speed = speed_table[0];
        int temp = cpu_temp;

        if(rsx_temp > temp) temp = rsx_temp;

        if(temp >= (int) temp_control[1]) speed_up = 1;

        if     (temp >= (int) temp_control[5]) speed = speed_table[7];
        else if(temp >= (int) temp_control[4]) speed = speed_table[6];
        else if(temp >= (int) temp_control[3]) speed = speed_table[5];
        else if(temp >= (int) temp_control[2]) speed = speed_table[4];
        else if(temp >= (int) temp_control[1]) speed = speed_table[3];
        else if(temp >= (int) temp_control[0] && temp < (int) temp_control[1])
        {
             if(speed_up) speed = speed_table[2]; else speed = speed_table[1];
        }
        else speed_up = 0;

        if(speed !=cspeed && temp_enable)
        {
            if(sys_sm_set_fan_policy(0, 2, speed)==0)
            {
                cspeed = speed;
                if((temp_enable & 127)!=2 && lv2peek32(PAYLOAD_BASE) == 0x50534D58)
                {
                    payload_ctrl = (PAYLOAD_BASE + (lv2peek32(PAYLOAD_BASE + 4ULL ))) - 8ULL;
                    lv2poke32(payload_ctrl, cspeed);

                    if(temp >= (int) temp_control[5]) vshtask_notify("WARNING: OVERHEAT DANGER!\r\nFAN SPEED INCREASED!");
                }
            }
        }

        if(temp >= (int) temp_control[4]) leds = 3;
        else if(temp >= (int) temp_control[2]) leds = 2;
        else leds = 1;

        alive_cont--; if (alive_cont == 0) {leds = 0; alive_cont= 5; cspeed = 0;}


        if(temp_enable && leds != cleds && cleds != 0x10)
        {
           switch(leds)
           {
              case 0:
                sys_set_leds(2, 0);
                sys_set_leds(1, 1);
                break;

              case 1:
                sys_set_leds(2, 1);
                sys_set_leds(1, 1);
                break;

              case 2:
                sys_set_leds(2, 2);
                sys_set_leds(1, 1);
                break;

              case 3:
                sys_set_leds(2, 2);
                sys_set_leds(1, 2);
                break;
           }

            cleds =leds;
        }
    }
    else if(!temp_enable)
    {
        if(!set_main_flag)
        {
            sys_sm_set_fan_policy(0, 1, 0x5f);
            set_main_flag = 1;
        }

        stopped = 1;
    }
}


volatile int usb_enable = 0;

char usb_pdevice[]= "/dev_usb000";

char usb_path[]= "/dev_usb000/nosleep";


u32 usb_data[4] = {0, 1, 2, 3};
u32 usb_timer = 60;

u32 usb_device = 0;
int doit = 0;
int no_multitheaded = 0;

int set_usb_signal = 0;

static void UsbWakeup_tick(void)
{
    CellFsStat stat;
    s32 fd;
    u64 writed;
    int ret;
    int n;

    doit = 0;

        if(usb_enable) {
            usb_data[0]++;
            usb_data[1]^= usb_data[0] ^ 0x43651895;
            usb_data[2]^= usb_data[1] ^ 0x1a85f243;
            usb_data[3]=  usb_data[1] ^ usb_data[2];

            if(usb_enable == 2) n = 0; else n = usb_device; // usb_enable = 2 => scan all devices

            while(1)
            {
                if(n >= 9)
                {
                    ret= cellFsStat("/dev_bdvd", &stat);
                }
                else
                {
                    usb_pdevice[10]= '0' + n;
                    usb_path[10]= '0' + n;

                    ret= cellFsStat(usb_pdevice, &stat);
                }

                if(ret == 0)
                {
                    if(n>=9)
                    {
                        ret= cellFsStat("/dev_bdvd/nosleep", &stat);
                        if(ret == 0)
                            ret = cellFsOpen("/dev_bdvd/nosleep", CELL_FS_O_WRONLY | CELL_FS_O_CREAT | CELL_FS_O_TRUNC, &fd, NULL, 0);
                    }
                    else
                    {
                        ret= cellFsStat(usb_path, &stat); // only write in devices with "nosleep" file
                        if(ret == 0)
                            ret = cellFsOpen(usb_path, CELL_FS_O_WRONLY | CELL_FS_O_CREAT | CELL_FS_O_TRUNC, &fd, NULL, 0);
                    }

                    if(ret == 0)
                    {
                        if(!doit)
                        {
                            cleds = 0x10;
                            sys_set_leds(2, 0);
                            sys_set_leds(1, 0);
                        }

                        doit = 1;
                        cellFsChmod(usb_path, FS_S_IFMT | 0777);
                        cellFsWrite(fd, &usb_data, 16ULL, &writed);
                        cellFsClose(fd);
                    }
                }

                if(usb_enable == 1) break;
                    else {n++; if(n>=10) {n=0; break;}}

            }

            if(doit)
            {
                if(!no_multitheaded)
                {
                    sys_timer_sleep(2);
                    cleds = 0xff;
                    sys_set_leds(2, 1);
                    sys_set_leds(1, 1);
                }
                else
                    set_usb_signal = 1;
            }
        }
}


static void sm_thread(uint64_t arg)
{
    lv2poke32(PAYLOAD_BASE + 0x20ULL , 0xCAFE0ACA);

    vshtask_notify("SM Monitor loaded\r\nPress [SELECT] + [L2] to show");

    uint32_t cur_pad = 0, new_pad = 0, old_pad = 0; u8 show = 0;
    CellPadData data;

    #define PERSIST  100

    int sm_present = (lv2peek32(PAYLOAD_BASE) == 0x50534D45); // sm.self

    while (!done)
    {
        if(!sm_present) fan_manager();

        if(cellPadGetData(0, &data) == CELL_PAD_OK && (data.len > 0))
        {
            cur_pad = (data.button[3] | (data.button[2] << 8));

            new_pad = cur_pad & (~old_pad);

            old_pad = cur_pad;
        }
        else
            old_pad = 0;

        if((old_pad & (BUTTON_SELECT | BUTTON_L2)) == (BUTTON_SELECT | BUTTON_L2))
        {
            if(show == 0)        show = 1;               else
            if(show  < PERSIST) {BEEP1; show = PERSIST;} else
            if(show >= PERSIST) {BEEP2; show = 0;}
        }

        if(show == 1 || show == PERSIST)
        {
            uint32_t cpu_temp = 0, rsx_temp = 0;

            sys_game_get_temperature(0, &cpu_temp);
            sys_game_get_temperature(1, &rsx_temp);

            uint32_t blockSize;
            uint64_t freeSize;
            cellFsGetFreeSize((char*)"/dev_hdd0", &blockSize, &freeSize);

            _meminfo meminfo;
            {system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}

            u8 st, mode, speed, unknown;
            sys_sm_get_fan_policy(0, &st, &mode, &speed, &unknown);

            sprintf(temp_buf, "FAN :  %i%%\r\n"
                              "CPU :  %i°C / %i°F\r\n"
                              "RSX :  %i°C / %i°F\r\n"
                              "HDD :  %i MB Free\r\n"
                              "MEM :  %i KB Free", (int)(((int)speed*100)/255), cpu_temp, ((cpu_temp * 18) + 320)/10, rsx_temp, ((rsx_temp * 18) + 320)/10,
                                                   (int)((blockSize*freeSize)>>20), meminfo.avail>>10);


            vshtask_notify(temp_buf);
        }

        sys_timer_usleep(400000);

        if(show) {show++; if(show > 10 && show < PERSIST) show = 0; else if(show >= PERSIST + 12) show = PERSIST;}
    }

    lv2poke32(PAYLOAD_BASE + 0x20ULL , 0x0);
	sys_ppu_thread_exit(0);
}

static void thread_UsbWakeup(uint64_t arg)
{
	int count_timer = 0;

	while (!done2)
	{
        sys_timer_sleep(10);
        count_timer+= 10;
        if(count_timer < (int) usb_timer) continue;
        else count_timer= 0;

        UsbWakeup_tick();

	}

	sys_ppu_thread_exit(0);
}

int sm_start(uint64_t arg)
{
	sys_ppu_thread_create(&thread_id, sm_thread, 0, 3069, 0x1000, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME);
	sys_ppu_thread_create(&usbwakeup_id, thread_UsbWakeup, 0, 3070, 0x1000, SYS_PPU_THREAD_CREATE_JOINABLE, "UsbWakeup_Thread");
	// Exit thread using directly the syscall and not the user mode library or we will crash
	_sys_ppu_thread_exit(0);
	return SYS_PRX_RESIDENT;
}

static void sm_stop_thread(uint64_t arg)
{
	done = 1;

	if (thread_id != (sys_ppu_thread_t)-1)
	{
		uint64_t exit_code;
		sys_ppu_thread_join(thread_id, &exit_code);
	}

    done2 = 1;

	if (usbwakeup_id != (sys_ppu_thread_t)-1)
	{
		uint64_t exit_code;
		sys_ppu_thread_join(usbwakeup_id, &exit_code);
	}

	sys_ppu_thread_exit(0);
}

static void finalize_module(void)
{
	uint64_t meminfo[5];

	sys_prx_id_t prx = prx_get_module_id_by_address(finalize_module);

	meminfo[0] = 0x28;
	meminfo[1] = 2;
	meminfo[3] = 0;

	system_call_3(482, prx, 0, (uint64_t)(uint32_t)meminfo);
}

int sm_stop(void)
{
	sys_ppu_thread_t t;
	uint64_t exit_code;

	sys_ppu_thread_create(&t, sm_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	sys_ppu_thread_join(t, &exit_code);

	finalize_module();
	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}
