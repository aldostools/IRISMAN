static  float copy_parts;
static float copy_cpart;

#include "event_threads.h"

static int use_async_fd = 0;

volatile struct f_async {
    int flags;
    int fd;
    void * mem;
    int size;
    u64 readed;
} my_f_async;

#define ASYNC_ENABLE 128
#define ASYNC_ERROR 16
#define ASYNC_FCLOSE 2
#define ASYNC_NTFS 1

static void my_func_async(struct f_async * v)
{
    int ret = FAILED;

    if(v && v->flags & ASYNC_ENABLE)
    {
        v->readed = 0;
        int flags = v->flags;
        if(v->mem)
        {
            if(flags & ASYNC_NTFS)
                {ret = ps3ntfs_write(v->fd, v->mem, v->size); v->readed = (u64) ret; if(ret>0) ret = 0;}
            else ret =sysLv2FsWrite(v->fd, v->mem, v->size, &v->readed);

            free(v->mem); v->mem = 0;

            if(ret == 0 && v->size != v->readed) ret = FAILED;
        }

        if(ret) flags|= ASYNC_ERROR;

        if(flags & (ASYNC_ERROR | ASYNC_FCLOSE))
        {
            if(flags & ASYNC_NTFS) ps3ntfs_close(v->fd); else sysLv2FsClose(v->fd);
        }

        flags &= ~ASYNC_ENABLE;

        v->flags = flags;
    }
}

#define CPY_NOTCLOSE 256
#define CPY_FILE1_IS_NTFS 1
#define CPY_FILE2_IS_NTFS 2

static int CopyFd(s32 flags, s32 fd, s32 fd2, char *mem, u64 length)
{
    int ret = 0;
    int one = 0;
    u64 pos = 0ULL;
    u64 readed = 0, writed = 0;

    while(pos < length)
    {
        readed = length - pos; if(readed > 0x100000ULL) readed = 0x100000ULL;

        if(flags & CPY_FILE1_IS_NTFS) {ret = ps3ntfs_read(fd, mem, readed); writed = (u64) ret; if(ret>0) ret = 0;}
        else ret =sysLv2FsRead(fd, mem, readed, &writed);

        if(ret < 0) goto skip;
        if(readed != writed) {ret = 0x8001000C; goto skip;}

        loop_write:

        if(use_async_fd)
        {
            if(use_async_fd == ASYNC_ENABLE)
            {
                use_async_fd = 1;
                my_f_async.flags = 0;
                my_f_async.fd = fd2;
                my_f_async.mem = malloc(readed);
                if(my_f_async.mem) memcpy(my_f_async.mem, mem, readed);
                my_f_async.size = readed;
                my_f_async.readed = 0;
                my_f_async.flags = ASYNC_ENABLE | ((flags & CPY_FILE2_IS_NTFS) != 0)
                    | (ASYNC_FCLOSE * (pos + readed >= length  && !(flags & CPY_NOTCLOSE)));
                event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
            }
            else
            {
                if(!(my_f_async.flags & ASYNC_ENABLE))
                {
                    if(my_f_async.flags & ASYNC_ERROR) {ret = 0x8001000C; goto skip;}
                    my_f_async.flags = 0;
                    my_f_async.fd = fd2;
                    my_f_async.mem = malloc(readed);
                    if(my_f_async.mem) memcpy(my_f_async.mem, mem, readed);
                    my_f_async.size = readed;
                    my_f_async.readed = 0;
                    my_f_async.flags = ASYNC_ENABLE | ((flags & CPY_FILE2_IS_NTFS) != 0)
                        | (ASYNC_FCLOSE * (pos + readed >= length && !(flags & CPY_NOTCLOSE)));
                    event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);

                }
                else
                {
                    //wait_event_thread();
                    goto loop_write;
                }
            }

        }
        else
        {
            if(flags & CPY_FILE2_IS_NTFS) {ret = ps3ntfs_write(fd2, mem, readed); writed = (u64) ret; if(ret>0) ret = 0;}
            else ret = sysLv2FsWrite(fd2, mem, readed, &writed);
            if(ret < 0) goto skip;
            if(readed != writed) {ret = 0x8001000C; goto skip;}
        }

        pos += readed;

        if(progress_action == 2) {ret = 0x8001000C; goto skip;}

        copy_cpart += copy_parts;
        if(copy_cpart >= 1.0f)
        {
            one= 1;
            update_bar2((u32) copy_cpart);
            copy_cpart-= (float) ((u32) copy_cpart);
        }

    }

    if(!one) update_bar2((u32) 100.0f);

