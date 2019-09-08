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
#include "payload_355.h"
#include "hvcall.h"
#include "mm.h"
#include "syscall8.h"

#include "payload_sky_355_bin.h"
#include "umount_355_bin.h"

#define CONFIG_USE_SYS8PERMH4 1

#undef SYSCALL_BASE
#define SYSCALL_BASE                    0x8000000000346570ULL
#define PAYLOAD_OFFSET                  0xEF48
#define PERMS_OFFSET                    0x0e7f0
#define UMOUNT_SYSCALL_OFFSET           (0x193344+0x8) // SYSCALL (838)

#define PAYLOAD_UMOUNT_OFFSET           (0xEF48+0x400)
#define LV2MOUNTADDR_355 0x80000000003F64C4ULL
#define LV2MOUNTADDR_355_ESIZE 0x100
#define LV2MOUNTADDR_355_CSIZE 0xF0
//(LV2MOUNTADDR_355_ESIZE - 0x14 + 4 +4) //0xEC

extern int noBDVD;
extern u64 restore_syscall8[2];

#define PATCH_JUMP(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc))
#define PATCH_CALL(add_orig, add_dest) _poke32(add_orig, 0x48000000 | ((add_dest-add_orig) & 0x3fffffc) | 1)

static int lv2_unpatch_bdvdemu_355(void);
static int lv2_patch_bdvdemu_355(uint32_t flags);
static int lv2_unpatch_bdvdemu_null(void);
static int lv2_patch_bdvdemu_null(uint32_t flags);
static int lv2_patch_storage_355(void);
static int lv2_unpatch_storage_355(void);


u64 mmap_lpar_addr;
static int poke_syscall = 7;

extern char path_name[MAXPATHLEN];
extern char temp_buffer[8192];

u64 peekq(u64 addr)
{
    lv2syscall1(6, addr);
    return_to_user_prog(u64);
}


void pokeq(u64 addr, u64 val)
{
    lv2syscall2(poke_syscall, addr, val);
}

void pokeq32(u64 addr, uint32_t val)
{
    uint32_t next = peekq(addr) & 0xffffffff;
    pokeq(addr, (((u64) val) << 32) | next);
}

void lv1_poke(u64 address, u64 value)
{
    lv2syscall2(7, HV_BASE + address, value);
}

static inline void _poke(u64 addr, u64 val)
{
    pokeq(0x8000000000000000ULL + addr, val);
}

static inline void _poke32(u64 addr, uint32_t val)
{
    pokeq32(0x8000000000000000ULL + addr, val);
}

int is_firm_355(void)
{
    u64 addr = peekq((SYSCALL_BASE + NEW_POKE_SYSCALL * 8));
    // check address first
    if(addr < 0x8000000000000000ULL || addr > 0x80000000007FFFFFULL || (addr & 3)!=0)
        return 0;

    addr = peekq(addr);

    if(addr == NEW_POKE_SYSCALL_ADDR) return 1;

    return 0;
}

extern u64 syscall_base;

int is_payload_loaded_355(void)
{

    //1st classic syscall 36 check

    syscall_base = SYSCALL_BASE;

    u64 addr = peekq(0x80000000000004f0ULL);

    if((addr>>32) == 0x534B3145) { // new method to detect the payload
        addr&= 0xffffffff;
        if(addr) {
            restore_syscall8[0]= SYSCALL_BASE + (u64) (SYSCALL_SK1E * 8ULL); // (8*8)
            restore_syscall8[1]= peekq(restore_syscall8[0]);
            pokeq(restore_syscall8[0], 0x8000000000000000ULL + (u64) (addr + 0x20));
        }

        return SKY10_PAYLOAD;
    }


    if(peekq(0x80000000002be4a0ULL) == 0x2564257325303136ULL)
        return SYS36_PAYLOAD;

    //2nd new syscall 36 - sky mod check
    if(peekq(0x800000000000ef58ULL) == 0x534B313000000000ULL){ //SK10 HEADER

        return SKY10_PAYLOAD;
    }

    //WaninV2 CFW
    if(peekq(0x8000000000079d80ULL) == 0x3880000090830000ULL) //WaninV2
        return WANIN_PAYLOAD;

    return ZERO_PAYLOAD;

}

