// Defines for PS3 4.70
#define strncmp                     0x4D868 // bytes matched: 0x80  2C2500004182005089640000892300005560063E7F895800409E00482F800000
#define strcpy                      0x4D814 // bytes matched: 0x80  880400002F800000980300004D9E00207C691B788C0400012F8000009C090001
#define strlen                      0x4D83C // bytes matched: 0x80  7C691B7838600000880900002F8000004D9E00207D234B788C0300012F800000
#define alloc                       0x64820 // bytes matched: 0x80  2C2300007C85237838C000007C641B784D820020E9229EC8E86900004BFFFEB8
#define free                        0x64C5C // bytes matched: 0x50  E9229EC87C85237838C000007C641B78E86900004BFFFBD0F821FF717C0802A6

#define memory_patch_func           0x2975E8 // bytes matched: 0x80*  4BDB62552BA304203FE0800163FF0034409D002CE80100B07FE307B4EB410070
//#define pathdup_from_user           0x1b1dc4
#define open_mapping_table_ext      0x7fff00

/* Common Symbols PL3 */

#define memcpy                      0x7E928 // bytes matched: 0x80  2BA500077C6B1B78419D002C2C2500007C691B784D8200207CA903A688040000
#define memset                      0x4D668 // bytes matched: 0x80  2BA500177C6A1B78419D00242FA500004D9E00207C8023787CA903A6980A0000

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3D90
#define BASE2       (0x3D90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

