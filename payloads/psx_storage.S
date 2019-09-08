 /*
 * psx_storage.S       
 *
 * Copyright (C) (Estwald)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 ********************************************************************************
 */

#define BASE        (0x1820)


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
_start:
    .quad 0x45505331454D5531    // ID     + 0x20
  
syscall_base:                   //        + 0x28
    .quad 0x8000000000000000

operation_mode:                 //        + 0x30
    .quad 0x0000000000000000 // BDVD broken: higher 32 bits (0/1) mode: lower 32 bits

syscall_open_table:             //        + 0x38
    .long 0x80000000
    .long BASE + syscall_storage_open
    .quad 0x8000000000000000 // TOC       + 0x40
    .quad 0                  //           + 0x48
    .quad 0                  //           + 0x50

syscall_send_cmd_table:      //           + 0x58
    .long 0x80000000
    .long BASE + syscall_storage_send_cmd
    .quad 0x8000000000000000 // TOC       + 0x60
    .quad 0                  //           + 0x68
    .quad 0                  //           + 0x70

syscall_usleep_table:        //           + 0x78
    .long 0x80000000
    .long BASE + syscall_usleep
    .quad 0x8000000000000000 // TOC       + 0x80
    .quad 0                  //           + 0x88
    .quad 0                  //           + 0x90
syscall_usleep_base:         //           + 0x98
    .quad 0
syscall_open_base:           //           + 0xA0
    .quad 0
syscall_send_cmd_base:       //           + 0xA8
    .quad 0

syscall_usleep:
    blr

syscall_storage_open:

    stdu    %sp, -0x0090(%sp)
    std     %r31, 0x0088(%sp)
    std     %r2,  0x0080(%sp)
    mflr    %r0
    std     %r0, 0x00A0(%sp)
    
    MEM_BASE(%r31)
    LOAD_LABEL2(%r10, %r31, syscall_open_table)
    ld      %r0, 0x18(%r10)
    
    li      %r31, 0 
    cmplwi  %r3, 6 // hdd
    bne     sys_st_open1
    
    mr      %r31, %r5
   
sys_st_open1:
   
    mtctr   %r0
    bctrl

    cmplwi  %r31, 0x0
    beq     sys_st_open3

    MEM_BASE(%r10)
    LOAD_LABEL2(%r11, %r10, operation_mode)
    lwz     %r4, 0(%r11) // test if have flag bdvd broken
    cmplwi  %r4, 0
    bne     sys_st_open2
    cmplwi  %r3, 0x0
    bne     sys_st_open3
    
sys_st_open2:  
    lwz     %r0, 0x0(%r31)
    MEM_BASE(%r31)
    LOAD_LABEL2(%r11, %r31, datas)
    std     %r0, -8(%r11) // store fd
    li      %r3, 0

sys_st_open3:

    ld      %r31, 0x0088(%sp)
    ld      %r2, 0x0080(%sp)
    ld      %r0, 0x00A0(%sp)
    mtlr    %r0
    addi    %sp, %sp, 0x90
    blr

syscall_storage_send_cmd:
    
    stdu    %sp, -0x130(%sp)
    mflr    %r0
    std     %r26, 0x100(%sp)
    std     %r27, 0x108(%sp)
    std     %r28, 0x110(%sp)
    std     %r29, 0x118(%sp)
    std     %r30, 0x120(%sp)
    std     %r31, 0x128(%sp)
    std     %r0,  0x140(%sp)

    std     %r3,  0x80(%sp)
    std     %r4,  0x88(%sp)
    std     %r5,  0x90(%sp)
    std     %r6,  0x98(%sp)
    std     %r7,  0xA0(%sp)
    std     %r8,  0xA8(%sp)
    std     %r9,  0xB0(%sp)
    std     %r10, 0xB8(%sp)
 
    MEM_BASE(%r31)
    mr      %r26, %r5
    mr      %r27, %r6
    mr      %r28, %r7
    mr      %r29, %r8

    cmpldi   %r4, 1
    bne     syscall_604_a

    LOAD_LABEL2(%r11, %r31, operation_mode)
    lwz     %r0, 4(%r11)
    cmplwi  %r0, 1
    beq     syscall_604

    LOAD_LABEL2(%r30, %r31, datas)
    
