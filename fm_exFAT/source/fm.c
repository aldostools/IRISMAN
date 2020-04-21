//fm.c

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <tiny3d.h>
#include <libfont.h>
#include <sys/file.h>
#include <sysutil/msg.h>
#include <sysutil/sysutil.h>

#include "fm.h"
#include "util.h"
#include "fsutil.h"
#include "console.h"
#include "pad.h"

#include "ff.h"
#include "ntfs.h"

//status message
static char *s_msg[STATUS_H] = {NULL, NULL, NULL, NULL};
//status message text color
static int c_msg[STATUS_H] = {-1, -1, -1, -1};
//set status message for intex idx
int fm_status_set (char * sm, int idx, int col)
{
    if (idx < 0 || idx > STATUS_H - 1)
        return -1;
    if (s_msg[idx])
        free (s_msg[idx]);
    if (sm)
        s_msg[idx] = strdup (sm);
    else
        s_msg[idx] = NULL;
    c_msg[idx] = col;
    return 0;
}

int fm_status_draw (int dat)
{
    SetCurrentFont (2);
    SetFontSize (8, 8);
    //title - current path
    SetFontAutoCenter (0);
    int i;
    for (i = 0; i < STATUS_H; i++)
    {
        if (c_msg[i] != -1)
            SetFontColor (c_msg[i], 0x00000000);
        if (s_msg[i])
            DrawString (0, (PANEL_H + i) * 8, s_msg[i]);
    }
    return 0;
}

//enter current dir
int fm_panel_enter (struct fm_panel *p)
{
    int ret;
    //can't enter file
    if (!p->current->dir)
        return -1;
    //move deeper
    char np[CBSIZE];
    if (p->path && *p->path)
        snprintf (np, CBSIZE, "%s/%s", p->path, p->current->name);
    else
        snprintf (np, CBSIZE, "%s", p->current->name);
    ret = fm_panel_scan (p, np);
    if (0 == ret)
    {
        //add to navigation history
        fm_entry_add (&p->history, np, 1, 0);
        NPrintf ("navi add: %s\n", np);
    }
    return ret;
}

//exit current dir
int fm_panel_exit (struct fm_panel *p)
{
    int ret;
    char np[CBSIZE];
    char lp[CBSIZE];
    struct fm_file *list = p->history;
    struct fm_file *current = list;
    *np = 0; *lp = 0;
    //
    if (current)
    {
        // move to the end of the list
        while (current->next != NULL)
            current = current->next;
        //
        NPrintf("navi from [%s] to [%s]\n", current->name, current->prev?current->prev->name:"none");
        //remove last entry
        if (current->name)
        {
            snprintf (lp, CBSIZE, "%s", current->name);
            free (current->name);
        }
        if (current->prev)
        {
            snprintf (np, CBSIZE, "%s", current->prev->name);
            current->prev->next = NULL;
        }
        //was this the last one?
        if (current == list)
            p->history = NULL;
        if(current) free (current);
    }
    //can't return from here on root with no FS
    if (!p->path)
        return -1;
    //
#if 1
    if (*np)
    {
        ret = fm_panel_scan (p, np);
        //select previous item
        if (*lp)
        {
            char *plp = strrchr (lp, '/');
            if (plp && *(plp + 1))
                fm_panel_locate (p, plp + 1);
        }
    }
    else
        ret = fm_panel_scan (p, NULL);
#else
    char *lp = strrchr (p->path, '/');
    if (lp)
    {
        //is this already on root?
        if (*(lp + 1) == '\0')
            //exit to rootfs
            return fm_panel_scan (p, NULL);
        *lp = 0;
    }
    else
        //exit to rootfs
        return fm_panel_scan (p, NULL);
    //
    snprintf (np, CBSIZE, "%s", p->path);
    ret = fm_panel_scan (p, np);
    //
    NPrintf ("panel exit to %s\n", np);
#endif
    //
    return ret;
}

int fm_panel_clear (struct fm_panel *p)
{
    if(!p) return -1;

    struct fm_file *ptr;
    for (ptr = p->entries; ptr != NULL; ptr = p->entries)
    {
        p->entries = ptr->next;
        if (ptr->name)
            free (ptr->name);
        if(ptr) free (ptr);
    }
    p->current = NULL;
    p->current_idx = -1;
    //
    p->files = 0;
    p->dirs = 0;
    p->fsize = 0;
    //
    return 0;
}

int fm_job_clear (struct fm_job *job)
{
    if(!job) return -1;

    struct fm_file *ptr;
    for (ptr = job->entries; ptr != NULL; ptr = job->entries)
    {
        job->entries = ptr->next;
        if (ptr->name)
            free (ptr->name);
        if(ptr) free (ptr);
    }
    //
    if (job->spath)
        free (job->spath);
    job->spath = NULL;
    if (job->dpath)
        free (job->dpath);
    job->dpath = NULL;
    //
    job->files = 0;
    job->dirs = 0;
    job->fsize = 0;
    //
    return 0;
}

