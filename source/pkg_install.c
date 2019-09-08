/*
    (c) 2011 Hermes <www.elotrolado.net>
    IrisManager (HMANAGER port) (c) 2011 D_Skywalk <http://david.dantoine.org>
    IrisManager (HMANAGER port 4.30) (c) 2012 D_Skywalk/Estwald

    HMANAGER4 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    HMANAGER4 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with HMANAGER4.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <lv2/process.h>
#include <ppu-lv2.h>
#include <sys/stat.h>

#include <sys/file.h>
#include <lv2/sysfs.h>

#include <io/pad.h>

#include <tiny3d.h>
#include "libfont2.h"
#include "language.h"
#include "syscall8.h"
#include "utils.h"
#include "main.h"
#include "gfx.h"
#include "pad.h"
#include "ntfs.h"

extern char temp_buffer[8192];
extern char self_path[MAXPATHLEN];

#undef AUTO_BUTTON_REP2
#define AUTO_BUTTON_REP2(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 10) {v = 0; new_pad |= b;} \
                                 } else v = 0;

int copy_async(char *path1, char *path2, u64 size, char *progress_string1, char *progress_string2);

typedef struct {
    u32 flags;
    char name[256];
} t_list;

static t_list * entries = NULL;

static char path_hdd[1024];
static char path_name[1024];

static int compare(const void *va, const void *vb)
{
    t_list * a =  (t_list *) va;
    t_list * b =  (t_list *) vb;

    if((a->flags & 1)>(b->flags & 1) || ((a->flags & 1)==(b->flags & 1) && strcmp(a->name, b->name) <0) || !strcmp(a->name,"..")) return -1;
    else return 1;
}

void install_pkg(char *path, char *filename, u8 show_done);

extern int set_install_pkg;

void draw_pkginstall(float x, float y)
{
    float x2, y2;
    float xold = x, yold = y;

    int n;
    int autolist = 2;
    int nentries = 0;
    DIR  *dir;
    int sel = 0;
    int pos = 0;
    int flash = 0;
    u64 frame_count = 0;

    int is_hdd = 0;

    char path_pkghdd[256];

    entries = (t_list *) malloc(sizeof(t_list)*1024);
    if(!entries) return; // out of memory

    sprintf(path_name, "/dev_usb000");
    //sprintf(path_hdd, "%s/PKG", self_path);
    sprintf(path_hdd, "/dev_hdd0/packages");
    //sprintf(path_pkghdd, "%s/PKG", self_path);
    sprintf(path_pkghdd, "/dev_hdd0/packages");

    while(true)
    {
        flash = (frame_count >> 4) & 1;

        frame_count++;

        if((frame_count & 7) == 1)
        {
            char l= path_name[11]; path_name[11] = 0; // break to "/dev_usb00x"
            dir = opendir(path_name);
            if(dir) closedir(dir);
            else
            {
                for(n = 0; n < 10; n++)
                {
                    sprintf(path_name,"/dev_usb00%c", 48+n);
                    dir = opendir(path_name);
                    if(dir) {closedir(dir);break;}
                }

                if(n<10) {autolist = 2 + n;nentries = 0;}
                else
                {
                    sprintf(path_name,"/dev_usb00%c", 48);

                    if(!is_hdd)
                    {
                        nentries = 0;
                        dir = opendir(path_pkghdd);

                        if(dir)  {closedir(dir); autolist = 12;}
                    }
                }
            }
            path_name[11]= l;
        }


        if(autolist)
        {
            pos= sel = 0; nentries = 0;

            if(autolist > 1 && autolist!=12)
                {sprintf(path_name,"/dev_usb00%c", 46 + autolist);}

            dir = opendir(path_name);
            is_hdd=0;
            if(!dir)
            {
                dir = opendir(path_pkghdd);
                if(dir) {is_hdd=1;autolist=12;}
                else autolist = 2;
            }

            if(dir)
            {
                while(true)
                {
                    if(nentries >= 1024) break;

                    struct dirent *entry = readdir(dir);

                    if(!entry) break;
                    if(entry->d_name[0] == '.' && entry->d_name[1] == 0) continue;

                    if((entry->d_type & DT_DIR) && is_hdd) continue;

                    if((entry->d_type & DT_DIR)) entries[nentries].flags = 1;
                    else
                    {
                        int flen = strlen(entry->d_name) - 4;

                        if (flen < 0) continue;

                        if(strncmp(entry->d_name + flen, ".pkg", 5) && strncmp(entry->d_name + flen, ".PKG", 5)) continue;

                        entries[nentries].flags = 0;
                    }

                    strncpy(entries[nentries].name, entry->d_name, 256);
                    nentries++;
                }

                closedir(dir);

                qsort(entries, nentries, sizeof(t_list), compare);

                autolist = 0;
            }
        }

        x=xold; y=yold;

        cls();
        SetCurrentFont(FONT_TTF);

        // header title

        DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

        SetFontColor(0xffffffff, 0x00000000);

        SetFontSize(18, 20);

        SetFontAutoCenter(0);

        DrawFormatString(x, y, " %s", language[PKG_HEADER]);
        update_twat(true);

        y += 24;

        DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

        y2 = y;

        if(nentries)
        {
            for(n = 0; n < 18; n++)
            {
                if(pos + n >= nentries) break;

                x2 = x;

                if(entries[pos + n].flags & 1)
                    SetFontColor(0x009fffff, 0x00000000);
                else
                    SetFontColor(0xffffffff, 0x00000000);

                if((sel == (pos + n) && flash) || sel != (pos + n))
                {
                    if(entries[pos + n].flags & 1)
                        DrawFormatString(x2, y2, "<%s>", entries[pos + n].name);
                    else
                        DrawFormatString(x2, y2, "%s", entries[pos + n].name);
                }
                y2 += 24;
            }
        }
        else
        {
            SetFontColor(0xffffffff, 0x00000000);
            x2 = x;
            if(flash) DrawFormatString(x2, y2, "%s", language[PKG_INSERTUSB]);
        }

        SetFontColor(0xffffffff, 0x00000000);

        tiny3d_Flip();

        ps3pad_read();

        static int auto_up = 0, auto_down = 0;

        AUTO_BUTTON_REP2(auto_up, BUTTON_UP)
        AUTO_BUTTON_REP2(auto_down, BUTTON_DOWN)

        if(new_pad & (BUTTON_TRIANGLE | BUTTON_CIRCLE_))
        {
            free(entries);
            if(set_install_pkg) exit(0);
            return;
        }
        else if(new_pad & BUTTON_CROSS_)
        {
            if(entries[sel].flags & 1)
            {
                // change dir
                if(is_hdd)
                {
                    if(!strcmp(entries[sel].name,".."))
                    {
                        n = strlen(path_hdd);
                        while(n > 0 && path_hdd[n] != '/') n--;

                        if(n!=0) path_hdd[n] = 0;
                    }
                    else
                    {
                        strcat(path_hdd, "/");
                        strcat(path_hdd, entries[sel].name);
                    }
                }
                else
                {
                    if(!strcmp(entries[sel].name,".."))
                    {
                        n = strlen(path_name);
                        while(n > 0 && path_name[n] != '/') n--;

                        if(n != 0) path_name[n] = 0;
                    }
                    else
                    {
                        strcat(path_name, "/");
                        strcat(path_name, entries[sel].name);
                    }
                }
                autolist = 1;
            }
            else
            {
                //install
                if(is_hdd)
                {
                    autolist = 12;nentries = 0;
                    install_pkg(path_hdd, entries[sel].name, 1);
                }
                else
                    install_pkg(path_name, entries[sel].name, 1);
            }
        }
        else if(new_pad & BUTTON_UP)
        {
            auto_up = 1;
            if(sel > 0) sel--;
            else {sel = (nentries - 1); pos = sel - 17;}
            if(sel < pos + 9) pos--;
            if(pos < 0) pos = 0;
        }
        else if(new_pad & BUTTON_DOWN)
        {
            auto_down = 1;
            if(sel < (nentries-1)) sel++;
            else {sel = 0;pos = 0;}
            if(sel > (pos + 9)) pos++;
            if(pos > (nentries - 1)) {pos = 0; sel = 0;}
        }
   }
}

extern int firmware;
int flag_build = 1;

int build_pkg(char *path, char *title, char *path_icon, u64 size);

int use_folder = 0;
char pkg_folder[2][16]= {"game_pkg", "task"};

void install_pkg(char *path, char *filename, u8 show_done)
{
    u32 blockSize;
    u64 freeSize;
    u64 free_hdd0;
    struct stat s;
    int n;
    char string1[] = "80000000";
    int free_slot = -1;

    bool is_ntfs = is_ntfs_path(path);

    if(firmware == 0x341C || firmware == 0x355C || firmware == 0x355D) use_folder = 1;

    sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
    free_hdd0 = ( ((u64)blockSize * freeSize));

    sprintf(temp_buffer, "%s/%s", path, filename);

    if((!is_ntfs && stat(temp_buffer, &s) == 0) || (is_ntfs && ps3ntfs_stat(temp_buffer, &s) == 0))
    {
        if(s.st_size + 0x40000000ULL > free_hdd0/*|| s.st_size >= 0x100000000ULL*/)
        {
            DrawDialogOK(language[PKG_ERRTOBIG]);
            return;
        }
    }
    else return; // error

    sprintf(temp_buffer + 1024, "%s\n\n%s", language[PKG_WANTINSTALL], filename);

    if(show_done && (DrawDialogYesNo(temp_buffer + 1024) != YES)) return;

