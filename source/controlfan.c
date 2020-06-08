/*
    (c) 2013 Estwald <www.elotrolado.net>

    "ControlFan" is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    "ControlFan" is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with "ControlFan". If not, see <http://www.gnu.org/licenses/>.

*/

/* NOTE: Added CFW Debug and CFW 4.41 patch from PS3 Ita Manager versionb

Credits:

- Rancid-o
- Zz_SACRO_zZ

*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <ppu-lv2.h>

#include <tiny3d.h>
#include "libfont2.h"
#include "main.h"
#include "utils.h"
#include "gfx.h"
#include "language.h"
#include "controlfan.h"

#include "cobra.h"
#include "modules.h"
#include "ps3_controlfan_bin.h"
#include "payload_sm_bin.h"

char * LoadFile(char *path, int *file_size);
int SaveFile(char *path, char *mem, int file_size);

#define MAX(a, b)	((a) >= (b) ? (a) : (b))

#define FANCONTROL_PATH    (temp_buffer + 2048)

#define LV2_SM_CMD_ADDR 0x8000000000000450ULL

#define WITH_LEDS 2
#define WITHOUT_LEDS 1

#undef AUTO_BUTTON_REP2
#define AUTO_BUTTON_REP2(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 10) {v = 0; new_pad |= b;} \
                                 } else v = 0;

#define ROT_INC(x ,y , z) {x++; if(x > y) x = z;}
#define ROT_DEC(x ,y , z) {x--; if(x < y) x = z;}


#define WHITE  0xffffffff
#define RED    0xff0000ff
#define BLUE   0x0020c020
#define YELLOW 0xfff000ff

extern char temp_buffer[8192];
extern int flash;
extern u64 frame_count;
extern char self_path[MAXPATHLEN];
extern bool is_ps3hen;

static bool is_sm_sprx_loaded = false;

/*
static u64 lv2peek(u64 addr)
{
    lv2syscall1(6, (u64) addr);
    return_to_user_prog(u64);

}

static u64 lv2poke(u64 addr, u64 value)
{
    lv2syscall2(7, (u64) addr, (u64) value);
    return_to_user_prog(u64);
}
*/

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

    lv2syscall2(383, (u64) sel, (u64) &temp);
    *temperature = (temp >> 24);
    return_to_user_prog(int);
}

static int sys_sm_set_fan_policy(u8 arg0, u8 arg1, u8 arg2)
{
    lv2syscall3(389, (u64) arg0, (u64) arg1, (u64) arg2);
    return_to_user_prog(int);
}

static int sys_sm_get_fan_policy(u8 id, u8 *st, u8 *mode, u8 *speed, u8 *unknown)
{
    lv2syscall5(409, (u64) id, (u64) st, (u64) mode, (u64) speed, (u64) unknown);
    return_to_user_prog(int);
}


static int sys_set_leds(u64 color, u64 state)
{
    lv2syscall2(386,  (u64) color, (u64) state);
    return_to_user_prog(int);
}


extern u64 syscall_base;
extern int firmware;
static u64 PAYLOAD_BASE = 0x8000000000000f70ULL;

static bool is_ctrl_fan_loaded = false;

static u64 sys383_offset = 0;
static u64 sys409_offset = 0;
static u64 sys389_offset = 0;
static u64 sys386_offset = 0;

int test_controlfan_compatibility()
{
    return get_controlfan_offsets();
}