int fm_job_list (char *path)
{
    struct fm_job fmjob;
    struct fm_job *job = &fmjob;
    //
    job->spath = strdup (path);
    job->dpath = NULL;
    job->stype = FS_TNONE;
    job->dtype = FS_TNONE;
    //
    job->entries = NULL;
    //
    job->files = 0;
    job->dirs = 0;
    job->fsize = 0;
    //
    fs_job_scan (job);
    char lp[256];
    snprintf (lp, 256, "job scan %dfiles, %ddirs, %llubytes", job->files, job->dirs, job->fsize);
    fm_status_set (lp, 0, 0xeeeeeeFF);
    //
    #if 1
    struct fm_file *ptr;
    for (ptr = job->entries; ptr != NULL; ptr = ptr->next)
    {
        NPrintf ("job %d> %8luB> %s\n", ptr->dir, ptr->size, ptr->name);
    }
    #endif
    //
    fm_job_clear (job);
    //
    return 0;
}

int fm_file_copy (char *src, char *dst, char srct, char dstt, unsigned long long ssz, int (*ui_render)(int dt))
{
    FATFS fs;      /* Work area (filesystem object) for logical drives */
    int fsx = 0;
    BYTE *buffer = NULL;    // File copy buffer
    FIL f1src, f1dst;       // File objects
    int fd3src = -1, fd3dst = -1;
    int ret = 0;
    int f2src = -1, f2dst = -1;
    u64 br, bw;
    char src_ok = 0, dst_ok = 0;
    unsigned long long dsz = 0;
    time_t times, timee;    //start end/time
    char lp[CBSIZE];
    //prep copy buffer
    #define BSZ (3*MBSZ)
    buffer = malloc (BSZ);
    if (buffer == NULL)
    {
        NPrintf ("!failed to allocate buffer of %dbytes\n", BSZ);
        return -1;
    }
    //prepare source
    switch (srct)
    {
        case FS_TFAT:
        {
            f_mount(&fs, src, 0);
            if (f_open (&f1src, src, FA_READ))
            {
                NPrintf ("!fm_file_copy: FAT src open %s\n", src);
                ret = -1;
            }
            else
            {
                src_ok = 1;
                fsx++;
            }
        }
        break;
        case FS_TSYS:
        {
            if (sysFsOpen (src, SYS_O_RDONLY, &f2src, NULL, 0))
            {
                NPrintf ("!fm_file_copy: SYS src open %s\n", src);
	            ret = -1;
            }
            else
                src_ok = 1;
        }
        break;
        case FS_TEXT:
        {
            
        }
        break;
        case FS_TNTFS:
        {
            fd3src = ps3ntfs_open (src, O_RDONLY, 0);
            if (fd3src < 0)
            {
                NPrintf ("!fm_file_copy: NTFS src open %s\n", src);
	            ret = -1;
            }
            else
                src_ok = 1;
        }
        break;
    }
    //errors?
    if (ret)
    {
        if(buffer) free (buffer);
        return ret;
    }
    //prepare destination
    switch (dstt)
    {
        case FS_TFAT:
        {
            if (fsx == 0)
                f_mount(&fs, dst, 0);
            if (f_open (&f1dst, dst, FA_WRITE | FA_CREATE_ALWAYS))
            {
                NPrintf ("!fm_file_copy: FAT dst create %s\n", dst);
                ret = -1;
            }
            else
            {
                dst_ok = 1;
                fsx++;
            }
        }
        break;
        case FS_TSYS:
        {
            if (sysLv2FsOpen (dst, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &f2dst, 0777, NULL, 0))
            {
                NPrintf ("!fm_file_copy: SYS dst create %s\n", dst);
                ret = -1;
            }
            else
                dst_ok = 1;
        }
        break;
        case FS_TEXT:
        {
            
        }
        break;
        case FS_TNTFS:
        {
            fd3dst = ps3ntfs_open (dst, O_WRONLY | O_CREAT | O_TRUNC, 0);
            if (fd3dst < 0)
            {
                NPrintf ("!fm_file_copy: NTFS dst create %s\n", dst);
	            ret = -1;
            }
            else
                dst_ok = 1;
        }
        break;
    }
    //ready to read+write
    ProgressBar2Update (0, NULL);  //reset
    if (src_ok && dst_ok)
    {
        time (&times);
        while (1)
        {
            //read data
            switch (srct)
            {
                case FS_TFAT:
                {
                    UINT lbr;
                    if (f_read (&f1src, buffer, BSZ, &lbr))
                    {
                        NPrintf ("!fm_file_copy: FAT src read %s\n", src);
                        ret = -1;
                    }
                    br = lbr;
                }
                break;
                case FS_TSYS:
                {
                    if (sysLv2FsRead (f2src, buffer, BSZ, &br))
                    {
                        NPrintf ("!fm_file_copy: SYS src read %s\n", src);
                        ret = -1;
                    }
                }
                break;
                case FS_TEXT:
                {
                    
                }
                break;
                case FS_TNTFS:
                {
                    br = ps3ntfs_read (fd3src, (char *)buffer, (size_t)BSZ);
                }
                break;
            }
            if (br == 0)
                NPrintf ("fm_file_copy: read 0B/EOF from %s\n", src);
            if (ret == -1 || br <= 0)
                break;
            //write data
            switch (dstt)
            {
                case FS_TFAT:
                {
                    UINT lbw;
                    if (f_write (&f1dst, buffer, (UINT)br, &lbw))
                    {
                        NPrintf ("!fm_file_copy: FAT dst write %s\n", dst);
                        ret = -1;
                    }
                    bw = lbw;
                }
                break;
                case FS_TSYS:
                {
                    if (sysLv2FsWrite (f2dst, buffer, br, &bw))
                    {
                        NPrintf ("!fm_file_copy: SYS dst write %s\n", dst);
                        ret = -1;
                    }
                }
                break;
                case FS_TEXT:
                {
                    
                }
                break;
                case FS_TNTFS:
                {
                   bw = ps3ntfs_write (fd3dst, (char *)buffer, (size_t) br);
                }
                break;
            }
            if (br != bw)
            {
                NPrintf ("!fm_file_copy: read %lluB wrote %lluB\n", br, bw);
                ret = -1;
            }
            if (ret == -1)
                break;
            //stats
            dsz += bw;
            time (&timee);
            if (timee == times) //avoid fault division by zero
                timee++;
            //performance
            uint mbps = (dsz / MBSZ) / (timee - times);
            //time left
            //xM .. ySEC > tM .. ?SEC >> 
            uint etae = 0;
            if (ssz && mbps)
                etae = (ssz - dsz) / MBSZ / mbps;
            //report stats
            u32 cprc = ssz ? dsz * 100 / ssz : 0;
            snprintf (lp, CBSIZE, "%uMB of %uMB (%u%%) %uMBps %usec left", (uint)(dsz/MBSZ), (uint)(ssz/MBSZ), cprc, mbps, etae);
            fm_status_set (lp, 3, 0xffff00FF);
            //msgDialogProgressBarSetMsg (MSG_PROGRESSBAR_INDEX1, lp);
            ProgressBar2Update (cprc, lp);  //also handles flipping
            //4: 1 = OK, YES; 2 = NO/ESC/CANCEL; -1 = NONE
            if (ProgressBarActionGet() == 2)
            {
                ret = -2;
                break;
            }
            #if 0
            //render the app so we update the user on status
            if (ui_render)
                ui_render (1);
            //cancel job?
            ps3pad_read ();
            if (NPad (BUTTON_CIRCLE))
            {
                ret = -2;
                break;
            }
            #endif
        }//while read
    }//if all ok
    //
    if(buffer) free (buffer);
    //close files
    switch (srct)
    {
        case FS_TFAT:
        {
            if (src_ok)
                f_close (&f1src);
        }
        break;
        case FS_TSYS:
        {
            if (src_ok)
                sysLv2FsClose (f2src);
        }
        break;
        case FS_TEXT:
        {
            
        }
        break;
        case FS_TNTFS:
        {
            if (src_ok)
                ps3ntfs_close (fd3src);
        }
        break;
    }
    switch (dstt)
    {
        case FS_TFAT:
        {
            if (dst_ok)
                f_close (&f1dst);
            //remove file on error
            if (ret)
                f_unlink (dst);
        }
        break;
        case FS_TSYS:
        {
            if (dst_ok)
                sysLv2FsClose (f2dst);
            //remove file on error
            if (ret)
                sysLv2FsUnlink (dst);
        }
        break;
        case FS_TEXT:
        {
            
        }
        break;
        case FS_TNTFS:
        {
            if (dst_ok)
                ps3ntfs_close (fd3dst);
        }
        break;
    }
    //did we use FATfs?
    if (fsx)
    {
        if (srct == FS_TFAT)
            f_mount (0, src, 0);
        else
            f_mount (0, dst, 0);
    }
    //
    return ret;
}