//    sys8_perm_mode(1);

    mkdir("/dev_hdd0/vsh/game_pkg", S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

    for(n = 0; n < 16; n++) {
        string1[6]= 48 + ((n/10) % 10);
        string1[7]= 48 + (n % 10);
        DIR  *dir;
        sprintf(temp_buffer + 1024, "/dev_hdd0/vsh/%s/%s",&pkg_folder[use_folder][0], string1);

        dir = opendir(temp_buffer + 1024);
        if(!dir) {if(free_slot < 0) free_slot = n;}
        else {
            closedir(dir);
            sprintf(temp_buffer + 1024, "/dev_hdd0/vsh/%s/%s/%s", &pkg_folder[use_folder][0], string1, filename);

            if(stat(temp_buffer + 1024, &s) == 0) break;
        }
    }

//    sys8_perm_mode(0);

    if(n < 16)
    {
        DrawDialogOK(language[PKG_ERRALREADY]);return;
    }

    if(n == 16 && free_slot < 0)
    {
        DrawDialogOK(language[PKG_ERRFULLSTACK]);return;
    }

    string1[6]= 48 + ((free_slot/10) % 10);
    string1[7]= 48 + (free_slot % 10);

    sprintf(temp_buffer + 1024, "%s/%s/%s", self_path, string1, filename);
    sprintf(temp_buffer + 2048, "%s/%s", self_path, string1);

    mkdir(temp_buffer + 2048, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

    sprintf(temp_buffer + 3072, "/dev_hdd0/vsh/%s/%s/ICON_FILE", &pkg_folder[use_folder][0], string1);

    if(build_pkg(temp_buffer + 2048, filename, temp_buffer + 3072, s.st_size))
    {
        DeleteDirectory(temp_buffer + 2048);rmdir(temp_buffer + 2048);
        DrawDialogOK(language[PKG_ERRBUILD]);return;
    }

    int ret = 0;

    if(!strncmp(temp_buffer, "/dev_hdd0", 9))
        ret= sysLv2FsRename(temp_buffer, temp_buffer  + 1024);
    else
        ret = copy_async(temp_buffer, temp_buffer + 1024, s.st_size, language[PKG_COPYING], NULL);

    if(ret < 0) {DeleteDirectory(temp_buffer + 2048);rmdir(temp_buffer + 2048);}
    if(ret == -3) {DrawDialogOK(language[OUT_OFMEMORY]);return;}
    if(ret == -1) {DrawDialogOK(language[PKG_ERROPENING]);return;}

    if(ret == -2) {DrawDialogOK(language[PKG_ERRCREATING]);return;}

    if(ret == -4 || ret == -5)
    {
        DrawDialogOK(language[PKG_ERRREADING]);return;
    }

    if(ret == -6 || ret == -7)
    {
        DrawDialogOK(language[PKG_ERRREADING]);return;
    }
    if(ret==-666)
    {
        DrawDialogOK(language[GLUTIL_ABORTEDUSER]);return;
    }


    sprintf(temp_buffer, "%s/ICON0.PNG", self_path);
    sprintf(temp_buffer + 1024, "%s/%s/ICON_FILE", self_path, string1);

    int file_size;
    char *file = LoadFile(temp_buffer, &file_size);
    if(!file) {
        DeleteDirectory(temp_buffer + 2048);rmdir(temp_buffer + 2048);
        DrawDialogOK(language[PKG_ERRLICON]);return;
    }

    if(SaveFile(temp_buffer + 1024, file, file_size)) {
        free(file); DeleteDirectory(temp_buffer + 2048);rmdir(temp_buffer + 2048);DrawDialogOK("Error Saving ICON file");return;
    }

    free(file);

    sprintf(temp_buffer, "/dev_hdd0/vsh/%s/%s", &pkg_folder[use_folder][0], string1);

//    sys8_perm_mode(1);
    n = sysLv2FsRename(temp_buffer  + 2048, temp_buffer);
//    sys8_perm_mode(0);

    if(n != 0)
    {
        DeleteDirectory(temp_buffer + 2048);rmdir(temp_buffer + 2048);
        sprintf(temp_buffer + 1024, language[PKG_ERRMOVING]);
        DrawDialogOK(temp_buffer + 1024);
        return;
    }
    else
    {
        if(firmware == 0x341C || firmware == 0x355C || firmware == 0x355D) set_install_pkg=1;
    }

    if(show_done) DrawDialogOK(language[GAMECPYSL_DONE]);
}

unsigned char data_pdb[112] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x6B, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x03, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00
};

