# modified by Hermes to add /app_home/PS3_GAME /see readme.txt) and the shortcut to launch bluray from here
# added syscall 8, perm mode, relocatable code....

#define CFW_341 1

#ifndef CFW_341
#define USB_MODULE
#endif

#define SYS8_VERSION 0x308
#define PERM_MODE 2  // 0 -> restricted 1-> original 2-> new for some games

// kernel 3.41 address

#define memcpy 0x7c01c
#define memset 0x4d144
#define strcpy 0x4d2f0
#define strncmp 0x4d344
#define strlen 0x4D318
#define unk_4E81C 0x4E81C
#define memcmp 0x4C454

#define alloc 0x62088
#define free 0x624c8
#define add_kernel_module 0xd22d8
#define unk_D292C 0xD292C
#define unk_D2998 0xD2998
#define unk_D29C4 0xD29C4
#define strdup 0x1B3B3C
#define hook_open_out (0x2AAFF4)

#define SYSCALL_TABLE 0x2eb128
#define USER_DATA 0x800000000033E720


/* Addressing Macros */
// Payload bases


#define DESTINATION  0x7FF000

#define BASE (DESTINATION-(resident_code-destination_code))

// Absolute branching
#define ABS(target) ((target) - (BASE) - .)

// Dynamic macros to load a label into a register
#define MEM_BASE(dest) \
	li      dest, 1; \
	rldicr  dest, dest, 63, 0;
#define LOAD_LABEL(dest, source, address) \
	oris	dest, source, ((BASE) + (address))@h; \
	ori		dest, dest, ((BASE) + (address))@l;

#define LOADI_LABEL(dest, address) LOAD_LABEL(dest, dest, address)

// For loading an absolute value
#define LOAD_ABS(dest, source, address)  \
	oris	dest, source, ((address))@h; \
	ori		dest, dest, ((address))@l;

#define LOADI_ABS(dest, address) LOAD_ABS(dest, dest, address)
// Absolute .quads
#define QUAD_ABS(address) .quad 0x8000000000000000 + BASE + address


/* Patch Table Macros */
#define PATCH_INST(offset, instruction...) .long offset; instruction
#define PATCH_DATA(offset, data...) .long offset; .long data
#define PATCH_BRANCH(offset, op, target) .long offset; op (. + (target - offset))

.org 0x0

destination_code:

#ifndef CFW_341

 USB_desc:       
	.byte 9, 2, 0x12, 0, 1, 0, 0, 0x80, 0xFA, 9, 4, 0, 0, 0, 0xFE, 1, 2, 0, 0, 0, 0, 0, 0, 0, 0xFA, 0xCE, 0xB0, 3, 0xAA, 0xBB, 0xCC, 0xDD
#endif


# copy code and jump to main
 exploit_entry: 


#ifndef CFW_341
	addi    %r3, %r3, -0x1000	#t_u64* src_addr_r3 = start section ROM Addr	
#else
    b       exploit_entry2
 
    .byte 'H', 'V', '5','D'
 
 exploit_entry2:

#endif
	li      %r5, 0x1000		#t_u64 size_bytes_r5 = 0x1000 / 4096
	li      %r4, 1			
	rldicr  %r4, %r4, 63,0
	oris	%r4, %r4, (BASE)@h // dest addr
	ori	    %r4, %r4, (BASE)@l
		
 copy_to_dest:                             
	
	addi    %r5, %r5, -8
	ldx     %r6, %r3, %r5
	stdx    %r6, %r4, %r5
	cmpldi  %r5, 0
	bne     copy_to_dest
	addi    %r4, %r4, (exploit_main)@l  # load exploit_main addr
	mtctr   %r4		 # jump dest_addr exploit main
	bctr


 exploit_main:

	mflr    %r0
	stdu    %sp, -0xA0(%sp)
	std     %r27, 0x78(%sp)
	std     %r28, 0x80(%sp)
	std     %r29, 0x88(%sp)
	std     %r30, 0x90(%sp)
	std     %r31, 0x98(%sp)
	std     %r0, 0xB0(%sp)
	MEM_BASE(%r31)
      
#ifdef USB_MODULE	  
	mr      %r3, %r31
	LOADI_LABEL(%r3, memory_patch_amod) # 0x800050B3C
	
	bl      ABS(add_kernel_module)
       
#endif             
	mr      %r4, %r31
	LOADI_ABS(%r4, SYSCALL_TABLE)

	# syscall 36 
	mr      %r3, %r31
	LOADI_LABEL(%r3, syscall36_desc)
	std     %r3, 36*8(%r4)
    
	mr      %r5, %r31
	LOADI_LABEL(%r5, patch_table_data)

 patch_lv2:

	lwz     %r3, 0(%r5)
	cmplwi  %r3, 0
	beq     zero_payload
	lwz     %r4, 4(%r5)
	add     %r3, %r3, %r31
	stw     %r4, 0(%r3)
	addi    %r5, %r5, 8
	b        patch_lv2

// memset 0 from  BASE to DESTINATION

 zero_payload:

	ld	    %r27, 0x78(%sp)
	ld	    %r28, 0x80(%sp)
	ld	    %r29, 0x88(%sp)
	ld  	%r30, 0x90(%sp)
	ld	    %r31, 0x98(%sp)
	ld	    %r0,  0xb0(%sp)
	addi	%sp, %sp, 0xa0
	mtlr	%r0
	
	MEM_BASE(%r3)
	oris	%r3, %r3, (BASE)@h
	ori	    %r3, %r3, (BASE)@l
	li	    %r4, 0
	li	    %r5, (resident_code)@l
	b	    ABS(memset)