int fm_job_copy (char *src, char *dst, int (*ui_render)(int dt))
{
    struct fm_job fmjob;
    struct fm_job *job = &fmjob;
    int ret = 0;
    //
    if (!src || !dst)
        return -1;
    //
    job->spath = strdup (src);
    job->dpath = strdup (dst);
    job->stype = FS_TNONE;
    job->dtype = FS_TNONE;
    //
    job->entries = NULL;
    //
    job->files = 0;
    job->dirs = 0;
    job->fsize = 0;
    //
    fs_job_scan (job);
    //restore source location
    char *lbp = strrchr (src, '/');
    if (lbp)
        *lbp = 0;
    else
    {
        //we have a HUUUGE problem here, bail out
        fm_job_clear (job);
    }
    if(job->spath) free (job->spath);
    job->spath = strdup (src);
    //
    char lp[CBSIZE];
    char dp[CBSIZE];
    //TODO: check for space at destination
    //SYS
    //sysFsGetFreeSize(temp_buffer, &blockSize, &freeSize);
    //freeSize = (((u64)blockSize * freeSize));
    //NTFS
    //struct statvfs vfs;
    //ps3ntfs_statvfs(temp_buffer, &vfs);
    //freeSize = (((u64)vfs.f_bsize * vfs.f_bfree));
    //FAT
    //
    snprintf (lp, CBSIZE, "Do you want to copy the selected files and folders?\n\n%u items, %uMB", job->files + job->dirs, (uint)(job->fsize/MBSZ));
    if (1 != YesNoDialog (lp))
        return fm_job_clear (job);
    //
    snprintf (lp, CBSIZE, "copy job: %dfiles, %ddirs, %lluMB to %s", job->files, job->dirs, job->fsize/MBSZ, job->dpath);
    fm_status_set (lp, 0, 0xeeeeeeFF);
    DoubleProgressBarDialog (lp);
    //copy file from source to dest
    struct fm_file *ptr;
    int ktr = 1;
    int spp = strlen (job->spath);
    int nsp, ndp;
    //adjust source path prefix to remove FS type like fat, ntfs, ext, sys
    job->stype = fs_get_fstype (src, &nsp);
    if (job->stype > FS_TNONE)
        spp -= nsp;
    job->dtype = fs_get_fstype (dst, &ndp);
    //
    for (ptr = job->entries; ptr != NULL; ptr = ptr->next)
    {
        //prep destination, skip FS prefix
        snprintf (dp, CBSIZE, "%s%s", job->dpath + ndp, ptr->name + spp);
        //get the name only
        lbp = strrchr (dp, '/');
        if (lbp)
            lbp++;
        else
            lbp = na_string;
        //
        if (ptr->dir)
            snprintf (lp, CBSIZE, "task %d/%d create dir %s", ktr, job->files + job->dirs, lbp);
        else
            snprintf (lp, CBSIZE, "task %d/%d copy file to %s", ktr, job->files + job->dirs, lbp);
        fm_status_set (lp, 1, 0xffeeeeFF);
        //3
        //Update message and progress
        //msgDialogProgressBarSetMsg (MSG_PROGRESSBAR_INDEX0, lp);
        //msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, msg);
        ProgressBarUpdate ((u32)(ktr * 100/(job->files + job->dirs)), lp);
        //same file?
        NPrintf ("job %d> %8luB> %s to %s\n", ptr->dir, ptr->size, ptr->name, dp);
        if (strcmp (dp, ptr->name) == 0)
        {
            snprintf (lp, CBSIZE, "job: same file/dir, skip %s", dp);
            fm_status_set (lp, 2, 0xffff00FF);
        }
        else
        {
            if (ptr->dir)
            {
                snprintf (lp, CBSIZE, "create %s", lbp);
                fm_status_set (lp, 2, 0xffffeeFF);
                ProgressBar2Update (0, lp);
                //create dir
                switch (job->dtype)
                {
                    case FS_TFAT:
                    {
                        FATFS fs;      /* Work area (filesystem object) for logical drives */
                        f_mount (&fs, dp, 0);
                        snprintf (lp, CBSIZE, "job: FAT create dir %s", dp);
                        fm_status_set (lp, 2, 0xffffeeFF);
                        //remove file/dir
                        if ((ret = f_mkdir (dp)))
                            NPrintf ("!fm_job_copy: FAT can't create dir %s, res %d\n", dp, ret);
                        //unmount
                        f_mount (0, dp, 0);
                    }
                    break;
                    case FS_TSYS:
                    {
                        if ((ret = sysLv2FsMkdir (dp, 0777)))
                            NPrintf ("!fm_job_copy: SYS can't create dir %s, res %d\n", dp, ret);
                    }
                    break;
                    case FS_TEXT:
                    {
                        
                    }
                    break;
                    case FS_TNTFS:
                    {
                        if ((ret = ps3ntfs_mkdir (dp, 0777)))
                            NPrintf ("!fm_job_copy: SYS can't create dir %s, res %d\n", dp, ret);
                    }
                    break;
                }
                ProgressBar2Update (100, NULL);
            }
            else
            {
                snprintf (lp, CBSIZE, "Copy file to %s", dp);
                fm_status_set (lp, 2, 0xffffeeFF);
                //copy file
                ret = fm_file_copy (ptr->name, dp, job->stype, job->dtype, ptr->size, ui_render);
            }
        }
        if (ret == -2)
        {
            //canceled
            snprintf (lp, CBSIZE, "Copy job CANCELED");
            fm_status_set (lp, 3, 0xffff00FF);
            break;
        }
        //
        ktr++;
        do_flip ();
        //4: 1 = OK, YES; 2 = NO/ESC/CANCEL; -1 = NONE
        if (ProgressBarActionGet() == 2)
            break;
        #if 0
        //render the app so we update the user on status
        if (ui_render)
            ui_render (1);
        //cancel job?
        ps3pad_read ();
        if (NPad (BUTTON_CIRCLE))
        {
            //canceled
            snprintf (lp, CBSIZE, "Copy job CANCELED");
            fm_status_set (lp, 3, 0xffff00FF);
            break;
        }
        #endif
    }
    //
    msgDialogAbort();
    //
    fm_job_clear (job);
    //
    return 0;
}

