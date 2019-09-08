/*
    (c) 2011 Hermes/Estwald <www.elotrolado.net>
    IrisManager (HMANAGER port) (c) 2011 D_Skywalk <http://david.dantoine.org>

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

/* NOTE: Added patch_error_09() with some modifications

Credits:
- Rancid-o
- Zz_SACRO_zZ

*/

#include "utils.h"
#include "language.h"
#include <sys/file.h>
#include "ntfs.h"
#include "iso.h"
#include "osk_input.h"
#include "main.h"
#include "file_manager.h"
#include "ftp/functions.h"

//---
#define USB_MASS_STORAGE_1(n)	(0x10300000000000AULL+(n)) /* For 0-5 */
#define USB_MASS_STORAGE_2(n)	(0x10300000000001FULL+((n)-6)) /* For 6-127 */
#define USB_MASS_STORAGE(n)	(((n) < 6) ? USB_MASS_STORAGE_1(n) : USB_MASS_STORAGE_2(n))

#define MAX_SECTIONS	((0x10000-sizeof(rawseciso_args))/8)

#define cue_buf  plugin_args

#define MAX_PATH_LEN 0x420

enum DiscEmu
{
	EMU_OFF = 0,
	EMU_PS3,
	EMU_PS2_DVD,
	EMU_PS2_CD,
	EMU_PSX,
	EMU_BD,
	EMU_DVD,
	EMU_MAX,
};

typedef struct
{
	uint64_t device;
	uint32_t emu_mode;
	uint32_t num_sections;
	uint32_t num_tracks;
} __attribute__((packed)) rawseciso_args;

typedef struct
{
	uint32_t lba;
	int is_audio;
} TrackDef;

typedef struct _ScsiTrackDescriptor
{
	uint8_t reserved;
	uint8_t adr_control;
	uint8_t track_number;
	uint8_t reserved2;
	uint32_t track_start_addr;
} __attribute__((packed)) ScsiTrackDescriptor;

int emu_mode;
u8 cue=0;
TrackDef tracks[100];
int i, parts;
unsigned int num_tracks;

ScsiTrackDescriptor *scsi_tracks;
uint32_t sections[MAX_SECTIONS], sections_size[MAX_SECTIONS];

rawseciso_args *p_args;

char ntfs_path[MAX_PATH_LEN];

int cobra_parse_cue(void *cue, uint32_t size, TrackDef *tracks, unsigned int max_tracks, unsigned int *num_tracks, char *filename, unsigned int fn_size);
//--


typedef s32 Lv2FsMode;
typedef s32 Lv2FsFile;

extern bool is_mamba_v2;

extern int filter_by_letter;

extern int num_box;
extern int show_custom_icons;
extern char * language[];
extern char self_path[MAXPATHLEN];
extern char video_path[MAXPATHLEN];
extern int game_list_category;
extern int mode_homebrew;

extern char ps2classic_path[MAXPATHLEN];

extern char audio_extensions[300];
extern char video_extensions[300];
extern char browser_extensions[100];
extern char custom_homebrews[400];

extern int retro_mode;

extern int roms_count;
extern int max_roms;

// retroArch cores 1.0
extern char retro_root_path[ROMS_MAXPATHLEN];
extern char retro_snes_path[ROMS_MAXPATHLEN];
extern char retro_gba_path[ROMS_MAXPATHLEN];
extern char retro_gen_path[ROMS_MAXPATHLEN];
extern char retro_nes_path[ROMS_MAXPATHLEN];
extern char retro_mame_path[ROMS_MAXPATHLEN];
extern char retro_fba_path[ROMS_MAXPATHLEN];
extern char retro_doom_path[ROMS_MAXPATHLEN];
extern char retro_quake_path[ROMS_MAXPATHLEN];
extern char retro_pce_path[ROMS_MAXPATHLEN];
extern char retro_gb_path[ROMS_MAXPATHLEN];
extern char retro_gbc_path[ROMS_MAXPATHLEN];
extern char retro_atari_path[ROMS_MAXPATHLEN];
extern char retro_vb_path[ROMS_MAXPATHLEN];
extern char retro_nxe_path[ROMS_MAXPATHLEN];
extern char retro_wswan_path[ROMS_MAXPATHLEN];

// retroArch cores 1.2
extern char retro_a7800_path[ROMS_MAXPATHLEN];
extern char retro_lynx_path[ROMS_MAXPATHLEN];
extern char retro_gw_path[ROMS_MAXPATHLEN];
extern char retro_vectrex_path[ROMS_MAXPATHLEN];
extern char retro_2048_path[ROMS_MAXPATHLEN];

//void UTF8_to_Ansi(char *utf8, char *ansi, int len); // from osk_input
void UTF32_to_UTF8(u32 *stw, u8 *stb);

int copy_async(char *path1, char *path2, u64 size, char *progress_string1, char *progress_string2);     // pkg_install.c
int copy_async_gbl(char *path1, char *path2, u64 size, char *progress_string1, char *progress_string2); // updates.c (it can copy to ntfs devices)

msgType mdialogyesno = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO  | MSG_DIALOG_DEFAULT_CURSOR_NO;
msgType mdialogyesno2 = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DISABLE_CANCEL_ON;
msgType mdialogyesno3 = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO;

msgType mdialogok = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK;

msgType mdialog = MSG_DIALOG_NORMAL | MSG_DIALOG_DISABLE_CANCEL_ON;

volatile int dialog_action = 0;

void my_dialog(msgButton button, void *userdata)
{
    switch(button)
    {
        case MSG_DIALOG_BTN_YES:
            dialog_action = 1;
            break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
        case MSG_DIALOG_BTN_NONE:
            dialog_action = 2;
            break;
        default:
            break;
    }
}

void my_dialog2(msgButton button, void *userdata)
{
    switch(button)
    {
        case MSG_DIALOG_BTN_OK:
        case MSG_DIALOG_BTN_ESCAPE:
        case MSG_DIALOG_BTN_NONE:
            dialog_action = 1;
            break;
        default:
            break;
    }
}

static volatile int progress_action2 = 0;

static float bar1_countparts = 0.0f, bar2_countparts = 0.0f;

char extension[10];
char progress_bar_title[256];

static msgType mdialogprogress = MSG_DIALOG_SINGLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON | MSG_DIALOG_DISABLE_CANCEL_ON | MSG_DIALOG_BKG_INVISIBLE;
static msgType mdialogprogress2 = MSG_DIALOG_DOUBLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;

static void progress_callback(msgButton button, void *userdata)
{
    switch(button)
    {
        case MSG_DIALOG_BTN_OK:
            progress_action2 = 1;
            break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
            progress_action2 = 2;
            break;
        case MSG_DIALOG_BTN_NONE:
            progress_action2 = -1;
            break;
        default:
            break;
    }
}

extern bool scan_canceled;

bool ps3pad_poll()
{
    pad_last_time = 0;
    ps3pad_read();

    if((old_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE)) || (new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))) {scan_canceled = true; return true;}

    return false;
}

void wait_dialog()
{
    while(!dialog_action)
    {
        sysUtilCheckCallback();
        tiny3d_Flip();
    }

    msgDialogAbort();
    usleep(100000);
}

void DrawDialogOKTimer(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogok, str, my_dialog2, (void*) 0x0000aaab, NULL );
    msgDialogClose(milliseconds);

    wait_dialog();
}


void DrawDialogOK(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogok, str, my_dialog2, (void*) 0x0000aaab, NULL );

    wait_dialog();
}


void DrawDialogTimer(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialog, str, my_dialog2, (void*) 0x0000aaab, NULL );
    msgDialogClose(milliseconds);

    wait_dialog();
}

int DrawDialogYesNoTimer(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno, str, my_dialog, (void*)  0x0000aaaa, NULL );
    msgDialogClose(milliseconds);

    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNo(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno, str, my_dialog, (void*)  0x0000aaaa, NULL );

    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNoDefaultYes(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno3, str, my_dialog, (void*)  0x0000aaaa, NULL );

    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNoTimer2(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno2, str, my_dialog, (void*)  0x0000aaaa, NULL );
    msgDialogClose(milliseconds);

    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNo2(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno2, str, my_dialog, (void*)  0x0000aaaa, NULL );

    wait_dialog();

    return dialog_action;
}

char * LoadFile(char *path, int *file_size)
{
    *file_size = (int)get_filesize(path);

    if(!is_ntfs_path(path)) sysLv2FsChmod(path, FS_S_IFMT | 0777);

    if(*file_size==0) return NULL;

    char *mem = NULL;
    mem = malloc(*file_size); if(!mem) return NULL;

    int fd = ps3ntfs_open(path, O_RDONLY, 0777);

    if(fd >= 0)
    {
        ps3ntfs_read(fd, mem, *file_size);
        ps3ntfs_close(fd);
    }

    return mem;
}

int SaveFile(char *path, char *mem, int file_size)
{
    unlink_secure(path);

    int fd = ps3ntfs_open(path, O_WRONLY | O_CREAT | O_TRUNC, 0777);

    if(fd >= 0)
    {
        if(ps3ntfs_write(fd, (void *) mem, file_size)!=file_size)
        {
            ps3ntfs_close(fd);
            return FAILED;
        }
        ps3ntfs_close(fd);
    }

    if(!is_ntfs_path(path)) sysLv2FsChmod(path, FS_S_IFMT | 0777);

    return SUCCESS;
}

int ExtractFileFromISO(char *iso_file, char *file, char *outfile)
{
    if(iso_file[0]=='.' || file_exists(iso_file)==false) return FAILED;

    int fd = ps3ntfs_open(iso_file, O_RDONLY, 0);
    if(fd >= 0)
    {
        u32 flba;
        u64 size;
        char *mem = NULL;

        int re = get_iso_file_pos(fd, file, &flba, &size);

        if(!re && (mem = malloc(size)) != NULL)
        {
            re = ps3ntfs_read(fd, (void *) mem, size);
            ps3ntfs_close(fd);
            if(re == size)
              SaveFile(outfile, mem, size);

            free(mem);

            return (re == size) ? SUCCESS : FAILED;
        }
        else
            ps3ntfs_close(fd);
    }

    return FAILED;
}

u8 game_category[3] = "??";

bool is_ntfs_path(char *path)
{
    return (!strncmp(path, "/ntfs", 5) || !strncmp(path, "/ext", 4));
}


bool is_video(char *ext)
{
    sprintf(extension, "%s ", ext);

    return (strcasestr(video_extensions, extension) != NULL);
}

bool is_audio(char *ext)
{
    sprintf(extension, "%s ", ext);

    return (strcasestr(audio_extensions, extension) != NULL);
}

bool is_audiovideo(char *ext)
{
    return (is_video(ext) || is_audio(ext));
}

bool is_browser_file(char *ext)
{
    sprintf(extension, "%s ", ext);

    return (strcasestr(browser_extensions, extension) != NULL);
}

u64 get_filesize(char *path)
{
    bool is_ntfs = is_ntfs_path(path);

    if(is_ntfs)
    {
        struct stat st;
        if (ps3ntfs_stat(path, &st) < 0) return 0ULL;
        return st.st_size;
    }
    else
    {
        sysFSStat stat;
        if (sysLv2FsStat(path, &stat) < 0) return 0;
        return stat.st_size;
    }
}

bool isDir( char* path )
{
    if(is_ntfs_path(path))
    {
        struct stat st;
        return ps3ntfs_stat(path, &st) >= SUCCESS && (st.st_mode & FS_S_IFDIR);
    }

    sysFSStat stat;
    return sysLv2FsStat(path, &stat) == SUCCESS && (stat.st_mode & FS_S_IFDIR);;
}

bool file_exists( char* path )
{
    if(is_ntfs_path(path))
    {
        struct stat st;
        return ps3ntfs_stat(path, &st) >= SUCCESS;
    }

    sysFSStat stat;
    return sysLv2FsStat(path, &stat) == SUCCESS;
}

char * get_extension(char *path)
{
    int n = strlen(path);
    int m = n;

    while(m > 1 && path[m] != '.' && path[m] != '/') m--;

    if(!strcmp(&path[m], ".0"))
        while(m > 1 && path[m] != '.' && path[m] != '/') m--;

    if(path[m] == '.') return &path[m];

    return &path[n];
}

char *str_replace(char *orig, char *rep, char *with)
{
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig) return NULL;

    if (!rep)
    {
        result = malloc(strlen(orig) + 1);
        strcpy(result, orig);
        return result;
    }

    len_rep = strlen(rep);

    if (!with) with = "";

    len_with = strlen(with);

    ins = orig;
    for (count = 0; (tmp = strstr(ins, rep)); ++count)
    {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result) return NULL;

    while (count--)
    {
        ins = strstr(orig, rep);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

char * get_filename(char *path)
{
    int n = strlen(path);
    int m = n;

    while(m > 0 && path[m] != '/') m--;

    if(path[m] == '/') m++;

    return &path[m];
}

int strcmpext(char *path, char *ext)
{
    int path_len = strlen(path);
    int ext_len = strlen(ext);

    if(ext_len >= path_len) return FAILED;

    return strncasecmp(path + path_len - ext_len, ext, ext_len);
}

int fix_PS3_EXTRA_attribute(char *path)
{
    if(is_ntfs_path(path) || strlen(path) <= 4) return FAILED;

    char filepath[MAXPATHLEN];

    sprintf(filepath, "%s/PS3_EXTRA", path);
    if(!file_exists(filepath))  return SUCCESS;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    unsigned pos, str, len = 0;
    unsigned char *mem = NULL;

    sprintf(filepath, "%s/PS3_GAME/PARAM.SFO", path);
    if(!is_ntfs_path(filepath)) sysLv2FsChmod(filepath, FS_S_IFMT | 0777);

    if(!sysLv2FsOpen(filepath, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes) != 0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem[8] + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0;

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strncmp((char *) &mem[str], "ATTRIBUTE", 10))
            {
                if(!(mem[pos + 2] & 0x2))
                {
                    mem[pos + 2] |= 0x2; //Turn on PS3_EXTRA Flag

                    SaveFile(filepath, (char *) mem, len);
                }

                free(mem);
                return SUCCESS;
            }
            while(mem[str] && str < len) str++; str++;
            pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
            indx += 16;
        }

        if(mem) free(mem);
    }
    return FAILED;
}

int get_field_param_sfo(char *file, char *fieldname, char *value, int field_len)
{
    if(is_ntfs_path(file) || strlen(file) <= 4) return FAILED;
    if(strncmp(file + strlen(file) - 4, ".SFO", 4)) return FAILED;
    if(!file_exists(file)) return FAILED;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    unsigned pos, str, len = 0;
    unsigned char *mem = NULL;

    if(!sysLv2FsOpen(file, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes) != 0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem[8] + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0, fieldname_len;

        fieldname_len = strlen(fieldname);

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strncmp((char *) &mem[str], fieldname, fieldname_len + 1))
            {
                memcpy(value, (char *) &mem[pos], field_len);
                value[field_len] = 0;

                free(mem);
                return SUCCESS;
            }
            while(mem[str] && str < len) str++; str++;
            pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
            indx += 16;
        }


        if(strcmp(fieldname, "APP_VER") == SUCCESS)
        {
            str = (mem[8] + (mem[9]<<8));
            pos = (mem[0xc] + (mem[0xd]<<8));

            indx = 0;

            while(str < len)
            {
                if(mem[str] == 0) break;

                if(!strncmp((char *) &mem[str], "VERSION", 8))
                {
                    memcpy(value, (char *) &mem[pos], 5);
                    value[field_len] = 0;

                    free(mem);
                    return SUCCESS;
                }
                while(mem[str] && str < len) str++; str++;
                pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
                indx += 16;
            }
        }

        if(mem) free(mem);
    }

    return FAILED;
}

