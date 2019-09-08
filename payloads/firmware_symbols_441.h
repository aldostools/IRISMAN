// Defines for PS3 4.41
#define strncmp                     0x4D694
#define strcpy                      0x4D640
#define strlen                      0x4D668
#define alloc                       0x62F78
#define free                        0x633B4

#define memory_patch_func           0x2C42B8
//#define pathdup_from_user           0x1b1dc4
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x7D04C
#define memset                      0x4D494

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3D90
#define BASE2        (0x3D90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