int get_controlfan_offsets()
{
    if(is_ps3hen) return 0;

    if(sys389_offset) return 1;

    if(firmware == 0x341C) {        // firmware 3.41 2EB128

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000AEA0ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000008644ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x8000000000008B40ULL;
        // enables sys_set_leds
        sys386_offset = 0x8000000000008C08ULL;

    } else if(firmware == 0x355C) { // firmware 3.55 346570

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000B518ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000008CBCULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x80000000000091B8ULL;
        // enables sys_set_leds
        sys386_offset = 0x8000000000009280ULL;

    } else if(firmware == 0x355D || firmware == 0x355E) { // firmware 3.55 dex / deh

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000B598ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000008D3CULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x8000000000009238ULL;
        // enables sys_set_leds
        sys386_offset = 0x8000000000009300ULL;

    } else if(firmware == 0x421C) { // firmware 4.21 cex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C698ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009E28ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A324ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A3ECULL;

    } else if(firmware == 0x421D) { // firmware 4.21 dex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C718ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009EA8ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A3A4ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A46CULL;

    } else if(firmware == 0x430C) { // firmware 4.30 cex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C694ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009E28ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A324ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A3ECULL;

    } else if(firmware == 0x430D) { // firmware 4.30 dex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C714ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009EA8ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A3A4ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A46CULL;

    } else if(firmware == 0x431C) { // firmware 4.31 cex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C698ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009E28ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A324ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A3ECULL;

    } else if(firmware == 0x440C) { // firmware 4.40 cex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C694ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009E28ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A324ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A3ECULL;

    } else if(firmware == 0x441C) { // firmware 4.41 cex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C698ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009E28ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A324ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A3ECULL;

    } else if(firmware == 0x441D) { // firmware 4.41 DEX

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C718ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009EA8ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A3A4ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A46CULL;

    } else if(firmware == 0x446C) { // firmware 4.46 cex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C698ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009E28ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A324ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A3ECULL;

    } else if(firmware == 0x446D) { // firmware 4.46 DEX

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C718ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009EA8ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A3A4ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A46CULL;

    } else if(firmware == 0x450C) { // firmware 4.50 cex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C694ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009E28ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A324ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A3ECULL;

    } else if(firmware == 0x450D) { // firmware 4.50 DEX

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C714ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009EA8ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A3A4ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A46CULL;

    } else if(firmware == 0x453C) { // firmware 4.53 cex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C698ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009E28ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A324ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A3ECULL;

    } else if(firmware == 0x453D) { // firmware 4.53 dex

        // enables sys_game_get_temperature
        sys383_offset = 0x800000000000C718ULL;
        // enables sys_sm_get_fan_policy
        sys409_offset = 0x8000000000009EA8ULL;
        // enables sys_sm_set_fan_policy
        sys389_offset = 0x800000000000A3A4ULL;
        // enables sys_set_leds
        sys386_offset = 0x800000000000A46CULL;

    } else if(firmware == 0x455C) { // firmware 4.55 cex

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C6A8ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009E38ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A334ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A3FCULL;

    } else if(firmware == 0x455D) { // firmware 4.55 dex

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C728ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009EB8ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A3B4ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A47CULL;

    } else if(firmware == 0x460C) { // firmware 4.60 cex

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C6A4ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009E38ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A334ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A3FCULL;

    } else if((firmware == 0x460D) || (firmware == 0x460E)) { // firmware 4.60 DEX / DEH

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C724ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009EB8ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A3B4ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A47CULL;

    } else if((firmware == 0x465C) || (firmware == 0x466C)) { // firmware 4.65-4.66 cex

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C6A8ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009E38ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A334ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A3FCULL;

    } else if((firmware == 0x465D) || (firmware == 0x466D)) { // firmware 4.65-4.66 DEX

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C728ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009EB8ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A3B4ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A47CULL;

    } else if(firmware == 0x470C) { // firmware 4.70 cex

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C6A4ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009E38ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A334ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A3FCULL;

    } else if(firmware == 0x470D) { // firmware 4.70 dex

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C724ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009EB8ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A3B4ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A47CULL;

    } else if((firmware == 0x475C) || (firmware == 0x476C) || (firmware == 0x478C) || (firmware == 0x481C) || (firmware == 0x482C) || (firmware == 0x483C) || (firmware == 0x484C) || (firmware == 0x485C) || (firmware == 0x486C) || (firmware == 0x487C)) { // firmware 4.75-4.87 cex

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C6A8ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009E38ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A334ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A3FCULL;

    } else if((firmware == 0x475D) || (firmware == 0x476D) || (firmware == 0x478D) || (firmware == 0x481D) || (firmware == 0x482D) || (firmware == 0x483D) || (firmware == 0x484D) || (firmware == 0x485D) || (firmware == 0x486D) || (firmware == 0x487D)
           || (firmware == 0x475E) || (firmware == 0x476E) || (firmware == 0x478E) || (firmware == 0x481E) || (firmware == 0x482E) || (firmware == 0x483E) || (firmware == 0x484E) || (firmware == 0x485E) || (firmware == 0x486E) || (firmware == 0x487E)) { // firmware 4.75-4.87 dex / deh

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C728ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009EB8ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A3B4ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A47CULL;

    } else if(firmware == 0x480C) { // firmware 4.80 cex

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C6A4ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009E38ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A334ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A3FCULL;

    } else if((firmware == 0x480D) || (firmware == 0x480E)) { // firmware 4.80 dex / deh

       // enables sys_game_get_temperature
       sys383_offset = 0x800000000000C724ULL;
       // enables sys_sm_get_fan_policy
       sys409_offset = 0x8000000000009EB8ULL;
       // enables sys_sm_set_fan_policy
       sys389_offset = 0x800000000000A3B4ULL;
       // enables sys_set_leds
       sys386_offset = 0x800000000000A47CULL;
    }

    return (sys389_offset ? 1 : 0);
}

static u64 payload_ctrl;

static int load_ps3_controlfan_sm_sprx()
{
    // test if sm.sprx pseudo payload is loaded
    if(lv2peek32(PAYLOAD_BASE) == 0x50534D58 && use_cobra)
    {
        if(is_sm_sprx_loaded || lv2peek32(PAYLOAD_BASE + 0x20ULL) == 0xCAFE0ACA) { set_usleep_sm_main(1000); return 0;}

        struct stat s;

        sprintf(temp_buffer, PLUGIN_SM, self_path);

         // creates sm.sprx if it does not exist
        if(stat(temp_buffer, &s) < 0 || s.st_size != SIZE_SPRX_SM)
        {
            SaveFile(temp_buffer, (char *) sprx_sm, SIZE_SPRX_SM);

            if(stat(temp_buffer, &s) < 0 || s.st_size != SIZE_SPRX_SM)
            {
                DrawDialogTimer("error creating sprx_sm file", 2000.0f);
                return FAILED;
            }
        }

        // load sm.sprx
        int slot = get_vsh_plugin_free_slot();

        if(slot == FAILED)
        {
            DrawDialogTimer("error loading sprx_sm file", 2000.0f);
            return FAILED;
        }

        cobra_unload_vsh_plugin(slot);

        if (cobra_load_vsh_plugin(slot, temp_buffer, NULL, 0x0) == 0)
        {
            is_sm_sprx_loaded = true;
            set_usleep_sm_main(1000);
        }
        else
        {
            return FAILED;
        }
    }

    return 0;

}

