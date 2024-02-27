/*
 * Copyright (C) 2010 drizzt
 *
 * Authors:
 * drizzt <drizzt@ibeglab.org>
 * flukes1
 * kmeaw
 * D_Skywalk
 * Estwald
 * Rancid-o
 * Evilnat
 * Joonie
 * Aldostools
 * Smhabib
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
#include "payload_480.h"
#include "syscall8.h"

#include "payload_sky_480_bin.h"
#include "umount_480_bin.h"

#define CONFIG_USE_SYS8PERMH4 1

#undef SYSCALL_BASE
#undef NEW_POKE_SYSCALL
#undef NEW_POKE_SYSCALL_ADDR
#undef PAYLOAD_OFFSET

#define SYSCALL_BASE                    0x8000000000363BE0ULL // 4.80 CEX (same as 4.75-4.78)
#define NEW_POKE_SYSCALL                813
#define NEW_POKE_SYSCALL_ADDR           0x80000000002A1474ULL  // Syscall 813

#define PAYLOAD_OFFSET                  0x3d90
#define PERMS_OFFSET                    0x3560

#define PAYLOAD_UMOUNT_OFFSET           (0x3d90+0x400)
#define UMOUNT_SYSCALL_OFFSET           (0x29FB8C + 0x8) // SYSCALL (838) + 8 -> peek(SYSCALL_BASE + 838*8) -> peek(0x363BE0 + 0x1A30)

#define LV2MOUNTADDR_480 0x8000000000479FC0ULL
//0xff0 => 0x116c (458098 - 459204)
#define LV2MOUNTADDR_480_ESIZE 0x118
#define LV2MOUNTADDR_480_CSIZE 0x108

extern int noBDVD;
extern u64 restore_syscall8[2];

#define PATCH_JUMP(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc))
#define PATCH_CALL(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc) | 1)

static int lv2_unpatch_bdvdemu_480(void);
static int lv2_patch_bdvdemu_480(uint32_t flags);
static int lv2_patch_storage_480(void);
static int lv2_unpatch_storage_480(void);

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
    lv2syscall2(poke_syscall, addr, val);
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

int is_firm_480(void)
{
   // TOC 4.80
   u64 toc;
   toc = peekq(0x8000000000003000ULL);
   if(toc == 0x800000000034FBA0ULL)
   {
      return (peekq(0x80000000002FCB58ULL) == 0x323031362F30332FULL); //timestamp: 2016/03/
   }
   else
   {
      return 0;
   }
}

int is_firm_490(void)
{
   // TOC 4.90 (same as 4.80)
   u64 toc;
   toc = peekq(0x8000000000003000ULL);
   if(toc == 0x800000000034FBA0ULL)
   {
      return (peekq(0x80000000002FCB58ULL) == 0x323032322F31322FULL); //timestamp: 2022/12/
   }
   else
   {
      return 0;
   }
}

extern u64 syscall_base;

int is_payload_loaded_480(void)
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

void set_bdvdemu_480(int current_payload)
{
    lv2_unpatch_bdvdemu = lv2_unpatch_bdvdemu_480;
    lv2_patch_bdvdemu   = lv2_patch_bdvdemu_480;
    lv2_patch_storage   = lv2_patch_storage_480;
    lv2_unpatch_storage = lv2_unpatch_storage_480;
}

static inline void lv2_memcpy( u64 to, const u64 from, size_t sz)
{
    lv2syscall3(NEW_POKE_SYSCALL, to, from, sz);
}

static inline void lv2_memset( u64 dst, const u64 val, size_t sz)
{

    u64 *tmp = memalign(32, (sz*(sizeof(u64))) );
    if(!tmp)
        return;

    memset(tmp, val, sz);

    lv2syscall3(NEW_POKE_SYSCALL, dst, (u64) tmp, sz);

    free(tmp);
}

/*
-- 3.55   NEW_POKE_SYSCALL_ADDR
00195A68  F8 21 FF 01 7C 08 02 A6  FB C1 00 F0 FB E1 00 F8
00195A78  EB C2 FE 28 7C 7F 1B 78  38 60 03 2D FB A1 00 E8

-- 4.30   NEW_POKE_SYSCALL_ADDR
001B6950  F8 21 FF 01 7C 08 02 A6  FB C1 00 F0 FB E1 00 F8
001B6960  EB C2 FE 88 7C 7F 1B 78  38 60 03 2D FB A1 00 E8

-- 4.60   NEW_POKE_SYSCALL_ADDR
001A6F3C  F8 21 FF 01 7C 08 02 A6  FB C1 00 F0 FB E1 00 F8
001A6F4C  EB C2 FE 10 7C 7F 1B 78  38 60 03 2D FB A1 00 E8

-- 4.65   NEW_POKE_SYSCALL_ADDR
001A6F44  F8 21 FF 01 7C 08 02 A6  FB C1 00 F0 FB E1 00 F8
001A6F54  EB C2 FE 10 7C 7F 1B 78  38 60 03 2D FB A1 00 E8

-- 4.75   NEW_POKE_SYSCALL_ADDR
002A1480  F8 21 FF 01 7C 08 02 A6  FB C1 00 F0 FB E1 00 F8
002A1490  EB C2 25 B8 7C 7F 1B 78  38 60 03 2D FB A1 00 E8
*/