int fm_job_rename (char *path, char *old, char *new)
{
    int nsp = 0, ret = 0;
    char lp[CBSIZE];
    char op[CBSIZE];
    char np[CBSIZE];
    switch (fs_get_fstype (path, &nsp))
    {
        case FS_TFAT:
        {
            char *npath = path + nsp;
            FATFS fs;      /* Work area (filesystem object) for logical drives */
            f_mount (&fs, npath, 0);
            snprintf (lp, CBSIZE, "job: FAT rename %s to %s in %s", old, new, npath);
            fm_status_set (lp, 2, 0xffffeeFF);
            //build paths
            snprintf (op, CBSIZE, "%s/%s", npath, old);
            snprintf (np, CBSIZE, "%s/%s", npath, new);
            //rename
            if ((ret = f_rename (op, np)))
                NPrintf ("!fm_job_rename: FAT can't rename %s to %s in %s res %d\n", op, np, npath, ret);
            snprintf (lp, CBSIZE, "job: FAT rename %s to %s in %s: %s", old, new, npath, ret?"KO":"OK");
            fm_status_set (lp, 3, ret?0xff0000FF:0x00ff00FF);
            //unmount
            f_mount (0, npath, 0);
        }
        break;
        case FS_TSYS:
        {
            char *npath = path + nsp;
            //
            snprintf (lp, CBSIZE, "job: SYS rename %s to %s in %s", old, new, npath);
            fm_status_set (lp, 2, 0xffffeeFF);
            //build paths
            snprintf (op, CBSIZE, "%s/%s", npath, old);
            snprintf (np, CBSIZE, "%s/%s", npath, new);
            //
            //sysLv2FsChmod (op, FS_S_IFDIR | 0777);
            //sysLv2FsChmod (op, FS_S_IFMT | 0777);
            if ((ret = sysLv2FsRename (op, np)))
                NPrintf ("!fm_job_rename: SYS can't rename %s to %s in %s res %d\n", old, new, npath, ret);
            //
            snprintf (lp, CBSIZE, "job: SYS rename %s to %s in %s: %s", old, new, npath, ret?"KO":"OK");
            fm_status_set (lp, 3, ret?0xff0000FF:0x00ff00FF);
        }
        break;
        case FS_TEXT:
        {
            
        }
        break;
        case FS_TNTFS:
        {
            char *npath = path + nsp;
            //
            snprintf (lp, CBSIZE, "job: NTFS rename %s to %s in %s", old, new, npath);
            fm_status_set (lp, 2, 0xffffeeFF);
            //build paths
            snprintf (op, CBSIZE, "%s/%s", npath, old);
            snprintf (np, CBSIZE, "%s/%s", npath, new);
            //
            if ((ret = ps3ntfs_rename (op, np)))
                NPrintf ("!fm_job_delete: NTFS can't rename %s to %s in %s res %d\n", old, new, npath, ret);
            //
            snprintf (lp, CBSIZE, "job: NTFS rename %s to %s in %s: %s", old, new, npath, ret?"KO":"OK");
            fm_status_set (lp, 3, ret?0xff0000FF:0x00ff00FF);
        }
        break;
    }        //
    return ret;
}

