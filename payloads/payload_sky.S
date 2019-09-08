/*
 * payload_sky.S -- Adds system call 36 and 8 code on custom firmware
 *                  It contains hermes, pl3 and psgroove parts of code
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Aaron Lindsay (Aaron')
 * Copyright (C) (subdub)
 * Copyright (C) (Hermes)
 * Copyright (C) Miguel Boton (Waninkoko)
 * Copyright (C) (D_Skywalk)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 * This payload is a modified version of the original PSJailbreak's payload.
 * The people behing PSJailbrak are the original authors and copyright holders
 * of the code they wrote.
 *
 *******************************************************************************
 *
 * Note some developers with close source projects that does not respect the GPL
 *
 * If you benefit from this code, then you SHOULD contribute back.
 * And it is very clear how I expect you to contribute back - by providing full
 * access to the COMPLETE system which is using my code. At the core of GPL, the
 * intention is that someone else should be able to make modifications to such a
 * system and build other things from it.
 *
 * No source? Syscall8 and new hook is not welcomed in your disturbing projects.
 * I am not interested in any line from your source, but scene play together...
 *
 * - D_Skywalk, syscall8 + hook_table for 3.55 developer -
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

#define SYS8_VERSION 0x308
#define PERM_MODE 0  // 0 -> restricted 1-> original 2-> new for some games

#define ABS(target) ((target) - (BASE) - .)

// Dynamic macros to load a label into a register
#define MEM_BASE(dest) \
    li dest, 1; \
    rldicr dest, dest, 63, 0;
#define LOAD_LABEL(base, dest, source, address) \
    oris    dest, source, ((base) + (address))@h; \
    ori    dest, dest, ((base) + (address))@l;
#define LOAD_LABEL2(dest, source, address) \
    LOAD_LABEL(BASE, dest, source, address)
#define LOADI_LABEL2(dest, address) \
    LOAD_LABEL2(dest, dest, address)

.org 0
destination_code:
return1_h4:
    li      %r3, 1
    blr
return0_h4:
    li      %r3, 0
    blr
.org 0x10
    .byte 'S', 'K', '1','E'
.org 0x18
    b       perm_routine
    .long end_data
.org 0x20
syscall_8_desc:
    .long   0x80000000
    .long   BASE + syscall8
    .quad   0

.org 0x30

/*********************************************************************/

open_hook:
    //overwriten by the jump

    lbz	    %r4, 0(%r3)
	cmpwi	%r4, '/'
	bne	    end_test_bar

 test_bar:
	lbzu	%r4, 1(%r3)
	cmpwi	%r4, '/'
	beq	    test_bar
	subi    %r3, %r3, 1

 end_test_bar:

    mr      %r29, %r3

    MEM_BASE(%r31)

    LOAD_LABEL2(%r27, %r31, str_open_cmp)
    ld      %r26, 0(%r27)
    cmpldi  %r26, 0 // if open_table is null go to optimized hook sky
    beq     return_org_open_func

open_table_loop:
// hermes loop with love :D
    mr      %r3, %r29 //restore ptr? (loop)
    ld      %r4, 0(%r26)  // get compare path (%r27 open_table and %r26 contains the 1st ptr)
    cmpldi  %r4, 0 // check null ptr
    beq     return_org_open_func // null? then we finished...

    lwz     %r5, 16(%r26) // get len of compare path
    bl      ABS(strncmp)

    cmpldi  %r3, 0 // if ptr is null
    bne     open_table_nextvalue

    lwz     %r5, 16(%r26)
    add     %r4, %r29, %r5 // &open_path[compare_path_len]

    ld      %r3, 8(%r26) // get new path
    mr      %r29, %r3    // changes open_path
    lwz     %r5, 20(%r26) // get len of new path

    add     %r3, %r3, %r5 // prepare for copy

    bl      ABS(strcpy)
    b       return_org_open_func

open_table_nextvalue:
    addi    %r26, %r26, 24 // next ptr?
    b       open_table_loop

/*********************************************************************/

return_org_open_func:
//start new code
    mr      %r3, %r29 // overwrite
    b       ABS(memory_patch_func)



/*********************************************************************/

perm_routine:
    MEM_BASE(%r9)
    LOADI_LABEL2(%r9, perm_mode)
    lwz     %r0, 0(%r9)
    cmplwi  %r0, 1
    beq     return1_h4
    ld      %r9, perm_var_offset(%rtoc)
    mflr    %r0
    b       ABS(perm_patch_func + 8)

/*********************************************************************/
    .align 3
syscall8_table:
    .long   0x80000000
    .long   BASE + sys8_disable       //0 - not work yet
    .long   0x80000000
    .long   BASE + sys8_enable        //1 - not work yet
    .long   0x80000000
    .long   BASE + sys8_memcpy        //2
    .long   0x80000000
    .long   BASE + sys8_memset        //3
    .long   0x80000000
    .long   BASE + sys8_call          //4
    .long   0x80000000
    .long   BASE + sys8_alloc         //5
    .long   0x80000000
    .long   BASE + sys8_free          //6
    .long   0x80000000
    .long   BASE + sys8_stealth       //7 - sys8_panic
    .long   0x80000000
    .long   BASE + sys8_perm_mode     //8 - sys8_perm_mode
    .long   0x80000000
    .long   BASE + sys8_open_table    //9
    .long   0x80000000
    .long   BASE + syscall8_unsupported   //10 - sys8_configure ?
    .long   0x80000000
    .long   BASE + sys8_lv1call       //11 - sys8_lv1call
    .long   0x80000000
    .long   BASE + sys8_instcpy       //12 - sys8_instcpy
    .long   0x80000000
    .long   BASE + sys8_pokeinst      //13 - sys8_pokeinst