sys_send_cmd0:

    ld      %r0, -8(%r30) // store fd
    cmpld   %r3, %r0
    bne     syscall_604

    li      %r3, 0 // return OK
    
    cmplwi  %r29, 0 // skip CMD without output buffer
    beq     return

    lbz     %r4, 0(%r26)
    stw     %r4, 0x8(%r30) // mode
    
    cmplwi  %r4, 0x51
    beq     disc_information
    
    cmplwi  %r4, 0x43
    beq     disc_toc
    
    cmplwi  %r4, 0x4A
    beq     disc_event_status

    cmplwi  %r4, 0xbe // read cd sector
    bne     syscall_604_a

read_sector:

    lwz     %r11, 0x0(%r30)
    cmplwi  %r11, 0
    beq     sleep_method
    cmplwi  %r11, 1
    beq     open_method
    cmplwi  %r11, 2
    beq     lseek_method
    cmplwi  %r11, 3
    beq     read_method
    cmplwi  %r11, 4
    beq     close_method
    cmplwi  %r11, 5
    beq     check_and_ret
    cmplwi  %r11, 0x81
    beq     eio_method
    cmplwi  %r11, 0x82
    beq     force_close_method
 
    stw     %r28, 0x18(%r30) // addr
    stw     %r28, 0x2c(%r30) // addr2
 
    lbz     %r5, 2(%r26) // get sector
    sldi    %r5, %r5, 8
    lbz     %r4, 3(%r26)
    add     %r5, %r5, %r4
    sldi    %r5, %r5, 8
    lbz     %r4, 4(%r26)
    add     %r5, %r5, %r4
    sldi    %r5, %r5, 8
    lbz     %r4, 5(%r26)
    add     %r5, %r5, %r4
    stw     %r5, 0x10(%r30) // sector

    lbz     %r5, 8(%r26) // get number of sectors
    stw     %r5, 0x14(%r30) // number of sectors
    cmplwi  %r5, 1

    lwz     %r5, 40(%r26)
    stw     %r5, 0xc(%r30) // size to read

    li      %r4, 1
    lwz     %r3, 0x4(%r30) // fd
    cmpwi   %r3, 0
    bgt     lseek_method
    stw     %r4, 0x0(%r30)

    b       open_method

eio_method:
    lis     %r3, -0x7FFF
    ori     %r3, %r3, 0x2B # 0x8001002B
    b       return

disc_information:

    lwz     %r11, 0x0(%r30)
    cmplwi  %r11, 0x81
    beq     eio_method

    addi    %r4, %r31, 0x50 // addr lv2 0x50 // custom datas
    lwz     %r5, 0x24(%r30) // file pointer (get current filename)
    sldi    %r6, %r5, 4
    add     %r4, %r4, %r6
    lwz     %r5, 0x0(%r4)
    add     %r4, %r5, %r31 // lv2 addr
    cmplwi  %r5, 0
    bne     disc_info2
    
    LOAD_LABEL2(%r4, %r31, disc_info_data) // standard datas

disc_info2:
    
    li      %r5, 16
    bl      copy_to_outbuf
    li      %r3, 0
    b       return

disc_toc:

    lwz     %r11, 0x0(%r30)
    cmplwi  %r11, 0x81
    beq     eio_method

    addi    %r4, %r31, 0x50 // addr lv2 0x50 // custom datas
    lwz     %r5, 0x24(%r30) // file pointer (get current filename)
    sldi    %r6, %r5, 4
    add     %r4, %r4, %r6
    lwz     %r6, 0x8(%r4)
    lwz     %r5, 0xC(%r4)
    add     %r4, %r6, %r31 // lv2 addr
    cmplwi  %r6, 0
    bne     disc_toc2

    LOAD_LABEL2(%r4, %r31, toc_data) // standard datas
    li      %r5, 20

disc_toc2:

    bl      copy_to_outbuf
    li      %r3, 0
    b       return

