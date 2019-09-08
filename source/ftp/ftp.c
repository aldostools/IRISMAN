#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <ppu-types.h>
#include <net/net.h>
#include <net/netctl.h>

#include <sys/file.h>
#include <sys/thread.h>
#include <sys/systime.h>

#include "defines.h"
#include "server.h"
#include "functions.h"
#include "ftp.h"

extern char passwd[64];

int ftp_port = 21;

int appstate = 0;

static int ftp_initialized = 0;

volatile int ftp_working = 0;

char ftp_ip_str[256] = "";

static sys_ppu_thread_t thread_id;
static sys_ppu_thread_t thread_id2;

static void control_thread(void *a)
{
    int n = 0;

    while(appstate != 1) {
        sysUsleep(100000);
        n++;
        if(ftp_working == 2) {ftp_working = 3; n = 0;}
        if(n >= 100) {
            if(ftp_working == 3) ftp_working = 0;
            n = 0;
        }
    }

    sysThreadExit(0);
}

int get_ftp_activity()
{
    if(!ftp_initialized) return 0;

    return (ftp_working!=0);
}

static int _net_initialized = 0;

int ftp_net_init()
{
    if(_net_initialized) return 0;

    if(netInitialize()<0) return -1;

    if(netCtlInit()<0) {netDeinitialize();return -2;}

    _net_initialized = 1;

    return 0;

}

int ftp_net_deinit()
{
    if(!_net_initialized) return 0;

    _net_initialized = 0;

    netCtlTerm();
    netDeinitialize();

    return 0;

}

int ftp_net_status()
{
    if(!_net_initialized) return -1;

    s32 state = 0;

    if(netCtlGetState(&state)<0 || state !=NET_CTL_STATE_IPObtained) {

        return -4;
    }

    return 0;

}

int ftp_init()
{
    if(ftp_initialized) return 1;

    ftp_working = 0;

    int r = ftp_net_init();
    if(r < 0) return r;

    s32 state = 0;

    if(netCtlGetState(&state) < 0 || state != 3) {
        ftp_initialized = 0;
        ftp_net_deinit();
        return -4;
    }

    union net_ctl_info info;

    if(netCtlGetInfo(NET_CTL_INFO_IP_ADDRESS, &info) == 0)
    {
        // start server thread

        appstate = 0;
        sprintf(ftp_ip_str, "FTP active (%s:%i)", info.ip_address, ftp_port);
        sysThreadCreate(&thread_id, listener_thread, NULL, 999, 0x400, THREAD_JOINABLE, "listener");
        sysThreadCreate(&thread_id2, control_thread, NULL, 1501, 0x400, THREAD_JOINABLE, "ctrl_ftp");
/*
        s32 fd;
        u64 read = 0;

        if(sysLv2FsOpen(OFTP_PASSWORD_FILE, SYS_O_RDONLY | SYS_O_CREAT, &fd, 0660, NULL, 0) == 0)
        {
            sysLv2FsRead(fd, passwd, 63, &read);
        }

        passwd[read] = '\0';
        sysLv2FsClose(fd);

        // prevent multiline passwords
        strreplace(passwd, '\r', '\0');
        strreplace(passwd, '\n', '\0');
*/

        char dlgmsg[256];
        sprintf(dlgmsg, "OpenPS3FTP %s by jjolano (Twitter: @jjolano)\n"
                        "Website: http://jjolano.dashhacks.com\n"
                        "Donations: http://bit.ly/gB8CJo\n"
                        "Status: FTP Server Active (%s port 21)\n\n"
                        "Press OK to exit this program.",
            OFTP_VERSION, info.ip_address);

        //msgDialogOpen2(mt_ok, dlgmsg, dialog_handler, NULL, NULL);
        ftp_initialized = 1;

        return 0;

    }
    else
    {
        //msgDialogOpen2(mt_ok, OFTP_ERRMSG_NETWORK, dialog_handler, NULL, NULL);

        ftp_initialized = 0;
        ftp_net_deinit();
    }

    return -3;
}

void ftp_deinit()
{
    if(!ftp_initialized) return;

    appstate = 1;

    ftp_net_deinit();

    u64 retval;
    sysThreadJoin(thread_id, &retval);
    sysThreadJoin(thread_id2, &retval);

    ftp_initialized = 0;
    ftp_working = 0;

    memset(ftp_ip_str, 0, sizeof(ftp_ip_str));
}