skip:
    return ret;
}

static int use_iso_splits = 0;

int CopyFile(char* path, char* path2)
{
    int ret = 0;
    s32 fd = FAILED;
    s32 fd2 = FAILED;
    u64 length = 0LL;

    char *mem = NULL;

    sysFSStat stat;
    struct stat fstat;

    s32 flags = 0;

    filepath_check(path2);

    if(is_ntfs_path(path )) flags|= CPY_FILE1_IS_NTFS;
    if(is_ntfs_path(path2)) flags|= CPY_FILE2_IS_NTFS;

    if(allow_shadow_copy && !strncmp(path, "/dev_hdd0", 9) && !strncmp(path2, "/dev_hdd0", 9))
    {
        sysLv2FsUnlink(path2);
        return sysLv2FsLink(path, path2);
    }

    if(Files_To_Copy == 0) Files_To_Copy = 1;

    progress_0 += 100.0f / (float) Files_To_Copy;
    if(progress_0 >= 1.0f)
    {
        msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) progress_0);
        progress_0-= (float) ((u32) (100.0f/(float) Files_To_Copy));
    }

    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, "Progress");
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, dyn_get_name(path));

    if(reset_copy)
    {
        reset_copy = 0;

        for(ret =0; ret < 20; ret++)
        {
            sysUtilCheckCallback();tiny3d_Flip();
        }
    }

    char *ext = get_extension(path);
    char *ext0 = path;

    while(use_iso_splits)
    {
        char *e = strstr(ext0, ".iso.");
        if(!e) e = strstr(ext0, ".ISO.");
        if(!e) break;
        ext0 = e + 4;
    }

    if(use_iso_splits && strlen(path) > 6 && strncasecmp(ext0 - 4, ".iso.", 5) == SUCCESS)
    {
        // split files
        if(strcmp(ext0, ".0")) goto skip;

        int n;

        fd = fd2 = FAILED;

        mem = malloc(0x100000);
        if(!mem) {ret = (int) 0x80010004; goto skip2;}

        char *ext2 = get_extension(path2);

        if(!strncmp(ext2, ".0", 2)) ext2[0]=0;

        for(n = 0; n < 99; n++)
        {
            fd = FAILED;

            sprintf(ext0, ".%i", n);


            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, dyn_get_name(path2));

            if(flags & CPY_FILE1_IS_NTFS)
               {ret = ps3ntfs_stat(path, &fstat);stat.st_size = fstat.st_size;}
            else
                ret = sysLv2FsStat(path, &stat);

            if(ret < 0 || stat.st_size == 0) {ret = 0;goto skip2;}

            length = stat.st_size;
            if(flags & CPY_FILE1_IS_NTFS) {fd = ps3ntfs_open(path, O_RDONLY, 0);if(fd < 0) ret = FAILED; else ret = SUCCESS;}
            else
                ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
            if(ret) goto skip2;

            if(n == 0)
            {
                if(flags & CPY_FILE2_IS_NTFS)
                {
                    fd2 = ps3ntfs_open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0);if(fd2 < 0) ret = FAILED; else ret = SUCCESS;
                    if(ret) goto skip2;
                }
                else
                {
                    ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0);
                    if(ret) goto skip2;
                    sysLv2FsChmod(path2, FS_S_IFMT | 0777);
                }
            }

            copy_parts = (length == 0) ? 0.0f : 100.0f / ((double) length / (double) 0x100000);
            copy_cpart = 0;

            ret = CopyFd(flags | CPY_NOTCLOSE, fd, fd2, mem, length);
            if(ret < 0) goto skip2;

            if(flags & CPY_FILE1_IS_NTFS) ps3ntfs_close(fd); else sysLv2FsClose(fd); fd = FAILED;
        }

        loop_wait0:

        if(use_async_fd)
        {
            if(!(my_f_async.flags & ASYNC_ENABLE))
            {
                if(flags & CPY_FILE2_IS_NTFS) ps3ntfs_close(fd2); else sysLv2FsClose(fd2);
                if(my_f_async.flags  & ASYNC_ERROR) {ret = 0x8001000C; goto skip2;}
            }
            else
                goto loop_wait0;
        }
    }
    else if(!strncmp(ext, ".666", 4))
    {
        // split files
        if(strcmp(ext, ".66600")) goto skip;

        int n;

        fd = fd2 = FAILED;

        mem = malloc(0x100000);
        if(!mem) {ret = (int) 0x80010004; goto skip2;}

        char *ext2 = get_extension(path2);

        if(!strncmp(ext2, ".666", 4)) ext2[0]=0;

        for(n = 0; n < 99; n++)
        {
            fd = FAILED; ext[4]= 48 + n/10; ext[5]= 48 + (n % 10);

            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, dyn_get_name(path2));

            if(flags & CPY_FILE1_IS_NTFS)
               {ret = ps3ntfs_stat(path, &fstat);stat.st_size = fstat.st_size;}
            else
                ret = sysLv2FsStat(path, &stat);

            if(ret < 0 || stat.st_size == 0) {ret = 0;goto skip2;}

            length = stat.st_size;
            if(flags & CPY_FILE1_IS_NTFS)
            {
                fd = ps3ntfs_open(path, O_RDONLY, 0);
                if(fd < 0) ret = FAILED; else ret = SUCCESS;
            }
            else
                ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);

            if(ret) goto skip2;

            if(n == 0)
            {
                if(flags & CPY_FILE2_IS_NTFS)
                {
                    fd2 = ps3ntfs_open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0);if(fd2 < 0) ret = FAILED; else ret = SUCCESS;
                    if(ret) goto skip2;
                }
                else
                {
                    ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0);
                    if(ret) goto skip2;
                    sysLv2FsChmod(path2, FS_S_IFMT | 0777);
                }
            }

            copy_parts = (length == 0) ? 0.0f : 100.0f / ((double) length / (double) 0x100000);
            copy_cpart = 0;

            ret = CopyFd(flags | CPY_NOTCLOSE, fd, fd2, mem, length);
            if(ret < 0) goto skip2;

            if(flags & CPY_FILE1_IS_NTFS) ps3ntfs_close(fd); else sysLv2FsClose(fd); fd = FAILED;

        }

        loop_wait:

        if(use_async_fd) {

            if(!(my_f_async.flags & ASYNC_ENABLE))
            {
                if(flags & CPY_FILE2_IS_NTFS) ps3ntfs_close(fd2); else sysLv2FsClose(fd2);
                if(my_f_async.flags  & ASYNC_ERROR) {ret = 0x8001000C; goto skip2;}
            }
            else
                goto loop_wait;
        }

    }
    else
    {
        if(flags & CPY_FILE1_IS_NTFS)
        {
            ret = ps3ntfs_stat(path, &fstat);
            stat.st_size = fstat.st_size;
        }
        else
            ret = sysLv2FsStat(path, &stat);

        if(ret) goto skip;

        length = stat.st_size;

        if(length >= 0xFFFF0001LL &&
           strncmp(path2, "/dev_hdd0", 9) && is_ntfs_path(path2) == false)
        {
            // split the file
            if(flags & CPY_FILE1_IS_NTFS) {fd = ps3ntfs_open(path, O_RDONLY, 0);if(fd < 0) ret = FAILED; else ret = SUCCESS;}
            else
                ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
            if(ret) goto skip2;

            mem = malloc(0x100000);
            if(!mem) {ret = (int) 0x80010004; goto skip2;}

            u64 pos = 0;
            int n = 0;

            copy_parts = (length == 0) ? 0.0f : 100.0f / ((double) length / (double) 0x100000);
            copy_cpart = 0;

            char *ext2 =&path2[strlen(path2)];

            int is_iso = strlen(path2) > 4 && (!strcmp(ext2 - 4, ".iso") || !strcmp(ext2 - 4, ".ISO"));

            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);

            while(pos < stat.st_size)
            {
                ext2[0] = 0;
                if(is_iso) sprintf(ext2,".%i", n);
                else sprintf(ext2,".666%2.2i", n);

                msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, dyn_get_name(path2));

                length = (stat.st_size - pos);

                if(is_iso)
                {
                    if(length > 0xFFFF0000LL) length = 0xFFFF0000LL;
                }
                else
                {
                    if(length > 0x40000000LL) length = 0x40000000LL;
                }

                if(flags & CPY_FILE2_IS_NTFS) {fd2 = ps3ntfs_open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0);if(fd2 < 0) ret = FAILED; else ret = SUCCESS;}
                else ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0);
                if(ret) goto skip2;

                if(!(flags & CPY_FILE2_IS_NTFS)) sysLv2FsChmod(path2, FS_S_IFMT | 0777);

                ret = CopyFd(flags, fd, fd2, mem, length);
                if(ret < 0) goto skip2;

                if(!use_async_fd)
                {
                    if(flags & CPY_FILE2_IS_NTFS) ps3ntfs_close(fd2); else sysLv2FsClose(fd2);
                }

                fd2 = FAILED;

                pos+= length;

                n++;
            }
        }
        else
        {
            if(flags & CPY_FILE1_IS_NTFS) {fd = ps3ntfs_open(path, O_RDONLY, 0);if(fd < 0) ret = FAILED; else ret = SUCCESS;}
            else ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
            if(ret) goto skip;


            if(flags & CPY_FILE2_IS_NTFS) {fd2 = ps3ntfs_open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0);if(fd2 < 0) ret = FAILED; else ret = SUCCESS;}
            else ret = sysLv2FsOpen(path2, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd2, 0777, NULL, 0);

            if(ret) {if(flags & CPY_FILE1_IS_NTFS) ps3ntfs_close(fd); else sysLv2FsClose(fd); goto skip;}
            if(!(flags & CPY_FILE2_IS_NTFS)) sysLv2FsChmod(path2, FS_S_IFMT | 0777);


            mem = malloc(0x100000);
            if(!mem) {ret = (int) 0x80010004; goto skip2;}

            copy_parts = (length == 0) ? 0.0f : 100.0f / ((double) length / (double) 0x100000);
            copy_cpart = 0;

            ret = CopyFd(flags, fd, fd2, mem, length);
        }
    }


