 /*
 * ps3_storage.S       
 *
 * Copyright (C) (Estwald)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 ********************************************************************************
 */

#define STACK  0x100
#define VAR_R3 0x80
#define VAR_R4 0x88
#define VAR_R5 0x90
#define VAR_R6 0x98
#define VAR_R7 0xA0
#define VAR_R8 0xA8
#define VAR_R9 0xB0
#define VAR_0  0xB8
#define VAR_1  0xC0
#define VAR_RET 0xC8
#define VAR_R26 0xD0
#define VAR_R27 0xD8
#define VAR_R28 0xE0
#define VAR_R29 0xE8
#define VAR_R30 0xF0
#define VAR_R31 0xF8
#define VAR_R0  0x110

// Dynamic macros to load a label into a register
#define MEM_LV2_BASE(dest) \
    li dest, 1; \
    rldicr dest, dest, 63, 0;
#define LOAD_LABEL(dest, source, address) \
    addis    dest, source, ((address))@h; \
    addi    dest, dest, ((address))@l;
#define LOAD_LABEL2(dest, source, address) \
    LOAD_LABEL(dest, source, address)
#define LOADI_LABEL2(dest, address) \
    LOAD_LABEL2(dest, dest, address)

.org 0

/*********************************************************************************************************/
/* ID, SYSCALL BASE AND SYSCALL TABLES                                                                   */
/*********************************************************************************************************/

_start:
    .quad 0x444953434C455353    // ID     + 0x0
  
syscall_base:                   //        + 0x8
    .quad 0x8000000000000000   

    .quad sys40  //  + 0x10
syscall_40:
    .quad 0x0000000000000000
    
    .quad sys130  // + 0x20
syscall_130:
    .quad 0x0000000000000000

    .quad sys879  // + 0x30
syscall_879:
    .quad 0x0000000000000000

    .quad sys864  // + 0x40
syscall_864:
    .quad 0x0000000000000000

    .quad sys619  // + 0x50
syscall_619:
    .quad 0x0000000000000000 

    .quad sys837  // + 0x60
syscall_837:
    .quad 0x0000000000000000

    .quad sys609  // + 0x70
syscall_609:
    .quad 0x0000000000000000


/*********************************************************************************************************/
/* UTILS                                                                                                 */
/*********************************************************************************************************/

reubicador:
	mflr    %r0
    bl      1f
1:
	mflr    %r31
	addi    %r31, %r31, (_start - 1b)
    mtlr    %r0
    blr

panic:
    li      %r3, 0
    li      %r11, 0xff
    sc 1

syscall:
    stdu    %sp, -0x0090(%sp)
    std     %r31, 0x0088(%sp)
    std     %r2, 0x0080(%sp)
    mflr    %r0
    std     %r0, 0x00A0(%sp)
    bl      reubicador
    ld      %r31, syscall_base(%r31)
    sldi    %r11, %r11, 3
    add     %r31, %r31, %r11
    li      %r2, 1
    rldicr  %r2, %r2, 63, 0
    ld      %r2, (0x3000)@l(%r2)
    ld      %r31, 0(%r31)
    ld      %r0, 0(%r31)
    mtctr   %r0
    bctrl
    ld      %r31, 0x0088(%sp)
    ld      %r2, 0x0080(%sp)
    ld      %r0, 0x00A0(%sp)
    mtlr    %r0
    addi    %sp, %sp, 0x90
    blr

lv2memcpy:
    addi    %r3, %r3, -1
    addi    %r4, %r4, -1
    mtctr   %r5

1:
    lbzu    %r0, 1(%r4)
    stbu    %r0, 1(%r3)
    bdnz    1b
    blr

lv2memcmp:
    addi    %r3, %r3, -1
    addi    %r4, %r4, -1
    mtctr   %r5

1:
    lbzu    %r11, 1(%r4)
    lbzu    %r12, 1(%r3)
    cmplw   %r11, %r12
    bne     ret_1
    bdnz    1b

ret_0:
    li      %r3, 0
    blr

