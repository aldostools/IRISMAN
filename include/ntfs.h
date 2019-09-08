/**
 * ntfs.h - Simple functionality for startup, mounting and unmounting of NTFS-based devices.
 *
 * Copyright (c) 2013 Estwald
 * Copyright (c) 2010 Dimok
 * Copyright (c) 2009 Rhys "Shareese" Koedijk
 * Copyright (c) 2006 Michael "Chishm" Chisholm
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LIBNTFS_H
#define _LIBNTFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <ppu-lv2.h>
#include <stdio.h>

#include <fcntl.h> // file flags
#include <sys/syslimits.h>

#ifndef ATTRIBUTE_ALIGN
# define ATTRIBUTE_ALIGN(v)				__attribute__((aligned(v)))
#endif
#ifndef ATTRIBUTE_PACKED
# define ATTRIBUTE_PACKED				__attribute__((packed))
#endif

// disc_io.h
#define FEATURE_MEDIUM_CANREAD      0x00000001
#define FEATURE_MEDIUM_CANWRITE     0x00000002
#define FEATURE_PS3_SD              0x00000100
#define FEATURE_PS3_USB             0x00000200
#define FEATURE_PS3_BDVD            0x00000400

typedef uint32_t sec_t;

typedef bool (* FN_MEDIUM_STARTUP)(void) ;
typedef bool (* FN_MEDIUM_ISINSERTED)(void) ;
typedef bool (* FN_MEDIUM_READSECTORS)(sec_t sector, sec_t numSectors, void* buffer) ;
typedef bool (* FN_MEDIUM_WRITESECTORS)(sec_t sector, sec_t numSectors, const void* buffer) ;
typedef bool (* FN_MEDIUM_CLEARSTATUS)(void) ;
typedef bool (* FN_MEDIUM_SHUTDOWN)(void) ;

struct DISC_INTERFACE_STRUCT {
	unsigned long			ioType ;
	unsigned long			features ;
	FN_MEDIUM_STARTUP		startup ;
	FN_MEDIUM_ISINSERTED	isInserted ;
	FN_MEDIUM_READSECTORS	readSectors ;
	FN_MEDIUM_WRITESECTORS	writeSectors ;
	FN_MEDIUM_CLEARSTATUS	clearStatus ;
	FN_MEDIUM_SHUTDOWN		shutdown ;
} ;

typedef struct DISC_INTERFACE_STRUCT DISC_INTERFACE;


/* NTFS errno values */
#define ENOPART                         3000 /* No partition was found */
#define EINVALPART                      3001 /* Specified partition is invalid or not supported */
#define EDIRTY                          3002 /* Volume is dirty and NTFS_RECOVER was not specified during mount */
#define EHIBERNATED                     3003 /* Volume is hibernated and NTFS_IGNORE_HIBERFILE was not specified during mount */

/* NTFS cache options */
#define CACHE_DEFAULT_PAGE_COUNT        128  /* The default number of pages in the cache */
#define CACHE_DEFAULT_PAGE_SIZE         8    /* The default number of sectors per cache page */

/* NTFS mount flags */
#define NTFS_DEFAULT                    0x00000000 /* Standard mount, expects a clean, non-hibernated volume */
#define NTFS_SHOW_HIDDEN_FILES          0x00000001 /* Display hidden files when enumerating directories */
#define NTFS_SHOW_SYSTEM_FILES          0x00000002 /* Display system files when enumerating directories */
#define NTFS_UPDATE_ACCESS_TIMES        0x00000004 /* Update file and directory access times */
#define NTFS_RECOVER                    0x00000008 /* Reset $LogFile if dirty (i.e. from unclean disconnect) */
#define NTFS_IGNORE_HIBERFILE           0x00000010 /* Mount even if volume is hibernated */
#define NTFS_READ_ONLY                  0x00000020 /* Mount in read only mode */
#define NTFS_IGNORE_CASE                0x00000040 /* Ignore case sensitivity. Everything must be and  will be provided in lowercase. */
#define NTFS_SU                         NTFS_SHOW_HIDDEN_FILES | NTFS_SHOW_SYSTEM_FILES
#define NTFS_FORCE                      NTFS_RECOVER | NTFS_IGNORE_HIBERFILE

/**
 * ntfs_md - NTFS mount descriptor
 */
typedef struct _ntfs_md {
    char name[32];                      /* Mount name (can be accessed as "name:/") */
    const DISC_INTERFACE *interface;    /* Block device containing the mounted partition */
    sec_t startSector;                  /* Local block address to first sector of partition */
} ntfs_md;

/**
 * Find all NTFS partitions on a block device.
 *
 * @param INTERFACE The block device to search
 * @param PARTITIONS (out) A pointer to receive the array of partition start sectors
 *
 * @return The number of entries in PARTITIONS or -1 if an error occurred (see errno)
 * @note The caller is responsible for freeing PARTITIONS when finished with it
 */
extern int ntfsFindPartitions (const DISC_INTERFACE *interface, sec_t **partitions);

/**
 * Mount all NTFS partitions on all inserted block devices.
 *
 * @param MOUNTS (out) A pointer to receive the array of mount descriptors
 * @param FLAGS Additional mounting flags. (see above)
 *
 * @return The number of entries in MOUNTS or -1 if an error occurred (see errno)
 * @note The caller is responsible for freeing MOUNTS when finished with it
 * @note All device caches are setup using default values (see above)
 */
extern int ntfsMountAll (ntfs_md **mounts, u32 flags);