disc_event_status:
/*
    // force connected
    li      %r3, 2 // connected
    LOAD_LABEL2(%r4, %r31, get_event_status)
    stb     %r3, 5(%r4)
    li      %r5, 8
    bl      copy_to_outbuf
    li      %r3, 0
    stw     %r3, 0x1c(%r30) // set event cmd
    b       return
*/
    lwz     %r11, 0x28(%r30) // counter loop
    addi    %r11, %r11, 1
    andi.   %r11, %r11, 3
    stw     %r11, 0x28(%r30)
    cmplwi  %r11, 1
    bne     disc_last_state

    lwz     %r11, 0x1c(%r30) // event cmd   
    cmplwi  %r11, 0x1
    beq     closedir_event

    LOAD_LABEL2(%r3, %r31, end_datas)
    mr      %r4, %r26
    li      %r5, 0x20
    bl      lv2memcpy  // copy user area to lv2

    mr      %r3, %r26
    LOAD_LABEL2(%r4, %r31, dvd_dev)
    bl      lv2strcpy

    mr      %r3, %r26
    addi    %r4, %r26, 0x18
    li      %r11, 805  // openDir
    bl      syscall
    mr      %r10, %r3

    lwz     %r4, 0x18(%r26)
    stw     %r4, 0x20(%r30) // store fd opendir

    mr      %r3, %r26
    LOAD_LABEL2(%r4, %r31, end_datas)
    li      %r5, 0x20
    bl      lv2memcpy  // copy lv2 to user area

    cmplwi  %r10, 0 
    bne     set_discon

    lwz     %r5, 0x1c(%r30)
    li      %r3, 0x1
    stw     %r3, 0x1c(%r30) // event cmd (closedir)

    cmplwi  %r5, 0x80 // test previous disconnected status
    bne     sys_recall

    LOAD_LABEL2(%r11, %r31, operation_mode)
    lwz     %r4, 4(%r11)

    lwz     %r5, 0x24(%r30)
    
    cmplwi  %r4, 2  // if operation_mode == 2, set disable_emulation 0 and ignore file pointer increment
    bne     incre_file
    li      %r4, 0
    std     %r4, 4(%r11)
    addi    %r5, %r5, -0x1

incre_file:    
    addi    %r5, %r5, 0x1  // increment file pointer
    andi.   %r6, %r5, 0x7
    stw     %r6, 0x24(%r30) // file pointer
    li      %r4, 0x82
    stw     %r4, 0x0(%r30) // read cmd
    b       sys_recall

closedir_event:
    lwz     %r3, 0x20(%r30) // fd opendir
    li      %r11, 807 // closeDir
    bl      syscall

    lwz     %r3, 0x34(%r30)
    cmplwi  %r3, 1
    beq     1f
    li      %r3, 1
    li      %r4, 1
    stw     %r4, 0x34(%r30)
    li      %r11, 386
    bl      syscall
1:    
    li      %r3, 0
    stw     %r3, 0x1c(%r30) // set event cmd
  
    LOAD_LABEL2(%r11, %r31, operation_mode)
    lwz     %r4, 4(%r11)
    li      %r3, 2 // connected
    cmplwi  %r4, 2
    bne     nclosedir_event
    li      %r3, 1 // disconnected

nclosedir_event:

    LOAD_LABEL2(%r4, %r31, get_event_status)
    stb     %r3, 5(%r4)
    li      %r5, 8
    bl      copy_to_outbuf
    li      %r3, 0
    b       return

disc_last_state:
    LOAD_LABEL2(%r4, %r31, get_event_status)
    li      %r5, 8
    bl      copy_to_outbuf
    li      %r3, 0
    b       return

set_discon:
    li      %r3, 0x80
    stw     %r3, 0x1c(%r30)

    LOAD_LABEL2(%r11, %r31, operation_mode)
    lwz     %r0, 4(%r11)
    cmplwi  %r0, 3         // if disable mode 3
    bne     nset_discon1
    li      %r0, 1         // disable the emulation
    stw     %r0, 4(%r11)
    b       nset_discon2

nset_discon1:
    li      %r3, 1
    li      %r4, 0x2
    stw     %r4, 0x34(%r30)
    li      %r11, 386
    bl      syscall

nset_discon2:

    li      %r3, 1 // disconnected
    LOAD_LABEL2(%r4, %r31, get_event_status)
    stb     %r3, 5(%r4)
    li      %r5, 8
  
    bl      copy_to_outbuf

    li      %r4, 0x81
    stw     %r4, 0x0(%r30) // read cmd

    li      %r3, 0
    b       return
  
copy_to_outbuf:
    mr      %r3, %r28
    cmplwi  %r29, %r5
    bge     copy_to_b1
    mr      %r5, %r29
copy_to_b1:
    b       lv2memcpy


syscall_604_a:

    LOAD_LABEL2(%r11, %r31, operation_mode)
    lwz     %r0, 0(%r11) // test if have flag bdvd broken
    cmplwi  %r0, 0
    beq     syscall_604
    li      %r3, 0
    b       return

// call original syscall 604 routine
syscall_604:
    
#if 0
    b     return
