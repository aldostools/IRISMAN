 /*
 * ps3_fancontrol.S       
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
    .long 0x5046414E    // ID     + 0x0
    .long fan_control   // DATAS  + 0x4
  
syscall_base:           //        + 0x8
    .quad 0x8000000000000000

    .quad sys130  //  + 0x10
syscall_130:
    .quad 0x0000000000000000

    .quad sys138  //  + 0x20
syscall_138:
    .quad 0x0000000000000000

    .quad sys379  //  + 0x30
syscall_379:
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
/* LEDs functions                                                                                        */
/*********************************************************************************************************/

set_led2:
    li      %r3, 2
set_leds:
    li      %r11, 386
    b       syscall

/*********************************************************************************************************/
/* SYSCALL 130: sys_event_queue_receive(equeue_id, event, timeout)                                       */
/*********************************************************************************************************/

sys130: // sys_event_queue_receive

    stdu    %sp, -0x90(%sp)
    mflr    %r0
    std     %r31, 0x78(%sp)
    std     %r4,  0x80(%sp)
    std     %r6,  0x88(%sp)
    std     %r0,  0xA0(%sp)
    
    bl      reubicador

    ld      %r11, syscall_130@l(%r31)  // call to the syscall
    
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl

    ld      %r31, 0x78(%sp)
    ld      %r0,  0xA0(%sp)
    mtlr    %r0
    ld      %r11, 0x80(%sp)
    ld      %r0, 0x88(%sp)
    
    addi    %sp, %sp, 0x90
    
    cmpldi  %r3, 0                     // skip with error
    bnelr
    cmpldi  %r0, 0                     // skip infinite timeouts
    beqlr

    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers
    bl      reubicador

    //or      %r0, %r5, %r6             // combines data1, data2 and data3
    //or.     %r0, %r0, %r7             // the result must be 0
    // bne     sys130_exit
    
    cmpldi  %r6, 0x4ABC                // compares modified data2
    bne     sys130_exit
    or.     %r0, %r5, %r7              // combines data1 and data3
    bne     sys130_exit                // the result must be 0
    li      %r6, 0
    std     %r6,  VAR_R5(%sp)

    lwz     %r0, pid_app@l(%r31)       // get PID from sys138 and store it in the stack
    stw     %r0, VAR_PID(%sp)
    
    addi    %r26, %r11, -1024          // get variable event for get temps

    /*
    li      %r27, 1
    lbz     %r28, in_use@l(%r31)       // skip possible reentry / safe method
    cmplwi  %r28, 0
    bne     sys130_continue2
    stb     %r27, in_use@l(%r31)
    sync
    */

    li      %r27, 1
    addi    %r29, %r31, in_use@l
    lbz     %r28, 0(%r29)              // skip possible reentry in method
    cmplwi  %r28, 0
    bne     sys130_continue2
    stb     %r27, 0(%r29)
    dcbst 0, %r29
    sync

    li      %r3, 0                     // reset sys130 PID
    stw     %r3, pid_app2@l(%r31)
    stw     %r3, mem_app@l(%r31)       // reset mem app

    lwz     %r0, enable@l(%r31)        // test control fan mode (test disable mode)
    cmplwi  %r0, 0
    beq     sys130_continue

    li      %r4, 255
    lbz     %r3, internal_step@l(%r31) // get internal step
    cmplwi  %r3, 2
    bge     sys130_continue            // jump if control fan/control leds steps
    stb     %r4, internal_step@l(%r31) // stores 255 to avoid reentries
    sync

// only to get variable space for get temp method

    li      %r11, 1                    // get current PID
    bl      syscall

    lwz     %r0, VAR_PID(%sp)          // compares with sys138 PID
    cmplw   %r3, %r0
    bne     sys130_release             // skip if not equal
    cmplwi  %r3, 0
    beq     sys130_release             // skip if zero

    stw     %r3, pid_app2@l(%r31)      // adjust sys130 PID
    stw     %r26, mem_app@l(%r31)      // asign sys130 event to internal uses

sys130_release:

    li      %r3, 1
    stb     %r3, internal_step@l(%r31) // adjust STEP 1 as next step
    sync
   
sys130_continue:

    li      %r4, 0
    stb     %r4, in_use@l(%r31)        // free reentry flag
    sync

sys130_continue2:

    bl      restore_regs               // restore registers and exit

sys130_exit:

    b       return
 
/*********************************************************************************************************/
/* SYSCALL 138: sys_event_port_send(port_id, data1, data2, data3)                                        */
/*********************************************************************************************************/