/**
 * Mount all NTFS partitions on a block devices.
 *
 * @param INTERFACE The block device to mount.
 * @param MOUNTS (out) A pointer to receive the array of mount descriptors
 * @param FLAGS Additional mounting flags. (see above)
 *
 * @return The number of entries in MOUNTS or -1 if an error occurred (see errno)
 * @note The caller is responsible for freeing MOUNTS when finished with it
 * @note The device cache is setup using default values (see above)
 */
extern int ntfsMountDevice (const DISC_INTERFACE* interface, ntfs_md **mounts, u32 flags);

/**
 * Mount a NTFS partition from a specific sector on a block device.
 *
 * @param NAME The name to mount the device under (can then be accessed as "NAME:/")
 * @param INTERFACE The block device to mount
 * @param STARTSECTOR The sector the partition begins at (see @ntfsFindPartitions)
 * @param CACHEPAGECOUNT The total number of pages in the device cache
 * @param CACHEPAGESIZE The number of sectors per cache page
 * @param FLAGS Additional mounting flags (see above)
 *
 * @return True if mount was successful, false if no partition was found or an error occurred (see errno)
 * @note ntfsFindPartitions should be used first to locate the partitions start sector
 */
extern bool ntfsMount (const char *name, const DISC_INTERFACE *interface, sec_t startSector, u32 cachePageCount, u32 cachePageSize, u32 flags);

/**
 * Unmount a NTFS partition.
 *
 * @param NAME The name of mount used in ntfsMountSimple() and ntfsMount()
 * @param FORCE If true unmount even if the device is busy (may lead to data lose)
 */
extern void ntfsUnmount (const char *name, bool force);

/**
 * Get the volume name of a mounted NTFS partition.
 *
 * @param NAME The name of mount (see @ntfsMountAll, @ntfsMountDevice, and @ntfsMount)
 *
 * @return The volumes name if successful or NULL if an error occurred (see errno)
 */
extern const char *ntfsGetVolumeName (const char *name);

/**
 * Set the volume name of a mounted NTFS partition.
 *
 * @param NAME The name of mount (see @ntfsMountAll, @ntfsMountDevice, and @ntfsMount)
 * @param VOLUMENAME The new volume name
 *
 * @return True if mount was successful, false if an error occurred (see errno)
 * @note The mount must be write-enabled else this will fail
 */
extern bool ntfsSetVolumeName (const char *name, const char *volumeName);

// file operations

int ps3ntfs_open(const char *path, int flags, int mode);
int ps3ntfs_close(int fd);
int ps3ntfs_write(int fd, const char *ptr, size_t len);
int ps3ntfs_read(int fd, char *ptr, size_t len);
off_t ps3ntfs_seek(int fd, off_t pos, int dir);
s64 ps3ntfs_seek64(int fd, s64 pos, int dir);
int ps3ntfs_fstat(int fd, struct stat *st);
int ps3ntfs_stat(const char *file, struct stat *st);
int ps3ntfs_link(const char *existing, const char  *newLink);
int ps3ntfs_unlink(const char *name);

int ps3ntfs_chdir(const char *name);
int ps3ntfs_rename(const char *oldName, const char *newName);
int ps3ntfs_mkdir(const char *path, int mode);
int ps3ntfs_file_to_sectors(const char *path, uint32_t *sec_out, uint32_t *size_out, int max, int phys);
int ps3ntfs_get_fd_from_FILE(FILE *fp);

typedef struct {
    int device;
    void *dirStruct;
} DIR_ITER;

DIR_ITER*  ps3ntfs_diropen(const char *path);
int ps3ntfs_dirreset(DIR_ITER *dirState);
int ps3ntfs_dirnext(DIR_ITER *dirState, char *filename, struct stat *filestat);
int ps3ntfs_dirclose(DIR_ITER *dirState);

// map file functions to libc open, fopen, ...
void NTFS_init_system_io(void);
void NTFS_deinit_system_io(void);


#ifndef _SYS_STATVFS_H
#define _SYS_STATVFS_H

#define ST_RDONLY 0x0001
#define ST_NOSUID 0x0002

typedef __uint32_t fsblkcnt_t;
typedef __uint32_t fsfilcnt_t;

struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	fsblkcnt_t f_blocks;
	fsblkcnt_t f_bfree;
	fsblkcnt_t f_bavail;
	fsfilcnt_t f_files;
	fsfilcnt_t f_ffree;
	fsfilcnt_t f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
}; 

#endif

int ps3ntfs_statvfs(const char *path, struct statvfs *buf);
int ps3ntfs_ftruncate(int fd, off_t len);
int ps3ntfs_fsync(int fd);
int ps3ntfs_errno(void);

bool PS3_NTFS_IsInserted(int fd); // fd from  0 to 7 (usb000... usb007)
bool PS3_NTFS_Shutdown(int fd);   // fd from  0 to 7 (usb000... usb007)

extern const DISC_INTERFACE __io_ntfs_usb000;
extern const DISC_INTERFACE __io_ntfs_usb001;
extern const DISC_INTERFACE __io_ntfs_usb002;
extern const DISC_INTERFACE __io_ntfs_usb003;
extern const DISC_INTERFACE __io_ntfs_usb004;
extern const DISC_INTERFACE __io_ntfs_usb005;
extern const DISC_INTERFACE __io_ntfs_usb006;
extern const DISC_INTERFACE __io_ntfs_usb007;


#ifdef __cplusplus
}
#endif

#endif /* _LIBNTFS_H */
