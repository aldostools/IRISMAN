// Defines for PS3 4.21 DEX
#define strncmp                     0x51FF8 //done
#define strcpy                      0x51FA4 //done
#define strlen                      0x51FCC //done
#define alloc                       0x677F0 //done
#define free                        0x67C2C //done

#define memory_patch_func           0x2D9740 //done
#define pathdup_from_user           0x1B7D78 //done
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x81FF4 //done
#define memset                      0x51DF8 //done

#define perm_patch_func             0x3560 //done
#define perm_var_offset             -0x7FF8

#define BASE        0x3d90
#define BASE2        (0x3d90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

