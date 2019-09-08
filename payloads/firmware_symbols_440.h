// Defines for PS3 4.40 cex from Miralatijera
#define strncmp                     0x4D690
#define strcpy                      0x4d63c // nein! 0x4D704


#define strlen                      0x4D664
#define alloc                       0x62F74
#define free                        0x633B0

#define memory_patch_func           0x2C42AC
// #define pathdup_from_user           0x1B1DCC
#define open_mapping_table_ext      0x7fff00  


/* Common Symbols PL3 */

#define memcpy                      0x7D048
#define memset                      0x4D490

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3d90
#define BASE2        (0x3d90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