sys138: // sys_event_port_send 

    stdu    %sp, -0x90(%sp)
    mflr    %r0
    std     %r0,  0xA0(%sp)
    std     %r31, 0x78(%sp)

    bl      reubicador
    
    or      %r0, %r4, %r5 // combines data1, data2 and data3
    or.     %r0, %r0, %r6 
    std     %r0,  0x80(%sp)
    bne     1f
    lbz     %r0, internal_step@l(%r31)
    cmplwi  %r0, 255
    beq     1f
    cmplwi  %r0, 2                     // if internal step >= 2
    bge     1f

    li     %r5, 0x4ABC                 // modify data 2 from 0 to 0x4ABC to mark it
    
1:  
    ld      %r11, syscall_138@l(%r31)  // call to the syscall
    
    ld      %r0, 0(%r11)
    mtctr   %r0
    bctrl
    ld      %r31, 0x78(%sp)
    ld      %r11, 0x80(%sp)
    ld      %r0,  0xA0(%sp)
    addi    %sp, %sp, 0x90
    mtlr    %r0
    
    cmpldi  %r3, 0
    bnelr

    cmpldi  %r11, 0 // if datas != 0  return
    bnelr

    stdu    %sp, -STACK(%sp)
    mflr    %r0
    bl      push_registers
    bl      reubicador
    
    lbz     %r0, internal_step@l(%r31)
    cmplwi  %r0, 255
    beq     sys138_call2

    li      %r27, 1
    addi    %r29, %r31, in_use@l
    lbz     %r28, 0(%r29)              // skip possible reentry in method
    cmplwi  %r28, 0
    bne     sys138_call2
    stb     %r27, 0(%r29)
    dcbst 0, %r29
    sync

    lwz     %r0, pid_app2@l(%r31)      // get sys130 PID and stores in the stack
    lwz     %r11, mem_app@l(%r31)      // get sys130 mem_app and stores in the stack
    stw     %r0,  VAR_PID(%sp)
    stw     %r11, VAR_MEM(%sp)

    lwz     %r0, enable@l(%r31)        // test control flag, if disabled reset some variables
    cmplwi  %r0, 0
    bne     get_ticks
    std     %r0, current_time@l(%r31)
    li      %r0, 0
    stb     %r0, internal_step@l(%r31)
    stb     %r0, current_led@l(%r31)
    b       sys138_call
                        
get_ticks:    
    mftb    %r4                        // get tick counter 1s = 79799997 ticks aprox
    cmpldi  %r4, 0
    beq     sys138_call
    srdi    %r4, %r4, 12               // ticks/4096 0 => 1s 0x4C1A ticks aprox

    lbz     %r6, internal_step@l(%r31)
     
    ld      %r3, current_time@l(%r31)
    sub     %r3, %r4, %r3
    cmpldi  %r3, 0xE44F                // compares for 1.5 second
    bge     1f
    cmplwi  %r6, 1
    bne     sys138_call                // if internal step == 1 continue
     
1:
    std     %r4, current_time@l(%r31)

    cmplwi  %r6, 2                     // if internal step >= 2
    bge     2f

    li      %r11, 1                    // get current PID
    bl      syscall
    lwz     %r0, VAR_PID(%sp)          // load sys130 PID
    stw     %r3, pid_app@l(%r31)
    sync

    cmplw   %r3, %r0
    bne     sys138_call
    cmplwi  %r3, 0
    beq     sys138_call                // skip if zero

    lwz     %r26, VAR_MEM(%sp)         // test if mem_app is null and store it in %r26
    cmplwi  %r26, 0
    beq     sys138_call

    lwz     %r27, 0(%r26)              // safe old values from 0(mem_app)

    lbz     %r3, cycle_temp@l(%r31)
    cmplwi  %r3, 0
    beq     get_rsx_temp

// WARNING: sys_game_get_temperature() uses around 7 ms to get values and some times, 20 ms

// get CPU temperature
    
    li      %r3, 0
    stb     %r3, cycle_temp@l(%r31)
    addi    %r4, %r26, 0
    li      %r11, 383                  // sys_game_get_temperature
    bl      syscall
    lbz     %r28, 0(%r26)              // store CPU temperature in %r28
    stb     %r28, cpu_temp@l(%r31)
    sync
    stw     %r27, 0(%r26)              // restore old values from 0(mem_app)
    cmpldi  %r3, 0
    bne     sys138_call                // skip with error
    b       temp_compares
   
get_rsx_temp:

// get RSX temperature

    li      %r3, 1
    stb     %r3, cycle_temp@l(%r31)
    addi    %r4, %r26, 0
    li      %r11, 383                  // sys_game_get_temperature
    bl      syscall
    lbz     %r29, 0(%r26)              // store RSX temperature in %r29
    stb     %r29, rsx_temp@l(%r31)
    sync
    stw     %r27, 0(%r26)              // restore old values from 0(mem_app)
    cmpldi  %r3, 0
    bne     sys138_call                // skip with error

temp_compares:

    lbz     %r28, cpu_temp@l(%r31)
    lbz     %r29, rsx_temp@l(%r31)

    cmplw   %r29, %r28                 // get the most higher temperature
    blt     1f
    mr      %r28, %r29
1:
    stb     %r28, internal_temp@l(%r31) // store current temperature
    li      %r3, 2
    stb     %r3, internal_step@l(%r31)
    sync
    b       sys138_call

2:
    lbz     %r3, internal_step@l(%r31)
    cmplwi  %r3, 2
    blt     sys138_call
    cmplwi  %r3, 255
    beq     sys138_call

    li      %r0, 0                     // reset pid
    stw     %r0, pid_app@l(%r31)
    stw     %r0, mem_app@l(%r31)

    lbz     %r3, internal_step@l(%r31) // get internal step (3 steps secuence)
    cmplwi  %r3, 2
    beq     adjust_temp_method         // 2 adjust fan speed
    cmplwi  %r3, 3
    beq     adjust_temp_leds           // 3 adjust current LED
    cmplwi  %r3, 4
    beq     adjust_temp_leds2          // 4 LED alive method (alternative to step 2)  

sys138_call:
    li      %r4, 0
    stb     %r4, in_use@l(%r31)        // free reentry flag
    sync
sys138_call2:
    bl      restore_regs               // restore registers and exit
    b       return
 

/*********************************************************************************************************/
/* adjust_temp procedure                                                                                 */
/*********************************************************************************************************/

    
adjust_temp_method:                    // STEP 2: adjust fan speed
    
    addi    %r29, %r31, (fan_control)@l 
    lbz     %r28, internal_temp@l(%r31)
    lwz     %r3, temp_control0@l(%r31)
    cmplw   %r28, %r3
    blt     set_fan_lower              // < temp_control0

    addi    %r29, %r29, 8
    
    lbz     %r6, speed_mode@l(%r31)    // temp_control0 <=> temp_control1
    
    cmplwi  %r6, 0
    beq     speed_interval
    addi    %r29, %r29, 4

speed_interval:
    lwz     %r3, temp_control1@l(%r31)
    cmplw   %r28, %r3
    blt     set_fan
    li      %r6, 1
    stb     %r6, speed_mode@l(%r31)    // >= temp_control1
    addi    %r29, %r29, 4

    lwz     %r3, temp_control2@l(%r31) 
    cmplw   %r28, %r3
    blt     set_fan

    addi    %r29, %r29, 4              // >= temp_control2
    lwz     %r3, temp_control3@l(%r31)
    cmplw   %r28, %r3
    blt     set_fan
    addi    %r29, %r29, 4             // >= temp_control3
    lwz     %r3, temp_control4@l(%r31)
    cmplw   %r28, %r3
    blt     set_fan
    addi    %r29, %r29, 4             // >= temp_control4

set_fan:
    lbz     %r26, 3(%r29)
    b       set_fan1

set_fan_lower:

    li      %r6, 0
    stb     %r6, speed_mode@l(%r31)
    lbz     %r26, 7(%r29)

set_fan1:

    li      %r3, 4
    stb     %r3, internal_step@l(%r31) // adjust STEP 4 as next step
    sync
    lbz     %r4, (fan_speed + 3)@l(%r31) // compare if fan_speed is current and skip
    cmplw   %r26, %r4
    beq     adjust_temp_leds2 //sys138_call
    
    li      %r3, 0                     // set fan speed
    li      %r4, 2
    mr      %r5, %r26
    stw     %r5, fan_speed@l(%r31)
    li      %r11, 389                  // sys_sm_set_fan_policy (about 0.01361 ms taken)
    bl      syscall
    li      %r3, 3
    stb     %r3, internal_step@l(%r31) // adjust STEP 3 as next step
    sync
   // b       sys138_call

adjust_temp_leds:                      // STEP 3: adjust current LED
    lbz     %r28, internal_temp@l(%r31)
    li      %r4, 1
    lwz     %r3, (temp_control2)@l(%r31)
    cmplw   %r28, %r3                  // compares temperature < temp_control2
    blt     led_ctrl
    li      %r4, 2