int edit_title_param_sfo(char * file)
{
    int ret = FAILED;

    bool more = false;

    char sfo[MAX_PATH_LEN]; // sfo path + file name

    char title_name[64];

    char title_id[10] = "         ";
    char ps3_sys_ver[8] = "00.0000";

    char sub_title[128];
    char savedata_directory[64];

    if(!strncmp(file, "/bdvd", 5) || is_ntfs_path(file) || strlen(file) <= 4)
    {
        DrawDialogOKTimer("PARAM.SFO cannot be edited on this device", 3000.0f);
        return FAILED;
    }
    if(strncmp(file + strlen(file) - 4, ".SFO", 4)) return FAILED;
    if(!file_exists(file)) return FAILED;

    strcpy(sfo, file);

    ret = parse_param_sfo(sfo, title_name);

    if(Get_OSK_String("New Title", title_name, 63) == SUCCESS)
    {
        ps3pad_poll();
        if(old_pad & (BUTTON_SELECT | BUTTON_L1 | BUTTON_R1 | BUTTON_L2 | BUTTON_R2))
        {
            if(strcmp((const char *) game_category, "SD") == SUCCESS)
            {
                ret = get_field_param_sfo(sfo, "SUB_TITLE", sub_title, 127);
                if(Get_OSK_String("Subtitle", sub_title, 127) != SUCCESS) return FAILED;

                ret = get_field_param_sfo(sfo, "SAVEDATA_DIRECTORY", savedata_directory, 63);
                if(Get_OSK_String("Save Data Directory", savedata_directory, 63) != SUCCESS) return FAILED;
            }
            else
            {
                ret = get_field_param_sfo(sfo, "TITLE_ID", title_id, 9);
                if(Get_OSK_String("Title ID", title_id, 9) != SUCCESS) return FAILED;

                ret = get_field_param_sfo(sfo, "PS3_SYSTEM_VER", ps3_sys_ver, 7);
                if(Get_OSK_String("PS3 System Version", ps3_sys_ver, 7) != SUCCESS) return FAILED;
            }

            more = true;
        }

        Lv2FsFile fd;
        u64 bytes;
        u64 position = 0LL;

        if(!sysLv2FsOpen(sfo, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
        {
            unsigned len, pos, str;
            unsigned char *mem = NULL;

            sysLv2FsLSeek64(fd, 0, 2, &position);
            len = (u32) position;

            if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

            mem = (unsigned char *) malloc(len + 16);
            if(!mem) {sysLv2FsClose(fd); return -2;}

            memset(mem, 0, len + 16);

            sysLv2FsLSeek64(fd, 0, 0, &position);

            if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes = 0LL;

            len = (u32) bytes;

            sysLv2FsClose(fd);

            str = (mem[8] + (mem[9]<<8));
            pos = (mem[0xc] + (mem[0xd]<<8));

            int indx = 0;
            int ct = 0;

            while(str < len)
            {
                if(mem[str] == 0) break;

                if(!strncmp((char *) &mem[str], "TITLE", 6))
                {
                    title_name[63] = 0;
                    for(int i = 0; title_name[i] && (i < 64); i++) mem[pos + i] = title_name[i];
                    ct++;
                }
                else if(more)
                {
                    if(!strncmp((char *) &mem[str], "TITLE_ID", 8))
                    {
                        title_id[9] = 0;
                        for(int i = 0; title_id[i] && (i < 10); i++) mem[pos + i] = title_id[i];
                        ct++;
                    }
                    else if(!strncmp((char *) &mem[str], "PS3_SYSTEM_VER", 14))
                    {
                        ps3_sys_ver[7] = 0;
                        for(int i = 0; ps3_sys_ver[i] && (i < 7); i++) mem[pos + i] = ps3_sys_ver[i];
                        ct++;
                    }
                    else if(!strncmp((char *) &mem[str], "SUB_TITLE", 9))
                    {
                        sub_title[127] = 0;
                        for(int i = 0; sub_title[i] && (i < 128); i++) mem[pos + i] = sub_title[i];
                        ct++;
                    }
                    else if(!strncmp((char *) &mem[str], "SAVEDATA_DIRECTORY", 18))
                    {
                        savedata_directory[63] = 0;
                        for(int i = 0; savedata_directory[i] && (i < 64); i++) mem[pos + i] = savedata_directory[i];
                        ct++;
                    }
                }

                if(ct >= (more ? 3 : 1)) break;

                while(mem[str] && str < len) str++; str++;
                pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
                indx += 16;
            }

            ret = FAILED;

            if(mem)
            {
                ret = SaveFile(sfo, (char *) mem, len);
                free(mem);
            }

            return ret;
        }
        else
            DrawDialogOKTimer("Error opening PARAM.SFO", 3000.0f);
    }

    return FAILED;
}

int parse_param_sfo(char * file, char *title_name)
{
    if(is_ntfs_path(file) || strlen(file) <= 4) return FAILED;
    if(strncmp(file + strlen(file) - 4, ".SFO", 4)) return FAILED;
    if(!file_exists(file)) return FAILED;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    if(!sysLv2FsOpen(file, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        unsigned len, pos, str;
        unsigned char *mem = NULL;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem[8] + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0;
        int ct = 0;

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strncmp((char *) &mem[str], "TITLE", 6))
            {
                strncpy(title_name, (char *) &mem[pos], 63);
                title_name[63] = 0;
                ct++;
            }
            else if(!strncmp((char *) &mem[str], "CATEGORY", 8))
            {
                memcpy((char *) game_category, (char *) &mem[pos], 2);
                game_category[2] = 0;
                ct++;
            }

            if(ct >= 2)
            {
                free(mem);
                return SUCCESS;
            }

            while(mem[str] && str < len) str++; str++;
            pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
            indx += 16;
        }

        if(mem) free(mem);
    }

    return FAILED;
}

int mem_parse_param_sfo(u8 *mem, u32 len, char *field, char *value)
{
    u32 pos, str;

    if(mem[0] !=0 || mem[1] != 0x50 || mem[2] != 0x53 || mem[3] != 0x46) return -1; // unknown file!

    str = (mem[8] + (mem[9]<<8));
    pos = (mem[0xc] + (mem[0xd]<<8));

    int indx = 0;

    while(str < len)
    {
        if(mem[str] == 0) break;

        if(!strcmp((char *) &mem[str], field))
        {
            strncpy(value, (char *) &mem[pos], 63);
            if(!strncmp(field, "TITLE_ID", 8) && value[4] != 0x2D)
            {
                sprintf(value, "%c%c%c%c-%c%c%c%c%c", value[0], value[1], value[2], value[3],
                                                      value[4], value[5], value[6], value[7], value[8]);
            }
            value[63] = 0;
            return SUCCESS;
        }

        while(mem[str] && str < len) str++; str++;
        pos  += (mem[0x1c+indx] + (mem[0x1d+indx]<<8));
        indx += 16;
    }

    return FAILED;

}

int parse_iso_titleid(char * path_iso, char * title_id)
{
    if(get_filesize(path_iso) < 0x9320LL) return -2;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    unsigned char *mem = NULL;

    int n, flag;

    flag = FAILED;

    unsigned pos, len = (u32) 0x6000;

    bool is_ntfs = is_ntfs_path(path_iso);

    if(is_ntfs)
    {
        int fd = ps3ntfs_open(path_iso, O_RDONLY, 0777);
        if(fd >= SUCCESS)
        {
            if(ps3ntfs_seek64(fd, 0x9320LL, SEEK_SET) != 0x9320LL) {ps3ntfs_close(fd); return -2;}

            mem = (unsigned char *) malloc(len + 16);
            if(!mem) {ps3ntfs_close(fd); return -2;}
            memset(mem, 0, len + 16);

            bytes = ps3ntfs_read(fd, (void *) mem, len);

            ps3ntfs_close(fd);
        }
    }
    else
    {
        sysLv2FsChmod(path_iso, FS_S_IFMT | 0777);
        n = sysLv2FsOpen(path_iso, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0);

        if(!n)
        {
            sysLv2FsLSeek64(fd, 0x9320LL, 0, &position);
            if(position != 0x9320LL) {sysLv2FsClose(fd); return -2;}

            mem = (unsigned char *) malloc(len + 16);
            if(!mem) {sysLv2FsClose(fd); return -2;}
            memset(mem, 0, len + 16);

            if(sysLv2FsRead(fd, mem, len, &bytes) != 0) bytes = 0LL;

            len = (u32) bytes;

            sysLv2FsClose(fd);
        }
    }


    pos = 0;
    if(bytes > 11) len = (u32) bytes - 11; else len = 0;

    while(pos < len)
    {
        if(mem[pos] == 'S' && mem[pos + 8] == '.')
        {
            if(!strncmp((char *) &mem[pos], "SLUS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCUS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLES_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCES_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLPM_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLPS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCPM_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCPS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SIPS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCPS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLUD_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCUD_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLED_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCED_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
        }
        else if(mem[pos] == 'P' && mem[pos + 8] == '.')
        {
            if(!strncmp((char *) &mem[pos], "PAPX_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "PBPX_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "PCPX_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
        }
        else if(mem[pos] == 'N' && mem[pos + 1] == 'P')
        {
            if(!strncmp((char *) &mem[pos], "NPUZ", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPEZ", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPUH", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPEH", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPJH", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPUG", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPEG", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPJG", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
        }
        else if(mem[pos] == 'U')
        {
            if(!strncmp((char *) &mem[pos], "ULUS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "ULES", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "ULJS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "ULAS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCJM", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCUS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCES", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCJS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCAS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCES", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCKS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
        }

        pos++;
    }

    if(mem) free(mem);
    return flag;
}

int parse_param_sfo_id(char * file, char *title_id)
{
    char titleid[16];

    int ret = get_field_param_sfo(file, "TITLE_ID", titleid, 9);

    sprintf(title_id, "%c%c%c%c-%c%c%c%c%c", titleid[0], titleid[1], titleid[2], titleid[3],
                                             titleid[4], titleid[5], titleid[6], titleid[7], titleid[8]);

    return ret;
}

int parse_param_sfo_appver(char *file, char *app_ver)
{
    return get_field_param_sfo(file, "APP_VER", app_ver, 5);
}

int parse_ps3_disc(char *path, char * id)
{
    if(is_ntfs_path(path) || strlen(path) <= 4) return FAILED;
    if(strncmp(path + strlen(path) - 4, ".SFB", 4)) return FAILED;
    if(!file_exists(path)) return FAILED;

    int n;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    strncpy(id, "UNKNOWN", 63);

    if(!sysLv2FsOpen(path, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        unsigned len;
        unsigned char *mem = NULL;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes) != 0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        for(n = 0x20; n < 0x200; n += 0x20)
        {
            if(!strcmp((char *) &mem[n], "TITLE_ID"))
            {
                n = (mem[n + 0x12]<<8) | mem[n + 0x13];
                memcpy(id, &mem[n], 16);

                return SUCCESS;
            }
        }
    }

    return FAILED;
}

extern int firmware;


int patch_exe_error_09(char *path_exe)
{
    if(is_ntfs_path(path_exe)) return 0;

    u16 fw_421 = 42100;
    u16 fw_485 = 48500;
    int offset_fw;
    s32 ret;
    u64 bytesread = 0;
    u64 written = 0;
    u64 pos = 0;
    u16 ver = 0;
    int file = -1;
    int flag = 0; //not patched

    sysLv2FsChmod(path_exe, FS_S_IFMT | 0777);

    if(firmware >= 0x485C) return SUCCESS;

    // open self/sprx and changes the fw version
    ret = sysLv2FsOpen( path_exe, SYS_O_RDWR, &file, 0, NULL, 0 );
    if(ret == SUCCESS)
    {
        // set to offset position
        ret = sysLv2FsLSeek64( file, 0xC, 0, &pos );
        if(ret == SUCCESS && pos == 0xCULL)
        {
            // read offset in file
            ret = sysLv2FsRead( file, &offset_fw, 0x4, &bytesread );

            if(ret == SUCCESS && bytesread == 4ULL && offset_fw > 0xF)
            {
                u8 retried; retried = 0; offset_fw -= 0x78;

                retry_offset_exe:

                if(offset_fw < 0x90 || offset_fw > 0x800) offset_fw = strstr(path_exe, ".sprx") ? 0x258 : 0x428;
                offset_fw += 6;

                ret = sysLv2FsLSeek64( file, (u64) offset_fw, 0, &pos );

                if(ret == SUCCESS && pos == (u64) offset_fw)
                {
                    ret = sysLv2FsRead( file, &ver, 0x2, &bytesread ); //self/sprx min fw version

                    if(retried == 0 && (ver % 100) > 0) {offset_fw = (offset_fw==0x258) ? 0x278 : 0; retried = 1; goto retry_offset_exe;}

                    if(ret == SUCCESS && bytesread == 0x2ULL && (ver >= 34000 && ver <= fw_485))
                    {
                        ret = sysLv2FsLSeek64( file, (u64) offset_fw, 0, &pos );
                        u16 cur_firm = ((firmware>>12) & 0xF) * 10000 + ((firmware>>8) & 0xF) * 1000 + ((firmware>>4) & 0xF) * 100;

                        if(ret == SUCCESS && ver > cur_firm)
                        {
                            if(ver > fw_421 && (firmware >= 0x421C && firmware < 0x485C))
                            {
                                sysLv2FsWrite( file, &cur_firm, 0x2, &written );
                                flag = 1; //patch applied
                            }
                            else
                                flag = -1; //requires a higher firmware
                        }
                    }
                }
            }
        }

        sysLv2FsClose( file );
   }

   return flag;
}


// exported from PS3 Ita Manager method with some modifications (support caps types, error control
// automatic file offset and "intelligent" patch method

static int self_alarm_version = 0;

void patch_error_09( const char *path, int quick_ver_check )
{

    if(quick_ver_check)
    {
        char ps3_sys_ver[8] = "00.0000";
        char sfo[MAX_PATH_LEN];
        sprintf(sfo, "%s/PS3_GAME/PARAM.SFO", path);
        get_field_param_sfo(sfo, "PS3_SYSTEM_VER", ps3_sys_ver, 7);

        int cur_firm = ((firmware>>12) & 0xF) * 10000 + ( (firmware>>8) & 0xF) * 1000 + ( (firmware>>4) & 0xF) * 100;
        int gam_firm = (ps3_sys_ver[1] - '0') * 10000 + (ps3_sys_ver[3] - '0') * 1000 + (ps3_sys_ver[4] - '0') * 100;
        if(cur_firm >= gam_firm) return;
    }


    int d = -1;
    s32 ret = 1;

    /* Open the directory specified by "path". */
    ret = sysLv2FsOpenDir( path, &d );

    /* Check it was opened. */
    if( d == -1 ) return;

    int ext;
    char f[MAX_PATH_LEN];

    while(true)
    {
        sysFSDirent entry;
        u64 read = 0;

        /* "Readdir" gets subsequent entries from "d". */
        ret = sysLv2FsReadDir( d, &entry, &read );
        if ( read == 0 || ret != SUCCESS )
        {
            /* There are no more entries in this directory, so break
               out of the while loop. */
            break;
        }

        //DIRECTORY
        if(entry.d_type & DT_DIR)
        {
            if(strncmp( entry.d_name, ".", 1) != SUCCESS && strncmp( entry.d_name, "..", 2) != SUCCESS &&
               strstr("GAMES|GAMEZ", entry.d_name) == NULL)
            {
                sprintf( f, "%s/%s", path, entry.d_name);

             /* Recursively call "list_dir" with the new path. */
                patch_error_09(f, 0);
            }
        }
        else
        {
            ext = entry.d_namlen - 4;

            if(ext > 1 )
            {
                // SELF/SPRX/EBOOT.BIN
                if( strcasestr("sprx|self", entry.d_name + ext) != NULL ||
                   (strcmp(entry.d_name, "EBOOT.BIN" ) == SUCCESS))
                {
                    sprintf( f, "%s/%s", path, entry.d_name);

                    int r = patch_exe_error_09(f);
                    if(r == -1)
                    {
                        self_alarm_version = 1;
                        DPrintf(">> %s requires a higher CFW!!!\n", entry.d_name);
                    }
                    else if(r == 1)
                        DPrintf(">> %s [fixed]\n", entry.d_name);
                    else
                        DPrintf(">> %s\n", entry.d_name);
                }
            }
        }
    }

    /* After going through all the entries, close the directory. */
    sysLv2FsCloseDir( d );
}

int sys_shutdown()
{
    unlink_secure("/dev_hdd0/tmp/turnoff");

    lv2syscall4(SC_SYS_POWER,SYS_SHUTDOWN,0,0,0);
    return_to_user_prog(int);
}

int sys_reboot()
{
    unlink_secure("/dev_hdd0/tmp/turnoff");

    //lv2syscall4(SC_SYS_POWER,SYS_HARD_REBOOT,0,0,0);
    lv2syscall3(SC_SYS_POWER,SYS_REBOOT,0,0);
    return_to_user_prog(int);
}

int sys_soft_reboot()
{
    unlink_secure("/dev_hdd0/tmp/turnoff");

    lv2syscall4(SC_SYS_POWER,SYS_SOFT_REBOOT,0,0,0);
    return_to_user_prog(int);
}

int unlink_secure(void *path)
{
    bool is_ntfs = is_ntfs_path(path);

    if(is_ntfs)
    {
        return ps3ntfs_unlink(path);
    }

    sysFSStat s;

    if(sysLv2FsStat(path, &s) >= 0)
    {
        sysLv2FsChmod(path, FS_S_IFMT | 0777);
        return sysLv2FsUnlink(path);
    }

    return FAILED;
}

int rename_secure(void *path1, void *path2)
{
    bool is_ntfs = is_ntfs_path(path1);

    if(is_ntfs)
    {
        return ps3ntfs_rename(path1, path2);
    }

    sysFSStat s;

    if(sysLv2FsStat(path1, &s) >= 0)
    {
        return sysLv2FsRename(path1, path2);
    }

    return FAILED;
}

int mkdir_secure(void *path)
{
    int ret = FAILED;

    bool is_ntfs = is_ntfs_path(path);

    if(is_ntfs)
    {
        ret = ps3ntfs_mkdir(path, 0777);
    }
    else
    {
        DIR  *dir = opendir(path);
        if(!dir)
        {
            ret = mkdir(path, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
        }
        else
            closedir(dir);
    }

    return ret;
}

int rmdir_secure(void *path)
{
    int ret = FAILED;

    bool is_ntfs = is_ntfs_path(path);

    if(is_ntfs)
    {
        return ps3ntfs_unlink((char*)path);
    }

    DIR  *dir = opendir(path);
    if(dir)
    {
        closedir(dir);

        sysFsChmod(path, FS_S_IFDIR | 0777);
        ret = sysLv2FsRmdir(path);
    }

    return ret;
}

void utf8_truncate(char *utf8, char *utf8_trunc, int len)
{
u8 *ch= (u8 *) utf8;

    *utf8_trunc = 0;

    while(*ch != 0 && len > 0)
    {
        // 3, 4 bytes utf-8 code
        if(((*ch & 0xF1) == 0xF0 || (*ch & 0xF0) == 0xe0) && (*(ch+1) & 0xc0) == 0x80)
        {
            //*utf8_trunc++=' '; // ignore
            memcpy(utf8_trunc, &ch, 3+1*((*ch & 0xF1) == 0xF0));
            utf8_trunc+= 3+1*((*ch & 0xF1) == 0xF0);
            len--;
            ch += 2 + 1 * ((*ch & 0xF1) == 0xF0);
        }
        else if((*ch & 0xE0) == 0xc0 && (*(ch+1) & 0xc0) == 0x80) // 2 bytes utf-8 code
        {
            memcpy(utf8_trunc, &ch, 2);
            utf8_trunc+=2;
            len--;
            ch++;
        }
        else
        {
            if(*ch<32) *ch=32;
            *utf8_trunc++=*ch;

            len--;
        }

        ch++;
    }

    while(len > 0)
    {
        *utf8_trunc++=0;
        len--;
    }
}
void utf8_to_ansi(char *utf8, char *ansi, int len)
{
    u8 *ch= (u8 *) utf8;
    u8 c;

    *ansi = 0;

    while(*ch != 0 && len > 0)
    {
        // 3, 4 bytes utf-8 code
        if(((*ch & 0xF1) == 0xF0 || (*ch & 0xF0) == 0xe0) && (*(ch+1) & 0xc0) == 0x80)
        {
            *ansi++=' '; // ignore
            len--;
            ch+=2+1*((*ch & 0xF1) == 0xF0);
        }
        else
        // 2 bytes utf-8 code
        if((*ch & 0xE0) == 0xc0 && (*(ch+1) & 0xc0) == 0x80)
        {
            c = (((*ch & 3)<<6) | (*(ch+1) & 63));

            if(c >= 0xC0 && c <= 0xC5) c='A';
            else if(c == 0xc7) c='C';
            else if(c >= 0xc8 && c <= 0xcb) c='E';
            else if(c >= 0xcc && c <= 0xcf) c='I';
            else if(c == 0xd1) c='N';
            else if(c >= 0xd2 && c <= 0xd6) c='O';
            else if(c >= 0xd9 && c <= 0xdc) c='U';
            else if(c == 0xdd) c='Y';
            else if(c >= 0xe0 && c <= 0xe5) c='a';
            else if(c == 0xe7) c='c';
            else if(c >= 0xe8 && c <= 0xeb) c='e';
            else if(c >= 0xec && c <= 0xef) c='i';
            else if(c == 0xf1) c='n';
            else if(c >= 0xf2 && c <= 0xf6) c='o';
            else if(c >= 0xf9 && c <= 0xfc) c='u';
            else if(c == 0xfd || c == 0xff) c='y';
            else if(c>127) c=*(++ch+1); //' ';

            *ansi++=c;
            len--;
            ch++;
        }
        else
        {
            if(*ch<32) *ch=32;
            *ansi++=*ch;

            len--;
        }

        ch++;
    }

    while(len > 0)
    {
        *ansi++=0;
        len--;
    }
}

#include "utils_gamelist.h"

/*******************************************************************************************************************************************************/
/* CONSOLE DEBUG                                                                                                                                       */
/*******************************************************************************************************************************************************/

#define CONSOLE_WIDTH       (100)
#define CONSOLE_HEIGHT      (27)

static char dbg_str1[128];
static char dbg_str2[128];

static u32 dbg_data[128 * (CONSOLE_HEIGHT + 1)];
static char dbg_string[1024];

int con_x = 0, con_y =0;

void DCls()
{
    con_x = 0; con_y =0;
    dbg_str1[0] = dbg_str2[0] = 0;
    memset((void *) dbg_data, 0, 128 * CONSOLE_HEIGHT * 4);
}

static char buff[4096];

void DbgHeader(char *str)
{
    strncpy(dbg_str1, str, 128);
    dbg_str1[127] = 0;
}

void DbgMess(char *str)
{
    strncpy(dbg_str2, str, 128);
    dbg_str2[127] = 0;
}


void DbgDraw()
{
    int n;

    cls2();


    SetFontColor(0x0fcf2fff, 0x00000000);
    SetFontAutoCenter(0);
    SetCurrentFont(FONT_TTF/*BUTTON*/);
    SetFontSize(14, 16);


    for(n = 0; n < CONSOLE_HEIGHT; n++)
    {
       UTF32_to_UTF8(&dbg_data[128 * n], (void *) dbg_string);
       DrawString(4, 56 + n * 16, dbg_string);
    }

    SetFontColor(0xffffffff, 0x00000000);
    SetCurrentFont(FONT_TTF);

    SetFontSize(14, 32);

    SetFontAutoCenter(1);
    DrawString(0, 16, dbg_str1);


    DrawString(0, 480, dbg_str2);

    SetFontAutoCenter(0);

    SetCurrentFont(FONT_BUTTON);
}

void DPrintf(char *format, ...)
{
    char *str = (char *) buff;
    va_list opt;

    va_start(opt, format);
    vsprintf( (void *) buff, format, opt);
    va_end(opt);

    while(*str)
    {
        u32 ttf_char;
        int n, m;

        if(*str & 128)
        {
            m = 1;

            if((*str & 0xf8) == 0xf0)
            {
                // 4 bytes
                ttf_char = (u32) (*(str++) & 3);
                m = 3;
            }
            else if((*str & 0xE0) == 0xE0)
            {
                // 3 bytes
                ttf_char = (u32) (*(str++) & 0xf);
                m = 2;
            }
            else if((*str & 0xE0) == 0xC0)
            {
                // 2 bytes
                ttf_char = (u32) (*(str++) & 0x1f);
                m = 1;
            }
            else {str++; continue;} // error!

             for(n = 0; n < m; n++)
             {
                if(!*str) break; // error!

                if((*str & 0xc0) != 0x80) break; // error!
                ttf_char = (ttf_char <<6) |((u32) (*(str++) & 63));
             }

            if((n != m) && !*str) break;

        } else ttf_char = (u32) *(str++);

        if(ttf_char == '\n')
        {
            con_y++;
            con_x = 0;

            if(con_y >= CONSOLE_HEIGHT)
            {
                con_y = CONSOLE_HEIGHT - 1;
                memcpy((void *) dbg_data, (void *) (dbg_data + 128), 128 * (CONSOLE_HEIGHT -1) * 4);
                dbg_data[128 * (CONSOLE_HEIGHT -1)] = 0;
            }
            else dbg_data[128 * con_y + con_x] = 0;
        }
        else
        {
            if(con_x < CONSOLE_WIDTH)
            {
                dbg_data[128 * con_y + con_x] = ttf_char;
                dbg_data[128 * con_y + con_x + 1] = 0;
                con_x++;
            }
            else
            {
                con_y++;
                con_x = 0;
                if(con_y >= CONSOLE_HEIGHT)
                {
                    con_y = CONSOLE_HEIGHT - 1;
                    memcpy((void *) dbg_data, (void *) (dbg_data + 128), 128 * (CONSOLE_HEIGHT -1) * 4);
                    dbg_data[128 * (CONSOLE_HEIGHT -1)] = 0;
                }

                dbg_data[128 * con_y + con_x] = ttf_char;
                dbg_data[128 * con_y + con_x + 1] = 0;
                con_x++;
            }
        }
    }

    DbgDraw();
    tiny3d_Flip();
}

char *getfilename_part(char *path)
{
    int len = strlen(path) - 1;

    while(len > 0) {if(path[len] == '/') {len++; break;}; len--;}

    return &path[len];
}


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

static s64 count_cache_bytes(char *path)
{
    int n = 0;
    s64 bytes = 0;

    while(n < 100)
    {
        struct stat s;
        sprintf(buff, "%s.666%2.2i", path, n);

        if(stat(buff, &s) < 0) break; //end loop

        bytes += s.st_size;
        n++;
    }

    return bytes; //return total bytes counted

}

static int num_directories = 0, num_files_big = 0, num_files_split = 0, fixed_self_sprx = 0;

static int my_game_test(char *path)
{
    DIR  *dir;
    int seconds = 0, seconds2 = 0;

    if(strstr(path, "/PS3_GAME"))
        DPrintf("%s\n", strstr(path, "/PS3_GAME"));
    else
        DPrintf("%s\n", path);

    char f[MAX_PATH_LEN];
    struct stat s;

    dir = opendir(path);
    if(!dir) return FAILED;

    sysFsChmod(path, FS_S_IFDIR | 0777);

    while(true)
    {
        struct dirent *entry = readdir(dir);

        if(!entry) break;

        if(entry->d_name[0] == '.' && (entry->d_name[1] == 0 || entry->d_name[1] == '.')) continue;

        if((entry->d_type & DT_DIR))
        {
            num_directories++;

            sprintf(f, "%s/%s", path, entry->d_name);
            my_game_test(f);

            if(abort_copy) break;
        }
        else
        {
            s64 size = 0LL;

            sprintf(f, "%s/%s", path, entry->d_name);

            if(stat(f, &s) != SUCCESS) {DPrintf("File error!!!\n -> %s\n\n", f); continue;}
            size = s.st_size;

            int ext = entry->d_namlen - 4;
            if(ext > 1 && ( strcasestr("sprx|self", entry->d_name + ext) != NULL ||
                          ( strcmp(entry->d_name, "EBOOT.BIN" ) == SUCCESS )))
            {
                int r = patch_exe_error_09(f);

                if(r == -1)
                {
                    self_alarm_version = 1;
                    DPrintf(">> %s requires a higher CFW!!!\n", entry->d_name);
                }
                else if(r == 1)
                {
                    fixed_self_sprx++;
                    DPrintf(">> %s [fixed]\n", entry->d_name);
                }
                else
                    DPrintf(">> %s\n", entry->d_name);

            }

            if(entry->d_namlen > 6)
            {
                char *p = f;
                p += strlen(f) - 6; // adjust for .666xx
                if(p[0] ==  '.' && p[1] ==  '6' && p[2] ==  '6' && p[3] ==  '6')
                {
                    DPrintf("%s %lli MB %s\n\n", language[GLUTIL_SPLITFILE], size/0x100000LL, f);
                    num_files_split++;

                    if(copy_split_to_cache && p[4] == '0' && p[5] == '0')
                    {
                        if(nfilecached < MAX_FILECACHED)
                        {
                            sprintf(buff, language[GAMETESTS_FOUNDINSTALL], entry->d_name);
                            if(DrawDialogYesNo(buff) == YES)
                            {
                                sprintf(&filecached[nfilecached][0][0], "%s/%s", path, entry->d_name);
                                sprintf(&filecached[nfilecached][1][0], "%s", entry->d_name);

                                char * a = strstr((char *) &filecached[nfilecached][0][0], ".66600");
                                if(a) a[0] = 0;
                                a = strstr((char *) &filecached[nfilecached][1][0], ".66600");
                                if(a) a[0] = 0;

                                filecached_bytes[nfilecached] = count_cache_bytes(&filecached[nfilecached][0][0]);

                                //prepare next
                                nfilecached++;
                            }
                        }
                    }
                }
                else
                if(entry->d_namlen > 7)
                {
                    char *p = f;
                    p += strlen(f) - 7; // adjust for .666xx
                    if(p[2] ==  '.' && p[3] ==  'p' &&  p[4] ==  'a' && p[5] ==  'r' && p[6] ==  't')
                    {
                        DPrintf("%s %lli MB %s\n\n", language[GLUTIL_SPLITFILE], size / 0x100000LL, f);
                        num_files_split++;

                        if(copy_split_to_cache && p[0] == '.' && p[1] == '1')
                        {   num_files_split++;
                            if(nfilecached < MAX_FILECACHED)
                            {
                                sprintf(buff, language[GAMETESTS_FOUNDINSTALL], entry->d_name);
                                if(DrawDialogYesNo(buff) == YES)
                                {
                                    sprintf(&filecached[nfilecached][0][0], "%s/%s", path, entry->d_name);
                                    sprintf(&filecached[nfilecached][1][0], "%s", entry->d_name);

                                    char * a = strstr((char *) &filecached[nfilecached][0][0], ".1.part");
                                    if(a) a[0] = 0;
                                    a = strstr((char *) &filecached[nfilecached][1][0], ".1.part");
                                    if(a) a[0] = 0;

                                    filecached_bytes[nfilecached]=count_cache_bytes(&filecached[nfilecached][0][0]);

                                    //prepare next
                                    nfilecached++;
                                }
                            }
                        }
                    }
                }
            }

            if(size >= 0x100000000LL)
            {
                DPrintf("%s %lli MB %s\n\n", language[GAMETESTS_BIGFILE], size/0x100000LL, f); num_files_big++;
            }

            //prepare info for user
            seconds = (int) (time(NULL)-time_start);

            file_counter++;

            global_device_bytes += size;

            if(seconds != seconds2)
            {
                sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMETESTS_TESTFILE], file_counter,
                        language[GLUTIL_TIME], seconds/3600, (seconds / 60) % 60, seconds % 60,
                        ((double) global_device_bytes) / GIGABYTES);

                cls2();

                DbgHeader( string1);
                DbgMess(language[GLUTIL_HOLDTRIANGLEAB]);

                DbgDraw();

                tiny3d_Flip();

                ps3pad_poll();

                seconds2 = seconds;
            }

            if(abort_copy) break;
            if(!copy_split_to_cache && (new_pad & BUTTON_CIRCLE_)) abort_copy = 1;

            if(abort_copy) break;
        }

    }

    sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMETESTS_TESTFILE], file_counter,
                    language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60,
                    ((double) global_device_bytes) / GIGABYTES);

    cls2();

    DbgHeader( string1);
    DbgMess(language[GLUTIL_HOLDTRIANGLEAB]);

    DbgDraw();

    tiny3d_Flip();


    closedir(dir);

    return SUCCESS;
}

static int my_game_countsize(char *path)
{
    DIR  *dir;
    dir = opendir(path);
    if(!dir) return FAILED;

    DPrintf("count in %s\n", path);

    char f[MAX_PATH_LEN];
    struct stat s;

    while(true)
    {
        struct dirent *entry = readdir(dir);

        if(!entry) break;

        if(entry->d_name[0] == '.' && (entry->d_name[1] == 0 || entry->d_name[1] == '.')) continue;

        if((entry->d_type & DT_DIR))
        {
            sprintf(f, "%s/%s", path, entry->d_name);

            if(strcmp(entry->d_name, "PS3_UPDATE") != 0) my_game_countsize(f);

            if(abort_copy) break;
        }
        else
        {
            s64 size = 0LL;

            sprintf(f, "%s/%s", path, entry->d_name);

            if(stat(f, &s) != SUCCESS) {DPrintf("File error!!!\n -> %s\n\n", f); continue;}
            size = s.st_size;

            file_counter++;

            copy_total_size += size;

            if((file_counter  & 15) ==  1)
            {
                sprintf(string1,"%s: %i Vol: %1.2f GB\n", language[GAMETESTS_CHECKSIZE], file_counter, ((double) copy_total_size) / GIGABYTES);
                cls2();

                DbgHeader( string1);
                DbgMess(language[GLUTIL_HOLDTRIANGLESK]);

                DbgDraw();

                tiny3d_Flip();
            }

            if(ps3pad_poll())
            {
                abort_copy = 1;
            }

            if(abort_copy) break;
        }
    }

    closedir(dir);

    {
        sprintf(string1,"%s: %i Vol: %1.2f GB\n", language[GAMETESTS_CHECKSIZE], file_counter, ((double) copy_total_size) / GIGABYTES);
        cls2();

        DbgHeader( string1);
        DbgMess(language[GLUTIL_HOLDTRIANGLESK]);

        DbgDraw();

        tiny3d_Flip();
    }

    return SUCCESS;
}

static int my_game_delete(char *path)
{
    DIR  *dir;
    char f[MAX_PATH_LEN];
    int seconds;

    dir = opendir(path);
    if(!dir) return FAILED;
    sysFsChmod(path, FS_S_IFDIR | 0777);

    copy_split_to_cache = 0;

    while(true)
    {
        struct dirent *entry = readdir(dir);

        if(!entry) break;

        if(entry->d_name[0] == '.' && (entry->d_name[1] == 0 || entry->d_name[1] == '.')) continue;

        if((entry->d_type & DT_DIR))
        {
            sprintf(f, "%s/%s", path, entry->d_name);
            my_game_delete(f);

            DPrintf("Deleting <%s>\n\n", path);
            if(sysLv2FsRmdir(f)) {abort_copy = 3; DPrintf("Deleting Error!!!\n -> <%s>\n\n", entry->d_name); break;}

            if(abort_copy) break;

            file_counter--;

            goto display_mess;
        }
        else
        {
            sprintf(f, "%s/%s", path, entry->d_name);

            sysFsChmod(f, FS_S_IFMT | 0777);
            if(sysLv2FsUnlink(f)){abort_copy = 3;DPrintf("Deleting Error!!!\n -> %s\n\n", f); break;}

            DPrintf("%s %s\n\n", language[GAMEDELFL_DELETED], f);

            display_mess:

            seconds = (int) (time(NULL) - time_start);

            sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i\n", language[GAMEDELFL_DELETING], file_counter,
                    language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60);

            file_counter++;

            cls2();

            DbgHeader( string1);
            DbgMess(language[GLUTIL_HOLDTRIANGLEAB]);
            DbgDraw();

            tiny3d_Flip();

            if(abort_copy) break;

            if(ps3pad_poll())  abort_copy = 1;

            if(abort_copy) break;
        }
    }

    closedir(dir);

    return SUCCESS;
}

static int _my_game_copy(char *path, char *path2)
{
    Lv2FsFile  dir;

    filepath_check(path2);

    if (sysFsOpendir(path, &dir)) {DPrintf("Error in sysFsOpendir()\n"); abort_copy = 7; return FAILED;}

    if(!copy_split_to_cache) DPrintf("\n%s: %s\n%s: %s\n\n", language[FASTCPADD_COPYING], path, language[GLUTIL_WTO], path2);

    while(true)
    {
        sysFSDirent entry;
        u64 read;
        read = sizeof(sysFSDirent);

        if(sysFsReaddir(dir, &entry, &read) || !read || abort_copy) break;

        if(entry.d_name[0] == '.' && entry.d_name[1] == 0) continue;
        if(entry.d_name[0] == '.' && entry.d_name[1] == '.' && entry.d_name[2] == 0) continue;

        if((entry.d_type & DT_DIR))
        {
            char *d1 = (char *) malloc(MAX_PATH_LEN);
            char *d2 = (char *) malloc(MAX_PATH_LEN);

            if(!d1 || !d2) {if(d1) free(d1); if(d2) free(d2); sysFsClosedir(dir); DPrintf("malloc() Error!!!\n\n"); abort_copy = 2; return FAILED;}

            sprintf(d1,"%s/%s", path, entry.d_name);
            sprintf(d2,"%s/%s", path2, entry.d_name);

            if(!copy_split_to_cache) mkdir_secure(d2);

            if(strcmp(entry.d_name, "PS3_UPDATE") != 0) _my_game_copy(d1, d2);

            free(d1); free(d2);
        }
        else if(strcmp(entry.d_name, "PS3UPDAT.PUP") == 0) ;
        else
        {
           //if(!copy_split_to_cache) DPrintf("> %s\n", entry.d_name);

           if(fast_copy_add(path, path2, entry.d_name) < 0)
           {
                abort_copy = 666;
                DPrintf("Failed in fast_copy_add()\n");
                sysFsClosedir(dir);
                return FAILED;
           }
        }

        if(abort_copy) break;
    }

/*
    if(fast_num_files >= 1) {
        int ret = fast_copy_process();

        if(ret < 0 || abort_copy) {
            UTF8_to_Ansi(language[FASTCPADD_FAILED], ansi, 1024);
            DPrintf("%s%i\n", ansi, ret);
            abort_copy = 666;
            DPrintf("Failed in fast_copy_add()\n");
            sysFsClosedir(dir);
            return FAILED;
        }
    }
*/
    sysFsClosedir(dir);
    if(abort_copy) return FAILED;

    return SUCCESS;
}

static int my_game_copy(char *path, char *path2)
{

    progress_action2 = 0;

    bar1_countparts = 0.0f;
    bar2_countparts = 0.0f;

    msgDialogOpen2(mdialogprogress2, progress_bar_title, progress_callback, (void *) 0xadef0045, NULL);

    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, " ");
    msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX1, " ");
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX1);

    sysUtilCheckCallback(); tiny3d_Flip();

    global_device_bytes = 0;

    if(fast_copy_async(path, path2, 1) < 0) {abort_copy = 665;  msgDialogAbort(); return FAILED;}

    int ret = _my_game_copy(path, path2);

    int ret2 = fast_copy_process();

    fast_copy_async(path, path2, 0);

    msgDialogAbort();

    if(ret < 0 || ret2 < 0) return FAILED;

    return SUCCESS;
}


static char filename[MAX_PATH_LEN];

void copy_from_selection(int game_sel)
{
    copy_split_to_cache = 0;
    copy_total_size = 0LL;
    file_counter = 0;
    abort_copy = 0;

    DCls();
    my_game_countsize(directories[game_sel].path_name);
    total_fast_files = file_counter;

    sleep(2); DCls();

    if(!(directories[game_sel].flags & D_FLAG_BDVD))
    {
        if(!(directories[game_sel].flags & D_FLAG_HDD0))
        {
            // test for HDD0
            u32 blockSize;
            u64 freeSize;
            u64 free_hdd0;
            sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
            free_hdd0 = ( ((u64)blockSize * freeSize));

            if((copy_total_size + 0x40000000LL) >= (s64) free_hdd0)
            {
                sprintf(filename, language[GAMECHCPY_NOSPACE], free_hdd0, ((double) (copy_total_size + 0x40000000LL - free_hdd0)) / GIGABYTES);
                DrawDialogOK(filename);

                return;
            }
        }
    }

    if(abort_copy) //abort by user or got an error
    {
        if(DrawDialogYesNo(language[GAMECPYSL_GSIZEABCNTASK]) != YES)
        {
            forcedevices = D_FLAG_USB;
            return;
        }
    }

    if(directories[game_sel].flags & D_FLAG_BDVD)  {copy_from_bluray(); return;}

#ifdef PSDEBUG
    int ret;
#endif
    int n;
    int curr_device = 0;
    char name[MAX_PATH_LEN];
    int dest = 0;

    dialog_action = 0;
    abort_copy = 0;

    char * hdd_folder2 = hdd_folder;

    char * path_install = __MKDEF_GAMES_DIR;

    if(directories[game_sel].flags & (PS1_FLAG)) {path_install = "PSXGAMES"; hdd_folder2 = "dev_hdd0";}

    if(directories[game_sel].flags & D_FLAG_HDD0)
    {
        // is hdd0
        for(n = 1; n < 11; n++)
        {
            dialog_action = 0;

            if((fdevices >> n) & 1)
            {
                if(copy_total_size)
                {
                    sprintf(filename, "%s\n\n%s HDD0 %s USB00%c?\nVol: %1.2f GB", directories[game_sel].title,
                            language[GLUTIL_WANTCPYFROM], language[GLUTIL_WTO], 47 + n,
                            ((double) copy_total_size) / GIGABYTES);
                }
                else
                {
                    sprintf(filename, "%s\n\n%s HDD0 %s USB00%c?", directories[game_sel].title, language[GLUTIL_WANTCPYFROM], language[GLUTIL_WTO], 47 + n);
                }

                msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );

                wait_dialog();

                if(dialog_action == 1)  {curr_device = n; break;} // exit
            }
        }


       dest = n;

       if(dialog_action == 1)
       {
            u32 blockSize;
            u64 freeSize;
            u64 free_usb;

            sprintf(progress_bar_title, "HDD0 -> USB00%c", 47 + n);

            sprintf(name, "/dev_usb00%c/", 47 + curr_device);
            sysFsGetFreeSize(name, &blockSize, &freeSize);

            free_usb = ( ((u64)blockSize * freeSize));

            if((copy_total_size + 0x100000LL) >= (s64) free_usb)
            {
                sprintf(filename, "%s\n\n%s%1.2f GB\n\n%s", "Warning: There is not enough space in USB to copy it", "You need ",
                    ((double) (copy_total_size + 0x100000LL - free_usb)) / GIGABYTES,  "Do you want to abort?");

                dialog_action = 0;

                msgDialogOpen2( mdialogyesno2, filename, my_dialog, (void*) 0x0000aaaa, NULL);

                wait_dialog();

                if(dialog_action == 1)  {return;} else dialog_action = 1; // exit
            }

            if(directories[game_sel].flags & (PS1_FLAG))
            {
                sprintf(name, "/dev_usb00%c/PSXGAMES", 47 + curr_device);
                mkdir_secure(name);
                sprintf(name, "/dev_usb00%c/PSXGAMES", 47 + curr_device);
                mkdir_secure(name);

                char * p = strstr(directories[game_sel].path_name, "/PSXGAMES");

                if(!p) p = "NULL"; else p += 10;

                sprintf(name, "/dev_usb00%c/%s/%s", 47 + curr_device, path_install, p);
                mkdir_secure(name);

            }
            else
            {
                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47 + curr_device);
                mkdir_secure(name);
                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47 + curr_device);
                mkdir_secure(name);

                char * p = strstr(directories[game_sel].path_name, "/" __MKDEF_GAMES_DIR);
                if(!p) p = strstr(directories[game_sel].path_name, "/GAMES");

                if(!p) p = "NULL"; else p += 7;

                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/%s", 47 + curr_device, p);
                mkdir_secure(name);
            }
        }

    }
    else if(fdevices & 1)
    {
        //is usb
        for(n = 1; n < 11; n++)
        {
            if((directories[game_sel].flags >> n) & 1) break;
        }

        if(n == 11) return;

        curr_device = 0;

        dest = 0;

        sprintf(filename, "%s\n\n%s USB00%c %s HDD0?\nVol: %1.2f GB", directories[game_sel].title,
                        language[GLUTIL_WANTCPYFROM], 47 + n, language[GLUTIL_WTO],
                        ((double) copy_total_size) / GIGABYTES);

        dialog_action = 0;
        msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*)0x0000aaaa, NULL );

        wait_dialog();

        if(dialog_action == 1)
        {
            sprintf(progress_bar_title, "USB00%c -> HDD0", 47 + n);

            char *p;
            if((directories[game_sel].flags & (PS1_FLAG)) == (PS1_FLAG))
            {
                p = strstr(directories[game_sel].path_name, "/PSXGAMES");

                if(!p) {p = strstr(directories[game_sel].path_name, "/PSXISO"); if(p) p += 8;} else p += 10;

                if(!p) p = "NULL";
            }
            else
            if((directories[game_sel].flags & (PS2_FLAG)) == (PS2_FLAG))
            {
                p = strstr(directories[game_sel].path_name, "/PS2ISO");

                if(!p) p = "NULL"; else p += 8;
            }
            else
            if((directories[game_sel].flags & (PSP_FLAG)) == (PSP_FLAG))
            {
                p = strstr(directories[game_sel].path_name, "/PSPISO");

                if(!p) p = "NULL"; else p += 8;
            }
            else
            {
                p = strstr(directories[game_sel].path_name, "/" __MKDEF_GAMES_DIR); if(p) p += 7;

                if(!p) {p = strstr(directories[game_sel].path_name, "/GAMES"); if(p) p += 7;}

                if(!p) {p = strstr(directories[game_sel].path_name, "/GAMEZ"); if(p) p += 7;}

                if(!p) {p = strstr(directories[game_sel].path_name, "/PS3ISO"); if(p) p += 8;}

                if(!p) p = "NULL";
            }


            while(p[0] == '_') p++; // skip special char

            if(!memcmp(hdd_folder,"dev_hdd0", 9) || (directories[game_sel].flags & (PS1_FLAG)))
            {

                sprintf(name, "/%s/%s", hdd_folder2, path_install);
                mkdir_secure(name);
                sprintf(name, "/%s/%s/%s", hdd_folder2, path_install, p);
                mkdir_secure(name);
            }
            else if (!memcmp(hdd_folder, "GAMES", 6) || !memcmp(hdd_folder, "dev_hdd0_2", 11))
            {
                sprintf(name, "/%s/GAMES", "dev_hdd0");
                mkdir_secure(name);
                sprintf(name, "/%s/GAMES/%s", "dev_hdd0", p);
                mkdir_secure(name);

            }
            else
            {
                sprintf(name, "/dev_hdd0/game/%s", hdd_folder);
                mkdir_secure(name);
                sprintf(name, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR, hdd_folder);
                mkdir_secure(name);
                sprintf(name, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/%s", hdd_folder, p);
                mkdir_secure(name);
            }
        }
    }

    // reset to update datas
    forcedevices = (1 << curr_device);

    if(dialog_action == 1)
    {
        time_start = time(NULL);

        abort_copy = 0;
        DCls();
        file_counter = 0;
        new_pad = 0;

        DPrintf("%s %s\n %s %s\n\n", language[GAMECPYSL_STARTED], directories[game_sel].path_name, language[GLUTIL_WTO], name);

        if(curr_device != 0) copy_mode = 1; // break files >= 4GB
        else copy_mode = 0;

        copy_is_split=0;

        my_game_copy((char *) directories[game_sel].path_name, (char *) name);

        cls2();

        int seconds = (int) (time(NULL) - time_start);
        int vflip = 0;

        if(!abort_copy)
        {
            char *p;
            if(directories[game_sel].flags & (PS1_FLAG)) p = strstr(directories[game_sel].path_name, "/PSXGAMES");
            else
            {
                p = strstr(directories[game_sel].path_name, "/" __MKDEF_GAMES_DIR);

                if(!p) p = strstr(directories[game_sel].path_name, "/GAMES");
            }

            if(!p) p = "NULL"; else p+= 7;

            if(p[0] == '_') copy_is_split=555; // only rename the game
            while(p[0] == '_') p++; // skip special char

            if(dest == 0)
            {
                // change to no split!!!
                if(!memcmp(hdd_folder,"dev_hdd0", 9) || (directories[game_sel].flags & (PS1_FLAG)))
                    sprintf(filename, "/%s/%s/%s", hdd_folder2, path_install, p);
                else if (!memcmp(hdd_folder, "GAMES", 6) || !memcmp(hdd_folder, "dev_hdd0_2", 11))
                    sprintf(filename, "/%s/GAMES/%s", "dev_hdd0", p);
                else
                    sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/%s", hdd_folder, p);
            }
            else
            {
                if(copy_is_split)
                    sprintf(filename, "/dev_usb00%c/%s/_%s", 47+dest, path_install, p);
                else
                    sprintf(filename, "/dev_usb00%c/%s/%s", 47+dest, path_install, p);
            }
            //DrawDialogOK(name);
            //DrawDialogOK(filename);

            filepath_check(filename);

            // try rename
            if(copy_is_split)
            {
            #ifdef PSDEBUG
                ret =
            #endif
                sysLv2FsRename(name, filename);
            }


            if(dest != 0 && copy_is_split)
            {
                sprintf(filename, language[GAMECPYSL_SPLITEDUSBNFO], directories[game_sel].title, 47 + curr_device);

                dialog_action = 0;

                msgDialogOpen2( mdialogok, filename, my_dialog2, (void*) 0x0000aaab, NULL );

                wait_dialog();
            }

        }

        while(true)
        {
            if(abort_copy)
            {
                sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], language[GLUTIL_TIME],
                    seconds / 3600, (seconds/60) % 60, seconds % 60);
            }
            else
            {
                sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMECPYSL_DONE], language[GLUTIL_TIME],
                    seconds / 3600, (seconds / 60) % 60, seconds % 60, ((double) global_device_bytes) / GIGABYTES);
            }

            cls2();

            DbgHeader( string1);

            if(vflip & 32)
                DbgMess(language[GLUTIL_XEXIT]);
            else
                DbgMess("");

            vflip++;

            DbgDraw();

            tiny3d_Flip();

            if(ps3pad_poll())
            {
               new_pad = 0;
               break;
            }

        }

        if(abort_copy)
        {
            if(dest == 0)
                sprintf(filename, "%s\n\n%s HDD0?", directories[game_sel].title, language[GAMECPYSL_FAILDELDUMP]);
            else
                sprintf(filename, "%s\n\n%s USB00%c?", directories[game_sel].title, language[GAMECPYSL_FAILDELDUMP], 47 + dest);

            dialog_action = 0;

            msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );

            wait_dialog();

            if(dialog_action == 1)
            {
                abort_copy = 0;
                time_start = time(NULL);
                file_counter = 0;

                my_game_delete((char *) name);

                rmdir_secure((char *) name); // delete this folder

                game_sel = 0;
            }
            else
            {
                char *p;

                if(directories[game_sel].flags & (PS1_FLAG))
                    p = strstr(directories[game_sel].path_name, "/PSXGAMES");
                else
                {
                    p = strstr(directories[game_sel].path_name, "/" __MKDEF_GAMES_DIR);
                    if(!p) p = strstr(directories[game_sel].path_name, "/GAMES");
                }

                if(!p) p = "NULL"; else p+= 7;

                if(p[0] == '_') p++; // skip special char

                if(dest == 0)
                {
                    if(!memcmp(hdd_folder,"dev_hdd0", 9) || (directories[game_sel].flags & (PS1_FLAG)))
                        sprintf(filename, "/%s/%s/_%s", hdd_folder2, path_install, p);
                    else if (!memcmp(hdd_folder, "GAMES", 6) || !memcmp(hdd_folder, "dev_hdd0_2", 11))
                        sprintf(filename, "/%s/GAMES/_%s", "dev_hdd0", p);
                    else
                        sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, p);
                }
                else
                    sprintf(filename, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/_%s", 47 + dest, p);


                #ifdef PSDEBUG
                ret =
                #endif
                sysLv2FsRename(name, filename);
            }
        }

        game_sel = 0;

    }

}

