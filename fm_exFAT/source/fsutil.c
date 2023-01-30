//fsutil.c

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <sysutil/osk.h>
#include <sysutil/sysutil.h>
#include <sys/memory.h>
#include <ppu-lv2.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <lv2/sysfs.h>

#include "ff.h"
#include "fflib.h"
#include "fm.h"
#include "fsutil.h"
#include "console.h"

//*****************************************************************************
//NTFS
#include "ntfs.h"

static const DISC_INTERFACE *disc_ntfs[8]= {
    &__io_ntfs_usb000,
    &__io_ntfs_usb001,
    &__io_ntfs_usb002,
    &__io_ntfs_usb003,
    &__io_ntfs_usb004,
    &__io_ntfs_usb005,
    &__io_ntfs_usb006,
    &__io_ntfs_usb007
};
// mounts from /dev_usb000 to 007
static ntfs_md *mounts[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
//
static int ntfs_scan_path (struct fm_panel *p);
static int ntfs_job_scan (struct fm_job *p, char *path);
#define ext_scan_path	ntfs_scan_path
#define ext_job_scan	ntfs_job_scan
//-----------------------------------------------------------------------------

//sys fs
#define FS_S_IFMT 0170000
#define FS_S_IFDIR 0040000
#define DT_DIR 1

struct fs_root {
    u64 devid;
    char *fs;
    int fs_type;
    int fs_idx;
    int fs_parts;
};

static struct fs_root rootfs[] = {
    //ntfs needs index 0 to 7, move SYS at the end
	{0x010300000000000AULL, NULL, FS_TNONE, -1, -1},
    {0x010300000000000BULL, NULL, FS_TNONE, -1, -1},
    {0x010300000000000CULL, NULL, FS_TNONE, -1, -1},
    {0x010300000000000DULL, NULL, FS_TNONE, -1, -1},
	{0x010300000000000EULL, NULL, FS_TNONE, -1, -1},
    {0x010300000000000FULL, NULL, FS_TNONE, -1, -1},
    {0x010300000000001FULL, NULL, FS_TNONE, -1, -1},
    {0x0103000000000020ULL, NULL, FS_TNONE, -1, -1},
    {0x0ULL,             "sys:/", FS_TSYS, 0, 0},
};
//fs type counters
static int fs_fat_k = 0;
static int fs_ext_k = 0;
static int fs_ntfs_k = 0;

static int sys_scan_path (struct fm_panel *p);
static int fat_scan_path (struct fm_panel *p);

static int sys_job_scan (struct fm_job *p, char *path);
static int fat_job_scan (struct fm_job *p, char *path);
/*
sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
sprintf(filename, "/dev_usb00%c/", 47+find_device);
sysFsGetFreeSize(filename, &blockSize, &freeSize);
if(find_device==11) sprintf(filename, "/dev_bdvd");

//dir
	sysFSDirent dir;
    DIR_ITER *pdir = NULL;

    if(is_ntfs) {pdir = ps3ntfs_diropen(path); if(pdir) ret = 0; else ret = -1; }
    else ret=sysLv2FsOpenDir(path, &dfd);

    while ((!is_ntfs && !sysLv2FsReadDir(dfd, &dir, &read))
        || (is_ntfs && ps3ntfs_dirnext(pdir, dir.d_name, &st) == 0)) {

	if(is_ntfs) ps3ntfs_dirclose(pdir); else sysLv2FsCloseDir(dfd);
//file
    ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
                        ret = sysLv2FsOpen(temp_buffer, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0);
            if(flags & CPY_FILE1_IS_NTFS) {fd = ps3ntfs_open(path, O_RDONLY, 0);if(fd < 0) ret = -1; else ret = 0;}

                    fd2 = ps3ntfs_open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0);if(fd2 < 0) ret = -1; else ret = 0;
                    ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0);
                    sysLv2FsChmod(path2, FS_S_IFMT | 0777);

                {ret = ps3ntfs_write(v->fd, v->mem, v->size); v->readed = (u64) ret; if(ret>0) ret = 0;}
            else ret=sysLv2FsWrite(v->fd, v->mem, v->size, &v->readed);

            if(flags & ASYNC_NTFS) ps3ntfs_close(v->fd); else sysLv2FsClose(v->fd);

 */
// np is normalized path offset, from path fat0:/abc/ means np is 3 such that norm path is path + 3 -> 0:/abc/
int fs_get_fstype (char *path, int *np)
{
    if (!path)
        return FS_TNONE;

    // fix paths with /..
    char *up = strstr(path, "/..");
    if(up) {*up = 0; up = strrchr(path, '/'); if(up) *up = 0; return FS_TRETURN;}

    //FAT/ExFAT path
    if (strncmp (path, "fat", 3) == 0)
    {
        if (np)
            *np = 3;
        return FS_TFAT;
    }
    //EXT path
    else if (strncmp (path, "ext", 3) == 0)
    {
        if (np)
            *np = 0; //ext and ntfs need this prefix
        return FS_TEXT;
    }
    //NTFS path
    else if (strncmp (path, "ntfs", 4) == 0)
    {
        if (np)
            *np = 0; //ext and ntfs need this prefix
        return FS_TNTFS;
    }
    //sys path
    else if (strncmp (path, "sys:", 4) == 0)
    {
        if (np)
            *np = 4;
        return FS_TSYS;
    }
    //
    return FS_TNONE;
}

int fs_job_scan (struct fm_job *job)
{
    if (!job->spath)
    {
        job->stype = FS_TNONE;
        return -1;
    }
    int npo = 0;
    job->stype = fs_get_fstype (job->spath, &npo);
    switch (job->stype)
    {
        //scan FAT/ExFAT path
        //if (strncmp (job->spath, "fat", 3) == 0)
        case FS_TFAT:
        {
            FATFS fs;     /* Ponter to the filesystem object */
            char *nsrc = job->spath + npo;
            int res = f_mount (&fs, nsrc, 0);                    /* Mount the default drive */
            if (res != FR_OK)
            {
                NPrintf ("!job:failed mounting fat path %s, res %d\n", job->spath, res);
                return res;
            }
            NPrintf ("job:scanning fat path %s\n", job->spath);
            res = fat_job_scan (job, nsrc);
            f_mount (NULL, nsrc, 0);                    /* UnMount the default drive */
            return res;
        }
        //scan EXT path
        //else if (strncmp (job->spath, "ext", 3) == 0)
        case FS_TEXT:
        {
            return ext_job_scan (job, job->spath);
        }
        //scan NTFS path
        //else if (strncmp (job->spath, "ntfs", 4) == 0)
        case FS_TNTFS:
        {
            return ntfs_job_scan (job, job->spath);
        }
        //scan sys path
        case FS_TSYS:
        {
            NPrintf ("job:scanning sys path %s\n", job->spath);
            return sys_job_scan (job, job->spath + npo);
        }
    }
    return 0;
}

//probe for supported FSs on various devices
static int _attach_fs (int k, char *flag, int *fsk, int fst, char *devfs)
{
    NPrintf ("_attach_fs on dev %d fs %d knt %d\n", k, fst, *fsk);
    rootfs[k].fs = strdup (devfs);
    rootfs[k].fs_type = fst;
    rootfs[k].fs_idx = *fsk;
    //
    int lk = *fsk;
    *fsk = ++lk;
    lk = *flag;
    *flag = ++lk;
    NPrintf ("_attach_fs on dev %d fs %d knt %d\n", k, fst, *fsk);
    //
    return 1;
}

static int _detach_fs (int k, char *flag, int *fsk)
{
    NPrintf ("_detach_fs on dev %d fs %d knt %d\n", k, rootfs[k].fs_type, *fsk);
    rootfs[k].fs_type = FS_TNONE;
    rootfs[k].fs_parts = -1;
    rootfs[k].fs_idx = -1;
    if(rootfs[k].fs) free (rootfs[k].fs);
    rootfs[k].fs = NULL;
    //
    int lk = *fsk;
    *fsk = --lk;
    lk = *flag;
    *flag = ++lk;
    NPrintf ("_detach_fs on dev %d knt %d\n", k, *fsk);
    //
    return 1;
}

int rootfs_probe ()
{
    char devfs[15];
    char flag = 0;  //notify caller that there is a change
    int k, res;
    NPrintf ("rootfs_probe in mounted: %dFAT %dNTFS %dEXT3\n", fs_fat_k, fs_ntfs_k, fs_ext_k);
    //check existing for unplug
    for (k = 0; k < sizeof (rootfs) / sizeof (struct fs_root); k++)
    {
        if (rootfs[k].fs && rootfs[k].devid > 0)
        {
            //are these still plugged in?
            NPrintf ("rootfs_probe probing detach on dev %d fs: %s idx %d\n", k, rootfs[k].fs, rootfs[k].fs_idx);
            //FAT
            if (fs_fat_k && rootfs[k].fs_type == FS_TFAT /*&& !fflib_is_fatfs (rootfs[k].fs + 3)*/)
            {
                char *path = rootfs[k].fs + 3;
                FDIR dir;
                FATFS fs;       /* Work area (filesystem object) for logical drive */
                if (f_mount (&fs, path, 0) == FR_OK)
                {
                    if (f_opendir (&dir, path) == FR_OK)
                    {
                        f_closedir (&dir);
                        f_mount (0, path, 0);
                        continue;
                    }
                    f_mount (0, path, 0);
                }
                NPrintf ("rootfs_probe detach fat%d: on dev %d fs: %s\n", rootfs[k].fs_idx, k, rootfs[k].fs);
                //no longer valid - detach
                fflib_detach (rootfs[k].fs_idx);
                //
                _detach_fs (k, &flag, &fs_fat_k);
                continue;
            }
            //NTFS
            if (fs_ntfs_k && (rootfs[k].fs_type == FS_TNTFS || rootfs[k].fs_type == FS_TEXT))
            {
                if (rootfs[k].fs_parts > 0 && !PS3_NTFS_IsInserted (k))
                {
                    if (mounts[k])
                    {
                        int j;
                        for (j = 0; j < rootfs[k].fs_parts; j++)
                            if ((mounts[k] + j)->name[0])
                            {
                                ntfsUnmount ((mounts[k] + j)->name, true);
                                (mounts[k] + j)->name[0] = 0;
                            }
                        free (mounts[k]);
                        mounts[k] = NULL;
                    }
                    // PS3_NTFS_IsInserted calls PS3_NTFS_Shutdown on failure.. anyway
                    PS3_NTFS_Shutdown (k);
                    NPrintf ("rootfs_probe detach %s on dev %d (%d)\n", rootfs[k].fs, k, rootfs[k].fs_idx);
                    //no longer valid - detach
                    if(rootfs[k].fs_type == FS_TEXT)
                        _detach_fs (k, &flag, &fs_ext_k);
                    else
                        _detach_fs (k, &flag, &fs_ntfs_k);
                    continue;
                } //mount count > 0
            }
        }
    }
    //check new devices
    for (k = 0; k < sizeof (rootfs) / sizeof (struct fs_root); k++)
    {
        if (rootfs[k].fs == NULL && rootfs[k].devid > 0)
        {
            //check for new devices connected
            NPrintf ("rootfs_probe probing attach on %d for 0x%llx or %lld %d\n", k, rootfs[k].devid, rootfs[k].devid, (rootfs[k].devid<0));
            //probe FAT
            res = fflib_attach (fs_fat_k, rootfs[k].devid, 1);
            if (0 == res)
            {
                snprintf (devfs, 14, "fat%d:/", fs_fat_k);
                _attach_fs (k, &flag, &fs_fat_k, FS_TFAT, devfs);
                NPrintf ("rootfs_probe attach fat%d: on dev %d\n", rootfs[k].fs_idx, k);
                //move on, this is taken now
                continue;
            }
            else
            {
                int res2 = fflib_detach (fs_fat_k);
                NPrintf ("!rootfs_probe attach fat%d: on dev %d, res %d, res2 %d\n", fs_fat_k, k, res, res2);
            }
            //probe NTFS
            if (mounts[k] == NULL)
            {
                int ntfsParts = ntfsMountDevice (disc_ntfs[k], &mounts[k], NTFS_DEFAULT | NTFS_RECOVER);
                NPrintf ("rootfs_probe attach ntfs%d: on dev %d, count %d\n", fs_ntfs_k, k, ntfsParts);
                if (ntfsParts > 0)
                {
                    rootfs[k].fs_parts = ntfsParts;
                    //mount ONLY first partition
                    if((mounts[k])->name[0])
                    {
                        snprintf (devfs, 14, "%s:/", (mounts[k])->name);
                        if(!strncmp((mounts[k])->name, "ext", 3))
                            _attach_fs (k, &flag, &fs_ext_k, FS_TEXT, devfs);
                        else
                            _attach_fs (k, &flag, &fs_ntfs_k, FS_TNTFS, devfs);
                        NPrintf ("rootfs_probe attach %s: on dev %d (%d)\n", rootfs[k].fs, k, rootfs[k].fs_idx);
                        #if 0
                        //stat for free space
                        struct statvfs vfs;
                        u64 freeSpace = 0;
                        snprintf (devfs, 14, "/%s:/", (mounts[k])->name);
                        if (!ps3ntfs_statvfs (devfs, &vfs) < 0)
                            freeSpace = (((u64)vfs.f_bsize * vfs.f_bfree));
                        NPrintf ("rootfs_probe attach %s: on dev %d (%d), free %lluMB\n", rootfs[k].fs, k, rootfs[k].fs_idx, freeSpace/MBSZ);
                        #endif
                    }
                    //move on, this is taken now
                    continue;
                }
            }
            //probe EXT
        } //if nothing is mounted on the USB drive
    }
    //
    NPrintf ("rootfs_probe out mounted: %dFAT %dNTFS %dEXT3\n", fs_fat_k, fs_ntfs_k, fs_ext_k);
    //
    return flag;
}

int root_scan_path (struct fm_panel *p)
{
    //probe rootfs devices - done in app update by mount monitoring
    //rootfs_probe ();
    //
    int k;
    for (k = 0; k < sizeof (rootfs) / sizeof (struct fs_root); k++)
    {
        if (rootfs[k].fs)
            fm_panel_add (p, rootfs[k].fs, 1, 0);
    }
    return 0;
}

int fs_path_scan (struct fm_panel *p)
{
    if (!p->path)
    {
        p->fs_type = FS_TNONE;
        return root_scan_path (p);
    }

    p->fs_type = fs_get_fstype (p->path, NULL);
    switch (p->fs_type)
    {
        case FS_TRETURN:
        {
           fm_panel_exit ( p );
           return -1;
        }
        //scan FAT/ExFAT path
        //if (strncmp (p->path, "fat", 3) == 0)
        case FS_TFAT:
        {
            NPrintf ("scanning fat path %s\n", p->path);
            return fat_scan_path (p);
        }
        //scan EXT path
        //else if (strncmp (p->path, "ext", 3) == 0)
        case FS_TEXT:
        {
            return ext_scan_path (p);
        }
        //scan NTFS path
        //else if (strncmp (p->path, "ntfs", 4) == 0)
        case FS_TNTFS:
        {
            return ntfs_scan_path (p);
        }
        //scan sys path
        case FS_TSYS:
        {
            NPrintf ("scanning sys path %s\n", p->path);
            return sys_scan_path (p);
        }
    }
    return 0;
}

static int sys_scan_path (struct fm_panel *p)
{
    if (!p->path) return -1;

    char lp[256];
	int dfd;
	u64 read;
	sysFSDirent dir;
    char *lpath = p->path + 4;  //from 'sys:/' to '/'
    int res = sysLv2FsOpenDir (lpath, &dfd);
    if (res)
    {
        NPrintf ("!failed sysLv2FsOpenDir path %s, res %d\n", p->path, res);
        return 0; //res;
    }
    for (; !sysLv2FsReadDir (dfd, &dir, &read); )
    {
		if (!read)
			break;
		if (!strcmp (dir.d_name, ".")) // || !strcmp (dir.d_name, ".."))
			continue;
        //
        if (dir.d_type & DT_DIR)
        {
            fm_panel_add (p, dir.d_name, 1, 0);
        }
        else
        {
            snprintf (lp, 255, "%s/%s", lpath, dir.d_name);
            sysFSStat stat;
            res = sysLv2FsStat (lp, &stat);
            //NPrintf ("sysLv2FsStat for '%s', res %d\n", lp, res);
            if (res >= 0)
                fm_panel_add (p, dir.d_name, 0, stat.st_size);
            else
                fm_panel_add (p, dir.d_name, 0, -1);
        }
    }
    sysLv2FsCloseDir (dfd);
    //
    return 0;
}

static int fat_scan_path (struct fm_panel *p)
{
    if (!p->path) return -1;

    char lp[256];
    FRESULT res = 0;
    FDIR dir;
    FILINFO fno;
    FATFS fs;     /* Ponter to the filesystem object */
    char *lpath = p->path + 3;
    res = f_mount (&fs, lpath, 0);                    /* Mount the default drive */
    if (res != FR_OK)
        return res;
    //strip the 'fat' preffix on path from 'fat0:/' to '0:/'
    res = f_opendir (&dir, lpath);                       /* Open the directory */
    NPrintf ("scanning fat path %s, res %d\n", lpath, res);
    //
    if (res == FR_OK)
    {
        for (;;)
        {
            FRESULT res1 = f_readdir (&dir, &fno);                   /* Read a directory item */
            if (res1 != FR_OK || fno.fname[0] == 0)
                break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR)
            {                    /* It is a directory */
                fm_panel_add (p, fno.fname, 1, 0);
            }
            else
            {                                       /* It is a file. */
                snprintf (lp, 255, "%s/%s", lpath, fno.fname);
                //NPrintf ("FAT f_stat for '%s', res %d\n", lp, res);
                if (f_stat (lp, &fno) == FR_OK)
                    fm_panel_add (p, fno.fname, 0, fno.fsize);
                else
                    fm_panel_add (p, fno.fname, 0, -1);
            }
        }
        f_closedir (&dir);
    }
    else
    {
        ;//DPrintf("!unable to open path '%s' result %d\n", path, res);
    }
    f_mount (NULL, lpath, 0);                    /* UnMount the default drive */
    //
    return res;
}

