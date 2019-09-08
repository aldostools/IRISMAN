/*
 * Copyright (C) 2010 drizzt
 *
 * Authors:
 * drizzt <drizzt@ibeglab.org>
 * flukes1
 * kmeaw
 * D_Skywalk
 * Estwald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ppu-lv2.h>
#include "payload.h"
#include "payload_440.h"
#include "syscall8.h"

#include "payload_sky_440_bin.h"
#include "umount_440_bin.h"

#define CONFIG_USE_SYS8PERMH4 1

#undef SYSCALL_BASE
#undef NEW_POKE_SYSCALL
#undef NEW_POKE_SYSCALL_ADDR
#undef PAYLOAD_OFFSET

#define SYSCALL_BASE                    0x800000000035E260ULL
#define NEW_POKE_SYSCALL                813
#define NEW_POKE_SYSCALL_ADDR           0x80000000001B5934ULL   // where above syscall is in lv2
#define PAYLOAD_OFFSET                  0x3d90
#define PERMS_OFFSET                    0x3560

#define PAYLOAD_UMOUNT_OFFSET           (0x3d90+0x400)
#define UMOUNT_SYSCALL_OFFSET           (0x1B404C +0x8) // SYSCALL (838)
#define LV2MOUNTADDR_440 0x8000000000458098ULL //?
//0xff0 => 0x116c (458098 - 459204)
#define LV2MOUNTADDR_440_ESIZE 0x118
#define LV2MOUNTADDR_440_CSIZE 0x108

extern int noBDVD;
extern u64 restore_syscall8[2];

#define PATCH_JUMP(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc))
#define PATCH_CALL(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc) | 1)

static int lv2_unpatch_bdvdemu_440(void);
static int lv2_patch_bdvdemu_440(uint32_t flags);
static int lv2_patch_storage_440(void);
static int lv2_unpatch_storage_440(void);

static int poke_syscall = 7;

extern char path_name[MAXPATHLEN];
extern char temp_buffer[8192];

static u64 peekq(u64 addr)
{
    lv2syscall1(6, addr);
    return_to_user_prog(u64);
}


static void pokeq(u64 addr, u64 val)
{
    if(!poke_syscall) {
        sys8_pokeinstr(addr, val);
    }
    else {
        lv2syscall2(poke_syscall, addr, val);
    }
}

static void pokeq32(u64 addr, uint32_t val)
{
    uint32_t next = peekq(addr) & 0xffffffff;
    pokeq(addr, (((u64) val) << 32) | next);
}

static inline void _poke(u64 addr, u64 val)
{
    pokeq(0x8000000000000000ULL + addr, val);
}

static inline void _poke32(u64 addr, uint32_t val)
{
    pokeq32(0x8000000000000000ULL + addr, val);
}

int is_firm_440(void)
{
// 4.40 cex
   u64 dex2;
   dex2 =peekq(0x80000000003487D0ULL);
   if(dex2 == 0x80000000003004C8ULL && peekq( 0x800000000000C448ULL ) != 0x614A10007F84E378ULL )
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

extern u64 syscall_base;

int is_payload_loaded_440(void)
{
    u64 addr = peekq(0x80000000000004f0ULL);
    syscall_base = SYSCALL_BASE;

    if((addr>>32) == 0x534B3145) { // new method to detect the payload
        addr&= 0xffffffff;
        if(addr) {
            restore_syscall8[0]= SYSCALL_BASE + (u64) (SYSCALL_SK1E * 8ULL); // (8*8)
            restore_syscall8[1]= peekq(restore_syscall8[0]);
            pokeq(restore_syscall8[0], 0x8000000000000000ULL + (u64) (addr + 0x20));
        }

        return SKY10_PAYLOAD;
    }

    addr = peekq((SYSCALL_BASE + SYSCALL_36 * 8));
    addr = peekq(addr);
    if(peekq(addr - 0x20) == 0x534B313000000000ULL) //SK10 HEADER
        return SKY10_PAYLOAD;

    return ZERO_PAYLOAD;
}

void set_bdvdemu_440(int current_payload)
{
    lv2_unpatch_bdvdemu = lv2_unpatch_bdvdemu_440;
    lv2_patch_bdvdemu   = lv2_patch_bdvdemu_440;
    lv2_patch_storage   = lv2_patch_storage_440;
    lv2_unpatch_storage = lv2_unpatch_storage_440;
}

static inline void lv2_memcpy( u64 to, const u64 from, size_t sz)
{
    lv2syscall3(9/*NEW_POKE_SYSCALL*/, to, from, sz);
}