# ---------------------------------------------------------------------------

# 00700150

 patch_table_data:

	PATCH_INST(0x0490e0, ld %r4, 0xf08(%r2))
	PATCH_INST(0x0490e4, ld %r3, 0x20(%r28))
	PATCH_INST(0x0490e8, std %r3, 0(%r4))


	PATCH_BRANCH(0x04f0a8, bl, (BASE+(some_additional_patching_on_the_fly-destination_code))) // memory_patching_desc
	PATCH_BRANCH(/*0x2aafc8*/ 0x2AAFF0, b, (BASE+(vsh_hook_open-destination_code))) // vsh_open_hook_desc
	PATCH_INST(0x04ed18, li %r4, 0)
	PATCH_INST(0x04ed1c, stw %r4, 0(%r3))
	PATCH_INST(0x04ed20, blr)
	PATCH_DATA(0x3ba890, 0x01000000)
	
####### WARNING! This patch cause problems with games as F1 2010 with USB devices ########

	#PATCH_INST(0x0505d0, li %r3, 1)
	#PATCH_INST(0x0505d4, blr)

####### New patch to avoid problems with games as F1 2010 with USB devices ########

	PATCH_BRANCH(0x0505d0, b, (BASE+(perm_routine-destination_code)))
	PATCH_BRANCH(0x234D0, bl, (BASE+(perm0_routine-destination_code)))
	PATCH_BRANCH(0xE83D4, bl, (BASE+(perm0_routine-destination_code)))

##########################################################################################

	 
	PATCH_BRANCH(0x4F290, b, 0x4F264) // Skip 0x80010019 error in kernel (version>3.41). You need change de PARAM.SFO to <= 3.41. Only for test (not useful)

	PATCH_INST(0x57398, nop) // Skip 0x8001003D error (not PARAM.SFO or not disc?) for some PKG from games
	
	PATCH_BRANCH(0x57408, b, 0x574A8) //  update patch from mathieulh, modified by Hermes to avoid 0x80010009 error in Uncharted 2 update
	PATCH_INST(0x5745C, li %r31, 0) // from mathieulh, skip 0x8001003E error (different PARAM.SFO?)
	

// syscalls patch
#ifndef CFW_341
	PATCH_INST(0x17CBC, ld %r3, 0(%r3)) // syscall 6
	PATCH_INST(0x17CC0, blr)
	PATCH_INST(0x17CC8, std %r4, 0(%r3)) // syscall 7
	PATCH_INST(0x17CCC, blr)
#else
    PATCH_INST(0x17CE0, lis %r3, -0x7FFF) // restore syscall 9 (used to call this payload)
    PATCH_INST(0x17CE4, ori %r3, %r3, 3)
#endif
	PATCH_BRANCH(0x17CD4, b, (BASE+(syscall8-destination_code))) // syscall 8
    PATCH_BRANCH(0x1BB004, b, (BASE+(lv2_open-destination_code))) // syscall 0x801
    PATCH_BRANCH(0x1B61F0, b, (BASE+(lv2_read-destination_code))) // syscall 0x802

	PATCH_INST(0x50B3C, li      %r3, 1) # use this patch if you move the payload resident code to other address 
	PATCH_INST(0x50B40, blr)
		
# 007001C8
	.long 0


########################################################################################################################################################

	.quad 0

 resident_code:

 return1:
	li	    %r3, 1
    blr
		 
 return0:
	li      %r3, 0
    blr

# ---------------------------------------------------------------------------


# 00050B4C 
 syscall36_desc:
	QUAD_ABS(Syscall_36_activate_virtual_bluray_drive)
	.quad USER_DATA

#ifdef USB_MODULE
 memory_patch_amod:	
	QUAD_ABS(aMod)
	QUAD_ABS(probe)
	QUAD_ABS(initialize)
	QUAD_ABS(disconnect)         
 probe:                 
	QUAD_ABS(kmod_func1) # 50BD4 - struct func {function_ptr, = kmod_func1
        .quad USER_DATA #                      user_data?}

 initialize:
	QUAD_ABS(kmod_func2) # 50C1C - struct func {function_ptr, = kmod_func2
        .quad USER_DATA #                      user_data?}
 disconnect:
	QUAD_ABS(kmod_func3) # 50C78 - struct func {function_ptr, = kmod_func3
        .quad USER_DATA #                      user_data?}
 initialized_flag:
        QUAD_ABS(set_initialized_flag) # 50C84 - ptr to set_initialized_flag (unused)
        .quad USER_DATA #                      user_data?}




# ---------------------------------------------------------------------------

#00050BD4


 kmod_func1:

	stdu    %sp, -0x80(%sp)
	mflr    %r0
	std     %r0, 0x90(%sp)
	li      %r4, 0
	li      %r5, 1
	bl      ABS(unk_D2998)
	lwz     %r5, 8(%r3)
	li      %r3, 0
	lis     %r4, 0xAAAA # 0xAAAAC0DE
	ori     %r4, %r4, 0xC0DE # 0xAAAAC0DE
	cmplw   %r4, %r5
	beq     kmod_func1_ret
	li      %r3, -1

 kmod_func1_ret:

	extsw   %r3, %r3
	ld      %r0, 0x90(%sp)
	mtlr    %r0
	addi    %sp, %sp, 0x80
	blr

