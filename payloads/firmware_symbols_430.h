// Defines for PS3 4.30
#define strncmp                     0x4e744
#define strcpy                      0x4e6f0
#define strlen                      0x4e718
#define alloc                       0x64028
#define free                        0x64464

#define memory_patch_func           0x2c3cfc
//#define pathdup_from_user           0x1b1dc4
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x7e0fc
#define memset                      0x4e544

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3d90
#define BASE2        (0x3d90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