#else
    ld      %r3,  0x80(%sp)
    ld      %r4,  0x88(%sp)
    ld      %r9,  0xB0(%sp)
    mr      %r5, %r26
    mr      %r6, %r27
    mr      %r7, %r28
    mr      %r8, %r29
    
    LOAD_LABEL2(%r10, %r31, syscall_send_cmd_table)
    ld      %r0, 0x18(%r10)
    mtctr   %r0
    bctrl
    // space to receive datas and modify
    b       return

#endif


sleep_method:
    li      %r3, 1
    li      %r11, 142 // sleep
    bl      syscall
    li      %r4, 1
    stw     %r4, 0x0(%r30) // new mode
    b       e_busy

open_method:
    
    li      %r4, 0
    stw     %r4, 0x4(%r30) // new fd
    
    lwz     %r3, 0x18(%r30)
   // mr      %r3, %r28
    LOAD_LABEL2(%r4, %r31, file_name)
    lwz     %r5, 0x24(%r30) // file pointer (get current filename)
    sldi    %r6, %r5, 3
    add     %r4, %r4, %r6

    bl      lv2strcpy
   
    lwz     %r3, 0x18(%r30)
    //mr      %r3, %r28
    li      %r4, 0
    addi    %r5, %r3, 0x930
    li      %r6, 511
    li      %r7, 0
    li      %r8, 0
 
    li      %r11, 801
    bl      syscall
    nop
  
    cmplwi  %r3, 0
    beq     nopen_method2
  
    b       e_busy

nopen_method2:

    lwz     %r11, 0x18(%r30)
    lwz     %r5, 0x930(%r11)
    stw     %r5, 0x4(%r30) // save fd
    li      %r4, 2
    stw     %r4, 0x0(%r30) // new mode
    b       sys_recall

lseek_method:

    lwz     %r3, 0x4(%r30) // fd
    lwz     %r4, 0x10(%r30) // sector
    
    ld      %r5, 0xD0(%r31) // sector size from lv2
    cmpldi  %r5, 0
    bne     nlseek_method
    
    li      %r5, 2352 // by default 2352 bytes

nlseek_method:

    lwz     %r11, 0x18(%r30)
    mullw   %r4, %r4, %r5
    li      %r5, 0
    addi    %r6, %r11, 0x930
    li      %r11, 818
    bl      syscall
    li      %r4, 3
    stw     %r4, 0x0(%r30) // new mode
    cmplwi  %r3, 0
    beq     sys_recall
    b       e_close

read_method:

    lwz     %r3, 0x18(%r30) // addr
    lwz     %r6, 0x10(%r30) // sector
    addi    %r4, %r6, 150

    bl      sector_to_time   // generate header information for 2048 bytes sector

    lwz     %r3, 0x4(%r30) // fd
    lwz     %r4, 0x18(%r30)
    ld      %r7, 0xD0(%r31) // sector size from lv2
    li      %r5, 2352
    cmpldi  %r7, 0
    beq     nread_method
    cmpldi  %r7, 2352
    bge     nread_method
    mr      %r5, %r7
    cmpldi  %r7, 2048
    bne     nread_method
    li      %r5, 2048
    addi    %r4, %r4, 24
    
nread_method:
    
    lwz     %r10, 0x18(%r30) // addr to read sector
    addi    %r6, %r10, 0x930
    li      %r11, 802
    bl      syscall
    cmplwi  %r3, 0
    bne     e_close

    lwz     %r10, 0x18(%r30)
    addi    %r3, %r10, 0x930
    lwz     %r6, 0x10(%r30) // sector
    addi    %r4, %r6, 1

    bl      sector_to_time2   // generate time information for next sector - 2 seconds
    
    li      %r5, 0x41
    stb     %r5, 0(%r3)

    li      %r5, 0x1
    stb     %r5, 1(%r3)
    stb     %r5, 2(%r3)

    lbz     %r0, 12(%r3)
    stb     %r0, 3(%r3)
    lbz     %r0, 13(%r3)
    stb     %r0, 4(%r3)
    lbz     %r0, 14(%r3)
    stb     %r0, 5(%r3)

    lwz     %r6, 0x10(%r30) // sector
    addi    %r4, %r6, 151

    bl      sector_to_time2   // generate time information for next sector

    lbz     %r0, 12(%r3)
    stb     %r0, 7(%r3)
    lbz     %r0, 13(%r3)
    stb     %r0, 8(%r3)
    lbz     %r0, 14(%r3)
    stb     %r0, 9(%r3)

    li      %r5, 0x0
    stb     %r5, 6(%r3)
    sth     %r5, 10(%r3)
    stw     %r5, 12(%r3)
  
    li      %r4, 5
    stw     %r4, 0x0(%r30) // new mode

    li      %r3, 0

    lwz     %r7, 0x10(%r30) // sector
 /*
 // patch sector 16 (to avoid ps1_netemu from ps1_emu
    
    cmplwi  %r7, 16         //  test sector 16
    bne     iread_method
    li      %r0, 57  
    lwz     %r4, 0x18(%r30)
    stb     %r0,  0x398(%r4) // patch ID
    
iread_method:
*/
    lwz     %r4, 0x18(%r30)
    lwz     %r5, 0x14(%r30) // number of sectors
    lwz     %r6, 0xc(%r30)  // size to read
    add     %r4, %r4, %r6   // addr + size to read
    stw     %r4, 0x18(%r30)
    addi    %r5, %r5, -1   // number of sectors - 1
    stw     %r5, 0x14(%r30)
    addi    %r7, %r7, 1    // increment sector
    stw     %r7, 0x10(%r30)

    cmplwi  %r5, 0  // if zero end
    beq     check_and_ret
   
    li      %r4, 2 // read more datas
    stw     %r4, 0x0(%r30) // new mode 
    b       sys_recall