void set_bdvdemu_355(int current_payload)
{
    if((current_payload == SKY10_PAYLOAD)||(current_payload == ZERO_PAYLOAD)) {
        lv2_unpatch_bdvdemu = lv2_unpatch_bdvdemu_355;
        lv2_patch_bdvdemu   = lv2_patch_bdvdemu_355;
        lv2_patch_storage   = lv2_patch_storage_355;
        lv2_unpatch_storage = lv2_unpatch_storage_355;
    } else {
        lv2_unpatch_bdvdemu = lv2_unpatch_bdvdemu_null;
        lv2_patch_bdvdemu   = lv2_patch_bdvdemu_null;
        // storage null not need it here?
    }
}

inline static void lv2_memcpy( u64 to, const u64 from, size_t sz)
{
    lv2syscall3(NEW_POKE_SYSCALL, to, from, sz);
}

inline static void lv2_memset( u64 dst, const u64 val, size_t sz)
{

    u64 *tmp = memalign(32, (sz*(sizeof(u64))) );
    if(!tmp)
        return;

    memset(tmp, val, sz);

    lv2syscall3(NEW_POKE_SYSCALL, dst, (u64) tmp, sz);

    free(tmp);
}

void install_lv2_memcpy()
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 80; n++) {
    /* install memcpy */
    /* This does not work on some PS3s */
        pokeq(NEW_POKE_SYSCALL_ADDR, 0x4800000428250000ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4182001438a5ffffULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0x7cc428ae7cc329aeULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x4bffffec4e800020ULL);
        peekq(0x8000000000000570ULL);
        usleep(5000);
    }
}

void remove_lv2_memcpy()
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 80; n++) {
    /* restore syscall */
        pokeq(NEW_POKE_SYSCALL_ADDR, 0xF821FF017C0802A6ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0xFBC100F0FBE100F8ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0xebc2fe287c7f1b78ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x3860032dfba100e8ULL);
        peekq(0x8000000000000570ULL);
        usleep(5000);
    }
}

void install_payload_exploit(void)
{
    int n;
    for(n = 0; n < 80; n++) {
        /* install jump to exploit */
        pokeq(NEW_POKE_SYSCALL_ADDR, 0x7C6903A64E800420ULL); //mtctr   %r3 // bctr /* jump to exploit addr */
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4E8000204E800020ULL); // blr //blr /* maybe not need it */
        peekq(0x8000000000000570ULL);
        usleep(5000);
    }
    poke_syscall = NEW_POKE_SYSCALL;
}

inline static int lv2_call_payload(u64 addr)
{
    lv2syscall3(NEW_POKE_SYSCALL, (u64) addr, 0,0); /* call new syscall and jump to exploit */
    return_to_user_prog(int);
}


void remove_payload_exploit(void)
{
    /* restore syscall */
    remove_new_poke();
}


