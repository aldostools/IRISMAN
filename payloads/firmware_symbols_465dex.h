// Defines for PS3 4.65DEX
#define strncmp                     0x51214 // 2C 25 00 00 41 82 00 50 89 64 00 00 89 23 00 00 55 60 06 3E 7F 89 58 00 40 9E 00 48 2F 80 00 00
#define strcpy                      0x511C0 // 88 04 00 00 2F 80 00 00 98 03 00 00 4D 9E 00 20 7C 69 1B 78 8C 04 00 01 2F 80 00 00 9C 09 00 01
#define strlen                      0x511E8 // 7C 69 1B 78 38 60 00 00 88 09 00 00 2F 80 00 00 4D 9E 00 20 7D 23 4B 78 8C 03 00 01 2F 80 00 00
#define alloc                       0x6816C // 2C 23 00 00 7C 85 23 78 38 C0 00 00 7C 64 1B 78
#define free                        0x685A8 // E9 22 9F A8 7C 85 23 78 38 C0 00 00 7C 64 1B 78

#define memory_patch_func           0x2BB038 // 4B D9 61 B1 2B A3 04 20 3F E0 80 01 63 FF 00 34
//#define pathdup_from_user           0x1b1dc4
#define open_mapping_table_ext      0x7fff00

/* Common Symbols PL3 */

#define memcpy                      0x82980 // 2B A5 00 07 7C 6B 1B 78 41 9D 00 2C 2C 25 00 00 7C 69 1B 78 4D 82 00 20 7C A9 03 A6 88 04 00 00
#define memset                      0x51014 // 2B A5 00 17 7C 6A 1B 78 41 9D 00 24 2F A5 00 00 4D 9E 00 20 7C 80 23 78 7C A9 03 A6 98 0A 00 00

#define perm_patch_func             0x3560
#define perm_var_offset             -0x7FF8

#define BASE        0x3D90
#define BASE2       (0x3D90+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)