unsigned char data_pdb2[13] = {
    0xE2, 0x98, 0x85, 0x20, 0x49, 0x6E, 0x73, 0x74, 0x61, 0x6C, 0x6C, 0x20, 0x22
};

#undef FWRITE
#define FWRITE(a, b, c) if(fwrite(a, 1, b, c)!=b) goto error

int build_pkg(char *path, char *title, char *path_icon, u64 size)
{
    FILE *fp1;
    FILE *fp2 = NULL;

    u32 data;
    u8 data2;

    sprintf(temp_buffer + 4096, "%s/%s", path, "d0.pdb");
    fp1 = fopen(temp_buffer + 4096, "wb");

    sprintf(temp_buffer + 4096, "%s/%s", path, "d1.pdb");
    if(!use_folder) fp2 = fopen(temp_buffer + 4096, "wb");

    if(fp1 && (fp2 || use_folder))
    {
        FWRITE(data_pdb, 112, fp1);
        if(!use_folder) FWRITE(data_pdb, 112, fp2);

        data2 = (size>>32);
        FWRITE(&data2, 1, fp1);
        if(!use_folder) FWRITE(&data2, 1, fp2);

        data = (size);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = 0;
        FWRITE(&data, 3, fp1);
        if(!use_folder) FWRITE(&data, 3, fp2);

        data = (0xCE000000);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = (0x8000000);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = (0x8000000);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data2 = (size>>32);
        FWRITE(&data2, 1, fp1);
        if(!use_folder) FWRITE(&data2, 1, fp2);

        data = (size);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = 0;
        FWRITE(&data, 3, fp1);
        if(!use_folder) FWRITE(&data, 3, fp2);

        data2 = 0x69;
        FWRITE(&data2, 1, fp1);
        if(!use_folder) FWRITE(&data2, 1, fp2);

        data = (strlen(title) + 15);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        FWRITE(data_pdb2, 13, fp1);
        if(!use_folder) FWRITE(data_pdb2, 13, fp2);

        FWRITE(title, strlen(title), fp1);
        if(!use_folder) FWRITE(title, strlen(title), fp2);

        data2 = 0x22;
        FWRITE(&data2, 1, fp1);
        if(!use_folder) FWRITE(&data2, 1, fp2);

        data2 = 0x0;
        FWRITE(&data2, 1, fp1);
        if(!use_folder) FWRITE(&data2, 1, fp2);

        data = (0xCB);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = (strlen(title) + 1);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        FWRITE(title, strlen(title)+1, fp1);
        if(!use_folder) FWRITE(title, strlen(title)+1, fp2);

        data = (0xda);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = (1);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = (0x1000000);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data2 = 0xcd;
        FWRITE(&data2, 1, fp1);
        if(!use_folder) FWRITE(&data2, 1, fp2);

        data = (1);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = 0x0;
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data2 = 0x6a;
        FWRITE(&data2, 1, fp1);

        data2 = 0x0;
        if(!use_folder) FWRITE(&data2, 1, fp2);

        data = strlen(path_icon) +1;
        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        FWRITE(&data, 4, fp1);
        if(!use_folder) FWRITE(&data, 4, fp2);

        FWRITE(path_icon, data, fp1);
        if(!use_folder) FWRITE(path_icon, data, fp2);

        data = (0x6a);
        if(!use_folder) FWRITE(&data, 4, fp2);

        data = (strlen(path_icon) + 1);
        if(!use_folder)
        {
            FWRITE(&data, 4, fp2);
            FWRITE(&data, 4, fp2);

            FWRITE(path_icon, strlen(path_icon)+1, fp2);
        }
    }

    if(fp1) fclose(fp1);
    if(fp2) fclose(fp2);
    if(!use_folder)
    {
        sprintf(temp_buffer + 4096, "%s/%s", path, "f0.pdb");
        fp1 = fopen(temp_buffer + 4096, "wb");
        if(fp1) fclose(fp1);
    }
    return 0;

error:

    if(fp1) fclose(fp1);
    if(fp2) fclose(fp2);

    return -1;
}

