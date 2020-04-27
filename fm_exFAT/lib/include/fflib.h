//fflib.h

#include "types.h"

#define MAXFDS  128

//initialize the fatfs managed lib
int fflib_init();

/*
attach a drive number/idx to a system device id

FatFS uses drive numbers like 0:/, 1:/, 2:/.. etc
this allows attaching a system device id to a drive number
if you want to probe for FAT/ExFAT FS on the drive, use 'now'
e.g. fflib_attach (0, 0x010300000000000AULL, 1);
this will assign drive 0 to system device 0x010300000000000AULL and probe for FAT/ExFAT
the assignment will remain even if a FAT/ExFAT FS is not found, use fflib_detach to de-assign
*/
int fflib_attach(int idx, u64 id, int now);

//detach a drive number/idx from a system device
int fflib_detach(int idx);

//get system device id assigned to the drive number/idx
u64 fflib_id_get(int idx);

//get file descriptor associated to a drive number/idx
int fflib_fd_get(int idx);

//get sector size associated to a drive number/idx
int fflib_ss_get(int idx);

//set file descriptor associated to a drive number/idx - this shouldn't be used in app
int fflib_fd_set(int idx, int fd);

//set sector size associated to a drive number/idx - this shouldn't be used in app
int fflib_ss_set(int idx, int ss);

//returns f_stat on a path: FR_OK or,
//FR_NO_FILE Could not find the file in the directory.
//FR_NO_PATH Could not find the path. A directory in the path name could not be found.
//or other errors according to f_stat
int fflib_is_fatfs (char *path);

int fflib_file_to_sectors(const char *path, uint32_t *sec_out, uint32_t *size_out, int max, int phys);