static inline void install_lv2_memcpy()
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 50; n++) {
    /* install memcpy */
    /* This does not work on some PS3s */
        pokeq(NEW_POKE_SYSCALL_ADDR + 0x00, 0x4800000428250000ULL); // Original: 0xF821FF017C0802A6ULL
        pokeq(NEW_POKE_SYSCALL_ADDR + 0x08, 0x4182001438a5ffffULL); // Original: 0xFBC100F0FBE100F8ULL
        pokeq(NEW_POKE_SYSCALL_ADDR + 0x10, 0x7cc428ae7cc329aeULL); // Original: 0xEBC225B87C7F1B78ULL
        pokeq(NEW_POKE_SYSCALL_ADDR + 0x18, 0x4bffffec4e800020ULL); // Original: 0x3860032DFBA100E8ULL
        usleep(5000);
    }
}

static inline void remove_lv2_memcpy()
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 50; n++) {
    /* restore syscall */
    //remove_new_poke();
// Found @ 0x2A1474 This is per FW, syscall_rmdir_desc // SC813
        pokeq(NEW_POKE_SYSCALL_ADDR + 0x00, 0xF821FF017C0802A6ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 0x08, 0xFBC100F0FBE100F8ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 0x10, 0xEBC225B87C7F1B78ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 0x18, 0x3860032DFBA100E8ULL);
        usleep(5000);
    }
}

static u64 lv1poke(u64 addr, u64 value)
{
    lv2syscall2(9, (u64) addr, (u64) value);
    return_to_user_prog(u64);
}