skip2:

    if(mem) free(mem);
    if(fd >= SUCCESS) {if(flags & CPY_FILE1_IS_NTFS) ps3ntfs_close(fd); else sysLv2FsClose(fd);}
    if(!use_async_fd) {if(fd2 >= SUCCESS){if(flags & CPY_FILE2_IS_NTFS) ps3ntfs_close(fd2); else sysLv2FsClose(fd2);}}
    if(ret > 0) ret = 0;

skip:

    if(progress_action == 2)
    {
        if(my_f_async.flags & ASYNC_ENABLE)
        {
            wait_event_thread();
            if(my_f_async.flags  & ASYNC_ERROR) {ret = 0x8001000C;}
        }

        unlink_secure(path2);
    }

    //msgDialogAbort();
    return ret;
}

int CopyDirectory(char* path, char* path2, char* path3)
{
    int dfd;
    u64 read;
    sysFSDirent dir;
    DIR_ITER *pdir = NULL;
    struct stat st;
    int ret = 0;
    int p1 = strlen(path);
    int p2 = strlen(path2);

    s32 flags = 0;

    // avoid recursive-infinite copy
    if(!strncmp(path, path3, strlen(path3))) return SUCCESS;

    filepath_check(path2);

    if(is_ntfs_path(path )) flags|= CPY_FILE1_IS_NTFS;
    if(is_ntfs_path(path2)) flags|= CPY_FILE2_IS_NTFS;

    if(flags & CPY_FILE1_IS_NTFS)
    {
        pdir = ps3ntfs_diropen(path);
        if(pdir) ret = SUCCESS; else ret = FAILED;
    }
    else
        ret = sysLv2FsOpenDir(path, &dfd);

    if(ret) return FAILED;

    if(flags & CPY_FILE2_IS_NTFS) ps3ntfs_mkdir(path2, 0777); else sysLv2FsMkdir(path2, 0777);

    read = sizeof(sysFSDirent);

    while ((!(flags & CPY_FILE1_IS_NTFS) && !sysLv2FsReadDir(dfd, &dir, &read)) ||
           ( (flags & CPY_FILE1_IS_NTFS) && ps3ntfs_dirnext(pdir, dir.d_name, &st) == SUCCESS))
    {
        if (!(flags & CPY_FILE1_IS_NTFS) && !read)
            break;
        if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
            continue;

        if(flags & CPY_FILE1_IS_NTFS) {dir.d_type = (S_ISDIR(st.st_mode)) ? IS_DIRECTORY : IS_FILE;}

        path[p1]= 0;
        path2[p2]= 0;
        strcat(path, "/");
        strcat(path, dir.d_name);
        strcat(path2, "/");
        strcat(path2, dir.d_name);

        if (dir.d_type & IS_DIRECTORY)
        {
            // avoid recursive-infinite copy
            if(!strncmp(path, path3, strlen(path3))) {ret = 0;}
            else
            {
                ret = CopyDirectory(path, path2, path3);
                if(ret) goto skip;
            }
        }
        else
        {
            ret = CopyFile(path, path2);
            if(ret < 0) goto skip;
        }
    }

skip:

    path[p1] = 0;
    path2[p2] = 0;

    if(flags & CPY_FILE1_IS_NTFS) ps3ntfs_dirclose(pdir); else sysLv2FsCloseDir(dfd);

    return ret;
}

