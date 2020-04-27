/*******************************************************************************************************************************************************/
/* FAST FILES                                                                                                                                          */
/*******************************************************************************************************************************************************/

static int copy_split_to_cache = 0;

static int file_counter = 0; // to count files

static time_t time_start; // time counter init

static char string1[256];

static int abort_copy = 0; // abort process

static int copy_mode = 0; // 0- normal 1-> pack files >= 4GB

static int copy_is_split = 0; // return 1 if files is split

static s64 global_device_bytes = 0;

#define MAX_FAST_FILES 16
#define FILESIZE_MAX 0x200000

typedef struct _t_fast_files
{
    s64 readed; // global bytes readed
    s64 writed; // global bytes writed
    s64 off_readed; // offset correction for bigfiles_mode == 2  (joining)
    s64 len;    // global len of the file (value increased in the case of bigfiles_ mode == 2)

    int giga_counter; // counter for split files to 1GB for bigfiles_mode == 1 (split)
    u32 fl; // operation control
    int bigfile_mode;
    int pos_path; // filename position used in bigfiles

    char pathr[MAX_PATH_LEN]; // read path
    char pathw[MAX_PATH_LEN]; // write path


    int use_doublebuffer; // if files >= 4MB use_doblebuffer =1;

    void *mem; // buffer for read/write files ( x2 if use_doublebuffer is fixed)
    int size_mem; // size of the buffer for read

    int number_frag; // used to count fragments files in bigfile_mode

    sysFSAio t_read;  // used for async read
    sysFSAio t_write; // used for async write

} t_fast_files __attribute__((aligned(8)));

static t_fast_files *fast_files=NULL;

static int fast_num_files=0;

static int fast_used_mem=0;

static volatile int current_fast_file_r=0;
static volatile int current_fast_file_w=0;

static volatile int fast_read=0;
static volatile int fast_writing=0;

static int files_opened=0;


static int total_fast_files = 0;

void filepath_check(char *file)
{
    if((file[5] == 'u' && !strncmp(file, "/dev_usb", 8)) || (file[2] != 'd' && is_ntfs_path(file)))
    {
        u16 n = 8, c = 8;
        // remove invalid chars
        while(true)
        {
            if(file[c] == '\\') file[c] = '/';
            if(strchr("\"<|>:*?", file[c]) == NULL) file[n++] = file[c];
            if(!file[c++]) break;
        }
    }
}

static int fast_copy_async(char *pathr, char *pathw, int enable)
{

    fast_num_files = 0;

    fast_read    = 0;
    fast_writing = 0;

    fast_used_mem = 0;
    files_opened  = 0;

    current_fast_file_r = current_fast_file_w = 0;

    filepath_check(pathw);

    if(enable)
    {
        if(sysFsAioInit(pathr)!= 0)  return FAILED;
        if(sysFsAioInit(pathw)!= 0)  return FAILED;

        fast_files = (t_fast_files *) memalign(8, sizeof(t_fast_files) * (MAX_FAST_FILES));
        if(!fast_files) return -2;

        memset((void *) fast_files, 0, sizeof(t_fast_files) * (MAX_FAST_FILES));

        return SUCCESS;
    }
    else
    {
        if(fast_files)
        {
            free(fast_files);
            fast_files = NULL;
        }

        sysFsAioFinish(pathr);
        sysFsAioFinish(pathw);
    }

    return SUCCESS;
}

static int fast_copy_process();

#define MAX_FILECACHED 4

static int nfilecached = 0;
static s64 filecached_bytes[MAX_FILECACHED];
static char filecached[MAX_FILECACHED][2][MAX_PATH_LEN];

static char * path_cache = NULL;

static s64 copy_total_size = 0;