static int ntfs_scan_path (struct fm_panel *p)
{
    if (!p->path) return -1;

    char lp[256];
    DIR_ITER *pdir = NULL;
    sysFSDirent dir;
    struct stat st;
    //strip the 'fat' preffix on path from 'fat0:/' to '0:/'
    pdir = ps3ntfs_diropen (p->path);
    NPrintf ("scanning ntfs path %s, res %d\n", p->path, pdir);
    //
    if (pdir)
    {
        for (;;)
        {
            if (ps3ntfs_dirnext (pdir, dir.d_name, &st))
                break;  // Break on error or end of dir
            //skip parent and child listing
            if (!strcmp(dir.d_name, ".")) // || !strcmp(dir.d_name, ".."))
                continue;
            if (S_ISDIR(st.st_mode))
            {
                // It is a directory
                fm_panel_add (p, dir.d_name, 1, 0);
            }
            else
            {
                // It is a file
                snprintf (lp, 255, "%s/%s", p->path, dir.d_name);
                //NPrintf ("FAT f_stat for '%s', res %d\n", lp, res);
                if (ps3ntfs_stat (lp, &st) < 0)
                    fm_panel_add (p, dir.d_name, 0, -1);
                else
                    fm_panel_add (p, dir.d_name, 0, st.st_size);
            }
        }
        ps3ntfs_dirclose (pdir);
    }
    else
    {
        ;//DPrintf("!unable to open path '%s' result %d\n", path, res);
    }
    //
    return 0;
}