int load_ps3_controlfan_payload()
{
    if(!test_controlfan_compatibility()) return 0;
    if(!syscall_base) return 0;

    int ret = 0, use_sm_prx = 0;

    int payload_size = MAX(ps3_controlfan_bin_size, payload_sm_bin_size);

    u64 *addr = (u64 *) memalign(8, payload_size + 31);

    if(!addr) return 0;

    if(lv2peek(PAYLOAD_BASE)) {if(lv2peek32(PAYLOAD_BASE) == 0x50534D58) set_usleep_sm_main(1000); goto skip_the_load;}

    if(use_cobra || use_mamba)
    {
        struct stat s;

         // creates sm.sprx if it does not exist
        sprintf(temp_buffer, PLUGIN_SM, self_path);
        if(stat(temp_buffer, &s) < 0 || s.st_size != SIZE_SPRX_SM)
        {
            SaveFile(temp_buffer, (char *) sprx_sm, SIZE_SPRX_SM);
        }

        if(!stat(temp_buffer, &s) && s.st_size == SIZE_SPRX_SM)
        {
            memcpy((char *) addr, (char *) payload_sm_bin, payload_sm_bin_size);
            use_sm_prx = 1;
            addr[1] = syscall_base;
            addr[2] += PAYLOAD_BASE;
            addr[3] = lv2peek(syscall_base + (u64) (379 * 8));
            goto set_control_datas;
        }
    }

    // ps3_controlfan_bin
    memcpy((char *) addr, (char *) ps3_controlfan_bin, ps3_controlfan_bin_size);

    addr[1] = syscall_base;

    addr[2] += PAYLOAD_BASE;
    addr[3] = lv2peek(syscall_base + (u64) (130 * 8));
    addr[4] += PAYLOAD_BASE;
    addr[5] = lv2peek(syscall_base + (u64) (138 * 8));

    addr[6] += PAYLOAD_BASE;
    addr[7] = lv2peek(syscall_base + (u64) (379 * 8));

set_control_datas:

    for(int n = 0; n < 200; n++)
    {
        for(int m = 0; m < ((payload_size + 7) & ~7); m += 8)
            lv2poke(PAYLOAD_BASE + (u64) m, addr[m>>3]);

        if(use_sm_prx)
        {
            lv2poke(syscall_base + (u64) (379 * 8), PAYLOAD_BASE + 0x10ULL);
        }
        else
        {
            lv2poke(syscall_base + (u64) (130 * 8), PAYLOAD_BASE + 0x10ULL);
            lv2poke(syscall_base + (u64) (138 * 8), PAYLOAD_BASE + 0x20ULL);
            lv2poke(syscall_base + (u64) (379 * 8), PAYLOAD_BASE + 0x30ULL);
        }

        usleep(10000);
    }

    sleep(1);

    ret = get_controlfan_offsets();

    if(ret)
    {
        // enables sys_game_get_temperature
        lv2poke32(sys383_offset, 0x38600000); //sys383
        // enables sys_sm_get_fan_policy
        lv2poke32(sys409_offset, 0x38600001); //sys409
        // enables sys_sm_set_fan_policy
        lv2poke32(sys389_offset, 0x38600001); //sys389
        // enables sys_set_leds
        lv2poke32(sys386_offset, 0x38600001); //sys386
    }

    if(use_sm_prx) load_ps3_controlfan_sm_sprx();

skip_the_load:
    is_ctrl_fan_loaded = true;
    free(addr);
    return ret;
}

static u32 speed_table[8] = {
    0x5f,     // sm_shutdown / manual mode
    0x4d,     // < temp_control0
    0x54,     // temp_control0 => temp_control1
    0x60,     // temp_control0 <= temp_control1
    0x68,     // >= temp_control1
    0x70,     // >= temp_control2
    0x78,     // >= temp_control3
    0xA0,     // >= temp_control4
};

static u32 temp_control[6] = {
    62,
    68,
    70,
    72,
    75,
    80
};

static u32 speed_table_default[8] = {
    0x5f,     // sm_shutdown / manual mode (37%)
    0x4d,     // < temp_control0 (30%)
    0x54,     // temp_control0 => temp_control1 (32%)
    0x60,     // temp_control0 <= temp_control1 (37%)
    0x68,     // >= temp_control1 (40%)
    0x70,     // >= temp_control2 (43%)
    0x78,     // >= temp_control3 (47%)
    0xA0,     // >= temp_control4 (62%)
};

static u32 temp_control_default[6] = {
    62,
    68,
    70,
    72,
    75,
    80
};

enum fan_modes {
    FANCTRL_PAYLOAD     = 0,
    FANCTRL_SYSCON      = 1,
    FANCTRL_USER_VALUES = 2,
    FANCTRL_DISABLED    = 3,
};

static u32 speed_table_backup[8];

static u32 temp_control_backup[5];
static u32 wakeup_time_backup;
static int fan_mode_backup;

static u32 wakeup_time = 60;

static int fan_mode = FANCTRL_DISABLED;     //fanctrl disabled by default
static int cur_fan_mode = FANCTRL_DISABLED; //fanctrl disabled by default