void load_payload_355(int mode)
{

    install_lv2_memcpy();
    /* WARNING!! It supports only payload with a size multiple of 8 */
    lv2_memcpy(0x800000000000ef48ULL,
                   (u64) payload_sky_355_bin,
                   payload_sky_355_bin_size);

    lv2_memcpy(0x8000000000000000ULL + (u64) PAYLOAD_UMOUNT_OFFSET, // copy umount routine
                      (u64) umount_355_bin,
                      umount_355_bin_size);

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


    pokeq(0x80000000007EF000ULL, 0ULL);// BE Emu mount
    pokeq(0x80000000007EF220ULL, 0ULL);

    /* BASIC PATCHES SYS36 */
    // by 2 anonymous people
    _poke32(0x55f14, 0x60000000);
    _poke32(0x55f1c, 0x48000098);
    _poke32(0x7af68, 0x60000000);
    _poke32(0x7af7c, 0x60000000);
    _poke(0x55EA0, 0x63FF003D60000000);  // fix 8001003D error
    _poke(0x55F64, 0x3FE080013BE00000);  // fix 8001003E error

    /*
        -002b3290  f8 01 00 b0 7c 9c 23 78  7c 7d 1b 78 4b d9 b4 11  |....|.#x|}.xK...|
        +002b3290  f8 01 00 b0 7c 9c 23 78  4b d5 bf 40 4b d9 b4 11  |....|.#xK..@K...| (openhook jump - 0xF1D8)
    */
    //_poke(0x2b3298, 0x4bd5bda04bd9b411ULL); //jump hook

    PATCH_JUMP(0x2b3298, (PAYLOAD_OFFSET+0x30));

    /**  Rancid-o: Fix 0x8001003C error (incorrect version in sys_load_param) - It is present in the new game updates **/
    _poke(0x28A404, 0x386000007C6307B4);
    _poke32(0x28A40C, 0x4E800020);


#ifdef CONFIG_USE_SYS8PERMH4
    PATCH_JUMP(PERMS_OFFSET, (PAYLOAD_OFFSET+0x18));
#endif

}

int map_lv1(void)
{
    int result = lv1_undocumented_function_114(0, 0xC, HV_SIZE, &mmap_lpar_addr);
    if (result != 0) {
        return 0;
    }

    result = mm_map_lpar_memory_region(mmap_lpar_addr, HV_BASE, HV_SIZE, 0xC, 0);
    if (result) {
        return 0;
    }

    return 1;
}

void unmap_lv1(void)
{
    if (mmap_lpar_addr != 0)
        lv1_undocumented_function_115(mmap_lpar_addr);
}

void patch_lv2_protection(void)
{
    int n;
    for(n = 0; n < 20; n++) {
        // changes protected area of lv2 to first byte only
        lv1_poke(0x363a78, 0x0000000000000001ULL);
        lv1_poke(0x363a80, 0xe0d251b556c59f05ULL);
        lv1_poke(0x363a88, 0xc232fcad552c80d7ULL);
        lv1_poke(0x363a90, 0x65140cd200000000ULL);
    usleep(5000);
    }
}

void install_new_poke(void)
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 80; n++) {
        // install poke with icbi instruction
        pokeq(NEW_POKE_SYSCALL_ADDR, 0xF88300007C001FACULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4C00012C4E800020ULL);
        peekq(0x8000000000000570ULL);
        usleep(5000);
    }

    poke_syscall = NEW_POKE_SYSCALL;
}

void remove_new_poke(void)
{
    int n;

    poke_syscall = 7;

    for(n = 0; n < 80; n++) {
        pokeq(NEW_POKE_SYSCALL_ADDR, 0xF821FF017C0802A6ULL);
        pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0xFBC100F0FBE100F8ULL);
        peekq(0x8000000000000570ULL);
        usleep(5000);
    }
}