check_and_ret:   
    lwz     %r11, 0x2c(%r30)
    cmplw   %r28, %r11
    bne     e_busy
    li      %r4, 0x80
    stw     %r4, 0x0(%r30)
    b       return


force_close_method:
    lwz     %r3, 0x4(%r30) // fd
    cmpwi   %r3, 0
    beq     nfclose_method
    li      %r11, 804
    bl      syscall
nfclose_method:
    li      %r4, 0x80
    stw     %r4, 0x0(%r30) // new mode
    li      %r4, 0
    stw     %r4, 0x4(%r30) // new fd
    b       e_busy

close_method:
    lwz     %r3, 0x4(%r30) // fd
    cmpwi   %r3, 0
    beq     nclose_method
    li      %r11, 804
    bl      syscall
nclose_method:
    li      %r4, 0
    stw     %r4, 0x0(%r30) // new mode
    li      %r4, 0
    stw     %r4, 0x4(%r30) // new fd
    b       e_busy

sys_recall:
// new recall syscall method
   
    ld      %r6, (0x130 + 0x80)(%sp) // decrease 4 bytes the syscall return address
    addi    %r6, %r6, -4
    std     %r6, (0x130 + 0x80)(%sp)
    sync

    ld      %r3,  0x80(%sp)
    ld      %r4,  0x88(%sp)
    ld      %r5,  0x90(%sp)
    ld      %r6,  0x98(%sp)
    ld      %r7,  0xA0(%sp)
    ld      %r8,  0xA8(%sp)
    ld      %r9,  0xB0(%sp)
    li      %r11, 604
    b       return

e_close:
    li      %r4, 4
    stw     %r4, 0x0(%r30) // new mode (close)
e_busy:
    lis     %r3, -0x7FFF
    ori     %r3, %r3, 0xA # 0x8001000A
    
return:

    ld      %r26, 0x100(%sp)
    ld      %r27, 0x108(%sp)
    ld      %r28, 0x110(%sp)
    ld      %r29, 0x118(%sp)
    ld      %r30, 0x120(%sp)
    ld      %r31, 0x128(%sp)
    ld      %r0,  0x140(%sp)
    addi    %sp, %sp, 0x130
    mtlr    %r0
    blr

syscall:
    stdu    %sp, -0x0090(%sp)
    std     %r31, 0x0088(%sp)
    std     %r2, 0x0080(%sp)
    mflr    %r0
    std     %r0, 0x00A0(%sp)
    MEM_BASE(%r31)
    LOADI_LABEL2(%r31, syscall_base)
    ld      %r31, 0(%r31)
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
    nop

lv2memcpy_t1:
    lbzu    %r0, 1(%r4)
    stbu    %r0, 1(%r3)
    bdnz    lv2memcpy_t1
    blr

lv2strcpy:
    li      %r5, 1024
    addi    %r3, %r3, -1
    addi    %r4, %r4, -1
    mtctr   %r5
    nop

lv2strcpy_t1:
    lbzu    %r0, 1(%r4)
    stbu    %r0, 1(%r3)
    cmplwi  %r0, 0
    beq     lv2strcpy_t2
    bdnz    lv2strcpy_t1

lv2strcpy_t2:
    blr