static int sys_job_scan (struct fm_job *p, char *path)
{
    if (!path) return -1;

    char lp[256];
    int dfd;
    u64 read;
    sysFSDirent dir;
    sysFSStat stat;
    int res = sysLv2FsOpenDir (path, &dfd);
    if (res)
    {
        //NPrintf ("!failed sysLv2FsOpenDir path %s, res %d\n", path, res);
        //add file
        res = sysLv2FsStat (path, &stat);
        //NPrintf ("sysLv2FsStat for '%s', res %d\n", lp, res);
        if (res >= 0)
            //fm_job_add (p, dir.d_name, 0, stat.st_size);
            fm_job_add (p, path, 0, stat.st_size);
        return res;
    }
    //add dir
    fm_job_add (p, path, 1, 0);
    //
    for (; !sysLv2FsReadDir (dfd, &dir, &read); )
    {
        if (!read)
            break;
        if (!strcmp (dir.d_name, ".") || !strcmp (dir.d_name, ".."))
            continue;
        //
        snprintf (lp, 255, "%s/%s", path, dir.d_name);
        if (dir.d_type & DT_DIR)
        {
            //fm_job_add (p, dir.d_name, 1, 0);
            //fm_job_add (p, lp, 1, 0);
            //recurse
            sys_job_scan (p, lp);
        }
        else
        {
            res = sysLv2FsStat (lp, &stat);
            //NPrintf ("sysLv2FsStat for '%s', res %d\n", lp, res);
            if (res >= 0)
                //fm_job_add (p, dir.d_name, 0, stat.st_size);
                fm_job_add (p, lp, 0, stat.st_size);
            else
                //fm_job_add (p, dir.d_name, 0, -1);
                fm_job_add (p, lp, 0, -1);
        }
    }
    sysLv2FsCloseDir (dfd);
    //
    return 0;
}