void copy_from_bluray()
{
    char name[MAX_PATH_LEN];

    int curr_device = 0;
    sysFSStat status;

    char id[16];

    int n;
#ifdef PSDEBUG
    int ret;
#endif

    dialog_action = 0;
    abort_copy = 0;

    for(n = 0; n < 11; n++)
    {
        dialog_action = 0;

        if((fdevices >> n) & 1)
        {
            if(n == HDD0_DEVICE)
                sprintf(filename, "%s\n\n%s BDVD %s HDD0?\nVol: %1.2f GB", bluray_game,
                        language[GLUTIL_WANTCPYFROM], language[GLUTIL_WTO], ((double) copy_total_size) / GIGABYTES);
            else
                sprintf(filename, "%s\n\n%s BDVD %s USB00%c?\nVol: %1.2f GB",  bluray_game,
                        language[GLUTIL_WANTCPYFROM], language[GLUTIL_WTO], 47 + n, ((double) copy_total_size) / GIGABYTES);

            msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL);

            wait_dialog();

            if(dialog_action == 1)
            {
                if(n != 0) sprintf(progress_bar_title, "BDVD -> USB00%c", 47 + n);
                else       sprintf(progress_bar_title, "BDVD -> HDD0");
                           curr_device = n; break;
            }              // exit
        }
    }


    // reset to update datas
    forcedevices = (1 << curr_device);

    if(dialog_action == 1)
    {
        if(curr_device == 0) sprintf(name, "/dev_hdd0");
        else sprintf(name, "/dev_usb00%c", 47 + curr_device);

        if(curr_device == 0)
        {
            u32 blockSize;
            u64 freeSize;
            u64 free_hdd0;
            sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
            free_hdd0 = ( ((u64)blockSize * freeSize));

            if((copy_total_size + 0x40000000LL) >= (s64) free_hdd0)
            {
                sprintf(filename, language[GAMECHCPY_NOSPACE], free_hdd0, ((double) (copy_total_size + 0x40000000LL - free_hdd0)) / GIGABYTES);
                DrawDialogOK(filename);
                return;
            }
        }
        else
        {
            u32 blockSize;
            u64 freeSize;
            u64 free_usb;

            sprintf(filename,"%s/", name);
            sysFsGetFreeSize(filename, &blockSize, &freeSize);
            free_usb = ( ((u64)blockSize * freeSize) );

            if((copy_total_size + 0x100000LL) >= (s64) free_usb)
            {
                sprintf(filename, "%s\n\n%s%1.2f GB\n\n%s", "Warning: There is not enough space in USB to copy it", "You need ",
                    ((double) (copy_total_size + 0x100000LL - free_usb)) / GIGABYTES, "Do you want to abort?");

                dialog_action = 0;

                msgDialogOpen2( mdialogyesno2, filename, my_dialog, (void*) 0x0000aaaa, NULL);

                wait_dialog();

                if(dialog_action == 1)  {return;}  // exit
                else dialog_action = 1;
            }
        }

        if (sysFsStat(name, &status) == 0 && !parse_ps3_disc((char *) "/dev_bdvd/PS3_DISC.SFB", id))
        {
            if(curr_device == 0)
            {
                if(!memcmp(hdd_folder,"dev_hdd0", 9))
                {
                    sprintf(name, "/%s/" __MKDEF_GAMES_DIR, hdd_folder);
                    mkdir_secure(name);
                    sprintf(name, "/%s/" __MKDEF_GAMES_DIR "/%s", hdd_folder, id);
                    mkdir_secure(name);
                }
                else if (!memcmp(hdd_folder, "GAMES", 6) || !memcmp(hdd_folder, "dev_hdd0_2", 11))
                {
                    sprintf(name, "/%s/GAMES", "dev_hdd0");
                    mkdir_secure(name);
                    sprintf(name, "/%s/GAMES/%s", "dev_hdd0", id);
                    mkdir_secure(name);
                }
                else
                {
                    sprintf(name, "/dev_hdd0/game/%s", hdd_folder);
                    mkdir_secure(name);
                    sprintf(name, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR, hdd_folder);
                    mkdir_secure(name);
                    sprintf(name, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/%s", hdd_folder, id);
                    mkdir_secure(name);
                }
            }
            else
            {
                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47 + curr_device);
                mkdir_secure(name);
                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47 + curr_device);
                mkdir_secure(name);
                sprintf(name, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/%s", 47 + curr_device, id);
                mkdir_secure(name);
            }

            time_start = time(NULL);
            abort_copy = 0;
            DCls();
            file_counter = 0;
            new_pad = 0;

            if(curr_device != 0) copy_mode = 1; // break files >= 4GB
            else copy_mode = 0;

            copy_is_split = 0;

            my_game_copy((char *) "/dev_bdvd", (char *) name);

            int seconds = (int) (time(NULL) - time_start);
            int vflip = 0;

            if(copy_is_split && !abort_copy)
            {
                if(curr_device == 0)
                {
                    if (!memcmp(hdd_folder,"dev_hdd0", 9))
                        sprintf(filename, "/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, id);
                    else if (!memcmp(hdd_folder, "GAMES", 6) || !memcmp(hdd_folder, "dev_hdd0_2", 11))
                        sprintf(filename, "/%s/GAMES/_%s", "dev_hdd0", id);
                    else
                        sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, id);
                }
                else
                        sprintf(filename, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/_%s", 47 + curr_device, id);

                #ifdef PSDEBUG
                ret =
                #endif
                sysLv2FsRename(name, filename);

                if(curr_device == 0)
                    sprintf(filename, language[GAMECPYSL_SPLITEDHDDNFO], id);
                else
                    sprintf(filename, language[GAMECPYSL_SPLITEDUSBNFO], id, 47 + curr_device);

                dialog_action = 0;
                msgDialogOpen2( mdialogok, filename, my_dialog2, (void*) 0x0000aaab, NULL );
                wait_dialog();

            }

            while(true)
            {
                if(abort_copy)
                    sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], language[GLUTIL_TIME],
                            seconds / 3600, (seconds/60) % 60, seconds % 60);
                else
                    sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMECPYSL_DONE], language[GLUTIL_TIME],
                        seconds / 3600, (seconds/60) % 60, seconds % 60, ((double) global_device_bytes) / GIGABYTES);

                cls2();

                DbgHeader( string1);

                if(vflip & 32)
                    DbgMess(language[GLUTIL_XEXIT]);
                else
                    DbgMess("");

                vflip++;

                DbgDraw();

                tiny3d_Flip();

                if(ps3pad_poll())
                {
                   new_pad = 0;
                   break;
                }

            }

            if(abort_copy)
            {
                if(curr_device == 0)
                    sprintf(filename, "%s\n\n%s HDD0?", id, language[GAMECPYSL_FAILDELDUMP]);
                else
                    sprintf(filename, "%s\n\n%s USB00%c?", id, language[GAMECPYSL_FAILDELDUMP], 47 + curr_device);

                dialog_action = 0;
                msgDialogOpen2(mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );

                wait_dialog();

                if(dialog_action == 1)
                {
                    time_start = time(NULL);
                    file_counter = 0;
                    abort_copy = 0;
                    my_game_delete((char *) name);

                    rmdir_secure((char *) name); // delete this folder

                }
                else
                {
                    if(curr_device == 0)
                    {
                        if(!memcmp(hdd_folder,"dev_hdd0", 9))
                            sprintf(filename, "/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, id);
                        else if (!memcmp(hdd_folder, "GAMES", 6) || !memcmp(hdd_folder, "dev_hdd0_2", 11))
                            sprintf(filename, "/%s/GAMES/_%s", "dev_hdd0", id);
                        else
                            sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR "/_%s", hdd_folder, id);
                    }
                    else
                        sprintf(filename, "/dev_usb00%c/" __MKDEF_GAMES_DIR "/_%s", 47 + curr_device, id);

                    #ifdef PSDEBUG
                    ret =
                    #endif
                    sysLv2FsRename(name, filename);

                }
            }
        }
    }
}

