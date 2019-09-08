#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "ntfs.h"

int file_manager(char *pathw1, char *pathw2);

extern const DISC_INTERFACE *disc_ntfs[8];

// mounts from /dev_usb000 to 007
extern ntfs_md *mounts[8];
extern int mountCount[8];
extern int automountCount[8];

extern u32 ports_cnt;
extern u32 old_ports_cnt;

#define TEXTNOTFOUND -1LL

enum FindModes
{
    FIND_HEX_MODE = 0,
    FIND_TEXT_MODE = 1,
    FIND_CASE_INSENSITIVE_MODE = 2,
};

enum FileIconTypes
{
    FILE_TYPE_FOLDER = 0,
    FILE_TYPE_NORMAL = 1,
    FILE_TYPE_PKG = 2,
    FILE_TYPE_SELF = 3,
    FILE_TYPE_ZIP = 3,
    FILE_TYPE_PNG = 4,
    FILE_TYPE_JPG = 44,
    FILE_TYPE_ISO = 5,
    FILE_TYPE_LUA = 5,
    FILE_TYPE_BIN = 55,
};

enum DirectoryEntryTypes
{
    IS_FILE = 0,
    IS_DIRECTORY = 1,
    IS_MARKED = 2,
    IS_NOT_AVAILABLE = 128,
};

u64 get_free_space(char * path, bool usecache);

int NTFS_Event_Mount(int id);
int NTFS_UnMount(int id);
int NTFS_UnMount_dev(int id, char * name);
void NTFS_UnMountAll(void);
int NTFS_Test_Device(char *name);

void install_pkg(char *path, char *filename, u8 show_done);

bool is_retro_file(char *rom_path, char *rom_file);
void launch_retro(char *rom_path);
void launch_showtime(bool playmode);
void launch_video(char *filename);
void launch_ps2classic(char *ps2iso_path, char *ps2iso_title);

#define DETECT_EMU_TYPE -1

int launch_iso_game(char *path, int mtype);
int launch_iso_game_mamba(char *path, int mtype);
int launch_iso_build(char *iso_path, char *src_path, bool run_showtime);

int copy_archive_file(char *path1, char *path2, char *file, u64 free);
int CopyFile(char* path, char* path2);
int CopyDirectory(char* path, char* path2, char* path3); // path3 = path2 (used to avoid infinit recursivity)

void draw_file_manager();
void draw_hex_editor();

#endif