# ---------------------------------------------------------------------------
# 00050C1C
 
 kmod_func2:
	stdu    %sp, -0x80(%sp)
	mflr    %r0
	std     %r0, 0x90(%sp)
	li      %r4, 0
	bl      ABS(unk_D29C4)
	addi    %r4, %sp, 0x70
	li      %r5, 0
	std     %r5, 0(%r4)
	li      %r6, 0x21AA
	sth     %r6, 0(%r4)
	li      %r6, 0
	sth     %r6, 6(%r4)
	MEM_BASE(%r6)
	LOADI_LABEL(%r6, initialized_flag)
	li      %r7, 0
	bl      ABS(unk_D292C)
	li      %r3, 0
	ld      %r0, 0x90(%sp)
	mtlr    %r0
	addi    %sp, %sp, 0x80
	blr

# ---------------------------------------------------------------------------

# 00050C78 
 kmod_func3:

	li      %r3, 0
	blr

    # li      %r11, 0xFF
    # sc	1
 
# ---------------------------------------------------------------------------
# 00050C84

set_initialized_flag:

	cmpwi   %r3, 0
	bne     locret_50CA4
	 
	MEM_BASE(%r3)
	LOADI_LABEL(%r3, GAME_NAME_PTR)
	li      %r4, 1
	stw     %r4, 0x10(%r3)

 locret_50CA4:

        blr

#endif

# ---------------------------------------------------------------------------
# 00050CA8

Syscall_36_activate_virtual_bluray_drive:

	stdu    %sp, -0xD0(%sp)
	mflr    %r0
	std     %r0, 0xE0(%sp)
	std     %r31, 0xC8(%sp)
    
    mr      %r31, %r3
    
    MEM_BASE(%r4)
	LOADI_LABEL(%r4, aDev_bdvd); // test if you are using "/dev_bdvd" exactly
	li      %r5, 10
	bl      ABS(strncmp)
	cmpldi  %r3, 0
	bne     syscall36_continue
    bl      _sys8_free_redirection
    b       syscall36_exit

 syscall36_continue:
    mr      %r3, %r31
	addi    %r4, %sp, 0x70
	bl      ABS(strdup)
	MEM_BASE(%r31)

// new for enable skip_end_table_3
	li	    %r4, 2
	LOAD_LABEL(%r3, %r31, skip_end_table_3)
	stw	    %r4, 0(%r3)
	
	LOADI_LABEL(%r31, GAME_NAME_PTR)
	ld      %r3, 0(%r31)
	cmpldi   %r3, 0
	beq     loc_50CE4
	li      %r4, 0x27
	bl      ABS(free)

 loc_50CE4:
   
	li      %r4, 0x27
	li      %r3, 0x800
	bl      ABS(alloc)
	std     %r3, 0(%r31)
	ld      %r4, 0x70(%sp)
	bl      ABS(strcpy)
	ld      %r3, 0x70(%sp)
	li      %r4, 0x27
	bl      ABS(free)
	ld      %r3, 0(%r31)
	bl      ABS(strlen)
	ld      %r4, 0(%r31)
	add     %r3, %r4, %r3
	std     %r3, 8(%r31)

 syscall36_exit:
	li      %r3, 0
	ld      %r31, 0xC8(%sp)
	ld      %r0, 0xE0(%sp)
	addi    %sp, %sp, 0xD0
	mtlr    %r0
	blr

# ---------------------------------------------------------------------------

 lv2_open:

	stdu    %sp, -0xA0(%sp)
	mflr    %r0

	std     %r26, 0x70(%sp)
	std     %r27, 0x78(%sp)
	std     %r28, 0x80(%sp)
	std     %r29, 0x88(%sp)
	std     %r30, 0x90(%sp)
	std     %r31, 0x98(%sp)
	std     %r0, 0xB0(%sp)

	mr	    %r26, %r3
	mr	    %r28, %r5

    stdu    %sp, -0x1b0(%sp)
	bl	    ABS(0x1BB008)               // call real lv2_open

    mr	    %r27, %r3

    cmpldi   %r3, 0
    bne     lv2_open_skip1

	MEM_BASE(%r31)

    LOAD_LABEL(%r3, %r31, GAME_NAME_PTR) // test if /dev_bdvd is re directed
	ld      %r3, 0(%r3)
	cmpldi   %r3, 0
	beq     lv2_open_skip1
    
    mr	    %r3, %r26
	LOAD_LABEL(%r4, %r31, astring1);    // compare for bdvd PARAM.SFO
	li      %r5, 29
	bl      ABS(strncmp)
	cmplwi  %r3, 0
	beq     lv2_open_ok

    mr	    %r3, %r26
	LOAD_LABEL(%r4, %r31, astring2);    // compare for app_home PARAM.SFO
	li      %r5, 29
	bl      ABS(strncmp)
	cmplwi  %r3, 0
	bne     lv2_open_skip1

 lv2_open_ok:
    lwz     %r3, 0(%r28)
    LOAD_LABEL(%r4, %r31, file_fd1);    // save file fd for PARAM.SFO
    stw     %r3, 0(%r4)

 lv2_open_skip1:
	
	mr	    %r3, %r27 
	mr	    %r5, %r28

	ld     %r26, 0x70(%sp)
	ld     %r27, 0x78(%sp)
	ld     %r28, 0x80(%sp)
	ld     %r29, 0x88(%sp)
	ld     %r30, 0x90(%sp)
	ld     %r31, 0x98(%sp)
	ld     %r0, 0xB0(%sp)
	addi    %sp, %sp, 0xA0
	mtlr    %r0
	blr

