// Defines for PS3 4.30 DEX from Miralatijera
#define strncmp                     0x52034
#define strcpy                      0x51FE0
#define strlen                      0x52008 
#define alloc                       0x67940 //0x663F4 BAD
#define free                        0x67D7C 

#define memory_patch_func           0x2DAE74
// #define pathdup_from_user           0x1B1DCC
#define open_mapping_table_ext      0x7fff00  

/* Common Symbols PL3 */

#define memcpy                      0x82120
#define memset                      0x51E34

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3d90
#define BASE2        (0x3d90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)
//#define UMOUNT_DATAS 0x108