ret_1:
    li      %r3, 1
    blr

lv2strcmp:
    li      %r5, 1024
lv2strncmp:
    addi    %r3, %r3, -1
    addi    %r4, %r4, -1
    mtctr   %r5
1:
    lbzu    %r11, 1(%r4)
    lbzu    %r12, 1(%r3)
    cmplw   %r11, %r12
    bne     ret_1
    cmplwi  %r11, 0
    beq     ret_0
    bdnz    1b


/*********************************************************************************************************/
/* SYSCALL 40: Management                                                                                */
/*********************************************************************************************************/

sys40: // Manager
    stdu    %sp, -STACK(%sp)
    mflr    %r0
   
    bl      push_registers2
    bl      reubicador

    cmplwi  %r3, 1 // operation mode: in %r4, disable = 0, enable = 1, enable with autodetect = 2
    beq     func_1 
    cmplwi  %r3, 2 // set async table
    beq     func_2
    cmplwi  %r3, 3 // get mounted flag (0 if the disc is not mounted, 1 when VSH.SELF mount the disc)
    beq     func_3
    cmplwi  %r3, 4 // return event id (event when you connect an USB device)
    beq     func_4
    cmplwi  %r3, 5 // ignore pid for operations
    beq     func_5
    cmplwi  %r3, 6 // set vsh_event
    beq     func_6
    cmplwi  %r3, 7 // set BDMedia Id
    beq     func_7
    b       return0

// func 1: enable/disable     
func_1:
    stw     %r4, enable_mode@l(%r31)
    cmplwi  %r4, 0
    beq     return0
    li      %r0, 0
    stw     %r0, flag_use@l(%r31)
    stw     %r0, event_id@l(%r31)
    stw     %r0, event_set@l(%r31)
    b       return0

// func 2: // set async table
func_2:
    std     %r4, table_async@l(%r31)
    b       return0

func_3: // get mounted flag
    lwz     %r3, mounted_flag@l(%r31)
    b       return

func_4: // return event id
    lwz     %r3, event_id@l(%r31)
    b       return

func_5: // ignore pid for operations
    stw     %r4, ignore_pid@l(%r31)
    b       return0

func_6: // set vsh_event
    stw     %r4, vsh_event@l(%r31)
    b       return0

func_7: // set BDMedia Id

    addi    %r3, %r31, media_id@l
    li      %r5, 0x10
    bl      lv2memcpy
    b       return0
    nop

/*********************************************************************************************************/
/* STACK AND RETURN MODES                                                                                */
/*********************************************************************************************************/

return0:
    li      %r3, 0

return:
    ld      %r26, VAR_R26(%sp)
    ld      %r27, VAR_R27(%sp)
    ld      %r28, VAR_R28(%sp)
    ld      %r29, VAR_R29(%sp)
    ld      %r30, VAR_R30(%sp)
    ld      %r31, VAR_R31(%sp)
    ld      %r0,  VAR_R0(%sp)
    addi    %sp, %sp, STACK
    mtlr    %r0
    blr

return2:
    ld      %r3,  VAR_RET(%sp)
    b       return

push_registers:
    std     %r3,  VAR_R3(%sp)
    std     %r4,  VAR_R4(%sp)
    std     %r5,  VAR_R5(%sp)
    std     %r6,  VAR_R6(%sp)
    std     %r7,  VAR_R7(%sp)
    std     %r8,  VAR_R8(%sp)
    std     %r9,  VAR_R9(%sp)

push_registers2:
    std     %r26, VAR_R26(%sp)
    std     %r27, VAR_R27(%sp)
    std     %r28, VAR_R28(%sp)
    std     %r29, VAR_R29(%sp)
    std     %r30, VAR_R30(%sp)
    std     %r31, VAR_R31(%sp)
    std     %r0,  VAR_R0(%sp)
    blr