sector_to_time:

    li      %r10, 0x930
    addi    %r9, %r3, -1
    mr      %r11, %r3
    mtctr   %r10
    li      %r0, 0
    nop

sector_t1:
    stbu    %r0, 1(%r9)
    bdnz    sector_t1
    li      %r9, 0xA
    li      %r0, -1
    mtctr   %r9
    nop

sector_t2:
    stbu    %r0, 1(%r11)
    bdnz    sector_t2

sector_to_time2:
    lis     %r10, 0x7482
    lis     %r8, 0x1B4E
    ori     %r10, %r10, 0x296B
    srawi   %r0, %r4, 0x1F
    lis     %r11, 0x6666
    ori     %r8, %r8, 0x81B5
    ori     %r11, %r11, 0x6667
    mulhw   %r8, %r4, %r8
    li      %r9, 2
    srawi   %r8, %r8, 3
    stb     %r9, 0xF(%r3)
    mulhw   %r10, %r4, %r10
    srawi   %r10, %r10, 0xB
    subf    %r10, %r0, %r10
    subf    %r0, %r0, %r8
    srawi   %r9, %r10, 0x1F
    srawi   %r8, %r0, 0x1F
    mulhw   %r7, %r10, %r11
    srawi   %r7, %r7, 2
    subf    %r9, %r9, %r7
    mulli   %r7, %r0, 0x4B
    slwi    %r6, %r9, 3
    subf    %r4, %r7, %r4
    slwi    %r5, %r9, 1
    slwi    %r7, %r9, 4
    add     %r5, %r5, %r6
    srawi   %r9, %r4, 0x1F
    subf    %r10, %r5, %r10
    mulhw   %r6, %r4, %r11
    add     %r7, %r7, %r10
    srawi   %r6, %r6, 2
    stb     %r7, 0xC(%r3)
    subf    %r9, %r9, %r6
    slwi    %r5, %r9, 1
    slwi    %r6, %r9, 3
    slwi    %r10, %r9, 4
    add     %r9, %r5, %r6
    subf    %r4, %r9, %r4
    lis     %r9, 0x8888
    add     %r10, %r10, %r4
    ori     %r9, %r9, 0x8889
    stb     %r10, 0xE(%r3)
    mulhw   %r9, %r0, %r9
    add     %r9, %r9, %r0
    srawi   %r9, %r9, 5
    subf    %r9, %r8, %r9
    slwi    %r10, %r9, 2
    slwi    %r9, %r9, 6
    subf    %r9, %r10, %r9
    subf    %r0, %r9, %r0
    srawi   %r9, %r0, 0x1F
    mulhw   %r11, %r0, %r11
    srawi   %r11, %r11, 2
    subf    %r9, %r9, %r11
    slwi    %r10, %r9, 1
    slwi    %r11, %r9, 3
    slwi    %r9, %r9, 4
    add     %r11, %r10, %r11
    subf    %r0, %r11, %r0
    add     %r0, %r9, %r0
    stb     %r0, 0xD(%r3)
    blr

    .align 3
//////////////////////////////////////////
    .quad   0x0 // storage fd
    .align 2
datas:
    .long   0x80 // mode cd read   // 00
    .long   0 // fd                // 04
    .long   0 // cmd               // 08
    .long   0 // size to read      // 0c
    .long   0 // sector            // 10
    .long   0 // number of sectors // 14
    .long   0 // addr              // 18
    .long   0 // mode cmd event    // 1c
    .long   0 // fd opendir        // 20
    .long   0 // file pointer      // 24
    .long   0 // counter           // 28
    .long   0 // addr2             // 2c
    .long   0 // check usleep      // 30
    .long   0 // check leds        // 34
    
    .align 2
file_name:
    .string "/psx_d0"
    .string "/psx_d1"
    .string "/psx_d2"
    .string "/psx_d3"
    .string "/psx_d4"
    .string "/psx_d5"
    .string "/psx_d6"
    .string "/psx_d7"

dvd_dev:
    .string "/psx_cdrom0"

disc_info_data:
	.byte 0x00, 0x20, 0x0E, 0x01, 0x01, 0x01, 0x01, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
toc_data:
    .byte 0x00, 0x12, 0x01, 0x01, 0x00, 0x14, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0xAA, 0x00, 0x00, 0x05, 0x7F, 0xFF
get_event_status:
    .byte 0x00, 0x06, 0x04, 0x56, 0x00, 0x02, 0x00, 0x00
end_datas:
    .byte 0