static volatile int progress_action = 0;

static void progress_callback(msgButton button, void *userdata)
{
    switch(button)
    {
        case MSG_DIALOG_BTN_OK:
            progress_action = 1;
            break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
            progress_action = 2;
            break;
        case MSG_DIALOG_BTN_NONE:
            progress_action = -1;
            break;
        default:
            break;
    }
}

static sysFSAio t_read;  // used for async read
static sysFSAio t_write; // used for async write

typedef struct {
    s64 readed;
    s64 writed;
} t_async;

t_async async_data;

static void fast_func_read(sysFSAio *xaio, s32 error, s32 xid, u64 size)
{
    t_async* fi = (t_async *) xaio->usrdata;

    if(error != 0 || size != xaio->size)
    {
        fi->readed = -1; return;
    }
    else fi->readed += (s64) size;
}

static void fast_func_write(sysFSAio *xaio, s32 error, s32 xid, u64 size)
{
    t_async* fi = (t_async *) xaio->usrdata;

    if(error != 0 || size != xaio->size)
    {
        fi->writed = -1; return;
    }
    else fi->writed += (s64) size;
}

static msgType mdialogprogress = MSG_DIALOG_SINGLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;

int copy_async(char *path1, char *path2, u64 size, char *progress_string1, char *progress_string2)
{
    int ret;
    t_read.fd  = -1;
    t_write.fd = -1;
    int fdr, fdw;
    static int id_r = -1, id_w = -1;

    u64 pos = 0ULL;
    u64 pos2 = 0ULL;

    if(!strncmp(path1, "/dev_hdd0", 9) && !strncmp(path2, "/dev_hdd0", 9))
    {
        sysLv2FsUnlink(path2);
        return sysLv2FsLink(path1, path2);
    }

    bool is_ntfs = is_ntfs_path(path1);

    int alternate = 0;
    char *mem= malloc(0x20000);
    if(!mem) return -3;

    if(!is_ntfs)
    {
        if(sysFsAioInit(path1)!= 0)  return -1;

        if(sysFsOpen(path1, SYS_O_RDONLY, &fdr, 0,0) != 0)
        {
           free(mem);return -1;
        }
    }
    else
    {
        fdr= ps3ntfs_open(path1, O_RDONLY, 0);
        if(fdr < 0) {free(mem);return -1;}
    }

    if(sysFsAioInit(path2)!= 0)
    {
            free(mem);sysFsAioFinish(path1);sysFsClose(fdr); return -2;
    }

    if(sysFsOpen(path2, SYS_O_CREAT | SYS_O_TRUNC | SYS_O_WRONLY, &fdw, 0, 0) != 0)
    {
       free(mem);sysFsAioFinish(path1);sysFsAioFinish(path2); sysFsClose(fdr); return -2;
    }

    async_data.readed = -666;
    async_data.writed = -666;

    progress_action = 0;

    msgDialogOpen2(mdialogprogress, progress_string1, progress_callback, (void *) 0xadef0044, NULL);

    if(progress_string2) msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, progress_string2);
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    sysUtilCheckCallback();tiny3d_Flip();

    float parts = 100.0f / (((double) size)/ (double) 0x10000ULL);
    float cpart = 0;

    while(pos2 < size)
    {
        if(progress_action) {ret= -666;goto error;}

        if(async_data.readed == -666)
        {
            async_data.readed = -555;
            t_read.fd = fdr;
            t_read.offset = pos;
            t_read.buffer_addr = (u32) (u64) &mem[alternate*0x10000];
            t_read.size = size - pos; if(t_read.size > 0x10000ULL) t_read.size = 0x10000ULL;
            t_read.usrdata = (u64 ) &async_data;

            if(!is_ntfs)
            {
                if(sysFsAioRead(&t_read, &id_r, fast_func_read) != 0)
                {
                    ret= -4; goto error;
                }
            }
            else
            {
                int rd =ps3ntfs_read(fdr, &mem[alternate*0x10000], (size_t) t_read.size);
                if(rd < 0 || rd != (int)t_read.size) async_data.readed = -1;
                else async_data.readed = (s64) rd;
            }

        }
        if(async_data.readed == -1)
        {
                ret= -5; goto error;
        }
        else if(async_data.readed >= 0 && async_data.writed == -666)
        {
            async_data.writed = -555;
            t_write.fd = fdw;
            t_write.offset = t_read.offset;
            t_write.buffer_addr = t_read.buffer_addr;
            t_write.size = t_read.size;
            t_write.usrdata = (u64 ) &async_data;
            pos+= t_read.size;
            alternate^=1;
            async_data.readed = -666;

            if(sysFsAioWrite(&t_write, &id_w, fast_func_write) != 0)
            {
                 ret= -6; goto error;
            }
        }

        if(async_data.writed == -1)
        {
                 ret= -7; goto error;
        }
        else if(async_data.writed >= 0)
        {
            if(pos >= size) async_data.readed = -555; else async_data.writed = -666;
            pos2 = t_write.offset + t_write.size;

            cpart += parts;
            if(cpart >= 1.0f)
            {
                msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) cpart);
                cpart-= (float) ((u32) cpart);
                sysUtilCheckCallback();tiny3d_Flip();
            }
        }

        usleep(4000);
    }

    msgDialogAbort();
    if(!is_ntfs) sysFsClose(t_read.fd); else ps3ntfs_close(fdr);
    sysFsClose(t_write.fd);
    if(!is_ntfs) sysFsAioFinish(path1);
    sysFsAioFinish(path2);
    free(mem);
    usleep(10000);

    return 0;

error:
    msgDialogAbort();
    if(!is_ntfs) sysFsAioCancel(id_r);
    sysFsAioCancel(id_w);
    usleep(200000);
    if(!is_ntfs) sysFsClose(t_read.fd); else ps3ntfs_close(fdr);
    sysFsClose(t_write.fd);
    if(!is_ntfs) sysFsAioFinish(path1);
    sysFsAioFinish(path2);
    free(mem);

    return ret;
}