void pause_music(int pause);

static int copy_file_manager(char *path1, char *path2, sysFSDirent *ent, int nent, int sel, u64 free)
{
    int ret = 0;
    int n;

    u64 size = 0;

    //allow_shadow_copy = false;

    use_async_fd = ASYNC_ENABLE;
    pause_music(1);

    reset_copy = 1;
    cpy_str = "Copy";
    Files_To_Copy = 0;
    Folders_To_Copy = 0;

    int msg_en = 0;

    use_iso_splits = 0;
    if(!strncmp(path2, "/dev_hdd0", 9))
    {
        use_iso_splits = 1;
        if(allow_shadow_copy && !strncmp(path1, "/dev_hdd0", 9)) cpy_str = "Shadow Copy";
    }


    if(sel >= 0)
    {
        sprintf(MEM_MESSAGE, "Do you want to %s %s\n\nfrom %s\n\nto %s?", cpy_str, ent[sel].d_name, path1, path2);

        if(DrawDialogYesNo(MEM_MESSAGE) == YES)
        {
            double_bar(cpy_str);
            msg_en = 1;

            sprintf(TEMP_PATH1 , "%s/%s", path1, ent[sel].d_name);
            sprintf(TEMP_PATH2 , "%s/%s", path2, ent[sel].d_name);
            sprintf(TEMP_PATH  , "%s/%s", path2, ent[sel].d_name);

            if(ent[sel].d_type & IS_DIRECTORY)
            {
                ret = CountFiles(TEMP_PATH1, &Files_To_Copy, &Folders_To_Copy, &size);

                if(ret < 0) goto end;
                if(allow_shadow_copy == false && size > free) goto end;

                sprintf(TEMP_PATH1, "%s/%s", path1, ent[sel].d_name);
                ret = CopyDirectory(TEMP_PATH1, TEMP_PATH2, TEMP_PATH);
            }
            else
            {
                if(is_ntfs_path(TEMP_PATH1))
                {
                    struct stat fstat;
                    ret = ps3ntfs_stat(TEMP_PATH1, &fstat);
                    size+= fstat.st_size;
                }
                else
                {
                    sysFSStat stat;
                    ret = sysLv2FsStat(TEMP_PATH1, &stat);
                    size+= stat.st_size;
                }

                if(ret != SUCCESS) goto end;
                if(allow_shadow_copy == false && size > free) goto end;

                Files_To_Copy = 1;

                ret = CopyFile(TEMP_PATH1, TEMP_PATH2);
            }

            msgDialogAbort();
            usleep(100000);
            msg_en = 0;
        }
    }
    else
    {
        // multiple
        sprintf(MEM_MESSAGE, "Do you want to %s the selected Files and Folders\n\nfrom %s\n\nto %s?", cpy_str, path1, path2);

        if(DrawDialogYesNo(MEM_MESSAGE) == YES)
        {
            double_bar(cpy_str);
            msg_en = 1;

            for(n = 0; n < nent; n++)
            {
                if(!(ent[n].d_type & IS_MARKED)) continue; // skip no marked

                sprintf(TEMP_PATH, "%s/%s", path1, ent[n].d_name);

                if(ent[n].d_type & IS_DIRECTORY) ret = CountFiles(TEMP_PATH, &Files_To_Copy, &Folders_To_Copy, &size);
                else
                {
                    if(is_ntfs_path(TEMP_PATH))
                    {
                        struct stat fstat;
                        ret = ps3ntfs_stat(TEMP_PATH, &fstat);
                        size+= fstat.st_size;
                    }
                    else
                    {
                        sysFSStat stat;
                        ret = sysLv2FsStat(TEMP_PATH, &stat);
                        size+= stat.st_size;
                    }

                    Files_To_Copy++;
                }

                if(ret < 0) goto end;
                if(allow_shadow_copy == false && size > free) goto end;
            }

            if(ret == 0)
            for(n = 0; n < nent; n++)
            {
                if(!(ent[n].d_type & IS_MARKED)) continue; // skip no marked

                sprintf(TEMP_PATH1, "%s/%s", path1, ent[n].d_name);
                sprintf(TEMP_PATH2, "%s/%s", path2, ent[n].d_name);
                sprintf(TEMP_PATH , "%s/%s", path2, ent[n].d_name);

                if(ent[n].d_type & IS_DIRECTORY)
                    ret = CopyDirectory(TEMP_PATH1, TEMP_PATH2, TEMP_PATH);
                else
                    ret = CopyFile(TEMP_PATH1, TEMP_PATH2);

                if(ret <0) break;
            }
        }

        msgDialogAbort();
        usleep(100000);
        msg_en = 0;
    }

 end:

    use_iso_splits = 0;

    if(msg_en)
    {
        msgDialogAbort();
        usleep(100000);
    }

    if(use_async_fd)
    {
        if(my_f_async.flags & ASYNC_ENABLE)
        {
            wait_event_thread();
            if(my_f_async.flags  & ASYNC_ERROR) {ret = 0x8001000C;}
        }

        event_thread_send(0x555ULL, (u64) 0, 0);
    }

    use_async_fd = 0;
    pad_last_time = 0;

    pause_music(0);

    if(ret < 0) ;
    else if(allow_shadow_copy == false && size > free)
    {
        DrawDialogOK("There is not enough free space to Copy Files/Folders");
    }

    allow_shadow_copy = true;
    return ret;
}