int fm_job_newdir (char *path, char *new)
{
    int nsp = 0, ret = 0;
    char lp[CBSIZE];
    char np[CBSIZE];
    switch (fs_get_fstype (path, &nsp))
    {
        case FS_TFAT:
        {
            char *npath = path + nsp;
            FATFS fs;      /* Work area (filesystem object) for logical drives */
            f_mount (&fs, npath, 0);
            snprintf (lp, CBSIZE, "job: FAT mkdir %s in %s", new, npath);
            fm_status_set (lp, 2, 0xffffeeFF);
            //build paths
            snprintf (np, CBSIZE, "%s/%s", npath, new);
            //rename
            if ((ret = f_mkdir (np)))
                NPrintf ("!fm_job_newdir: FAT can't mkdir %s in %s res %d\n", np, npath, ret);
            snprintf (lp, CBSIZE, "job: FAT mkdir %s in %s: %s", new, npath, ret?"KO":"OK");
            fm_status_set (lp, 3, ret?0xff0000FF:0x00ff00FF);
            //unmount
            f_mount (0, npath, 0);
        }
        break;
        case FS_TSYS:
        {
            char *npath = path + nsp;
            //
            snprintf (lp, CBSIZE, "job: SYS mkdir %s in %s", new, npath);
            fm_status_set (lp, 2, 0xffffeeFF);
            //build paths
            snprintf (np, CBSIZE, "%s/%s", npath, new);
            //
            //sysLv2FsChmod (op, FS_S_IFDIR | 0777);
            //sysLv2FsChmod (op, FS_S_IFMT | 0777);
            if ((ret = sysLv2FsMkdir (np, 0777)))
                NPrintf ("!fm_job_newdir: SYS can't mkdir %s in %s res %d\n", new, npath, ret);
            //
            snprintf (lp, CBSIZE, "job: SYS mkdir %s in %s: %s", new, npath, ret?"KO":"OK");
            fm_status_set (lp, 3, ret?0xff0000FF:0x00ff00FF);
        }
        break;
        case FS_TEXT:
        {
            
        }
        break;
        case FS_TNTFS:
        {
            char *npath = path + nsp;
            //
            snprintf (lp, CBSIZE, "job: NTFS mkdir %s in %s", new, npath);
            fm_status_set (lp, 2, 0xffffeeFF);
            //build paths
            snprintf (np, CBSIZE, "%s/%s", npath, new);
            //
            if ((ret = ps3ntfs_mkdir (np, 0777)))
                NPrintf ("!fm_job_delete: NTFS can't mkdir %s in %s res %d\n", new, npath, ret);
            //
            snprintf (lp, CBSIZE, "job: NTFS mkdir %s in %s: %s", new, npath, ret?"KO":"OK");
            fm_status_set (lp, 3, ret?0xff0000FF:0x00ff00FF);
        }
        break;
    }        //
    return ret;
}