static u64 restore_syscall;

static inline void install_lv2_memcpy()
{
    int n;

    restore_syscall = peekq(SYSCALL_BASE + (u64) (9 * 8));

    for(n = 0; n < 50; n++) {
        pokeq(0x8000000000001820ULL, 0x8000000000001830ULL);
        pokeq(0x8000000000001828ULL, peekq(0x8000000000003000ULL));
        pokeq(0x8000000000001830ULL, 0x282500004D820020ULL);
        pokeq(0x8000000000001838ULL, 0x38A5FFFF7CC428AEULL);
        pokeq(0x8000000000001840ULL, 0x7CC329AE7C0006ACULL);
        pokeq(0x8000000000001848ULL, 0x7CE32A1470E80003ULL);
        pokeq(0x8000000000001850ULL, 0x282500004082000CULL);
        pokeq(0x8000000000001858ULL, 0x7CE838504800000CULL);
        pokeq(0x8000000000001860ULL, 0x282800004082FFCCULL);
        pokeq(0x8000000000001868ULL, 0x7C0038AC7C0004ACULL);
        pokeq(0x8000000000001870ULL, 0x7C003FAC4C00012CULL);
        pokeq(0x8000000000001878ULL, 0x4BFFFFB800000000ULL);
        _poke((u32) (SYSCALL_BASE + 9 * 8), 0x8000000000001820ULL);
        usleep(5000);
    }

}

static inline void remove_lv2_memcpy()
{
    int n;

    for(n = 0; n < 50; n++) {
        pokeq(0x8000000000001820ULL, 0x0ULL);
        pokeq(0x8000000000001828ULL, 0x0ULL);
        pokeq(0x8000000000001830ULL, 0x0ULL);
        pokeq(0x8000000000001838ULL, 0x0ULL);
        pokeq(0x8000000000001840ULL, 0x0ULL);
        pokeq(0x8000000000001848ULL, 0x0ULL);
        pokeq(0x8000000000001850ULL, 0x0ULL);
        pokeq(0x8000000000001858ULL, 0x0ULL);
        pokeq(0x8000000000001860ULL, 0x0ULL);
        pokeq(0x8000000000001868ULL, 0x0ULL);
        pokeq(0x8000000000001870ULL, 0x0ULL);
        pokeq(0x8000000000001878ULL, 0x0ULL);
        _poke((u32) (SYSCALL_BASE + 9 * 8), restore_syscall);
        usleep(5000);
    }
}


u8 lv1_peek_poke_call_routines440[136] = {
    0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x01, 0x00, 0x10, 0x39, 0x60, 0x00, 0xB6, 0x44, 0x00, 0x00, 0x22,
    0x7C, 0x83, 0x23, 0x78, 0xE8, 0x01, 0x00, 0x10, 0x7C, 0x08, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20,
    0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x01, 0x00, 0x10, 0x39, 0x60, 0x00, 0xB7, 0x44, 0x00, 0x00, 0x22,
    0x38, 0x60, 0x00, 0x00, 0xE8, 0x01, 0x00, 0x10, 0x7C, 0x08, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20,
    0x7C, 0x08, 0x02, 0xA6, 0xF8, 0x01, 0x00, 0x10, 0x7D, 0x4B, 0x53, 0x78, 0x44, 0x00, 0x00, 0x22,
    0xE8, 0x01, 0x00, 0x10, 0x7C, 0x08, 0x03, 0xA6, 0x4E, 0x80, 0x00, 0x20, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x17, 0x0C, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x14, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x17, 0x1C, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x3C, 0x80, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x17, 0x5C, 0x00, 0x00, 0x00, 0x00
};