void set_fan_mode(int mode)
{
    u64 command;

    if(mode == -1) mode = fan_mode;

    if(!test_controlfan_compatibility()) return;

    cur_fan_mode = mode;
    if(fan_mode == FANCTRL_DISABLED) return;

    if(is_ctrl_fan_loaded == false)
    {
      load_ps3_controlfan_payload();
    }

    payload_ctrl = (PAYLOAD_BASE + (lv2peek32(PAYLOAD_BASE + 4ULL ))) - 8ULL;
    lv2poke32(payload_ctrl + 4ULL, 0); // disabled

    command =lv2peek(LV2_SM_CMD_ADDR)>>56ULL;
    if(command == 0x55ULL)
    {   // SM present!
        while(true)
        {
            lv2poke(LV2_SM_CMD_ADDR, 0xFF00000000000000ULL); // get status
            usleep(2000);
            command =lv2peek(LV2_SM_CMD_ADDR);
            if((command>>48ULL) == 0x55FFULL && (command & 0xF) == 1) break; // SM Fan Control Stopped
        }
    }
    usleep(100000); // waits

    sys_set_leds(2, 0); // restore LEDS
    sys_set_leds(0, 0);
    sys_set_leds(1, 1);

    if(mode == FANCTRL_SYSCON)
        sys_sm_set_fan_policy(0, 1, 0x0);
    else if(mode == FANCTRL_USER_VALUES)
        sys_sm_set_fan_policy(0, 2, speed_table[0]);
    else
    {
        lv2poke32(payload_ctrl +  0ULL, 0x33); // current fan speed
        lv2poke32(payload_ctrl +  4ULL, 0); // 0 - disabled, 1 - enabled without leds, 2 - enabled with leds
        usleep(100000);  // waits

        lv2poke32(payload_ctrl +  8ULL, speed_table[0]); // fan speed in shutdown syscall (when it calls PS2 Emulator)
        lv2poke32(payload_ctrl + 12ULL, speed_table[1]); // fan speed < temp_control0
        lv2poke32(payload_ctrl + 16ULL, speed_table[2]); // fan speed temp_control0 => temp_control1
        lv2poke32(payload_ctrl + 20ULL, speed_table[3]); // fan speed temp_control0 <= temp_control1
        lv2poke32(payload_ctrl + 24ULL, speed_table[4]); // fan speed >= temp_control1
        lv2poke32(payload_ctrl + 28ULL, speed_table[5]); // fan speed >= temp_control2
        lv2poke32(payload_ctrl + 32ULL, speed_table[6]); // fan speed >= temp_control3
        lv2poke32(payload_ctrl + 36ULL, speed_table[7]); // fan speed >= temp_control4

        lv2poke32(payload_ctrl + 40ULL, temp_control[0]); // temp_control0 (ºC)
        lv2poke32(payload_ctrl + 44ULL, temp_control[1]); // temp_control1 (ºC)
        lv2poke32(payload_ctrl + 48ULL, temp_control[2]); // temp_control2 (ºC)
        lv2poke32(payload_ctrl + 52ULL, temp_control[3]); // temp_control3 (ºC)
        lv2poke32(payload_ctrl + 56ULL, temp_control[4]); // temp_control4 (ºC)

        //lv2poke32(payload_ctrl + 4ULL, WITHOUT_LEDS); // enable with leds
        lv2poke32(payload_ctrl + 4ULL, WITH_LEDS); // enable with leds

        command =lv2peek(LV2_SM_CMD_ADDR)>>56ULL;
        if(command == 0x55ULL)
        {   // SM present!
            lv2poke(LV2_SM_CMD_ADDR, 0xAA01000000000000ULL); // enable SM Fan Control Mode 1
            while(true)
            {
                usleep(1000);
                command = lv2peek(LV2_SM_CMD_ADDR)>>48ULL;
                if(command == 0x55AAULL) break;
            }
        }
    }

    usleep(10000);  // waits
}

void set_device_wakeup_mode(u32 flags)
{
    u64 command;
    int mode = 0;

    if(!test_controlfan_compatibility()) return;

    if(flags == 0xFFFFFFFF) {flags = 9; mode = 1;} // use bdvd
    else
    {
        for(int n = 1; n < 10; n++) {if(flags & (1<<n)) {flags = n - 1; mode = 1; break;}} // test/use usb port x
    }

    if(!wakeup_time) mode = 0;

    if(!mode) flags = 0;

    command = lv2peek(LV2_SM_CMD_ADDR)>>56ULL;
    if(command == 0x55ULL)
    {   // SM present!
        if(!mode) lv2poke(LV2_SM_CMD_ADDR, 0xBB00000000000000ULL); // disable UsbWakeup
        else      lv2poke(LV2_SM_CMD_ADDR, 0xBB01000000000000ULL | (((u64) flags) << 32ULL)
                                                                 | (((u64) (wakeup_time - 1))<<40ULL)); // enable UsbWakeup Mode 2
        while(true)
        {
            usleep(1000);
            command =lv2peek(LV2_SM_CMD_ADDR)>>48ULL;
            if(command== 0x55BBULL) break;
        }
    }
}

void set_usleep_sm_main(u32 us)
{
    if(!test_controlfan_compatibility()) return;

    u64 command = lv2peek(LV2_SM_CMD_ADDR)>>56ULL;
    if(command == 0x55ULL)
    {   // SM present!
        lv2poke(LV2_SM_CMD_ADDR, 0xCCFE000000000000ULL | ((u64) us)); // set usleep time
        while(true)
        {
            usleep(1000);
            command =lv2peek(LV2_SM_CMD_ADDR)>>48ULL;
            if(command== 0x55CCULL) break;
        }
    }
}