/////////////////

float cache_need_free = 0.0f;

void copy_to_cache(int game_sel, char * hmanager_path)
{

    if(directories[game_sel].flags & D_FLAG_BDVD)  {return;}
    if(directories[game_sel].flags & D_FLAG_HDD0)  {return;}

    int n;

    char name[MAX_PATH_LEN];
    char name2[MAX_PATH_LEN];
    int dest = 0;

    dialog_action = 0;
    abort_copy = 0;


    for(n = 1; n < 11; n++)
    {
       if((directories[game_sel].flags >> n) & 1) break;
    }

    if(n == 11) return;

    dest = n;

    sprintf(filename, "%s\n\n%s USB00%c %s HDD0 CACHE?", directories[game_sel].title, language[GLUTIL_WANTCPYFROM], 47 + dest, language[GLUTIL_WTO]);

    dialog_action = 0;
    msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*)0x0000aaaa, NULL );

    wait_dialog();

    if(dialog_action == 1)
    {
        sprintf(name2, "%s/PS3_GAME", directories[game_sel].path_name);

        path_cache = name;

        nfilecached = 0;
        filecached[nfilecached][0][0] = 0;
        filecached[nfilecached][1][0] = 0;

        // reset to update data

        time_start = time(NULL);

        abort_copy = 0;
        DCls();
        file_counter = 0;
        new_pad = 0;

        //////////////

        global_device_bytes = 0;
        cache_need_free = 0;
        num_directories = file_counter = num_files_big = num_files_split = fixed_self_sprx = 0;

        copy_split_to_cache = 1;
        my_game_test((char *) name2);
        copy_split_to_cache = 0;

        if(!nfilecached)
        {
            sprintf(string1, language[GAMECHCPY_ISNEEDONEFILE]);

            DrawDialogOK(string1);

            cache_need_free = 0.0f;

            global_device_bytes = 0;

            abort_copy = 0;
            file_counter = 0;
            copy_mode = 0;

            new_pad = 0;

            return;
        }

        u32 blockSize;
        u64 freeSize;
        float freeSpace;
        float tmp_total_bytes = (global_device_bytes / GIGABYTES); //save total bytes counted

        sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
        freeSpace = ( ((u64)blockSize * freeSize) );
        freeSpace = freeSpace / GIGABYTES;

        for(n = 0; n < nfilecached; n++)
            cache_need_free += filecached_bytes[n];

        global_device_bytes = cache_need_free; // update with correct value

        cache_need_free = (cache_need_free / GIGABYTES) + 2.0f; // +2 for system

        if(freeSpace < cache_need_free) {
            sprintf(string1, language[GAMECHCPY_NEEDMORESPACE], freeSpace, cache_need_free);

            DrawDialogOK(string1);

            draw_cache_external();

            new_pad = 0;
        }

        sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
        freeSpace = ( ((u64)blockSize * freeSize));
        freeSpace = freeSpace / GIGABYTES;

        if(freeSpace < cache_need_free)
        {
            sprintf(string1, language[GAMECHCPY_NOSPACE], freeSpace, cache_need_free);

            DrawDialogOK(string1);

            cache_need_free = 0.0f;

            global_device_bytes = 0;

            abort_copy = 0;
            file_counter = 0;
            copy_mode = 0;

            new_pad = 0;

            return;
        }

        sprintf(string1, language[GAMECHCPY_CACHENFOSTART], (cache_need_free - 2.0f), tmp_total_bytes, (tmp_total_bytes - (cache_need_free - 2.0f)) , freeSpace);
        DrawDialogOK(string1);

        sprintf(name, "%s/cache", hmanager_path);
        mkdir_secure(name);
        sprintf(name, "%s/cache/%s", hmanager_path, directories[game_sel].title_id);
        mkdir_secure(name);

        copy_total_size = global_device_bytes;

        cache_need_free = 0.0f;
        global_device_bytes = 0;

        ////////////////

        DPrintf("%s %s\n %s %s\n\n", language[GAMECPYSL_STARTED], directories[game_sel].path_name, language[GLUTIL_WTO], name);

        abort_copy = 0;
        file_counter = 0;
        copy_mode = 0;

        copy_is_split = 0;

        copy_split_to_cache = 1;
        my_game_copy((char *) name2, (char *) name);
        copy_split_to_cache = 0;

        int seconds = (int) (time(NULL) - time_start);
        int vflip = 0;

        while(true)
        {
            if(abort_copy)
                sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], language[GLUTIL_TIME],
                        seconds / 3600, (seconds/60) % 60, seconds % 60);
            else
                sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMECPYSL_DONE], language[GLUTIL_TIME],
                        seconds / 3600, (seconds/60) % 60, seconds % 60, ((double) global_device_bytes) / GIGABYTES);

            cls2();

            DbgHeader( string1);

            if(vflip & 32)
                DbgMess(language[GLUTIL_XEXIT]);
            else
                DbgMess("");

            vflip++;

            DbgDraw();

            tiny3d_Flip();

            if(ps3pad_poll())
            {
               new_pad = 0;
               break;
            }

        }

        if(abort_copy || nfilecached  == 0)
        {
            sprintf(filename, "%s\n\n%s USB00%c?", directories[game_sel].title, language[GAMECHCPY_FAILDELFROM], 47 + dest);

            dialog_action = 0;

            msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );

            wait_dialog();

            if(dialog_action == 1)
            {
                abort_copy = 0;
                time_start = time(NULL);
                file_counter = 0;

                my_game_delete((char *) name);

                rmdir_secure((char *) name); // delete this folder

                game_sel = 0;
            }
        }
        else
        {
           sprintf(name, "%s/cache/%s/paths.dir", hmanager_path, directories[game_sel].title_id);
           SaveFile(name, (char *) filecached, 2048 * nfilecached);
           sprintf(name, "%s/cache/%s/name_entry", hmanager_path, directories[game_sel].title_id);
           SaveFile(name, (char *) directories[game_sel].title, 64);
        }

    }

}
////////////////////
void delete_game(int game_sel)
{

    int n;

    copy_split_to_cache = 0;

    if(directories[game_sel].flags & D_FLAG_BDVD) return;

    for(n = 0; n < 11; n++)
    {
        if((directories[game_sel].flags >> n) & 1) break;
    }

    if(n == HDD0_DEVICE)
        sprintf(filename, "%s\n\n%s HDD0?", directories[game_sel].title, language[GAMEDELSL_WANTDELETE]);
    else
        sprintf(filename, "%s\n\n%s USB00%c?", directories[game_sel].title, language[GAMEDELSL_WANTDELETE], 47 + n);

    dialog_action = 0;

    msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*) 0x0000aaaa, NULL );

    wait_dialog();

    // reset to update datas
    forcedevices = (1 << n);

    if(dialog_action == 1) {

        time_start = time(NULL);

        abort_copy = 0;
        DCls();
        file_counter = 0;
        new_pad = 0;

        DPrintf("%s %s\n\n", language[GAMEDELSL_STARTED], directories[game_sel].path_name);

        my_game_delete((char *) directories[game_sel].path_name);

        rmdir_secure((char *) directories[game_sel].path_name); // delete this folder

        game_sel = 0;

        int seconds = (int) (time(NULL) - time_start);
        int vflip = 0;

        while(true)
        {
            if(abort_copy)
                sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], language[GLUTIL_TIME],
                        seconds / 3600, (seconds/60) % 60, seconds % 60);
            else
                sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GAMEDELSL_DONE], language[GLUTIL_TIME],
                        seconds / 3600, (seconds/60) % 60, seconds % 60);

            cls2();

            DbgHeader( string1);

            if(vflip & 32)
                DbgMess(language[GLUTIL_XEXIT]);
            else
                DbgMess("");

            vflip++;

            DbgDraw();

            tiny3d_Flip();

            if(ps3pad_poll())
            {
               new_pad = 0;
               break;
            }
        }
    }
}


