/*
 * umount.S -- umount syscall 838 patch
 *
 * Copyright (C) (Estwald)
 * Copyright (C) (D_Skywalk)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 ********************************************************************************
 */

#ifdef CFW_341
#include "firmware_symbols_341.h"
#endif

#ifdef CFW_355
#include "firmware_symbols_355.h"
#endif

#ifdef CFW_355dex
#include "firmware_symbols_355dex.h"
#endif

#ifdef CFW_355deh
#include "firmware_symbols_355deh.h"
#endif

#ifdef CFW_421
#include "firmware_symbols_421.h"
#endif

#ifdef CFW_421dex
#include "firmware_symbols_421dex.h"
#endif

#ifdef CFW_430
#include "firmware_symbols_430.h"
#endif

#ifdef CFW_430dex
#include "firmware_symbols_430dex.h"
#endif

#ifdef CFW_431
#include "firmware_symbols_431.h"
#endif

#ifdef CFW_440
#include "firmware_symbols_440.h"
#endif

#ifdef CFW_441
#include "firmware_symbols_441.h"
#endif

#ifdef CFW_441dex
#include "firmware_symbols_441dex.h"
#endif

#ifdef CFW_446
#include "firmware_symbols_446.h"
#endif

#ifdef CFW_446dex
#include "firmware_symbols_446dex.h"
#endif

#ifdef CFW_450
#include "firmware_symbols_450.h"
#endif

#ifdef CFW_450dex
#include "firmware_symbols_450dex.h"
#endif

#ifdef CFW_453
#include "firmware_symbols_453.h"
#endif

#ifdef CFW_453dex
#include "firmware_symbols_453dex.h"
#endif

#ifdef CFW_455
#include "firmware_symbols_455.h"
#endif

#ifdef CFW_455dex
#include "firmware_symbols_455dex.h"
#endif

#ifdef CFW_460
#include "firmware_symbols_460.h"
#endif

#ifdef CFW_460dex
#include "firmware_symbols_460dex.h"
#endif

#ifdef CFW_460deh
#include "firmware_symbols_460deh.h"
#endif

#ifdef CFW_465
#include "firmware_symbols_465.h"
#endif

#ifdef CFW_465dex
#include "firmware_symbols_465dex.h"
#endif

#ifdef CFW_466
#include "firmware_symbols_466.h"
#endif

#ifdef CFW_470
#include "firmware_symbols_470.h"
#endif

#ifdef CFW_470dex
#include "firmware_symbols_470dex.h"
#endif

#ifdef CFW_475
#include "firmware_symbols_475.h"
#endif

#ifdef CFW_475dex
#include "firmware_symbols_475dex.h"
#endif

#ifdef CFW_475deh
#include "firmware_symbols_475deh.h"
#endif

#ifdef CFW_480
#include "firmware_symbols_480.h"
#endif

#ifdef CFW_480dex
#include "firmware_symbols_480dex.h"
#endif

#ifdef CFW_480deh
#include "firmware_symbols_480deh.h"
#endif

#ifdef CFW_481dex
#include "firmware_symbols_481dex.h"
#endif

#define ABS(target) ((target) - (BASE2) - .)

// Dynamic macros to load a label into a register
#define MEM_BASE(dest) \
    li dest, 1; \
    rldicr dest, dest, 63, 0;
#define LOAD_LABEL(base, dest, source, address) \
    oris    dest, source, ((base) + (address))@h; \
    ori    dest, dest, ((base) + (address))@l;
#define LOAD_LABEL2(dest, source, address) \
    LOAD_LABEL(BASE2, dest, source, address)
#define LOADI_LABEL2(dest, address) \
    LOAD_LABEL2(dest, dest, address)

#define PATHSTR_LEN    0x9

.org 0

umount_sys_patch: // 0x1B509C
    std      %r0, 0x110(%sp)
    std      %r3, 0xD0(%sp)
    std      %r4, 0xD8(%sp)
    std      %r5, 0xE0(%sp)
    std      %r29, 0xE8(%sp)
    std      %r31, 0xF8(%sp)

    MEM_BASE(%r31)
    LOAD_LABEL2(%r4, %r31, aDev_bdvd)

    oris   %r29, %r31, 0x7E    // 0x7EF000 -> backup of /dev_bdvd
    ori    %r29, %r29, 0xF000

    li      %r5, PATHSTR_LEN
    mflr    %r31
    bl      ABS(strncmp)
    cmpldi  %r3, 0
    bne     umount_normally
    ld      %r3, 0(%r29)
    cmpldi  %r3, 0
    beq     umount_normally
    li      %r0, 0
    std     %r0, 0(%r29)
    ld      %r4, 8(%r29)
    lwz     %r5, 16(%r29)
    stw     %r5, 0(%r4)
    addi    %r4, %r29, 0x20
    lwz     %r5, 20(%r29)
    bl       ABS(memcpy)

umount_normally:
    mtlr    %r31
    ld      %r0, 0x110(%sp)
    ld      %r3, 0xD0(%sp)
    ld      %r4, 0xD8(%sp)
    ld      %r5, 0xE0(%sp)
    ld      %r29, 0xE8(%sp)
    ld      %r31, 0xF8(%sp)
    sync
    blr

aDev_bdvd:    .string    "/dev_bdvd"

.org 160