void load_controlfan_config()
{
    if(is_ps3hen) return;

    int file_size = 0, n;
    sprintf(FANCONTROL_PATH, "%s/config/fancontrol.dat", self_path);
    if(file_exists(FANCONTROL_PATH) == false)
        sprintf(FANCONTROL_PATH, "/dev_hdd0/game/IRISMAN00/config/fancontrol.dat");

    char * mem = LoadFile((void *) (FANCONTROL_PATH), &file_size);

    n = sizeof(temp_control) + sizeof(speed_table) + sizeof(wakeup_time) + sizeof(fan_mode);
    if(!mem || n != file_size) {if(mem) free(mem); set_fan_mode(FANCTRL_PAYLOAD); return;}

    n = 0;
    memcpy((void *) temp_control, (void *) mem, sizeof(temp_control));
    n += sizeof(temp_control);
    memcpy((void *) speed_table, (void *) (mem + n), sizeof(speed_table));
    n += sizeof(speed_table);
    memcpy((void *) &wakeup_time, (void *) (mem + n), sizeof(wakeup_time));
    n += sizeof(wakeup_time);
    memcpy((void *) &fan_mode, (void *) (mem + n), sizeof(fan_mode));
    n += sizeof(fan_mode);

    if(mem) free(mem);

    if(fan_mode == FANCTRL_DISABLED)
    {
        if(get_controlfan_offsets())
        {
            if(lv2peek(PAYLOAD_BASE) || (lv2peek32(sys409_offset) != 0x38600001) || (lv2peek32(sys389_offset) != 0x38600001)) fan_mode = FANCTRL_PAYLOAD;
        }
    }

    if(get_vsh_plugin_slot_by_name("WWWD") > 0) fan_mode = FANCTRL_DISABLED;

    set_fan_mode(fan_mode);
}