void test_game(int game_sel)
{

    int r = 0;

    time_start = time(NULL);

    abort_copy = 0;

    copy_split_to_cache = 0;

    DCls();

    file_counter = 0;
    new_pad = 0;

    global_device_bytes = 0;

    num_directories = file_counter = num_files_big = num_files_split = fixed_self_sprx = 0;


    self_alarm_version = 0;

    if(!(directories[game_sel].flags & D_FLAG_NTFS))
    {
        my_game_test(directories[game_sel].path_name);

        r = self_alarm_version;
    }

    char game_update_path[MAX_PATH_LEN];

    sprintf(game_update_path, "/dev_hdd0/game/%c%c%c%c%s", directories[game_sel].title_id[0], directories[game_sel].title_id[1],
            directories[game_sel].title_id[2], directories[game_sel].title_id[3], &directories[game_sel].title_id[5]);
    sysFSStat s;

    self_alarm_version = 0;

    if(!sysLv2FsStat(game_update_path, &s))
        patch_error_09(game_update_path, !(old_pad & BUTTON_SELECT));
    //else if((directories[game_sel].flags & (BDVD_FLAG | PS3_FLAG)) == (PS3_FLAG))
    //    patch_error_09(directories[game_sel].path_name, 1);

    DPrintf(language[GAMETSTSL_FINALNFO2], num_directories, file_counter, num_files_big, num_files_split);
    DPrintf("\n\n");
    DPrintf("SPRX/SELF fixed (error 0x80010009) %i", fixed_self_sprx);
    DPrintf("\n\n");

    if(r)
        DPrintf("This game requires a higher CFW or rebuild the SELFs/SPRX\n\nEste juego requiere un CFW superior o reconstruir los SELFs/SPRX\n\n");

    if(self_alarm_version)
        DPrintf("The update of this game requires a higher CFW or rebuild the SELFs/SPRX\n\nLa actualizacion de este juego requiere un CFW superior o reconstruir los SELFs/SPRX\n\n");

    int seconds = (int) (time(NULL) - time_start);
    int vflip = 0;

    while(true)
    {
        if(abort_copy)
            sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED],
                    language[GLUTIL_TIME], seconds / 3600, (seconds/60) % 60, seconds % 60);
        else
            sprintf(string1,"%s: %i %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMETSTSL_TESTED], file_counter,
                    language[GLUTIL_TIME], seconds / 3600, (seconds / 60) % 60, seconds % 60,
                    ((double) global_device_bytes) / GIGABYTES);

        cls2();

        DbgHeader( string1);

        if(vflip & 32)
            DbgMess(language[GLUTIL_XEXIT]);
        else
            DbgMess("");

        vflip++;

        DbgDraw();

        tiny3d_Flip();

        ps3pad_poll();
        if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE | BUTTON_CROSS_))
        {
           new_pad = 0;
           break;
        }
    }


    // rename in test for non executable games
    if(num_files_split && (directories[game_sel].flags & D_FLAG_USB))
    {
        char *str = strstr(directories[game_sel].path_name, __MKDEF_GAMES_DIR);

        if(str && str[7] != '_')
        {
            int n = (str - directories[game_sel].path_name);
            memcpy(filename, directories[game_sel].path_name, n + 7);filename[n+7] = '_'; filename[n+8] =0;
            strcat(filename, str + 7);
            sysLv2FsRename(directories[game_sel].path_name, filename);
            //DrawDialogOK(filename);
            forcedevices = D_FLAG_USB;
        }
    }
}

