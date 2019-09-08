// Defines for PS3 4.60DEH - Porting by Alexander's thanks getsymbol aldostools & IDA PRO for Verify
#define strncmp                     0x515F8 
#define strcpy                      0x515A4 
#define strlen                      0x515CC 
#define alloc                       0x68550 
#define free                        0x6898C 

#define memory_patch_func           0x2D148C 
//#define pathdup_from_user           0x1b1dc4
#define open_mapping_table_ext      0x7fff00

/* Common Symbols PL3 */

#define memcpy                      0x8300C 
#define memset                      0x513F8 

#define perm_patch_func             0x3560 
#define perm_var_offset             -0x7FF8

#define BASE        0x3D90
#define BASE2        (0x3D90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)
