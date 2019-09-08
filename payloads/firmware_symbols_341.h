// Defines for PS3 3.41
#define strncmp                     0x4d344                                                                                                                                    
#define strcpy                      0x4d2f0                                                                                                                                            
#define strlen                      0x4D318                                                                                                                                            
#define alloc                       0x62088                                                                                                                                            
#define free                        0x624c8

#define memory_patch_func           0x2AAFF0
#define pathdup_from_user           0x1B3B3C                                                                                                                                           
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x7c01c
#define memset                      0x4d144

#define perm_patch_func             0x505d0

#define BASE2        (0x50B3C+0x14)  // (syscall 838)