int fat_job_scan (struct fm_job *p, char *path)
{
    if (!path) return -1;

    char lp[256];
    FRESULT res;
    FDIR dir;
    FILINFO fno;
#if 1
    //file or dir?
    res = f_stat (path, &fno);
    if (res != FR_OK)
    {
        NPrintf ("!job:f_stat fat path %s, res %d\n", path, res);
        return res;
    }
    if (fno.fattrib & AM_DIR)
    {
        //add dir
        fm_job_add (p, path, 1, 0);
    }
    else
    {
        //add file
        fm_job_add (p, path, 0, fno.fsize);
        f_mount (NULL, path, 0);                    /* UnMount the default drive */
        //
        return res;
    }
#endif
    //open dir
    res = f_opendir (&dir, path);                       /* Open the directory */
    NPrintf ("job:scanning fat path %s, res %d\n", path, res);
    //
    if (res == FR_OK)
    {
        for (;;)
        {
            FRESULT res1 = f_readdir (&dir, &fno);                   /* Read a directory item */
            if (res1 != FR_OK || fno.fname[0] == 0)
            {
                //NPrintf ("job:done f_readdir fat path %s, res1 %d\n", path, res1);
                break;  /* Break on error or end of dir */
            }
            snprintf (lp, 255, "%s/%s", path, fno.fname);
            //NPrintf ("job:processing %s\n", lp);
            if (fno.fattrib & AM_DIR)
            {                    /* It is a directory */
                //fm_job_add (p, fno.fname, 1, 0);
                //fm_job_add (p, lp, 1, 0);
                //recurse
                fat_job_scan (p, lp);
            }
            else
            {                                       /* It is a file. */
                //NPrintf ("FAT f_stat for '%s', res %d\n", lp, res);
                if (f_stat (lp, &fno) == FR_OK)
                    //fm_job_add (p, fno.fname, 0, fno.fsize);
                    fm_job_add (p, lp, 0, fno.fsize);
                else
                    //fm_job_add (p, fno.fname, 0, -1);
                    fm_job_add (p, lp, 0, -1);
            }
        }
        f_closedir (&dir);
    }
    else
    {
        NPrintf("job:!unable to open path '%s' result %d\n", path, res);
    }
    //
    return res;
}


