/*
    (c) 2011 Hermes/Estwald <www.elotrolado.net>
    IrisManager (HMANAGER port) (c) 2011 D_Skywalk <http://david.dantoine.org>

    HMANAGER4 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    HMANAGER4 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    apayloadlong with HMANAGER4.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <string.h>

#include <unistd.h>
#include <ppu-lv2.h>
#include <lv2/process.h>
#include <sys/process.h>
#include "syscall8.h"

char self_path[1024] = "/"__MKDEF_MANAGER_FULLDIR__;

/*******************************************************************************************************************************************************/
/* Payload                                                                                                                               */
/*******************************************************************************************************************************************************/

static u64 peekq(u64 addr)
{
    lv2syscall1(6, addr);
    return_to_user_prog(u64);
}

static void pokeq(u64 addr, u64 data)
{
    lv2syscall2(7, addr, data);
}

u64 restore_syscall8[2]= {0,0};

int is_payload_loaded(void)
{
    u64 addr = peekq(0x80000000000004f0ULL);

    if((addr>>32) == 0x534B3145)
    {
        addr&= 0xffffffff;
        if(addr && peekq(0x80000000000004f8ULL))
        {
            restore_syscall8[0]= peekq(0x80000000000004f8ULL); // (8*8)
            restore_syscall8[1]= peekq(restore_syscall8[0]);
            pokeq(restore_syscall8[0], 0x8000000000000000ULL + (u64) (addr + 0x20));
            return 2;
        }

        return 1;
    }

    return 0;
}

s32 main(s32 argc, const char* argv[])
{
    if(peekq(0x80000000007EF220ULL) == 0x45737477616C6420ULL && is_payload_loaded())
    {
        sys8_path_table(0LL); // break libfs.sprx re-direction

        if(restore_syscall8[0]) sys8_pokeinstr(restore_syscall8[0], restore_syscall8[1]);
    }

    if(argc > 0 && argv)
    {
        if(!strncmp(argv[0], "/dev_hdd0/game/", 15))
        {
            int n;

            strcpy(self_path, argv[0]);

            n = 15; while(self_path[n] != '/' && self_path[n] != 0) n++;

            if(self_path[n] == '/') self_path[n] = 0;
        }
    }

    strcat(self_path, "/USRDIR/RELOAD.SELF");

    sysProcessExitSpawn2(self_path, NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
}

