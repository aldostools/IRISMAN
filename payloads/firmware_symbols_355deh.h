// Defines for PS3 3.55 DEH
#define strncmp                     0x52468 //done
#define strcpy                      0x52414 //done
#define strlen                      0x5243C //done
#define alloc                       0x64930 //done
#define free                        0x64D6C //done

#define memory_patch_func           0x2E321C //done
#define pathdup_from_user           0x194D50 //done
#define open_mapping_table_ext      0x7fff00

/* Common Symbols PL3 */

#define memcpy                      0x80B10 //done
#define memset                      0x52268 //done

#define perm_patch_func             0xEE38  //done
#define perm_var_offset             -0x7B30

#define BASE          0xF590
#define BASE2        (0xF590+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)