/******************************************************************************************************************************************************/
/* STORAGE FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

static int is_patched = 0;

static u64 save_lv2_storage_patch;
static u64 save_lv1_storage_patches[4];

static void pokeq7(u64 addr, u64 val)
{
    lv2syscall2(7, addr, val);
}


static int lv2_patch_storage_355(void)
{
    install_new_poke();
    if (!map_lv1()) {
        remove_new_poke();
        return -1;
    }

    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 enable syscall storage
    save_lv2_storage_patch= peekq(0x80000000002D7820ULL);
    save_lv1_storage_patches[0] = peekq(HV_BASE + 0x16f3b8);
    save_lv1_storage_patches[1] = peekq(HV_BASE + 0x16f3dc);
    save_lv1_storage_patches[2] = peekq(HV_BASE + 0x16f454);
    save_lv1_storage_patches[3] = peekq(HV_BASE + 0x16f45c);

    int n;
    for(n = 0; n < 20; n++) {
        pokeq32(0x80000000002D7820ULL, 0x40000000);
        pokeq7(HV_BASE + 0x16f3b8, 0x7f83e37860000000ULL);
        pokeq7(HV_BASE + 0x16f3dc, 0x7f85e37838600001ULL);
        pokeq7(HV_BASE + 0x16f454, 0x7f84e3783be00001ULL);
        pokeq7(HV_BASE + 0x16f45c, 0x9be1007038600000ULL);
        usleep(5000);
    }

    remove_new_poke(); /* restore pokes */

    unmap_lv1();
    is_patched = 1;

    return 0;
}

static int lv2_unpatch_storage_355(void)
{
    if(!is_patched) return -1;

    install_new_poke();
    if (!map_lv1()) {
        remove_new_poke();
        return -2;
    }

    //search bin "5F 6F 66 5F 70 72 6F 64  75 63 74 5F 6D 6F 64 65" to find
    // LV2 disable syscall storage

    int n;
    for(n = 0; n < 20; n++) {
        pokeq(0x80000000002D7820ULL, save_lv2_storage_patch);

        pokeq7(HV_BASE + 0x16f3b8, save_lv1_storage_patches[0]);
        pokeq7(HV_BASE + 0x16f3dc, save_lv1_storage_patches[1]);
        pokeq7(HV_BASE + 0x16f454, save_lv1_storage_patches[2]);
        pokeq7(HV_BASE + 0x16f45c, save_lv1_storage_patches[3]);
        usleep(5000);
    }

    remove_new_poke(); /* restore pokes */

    unmap_lv1();

    return 0;
}