# ---------------------------------------------------------------------------

 lv2_read:

	stdu    %sp, -0xA0(%sp)
	mflr    %r0

	std     %r26, 0x70(%sp)
	std     %r27, 0x78(%sp)
	std     %r28, 0x80(%sp)
	std     %r29, 0x88(%sp)
	std     %r30, 0x90(%sp)
	std     %r31, 0x98(%sp)
	std     %r0, 0xB0(%sp)

	mr	    %r26, %r3
	mr  	%r27, %r4
	
    stdu    %sp, -0x110(%sp)
	bl	    ABS(0x1B61F4)               // real lv2_read
    mr	    %r30, %r3

	MEM_BASE(%r31)

    LOAD_LABEL(%r4, %r31, file_fd1);
    lwz     %r5, 0(%r4)
    cmplw   %r26, %r5                   // compare file fd for PARAM.SFO
    bne     lv2_read_skip1
    lis     %r5, -1                     // erase file fd (only a time per lv2_open)
    stw     %r5, 0(%r4)

    mr	    %r3, %r27
	LOAD_LABEL(%r4, %r31, acompare1);   // compare for PARAM.SFO header ID
	li      %r5, 4
	bl      ABS(memcmp)
	cmplwi  %r3, 0
	bne     lv2_read_skip1

//  parse in memory for PS3_SYSTEM_VER

    li      %r3, 0x8
    lwbrx   %r28, %r3, %r27
    li      %r3, 0xc
    lwbrx   %r26, %r3, %r27
    li      %r29, 0x1c
    add     %r28, %r28, %r27

 lv2_read_skip0:
    add     %r4, %r30, %r27
    cmpld   %r4, %r28
    bgt     lv2_read_skip1              // compare memory limits

    lbz     %r3, 0(%r28)
    cmplwi  %r3, 0
    beq     lv2_read_skip1

    LOAD_LABEL(%r4, %r31, patch_app_ver); // test for APP_VER patch
    lwz     %r3, 0(%r4)
    cmplwi  %r3, 0
    beq     lv2_read_skip00
    mr      %r3, %r28
    li      %r5, 8
	LOAD_LABEL(%r4, %r31, acompare3);   // compare for APP_VER
    bl      ABS(strncmp)
    cmplwi  %r3, 0
	bne     lv2_read_skip00

    add     %r3, %r26, %r27
    add     %r4, %r30, %r27
    cmpld   %r4, %r3
    bgt     lv2_read_skip1              // compare memory limits

    li      %r4, 57 // '9' put version 9
    stb     %r4, 1(%r3)
    b       lv2_read_skip01

lv2_read_skip00:
    mr      %r3, %r28
    li      %r5, 15
	LOAD_LABEL(%r4, %r31, acompare2);   // compare for PS3_SYSTEM_VER
    bl      ABS(strncmp)
    cmplwi  %r3, 0
	bne     lv2_read_skip01

    //b sys8_panic
    add     %r3, %r26, %r27
    add     %r4, %r30, %r27
    cmpld   %r4, %r3
    bgt     lv2_read_skip1              // compare memory limits

    LOAD_LABEL(%r4, %r31, aversion);    // compare if PS3_SYSTEM_VER > 03.4100
    li      %r5, 7
    bl      ABS(strncmp)
    cmpwi   %r3, 0
	ble     lv2_read_skip1


    add     %r3, %r26, %r27

    LOAD_LABEL(%r4, %r31, aversion);
    li      %r5, 7
    bl      ABS(memcpy)                 // patch PS3_SYSTEM_VER to 03.4100
    b       lv2_read_skip1

 lv2_read_skip01:
    lbzu    %r3, 1(%r28)
    cmplwi  %r3, 0
	bne     lv2_read_skip01             // increase to next string
    addi    %r28, %r28, 1
    
    lwbrx   %r4, %r29, %r27
    addi    %r29, %r29, 16
    add     %r26, %r26, %r4             // get position for string value

    b       lv2_read_skip0    

 lv2_read_skip1:

	mr	    %r3, %r30

	ld      %r26, 0x70(%sp)
	ld      %r27, 0x78(%sp)
	ld      %r28, 0x80(%sp)
	ld      %r29, 0x88(%sp)
	ld      %r30, 0x90(%sp)
	ld      %r31, 0x98(%sp)
	ld      %r0, 0xB0(%sp)
	addi    %sp, %sp, 0xA0
	mtlr    %r0
	blr


 vsh_hook_open:
	/*
	// skipped by new jump

	stdu    %sp, -0xA0(%sp)
	mflr    %r0
	std     %r28, 0x80(%sp)
	std     %r29, 0x88(%sp)
	std     %r31, 0x98(%sp)
	std     %r26, 0x70(%sp)
	std     %r27, 0x78(%sp)
	std     %r0, 0xB0(%sp)
	mr      %r28, %r4
	mr      %r29, %r3
	*/

    cmpldi	%r3, 0
	beq	    proceed
        
    lbz	    %r4, 0(%r3)
	cmpwi	%r4, '/'
	bne	    end_test_bar

 test_bar:
	lbzu	%r4, 1(%r3)
	cmpwi	%r4, '/'
	beq	    test_bar
	subi	%r3, %r3, 1

 end_test_bar:
	mr	    %r29, %r3

	MEM_BASE(%r31)

	LOAD_LABEL(%r27, %r31, str_open_cmp)
	ld	    %r26, 0(%r27)
	cmpldi	%r26, 0
	beq	    hook_open1

 _b_hook_open00:

	mr      %r3, %r29
	ld	    %r4, 0(%r26)  // get compare path
	cmpldi	%r4, 0
	beq	    hook_open1
	
	lwz	    %r5, 16(%r26) // get len of compare path
	bl      ABS(strncmp)

	
	cmpldi	%r3, 0
	bne	    _b_hook_open01

	lwz	    %r5, 16(%r26) 
	add	    %r4, %r29, %r5 // &open_path[compare_path_len]

	ld	    %r3, 8(%r26) // get new path
	mr      %r29, %r3    // changes open_path
	lwz	    %r5, 20(%r26) // get len of new path

	add	    %r3, %r3, %r5

	bl	    ABS(strcpy)
	b	    proceed

 _b_hook_open01:

	addi	%r26, %r26, 24
	b	    _b_hook_open00
		
 hook_open1:
	LOAD_LABEL(%r4, %r31, strAppHome)
	li	    %r5, 9
	bl	    ABS(strncmp)
	cmpldi	%r3, 0
	bne	    hook_open_bdvd

	LOAD_LABEL(%r26, %r31, GAME_NAME_PTR2)
	ld	    %r27, 0x00(%r26)
	cmpldi	%r27, 0
	bne	    vsh_open_noalloc
	li	    %r4, 0x27
	li	    %r3, 0x800
	bl	    ABS(alloc)
	std	    %r3, 0x00(%r26);
	mr	    %r27, %r3
	
	LOAD_LABEL(%r4, %r31, strDevUSB)
	bl		ABS(strcpy)