void load_payload_480(int mode)
{

//Remove Lv2 memory protection, NOT needed for REBUG 4.7x
    lv1poke(0x370F28 + 0x00, 0x0000000000000001ULL); // Original: 0x0000000000351FD8ULL
    lv1poke(0x370F28 + 0x08, 0xE0D251B556C59F05ULL); // Original: 0x3B5B965B020AE21AULL
    lv1poke(0x370F28 + 0x10, 0xC232FCAD552C80D7ULL); // Original: 0x7D6F60B118E2E81BULL
    lv1poke(0x370F28 + 0x18, 0x65140CD200000000ULL); // Original: 0x315D8B7700000000ULL

    install_lv2_memcpy();

    /* WARNING!! It supports only payload with a size multiple of 8 */
    lv2_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_OFFSET,
                   (u64) payload_sky_480_bin,
                   payload_sky_480_bin_size);

    lv2_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_UMOUNT_OFFSET, // copy umount routine
                      (u64) umount_480_bin,
                      umount_480_bin_size);

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

    remove_lv2_memcpy();

    pokeq(0x80000000007EF000ULL, 0ULL);// BD Emu mount
    pokeq(0x80000000007EF220ULL, 0ULL);

    //patches by deank for webMAN, I left them here just in case someone wants to play with, but basically the same thing with SYS36 patches below

			pokeq(0x8000000000267144ULL, 0x4E80002038600000ULL ); // fix 8001003C error  Original: 0x4E8000208003026CULL
			pokeq(0x800000000026714CULL, 0x7C6307B44E800020ULL ); // fix 8001003C error  Original: 0x3D201B433C608001ULL

    /*
			pokeq(0x800000000005688CULL, 0x63FF003D60000000ULL ); // fix 8001003D error  Original: 0x63FF003D419EFFD4ULL
			pokeq(0x800000000005664CULL, 0x3FE080013BE00000ULL ); // fix 8001003E error  Original: 0x3FE0800163FF003EULL

			pokeq(0x80000000000565F8ULL, 0x419E00D860000000ULL ); // Original: 0x419E00D8419D00C0ULL
			pokeq(0x8000000000056600ULL, 0x2F84000448000098ULL ); // Original: 0x2F840004409C0048ULL //PATCH_JUMP
			pokeq(0x800000000005A6DCULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL
			pokeq(0x800000000005A6F0ULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL
    */
			pokeq(0x800000000005622CULL, 0x386000012F830000ULL ); // ignore LIC.DAT check
			pokeq(0x80000000002275ECULL, 0x38600000F8690000ULL ); // fix 0x8001002B / 80010017 errors

			//pokeq(0x8000000000055C58ULL, 0xF821FE917C0802A6ULL ); // just restore the original
			//pokeq(0x8000000000058E18ULL, 0x419E0038E8610098ULL ); // just restore the original

    /* BASIC PATCHES SYS36 */
    // by 2 anonymous people
    _poke32(0x565FC, 0x60000000);             //
    PATCH_JUMP(0x56604, 0x5669C);             //
    _poke32(0x5A6E0,  0x60000000);            // fix 80010009 error
    _poke32(0x5A6F4,  0x60000000);            // fix 80010019 error
    _poke(  0x56588,  0x63FF003D60000000); // fix 8001003D error  "ori     %r31, %r31, 0x3D\n nop\n" done
    _poke32(0x56650,  0x3BE00000);            // fix 8001003E error -- 3.55 ok in 0x055F64 "li      %r31, 0"  done

    //Fix 0x8001003C error (incorrect version in sys_load_param) - It is present in the new game updates **/
    //_poke(0x267148, 0x386000007C6307B4); //
    //_poke32(0x267148 + 0x8, 0x4E800020);  //

    /*
        -002c3cf0  f8 01 00 b0 7c 9c 23 78  7c 7d 1b 78 4b d8 aa 1d  |....|.#x|}.xK...|
        +002c3cf0  f8 01 00 b0 7c 9c 23 78  4b d4 01 88 4b d8 aa 1d  |....|.#xK...K...| (openhook jump - 0x3E80)
    */

    PATCH_JUMP(0x297650, (PAYLOAD_OFFSET+0x30)); // patch openhook - done
    //_poke32(0x29762C, 0xF821FF61); // free openhook Rogero 4.30 (put "stdu    %sp, -0xA0(%sp)" instead   "b       sub_2E9F98")

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

static int lv2_patch_storage_480(void)
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
    save_lv2_storage_patch= peekq(0x80000000002ECAA0ULL);
    pokeq32(0x80000000002ECAA0ULL, 0x40000000); //FIXED

// LV1 Offsets
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