led_ctrl:

    lwz     %r3, enable@l(%r31)        // test control fan mode
    cmplwi  %r3, 2
    bne     sys138_call

    lwz     %r3, (temp_control4)@l(%r31) // test if temperature alarm
    cmplw   %r28, %r3
    blt     led_normally
    lbz     %r5, current_led@l(%r31)
    cmplwi  %r5, 3
    beq     led_skip1
    li      %r5, 3
    stb     %r5, current_led@l(%r31)
    li      %r4, 2
    bl      set_led2                   // leds about 0.01915 ms taken
    li      %r3, 1
    li      %r4, 2
    bl      set_leds
    li      %r0, 0
    stb     %r0, alive_time@l(%r31)
    
led_skip1:    

    li      %r3, 1
    stb     %r3, alarm_temp@l(%r31)    // set temperature alarm
    b       led_exit

led_normally:
// led control
    lbz     %r5, current_led@l(%r31)
    cmplw   %r5, %r4
    beq     led_skip2
    stb     %r4, current_led@l(%r31)
    bl      set_led2                   // Yellow LED on/blink

led_skip2:

    lbz     %r3, alarm_temp@l(%r31)
    cmplwi  %r3, 0
    beq     led_exit

    li      %r4, 2
    beql    set_led2                   // to be sure Yellow led is in current status
    
    li      %r3, 1                     // restore led from temp alarm
    li      %r4, 1
    bl      set_leds

    li      %r3, 0
    stb     %r3, alarm_temp@l(%r31)    // reset temperature alarm
    
led_exit:  
    li      %r3, 0
    stb     %r3, internal_step@l(%r31) // adjust STEP 0 as next step
    sync
    b       sys138_call

adjust_temp_leds2:                    // STEP 4: LED alive method (alternative to step 2)
    lbz     %r28, internal_temp@l(%r31)

    lbz     %r3, alarm_temp@l(%r31)   // skip in temperature alarm condition
    cmplwi  %r3, 0
    bne     1f
    lbz     %r3, alive_time@l(%r31)
    addi    %r3, %r3, 1
    stb     %r3, alive_time@l(%r31)
    sync
    cmplwi  %r3, 4                     // wait 12 seconds aprox
    blt     1f

    li      %r6, 5
    stb     %r6, current_led@l(%r31)   // fore to change the leds
    li      %r3, 0
    stb     %r3, alive_time@l(%r31)
    li      %r3, 3
    stb     %r3, internal_step@l(%r31) // adjust STEP 3 as next step
    sync
    lwz     %r3, enable@l(%r31)        // test control fan mode
    cmplwi  %r3, 2
    bne     sys138_call
    li      %r4, 0
    bl      set_led2                   // reset Yellow led
    b       sys138_call
1:
    li      %r3, 0
    stb     %r3, internal_step@l(%r31) // adjust STEP 0 as next step
    sync
    b       sys138_call

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
/*    
panic:
    li      %r3, 0
    li      %r11, 0xff
    sc 1
*/

/*********************************************************************************************************/
/* VARIABLES                                                                                             */
/*********************************************************************************************************/

in_use:            // safe the integrity
    .byte 0

cpu_temp:          // stores the CPU temperature in ºC
    .byte 0
rsx_temp:          // stores the RSX temperature in ºC
    .byte 0
cycle_temp:        // swap variable to get CPU/RSX temperatures
    .byte 0
internal_temp:     // the combined CPU/RSX more high temperature used to work
    .byte 0
internal_step:     // dispatch the different process (get temperature, fix fan and leds) step to step avoiding the system overload
    .byte 0
speed_mode:        // used for the temp_control0/1 range to decide if fan speed is increasing or decreasing
    .byte 0
alive_time:        // used to change to the green led a second to confirm this payload is working!
    .byte 0
current_led:       // status variable to avoid set the led continually
    .byte 0
alarm_temp:        // flag to change te leds status when temperature >= temp_control4
    .byte 0

    .align 2
pid_app:           // PIDs to check it is secure to use mem_app to read the temperature using the syscall 383
    .long 0
pid_app2:          // It requires an u32 in user space memory and usually sys_event_queue_receive() stores
    .long 0
mem_app:           // the values in one struct sys_event_t that is used here to stores the temperature value
    .long 0        // temporally (old values is preserved but surely is unnecessary)

// externally adjustable

fan_speed:         // stores the current fan speed to avoid change it continually overloading the system
    .long 0x33

enable:            // control all: 0 - disable all, 1- enable without leds, 2 - enable with leds
    .long 0

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

    .align 3
current_time:      // timer (count seconds)
    .quad 0