/******************************************************************************************************************************************************/
/* BDVDEMU FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

static int lv2_unpatch_bdvdemu_355(void)
{
    int n;
    int flag = 0;

    char * mem = temp_buffer;
    memset(mem, 0, 0x10 * 0x100);

    sys8_memcpy((u64) mem, LV2MOUNTADDR_355, 0x10 * 0x100);
    sys8_memcpy((u64) (mem + 0x1200), 0x80000000007EF020ULL , LV2MOUNTADDR_355_CSIZE);

    for(n = 0; n < 0xff0; n+= LV2MOUNTADDR_355_ESIZE)
    {
        if(!memcmp(mem + n, "CELL_FS_UTILITY:HDD1", 21) && mem[n-9]== 1 && mem[n-5]== 1)
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }
        }

        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29) && mem[n-9]== 1 && mem[n-5]== 1)
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }
        }
        if(!memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29) && mem[n-9]== 1 && mem[n-5]== 1)
        {
            if(!memcmp(mem + n + 0x69, "dev_bdvd", 9) || !memcmp(mem + n + 0x69, "temp_usb", 9))
            {
                sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) (mem + n + 0x79), 11);
                sys8_memset(LV2MOUNTADDR_355 + n + 0x79, 0ULL, 12);
                flag += 10;
           }
        }
        if(!memcmp(mem + n, "CELL_FS_UTILITY:HDD0", 21) && mem[n-9]== 1 && mem[n-5]== 1)
        {
           if(!memcmp(mem + n + 0x69, "dev_bdvd", 9)
              && !memcmp(mem + n + 0x79, "esp_bdvd", 9) && peekq(0x80000000007EF000ULL)!=0)
           {
                mem[0x1200+ 0xc -1] = mem[n-1];
                sys8_memcpy(LV2MOUNTADDR_355 + (u64) (n - 0xc), (u64) (mem + 0x1200) , (u64) LV2MOUNTADDR_355_CSIZE);
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

static int lv2_patch_bdvdemu_355(uint32_t flags)
{
    int n;
    int flag =  0;
    int usb  = -1;
    int pos  = -1;
    int pos2 = -1;

    char * mem = temp_buffer;
    memset(mem, 0, 0x10 * 0x100);

    sys8_memcpy((u64) mem, LV2MOUNTADDR_355, 0x10 * 0x100);

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

    for(n = 0; n < 0xff0; n+= LV2MOUNTADDR_355_ESIZE)
    {
        if(noBDVD && !memcmp(mem + n, "CELL_FS_UTILITY:HDD1", 21)
            && !memcmp(mem + n + 0x69, "dev_bdvd", 9) && mem[n-9]== 1 && mem[n-5]== 1)
        {
            if(pos2 < 0) pos2 = n;

            if(usb >= 0)
                sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) "temp_bdvd", 10);

            flag++;
        }
        else
        if(!noBDVD && !memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29)
            && (!memcmp(mem + n + 0x69, "dev_bdvd", 9)) && mem[n-9]== 1 && mem[n-5]== 1)
        {
            sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) "temp_bdvd", 10);
            flag++;
        }
        else
        if(!noBDVD && usb < 0 && !memcmp(mem + n, "CELL_FS_IOS:BDVD_DRIVE", 29)
            && !memcmp(mem + n + 0x69, "dev_ps2disc", 12) && mem[n-9]== 1 && mem[n-5]== 1)
        {
            if(pos2 < 0) pos2 = n;

            flag++;
        }
        else if(usb >= 0 && !memcmp(mem + n, path_name, 32) && mem[n-9]== 1 && mem[n-5]== 1)
        {
            if(noBDVD) pos = -1;
            sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) "dev_bdvd\0\0", 11);
            sys8_memcpy(LV2MOUNTADDR_355 + n + 0x79, (u64) &path_name[128], 11);

            flag += 10;
        }
        else if(usb < 0 && !memcmp(mem + n, "CELL_FS_UTILITY:HDD0", 21)
                && !memcmp(mem + n + 0x48, "CELL_FS_UFS", 11)
                && !memcmp(mem + n + 0x69, "dev_hdd0", 9) && mem[n-9] == 1 && mem[n-5] == 1)
        {
            if(pos < 0) pos = n;
        }
    }

    if(pos > 0 && pos2 > 0)
    {
      u64 dat;

      memcpy(mem + 0x1220, mem + pos2 - 0xc, LV2MOUNTADDR_355_CSIZE);
      dat = LV2MOUNTADDR_355 + (u64) (pos2 - 0xc);
      memcpy(mem + 0x1200, &dat, 0x8);
      dat = 0x8000000000000000ULL + (u64)UMOUNT_SYSCALL_OFFSET;
      memcpy(mem + 0x1208, &dat, 0x8);
      n = (int) 0xFBA100E8; // UMOUNT RESTORE
      memcpy(mem + 0x1210, &n, 0x4);
      n = (int) LV2MOUNTADDR_355_CSIZE; // CDATAS
      memcpy(mem + 0x1214, &n, 0x4);

      memcpy(mem + pos2, mem + pos, LV2MOUNTADDR_355_CSIZE - 0xc);
      memcpy(mem + pos2 + 0x69, "dev_bdvd\0\0", 11);
      memcpy(mem + pos2 + 0x79, "esp_bdvd\0\0", 11);
      memset(mem + pos2 + 0xa4, 0, 8);

      sys8_memcpy(0x80000000007EF000ULL , ((u64) mem + 0x1200), LV2MOUNTADDR_355_CSIZE + 0x20);
      sys8_memcpy(LV2MOUNTADDR_355 + (u64) pos2, ((u64) (mem + pos2)), (u64) (LV2MOUNTADDR_355_CSIZE - 0xc));

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

static int lv2_unpatch_bdvdemu_null(void) { return 0; }
static int lv2_patch_bdvdemu_null(uint32_t flags) { return 0; }
