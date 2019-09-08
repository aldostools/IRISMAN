#include "pad.h"
#include <sysutil/sysutil.h>
#include <sys/systime.h>

unsigned temp_pad = 0, new_pad = 0, old_pad = 0;

padInfo padinfo;
padData paddata;
int pad_alive = 0;

int rumble1_on  = 0;
int rumble2_on  = 0;
int last_rumble = 0;

u64 pad_last_time = 0;
static u64 hot_temp_alarm = 0;

extern u16 iTimeoutByInactivity;

void fun_exit();
int sys_shutdown();
int sys_game_get_temperature(int sel, u32 *temperature);
void DrawDialogOKTimer(char * str, float milliseconds);
int DrawDialogYesNoTimer(char * str, float milliseconds);

unsigned ps3pad_read()
{
    int n;

    padActParam actparam;

    unsigned butt = 0;

    pad_alive = 0;

    static int count = 16;
    static u64 sec, nsec;
    count++;

    if(count > 15)
    {
        sysGetCurrentTime(&sec, &nsec);
        count = 0;

        if(pad_last_time == 0)
            pad_last_time = sec;

        if(iTimeoutByInactivity)
        {
            if((sec - pad_last_time) > (iTimeoutByInactivity * 3600))
            {
                if(DrawDialogYesNoTimer("System will be shutdown in two minutes by inactivity\nDo you want to abort the countdown?", 120000.0f) != 1)
                {
                    fun_exit();
                    sys_shutdown();
                    exit(0);
                }
                else
                    pad_last_time = 0;
            }
        }

        u32 temp = 0;
        u32 temp2 = 0;

        sys_game_get_temperature(0, &temp);
        sys_game_get_temperature(1, &temp2);

        if((temp >= 80 || temp2 >= 80))
        {
            if(!hot_temp_alarm) hot_temp_alarm = sec;
        } else
            hot_temp_alarm = 0;

        if(hot_temp_alarm && (sec - hot_temp_alarm) > 90)
        {
            DrawDialogOKTimer("WARNING: CPU/RSX Temperature is too high!\nSystem will be shutdown in 10 seconds\n", 10000.0f);
            fun_exit();
            sys_shutdown();
            exit(0);
        }
    }

    sysUtilCheckCallback();

    ioPadGetInfo(&padinfo);

    for(n = 0; n < MAX_PADS; n++)
    {
        if(padinfo.status[n])
        {
            ioPadGetData(n, &paddata);
            pad_alive = 1;
            butt = (paddata.button[2] << 8) | (paddata.button[3] & 0xff);

            /* Analog stick management */
            if (paddata.button[6] < 0x10)
                butt |= BUTTON_LEFT;
            else if (paddata.button[6] > 0xe0)
                butt |= BUTTON_RIGHT;

            if (paddata.button[7] < 0x10)
                butt |= BUTTON_UP;
            else if (paddata.button[7] > 0xe0)
                butt |= BUTTON_DOWN;

            if(butt) pad_last_time = sec;

            if (paddata.len > 0) break;
        }
    }


    if(!pad_alive) butt = 0;
    else
    {
        actparam.small_motor = 0;
        actparam.large_motor = 0;

        if(rumble1_on)
        {
            actparam.large_motor = 255;

            rumble1_on++;

            if(rumble1_on > 15) rumble1_on = 0;
        }

        if(rumble2_on)
        {
            actparam.small_motor = 1;

            rumble2_on++;

            if(rumble2_on > 10) rumble2_on = 0;
        }

        last_rumble = n;

        ioPadSetActDirect(n, &actparam);
    }

    temp_pad = butt;

    new_pad = temp_pad & (~old_pad); old_pad = temp_pad;

    return butt;
}