void load_payload_440(int mode)
{

    install_lv2_memcpy();
    /* install lv1 peek/poke/call */
    lv2_memcpy(0x800000000000171C,
                   (u64) lv1_peek_poke_call_routines440,
                   sizeof(lv1_peek_poke_call_routines440));

    /* WARNING!! It supports only payload with a size multiple of 8 */
    lv2_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_OFFSET,
                   (u64) payload_sky_440_bin,
                   payload_sky_440_bin_size);

    lv2_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_UMOUNT_OFFSET, // copy umount routine
                      (u64) umount_440_bin,
                      umount_440_bin_size);

    restore_syscall8[0]= SYSCALL_BASE + (u64) (SYSCALL_SK1E * 8ULL); // (8*8)
    restore_syscall8[1]= peekq(restore_syscall8[0]);

    u64 id[2];
    // copy the id
    id[0]= 0x534B314500000000ULL | (u64) PAYLOAD_OFFSET;
    id[1] = SYSCALL_BASE + (u64) (SYSCALL_SK1E * 8ULL); // (8*8)
    lv2_memcpy(0x80000000000004f0ULL, (u64) &id[0], 16);

    u64 inst8 =  peekq(0x8000000000003000ULL);                     // get TOC
    lv2_memcpy(0x8000000000000000ULL + (u64) (PAYLOAD_OFFSET + 0x28), (u64) &inst8, 8);
    inst8 = 0x8000000000000000ULL + (u64) (PAYLOAD_OFFSET + 0x20); // syscall_8_desc - sys8
    lv2_memcpy(SYSCALL_BASE + (u64) (SYSCALL_SK1E * 8ULL), (u64) &inst8, 8);

    usleep(1000);

    poke_syscall = 0; // uses sys8_pokeinst

    remove_lv2_memcpy();

    pokeq(0x80000000007EF000ULL, 0ULL); // BE Emu mount
    pokeq(0x80000000007EF220ULL, 0ULL);

    //Patches from webMAN
    pokeq(0x8000000000296DE8ULL, 0x4E80002038600000ULL );
    pokeq(0x8000000000296DF0ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
    pokeq(0x80000000000560BCULL, 0x63FF003D60000000ULL ); // fix 8001003D error
    pokeq(0x8000000000056180ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error +C4

    pokeq(0x800000000005612CULL, 0x419E00D860000000ULL );
    pokeq(0x8000000000056134ULL, 0x2F84000448000098ULL );
    pokeq(0x8000000000059AF0ULL, 0x2F83000060000000ULL );
    pokeq(0x8000000000059B04ULL, 0x2F83000060000000ULL );


    _poke((u32) (SYSCALL_BASE + 9 * 8) ,      0x8000000000001790ULL);
    _poke((u32) (SYSCALL_BASE + 10 * 8),      0x8000000000001798ULL);

    /* BASIC PATCHES SYS36 */
    // by 2 anonymous people

    _poke32(0x056130, 0x60000000); // portado
    PATCH_JUMP(0x056138, 0x561D0); // portado
    _poke32(0x059AF4, 0x60000000); // portado
    _poke32(0x059B08, 0x60000000); // portado

    _poke(  0x0560BC, 0x63FF003D60000000);  // fix 8001003D error  "ori     %r31, %r31, 0x3D\n nop\n" *HECHO
    _poke32(0x05614C, 0x3BE00000);  // 0F
    _poke32(0x056184, 0x3BE00000);  // 3E
    _poke32(0x055FD8, 0x3BE00000);  // 02
    _poke32(0x056208, 0x3BE00000);  // 04
    _poke32(0x0561FC, 0x3BE00000);  // 09
    //PATCH_JUMP(0x056188, 0x56094);          // fix E3 4.30 added error

    _poke(0x296DEC, 0x386000007C6307B4);
    _poke32(0x296DF4, 0x4E800020);

    /*
        -002c3cf0  f8 01 00 b0 7c 9c 23 78  7c 7d 1b 78 4b d8 aa 1d  |....|.#x|}.xK...|
        +002c3cf0  f8 01 00 b0 7c 9c 23 78  4b d4 01 88 4b d8 aa 1d  |....|.#xK...K...| (openhook jump - 0x3E80)
    */
    //0x7C7D1B78
   PATCH_JUMP(0x2C42A8, (PAYLOAD_OFFSET+0x30)); // patch openhook
   _poke(0x2C428C, 0xFB810080FBA10088ULL); // free openhook Rogero 4.40 :-/


   // deleted _poke((u32) (SYSCALL_BASE + SYSCALL_36 * 8), 0x8000000000000000ULL + (u64) (PAYLOAD_OFFSET + 0xC8)); // syscall_map_open_desc - sys36


#ifdef CONFIG_USE_SYS8PERMH4
    PATCH_JUMP(PERMS_OFFSET, (PAYLOAD_OFFSET+0x18));
#endif


}


/******************************************************************************************************************************************************/
/* STORAGE FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

static int is_patched = 0;

static u64 save_lv2_storage_patch;
static u64 save_lv1_storage_patches[4];

static int lv2_patch_storage_440(void)
{
    lv1_reg regs_i, regs_o;

    // test if LV1 Peek is supported

    memset(&regs_i, 0, sizeof(regs_i));

    regs_i.reg11 = 0xB6;
    sys8_lv1_syscall(&regs_i, &regs_o);

    if(((int) regs_o.reg3) <0) {
        return -1;
    }

    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 enable syscall storage
    save_lv2_storage_patch= peekq(0x80000000002E9798ULL);
    pokeq32(0x80000000002E9798ULL, 0x40000000);

    regs_i.reg3 = 0x16fa60; regs_i.reg4 = 0x7f83e37860000000ULL;
    regs_i.reg11 = 0xB6;
    sys8_lv1_syscall(&regs_i, &regs_o); save_lv1_storage_patches[0]= regs_o.reg4;
    regs_i.reg11 = 0xB7; sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fa84; regs_i.reg4 = 0x7f85e37838600001ULL;
    regs_i.reg11 = 0xB6;
    sys8_lv1_syscall(&regs_i, &regs_o); save_lv1_storage_patches[1]= regs_o.reg4;
    regs_i.reg11 = 0xB7; sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fafc; regs_i.reg4 = 0x7f84e3783be00001ULL;
    regs_i.reg11 = 0xB6;
    sys8_lv1_syscall(&regs_i, &regs_o); save_lv1_storage_patches[2]= regs_o.reg4;
    regs_i.reg11 = 0xB7; sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fb04; regs_i.reg4 = 0x9be1007038600000ULL;
    regs_i.reg11 = 0xB6;
    sys8_lv1_syscall(&regs_i, &regs_o); save_lv1_storage_patches[3]= regs_o.reg4;
    regs_i.reg11 = 0xB7; sys8_lv1_syscall(&regs_i, &regs_o);

    is_patched = 1;

    return 0;
}

static int lv2_unpatch_storage_440(void)
{
    lv1_reg regs_i, regs_o;

    if(!is_patched) return -1;

    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 disable syscall storage
    pokeq(0x80000000002E9798ULL, save_lv2_storage_patch);

    regs_i.reg11 = 0xB7;

    regs_i.reg3 = 0x16fa60; regs_i.reg4 = save_lv1_storage_patches[0];
    sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fa84; regs_i.reg4 = save_lv1_storage_patches[1];
    sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fafc; regs_i.reg4 = save_lv1_storage_patches[2];
    sys8_lv1_syscall(&regs_i, &regs_o);

    regs_i.reg3 = 0x16fb04; regs_i.reg4 = save_lv1_storage_patches[3];
    sys8_lv1_syscall(&regs_i, &regs_o);

    return 0;
}


/******************************************************************************************************************************************************/
/* BDVDEMU FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

static int lv2_unpatch_bdvdemu_440(void)
{
    int n;
    int flag = 0;

    char * mem = temp_buffer;
    memset(mem, 0, 0x10 * 0x118);

    poke_syscall = 7;

    sys8_memcpy((u64) mem, LV2MOUNTADDR_440, 0x10 * 0x118);
    sys8_memcpy((u64) (mem + 0x1200), 0x80000000007EF020ULL , LV2MOUNTADDR_440_CSIZE);

    for(n = 0; n < 0x116c; n+= LV2MOUNTADDR_440_ESIZE)
    {
        if(!memcmp(mem + n, "CELL_FS_UTILITY:HDD1", 21) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_440 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }
        }

        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_440 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }
        }
        if(!memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(!memcmp(mem + n + 0x69, "dev_bdvd", 9) || !memcmp(mem + n + 0x69, "temp_usb", 9))
            {
                sys8_memcpy(LV2MOUNTADDR_440 + n + 0x69, (u64) (mem + n + 0x79), 11);
                sys8_memset(LV2MOUNTADDR_440 + n + 0x79, 0ULL, 12);
                flag += 10;
            }
        }
        if(!memcmp(mem + n, "CELL_FS_UTILITY:HDD0", 21) && mem[n-9]== 1 && mem[n-13]== 1)
        {
           if(!memcmp(mem + n + 0x69, "dev_bdvd", 9)
              && !memcmp(mem + n + 0x79, "esp_bdvd", 9) && peekq(0x80000000007EF000ULL)!=0)
           {
                mem[0x1200+ 0x10 -1] = mem[n-1];
                sys8_memcpy(LV2MOUNTADDR_440 + (u64) (n - 0x10), (u64) (mem + 0x1200) , (u64) LV2MOUNTADDR_440_CSIZE);
                flag += 10;
           }
        }
    }

    for(n = 0; n < 100; n++)
    {
        _poke32(UMOUNT_SYSCALL_OFFSET, 0xFBA100E8); // UMOUNT RESTORE
        usleep(1000);
    }


    pokeq(0x80000000007EF000ULL, 0ULL);

    poke_syscall = 0;

    if((mem[0] == 0) && (flag == 0))
        return -1;
    else
        return flag;
}

static int lv2_patch_bdvdemu_440(uint32_t flags)
{
    int n;
    int flag =  0;
    int usb  = -1;
    int pos  = -1;
    int pos2 = -1;

    char * mem = temp_buffer;
    memset(mem, 0, 0x10 * 0x118);

    poke_syscall = 7;

    sys8_memcpy((u64) mem, LV2MOUNTADDR_440, 0x10 * 0x118);

    for(n = 0; n < 11; n++)
    {
        if(flags == (1 << n))
        {
            usb = n - 1;
            break;
        }
    }

    if(usb >= 0)
    {
        sprintf(path_name, "CELL_FS_IOS:USB_MASS_STORAGE00%c", 48 + usb);
        sprintf(&path_name[128], "dev_usb00%c", 48 + usb);
    }

    for(n = 0; n < 0x116c; n+= LV2MOUNTADDR_440_ESIZE)
    {
        if(noBDVD && !memcmp(mem + n, "CELL_FS_UTILITY:HDD1", 21)
            && !memcmp(mem + n + 0x69, "dev_bdvd", 9) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(pos2 < 0) pos2 = n;

            if(usb >= 0)
                sys8_memcpy(LV2MOUNTADDR_440 + n + 0x69, (u64) "temp_bdvd", 10);

            flag++;
        }
        else
        if(!noBDVD && !memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29)
            && (!memcmp(mem + n + 0x69, "dev_bdvd", 9)) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            sys8_memcpy(LV2MOUNTADDR_440 + n + 0x69, (u64) "temp_bdvd", 10);
            flag++;
        }
        else
        if(!noBDVD && usb < 0 && !memcmp(mem + n, "CELL_FS_IOS:BDVD_DRIVE", 29)
            && !memcmp(mem + n + 0x69, "dev_ps2disc", 12) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(pos2 < 0) pos2 = n;

            flag++;
        }
        else if(usb >= 0 && !memcmp(mem + n, path_name, 32) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(noBDVD) pos = -1;
            sys8_memcpy(LV2MOUNTADDR_440 + n + 0x69, (u64) "dev_bdvd\0\0", 11);
            sys8_memcpy(LV2MOUNTADDR_440 + n + 0x79, (u64) &path_name[128], 11);

            flag += 10;
        }
        else if(usb < 0 && !memcmp(mem + n, "CELL_FS_UTILITY:HDD0", 21)
                && !memcmp(mem + n + 0x48, "CELL_FS_UFS", 11)
                && !memcmp(mem + n + 0x69, "dev_hdd0", 9) && mem[n-9] == 1 && mem[n-13] == 1)
        {
            if(pos < 0) pos = n;
        }
    }

    if(pos > 0 && pos2 > 0)
    {
      u64 dat;

      memcpy(mem + 0x1220, mem + pos2 - 0x10, LV2MOUNTADDR_440_CSIZE);
      dat = LV2MOUNTADDR_440 + (u64) (pos2 - 0x10);
      memcpy(mem + 0x1200, &dat, 0x8);
      dat = 0x8000000000000000ULL + (u64)UMOUNT_SYSCALL_OFFSET;
      memcpy(mem + 0x1208, &dat, 0x8);
      n = (int) 0xFBA100E8; // UMOUNT RESTORE
      memcpy(mem + 0x1210, &n, 0x4);
      n = (int) LV2MOUNTADDR_440_CSIZE; // CDATAS
      memcpy(mem + 0x1214, &n, 0x4);

      memcpy(mem + pos2, mem + pos, LV2MOUNTADDR_440_CSIZE - 0x10);
      memcpy(mem + pos2 + 0x69, "dev_bdvd\0\0", 11);
      memcpy(mem + pos2 + 0x79, "esp_bdvd\0\0", 11);
      memset(mem + pos2 + 0xa4, 0, 8);

      sys8_memcpy(0x80000000007EF000ULL , ((u64) mem + 0x1200), LV2MOUNTADDR_440_CSIZE + 0x20);
      sys8_memcpy(LV2MOUNTADDR_440 + (u64) pos2, ((u64) (mem + pos2)), (u64) (LV2MOUNTADDR_440_CSIZE - 0x10));

      for(int k = 0; k < 100; k++)
      {
        PATCH_CALL(UMOUNT_SYSCALL_OFFSET, PAYLOAD_UMOUNT_OFFSET); // UMOUNT ROUTINE PATCH
        usleep(1000);
      }

      flag = 100;
    }

    poke_syscall = 0;

    if(flag < 11) return -1;

    return 0;
}
