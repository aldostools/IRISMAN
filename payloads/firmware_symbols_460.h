// Defines for PS3 4.60
#define strncmp                     0x4D868 //done
#define strcpy                      0x4D814 //done
#define strlen                      0x4D83C //done
#define alloc                       0x64798 //done
#define free                        0x64BD4 //done

#define memory_patch_func           0x2A02E4  //done
//#define pathdup_from_user           0x1b1dc4
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x7E8A0 //done
#define memset                      0x4D668 //done

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3D90
#define BASE2        (0x3D90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

