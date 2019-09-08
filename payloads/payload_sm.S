 /*
 * payload.S       
 *
 * Copyright (C) (Estwald)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 ********************************************************************************
 */


#define STACK  0x100
#define VAR_R0  0x110

#define VAR_PID 0x78
#define VAR_MEM 0x7C

#define VAR_R3 0x80
#define VAR_R4 0x88
#define VAR_R5 0x90
#define VAR_R6 0x98
#define VAR_R7 0xA0
#define VAR_R8 0xA8
#define VAR_R9 0xB0
#define VAR_R10 0xB8

#define VAR_LR2 0xC0 // used in bl syscall
#define VAR_REG 0xC8 // used in bl syscall

#define VAR_R26 0xD0
#define VAR_R27 0xD8
#define VAR_R28 0xE0
#define VAR_R29 0xE8
#define VAR_R30 0xF0
#define VAR_R31 0xF8

//#define BASE        (0xf70)


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
    .long 0x50534D58    // ID     + 0x0
    .long fan_control   // DATAS  + 0x4
  
syscall_base:           //        + 0x8
    .quad 0x8000000000000000

    .quad sys379  //  + 0x10
syscall_379:
    .quad 0x0000000000000000 // + 0x18
is_sm_sprx_loaded:
    .quad 0x0000000000000000 // + 0x20
 
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

syscall:
 
    mflr    %r0
    std     %r0, VAR_LR2(%sp)
    std     %r2, VAR_REG(%sp)
    ld      %r30, syscall_base@l(%r31)
    sldi    %r11, %r11, 3
    add     %r30, %r30, %r11
    
    ld      %r30, 0(%r30)
    sync
    ld      %r0, 0(%r30)
    mtctr   %r0
    bctrl
    
    ld      %r2, VAR_REG(%sp)
    ld      %r0, VAR_LR2(%sp)
    mtlr    %r0
    blr

/*********************************************************************************************************/
/* STACK AND RETURN MODES                                                                                */
/*********************************************************************************************************/

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

push_registers:
    std     %r3,  VAR_R3(%sp)
    std     %r4,  VAR_R4(%sp)
    std     %r5,  VAR_R5(%sp)
    std     %r6,  VAR_R6(%sp)
    std     %r7,  VAR_R7(%sp)
    std     %r8,  VAR_R8(%sp)
    std     %r9,  VAR_R9(%sp)
    std     %r10, VAR_R10(%sp)
  
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
    ld      %r10, VAR_R10(%sp)
    blr

/*********************************************************************************************************/
/* SYSCALL 379: sys_sm_shutdown()                                                                        */
/*********************************************************************************************************/
    
sys379:
    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers
    bl      reubicador

    cmpldi  %r3, 0x100
    beq     1f
    cmpldi  %r3, 0x200
    beq     1f
    cmpldi  %r3, 0x1100
    beq     1f
    cmpldi  %r3, 0x1200
    beq     1f

    lwz     %r0, enable@l(%r31)        // test control fan mode (test disable mode)
    cmplwi  %r0, 0
    beq     3f

// set fan mode 2
    li      %r4, 2
    lwz     %r5, fan_control@l(%r31)
    b       2f

1:

// set fan mode 1            
    li      %r4, 1
    li      %r5, 0x5f
2:
    li      %r3, 0
    li      %r11, 389                  // sys_sm_set_fan_policy
    bl      syscall

    bl      restore_regs               // restore registers
3:    
    ld      %r11, syscall_379@l(%r31)  // call to the syscall
    
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl
    b       return


/*********************************************************************************************************/
/* VARIABLES                                                                                             */
/*********************************************************************************************************/

// externally adjustable

fan_speed:         // stores the current fan speed to avoid change it continually overloading the system
    .long 0x33

enable:            // control all: 0 - disable all, 1- enable without leds, 2 - enable with leds
    .long 2

// fan control variables offset

fan_control:       // FAN SPEEDS
    .long 0x5f     // from shutdown
    .long 0x4d     // < temp_control0
    .long 0x54     // temp_control0 => temp_control1
    .long 0x60     // temp_control0 <= temp_control1
    .long 0x68     // >= temp_control1
    .long 0x70     // >= temp_control2
    .long 0x78     // >= temp_control3
    .long 0xA0     // >= temp_control4

temp_control0:     // CONTROL TEMPERATURES (ºC)
    .long 62
temp_control1:
    .long 68
temp_control2:
    .long 70
temp_control3:
    .long 72
temp_control4:
    .long 75
temp_control5:
    .long 80
   