/*******************************************************************/	

 vsh_open_noalloc:

	addi	%r3, %r27, 11
	LOAD_LABEL(%r5, %r31, GAME_NAME_PTR)
	ld      %r4, 0(%r5)
	cmpldi  %r4, 0
	beq     skip_usb1

	mr	    %r27, %r4
	ld      %r3, 8(%r5)

 skip_usb1:	

	addi	%r4, %r29, 9
	bl	    ABS(strcpy)
	mr	    %r29, %r27
	beq     proceed

 hook_open_bdvd:

	mr      %r3, %r29
	mr      %r4, %r31
	LOADI_LABEL(%r4, aDev_bdvd);
	li      %r5, 9
	bl      ABS(strncmp)
	cmpldi  %r3, 0
	bne     proceed


	LOADI_LABEL(%r31, GAME_NAME_PTR)

	ld      %r3, 0(%r31)
	cmpldi  %r3, 0
	beq     proceed
	ld      %r3, 8(%r31)
	addi    %r4, %r29, 9
	bl      ABS(strcpy)
	ld      %r29, 0(%r31)

	
 proceed:

	mr      %r4, %r28
    mr      %r3, %r29
	bl	    ABS(strlen)
    b       ABS(hook_open_out)

# ---------------------------------------------------------------------------
# 00050DBC

 some_additional_patching_on_the_fly:

	mflr    %r0
	stdu    %sp, -0x1A0(%sp)
	std     %r27, 0x78(%sp)
	std     %r28, 0x80(%sp)
	std     %r29, 0x88(%sp)
	std     %r30, 0x90(%sp)
	std     %r31, 0x98(%sp)
	std     %r0, 0x1B0(%sp)
	mr      %r29, %r3
	mr      %r30, %r4
	MEM_BASE(%r31)
	ld      %r28, -0x6A00(%rtoc)
	ld      %r28, 0x68(%r28)
	ld      %r28, 0x18(%r28)
	ld      %r27, 0xF08(%rtoc)
	ld      %r9, 0x18(%r29)
	lwz     %r9, 0x30(%r9)
	rldicl  %r9, %r9, 48,16
	cmpwi   %r9, 0x29
	bne     loc_50E64
	ld      %r4, 0x10(%r28)
	rldicr  %r5, %r4, 24,39
	rldicl  %r5, %r5, 8,56
	cmpwi   %r5, 0xFF
	beq     loc_50E38
	ori     %r4, %r4, 3
	std     %r4, 0x10(%r28)
	li      %r3, 6
	stw     %r3, 0(%r30)
	b       loc_50E48

 loc_50E38:
	ori     %r4, %r4, 2
	std     %r4, 0x10(%r28)
	li      %r3, 0x2C
	stw     %r3, 0(%r30)

 loc_50E48:
	lwz     %r5, 4(%r28)
	ld      %r4, 8(%r28)
	ld      %r3, 0(%r27)
	add     %r9, %r3, %r5
	std     %r9, 0(%r27)
	bl      ABS(memcpy)
	b       loc_50F24

 loc_50E64:

	mr      %r3, %r29
	mr      %r4, %r30
	bl      ABS(unk_4E81C)
	mr      %r29, %r31
	LOADI_LABEL(%r29, patch_flag)
	lwz     %r3, 0(%r29)
	lwz     %r5, 4(%r28)
	add     %r3, %r3, %r5
	stw     %r3, 0(%r29)
	ld      %r4, 0x10(%r28)
	rldicr  %r5, %r4, 24,39
	rldicl  %r5, %r5, 8,56
	cmpwi   %r5, 0xFF
	bne     loc_50F24
	ld      %r3, 0(%r27)
	li      %r4, 0
	li      %r6, 0

 loc_50EAC:
	add     %r7, %r3, %r4
	lwz     %r5, 0(%r7)
	xor     %r6, %r6, %r5
	addi    %r4, %r4, 4
	cmpldi  %r4, 0x400
	bne     loc_50EAC
	lwz     %r3, 0(%r29)
	rldicr  %r6, %r6, 32,31
	or      %r6, %r6, %r3
	li      %r3, 0
	stw     %r3, 0(%r29)
	mr      %r7, %r31
	LOADI_LABEL(%r7, memory_patch_table) #hermes

 loc_50EE4:

	ld      %r3, 0(%r7)
	cmpldi  %r3, 0
	beq     loc_50F24
	addi    %r7, %r7, 0x10
	cmpld   %r3, %r6
	bne     loc_50EE4
	ld      %r5, -8(%r7)
	ld      %r7, 0(%r27)