int fm_job_delete (char *src, int (*ui_render)(int dt))
{
    struct fm_job fmjob;
    struct fm_job *job = &fmjob;
    int ret;
    //
    if (!src)
        return -1;
    //
    job->spath = strdup (src);
    job->dpath = NULL;
    job->stype = FS_TNONE;
    job->dtype = FS_TNONE;
    //
    job->entries = NULL;
    //
    job->files = 0;
    job->dirs = 0;
    job->fsize = 0;
    //
    fs_job_scan (job);
    //restore source location
    char *lbp = strrchr (src, '/');
    if (lbp)
        *lbp = 0;
    else
    {
        //we have a HUUUGE problem here, bail out
        fm_job_clear (job);
    }
    if(job->spath) free (job->spath);
    job->spath = strdup (src);
    //
    char lp[CBSIZE];
    //remove files - 2do: show dialog box for confirmation
    snprintf (lp, CBSIZE, "Do you want to delete the selected Files and Folders?\n\n%u items, %uMB", job->files + job->dirs, (uint)(job->fsize/MBSZ));
    if (1 != YesNoDialog(lp))
        return fm_job_clear (job);
    snprintf (lp, CBSIZE, "delete job: %dfiles, %ddirs, %lluMB from %s", job->files, job->dirs, job->fsize/MBSZ, job->spath);
    fm_status_set (lp, 0, 0xeeeeeeFF);
    DoubleProgressBarDialog (lp);
    //
    struct fm_file *ptr, *ptail = NULL;
    int ktr = 1;
    int nsp;
    //adjust source path prefix to remove FS type like fat, ntfs, ext, sys
    job->stype = fs_get_fstype (src, &nsp);
    //find the last entry and move backwards such that we remove files first, then remove dirs
    for (ptr = job->entries; ptr != NULL; ptr = ptr->next)
        ptail = ptr;
    //reverse removal
    for (ptr = ptail; ptr != NULL; ptr = ptr->prev)
    {
        lbp = strrchr (ptr->name, '/'); //file/dir name
        if (lbp)
            lbp++;
        else
            lbp = na_string;
        if (ptr->dir)
            snprintf (lp, CBSIZE, "task %d/%d delete dir %s", ktr, job->files + job->dirs, lbp);
        else
            snprintf (lp, CBSIZE, "task %d/%d delete file %s", ktr, job->files + job->dirs, lbp);
        fm_status_set (lp, 1, 0xffeeeeFF);
        ProgressBarUpdate ((u32)(ktr * 100/(job->files + job->dirs)), lp);
        NPrintf ("%s\n", lp);
        ProgressBar2Update (0, lbp);  //reset
        //
        switch (job->stype)
        {
            case FS_TFAT:
            {
                FATFS fs;      /* Work area (filesystem object) for logical drives */
                f_mount (&fs, ptr->name, 0);
                snprintf (lp, CBSIZE, "job: FAT delete file/dir %s", ptr->name);
                fm_status_set (lp, 2, 0xffffeeFF);
                //remove file/dir
                if ((ret = f_unlink (ptr->name)))
                    NPrintf ("!fm_job_delete: FAT can't remove file/dir %s, res %d\n", ptr->name, ret);
                //unmount
                f_mount (0, ptr->name, 0);
            }
            break;
            case FS_TSYS:
            {
                snprintf (lp, CBSIZE, "job: SYS delete file/dir %s", ptr->name);
                fm_status_set (lp, 2, 0xffffeeFF);
                //
                if (ptr->dir)
                {
                    sysLv2FsChmod (ptr->name, FS_S_IFDIR | 0777);
                    if ((ret = sysLv2FsRmdir (ptr->name)))
                        NPrintf ("!fm_job_delete: SYS can't remove dir %s, res %d\n", ptr->name, ret);
                }
                else
                {
                    sysLv2FsChmod (ptr->name, FS_S_IFMT | 0777);
                    if ((ret = sysLv2FsUnlink (ptr->name)))
                        NPrintf ("!fm_job_delete: SYS can't remove file %s, res %d\n", ptr->name, ret);
                }
                snprintf (lp, CBSIZE, "job: SYS delete file/dir %s - %s", ptr->name, ret?"KO":"OK");
                fm_status_set (lp, 3, ret?0xff0000FF:0x00ff00FF);
            }
            break;
            case FS_TEXT:
            {
                
            }
            break;
            case FS_TNTFS:
            {
                snprintf (lp, CBSIZE, "job: NTFS delete file/dir %s", ptr->name);
                fm_status_set (lp, 2, 0xffffeeFF);
                //
                if ((ret = ps3ntfs_unlink (ptr->name)))
                    NPrintf ("!fm_job_delete: NTFS can't remove file %s, res %d\n", ptr->name, ret);
                //
                snprintf (lp, CBSIZE, "job: NTFS delete file/dir %s - %s", ptr->name, ret?"KO":"OK");
                fm_status_set (lp, 3, ret?0xff0000FF:0x00ff00FF);
            }
            break;
        }        //
        ProgressBar2Update (100, NULL);  //reset and flip
        //
        ktr++;
        //4: 1 = OK, YES; 2 = NO/ESC/CANCEL; -1 = NONE
        if (ProgressBarActionGet() == 2)
            break;
        #if 0
        //render the app so we update the user on status
        if (ui_render)
            ui_render (1);
        //cancel job?
        ps3pad_read ();
        if (NPad (BUTTON_CIRCLE))
        {
            //canceled
            snprintf (lp, CBSIZE, "delete job CANCELED");
            fm_status_set (lp, 3, 0xffff00FF);
            break;
        }
        #endif
    }
    //
    msgDialogAbort();
    //
    fm_job_clear (job);
    //
    return 0;
}