restore_regs:
    ld      %r3,  VAR_R3(%sp)
    ld      %r4,  VAR_R4(%sp)
    ld      %r5,  VAR_R5(%sp)
    ld      %r6,  VAR_R6(%sp)
    ld      %r7,  VAR_R7(%sp)
    ld      %r8,  VAR_R8(%sp)
    ld      %r9,  VAR_R9(%sp)
    blr

/*********************************************************************************************************/
/* SYSCALL 130: sysEventQueueReceive()                                                                   */
/*********************************************************************************************************/

sys130: // sysEventQueueReceive
    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers
    bl      reubicador

    li      %r11, 1
    bl      syscall // get pid
    mr      %r26, %r3
    bl      restore_regs

    lwz     %r0, ignore_pid@l(%r31)
    cmplw   %r0, %r26
    beq     sys_130_call

    lwz     %r0, event_set@l(%r31)
    cmplwi  %r0, 1
    beq     sys130_event_set

sys_130_call:
    ld      %r11, syscall_130@l(%r31)
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl
    
    std     %r3,  VAR_RET(%sp)
    cmpldi  %r3, 0
    bne     return2

    lwz     %r0, ignore_pid@l(%r31)
    cmplw   %r0, %r26
    beq     return2

    ld      %r0, id_bdvd@l(%r31)
    cmpld   %r7, %r0
    beq     sys130_bdvd_event

    lwz     %r0, enable_mode@l(%r31) // disabled ?
    cmpldi  %r0, 0
    beq     return2

    lwz     %r0, flag_use@l(%r31)
    cmplwi  %r0, 2
    beq     sys130_book_test //return2 // 
    
    // test one DEVICE event from the table

    addi    %r11, %r31, id_usb000@l

    li      %r0, 11
    mtctr   %r0

1:
    ld      %r0, 0(%r11)
    cmpld   %r7, %r0
    beq     sys130_usb_event

    addi    %r11, %r11, 8
    bdnz    1b

sys130_book_test:    
    addi    %r30, %r31, booktype_mess@l
    ld      %r0, 0(%r30)
    cmpldi  %r0, 0
    beq     sys130_38
    cmpld   %r5, %r0
    bne     sys130_38
    ld      %r0, 8(%r30) // check for pid
    cmpld   %r26, %r0
    bne     sys130_38
  
// cmd 0x11
    ld      %r29, 16(%r30)
    ld      %r27, 24(%r30)
    lwz     %r28, 32(%r30)
    cmpldi  %r29, 0
    beq     return2
    cmpldi  %r27, 0
    beq     return2

    stw     %r28, 0(%r29)
    li      %r0, 0
    std     %r0, 0(%r30)
    li      %r6, 0
    li      %r7, 0
    b       return2

sys130_38:

// cmd 0x38
    std     %r4,  VAR_R4(%sp)
    std     %r5,  VAR_R5(%sp)
    
    addi    %r29, %r31, send_async_mess@l

    ld      %r0, 0(%r29)
    cmpldi  %r0, 0
    beq     return2
    cmpld   %r5, %r0
    bne     return2
    ld      %r0, 8(%r29) // check for pid
    cmpld   %r26, %r0
    bne     return2
    
    ld      %r27, table_async@l(%r31)
    cmpldi  %r27, 0
    beq     return2

sys130_38_loop:
    
    lwz     %r4,  0(%r27)
    lwz     %r28, 4(%r27)
    cmplwi  %r4, 0
    beq     sys130_38_2

    MEM_LV2_BASE(%r3)
    add     %r4, %r4, %r3
    add     %r28, %r28, %r3
    
    ld      %r3, 16(%r29)
    li      %r5, 8
    bl      lv2memcmp
    cmplwi  %r3, 0
    beq     sys130_38_copy

    addi    %r27, %r27, 8
    b       sys130_38_loop

sys130_38_copy:
    
    ld      %r3, 32(%r29)
    mr      %r4, %r28
    ld      %r5, 40(%r29)
    bl      lv2memcpy

sys130_38_2:
    li  %r6, 0
    li  %r7, 0
    
    ld      %r4,  VAR_R4(%sp)
    ld      %r5,  VAR_R5(%sp)
    b       return2 