int copy_archive_file(char *path1, char *path2, char *file, u64 free)
{
    int ret = 0;
    u64 size = 0;
    int n, len;
    int nfiles = 0;
    int multi = 0;
    struct stat s;
    int msg_en = 0;

    use_async_fd = ASYNC_ENABLE;
    pause_music(1);

    reset_copy = 1;
    cpy_str = "Copy";

    Files_To_Copy = 0;
    Folders_To_Copy = 0;

    use_iso_splits = 0;
    if(!strncmp(path2, "/dev_hdd0", 9)) use_iso_splits = 1;

    len = strlen(file);
    if(len > 6 && strcasecmp(file + len - 6, ".iso.0") == SUCCESS) {

        file[len - 2] = 0;
        multi = 1;

        for(n = 0; n < 64; n++)
        {
            sprintf(TEMP_PATH, "%s/%s.%i", path1, file, n);
            if(stat(TEMP_PATH, &s) == SUCCESS)
            {
                Files_To_Copy++;
                nfiles++;
                size += s.st_size;
            }
            else
                break;
        }

        if(use_iso_splits)
        {
            multi = 0;
            Files_To_Copy = 1;
            nfiles = 1;
            file[len - 2] = '.';
        }
    }
    else
    {
        Files_To_Copy = 1;
        nfiles = 1;

        sprintf(TEMP_PATH, "%s/%s", path1, file);
        if(stat(TEMP_PATH, &s) == SUCCESS)
        {
            size += s.st_size;
        }
    }


    sprintf(MEM_MESSAGE, "Copy %s\n\nfrom %s\n\nto %s?", file, path1, path2);
    if(DrawDialogYesNo(MEM_MESSAGE) == YES)
    {
        double_bar(cpy_str);
        msg_en = 1;

        if(size > free || size == 0) goto end;

        for(n = 0; n < nfiles; n++)
        {
            if(multi)
            {
                sprintf(TEMP_PATH1, "%s/%s.%i", path1, file, n);
                sprintf(TEMP_PATH2, "%s/%s.%i", path2, file, n);

            }
            else
            {
                sprintf(TEMP_PATH1, "%s/%s", path1, file);
                sprintf(TEMP_PATH2, "%s/%s", path2, file);
            }

            ret = CopyFile(TEMP_PATH1, TEMP_PATH2);
            if(ret < 0) goto end;
        }

        msgDialogAbort();
        usleep(100000);
        msg_en = 0;
    }


 end:

    use_iso_splits = 0;

    if(msg_en)
    {
        msgDialogAbort();
        usleep(100000);
    }

    if(use_async_fd)
    {
        if(my_f_async.flags & ASYNC_ENABLE)
        {
            wait_event_thread();
            if(my_f_async.flags  & ASYNC_ERROR) {ret = 0x8001000C;}
        }

        event_thread_send(0x555ULL, (u64) 0, 0);
    }

    use_async_fd = 0;
    pad_last_time = 0;

    pause_music(0);

    if(ret < 0)
    {
        sprintf(MEM_MESSAGE, "Copy error: 0x%08x\n\n%s", ret, getlv2error(ret));
        DrawDialogOK(MEM_MESSAGE);
        return ret;
    }

    if(size > free)
    {
        DrawDialogOK("There is not free space to Copy the Files/Folders");
    }

    return ret;
}