static int fast_copy_add(char *pathr, char *pathw, char *file)
{
    int size_mem;

    int strl = strlen(file);

    sysFSStat s;

    filepath_check(pathw);

    if(fast_num_files >= MAX_FAST_FILES || fast_used_mem >= FILESIZE_MAX)
    {
        int ret = fast_copy_process();

        if(ret < 0 || abort_copy)
        {
            DPrintf("%s%i\n", language[FASTCPADD_FAILED], ret);
            return ret;
        }
    }

    if(fast_num_files >= MAX_FAST_FILES)
    {
        DPrintf("%s\n", language[FASTCPADD_ERRTMFILES]); return -1;
    }

    fast_files[fast_num_files].bigfile_mode = 0;

    if(strl > 6)
    {
        char *p = file;
        p+= strl - 6; // adjust for .666xx
        if(p[0] == '.' && p[1] == '6' && p[2] == '6' && p[3] == '6')
        {
            if(p[4] != '0' ||  p[5] != '0')  {return 0;} // ignore this files
            fast_files[fast_num_files].bigfile_mode = 2; // joining split files
        }

    }

    if(strl > 7)
    {
        char *p = file;
        p+= strl - 7; // adjust for .x.part
        if(p[0] == '.'  && p[2] == '.' && p[3] == 'p')
        {
            if(p[4] == 'a' ||  p[5] == 'r' ||  p[6] == 't')  {return 0;} // ignore this files
        }
    }

    if(strl > 8)
    {
        char *p = file;
        p+= strl - 8; // adjust for .xx.part
        if(p[0] == '.'  && p[3] == '.' && p[4] == 'p')
        {
            if(p[5] == 'a' ||  p[6] == 'r' ||  p[7] == 't')  {return 0;} // ignore this files
        }
    }

    // test if exists .1.part
    sprintf(buff,"%s/%s.1.part", pathr, file);

    if(sysFsStat(buff, &s) == 0)
    {
        fast_files[fast_num_files].bigfile_mode = 3; // joining split files
    }

    if(copy_split_to_cache
        && fast_files[fast_num_files].bigfile_mode != 2
        && fast_files[fast_num_files].bigfile_mode != 3) return 0;


    sprintf(fast_files[fast_num_files].pathr, "%s/%s", pathr, file);

    if(sysFsStat(fast_files[fast_num_files].pathr, &s) < 0)
    {
        DPrintf("%s\n", language[FASTCPADD_FAILEDSTAT]);
        abort_copy = 1;
        return -1;
    }

    if(copy_split_to_cache)
    {
        if(nfilecached <= 0) return 0;

        if(fast_files[fast_num_files].bigfile_mode == 3)
        {
            sprintf(buff, "%s/%s", pathr, file);

            int n;

            for(n = 0; n < nfilecached; n++)
                if(!strcmp(&filecached[n][0][0], buff)) break;

            if(n == nfilecached) return 0;

            sprintf(fast_files[fast_num_files].pathw, "%s/%s", path_cache, file);
        }
        else
        {
            sprintf(buff, "%s/%s", pathr, file);

            char * a = strstr((char *) buff, ".66600");

            if(a && a[6] == 0) a[0] = 0;
            //if(a) a[0] = 0;

            int n;

            for(n = 0; n < nfilecached; n++)
                if(!strcmp(&filecached[n][0][0], buff)) break;

            if(n == nfilecached) return 0;

            sprintf(fast_files[fast_num_files].pathw, "%s/%s", path_cache, file);
        }
    }
    else
        sprintf(fast_files[fast_num_files].pathw, "%s/%s", pathw, file);

    // zero files
    if((s64) s.st_size == 0LL)
    {
        int fdw;

        if(sysFsOpen(fast_files[fast_num_files].pathw, SYS_O_CREAT | SYS_O_TRUNC | SYS_O_WRONLY, &fdw, 0,0) != 0)
        {
            DPrintf("%s:\n%s\n\n", language[FASTCPADD_ERROPEN], fast_files[current_fast_file_r].pathw);
            abort_copy = 1;
            return -1;
        }

        sysFsClose(fdw);

        sysFsChmod(fast_files[fast_num_files].pathw, FS_S_IFMT | 0777);

        DPrintf("%s ", language[FASTCPADD_COPYING]);
        DPrintf("%s\n", fast_files[current_fast_file_r].pathr);

        DPrintf("w%s 0 B\n", language[GLUTIL_WROTE]);

        file_counter++;

        return 0;
    }

    if(fast_files[fast_num_files].bigfile_mode == 2)
    {
        fast_files[fast_num_files].pathw[strlen(fast_files[fast_num_files].pathw) - 6] = 0; // truncate the extension
        fast_files[fast_num_files].pos_path = strlen(fast_files[fast_num_files].pathr) - 6;
        fast_files[fast_num_files].pathr[fast_files[fast_num_files].pos_path] = 0; // truncate the extension
    }

    if(fast_files[fast_num_files].bigfile_mode == 3)
    {
        //fast_files[fast_num_files].pathw[strlen(fast_files[fast_num_files].pathw)-6] = 0; // truncate the extension
        fast_files[fast_num_files].pos_path = strlen(fast_files[fast_num_files].pathr);
        //fast_files[fast_num_files].pathr[fast_files[fast_num_files].pos_path] = 0; // truncate the extension
    }

    if(copy_mode == 1)
    {
        if(((s64) s.st_size) >= 0x100000000LL)
        {
            fast_files[fast_num_files].bigfile_mode = 1;
            fast_files[fast_num_files].pos_path     = strlen(fast_files[fast_num_files].pathw);
            fast_files[fast_num_files].giga_counter = 0;

            copy_is_split = 1;
        }
    }

    fast_files[fast_num_files].number_frag = 0;
    fast_files[fast_num_files].fl = 1;

    fast_files[fast_num_files].len = (s64) s.st_size;
    fast_files[fast_num_files].use_doublebuffer = 0;
    fast_files[fast_num_files].readed = 0LL;
    fast_files[fast_num_files].writed = 0LL;

    fast_files[fast_num_files].t_read.fd  = -1;
    fast_files[fast_num_files].t_write.fd = -1;

    if(((s64) s.st_size) >= (s64) FILESIZE_MAX)
    {
        size_mem = FILESIZE_MAX;
        fast_files[fast_num_files].use_doublebuffer = 1;
    }
    else size_mem = ((int) s.st_size);


    fast_files[fast_num_files].mem = memalign(32, size_mem+size_mem * (fast_files[fast_num_files].use_doublebuffer != 0) + MAX_PATH_LEN);
    fast_files[fast_num_files].size_mem = size_mem;

    if(!fast_files[fast_num_files].mem)
    {
        DPrintf("%s\n", language[FASTCPADD_FAILFASTFILE]);
        abort_copy = 1;
        return -1;
    }

    fast_used_mem += size_mem;

    fast_num_files++;

    return 0;
}