sys130_event_set:
    ld      %r0, VAR_R3(%sp)
    lwz     %r11, event_id@l(%r31)
    cmplw   %r0, %r11
    bne     sys_130_call
    
    li      %r0, 0
    stw     %r0, event_set@l(%r31)

sys130_setbookevent:
    li      %r0, 1
    stw     %r0, flag_use@l(%r31)
    ld      %r7, id_bdvd@l(%r31)
    li      %r5, 3
    lwz     %r6, book_type@l(%r31)
    b       return2

sys130_usb_event:
    cmpldi  %r5, 3
    bne     return2
    lwz     %r0, flag_use@l(%r31)
    cmplwi  %r0, 0
    bne     return2
    lwz     %r0, mounted_flag@l(%r31)
    cmplwi  %r0, 0
    bne     return2
    ld      %r0, VAR_R3(%sp)
    stw     %r0, event_id@l(%r31)

    li      %r0, 1
    stw     %r0, event_set@l(%r31)
    b       return2

sys130_bdvd_event:
    cmpldi  %r5, 0
    bne     1f
    cmpldi  %r6, 0x1234
    beq     2f

1:
    cmpldi  %r5, 8 // ejection?
    bne     return2
    li      %r0, 0
    stw     %r0, mounted_flag@l(%r31)
    stw     %r0, enable_mode@l(%r31) // disables
    li      %r5, 0
    li      %r6, 0
    b       return2

2:
    li      %r0, 1
    stw     %r0, enable_mode@l(%r31) // to 1 because is possible it is in mode 2
    lwz     %r0, flag_use@l(%r31)
    cmplwi  %r0, 0
    bne     return2
    lwz     %r0, mounted_flag@l(%r31)
    cmplwi  %r0, 0
    bne     return2
    b       sys130_setbookevent

/*********************************************************************************************************/
/* SYSCALL 864: storage_manager()                                                                        */
/*********************************************************************************************************/

sys864:  // storage_manager
    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers2
    bl      reubicador

    lwz     %r30, enable_mode@l(%r31) // disabled ?
    cmpldi  %r30, 0
    beq     1f

    cmpldi  %r3, 0x5007
    bne     1f

    ld      %r0, 0(%r4)
    cmpldi  %r0, 0x3f   // from DISC?
    bne     1f
    li      %r0, 0x41   // change from HDD
    std     %r0, 0(%r4)

1:
   
    ld      %r11, syscall_864@l(%r31)
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl
    b       return
 
/*********************************************************************************************************/
/* SYSCALL 837: sys_fs_mount()                                                                           */
/*********************************************************************************************************/

sys837: // sys_fs_mount 
    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers2

    bl      reubicador

    mr      %r26, %r3
    mr      %r27, %r4
    mr      %r28, %r5

#if 0
// before with ignore
    mr      %r3, %r26
    addi    %r4, %r31, bdvd1@l
    bl      lv2strcmp
    cmplwi  %r3, 0
    bne     1f
 /*
    mr      %r3, %r27
    addi    %r4, %r31, bdvd2@l
    bl      lv2strcmp
    cmplwi  %r3, 0
    bne     1f
*/
    mr      %r3, %r28
    addi    %r4, %r31, bdvd3@l
    bl      lv2strcmp
    cmplwi  %r3, 0
    bne     1f
    
    li      %r3, 0
    b       return