int fm_entry_add (struct fm_file **entries, char *fn, char dir, unsigned long fsz)
{
    // Allocate memory for new node;
    struct fm_file *list = *entries;
    struct fm_file *link = (struct fm_file*) malloc (sizeof (struct fm_file));
    if (!link)
    {
        NPrintf ("!fm_job_add malloc failed for %d>%s\n", dir, fn);
        return -1;
    }
    //
    link->name = strdup (fn);
    link->dir = dir;
    link->size = fsz;
    //
    link->prev = NULL;
    link->next = NULL;
    // If head is empty, create new list
    if (list == NULL)
    {
        *entries = link;
    }
    else
    {
        struct fm_file *current = list;
        // move to the end of the list
        while (current->next != NULL)
            current = current->next;
        // Insert link at the end of the list
        current->next = link;
        link->prev = current;
    }
    return 0;
}

int fm_entry_pull (struct fm_file **entries)
{
    struct fm_file *list = *entries;
    struct fm_file *current = list;
    if (current)
    {
        // move to the end of the list
        while (current->next != NULL)
            current = current->next;
        //remove last entry
        if (current->name)
            free (current->name);
        current->prev->next = NULL;
        //was this the last one?
        if (current == list)
            *entries = NULL;
        if(current) free (current);
    }
    return 0;
}

int fm_job_add (struct fm_job *p, char *fn, char dir, unsigned long fsz)
{
    if (0 == fm_entry_add (&p->entries, fn, dir, fsz))
    {
        //
        NPrintf ("fm_job_add %d>%s\n", dir, fn);
        //stats
        if (fsz > 0)
            p->fsize += fsz;
        if (dir)
            p->dirs++;
        else
            p->files++;
        //
        return 0;
    }
    return -1;
}

int fm_panel_reload (struct fm_panel *p)
{
    if(!p) return -1;

    //cleanup
    fm_panel_clear (p);
    //
    return fs_path_scan (p);
}

int fm_panel_scan (struct fm_panel *p, char *path)
{
    if(!p) return -1;

    if (p->path)
        free (p->path);
    if (path)
        p->path = strdup (path);
    else
        p->path = NULL;
    //cleanup
    fm_panel_clear (p);
    //
    return fs_path_scan (p);
}

int fm_panel_init (struct fm_panel *p, int x, int y, int w, int h, char act)
{
    p->x = x;
    p->y = y;
    p->w = w;
    p->h = h;
    //
    p->active = act;
    p->entries = NULL;
    p->history = NULL;
    p->current = NULL;
    p->current_idx = -1;
    p->path = NULL;
    //
    p->files = 0;
    p->dirs = 0;
    p->fsize = 0;
    p->fs_type = FS_TNONE;
    //
    return 0;
}

int fm_panel_draw (struct fm_panel *p)
{
    static char fname[53];
    int wh = p->h/8 - 2;    //scroll rows: panel height - 2 rows
    int se = 0;             //skipped entries
    //
    if (p->active == TRUE)
        DrawRect2d (p->x, p->y, 0, p->w, p->h, 0xb4b4b4ff);
    else
        DrawRect2d (p->x, p->y, 0, p->w, p->h, 0x787878ff);
    //draw panel content: 56 lines - 1 for dir path, 1 for status - 54
    int k;
    SetCurrentFont (2);
    SetFontSize (8, 8);
    //title - current path
    SetFontColor (0x0000ffff, 0x00000000);
    SetFontAutoCenter (0);
    if (p->path)
        snprintf (fname, 51, "%s", p->path);
    else
        snprintf (fname, 51, "%s", "[root]");
    DrawString (p->x, p->y, fname);
    //
    SetFontColor (0x000000ff, 0x00000000);
    SetFontAutoCenter (0);
    //
    struct fm_file *ptr = p->entries;
    //do we need to skip entries on listing?
    for (se = p->current_idx - wh + 1; se > 0 && ptr != NULL; ptr = ptr->next, se--)
        ;//skip some entries
    for (k = 0; k < wh && ptr != NULL; k++, ptr = ptr->next)
    {
        //draw current item
        if (p->current == ptr)
        {
            DrawRect2d (p->x, p->y + 8 + k * 8, 0, p->w, 8, 0x787878ff);
            //
            fname[0] = '>';
            fm_fname_get (ptr, 51, fname + 1);
            DrawString (p->x, p->y + 8 + k * 8, fname);
        }
        else
        {
            if (ptr->selected)
            {
                fname[0] = '*';
                fm_fname_get (ptr, 51, fname + 1);
                DrawString (p->x, p->y + 8 + k * 8, fname);
            }
            else
            {
                fm_fname_get (ptr, 51, fname);
                DrawString (p->x + 8, p->y + 8 + k * 8, fname);
            }
        }
        //file size - to the right side of the name
        if (!ptr->dir)
        {
            if (ptr->size > GBSZ)
                snprintf (fname, 52, "%4luGB", ptr->size / GBSZ);
            else if (ptr->size > MBSZ)
                snprintf (fname, 52, "%4luMB", ptr->size / MBSZ);
            else
                snprintf (fname, 52, "%4luKB", ptr->size / KBSZ);
            DrawString (p->x + 8 + (46 * 8), p->y + 8 + k * 8, fname);
        }
    }
    //status - size, files, dirs
    SetFontColor (0x0000ffff, 0x00000000);
    SetFontAutoCenter (0);
    if (p->path)
    {
        int bw = snprintf (fname, 51, "%d dirs, %d files - ", p->dirs, p->files);
        if (p->fsize > GBSZ)
            snprintf (fname + bw, 51 - bw, "%llu GB", p->fsize / GBSZ);
        else if (p->fsize > MBSZ)
            snprintf (fname + bw, 51 - bw, "%llu MB", p->fsize / MBSZ);
        else
            snprintf (fname + bw, 51 - bw, "%llu KB", p->fsize / KBSZ);
        DrawString (p->x, p->y + wh * 8 + 8, fname);
    }
    //
    return 0;
}