loc_50F04:

	lwz     %r3, 0(%r5)
	cmplwi  %r3, 0
	beq     loc_50F24
	cmplwi  %r3, 1  // skip patch  n bytes (it is a trick used to skip conditional patch)
	beq	    is_type_1
	cmplwi  %r3, 2  // non skip patch (it is a trick used to skip conditional patch. In this case patch successfull)
	beq	    is_type_2

	lwz     %r4, 4(%r5)
	add     %r3, %r3, %r7
	stw     %r4, 0(%r3)
	addi    %r5, %r5, 8
	b       loc_50F04

is_type_1:
	lwz     %r3, 4(%r5)
	add	    %r5, %r5, %r3
	addi	%r5, %r5, 8 // skip n+8 bytes (non patch)
	b	    loc_50F04
is_type_2:
	addi	%r5, %r5, 8 // skip 8 bytes (patch)
	b	    loc_50F04

loc_50F24:

	li      %r3, 0
	ld      %r27, 0x78(%sp)
	ld      %r28, 0x80(%sp)
	ld      %r29, 0x88(%sp)
	ld      %r30, 0x90(%sp)
	ld      %r31, 0x98(%sp)
	ld      %r0, 0x1B0(%sp)
	addi    %sp, %sp, 0x1A0
	mtlr    %r0
	blr


########################################################################################################################################################

# generic tool main syscall

syscall8:

	mflr    %r0
	stdu    %sp, -0xA0(%sp)
	std     %r27, 0x78(%sp)
	std     %r28, 0x80(%sp)
	std     %r29, 0x88(%sp)
	std     %r30, 0x90(%sp)
	std     %r31, 0x98(%sp)
	std     %r0, 0xB0(%sp)

	MEM_BASE(%r31)

	LOAD_LABEL(%r27, %r31, save_syscall_values)
	
	lwz     %r28, 0(%r27)	   // stealth mode
	cmplwi  %r28, 0x0
	beq	    syscall8_ok	   // if not stealth mode work normally

	cmplwi  %r28, 8
	bge	    syscall8_stealth // if stealth mode >=8 ignore all
	
	cmpldi  %r3, 1	           // test for command == 1
    bne	    syscall8_stealth

	cmpldi  %r4, 0	           // compare if key is 0 to skip count times
    beq	    syscall8_skip_count

	addi	%r28, %r28, 1	  // stealth_mode++ (count times)
	stw     %r28, 0(%r27)

 syscall8_skip_count:

	ld      %r28,  8(%r27)	   // get key and compares if same key
	cmpld	%r4, %r28
    bne	    syscall8_stealth
	li	    %r28, 1
	stw     %r28, 0(%r27) // stealth mode

// syscall8 enabled: system_call_2(8, 1, key)
	
	LOAD_ABS(%r4, %r31, SYSCALL_TABLE)

	ld      %r28,  16(%r27)
	std     %r28,  6*8(%r4) // patch syscall 6 to enable

	ld      %r28,  24(%r27)
	std     %r28,  7*8(%r4) // patch syscall 7 to enable
	
	LOAD_LABEL(%r28, %r31, syscall36_desc)
	std     %r28, 36*8(%r4) // patch syscall 36 to enable

	li	    %r28, 0
	stw     %r28, 0(%r27) // stealth mode off
	
	li	    %r3, SYS8_VERSION
	b	    syscall8_end

 syscall8_ok:

// syscall switch

	andi.   %r3, %r3, 0xff
	cmpldi  %r3, 14
	bge	syscall8_unsupported
	
	LOAD_LABEL(%r27, %r31, syscall8_table)
	sldi 	%r28, %r3, 3
	add 	%r27, %r27, %r28
	
	ld	    %r29, 0(%r27)

// move param registers
	mr      %r3, %r4
	mr      %r4, %r5
	mr      %r5, %r6

// jump to register address with link
	bl	    syscall8_bl

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

