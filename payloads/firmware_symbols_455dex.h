// Defines for PS3 4.55DEX
#define strncmp                     0x51280 //done
#define strcpy                      0x5122C //done
#define strlen                      0x51254 //done
#define alloc                       0x67D84 //done
#define free                        0x681C0 //done

#define memory_patch_func           0x2B9C3C //done
//#define pathdup_from_user           0x1b1dc4
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x82564 //done
#define memset                      0x51080 //done

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3D90
#define BASE2        (0x3D90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