void DeleteDirectory(const char* path)
{
    char newpath[0x440];
    sysFSDirent dir; u64 read = sizeof(sysFSDirent);

    bool is_ntfs = is_ntfs_path((char *) path);

    if (!is_ntfs)
    {
        int dfd;

        if (sysLv2FsOpenDir(path, &dfd)) return;

        sysFsChmod(path, FS_S_IFDIR | 0777);

        read = sizeof(sysFSDirent);
        while (!sysLv2FsReadDir(dfd, &dir, &read))
        {
            if (!read) break;

            if(dir.d_name[0]=='.' && (dir.d_name[1]==0 || dir.d_name[1]=='.')) continue;

            sprintf(newpath, "%s/%s", path, dir.d_name);

            if (dir.d_type & DT_DIR)
            {
                DeleteDirectory(newpath);
                sysLv2FsChmod(newpath, FS_S_IFDIR | 0777);
                sysLv2FsRmdir(newpath);
            }
            else
            {
                sysLv2FsChmod(path, FS_S_IFMT | 0777);
                sysLv2FsUnlink(newpath);
            }
        }

        sysLv2FsCloseDir(dfd);
    }
    else
    {
        struct stat st;
        DIR_ITER *pdir = NULL;

        if ((pdir = ps3ntfs_diropen(path)) == NULL) return;

        while (ps3ntfs_dirnext(pdir, dir.d_name, &st) == 0)
        {
            if(dir.d_name[0]=='.' && (dir.d_name[1]==0 || dir.d_name[1]=='.')) continue;

            sprintf(newpath, "%s/%s", path, dir.d_name);

            if (S_ISDIR(st.st_mode))
            {
                DeleteDirectory(newpath);
            }

            ps3ntfs_unlink(newpath);
        }

        ps3ntfs_dirclose(pdir);
    }
}