# ---------------------------------------------------------------------------

	// system_call_2(8, 0, key): disables the syscalls vectors and fix a key to enable it again using system_call_2(8, 1, key)
 sys8_disable:

	LOAD_LABEL(%r27, %r31, save_syscall_values)
	std     %r3,  8(%r27) // save key

	li	    %r28, 1
	stw     %r28, 0(%r27) // stealth mode

	LOAD_ABS(%r4, %r31, SYSCALL_TABLE)

	ld      %r3, 37*8(%r4) // get unused syscall addr

	ld      %r28, 6*8(%r4)
	std     %r3,  6*8(%r4) // patch syscall 6 to stealth
	std     %r28, 16(%r27)

	ld      %r28, 7*8(%r4)
	std     %r3,  7*8(%r4) // patch syscall 7 to stealth
	std     %r28,  24(%r27)
	

    std     %r3, 36*8(%r4) // patch syscall 36 to stealth

	li	    %r3, 0
	blr

      // dummy for system_call_2(8, 1, key)

 sys8_enable: // system_call_2(8, 1, key) when syscalls are enabled

	li	    %r3, SYS8_VERSION
	blr

 sys8_memcpy: // system_call_4(8, 2, dest, src, len)

	b       ABS(memcpy)

 sys8_memset: // system_call_4(8, 3, dest, dat, len)

	b       ABS(memset)

 sys8_call: // system_call_4(8, 4, addr, param1, param2)

	
	mtctr   %r3
	mr	    %r3, %r4
	mr	    %r4, %r5
	bctr

 sys8_alloc: // system_call_3(8, 5, size, pool)

	b      ABS(alloc)

 sys8_free: // system_call_3(8, 6, ptr, pool)

	b      ABS(free)

 sys8_panic: // system_call_1(8, 7)

	li      %r3, 0
	li      %r11, 0xFF
    sc	    1

 sys8_perm_mode: // system_call_2(8, 8, perm)

	LOAD_LABEL(%r27, %r31, perm_mode)
	stw	    %r3, 0(%r27)
	li	    %r3, 0
	blr

 sys8_open_table:  // system_call_2(8, 9, ptr)

	mr	    %r4, %r3
	LOAD_LABEL(%r27, %r31, str_open_cmp)
	ld	    %r3, 0(%r27)
    std	    %r4, 0(%r27)
	blr

 sys8_sysconfigure: // system_call_1(8, 10, mode) 
    
    li      %r4, 1 // set retail xml
    cmplwi  %r3, 0
    beq     b_004
    li      %r4, 2 // set debug xml
    cmplwi  %r3, 1
    beq     b_004
    cmplwi  %r3, 2  // redirection off
    beq     b_001_free

    li      %r4, 1
    cmplwi  %r3, 3 // switch patch app_ver version in PARAM.SFO
    bne     b_001
    li      %r4, 0
 b_001:

    LOAD_LABEL(%r27, %r31, patch_app_ver); // set APP_VER patch in PARAM.SFO
    lwz     %r3, 0(%r27)
    stw     %r4, 0(%r27)
    
    blr
    
 b_001_free:
    mflr    %r0
	stdu    %sp, -0xA0(%sp)
	std     %r0, 0xB0(%sp)

    bl      _sys8_free_redirection

    ld	    %r0, 0xB0(%sp)
    addi    %sp, %sp, 0xA0
	mtlr    %r0
    li      %r4, 0  // unpatch APP_VER
	b       b_001

b_004:
    LOAD_LABEL(%r27, %r31,  memory_patch_table_3)
    stw	    %r4, 0(%r27)
    LOAD_LABEL(%r27, %r31,  memory_patch_table_4)
    stw	    %r4, 0(%r27)
    li	    %r3, 0
    blr

// free /dev_bdvd, /app_home and sys8_open_table() re directions

_sys8_free_redirection:
    mflr    %r0
	stdu    %sp, -0xA0(%sp)
	std     %r0, 0xB0(%sp)
    std     %r27, 0x78(%sp)
    std     %r31, 0x98(%sp)

    MEM_BASE(%r31)

// free device memory
    LOAD_LABEL(%r27, %r31, GAME_NAME_PTR)
	ld      %r3, 0(%r27)
	cmpldi  %r3, 0
	beq     b_002
	li      %r4, 0x27
	bl      ABS(free)

    li      %r3, 0
    std     %r3, 0(%r27)
    std     %r3, 8(%r27)

b_002:
    LOAD_LABEL(%r27, %r31, GAME_NAME_PTR2)
	ld	    %r3, 0x0(%r27)
    cmpldi  %r3, 0
	beq     b_003
	li      %r4, 0x27
	bl      ABS(free)
    li      %r3, 0
    std     %r3, 0(%r27)
b_003:
    li      %r4, 1
    LOAD_LABEL(%r27, %r31, skip_end_table_3)
    stw     %r4, 0(%r27) 
    LOAD_LABEL(%r27, %r31, str_open_cmp)
   // std     %r3, 0(%r27)  // disable this in Iris Manager
    ld      %r27, 0x78(%sp)
    ld      %r31, 0x98(%sp)
    ld	    %r0, 0xB0(%sp)
    addi    %sp, %sp, 0xA0
	mtlr    %r0
	blr    
    
sys8_lv1call: // system_call_2(8, 11, input_reg_table, output_reg_table) 

    mflr    %r0
	stdu    %sp, -0xA0(%sp)
	std     %r0, 0xB0(%sp)

	std     %r4, 0x60(%sp)

	ld	    %r4, 0x08(%r3)
	ld	    %r5, 0x10(%r3)
	ld	    %r6, 0x18(%r3)
	ld	    %r7, 0x20(%r3)
	ld	    %r8, 0x28(%r3)
	ld	    %r9, 0x30(%r3)
	ld	    %r10,0x38(%r3)
	ld	    %r11,0x40(%r3)
	ld	    %r3, 0x00(%r3)

	sc      1
	
	ld	    %r11, 0x60(%sp)

	std	    %r3, 0x00(%r11)
	std	    %r4, 0x08(%r11)
	std	    %r5, 0x10(%r11)
	std	    %r6, 0x18(%r11)
	std	    %r7, 0x20(%r11)
	std	    %r8, 0x28(%r11)
	std	    %r9, 0x30(%r11)
	std	    %r10,0x38(%r11)
	
	ld	    %r0, 0xB0(%sp)
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
    
