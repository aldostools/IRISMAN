// Defines for PS3 4.53DEX Ported by Joonie
#define strncmp                     0x5103C //DONE
#define strcpy                      0x50FE8 //DONE
#define strlen                      0x51010 //DONE 
#define alloc                       0x66948 //DONE
#define free                        0x66D84 //DONE

#define memory_patch_func           0x2B83E8 //DONE
//#define pathdup_from_user           0x1b1dc4
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x81128 //DONE
#define memset                      0x50E3C //DONE

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3D90
#define BASE2        (0x3D90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