1:
#endif
    mr      %r3, %r26 
    mr      %r4, %r27 
    mr      %r5, %r28

    ld      %r11, syscall_837@l(%r31)
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl
    
    lwz     %r0, enable_mode@l(%r31) // disabled ?
    cmpldi  %r0, 0
    beq     return

    lwz     %r0, flag_use@l(%r31)
    cmplwi  %r0, 1
    bne     return

    li      %r0, 2
    stw     %r0, flag_use@l(%r31)

    std     %r3, VAR_RET(%sp)
    cmpld   %r3, 0
    beq     return

    mr      %r3, %r26
    addi    %r4, %r31, bdvd1@l
    bl      lv2strcmp
    cmplwi  %r3, 0
    bne     return2
 /*
    mr      %r3, %r27
    addi    %r4, %r31, bdvd2@l
    bl      lv2strcmp
    cmplwi  %r3, 0
    bne     return2
*/
    mr      %r3, %r28
    addi    %r4, %r31, bdvd3@l
    bl      lv2strcmp
    cmplwi  %r3, 0
    bne     return2
    li      %r0, 1
    stw     %r0, mounted_flag@l(%r31)
    // it assumes r3 is zero via lv2strcmp
    b       return

/*********************************************************************************************************/
/* SYSCALL 619: sys_storage_async_send_device_command()                                                  */
/*********************************************************************************************************/

sys619: // sys_storage_async_send_device_command

    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers
    bl      reubicador

    lwz     %r0, flag_use@l(%r31)
    cmplwi  %r0, 2
    beq     sys619_call

    li      %r11, 1
    bl      syscall    // get pid
    mr      %r26, %r3
    bl      restore_regs

    lwz     %r0, ignore_pid@l(%r31)
    cmplw   %r0, %r26
    beq     sys619_call
   
    cmpldi  %r6, 0x38
    beq     sys619_38  // ATAPI CMD (0x38 in)

    cmpldi  %r4, 0x11  // detect booktype CMD
    bne     sys619_call
    cmpldi  %r6, 4
    bne     sys619_call
    lwz     %r26, 0(%r5)
    cmplwi  %r26, 0x0
    bne     sys619_call

// 0x11
    addi    %r30, %r31, booktype_mess@l
    std     %r9,  0(%r30)
    std     %r26, 8(%r30) // pid
    std     %r7, 16(%r30)
    std     %r8, 24(%r30)
    b       sys619_call

sys619_38:
    addi    %r30, %r31, send_async_mess@l
    std     %r9,  0(%r30)
    std     %r26, 8(%r30) // pid
    std     %r5, 16(%r30)
    std     %r6, 24(%r30)
    std     %r7, 32(%r30)
    std     %r8, 40(%r30)

sys619_call:

    ld      %r11, syscall_619@l(%r31)
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl

    std     %r3,  VAR_RET(%sp)
    b       return2


/*********************************************************************************************************/
/* SYSCALL 609: sys_storage_get_device_info()                                                            */
/*********************************************************************************************************/

sys609:  //  sys_storage_get_device_info 

    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers
    bl      reubicador
   
    mr      %r26, %r3
    mr      %r27, %r4

    lhz     %r0, 16(%r4) // hack to use from Iris Manager
    cmplwi  %r0, 0x1234          
    bne     1f
    lhz     %r0, 18(%r4) 
    cmplwi  %r0, 0x5678          
    beq     sys609_call
1:
    lwz     %r0, enable_mode@l(%r31) 
    cmpldi  %r0, 2                   // only works with mode 2
    bne     sys609_call

    lwz     %r0, mounted_flag@l(%r31)
    cmplwi  %r0, 0
    bne     sys609_call

    ld      %r0, id_bdvd@l(%r31)
    cmpld   %r3, %r0
    beq     sys609_bdvd
    
    mr      %r3, %r27

// create port

    li      %r4, 1
    li      %r5, 0x1234
    li      %r11, 134
    bl      syscall
    cmpldi  %r3, 0
    bne     sys609_call
    
// connect port with event (bucle to find it)

    lwz     %r28, vsh_event@l(%r31)
    li      %r29, 0
1:
    lwz     %r3, 0(%r27)
    mr      %r4, %r28
    mr      %r30, %r28
    addi    %r28, %r28, 0x100
    
    li      %r11, 136
    bl      syscall
    cmpldi  %r3, 0
    beq     1f
    addi    %r29, %r29, 1
    cmpldi  %r29, 3
    bne     1b
    b       2f
   