static int lv2_unpatch_storage_480(void)
{
    lv1_reg regs_i, regs_o;

    if(!is_patched) return -1;

    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 disable syscall storage
    pokeq(0x80000000002ECAA0ULL, save_lv2_storage_patch); //FIXED

// LV1 Offsets
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

static int lv2_unpatch_bdvdemu_480(void)
{
    int n;
    int flag = 0;

    char * mem = temp_buffer;
    memset(mem, 0, 0x10 * 0x118);

    sys8_memcpy((u64) mem, LV2MOUNTADDR_480, 0x10 * 0x118);
    sys8_memcpy((u64) (mem + 0x1200), 0x80000000007EF020ULL , LV2MOUNTADDR_480_CSIZE);

    for(n = 0; n < 0x116c; n+= LV2MOUNTADDR_480_ESIZE)
    {
        if(!memcmp(mem + n, "CELL_FS_UTILITY:HDD1", 21) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_480 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }
        }

        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_480 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }
        }
        if(!memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(!memcmp(mem + n + 0x69, "dev_bdvd", 9) || !memcmp(mem + n + 0x69, "temp_usb", 9))
            {
                sys8_memcpy(LV2MOUNTADDR_480 + n + 0x69, (u64) (mem + n + 0x79), 11);
                sys8_memset(LV2MOUNTADDR_480 + n + 0x79, 0ULL, 12);
                flag += 10;
            }
        }
        if(!memcmp(mem + n, "CELL_FS_UTILITY:HDD0", 21) && mem[n-9]== 1 && mem[n-13]== 1)
        {
           if(!memcmp(mem + n + 0x69, "dev_bdvd", 9)
              && !memcmp(mem + n + 0x79, "esp_bdvd", 9) && peekq(0x80000000007EF000ULL)!=0)
           {
                mem[0x1200+ 0x10 -1] = mem[n-1];
                sys8_memcpy(LV2MOUNTADDR_480 + (u64) (n - 0x10), (u64) (mem + 0x1200) , (u64) LV2MOUNTADDR_480_CSIZE);
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

    if((mem[0] == 0) && (flag == 0))
        return -1;
    else
        return flag;
}

static int lv2_patch_bdvdemu_480(uint32_t flags)
{
    int n;
    int flag =  0;
    int usb  = -1;
    int pos  = -1;
    int pos2 = -1;

    char * mem = temp_buffer;
    memset(mem, 0, 0x10 * 0x118);

    sys8_memcpy((u64) mem, LV2MOUNTADDR_480, 0x10 * 0x118);

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

    for(n = 0; n < 0x116c; n+= LV2MOUNTADDR_480_ESIZE)
    {
        if(noBDVD && !memcmp(mem + n, "CELL_FS_UTILITY:HDD1", 21)
            && !memcmp(mem + n + 0x69, "dev_bdvd", 9) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            if(pos2 < 0) pos2 = n;

            if(usb >= 0)
                sys8_memcpy(LV2MOUNTADDR_480 + n + 0x69, (u64) "temp_bdvd", 10);

            flag++;
        }
        else
        if(!noBDVD && !memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29)
            && (!memcmp(mem + n + 0x69, "dev_bdvd", 9)) && mem[n-9]== 1 && mem[n-13]== 1)
        {
            sys8_memcpy(LV2MOUNTADDR_480 + n + 0x69, (u64) "temp_bdvd", 10);
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
            sys8_memcpy(LV2MOUNTADDR_480 + n + 0x69, (u64) "dev_bdvd\0\0", 11);
            sys8_memcpy(LV2MOUNTADDR_480 + n + 0x79, (u64) &path_name[128], 11);

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

      memcpy(mem + 0x1220, mem + pos2 - 0x10, LV2MOUNTADDR_480_CSIZE);
      dat = LV2MOUNTADDR_480 + (u64) (pos2 - 0x10);
      memcpy(mem + 0x1200, &dat, 0x8);
      dat = 0x8000000000000000ULL + (u64)UMOUNT_SYSCALL_OFFSET;
      memcpy(mem + 0x1208, &dat, 0x8);
      n = (int) 0xFBA100E8; // UMOUNT RESTORE
      memcpy(mem + 0x1210, &n, 0x4);
      n = (int) LV2MOUNTADDR_480_CSIZE; // CDATAS
      memcpy(mem + 0x1214, &n, 0x4);

      memcpy(mem + pos2, mem + pos, LV2MOUNTADDR_480_CSIZE - 0x10);
      memcpy(mem + pos2 + 0x69, "dev_bdvd\0\0", 11);
      memcpy(mem + pos2 + 0x79, "esp_bdvd\0\0", 11);
      memset(mem + pos2 + 0xa4, 0, 8);

      sys8_memcpy(0x80000000007EF000ULL , ((u64) mem + 0x1200), LV2MOUNTADDR_480_CSIZE + 0x20);
      sys8_memcpy(LV2MOUNTADDR_480 + (u64) pos2, ((u64) (mem + pos2)), (u64) (LV2MOUNTADDR_480_CSIZE - 0x10));

      for(int k = 0; k < 100; k++)
      {
        PATCH_CALL(UMOUNT_SYSCALL_OFFSET, PAYLOAD_UMOUNT_OFFSET); // UMOUNT ROUTINE PATCH
        usleep(1000);
      }

      flag = 100;
    }

    if(flag < 11) return -1;

    return 0;
}
