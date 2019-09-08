#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <malloc.h>
#include <math.h>
#include "pad.h"

#include <sys/process.h>
#include <ppu-lv2.h>
#include <sys/stat.h>
#include <lv2/sysfs.h>

// for msgdialogs
#include <sysutil/sysutil.h>
#include <sysutil/msg.h>

#include <tiny3d.h>
#include <libfont.h>

#include "syscall8.h"

#define FONT_TTF -1
#define FONT_BUTTON  0

#define DT_DIR 1

typedef struct {
    u32 flags;
    int splitted;
    char path_name[MAXPATHLEN];
    char title[64];
    char title_id[64];

} t_directories;

typedef struct {
    int index;
    u32 flags;
    char title[64];
    char title_id[64];
} entry_favourites;

#define MAX_FAVORITES 48

typedef struct {
    u32 version;
    entry_favourites list[12];
} tfavourites;

typedef struct {
    u32 version;
    entry_favourites list[MAX_FAVORITES];
} tfavourites2;

#define MAX_DIRECTORIES 3000
#define MAX_CFGLINE_LEN 256

#define GAMELIST_FILTER 0xFFFF

#define GAMEBASE_MODE 0
#define HOMEBREW_MODE 2
#define VIDEOS_MODE   3

#define FS_S_IFMT 0170000
#define FS_S_IFDIR 0040000

#define S_ISDIR(m)	(((m)&_IFMT) == _IFDIR)

//#define PSDEBUG 1

#define SC_SYS_POWER 					(379)
#define SYS_SOFT_REBOOT 				0x200
#define SYS_HARD_REBOOT					0x1200
#define SYS_REBOOT						0x8201
#define SYS_SHUTDOWN					0x1100

extern int ndirectories;
extern t_directories directories[MAX_DIRECTORIES];

extern u32 fdevices;
extern u32 fdevices_old;
extern u32 forcedevices;

extern int noBDVD;

extern bool use_cobra;
extern bool use_mamba;
extern bool is_mamba_v3;

extern char hdd_folder[64];
extern char bluray_game[64];
extern float cache_need_free;

void draw_cache_external();

void cls();
void cls2();

char * LoadFile(char *path, int *file_size);
int SaveFile(char *path, char *mem, int file_size);
int ExtractFileFromISO(char *iso_file, char *file, char *outfile);

void DrawDialogOK(char * str);
void DrawDialogOKTimer(char * str, float milliseconds);
void DrawDialogTimer(char * str, float milliseconds);

int DrawDialogYesNo(char * str);
int DrawDialogYesNo2(char * str);
int DrawDialogYesNoDefaultYes(char * str);
int DrawDialogYesNoTimer(char * str, float milliseconds);
int DrawDialogYesNoTimer2(char * str, float milliseconds);


int parse_param_sfo(char * file, char *title_name);
int parse_ps3_disc(char *path, char * id);
int parse_param_sfo_id(char * file, char *title_id);
int parse_param_sfo_appver(char * file, char *app_ver);
int parse_iso_titleid(char * path_iso, char * title_id);
int mem_parse_param_sfo(u8 *mem, u32 len, char *field, char *title_name);
void utf8_to_ansi(char *utf8, char *ansi, int len);
void utf8_truncate(char *utf8, char *utf8_trunc, int len);

void sort_entries(t_directories *list, int *max);
void sort_entries2(t_directories *list, int *max, u32 mode);

void add_custom_icons(t_directories *list, int *max);
int delete_custom_icons(t_directories *list, int *max);

int delete_entries(t_directories *list, int *max, u32 flag);
int fill_entries_from_device(char *path, t_directories *list, int *max, u32 flag, int sel, bool append);
//int fill_iso_entries_from_device(char *path, u32 flag, t_directories *list, int *max);
int fill_iso_entries_from_device(char *path, u32 flag, t_directories *list, int *max, unsigned long ioType);
void fill_directory_entries_with_alt_path(char *file, int n, char *retro_path, char *alt_path, t_directories *list, int *max, u32 flag);
void fill_psx_iso_entries_from_device(char *path, u32 flag, t_directories *list, int *max);

void copy_from_selection(int game_sel);
void copy_from_bluray();
void delete_game(int game_sel);
void test_game(int game_sel);

void copy_to_cache(int game_sel, char * hmanager_path);
void copy_usb_to_iris(char * path);

void DeleteDirectory(const char* path);

int FixDirectory(const char* path, int fcount);

extern tfavourites2 favourites;
extern int havefavourites;

int getConfigMemValueInt (char* mem, int size, char* pchSection, char* pchKey, int iDefaultValue);
int getConfigMemValueString (char* mem, int size, char* pchSection, char* pchKey, char* pchValue, int iSize, char* pchDefaultValue);

void LoadFavourites(char * path, int mode);
void SaveFavourites(char * path, int mode);

void GetFavourites(int mode);
void SetFavourites(int mode);

void UpdateFavourites(t_directories *list, int nlist);
int TestFavouritesExits(char *id);
void AddFavourites(int indx, t_directories *list, int position_list);
void DeleteFavouritesIfExits(char *id);

int param_sfo_util(char * path, int patch_app);
int param_sfo_patch_category_to_cb(char * path_src, char *path_dst);

void reset_sys8_path_table();
void add_sys8_path_table(char * compare, char * replace);
void build_sys8_path_table();
void add_sys8_bdvd(char * bdvd, char * app_home);

bool is_ntfs_path(char *path);
bool is_video(char *ext);
bool is_audio(char *ext);
bool is_audiovideo(char *ext);
bool is_browser_file(char *ext);

void filepath_check(char *file);

u64 get_filesize(char *path);
bool file_exists( char* path );
char * get_extension(char *path);
char * get_filename(char *path);
int strcmpext(char *path, char *ext);
char *str_replace(char *orig, char *rep, char *with);

// console

extern int con_x;
extern int con_y;

void DCls();
void DbgHeader(char *str);
void DbgMess(char *str);
void DbgDraw();
void DPrintf(char *format, ...);

int sys_shutdown();
int sys_reboot();
int sys_soft_reboot();

int unlink_secure(void *path);
int rename_secure(void *path1, void *path2);
int mkdir_secure(void *path);
int rmdir_secure(void *path);

int patch_exe_error_09(char *path_exe);
void patch_error_09( const char *path, int quick_ver_check );
int fix_PS3_EXTRA_attribute(char *path);

int game_update(char *title_id);
int cover_update(char *title_id);
int covers_update(int pass);
int download_file(char *url, char *file, int mode, u64 *size);
void call_webman(const char *cmd);

void urldec(char *url);
u64 string_to_ull( char *string );

unsigned int get_vsh_plugin_slot_by_name(const char *name);
unsigned int get_vsh_plugin_free_slot(void);

int edit_title_param_sfo(char * file);
#endif