static int add_before (struct fm_panel *p, struct fm_file *link, struct fm_file *next)
{
    /* 4. Make prev of new node as prev of next_node */
    link->prev = next->prev;  
  
    /* 5. Make the prev of next_node as new_node */
    next->prev = link;  
  
    /* 6. Make next_node as next of new_node */
    link->next = next;  
  
    /* 7. Change next of new_node's previous node */
    if (link->prev != NULL)  
        link->prev->next = link;  
    /* 8. If the prev of new_node is NULL, it will be 
       the new head node */
    else
        (p->entries) = link; 
    return 0;
}

int fm_fname_get (struct fm_file *link, int cw, char *out)
{
    *out = 0;
    if (!link)
        return -1;
    if (link->dir)
        snprintf (out, cw, "/%s", link->name);
    else
        snprintf (out, cw, "%s", link->name);
    return 0;
}

int fm_panel_locate (struct fm_panel *p, char *path)
{
    struct fm_file *ptr;
    int cidx = -1;
    for (ptr = p->entries; ptr != NULL; ptr = ptr->next)
    {
        cidx++;
        NPrintf ("locate %s vs %s\n", path, ptr->name);
        if (0 == strcmp (ptr->name, path))
        {
            p->current = ptr;
            p->current_idx = cidx;
            break;
        }
    }
    return 0;
}

int fm_panel_scroll (struct fm_panel *p, int dn)
{
    if (dn)
    {
        if (p->current->next != NULL)
        {
            p->current = p->current->next;
            p->current_idx++;
        }
        else
            return -1;
    }
    else
    {
        if (p->current->prev != NULL)
        {
            p->current = p->current->prev;
            p->current_idx--;
        }
        else
            return -1;
    }
    return 0;
}

int fm_panel_add (struct fm_panel *p, char *fn, char dir, unsigned long fsz)
{
    // Allocate memory for new node;
    struct fm_file *link = (struct fm_file*) malloc (sizeof (struct fm_file));
    if (!link)
        return -1;
    //
    link->name = strdup (fn);
    link->dir = dir;
    link->size = fsz;
    //
    link->selected = FALSE;
    //
    link->prev = NULL;
    link->next = NULL;
    //NPrintf ("fm_panel_add %s dir %d\n", fn, dir);
    //stats
    if (fsz > 0)
        p->fsize += fsz;
    if (dir)
        p->dirs++;
    else
        p->files++;
    // If head is empty, create new list
    if (p->entries == NULL)
    {
        p->entries = link;
    }
    else
    {
        struct fm_file *current = p->entries;
    #if 1
        if (dir)
        {
            while (current->next && current->dir && strcmp (current->name, fn) < 0)
                current = current->next;
            //don't add after file
            if (current->next == NULL && current->dir)
            {
                if (strcmp (current->name, fn) < 0)
                {
                    current->next = link;
                    link->prev = current;
                }
                else
                {
                    add_before (p, link, current);
                }
            }
            else
            {
                //NPrintf ("fm_panel_add before %s\n", current->name);
                add_before (p, link, current);
            }
        }
        else
        {
            //skip dirs
            while (current->next && current->dir)
                current = current->next;
            //compare only with files
            if (!current->dir && strcmp (current->name, fn) < 0)
                while (current->next && strcmp (current->name, fn) < 0)
                    current = current->next;
            //
            if (strcmp (current->name, fn) > 0)
            {
                //NPrintf ("fm_panel_add before %s\n", current->name);
                add_before (p, link, current);
            }
            else
            //if (current->next == NULL)
            {
                current->next = link;
                link->prev = current;
            }
        }
    #else
        // move to the end of the list
        while (current->next != NULL)
            current = current->next;
        // Insert link at the end of the list
        current->next = link;
        link->prev = current;
    #endif
    }
    //set current item
    p->current = p->entries;
    p->current_idx = 0;
    //
    return 0;
}

int fm_panel_del (struct fm_panel *p, char *fn)
{
    struct fm_file *pre_node;

    if(p->entries == NULL)
    {
        //printf("Linked List not initialized");
        return -1;
    } 
    struct fm_file *current = p->entries;
    pre_node = current;
    while (current->next != NULL && strcmp (current->name, fn) != 0) 
    {
        pre_node = current;
        current = current->next;
    }        

    if (strcmp (current->name, fn) == 0)
    {
        pre_node->next = pre_node->next->next;
        if (pre_node->next != NULL)
        {          // link back
            pre_node->next->prev = pre_node;
        }
        if (p->entries == current)
            p->entries = NULL;
        if(current->name) free (current->name);
        if(current) free (current);
    }
    return 0;
}