void draw_controlfan_options()
{
    if(is_ps3hen) return;

    int n;

    float y2, x2;

    float x, y;
    int select_option = 0;

    int auto_l1 = 0, auto_r1 = 0;

    bool set_adjust = false;

    int sm_present = 0;

    if(!test_controlfan_compatibility()) return;

    if(lv2peek32(PAYLOAD_BASE) == 0x50534D45) sm_present = 1; // sm.self
    if(lv2peek32(PAYLOAD_BASE) == 0x50534D58) sm_present = 2; // sm.sprx

    memcpy((void *) temp_control_backup, (void *) temp_control, sizeof(temp_control));
    memcpy((void *) speed_table_backup, (void *) speed_table, sizeof(speed_table));
    wakeup_time_backup = wakeup_time;
    fan_mode_backup = fan_mode;

    u32 temp_cpu = 0;
    u32 temp_rsx = 0;

    u8 st, mode, speed, unknown;

    while(true)
    {
        flash = (frame_count >> 5) & 1;

        frame_count++;

        x = 28; y = 0;

        cls();

        update_twat(true);

        SetCurrentFont(FONT_TTF);

        // header title

        DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

        SetFontColor(WHITE, 0x00000000);

        SetFontSize(18, 20);

        SetFontAutoCenter(0);

        x2 = DrawFormatString(x, y - 0, " %s", "Control Fan & USB Wakeup") + 8;

        if(cur_fan_mode == FANCTRL_PAYLOAD)
        {
            if(sm_present == 1)
                DrawFormatString(x2, y - 0, "Current: #S. Manager");
            else if(sm_present == 2)
                DrawFormatString(x2, y - 0, "Current: #S. Manager SPRX");
            else
                DrawFormatString(x2, y - 0, "Current: #Payload");
        }
        else if(cur_fan_mode == FANCTRL_SYSCON)
            DrawFormatString(x2, y - 0, "Current: #SYSCON");
        else if(cur_fan_mode == FANCTRL_USER_VALUES)
            DrawFormatString(x2, y - 0, "Current: #By User");
        else if(cur_fan_mode == FANCTRL_DISABLED)
            DrawFormatString(x2, y - 0, "Current: #Disabled");

        SetFontSize(16, 20);

        y += 24;

        DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

        x2 = x;
        y2 = y + 8;

        x2 = DrawButton1_UTF8(x + 32, y2, 320, "Fan Control Mode >", (flash && select_option == 0)) + 8;

        if(sm_present == 1)
            x2 = DrawButton2_UTF8(x2, y2, 0, " S. Manager ", (fan_mode == FANCTRL_PAYLOAD)) + 6;
        else if(sm_present == 2)
            x2 = DrawButton2_UTF8(x2, y2, 0, " SM SPRX ", (fan_mode == FANCTRL_PAYLOAD)) + 6;
        else
            x2 = DrawButton2_UTF8(x2, y2, 0, " Payload ", (fan_mode == FANCTRL_PAYLOAD)) + 6;

        x2 = DrawButton2_UTF8(x2, y2, 0, " SYSCON ", (fan_mode  == FANCTRL_SYSCON)) + 6;
        x2 = DrawButton2_UTF8(x2, y2, 0, " By User ", (fan_mode == FANCTRL_USER_VALUES)) + 6;
        x2 = DrawButton2_UTF8(x2, y2, 0, " Disabled ", (fan_mode == FANCTRL_DISABLED)) + 6;

        y2 += 48;
        x2 = DrawButton1_UTF8(x + 32, y2, 212, "User/Shutdown Speed", (flash && select_option == 1)) + 8;
        sprintf(temp_buffer, " %02X ", speed_table[0]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        x2 = DrawButton1_UTF8(x + 360, y2, 212, "Temp1", (flash && select_option == 9)) + 8;
        sprintf(temp_buffer, " %2i ºC ", temp_control[0]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        y2 += 48;
        sprintf(temp_buffer, " < %2i ºC Speed ", temp_control[0]);
        x2 = DrawButton1_UTF8(x + 32, y2, 212, temp_buffer, (flash && select_option == 2)) + 8;
        sprintf(temp_buffer, " %02X ", speed_table[1]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        x2 = DrawButton1_UTF8(x + 360, y2, 212, "Temp2", (flash && select_option == 10)) + 8;
        sprintf(temp_buffer, " %2i ºC ", temp_control[1]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        y2 += 48;
        sprintf(temp_buffer, " >= %2i ºC Speed ", temp_control[0]);
        x2 = DrawButton1_UTF8(x + 32, y2, 212, temp_buffer, (flash && select_option == 3)) + 8;
        sprintf(temp_buffer, " %02X ", speed_table[2]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        x2 = DrawButton1_UTF8(x + 360, y2, 212, "Temp3", (flash && select_option == 11)) + 8;
        sprintf(temp_buffer, " %2i ºC ", temp_control[2]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        y2 += 48;
        sprintf(temp_buffer, " < %2i ºC Speed ", temp_control[1]);
        x2 = DrawButton1_UTF8(x + 32, y2, 212, temp_buffer, (flash && select_option == 4)) + 8;
        sprintf(temp_buffer, " %02X ", speed_table[3]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        x2 = DrawButton1_UTF8(x + 360, y2, 212, "Temp4", (flash && select_option == 12)) + 8;
        sprintf(temp_buffer, " %2i ºC ", temp_control[3]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        y2 += 48;
        sprintf(temp_buffer, " >= %2i ºC Speed ", temp_control[1]);
        x2 = DrawButton1_UTF8(x + 32, y2, 212, temp_buffer, (flash && select_option == 5)) + 8;
        sprintf(temp_buffer, " %02X ", speed_table[4]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        x2 = DrawButton1_UTF8(x + 360, y2, 212, "Temp5", (flash && select_option == 13)) + 8;
        sprintf(temp_buffer, " %2i ºC ", temp_control[4]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        y2 += 48;
        sprintf(temp_buffer, " >= %2i ºC Speed ", temp_control[2]);
        x2 = DrawButton1_UTF8(x + 32, y2, 212, temp_buffer, (flash && select_option == 6)) + 8;
        sprintf(temp_buffer, " %02X ", speed_table[5]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        x2 = DrawButton1_UTF8(x + 360, y2, 212, "USB Wakeup Time", (flash && select_option == 14)) + 8;
        if(!wakeup_time) sprintf(temp_buffer, " Disabled ");
        else sprintf(temp_buffer, " %i s ", wakeup_time * 10);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        y2 += 48;
        sprintf(temp_buffer, " >= %2i ºC Speed ", temp_control[3]);
        x2 = DrawButton1_UTF8(x + 32, y2, 212, temp_buffer, (flash && select_option == 7)) + 8;
        sprintf(temp_buffer, " %02X ", speed_table[6]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        x2 = DrawButton1_UTF8(x + 360, y2, 212, "Restore Default", (flash && select_option == 15)) + 8;

        SetFontColor(WHITE, 0x00000000);
        SetFontSize(16, 20);
        DrawFormatString(x2, y2 + 8, "Use L1/R1 or X/O");
        DrawFormatString(x2, y2 + 32, "to change values");
        DrawFormatString(x2, y2 + 56, "LEFT/RIGHT to change");
        DrawFormatString(x2, y2 + 80, "of column");

        y2 += 48;
        sprintf(temp_buffer, " >= %2i ºC Speed ", temp_control[4]);
        x2 = DrawButton1_UTF8(x + 32, y2, 212, temp_buffer, (flash && select_option == 8)) + 8;
        sprintf(temp_buffer, " %02X ", speed_table[7]);
        x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

        x2 = DrawButton1_UTF8(x + 360, y2, 212, "Save Settings", (flash && select_option == 16)) + 8;


        // display temp

        if((frame_count & 0x1f) == 0x0)
        {
            sys_game_get_temperature(0, &temp_cpu);
            sys_game_get_temperature(1, &temp_rsx);
        }

        n = sys_sm_get_fan_policy(0, &st, &mode, &speed, &unknown);
        if(n < 0) st = mode = speed = 0;

        SetCurrentFont(FONT_TTF);
        SetFontSize(20, 20);

        x2 = DrawFormatString(1024, 0, " Temp CPU: 99ºC RSX: 99ºC ");

        y2 = y + 3 * 150 - 4 + 12;

        SetFontColor(WHITE, BLUE);
        x2 = DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y2, " Temp CPU: ");

        if(temp_cpu < temp_control[5]) SetFontColor(YELLOW, BLUE); else SetFontColor(RED, BLUE);
        x2 = DrawFormatString(x2, y2, "%uºC",  temp_cpu);

        SetFontColor(WHITE, BLUE);
        x2 = DrawFormatString(x2, y2, " RSX: ");

        if(temp_rsx < 75) SetFontColor(YELLOW, BLUE); else SetFontColor(RED, BLUE);
        x2 = DrawFormatString(x2, y2, "%uºC ", temp_rsx);

        SetFontColor(YELLOW, BLUE);
        DrawFormatString(x, y2, "sys_sm_get_fan_policy: (mode: %X speed: 0x%X)", mode, speed);

        SetFontColor(WHITE, 0x00000000);



        tiny3d_Flip();

        ps3pad_read();

        AUTO_BUTTON_REP2(auto_l1, BUTTON_L1)
        AUTO_BUTTON_REP2(auto_r1, BUTTON_R1)

        if(new_pad & BUTTON_L1)
        {
            switch(select_option)
            {
                case 0:
                    set_adjust = true;
                    new_pad = BUTTON_LEFT;
                    break;

                case 1:
                    n = speed_table[0];
                    n--; if(n < 0x44) n = 0x44;
                    speed_table[0] = n;
                    set_adjust = true;
                    break;

                case 2:
                    n = speed_table[1];
                    n--; if(n < 0x33) n = 0x33;
                    speed_table[1] = n;
                    set_adjust = true;
                    break;

                case 3:
                    n = speed_table[2];
                    n--; if(n < speed_table[1]) n = speed_table[1]; //0x40
                    speed_table[2] = n;
                    set_adjust = true;
                    break;

                case 4:
                    n = speed_table[3];
                    n--; if(n < speed_table[2]) n = speed_table[2]; //0x44
                    speed_table[3] = n;
                    set_adjust = true;
                    break;

                case 5:
                    n = speed_table[4];
                    n--; if(n < speed_table[3]) n = speed_table[3]; //0x48
                    speed_table[4] = n;
                    set_adjust = true;
                    break;

                case 6:
                    n = speed_table[5];
                    n--; if(n < speed_table[4]) n = speed_table[4]; //0x5f
                    speed_table[5] = n;
                    set_adjust = true;
                    break;

                case 7:
                    n = speed_table[6];
                    n--; if(n < speed_table[5]) n = speed_table[5]; //0x70
                    speed_table[6] = n;
                    set_adjust = true;
                    break;

                case 8:
                    n = speed_table[7];
                    n--; if(n < speed_table[6]) n = speed_table[6]; //0x80
                    speed_table[7] = n;
                    set_adjust = true;
                    break;

                case 9:
                    n = temp_control[0];
                    n--; if(n < 40) n = 40;
                    temp_control[0] = n;
                    set_adjust = true;
                    break;

                case 10:
                    n = temp_control[1];
                    n--; if(n < temp_control[0]) n = temp_control[0]; //40
                    temp_control[1] = n;
                    set_adjust = true;
                    break;

                case 11:
                    n = temp_control[2];
                    n--; if(n < temp_control[1]) n = temp_control[1]; //40
                    temp_control[2] = n;
                    set_adjust = true;
                    break;

                case 12:
                    n = temp_control[3];
                    n--; if(n < temp_control[2]) n = temp_control[2]; //40;
                    temp_control[3] = n;
                    set_adjust = true;
                    break;

                case 13:
                    n = temp_control[4];
                    n--; if(n < temp_control[3]) n = temp_control[3]; //40;
                    temp_control[4] = n;
                    set_adjust = true;
                    break;

                case 14:
                    n = wakeup_time;
                    n--; if(n < 0) n = 0;
                    wakeup_time = n;
                    set_adjust = true;
                    break;

            }
            auto_l1 = 1;
        }

        if(new_pad & BUTTON_R1)
        {
            switch(select_option)
            {
                case 0:
                    set_adjust = true;
                    new_pad = BUTTON_RIGHT;
                    break;

                case 1:
                    n = speed_table[0];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[0] = n;
                    set_adjust = true;
                    break;

                case 2:
                    n = speed_table[1];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[1] = n;
                    set_adjust = true;
                    break;

                case 3:
                    n = speed_table[2];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[2] = n;
                    set_adjust = true;
                    break;

                case 4:
                    n = speed_table[3];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[3] = n;
                    set_adjust = true;
                    break;

                case 5:
                    n = speed_table[4];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[4] = n;
                    set_adjust = true;
                    break;

                case 6:
                    n = speed_table[5];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[5] = n;
                    set_adjust = true;
                    break;

                case 7:
                    n = speed_table[6];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[6] = n;
                    set_adjust = true;
                    break;

                case 8:
                    n = speed_table[7];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[7] = n;
                    set_adjust = true;
                    break;

                case 9:
                    n = temp_control[0];
                    n++; if(n > 74) n = 74;
                    temp_control[0] = n;
                    set_adjust = true;
                    break;

                case 10:
                    n = temp_control[1];
                    n++; if(n > 75) n = 75;
                    temp_control[1] = n;
                    set_adjust = true;
                    break;

                case 11:
                    n = temp_control[2];
                    n++; if(n > 76) n = 76;
                    temp_control[2] = n;
                    set_adjust = true;
                    break;

                case 12:
                    n = temp_control[3];
                    n++; if(n > 77) n = 77;
                    temp_control[3] = n;
                    set_adjust = true;
                    break;

                case 13:
                    n = temp_control[4];
                    n++; if(n > temp_control[5]) n = temp_control[5];
                    temp_control[4] = n;
                    set_adjust = true;
                    break;

                case 14:
                    n = wakeup_time;
                    n++; if(n > 0xff) n = 0xff;
                    wakeup_time = n;
                    set_adjust = true;
                    break;

            }
            auto_r1 = 1;
        }

        if(new_pad & (BUTTON_SQUARE))
        {
            switch(select_option)
            {
                case 0:
                    fan_mode = FANCTRL_DISABLED;
                    set_adjust = true;
                    break;
                case 14:
                    wakeup_time = 0;
                    set_adjust = true;
                    break;
            }
        }

        if(new_pad & (BUTTON_START))
        {
            select_option = 16;
            set_adjust = true;
            new_pad = BUTTON_CROSS_;
        }

        if(new_pad & (BUTTON_CROSS_))
        {
            switch(select_option)
            {
                case 0:
                    fan_mode++;if(fan_mode > 3) fan_mode = FANCTRL_PAYLOAD;
                    set_adjust = true;
                    break;

                case 1:
                    n = speed_table[0];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[0] = n;
                    set_adjust = true;
                    break;

                case 2:
                    n = speed_table[1];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[1] = n;
                    set_adjust = true;
                    break;

                case 3:
                    n = speed_table[2];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[2] = n;
                    set_adjust = true;
                    break;

                case 4:
                    n = speed_table[3];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[3] = n;
                    set_adjust = true;
                    break;

                case 5:
                    n = speed_table[4];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[4] = n;
                    set_adjust = true;
                    break;

                case 6:
                    n = speed_table[5];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[5] = n;
                    set_adjust = true;
                    break;

                case 7:
                    n = speed_table[6];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[6] = n;
                    set_adjust = true;
                    break;

                case 8:
                    n = speed_table[7];
                    n++; if(n > 0xff) n = 0xff;
                    speed_table[7] = n;
                    set_adjust = true;
                    break;

                case 9:
                    n = temp_control[0];
                    n++; if(n > 74) n = 74;
                    temp_control[0] = n;
                    set_adjust = true;
                    break;

                case 10:
                    n = temp_control[1];
                    n++; if(n > 75) n = 75;
                    temp_control[1] = n;
                    set_adjust = true;
                    break;

                case 11:
                    n = temp_control[2];
                    n++; if(n > 76) n = 76;
                    temp_control[2] = n;
                    set_adjust = true;
                    break;

                case 12:
                    n = temp_control[3];
                    n++; if(n > 77) n = 77;
                    temp_control[3] = n;
                    set_adjust = true;
                    break;

                case 13:
                    n = temp_control[4];
                    n++; if(n > temp_control[5]) n = temp_control[5];
                    temp_control[4] = n;
                    set_adjust = true;
                    break;

                case 14:
                    n = wakeup_time;
                    n++; if(n > 0xff) n = 0xff;
                    wakeup_time = n;
                    set_adjust = true;
                    break;

                case 15:
                    memcpy((void *) temp_control, (void *) temp_control_default, sizeof(temp_control));
                    memcpy((void *) speed_table, (void *) speed_table_default, sizeof(speed_table));
                    wakeup_time = 60;
                    fan_mode = FANCTRL_PAYLOAD;
                    set_adjust = true;
                    break;

                case 16:
                    if(set_adjust)
                    {
                        set_fan_mode(fan_mode); // adjust fan mode

                        // copy datas to the buffer
                        n = 0;
                        memcpy((void *) temp_buffer, (void *) temp_control, sizeof(temp_control));
                        n+= sizeof(temp_control);
                        memcpy((void *) (temp_buffer + n), (void *) speed_table, sizeof(speed_table));
                        n+= sizeof(speed_table);
                        memcpy((void *) (temp_buffer + n), (void *) &wakeup_time, sizeof(wakeup_time));
                        n+= sizeof(wakeup_time);
                        memcpy((void *) (temp_buffer + n), (void *) &fan_mode, sizeof(fan_mode));
                        n+= sizeof(fan_mode);

                        // update backup datas
                        memcpy((void *) temp_control_backup, (void *) temp_control, sizeof(temp_control));
                        memcpy((void *) speed_table_backup, (void *) speed_table, sizeof(speed_table));
                        wakeup_time_backup = wakeup_time;
                        fan_mode_backup = fan_mode;

                        // save datas
                        sprintf(FANCONTROL_PATH, "%s/config/fancontrol.dat", self_path);
                        SaveFile((void *) (FANCONTROL_PATH), (void *) temp_buffer, n);

                        set_adjust = false;
                    }
                    else
                        return;

                    break;

            }
        }

        // test integrity

        if(temp_control[0] > 74) temp_control[0] = 74;
        if(temp_control[0] >= temp_control[1]) temp_control[1] = temp_control[0] + 1;
        if(temp_control[1] > 75) temp_control[1] = 75;
        if(temp_control[1] >= temp_control[2]) temp_control[2] = temp_control[1] + 1;
        if(temp_control[2] > 76) temp_control[2] = 76;
        if(temp_control[2] >= temp_control[3]) temp_control[3] = temp_control[2] + 1;
        if(temp_control[3] > 77) temp_control[3] = 77;
        if(temp_control[3] >= temp_control[4]) temp_control[4] = temp_control[3] + 1;
        if(temp_control[4] > temp_control[5]) temp_control[4] = temp_control[5];

        if(speed_table[0] > 0xff) speed_table[0] = 0xff;
        for(n = 1; n < 7; n++)
        {
            if(speed_table[n] > 0xff) speed_table[n] = 0xff;
            if(speed_table[n] > speed_table[n + 1]) speed_table[n + 1] = speed_table[n] + 1;
            set_adjust = true;
        }

        if(speed_table[n] > 0xff) speed_table[n] = 0xff;


        if(new_pad & BUTTON_UP)
        {
            frame_count = 32;
            if(select_option == 9) select_option = 1;

            ROT_DEC(select_option, 0, 16);
        }
        else if(new_pad & BUTTON_DOWN)
        {
            frame_count = 32;

            ROT_INC(select_option, 16, 0);
        }
        else if(new_pad & BUTTON_LEFT)
        {
            frame_count = 32;

            if(select_option == 0)
            {
                fan_mode--; if(fan_mode < 0) fan_mode = FANCTRL_DISABLED;
                set_adjust = true;
            }
            else if(select_option >= 9)
                select_option -= 8;
            else
                select_option += 8;

            if(select_option > 16) select_option = 16;
        }
        else if(new_pad & BUTTON_RIGHT)
        {
            frame_count = 32;

            if(select_option == 0)
            {
                fan_mode++; if(fan_mode > 3) fan_mode = FANCTRL_PAYLOAD;
            }
            else if(select_option >= 9)
                select_option -= 8;
            else
                select_option += 8;

            if(select_option > 16) select_option = 16;
        }
        else if(new_pad & (BUTTON_TRIANGLE | BUTTON_CIRCLE_))
        {
            memcpy((void *) temp_control, (void *) temp_control_backup, sizeof(temp_control));
            memcpy((void *) speed_table, (void *) speed_table_backup, sizeof(speed_table));

            wakeup_time = wakeup_time_backup;
            fan_mode = fan_mode_backup;
            return;
        }
    }
}
