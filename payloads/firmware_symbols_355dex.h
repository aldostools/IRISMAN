// Defines for PS3 3.55 DEX
#define strncmp                     0x51F9C //done
#define strcpy                      0x51F48 //done
#define strlen                      0x51F70 //done
#define alloc                       0x64464 //done
#define free                        0x648A0 //done

#define memory_patch_func           0x2C8ABC //done
//#define pathdup_from_user           0x194024 //done
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x8039C //done
#define memset                      0x51D9C //done

#define perm_patch_func             0xEE38  //done
#define perm_var_offset             -0x7B30

#define BASE        0xF590
#define BASE2        (0xF590+0x400)  // 0x4290  // pincha en -> 1B5070 (syscall 838)