########################################################################################################################################################

 perm0_routine:

	MEM_BASE(%r9)
	LOADI_LABEL(%r9, perm_mode)
	lwz	    %r0, 0(%r9)
	cmplwi  %r0, 1
	beq	    return1
	cmplwi  %r0, 2
	beq	    return0
	ld      %r9, -0x6978(%rtoc)
	mflr    %r0
	b	    ABS(0x000505D8)

 perm_routine:
	
	MEM_BASE(%r9)
	LOADI_LABEL(%r9, perm_mode)
	lwz	    %r0, 0(%r9)
	cmplwi  %r0, 0
	bne	    return1
	ld      %r9, -0x6978(%rtoc)
	mflr    %r0
	b	    ABS(0x000505D8)

########################################################################################################################################################	
    .align 3
# 00050F70
 memory_patch_table:

#ifndef CFW_341
	.quad	0xa0556f3d002cb8fd
	QUAD_ABS(memory_patch_table_1)
#endif

	.quad	0x8c0a948c000d99b1
	QUAD_ABS(memory_patch_table_3)
	.quad	0xa2bc1a5600052adc
	QUAD_ABS(memory_patch_table_4)

#ifndef CFW_341
	.quad	0x6b70280200020017
	QUAD_ABS(memory_patch_table_2)
#endif
	.quad	0
    .align 2

#ifndef CFW_341
// vsh.self
 memory_patch_table_1:
	PATCH_INST(0x305354, li %r3, 130)
	PATCH_INST(0x5f3fc0, li %r3, 1)
	PATCH_INST(0x5f3fc4, blr)
	.long 0

// nash_pluging.sprx
 memory_patch_table_2:
	#PATCH_INST(0x2ed0c, li %r29, 1)
	PATCH_INST(0x2eef0, nop) # mathieulh pkg patch
	.long 0
 #endif

 memory_patch_table_3:
    PATCH_DATA(2, 4*8) // conditional patch: if first 4 bytes == 1 skip patch if == 2 patch successfull
	PATCH_DATA(0x0022b888, 0x5f746f6f) # "_tool2.xml#root"
	PATCH_DATA(0x0022b88c, 0x6c322e78)
	PATCH_DATA(0x0022b890, 0x6d6c2372)
	PATCH_DATA(0x0022b894, 0x6f6f7400)

 skip_end_table_3: // conditional patch: if first 4 bytes == 1 skip patch if == 2 patch successfull
        PATCH_DATA(1, 3*8) // skip 3 patchs
		PATCH_DATA(0x2367e8, 0x2f646576) // "/dev"	// replace "/home_apps/PS3_GAME" by "/dev_bdvd/PS3_GAME"
		PATCH_DATA(0x2367ec, 0x5f626476) // "_bdv"
		PATCH_DATA(0x2367f0, 0x642f5053) // "d/PS"

	.long 0

 memory_patch_table_4:
    PATCH_DATA(2, 4*8) // conditional patch: if first 4 bytes == 1 skip patch if == 2 patch successfull
	PATCH_DATA(0x000d68b8, 0x5f746f6f) # "_tool2.xml#root"
	PATCH_DATA(0x000d68bc, 0x6c322e78)
	PATCH_DATA(0x000d68c0, 0x6d6c2372)
	PATCH_DATA(0x000d68c4, 0x6f6f7400)
	.long 0
    .align 3
 GAME_NAME_PTR:  
	.quad 0
 GAME_MOUNTPOINT_PTR:
	.quad 0
    .align 2
 INITIALIZED:
 #ifndef CFW_341
	.long 0
 #else
    .long 1
 #endif

 patch_flag: 
	.long 0
    .align 3
 str_open_cmp:
	.quad 0
 save_syscall_values: // (for stealth)
	.quad 0 // stealth mode
	.quad 0 // key
	.quad 0 // syscall 6
	.quad 0 // syscall 7

 perm_mode:
	.long PERM_MODE

 aDev_bdvd:	
	.string "/dev_bdvd"
 aMod:  
	.string "mod"

astring1:
    .string "/dev_bdvd/PS3_GAME/PARAM.SFO" # 28 + 1
astring2:
    .string "/app_home/PS3_GAME/PARAM.SFO" # 28 + 1
 
file_fd1:
    .long 0xffff0000

acompare1:
    .string "\0PSF"
acompare2:
    .string "PS3_SYSTEM_VER" # 14 + 1
acompare3:
    .string "APP_VER" # 7 + 1
aversion:
    .string "03.4100"
    .align 2
patch_app_ver:
    .long 0
    .align 3
 GAME_NAME_PTR2:  
	.quad 0
 strAppHome:
	.string "/app_home"	
 strDevUSB:
	.string "/dev_usb000"
    .align 3
 syscall8_table:
	QUAD_ABS(sys8_disable);
	QUAD_ABS(sys8_enable);
	QUAD_ABS(sys8_memcpy);
	QUAD_ABS(sys8_memset);
	QUAD_ABS(sys8_call);
	QUAD_ABS(sys8_alloc);
	QUAD_ABS(sys8_free);
	QUAD_ABS(sys8_panic);
	QUAD_ABS(sys8_perm_mode);
	QUAD_ABS(sys8_open_table);
	QUAD_ABS(sys8_sysconfigure);
    QUAD_ABS(sys8_lv1call);
    QUAD_ABS(sys8_instcpy);
    QUAD_ABS(sys8_pokeinst);

 end_dest_code:
	.byte 0

########################################################################################################################################################



	// fill out to 3840 bytes
.org 0xf00