int FixDirectory(const char* path, int fcount)
{
    int dfd;
    u64 read;
    sysFSDirent dir;

    if (sysLv2FsOpenDir(path, &dfd)) return FAILED;

    sysFsChmod(path, FS_S_IFDIR | 0777);

    if (fcount == 0)
    {
        msgDialogOpen2(mdialogprogress, progress_bar_title, progress_callback, (void *) 0xadef0045, NULL);
        msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    }

    read = sizeof(sysFSDirent);
    while (!sysLv2FsReadDir(dfd, &dir, &read) && (fcount >=0))
    {
        if (!read)
            break;
        if (!strcmp(dir.d_name, ".") || !strcmp(dir.d_name, ".."))
            continue;

        char newpath[0x440];
        strcpy(newpath, path);
        strcat(newpath, "/");
        strcat(newpath, dir.d_name);

        fcount++;

        if ((fcount & 4) == 4)
        {
            static char string1[256];
            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);

            sprintf(string1, "Fixing permissions: %s", dir.d_name);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, string1);

            //estimated progress bar (just to show some progress)
            bar1_countparts = 10 * fcount / (5 * fcount + 100);
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) bar1_countparts);

            DbgDraw();
            tiny3d_Flip();
        }

        if(ps3pad_poll()) return FAILED;

        if (dir.d_type & DT_DIR)
        {
            if(FixDirectory(newpath, fcount) == FAILED) {sysLv2FsCloseDir(dfd); return FAILED;}
        }
        else
        {
            sysLv2FsChmod(newpath, FS_S_IFMT | 0777);
        }
    }

    sysLv2FsCloseDir(dfd);
    return SUCCESS;
}

/*******************************************************************************************************************************************************/
/* Configfiles                                                                                                                                         */
/*                                                                                                                                                     */
/* Caprice32 - Amstrad CPC Emulator                                                                                                                    */
/*   (c) Copyright 1997-2004 Ulrich Doewich                                                                                                            */
/*   (c) Copyright 2011 D_Skywalk - ported and adapted for Irismanager                                                                                 */
/*******************************************************************************************************************************************************/

/*
   Ex:
   snd_pp_device = getConfigValueInt("/home/user/config.ini", "sound", "pp_device", 0);
   max_tracksize = getConfigValueInt(chFileName, "file", "max_track_size", 6144-154);
*/

int getConfigMemValueInt (char* mem, int size, char* pchSection, char* pchKey, int iDefaultValue)
{

   char chLine[MAX_CFGLINE_LEN + 1];
   char* pchToken;
   int n = 0;

      while(n < size)
      {
        // grab one line
        int flag = 0;
        int m = 0;
        while(mem && n < size && m<MAX_CFGLINE_LEN-1)
        {
            chLine[m] = mem[n]; n++;
            if(chLine[m] == '\n' || chLine[m] == '\r') {m++; flag = 1; continue;}
            if(flag) {n--;break;}
            m++;
        }

        chLine[m]= 0;  // grab one line
         pchToken = strtok(chLine, "[]"); // check if there's a section key
         if((pchToken != NULL) && (pchToken[0] != '#') && (strcmp(pchToken, pchSection) == 0)) {
            while(n < size) { // get the next line
               m = 0; flag = 0;
               while(mem && n < size && m < MAX_CFGLINE_LEN - 1)
               {
                    chLine[m] = mem[n]; n++;
                    if(chLine[m] == '\n' || chLine[m] == '\r') {m++; flag = 1; continue;}
                    if(flag) {n--;break;}
                    m++;
               }
               chLine[m]= 0;
               pchToken = strtok(chLine, "\t =\n\r"); // check if it has a key=value pair
               if((pchToken != NULL) && (pchToken[0] != '#') && (strcmp(pchToken, pchKey) == 0)) {
                  char* pchPtr = strtok(NULL, "\t =#\n\r"); // get the value if it matches our key
                  if (pchPtr != NULL) {
                     return (strtol(pchPtr, NULL, 0)); // return as integer
                  } else {
                     return iDefaultValue; // no value found
                  }
               }
            }
         }
      }

   return iDefaultValue; // no value found
}

/*
   Ex:
   getConfigValueString(chFileName, "file", "snap_path", snap_path, sizeof(snap_path)-1, defaultPath);
   getConfigValueString(chFileName, "file", "push_file", snap_file, sizeof(push_file)-1, "push0.sna");

*/

void reverse_strings(u8 *str)
{
    int n, m, l;
    int len1, len2;
    u32 string_buffer[1024];

    while(true)
    {
        if(*str == 0) break;
        while(*str < 32) str++;
        if(*str == 0) break;

        len1 = 0; len2 =0;
        // process one phrase
        while(str[len1]>=32)
        {
            if((str[len1] & 0xE0) == 0xC0 && (str[len1 + 1] & 0xc0) == 0x80)
            {
                string_buffer[len2] = ((u32) str[len1 + 1] << 8) | (u32) str[len1 + 0];
                len1+=2; len2++;
            }
            else if((str[len1] & 0xF0) == 0xE0  && (str[len1 + 1] & 0xc0) == 0x80 && (str[len1 + 2] & 0xc0) == 0x80)
            {
                string_buffer[len2] = ((u32) str[len1 + 2] << 16) | ((u32) str[len1 + 1] << 8) | (u32) str[len1 + 0];
                len1+=3; len2++;
            }
            else if((str[len1] & 0xF0) == 0xF0  && (str[len1 + 1] & 0xc0) == 0x80  && (str[len1 + 2] & 0xc0) == 0x80  && (str[len1 + 3] & 0xc0) == 0x80)
            {
                string_buffer[len2] = ((u32) str[len1 + 3] << 24) | ((u32) str[len1 + 2] << 16) | ((u32) str[len1 + 1] << 8) | (u32) str[len1 + 0];
                len1+=4; len2++;
            }
            else
            {
                if(str[len1 + 0] & 0x80)  string_buffer[len2] = '?'; else string_buffer[len2] = (u32) str[len1 + 0];
                len1++; len2++;
            }
        }

        l = 0;

        len2--;
        for(n = 0; n < len2 + 1; n++)
        {
            // reverse UTF8 symbols
            if(string_buffer[len2 - n] == 32   || (string_buffer[len2 - n] & 0xffffff80))
            {
                str[l] = string_buffer[len2 - n] & 0xff; string_buffer[len2 - n]>>= 8;l++;
                if(string_buffer[len2 - n] & 0xff)
                {
                    str[l] = string_buffer[len2 - n] & 0xff; string_buffer[len2 - n]>>= 8;l++;
                    if(string_buffer[len2 - n] & 0xff)
                    {
                        str[l] = string_buffer[len2 - n] & 0xff; string_buffer[len2 - n]>>= 8;l++;
                        if(string_buffer[len2 - n] & 0xff)
                        {
                            str[l] = string_buffer[len2 - n] & 0xff;
                            l++;
                        }
                    }
                }
            }
            else
            {
                // dont reverse words with symbols from 32 to 127 UTF8
                int s;
                m= 0;
                while((m+n) < (len2 + 1) && (string_buffer[len2 - n - m] & 0xff) > 32 &&
                      (string_buffer[len2 - n - m] & 0xffffff80) == 0) m++;

                for(s = 0; s < m; s++)
                {
                    str[l + m - s - 1] = string_buffer[len2 - n - s] & 0xff;
                }
                l += m;
                n += m - 1;
            }
        }

        str += len1;
    }
}

void convertStringEndl(char* string, int iSize);

extern int reverse_language;

int getConfigMemValueString(char* mem, int size, char* pchSection, char* pchKey, char* pchValue, int iSize, char* pchDefaultValue)
{

   char chLine[MAX_CFGLINE_LEN + 1];
   int n = 0;
   int find_section = 0;

   // open the config file

   while(n < size)
   {
      if(mem[n] == '[') break;
      n++;
   }

   while(n<size)
   {
       // grab one line
       int flag = 0;
       int m = 0, l;
       while(mem && n < size && m < MAX_CFGLINE_LEN - 1)
       {
           chLine[m] = mem[n]; n++;
           if(chLine[m] == '\n' || chLine[m] == '\r') {m++; flag = 1; continue;}
           if(flag) {n--; break;}
           m++;
       }
       chLine[m]= 0;

       // note from Estwald
       // routines to avoid the stupid strok() "break-lines" function. XD
       // standard function is not the better way to get custom strings

       m = 0; while (chLine[m] == ' ' || chLine[m] == '\t') m++; // skip spaces and tabs
       if(chLine[m] == '#') continue; // skip comment. Next line

       if(chLine[m] == '[')
       {
           // section token finded
           if(find_section) break; // this is another different section, sure

           m++;
           l = m;

           while (chLine[l]!=']' && chLine[l]!=0) l++;

           if(chLine[l]!=']') break;  // section error!!!

           // test if section do not match
           if(strlen(pchSection) != (l - m) || strncmp(pchSection, &chLine[m], l - m)) continue;
           find_section = 1;
       }

       if(find_section)
       {
           // get strings

           // find string to compare
           l = m; while (chLine[l] != '=' && chLine[l] != ' ' && chLine[l] != '\t' && chLine[l] != 0) l++;

           if(chLine[l] == 0) continue; // no match, error in line?. Next line

           if((l-m) < 4) continue; // invalid, sure (string too short). Next line

           if(strlen(pchKey) != (l - m) || strncmp(pchKey, &chLine[m], l - m)) continue;// no match. Next line

           if(chLine[l]!='=') while (chLine[l] == ' ' || chLine[l] == '\t') l++; // skip spaces

           if(chLine[l]!='=') continue;// no match. Next line (string can be valid, but you need find '=')

           m = l + 1; // skip '='
           while (chLine[m] == ' ' || chLine[m] == '\t') m++; // skip spaces and tabs

           l=m; while(chLine[l]!=0 && chLine[l]!='\n' && chLine[l]!='\r') l++; // find the end of line

           chLine[l] = 0; // break line to avoid \n \r

           // &chLine[m] countain the replacement string without string splitted ;)

           convertStringEndl(&chLine[m], iSize);
           strncpy(pchValue, &chLine[m], iSize); // copy to destination

           pchValue[iSize -1] = 0;

           if(reverse_language)
           {
               if(pchValue[0] == '-')
               {
                   // don't reverse this string
                   strncpy(pchValue, &chLine[m + 1], iSize);
                   pchValue[iSize - 1] = 0;
               }
               else
                   reverse_strings((u8*) pchValue);
           }

           return SUCCESS;
       }
   }

   strncpy(pchValue, pchDefaultValue, iSize); // no value found, return the default
   pchValue[iSize -1] = 0;
   return FAILED;
}

#define MAX_CONVERSIONS 1

char conversionTable[MAX_CONVERSIONS][2] =
{
    { '@', '\n' },
   // { '_', ' ' },
};

void convertStringEndl(char* string, int iSize)
{
    int n;
    int m = 0;
    int flag = 0;

    do
    {
        if(string[m] == 9) string[m] = ' ';
        if(string[m] < 32) {string[m] = 0; return;} // break the string
        // special symbol '_' used as space when it is concatenated
        if(string[m] == '_') {
            if(flag || string[m + 1] == '_') {flag = 1; string[m]=' ';}
        }
        else flag = 0;

        for(n = 0; n < MAX_CONVERSIONS; n++)
            if(string[m] == conversionTable[n][0])
                string[m] = conversionTable[n][1];
        m++;
    }
    while(iSize--);
}

/*******************************************************************************************************************************************************/
/* Favourites                                                                                                                                          */
/*******************************************************************************************************************************************************/

int havefavourites = 0;
tfavourites2 favourites;

static tfavourites2 favourites_gamebase;
static tfavourites2 favourites_homebrew;
static tfavourites2 favourites_films;

void LoadFavourites(char * path, int mode)
{
    int n, mode_flag;
    tfavourites2 * pfavourites = NULL;

    if(mode == GAMEBASE_MODE)
    {
        pfavourites = &favourites_gamebase;
        mode_flag = 0;
        strcat(path, "favourites.bin");
    }
    else if(mode == HOMEBREW_MODE)
    {
        pfavourites = &favourites_homebrew;
        mode_flag = 0xffff0000;
        strcat(path, "favourites2.bin");
    }
    else
    {
        pfavourites = &favourites_films;
        mode_flag = 0xffff0000;
        strcat(path, "favourites3.bin");
    }

    memset(pfavourites, 0, sizeof(*pfavourites));

    int file_size;
    char *file = LoadFile(path, &file_size);

    if(file)
    {
        tfavourites2 *fav = (tfavourites2 *) file;
        if(file_size == sizeof(tfavourites) && fav->version == 100)
            memcpy(pfavourites, file, sizeof(tfavourites));
        if(file_size == sizeof(*pfavourites) && fav->version == 101)
            memcpy(pfavourites, file, sizeof(*pfavourites));
        free(file);
    }

    for(n = 0; n < 48; n++)
    {
        pfavourites->list[n].index  = -1;
        pfavourites->list[n].flags &=  mode_flag;
    }

}

void SaveFavourites(char * path, int mode)
{
    favourites.version = 101;
    if(mode == GAMEBASE_MODE)
    {
        strcat(path, "favourites.bin");
        favourites_gamebase = favourites;
    }
    else if (mode == HOMEBREW_MODE)
    {
        strcat(path, "favourites2.bin");
        favourites_homebrew = favourites;
    }
    else return;

    SaveFile(path, (void *) &favourites, sizeof(favourites)); // use favourites_gamebase?
}

void GetFavourites(int mode)
{
    if(mode > HOMEBREW_MODE)
        favourites = favourites_films;
    else if(mode == HOMEBREW_MODE)
        favourites = favourites_homebrew;
    else
        favourites = favourites_gamebase;
}

void SetFavourites(int mode)
{
    if(mode > HOMEBREW_MODE)
        favourites_films = favourites;
    else if(mode == HOMEBREW_MODE)
        favourites_homebrew = favourites;
    else
        favourites_gamebase = favourites;
}

void UpdateFavourites(t_directories *list, int nlist)
{
    int n, m;

    havefavourites = 0;

    for(m = 0; m < num_box; m++)
    {
        favourites.list[m].index = -1;

        for(n = 0; n < nlist; n++)
        {
            if(favourites.list[m].title_id[0] !=0 && !strncmp(list[n].title_id, favourites.list[m].title_id, 64))
            {
                if((/*1*/ favourites.list[m].index < 0) ||
                    /*2*/((favourites.list[m].flags & NTFS_FLAG) != NTFS_FLAG &&
                    ((favourites.list[m].flags & GAMELIST_FILTER) > (list[n].flags & GAMELIST_FILTER)
                    || (list[n].flags & NTFS_FLAG) == NTFS_FLAG)) ||
                    /*3*/((favourites.list[m].flags & NTFS_FLAG) == NTFS_FLAG && (list[n].flags & D_FLAG_HDD0) == D_FLAG_HDD0))
                {
                    //strncpy(favourites.list[m].title_id, list[n].title_id, 64);
                    //strncpy(favourites.list[m].title, list[n].title, 64);
                    favourites.list[m].index = n;
                    favourites.list[m].flags = list[n].flags;
                    havefavourites = 1;
                }
            }
        }
    }
}


int TestFavouritesExits(char *id)
{
    int m;
    for(m = 0; m < 48; m++)
    {
        if(!strncmp(favourites.list[m].title_id, id, 64)) return true;
    }

    return false;
}