static int move_file_manager(char *path1, char *path2, sysFSDirent *ent, int nent, int sel, u64 free)
{
    int ret = 0;
    int n;
    int flag = 0;

    u64 size = 0;

    use_async_fd = ASYNC_ENABLE;

    pause_music(1);

    n = 1; while(path1[n] != '/' && path1[n] != 0) n++;

    if(!strncmp(path1, path2, n - 1)) flag = 1; // can move

    cpy_str = "Move";
    Files_To_Copy = 0;
    Folders_To_Copy = 0;

    reset_copy = 1;


    if(sel >= 0)
    {
        sprintf(MEM_MESSAGE, "Do you want to Move %s\n\nfrom %s\n\nto %s?", ent[sel].d_name, path1, path2);
        if(DrawDialogYesNo(MEM_MESSAGE) == YES)
        {
            double_bar(cpy_str);

            sprintf(TEMP_PATH1, "%s/%s", path1, ent[sel].d_name);
            sprintf(TEMP_PATH2, "%s/%s", path2, ent[sel].d_name);
            sprintf(TEMP_PATH , "%s/%s", path2, ent[sel].d_name);

            if(flag)
            {
                ret = rename_secure(TEMP_PATH1, TEMP_PATH2);
            }
            else if(ent[sel].d_type & IS_DIRECTORY)
            {
                ret = CountFiles(TEMP_PATH1, &Files_To_Copy, &Folders_To_Copy, &size);

                if(ret < 0) goto end;
                if(size > free) goto end;

                sprintf(TEMP_PATH1, "%s/%s", path1, ent[sel].d_name);
                ret = CopyDirectory(TEMP_PATH1, TEMP_PATH2, TEMP_PATH);

                if(ret == 0)
                {
                    DeleteDirectory(TEMP_PATH1);
                    if(is_ntfs_path(TEMP_PATH1))
                        ret = ps3ntfs_unlink(TEMP_PATH1);
                    else
                        ret = rmdir_secure(TEMP_PATH1);
                }
            }
            else
            {
                if(is_ntfs_path(TEMP_PATH1))
                {
                    struct stat fstat;
                    ret = ps3ntfs_stat(TEMP_PATH1, &fstat);
                    size+= fstat.st_size;
                }
                else
                {
                    sysFSStat stat;
                    ret = sysLv2FsStat(TEMP_PATH1, &stat);
                    size+= stat.st_size;
                }

                if(ret != SUCCESS) goto end;
                if(size > free) goto end;

                Files_To_Copy = 1;
                ret = CopyFile(TEMP_PATH1, TEMP_PATH2);

                if(ret == 0) ret = unlink_secure(TEMP_PATH1);
            }

            msgDialogAbort();
            usleep(100000);
        }
    }
    else
    {
        // multiple
        sprintf(MEM_MESSAGE, "Do you want to Move the selected Files and Folders\n\nfrom %s\n\nto %s?", path1, path2);
        if(DrawDialogYesNo(MEM_MESSAGE) == YES)
        {
            double_bar(cpy_str);

            if(!flag)
            for(n = 0; n < nent; n++)
            {
                if(!(ent[n].d_type & IS_MARKED)) continue; // skip no marked

                sprintf(TEMP_PATH1, "%s/%s", path1, ent[n].d_name);


                if(ent[n].d_type & IS_DIRECTORY) ret = CountFiles(TEMP_PATH1, &Files_To_Copy, &Folders_To_Copy, &size);
                else
                {
                    if(is_ntfs_path(TEMP_PATH1))
                    {
                        struct stat fstat;
                        ret = ps3ntfs_stat(TEMP_PATH1, &fstat);
                        size+= fstat.st_size;
                    }
                    else
                    {
                        sysFSStat stat;
                        ret = sysLv2FsStat(TEMP_PATH1, &stat);
                        size+= stat.st_size;
                    }

                    Files_To_Copy++;
                }

                if(ret < 0) goto end;
                if(size > free) goto end;
            }

            if(ret == 0)
            for(n = 0; n < nent; n++)
            {
                if(!(ent[n].d_type & IS_MARKED)) continue; // skip no marked

                sprintf(TEMP_PATH1, "%s/%s", path1, ent[n].d_name);
                sprintf(TEMP_PATH2, "%s/%s", path2, ent[n].d_name);

                if(flag)
                {
                    ret = rename_secure(TEMP_PATH1, TEMP_PATH2);
                }
                else if(ent[n].d_type & IS_DIRECTORY)
                {
                    sprintf(TEMP_PATH , "%s/%s", path2, ent[n].d_name);
                    ret = CopyDirectory(TEMP_PATH1, TEMP_PATH2, TEMP_PATH);
                    if(ret == 0)
                    {
                        DeleteDirectory(TEMP_PATH1);
                        if(is_ntfs_path(TEMP_PATH1))
                            ret = ps3ntfs_unlink(TEMP_PATH1);
                        else
                            ret = rmdir_secure(TEMP_PATH1);
                    }
                }
                else
                {
                    ret = CopyFile(TEMP_PATH1, TEMP_PATH2);
                    if(ret == 0)
                    {
                        if(is_ntfs_path(TEMP_PATH1))
                            ret = ps3ntfs_unlink(TEMP_PATH1);
                        else
                            ret = unlink_secure(TEMP_PATH1);
                    }
                }

                if(ret < 0) break;
            }
            msgDialogAbort();
            usleep(100000);
        }
    }

 end:
     if(use_async_fd)
     {
        if(my_f_async.flags & ASYNC_ENABLE)
        {
            wait_event_thread();
            if(my_f_async.flags  & ASYNC_ERROR) {ret = 0x8001000C;}
        }

        event_thread_send(0x555ULL, (u64) 0, 0);
    }

    use_async_fd = 0;
    pad_last_time = 0;

    pause_music(0);

    if(ret < 0) return ret;

    if(size > free)
    {
        msgDialogAbort();
        usleep(100000);
        DrawDialogOK("There is not free space to Move Files/Folders");
    }

    return ret;
}