1:
// send event
    lwz     %r3, 0(%r27)
    li      %r4, 0x0
    li      %r5, 0x1234
    ld      %r6, id_bdvd@l(%r31)
    li      %r11, 138
    bl      syscall
    cmpldi  %r3, 0
    bne     2f
    
    stw     %r30, event_id@l(%r31) // store event
    b       sys609_call
   
2:
// destroy port
    mr      %r3, %r28
    li      %r11, 135
    bl      syscall

sys609_call:

    mr      %r3, %r26
    mr      %r4, %r27

    ld      %r11, syscall_609@l(%r31)
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl
    b       return

sys609_bdvd:           // simulate device info for BDVD
    mr      %r3, %r27
    addi    %r4, %r31, disc_info@l
    li      %r5, 64
    bl      lv2memcpy
    b       return0

/*********************************************************************************************************/
/* SYSCALL 879: sys_ss_media_id()                                                                        */
/*********************************************************************************************************/

sys879: // sysEventQueueReceive
    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers
    bl      reubicador
    lis     %r5, 0x1
    ori     %r5, %r5, 1
    cmpld   %r3, %r5
    bne     sys879_call

    lwz     %r5, enable_mode@l(%r31)
    cmplwi  %r5, 0
    beq     sys879_call

    mr      %r3, %r4
    addi    %r4, %r31, media_id@l
    mr      %r26, %r4
    li      %r5, 0x10
    bl      lv2memcpy
    ld      %r3, 0(%r26)
    ld      %r4, 8(%r26)
    or.     %r3, %r3, %r4 
    bne     return0

    lis     %r3, -0x7FFF // if datas == zero return ENOENT
    ori     %r3, %r3, 0x6
    b       return

sys879_call:
    ld      %r11, syscall_879@l(%r31)
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl
    b       return

/*********************************************************************************************************/
/* DATAS for SYSCALL 619 messages                                                                        */
/*********************************************************************************************************/

    .align 3
booktype_mess:
    .quad 0 // reference
    .quad 0 // pid
    .quad 0 // addr out
    .quad 0 // len out

book_type:
    .long 0xff71 // ps3 booktype

send_async_mess:
    .quad 0 // reference
    .quad 0 // pid
    .quad 0 // addr in
    .quad 0 // len in
    .quad 0 // addr out
    .quad 0 // len out

table_async:
    .quad 0x80000000007DF000

/*********************************************************************************************************/
/* DATAS for devices ID                                                                                  */
/*********************************************************************************************************/

id_bdvd:
    .quad 0x0101000000000006
id_usb000:
    .quad 0x010300000000000A
    .quad 0x010300000000000B
    .quad 0x010300000000000C
    .quad 0x010300000000000D
    .quad 0x010300000000000E
    .quad 0x010300000000000F
    .quad 0x010300000000001F
    .quad 0x0103000000000020

    .quad 0x0103000000000010 // Memory Stick
    .quad 0x0103000100000010 // SD Card
    .quad 0x0103000200000010 // Compact Flash

/*********************************************************************************************************/
/* STRINGS for SYSCALL 837                                                                               */
/*********************************************************************************************************/

    .align 2
bdvd1:
    .string "CELL_FS_IOS:PATA0_BDVD_DRIVE"
/*
bdvd2:
    .string "CELL_FS_ISO9660"
*/
bdvd3:
    .string "/dev_bdvd"

/*********************************************************************************************************/
/* GENERAL DATAS                                                                                         */
/*********************************************************************************************************/

enable_mode:
    .long 0
flag_use:
    .long 0

event_id:
    .long 0
event_set:
    .long 0

mounted_flag:
    .long 0

vsh_event:
    .long 0x8d001504

ignore_pid:
    .long 0

/*********************************************************************************************************/
/* STRING for SYSCALL 609                                                                                */
/*********************************************************************************************************/

disc_info:
	.byte 0x75, 0x6E, 0x6E, 0x61, 0x6D, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5C, 0x96, 0xE0
	.byte 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01

media_id:
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

    .align 3
    .quad 0x45535457414C4431