void AddFavourites(int indx, t_directories *list, int position_list)
{
    strncpy(favourites.list[indx].title_id, list[position_list].title_id, 64);
    strncpy(favourites.list[indx].title, list[position_list].title, 64);
    favourites.list[indx].index = position_list;
    favourites.list[indx].flags = list[position_list].flags;
    havefavourites = 1;
}

void DeleteFavouritesIfExits(char *id)
{
    int m;
    for(m = 0; m < 48; m++)
    {
        if(!strcmp(favourites.list[m].title_id, id))
        {
            memset(favourites.list[m].title_id, 0, 64);
            memset(favourites.list[m].title, 0, 64);
            favourites.list[m].index = -1;
        }
    }

    havefavourites = 0;

    for(m = 0; m < 48; m++)
    {
        if(favourites.list[m].index >= 0) {havefavourites = 1; break;}
    }

    return;
}

int param_sfo_util(char * path, int patch_app)
{
    bool patched = false;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;
    char file[MAX_PATH_LEN];
    char file2[MAX_PATH_LEN];

    u8 * app_ver = NULL;

    char str_version[8];

    unsigned char *mem = NULL;
    unsigned char *mem2 = NULL;

    int n;
    char * version = LoadFile("/dev_flash/vsh/etc/version.txt", &n);

    if(!version) return -2;

    char *v = strstr(version, "release:");

    if(v) {memcpy(str_version, v + 8, 7); str_version[7] = 0;}

    free(version);

    if(!v) return -3;


    sprintf(file, "%s/PS3_GAME/PARAM.SFO", path);
    sprintf(file2, "%s/PS3_GAME/_PARAM.SFO", path);

    if(!sysLv2FsOpen(file2, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        unsigned len, pos, str;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        mem2 = (unsigned char *) malloc(len + 16);
        if(!mem2) {sysLv2FsClose(fd); return -2;}

        memset(mem2, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem2, len, &bytes) != 0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem2[8] + (mem2[9]<<8));
        pos = (mem2[0xc] + (mem2[0xd]<<8));

        int indx = 0;

        while(str < len)
        {
            if(mem2[str] == 0) break;

            if(!strcmp((char *) &mem2[str], "APP_VER")) app_ver = &mem2[pos];

            while(mem2[str] && str < len) str++; str++;
            pos += (mem2[0x1c + indx] + (mem2[0x1d + indx]<<8));
            indx += 16;
        }

    }

    n = sysLv2FsOpen(file, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0);

    if(n) {n = sysLv2FsOpen(file2, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0); patched = true;}

    if(!n)
    {
        unsigned len, pos, str;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes) != 0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem[8] + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0;

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strcmp((char *) &mem[str], "PS3_SYSTEM_VER"))
            {
                if(strcmp((char *) &mem[pos], str_version) > 0)
                {
                    memcpy(&mem[pos], str_version, 8);
                    patched = true;
                }
            }
            else
            if(!strcmp((char *) &mem[str], "APP_VER"))
            {
               u8 old =  mem[pos + 1];

               if(app_ver)
               {
                   mem[pos + 1] = app_ver[1];
                   if(mem[pos + 1] == '9') mem[pos + 1] = '1';
               } else mem[pos + 1] = '1';

               if(patch_app)
               {
                    mem[pos + 1] = '9';
               }

               if(old != mem[pos + 1]) patched = true;
            }

            while(mem[str] && str < len) str++; str++;
            pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
            indx += 16;
        }

        if(patched)
        {
            sysLv2FsRename(file, file2);
            SaveFile(file, (char *) mem, len);
        }

        if(mem) free(mem);
        if(mem2) free(mem2);

        return SUCCESS;
    }

    return FAILED;
}

int param_sfo_patch_category_to_cb(char * path_src, char *path_dst)
{
    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;
    char file[MAX_PATH_LEN];
    char file2[MAX_PATH_LEN];

    unsigned char *mem2 = NULL;

    sprintf(file, "%s", path_src);
    sprintf(file2, "%s", path_dst);

    if(!sysLv2FsOpen(file, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        unsigned len, pos, str;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        mem2 = (unsigned char *) malloc(len + 16);
        if(!mem2)
        {
            sysLv2FsClose(fd);
            return -2;
        }

        memset(mem2, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem2, len, &bytes)!=0)
            bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem2[8] + (mem2[9]<<8));
        pos = (mem2[0xc] + (mem2[0xd]<<8));
        int indx = 0;

        while(str < len)
        {
            if(mem2[str] == 0)
                break;

            if(!strcmp((char *) &mem2[str], "CATEGORY"))
            {
                mem2[pos] = 'H';
                mem2[pos+1] = 'G';
            }

            if(!strcmp((char *) &mem2[str], "APP_VER"))
            {
                mem2[pos] = '9'; mem2[pos + 1] = '9'; mem2[pos + 3] = '9'; mem2[pos + 4] = '9';
            }

            while(mem2[str] && str < len) str++; str++;
            pos  += (mem2[0x1c + indx] + (mem2[0x1d + indx]<<8));
            indx += 16;
        }

        if(SaveFile(file2, (char *) mem2, len))
        {
            free(mem2);
            return -2;
        }

        if(mem2) free(mem2);

        return SUCCESS;
    }

    return FAILED;
}

/*******************************************************************************************************************************************************/
/* sys8 path table                                                                                                                                     */
/*******************************************************************************************************************************************************/


static char *table_compare[19];
static char *table_replace[19];

static int ntable = 0;

void reset_sys8_path_table()
{
    while(ntable > 0)
    {
        if(table_compare[ntable - 1]) free(table_compare[ntable - 1]);
        if(table_replace[ntable - 1]) free(table_replace[ntable - 1]);

        ntable --;
    }
}

void add_sys8_path_table(char * compare, char * replace)
{
    if(ntable >= 16) return;

    table_compare[ntable] = malloc(strlen(compare) + 1);
    if(!table_compare[ntable]) return;
    strncpy(table_compare[ntable], compare, strlen(compare) + 1);

    table_replace[ntable] = malloc(strlen(replace) + 1);
    if(!table_replace[ntable]) return;
    strncpy(table_replace[ntable], replace, strlen(replace) + 1);

    ntable++;

    table_compare[ntable] = NULL;
}

void add_sys8_bdvd(char * bdvd, char * app_home)
{
    static char compare1[]="/dev_bdvd";
    static char compare2[]="/app_home";
    static char replace1[MAX_PATH_LEN];
    static char replace2[MAX_PATH_LEN];
    int pos = 17;

    table_compare[pos] = NULL;
    table_compare[pos + 1] = NULL;

    if(bdvd)
    {
        strncpy(replace1, bdvd, MAX_PATH_LEN);
        table_compare[pos] = compare1;
        table_replace[pos] = replace1;
        pos++;
    }

    if(app_home)
    {
        strncpy(replace2, app_home, MAX_PATH_LEN);
        table_compare[pos] = compare2;
        table_replace[pos] = replace2;
        pos++;
    }

}

void build_sys8_path_table()
{

    path_open_entry *pentries;

    int entries = 0;

    int arena_size = 0;

    int n, m;

    sys8_path_table(0LL);

    if(ntable <= 0 && !table_compare[17] && !table_compare[18]) return;
    if(ntable <= 0) {table_compare[0] = 0; ntable = 0;}

    while(ntable > 0 && table_compare[entries] != NULL) entries++;

    // /dev_bdvd & /app_home entries
    if(table_compare[17])
    {
        table_compare[entries] = table_compare[17];
        table_replace[entries] = table_replace[17];
        entries++;
    }

    if(table_compare[18])
    {
        table_compare[entries] = table_compare[18];
        table_replace[entries] = table_replace[18];
        entries++;
    }

    table_compare[entries] = NULL;

    entries = 0;

    while(table_compare[entries] != NULL)
    {
        int l = strlen(table_compare[entries]);

        arena_size += MAX_PATH_LEN;
        for(m = 0x80; m <= MAX_PATH_LEN; m += 0x20)
            if(l < m) {arena_size += m; break;}

        entries++;
    }


    if(!entries) return;

    char * datas = memalign(16, arena_size + sizeof(path_open_entry) * (entries + 2));

    if(!datas) return;

    u64 dest_table_addr = 0x80000000007FE000ULL - (u64)((arena_size + sizeof(path_open_entry) * (entries + 1) + 15) & ~15);

    u32 arena_offset = (sizeof(path_open_entry) * (entries + 1));

    pentries = (path_open_entry *) datas;

    for(n = 0; n < entries; n++)
    {
        int l = strlen(table_compare[n]);

        int size = 0;
        for(m = 0x80; m <= MAX_PATH_LEN; m += 0x20)
            if(l < m) {size += m; break;}

        pentries->compare_addr = dest_table_addr + (u64) (arena_offset);

        pentries->replace_addr = dest_table_addr + (u64) (arena_offset + size);

        strncpy(&datas[arena_offset], table_compare[n], size);
        strncpy(&datas[arena_offset + size], table_replace[n], MAX_PATH_LEN);

        pentries->compare_len = strlen(&datas[arena_offset]);
        pentries->replace_len = strlen(&datas[arena_offset + size]);

        arena_offset += size + MAX_PATH_LEN;
        pentries ++;
    }

    pentries->compare_addr = 0ULL;

    sys8_memcpy(dest_table_addr, (u64) datas, (u64) (arena_size + sizeof(path_open_entry) * (entries + 1)));

    free(datas);

    reset_sys8_path_table();

    // set the path table
    sys8_path_table( dest_table_addr);
}

void copy_usb_to_iris(char * path)
{
    int n;
    char name[MAX_PATH_LEN];

    copy_split_to_cache = 0;
    copy_total_size = 0LL;
    file_counter = 0;
    abort_copy = 0;

    for(n = 1; n < 11; n++)
    {
         if((fdevices >> n) & 1) break;
    }

    if(n == 11) return;

    n+= 47;
    sprintf(name, "/dev_usb00%c/iris", n);

    DCls();
    my_game_countsize(name);

    u32 blockSize;
    u64 freeSize;
    u64 free_hdd0;
    sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
    free_hdd0 = ( ((u64)blockSize * freeSize));

    if((copy_total_size + 0x40000000LL) >= (s64) free_hdd0)
    {
        sprintf(filename, language[GAMECHCPY_NOSPACE], free_hdd0, ((double) (copy_total_size + 0x40000000LL - free_hdd0)) / GIGABYTES);
        DrawDialogOK(filename);
        return;
    }

    if(abort_copy) //abort by user or got an error
    {
        if(DrawDialogYesNo(language[GAMECPYSL_GSIZEABCNTASK]) != 1)
        {
            return;
        }
        else
        {
            //old mode
            copy_total_size = 0;
        }
    }

    dialog_action = 0;
    abort_copy = 0;

    sprintf(filename, "/dev_usb/iris %s Iris Manager?\nVol: %1.2f MB",
                        language[GLUTIL_WTO],
                        ((double) copy_total_size)/(1024.0*1024));

    dialog_action = 0;

    msgDialogOpen2( mdialogyesno, filename, my_dialog, (void*)0x0000aaaa, NULL );

    wait_dialog();

    if(dialog_action == 1)
    {
        time_start = time(NULL);

        abort_copy = 0;
        DCls();
        file_counter = 0;
        new_pad = 0;

        DPrintf("%s %s\n %s %s\n\n", language[GAMECPYSL_STARTED], "USB", language[GLUTIL_WTO], "Iris Manager");

        copy_mode = 0;

        copy_is_split = 0;

        // sys8_perm_mode(1);
        my_game_copy((char *) name, (char *) path);
        // sys8_perm_mode(0);

        cls2();

        int seconds = (int) (time(NULL) - time_start);
        int vflip = 0;

        while(true)
        {
            if(abort_copy)
                sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i\n", language[GLUTIL_ABORTED], language[GLUTIL_TIME],
                        seconds / 3600, (seconds/60) % 60, seconds % 60);
            else
                sprintf(string1,"%s  %s: %2.2i:%2.2i:%2.2i Vol: %1.2f GB\n", language[GAMECPYSL_DONE], language[GLUTIL_TIME],
                        seconds / 3600, (seconds/60) % 60, seconds % 60, ((double) global_device_bytes) / GIGABYTES);


            cls2();

            DbgHeader( string1);

            if(vflip & 32)
                DbgMess(language[GLUTIL_XEXIT]);
            else
                DbgMess("");

            vflip++;

            DbgDraw();

            tiny3d_Flip();

            if(ps3pad_poll())
            {
               new_pad = 0;
               break;
            }
        }
    }
}

u64 string_to_ull( char *string )
{
    u64 ull;
    ull = strtoull( (const char *)string, NULL, 16 );
    return ull;
}

#define SYSCALL8_OPCODE_PS3MAPI			 			0x7777
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO			0x0047
#define PS3MAPI_OPCODE_GET_CORE_MINVERSION			0x0012

LV2_SYSCALL ps3mapi_get_core_minversion(void)
{
    lv2syscall2(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION);
    return_to_user_prog(s32);
}

static void sys8_get_plugin_slot(unsigned int slot, char *tmp_name, char *tmp_filename)
{
    memset(tmp_name, 0, sizeof(tmp_name));
    memset(tmp_filename, 0, sizeof(tmp_filename));

    lv2syscall5(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)tmp_name, (u64)tmp_filename);
}

unsigned int get_vsh_plugin_slot_by_name(const char *name)
{
    char tmp_name[30];
    char tmp_filename[256];
    unsigned int slot;

    bool find_free_slot = (!name || (*name == 0));

    for (slot = 1; slot < 7; slot++)
    {
        sys8_get_plugin_slot(slot, tmp_name, tmp_filename);

        if(find_free_slot) {if(*tmp_name) continue; return slot;}

        if(!strcmp(tmp_name, name) || strstr(tmp_filename, name)) return slot;
    }

    return 0;
}

unsigned int get_vsh_plugin_free_slot(void)
{
    if(ps3mapi_get_core_minversion() == 0) return 6;

    char tmp_name[30];
    char tmp_filename[256];
    int slot;

    for (slot = 1; slot < 7; slot++)
    {
        memset(tmp_name, 0, sizeof(tmp_name));
        memset(tmp_filename, 0, sizeof(tmp_filename));
        lv2syscall5(SYSCALL_MAMBA, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)tmp_name, (u64)tmp_filename);
        if(strlen(tmp_filename) == 0 && strlen(tmp_name) == 0) {return slot;}
    }

    return FAILED;
}

void urldec(char *url)
{
	if(strchr(url, '%'))
	{
		u16 pos = 0; char c;
		for(u16 i = 0; url[i] >= ' '; i++, pos++)
		{
			if(url[i] == '+')
				url[pos] = ' ';
			else if(url[i] != '%')
				url[pos] = url[i];
			else
			{
				url[pos] = 0; u8 n = 2;
				while(n--)
				{
					url[pos] <<= 4, i++, c = (url[i] | 0x20);
					if(c >= '0' && c <= '9') url[pos] += c -'0';      else
					if(c >= 'a' && c <= 'f') url[pos] += c -'a' + 10;
				}
			}
		}
		url[pos] = 0;
	}
}