static int ntfs_job_scan (struct fm_job *p, char *path)
{
    if (!path) return -1;

    char lp[256];
    DIR_ITER *pdir = NULL;
    sysFSDirent dir;
    struct stat st;
    int res;
#if 1
    //file or dir?
    if ((res = ps3ntfs_stat (path, &st)) < 0)
    {
        NPrintf ("!job:ps3ntfs_stat NTFS path %s, res %d\n", path, res);
        return res;
    }
    if (S_ISDIR (st.st_mode))
    {
        //add dir
        fm_job_add (p, path, 1, 0);
    }
    else
    {
        //add file
        fm_job_add (p, path, 0, st.st_size);
        return res;
    }
#endif
    //open dir
    pdir = ps3ntfs_diropen (path);
    NPrintf ("job:scanning NTFS path %s, res %d\n", path, pdir);
    //
    if (pdir)
    {
        for (;;)
        {
            if (ps3ntfs_dirnext (pdir, dir.d_name, &st))
                break;  // Break on error or end of dir
            //skip parent and child listing
            if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
                continue;
            snprintf (lp, 255, "%s/%s", path, dir.d_name);
            if (S_ISDIR (st.st_mode))
            {
                // It is a directory - recurse call
                ntfs_job_scan (p, lp);
            }
            else
            {
                // It is a file
                if (ps3ntfs_stat (lp, &st) < 0)
                    fm_job_add (p, lp, 0, -1);
                else
                    fm_job_add (p, lp, 0, st.st_size);
            }
        }
        ps3ntfs_dirclose (pdir);
    }
    else
    {
        NPrintf("job:!unable to open NTFS path '%s' result %d\n", path, pdir);
    }
    //
    return res;
}