str_open_cmp:
    .quad 0

    .align 2

syscall8:
    mflr    %r0
    stdu    %sp, -0xA0(%sp)
    std     %r27, 0x78(%sp)
    std     %r28, 0x80(%sp)
    std     %r29, 0x88(%sp)
    std     %r30, 0x90(%sp)
    std     %r31, 0x98(%sp)
    std     %r0, 0xB0(%sp)

// hermes stealth code no supported yet
// syscall switch
    andi.   %r3, %r3, 0xff
    cmpldi  %r3, 14
    bge     syscall8_stealth //org unsupported

    MEM_BASE(%r31)
    LOAD_LABEL2(%r27, %r31, syscall8_table)
    sldi    %r28, %r3, 3
    add     %r27, %r27, %r28

    ld      %r29, 0(%r27)

// move param registers
    mr      %r3, %r4
    mr      %r4, %r5
    mr      %r5, %r6

// jump to register address with link
    bl      syscall8_bl

syscall8_end:
    ld      %r27, 0x78(%sp)
    ld      %r28, 0x80(%sp)
    ld      %r29, 0x88(%sp)
    ld      %r30, 0x90(%sp)
    ld      %r31, 0x98(%sp)
    ld      %r0, 0xB0(%sp)
    addi    %sp, %sp, 0xA0
    mtlr    %r0
    blr

syscall8_stealth:
    lis     %r3, -0x7FFF
    ori     %r3, %r3, 3
    b       syscall8_end

syscall8_unsupported:
    li     %r3, -1
    b      syscall8_end

syscall8_bl:
    mtctr   %r29
    bctr

sys8_disable: // system_call_2(8, 0, key)
    // hermes stealth code no ported yet
    b       syscall8_stealth

sys8_enable: // system_call_2(8, 1, key) when syscalls are enabled
    // hermes stealth code no supported yet
    li      %r3, SYS8_VERSION
    blr

sys8_memcpy: // system_call_4(8, 2, dest, src, len)
    b       ABS(memcpy)

sys8_memset: // system_call_4(8, 3, dest, dat, len)
    b       ABS(memset)

sys8_call: // system_call_4(8, 4, addr, param1, param2)
    mtctr   %r3
    mr      %r3, %r4
    mr      %r4, %r5
    bctr

sys8_alloc: // system_call_3(8, 5, size, pool)
    b       ABS(alloc)

sys8_free: // system_call_3(8, 6, ptr, pool)
    b       ABS(free)


sys8_perm_mode: // system_call_2(8, 8, perm)
    LOAD_LABEL2(%r27, %r31, perm_mode)
    stw     %r3, 0(%r27)
    li      %r3, 0
    blr

sys8_open_table:  // system_call_2(8, 9, ptr)
    mr      %r4, %r3
    LOAD_LABEL2(%r27, %r31, str_open_cmp)
    ld      %r3, 0(%r27)
    std     %r4, 0(%r27)
    blr

sys8_lv1call:  // system_call_2(8, 11, input_reg_table, output_reg_table)
    mflr    %r0
    stdu    %sp, -0xA0(%sp)
    std     %r0, 0xB0(%sp)

    std     %r4, 0x60(%sp)

    ld      %r4, 0x08(%r3)
    ld      %r5, 0x10(%r3)
    ld      %r6, 0x18(%r3)
    ld      %r7, 0x20(%r3)
    ld      %r8, 0x28(%r3)
    ld      %r9, 0x30(%r3)
    ld      %r10,0x38(%r3)
    ld      %r11,0x40(%r3)
    ld      %r3, 0x00(%r3)

    sc      1

    ld      %r11, 0x60(%sp)

    std     %r3, 0x00(%r11)
    std     %r4, 0x08(%r11)
    std     %r5, 0x10(%r11)
    std     %r6, 0x18(%r11)
    std     %r7, 0x20(%r11)
    std     %r8, 0x28(%r11)
    std     %r9, 0x30(%r11)
    std     %r10,0x38(%r11)

    ld      %r0, 0xB0(%sp)
    addi    %sp, %sp, 0xA0
    mtlr    %r0
    blr


sys8_instcpy:
    addi    %r5, %r5, 0x3
    rldicr  %r5, %r5, 0,61
    cmpldi  %r5, 0
    beqlr

    li      %r6, 0
sys8_instcpy_1:

    lwzx    %r7, %r4, %r6
    stwx    %r7, %r3, %r6
    addi    %r6, %r6, 0x4
    cmpld   %r6, %r5
    bne      sys8_instcpy_1

    li      %r6, 0
sys8_instcpy_2:

    dcbf   %r6, %r4
    addi    %r6, %r6, 0x4
    cmpld   %r6, %r5
    bne     sys8_instcpy_2
    sync    0
    li      %r6, 0
sys8_instcpy_3:

    icbi    %r6, %r4
    addi    %r6, %r6, 0x4
    cmpld   %r6, %r5
    bne     sys8_instcpy_3
    isync
    blr

sys8_pokeinst:
    std     %r4, 0(%r3)

    dcbf   %r0, %r3
    sync    0

    icbi    %r0, %r3
    isync
    blr

perm_mode:
    .long PERM_MODE

end_data:
.org 0x338
//.org 0x500

