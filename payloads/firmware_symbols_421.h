// Defines for PS3 4.21
#define strncmp                     0x4E708
#define strcpy                      0x4E6B4
#define strlen                      0x4E6DC
#define alloc                       0x63ED8
#define free                        0x64314

#define memory_patch_func           0x2c2580
//#define pathdup_from_user           0x1B1988
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x7DFD0
#define memset                      0x4E508

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE         0x3d90
#define BASE2        (0x3d90+0x400)  // 0x4290  // pincha en (syscall 838)