void fast_func_read(sysFSAio *xaio, s32 error, s32 xid, u64 size)
{
    t_fast_files* fi = (t_fast_files *) xaio->usrdata;

    if(error != 0 || size != xaio->size)
    {
        fi->readed = -1; return;
    }
    else
        fi->readed += (s64) size;

    fast_read = 2;fi->fl = 3;

}

static volatile s64 write_progress = 0;

void fast_func_write(sysFSAio *xaio, s32 error, s32 xid, u64 size)
{
    t_fast_files* fi = (t_fast_files *) xaio->usrdata;

    if(error != 0 || size != xaio->size)
        fi->writed = -1;
    else
    {
        fi->writed += (s64) size;
        fi->giga_counter += (int) size;
        global_device_bytes += (s64) size;
        write_progress = (s64) size;
    }

    fast_writing = 2;
}

int fast_copy_process()
{

    int n;

    int fdr, fdw;

    static int id_r = -1, id_w = -1;

    int error = 0;
    int time_left = 0;

    int i_reading = 0;

    s64 write_end = 0, write_size = 0;

    write_progress = 0;

    current_fast_file_r = current_fast_file_w = 0;


    while(current_fast_file_r < fast_num_files ||
          current_fast_file_w < fast_num_files ||
          fast_writing || files_opened)
    {
        if(abort_copy) break;

        // open read
        if(current_fast_file_r < fast_num_files && fast_files[current_fast_file_r].fl == 1 && !i_reading && !fast_read && files_opened < 2)
        {
            fast_files[current_fast_file_r].readed = 0LL;
            fast_files[current_fast_file_r].writed = 0LL;
            fast_files[current_fast_file_r].off_readed = 0LL;

            fast_files[current_fast_file_r].t_read.fd  = -1;
            fast_files[current_fast_file_r].t_write.fd = -1;

            if(fast_files[current_fast_file_r].bigfile_mode == 1)
            {
                DPrintf("%s >= 4GB\n", language[GLUTIL_SPLITFILE]);
                DPrintf(" %s\n", fast_files[current_fast_file_r].pathr);

                sprintf(&fast_files[current_fast_file_r].pathw[fast_files[current_fast_file_r].pos_path], ".666%2.2i",
                         fast_files[current_fast_file_r].number_frag);
            }

            if(fast_files[current_fast_file_r].bigfile_mode == 2)
            {
                DPrintf("%s >= 4GB\n", language[FASTCPPRC_JOINFILE]);
                DPrintf(" %s\n", fast_files[current_fast_file_r].pathw);

                sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".666%2.2i",
                         fast_files[current_fast_file_r].number_frag);
            }

            if(fast_files[current_fast_file_r].bigfile_mode == 3)
            {

                DPrintf("%s >= 4GB\n", language[FASTCPPRC_JOINFILE]);
                DPrintf(" %s\n", fast_files[current_fast_file_r].pathw);

                if(fast_files[current_fast_file_r].number_frag!=0)
                    sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".%i.part",
                             fast_files[current_fast_file_r].number_frag);
            }

            //DPrintf("Open R: %s\nOpen W: %s, Index %i/%i\n", fast_files[current_fast_file_r].pathr,
            //  fast_files[current_fast_file_r].pathw, current_fast_file_r, fast_num_files);

            int err;


            err = sysFsOpen(fast_files[current_fast_file_r].pathr, SYS_O_RDONLY, &fdr, 0,0);

            if( err != 0)
            {
                DPrintf("Error Opening (read): %x %i\n%s\n\n", err, files_opened, fast_files[current_fast_file_r].pathr);
                error =-1;
                break;

            }
            else files_opened++;

            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1); write_progress = 0;
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, getfilename_part(fast_files[current_fast_file_r].pathw));
            if(sysFsOpen(fast_files[current_fast_file_r].pathw, SYS_O_CREAT | SYS_O_TRUNC | SYS_O_WRONLY, &fdw, 0, 0) != 0)
            {
                DPrintf("Error Opening (write):\n%s\n\n", fast_files[current_fast_file_r].pathw);
                error = -2;
                break;
            }
            else files_opened++;

            //if(fast_files[current_fast_file_r].bigfile_mode == 0) { }
                    //DPrintf("Copying %s\n", fast_files[current_fast_file_r].pathr);
            if(fast_files[current_fast_file_r].bigfile_mode && fast_files[current_fast_file_r].bigfile_mode!=3)
                DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_r].number_frag);

            if(fast_files[current_fast_file_r].bigfile_mode == 3)
                DPrintf("    -> .%i.part\n", fast_files[current_fast_file_r].number_frag);

            fast_files[current_fast_file_r].t_read.fd = fdr;

            fast_files[current_fast_file_r].t_read.offset = 0LL;
            fast_files[current_fast_file_r].t_read.buffer_addr = (u32) (u64) fast_files[current_fast_file_r].mem;

            fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].len - fast_files[current_fast_file_r].readed;

            if((s64) fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
                fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

            fast_files[current_fast_file_r].t_read.usrdata = (u64 )&fast_files[current_fast_file_r];

            fast_files[current_fast_file_r].t_write.fd = fdw;
            fast_files[current_fast_file_r].t_write.usrdata = (u64 )&fast_files[current_fast_file_r];
            fast_files[current_fast_file_r].t_write.offset = 0LL;
            if(fast_files[current_fast_file_r].use_doublebuffer)
                fast_files[current_fast_file_r].t_write.buffer_addr =
                    (u32) (u64) (((char *) fast_files[current_fast_file_r].mem) + fast_files[current_fast_file_r].size_mem);
            else
                fast_files[current_fast_file_r].t_write.buffer_addr = (u32) (u64) fast_files[current_fast_file_r].mem;

            fast_read = 1; fast_files[current_fast_file_r].fl = 2;

            if(sysFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read) != 0)
            {
                id_r  = -1;
                error = -3;
                DPrintf("Fail to perform Async Read\n\n");
                fast_read = 0;
                break;
            }

            i_reading = 1;
        }

        // fast read end

        if(current_fast_file_r < fast_num_files && fast_files[current_fast_file_r].fl == 3 && !fast_writing)
        {
            id_r = -1;

            if(fast_files[current_fast_file_r].readed < 0LL)
            {
                DPrintf("Error Reading %s\n", fast_files[current_fast_file_r].pathr);
                error = -3;
                break;
            }

            // double buffer

            if(fast_files[current_fast_file_r].use_doublebuffer)
            {
                //DPrintf("Double Buff Write\n");

                current_fast_file_w = current_fast_file_r;

                memcpy(((char *) fast_files[current_fast_file_r].mem) + fast_files[current_fast_file_r].size_mem,
                fast_files[current_fast_file_r].mem, fast_files[current_fast_file_r].size_mem);

                fast_files[current_fast_file_w].t_write.size = fast_files[current_fast_file_r].t_read.size;

                if(fast_files[current_fast_file_w].bigfile_mode == 1)
                    fast_files[current_fast_file_w].t_write.offset = (s64) fast_files[current_fast_file_w].giga_counter;
                else
                    fast_files[current_fast_file_w].t_write.offset = fast_files[current_fast_file_w].writed;

                fast_writing = 1;

                if(sysFsAioWrite(&fast_files[current_fast_file_w].t_write, &id_w, fast_func_write) != 0)
                {
                    id_w  = -1;
                    error = -4;
                    DPrintf("Fail to perform Async Write\n\n");
                    fast_writing = 0;
                    break;
                }

                if(fast_files[current_fast_file_r].readed < fast_files[current_fast_file_r].len)
                {
                    fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].len - fast_files[current_fast_file_r].readed;

                    if((s64) fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
                            fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

                    fast_files[current_fast_file_r].fl = 2;
                    fast_files[current_fast_file_r].t_read.offset = fast_files[current_fast_file_r].readed - fast_files[current_fast_file_r].off_readed;

                    fast_read = 1;

                    if(sysFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read) != 0)
                    {
                        id_r = -1;
                        error = -3;

                        DPrintf("Fail to perform Async Read\n\n");

                        fast_read = 0;
                        break;
                    }
                }
                else
                {
                    if(fast_files[current_fast_file_r].bigfile_mode == 2 || fast_files[current_fast_file_r].bigfile_mode == 3)
                    {
                        sysFSStat s;

                        fast_files[current_fast_file_r].number_frag++;

                        if(fast_files[current_fast_file_r].number_frag)
                            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);

                        fast_files[current_fast_file_r].off_readed = fast_files[current_fast_file_r].readed;

                        if(fast_files[current_fast_file_r].bigfile_mode == 3)
                        {
                            DPrintf("    -> .%i.part\n", fast_files[current_fast_file_r].number_frag);

                            sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".%i.part",
                            fast_files[current_fast_file_r].number_frag);
                        }
                        else
                        {
                            DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_r].number_frag);

                            sprintf(&fast_files[current_fast_file_r].pathr[fast_files[current_fast_file_r].pos_path], ".666%2.2i",
                            fast_files[current_fast_file_r].number_frag);
                        }

                        if(sysFsStat(fast_files[current_fast_file_r].pathr, &s) < 0) {current_fast_file_r++; i_reading = 0;}
                        else
                        {
                            if(fast_files[current_fast_file_r].t_read.fd >= SUCCESS)
                            {
                                 int r = sysFsClose(fast_files[current_fast_file_r].t_read.fd);
                                 if(r < 0)
                                 {
                                    DPrintf("Error en Close (.666) %x\n", (u32) r);
                                    error = -4;
                                    break;
                                 }

                                 fast_read = 0;

                                 files_opened--;
                            }

                            fast_files[current_fast_file_r].t_read.fd = -1;

                            int err =sysFsOpen(fast_files[current_fast_file_r].pathr, SYS_O_RDONLY, &fdr, 0, 0);
                            if(err != 0)
                            {
                                DPrintf("Error Opening (read): %x\n%s\n\n", err, fast_files[current_fast_file_r].pathr);
                                error = -1;
                                break;
                            }
                            else
                                files_opened++;


                            fast_files[current_fast_file_r].t_read.fd = fdr;
                            fast_files[current_fast_file_r].len += (s64) s.st_size;
                            fast_files[current_fast_file_r].t_read.offset = 0LL;
                            fast_files[current_fast_file_r].t_read.buffer_addr = (u32) (u64) fast_files[current_fast_file_r].mem;

                            fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].len-fast_files[current_fast_file_r].readed;
                            if((s64) fast_files[current_fast_file_r].t_read.size > fast_files[current_fast_file_r].size_mem)
                                fast_files[current_fast_file_r].t_read.size = fast_files[current_fast_file_r].size_mem;

                            fast_files[current_fast_file_r].t_read.usrdata = (u64 )&fast_files[current_fast_file_r];

                            fast_read = 1;

                            if(sysFsAioRead(&fast_files[current_fast_file_r].t_read, &id_r, fast_func_read) != 0)
                            {
                                id_r  = -1;
                                error = -3;
                                DPrintf("Fail to perform Async Read\n\n");
                                fast_read = 0;
                                break;
                            }

                            fast_files[current_fast_file_r].fl = 2;

                        }
                    }
                    else
                    {
                        fast_files[current_fast_file_r].fl = 5;
                        current_fast_file_r++;
                        i_reading = 0;
                    }

                }

            }
            else
            {
                // single buffer

                //DPrintf("Single Buff Write\n");

                current_fast_file_w = current_fast_file_r;
                fast_files[current_fast_file_w].t_write.size = fast_files[current_fast_file_r].t_read.size;

                fast_files[current_fast_file_w].t_write.offset = fast_files[current_fast_file_w].writed;

                fast_writing = 1;

                if(sysFsAioWrite(&fast_files[current_fast_file_w].t_write, &id_w, fast_func_write) != 0)
                {
                    id_w  = -1;
                    error = -4;
                    DPrintf("Fail to perform Async Write\n\n");
                    fast_writing = 0;
                    break;
                }

                current_fast_file_r++;
                i_reading = 0;
            }
        }

        // fast write end
        if(fast_writing > 1)
        {
            fast_writing = 0;
            id_w = -1;

            if(fast_files[current_fast_file_w].writed < 0LL)
            {
                DPrintf("Error Writing %s\n", fast_files[current_fast_file_w].pathw);
                error = -4;
                break;
            }

            write_end  = fast_files[current_fast_file_w].writed;
            write_size = fast_files[current_fast_file_w].len;

            if(fast_files[current_fast_file_w].writed >= fast_files[current_fast_file_w].len)
            {
                if(fast_files[current_fast_file_w].t_read.fd >= SUCCESS)
                {
                    int r = sysFsClose(fast_files[current_fast_file_w].t_read.fd);
                    if(r < 0)
                    {
                        DPrintf("Error en Close (read) %x\n", (u32) r);
                        error = -4;
                        break;
                    }

                    fast_read = 0;

                    files_opened--;
                }

                fast_files[current_fast_file_w].t_read.fd = -1;

                if(fast_files[current_fast_file_w].t_write.fd >= SUCCESS)
                {
                    int r= sysFsClose(fast_files[current_fast_file_w].t_write.fd);
                    if(r < 0)
                    {
                        DPrintf("Error en Close (write) %x\n", (u32) r);
                        error = -4;
                        break;
                    }

                    files_opened--;

                    fast_files[current_fast_file_w].t_write.fd = -1;

                    sysFsChmod(fast_files[current_fast_file_w].pathw, FS_S_IFMT | 0777);
                }



                if(fast_files[current_fast_file_w].bigfile_mode == 1)
                {
                    fast_files[current_fast_file_w].pathw[fast_files[current_fast_file_w].pos_path] = 0;
                }

                if(write_size < 1024LL)
                {
                    DPrintf("%s (%lli B)\n", fast_files[current_fast_file_w].pathw, write_size);
                }
                else if(write_size < 0x100000LL)
                {
                    DPrintf("%s (%lli KB)\n", fast_files[current_fast_file_w].pathw, write_size  / 1024LL);
                }
                else
                {
                    DPrintf("%s (%lli MB)\n", fast_files[current_fast_file_w].pathw, write_size / 0x100000LL);
                }


                fast_files[current_fast_file_w].fl = 4; //end of proccess

                fast_files[current_fast_file_w].writed = -1LL;
                current_fast_file_w++;
                //if(current_fast_file_r<current_fast_file_w) current_fast_file_w=current_fast_file_r;
                file_counter++;
            }
            else if(fast_files[current_fast_file_w].bigfile_mode == 1 && fast_files[current_fast_file_w].giga_counter >= 0x40000000)
            {
                // split big files

                if(fast_files[current_fast_file_w].t_write.fd >= SUCCESS)
                {
                    int r = sysFsClose(fast_files[current_fast_file_w].t_write.fd);
                    if(r < 0)
                    {
                        DPrintf("Error en Close (write split) %x\n", (u32) r);
                        error = -4;
                        break;
                    }
                    files_opened--;

                    fast_files[current_fast_file_w].t_write.fd = -1;

                    sysFsChmod(fast_files[current_fast_file_w].pathw, FS_S_IFMT | 0777);
                }


                fast_files[current_fast_file_w].giga_counter = 0;
                fast_files[current_fast_file_w].number_frag++;

                sprintf(&fast_files[current_fast_file_w].pathw[fast_files[current_fast_file_w].pos_path], ".666%2.2i",
                    fast_files[current_fast_file_w].number_frag);
                DPrintf("    -> .666%2.2i\n", fast_files[current_fast_file_w].number_frag);
                //msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1); write_progress = 0;
                msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, getfilename_part(fast_files[current_fast_file_w].pathw));
                if(sysFsOpen(fast_files[current_fast_file_w].pathw, SYS_O_CREAT | SYS_O_TRUNC | SYS_O_WRONLY, &fdw, 0, 0) != 0)
                {
                    DPrintf("Error Opening2 (write):\n%s\n\n", fast_files[current_fast_file_w].pathw);
                    error = -2;
                    break;
                }
                else files_opened++;

                fast_files[current_fast_file_w].t_write.fd = fdw;
            }
        }

        int seconds = (int) (time(NULL) - time_start);
        //calc time left
        if(!copy_total_size)
        {
            if(progress_action2)
            {
                abort_copy = 1;

                DPrintf("%s \n", language[GLUTIL_ABORTEDUSER]);
                error = -666;
            }

            if(write_progress != 0)
            {
                  bar1_countparts += (100.0f*((double) write_progress)) / ((double) copy_total_size);
                  bar2_countparts += (100.0f*((double) write_progress)) / ((double) write_size);

                  write_progress = 0;
            }

            sprintf(string1, "(%i/%i)", file_counter, total_fast_files);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, string1);

            if(bar1_countparts >= 1.0f)
            {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) bar1_countparts);
                bar1_countparts-= (float) ((u32) bar1_countparts);
            }

            if(bar2_countparts >= 1.0f)
            {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, (u32) bar2_countparts);
                bar2_countparts-= (float) ((u32) bar2_countparts);
            }

            sprintf(string1, "%s: %i (%2.2i%%) %s: %2.2i:%2.2i:%2.2i  Vol: %1.2f GB\n", language[FASTCPPRC_COPYFILE], file_counter,
                    (int)(write_end * 100ULL / write_size), language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60,
                    ((double) global_device_bytes) / GIGABYTES);
        }
        else
        {
            int tleft = ((copy_total_size - global_device_bytes) * seconds) / global_device_bytes;
            if(abs(time_left - tleft) >= 10) //more than 10 secs diff, update time
                time_left = tleft;

            if(progress_action2)
            {
                abort_copy = 1;
                DPrintf("%s \n", language[GLUTIL_ABORTEDUSER]);
                error = -666;
            }

            if(write_progress != 0)
            {
                bar1_countparts += (100.0f*((double) write_progress)) / ((double) copy_total_size);
                bar2_countparts += (100.0f*((double) write_progress)) / ((double) write_size);
                write_progress = 0;
            }

            sprintf(string1, "(%i/%i)", file_counter, total_fast_files);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, string1);

            if(bar1_countparts >= 1.0f)
            {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) bar1_countparts);
                bar1_countparts-= (float) ((u32) bar1_countparts);
            }

            if(bar2_countparts >= 1.0f)
            {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX1, (u32) bar2_countparts);
                bar2_countparts-= (float) ((u32) bar2_countparts);
            }

            sprintf(string1, "%s: %i (%2.2i%%) %s: %2.2i:%2.2i:%2.2i %1.2f/%1.2f GB\n", language[FASTCPPRC_COPYFILE], file_counter,
                    (int)(write_end * 100ULL / write_size), language[GLUTIL_TIMELEFT], time_left / 3600, (time_left / 60) % 60, time_left % 60,
                    ((double) global_device_bytes) / GIGABYTES, ((double) copy_total_size/ GIGABYTES));
        }

        cls2();

        DbgHeader( string1);

        DbgMess(language[GLUTIL_HOLDTRIANGLEAB]);

        DbgDraw();

        tiny3d_Flip();

        if(ps3pad_poll())
        {
            abort_copy = 1;
            DPrintf("%s \n", language[GLUTIL_ABORTEDUSER]);
            error = -666;
            break;
        }
    }

    if(error && error != -666)
    {
        DPrintf(language[FASTCPPTC_OPENERROR], files_opened);
        DPrintf("\n");

        cls2();

        DbgHeader( string1);
        DbgMess(language[GLUTIL_HOLDTRIANGLEAB]);

        DbgDraw();

        tiny3d_Flip();

        usleep(20*1000000);
    }


    if(fast_writing == 1 && id_w >= 0)
    {
        sysFsAioCancel(id_w);
        msgDialogAbort();
        id_w = -1;
        usleep(200000);
    }

    fast_writing = 0;

    if(fast_read == 1 && id_r >= 0)
    {
        sysFsAioCancel(id_r);
        id_r =-1;
        usleep(200000);
    }

    fast_read = 0;

    for(n = 0; n < fast_num_files; n++)
    {
        if(fast_files[n].t_read.fd >= SUCCESS)
        {
            sysFsClose(fast_files[n].t_read.fd);
            fast_files[n].t_read.fd = -1;
            files_opened--;
        }

        if(fast_files[n].t_write.fd >= SUCCESS)
        {
            sysFsClose(fast_files[n].t_write.fd);
            fast_files[n].t_write.fd = -1;
            files_opened--;
        }

        if(fast_files[n].mem) free(fast_files[n].mem); fast_files[n].mem = NULL;
    }

    fast_num_files = 0;
    fast_writing = 0;
    fast_used_mem = 0;

    current_fast_file_r = current_fast_file_w = 0;

    if(error) {msgDialogAbort(); abort_copy = 666; DPrintf("Error at point #1\n");}

    return error;
}
