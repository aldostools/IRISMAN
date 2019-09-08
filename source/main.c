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

#include "loader.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <lv2/process.h>
#include <sys/file.h>
#include <ppu-lv2.h>
#include <sys/stat.h>
#include <lv2/sysfs.h>

#include <sysutil/disc.h>

#include <sysmodule/sysmodule.h>
#include <pngdec/pngdec.h>
#include <jpgdec/jpgdec.h>

#include <io/pad.h>

u32 snd_inited = 0;

#define INITED_CALLBACK     1
#define INITED_SPU          2
#define INITED_SOUNDLIB     4
#define INITED_AUDIOPLAYER  8

#include <sysutil/sysutil.h>
#include <spu_soundlib.h>
#include <audioplayer.h>

#include <tiny3d.h>
#include "libfont2.h"
#include "language.h"
#include "syscall8.h"
#include "payload.h"
#include "payload341/payload_341.h"
#include "payload355/payload_355.h"
#include "payload355dex/payload_355dex.h"
#include "payload355deh/payload_355deh.h"
#include "payload421/payload_421.h"
#include "payload421dex/payload_421dex.h"
#include "payload430/payload_430.h"
#include "payload431/payload_431.h"
#include "payload430dex/payload_430dex.h"
#include "payload440/payload_440.h"
#include "payload441/payload_441.h"
#include "payload441dex/payload_441dex.h"
#include "payload446/payload_446.h"
#include "payload446dex/payload_446dex.h"
#include "payload450/payload_450.h"
#include "payload450dex/payload_450dex.h"
#include "payload453/payload_453.h"
#include "payload453dex/payload_453dex.h"
#include "payload455/payload_455.h"
#include "payload455dex/payload_455dex.h"
#include "payload460/payload_460.h"
#include "payload460dex/payload_460dex.h"
#include "payload460deh/payload_460deh.h"
#include "payload465/payload_465.h"
#include "payload465dex/payload_465dex.h"
#include "payload470/payload_470.h"
#include "payload470dex/payload_470dex.h"
#include "payload475/payload_475.h"
#include "payload475dex/payload_475dex.h"
#include "payload475deh/payload_475deh.h"
#include "payload480/payload_480.h"
#include "payload480dex/payload_480dex.h"
#include "payload480deh/payload_480deh.h"
#include "payload481dex/payload_481dex.h"

#include "spu_soundmodule.bin.h" // load SPU Module
#include "spu_soundlib.h"

#include <gcmodplay.h>
#include "main.h"

#include "ttf_render.h"
#include "gfx.h"
#include "utils.h"
#include "pad.h"

#include "ftp/ftp.h"
#include "ftp/functions.h"

#include "psx.h"
#include "file_manager.h"
#include "controlfan.h"
#include "sysregistry.h"
#include "osk_input.h"
#include "modules.h"

// include fonts

// font 2: 224 chr from 32 to 255, 16 x 32 pix 2 bit depth
#include "font_b.h"

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
#else
//
#include "credits.h"

#include "background_jpg_bin.h"
#include "bluray_png_bin.h"
#include "cricket_raw_bin.h"
#include "direct_png_bin.h"
#include "dvd_png_bin.h"
#include "file_png_bin.h"
#include "film_png_bin.h"
#include "folder_png_bin.h"
#include "ftp_png_bin.h"
#include "img_png_bin.h"
#include "iso_png_bin.h"
#include "missing_png_bin.h"
#include "nethost_png_bin.h"
#include "psone_png_bin.h"
#include "psoneiso_png_bin.h"
#include "pstwoiso_png_bin.h"
#include "pspiso_png_bin.h"
#include "pkg_png_bin.h"
#include "retro_png_bin.h"
#include "self_png_bin.h"
#include "usb_png_bin.h"
#include "usb_png2_bin.h"
//
#endif

#include "ps3_discless.h"

#include "event_threads.h"
#include "cobra.h"
#include "iso.h"

#include "console_id.h"

#define PAYLOAD_PS3HEN    7 /* ps3hen payload */

bool use_cobra = false;
bool use_mamba = false; // cobra app version
bool is_mamba_v2 = false;
bool unsupported_cfw = false;

bool install_mamba = true;

bool signal_ntfs_mount = false;
bool force_ntfs_mount  = false;

#define MAX_NTFS_RETRY 20
u8 ntfs_retry = MAX_NTFS_RETRY; // max number of tries to read the ntfs folders if there are 0 entries in the game list

u64 restore_syscall8[2] = {0,0};

#define NETHOST "<NETHOST>"

#define LIST_ALL_DEVICES   -1
#define RESET_GAME_INFO    -1

#define SYSTEM_LANGUAGE    99

#define GUI_MODES  15

enum GuiModes {
  MODE_XMB_LIKE  =  0,
  MODE_COVERFLOW =  1,
  MODE_GRID_3x2  =  2,
  MODE_GRID_4x2  =  3,
  MODE_GRID_3x3  =  4,
  MODE_GRID_4x3  =  5,
  MODE_GRID_5x3  =  6,
  MODE_GRID_6x3  =  7,
  MODE_GRID_4x4  =  8,
  MODE_GRID_5x4  =  9,
  MODE_GRID_6x4  = 10,
  MODE_GRID_5x5  = 11,
  MODE_GRID_6x5  = 12,
  MODE_GRID_8x5  = 13,
  MODE_GRID_7x6  = 14,
  MODE_GRID_8x6  = 15,
};

static int gui_mode = MODE_COVERFLOW; // GUI selector
static int cover_mode = 0;
static int sort_mode = 0;

u16  BUTTON_CROSS_  = BUTTON_CROSS;  //0x0040
u16  BUTTON_CIRCLE_ = BUTTON_CIRCLE; //0x0020

static int net_option = 0;
static int filter_by_device = LIST_ALL_DEVICES, opt_filter_by_device = LIST_ALL_DEVICES;
int filter_by_letter = 0;

bool refresh_game_list = false;
bool bAllowNetGames = false; // enabled only if webMAN is installed
bool bXMLScanOnlyNetGames = true;
bool bSkipParseXML = false;
bool bdvd_is_usb = false;
bool ftp_inited = false;
bool refresh_gui = false;
bool allow_save_lastgame = false;

static bool cached_game_list = false; //only once
static bool found_game_insert = false;
static bool found_game_remove = false;
static bool do_once = true;
bool show_http_errors = false;

// grid config for gui1
int cols = 4, rows = 3;
int num_box = 12; //cols * rows;

enum background_types {
    BG_NONE   = 0,
    BG_RANDOM = 1,
    BG_PIC1   = 2,
};

int bk_picture = BG_NONE;

static const float box_left = (848 - 520) / 2;
static const float box_width = (200 * 4) - 8;
static const float box_height = (150 * 3) - 8;

bool options_locked = false; // true when control parental is between 1 and 9, false for parental 0 (disabled)

bool is_ps3hen = false;

int noBDVD = MODE_DISCLESS;

static bool save_game_list_after_refresh = false;
static bool is_ps3game_running = false;

int game_list_category = GAME_LIST_ALL;

int mode_homebrew = GAMEBASE_MODE;
int homelaun = 0;

#define LAUNCHMODE_TOCHECK -1
#define LAUNCHMODE_CHECKED -2
#define LAUNCHMODE_REFRESH -3
#define LAUNCHMODE_STARTED 0
static int autolaunch = LAUNCHMODE_TOCHECK;

#ifndef LOADER_MODE
static int psx_inserted = 0;
#endif

u64 syscall_base = 0ULL;

int (*lv2_unpatch_bdvdemu)(void) = NULL;
int (*lv2_patch_bdvdemu)(uint32_t flags) = NULL;
int (*lv2_patch_storage)(void) = NULL;
int (*lv2_unpatch_storage)(void) = NULL;


//Console id
u64 off_idps = 0;
u64 off_idps2 = 0;
u64 val_idps_part1 = 0;
u64 val_idps_part2 = 0;

u64 off_psid = 0;
u64 val_psid_part1 = 0;
u64 val_psid_part2 = 0;

char psid[33];
char console_id[33];
char default_psid[33];
char default_console_id[33];

void UTF8_to_Ansi(char *utf8, char *ansi, int len); // from osk_input

u16 * ttf_texture;
bool update_title_utf8 = true;
u8 string_title_utf8[128] = "";
int width_title_utf8 = 0;

void draw_gamelist_options(float x, float y);

int get_last_selected_game(bool launch);
void get_grid_dimensions(bool update_coords);
void enable_draw_background_pic1();
void draw_background_pic1();
void load_background_picture();

void mount_iso_game();
void test_audio_file();

bool scan_canceled = false;

// music
static int MAX_SONGS = 0;

int song_selected = 0;

// SPU
u32 spu = 0;
sysSpuImage spu_image;
#define SPU_SIZE(x) (((x)+127) & ~127)

MODPlay mod_track;

u64 frame_count = 0;

#define ROT_INC(x , y , z) {x++; if(x > y) x = z;}
#define ROT_DEC(x , y , z) {if(x > y) x--; else x = z;}

int menu_screen = SCR_MAIN_GAME_LIST;
int mode_favourites = 1;

t_directories directories[MAX_DIRECTORIES];

int ndirectories = 0;

int currentdir = 0;
int currentgamedir = 0;
int int_currentdir = 0;

void unpatch_bdvdemu();
int patch_bdvdemu(u32 flags);
int move_origin_to_bdemubackup(char *path);
int move_bdemubackup_to_origin(u32 flags);

u8 * png_texture = NULL;

#define GET_ICON_FROM_ISO -666
#define ICON_LOAD_SUCCESS -6666

PngDatas Png_datas[MAX_PICTURES];
u32 Png_offset[MAX_PICTURES];
int Png_iscover[MAX_PICTURES];
int Png_index[MAX_PICTURES];

PngDatas Png_res[MAX_RESOURCES];
u32 Png_res_offset[MAX_RESOURCES];

char self_path[MAXPATHLEN] = "/"__MKDEF_MANAGER_FULLDIR__;

int firmware = 0;
//int //fw_ver = 0;

#define LASTGAME_PATH "/dev_hdd0/game/BLES80610/"
#define LASTGAME_PATH_SS "/dev_hdd0/game/NPEA00374/"

#define PATHS_SECTION "Paths"

char backgrounds_path[MAXPATHLEN];
char covers_path[MAXPATHLEN];
char retro_covers_path[MAXPATHLEN];
char updates_path[MAXPATHLEN];
char video_path[MAXPATHLEN];
char webman_path[MAXPATHLEN];
char psp_launcher_path[MAXPATHLEN];
char retroarch_path[MAXPATHLEN];
char ps2classic_path[MAXPATHLEN];
char psxiso_path[MAXPATHLEN];

#define EXTENSIONS_SECTION "Extensions"

char video_extensions[300];
char audio_extensions[300];
char rom_extensions[300];
char browser_extensions[100];
char custom_homebrews[400];

#define ROMS_SECTION "RetroPaths"

int  retro_mode = RETRO_ALL;

int  roms_count = 0;
int  max_roms = 80;

// retroArch cores 1.0
char retro_root_path[ROMS_MAXPATHLEN];
char retro_snes_path[ROMS_MAXPATHLEN];
char retro_gba_path[ROMS_MAXPATHLEN];
char retro_gen_path[ROMS_MAXPATHLEN];
char retro_nes_path[ROMS_MAXPATHLEN];
char retro_mame_path[ROMS_MAXPATHLEN];
char retro_fba_path[ROMS_MAXPATHLEN];
char retro_doom_path[ROMS_MAXPATHLEN];
char retro_quake_path[ROMS_MAXPATHLEN];
char retro_pce_path[ROMS_MAXPATHLEN];
char retro_gb_path[ROMS_MAXPATHLEN];
char retro_gbc_path[ROMS_MAXPATHLEN];
char retro_atari_path[ROMS_MAXPATHLEN];
char retro_vb_path[ROMS_MAXPATHLEN];
char retro_nxe_path[ROMS_MAXPATHLEN];
char retro_wswan_path[ROMS_MAXPATHLEN];

// retroArch cores 1.2
char retro_a7800_path[ROMS_MAXPATHLEN];
char retro_lynx_path[ROMS_MAXPATHLEN];
char retro_gw_path[ROMS_MAXPATHLEN];
char retro_vectrex_path[ROMS_MAXPATHLEN];
char retro_2048_path[ROMS_MAXPATHLEN];

#define GUI_SECTION "GUI"

bool bAddToLastGameApp = true;
bool bShowPlayOverlay = true;
bool bShowUSBIcon = true;
bool bBackgroundGears = true;
bool bIconPulse = true;
bool bFileManager = false;
bool bUnmountDevBlind = true;
bool bCachedGameList = true;
bool bLoadPIC1 = false;
bool bSkipPIC1 = false;
bool bSpoofVersion = false;
bool bHideCoverflowSortModeLabel = false;
bool bSkipBDMirrorMsg = true;
char passwd[64];

bool bLoadMambaAndQuit = false;

u8 bEnableLv2_memprot_patch  = 0;
u8 bEnableLv2_webman_patch   = 1;
u8 bEnableLv2_habib_patch    = 2;

u8 ftp_serv = 0;

u32 spoof_version  = 0;
u32 spoof_revision = 0;

u8 bShowPIC1 = 0;
u8 bShowVersion = 2;
u8 iTimeFormat = 1;
u16 iTimeoutByInactivity = 1;
u16 CoverflowSize = 10;
u8 mod_volume = 6;

bool bShowPath = false;
u32 default_psxoptions = 1;

static u32 last_game_flag = 0;
static char last_game_id[64];
static int last_game_favourites = 0;
static bool allow_restore_last_game = true;

inline int get_currentdir(int i);
inline int get_int_currentdir(int i);

char temp_buffer[8192];

//void MSGBOX(char *text, char *text2) {sprintf(temp_buffer, "%s = %s", text, text2); DrawDialogOKTimer(temp_buffer, 5000.0f);}    //debug message
//void MSGBOX2(char *text, int i) {sprintf(temp_buffer, "%s = %i", text, i); DrawDialogOKTimer(temp_buffer, 5000.0f);}    //debug message

//int load_boot_prx(char * config_path);

LV2_SYSCALL wm_sys_storage_ext_umount_discfile(void)
{
    lv2syscall1(SYSCALL_MAMBA, SYSCALL8_OPCODE_UMOUNT_DISCFILE);
    return_to_user_prog(s32);
}

static void do_umount_iso(void)
{
    unsigned int real_disctype, effective_disctype, iso_disctype;

    cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);

    // If there is an effective disc in the system, it must be ejected
    if(effective_disctype != DISC_TYPE_NONE)
    {
        cobra_send_fake_disc_eject_event();
        usleep(4000);
    }

    if(iso_disctype != DISC_TYPE_NONE) cobra_umount_disc_image();

    // If there is a real disc in the system, issue an insert event
    if(real_disctype != DISC_TYPE_NONE)
    {
        cobra_send_fake_disc_insert_event();
        for(u8 m=0; m<22; m++)
        {
            usleep(4000);

            if(file_exists("/dev_bdvd")) break;
        }
        cobra_disc_auth();
    }
}

LV2_SYSCALL sys_storage_ext_fake_storage_event(uint64_t event, uint64_t param, uint64_t device)
{
    lv2syscall4(SYSCALL_MAMBA, SYSCALL8_OPCODE_FAKE_STORAGE_EVENT, event, param, device);
    return_to_user_prog(s32);
}

#include "main_icons.h"

int show_custom_icons = 1;

int background_sel = 1;
int background_fx = 0;

#define MAX_COLORS  12

const u32 BLACK = 0x00000028;
const u32 WHITE = 0xffffffff;

u32 background_colors[MAX_COLORS] = {
    0xff000000, //Animated Background
    0xff000010, //Dark blue
    0xff000033, //Navy
    0xff0040cf, //Cobalt
    0xff6699ff, //Sky blue
    0x10e8a010, //Orange
    0xff320200, //Dark red
    0xff220345, //Purple
    0xff734367, //Pink
    0xff0b4143, //Cyan
    0xff556b2f, //Olive
    0xff404040, //Gray
};

u32 background_colors2[24] = {
    0x000000FF, //Animated Background
    0x000000FF,

    0x040404ff, //BLACK
    0x030303ff,

    0x000060FF, //NAVY
    0x000000FF,

    0x2040B8ff, //BLUE
    0x102048ff,

    0x4040ffff, //BRIGHT BLUE
    0x0040a8ff,

    0xf8b010ff, //ORANGE
    0x481008ff,

    0x400000ff, //RED
    0x200000ff,

    0x624385ff, //PURPLE
    0x220345ff,

    0x784078ff, //PLUM
    0x281028ff,

    0x008080ff, //CYAN
    0x001010FF,

    0x206F00ff, //GREEN
    0x002000ff,

    0x808080ff, //GRAY
    0x000000FF,
};

void cls0()
{
    cls2();

    if(background_sel == 0) { GFX1_background(); return; }

    if(Png_offset[BACKGROUND_PICT])
    {
        if (Png_datas[BACKGROUND_PICT].wpitch > 0 && Png_datas[BACKGROUND_PICT].width > 0 && Png_datas[BACKGROUND_PICT].height > 0)
        {
            tiny3d_SetTextureWrap(0, Png_offset[BACKGROUND_PICT], Png_datas[BACKGROUND_PICT].width,
                Png_datas[BACKGROUND_PICT].height, Png_datas[BACKGROUND_PICT].wpitch,
                TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP, 1);

            DrawTextBox(-1, -1, 1000, 850, 514, (background_sel & 1) ? WHITE : 0xcfcfcfff);
            return;
        }
    }

    u32 color1; //0x002088ff;
    u32 color2;

    color1 = background_colors2[(background_sel & 0xF) << 1];
    color2 = background_colors2[((background_sel & 0xF) << 1) + 1];

    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(-80, -80, 1000);
    tiny3d_VertexColor(color1);

    tiny3d_VertexPos(938, -80, 1000);
    tiny3d_VertexColor(color1);

    tiny3d_VertexPos(938, 400, 1000);
    tiny3d_VertexColor(color2);

    tiny3d_VertexPos(-80, 400, 1000);
    tiny3d_VertexColor(color2);

    tiny3d_VertexPos(-80, 400, 1000);
    tiny3d_VertexColor(color2);

    tiny3d_VertexPos(938, 400, 1000);
    tiny3d_VertexColor(color2);

    tiny3d_VertexPos(938, 592, 1000);
    tiny3d_VertexColor(0x080008ff);

    tiny3d_VertexPos(-80, 592, 1000);
    tiny3d_VertexColor(0x080008ff);

    tiny3d_End();
}

void cls()
{
    if(gui_mode == MODE_COVERFLOW || gui_mode == MODE_XMB_LIKE)  {cls0(); return;}

    if(background_sel == 0) {cls2(); GFX1_background(); return;}

    draw_background_pic1();

    tiny3d_Clear(background_colors[background_sel], TINY3D_CLEAR_ALL);

    // Enable alpha Test
    tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

    // Enable alpha blending.
            tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
                TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
                TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
    reset_ttf_frame();
}

void cls2()
{
    draw_background_pic1();

    tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);

    // Enable alpha Test
    tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

   // Enable alpha blending.
            tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
                TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
                TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
    reset_ttf_frame();
}

/******************************************************************************************************************************************************/
/* Payload functions                                                                                                                                  */
/******************************************************************************************************************************************************/
#ifndef LOADER_MODE
static int sys_ss_media_id(void * id)
{
    lv2syscall2(879, 0x10001ULL, (u64) id);
    return_to_user_prog(int);
}
#endif

u64 syscall_40(u64 cmd, u64 arg)
{
    lv2syscall2(40, cmd, arg);
    return_to_user_prog(u64);
}


int sys_set_leds(u64 color, u64 state)
{
    lv2syscall2(386,  (u64) color, (u64) state);
    return_to_user_prog(int);
}

int sys_game_get_temperature(int sel, u32 *temperature)
{
    u32 temp;

    lv2syscall2(383, (u64) sel, (u64) &temp);
    *temperature = (temp >> 24);
    return_to_user_prog(int);
}

int sys_fs_mount(char const* deviceName, char const* deviceFileSystem, char const* devicePath, int writeProt)
{
    lv2syscall8(837, (u64) deviceName, (u64) deviceFileSystem, (u64) devicePath, 0, (u64) writeProt, 0, 0, 0 );
    return_to_user_prog(int);
}

int sys_fs_umount(char const* devicePath)
{
    lv2syscall3(838,  (u64) devicePath, 0, 0 );
    return_to_user_prog(int);
}

u64 lv1peek(u64 addr)
{
    lv2syscall1(8, (u64) addr);
    return_to_user_prog(u64);
}

u64 lv2peek(u64 addr)
{
    lv2syscall1(6, (u64) addr);
    return_to_user_prog(u64);
}

u64 lv2poke(u64 addr, u64 value)
{
    lv2syscall2(7, (u64) addr, (u64) value);
    return_to_user_prog(u64);
}


int lv2launch(u64 addr)
{
    lv2syscall8(9, (u64) addr, 0,0,0,0,0,0,0);
    return_to_user_prog(int);
}

/*
int syscall36(char * path)
{
    lv2syscall1(36, (u64) path);
    return_to_user_prog(int);

}
*/

u64 hmanager_key = 0x1759829723742374ULL;

/******************************************************************************************************************************************************/

// manager


char payload_str[256];

int videoscale_x = 0;
int videoscale_y = 0;

int flash;

int select_px = 0;
int select_py = 0;

u32 blockSize;
u64 freeSize;
float freeSpace[12];

int select_option = 0;

u32 fdevices = 0;
u32 fdevices_old = 0;
u32 forcedevices = 0;
int find_device = 0;

char hdd_folder[64]="12345";

char bluray_game[64]; // name of the game

static bool exit_program = false;

#define ROUND_UPX(x)  ((((x) + num_box - 1) / num_box) * num_box)

void draw_grid(float x, float y);
void draw_coverflow(float x, float y);

int gui_control();

void draw_options(float x, float y, int index);
void draw_iso_options(float x, float y, int index);
void draw_configs(float x, float y, int index);
void draw_gbloptions(float x, float y);
void draw_toolsoptions(float x, float y);
void draw_cachesel(float x, float y);
void draw_pkginstall(float x, float y);
void draw_device_mkiso(float x, float y, int index, char *path);
void draw_device_xtiso(float x, float y, int index);
void draw_device_cpyiso(float x, float y, int index);
void draw_app_version(float x, float y);

char app_ver[8];

struct {
    int videoscale_x[4];
    int videoscale_y[4];
    int background_sel;
    char hdd_folder[64];
    u32 usekey;
    char pad[156];
} manager_oldcfg;

struct {
    int videoscale_x[4];
    int videoscale_y[4];
    int background_sel;
    char hdd_folder[64];
    u32 usekey;
    u8 language;
    u8 noBDVD;
    u8 gui_mode;
    u8 bk_picture;
    char pad[148];
    u32 event_flag;
    u32 opt_flags;
    int cover_mode;
    int background_fx;
    int show_custom_icons;

    int mode_homebrew;
    int game_list_category;
    int mode_favourites;
    int retro_mode;
    int filter_by_device;
    int filter_by_letter;
    u8  global_bd_mirror;
    u8  global_wm_autoplay;
} manager_cfg;

struct {
    int version;
    int perm;
    int useBDVD;
    int updates;
    int ext_ebootbin;
    int bdemu;
    int exthdd0emu;
    int direct_boot;
    int bdemu_ext;
    int pad[5];
} game_cfg;


int inited = 0;

#define INITED_SPU          2
#define INITED_SOUNDLIB     4
#define INITED_GCM_SYS      8
#define INITED_IO          16
#define INITED_PNGDEC      32
#define INITED_FS          64
#define INITED_JPGDEC      128
#define INITED_MODLIB      256
#define INITED_HTTPS       512

bool set_install_pkg = false;

#ifdef LOADER_MODE
void SaveManagerCfg() {}
#else
void SaveManagerCfg()
{
    manager_cfg.mode_homebrew = mode_homebrew;
    manager_cfg.game_list_category = game_list_category;
    manager_cfg.mode_favourites = mode_favourites;
    manager_cfg.retro_mode = retro_mode;
    manager_cfg.filter_by_device = filter_by_device;
    manager_cfg.filter_by_letter = filter_by_letter;

    sprintf(tmp_path, "%s/USRDIR/manager_setup.bin", self_path);
    if(file_exists(tmp_path)==false)
    {
        sprintf(tmp_path, "%s/config/manager_setup.bin", self_path);
    }

    SaveFile(tmp_path, (char *) &manager_cfg, sizeof(manager_cfg));
}
#endif

void LoadManagerCfg()
{
    sprintf(tmp_path, "%s/USRDIR/manager_setup.bin", self_path);
    if(file_exists(tmp_path)==false)
    {
        sprintf(tmp_path, "%s/config/manager_setup.bin", self_path);
    }

    int file_size;
    char *file = LoadFile(tmp_path, &file_size);

    if(file)
    {
        if(file_size != sizeof(manager_cfg))
        {
            file_size = sizeof(manager_oldcfg); // safe
            //manager_cfg.opt_flags |= OPTFLAGS_PLAYMUSIC; // enabled by default
            manager_cfg.gui_mode = MODE_COVERFLOW;
            manager_cfg.background_sel = 1;
            manager_cfg.background_fx = 0;
            manager_cfg.bk_picture = BG_NONE;
            manager_cfg.show_custom_icons = 1;
            manager_cfg.filter_by_device = LIST_ALL_DEVICES;
            manager_cfg.filter_by_letter = filter_by_letter;
            manager_cfg.global_bd_mirror = 0;
            manager_cfg.global_wm_autoplay = 0;

            manager_cfg.mode_homebrew = GAMEBASE_MODE;
            manager_cfg.game_list_category = GAME_LIST_ALL;
            manager_cfg.mode_favourites = 1;
            manager_cfg.retro_mode = RETRO_ALL;

            SaveManagerCfg();
        }

        memcpy(&manager_cfg, file, file_size);
        free(file);
    }
    else
    {
        //manager_cfg.opt_flags |= OPTFLAGS_PLAYMUSIC; // enabled by default
        manager_cfg.gui_mode = MODE_COVERFLOW;
        manager_cfg.background_sel = 1;
        manager_cfg.background_fx = 0;
        manager_cfg.bk_picture = BG_NONE;
        manager_cfg.show_custom_icons = 1;
        manager_cfg.filter_by_device = LIST_ALL_DEVICES;
        manager_cfg.filter_by_letter = filter_by_letter;
        manager_cfg.global_bd_mirror = 0;
        manager_cfg.global_wm_autoplay = 0;

        manager_cfg.mode_homebrew = GAMEBASE_MODE;
        manager_cfg.game_list_category = GAME_LIST_ALL;
        manager_cfg.mode_favourites = 1;
        manager_cfg.retro_mode = RETRO_ALL;

        SaveManagerCfg();
    }

    if((manager_cfg.gui_mode < 0) || (manager_cfg.gui_mode > GUI_MODES)) manager_cfg.gui_mode = MODE_COVERFLOW;

    gui_mode = manager_cfg.gui_mode & 0xF;
    cover_mode = manager_cfg.cover_mode;

    mode_homebrew = manager_cfg.mode_homebrew;
    game_list_category = manager_cfg.game_list_category;
    mode_favourites = manager_cfg.mode_favourites;
    retro_mode = manager_cfg.retro_mode;
    opt_filter_by_device = filter_by_device = manager_cfg.filter_by_device;

    get_grid_dimensions(false);

    background_sel = manager_cfg.background_sel;
    background_fx = manager_cfg.background_fx;
    show_custom_icons = manager_cfg.show_custom_icons;

    bk_picture = manager_cfg.bk_picture;
    if(gui_mode > MODE_GRID_6x5 && bk_picture == BG_PIC1) bk_picture = BG_NONE;

    switch(Video_Resolution.height)
    {
        case 480:
            videoscale_x = manager_cfg.videoscale_x[0];
            videoscale_y = manager_cfg.videoscale_y[0];
            break;
       case 576:
            videoscale_x = manager_cfg.videoscale_x[1];
            videoscale_y = manager_cfg.videoscale_y[1];
            break;
       case 720:
            videoscale_x = manager_cfg.videoscale_x[2];
            videoscale_y = manager_cfg.videoscale_y[2];
            break;
       default:
            videoscale_x = manager_cfg.videoscale_x[3];
            videoscale_y = manager_cfg.videoscale_y[3];
            break;
    }

    if(manager_cfg.language == SYSTEM_LANGUAGE)
        manager_cfg.language = (u8) get_system_language();

    if(manager_cfg.language < 0 || manager_cfg.language > LANGCOUNT)  manager_cfg.language = 0;

    sprintf(tmp_path, "%s/config/language.ini", self_path);
    open_language(manager_cfg.language, tmp_path);
}

void fun_exit()
{
    // multiple calls? (Yes!)
    static int one = 1;
    if(!one) return;
    one = 0;

    event_threads_finish();
    set_usleep_sm_main(250000);

    if(!is_ps3game_running && !use_cobra && lv2peek(0x80000000000004E8ULL)) syscall_40(1, 0); // disables PS3 Disc-less

    SaveManagerCfg();

    close_language();
    TTFUnloadFont();

#ifndef LOADER_MODE
    ftp_deinit();
    ftp_net_deinit();
#endif

    if(inited & INITED_SOUNDLIB)
    {
        if(inited & INITED_MODLIB)
            MODPlay_Unload (&mod_track);
        SND_End();
    }

    if(snd_inited & INITED_SOUNDLIB)
        SND_End();

    if(snd_inited & INITED_CALLBACK)
        sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);

    if(snd_inited & INITED_AUDIOPLAYER)
        StopAudio();

    if(inited & INITED_SPU)
    {
        sleep(1);
        sysSpuRawDestroy(spu);
        sysSpuImageClose(&spu_image);
    }

    NTFS_UnMountAll();

    if(inited & INITED_GCM_SYS) sysModuleUnload(SYSMODULE_GCM_SYS);
    if(inited & INITED_IO)      sysModuleUnload(SYSMODULE_IO);
    if(inited & INITED_PNGDEC)  sysModuleUnload(SYSMODULE_PNGDEC);
    if(inited & INITED_JPGDEC)  sysModuleUnload(SYSMODULE_JPGDEC);
    if(inited & INITED_FS)      sysModuleUnload(SYSMODULE_FS);
    if(inited & INITED_HTTPS)   sysModuleUnload(SYSMODULE_HTTPS);
    sysModuleUnload(SYSMODULE_SYSUTIL);

    inited = 0;
    //if(manager_cfg.usekey) sys8_disable(hmanager_key);

    ioPadEnd();


#ifdef SHOWTIME_LOADER
    #define FS_S_IFMT 0170000

    char video_path[1024];
    strcpy(video_path, self_path);
    strcat(video_path, "/USRDIR/SHOWTIME.TXT");

    if(file_exists("/dev_hdd0/game/IRISMAN00/USRDIR/SHOWTIME.SELF"))
    {
        mkdir("/dev_hdd0/game/IRISMAN00/USRDIR/TEMP", S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
        sysLv2FsChmod("/dev_hdd0/game/IRISMAN00/USRDIR/TEMP", FS_S_IFMT | 0777);

        sysLv2FsUnlink("/dev_hdd0/game/IRISMAN00/USRDIR/TEMP/SHOWTIME.TXT");

        sysLv2FsLink(video_path, "/dev_hdd0/game/IRISMAN00/USRDIR/TEMP/SHOWTIME.TXT");
        sysLv2FsChmod("/dev_hdd0/game/IRISMAN00/USRDIR/TEMP/SHOWTIME.TXT", FS_S_IFMT | 0777);

        strcpy(self_path, "/dev_hdd0/game/IRISMAN00");
        launch_showtime(true);
    }

    if(file_exists("/dev_hdd0/game/BLES80608/USRDIR/sys/SHOWTIME.SELF"))
    {
        mkdir("/dev_hdd0/game/BLES80608/USRDIR/sys/TEMP", S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
        sysLv2FsChmod("/dev_hdd0/game/BLES80608/USRDIR/sys/TEMP", FS_S_IFMT | 0777);

        sysLv2FsUnlink("/dev_hdd0/game/BLES80608/USRDIR/sys/TEMP/SHOWTIME.TXT");

        sysLv2FsLink(video_path, "/dev_hdd0/game/BLES80608/USRDIR/sys/TEMP/SHOWTIME.TXT");
        sysLv2FsChmod("/dev_hdd0/game/BLES80608/USRDIR/sys/TEMP/SHOWTIME.TXT", FS_S_IFMT | 0777);

        sysProcessExitSpawn2("/dev_hdd0/game/BLES80608/USRDIR/sys/SHOWTIME.SELF", NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
    }

    sysProcessExitSpawn2("/dev_hdd0/game/HTSS00003/USRDIR/showtime.self", NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
#endif

    if(lv2_unpatch_storage) lv2_unpatch_storage();

    if(set_install_pkg)
    {
        unlink_secure("/dev_hdd0/tmp/turnoff");
        unlink_secure("/dev_hdd0/tmp/wm_request");
    }

    sys8_perm_mode(0); // perms to 0 from exit()

    if(restore_syscall8[0]) sys8_pokeinstr(restore_syscall8[0], restore_syscall8[1]);

#ifndef LOADER_MODE
    if(game_cfg.direct_boot == 555 && use_cobra)
        sysProcessExitSpawn2("/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN", NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);

    if(set_install_pkg) sys_reboot();
#endif
}

static void exit_to_xmb(void)
{
    fun_exit();
    exit(0);
}

static void sys_callback(uint64_t status, uint64_t param, void* userdata) {

     switch (status)
     {
       case SYSUTIL_EXIT_GAME:

            SaveGameList();
            fun_exit();

            sysProcessExit(1);
            break;

       default:
            break;
     }
}

#ifndef LOADER_MODE
void show_ftp_error(int r)
{
    if(r == -1) DrawDialogOKTimer("Error in netInitialize()", 2000.0f); else
    if(r == -2) DrawDialogOKTimer("Error in netCtlInit()", 2000.0f);    else
    if(r == -3) DrawDialogOKTimer("Error in netCtlGetInfo()", 2000.0f); else
    if(r == -4) DrawDialogOKTimer("Net Disconnected or Connection not Established", 2000.0f); else
                DrawDialogOKTimer("FTP Unknown Error", 2000.0f);
}

void auto_ftp(void)
{
    static int one = 1;
    static int counter = 0;

    if(!one) goto ftp_net;

    one = 0;
    if (ftp_serv || (manager_cfg.opt_flags & OPTFLAGS_FTP)) // maybe we need add an icon to user...
    {
        manager_cfg.opt_flags |= OPTFLAGS_FTP;

        int r = ftp_init();

        if(r == SUCCESS) ftp_inited = true; //DrawDialogOK("FTP Service init on boot: OK");
        else
            show_ftp_error(r);
    }

ftp_net:
    if(!ftp_inited) return;
    counter++;

    if(counter < 600)  return;

    counter = 0;

    get_net_status();
}
#endif

void RemoveNtfsFromGameList()
{
    delete_entries(directories, &ndirectories, NTFS_FLAG);

    for (int i = ndirectories; i < MAX_DIRECTORIES; i++)
    {
        directories[i].flags = 0;
        directories[i].title[0] = 0;
        directories[i].title_id[0] = 0;
        directories[i].path_name[0] = 0;
    }

    sort_entries2(directories, &ndirectories, sort_mode);
}

void LoadGameList()
{
    if(!bCachedGameList) return;

    sprintf(tmp_path, "%s/config/gamelist.bin", self_path);

    int file_size;
    char *file = LoadFile(tmp_path, &file_size);

    if(file)
    {
        int i;

        if(file_size == sizeof(directories))
        {
            memcpy(&directories, file, file_size);
            free(file);

            ndirectories = MAX_DIRECTORIES - 1;
            while (ndirectories > 0 && directories[ndirectories].flags == 0) ndirectories--;
            ndirectories++;

            RemoveNtfsFromGameList();
            delete_custom_icons(directories, &ndirectories);

            find_device = 0;
            sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
            freeSpace[find_device] = ( ((u64)blockSize * freeSize));
            freeSpace[find_device] = (float) freeSpace[find_device] / GIGABYTES;

            fdevices = 2;

            for (i = 1; i < BDVD_DEVICE; i++) freeSpace[i] = 0;

            for (i = 0; i < ndirectories; i++)
            {
                if(bAllowNetGames && strcmp(directories[i].title_id, NETHOST) == SUCCESS)
                {
                    if(bXMLScanOnlyNetGames)
                    {
                        if(strncmp(directories[i].path_name, "net", 3)) bXMLScanOnlyNetGames = false;
                    }

                    bSkipParseXML = true;
                }
                else if(/*is_ntfs_path(directories[i].path_name) ||*/
                        !strncmp(directories[i].path_name, "/dev_bdvd", 9) ||
                        !file_exists(directories[i].path_name))
                {
                    directories[i].flags = 0;
                    directories[i].title[0] = 0;
                    directories[i].title_id[0] = 0;
                    directories[i].path_name[0] = 0;
                }
                else if (directories[i].path_name[5] == 'u' &&
                         directories[i].path_name[6] == 's' &&
                         directories[i].path_name[7] == 'b')
                {
                    // /dev_usb
                    find_device = (directories[i].path_name[10]);
                    if(find_device > 47 && find_device < 58) fdevices |= 1<<(find_device - 47);
                }
            }

            fdevices_old = fdevices;

            // check bdemu
            char dev_path[12];
            for (find_device = HDD0_DEVICE; find_device < BDVD_DEVICE; find_device++)
            {
                if(find_device == HDD0_DEVICE)
                    sprintf(dev_path, "/dev_hdd0");
                else
                    sprintf(dev_path, "/dev_usb00%c", 47 + find_device);

                if(file_exists(dev_path))
                    move_bdemubackup_to_origin(1 << find_device);
            }
        }
        else free(file);

        delete_entries(directories, &ndirectories, 0);
        sort_entries2(directories, &ndirectories, sort_mode);

        cached_game_list = (ndirectories > 0);
    }
}

void SaveLastGame()
{
    if(!allow_save_lastgame) return;

    sprintf(tmp_path, "%s/config/lastgame", self_path);

    memset(temp_buffer, 0, 72 + MAXPATHLEN);
    memcpy(temp_buffer, directories[currentgamedir].title_id, 65);
    memcpy(temp_buffer + 68, &directories[currentgamedir].flags, 4);
    memcpy(temp_buffer + 72, &directories[currentgamedir].path_name, MAXPATHLEN);

    SaveFile(tmp_path, temp_buffer, 72 + MAXPATHLEN);
}

void SaveGameList()
{
    SaveLastGame();

    if(!bCachedGameList) return;

    for(int i = ndirectories; i < MAX_DIRECTORIES; i++)
    {
        directories[i].flags = 0;
        directories[i].title[0] = 0;
        directories[i].title_id[0] = 0;
        directories[i].path_name[0] = 0;
    }

    sprintf(tmp_path, "%s/config/gamelist.bin", self_path);
    SaveFile(tmp_path, (char *) &directories, sizeof(directories));
}

int get_last_selected_game(bool launch)
{
    sprintf(tmp_path, "%s/config/lastgame", self_path);

    int size;
    char * mem = LoadFile(tmp_path, &size);

    if(mem && (size >= 72))
    {
        mem[63] = 0;

        if(launch && size > 72 && file_exists(mem + 72))
        {
            ndirectories = 1;
            currentgamedir = currentdir = select_px = select_py = 0;
            memcpy( directories[currentgamedir].title_id , mem + 0, 65);
            memcpy(&directories[currentgamedir].flags    , mem + 68, 4);
            memcpy( directories[currentgamedir].path_name, mem + 72, MAXPATHLEN);

            if(mem) free(mem);

            last_game_flag = directories[currentgamedir].flags;

            return 1;
        }
        else
        {
            get_grid_dimensions(false);

            if(ndirectories <= 0)
            {
                get_games();
                load_gamecfg(RESET_GAME_INFO); // force refresh game info
            }

            for(int n = 0; n < ndirectories; n++)
            {
                if(!strcmp(directories[n].title_id, mem) && !memcmp(&directories[n].flags, mem + 68, 4))
                {
                    last_game_flag = directories[n].flags;

                    frame_count = 32;

                    currentgamedir = n;
                    get_grid_dimensions(true);

                    return 0;
                }
            }
        }
    }

    if(mem) free(mem);

    last_game_flag = 0;
    return 0;
}

void video_adjust()
{

    SetCurrentFont(FONT_TTF);
    while(true)
    {
        double sx = (double) Video_Resolution.width;
        double sy = (double) Video_Resolution.height;
        double psx = (double) (1000 + videoscale_x)/1000.0;
        double psy = (double) (1000 + videoscale_y)/1000.0;

        tiny3d_UserViewport(1,
            (float) ((sx - sx * psx) / 2.0), // 2D position
            (float) ((sy - sy * psy) / 2.0),
            (float) ((sx * psx) / 848.0),    // 2D scale
            (float) ((sy * psy) / 512.0),
            (float) ((sx / 1920.0) * psx),  // 3D scale
            (float) ((sy / 1080.0) * psy));

        tiny3d_Clear(background_colors[background_sel], TINY3D_CLEAR_ALL);


        // Enable alpha Test
        tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

        // Enable alpha blending.
                tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
                    TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
                    TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
        reset_ttf_frame();

        DrawAdjustBackground(WHITE);

        update_twat(false);
        SetFontSize(16, 24);
        SetFontColor(WHITE, BLACK);

        SetFontAutoCenter(1);

        DrawFormatString(0, (512 - 24)/2 - 64, "%s", language[VIDEOADJUST_POSITION]);

        DrawFormatString(0, (512 - 24)/2, language[VIDEOADJUST_SCALEINFO], videoscale_x, videoscale_y);

        DrawFormatString(0, (512 - 24)/2 + 64, "%s", language[VIDEOADJUST_EXITINFO]);

        DrawFormatString(0, (512 - 24)/2 + 96, "%s", language[VIDEOADJUST_DEFAULTS]);

        // Warning!! don´t traslate this string!
        DrawFormatString(0, (512 - 24)/2 + 128, "%s", "Press [] to English language");

        SetFontAutoCenter(0);

        tiny3d_Flip();
        ps3pad_read();

        if(!(frame_count & 3))
        {
            if(old_pad & BUTTON_UP) {if(videoscale_y > -179) videoscale_y--;}
            if(old_pad & BUTTON_DOWN) {if(videoscale_y < 10) videoscale_y++;}
            if(old_pad & BUTTON_LEFT) {if(videoscale_x > -199) videoscale_x--;}
            if(old_pad & BUTTON_RIGHT) {if(videoscale_x < 10) videoscale_x++;}
        }

        if(new_pad & BUTTON_SQUARE)
        {
            manager_cfg.language = 0;
            sprintf(tmp_path, "%s/config/language.ini", self_path);
            open_language(manager_cfg.language, tmp_path);
        }

        if(new_pad & (BUTTON_CROSS_ | BUTTON_START))
        {
            switch(Video_Resolution.height)
            {
               case 480:
                    manager_cfg.videoscale_x[0] = videoscale_x;
                    manager_cfg.videoscale_y[0] = videoscale_y;
                    break;
               case 576:
                    manager_cfg.videoscale_x[1] = videoscale_x;
                    manager_cfg.videoscale_y[1] = videoscale_y;
                    break;
               case 720:
                    manager_cfg.videoscale_x[2] = videoscale_x;
                    manager_cfg.videoscale_y[2] = videoscale_y;
                    break;
               default:
                    manager_cfg.videoscale_x[3] = videoscale_x;
                    manager_cfg.videoscale_y[3] = videoscale_y;
                    break;
            }


            SaveManagerCfg();

            break;
        }

        if(new_pad & BUTTON_CIRCLE_)
        {
            if(videoscale_x == -120 && videoscale_y == -120)
            {
                videoscale_x = -40;
                videoscale_y = -60;
            }
            else
            {
                videoscale_x = videoscale_y = -120;
            }
        }

        frame_count++;
    }
}

void Select_games_folder()
{

    DIR  *dir, *dir2;
    int selected = 0;
    char tmp[256];

    sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_WANTUSE], "/dev_hdd0/GAMES", language[GAMEFOLDER_TOINSTALLNTR]);

    if(DrawDialogYesNoDefaultYes(temp_buffer) == YES)
    {
        strncpy(hdd_folder, "GAMES", 64);
        strncpy(manager_cfg.hdd_folder, "GAMES", 64);
        mkdir_secure("/dev_hdd0/GAMES");
        return;
    }

    dir = opendir("/host_root");
    if(dir)
    {
        closedir(dir);
        sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_WANTUSE], "/host_root", language[GAMEFOLDER_TOINSTALLNTR]);

        if(DrawDialogYesNo(temp_buffer) == YES)
        {
            strncpy(hdd_folder, "host_root", 64);
            strncpy(manager_cfg.hdd_folder, "host_root", 64);
            return;
        }
    }

    dir = opendir("/dev_hdd0/game");

    if(dir)
    {
        while(true)
        {
            struct dirent *entry = readdir(dir);

            if(!entry) break;

            if(entry->d_name[0] == '.' && (entry->d_name[1] == 0 || entry->d_name[1]== '.')) continue;

            if(!(entry->d_type & DT_DIR)) continue;

            sprintf(tmp_path, "/dev_hdd0/game/%s/"__MKDEF_GAMES_DIR, entry->d_name);

            dir2 = opendir(tmp_path);

            if(dir2)
            {
                closedir(dir2);

                sprintf(temp_buffer, "%s /%s %s", language[GAMEFOLDER_WANTUSE], entry->d_name, language[GAMEFOLDER_TOINSTALLNTR]);

                if(DrawDialogYesNo(temp_buffer) == YES)
                {
                    strncpy(hdd_folder, entry->d_name, 64);
                    strncpy(manager_cfg.hdd_folder, entry->d_name, 64);
                    selected = 1;
                    break;
                }
            }

        }
        closedir(dir);
    }

    dir = opendir("/dev_hdd0/video");
    if(dir)
    {
        closedir(dir);
        sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_WANTUSE], "/dev_hdd0/video", language[GAMEFOLDER_TOINSTALLNTR]);

        if(DrawDialogYesNoDefaultYes(temp_buffer) == YES)
        {
            strncpy(hdd_folder, "video", 64);
            strncpy(manager_cfg.hdd_folder, "video", 64);
            return;
        }
    }

    if(!selected)
    {
        sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_WANTUSE], "/dev_hdd0/" __MKDEF_GAMES_DIR, language[GAMEFOLDER_TOINSTALLNTR]);

        if(DrawDialogYesNo(temp_buffer) == YES)
        {
            strncpy(hdd_folder, "dev_hdd0", 64);
            strncpy(manager_cfg.hdd_folder, "dev_hdd0", 64);
            mkdir_secure("/dev_hdd0/" __MKDEF_GAMES_DIR);
        }
        else
        {
            strncpy(hdd_folder, __MKDEF_MANAGER_DIR__, 64);
            strncpy(manager_cfg.hdd_folder, __MKDEF_MANAGER_DIR__, 64);
            sprintf(tmp, "/%s/" __MKDEF_GAMES_DIR, __MKDEF_MANAGER_FULLDIR__);
            mkdir_secure(tmp);

            sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_USING], tmp, language[GAMEFOLDER_TOINSTALL]);
            DrawDialogOK(temp_buffer);
        }
    }
}

void pause_music(int pause)
{
    if((!pause)&&(!(manager_cfg.opt_flags & OPTFLAGS_PLAYMUSIC)))
        return;

    if(mod_volume < 1) mod_volume = 6;
    SND_Pause(pause);
}

int get_net_status()
{
#ifdef LOADER_MODE
    return FAILED;
#else
    int r = ftp_net_status();

    if(r == -1)
    {
       ftp_net_init();
       r = ftp_net_status();
    }

    if(r == -4)
    {
       ftp_net_deinit();
       ftp_net_init();
       r = ftp_net_status();
    }

    return r;
#endif
}

void init_music(int select_song)
{
    test_audio_file(true); // stop mp3/ogg

    if(MAX_SONGS == 0)
    {
        sprintf(tmp_path, "%s/USRDIR/music", self_path);
        DIR  *dir = opendir(tmp_path);

        if(dir)
        {
            while(true)
            {
                struct dirent *entry = readdir(dir);

                if(!entry) break;
                if(entry->d_name[0] == '.' && (entry->d_name[1] == 0 || entry->d_name[1] == '.')) continue;

                if(entry->d_name[0] == 'm' && (strstr(entry->d_name, ".mod") || strstr(entry->d_name, ".mp3") || strstr(entry->d_name, ".ogg"))) MAX_SONGS++;
            }
            closedir(dir);
        }

        if(MAX_SONGS == 0) return;
    }

    if(select_song < 0 && (manager_cfg.opt_flags & OPTFLAGS_PLAYMUSIC))
    {
        sprintf(tmp_path, "%s/USRDIR/music/music.mp3", self_path);
        if(PlayAudio(tmp_path, 0, AUDIO_ONE_TIME)==0) {snd_inited|= INITED_AUDIOPLAYER; return;}
        sprintf(tmp_path, "%s/USRDIR/music/music.ogg", self_path);
        if(PlayAudio(tmp_path, 0, AUDIO_ONE_TIME)==0) {snd_inited|= INITED_AUDIOPLAYER; return;}

        srand(time(0)); // randomize seed

        int song_index = rand() % MAX_SONGS;

        do
        {
            sprintf(tmp_path, "%s/USRDIR/music/music%i.mp3", self_path, song_index + 1);
            if(PlayAudio(tmp_path, 0, AUDIO_ONE_TIME)==0) {snd_inited|= INITED_AUDIOPLAYER; song_selected = song_index; return;}
            sprintf(tmp_path, "%s/USRDIR/music/music%i.ogg", self_path, song_index + 1);
            if(PlayAudio(tmp_path, 0, AUDIO_ONE_TIME)==0) {snd_inited|= INITED_AUDIOPLAYER; song_selected = song_index; return;}
            if(song_index > 10) song_index = rand() % MAX_SONGS; else song_index--;
        }
        while(song_index > 0);
    }

    if(select_song >= 0 && select_song < MAX_SONGS)
    {
        song_selected = select_song;

        do
        {
            sprintf(tmp_path, "%s/USRDIR/music/music%i.mp3", self_path, song_selected + 1);
            if(PlayAudio(tmp_path, 0, AUDIO_ONE_TIME)==0) {snd_inited|= INITED_AUDIOPLAYER; return;}
            sprintf(tmp_path, "%s/USRDIR/music/music%i.ogg", self_path, song_selected + 1);
            if(PlayAudio(tmp_path, 0, AUDIO_ONE_TIME)==0) {snd_inited|= INITED_AUDIOPLAYER; return;}
            song_selected--;
        }
        while(song_selected > 0);
    }

    MODPlay_Init(&mod_track);

    int file_size;
    char *file = NULL;

    if(select_song < 0)
    {
        sprintf(tmp_path, "%s/music.mod", self_path);
        file = LoadFile(tmp_path, &file_size);

        if(!file)
        {
            sprintf(tmp_path, "%s/MUSIC.MOD", self_path);
            file = LoadFile(tmp_path, &file_size);
        }
    }

    if(select_song >= 0 && select_song < MAX_SONGS)
    {
        song_selected = select_song;

        do
        {
            sprintf(tmp_path, "%s/USRDIR/music/music%i.mod", self_path, song_selected + 1);
            file = LoadFile(tmp_path, &file_size);
            if(!file) song_selected--;
        }
        while(!file && song_selected > 0);
    }

    if(select_song >= 0 && select_song < MAX_SONGS)
    {
        song_selected = select_song;

        do
        {
            sprintf(tmp_path, "%s/USRDIR/music/music%i.mod", self_path, song_selected + 1);
            file = LoadFile(tmp_path, &file_size);
            if(!file) song_selected--;
        }
        while(!file && song_selected > 0);
    }

    if(!file)
    {
        srand(time(0)); // randomize seed

        do
        {
            sprintf(tmp_path, "%s/USRDIR/music/music%i.mod", self_path, song_selected + 1);
            file = LoadFile(tmp_path, &file_size);
            if(!file) {if(song_selected > 10) song_selected = rand() % MAX_SONGS; else song_selected--;}
        }
        while(!file && song_selected > 0);
    }


    if(!file) return;

    // paranoic code to copy the .mod in aligned and large memory
    char *file2 = memalign(32, file_size + 32768);
    if(file2) {memcpy(file2, file, file_size);free(file); file = file2;}

    if(MODPlay_SetMOD (&mod_track, file) < 0) MODPlay_Unload (&mod_track);

    if(mod_volume < 1) mod_volume = 6;

    MODPlay_SetVolume( &mod_track, mod_volume, mod_volume); // fix the volume to 16 (max 64)
    MODPlay_Start (&mod_track); // Play the MOD
    inited |= INITED_MODLIB;
    SND_Pause(1); //force pause here

}


int payload_mode = 0;

int load_from_bluray = 0;

//#ifndef LOADER_MODE
static char filename[MAXPATHLEN];
//#endif

/******************************************************************************************************************************************************/

static volatile int bdvd_notify = 0;
static volatile int disc_less_on = 0;

static volatile bool bdvd_ejected = true;

void DiscEjectCallback(void)
{
    bdvd_notify = -1;
    bdvd_ejected = true;
}

void DiscInsertCallback(u32 discType, char *title)
{

    bdvd_notify = 0;

    if((noBDVD == MODE_WITHBDVD) && lv2_patch_storage)
    {
        if(firmware < 0x421C)
            Reset1_BDVD();
        else
            Reset2_BDVD();
    }

    if(noBDVD != MODE_DISCLESS)
    {
        bdvd_notify = 1;
        bdvd_ejected = false;

        mode_favourites = 0;
        select_option = 0;
        currentgamedir = currentdir = select_px = select_py = 0;
    }
    else
        disc_less_on = 1;
}

bool is_libfs_patched(void)
{
    char path[256];

    sprintf(path, "%s/libfs_patched.sprx", self_path);
    return (file_exists(path));
}


/******************************************************************************************************************************************************/

bool test_ftp_working()
{
#ifndef LOADER_MODE
    if(get_ftp_activity())
    {
        if(DrawDialogYesNoDefaultYes("FTP is working now\n"
                                     "Do you want to interrupt the FTP activity?\n\n"
                                     "El FTP está trabajando ahora mismo\n"
                                     "Desea interrumpir su actividad?\n\n"
                                     "Service FTP en cours\n"
                                     "L'intérompre?") == YES)
        {
            ftp_deinit();
            ftp_inited = false;
            return false;
        }
        else
            return true;
    } return false;
#endif
    return false;
}

int is_payload_loaded(void)
{
    u64 addr = lv2peek(0x80000000000004f0ULL);

    if((addr>>32) == 0x534B3145)
    {
        addr&= 0xffffffff;
        if(addr && peekq(0x80000000000004f8ULL))
        {
            restore_syscall8[0]= lv2peek(0x80000000000004f8ULL); // (8*8)
            restore_syscall8[1]= lv2peek(restore_syscall8[0]);
            lv2poke(restore_syscall8[0], 0x8000000000000000ULL + (u64) (addr + 0x20));
            return 2;
        }

        return 1;
    }

    return 0;
}

void set_last_game()
{
    int i = select_px + select_py * cols;
    currentgamedir = get_currentdir(i);
    if(currentgamedir < 0 || currentgamedir >= ndirectories || ndirectories <= 0) return;
    last_game_favourites = mode_favourites;
    last_game_flag = directories[currentgamedir].flags;
    strncpy(last_game_id, directories[currentgamedir].title_id, 63);
    last_game_id[63] = 0;
}

void locate_last_game()
{

    u32 flags = 0, f;

    int n, pos = -1;

    if(last_game_flag == 0 || ndirectories <= 0 || allow_restore_last_game == false) return;

    if(mode_favourites && last_game_favourites)
    {
        for(n = 0; n < num_box; n++)
        {
            if(favourites.list[n].index >= 0 && favourites.list[n].index < ndirectories &&
               favourites.list[n].title_id[0] != 0 && directories[favourites.list[n].index].flags)
            {
                int i = favourites.list[n].index;
                if(!strcmp(directories[i].title_id, last_game_id))
                {
                    f = directories[i].flags;

                    if(f & D_FLAG_HDD0)
                    {
                        flags = f;
                        pos = n;
                        break;
                    }
                    else if((f & NTFS_FLAG) && !(flags & D_FLAG_HDD0))
                    {
                        flags = f;
                        pos = n;
                    }
                    else if(!(flags & (NTFS_FLAG | D_FLAG_HDD0)))
                    {
                        flags = f;
                        pos = n;
                    }
                }
            }
        }

        if(pos >= 0)
        {
            currentgamedir =  favourites.list[pos].index;
            get_grid_dimensions(true);

            last_game_flag = 0;
            return;
        }

    }

    for(n = 0; n < ndirectories; n++)
    {
        if(!strcmp(directories[n].title_id, last_game_id))
        {
            f = directories[n].flags;
            if((f & last_game_flag) == last_game_flag)
            {
                flags = f;
                pos = n;
                break;
            }
            else if(f & D_FLAG_HDD0)
            {
                flags = f;
                pos = n;
            }
            else if((f & NTFS_FLAG) && !(flags & D_FLAG_HDD0))
            {
                flags = f;
                pos = n;
            }
            else if(!(flags & (NTFS_FLAG | D_FLAG_HDD0)))
            {
                flags = f;
                pos = n;
            }
        }
    }

    if(pos >= 0)
    {
        mode_favourites = 0;

        get_grid_dimensions(false);

        if(gui_mode != MODE_COVERFLOW)
        {
            currentdir = (pos / num_box) * num_box;
            select_py = ((pos - currentdir) / cols);
            select_px = (pos - currentdir) % cols;
        }
        else
        {
            currentdir = pos;

            if(currentdir <= (cols - 1) )
            {
                select_px = currentdir; select_py = 0;
                currentdir = 0;
            }
            else
            {
                currentdir -= (cols - 1);
                select_px = (cols - 1); select_py = 0;
            }
        }

        int i = select_px + select_py * cols;

        currentgamedir = (currentdir + i);

        get_games();
        load_gamecfg(RESET_GAME_INFO); // force refresh game info

    }

    last_game_flag = 0;
}

static int locate_game_url(u8 *mem, int pos, int size, char *key, int *length)
{
    int start = 0;
    int end = 0;

    int sig = 0;

    *length = 0;

    while(pos < size)
    {
        if(mem[pos] == '"' || sig)
        {
            if(mem[pos] == '"') sig ^= 1;
            pos++; continue;
        }

        if(!strncmp((char *) &mem[pos], key, strlen(key)))
        {
            pos += strlen(key);
            start = pos;
            end = pos;
            break;
        }

        pos++;
    }

    while(pos < size)
    {
        if(mem[pos] == '<')
        {
            end = pos - start;
            break;
        }

        pos++;
    }

    if(pos >= size) return -99;

    *length = end;

    return start;
}

static void parse_mygames_xml()
{
    char game_path[1024];

    int n = 0;
    int file_size = 0;
    int m = 0, l = 0;
    u8 *mem = (u8 *) LoadFile("/dev_hdd0/xmlhost/game_plugin/mygames.xml", &file_size);

    while(n < file_size)
    {
        n = m + l + 1;
        m = locate_game_url(mem, n, file_size, "<String>http://127.0.0.1/mount_ps3/", &l);

        if(m < 0) break;

        if(l <= 0) continue; // empty
        if(l > 1023) l = 1023;

        memset(game_path, 0, 1024);
        strncpy(game_path, (char *) &mem[m], l);
        game_path[l] = 0;

        if(bXMLScanOnlyNetGames && strncmp(game_path, "net", 3)) continue; // if not a net game

        if(strstr(game_path, "/GAMES"))
            directories[ndirectories].flags = PS3_FLAG;
        else if(strstr(game_path, "/GAMEZ"))
            directories[ndirectories].flags = PS3_FLAG;
        else if(strstr(game_path, "/PS3ISO"))
            directories[ndirectories].flags = PS3_FLAG;
        else if(strstr(game_path, "/PSXISO"))
            directories[ndirectories].flags = PS1_FLAG;
        else if(strstr(game_path, "/PS2ISO"))
            directories[ndirectories].flags = PS2_FLAG;
        else if(strstr(game_path, "/PSPISO"))
            directories[ndirectories].flags = PSP_FLAG;
        else if(strstr(game_path, "/DVDISO"))
            directories[ndirectories].flags = D_FLAG_HOMEB | D_FLAG_HOMEB_DVD;
        else if(strstr(game_path, "/BDISO"))
            directories[ndirectories].flags = D_FLAG_HOMEB | D_FLAG_HOMEB_BD;
        else if(strstr(game_path, "/VIDEO"))
            directories[ndirectories].flags = D_FLAG_HOMEB | D_FLAG_HOMEB_MKV;
        else if(strstr(game_path, "/MUSIC"))
            directories[ndirectories].flags = D_FLAG_HOMEB | D_FLAG_HOMEB_MKV;
        else
            continue;

        // add net game
        strcpy(directories[ndirectories].title_id, NETHOST);
        strncpy(directories[ndirectories].path_name, game_path, l);

        char *pos = strstr(directories[ndirectories].path_name, "?");
        if(pos) *pos = 0;

        sprintf(directories[ndirectories].title, "%s", get_filename(directories[ndirectories].path_name));
        urldec(directories[ndirectories].title);

        directories[ndirectories].title[63] = 0;

        ndirectories++;
    }

    if(mem) free(mem);
}

void read_settings()
{
    char * file = NULL;
    int file_size = 0;

    char AddToLastGameApp[2] = "1";
    char ShowPlayOverlay[2] = "1";
    char ShowUSBIcon[2] = "1";
    char BackgroundGears[2] = "1";
    char IconPulse[2] = "1";
    char FileManager[2] = "0";
    char TimeFormat[2] = "4";
    char UnmountDevBlind[2] = "0";
    char CachedGameList[2] = "1";
    char ShowPIC1[2] = "0";
    char ShowVersion[2] = "2";
    char SpoofVersion[2] = "0";
    char HideCoverflowSortModeLabel[2] = "0";
    char TimeoutByInactivity[2] = "1"; // 1 hour
    char SkipBDMirrorMsg[2] = "1";
    char InstallMamba[2] = "1";
    char LoadMambaAndQuit[2] = "0";

    spoof_version  = 0x0485;
    spoof_revision = 67869; // 0x0001091d

    // set default values
    sprintf(covers_path, "%s/USRDIR/covers/", MM_PATH);
    sprintf(retro_covers_path, "%s/USRDIR/covers_retro/psx/", MM_PATH);
    sprintf(backgrounds_path, "%s/USRDIR/background/", self_path);
    strcpy(updates_path, "/dev_hdd0/packages");
    strcpy(video_path, "/MKV");
    strcpy(webman_path, "/dev_hdd0/plugin/webftp_server.sprx");
    strcpy(psp_launcher_path, "/dev_hdd0/game/PSPC66820");
    strcpy(retroarch_path, "/dev_hdd0/game/SSNE10000");
    strcpy(ps2classic_path, "/PS2ISO");
    sprintf(psxiso_path, "%s/USRDIR/ps1_iso", self_path);

    sprintf(retro_root_path, "/ROMS/");
    sprintf(retro_snes_path, "%sSNES", retro_root_path);
    sprintf(retro_gba_path, "%sGBA", retro_root_path);
    sprintf(retro_gen_path, "%sGEN", retro_root_path);
    sprintf(retro_nes_path, "%sNES", retro_root_path);
    sprintf(retro_mame_path, "%sMAME", retro_root_path);
    sprintf(retro_fba_path, "%sFBA", retro_root_path);
    sprintf(retro_doom_path, "%sDOOM", retro_root_path);
    sprintf(retro_quake_path, "%sQUAKE", retro_root_path);
    sprintf(retro_pce_path, "%sPCE", retro_root_path);
    sprintf(retro_gb_path, "%sGB", retro_root_path);
    sprintf(retro_gbc_path, "%sGBC", retro_root_path);
    sprintf(retro_atari_path, "%sATARI", retro_root_path);
    sprintf(retro_vb_path, "%sVB", retro_root_path);
    sprintf(retro_nxe_path, "%sNXE", retro_root_path);
    sprintf(retro_wswan_path, "%sWSWAN", retro_root_path);

    sprintf(retro_a7800_path, "%sA7800", retro_root_path);
    sprintf(retro_lynx_path, "%sLYNX", retro_root_path);
    sprintf(retro_gw_path, "%sGW", retro_root_path);
    sprintf(retro_vectrex_path, "%sVECTX", retro_root_path);
    sprintf(retro_2048_path, "%s2048",  retro_root_path);

    strcpy(video_extensions, ".MKV .MP4 .AVI .MPG .MPEG .MOV .M2TS .VOB .FLV .WMV .ASF .DIVX .XVID .PAM .BIK .BINK .VP6 .MTH .3GP .RMVB .OGM .OGV .M2T .MTS .TS .TSV .TSA .TTS .RM .RV .VP3 .VP5 .VP8 .264 .M1V .M2V .M4B .M4P .M4R .M4V .MP4V .MPE .BDMV .DVB .WEBM .NSV");
    strcpy(audio_extensions, ".MP3 .WAV .WMA .AAC .AC3 .AT3 .OGG .OGA .MP2 .MPA .M4A .FLAC .RA .RAM .AIF .AIFF .MOD .S3M .XM .IT .MTM .STM .UMX .MO3 .NED .669 .MP1 .M1A .M2A .M4B .AA3 .OMA .AIFC");
    strcpy(rom_extensions, ".ZIP .GBA .NES .UNIF .GB .GBC .DMG .MD .SMD .GEN .SMS .GG .SG .BIN .CUE .IOS .FLAC .NGP .NGC .PCE .SGX .CUE .VB .VBOY .BIN .WS .WSC .FDS .EXE .WAD .IWAD .SMC .FIG .SFC .GD3 .GD7 .DX2 .BSX .SWC .A26 .BIN .PAK");
    strcpy(browser_extensions, ".HTML .HTM .TXT .INI .CFG .GIF");
    strcpy(custom_homebrews, "HTSS00003|NETBROWSE|CDGPLAYER|GMPADTEST|IRISMAN00|BLES80616|IMANAGER4|GMANAGER9|PRXLOADER|RBGTLBOX2|BLES80608|BLES80688|SSNE10000|SNES90000|VBAM90000|GENP00001|FCEU90000|FBAN00000|FBAL00123|MAME90000|PSID81257|PS2L00123|ROGE12345|CNDR00001|BLJS10018|");


    default_psxoptions = 1; //ps1_netemu

    ftp_port = 21;

    roms_count = 0;
    max_roms = 80;

    if(sys_button_layout == 0)
    {
        BUTTON_CROSS_ = BUTTON_CIRCLE; //0x0020
        BUTTON_CIRCLE_ = BUTTON_CROSS; //0x0040
    }

    // read settings.ini
    sprintf(tmp_path, "%s/USRDIR/settings.ini", self_path);

    if(file_exists(tmp_path) == false) //if settings.ini is not found, then use the settings_default.ini
    {
       sprintf(tmp_path, "%s/USRDIR/settings_default.ini", self_path);

       if(file_exists(tmp_path))
          file = LoadFile(tmp_path, &file_size);

       if(file)
       {
           sprintf(tmp_path, "%s/USRDIR/settings.ini", self_path);
           SaveFile(tmp_path, (char *) file, file_size);
       }
    }
    else if(file_exists(tmp_path))
            file = LoadFile(tmp_path, &file_size);

    if(file)
    {
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "Covers", covers_path, MAXPATHLEN - 1, covers_path);
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "RetroCovers", retro_covers_path, MAXPATHLEN - 1, retro_covers_path);

        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "Background", backgrounds_path, MAXPATHLEN - 1, backgrounds_path);
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "Packages", updates_path, MAXPATHLEN - 1, updates_path);
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "Video", video_path, MAXPATHLEN - 1, video_path);
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "webMAN", webman_path, MAXPATHLEN - 1, webman_path);
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "PSPLauncher", psp_launcher_path, MAXPATHLEN - 1, psp_launcher_path);
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "RetroArch", retroarch_path, MAXPATHLEN - 1, retroarch_path);
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "PS2Classics", ps2classic_path, MAXPATHLEN - 1, ps2classic_path);
        getConfigMemValueString((char *) file, file_size, PATHS_SECTION, "PSXISO", psxiso_path, MAXPATHLEN - 1, psxiso_path);

        getConfigMemValueString((char *) file, file_size, EXTENSIONS_SECTION, "Video", video_extensions, 300, video_extensions);
        getConfigMemValueString((char *) file, file_size, EXTENSIONS_SECTION, "Audio", audio_extensions, 300, audio_extensions);
        getConfigMemValueString((char *) file, file_size, EXTENSIONS_SECTION, "Roms", rom_extensions, 300, rom_extensions);
        getConfigMemValueString((char *) file, file_size, EXTENSIONS_SECTION, "Browser", browser_extensions, 100, browser_extensions);
        getConfigMemValueString((char *) file, file_size, EXTENSIONS_SECTION, "Homebrews", custom_homebrews, 400, custom_homebrews);

#ifndef LOADER_MODE
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "ROMS", retro_root_path, ROMS_MAXPATHLEN - 1, retro_root_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "SNES", retro_snes_path, ROMS_MAXPATHLEN - 1, retro_snes_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "GBA", retro_gba_path, ROMS_MAXPATHLEN - 1, retro_gba_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "GEN", retro_gen_path, ROMS_MAXPATHLEN - 1, retro_gen_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "NES", retro_nes_path, ROMS_MAXPATHLEN - 1, retro_nes_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "MAME", retro_mame_path, ROMS_MAXPATHLEN - 1, retro_mame_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "FBA", retro_fba_path, ROMS_MAXPATHLEN - 1, retro_fba_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "DOOM", retro_doom_path, ROMS_MAXPATHLEN - 1, retro_doom_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "QUAKE", retro_quake_path, ROMS_MAXPATHLEN - 1, retro_quake_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "PCE", retro_pce_path, ROMS_MAXPATHLEN - 1, retro_pce_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "GB", retro_gb_path, ROMS_MAXPATHLEN - 1, retro_gb_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "GBC", retro_gbc_path, ROMS_MAXPATHLEN - 1, retro_gbc_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "ATARI", retro_atari_path, ROMS_MAXPATHLEN - 1, retro_atari_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "VB", retro_vb_path, ROMS_MAXPATHLEN - 1, retro_vb_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "NXE", retro_nxe_path, ROMS_MAXPATHLEN - 1, retro_nxe_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "WSWAN", retro_wswan_path, ROMS_MAXPATHLEN - 1, retro_wswan_path);

        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "A7800", retro_a7800_path, ROMS_MAXPATHLEN - 1, retro_a7800_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "LYNX", retro_lynx_path, ROMS_MAXPATHLEN - 1, retro_lynx_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "GW", retro_gw_path, ROMS_MAXPATHLEN - 1, retro_gw_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "VECTX", retro_vectrex_path, ROMS_MAXPATHLEN - 1, retro_vectrex_path);
        getConfigMemValueString((char *) file, file_size, ROMS_SECTION, "2048", retro_2048_path, ROMS_MAXPATHLEN - 1, retro_2048_path);

        max_roms = getConfigMemValueInt((char *) file, file_size, ROMS_SECTION, "MaxRoms", 80);

        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "AddToLastGameApp", AddToLastGameApp, 2, AddToLastGameApp);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "ShowPlayOverlay", ShowPlayOverlay, 2, ShowPlayOverlay);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "ShowUSBIcon", ShowUSBIcon, 2, ShowUSBIcon);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "BackgroundGears", BackgroundGears, 2, BackgroundGears);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "IconPulse", IconPulse, 2, IconPulse);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "FileManager", FileManager, 2, FileManager);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "TimeFormat", TimeFormat, 2, TimeFormat);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "UnmountDevBlind", UnmountDevBlind, 2, UnmountDevBlind);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "CachedGameList", CachedGameList, 2, CachedGameList);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "OptionsShowPIC1", ShowPIC1, 2, ShowPIC1);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "ShowVersion", ShowVersion, 2, ShowVersion);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "SpoofVersion", SpoofVersion, 2, SpoofVersion);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "HideCoverflowSortModeLabel", HideCoverflowSortModeLabel, 2, HideCoverflowSortModeLabel);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "TimeoutByInactivity", TimeoutByInactivity, 2, TimeoutByInactivity);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "SkipBDMirrorMsg", SkipBDMirrorMsg, 2, SkipBDMirrorMsg);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "FTPpassword", passwd, 63, passwd);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "LoadMambaAndQuit", LoadMambaAndQuit, 2, LoadMambaAndQuit);
        getConfigMemValueString((char *) file, file_size, GUI_SECTION, "InstallMamba", InstallMamba, 2, InstallMamba);
#endif

        ftp_serv = getConfigMemValueInt((char *) file, file_size, GUI_SECTION, "FTPserver", 0);
        ftp_port = getConfigMemValueInt((char *) file, file_size, GUI_SECTION, "FTPport", 21);

        default_psxoptions = getConfigMemValueInt((char *) file, file_size, GUI_SECTION, "DefaultPSXOptions", 1); //ps1_netemu

        CoverflowSize = getConfigMemValueInt((char *) file, file_size, GUI_SECTION, "CoverflowSize", 10);
        if(CoverflowSize < 3) CoverflowSize = 3;
        if(CoverflowSize > 24) CoverflowSize = 24;

        mod_volume = getConfigMemValueInt((char *) file, file_size, GUI_SECTION, "ModVolume", 6);
        if(mod_volume < 1) mod_volume = 0;
        if(mod_volume > 63) mod_volume = 63;

        bEnableLv2_memprot_patch  = getConfigMemValueInt((char *) file, file_size, "LV2", "memprot_patch" , 0);
        bEnableLv2_webman_patch  = getConfigMemValueInt((char *) file, file_size, "LV2", "webman_patch" , 1);
        bEnableLv2_habib_patch  = getConfigMemValueInt((char *) file, file_size, "LV2", "habib_patch" , 2);

        if(getConfigMemValueInt((char *) file, file_size, GUI_SECTION, "SwapButtons", (sys_button_layout == 0)))
        {
            BUTTON_CROSS_ = BUTTON_CIRCLE; //0x0020
            BUTTON_CIRCLE_ = BUTTON_CROSS; //0x0040
        }
        else
        {
            BUTTON_CROSS_ = BUTTON_CROSS;   //0x0040
            BUTTON_CIRCLE_ = BUTTON_CIRCLE; //0x0020
        }

        bSpoofVersion = strncmp(SpoofVersion, "0", 1);
        if(bSpoofVersion)
        {
            spoof_revision = getConfigMemValueInt((char *) file, file_size, GUI_SECTION, "spoof_revision" , spoof_revision);
            spoof_version  = getConfigMemValueInt((char *) file, file_size, GUI_SECTION, "spoof_version"  , spoof_version);
            if(spoof_version < 800) spoof_version = (((u32)(spoof_version/100)) * 0x100) + (((u32)((spoof_version % 100)/10)) * 0x10) + (spoof_version % 10); // convert to hex: 475 -> 0x475
        }

        free(file);

        bAddToLastGameApp = strncmp(AddToLastGameApp, "0", 1);
        bShowPlayOverlay = strncmp(ShowPlayOverlay, "0", 1);
        bShowUSBIcon = strncmp(ShowUSBIcon, "0", 1);
        bBackgroundGears = strncmp(BackgroundGears, "0", 1);
        bIconPulse = strncmp(IconPulse, "0", 1);
        bFileManager = strncmp(FileManager, "0", 1);
        bUnmountDevBlind = strncmp(UnmountDevBlind, "0", 1);
        bCachedGameList = strncmp(CachedGameList, "0", 1);
        bHideCoverflowSortModeLabel = strncmp(HideCoverflowSortModeLabel, "0", 1);
        bLoadMambaAndQuit = strncmp(LoadMambaAndQuit, "0", 1);
        bSkipBDMirrorMsg = strncmp(SkipBDMirrorMsg, "0", 1);

        install_mamba = strncmp(InstallMamba, "0", 1);

        if(!strncmp(ShowVersion, "0", 1)) bShowVersion = 0; else //Don't show
        if(!strncmp(ShowVersion, "1", 1)) bShowVersion = 1; else //Show for all PS3 games
                                          bShowVersion = 2;      //Show except for NTFS games

        if(!strncmp(ShowPIC1, "0", 1)) bShowPIC1 = 0; else //Don't show
        if(!strncmp(ShowPIC1, "1", 1)) bShowPIC1 = 1; else //Show for all PS3 games
                                       bShowPIC1 = 2;      //Show except for NTFS games

        if(!strncmp(TimeFormat, "0", 1)) iTimeFormat = 0; else //Hidden
        if(!strncmp(TimeFormat, "1", 1)) iTimeFormat = 1; else //24h
        if(!strncmp(TimeFormat, "2", 1)) iTimeFormat = 2; else //12h
        if(!strncmp(TimeFormat, "3", 1)) iTimeFormat = 3; else //Full date + time (24h)
                                         iTimeFormat = 4;      //Date + time (12h)

        if(!strncmp(TimeoutByInactivity, "1", 1)) iTimeoutByInactivity = 1; else //1hr
        if(!strncmp(TimeoutByInactivity, "2", 1)) iTimeoutByInactivity = 2; else //2hrs
        if(!strncmp(TimeoutByInactivity, "3", 1)) iTimeoutByInactivity = 3; else //3hrs
        if(!strncmp(TimeoutByInactivity, "4", 1)) iTimeoutByInactivity = 4; else //4hrs
        if(!strncmp(TimeoutByInactivity, "5", 1)) iTimeoutByInactivity = 5; else //5hrs
        if(!strncmp(TimeoutByInactivity, "6", 1)) iTimeoutByInactivity = 6; else //6hrs
        if(!strncmp(TimeoutByInactivity, "7", 1)) iTimeoutByInactivity = 7; else //7hrs
        if(!strncmp(TimeoutByInactivity, "8", 1)) iTimeoutByInactivity = 8; else //8hrs
                                                  iTimeoutByInactivity = 0;      //disabled
    }

    //supported file extensions
    strtoupper(video_extensions);
    strcat(video_extensions, " ");

    strtoupper(audio_extensions);
    strcat(audio_extensions, " ");

    strtoupper(rom_extensions);
    strcat(rom_extensions, " ");

    strtoupper(browser_extensions);
    strcat(browser_extensions, " ");

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
#else
    // reset default psp launcher (if don't exist)
    if(file_exists(psp_launcher_path) == false)
       strcpy(psp_launcher_path, "/dev_hdd0/game/PSPC66820");

    // reset default covers path (if don't exist)
    if(file_exists(covers_path) == false)
        strcpy(covers_path, "/dev_hdd0/GAMES/covers/");

    // reset default RetroArch path (if don't exist)
    if(file_exists(retroarch_path) == false)
        strcpy(retroarch_path, "/dev_hdd0/game/SSNE10000");

    mkdir_secure(covers_path);
    mkdir_secure(backgrounds_path);
    mkdir_secure(updates_path);
#endif
}

void clear_game_list()
{
    Png_offset[BIG_PICT] = 0;

    delete_entries(directories, &ndirectories, 0xFFFFFFFF);
    for(int n = 0; n < BIG_PICT; n++) {Png_iscover[n] = Png_offset[n] = 0; Png_index[n] = n;}

    ndirectories = currentgamedir = currentdir = select_px = select_py = 0;
}

//////

static char str_home[5][10] = {" Homebrew", "", " PS3", " Retro", " Videos"};
static char str_retro_mode[25][6] = {"", "PSX", "PS2", "PSP", "SNES", "GBA", "GEN", "NES", "MAME", "FBA", "Quake", "Doom", "PCE", "GBC", "Atari", "VBoy", "NXE", "WSwan", "A7800", "Lynx", "G&W", "VECTX", "2048", "", "NET"};

s32 main(s32 argc, const char* argv[])
{
    int n;

    u32 entry = 0;
    u32 segmentcount = 0;
    sysSpuSegment * segments; 

    if(lv2peek(0x80000000000004E8ULL)) syscall_40(1, 0); // disables PS3 Disc-less

    NTFS_init_system_io();

    event_threads_init();

    atexit(fun_exit);

    int must_patch = 0;

    if(lv2peek(0x80000000007EF220ULL) == 0x45737477616C6420ULL && is_payload_loaded())
    {
        must_patch = 1;

        sys8_path_table(0LL); // break libfs.sprx re-direction

        if(restore_syscall8[0]) sys8_pokeinstr(restore_syscall8[0], restore_syscall8[1]);
    }

    if(sysModuleLoad(SYSMODULE_FS)      == SUCCESS) inited|= INITED_FS;      else exit(0);
    if(sysModuleLoad(SYSMODULE_PNGDEC)  == SUCCESS) inited|= INITED_PNGDEC;  else exit(0);
    if(sysModuleLoad(SYSMODULE_JPGDEC)  == SUCCESS) inited|= INITED_JPGDEC;  else exit(0);
    if(sysModuleLoad(SYSMODULE_IO)      == SUCCESS) inited|= INITED_IO;      else exit(0);
    if(sysModuleLoad(SYSMODULE_GCM_SYS) == SUCCESS) inited|= INITED_GCM_SYS; else exit(0);
    if(sysModuleLoad(SYSMODULE_HTTPS)   == SUCCESS) inited|= INITED_HTTPS;   else exit(0);

    sysModuleLoad(SYSMODULE_SYSUTIL);
    sysSpuInitialize(6, 5);

    sysSpuRawCreate(&spu, NULL);

    sysSpuElfGetInformation(spu_soundmodule_bin, &entry, &segmentcount);

    size_t segmentsize = sizeof(sysSpuSegment) * segmentcount;
    segments = (sysSpuSegment*)memalign(128, SPU_SIZE(segmentsize)); // must be aligned to 128 or it break malloc() allocations
    memset(segments, 0, segmentsize);

    sysSpuElfGetSegments(spu_soundmodule_bin, segments, segmentcount);

    sysSpuImageImport(&spu_image, spu_soundmodule_bin, 0);

    sysSpuRawImageLoad(spu, &spu_image);

    inited |= INITED_SPU;

    if(SND_Init(spu) == SUCCESS) inited |= INITED_SOUNDLIB;

    // register exit callback
    if(sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, sys_callback, NULL)==0) snd_inited |= INITED_CALLBACK;
    if(SND_Init(spu)==0) snd_inited |= INITED_SOUNDLIB;
    SND_Pause(0);

    if(argc > 0 && argv)
    {
        int n;

        if(!strncmp(argv[0], "/dev_hdd0/game/", 15))
        {
            strcpy(self_path, argv[0]);

            n = 15; while(self_path[n] != '/' && self_path[n] != 0) n++;

            if(self_path[n] == '/') self_path[n] = 0;
        }
        else if(!strncmp(argv[0], "/dev_bdvd/PS3_GAME/", 19))
        {
            strcpy(self_path, "/dev_bdvd/PS3_GAME");
        }
    }

    if (strstr(self_path, "/BLES80608")) sprintf(self_path, "/dev_hdd0/game/IRISMAN00");

    #include "main_payload.h"

    // ADVERTENCIA: este codigo no debe ser removido de aqui para evitar pantalla negra por los parches (problemas de cache)
    // CAUTION: this code should not be removed from here to avoid black screen caused by the patches (e.g. cache problems)
    tiny3d_Init(1024*1024);
    ioPadInit(7);
    usleep(250000);

/*
    if(sys8_disable_all != 0 && !unsupported_cfw)
    {
         if(DrawDialogYesNo2("Syscall is very old or not detected\n\nDo you want to REBOOT the PS3?\n\n(Select NO to exit to XMB)") == YES)
         {
             set_install_pkg = true;
             game_cfg.direct_boot = 0;
             fun_exit();
         }
         else
             exit_to_xmb();
    }
*/
/*
    if(is_ps3hen && !file_exists((char*)"/dev_usb000") && !file_exists((char*)"/dev_hdd1"))
	{
		if(file_exists((char*)"/dev_usb001"))
		{
			sys_fs_mount("CELL_FS_UTILITY:HDD1", "CELL_FS_FAT", "/dev_usb000", 1);
			{lv2syscall2(35, (u64)(char*)"/dev_usb000", (u64)(char*)"/dev_usb001");}
		}
		else
		if(file_exists((char*)"/dev_usb002"))
		{
			sys_fs_mount("CELL_FS_UTILITY:HDD1", "CELL_FS_FAT", "/dev_usb000", 1);
			{lv2syscall2(35, (u64)(char*)"/dev_usb000", (u64)(char*)"/dev_usb002");}
		}
		else
		if(file_exists((char*)"/dev_usb003"))
		{
			sys_fs_mount("CELL_FS_UTILITY:HDD1", "CELL_FS_FAT", "/dev_usb000", 1);
			{lv2syscall2(35, (u64)(char*)"/dev_usb000", (u64)(char*)"/dev_usb003");}
		}
		else
		if(file_exists((char*)"/dev_usb006"))
		{
			sys_fs_mount("CELL_FS_UTILITY:HDD1", "CELL_FS_FAT", "/dev_usb000", 1);
			{lv2syscall2(35, (u64)(char*)"/dev_usb000", (u64)(char*)"/dev_usb006");}
		}
	}
*/
    if(!use_mamba)
    {
        if(must_patch) sys8_pokeinstr(0x80000000007EF220ULL, 0x0ULL);

        // disable ps1 emulation
        unload_psx_payload();

        {
            FILE *fp = fopen("/dev_hdd0/game/HOMELAUN1/path.bin", "rb");
            if(fp)
            {
                fclose(fp); homelaun = 1;
                fp = fopen("/dev_hdd0/game/HOMELAUN1/path2.bin", "rb");
                if(fp) {fclose(fp); homelaun = 2;}
            }
        }

        // turn off
        sys8_perm_mode(1);
        usleep(5000);


        // sys8_perm_mode(0);
        sys8_path_table(0LL);


        if(lv2_patch_storage)
        {   // for PSX games
            if(lv2_patch_storage() < 0) lv2_patch_storage = NULL;
        }
    }
    else
        lv2_patch_storage = lv2_unpatch_storage = NULL;


    if(payload_mode < ZERO_PAYLOAD) //if mode is wanin or worse, launch advert
    {
        DrawDialogOK(temp_buffer);
    }

    if(use_cobra || use_mamba)
    {
        do_umount_iso();
        wm_sys_storage_ext_umount_discfile();
        cobra_unset_psp_umd();

        sys_map_path((char*)"/dev_bdvd", NULL);
        sys_map_path((char*)"/app_home", NULL);
        sys_map_path((char*)"//dev_bdvd", NULL);

        // Unmount game through webMAN
        if(get_net_status() == SUCCESS) call_webman("/mount.ps3/unmount");
    }

    show_http_errors = true;

    // spoof firmware
    if(use_cobra && !use_mamba)
    {
        u8 cconfig[15];
        CobraConfig *cobra_config = (CobraConfig*) cconfig;
        memset(cobra_config, 0, 15);
        cobra_read_config(cobra_config);

        cobra_config->spoof_version  = 0;
        cobra_config->spoof_revision = 0;

        if(bSpoofVersion && (firmware == 0x446C || firmware == 0x453C || firmware == 0x453D || firmware == 0x455C || firmware == 0x460C))
        {
            cobra_config->spoof_version  = spoof_version;
            cobra_config->spoof_revision = spoof_revision;
        }

        if(cobra_config->ps2softemu == 0 && cobra_get_ps2_emu_type() == PS2_EMU_SW)
            cobra_config->ps2softemu = 1;

        cobra_write_config(cobra_config);
    }

    if(use_cobra)
    {
        bAllowNetGames = get_vsh_plugin_slot_by_name("WWWD") > 0;

        if(!bAllowNetGames)
        {
            int slot = get_vsh_plugin_free_slot();

            if(file_exists(webman_path)) bAllowNetGames = (cobra_load_vsh_plugin(slot, webman_path, NULL, 0x0) == 0);
        }
    }

#ifndef LOADER_MODE
    sprintf(tmp_path, "%s/config", self_path);
    mkdir_secure(tmp_path);

    sprintf(tmp_path, "%s/cache", self_path);
    mkdir_secure(tmp_path);

    //sprintf(temp_buffer, "%s/PKG", self_path);
    if(strlen(updates_path)) mkdir_secure(updates_path);

    // copy SHOWTIME.SELF from multiMAN (if a local copy does not exist)
    sprintf(tmp_path, "%s/USRDIR/SHOWTIME.SELF", self_path);
    if(file_exists(tmp_path)==false)
    {
        sysLv2FsLink("/dev_hdd0/game/BLES80608/USRDIR/sys/SHOWTIME.SELF", tmp_path);
    }

    // auto upgrade
    if(strstr(self_path, "/dev_hdd0/game/IRISMAN00")==NULL && strstr(self_path, "/dev_hdd0/game/BLES80610")==NULL)
    {
        sprintf(tmp_path, "%s/USRDIR/RELOAD.SELF", self_path);
        sysLv2FsLink("/dev_hdd0/game/IRISMAN00/USRDIR/RELOAD.SELF", tmp_path);
    }
#endif

    // initialize manager conf
    memset(&manager_cfg, 0, sizeof(manager_cfg));

    for(n = 0; n < 4; n++)
        manager_cfg.videoscale_x[n] = 1024;

    if((firmware & 0xF) == 0xD)
        sprintf(tmp_path, "%s/explore_plugin_%xdex.sprx", self_path, firmware>>4);
    else
        sprintf(tmp_path, "%s/explore_plugin_%x.sprx", self_path, firmware>>4);

    manager_cfg.noBDVD = (use_cobra || use_mamba || file_exists(tmp_path)) ? MODE_DISCLESS : MODE_WITHBDVD; //set default to DISCLESS only for Cobra/Mamba firmwares
    manager_cfg.language = SYSTEM_LANGUAGE;
    manager_cfg.background_sel = 0;

    if(!unsupported_cfw)
    {
        // get default console id
        get_console_id_eid5();
        strcpy(default_console_id, console_id);

        get_psid_lv2();
        strcpy(default_psid, psid);

        // spoof console id (idps)/psid
        load_spoofed_console_id();
        load_spoofed_psid();
    }

    // load cfg and language strings
    LoadManagerCfg();

    noBDVD = manager_cfg.noBDVD;
    gui_mode = manager_cfg.gui_mode & 0xF;

    sort_mode = 0;
    if(gui_mode == MODE_COVERFLOW) sort_mode = (manager_cfg.gui_mode>>4);

    get_grid_dimensions(false);

    if(noBDVD == MODE_NOBDVD || unsupported_cfw)
    {
        use_cobra = false;
        use_mamba = false;
    }

    if(!unsupported_cfw)
    {
        // load ps3 disc less payload
        if(noBDVD == MODE_DISCLESS && !use_cobra && !use_mamba)
        {
            //lv2poke(syscall_base +(u64) (40 * 8), lv2peek(syscall_base));
            load_ps3_discless_payload();

            if(firmware == 0x341C || firmware == 0x355C)
                syscall_40(6, 0x8d000B04);
            /*else if(firmware == 0x421C)
                syscall_40(6, 0x8d001A04);*/
            else if(firmware != 0x430C && firmware != 0x431C && firmware != 0x440C && manager_cfg.event_flag)
                syscall_40(6, manager_cfg.event_flag);
        }

        load_controlfan_config();

        set_device_wakeup_mode(0);
    }

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
    sleep(1);
#else
    LoadPSXOptions(NULL);

    // Load texture
    LoadTexture();

    if (file_exists(psxiso_path))
    {
        videoscale_x = videoscale_y = -120;
        strncpy(manager_cfg.hdd_folder, "GAMES", 64);
    }
    else
    {
        init_twat();

        if(videoscale_x >= 1024)
        {
            videoscale_x = videoscale_y = 0;
            video_adjust();
        }

        if(manager_cfg.hdd_folder[0] == 0)
        {
            Select_games_folder();

            if(manager_cfg.hdd_folder[0] == 0) strcpy(manager_cfg.hdd_folder, __MKDEF_MANAGER_DIR__);
            SaveManagerCfg();
        }
    }

    strncpy(hdd_folder, manager_cfg.hdd_folder, 64);
#endif

    double sx = (double) Video_Resolution.width;
    double sy = (double) Video_Resolution.height;
    double psx = (double) (1000 + videoscale_x)/1000.0;
    double psy = (double) (1000 + videoscale_y)/1000.0;

    tiny3d_UserViewport(1,
        (float) ((sx - sx * psx) / 2.0), // 2D position
        (float) ((sy - sy * psy) / 2.0),
        (float) ((sx * psx) / 848.0),    // 2D scale
        (float) ((sy * psy) / 512.0),
        (float) ((sx / 1920.0) * psx),  // 3D scale
        (float) ((sy / 1080.0) * psy));

    if(use_mamba && !use_cobra)
    {
        syscall_40(1, 0); // disables PS3 Disc-less / load mamba

        fun_exit();

        restore_syscall8[1]= lv2peek(restore_syscall8[0]); // use mamba vector

        // relaunch iris manager to get vsh process for mamba
        sprintf(tmp_path, "%s/USRDIR/RELOAD.SELF", self_path);
        sysProcessExitSpawn2(tmp_path, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
        exit(0);
    }

    // disable cobra and mamba flags if "no BDVD device" mode is set
    if(noBDVD == MODE_NOBDVD)
    {
        use_cobra = false;
        use_mamba = false;
    }
    else
    {
        // from reload, use_mamba is zero: this code detect if mamba is present and set to 1
        if(sys8_mamba() == 0x666) use_mamba = true;
    }

    currentdir = select_px = select_py = 0;

    fdevices = 0;
    fdevices_old = 0;
    forcedevices = 0;
    find_device = 0;
    app_ver[0] = 0;

    init_music(-1);

    if(!unsupported_cfw)
    {
        //syscall36("/dev_bdvd");
        add_sys8_bdvd(NULL, NULL);

        //sys8_perm_mode((u64) 0);

        unpatch_bdvdemu();

        if(noBDVD && !use_cobra /*&& !is_ps3hen*/)
        {
            //sys_fs_umount("/dev_ps2disc");
            sys_fs_umount("/dev_bdvd");
            sleep(0);
            sys_fs_mount("CELL_FS_UTILITY:HDD1", "CELL_FS_FAT", "/dev_bdvd", 1);
        }

        //sys_fs_umount("/dev_bdvd");
        //    sleep(0);
        //sys_fs_mount("CELL_FS_IOS:BDVD_DRIVE", "CELL_FS_ISO9660", "/dev_bdvd", 1);

        // eject disc with cobra method
        if(noBDVD && use_cobra)
        {
            cobra_send_fake_disc_eject_event();
            cobra_umount_disc_image();
            cobra_unload_vsh_plugin(0); // unload ISO plugin in slot 0 (reserved)
        }

        if((noBDVD == MODE_WITHBDVD) && lv2_patch_storage)
        {
            if(!bdvd_notify)
            {
                DIR  *dir;
                dir = opendir("/dev_bdvd");

                if (dir)
                    closedir(dir);
                else if(firmware < 0x421C)
                    Reset1_BDVD();
                else
                    Reset2_BDVD();
            }

            bdvd_notify = 1;
        }



        sysDiscRegisterDiscChangeCallback(&DiscEjectCallback, &DiscInsertCallback);

        if(bUnmountDevBlind)
        {
            sys_fs_umount("/dev_blind");
            sys_fs_umount("/dev_habib");
            sys_fs_umount("/dev_rebug");
            sys_fs_umount("/dev_rewrite");
        }

        sys_fs_umount("/dev_moo");

        if(lv2peek(0x80000000000004E8ULL) && noBDVD == MODE_DISCLESS && !use_cobra)
        {
            syscall_40(1, 2); // PS3 Disc-less
        }

        if(noBDVD == MODE_DISCLESS && !use_cobra && syscall_40(3, 0) == 0)
        {
            DrawDialogTimer(language[PLUG_STORAGE1], 2000.0f);
        }
    }

#ifndef LOADER_MODE
    if(use_cobra)
    {
        struct stat s;
        sprintf(tmp_path, PLUGIN_ISO, self_path);
        if(stat(tmp_path, &s) != SUCCESS || s.st_size != SIZE_SPRX_ISO)
            SaveFile(tmp_path, (char *) sprx_iso, SIZE_SPRX_ISO);
    }
#endif

#ifdef LASTPLAY_LOADER
    char *mem = NULL;
    int file_size = 0;

    sprintf(tmp_path, "%s/USRDIR/LASTPLAY.BIN", self_path);
    if(file_exists(tmp_path))
        mem = LoadFile(tmp_path, &file_size);

    if(mem)
    {
        memset(temp_buffer, 0, 8192);
        strcpy(temp_buffer, "[SETTINGS]\n");
        strncat(temp_buffer, mem, file_size);
        file_size+=12;

        if(mem) free(mem);

        ndirectories = 1;
        currentgamedir = currentdir = select_px = select_py = 0;

        getConfigMemValueString((char *) temp_buffer, file_size, "SETTINGS", "TITLEID", directories[currentgamedir].title_id, 10, directories[currentgamedir].title_id);
        getConfigMemValueString((char *) temp_buffer, file_size, "SETTINGS", "SELF", directories[currentgamedir].path_name, MAXPATHLEN, directories[currentgamedir].path_name);
        if(!strcasestr(directories[currentgamedir].path_name, ".iso"))
            getConfigMemValueString((char *) temp_buffer, file_size, "SETTINGS", "PATH", directories[currentgamedir].path_name, MAXPATHLEN, directories[currentgamedir].path_name);

        int flag = -1;
        if(strncmp(directories[currentgamedir].path_name, "/dev_hdd0", 9))
            {directories[currentgamedir].flags = D_FLAG_HDD0; flag = 0;}
        else if(strncmp(directories[currentgamedir].path_name, "/dev_usb", 8))
            {directories[currentgamedir].flags = D_FLAG_USB; flag = (directories[currentgamedir].path_name[10] - '/');}
        else if(strncmp(directories[currentgamedir].path_name, "/ntfs", 5))
            directories[currentgamedir].flags = D_FLAG_NTFS;
        else if(strncmp(directories[currentgamedir].path_name, "/ext", 4))
            directories[currentgamedir].flags = D_FLAG_NTFS;

        if(strstr(directories[currentgamedir].path_name, "/PS3ISO"))
            directories[currentgamedir].flags |= PS3_FLAG;
        if(strstr(directories[currentgamedir].path_name, "/PSXISO"))
            directories[currentgamedir].flags |= PS1_FLAG;
        if(strstr(directories[currentgamedir].path_name, "/PS2ISO"))
            directories[currentgamedir].flags |= PS2_FLAG;
        if(strstr(directories[currentgamedir].path_name, "/PSPISO"))
            directories[currentgamedir].flags |= PSP_FLAG;

        if(flag >= 0) move_bdemubackup_to_origin(flag);

        last_game_flag = directories[currentgamedir].flags;

        if(!(strcasestr(directories[currentgamedir].path_name, ".iso") && file_exists("/dev_hdd0/game/IRISMAN00/sprx_iso") == false))
            strcpy(self_path, "/dev_hdd0/game/IRISMAN00");

        if(strstr(directories[currentgamedir].path_name, "/ntfs"))
        {
            {
                // NTFS Automount
                for(int i = 0; i < 8; i++)
                {
                    {   // mount device
                        NTFS_UnMount(i);
                        mounts[i] = NULL;
                        mountCount[i] = 0;

                        mountCount[i] = ntfsMountDevice(disc_ntfs[i], &mounts[i], NTFS_DEFAULT | NTFS_RECOVER);

                        if(autolaunch == LAUNCHMODE_CHECKED)
                        {
                            autolaunch = LAUNCHMODE_REFRESH;
                        }
                    }
                }
                // NTFS Automount
            }

            // NTFS/EXTx
            if(/*noBDVD == MODE_DISCLESS &&*/ use_cobra && mode_homebrew != HOMEBREW_MODE)
            {
                u32 ports_plug_cnt = 0;
                signal_ntfs_mount = false;
                for(find_device = 0; find_device < 8; find_device++)
                {
                    if(automountCount[find_device] > 0) signal_ntfs_mount = true;
                    if(mountCount[find_device]) ports_plug_cnt|= 1<<find_device; else ports_plug_cnt&= ~(1<<find_device);
                }
            }
        }

        mode_favourites = 0;
        menu_screen = SCR_MAIN_GAME_LIST;
        autolaunch = LAUNCHMODE_STARTED;
        load_gamecfg(autolaunch);
        gui_control();

        exit_to_xmb();
    }
#endif

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)

#else
    if(sys_parental_level > 0 && sys_parental_level < 9) options_locked = true;

    u32 old_ntfs_ports = 0;

    // Launch PSX iso stored on ./USRDIR/ps1_iso
    if (autolaunch == LAUNCHMODE_TOCHECK)
    {
        float x = 0.0f, y = 0.0f;

        memcpy(filename, psxiso_path, MAXPATHLEN);
        if (fill_iso_entries_from_device(psxiso_path, PS1_FLAG, directories, &ndirectories, 0) > 0)
        {
            sort_entries2(directories, &ndirectories, sort_mode);

            autolaunch = LAUNCHMODE_STARTED;
            directories[autolaunch].flags = PS1_FLAG;
            gui_mode = 1;

            // Replace ISO with Shadow Copy
            sprintf(psxiso_path, "/dev_hdd0/PSXISO/%s", (char *) get_filename(directories[autolaunch].path_name));
            if (file_exists(psxiso_path)==false)
            {
                sprintf(psxiso_path, "/dev_hdd0/PSXISO [auto]/%s", (char *) get_filename(directories[autolaunch].path_name));
            }
            if (file_exists(psxiso_path))
            {
                CopyFile(psxiso_path, directories[autolaunch].path_name);
            }
            //

            ps3pad_read();

            if ((new_pad | old_pad) & (BUTTON_L1 | BUTTON_R1))
            {
                frame_count = 32;
                menu_screen = SCR_MENU_PSX_OPTIONS;

                while (menu_screen != SCR_MAIN_GAME_LIST)
                {
                    flash = (frame_count >> 5) & 1;
                    frame_count++;
                    if(frame_count>1000) frame_count = 0;

                    cls();

                    update_twat(false);

                    switch(menu_screen)
                    {
                        case SCR_MENU_PSX_OPTIONS:
                            draw_psx_options(x, y, autolaunch);
                            break;
                        case SCR_MENU_PSX_VIDEO_OPTIONS:
                            draw_psx_options2(x, y, autolaunch);
                            break;
                        default:
                            menu_screen = SCR_MAIN_GAME_LIST;
                            break;
                    }

                    ps3pad_read();
                }
            }

            if (!((new_pad | old_pad) & BUTTON_SELECT))

            {
                if (strstr(directories[autolaunch].path_name, "/PSXISO") != NULL)
                    launch_iso_game(directories[autolaunch].path_name, EMU_PSX);
                else
                {
                    reset_sys8_path_table();

                    //syscall36("/dev_bdvd");
                    add_sys8_bdvd(NULL, NULL);

                    if(lv2peek(0x80000000000004E8ULL) && !use_cobra) syscall_40(1, 0); // disables PS3 Disc-less

                    // load PSX options
                    LoadPSXOptions(filename);

                    if(psx_iso_prepare(filename, directories[autolaunch].title, NULL) == 0)  exit_to_xmb();

                    psx_launch();
                }

                exit_to_xmb();
            }
        }
    }

    //// Load Favorites
    sprintf(tmp_path, "%s/config/", self_path);
    LoadFavourites(tmp_path, GAMEBASE_MODE);

    sprintf(tmp_path, "%s/config/", self_path);
    LoadFavourites(tmp_path, HOMEBREW_MODE);

    sprintf(tmp_path, "%s/config/", self_path);
    LoadFavourites(tmp_path, VIDEOS_MODE);

    GetFavourites(mode_homebrew);
    //

    //// Load Background Picture
    load_background_picture();
    //

    //// Launch File Manager / Showtime
    bool bLaunchShowtime = false;

    ps3pad_read();


    if((new_pad | old_pad) & (BUTTON_R1)) {bCachedGameList = false;}

    if (bFileManager || (new_pad | old_pad) & (BUTTON_L2 | BUTTON_R2))
    {
        autolaunch = LAUNCHMODE_CHECKED;

        ps3pad_read();
        if(((new_pad | old_pad) & (BUTTON_L2 | BUTTON_R2)) == (BUTTON_L2 | BUTTON_R2))
            bLaunchShowtime = true;
        else
        {
            if(get_net_status() == SUCCESS) auto_ftp();
            file_manager(NULL, NULL);
            if(bFileManager) exit_to_xmb();
        }

        frame_count = 0xFF; // force display temp
    }
    else if((new_pad | old_pad) & (BUTTON_L1 | BUTTON_CROSS_))
    {
        if(get_last_selected_game(true) == 1)
        {
            mode_favourites = 0;
            menu_screen = SCR_MAIN_GAME_LIST;
            autolaunch = LAUNCHMODE_STARTED;
            load_gamecfg(autolaunch);
            gui_control();
        }
    }

    // Load Cached Game List
    LoadGameList();

    if(bLaunchShowtime)
    {
        get_last_selected_game(false);

        launch_showtime(false);
    }

    // remove temporary file items
    sprintf(tmp_path, "%s/USRDIR/TEMP/SHOWTIME.TXT", self_path);
    unlink_secure(tmp_path);

    sprintf(tmp_path, "%s/USRDIR/TEMP/pkg.iso", self_path);
    unlink_secure(tmp_path);

    sprintf(tmp_path, "%s/USRDIR/TEMP/showtime.iso", self_path);
    unlink_secure(tmp_path);

    sprintf(tmp_path, "%s/USRDIR/temp.html", self_path);
    unlink_secure(tmp_path);

    sprintf(tmp_path, "%s/USRDIR/app.lua", self_path);
    unlink_secure(tmp_path);

    sprintf(tmp_path, "%s/USRDIR/TEMP", self_path);
    rmdir_secure(tmp_path);
    //

    // get estimated texture used
    //sprintf(temp_buffer, "Textures used: %u bytes", text_size);
    //DrawDialogOK(temp_buffer);

    if(use_mamba && bLoadMambaAndQuit) exit_program = true;


    //////////////////////////////////////////
    // Main loop
    /////////////////////////////////////////
    while(!exit_program)
    {
        float x = 0.0f, y = 0.0f;

        flash = (frame_count >> 5) & 1;

        frame_count++;

        int count_devices = 0;

        if(tiny3d_MenuActive()) frame_count = 32; // to avoid the access to hdd when menu is active

        // NTFS Automount
        for(int i = 0; i < 8; i++)
        {
            int r = NTFS_Event_Mount(i);

            if(r == NTFS_DEVICE_MOUNT)
            {   // mount device
                NTFS_UnMount(i);
                mounts[i] = NULL;
                mountCount[i] = 0; ntfs_retry = MAX_NTFS_RETRY;

                mountCount[i] = ntfsMountDevice(disc_ntfs[i], &mounts[i], NTFS_DEFAULT | NTFS_RECOVER);

                if(autolaunch == LAUNCHMODE_CHECKED)
                {
                    SaveLastGame();
                    autolaunch = LAUNCHMODE_REFRESH;
                    frame_count = 0;
                    allow_restore_last_game = true;
                }
            }
            else if(r == NTFS_DEVICE_UNMOUNT && mounts[i] != NULL)
            { // unmount device
               NTFS_UnMount(i);
               mounts[i] = NULL;
               mountCount[i] = 0; ntfs_retry = MAX_NTFS_RETRY;
            }
        }
        // NTFS Automount

        int signal_force = (fdevices == 0); scan_canceled = false;

        if(cached_game_list) {forcedevices = 0; fdevices_old = fdevices; signal_force = false; frame_count = 300; goto skip_refresh;}


        if(forcedevices || (frame_count & 63) == 0 || signal_force)
        {

          if(refresh_game_list && (game_list_category == GAME_LIST_NETHOST && retro_mode == NET_GAMES)) {parse_mygames_xml(); refresh_game_list = false; bSkipParseXML = true; cached_game_list = true; bCachedGameList = false;}

          for(find_device = HDD0_DEVICE; find_device <= BDVD_DEVICE; find_device++)
          {
            if(filter_by_device >= HDD0_DEVICE)
            {
                bSkipParseXML = true;
                if(find_device || filter_by_device > BDVD_DEVICE) {fdevices_old = fdevices; break;}

                find_device = filter_by_device;
            }

            if(find_device == HDD0_DEVICE)
                strcpy(filename, "/dev_hdd0");
            else if(find_device == BDVD_DEVICE)
                strcpy(filename, "/dev_bdvd");
/*
            else if(is_ps3hen)
            {
                if(find_device > 1) continue;
                sprintf(filename, "/dev_usb000");
            }
*/
			else
                sprintf(filename, "/dev_usb00%c", 47 + find_device);


            if((!forcedevices || (fdevices & D_FLAG_BDVD)) && find_device == BDVD_DEVICE && bdvd_notify == 0) goto skip_bdvd;

            sysFSStat dstat;
            int ret;

            if(find_device == BDVD_DEVICE && psx_inserted < 0)
                ret = FAILED;
            else
                ret = sysLv2FsStat(filename, &dstat);

            if (ret == SUCCESS)
            {

                // check bdemu
                if((fdevices & (1<<find_device)) == 0 && find_device >= HDD0_DEVICE && find_device < BDVD_DEVICE)
                    move_bdemubackup_to_origin(1 << find_device);

                if(fdevices == 0xff) {fdevices_old = fdevices = 0;}

                fdevices|= 1<<find_device;

                // check psx
                if(find_device == BDVD_DEVICE)
                {
                    if((noBDVD == MODE_WITHBDVD) && (get_psx_region_cd() & 0x10) == 0x10) {psx_inserted |= 0x100;}
                    else
                    {
                        ret = sysLv2FsStat("/dev_bdvd/PS3_GAME", &dstat);
                        if(ret != SUCCESS) {psx_inserted = 0; fdevices ^= 1<<find_device;}
                    }
                }

                if(find_device == BDVD_DEVICE && get_disc_ready()) bdvd_notify = 0;
            }
            else
            {
                // check psx
                if(find_device == BDVD_DEVICE && (noBDVD == MODE_WITHBDVD) && (get_psx_region_cd() & 0x10) == 0x10)
                    {bdvd_notify = 0;fdevices|= 1<<find_device; psx_inserted |= 0x100;}
                else
                {
                    if(find_device == BDVD_DEVICE) psx_inserted = 0;
                    fdevices&= ~ (1<<find_device);
                }
            }

skip_bdvd:
            // limit to 3 the devices selectables
            if(((fdevices>>find_device) & 1) && find_device != BDVD_DEVICE)
            {
                count_devices++;

               if(count_devices > 3) fdevices&= ~ (1<<find_device);
            }

            // bdvd
            if(find_device == BDVD_DEVICE)
            {
                if(((fdevices>>BDVD_DEVICE) != (fdevices_old>>BDVD_DEVICE) || ((forcedevices>>find_device) & 1)))
                {
                    struct stat s;
                    currentdir = 0;

                    // detect psx code
                    if((noBDVD == MODE_WITHBDVD) &&
                       (stat("/dev_bdvd/PSX.EXE", &s) == SUCCESS || stat("/dev_bdvd/SYSTEM.CNF", &s) == SUCCESS || (psx_inserted & 0x100) == 0x100))
                    {
                        psx_inserted &= ~0x100;
                        psx_inserted |= 0x80;
                        strncpy(bluray_game, "PSX-GAME", 64);
                        bdvd_ejected = false;
                        if((noBDVD == MODE_WITHBDVD) && lv2_patch_storage)
                        {
                            psx_inserted|= get_psx_region_cd();
                            strncpy(bluray_game, (char *) psx_id, 64);
                            mode_favourites = 0;
                            menu_screen = SCR_MAIN_GAME_LIST;
                            select_option = 0;
                            select_px = select_py = 0;
                        }
                        bluray_game[63] = 0;
                    }
                    else
                    {
                        strcpy(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");
                        bluray_game[0] = 0;
                        // whatever... unused if -- remove later?
                        if(parse_param_sfo("/dev_bdvd/PS3_GAME/PARAM.SFO", bluray_game) == -1);
                        bluray_game[63] = 0;
                    }


                    if(((fdevices>>BDVD_DEVICE) & 1)  && (mode_homebrew == GAMEBASE_MODE) && (noBDVD == MODE_WITHBDVD))
                    {
                        if(ndirectories >= MAX_DIRECTORIES) ndirectories = MAX_DIRECTORIES - 1;

                        strcpy(directories[ndirectories].path_name, "/dev_bdvd");

                        memcpy(directories[ndirectories].title, bluray_game, 63);
                        directories[ndirectories].title[63] = 0;
                        directories[ndirectories].flags = BDVD_FLAG | ((psx_inserted & 0xff)<<16);
                        if(!psx_inserted)
                        {
                            sprintf(filename, "%s/%s", directories[ndirectories].path_name, "PS3_GAME/PARAM.SFO");
                            if(parse_param_sfo_id(filename, directories[ndirectories].title_id) == FAILED)
                            {
                                sprintf(filename, "%s/%s", directories[ndirectories].path_name, "PS3_DISC.SFB");
                                parse_ps3_disc((char *) filename, directories[ndirectories].title_id);
                            }

                            directories[ndirectories].title_id[63] = 0;
                        }
                        else
                            strncpy(directories[ndirectories].title_id, "PSX-GAME", 64);

                        ndirectories++;
                        found_game_insert = true;

                        s32 fdr;

                        if((noBDVD == MODE_WITHBDVD) && !sysLv2FsOpen("/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN", 0, &fdr, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
                        {
                            u64 bytes;
                            u32 dat;
                            bdvd_ejected = false;
                            static int counter = 0;
                            if(sysLv2FsRead(fdr, (void *) &dat, 4, &bytes) != SUCCESS) bytes = 0LL;
                            sysLv2FsClose(fdr);

                            if(bytes == 4 && dat != 0x53434500)
                            {
                                if(counter == 0)
                                {
                                    counter = 1;
                                    if((noBDVD == MODE_WITHBDVD) && lv2_patch_storage)
                                    {
                                        bdvd_notify = 0;
                                        Eject_BDVD(EJECT_BDVD);
                                        Eject_BDVD(NOWAIT_BDVD | LOAD_BDVD);
                                    }
                                    else DrawDialogOK("Warning! You must eject/load the disc");
                                }
                                else DrawDialogOK("Warning! Disc authentication failed");

                                ndirectories--;

                                psx_inserted = 0;
                                fdevices &= ~ BDVD_FLAG;
                                goto skip_bdvd;
                            }
                            else
                            {
                                counter = 0;
                                if(!sys_ss_media_id(temp_buffer + 2048))
                                {
                                    sprintf(tmp_path, "%s/config/%s.did", self_path, directories[ndirectories - 1].title_id);
                                    if(file_exists(tmp_path) == false || s.st_size != 0x10)
                                        SaveFile(tmp_path, (char *) temp_buffer + 2048, 0x10);
                                }
                            }
                        }
                    }
                    else
                    {
                        int nd = ndirectories;
                        delete_entries(directories, &ndirectories, BDVD_FLAG);
                        if(ndirectories < nd) found_game_remove = true;
                    }

                    sort_entries2(directories, &ndirectories, sort_mode);
                }

                forcedevices &= ~ (1<<find_device);
                fdevices_old &= ~ (1<<find_device);
                fdevices_old |= fdevices & (1<<find_device);
            }
            else
            ////////////////////////////////////////
            //        refresh game list           //
            ////////////////////////////////////////
            if(fdevices != fdevices_old || ((forcedevices>>find_device) & 1))
            {
                roms_count = 0;
                //currentdir = 0;
                //found_game_insert = true;
                SaveLastGame();

                forcedevices &= ~ (1<<find_device);

                if(find_device == HDD0_DEVICE)
                {
                    int nd = ndirectories;

                    if(ndirectories > 0) ; // skip scanning message
                    else if(game_list_category == GAME_LIST_ALL)
                        DrawDialogTimer(language[GAMELIST_SCANNING0], 1000.0f);
                    else if(game_list_category == GAME_LIST_PS3_ONLY && mode_homebrew == GAMEBASE_MODE)
                        DrawDialogTimer(language[GAMELIST_SCANNING1], 1000.0f);
                    else if(game_list_category == GAME_LIST_PS3_ONLY && mode_homebrew == VIDEOS_MODE)
                        DrawDialogTimer(language[GAMELIST_SCANNING3], 1000.0f);
                    else if (game_list_category == GAME_LIST_RETRO && retro_mode == RETRO_PSALL)
                        DrawDialogTimer(language[GAMELIST_SCANNING2], 1000.0f);
                    else if (game_list_category == GAME_LIST_NETHOST && retro_mode == NET_GAMES)
                        DrawDialogTimer(language[GAMELIST_SCANNING5], 1000.0f);
                    else if(game_list_category == GAME_LIST_RETRO)
                    {
                        sprintf(temp_buffer, "%s %s", language[GAMELIST_SCANNING4], &str_retro_mode[retro_mode][0]);
                        DrawDialogTimer(temp_buffer, 1000.0f);
                    }

                    // List net_host games from /dev_hdd0/xmlhost/game_plugin/mygames.xml
                    if (bSkipParseXML || bAllowNetGames == false || mode_homebrew || filter_by_device >= HDD0_DEVICE)
                        bSkipParseXML = false;
                    else if(game_list_category == GAME_LIST_ALL || (game_list_category == GAME_LIST_NETHOST && retro_mode == NET_GAMES))
                        parse_mygames_xml();

                    if(mode_homebrew)
                        sprintf(filename, "/dev_hdd0/game");
                    else if (!memcmp(hdd_folder, "dev_hdd0", 9))
                        sprintf(filename, "/%s/" __MKDEF_GAMES_DIR, hdd_folder);
                    else if (!memcmp(hdd_folder, "GAMES", 6) || !memcmp(hdd_folder, "dev_hdd0_2", 11))
                        sprintf(filename, "/dev_hdd0/GAMES");
                    else if (!memcmp(hdd_folder, "host_root", 10))
                        sprintf(filename, "/host_root");
                    else if (!memcmp(hdd_folder, "video", 6))
                        sprintf(filename, "/dev_hdd0/video");
                    else if (strstr(hdd_folder, "/"))
                        sprintf(filename, hdd_folder);
                    else
                        sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR, hdd_folder);

                    sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
                    freeSpace[find_device] = ( ((u64)blockSize * freeSize));
                    freeSpace[find_device] = (float) freeSpace[find_device] / GIGABYTES;

                    if(show_custom_icons) add_custom_icons(directories, &ndirectories);

                    if(ndirectories > nd) found_game_insert = true;
                }
                else
                {
                    sprintf(filename, "/dev_usb00%c/", 47 + find_device);
                    sysFsGetFreeSize(filename, &blockSize, &freeSize);
                    double space = ( ((double)blockSize) * ((double) freeSize) ) /  GIGABYTES;
                    freeSpace[find_device] = (float) space;
                    if(mode_homebrew == GAMEBASE_MODE)
                        sprintf(filename, "/dev_usb00%c/GAMES", 47 + find_device);
                    else
                        sprintf(filename, "/dev_usb00%c/game", 47 + find_device);
                }

                // BDISO, DVDISO, MKV
                if(mode_homebrew == VIDEOS_MODE /*&& use_cobra && noBDVD == MODE_DISCLESS*/
                                                && find_device == HDD0_DEVICE && ((fdevices>>find_device) & 1))
                {
                    // isos BR-DVD
                    char file[0x420];

                    int n, nd = ndirectories;

                    delete_entries(directories, &ndirectories, (1<<find_device));

                    if(ndirectories < nd) found_game_remove = true;

                    nd = ndirectories;

                    strcpy(file, "/dev_bdvd"); n = 9;
                    fill_directory_entries_with_alt_path(file, n, "/.", "/.", directories, &ndirectories, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | (1<<find_device));

                    strncpy(file, filename, 0x420);
                    n = 1; while(file[n] != '/' && file[n] != 0) n++;

                    fill_directory_entries_with_alt_path(file, n, "/BDISO" , "/BDISO [auto]" , directories, &ndirectories, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | (1<<find_device));
                    fill_directory_entries_with_alt_path(file, n, "/DVDISO", "/DVDISO [auto]", directories, &ndirectories, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | (1<<find_device));
                    fill_directory_entries_with_alt_path(file, n, "/VIDEO", "/MOVIES", directories, &ndirectories, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | (1<<find_device));

                    fill_directory_entries_with_alt_path(file, n, "/game/RXMOV", "/game/RXMOV", directories, &ndirectories, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | (1<<find_device));

                    file[n] = 0; strcat(file, video_path);
                    fill_iso_entries_from_device(file, D_FLAG_HOMEB | D_FLAG_HOMEB_MKV | (1<<find_device), directories, &ndirectories, 0);

                    if(ndirectories > nd) found_game_insert = true;
                }
                else if(((fdevices>>find_device) & 1) && mode_homebrew == GAMEBASE_MODE)
                {
                    int nd = ndirectories;

                    u32 flag = (1<<find_device) | ((mode_homebrew == GAMEBASE_MODE) ? PS3_FOLDER_FLAG : HOMEBREW_FLAG);

                    if(fill_entries_from_device(filename, directories, &ndirectories, flag, mode_homebrew, false) == SUCCESS)
                    {
                        //append gamez, games_dup, games_bad
                        if((mode_homebrew == GAMEBASE_MODE) || (mode_homebrew && find_device != 0))
                        {
                            if(find_device == 0)
                            {
                                sprintf(filename, "/dev_hdd0/GAMES [auto]");
                                fill_entries_from_device(filename, directories, &ndirectories, flag, mode_homebrew, true);

                                sprintf(filename, "/dev_hdd0/GAMEZ [auto]");
                                fill_entries_from_device(filename, directories, &ndirectories, flag, mode_homebrew, true);
                            }

                            sprintf(filename, "/dev_usb00%c/GAMES [auto]", 47 + find_device);
                            fill_entries_from_device(filename, directories, &ndirectories, flag, mode_homebrew, true);

                            sprintf(filename, "/dev_usb00%c/GAMEZ [auto]", 47 + find_device);
                            fill_entries_from_device(filename, directories, &ndirectories, flag, mode_homebrew, true);

                            sprintf(filename, "/dev_usb00%c/GAMEZ", 47 + find_device);
                            if (fill_entries_from_device(filename, directories, &ndirectories, flag, mode_homebrew, true) == SUCCESS)
                            {
                                sprintf(filename, "/dev_usb00%c/GAMES_DUP", 47 + find_device);
                                if (fill_entries_from_device(filename, directories, &ndirectories, flag, mode_homebrew, true) == SUCCESS)
                                {
                                    sprintf(filename, "/dev_usb00%c/GAMES_BAD", 47 + find_device);
                                    fill_entries_from_device(filename, directories, &ndirectories, flag, mode_homebrew, true);
                                }
                            }
                        }
                    }

                    if(ndirectories > nd) found_game_insert = true;
                }
                else
                {
                    int nd = ndirectories;

                    delete_entries(directories, &ndirectories, (1<<find_device));

                    if(ndirectories < nd) found_game_remove = true;
                }

                sort_entries2(directories, &ndirectories, sort_mode);

                fdevices_old&= ~ (1<<find_device);
                fdevices_old|= fdevices & (1<<find_device);
            }
          }
        }

        // NTFS/EXTx
        if(/*noBDVD == MODE_DISCLESS &&*/ use_cobra && mode_homebrew != HOMEBREW_MODE)
        {
            u32 ports_plug_cnt = 0;
            signal_ntfs_mount = false;
            for(find_device = 0; find_device < 8; find_device++)
            {
                if(automountCount[find_device] > 0) signal_ntfs_mount = true;
                if(mountCount[find_device]) ports_plug_cnt|= 1<<find_device; else ports_plug_cnt&= ~(1<<find_device);
            }

            if(force_ntfs_mount || old_ntfs_ports != ports_plug_cnt || forcedevices || (ndirectories == 0 && ntfs_retry > 0) || (signal_force && filter_by_device == LIST_ALL_DEVICES))
            {
                force_ntfs_mount=false; if(ntfs_retry>0) ntfs_retry--;

                if(filter_by_device > BDVD_DEVICE) forcedevices = 0;

                //if(filter_by_device <= BDVD_DEVICE || old_ntfs_ports == 0 || ndirectories == 0)
                {
                    if(old_ntfs_ports == ports_plug_cnt) old_ntfs_ports = 0;
                    if(delete_entries(directories, &ndirectories, NTFS_FLAG)) found_game_remove = true;
                }

                if( automountCount[0] == 0 && automountCount[1] == 0 && automountCount[2] == 0 && automountCount[3] == 0 &&
                    automountCount[4] == 0 && automountCount[5] == 0 && automountCount[6] == 0 && automountCount[7] == 0)
                {
                    for(find_device = 0; find_device < 8; find_device++)
                    {
                        if(find_device == 0 && filter_by_device >= HDD0_DEVICE)
                        {
                            if(filter_by_device > BDVD_DEVICE)
                            {
                                if(find_device) {fdevices_old = fdevices; break;}

                                clear_game_list();

                                found_game_remove = true;

                                find_device = filter_by_device - 12;
                            }
                            else {fdevices_old = fdevices; break;}
                        }

                        if(mounts[find_device])
                        {
                            int k;
                            for (k = 0; k < mountCount[find_device]; k++)
                            {
                                if((mounts[find_device]+k)->name[0])
                                {
                                    int nd = ndirectories;

                                    if(mode_homebrew == GAMEBASE_MODE)
                                    {
                                        if(game_list_category == GAME_LIST_PS3_ONLY || game_list_category == GAME_LIST_ALL)
                                        {
                                            sprintf(filename, "/%s:/PS3ISO", (mounts[find_device]+k)->name);
                                            fill_iso_entries_from_device(filename, NTFS_FLAG, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                            sprintf(filename, "/%s:/PS3ISO [auto]", (mounts[find_device]+k)->name);
                                            fill_iso_entries_from_device(filename, NTFS_FLAG, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);
                                        }

                                        if(mode_homebrew == GAMEBASE_MODE  && (game_list_category == GAME_LIST_RETRO || game_list_category == GAME_LIST_ALL))
                                        {
                                            if(retro_mode == RETRO_ALL || retro_mode == RETRO_PSX || retro_mode == RETRO_PSALL)
                                            {
                                                sprintf(filename, "/%s:/PSXISO", (mounts[find_device]+k)->name);
                                                fill_iso_entries_from_device(filename, PS1_FLAG | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                                sprintf(filename, "/%s:/PSXISO [auto]", (mounts[find_device]+k)->name);
                                                fill_iso_entries_from_device(filename, PS1_FLAG | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                                sprintf(filename, "/%s:/PSXGAMES", (mounts[find_device]+k)->name);
                                                fill_psx_iso_entries_from_device(filename, PS1_FLAG | D_FLAG_NTFS, directories, &ndirectories);
                                            }

                                            if(retro_mode == RETRO_ALL || retro_mode == RETRO_PS2 || retro_mode == RETRO_PSALL)
                                            {
                                                sprintf(filename, "/%s:%s", (mounts[find_device]+k)->name, ps2classic_path);
                                                fill_psx_iso_entries_from_device(filename, PS2_CLASSIC_FLAG | D_FLAG_NTFS, directories, &ndirectories);
                                            }

                                            //RETRO
                                            char cfg_path[MAXPATHLEN];
                                            sprintf(cfg_path, "%s/USRDIR/cores", self_path);

                                            if(roms_count < max_roms && file_exists(cfg_path))
                                            {
                                                int n = sprintf(filename, "/%s:", (mounts[find_device]+k)->name);

                                                sprintf(cfg_path, "%s/USRDIR/cores/snes-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_SNES) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_snes_path, "/ROMS/snes", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/gba-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_GBA) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_gba_path, "/ROMS/vba", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/gen-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_GEN) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_gen_path, "/ROMS/gen", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/nes-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_NES) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_nes_path, "/ROMS/fceu", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/mame-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_MAME) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_mame_path, "/ROMS/mame", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/fba-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_FBA) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_fba_path, "/ROMS/fba", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/quake-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_QUAKE) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_quake_path, "/ROMS/pak", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/doom-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_DOOM) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_doom_path, "/ROMS/pak", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/pce-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_PCE) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_pce_path, "/ROMS/pce", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/gbc-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_GBC) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_gbc_path, "/ROMS/gbc", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);

                                                    if(roms_count < max_roms)
                                                    {
                                                        fill_directory_entries_with_alt_path(filename, n, retro_gb_path, "/ROMS/gb", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                    }
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/atari-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_ATARI) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_atari_path, "/ROMS/atari", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/vb-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_VBOY) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_vb_path, "/ROMS/vboy", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/nxe-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_NXE) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_nxe_path, "/ROMS/nxe", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/wswan-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_WSWAN) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_wswan_path, "/ROMS/wsw", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/a7800-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_A7800) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_a7800_path, "/ROMS/a7800", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/lynx-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_LYNX) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_lynx_path, "/ROMS/lynx", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/gw-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_GW) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_gw_path, "/ROMS/gw", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/vectrex-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_VECTX) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_vectrex_path, "/ROMS/vectrex", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                sprintf(cfg_path, "%s/USRDIR/cores/2048-retroarch.cfg", self_path);
                                                if(roms_count < max_roms && (retro_mode == RETRO_ALL || retro_mode == RETRO_2048) && file_exists(cfg_path))
                                                {
                                                    fill_directory_entries_with_alt_path(filename, n, retro_2048_path, "/ROMS/2048", directories, &ndirectories, RETRO_FLAG | D_FLAG_NTFS);
                                                }

                                                if(roms_count) roms_count = max_roms;
                                            }
                                        }
                                    }
                                    else if(mode_homebrew == VIDEOS_MODE)
                                    {
                                        sprintf(filename, "/%s:/BDISO", (mounts[find_device]+k)->name);
                                        fill_iso_entries_from_device(filename, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                        sprintf(filename, "/%s:/BDISO [auto]", (mounts[find_device]+k)->name);
                                        fill_iso_entries_from_device(filename, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                        sprintf(filename, "/%s:/DVDISO", (mounts[find_device]+k)->name);
                                        fill_iso_entries_from_device(filename, D_FLAG_HOMEB | D_FLAG_HOMEB_DVD | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                        sprintf(filename, "/%s:/DVDISO [auto]", (mounts[find_device]+k)->name);
                                        fill_iso_entries_from_device(filename, D_FLAG_HOMEB | D_FLAG_HOMEB_DVD | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                        sprintf(filename, "/%s:/VIDEO", (mounts[find_device]+k)->name);
                                        fill_iso_entries_from_device(filename, D_FLAG_HOMEB | D_FLAG_HOMEB_MKV | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                        sprintf(filename, "/%s:/MOVIES", (mounts[find_device]+k)->name);
                                        fill_iso_entries_from_device(filename, D_FLAG_HOMEB | D_FLAG_HOMEB_MKV | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);

                                        sprintf(filename, "/%s:%s", (mounts[find_device]+k)->name, video_path);
                                        fill_iso_entries_from_device(filename, D_FLAG_HOMEB | D_FLAG_HOMEB_MKV | D_FLAG_NTFS, directories, &ndirectories, (mounts[find_device]+k)->interface->ioType);
                                    }


                                    if(ndirectories > nd) allow_save_lastgame = found_game_insert = true;
                                }
                            }
                        }
                    }

                    if(found_game_insert)
                        sort_entries2(directories, &ndirectories, sort_mode);

                    old_ntfs_ports = ports_plug_cnt;
                }
            }
        }
        //-- endif (noBDVD == MODE_DISCLESS && use_cobra)

skip_refresh:

        if(cached_game_list)
        {
           found_game_insert = true;
           cached_game_list = false;
        }

        if(found_game_insert || found_game_remove)
        {
           get_last_selected_game(false);
           UpdateFavourites(directories, ndirectories);

           if(mode_favourites && !havefavourites) mode_favourites = 0;

           get_grid_dimensions(true);

           get_games();
           load_gamecfg(RESET_GAME_INFO); // force refresh game info

           mode_favourites = mode_favourites != 0; // avoid insert favourites

           found_game_remove = false;
           found_game_insert = false;
        }

        if(save_game_list_after_refresh)
        {
            allow_save_lastgame = true;
            save_game_list_after_refresh = false;
            SaveGameList();
        }

        pause_music(0);

        /////////////////////////////////////

        if(autolaunch == LAUNCHMODE_TOCHECK)
        {
            get_last_selected_game(false);

            ps3pad_read();
            if((new_pad | old_pad) & (BUTTON_L1 | BUTTON_CROSS_ | BUTTON_R1)) {autolaunch = currentgamedir; gui_control(); exit_to_xmb();}

            autolaunch = LAUNCHMODE_CHECKED;
        }

        /////////////////////////////////////


//        x = (848 - 640) / 2; y = (512 - 360) / 2;
//        DrawBox(x - 16, y - 16, 65535.0f, 640.0f + 32, 360 + 32, BLACK);
//        DrawBox(x, y, 65535.0f, 640.0f, 360, 0x30003018);


        x = 28; y = 0;

        if((old_pad & (BUTTON_L2 | BUTTON_R2 | BUTTON_START)) == (BUTTON_L2 | BUTTON_R2 | BUTTON_START))
        {
            videoscale_x = videoscale_y = 0;
            video_adjust();
        }

        // paranoic checks
        if(select_px < 0 || select_px > (cols - 1)) select_px = 0;
        if(select_py < 0 || select_py > (rows - 1)) select_py = 0;

        if(currentdir < 0 || currentdir >= ndirectories) currentdir = 0;
        if(currentgamedir < 0 || currentgamedir >= ndirectories) currentgamedir = 0;

        // paranoic favourite check
        if(mode_favourites)
        {
            for(n = 0; n < num_box; n++)
            {
                if(favourites.list[n].index >= 0)
                {
                    if(favourites.list[n].title_id[0] == 0) exit_to_xmb();
                    if(favourites.list[n].index >= ndirectories) exit_to_xmb();
                    if(directories[favourites.list[n].index].flags == 0) exit_to_xmb();
                }
            }
            for (; n < MAX_FAVORITES; n++) favourites.list[n].index = -1;
        }

        // fake disc insertion
        if(noBDVD == MODE_DISCLESS && disc_less_on == 1 && !use_cobra && syscall_40(3, 0) == 1)
        {
            int u;

            s16 * sound = memalign(32, (cricket_raw_bin_size + 31) & ~31);

            if(sound)
            {
                memset((void *) sound, 0, (cricket_raw_bin_size + 31) & ~31);
                memcpy((void *) sound, (void *)  cricket_raw_bin + 1, cricket_raw_bin_size - 1);

                SND_SetVoice(1, VOICE_MONO_16BIT, 22050, 0, sound, cricket_raw_bin_size - 1, 0x40, 0x20, NULL);
                SND_SetVoice(1, VOICE_MONO_16BIT, 22050, 1000, sound, cricket_raw_bin_size - 1, 0x20, 0x40, NULL);
            }

            for(u = 0; u < 190; u++)
            {
                cls();

                tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                      Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                      TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                update_twat(true);
                DrawTextBox((848 - 300)/2,(512 - 300)/2, 0, 300, 300, ((u >= 64) ? 0xff : u<<2) | 0xffffff00);
                SetCurrentFont(FONT_TTF);
                SetFontColor(((u & 16)) ? BLACK : WHITE, BLACK);

                SetFontSize(32, 48);

                SetFontAutoCenter(1);
                DrawString(0, 512 - 75,  language[PLUG_STORAGE2]);
                SetFontAutoCenter(0);
                tiny3d_Flip();
                ps3pad_read();
            }

            if(sound) free(sound);

            if(!use_cobra)
            {
                u32 eid = (u32) syscall_40(4, 0);

                if(eid != SUCCESS)
                {
                    eid -= 0x100;
                    manager_cfg.event_flag = eid;
                    SaveManagerCfg();
                }
            }

            cls();
            update_twat(true);
            disc_less_on = 2;

            if(new_pad & BUTTON_CROSS_) new_pad ^= BUTTON_CROSS_;
            if(new_pad & BUTTON_CIRCLE_) new_pad ^= BUTTON_CIRCLE_;
        }

        if(ndirectories < 1) gui_control();

        if(gui_mode == MODE_COVERFLOW)
            cls0();
        else
        {
            cls();
            update_twat(true);
        }

        if(do_once && frame_count>33) {do_once = false; get_games();}

        switch(menu_screen)
        {
            case SCR_MAIN_GAME_LIST:
                if(gui_mode == MODE_COVERFLOW) {if(gui_control() == SUCCESS) draw_coverflow(x, y);}
                else                           {if(gui_control() == SUCCESS) draw_grid(x, y);}
                break;
            case SCR_MENU_GAME_OPTIONS:
                draw_options(x, y, currentgamedir);
                break;
            case SCR_MENU_GAME_OPTIONS_CONFIG:
                draw_configs(x, y, currentgamedir);
                break;
            case SCR_MENU_ISO_OPTIONS:
                draw_iso_options(x, y, currentgamedir);
                break;
            case SCR_MENU_GLOBAL_OPTIONS:
                draw_gbloptions(x, y);
                break;
            case SCR_MENU_TOOLS:
                draw_toolsoptions(x, y);
                break;
            case SCR_TOOL_DELETE_CACHE:
                draw_cachesel(x, y);
                break;
            case SCR_MENU_GAME_LIST:
                draw_gamelist_options(x, y);
                break;
            case SCR_MENU_CONSOLE_ID_TOOLS:
                draw_console_id_tools(x, y);
                break;
            case SCR_MENU_PSX_OPTIONS:
                draw_psx_options(x, y, currentgamedir);
                break;
            case SCR_MENU_PSX_VIDEO_OPTIONS:
                draw_psx_options2(x, y, currentgamedir);
                break;
            case SCR_TOOL_BUILD_ISO:
                draw_device_mkiso(x, y, currentgamedir, NULL);
                break;
            case SCR_TOOL_EXTRACT_ISO:
                draw_device_xtiso(x, y, currentgamedir);
                break;
            case SCR_TOOL_COPY_ISO:
                draw_device_cpyiso(x, y, currentgamedir);
                break;
            default:
                menu_screen = SCR_MAIN_GAME_LIST;
                break;
        }

        auto_ftp(); // auto enable the ftp

        test_audio_file(false);
    }

#endif

    fun_exit();
    return SUCCESS;
}

void load_background_picture()
{
    if(bk_picture)
    {
        if(bk_picture == BG_RANDOM || bk_picture == BG_PIC1 || gui_mode == MODE_XMB_LIKE)
        {
          int retry = 0, bg;
          srand(time(0)); // randomize seed
          while(retry < 30)
          {
            bg = rand() % 15;
            // load background picture
            sprintf(tmp_path, "%sPICT%i.JPG", backgrounds_path, bg);
            if(LoadTextureJPG(tmp_path, BACKGROUND_PICT) == SUCCESS) break;
            if(LoadTexturePNG(tmp_path, BACKGROUND_PICT) == SUCCESS) break;
            Png_offset[BACKGROUND_PICT] = 0;
            retry++;
            set_usleep_sm_main(100000);
          }
        }
        else if(bk_picture > BG_PIC1)
        {
          // load background picture
          sprintf(tmp_path, "%sPICT%i.JPG", backgrounds_path, bk_picture - 3);
          if(LoadTextureJPG(tmp_path, BACKGROUND_PICT) == SUCCESS) return;
          if(LoadTexturePNG(tmp_path, BACKGROUND_PICT) == SUCCESS) return;
          {Png_offset[BACKGROUND_PICT] = 0; bk_picture = BG_NONE;}
        }
    }
    else
        Png_offset[BACKGROUND_PICT] = 0;

}

// draw_cachesel
struct {
    u64 size;
    char title[64];
    char title_id[64];
} cache_list[64];

int ncache_list = 0;

void LoadCacheDatas()
{
    DIR  *dir, *dir2;

    sprintf(tmp_path, "%s/cache", self_path);
    dir = opendir(tmp_path);
    if(!dir) return;

    sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
    freeSpace[0] = ((u64)blockSize * freeSize);
    freeSpace[0] = (float) freeSpace[0] / GIGABYTES;

    ncache_list = 0;

    while(true)
    {
        struct dirent *entry = readdir(dir);

        if(!entry) break;
        if(entry->d_name[0] == '.' && (entry->d_name[1] == 0 || entry->d_name[1] == '.')) continue;

        if(!(entry->d_type & DT_DIR)) continue;

        strncpy(cache_list[ncache_list].title_id, entry->d_name, 64);
        cache_list[ncache_list].title_id[63] = 0;

        cache_list[ncache_list].size = 0ULL;

        int size;
        sprintf(tmp_path, "%s/cache/%s/name_entry", self_path, entry->d_name);
        char *name = LoadFile(tmp_path, &size);

        memset(cache_list[ncache_list].title, 0, 64);
        if(name)
        {
            memcpy(cache_list[ncache_list].title, name, (size < 64) ? size : 63);
            free(name);
        }

        sprintf(tmp_path, "%s/cache/%s", self_path, entry->d_name);
        dir2 = opendir(tmp_path);
        if(dir2)
        {
            while(true)
            {
                struct dirent *entry2 = readdir(dir2);
                struct stat s;

                if(!entry2) break;
                if(entry2->d_name[0] == '.' && (entry2->d_name[1] == 0 || entry2->d_name[1] == '.')) continue;

                if((entry2->d_type & DT_DIR)) continue;

                sprintf(tmp_path, "%s/cache/%s/%s", self_path, entry->d_name, entry2->d_name);
                if(stat(tmp_path, &s) == 0)
                    cache_list[ncache_list].size += s.st_size;
            }
            closedir(dir2);
        }

        ncache_list++; if(ncache_list >= 64) break;
    }
    closedir(dir);
}

inline int get_currentdir(int i)
{
    if(mode_favourites)
        return favourites.list[i].index;

    return (currentdir + i);
}

inline int get_int_currentdir(int i)
{
    if(mode_favourites)
        return favourites.list[i].index;

    return (int_currentdir + i);
}

void load_gamecfg(int current_dir)
{
    static int last_selected = -1;

    char path_file[0x420];

    if(current_dir < 0) //check reset info
    {
        bShowPath = false;
        memset(&game_cfg, 0, sizeof(game_cfg));
        last_selected = -1;
        return;
    }

    if(last_selected == current_dir) return;

    last_selected = current_dir; //prevents load again
    bShowPath = false;

    memset(&game_cfg, 0, sizeof(game_cfg));

    if((directories[current_dir].flags & PS1_FLAG) == PS1_FLAG) return; //ps1/ps2/retro/psp/ps2_classic_flag

    sprintf(path_file, "%s/config/%s.cfg", self_path, directories[current_dir].title_id);

    int file_size;
    char *file = LoadFile(path_file, &file_size);
    if(file)
    {
        if(file_size > sizeof(game_cfg)) file_size = sizeof(game_cfg);
        memcpy(&game_cfg, file, file_size);
        free(file);
    }
    else
    {
        sprintf(path_file, "%s/PS3_GAME/PS3GAME.INI", directories[current_dir].path_name);
        char *file = LoadFile(path_file, &file_size);
        if(file)
        {
            if(file_size == 12 && strncmp(file, "MMGI0100", 8) == SUCCESS)
            {
                game_cfg.useBDVD       = (file[11] & (1<<0)) >> 0;
                game_cfg.direct_boot   = (file[11] & (1<<5)) >> 5;
                game_cfg.exthdd0emu    = (file[10] & (1<<1)) >> 1;

                if(strncmp(directories[current_dir].path_name, "/dev_hdd0", 9))
                    game_cfg.bdemu_ext = (file[11] & (1<<6)) >> 6;
                else
                    game_cfg.bdemu     = (file[11] & (1<<6)) >> 6;
            }
            free(file);
        }
    }

    if(game_cfg.direct_boot > 3) game_cfg.direct_boot = 3;
}


int save_gamecfg()
{
    // update multiMAN game config
    sprintf(tmp_path, "%s/PS3_GAME/PS3GAME.INI", directories[currentgamedir].path_name);
    int file_size = 0;
    char *file = LoadFile(tmp_path, &file_size);
    if(file)
    {
        if(file_size == 12 && strncmp(file, "MMGI0100", 8) == SUCCESS)
        {
            file[11] = (file[11] & 0x9E) | game_cfg.useBDVD |
                                         ((game_cfg.direct_boot > 0) ? (1<<5) : 0) |
                                         ((game_cfg.bdemu       > 0) ? (1<<6) : 0) |
                                         ((game_cfg.bdemu_ext   > 0) ? (1<<6) : 0);

            file[10] = (file[10] & 0xFD) | (game_cfg.exthdd0emu<<1);
            SaveFile(tmp_path, file, file_size);
        }
        free(file);
    }

    // save IRISMAN game config
    sprintf(tmp_path, "%s/config/%s.cfg", self_path, directories[currentgamedir].title_id);

    return (SaveFile(tmp_path, (char *) &game_cfg, sizeof(game_cfg)));
}

int check_disc(void)
{
    int get_user = 1;

    while(get_user == 1)
    {
        DIR  *dir;
        dir = opendir("/dev_bdvd");

        if(dir)
        {
            closedir(dir);
            return 1;
        }
        else
            get_user = DrawDialogYesNo(language[DRAWSCREEN_REQBR]);
    }

    return -1;
}

void mount_custom(char *path)
{
    int size, n;
    char * mem;
    sprintf(tmp_path, "%s/ps2disc.txt", path);

    mem = LoadFile(tmp_path, &size);

    if(mem)
    {
        n = 0; while(mem[n] >= 32) n++;

        mem[n] = 0;
        add_sys8_path_table("/dev_ps2disc", mem);
        free(mem);
    }

}

static u8 BdId[0x10];

void set_BdId(int index)
{
    struct stat s;
    int readed;
    int ret = SUCCESS;
    void *mem;


    if(strcmp(directories[index].title_id, NETHOST) == SUCCESS) return;


    if(use_cobra || !lv2peek(0x80000000000004E8ULL)) return;

    memset(BdId, 0, 0x10);

    ret = FAILED;

    sprintf(tmp_path, "%s/BDMEDIA_ID", directories[index].path_name);
    if(stat(tmp_path, &s) == SUCCESS && s.st_size == 0x10)
    {
        mem = LoadFile((void *) tmp_path, &readed);
        if(!mem || readed != 0x10) {if(mem) free(mem); ret = FAILED;}
        else {ret = SUCCESS; memcpy(BdId, mem, 0x10); free(mem);}
    }


    sprintf(temp_buffer, "%s/config/%s.did", self_path, directories[index].title_id);
    if(ret != SUCCESS && stat(temp_buffer, &s) == 0 && s.st_size == 0x10)
    {
        mem = LoadFile((void *) temp_buffer, &readed);
        if(!mem || readed != 0x10) {if(mem) free(mem); ret = FAILED;}
        else
        {
            ret = SUCCESS; memcpy(BdId, mem, 0x10);
            SaveFile(tmp_path, (char *) BdId, 0x10);
            free(mem);
        }
    }

    if(!use_cobra) syscall_40(7, (u64) (BdId)); // set BD Media Id
}

void get_grid_dimensions(bool update_coords)
{
    if(gui_mode == MODE_XMB_LIKE)  {cols = 1; rows = 1;} else // XMB-Like
    if(gui_mode == MODE_COVERFLOW) {cols = 4; rows = 3;} else // Coverflow
    if(gui_mode == MODE_GRID_3x2)  {cols = 3; rows = 2;} else // Grid 3x2
    if(gui_mode == MODE_GRID_4x2)  {cols = 4; rows = 2;} else // Grid 4x2
    if(gui_mode == MODE_GRID_3x3)  {cols = 3; rows = 3;} else // Grid 3x3
    if(gui_mode == MODE_GRID_4x3)  {cols = 4; rows = 3;} else // Grid 4x3
    if(gui_mode == MODE_GRID_5x3)  {cols = 5; rows = 3;} else // Grid 5x3
    if(gui_mode == MODE_GRID_6x3)  {cols = 6; rows = 3;} else // Grid 6x3
    if(gui_mode == MODE_GRID_4x4)  {cols = 4; rows = 4;} else // Grid 4x4
    if(gui_mode == MODE_GRID_5x4)  {cols = 5; rows = 4;} else // Grid 5x4
    if(gui_mode == MODE_GRID_6x4)  {cols = 6; rows = 4;} else // Grid 6x4
    if(gui_mode == MODE_GRID_5x5)  {cols = 5; rows = 5;} else // Grid 5x5
    if(gui_mode == MODE_GRID_6x5)  {cols = 6; rows = 5;} else // Grid 6x5
    if(gui_mode == MODE_GRID_7x6)  {cols = 7; rows = 6;} else // Grid 7x6
    if(gui_mode == MODE_GRID_8x5)  {cols = 8; rows = 5;} else // Grid 8x5
    if(gui_mode == MODE_GRID_8x6)  {cols = 8; rows = 6;} else // Grid 8x6
                                   {cols = 4; rows = 3;}      // Grid 4x3

    num_box = cols * rows;

    if(!update_coords) return;

    if(gui_mode == MODE_COVERFLOW)
    {
       select_py = 0;
       if (currentgamedir >= 3)
           select_px = 3;
       else
           select_px = currentgamedir;

       currentdir = currentgamedir - select_px;
    }
    else if(num_box)
    {
       currentdir = ((int) (currentgamedir / num_box)) * num_box;

       select_py = (currentgamedir % num_box) / cols;
       select_px = currentgamedir - currentdir - (select_py * cols);
    }
    else
        currentdir = select_px = select_py = 0;
}

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
void draw_grid(float x, float y) {}
void draw_coverflow(float x, float y) {}
void return_to_game_list(bool update) {}
void draw_background_pic1() {}
void enable_draw_background_pic1() {}
static int anim_mode = 0, anim_step = 0;
#ifdef LOADER_MODE
int move_origin_to_bdemubackup(char *path) {return FAILED;}
#endif
static int list_box_devices[16];
static int max_list_box_devices = 0;
#else
void draw_grid(float x, float y)
{
    int i, n, m, ii = 0;

    float x2;

    int str_type = (mode_homebrew != GAMEBASE_MODE) ? ((mode_homebrew == HOMEBREW_MODE) ? 0 : 4) : 1 + game_list_category;

    int selected = select_px + select_py * cols;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 18, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    if(mode_favourites >= 0x20000)
        DrawFormatString(x, y, " %s%s", language[DRAWSCREEN_FAVSWAP], &str_home[str_type][0]);
    else if(mode_favourites >= 0x10000)
        DrawFormatString(x, y, " %s%s", language[DRAWSCREEN_FAVINSERT], &str_home[str_type][0]);
    else if(mode_favourites)
        DrawFormatString(x, y, " %s%s", language[DRAWSCREEN_FAVORITES], &str_home[str_type][0]);
    else if(str_type == 4)
        DrawFormatString(x, y, " %s %i/%i (%i %s)", language[DRAWSCREEN_PAGE], currentdir / num_box + 1, ROUND_UPX(ndirectories) / num_box,
                                                    ndirectories, &str_home[str_type][0]);
    else if(str_type == 3)
    {
        if(retro_mode == NET_GAMES) str_home[str_type][0] = 0;

        DrawFormatString(x, y, " %i/%i %s%s %s", (selected + currentdir) < ndirectories ? selected + currentdir + 1 : 0, ndirectories, language[DRAWSCREEN_GAMES], &str_home[str_type][0], &str_retro_mode[retro_mode][0]);
    }
    else if(gui_mode == MODE_XMB_LIKE)
        DrawFormatString(x, y, " %i/%i %s%s", currentdir + 1, ndirectories, language[DRAWSCREEN_GAMES], &str_home[str_type][0]);
    else
        DrawFormatString(x, y, " %s %i/%i (%i %s)%s", language[DRAWSCREEN_PAGE], currentdir / num_box + 1, ROUND_UPX(ndirectories) / num_box,
                                                      ndirectories, language[DRAWSCREEN_GAMES], &str_home[str_type][0]);

    // list device space

    m = selected;

    if(Png_offset[m])
    {

        i = -1;

        if(!mode_favourites || ((mode_favourites != 0) && favourites.list[m].index >= 0))
        {
            for(i = HDD0_DEVICE; i < BDVD_DEVICE; i++)
                if((directories[(mode_favourites != 0) ? favourites.list[m].index : (currentdir + m)].flags & GAMELIST_FILTER) == (1<<i)) break;

            if(i == BDVD_DEVICE)
                if((directories[(mode_favourites != 0) ? favourites.list[m].index : (currentdir + m)].flags & GAMELIST_FILTER) == NTFS_FLAG)
                {
                     i = (ISO_FLAGS);
                     strcpy(tmp_path, directories[(mode_favourites != 0) ? favourites.list[m].index : (currentdir + m)].path_name);
                }
        }
        m = i;
    } else m = -1;

    x2 = 1200;
    for(n = 0; n < 2; n++)
    {
        if(m == (ISO_FLAGS))
        {
            char *p = (char *) tmp_path + 1; while(*p && *p != ':') p++; if(*p == ':') p[1] = 0;
            SetFontColor(0xafd836ff, BLACK);
            x2 = DrawFormatString(x2, 0, "%s .ISO", (void *) (tmp_path + 1));
        }
        else
            for(i = HDD0_DEVICE; i < BDVD_DEVICE; i++)
            {
                if(((fdevices>>i) & 1))
                {
                    if(freeSpace[i] <= 0 && i > HDD0_DEVICE)
                    {
                        sprintf(tmp_path, "/dev_usb00%c/", (47 + i));
                        sysFsGetFreeSize(tmp_path, &blockSize, &freeSize);
                        double space = ( ((double)blockSize) * ((double) freeSize) ) /  GIGABYTES;
                        freeSpace[i] = (float) space;
                    }

                    if(m == i) SetFontColor(0xafd836ff, BLACK); else SetFontColor(0xffffff44, BLACK);

                    if(i == HDD0_DEVICE)
                        x2 = DrawFormatString(x2, 0, "hdd0: %.2fGB ", freeSpace[i]);
                    else
                        x2 = DrawFormatString(x2, 0, "usb00%c: %.2fGB ", 47 + i, freeSpace[i]);
                }
            }

        x2 = 848 -(x2 - 1200) - x;
    }


    if(gui_mode == MODE_XMB_LIKE && ((old_pad & (BUTTON_L2 | BUTTON_R2)) == (BUTTON_L2 | BUTTON_R2))) goto skip_overlays;


    // Show Date/Time
    if (gui_mode == MODE_XMB_LIKE)
    {
        u32 hh = 0, mm = 0, ss = 0, day = 0, month = 0, year = 0;

        PS3GetDateTime(&hh, &mm, &ss, &day, &month, &year);

        SetFontColor(0xffffffcc, BLACK);
        SetFontSize(18, 24);

        SetFontAutoCenter(0);

        float y2 = y + 40;
        x2 = x + 645;

        if(hh > 12)
        {
            hh = hh - 12;
            if(sys_dateformat == 0 || sys_dateformat == 2)
                DrawFormatString(x2, y2, " %02u/%02u %02u:%02u PM ", month, day, hh, mm);
            else
                DrawFormatString(x2, y2, " %02u/%02u %02u:%02u PM ", day, month, hh, mm);
        }
        else
        {
            if(hh == 0) hh = 12;
            if(sys_dateformat == 0 || sys_dateformat == 2)
                DrawFormatString(x2, y2, " %02u/%02u %02u:%02u AM ", month, day, hh, mm);
            else
                DrawFormatString(x2, y2, " %02u/%02u %02u:%02u AM ", day, month, hh, mm);
        }


        if(Png_offset[selected] == 0)
        {
            // draw quick preview of title name
            SetFontSize(20, 32);
            if(!mode_favourites && directories[(currentdir + selected)].title[0] != 0)
                DrawFormatString(x + 250, y + 214, directories[(currentdir + selected)].title);
            else if(mode_favourites && favourites.list[selected].title[0] != 0)
                DrawFormatString(x + 250, y + 214, favourites.list[selected].title);
        }
    }


    if(bShowPath)
    {
        SetFontColor(0xffffffcc, BLACK);
        SetCurrentFont(FONT_TTF);
        SetFontSize(10, 16);

        SetFontAutoCenter(1);

        int ly =  11;

        // set label location
        switch(gui_mode)
        {
         case 0 : ly = -30; break;
         case 2 : ly =   0; break;
         case 3 : ly = -20; break;
         case 6 : ly = -10; break;
         case 7 : ly = -20; break;
         case 10: ly =   0; break;
         case 14: ly =  -8;
        }

        DrawString(0,  y + 3 * 150 + ly, directories[(mode_favourites != 0) ? favourites.list[selected].index : (currentdir + selected)].path_name);
    }

    SetFontAutoCenter(0);
    SetFontSize(18, 20);

    SetFontColor(WHITE, BLACK);

    y += 24;

    int ww = 800 / cols;
    int hh = ww * 150 / 200;
    int xx = x, yy = y;

    if(gui_mode == MODE_XMB_LIKE)
    {
        hh = 130;

        ww = (hh * 200 / 150);

        xx += 50;
        yy += 140;
    }
    else if(hh * rows > 450)
    {
        hh = 450 / rows;

        ww = (hh * 200 / 150);

        if(ww * cols > 800) ww = 800 / cols;
        else
            xx += (800 - ww * cols) / 2;

    }
    else
    {
        yy += (450 - hh * rows) / 2;
    }

    #define FIX_X(a) ((a) * ww / 200)

    i = 0;
    int flash2 = 0;
    #define MAX_FLASH 32

    if(bIconPulse)
    {
        if (frame_count & MAX_FLASH)
            flash2 = (MAX_FLASH - 1) - (frame_count & (MAX_FLASH - 1));
        else
            flash2 = (frame_count & (MAX_FLASH - 1));
    }
    else
        flash2 = (MAX_FLASH - 1);

    ///////////////////////

    for(n = 0; n < rows; n++)
        for(m = 0; m < cols; m++)
        {
            int f = (select_px == m && select_py == n);
            float f2 = (int) f;

            if(cover_mode || gui_mode == MODE_XMB_LIKE) f2 = 2.1f * ((float) (flash2 *(select_px == m && select_py == n))) / ((float) MAX_FLASH);

            DrawBox(xx + ww * m - 4 * f2, yy + n * hh - 4 * f2, 0, (ww-8) + 8 * f2, (hh-8) + 8 * f2, BLACK + (flash2 * f) );

            //draw Splited box
            //if(directories[currentgamedir].splitted)
            //    DrawBox(x + 198 * m, (y - 2) + n * 150, 0, 194, 144, 0x55ff3328 );

            int set_ps3_cover = 0;

            if(Png_offset[i])
            {
                // PS3 cover type
                if(cover_mode == 2) set_ps3_cover = 1; else
                if(cover_mode == 3 && Png_iscover[i] == 1) set_ps3_cover = 1;

                if(Png_iscover[i] < 1 && (directories[get_currentdir(i)].flags & D_FLAG_HOMEB_GROUP) > D_FLAG_HOMEB)
                {
                     if(directories[get_currentdir(i)].flags & D_FLAG_HOMEB_DVD)
                     {
                         if((directories[get_currentdir(i)].flags & D_FLAG_HOMEB_MKV) == D_FLAG_HOMEB_MKV)
                         tiny3d_SetTextureWrap(0, Png_res_offset[IMG_MOVIE_ICON], Png_res[IMG_MOVIE_ICON].width,
                                               Png_res[IMG_MOVIE_ICON].height, Png_res[IMG_MOVIE_ICON].wpitch,
                                               TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                         else
                         tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DVD_DISC], Png_res[IMG_DVD_DISC].width,
                                               Png_res[IMG_DVD_DISC].height, Png_res[IMG_DVD_DISC].wpitch,
                                               TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    }
                    else
                    {
                        Png_iscover[i] = -1;
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                               Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                               TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    }
                }
                else if((directories[get_currentdir(i)].flags  & ((BDVD_FLAG) | (PS1_FLAG))) == ((BDVD_FLAG) | (PS1_FLAG)))
                {
                    Png_iscover[i] = 0;
                    tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS1_DISC], Png_res[IMG_PS1_DISC].width,
                                          Png_res[IMG_PS1_DISC].height, Png_res[IMG_PS1_DISC].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                }
                else if(Png_iscover[i] < 1 && (directories[get_currentdir(i)].flags  & (PS1_FLAG)))
                {
                    // add PSP / PS2 / PSX ISO icon
                    if((directories[get_currentdir(i)].flags & (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG)) == (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG))
                    {
                        bool is_retro = (strlen(retro_root_path) > 0 && strstr(directories[get_currentdir(i)].path_name, retro_root_path) != NULL);
                        bool is_ps2_classic = !is_retro && strlen(ps2classic_path) > 0 &&
                                              (strstr(directories[get_currentdir(i)].path_name, ps2classic_path) != NULL);

                        if(is_retro)
                            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_RETRO_ICON], Png_res[IMG_RETRO_ICON].width,
                                                  Png_res[IMG_RETRO_ICON].height, Png_res[IMG_RETRO_ICON].wpitch,
                                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        else if(is_ps2_classic)
                        {
                            Png_iscover[i] = 0;
                            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS2_ISO], Png_res[IMG_PS2_ISO].width,
                                                  Png_res[IMG_PS2_ISO].height, Png_res[IMG_PS2_ISO].wpitch,
                                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        }
                        else
                            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PSP_ISO], Png_res[IMG_PSP_ISO].width,
                                                  Png_res[IMG_PSP_ISO].height, Png_res[IMG_PSP_ISO].wpitch,
                                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    }
                    else if((directories[get_currentdir(i)].flags & (PS2_FLAG)) == (PS2_FLAG))
                    {
                        Png_iscover[i] = 0;
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS2_ISO], Png_res[IMG_PS2_ISO].width,
                                              Png_res[IMG_PS2_ISO].height, Png_res[IMG_PS2_ISO].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    }
                    else
                    {
                        Png_iscover[i] = 0;
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS1_ISO], Png_res[IMG_PS1_ISO].width,
                                              Png_res[IMG_PS1_ISO].height, Png_res[IMG_PS1_ISO].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    }
                }
                else if(Png_iscover[i] < 1 && (directories[get_currentdir(i)].flags  & ((PS3_FLAG) | (BDVD_FLAG))) == (PS3_FLAG))
                {
                        Png_iscover[i] = -1;
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                              Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                }
                else
                        tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width,
                                              Png_datas[i].height, Png_datas[i].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                //if((directories[get_currentdir(i)].flags  & (BDVD_FLAG | PS1_FLAG)) == (PS1_FLAG)) {
                //    set_ps3_cover = 2; // PSX cover
                //    if((directories[get_currentdir(i)].flags & (PS2_FLAG)) == (PS2_FLAG)) set_ps3_cover = 3 + 1 * (Png_iscover[i] < 0); // PS2 cover
                //}

                if((directories[get_currentdir(i)].flags  & D_FLAG_HOMEB_GROUP) > D_FLAG_HOMEB)
                {
                    set_ps3_cover = 3;
                    if(Png_iscover[i] < 0) set_ps3_cover = 1;
                    else if(directories[get_currentdir(i)].flags & D_FLAG_HOMEB_DVD) set_ps3_cover = 1;
                    if(set_ps3_cover == 2 || set_ps3_cover == 4)
                        DrawTextBoxCover(xx + FIX_X(16) + ww * m - 4 * f2, yy + n * hh - 4 * f2, 0, FIX_X(160) + 8 * f2, (hh-8) + 8 * f2, (WHITE - (MAX_FLASH * 2 * f)) + (flash2 * 2 * f), set_ps3_cover - 1);
                    else
                        DrawTextBoxCover(xx + FIX_X(36) + ww * m - 4 * f2, yy + n * hh - 4 * f2, 0, FIX_X(124) + 8 * f2, (hh-8) + 8 * f2, (WHITE - (MAX_FLASH * 2 * f)) + (flash2 * 2 * f), set_ps3_cover - 1);
                }
                else if(set_ps3_cover && gui_mode != 0)
                {
                    if(set_ps3_cover == 2 || set_ps3_cover == 4)
                        DrawTextBoxCover(xx + FIX_X(16) + ww * m - 4 * f2, yy + n * hh - 4 * f2, 0, FIX_X(160) + 8 * f2, (hh-8) + 8 * f2, (WHITE - (MAX_FLASH * 2 * f)) + (flash2 * 2 * f), set_ps3_cover - 1);
                    else
                        DrawTextBoxCover(xx + FIX_X(36) + ww * m - 4 * f2, yy + n * hh - 4 * f2, 0, FIX_X(124) + 8 * f2, (hh-8) + 8 * f2, (WHITE - (MAX_FLASH * 2 * f)) + (flash2 * 2 * f), set_ps3_cover - 1);
                }
                else if(Png_iscover[i] == -1)
                    DrawTextBox(xx + FIX_X(25) + ww * m - 4 * f2, yy + n * hh - 4 * f2, 0, FIX_X(142) + 8 * f2, (hh-8) + 8 * f2, (WHITE - (MAX_FLASH * 2 * f)) + (flash2 * 2 * f));
                else if(Png_iscover[i] == 1)
                    DrawTextBox(xx + FIX_X(36) + ww * m - 4 * f2, yy + n * hh - 4 * f2, 0, FIX_X(124) + 8 * f2, (hh-8) + 8 * f2, (WHITE - (MAX_FLASH * 2 * f)) + (flash2 * 2 * f));
                else
                    DrawTextBox(xx + ww * m - 4 * f2, yy + n * hh - 4 * f2, 0, (ww-8) + 8 * f2, (hh-8) + 8 * f2, (WHITE - (MAX_FLASH * 2 * f)) + (flash2 * 2 * f));

                //if((mode_favourites != 0) && favourites.list[i].index < 0) exit(0);

                if(!mode_favourites || ((mode_favourites != 0) && favourites.list[i].index >= 0))
                {
                    // ignore Bluray icon
                    if((directories[get_currentdir(i)].flags  & (BDVD_FLAG | PS1_FLAG)) == (BDVD_FLAG | PS1_FLAG)) ;
                    // draw Bluray icon
                    else if((directories[get_currentdir(i)].flags  & GAMELIST_FILTER) == BDVD_FLAG)
                    {
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                              Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        DrawTextBox(xx + ww * m + 4 - 4 * f2, yy + n * hh + 4 - 4 * f2, 0, FIX_X(32), FIX_X(32), 0xffffffcf);
                    }
                    else if(bShowUSBIcon)
                    {
                        // draw USB icon
                        if(strncmp(directories[get_currentdir(i)].path_name, "net", 3) == SUCCESS)
                            ii = IMG_NETHOST;
                        else
                            ii = 1 + 13 * ((directories[get_currentdir(i)].flags & NTFS_FLAG) != 0);

                        if(ii == IMG_NETHOST || (directories[get_currentdir(i)].flags  & GAMELIST_FILTER) > 1)
                        {
                            tiny3d_SetTextureWrap(0, Png_res_offset[ii], Png_res[ii].width,
                            Png_res[ii].height, Png_res[ii].wpitch,
                                TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                            if(gui_mode == MODE_XMB_LIKE)
                                DrawTextBox(x + 250, y + 225, 0, FIX_X(32), FIX_X(24),
                                            directories[get_currentdir(i)].splitted ? 0xff9999aa : 0xffffffcf);
                            else
                                DrawTextBox(xx + ww * m + 4 - 4 * f2, yy + n * hh + 4 - 4 * f2, 0, FIX_X(32), FIX_X(24),
                                            directories[get_currentdir(i)].splitted ? 0xff9999aa : 0xffffffcf);
                        }
                    }
                }

            }
            else if(mode_favourites && favourites.list[i].title_id[0] != 0)
            {
                tiny3d_SetTextureWrap(0, Png_res_offset[IMG_MISSING_ICON], Png_res[IMG_MISSING_ICON].width,
                                      Png_res[IMG_MISSING_ICON].height, Png_res[IMG_MISSING_ICON].wpitch,
                                      TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                DrawTextBox(xx + ww * m + FIX_X(32) - 4 * f2, yy + n * hh + FIX_X(7) - 4 * f2, 0, FIX_X(128) + 8 * f2, FIX_X(128) + 8 * f2, 0xffffff3f);
            }

            i++;
        }

        #undef FIX_X

    i = selected;

    if(flash)
    {
        bool png_on = false;

        //DrawBox(x + 200 * select_px - 4, y + select_py * 150 - 4 , 0, 200, 150, 0xa0a06080);


        if(mode_favourites >= 0x10000)
        {
            if(mode_favourites < 0x20000)
            {
                if(Png_offset[BIG_PICT])
                {
                    tiny3d_SetTextureWrap(0, Png_offset[BIG_PICT], Png_datas[BIG_PICT].width,
                        Png_datas[BIG_PICT].height, Png_datas[BIG_PICT].wpitch,
                            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    png_on = true;
                }
            }
            else
            {
                i = mode_favourites - 0x20000;

                if(i >= 0 && i < num_box)
                {
                    if(!Png_offset[i] && favourites.list[i].title_id[0] != 0)
                    {
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_MISSING_ICON], Png_res[IMG_MISSING_ICON].width,
                                              Png_res[IMG_MISSING_ICON].height, Png_res[IMG_MISSING_ICON].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        png_on = true;

                    }
                    else if(Png_offset[i])
                    {
                        tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width,
                                              Png_datas[i].height, Png_datas[i].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        png_on = true;
                    }
                }
            }

            if(png_on)
                DrawTextBox(xx + ww * select_px - 4, yy + select_py * hh - 4 , 0, ww, hh, 0x8fff8fcf);
        }

    }



    SetFontColor(WHITE, BLACK);

    // display temperature
    if((frame_count & 0x100))
    {
        static u32 temp = 0;
        static u32 temp2 = 0;
        int y2;

        if(temp == 0 || (frame_count & 0x1f) == 0x0 )
        {
            sys_game_get_temperature(0, &temp);
            sys_game_get_temperature(1, &temp2);
        }

        SetCurrentFont(FONT_TTF);
        SetFontSize(20, 20);

        x2 = DrawFormatString(1024, 0, " Temp CPU: 99ºC RSX: 99ºC ");

        y2 = y + 3 * 150 - 4 + 12;
        SetFontColor(WHITE, BLACK);
        x2 = DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y2, " Temp CPU: ");
        if(temp < 80) SetFontColor(0xfff000ff, BLACK); else SetFontColor(0xff0000ff, BLACK);
        x2 = DrawFormatString(x2, y2, "%uºC",  temp);
        SetFontColor(WHITE, BLACK);
        x2 = DrawFormatString(x2, y2, " RSX: ");
        if(temp2 < 75) SetFontColor(0xfff000ff, BLACK); else SetFontColor(0xff0000ff, BLACK);
        x2 = DrawFormatString(x2, y2, "%uºC ", temp2);

        SetFontColor(WHITE, BLACK);
    }
    else if(Png_offset[i])
    {
        SetCurrentFont(FONT_TTF);
        SetFontSize(20, 20);
        x2 = DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_SOPTIONS]);
        DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 - 2, " %s ", language[DRAWSCREEN_SOPTIONS]);

    }
    else if(mode_favourites && mode_favourites < 0x10000 && favourites.list[i].title_id[0] != 0)
    {
        SetCurrentFont(FONT_TTF);
        SetFontSize(20, 20);
        x2 = DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_SDELETE]);
        DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 - 2, " %s ", language[DRAWSCREEN_SDELETE]);
    }


    if(!(frame_count & 0x100))
    {
        x2 = DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_STGLOPT]);
        DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 + 18, " %s ", language[DRAWSCREEN_STGLOPT]);
    }

    if(bShowPlayOverlay && cols > 1)
    {
        if(!(mode_favourites && mode_favourites < 0x10000 && favourites.list[i].title_id[0] != 0))
        {
            //DrawBox(xx + ww * select_px , yy + select_py * hh , 0, ww-8, hh-8, 0x404040a0);
            DrawBox(xx + ww * select_px - 4, yy + select_py * hh - 4 + hh - 40, 0, ww, 40, 0x40404080);
            SetCurrentFont(FONT_TTF); // get default
            SetFontSize(20, 20);
        }

        if(((Png_offset[i]) || (mode_favourites && mode_favourites < 0x10000 && favourites.list[i].title_id[0] != 0)))
        {
            DrawBox(xx + ww * select_px - 4, yy + select_py * hh - 4 + hh - 40, 0, ww, 40, 0x40404080);
            SetCurrentFont(FONT_TTF);
            SetFontSize(24 * 4 / cols, 24);

            x2 = DrawFormatString(xx + ww * select_px - 4, yy + select_py * hh - 4 + hh - 40, "  %s", language[DRAWSCREEN_PLAY]);
        }
    }

    // draw game name
    i = selected;

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);
    SetFontColor(0xffffffee, BLACK);


    if((Png_offset[i] && !mode_favourites) || (mode_favourites && favourites.list[i].title_id[0] != 0))
    {
        u32 str_color = 0xffffffee;

        if(mode_favourites)
        {
            if(strncmp((char *) string_title_utf8, favourites.list[i].title, 64))
            {
                strncpy((char *) string_title_utf8, favourites.list[i].title, 128);
                update_title_utf8 = true;
            }

        }
        else if((directories[(currentdir + i)].flags  & GAMELIST_FILTER) == BDVD_FLAG)
        {
            str_color = 0xafd836ee;
            if(strncmp((char *) string_title_utf8, bluray_game, 64))
            {
                strncpy((char *) string_title_utf8, bluray_game, 128);
                update_title_utf8 = true;
            }

        }
        else
        {
            if(strncmp((char *) string_title_utf8, directories[(currentdir + i)].title, 64))
            {
                strncpy((char *) string_title_utf8, directories[(currentdir + i)].title, 128);
                update_title_utf8 = true;
            }
        }

        if(update_title_utf8)
        {
            width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, (gui_mode == MODE_XMB_LIKE) ? 18 : 16, 24);
            update_title_utf8 = false;
        }

        tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
                32, 768 * 2,
                TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        if(gui_mode == MODE_XMB_LIKE)
            DrawTextBox(x + 250, y + 190 , 0, 780, 32,
                       (frame_count >> 5) & 1 ? str_color : (frame_count >> 6) & 1 ? 0xf4f4f4f4 : 0xe8e8e8e8);
        else
            DrawTextBox(x + 3, y + 3 * 150 , 0, 768, 32, str_color);


        SetFontAutoCenter(0);

        load_gamecfg (get_currentdir(i)); // refresh game info

        // BD, DVD, MKV

        if((directories[get_currentdir(i)].flags  & D_FLAG_HOMEB_GROUP) > D_FLAG_HOMEB)
        {
             if(directories[get_currentdir(i)].flags & D_FLAG_HOMEB_DVD)
             {
                if((directories[get_currentdir(i)].flags & D_FLAG_HOMEB_MKV) == D_FLAG_HOMEB_MKV)
                    tiny3d_SetTextureWrap(0, Png_res_offset[IMG_MOVIE_ICON], Png_res[IMG_MOVIE_ICON].width,
                                          Png_res[IMG_MOVIE_ICON].height, Png_res[IMG_MOVIE_ICON].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                else
                    tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DVD_DISC], Png_res[IMG_DVD_DISC].width,
                                          Png_res[IMG_DVD_DISC].height, Png_res[IMG_DVD_DISC].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
             }
             else tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                        Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                        TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

             DrawTextBox(xx + ww * select_px + ww - 52 + 8, yy + select_py * hh - 4 + hh - 36, 0, 32, 32, 0xffffff99);
        }
        else
        {
            if((game_cfg.useBDVD) || (game_cfg.direct_boot == 2) || (game_cfg.direct_boot == 3) || (directories[get_currentdir(i)].flags  & GAMELIST_FILTER) == NTFS_FLAG)
            {
                tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                      Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                      TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                DrawTextBox(xx + ww * select_px + ww - 52 + 8, yy + select_py * hh - 4 + hh - 36, 0, 32, 32, 0xffffff99);
            }

            if(game_cfg.direct_boot  && (directories[get_currentdir(i)].flags  & GAMELIST_FILTER) != NTFS_FLAG)
            {
                tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DIRECT_ICON], Png_res[IMG_DIRECT_ICON].width,
                                      Png_res[IMG_DIRECT_ICON].height, Png_res[IMG_DIRECT_ICON].wpitch,
                                      TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                DrawTextBox(xx + ww * select_px + ww - 52  + 8, yy + select_py * hh - 4 + hh - 36, 0, 32, 32, WHITE);
            }
        }

    }

skip_overlays:

    if(flash && ftp_inited)
    {
        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_FTP_ICON], Png_res[IMG_FTP_ICON].width,
                              Png_res[IMG_FTP_ICON].height, Png_res[IMG_FTP_ICON].wpitch,
                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox(290 /*200 * 4 -32*/, y - 32, 0, 64, 32, WHITE);
    }

    //SetCurrentFont(FONT_DEFAULT);
    if(signal_ntfs_mount && (frame_count & MAX_FLASH))
    {
        int ii = 1 + 13;
        tiny3d_SetTextureWrap(0, Png_res_offset[ii], Png_res[ii].width,
        Png_res[ii].height, Png_res[ii].wpitch,
            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBoxLine(848 - 64 - 16, 24, 0, 64, 48, 0xffffffcf, 0x404040ff);
    }

    tiny3d_Flip();
}

static int anim_mode = 0, anim_step = 0;
static int g_saved = 0;
static int g_rel_posx[2];

void draw_coverflow(float x, float y)
{
    int i, n, m;

    float x2;

    int str_type = (mode_homebrew != GAMEBASE_MODE) ? ((mode_homebrew == HOMEBREW_MODE) ? 0 : 4) : 1 + game_list_category;

    int selected = select_px + select_py * 4;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 18, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    if(mode_favourites >= 0x20000)
        DrawFormatString(x, y, " %s%s", language[DRAWSCREEN_FAVSWAP], &str_home[str_type][0]);
    else if(mode_favourites >= 0x10000)
        DrawFormatString(x, y, " %s%s", language[DRAWSCREEN_FAVINSERT], &str_home[str_type][0]);
    else if(mode_favourites)
        DrawFormatString(x, y, " %s%s", language[DRAWSCREEN_FAVORITES], &str_home[str_type][0]);
    else if(str_type == 4)
        DrawFormatString(x, y, " %i/%i %s", ndirectories ? select_px + currentdir + 1 : 0, ndirectories, &str_home[str_type][0]);
    else if(str_type == 3)
    {
        if(retro_mode == NET_GAMES) str_home[str_type][0] = 0;

        DrawFormatString(x, y, " %i/%i %s%s %s", ndirectories ? select_px + currentdir + 1 : 0, ndirectories, language[DRAWSCREEN_GAMES], &str_home[str_type][0], &str_retro_mode[retro_mode][0]);
    }
    else
        DrawFormatString(x, y, " %i/%i %s%s", ndirectories ? select_px + currentdir + 1 : 0, ndirectories, language[DRAWSCREEN_GAMES], &str_home[str_type][0]);

    // Show Date/Time
    if (iTimeFormat)
    {
        u32 hh = 0, mm = 0, ss = 0, day = 0, month = 0, year = 0;

        PS3GetDateTime(&hh, &mm, &ss, &day, &month, &year);

        SetFontColor(WHITE, BLACK);
        SetCurrentFont(FONT_BUTTON);

        SetFontSize(8, 20);

        SetFontAutoCenter(0);

        float y2 = y + 80;

        y2 = y + (3 * 150 - 4) + 12 + 24;
        x2 = x;

        if(iTimeFormat == 2)
        {
            if(hh > 12)
            {
                hh = hh - 12;
                DrawFormatString(x2, y2, " %02u:%02u PM ", hh, mm);
            }
            else
            {
                if(hh == 0) hh = 12;
                DrawFormatString(x2, y2, " %02u:%02u AM ", hh, mm);
            }
        }
        else if(iTimeFormat == 3)
        {
            if(sys_dateformat == 0)
                DrawFormatString(x2, y2, " %s, %04u/%02u/%02u %02u:%02u:%02u ", PS3TimeZone[sys_timezone % 110].name, year, month, day, hh, mm, ss);
            else if(sys_dateformat == 2)
                DrawFormatString(x2, y2, " %s, %02u/%02u/%04u %02u:%02u:%02u ", PS3TimeZone[sys_timezone % 110].name, month, day, year, hh, mm, ss);
            else
                DrawFormatString(x2, y2, " %s, %02u/%02u/%04u %02u:%02u:%02u ", PS3TimeZone[sys_timezone % 110].name, day, month, year, hh, mm, ss);
        }
        else if(iTimeFormat == 4)
        {
            if(hh > 12)
            {
                hh = hh - 12;
                if(sys_dateformat == 0 || sys_dateformat == 2)
                    DrawFormatString(x2, y2, " %02u/%02u %02u:%02u PM ", month, day, hh, mm);
                else
                    DrawFormatString(x2, y2, " %02u/%02u %02u:%02u PM ", day, month, hh, mm);
            }
            else
            {
                if(hh == 0) hh = 12;
                if(sys_dateformat == 0 || sys_dateformat == 2)
                    DrawFormatString(x2, y2, " %02u/%02u %02u:%02u AM ", month, day, hh, mm);
                else
                    DrawFormatString(x2, y2, " %02u/%02u %02u:%02u AM ", day, month, hh, mm);
            }
        }
        else
        {
            DrawFormatString(x2, y2, " %02u:%02u:%02u ", hh, mm, ss);
        }
    }

    ////////

    SetFontAutoCenter(0);

    SetFontColor(WHITE, BLACK);

    SetCurrentFont(FONT_TTF);
    SetFontSize(18, 20);

    // list device space

    m = selected;

    if(Png_offset[m])
    {
        i = -1;

        if(!mode_favourites || ((mode_favourites != 0) && favourites.list[m].index >= 0))
        {
            for(i = HDD0_DEVICE; i < BDVD_DEVICE; i++)
                if((directories[(mode_favourites != 0) ? favourites.list[m].index : (currentdir + m)].flags & GAMELIST_FILTER) == (1<<i)) break;
            if(i == BDVD_DEVICE)
                if((directories[(mode_favourites != 0) ? favourites.list[m].index : (currentdir + m)].flags & GAMELIST_FILTER) == NTFS_FLAG)
                {
                    i = (ISO_FLAGS);
                    strcpy(tmp_path, directories[(mode_favourites != 0) ? favourites.list[m].index : (currentdir + m)].path_name);
                }
        }
        m = i;
    }
    else
        m = -1;

    x2 = 1200;
    for(n = 0; n < 2; n++)
    {
        if(m == (ISO_FLAGS))
        {
            char *p = (char *) tmp_path + 1; while(*p && *p != ':') p++; if(*p == ':') p[1] = 0;
            SetFontColor(0xafd836ff, BLACK);
            x2 = DrawFormatString(x2, 0, "%s .ISO", (void *) (tmp_path + 1));
        }
        else
            for(i = HDD0_DEVICE; i < BDVD_DEVICE; i++)
            {
                if(((fdevices>>i) & 1))
                {
                    if(freeSpace[i] <= 0 && i > HDD0_DEVICE)
                    {
                        sprintf(tmp_path, "/dev_usb00%c/", (47 + i));
                        sysFsGetFreeSize(tmp_path, &blockSize, &freeSize);
                        double space = ( ((double)blockSize) * ((double) freeSize) ) /  GIGABYTES;
                        freeSpace[i] = (float) space;
                    }

                    if(m == i) SetFontColor(0xafd836ff, BLACK); else SetFontColor(0xffffff44, BLACK);

                    if(i == HDD0_DEVICE)
                        x2 = DrawFormatString(x2, 0, "hdd0: %.2fGB ", freeSpace[i]);
                    else
                        x2 = DrawFormatString(x2, 0, "usb00%c: %.2fGB ", 47 + i, freeSpace[i]);
                }
            }

        x2 = 848 -(x2 - 1200) - x;
    }

    SetFontAutoCenter(0);
    SetFontSize(18, 20);

    SetFontColor(WHITE, BLACK);

    y += 24;

    i = 0;
    int flash2 = 0;
    #define MAX_FLASH 32

    if(bIconPulse)
    {
        if (frame_count & MAX_FLASH)
            flash2 = (MAX_FLASH - 1) - (frame_count & (MAX_FLASH - 1));
        else
            flash2 = (frame_count & (MAX_FLASH - 1));
    }
    else
        flash2 = (MAX_FLASH - 1);

    ///////////////////////

    int rel_posx[12];
    int rel_posy[12];
    int rel_widthx[12];
    int rel_widthy[12];

    int relx = 0;
    int centerx = 0;

    int icony = 128;

    int l;

    for(n = 0; n < 3; n++)
    {
        for(m = 0; m < 4; m++)
        {
            rel_widthy[i] = 142;
            rel_posy[i] = y + icony;
            if(Png_offset[i])
            {
                if((directories[get_currentdir(i)].flags  & (BDVD_FLAG | PS1_FLAG)) == (BDVD_FLAG | PS1_FLAG))
                    Png_iscover[i] = -1;
                else if(Png_iscover[i] < 1 && (directories[get_currentdir(i)].flags  & PS1_FLAG))
                    Png_iscover[i] = -1;
            }

            if((select_px == m && select_py == n))
            {
                centerx = relx;
                if(Png_iscover[i] == -1 && ((directories[get_currentdir(i)].flags  & (BDVD_FLAG | PS1_FLAG)) == (PS1_FLAG)))
                    centerx += (162 + 10)/2;
                else if(Png_iscover[i] == -1)
                    centerx += (142 + 10)/2;
                else if(Png_iscover[i] ==  1)
                    centerx += (124 + 10)/2;
                else
                    centerx += (162 + 10)/2;
            }

            if(Png_offset[i])
            {
                if(Png_iscover[i] == -1 && ((directories[get_currentdir(i)].flags  & (BDVD_FLAG | PS1_FLAG)) == (PS1_FLAG)))
                    {rel_posx[i] = relx; rel_widthx[i] = 162; relx+= rel_widthx[i] + 10;}
                else if(Png_iscover[i] == -1)
                    {rel_posx[i] = relx; rel_widthx[i] = 142; relx+= rel_widthx[i] + 10;}
                else if(Png_iscover[i] == 1)
                    {rel_posx[i] = relx; rel_widthx[i] = 124; relx+= rel_widthx[i] + 10;}
                else
                    {rel_posx[i] = relx; rel_posy[i] += 16; rel_widthx[i] = 162; rel_widthy[i] = (142 - 16); relx += rel_widthx[i] + 10;}
            } else
                    {rel_posx[i] = relx; rel_widthx[i] = 128; relx+= rel_widthx[i] + 10;}

            i++;
        }
    }
    //////////////////////

    u32 color_line = 0x404040ff;

    // get_games_3

    if(centerx < 848/2) centerx = -(848/2 - centerx); else centerx -= 848/2;

    if((rel_posx[0] - centerx) > 30)  centerx = -30;

    // -> anim_mode
    if(!anim_mode || !g_saved)
    {
        // don´t scroll the covers
        n = select_px + select_py * 4;

        if((n - 1) < 0) g_rel_posx[0] = rel_posx[0] - centerx; else g_rel_posx[0] = rel_posx[n - 1] - centerx;
        if((n + 1) > 11) g_rel_posx[1] = rel_posx[11] - centerx; else g_rel_posx[1] = rel_posx[n + 1] - centerx;

        anim_mode = 0;
        g_saved = 1;
    }
    else
    {
        if(anim_mode == 2)
        {
            int a = g_rel_posx[1] - (rel_posx[select_px + select_py * 4] - centerx);

            centerx -= (a * (4 - anim_step))/5;

            anim_step++; if(anim_step > 4) {anim_mode = 0; g_saved = 0;}
        }
        else if(anim_mode == 1)
        {
            int a = g_rel_posx[0] - (rel_posx[select_px + select_py * 4] - centerx);

            centerx -= (a * (4 - anim_step))/5;

            anim_step++; if(anim_step > 4) {anim_mode = 0; g_saved = 0;}
        }
    }
    // -> end anim_mode

    float f3 = 0.0f;

    for(l = 0; l < 2; l++)
    {
        i = 0;

        for(n = 0; n < 3; n++)
            for(m = 0; m < 4; m++)
            {
                int f = (select_px == m && select_py == n);
                float f2 = (int) f;

                if(l != 0 && f) {i++;continue;}
                if(l == 0 && !f) {i++;continue;}

                if(!mode_favourites && get_currentdir(i) >= ndirectories) break;

                f2 =  ((float) (flash2 *(select_px == m && select_py == n))) / ((float) MAX_FLASH);

                if(f) {f2 = CoverflowSize + 2 * f2; f3 = f2;}

                int set_ps3_cover = 0;

                if(Png_offset[i])
                {
                    // PS3 cover type
                    if(cover_mode == 2) set_ps3_cover = 1; else
                    if(cover_mode == 3 && Png_iscover[i] == 1) set_ps3_cover = 1;

                    if(Png_iscover[i] < 1 && (directories[get_currentdir(i)].flags  & D_FLAG_HOMEB_GROUP) > D_FLAG_HOMEB)
                    {
                        if(directories[get_currentdir(i)].flags & D_FLAG_HOMEB_DVD)
                        {
                             if((directories[get_currentdir(i)].flags & D_FLAG_HOMEB_MKV) == D_FLAG_HOMEB_MKV)
                             tiny3d_SetTextureWrap(0, Png_res_offset[IMG_MOVIE_ICON], Png_res[IMG_MOVIE_ICON].width,
                                                   Png_res[IMG_MOVIE_ICON].height, Png_res[IMG_MOVIE_ICON].wpitch,
                                                   TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                             else
                             tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DVD_DISC], Png_res[IMG_DVD_DISC].width,
                                                   Png_res[IMG_DVD_DISC].height, Png_res[IMG_DVD_DISC].wpitch,
                                                   TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        }
                        else tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                                   Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                                   TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                    }
                    else if((directories[get_currentdir(i)].flags  & ((BDVD_FLAG) | (PS1_FLAG))) == ((BDVD_FLAG) | (PS1_FLAG)))
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS1_DISC], Png_res[IMG_PS1_DISC].width,
                                              Png_res[IMG_PS1_DISC].height, Png_res[IMG_PS1_DISC].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    else if(Png_iscover[i] < 1 && (directories[get_currentdir(i)].flags  & (PS1_FLAG)))
                    {
                        // add PSP / PS2 / PSX ISO icon
                        if((directories[get_currentdir(i)].flags & (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG)) == (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG))
                        {
                            bool is_retro = (strlen(retro_root_path) > 0 && strstr(directories[get_currentdir(i)].path_name, retro_root_path) != NULL);
                            bool is_ps2_classic = !is_retro && strlen(ps2classic_path) > 0 &&
                                                  (strstr(directories[get_currentdir(i)].path_name, ps2classic_path) != NULL);

                            if(is_retro)
                                tiny3d_SetTextureWrap(0, Png_res_offset[IMG_RETRO_ICON], Png_res[IMG_RETRO_ICON].width,
                                                      Png_res[IMG_RETRO_ICON].height, Png_res[IMG_RETRO_ICON].wpitch,
                                                      TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                            else if(is_ps2_classic)
                                tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS2_ISO], Png_res[IMG_PS2_ISO].width,
                                                      Png_res[IMG_PS2_ISO].height, Png_res[IMG_PS2_ISO].wpitch,
                                                      TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                            else
                                tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PSP_ISO], Png_res[IMG_PSP_ISO].width,
                                                      Png_res[IMG_PSP_ISO].height, Png_res[IMG_PSP_ISO].wpitch,
                                                      TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        }
                        else if((directories[get_currentdir(i)].flags & (PS2_FLAG)) == (PS2_FLAG))
                            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS2_ISO], Png_res[IMG_PS2_ISO].width,
                                                  Png_res[IMG_PS2_ISO].height, Png_res[IMG_PS2_ISO].wpitch,
                                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        else
                            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS1_ISO], Png_res[IMG_PS1_ISO].width,
                                                  Png_res[IMG_PS1_ISO].height, Png_res[IMG_PS1_ISO].wpitch,
                                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    }
                    else if(Png_iscover[i] < 1 && (directories[get_currentdir(i)].flags  & ((PS3_FLAG) | (BDVD_FLAG))) == (PS3_FLAG))
                            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                                          Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    else
                            tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width,
                                                 Png_datas[i].height, Png_datas[i].wpitch,
                                                 TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                    //if((directories[get_currentdir(i)].flags  & ((BDVD_FLAG) | (PS1_FLAG))) == (PS1_FLAG)) {
                    //    set_ps3_cover = 2; // PSX cover
                    //    if((directories[get_currentdir(i)].flags  & (PS2_FLAG)) == IS_PS2_FLAG) set_ps3_cover = 3 + 1 *(Png_iscover[i] < 0); // PS2 cover
                    //}

                    // BD, DVD, MKV
                    if((directories[get_currentdir(i)].flags  & D_FLAG_HOMEB_GROUP) > D_FLAG_HOMEB)
                    {
                        set_ps3_cover = 3;
                        if(Png_iscover[i] < 0) set_ps3_cover = 1;
                        else if(directories[get_currentdir(i)].flags & D_FLAG_HOMEB_DVD) set_ps3_cover = 1;
                    }

                    if(!set_ps3_cover)
                        DrawBox(rel_posx[i] - centerx - 4 * f2, rel_posy[i] - 4 * f2, !f ? 100 : 0, rel_widthx[i] + 8 * f2, rel_widthy[i] + 8 * f2, BLACK);

                    if(!set_ps3_cover)
                        DrawTextBoxLine(rel_posx[i] - centerx - 4 * f2, rel_posy[i] - 4 * f2, !f ? 100 : 0, rel_widthx[i] + 8 * f2, rel_widthy[i] + 8 * f2, WHITE, color_line);
                    else
                        DrawTextBoxCover(rel_posx[i] - centerx - 4 * f2, rel_posy[i] - 4 * f2, !f ? 100 : 0, rel_widthx[i] + 8 * f2, rel_widthy[i] + 8 * f2, WHITE, set_ps3_cover - 1);

                    if(!set_ps3_cover)
                    {
                        DrawBoxShadow(rel_posx[i] - centerx - 4 * f2, y + icony - 4 * f2 + 142 + 8 * f2 + 8, !f ? 100 : 0, rel_widthx[i] + 8 * f2, (rel_widthy[i]  * 5 / 8 + 8 * f2), BLACK);
                        DrawTextBoxShadow(rel_posx[i] - centerx - 4 * f2, y + icony - 4 * f2 + 142 + 8 * f2 + 8, !f ? 100 : 0, rel_widthx[i] + 8 * f2, (rel_widthy[i]  * 5 / 8 + 8 * f2), 0x60606090);
                    }
                    else
                    {
                        DrawTextBoxCoverShadow(rel_posx[i] - centerx - 4 * f2, y + icony - 4 * f2 + 142 + 8 * f2 + 8, !f ? 100 : 0, rel_widthx[i] + 8 * f2, (rel_widthy[i]  * 5 / 8 + 8 * f2), 0x60606090, set_ps3_cover - 1);
                    }

                    //if((mode_favourites != 0) && favourites.list[i].index < 0) exit(0);

                    if(!mode_favourites || ((mode_favourites != 0) && favourites.list[i].index >= 0))
                    {
                        // ignore bluray icon
                        if((directories[get_currentdir(i)].flags  & (BDVD_FLAG | PS1_FLAG)) == (BDVD_FLAG | PS1_FLAG)) ;
                        // draw Bluray icon
                        else if(bShowUSBIcon)
                        {
                            // draw USB icon
                            int ii;

                            if(strncmp(directories[get_currentdir(i)].path_name, "net", 3) == SUCCESS)
                                ii = IMG_NETHOST;
                            else if((directories[get_currentdir(i)].flags & GAMELIST_FILTER) == BDVD_FLAG)
                                ii = IMG_BLURAY_DISC;
                            else
                                ii = IMG_USB_ICON + (IMG_USB_ICON2 - IMG_USB_ICON) * ((directories[get_currentdir(i)].flags & NTFS_FLAG) != 0);

                            if(ii == IMG_NETHOST || (directories[get_currentdir(i)].flags  & GAMELIST_FILTER) > 1)
                            {
                                tiny3d_SetTextureWrap(0, Png_res_offset[ii], Png_res[ii].width,
                                Png_res[ii].height, Png_res[ii].wpitch,
                                    TINY3D_TEX_FORMAT_A8R8G8B8, TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                                int x_cor = 0, y_cor = 0;

                                if(set_ps3_cover & (1 | 3))
                                {
                                    x_cor = 0; y_cor = 16;
                                }
                                else if(set_ps3_cover & (2 | 4))
                                {
                                    x_cor = 12; y_cor = 0;
                                }

                                if(directories[get_currentdir(i)].splitted)
                                    DrawTextBoxLine(x_cor + rel_posx[i] - centerx - 4 * f2 + 4, y_cor + rel_posy[i] + 4 - 4 * f2, !f ? 100 : 0, 32, 24, 0xff9999aa, color_line);
                                else
                                    DrawTextBoxLine(x_cor + rel_posx[i] - centerx - 4 * f2 + 4, y_cor + rel_posy[i] + 4 - 4 * f2, !f ? 100 : 0, 32, 24, 0xffffffcf, color_line);
                            }
                        }
                    }


                }
                else if(mode_favourites && favourites.list[i].title_id[0] != 0)
                {
                    DrawBoxLine(rel_posx[i] - centerx - 4 * f2, rel_posy[i] - 4 * f2, !f ? 100 : 0, 128 + 8 * f2, rel_widthy[i] + 8 * f2, 0xa0a0a028, color_line);
                    tiny3d_SetTextureWrap(0, Png_res_offset[IMG_MISSING_ICON], Png_res[IMG_MISSING_ICON].width,
                                          Png_res[IMG_MISSING_ICON].height, Png_res[IMG_MISSING_ICON].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                    DrawTextBox(rel_posx[i] - centerx - 4 * f2, y  + 7 - 4 * f2 + icony, !f ? 100 : 0, 128 + 8 * f2, 128 + 8 * f2, 0xffffff3f);
                    DrawBoxShadow(rel_posx[i] - centerx - 4 * f2, y + icony - 4 * f2 + 142 + 8 * f2 + 8, !f ? 100 : 0, 128 + 8 * f2, (rel_widthy[i] * 5 / 8 + 8 * f2), 0x8f8f8f28);
                    DrawTextBoxShadow(rel_posx[i] - centerx - 4 * f2, y + icony - 4 * f2 + 142 + 8 * f2 + 8, !f ? 100 : 0, 128 + 8 * f2, (rel_widthy[i] * 5 / 8 + 8 * f2), 0x40404090);
                }
                else
                {
                    // draw Bluray icon with empty icon
                    if((directories[get_currentdir(i)].flags  & GAMELIST_FILTER) == BDVD_FLAG &&
                        ((directories[get_currentdir(i)].flags  & (BDVD_FLAG | PS1_FLAG))!= (BDVD_FLAG | PS1_FLAG)))
                    {

                        DrawBoxLine(rel_posx[i] - centerx - 4 * f2, rel_posy[i] - 4 * f2, !f ? 100 : 0, 128 + 8 * f2, rel_widthy[i] + 8 * f2, 0xa0a0a040, color_line);
                        DrawBoxShadow(rel_posx[i] - centerx - 4 * f2, y + icony - 4 * f2 + 142 + 8 * f2 + 8, !f ? 100 : 0, 128 + 8 * f2, (rel_widthy[i] * 5 / 8 + 8 * f2), 0x80808040);

                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                                 Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                                 TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        DrawTextBox(rel_posx[i] - centerx - 4 * f2 + (rel_widthx[i] - 124)/2, rel_posy[i] + 4 + (rel_widthy[i] - 124)/2 - 4 * f2, !f ? 100 : 0,  (124) + 8 * f2, (124) + 8 * f2, 0xffffffcf);

                        DrawTextBoxShadow(rel_posx[i] - centerx - 4 * f2 + (rel_widthx[i] - 124)/2, y + icony - 4 * f2 + 142 + 8 * f2 + 8, !f ? 100 : 0,  (124) + 8 * f2, (124 * 5 / 8) + 8 * f2, 0x60606090);

                    }
                    else
                    {
                        DrawBoxLine(rel_posx[i] - centerx - 4 * f2, rel_posy[i] - 4 * f2, !f ? 100 : 0, 128 + 8 * f2, rel_widthy[i] + 8 * f2, 0xa0a0a040, color_line);
                        DrawBoxShadow(rel_posx[i] - centerx - 4 * f2, y + icony - 4 * f2 + 142 + 8 * f2 + 8, !f ? 100 : 0, 128 + 8 * f2, (rel_widthy[i] * 5 / 8 + 8 * f2), 0x80808040);
                    }
                }

                i++;
            }
    }

    i = selected;

    if(flash)
    {
        bool png_on = false;

        //DrawBox(x + 200 * select_px - 4, y + select_py * 150 - 4 , 0, 200, 150, 0xa0a06080);

        if(mode_favourites >= 0x10000)
        {
            if(mode_favourites < 0x20000)
            {
                if(Png_offset[BIG_PICT])
                {
                    tiny3d_SetTextureWrap(0, Png_offset[BIG_PICT], Png_datas[BIG_PICT].width,
                                          Png_datas[BIG_PICT].height, Png_datas[BIG_PICT].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    png_on = true;
                }
            }
            else
           {
                i = mode_favourites - 0x20000;

                if(i >= 0 && i < num_box)
                {
                    if(!Png_offset[i] && favourites.list[i].title_id[0] != 0)
                    {
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_MISSING_ICON], Png_res[IMG_MISSING_ICON].width,
                                              Png_res[IMG_MISSING_ICON].height, Png_res[IMG_MISSING_ICON].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        png_on = true;
                    }
                    else if(Png_offset[i])
                    {
                        png_on = 1;
                        tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width,
                        Png_datas[i].height, Png_datas[i].wpitch,
                            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    }
                }
            }

            int ii = select_px + 4 * select_py;

            if(png_on)
                DrawTextBox(rel_posx[ii] - centerx - 4 * f3, rel_posy[ii] - 4 * f3 , 0 , rel_widthx[ii] + 8 * f3, rel_widthy[ii] + 8 * f3, 0x8fff8fcf);
            else
                DrawBox(rel_posx[ii] - centerx - 4 * f3, rel_posy[ii] - 4 * f3 , 0 , rel_widthx[ii] + 8 * f3, rel_widthy[ii] + 8 * f3, 0x8fff8fcf);
        }

    }

    SetFontColor(WHITE, BLACK);

    // display temperature
    if((frame_count & 0x100))
    {
        static u32 temp = 0;
        static u32 temp2 = 0;
        int y2;

        if(temp == 0 || (frame_count & 0x1f) == 0x0 )
        {
            sys_game_get_temperature(0, &temp);
            sys_game_get_temperature(1, &temp2);
        }

        SetCurrentFont(FONT_TTF);
        SetFontSize(20, 20);

        x2 = DrawFormatString(1024, 0, " Temp CPU: 99ºC RSX: 99ºC ");

        y2 = y + 3 * 150 - 4 + 12;
        SetFontColor(WHITE, BLACK);
        x2 = DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y2, " Temp CPU: ");
        if(temp < 80) SetFontColor(0xfff000ff, BLACK); else SetFontColor(0xff0000ff, BLACK);
        x2 = DrawFormatString(x2, y2, "%uºC",  temp);
        SetFontColor(WHITE, BLACK);
        x2 = DrawFormatString(x2, y2, " RSX: ");
        if(temp2 < 75) SetFontColor(0xfff000ff, BLACK); else SetFontColor(0xff0000ff, BLACK);
        x2 = DrawFormatString(x2, y2, "%uºC ", temp2);

        SetFontColor(WHITE, BLACK);
    }
    else if(Png_offset[i])
    {
        SetCurrentFont(FONT_TTF);
        SetFontSize(20, 20);
        x2 = DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_SOPTIONS]);
        DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 - 2, " %s ", language[DRAWSCREEN_SOPTIONS]);

    }
    else if(mode_favourites && mode_favourites < 0x10000 && favourites.list[i].title_id[0] != 0)
    {
        SetCurrentFont(FONT_TTF);
        SetFontSize(20, 20);
        x2 = DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_SDELETE]);
        DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 - 2, " %s ", language[DRAWSCREEN_SDELETE]);
    }
    else
    {
        //DrawBox(x + 200 * select_px , y + select_py * 150 , 0, 192, 142, 0x404040a0);
        SetCurrentFont(FONT_TTF); // get default
        SetFontSize(20, 20);
    }

    if(!(frame_count & 0x100))
    {
        x2 = DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_STGLOPT]);
        DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 + 18, " %s ", language[DRAWSCREEN_STGLOPT]);
    }

    // draw game name
    i = selected;

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);

    SetFontColor(0xffffffee, BLACK);

    if((Png_offset[i] && !mode_favourites) || (mode_favourites && favourites.list[i].title_id[0] != 0))
    {
        u32 str_color = 0xffffffee;

        if(mode_favourites)
        {
            if(strncmp((char *) string_title_utf8, favourites.list[i].title, 64))
            {
                strncpy((char *) string_title_utf8, favourites.list[i].title, 128);
                update_title_utf8 = true;
            }
        }
        else if((directories[(currentdir + i)].flags  & GAMELIST_FILTER) == BDVD_FLAG)
        {
            str_color = 0xafd836ee;
            if(strncmp((char *) string_title_utf8, bluray_game, 64))
            {
                strncpy((char *) string_title_utf8, bluray_game, 128);
                update_title_utf8 = true;
            }

        }
        else
        {
            if(strncmp((char *) string_title_utf8, directories[(currentdir + i)].title, 64))
            {
                strncpy((char *) string_title_utf8, directories[(currentdir + i)].title, 128);
                update_title_utf8 = true;
            }
        }

        n = 0;
        while(string_title_utf8[n])
        {
            if(string_title_utf8[n] == 13 || string_title_utf8[n] == 10) string_title_utf8[n] = '/';
            n++;
        }

        SetCurrentFont(FONT_TTF);
        SetFontSize(16, 32);
        SetFontAutoCenter(1);
        SetFontColor(str_color, 0x00000020);
        DrawFormatString(0 , y + 80 - 32, " %s ", string_title_utf8);
        SetFontColor(WHITE, BLACK);
/*
        if(update_title_utf8)
        {
            width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, 16, 24);
            update_title_utf8 = false;
        }

        tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
                32, 768 * 2,
                TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox(x + 3, y + 3 * 150 , 0, 768, 32, str_color);
*/

        SetFontAutoCenter(0);

        load_gamecfg (get_currentdir(i)); // refresh game info

        x2 = 0;


        // BD, DVD, MKV
        if((directories[get_currentdir(i)].flags  & D_FLAG_HOMEB_GROUP) > D_FLAG_HOMEB)
        {
             if(directories[get_currentdir(i)].flags & D_FLAG_HOMEB_DVD)
             {
                   if((directories[get_currentdir(i)].flags & D_FLAG_HOMEB_MKV) == D_FLAG_HOMEB_MKV)
                   tiny3d_SetTextureWrap(0, Png_res_offset[IMG_MOVIE_ICON], Png_res[IMG_MOVIE_ICON].width,
                                     Png_res[IMG_MOVIE_ICON].height, Png_res[IMG_MOVIE_ICON].wpitch,
                                     TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                   else
                   tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DVD_DISC], Png_res[IMG_DVD_DISC].width,
                                         Png_res[IMG_DVD_DISC].height, Png_res[IMG_DVD_DISC].wpitch,
                                         TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
             }
             else tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                        Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                        TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

             DrawTextBox(x + x2, y + 3 * 150 - 48, 0, 32, 32, 0xffffff99);

             x2+= 40;

             u32 ff = directories[get_currentdir(i)].flags & D_FLAG_HOMEB_MKV;

             x2 = DrawString(x + x2,  y + 3 * 150 - 48 + 4, ff == D_FLAG_HOMEB_MKV ? "VIDEO"   :
                                                            ff == D_FLAG_HOMEB_BD  ? "BD ISO"  :
                                                            ff == D_FLAG_HOMEB_DVD ? "DVD ISO" : "");

        }
        else
        {
            if((directories[get_currentdir(i)].flags & PS1_FLAG) || (directories[get_currentdir(i)].flags & D_FLAG_PS3_ISO) || (game_cfg.useBDVD) || (game_cfg.direct_boot == 2) || (game_cfg.direct_boot == 3))
            {
                // add PSP / PS2 / PSX ISO icon
                if((directories[get_currentdir(i)].flags & (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG)) == (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG))
                {
                    bool is_retro = (strlen(retro_root_path) > 0 && strstr(directories[get_currentdir(i)].path_name, retro_root_path) != NULL);
                    bool is_ps2_classic = !is_retro && strlen(ps2classic_path) > 0 &&
                                          (strstr(directories[get_currentdir(i)].path_name, ps2classic_path) != NULL);

                    if(is_retro)
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_RETRO_ICON], Png_res[IMG_RETRO_ICON].width,
                                              Png_res[IMG_RETRO_ICON].height, Png_res[IMG_RETRO_ICON].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    else if(is_ps2_classic)
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS2_ISO], Png_res[IMG_PS2_ISO].width,
                                              Png_res[IMG_PS2_ISO].height, Png_res[IMG_PS2_ISO].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    else
                        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PSP_ISO], Png_res[IMG_PSP_ISO].width,
                                              Png_res[IMG_PSP_ISO].height, Png_res[IMG_PSP_ISO].wpitch,
                                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                }
                else if((directories[get_currentdir(i)].flags & (PS2_FLAG)) == (PS2_FLAG))
                    tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS2_ISO], Png_res[IMG_PS2_ISO].width,
                                          Png_res[IMG_PS2_ISO].height, Png_res[IMG_PS2_ISO].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                else if((directories[get_currentdir(i)].flags & (PS1_FLAG)) == (PS1_FLAG))
                    tiny3d_SetTextureWrap(0, Png_res_offset[IMG_PS1_ISO], Png_res[IMG_PS1_ISO].width,
                                          Png_res[IMG_PS1_ISO].height, Png_res[IMG_PS1_ISO].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                else
                    tiny3d_SetTextureWrap(0, Png_res_offset[IMG_BLURAY_DISC], Png_res[IMG_BLURAY_DISC].width,
                                          Png_res[IMG_BLURAY_DISC].height, Png_res[IMG_BLURAY_DISC].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                DrawTextBox(x + x2,  y + 3 * 150 - 48, 0, 32, 32, 0xffffff99);

                x2+= 40;
            }


            if(!(directories[get_currentdir(i)].flags & D_FLAG_PS3_ISO))
            {
                if(game_cfg.direct_boot)
                {
                    tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DIRECT_ICON], Png_res[IMG_DIRECT_ICON].width,
                                          Png_res[IMG_DIRECT_ICON].height, Png_res[IMG_DIRECT_ICON].wpitch,
                                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                    DrawTextBox(x + x2,  y + 3 * 150 - 48, 0, 32, 32, WHITE);

                    x2+= 40;
                }

                n = (directories[get_currentdir(i)].flags & D_FLAG_HDD0) ? game_cfg.bdemu : game_cfg.bdemu_ext;

                SetFontColor(0xffffffee, BLACK);
                SetCurrentFont(FONT_TTF);
                SetFontSize(12, 24);

                x2 = DrawString(x + x2,  y + 3 * 150 - 48 + 4, (directories[get_currentdir(i)].flags & (PS1_FLAG)) == (PS1_FLAG) ? "PSX ISO" :
                                                                                      (manager_cfg.global_bd_mirror || (n == 1)) ? "BD EMU"  :
                                                                                                                        (n == 2) ? "LIBFS"   : "");
            }
            else if((directories[get_currentdir(i)].flags & (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG)) == (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG))
            {
                bool is_retro = (strlen(retro_root_path) > 0 && strstr(directories[get_currentdir(i)].path_name, retro_root_path) != NULL);
                bool is_ps2_classic = !is_retro && strlen(ps2classic_path) > 0 &&
                                      (strstr(directories[get_currentdir(i)].path_name, ps2classic_path) != NULL);

                x2 = DrawString(x + x2,  y + 3 * 150 - 48 + 4, is_retro       ? "RETRO" :
                                                               is_ps2_classic ? "PS2 CLASSIC" :
                                                                                "PSP ISO");
            }
            else
                x2 = DrawString(x + x2,  y + 3 * 150 - 48 + 4, (directories[get_currentdir(i)].flags & (PS2_FLAG)) == (PS2_FLAG) ? "PS2 ISO" : "PS3 ISO");

        }
    }

    if(bHideCoverflowSortModeLabel);
    else if(str_type == 1 || str_type == 2)
    {
        SetFontColor(0xffffffee, BLACK);
        SetCurrentFont(FONT_TTF);
        SetFontSize(12, 24);

        SetFontAutoCenter(1);

        DrawString(0,  y + 3 * 150 - 48 + 4, sort_mode == 1 ? "PS3 > PSX" :
                                             sort_mode == 2 ? "PSX > PS3" :
                                                              "PS3 - PSX");

        SetFontAutoCenter(0);
    }

    if(bShowPath)
    {
        SetFontColor(0xffffff88, BLACK);
        SetCurrentFont(FONT_TTF);
        SetFontSize(10, 16);

        SetFontAutoCenter(1);
        DrawString(0,  y + 3 * 150 - 80, directories[get_currentdir(i)].path_name);
    }

    if(flash && ftp_inited)
    {
        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_FTP_ICON], Png_res[IMG_FTP_ICON].width,
                              Png_res[IMG_FTP_ICON].height, Png_res[IMG_FTP_ICON].wpitch,
                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox(200 * 4 -32, y + 3 * 150 - 48, 0, 64, 32, WHITE);
    }

    //SetCurrentFont(FONT_DEFAULT);

    if(signal_ntfs_mount && (frame_count & MAX_FLASH))
    {
        int ii = IMG_USB_ICON2;
        tiny3d_SetTextureWrap(0, Png_res_offset[ii], Png_res[ii].width,
        Png_res[ii].height, Png_res[ii].wpitch,
            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBoxLine(848 - 64 - 16, 24, 0, 64, 48, 0xffffffcf, 0x404040ff);
    }

    tiny3d_Flip();

}
#endif

int gui_control()
{
    int i = 0, n = 0, r = SUCCESS;

    int selected = select_px + select_py * cols;

    ps3pad_read();

    if(menu_screen == SCR_MAIN_GAME_LIST)
    {
        if(autolaunch == LAUNCHMODE_TOCHECK) get_last_selected_game(false);

        if(autolaunch >= LAUNCHMODE_STARTED)
        {
            i = autolaunch;
            //currentdir = 0;

            selected = autolaunch;
            r = 1; //do not refresh gui
            load_gamecfg(autolaunch);

            mode_favourites = 0;
            autolaunch = LAUNCHMODE_CHECKED;
            goto autolaunch_proc;
        }
    }

    autolaunch = LAUNCHMODE_CHECKED;

    if(mode_favourites && (old_pad & BUTTON_L2) && (new_pad & BUTTON_TRIANGLE))
    {
        mode_favourites = 0x20000 | (selected);
        return r;
    }

    if(new_pad & BUTTON_CIRCLE_)
    {
        allow_save_lastgame = true;

        if(mode_favourites >= 0x10000) mode_favourites = 1;
        else
        {
            unlink_secure("/dev_hdd0/tmp/wm_request");

            if(!test_ftp_working())
            {
                if(old_pad & BUTTON_L2)
                {
                    SaveGameList();

                    fun_exit();

                    // relaunch iris manager
                    sprintf(tmp_path, "%s/USRDIR/RELOAD.SELF", self_path);
                    sysProcessExitSpawn2(tmp_path, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
                    exit(0);
                }

                if (old_pad & (BUTTON_SELECT))
                {
                    if(DrawDialogYesNo(language[DRAWSCREEN_RESTART]) == YES) {SaveGameList(); set_install_pkg = true; game_cfg.direct_boot = 0; fun_exit();}
                }
                if (old_pad & (BUTTON_R2))
                {
                    if(DrawDialogYesNo(language[DRAWSCREEN_SHUTDOWN]) == YES) {SaveGameList(); set_install_pkg = false; fun_exit(); sys_shutdown();}
                }
                else
                {
                    if(DrawDialogYesNo(language[DRAWSCREEN_EXITXMB]) == YES) {SaveGameList(); exit_program = true; fun_exit(); return r;}
                }

                tiny3d_Flip();
                ps3pad_read();

                if(old_pad & (BUTTON_L2 | BUTTON_R2 | BUTTON_SELECT)) load_background_picture();
                return r;
            }
        }
    }

    if(new_pad & BUTTON_SQUARE)
    {
        if(old_pad & BUTTON_SELECT)
        {
            // SELECT + SQUARE = toggle Cover mode
            ROT_INC(cover_mode, 3, 0);
            manager_cfg.cover_mode = cover_mode;

            if(mode_homebrew != HOMEBREW_MODE)
            {
                get_games();
                load_gamecfg(RESET_GAME_INFO); // force refresh game info
            }
        }

        if(mode_homebrew == GAMEBASE_MODE)
        {
            if(!bdvd_ejected)
                Eject_BDVD(NOWAIT_BDVD | EJECT_BDVD);
            else
                Eject_BDVD(NOWAIT_BDVD | LOAD_BDVD);
        }

    }

    if(new_pad & BUTTON_CROSS_)
    {
        i = selected;
        allow_save_lastgame = true;

        if(mode_favourites >= 0x20000)
        {
            // swap favourites
            entry_favourites swap = favourites.list[i];

            favourites.list[i] = favourites.list[mode_favourites - 0x20000]; favourites.list[mode_favourites - 0x20000] = swap;

            sprintf(tmp_path, "%s/config/", self_path);
            SaveFavourites(tmp_path, mode_homebrew);

            mode_favourites = 1;
            get_games();

            return r;
        }

        if(mode_favourites >= 0x10000)
        {
            // insert favourites
            DeleteFavouritesIfExits(directories[mode_favourites - 0x10000].title_id);
            AddFavourites(i, directories, mode_favourites - 0x10000);

            sprintf(tmp_path, "%s/config/", self_path);
            SaveFavourites(tmp_path, mode_homebrew);

            mode_favourites = 1;
            get_games();

            return r;
        }

        if(test_ftp_working()) return r;

        if(Png_offset[i])
        {
            if(mode_favourites != 0 && favourites.list[i].index < 0)
            {
                DrawDialogOK(language[DRAWSCREEN_CANRUNFAV]); return r;
            }
            else
            {
autolaunch_proc:
                //////////////////////////////////////////////////////////////////////////////////////////

                if(autolaunch >= LAUNCHMODE_STARTED)
                    currentgamedir = autolaunch;
                else
                    currentgamedir = mode_favourites ? favourites.list[i].index : (currentdir + i);

                if(currentgamedir < 0 || currentgamedir >= ndirectories) return r;

                bool use_cache = false;

                reset_sys8_path_table();

                SaveGameList();

                tiny3d_Flip(); r = 1; //do not refresh gui

#ifndef LASTPLAY_LOADER
                // Save LASTPLAY.BIN for lastGAME
                if(bAddToLastGameApp && file_exists(LASTGAME_PATH_SS "USRDIR/ORG.PNG"))
                {
                    unlink_secure(LASTGAME_PATH_SS "PARAM.SFO");
                    unlink_secure(LASTGAME_PATH_SS "PARAM.HIS");
                    unlink_secure(LASTGAME_PATH_SS "ICON0.PNG");
                    unlink_secure(LASTGAME_PATH_SS "ICON1.PAM");
                    unlink_secure(LASTGAME_PATH_SS "SND0.AT3");
                    unlink_secure(LASTGAME_PATH_SS "PIC0.PNG");
                    unlink_secure(LASTGAME_PATH_SS "PIC1.PNG");
                    unlink_secure(LASTGAME_PATH_SS "PIC2.PNG");
                    unlink_secure(LASTGAME_PATH_SS "USRDIR/LASTPLAY.BIN");

                    sprintf(tmp_path, "%s/PS3_GAME/ICON0.PNG", directories[currentgamedir].path_name);
                    if(file_exists(tmp_path))
                        CopyFile(tmp_path, LASTGAME_PATH_SS "ICON0.PNG");
                    else if(strstr(directories[currentgamedir].path_name, "/PSXISO"))
                        CopyFile(LASTGAME_PATH_SS "USRDIR/PS1.PNG", LASTGAME_PATH_SS "ICON0.PNG");
                    else if(strstr(directories[currentgamedir].path_name, "/PS2ISO"))
                        CopyFile(LASTGAME_PATH_SS "USRDIR/PS2.PNG", LASTGAME_PATH_SS "ICON0.PNG");
                    else if(strstr(directories[currentgamedir].path_name, "/PSPISO"))
                        CopyFile(LASTGAME_PATH_SS "USRDIR/PSP.PNG", LASTGAME_PATH_SS "ICON0.PNG");
                    else
                        CopyFile(LASTGAME_PATH_SS "USRDIR/ORG.PNG", LASTGAME_PATH_SS "ICON0.PNG");

                    if(strstr(directories[currentgamedir].path_name, "/PS3ISO"))
                    {
                        if(file_exists(directories[currentgamedir].path_name) && strcasestr(directories[currentgamedir].path_name, ".iso"))
                        {
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/ICON0.PNG;1", LASTGAME_PATH_SS "ICON0.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/ICON1.PAM;1", LASTGAME_PATH_SS "ICON1.PAM");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/SND0.AT3;1" , LASTGAME_PATH_SS "SND0.AT3");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/PIC0.PNG;1" , LASTGAME_PATH_SS "PIC0.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/PIC1.PNG;1" , LASTGAME_PATH_SS "PIC1.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/PIC2.PNG;1" , LASTGAME_PATH_SS "PIC2.PNG");
                        }
                    }
                    else if(strstr(directories[currentgamedir].path_name, "/PSPISO"))
                    {
                        if(file_exists(directories[currentgamedir].path_name) && strcasestr(directories[currentgamedir].path_name, ".iso"))
                        {
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/ICON0.PNG", LASTGAME_PATH_SS "ICON0.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/ICON1.PAM", LASTGAME_PATH_SS "ICON1.PAM");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/SND0.AT3" , LASTGAME_PATH_SS "SND0.AT3");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/PIC0.PNG" , LASTGAME_PATH_SS "PIC0.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/PIC1.PNG" , LASTGAME_PATH_SS "PIC1.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/PIC2.PNG" , LASTGAME_PATH_SS "PIC2.PNG");
                        }
                    }
                    else
                    {
                        sprintf(tmp_path, "%s/PS3_GAME/PIC0.PNG", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH_SS "PIC0.PNG");

                        sprintf(tmp_path, "%s/PS3_GAME/PIC1.PNG", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH_SS "PIC1.PNG");

                        sprintf(tmp_path, "%s/PS3_GAME/PIC2.PNG", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH_SS "PIC2.PNG");

                        sprintf(tmp_path, "%s/PS3_GAME/ICON1.PAM", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH_SS "ICON1.PAM");

                        sprintf(tmp_path, "%s/PS3_GAME/SND0.AT3", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH_SS "SND0.AT3");

                        sprintf(tmp_path, "%s/PS3_GAME/PARAM.HIS", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH_SS "PARAM.HIS");
                    }

                    int size = 0;
                    char *sfo = NULL;

                    sfo = LoadFile(LASTGAME_PATH_SS "USRDIR/ORG.SFO", &size);
                    if(sfo)
                    {
                        strncpy(sfo + 0x378, directories[currentgamedir].title, 64);
                        SaveFile(LASTGAME_PATH_SS "PARAM.SFO", (char *) sfo, size);
                        if(sfo) free(sfo);
                    }
                    else
                        CopyFile(LASTGAME_PATH_SS "USRDIR/ORG.SFO", LASTGAME_PATH_SS "PARAM.SFO");

                    char titleid[10];
                    sprintf(titleid, "%c%c%c%c%s", directories[currentgamedir].title_id[0], directories[currentgamedir].title_id[1],
                                                   directories[currentgamedir].title_id[2], directories[currentgamedir].title_id[3], &directories[currentgamedir].title_id[5]);

                    sprintf(tmp_path, "%s/PS3_GAME/USRDIR/EBOOT.BIN", directories[currentgamedir].path_name);
                    if(file_exists(tmp_path))
                        sprintf(temp_buffer, "SELF=%s\nPATH=%s\nTITLEID=%s\n", tmp_path, directories[currentgamedir].path_name, titleid);
                    else
                        sprintf(temp_buffer, "SELF=%s\nTITLEID=%s\n", directories[currentgamedir].path_name, titleid);

                    // write LASTPLAY.BIN
                    FILE *fp;
                    fp = fopen(LASTGAME_PATH_SS "USRDIR/LASTPLAY.BIN", "w");
                    fputs (temp_buffer, fp);
                    fclose(fp);
                }
                else if(bAddToLastGameApp && file_exists(LASTGAME_PATH "USRDIR/EBOOT.BIN"))
                {
                    unlink_secure(LASTGAME_PATH "PARAM.SFO");
                    unlink_secure(LASTGAME_PATH "PARAM.HIS");
                    unlink_secure(LASTGAME_PATH "ICON0.PNG");
                    unlink_secure(LASTGAME_PATH "ICON1.PAM");
                    unlink_secure(LASTGAME_PATH "SND0.AT3");
                    unlink_secure(LASTGAME_PATH "PIC0.PNG");
                    unlink_secure(LASTGAME_PATH "PIC1.PNG");
                    unlink_secure(LASTGAME_PATH "PIC2.PNG");
                    unlink_secure(LASTGAME_PATH "USRDIR/LASTPLAY.BIN");

                    sprintf(tmp_path, "%s/PS3_GAME/ICON0.PNG", directories[currentgamedir].path_name);
                    if(file_exists(tmp_path))
                        CopyFile(tmp_path, LASTGAME_PATH "ICON0.PNG");
                    else if(strstr(directories[currentgamedir].path_name, "/PSXISO"))
                        CopyFile(LASTGAME_PATH "USRDIR/PS1.PNG", LASTGAME_PATH "ICON0.PNG");
                    else if(strstr(directories[currentgamedir].path_name, "/PS2ISO"))
                        CopyFile(LASTGAME_PATH "USRDIR/PS2.PNG", LASTGAME_PATH "ICON0.PNG");
                    else if(strstr(directories[currentgamedir].path_name, "/PSPISO"))
                        CopyFile(LASTGAME_PATH "USRDIR/PSP.PNG", LASTGAME_PATH "ICON0.PNG");
                    else
                        CopyFile(LASTGAME_PATH "USRDIR/ORG.PNG", LASTGAME_PATH "ICON0.PNG");

                    if(strstr(directories[currentgamedir].path_name, "/PS3ISO"))
                    {
                        if(file_exists(directories[currentgamedir].path_name) && strcasestr(directories[currentgamedir].path_name, ".iso"))
                        {
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/ICON0.PNG;1", LASTGAME_PATH "ICON0.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/ICON1.PAM;1", LASTGAME_PATH "ICON1.PAM");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/SND0.AT3;1" , LASTGAME_PATH "SND0.AT3");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/PIC0.PNG;1" , LASTGAME_PATH "PIC0.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/PIC1.PNG;1" , LASTGAME_PATH "PIC1.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PS3_GAME/PIC2.PNG;1" , LASTGAME_PATH "PIC2.PNG");
                        }
                    }
                    else if(strstr(directories[currentgamedir].path_name, "/PSPISO"))
                    {
                        if(file_exists(directories[currentgamedir].path_name) && strcasestr(directories[currentgamedir].path_name, ".iso"))
                        {
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/ICON0.PNG", LASTGAME_PATH "ICON0.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/ICON1.PAM", LASTGAME_PATH "ICON1.PAM");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/SND0.AT3" , LASTGAME_PATH "SND0.AT3");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/PIC0.PNG" , LASTGAME_PATH "PIC0.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/PIC1.PNG" , LASTGAME_PATH "PIC1.PNG");
                            ExtractFileFromISO(directories[currentgamedir].path_name, "/PSP_GAME/PIC2.PNG" , LASTGAME_PATH "PIC2.PNG");
                        }
                    }
                    else
                    {
                        sprintf(tmp_path, "%s/PS3_GAME/PIC0.PNG", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH "PIC0.PNG");

                        sprintf(tmp_path, "%s/PS3_GAME/PIC1.PNG", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH "PIC1.PNG");

                        sprintf(tmp_path, "%s/PS3_GAME/PIC2.PNG", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH "PIC2.PNG");

                        sprintf(tmp_path, "%s/PS3_GAME/ICON1.PAM", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH "ICON1.PAM");

                        sprintf(tmp_path, "%s/PS3_GAME/SND0.AT3", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH "SND0.AT3");

                        sprintf(tmp_path, "%s/PS3_GAME/PARAM.HIS", directories[currentgamedir].path_name);
                        if(file_exists(tmp_path))
                            CopyFile(tmp_path, LASTGAME_PATH "PARAM.HIS");
                    }

                    int size = 0;
                    char *sfo = NULL;

                    sfo = LoadFile(LASTGAME_PATH "USRDIR/ORG.SFO", &size);
                    if(sfo)
                    {
                        strncpy(sfo + 0x378, directories[currentgamedir].title, 64);
                        SaveFile(LASTGAME_PATH "PARAM.SFO", (char *) sfo, size);
                        if(sfo) free(sfo);
                    }
                    else
                        CopyFile(LASTGAME_PATH "USRDIR/ORG.SFO", LASTGAME_PATH "PARAM.SFO");

                    char titleid[10];
                    sprintf(titleid, "%c%c%c%c%s", directories[currentgamedir].title_id[0], directories[currentgamedir].title_id[1],
                                                   directories[currentgamedir].title_id[2], directories[currentgamedir].title_id[3], &directories[currentgamedir].title_id[5]);

                    sprintf(tmp_path, "%s/PS3_GAME/USRDIR/EBOOT.BIN", directories[currentgamedir].path_name);
                    if(file_exists(tmp_path))
                        sprintf(temp_buffer, "SELF=%s\nPATH=%s\nTITLEID=%s\n", tmp_path, directories[currentgamedir].path_name, titleid);
                    else
                        sprintf(temp_buffer, "SELF=%s\nTITLEID=%s\n", directories[currentgamedir].path_name, titleid);

                    // write LASTPLAY.BIN
                    FILE *fp;
                    fp = fopen(LASTGAME_PATH "USRDIR/LASTPLAY.BIN", "w");
                    fputs (temp_buffer, fp);
                    fclose(fp);
                }
#endif
                if(!strncmp(directories[currentgamedir].title_id, "HTSS00003", 9))
                {
                    if((old_pad & (BUTTON_SELECT | BUTTON_L2)) && file_exists("/dev_hdd0/game/HTSS00003/USRDIR/showtime.self"))
                    {
                        sysProcessExitSpawn2("/dev_hdd0/game/HTSS00003/USRDIR/showtime.self", NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
                        return r;
                    }
                    else
                        launch_showtime(false);
                }
                else if(!strncmp(directories[currentgamedir].title_id, NETHOST, 9))
                {
                    // Mount Network Game through webMAN
                    if(bAllowNetGames && get_net_status() == SUCCESS)
                    {
                        sprintf(tmp_path, "/mount_ps3/%s", directories[currentgamedir].path_name);

                        call_webman(tmp_path);

                        SaveGameList();

                        exit_to_xmb();
                    }

                    return r;
                }
                else if(strlen(directories[currentgamedir].title_id) == 9 && strstr(custom_homebrews, directories[currentgamedir].title_id) != NULL)
                {
                    if(file_exists(directories[currentgamedir].path_name))
                        sysProcessExitSpawn2(directories[currentgamedir].path_name, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);

                    return r;
                }

                // Launch Retro / ROM
                bool is_retro = ((directories[currentgamedir].flags & (RETRO_FLAG)) == (RETRO_FLAG)) &&
                                (strlen(retro_root_path) > 0 && strstr(directories[currentgamedir].path_name, retro_root_path) != NULL);

                if(is_retro)
                {
                    char rom_path[MAXPATHLEN];
                    sprintf(rom_path, "%s", directories[currentgamedir].path_name);
                    launch_retro(rom_path);
                    return r;
                }


                // Mount PS2 Classic ISO (*.ENC.BIN)
                bool is_ps2_classic = ((directories[currentgamedir].flags & (PS2_CLASSIC_FLAG)) == (PS2_CLASSIC_FLAG)) &&
                                      (strlen(ps2classic_path) > 0 && strstr(directories[currentgamedir].path_name, ps2classic_path) != NULL);

                if(is_ps2_classic)
                {
                    launch_ps2classic(directories[currentgamedir].path_name, directories[currentgamedir].title);
                    return r;
                }
/*
                if(!is_ps3hen && unsupported_cfw)
                {
                    DrawDialogOKTimer("ERROR: This function is not supported in this CFW", 3000.0f);
                    return r;
                }
*/
                if((use_cobra || use_mamba) && strstr(directories[currentgamedir].path_name, "/dev_usb")!=NULL) reset_usb_ports(directories[currentgamedir].path_name);

                if((directories[currentgamedir].flags & (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG)) == (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG) || (directories[currentgamedir].flags & D_FLAG_HOMEB)) ;
                else if((game_cfg.direct_boot == 3) || manager_cfg.global_wm_autoplay || strstr(directories[currentgamedir].path_name, "[auto]")!=NULL) {game_cfg.direct_boot = 0; SaveFile((char*)"/dev_hdd0/tmp/wm_request", (char*)"GET /play.ps3\n", 14);}

                if((directories[currentgamedir].flags & (BDVD_FLAG | HOMEBREW_FLAG | PS1_FLAG)) == PS1_FLAG)
                {
                    if(!(directories[currentgamedir].flags & PS3_FLAG))
                    {
                        if (strstr(directories[currentgamedir].path_name, "/PSXISO") != NULL)
                            launch_iso_game(directories[currentgamedir].path_name, 0);
                        else
                        {
                            reset_sys8_path_table();

                            //syscall36("/dev_bdvd");
                            add_sys8_bdvd(NULL, NULL);

                            if(lv2peek(0x80000000000004E8ULL) && !use_cobra) syscall_40(1, 0); // disables PS3 Disc-less

                            // load PSX options
                            LoadPSXOptions(directories[currentgamedir].path_name);

                            if(psx_iso_prepare(directories[currentgamedir].path_name, directories[currentgamedir].title, NULL) == 0)
                                return r;

                            psx_launch();
                        }
                        return r;
                    }
                }

                if(/*noBDVD &&*/ (use_cobra || use_mamba) && (directories[currentgamedir].flags & (BDVD_FLAG | HOMEBREW_FLAG | PS3_FLAG)) == PS3_FLAG)
                {
                    mount_iso_game();
                    return r;
                }

                if(!(directories[currentgamedir].flags & BDVD_FLAG) && !(directories[currentgamedir].flags & D_FLAG_HOMEB))
                {
                    memcpy((char *) temp_buffer, (char *) directories[currentgamedir].title_id, 4);
                    strncpy((char *) &temp_buffer[4], (char *) &directories[currentgamedir].title_id[5], 58);

                    sprintf(tmp_path, "/dev_hdd0/game/%s/USRDIR/EBOOT.BIN", temp_buffer);

                    if(file_exists(tmp_path) == false)
                        sprintf(tmp_path, "%s/PS3_GAME/USRDIR/EBOOT.BIN", directories[currentgamedir].path_name);

                    if(file_exists(tmp_path))
                    {
                        int r = patch_exe_error_09(tmp_path);
                        if(r == 1)
                        {
                            pause_music(1);

                            test_game(currentgamedir);
                        }
                        else if(r == -1)
                        {
                            DrawDialogOKTimer("This game requires a higher CFW or rebuild the SELFs/SPRX\n\nEste juego requiere un CFW superior o reconstruir los SELFs/SPRX\n\nCe jeu à besoin d'un CFW supérieur ou reconstruire les SELFs/SPRX", 2000.0f);
                            return r;
                        }
                    }

                    if(noBDVD && use_cobra)
                    {
                        char *files[1];

                        memcpy((char *) temp_buffer, (char *) directories[currentgamedir].title_id, 4);
                        strncpy((char *) &temp_buffer[4], (char *) &directories[currentgamedir].title_id[5], 58);

                        char *blank_iso = build_blank_iso((char *) temp_buffer);

                        if (blank_iso)
                        {
                            files[0] = blank_iso;
                            int ret = cobra_mount_ps3_disc_image(files, 1);
                            free(blank_iso);

                            if (ret == 0)
                            {
                                cobra_send_fake_disc_insert_event();

                                //DrawDialogOKTimer("PS3 Disc inserted", 2000.0f);
                            }
                        }
                    }
                }
                if(!((directories[currentgamedir].flags & (BDVD_FLAG | PS1_FLAG)) == (BDVD_FLAG | PS1_FLAG)) || // !psx CD
                    (directories[currentgamedir].flags & D_FLAG_HOMEB))
                {  // !homebrew
                    if(!(directories[currentgamedir].flags & BDVD_FLAG) && lv2peek(0x80000000000004E8ULL))
                        set_BdId(currentgamedir);
                }
                if(directories[currentgamedir].splitted == 1)
                {
                    if( payload_mode >= ZERO_PAYLOAD )
                    {
                        sprintf(tmp_path, "%s/cache/%s/%s", self_path, directories[currentgamedir].title_id, "/paths.dir");

                        if(file_exists(tmp_path) == false)
                        {
                            sprintf(temp_buffer, "%s\n\n%s", directories[currentgamedir].title, language[DRAWSCREEN_MARKNOTEXEC]);
                            DrawDialogOK(temp_buffer);

                            copy_to_cache(currentgamedir, self_path);

                            sprintf(tmp_path, "%s/cache/%s/%s", self_path, directories[currentgamedir].title_id, "/paths.dir");
                            if(file_exists(tmp_path) == false) return r; // cannot launch without cache files
                        }

                        use_cache = true;
                    }
                    else
                    {
                        sprintf(temp_buffer, "%s\n\n%s", directories[get_currentdir(i)].title, language[DRAWSCREEN_MARKNOTEX4G]);
                        DrawDialogOK(temp_buffer);
                        return r;
                    }
                }

                /// cache
                u8  global_bd_mirror = manager_cfg.global_bd_mirror;
                if((directories[currentgamedir].flags & D_FLAG_HOMEB) == D_FLAG_HOMEB || (directories[currentgamedir].flags & PS1_FLAG) == PS1_FLAG) global_bd_mirror = 0;

                if(use_cache && (global_bd_mirror ||
                                 (game_cfg.bdemu     == 1 &&  (directories[currentgamedir].flags & D_FLAG_HDD0)) ||
                                 (game_cfg.bdemu_ext == 1 && !(directories[currentgamedir].flags & D_FLAG_HDD0))))
                {

                    DrawDialogOKTimer("BD EMU cannot work with big files in cache data\n\nBD EMU no puede trabajar con ficheros grandes en cache de datos\n\nBD EMU ne peut pas travailler avec de gros fichiers en Cache", 2000.0f);
                    return r;
                }
                if(game_cfg.exthdd0emu)
                {
                    if((directories[currentgamedir].flags & D_FLAG_USB) != 0)
                    {
                        for(n = 1; n < BDVD_DEVICE; n++) if((directories[currentgamedir].flags  & GAMELIST_FILTER) == (1 << n)) break;

                        sprintf(tmp_path, "/dev_usb00%c/GAMEI", 47 + n);
                        mkdir_secure(tmp_path);
                        add_sys8_path_table(self_path, self_path);

                        if(global_bd_mirror || ((directories[currentgamedir].flags & D_FLAG_HDD0) && game_cfg.bdemu) || (!(directories[currentgamedir].flags & D_FLAG_HDD0) && game_cfg.bdemu_ext))
                            add_sys8_path_table("/dev_hdd0/game", "/dev_bdvd/GAMEI");
                        else
                            add_sys8_path_table("/dev_hdd0/game", tmp_path);

                    }
                    else if((fdevices & D_FLAG_USB) != 0)
                    {
                        for(n = 1; n < BDVD_DEVICE; n++)
                        {
                            // searching directory
                            if(fdevices & (1 << n))
                            {
                                DIR  *dir;

                                sprintf(tmp_path, "/dev_usb00%c/GAMEI", 47 + n);
                                dir = opendir(tmp_path);
                                if(dir)
                                {
                                    closedir(dir);

                                    add_sys8_path_table(self_path, self_path);
                                    add_sys8_path_table("/dev_hdd0/game", tmp_path);
                                    break;
                                }
                            }
                        }

                        if(n == BDVD_DEVICE)
                        {
                             // directory not found, Asking to create one
                             for(n = 1; n < BDVD_DEVICE ; n++)
                             {
                                if(fdevices & (1 << n))
                                {
                                    sprintf(temp_buffer, "%s\n\n%s%c?", language[DRAWSCREEN_GAMEINOFMNT], language[DRAWSCREEN_GAMEIASKDIR], 47 + n);
                                    if(DrawDialogYesNo(temp_buffer) == YES)
                                    {
                                        sprintf(tmp_path, "/dev_usb00%c/GAMEI", 47 + n);
                                        mkdir_secure(tmp_path);
                                        add_sys8_path_table(self_path, self_path);
                                        add_sys8_path_table("/dev_hdd0/game", tmp_path);
                                        break;
                                    }
                                }
                             }

                             if(n == BDVD_DEVICE)
                             {
                                 sprintf(temp_buffer, "%s\n\n%s", language[DRAWSCREEN_GAMEICANTFD], language[DRAWSCREEN_GAMEIWLAUNCH]);
                                 if(DrawDialogYesNo(temp_buffer) != YES) return r;
                             }
                        }
                    }

                    if((fdevices & D_FLAG_USB) == 0)
                    {
                        sprintf(temp_buffer, "%s\n\n%s", language[DRAWSCREEN_GAMEICANTFD], language[DRAWSCREEN_GAMEIWLAUNCH]);
                        if(DrawDialogYesNo(temp_buffer) != YES) return r;
                    }

                }
                if((game_cfg.useBDVD && (fdevices & D_FLAG_BDVD) == 0) || (game_cfg.direct_boot == 2))
                {
                    load_from_bluray |= 1;
                    if((noBDVD == MODE_WITHBDVD) && check_disc() == -1)
                    return r;
                }
                if(!(directories[currentgamedir].flags & D_FLAG_BDVD))
                    param_sfo_util(directories[currentgamedir].path_name, (game_cfg.updates != 0));

                // Fix PS3_EXTRA flag in PARAM.SFO
                fix_PS3_EXTRA_attribute(directories[currentgamedir].path_name);

                if(!game_cfg.ext_ebootbin)
                    sys8_path_table(0LL);
                else
                {
                    set_device_wakeup_mode(bdvd_is_usb ? 0xFFFFFFFF : directories[currentgamedir].flags);

                    sprintf(tmp_path, "%s/self", self_path);
                    mkdir_secure(tmp_path);

                    sprintf(tmp_path, "%s/self/%s.BIN", self_path, directories[get_currentdir(i)].title_id);
                    FILE *fp = fopen(tmp_path, "rb");

                    if(!fp)
                    {
                        sprintf(temp_buffer, " %s.BIN\n %s\n\n%s", directories[currentgamedir].title_id, language[DRAWSCREEN_EXTEXENOTFND], language[DRAWSCREEN_EXTEXENOTCPY]);
                        DrawDialogOK(temp_buffer);
                        goto skip_sys8;
                    }
                    else
                    {
                        fclose(fp);
                        add_sys8_path_table("/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN", tmp_path);
                    }
                }

                sprintf(tmp_path, "%s/PS3_GAME", directories[currentgamedir].path_name);
                if(is_ps3hen || ((use_cobra || use_mamba) && (!(global_bd_mirror | game_cfg.bdemu | game_cfg.bdemu_ext)) && file_exists(tmp_path)))
                {   // mount ps3 game as /dev_bdvd & /app_home
/*                    if(is_ps3hen)
                    {
//                       sprintf(tmp_path, "%s/PS3_GAME/LICDIR/LIC.DAT", directories[currentgamedir].path_name);
//                       if(file_exists(tmp_path))
//                       {
//                           {lv2syscall2(35, (u64)(char*)"/dev_bdvd/PS3_GAME/LICDIR/LIC.DAT", (u64)(char*)"/dev_bdvd/PS3_GAME/LICDIR/LIC.BAK");}
//                           {lv2syscall2(35, (u64)(char*)"/app_home/PS3_GAME/LICDIR/LIC.DAT", (u64)(char*)"/dev_bdvd/PS3_GAME/LICDIR/LIC.BAK");}
//                           {lv2syscall2(35, (u64)(char*)"/app_home/USRDIR/LICDIR/LIC.DAT", (u64)(char*)"/dev_bdvd/PS3_GAME/LICDIR/LIC.BAK");}
//                       }
//                      sprintf(tmp_path, "%s/PS3_GAME", directories[currentgamedir].path_name);
//                      {lv2syscall2(35, (u64)(char*)"/app_home/PS3_GAME", (u64)(char*)tmp_path);}

//                      sprintf(tmp_path, "%s/PS3_GAME/USRDIR", directories[currentgamedir].path_name);
//                      {lv2syscall2(35, (u64)(char*)"/app_home/USRDIR", (u64)(char*)tmp_path);}

//                      sprintf(tmp_path, "%s/PS3_GAME/USRDIR/", directories[currentgamedir].path_name);
//                      {lv2syscall2(35, (u64)(char*)"/app_home/", (u64)(char*)tmp_path);}

                        {lv2syscall2(35, (u64)(char*)"/app_home", (u64)(char*)directories[currentgamedir].path_name);}
                        {lv2syscall2(35, (u64)(char*)"/dev_bdvd", (u64)(char*)directories[currentgamedir].path_name);}
                        {lv2syscall2(35, (u64)(char*)"//dev_bdvd", (u64)(char*)directories[currentgamedir].path_name);}
                    }
                    else
*/
                    {
                        sys8_disable(0ULL);
                        wm_cobra_map_game((char*)directories[currentgamedir].path_name, (char*)directories[currentgamedir].title_id);
                    }
                    goto exit_mount;
                }

                load_from_bluray = game_cfg.useBDVD;

                if(!game_cfg.useBDVD || noBDVD) sys8_sys_configure(CFG_XMB_DEBUG); else sys8_sys_configure(CFG_XMB_RETAIL);


                sys8_sys_configure(CFG_UNPATCH_APPVER + (game_cfg.updates != 0));

                if((directories[currentgamedir].flags & D_FLAG_HOMEB) == D_FLAG_HOMEB || (directories[currentgamedir].flags & PS1_FLAG) == PS1_FLAG) ;

                else if(global_bd_mirror ||
                  (game_cfg.bdemu     &&  (directories[currentgamedir].flags & D_FLAG_HDD0)) ||
                  (game_cfg.bdemu_ext && !(directories[currentgamedir].flags & D_FLAG_HDD0)))
                {
                    load_from_bluray |= 1;

                    if((noBDVD == MODE_WITHBDVD) && check_disc() == -1) return r;

                    if((noBDVD == MODE_WITHBDVD) || use_cobra)
                        sys_fs_mount("CELL_FS_IOS:BDVD_DRIVE", "CELL_FS_ISO9660", "/dev_ps2disc", 1);

                    if((game_cfg.bdemu     == 2 &&  (directories[currentgamedir].flags & D_FLAG_HDD0)) ||
                       (game_cfg.bdemu_ext == 2 && !(directories[currentgamedir].flags & D_FLAG_HDD0)))
                    {
                         // new to add BD-Emu 2
                         n = 0;
                    }
                    else
                    {
                        n = move_origin_to_bdemubackup(directories[currentgamedir].path_name);
                        sprintf(tmp_path, "%s/PS3_DISC.SFB", directories[currentgamedir].path_name);
                        add_sys8_path_table("/dev_bdvd/PS3_DISC.SFB", tmp_path);
                    }

                    if(n < 0)
                    {
                        //syscall36("/dev_bdvd"); // in error exits
                        //sys8_perm_mode((u64) 0);

                        add_sys8_bdvd(NULL, NULL);

                        //if(game_cfg.ext_ebootbin)
                        build_sys8_path_table(); //prepare extern eboot

                        exit_program = true;
                        return r;
                    }

                    is_ps3game_running = true;

                   // if(n == 1) game_cfg.bdemu = 0; // if !dev_usb... bdemu is not usable
                }

                // HDD BDEMU-LIBFS / USB BDEMU
                if((global_bd_mirror ||
                    (game_cfg.bdemu          &&  (directories[currentgamedir].flags & D_FLAG_HDD0)) ||
                    (game_cfg.bdemu_ext == 1 && !(directories[currentgamedir].flags & D_FLAG_HDD0))) &&
                     patch_bdvdemu(directories[currentgamedir].flags & GAMELIST_FILTER) == 0)
                {
                    // syscall36("//dev_bdvd"); // for hermes special flag see syscall36-3.41-hermes "//"
                    add_sys8_bdvd(NULL, NULL);

                    //we dont want libfs on USB devices
                    if(is_libfs_patched() && (game_cfg.bdemu && (directories[currentgamedir].flags & D_FLAG_HDD0)))
                    {
                        sprintf(tmp_path, "%s/libfs_patched.sprx", self_path);
                        add_sys8_path_table("/dev_flash/sys/external/libfs.sprx", tmp_path);
                        sys8_pokeinstr(0x80000000007EF220ULL, 0x45737477616C6420ULL);
                    }


                    // HDD LIBFS
                   if((game_cfg.bdemu == 2 && (directories[currentgamedir].flags & D_FLAG_HDD0)))
                   {    // new to add BD-Emu 2
                        mount_custom(directories[currentgamedir].path_name);

                        if(!game_cfg.useBDVD)
                        {
                            sprintf(tmp_path, "%s/PS3_DISC.SFB", directories[currentgamedir].path_name);
                            add_sys8_path_table("/app_home/PS3_DISC.SFB", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME", directories[currentgamedir].path_name);
                            add_sys8_path_table("/app_home/PS3_GAME", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME/USRDIR", directories[currentgamedir].path_name);
                            add_sys8_path_table("/app_home/USRDIR", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME/USRDIR", directories[currentgamedir].path_name);
                            add_sys8_path_table("/app_home", tmp_path);
                        }

                        add_sys8_bdvd(directories[currentgamedir].path_name, NULL);

                        //syscall36(directories[currentgamedir].path_name);
                   }
                   else
                   {
                        mount_custom(directories[currentgamedir].path_name);
                        if(!game_cfg.useBDVD)
                        {
                            sprintf(tmp_path, "%s/PS3_DISC.SFB", "/dev_bdvd");
                            add_sys8_path_table("/app_home/PS3_DISC.SFB", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME", "/dev_bdvd");
                            add_sys8_path_table("/app_home/PS3_GAME", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME/USRDIR", "/dev_bdvd");
                            add_sys8_path_table("/app_home/USRDIR", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME/USRDIR", "/dev_bdvd");
                            add_sys8_path_table("/app_home", tmp_path);
                        }
                   }

                }
                else
                {
                    // USB LIBFS (for cached games)
                    if(is_libfs_patched() && (!(directories[currentgamedir].flags & D_FLAG_HDD0) && game_cfg.bdemu_ext == 2) &&
                       !(directories[currentgamedir].flags & D_FLAG_BDVD))
                    {
                            // only with external bdemu LIBFS for cached files
                            sprintf(tmp_path, "%s/libfs_patched.sprx", self_path);
                            add_sys8_path_table("/dev_flash/sys/external/libfs.sprx", tmp_path);
                            sys8_pokeinstr(0x80000000007EF220ULL, 0x45737477616C6420ULL);
                    }

                    if(use_cache)
                    {
                        sprintf(tmp_path, "%s/cache/%s/%s", self_path,  //check replace (1024)
                        directories[currentgamedir].title_id, "/paths.dir");

                        char *path = LoadFile(tmp_path, &n);
                        char *mem = path;

                        if(path && path[0x400] == 0 && path[0x420] != 0)
                        {
                            // repair bug in some Iris Manager versions
                            memcpy(&path[0x400], &path[0x420], 0x380);
                            SaveFile(tmp_path, path, n);
                        }

                        n = n & ~2047; // if file truncated break bad datas...

                        if(path)
                        {
                            while(n > 0)
                            {
                                char *t = path;

                                sprintf(tmp_path, "%s/cache/%s/%s", self_path, directories[currentgamedir].title_id, path + 0x400);

                                path = strstr(path, "PS3_GAME/");
                                sprintf(temp_buffer, "/dev_bdvd/%s", path);

                                add_sys8_path_table(temp_buffer, tmp_path);

                                path = t + 2048;
                                n   -= 2048;
                            }
                            free(mem);
                        }
                    }

                    if(!(directories[currentgamedir].flags & D_FLAG_HOMEB) &&
                        (directories[currentgamedir].flags & GAME_FLAGS) == D_FLAG_PSX_ISO)
                    {   // add PSX iso
                        if(!lv2_patch_storage && (directories[currentgamedir].flags & BDVD_FLAG))
                        {
                            DrawDialogOKTimer("PSX Unsupported", 2000.0f);
                            return r;
                        }

                        //syscall36("/dev_bdvd");
                        add_sys8_bdvd(NULL, NULL);

                        if(!(directories[currentgamedir].flags & BDVD_FLAG))
                        {
                            if(lv2peek(0x80000000000004E8ULL) && !use_cobra) syscall_40(1, 0); // disables PS3 Disc-less

                            // load PSX options
                            LoadPSXOptions(directories[currentgamedir].path_name);

                            if(psx_iso_prepare(directories[currentgamedir].path_name, directories[currentgamedir].title, NULL) == 0)
                                return r;
                        }
                        else
                            psx_cd_with_cheats();

                        set_device_wakeup_mode(bdvd_is_usb ? 0xFFFFFFFF : directories[currentgamedir].flags);

                        psx_launch();
                    }
                    else if(directories[currentgamedir].flags & D_FLAG_HOMEB)
                    {   // is homebrew
                        reset_sys8_path_table();

                        if((directories[currentgamedir].flags & D_FLAG_HOMEB_MKV) == D_FLAG_HOMEB_BD)
                        {
                            // Is BDISO
                            launch_iso_game(directories[currentgamedir].path_name, EMU_BD);        // launch BD Video
                            return r;
                        }
                        else if(directories[currentgamedir].flags & D_FLAG_HOMEB_DVD)
                        {
                            // Is DVDISO or MKV
                            int p = strlen(directories[currentgamedir].path_name);
                            if(is_audiovideo(&directories[currentgamedir].path_name[p - 4]))
                            {
                                sprintf(temp_buffer, "%s", directories[currentgamedir].path_name); // launch MKV/MP4/AVI
                                sprintf(temp_buffer + 2048, "%s/USRDIR/TEMP/showtime.iso", self_path);
                                launch_iso_build(temp_buffer + 2048, temp_buffer, true);
                            }
                            else
                            {
                                launch_iso_game(directories[currentgamedir].path_name, EMU_DVD);   // launch DVD Video
                            }

                            return r;
                        }

                        add_sys8_bdvd(NULL, "/dev_kk");

                        sprintf(temp_buffer, "%s/PARAM.SFO", directories[currentgamedir].path_name);

                        if(homelaun == 2)
                        {
                            // test if same title id to skip homelaun pass
                            if(!parse_param_sfo_id("/dev_hdd0/game/HOMELAUN1/PARAMX.SFO", temp_buffer + 1024))
                                if(strcmp(directories[currentgamedir].title_id, temp_buffer + 1024)) {homelaun = 1;}
                        }

                        i = param_sfo_patch_category_to_cb(temp_buffer, "/dev_hdd0/game/HOMELAUN1/PARAMX.SFO");

                        switch(i)
                        {
                          case -1:
                            DrawDialogOK("Error: PARAM.SFO was not found for HOMELAUN1 (game broken)");
                            goto skip_homebrew;
                          case -2:
                            DrawDialogOK("Error: External USB Loader not found! (install HOMELAUN1)");
                            goto skip_homebrew;
                        }

                        sprintf(temp_buffer, "%s", directories[currentgamedir].path_name);

                        sprintf(temp_buffer + 1024, "%s/homelaunc1.bin", self_path);
                        sprintf(temp_buffer + 2048, "%s/homelaunc1.bin", self_path);
                        SaveFile(temp_buffer + 2048, temp_buffer, 2048);

                        i = strlen(directories[currentgamedir].path_name);
                        while(directories[currentgamedir].path_name[i] != '/') i--;

                        sprintf(temp_buffer + 1024, "/dev_hdd0/game%s", &directories[currentgamedir].path_name[i]);

                        if(homelaun == 2)
                            add_sys8_path_table("/dev_hdd0/game/HOMELAUN1/ICON0.PNG", "//dev_hdd0/game/HOMELAUN1/GICON0.PNG");
                        else
                            add_sys8_path_table("/dev_hdd0/game/HOMELAUN1/ICON0.PNG", "//dev_hdd0/game/HOMELAUN1/BICON0.PNG");

                        if(homelaun != 2)
                            add_sys8_path_table("/dev_hdd0/game/HOMELAUN1/USRDIR/EBOOT.BIN", "//dev_hdd0/game/HOMELAUN1/USRDIR/EBOOT.BIN");

                        if(homelaun != 1)
                            add_sys8_path_table("/dev_hdd0/game/HOMELAUN1/PARAM.SFO", "//dev_hdd0/game/HOMELAUN1/PARAMX.SFO");
                        else
                            add_sys8_path_table("/dev_hdd0/game/HOMELAUN1/PARAM.SFO", "//dev_hdd0/game/HOMELAUN1/PARAM.SFO");

                        add_sys8_path_table("/dev_hdd0/game/HOMELAUN1/path.bin", temp_buffer + 2048);
                        if(homelaun == 2)
                            add_sys8_path_table("/dev_hdd0/game/HOMELAUN1/path2.bin", temp_buffer + 2048);

                        add_sys8_path_table("/dev_hdd0/game/HOMELAUN1", temp_buffer);

                        for(i = HDD0_DEVICE; i < BDVD_DEVICE; i++) if((directories[currentgamedir].flags>>i) & 1) break;

                        if(i != HDD0_DEVICE)
                            add_sys8_path_table(temp_buffer + 1024, temp_buffer);

                        build_sys8_path_table();
                        set_device_wakeup_mode(bdvd_is_usb ? 0xFFFFFFFF : directories[currentgamedir].flags);
                        game_cfg.direct_boot = 0;

                        exit_to_xmb();

                        //////////////
                        skip_homebrew: ;
                        //////////////
                    }
                    else
                    {
                        mount_custom(directories[currentgamedir].path_name);
                        if(!game_cfg.useBDVD)
                        {
                            sprintf(tmp_path, "%s/PS3_DISC.SFB", directories[currentgamedir].path_name);
                            add_sys8_path_table("/app_home/PS3_DISC.SFB", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME", directories[currentgamedir].path_name);
                            add_sys8_path_table("/app_home/PS3_GAME", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME/USRDIR", directories[currentgamedir].path_name);
                            add_sys8_path_table("/app_home/USRDIR", tmp_path);

                            sprintf(tmp_path, "%s/PS3_GAME/USRDIR", directories[currentgamedir].path_name);
                            add_sys8_path_table("/app_home", tmp_path);
                        }

                        is_ps3game_running = true;

                        //syscall36(directories[currentgamedir].path_name); // is bdvd game

                        add_sys8_bdvd(directories[currentgamedir].path_name, NULL);
                    }
                }

                if(noBDVD && !use_cobra)
                {
                    patch_bdvdemu(NTFS_FLAG/*directories[currentgamedir].flags & GAMELIST_FILTER*/);

                    if(load_from_bluray)
                    {
                        if((firmware & 0xF) == 0xD)
                            sprintf(tmp_path, "%s/explore_plugin_%xdex.sprx", self_path, firmware>>4);
                        else
                            sprintf(tmp_path, "%s/explore_plugin_%x.sprx", self_path, firmware>>4);

                        if(!file_exists(tmp_path))
                        {
                             if(noBDVD == MODE_DISCLESS)
                                 strcat(tmp_path, " not found\n\nIt reduces the game compatibility");
                             else
                                 strcat(tmp_path, " not found\n\npath from /app_home");

                             DrawDialogOKTimer(tmp_path + strlen(self_path) + 1, 2000.0f);
                        }
                        else
                            add_sys8_path_table("/dev_flash/vsh/module/explore_plugin.sprx", tmp_path);
                    }

                }

                build_sys8_path_table();

    exit_mount:
                exit_program = true;

                if(game_cfg.direct_boot==1 || game_cfg.direct_boot==2) game_cfg.direct_boot = 555;

                set_device_wakeup_mode(bdvd_is_usb ? 0xFFFFFFFF : directories[currentgamedir].flags);
                skip_sys8:
                return r;
            }
        }
    }

    // don't remove this code
    if(mode_favourites < 0x10000) set_last_game();

    if(new_pad & BUTTON_START)
    {
        if(old_pad & (BUTTON_L2 | BUTTON_R2))
        {
            if (old_pad & BUTTON_L2)
            {
                if(gui_mode == MODE_COVERFLOW)
                {
                    if(manager_cfg.gui_mode == MODE_COVERFLOW)
                        gui_mode = MODE_GRID_6x4;
                    else
                        gui_mode = manager_cfg.gui_mode & 0xF;
                }
                else
                    gui_mode = MODE_COVERFLOW;

                if(gui_mode > MODE_GRID_6x5 && bk_picture == BG_PIC1) bk_picture = BG_NONE;
            }
            else
            {
                gui_mode++;
                if(gui_mode > GUI_MODES)
                {
                    gui_mode = MODE_XMB_LIKE;
                    ROT_INC(cover_mode, 3, 0);
                }

                if(gui_mode > MODE_GRID_6x5 && bk_picture == BG_PIC1) bk_picture = BG_NONE;

                manager_cfg.cover_mode = cover_mode;
                manager_cfg.gui_mode = ((sort_mode & 0xf)<<4) | (gui_mode & 0xf);
                SaveManagerCfg();
            }

            get_grid_dimensions(true);
            load_background_picture();

            get_games();

            new_pad = 0;

            return_to_game_list(false);
        }
        else if(old_pad & BUTTON_SELECT)
        {
            if(strncmp(directories[currentgamedir].title_id, NETHOST, 9) != SUCCESS)
            {
                strcpy(tmp_path, directories[currentgamedir].path_name);

                int flen = strlen(get_extension(tmp_path));

                if(flen)
                {
                    flen = strlen(get_filename(tmp_path));
                    if(flen > 0 && strlen(tmp_path) > flen)
                    {
                        tmp_path[strlen(tmp_path) - flen - 1] = 0;
                    }
                }
            }
            else
                memset(tmp_path, 0, MAXPATHLEN);

            if (file_manager(tmp_path, NULL) == REFRESH_GAME_LIST) return_to_game_list(true);

            get_grid_dimensions(true);
            load_background_picture();

            frame_count = 0xFF; // force display temp
            return r;
        }
        else
        {
            // Show Global Options menu
            if(options_locked)
                DrawDialogOKTimer(language[DRAWSCREEN_PARCONTROL], 2000.0f);
            else
            {
                app_ver[0] = 0;
                menu_screen = SCR_MENU_GLOBAL_OPTIONS;
                select_option = -1; // default: tools option
                return r;
            }
        }
    }

    if(new_pad & BUTTON_TRIANGLE)
    {
        if(options_locked)
            DrawDialogOKTimer(language[DRAWSCREEN_PARCONTROL], 2000.0f);
        else
        {
            if(old_pad & BUTTON_SELECT) {bShowVersion = 1; bShowPIC1 = 1;} // SELECT + /\ = force show version / pic1 on ISO

            i = selected;
            app_ver[0] = 0;

            int_currentdir = currentdir;
            int indx = mode_favourites ? favourites.list[i].index : (currentdir + i);

            if(strncmp(directories[indx].title_id, NETHOST, 9) == SUCCESS)
            {
                sprintf(temp_buffer, (strncmp(directories[indx].path_name, "net", 3) ? "%s\n\nPath:\n%s" : "%s\n\nNetwork Path:\n%s"),
                                                                                       directories[indx].title, directories[indx].path_name);

                urldec(temp_buffer);

                DrawDialogOKTimer(temp_buffer, 5000.0f);
                return r;
            }

            select_option = 0;

            if(!Png_offset[i] && mode_favourites && mode_favourites < 0x10000 && favourites.list[i].title_id[0] != 0)
            {
                DeleteFavouritesIfExits(favourites.list[i].title_id);

                sprintf(tmp_path, "%s/config/", self_path);
                SaveFavourites(tmp_path, mode_homebrew);

                if(mode_favourites && !havefavourites) mode_favourites = 0;
                get_games();
                return r;
            }

            if(Png_offset[i])
            {
                Png_offset[BIG_PICT] = 0;
                wait_event_thread();

                currentgamedir = get_currentdir(i);
                int_currentdir = currentdir;

                if(gui_mode > MODE_GRID_6x5) ; // avoid freeze on Game Options when large grids are used

                else if((bShowPIC1 == 1) || (bShowPIC1 == 2 && ((directories[indx].flags & (NTFS_FLAG)) != NTFS_FLAG)))
                {
                    // program new event thread function to show background picture
                    event_thread_send(0x555ULL, (u64) get_pict, (u64) &i);
                }

                if(currentgamedir >= 0 && currentgamedir < ndirectories)
                {
                    bool is_retro = ((directories[indx].flags & (RETRO_FLAG)) == (RETRO_FLAG)) &&
                                    (strlen(retro_root_path) > 0 && strstr(directories[indx].path_name, retro_root_path) != NULL);

                    if(is_retro) goto ask_delete_item;

                    bool is_ps2_classic = ((directories[indx].flags & (PS2_CLASSIC_FLAG)) == (PS2_CLASSIC_FLAG)) &&
                                          (strlen(ps2classic_path) > 0 && strstr(directories[indx].path_name, ps2classic_path) != NULL);

                    if(is_ps2_classic) goto ask_delete_item;


                    // access to PSX configuration menu
                    if((mode_homebrew == GAMEBASE_MODE) && (directories[indx].flags & (GAME_FLAGS)) == PS1_FLAG) // add PSX iso
                    {
                        if(!(directories[indx].flags & BDVD_FLAG))
                            LoadPSXOptions(directories[indx].path_name);
                        else
                            LoadPSXOptions(NULL);

                        if(get_psx_memcards() == 0)
                            menu_screen = SCR_MENU_PSX_OPTIONS;
                    }
                    else
                    {
                       if(is_retro || is_ps2_classic || (mode_homebrew && (directories[indx].flags & D_FLAG_HOMEB_GROUP) > D_FLAG_HOMEB))
                       {
ask_delete_item:
                           // SELECT + TRIANGLE or L2 + TRIANGLE = Delete movie
                           if(old_pad & (BUTTON_SELECT | BUTTON_L2) || bShowPath)
                           {
                                if(!strncmp(directories[indx].path_name, "/dev_hdd0", 9))
                                    sprintf(temp_buffer + 1024, "%s\n\n%s HDD0?", directories[indx].title, language[GAMEDELSL_WANTDELETE]);
                                else if(!strncmp(directories[indx].path_name, "/dev_usb", 8))
                                    sprintf(temp_buffer + 1024, "%s\n\n%s USB00%c?", directories[indx].title, language[GAMEDELSL_WANTDELETE], directories[currentgamedir].path_name[10]);
                                else if(!strncmp(directories[indx].path_name, "/ntfs", 5))
                                    sprintf(temp_buffer + 1024, "%s\n\n%s NTFS%c?", directories[indx].title, language[GAMEDELSL_WANTDELETE], directories[currentgamedir].path_name[5]);
                                else if(!strncmp(directories[indx].path_name, "/ext", 4))
                                    sprintf(temp_buffer + 1024, "%s\n\n%s EXT%c?", directories[indx].title, language[GAMEDELSL_WANTDELETE], directories[currentgamedir].path_name[4]);
                                else
                                    return r;

                                if(DrawDialogYesNo(temp_buffer + 1024) == YES)
                                {
                                    pause_music(1);

                                    delps3iso(directories[indx].path_name);

                                    pause_music(0);

                                    return_to_game_list(true);
                                }

                                return r;
                           }
                           else
                               bShowPath = true;

                           return r; // ignore Options menu
                       }

                       if((mode_homebrew == GAMEBASE_MODE) && (directories[indx].flags & (BDVD_FLAG | PS3_FLAG)) == PS3_FLAG)
                            menu_screen = SCR_MENU_ISO_OPTIONS;
                       else
                            menu_screen = SCR_MENU_GAME_OPTIONS;

                       load_gamecfg(RESET_GAME_INFO); // force refresh game info
                       load_gamecfg (get_currentdir(i));

                       if(!(directories[indx].flags & BDVD_FLAG) && lv2peek(0x80000000000004E8ULL))
                            set_BdId(indx);
                    }
                    return r;
                }
            }
        }
    }

    static int auto_up = 0, auto_down = 0, auto_left = 0, auto_right = 0;

/************************************************************************************************************/
/* GUI 0: GRID                                                                                              */
/************************************************************************************************************/
    if(gui_mode != MODE_COVERFLOW) {

    AUTO_BUTTON_REP(auto_up, BUTTON_UP)
    AUTO_BUTTON_REP(auto_down, BUTTON_DOWN)
    AUTO_BUTTON_REP(auto_left, BUTTON_LEFT)
    AUTO_BUTTON_REP(auto_right, BUTTON_RIGHT)

    if(new_pad & BUTTON_UP)
    {
        select_py--;

        auto_up = 1;

        if(select_py < 0)
        {
            select_py = (rows - 1);

            if(mode_favourites >= 0x10000) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir >= num_box) {mode_favourites = 0; currentdir -= num_box; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = ROUND_UPX(ndirectories) - num_box; get_games();}
        }

        enable_draw_background_pic1();
        return r;
    }
    else if(new_pad & BUTTON_DOWN)
    {
        select_py++;

        auto_down = 1;

        if(select_py > (rows - 1))
        {
            select_py = 0;

            if(mode_favourites >= 0x10000) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir < (ROUND_UPX(ndirectories) - num_box)) {mode_favourites = 0; currentdir += num_box; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = 0; get_games();}
        }

        enable_draw_background_pic1();
        return r;
    }
    else if(new_pad & BUTTON_LEFT)
    {
        select_px--;

        if(select_px < 0 && select_py == 0 && currentdir == 0 && ndirectories < 4 && !mode_favourites && !havefavourites)
        {
            select_px = ndirectories - 1;
            if(select_px < 0) select_px = 0;
            currentdir = 0;
        }

        auto_left = 1;

        if(select_px < 0)
        {
            select_px = (cols - 1);

            if(mode_favourites >= 0x10000) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir >= num_box) {mode_favourites = 0; currentdir -= num_box; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = ROUND_UPX(ndirectories) - num_box; get_games();}
        }

        GFX1_mode = 2; GFX1_counter = 20;

        enable_draw_background_pic1();
        return r;
    }
    else if(new_pad & BUTTON_RIGHT)
    {
        select_px++;

        auto_right = 1;

        if(select_px > (cols - 1))
        {
            select_px = 0;

            if(mode_favourites >= 0x10000) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir < (ROUND_UPX(ndirectories) - num_box)) {mode_favourites = 0; currentdir += num_box; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = 0; get_games();}
        }

        GFX1_mode = 1; GFX1_counter = 20;

        enable_draw_background_pic1();
        return r;
    }
    else if(new_pad & BUTTON_L1) //change page
    {
        if(gui_mode == MODE_XMB_LIKE)
        {
            if(old_pad & BUTTON_SELECT)
            {
                while((new_pad | old_pad) & BUTTON_L1) ps3pad_read();

                old_pad = BUTTON_L2 | new_pad;
                new_pad = BUTTON_R3;
            }
            else
            {
                if(mode_favourites >= 0x10000) ;
                else if(mode_favourites) {mode_favourites = 0; get_games();}
                else if(currentdir >= 10) {mode_favourites = 0; currentdir -= 10; get_games();}
                else {mode_favourites = (!mode_favourites && havefavourites); currentdir = ndirectories - 1; get_games();}

                GFX1_mode = 2; GFX1_counter = 20;

                enable_draw_background_pic1();
                return r;
            }
        }
        else
        {
            if(mode_favourites >= 0x10000) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir >= num_box) {mode_favourites = 0; currentdir -= num_box; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = ROUND_UPX(ndirectories) - num_box; get_games();}

            GFX1_mode = 2; GFX1_counter = 20;

            enable_draw_background_pic1();
            return r;
        }
    }
    else if(new_pad & BUTTON_R1) //change page
    {
        if(gui_mode == MODE_XMB_LIKE)
        {
            if(old_pad & BUTTON_SELECT)
            {
                while((new_pad | old_pad) & BUTTON_R1) ps3pad_read();

                new_pad = BUTTON_R3;
            }
            else
            {
                if(mode_favourites >= 0x10000) ;
                else if(mode_favourites) {mode_favourites = 0; get_games();}
                else if(currentdir < (ndirectories - 10)) {mode_favourites = 0; currentdir += 10; get_games();}
                else {mode_favourites = (!mode_favourites && havefavourites); currentdir = 0; get_games();}

                GFX1_mode = 1; GFX1_counter = 20;

                enable_draw_background_pic1();
                return r;
            }
        }
        else
        {
            if(mode_favourites >= 0x10000) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir < (ROUND_UPX(ndirectories) - num_box)) {mode_favourites = 0; currentdir += num_box; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = 0; get_games();}

            GFX1_mode = 1; GFX1_counter = 20;

            enable_draw_background_pic1();
            return r;
        }
    }
// end of GUI 0: GRID

    } else {

/************************************************************************************************************/
/* GUI 1: COVERFLOW                                                                                         */
/************************************************************************************************************/
    AUTO_BUTTON_REP3(auto_left, BUTTON_LEFT)
    AUTO_BUTTON_REP3(auto_right, BUTTON_RIGHT)

    num_box = 12;
    cols = 4;
    rows = 3;

    if(new_pad & BUTTON_UP)
    {
        while((new_pad | old_pad) & BUTTON_UP) ps3pad_read();

        auto_up = 1;
        anim_mode = 0; anim_step = 0;

        if(mode_favourites >= 0x10000) ;
        else if(mode_favourites)
        {
            mode_favourites = 0;
            currentdir = select_px = select_py = 0;
            get_games();
        }
        else
        {
            if(!havefavourites)
                new_pad = BUTTON_R3;
            else
            {
                mode_favourites = (!mode_favourites && havefavourites);
                currentdir = select_px = select_py = 0;
                get_games();
            }
        }

        if(havefavourites)
        {
            enable_draw_background_pic1();
            return r;
        }
    }
    else if(new_pad & BUTTON_DOWN)
    {
        auto_down = 1;
        anim_mode = 0; anim_step = 0;

        if(mode_favourites >= 0x10000) ;
        else if(mode_favourites)
        {
            mode_favourites = 0;
            currentdir = select_px = select_py = 0;
            get_games();
        }
        else
        {
            sort_mode++; if(sort_mode > 2) sort_mode = 0;

            sort_entries2(directories, &ndirectories, sort_mode);
            UpdateFavourites(directories, ndirectories);
            load_gamecfg(RESET_GAME_INFO); // force refresh game info
            manager_cfg.cover_mode = cover_mode;
            manager_cfg.gui_mode = ((sort_mode & 0xf)<<4) | (gui_mode & 0xf);
            SaveManagerCfg();

            currentdir = select_px = select_py = 0;
            get_games();
        }

        enable_draw_background_pic1();
        return r;
    }
    else if(new_pad & BUTTON_LEFT)
    {
        int tmp_currentdir = currentdir;
        int update = 0;

        select_px--;

        if(select_px < 0 && select_py == 0 && currentdir == 0 && ndirectories < 4 && !mode_favourites && !havefavourites)
        {
            select_px = ndirectories - 1;
            if(select_px < 0) select_px = 0;
            currentdir = 0;
        }

        auto_left = 1;

        if(select_px < 0)
        {
            select_px = 3;
            select_py--;

            if(select_py < 0)
            {
                select_py = 2;

                if(mode_favourites >= 0x10000) ;
                else if(mode_favourites)
                {
                    mode_favourites = 0; update = 1;
                    currentdir = ndirectories - 1;
                    if(currentdir < 0) currentdir = 0;
                    if(currentdir >= 3) {select_px = 3; currentdir-= 2;} else select_px = currentdir;
                }
                else
                    select_py = 5;
            }
        }

        if(!mode_favourites)
        {
            if(currentdir > 0 || (currentdir == 0 && select_py != 5))
            {
                if(currentdir > 0) {currentdir--; if(!update) update = 2; if(currentdir <= 3 && select_px == 2) select_px = 3;}
                if(currentdir >= 3) select_px = 3; select_py = 0;
            }
            else
            {
                mode_favourites = (!mode_favourites && havefavourites);

                if(mode_favourites) {currentdir = (ndirectories/12) * 12; select_px = 3; select_py = 2;}
                else
                {
                    currentdir = ndirectories - 1;
                    if(currentdir < 0) currentdir = 0;
                    if(currentdir >= 3)
                    {
                        select_px = 3;
                        currentdir-= 3;
                    }
                    else
                        select_px = currentdir;

                    select_py = 0;
                }

                update = 1;
            }
        }


        if(currentdir < 0 || currentdir >= ndirectories)  {currentdir = select_px = select_py = 0; update = 1;}

        if(update)
        {
            if(update == 2 && currentdir == (tmp_currentdir - 1))
            {
                wait_event_thread(); // wait previous event thread function

                int_currentdir = currentdir;

                // program new event thread function
                event_thread_send(0x555ULL, (u64) get_games_3, 0);
            }
            else
                get_games();
        }

        anim_mode = 1; anim_step = 0;
        GFX1_mode = 2; GFX1_counter = 20;

        enable_draw_background_pic1();
        return r;
    }
    else if(new_pad & BUTTON_RIGHT)
    {
        int tmp_currentdir = currentdir;
        int update = 0;
        select_px++;

        auto_right = 1;

        if(!mode_favourites && (currentdir + select_px) >= ndirectories) {currentdir = ndirectories; select_px = 4; select_py = 2;}

        if(select_px > 3)
        {
            select_px = 0;

            select_py++;

            if(select_py > 2)
            {
                select_py = 0;

                if(mode_favourites >= 0x10000) ;
                else if(mode_favourites)
                {
                    mode_favourites = 0;
                    if(!update) update = 1;
                    currentdir = 0;
                }
                else
                    select_py = 1;
            }
        }

        //

        if(!mode_favourites)
        {
            if(select_px <= 3 && select_py == 0 && currentdir <= 3) ;
            else if(currentdir < ndirectories)
            {
                currentdir++; update = 2;
                select_px = 3; select_py = 0;
            }
            else
            {
                mode_favourites = (!mode_favourites && havefavourites);
                currentdir = select_px = select_py = 0;
                update = 1;
            }
        }


        if(currentdir < 0 || currentdir >= ndirectories)  currentdir = 0;

        if(update)
        {
            if(update == 2 && currentdir == (tmp_currentdir + 1))
            {
                wait_event_thread(); // wait previous event thread function

                int_currentdir = currentdir;

                // program new event thread function
                event_thread_send(0x555ULL, (u64) get_games_3, 1ULL);
            }
            else
                get_games();
        }

        anim_mode = 2; anim_step = 0;
        GFX1_mode = 1; GFX1_counter = 20;

        enable_draw_background_pic1();
        return r;
    }
    else if(new_pad & BUTTON_L1) //change page
    {
        anim_mode = 0; anim_step = 0;

        if(mode_favourites >= 0x10000) ;
        else if(mode_favourites)
        {
            mode_favourites = 0;
            select_px = select_py = 0;
            get_games();
        }
        else if(currentdir >= 12)
        {
            mode_favourites = 0;

            currentdir -= 12;
            if(currentdir < 0) currentdir = 0;

            if(currentdir >= 3)
            {
                select_px = 3;
                currentdir-= 3;
            }
            else
                select_px = currentdir;

            select_py = 0;
            get_games();
        }
        else
        {
            mode_favourites = (!mode_favourites && havefavourites);

            currentdir = ndirectories - 12;
            if(currentdir < 0) currentdir = 0;

            if(currentdir >= 3)
            {
                select_px = 3;
                currentdir-= 3;
            }
            else
                select_px = currentdir;

            select_py = 0;
            get_games();
        }

        GFX1_mode = 2; GFX1_counter = 20;

        enable_draw_background_pic1();
        return r;
    }
    else if(new_pad & BUTTON_R1) //change page
    {
        //maybe wait some seconds here...
        anim_mode = 0; anim_step = 0;

        if(mode_favourites >= 0x10000) ;
        else if(mode_favourites)
        {
            if(currentdir >= ndirectories)
            {
                currentdir = select_px = select_py = 0;
            }

            mode_favourites = 0;
            get_games();
        }
        else if(currentdir + 12 < ndirectories)
        {
            mode_favourites = 0;
            currentdir += 12;
            select_px = select_py = 0;
            if(currentdir >= 3)
            {
                select_px = 3;
                currentdir-= 3;
            }
            else
                select_px = currentdir;

            get_games();
        }
        else
        {
            mode_favourites = (!mode_favourites && havefavourites);
            currentdir = 0;
            get_games();
        }

        GFX1_mode = 1; GFX1_counter = 20;

        enable_draw_background_pic1();
        return r;
    }
// end of GUI 1
    }

    if(new_pad & BUTTON_R3) //switch games list
    {
        // wait until R3 button is released
        int r = old_pad;
        while((new_pad | old_pad) & BUTTON_R3) ps3pad_read();
        old_pad = r;

        clear_game_list();

        if(old_pad & BUTTON_SELECT)
        {   // [SELECT + R3] = List All Games

            read_settings();

            mode_homebrew = GAMEBASE_MODE;
            game_list_category = GAME_LIST_ALL;
            mode_favourites = 0;

            force_ntfs_mount = true; frame_count = 62; ntfs_retry = MAX_NTFS_RETRY;
            for(int i = 0; i < 8; i++) automountCount[i] = 2;
        }
        else if(old_pad & BUTTON_L2)
        {   // [L2 + R3] = select previous game list

            read_settings();


            if(old_pad & BUTTON_SELECT)
            {
                mode_homebrew = GAMEBASE_MODE;
                game_list_category = GAME_LIST_ALL;
                mode_favourites = 0;
            }
            else if(game_list_category == 1 && (mode_homebrew == GAMEBASE_MODE))
            {
                SetFavourites(mode_homebrew);
                mode_homebrew = HOMEBREW_MODE;
                GetFavourites(mode_homebrew);
                mode_favourites = 1;
            }
            else if(mode_homebrew)
            {
                SetFavourites(mode_homebrew);

                if(mode_homebrew == HOMEBREW_MODE) mode_homebrew = VIDEOS_MODE; else
                if(mode_homebrew == VIDEOS_MODE)   mode_homebrew = GAMEBASE_MODE;

                GetFavourites(mode_homebrew);
                game_list_category = GAME_LIST_ALL;
                mode_favourites = 1;
            }
            else
            {
                mode_favourites = 0;
                ROT_DEC(game_list_category, 0, 2); // ALL <- PS3 Only <- HOMEBREW
            }
        }
        else
        {   // [R3] = Select next game list
            force_ntfs_mount = true; frame_count = 62; ntfs_retry = MAX_NTFS_RETRY;
            for(int i = 0; i < 8; i++) automountCount[i] = 2;

            read_settings();

            if(game_list_category == GAME_LIST_ALL && (mode_homebrew == GAMEBASE_MODE))
            {
                SetFavourites(mode_homebrew);
                mode_homebrew = VIDEOS_MODE;
                GetFavourites(mode_homebrew);
                mode_favourites = 1;
            }
            else if(mode_homebrew)
            {
                game_list_category = GAME_LIST_PS3_ONLY;
                SetFavourites(mode_homebrew);

                if(mode_homebrew == VIDEOS_MODE)   mode_homebrew = HOMEBREW_MODE; else
                if(mode_homebrew == HOMEBREW_MODE) mode_homebrew = GAMEBASE_MODE;

                GetFavourites(mode_homebrew);
                mode_favourites = 0;
            }
            else
            {
                mode_favourites = 0;
                game_list_category++; // ALL -> PS3 Only -> HOMEBREW
                if(game_list_category > 2) {game_list_category = GAME_LIST_ALL; mode_favourites = 1;}
            }
        }

        return_to_game_list(true);
    }
    else if(new_pad & BUTTON_L3) // Select Game List Menu
    {
        menu_screen = SCR_MENU_GAME_LIST;
        select_option = 0;
    }

    return r;
}

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
#else
void return_to_game_list(bool update)
{
    if(update)
    {
        new_pad = old_pad = 0;
        anim_mode = anim_step = 0;

        DCls();
        clear_game_list();

        bdvd_notify = 1;
        forcedevices = (fdevices && 0xFFFFF);
        fdevices_old = forcedevices;
        fdevices ^= forcedevices;
        save_game_list_after_refresh = true;

        force_ntfs_mount = true; ntfs_retry = MAX_NTFS_RETRY;
        for(int i = 0; i < 8; i++) automountCount[i] = 2;

        load_gamecfg(RESET_GAME_INFO); // force refresh game info
    }

    menu_screen = SCR_MAIN_GAME_LIST;
    select_option = 0;
    frame_count = 0xFF; // force display temp
}

void enable_draw_background_pic1()
{
    frame_count = 32;
    allow_restore_last_game = false;
    bLoadPIC1 = true;
}

void draw_background_pic1()
{
    int selected = select_px + select_py * cols;

    if((bLoadPIC1 && (bk_picture == BG_PIC1 || gui_mode == MODE_XMB_LIKE) && frame_count > 55) && Png_offset[selected])
    {
        int i = selected;
        bLoadPIC1 = false;

        Png_offset[BIG_PICT] = 0;
        wait_event_thread();

        int_currentdir = currentdir;
        int indx = mode_favourites ? favourites.list[i].index : (currentdir + i);

        if(bSkipPIC1)
        {
            // ignore PS1/PS2/RETRO
            if((directories[indx].flags & (RETRO_FLAG | PS2_CLASSIC_FLAG)) == (RETRO_FLAG | PS2_CLASSIC_FLAG))
            {
                bool is_retro = (strlen(retro_root_path) > 0 && strstr(directories[indx].path_name, retro_root_path) != NULL);

                if(gui_mode != MODE_XMB_LIKE)
                {
                    if(is_retro) return;

                    bool is_ps2_classic = !is_retro && strlen(ps2classic_path) > 0 &&
                                          (strstr(directories[indx].path_name, ps2classic_path) != NULL);

                    if(is_ps2_classic) return;
                }
            }
            else if((directories[indx].flags & (PS1_FLAG)) == (PS1_FLAG)) return;
        }

        if(gui_mode > MODE_GRID_6x5 || menu_screen == SCR_MENU_GLOBAL_OPTIONS) ; // avoid freeze on large grids

        else if((bShowPIC1 == 0 && bk_picture == BG_PIC1) || (bShowPIC1 == 1) || (bShowPIC1 == 2 && ((directories[indx].flags & (NTFS_FLAG)) != NTFS_FLAG)))
        {
            // program new event thread function to show background picture
            event_thread_send(0x555ULL, (u64) get_pict, (u64) &i);
        }
    }
}

static int list_box_devices[16];
static int max_list_box_devices = 0;
#endif

void mount_iso_game()
{
    wait_event_thread(); // wait previous event thread function

    if(directories[currentgamedir].flags & (PS1_FLAG)) goto mount_game;

    u16 cur_firm = ((firmware>>12) & 0xF) * 10000 + ((firmware>>8) & 0xF) * 1000 + ((firmware>>4) & 0xF) * 100;

    char str_version[8];
    sprintf(str_version, "%2.2u.%4.4u", cur_firm / 10000, cur_firm % 10000 );

    char version[8];

    int is_ntfs_dev = NTFS_DEVICE_UNMOUNT;

    if(directories[currentgamedir].flags && NTFS_FLAG)
    {
        is_ntfs_dev = NTFS_Test_Device(directories[currentgamedir].path_name + 1);
    }

    if(is_ntfs_path(directories[currentgamedir].path_name) == false)
        sysLv2FsChmod(directories[currentgamedir].path_name, 0170000 | 0777);

    bool is_iso = false;

    int pathlen = strlen(directories[currentgamedir].path_name);

    if(pathlen > 4) is_iso = (strcasecmp(directories[currentgamedir].path_name + pathlen - 4, ".iso")   == SUCCESS);
    if(pathlen > 6) is_iso = (strcasecmp(directories[currentgamedir].path_name + pathlen - 6, ".iso.0") == SUCCESS) || is_iso;

    if(is_ntfs_dev >= 0 || is_iso)
    {
        int fd = ps3ntfs_open(directories[currentgamedir].path_name, O_RDONLY, 0);
        if(fd >= 0)
        {
            u32 flba;
            u64 size;
            int re;
            char *mem = NULL;

            re = get_iso_file_pos(fd, "/PS3_GAME/PARAM.SFO;1", &flba, &size);

            if(!re && (mem = malloc(size + 16)) != NULL)
            {
                memset(mem, 0, size + 16);
                re = ps3ntfs_read(fd, (void *) mem, size);
                ps3ntfs_close(fd);

                if(re == size)
                {
                    if(mem_parse_param_sfo((u8 *) mem, size, "PS3_SYSTEM_VER", version) == SUCCESS) version[5] = 0;
                    if(mem) free(mem);

                    char game_update_path[0x420];
                    sprintf(game_update_path, "/dev_hdd0/game/%c%c%c%c%s", directories[currentgamedir].title_id[0], directories[currentgamedir].title_id[1],
                            directories[currentgamedir].title_id[2], directories[currentgamedir].title_id[3], &directories[currentgamedir].title_id[5]);

                    sysFSStat s;

                    if(!sysLv2FsStat(game_update_path, &s))
                        patch_error_09(game_update_path, !(old_pad & BUTTON_SELECT));

                    if(strcmp(version, str_version) > 0) if(patchps3iso(directories[currentgamedir].path_name, -1)) return;
                }
                else
                    if(mem) free(mem);
            }
            else
            {
                ps3ntfs_close(fd);
            }
        }
    }

mount_game:
    SaveGameList();

    launch_iso_game(directories[currentgamedir].path_name, DETECT_EMU_TYPE);

    exit_to_xmb();
}

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
void draw_device_mkiso(float x, float y, int index, char *path) {};
void draw_cache_external() {};
#else
void draw_app_version(float x, float y)
{
    wait_event_thread(); // wait previous event thread function

    if(app_ver[0])
    {
        DrawFormatString(848 - x - strlen(app_ver) * 16 - 190, y, app_ver);
        return;
    }

    if(directories[currentgamedir].flags & (PS1_FLAG)) return;

    if((bShowVersion == 0) || (bShowVersion == 2 && (((directories[currentgamedir].flags & (NTFS_FLAG)) == NTFS_FLAG) ||
                                                     ((directories[currentgamedir].flags & (PS3_FLAG )) == PS3_FLAG )))) return;

    sprintf(tmp_path, "/dev_hdd0/game/%c%c%c%c%s/PARAM.SFO", directories[currentgamedir].title_id[0], directories[currentgamedir].title_id[1],
                                                             directories[currentgamedir].title_id[2], directories[currentgamedir].title_id[3],
                                                             &directories[currentgamedir].title_id[5]);

    if(parse_param_sfo_appver(tmp_path, app_ver) == FAILED)
    {
        int is_ntfs_dev = NTFS_DEVICE_UNMOUNT;

        if(directories[currentgamedir].flags && NTFS_FLAG)
            is_ntfs_dev = NTFS_Test_Device(directories[currentgamedir].path_name + 1);

        bool is_iso = (strcasecmp(directories[currentgamedir].path_name + strlen(directories[currentgamedir].path_name) - 4, ".iso")   == 0) ||
                      (strcasecmp(directories[currentgamedir].path_name + strlen(directories[currentgamedir].path_name) - 4, ".iso.0") == 0);

        if(is_iso || is_ntfs_dev >= 0)
        {
            int fd = ps3ntfs_open(directories[currentgamedir].path_name, O_RDONLY, 0);
            if(fd >= 0)
            {
                u32 flba;
                u64 size;
                int re;
                char *mem = NULL;

                if((directories[currentgamedir].flags & (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG)) == (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG))
                {
                    bool is_retro = (strlen(retro_root_path) > 0 && strstr(directories[currentgamedir].path_name, retro_root_path) != NULL);
                    bool is_ps2_classic = !is_retro && strlen(ps2classic_path) > 0 &&
                                          (strstr(directories[currentgamedir].path_name, ps2classic_path) != NULL);

                    if(is_retro || is_ps2_classic)
                        re = FAILED;
                    else
                        re = get_iso_file_pos(fd, "/PSP_GAME/PARAM.SFO", &flba, &size);
                }
                else
                    re = get_iso_file_pos(fd, "/PS3_GAME/PARAM.SFO;1", &flba, &size);

                if(!re && (mem = malloc(size + 16)) != NULL)
                {
                    memset(mem, 0, size + 16);
                    re = ps3ntfs_read(fd, (void *) mem, size);
                    ps3ntfs_close(fd);

                    if(re == size)
                    {
                        if(mem_parse_param_sfo((u8 *) mem, size, "APP_VER", app_ver) == SUCCESS) app_ver[5] = 0;
                    }
                }
                else
                    ps3ntfs_close(fd);

                if(mem) free(mem);
            }
            else
                return;
        }
        else
        {
            if(is_ntfs_path(directories[currentgamedir].path_name) == false)
                sysLv2FsChmod(directories[currentgamedir].path_name, 0170000 | 0777);

            sprintf(tmp_path, "%s/PS3_GAME/PARAM.SFO", directories[currentgamedir].path_name);
            if(parse_param_sfo_appver(tmp_path, app_ver)) return;
        }
    }

    DrawFormatString(848 - x - strlen(app_ver) * 16 - 190, y, app_ver);
}

void draw_device_mkiso(float x, float y, int index, char *path)
{
    int i, n;

    int selected = select_px + select_py * cols;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWGMOPT_MKISO]);

    SetCurrentFont(FONT_BUTTON);

    SetFontSize(16, 20);

    if(index >= 0)
    {
        utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 64);
        temp_buffer[64] = 0;

        DrawFormatString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

        draw_app_version(x, y);
    }
    else
    {
        static int frame=0;
        frame++; if(frame>60) frame = 0;

        flash = (frame >> 5) & 1;
    }

    y += 24;

    if(bShowPIC1)
    {
        if(Png_offset[BIG_PICT])
            tiny3d_SetTextureWrap(0, Png_offset[BIG_PICT], Png_datas[BIG_PICT].width,
                                  Png_datas[BIG_PICT].height, Png_datas[BIG_PICT].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        else
            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DEFAULT_BACKGROUND], Png_res[IMG_DEFAULT_BACKGROUND].width,
                                  Png_res[IMG_DEFAULT_BACKGROUND].height, Png_res[IMG_DEFAULT_BACKGROUND].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
    }

    DrawTextBox(x, y, 0, box_width, box_height, WHITE);

    DrawBox(x, y, 0, box_width, box_height, BLACK);

    SetCurrentFont(FONT_TTF);

    SetFontAutoCenter(1);
    DrawFormatString(0, y, "%s", language[GLUTIL_HOLDTRIANGLEAB]);
    SetFontAutoCenter(0);

    SetFontSize(10, 12);

    if(index >= 0)
        utf8_to_ansi(directories[currentgamedir].path_name, temp_buffer, 128);
    else
        utf8_to_ansi(path, temp_buffer, 128);

    temp_buffer[128] = 0;
    DrawFormatString(x + 8, y + 3 * 150 - 6, "%s", temp_buffer);

    // get device list
    list_box_devices[0] = 0;
    max_list_box_devices = 1;

    for(i = 0; i < 8 ; i++)
    {
        if(mounts[i])
        {
            for (n = 0; n < mountCount[i]; n++)
            {
                if(max_list_box_devices < 16 && !strncmp((mounts[i]+n)->name, "ntfs", 4))
                    {list_box_devices[max_list_box_devices]= 128 + ((mounts[i]+n)->name[4] - 48); max_list_box_devices++;}
            }
        }
    }

    for(i = 0; i < 8 ; i++)
    {
        sprintf(filename, "/dev_usb00%c", 48 +i);
        if(max_list_box_devices < 16 && file_exists(filename))
            {list_box_devices[max_list_box_devices]= 64 + i; max_list_box_devices++;}
    }

    if(select_option < 0 || select_option >= max_list_box_devices) select_option = 0;

    for(n = 0; n < max_list_box_devices; n++)
    {
        if(list_box_devices[n] & 128)
        {
            sprintf(tmp_path, "/ntfs%i:", list_box_devices[n] & 127);
        }
        else if(list_box_devices[n] & 64)
        {
            sprintf(tmp_path, "/dev_usb00%i", list_box_devices[n] & 63);
        }
        else
            sprintf(tmp_path,"/dev_hdd0");

        if(n < 8)
            DrawButton1_UTF8(x + 32, y + 32 + 48 * n, 320, tmp_path, (flash && (select_option == n)));
        else
            DrawButton1_UTF8(x + 32 + 320 + 16, y + 32 + 48 * (n - 8), 320, tmp_path, (flash && (select_option == n)));
    }


    ////////////

    SetCurrentFont(FONT_TTF);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);

    SetFontColor(WHITE, BLACK);

    if(index >= 0)
    {
        i = selected;

        if(Png_offset[i])
        {
            u32 str_color = WHITE;

            if((directories[currentgamedir].flags  & GAMELIST_FILTER) == BDVD_FLAG)
            {
                if(strncmp((char *) string_title_utf8, bluray_game, 64))
                {
                    strncpy((char *) string_title_utf8, bluray_game, 128);
                    update_title_utf8 = true;
                }
                str_color = 0x00ff00ff;
            }
            else if(strncmp((char *) string_title_utf8, directories[currentgamedir].title, 64))
            {
                strncpy((char *) string_title_utf8, directories[currentgamedir].title, 128);
                update_title_utf8 = true;
            }

            if(update_title_utf8)
            {
                width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, 16, 24);
                update_title_utf8 = false;
            }

            tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
                    32, 768 * 2,
                    TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

            DrawTextBox((848 - width_title_utf8) / 2, y + 3 * 150 , 0, 768, 32, str_color);
        }
    }

    tiny3d_Flip();
    ps3pad_read();

    if(new_pad & BUTTON_CROSS_)
    {
        //select_option
        if(index < 0 && strstr(path, "/GAME") == NULL)
        {
            if(list_box_devices[select_option] & 128)
                sprintf(tmp_path, "/ntfs%i:/BDISO", list_box_devices[select_option] & 127);
            else if(list_box_devices[select_option] & 64)
                sprintf(tmp_path, "/dev_usb00%i/BDISO", list_box_devices[select_option] & 63);
            else
                strcpy(tmp_path,"/dev_hdd0/BDISO");
        }
        else if(list_box_devices[select_option] & 128)
            sprintf(tmp_path, "/ntfs%i:/PS3ISO", list_box_devices[select_option] & 127);
        else if(list_box_devices[select_option] & 64)
            sprintf(tmp_path, "/dev_usb00%i/PS3ISO", list_box_devices[select_option] & 63);
        else
            strcpy(tmp_path,"/dev_hdd0/PS3ISO");

        pause_music(1);

        ps3ntfs_mkdir(tmp_path, 0777);

        char source_path[MAXPATHLEN];

        if(index >= 0)
            strcpy(source_path, directories[currentgamedir].path_name);
        else
            strcpy(source_path, path);

        if(makeps3iso(source_path, tmp_path, (list_box_devices[select_option] & 64) != 0 ? 1 : 2) != -1)
        {
            pause_music(0);

            return_to_game_list(true);
        }
        return;
    }
    else if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))
    {
        DCls();
        menu_screen = SCR_MENU_GAME_OPTIONS;
        select_option = 5; return;
    }
    else if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, max_list_box_devices - 1);
    }
    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, max_list_box_devices - 1, 0);
    }

}

void draw_device_xtiso(float x, float y, int index)
{
    int i, n;


    int selected = select_px + select_py * cols;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWGMOPT_XTISO]);

    SetCurrentFont(FONT_BUTTON);

    SetFontSize(16, 20);

    utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 64);
    temp_buffer[64] = 0;

    DrawFormatString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

    draw_app_version(x, y);

    y += 24;

    if(bShowPIC1)
    {
        if(Png_offset[BIG_PICT])
            tiny3d_SetTextureWrap(0, Png_offset[BIG_PICT], Png_datas[BIG_PICT].width,
                                  Png_datas[BIG_PICT].height, Png_datas[BIG_PICT].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        else
            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DEFAULT_BACKGROUND], Png_res[IMG_DEFAULT_BACKGROUND].width,
                                  Png_res[IMG_DEFAULT_BACKGROUND].height, Png_res[IMG_DEFAULT_BACKGROUND].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
    }

    DrawTextBox(x, y, 0, box_width, box_height, WHITE);

    DrawBox(x, y, 0, box_width, box_height, BLACK);

    SetCurrentFont(FONT_TTF);

    SetFontAutoCenter(1);
    DrawFormatString(0, y, "%s", language[GLUTIL_HOLDTRIANGLEAB]);
    SetFontAutoCenter(0);

    SetFontSize(10, 12);

    utf8_to_ansi(directories[currentgamedir].path_name, temp_buffer, 128);
    temp_buffer[128] = 0;

    DrawFormatString(x + 8, y + 3 * 150 - 6, "%s", temp_buffer);

    for(n = 0; n < max_list_box_devices; n++)
    {
        if(list_box_devices[n] & 128)
            sprintf(tmp_path, "/ntfs%i:", list_box_devices[n] & 127);
        else if(list_box_devices[n] & 64)
            sprintf(tmp_path, "/dev_usb00%i", list_box_devices[n] & 63);
        else
            strcpy(tmp_path,"/dev_hdd0");

        if(n < 8)
            DrawButton1_UTF8(x + 32, y + 32 + 48 * n, 320, tmp_path, (flash && (select_option == n)));
        else
            DrawButton1_UTF8(x + 32 + 320 + 16, y + 32 + 48 * (n - 8), 320, tmp_path, (flash && (select_option == n)));
    }

    ////////////

    SetCurrentFont(FONT_TTF);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);

    SetFontColor(WHITE, BLACK);

    i = selected;

    if(Png_offset[i])
    {
        u32 str_color = WHITE;

        if((directories[currentgamedir].flags  & GAMELIST_FILTER) == BDVD_FLAG)
        {
            if(strncmp((char *) string_title_utf8, bluray_game, 64))
            {
                strncpy((char *) string_title_utf8, bluray_game, 128);
                update_title_utf8 = true;
            }
            str_color = 0x00ff00ff;
        }
        else if(strncmp((char *) string_title_utf8, directories[currentgamedir].title, 64))
        {
            strncpy((char *) string_title_utf8, directories[currentgamedir].title, 128);
            update_title_utf8 = true;
        }

        if(update_title_utf8)
        {
            width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, 16, 24);
            update_title_utf8 = false;
        }

        tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
                32, 768 * 2,
                TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox((848 - width_title_utf8) / 2, y + 3 * 150 , 0, 768, 32, str_color);
    }

    tiny3d_Flip();
    ps3pad_read();

    if(new_pad & BUTTON_CROSS_)
    {
        //select_option

        if(list_box_devices[select_option] & 128)
            sprintf(tmp_path, "/ntfs%i:/_GAMES", list_box_devices[select_option] & 127);
        else if(list_box_devices[select_option] & 64)
            sprintf(tmp_path, "/dev_usb00%i/GAMES", list_box_devices[select_option] & 63);
        else
        {
            if (!memcmp(hdd_folder,"dev_hdd0",9))
                sprintf(tmp_path, "/%s/" __MKDEF_GAMES_DIR,hdd_folder);
            else if (!memcmp(hdd_folder, "GAMES", 6) || !memcmp(hdd_folder, "dev_hdd0_2", 11))
                strcpy(tmp_path, "/dev_hdd0/GAMES");
            else if (!memcmp(hdd_folder,"host_root", 10))
                strcpy(tmp_path, "/host_root");
            else if (!memcmp(hdd_folder,"video", 6))
                strcpy(tmp_path, "/dev_hdd0/video");
            else if (strstr(hdd_folder, "/"))
                strcpy(tmp_path, hdd_folder);
            else
                sprintf(tmp_path, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR, hdd_folder);
        }

        pause_music(1);

        ps3ntfs_mkdir(tmp_path, 0777);

        extractps3iso(directories[currentgamedir].path_name, tmp_path, (list_box_devices[select_option] & 64) != 0 ? 1 : 0);

        pause_music(0);

        return_to_game_list(true);
        return;
    }
    else if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))
    {
        DCls();
        menu_screen = SCR_MENU_ISO_OPTIONS;
        select_option = 3;
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, max_list_box_devices - 1);
    }
    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, max_list_box_devices - 1, 0);
    }

}

void draw_device_cpyiso(float x, float y, int index)
{
    int i, n;

    char temp_buffer2[4096];


    int selected = select_px + select_py * cols;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWGMOPT_CPYISO]);

    SetCurrentFont(FONT_BUTTON);

    SetFontSize(16, 20);

    utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 64);
    temp_buffer[64] = 0;

    DrawFormatString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

    draw_app_version(x, y);

    y += 24;

    if(bShowPIC1)
    {
        if(Png_offset[BIG_PICT])
            tiny3d_SetTextureWrap(0, Png_offset[BIG_PICT], Png_datas[BIG_PICT].width,
                                  Png_datas[BIG_PICT].height, Png_datas[BIG_PICT].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        else
            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DEFAULT_BACKGROUND], Png_res[IMG_DEFAULT_BACKGROUND].width,
                                  Png_res[IMG_DEFAULT_BACKGROUND].height, Png_res[IMG_DEFAULT_BACKGROUND].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
    }

    DrawTextBox(x, y, 0, box_width, box_height, WHITE);

    DrawBox(x, y, 0, box_width, box_height, BLACK);

    SetCurrentFont(FONT_TTF);

    SetFontAutoCenter(1);
    DrawFormatString(0, y, "%s", language[GLUTIL_HOLDTRIANGLEAB]);
    SetFontAutoCenter(0);

    SetFontSize(10, 12);
    utf8_to_ansi(directories[currentgamedir].path_name, temp_buffer, 128);
    temp_buffer[128] = 0;

    DrawFormatString(x + 8, y + 3 * 150 - 6, "%s", temp_buffer);

    for(n = 0; n < max_list_box_devices; n++)
    {
        if(list_box_devices[n] & 128)
            sprintf(tmp_path, "/ntfs%i:", list_box_devices[n] & 127);
        else if(list_box_devices[n] & 64)
            sprintf(tmp_path, "/dev_usb00%i", list_box_devices[n] & 63);
        else
            strcpy(tmp_path,"/dev_hdd0");

        if(n < 8)
            DrawButton1_UTF8(x + 32, y + 32 + 48 * n, 320, tmp_path, (flash && (select_option == n)));
        else
            DrawButton1_UTF8(x + 32 + 320 + 16, y + 32 + 48 * (n - 8), 320, tmp_path, (flash && (select_option == n)));
    }

    ////////////

    SetCurrentFont(FONT_TTF);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);

    SetFontColor(WHITE, BLACK);

    i = selected;

    if(Png_offset[i])
    {
        u32 str_color = WHITE;

        if((directories[currentgamedir].flags  & GAMELIST_FILTER) == BDVD_FLAG)
        {
            if(strncmp((char *) string_title_utf8, bluray_game, 64))
            {
                strncpy((char *) string_title_utf8, bluray_game, 128);
                update_title_utf8 = true;
            }
            str_color = 0x00ff00ff;
        }
        else
        {
            if(strncmp((char *) string_title_utf8, directories[currentgamedir].title, 64))
            {
                strncpy((char *) string_title_utf8, directories[currentgamedir].title, 128);
                update_title_utf8 = true;
            }
        }

        if(update_title_utf8)
        {
            width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, 16, 24);
            update_title_utf8 = false;
        }

        tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
                32, 768 * 2,
                TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        DrawTextBox((848 - width_title_utf8) / 2, y + 3 * 150 , 0, 768, 32, str_color);
    }

    tiny3d_Flip();
    ps3pad_read();

    if(new_pad & BUTTON_CROSS_)
    {
        //select_option

        if(list_box_devices[select_option] & 128)
        {
            if((directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG))
                sprintf(temp_buffer2, "/ntfs%i:/PS2ISO", list_box_devices[select_option] & 127);
            else
                sprintf(temp_buffer2, "/ntfs%i:/PS3ISO", list_box_devices[select_option] & 127);
        }
        else if(list_box_devices[select_option] & 64)
        {
            if((directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG))
                sprintf(temp_buffer2, "/dev_usb00%i/PS2ISO", list_box_devices[select_option] & 63);
            else
                sprintf(temp_buffer2, "/dev_usb00%i/PS3ISO", list_box_devices[select_option] & 63);
        }
        else
        {
            if((directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG))
                strcpy(temp_buffer2, "/dev_hdd0/PS2ISO");
            else
                strcpy(temp_buffer2, "/dev_hdd0/PS3ISO");
        }

        ps3ntfs_mkdir(temp_buffer2, 0777);

        u64 avail = get_disk_free_space(temp_buffer2);

        strcpy(temp_buffer2 + 1024, directories[currentgamedir].path_name);
        char * filename = strrchr(temp_buffer2 + 1024, '/');
        if(filename) *filename++ = 0; // break the string

        if(filename && strncmp(temp_buffer2 + 1024, temp_buffer2, strlen(temp_buffer2)))
        {
            copy_archive_file(temp_buffer2 + 1024, temp_buffer2, filename, avail);

            return_to_game_list(true);
        }
        else
        {
            DrawDialogOKTimer("Error:! Cannot copy in the same device\n\n"
                              "Error!: No se puede copiar en el mismo dispositivo", 2000.0f);
            menu_screen = SCR_MENU_ISO_OPTIONS;
            select_option = 3;
        }

        return;
    }
    else if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))
    {
        DCls();
        menu_screen = SCR_MENU_ISO_OPTIONS;
        select_option = 3;
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, max_list_box_devices - 1);
    }
    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, max_list_box_devices - 1, 0);
    }

}

void draw_options(float x, float y, int index)
{

    if(strlen(directories[currentgamedir].title_id) == 9 && strstr("HTSS00003|NETBROWSE|IRISMAN00", directories[currentgamedir].title_id) != NULL)
    {
        sprintf(temp_buffer, "%s\n\nDo you want to remove Showtime and Internet Browser icons from the Game List?", directories[currentgamedir].title);
        if(DrawDialogYesNo(temp_buffer) == YES)
        {
            show_custom_icons = 0;
            manager_cfg.show_custom_icons = show_custom_icons;
            forcedevices = 1;
            fdevices ^= 1;
            SaveManagerCfg();
        }

        Png_offset[num_box] = 0;
        Png_offset[BIG_PICT] = 0;

        return_to_game_list(false);
        return;
    }

    int i, n;

    float y2;

    int copy_flag = 1;

    int selected;

    selected = select_px + select_py * cols;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWGMOPT_OPTS]);

    if(directories[currentgamedir].flags & D_FLAG_HDD0)
    {
        copy_flag = 0;

        for(n = 1; n < BDVD_DEVICE; n++)
        {
            if((fdevices >> n) & 1) copy_flag = 1;
        }

    }
    else if(directories[currentgamedir].flags & D_FLAG_USB)
    {
        copy_flag = 0;

        if((fdevices >> 0) & 1) copy_flag = 1;
    }

    if((directories[currentgamedir].title_id[0] == 0) && (select_option == 0))
    {
        select_option = 1;

        if(!copy_flag && select_option == 1) select_option++;

        if((directories[currentgamedir].flags & D_FLAG_BDVD) && (select_option == 2 || select_option == 6)) select_option++;
    }

    if(mode_homebrew >= HOMEBREW_MODE)
    {
        if(select_option <  2) select_option= 2;
        if(select_option == 5) select_option= 6;
        if(select_option == 7) select_option= 8;
    }

    SetCurrentFont(FONT_BUTTON);

    SetFontSize(16, 20);

    utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 64);
    temp_buffer[64] = 0;

    DrawFormatString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

    draw_app_version(x, y);

    y += 24;

    if(bShowPIC1)
    {
        if(Png_offset[BIG_PICT])
            tiny3d_SetTextureWrap(0, Png_offset[BIG_PICT], Png_datas[BIG_PICT].width,
                                  Png_datas[BIG_PICT].height, Png_datas[BIG_PICT].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        else
            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DEFAULT_BACKGROUND], Png_res[IMG_DEFAULT_BACKGROUND].width,
                                  Png_res[IMG_DEFAULT_BACKGROUND].height, Png_res[IMG_DEFAULT_BACKGROUND].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox(x, y, 0, box_width, box_height, WHITE);

        DrawBox(x, y, 0, box_width, box_height, BLACK);
    }

    y2 = y - 4;

    SetFontSize(12, 16);

    SetFontColor(WHITE, BLACK);

    y2 = y + 12;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_CFGGAME], (directories[currentgamedir].title_id[0] == 0 || mode_homebrew >= HOMEBREW_MODE) ? DISABLED : (flash && (select_option == 0)));

    y2+= 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_CPYGAME], (copy_flag != 0  && mode_homebrew == GAMEBASE_MODE) ? (flash && (select_option == 1)) : -1);

    y2+= 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_DELGAME], (directories[currentgamedir].flags & D_FLAG_BDVD) ? DISABLED  : ((flash && (select_option == 2)) ? 1 : 0));

    y2+= 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_FIXGAME], (flash && (select_option == 3)));

    y2+= 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_TSTGAME], (flash && (select_option == 4)));

    y2+= 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_BUILDISO], (directories[currentgamedir].title_id[0] == 0 || mode_homebrew >= HOMEBREW_MODE) ? DISABLED : (flash && (select_option == 5)));

    y2+= 48;

    if(!TestFavouritesExits(directories[currentgamedir].title_id))
        DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_CPYTOFAV], (directories[currentgamedir].flags & D_FLAG_BDVD) ? DISABLED : (flash && (select_option == 6)));
    else
        DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_DELFMFAV], (directories[currentgamedir].flags & D_FLAG_BDVD) ? DISABLED : (flash && (select_option == 6)));

    y2+= 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_GAMEUPDATE], (directories[currentgamedir].title_id[0] == 0 ||
                                                                       mode_homebrew >= HOMEBREW_MODE) ? DISABLED : (flash && (select_option == 7)));

    y2+= 60;



    if((directories[currentgamedir].flags & (BDVD_FLAG | PS1_FLAG)) == BDVD_FLAG)
    {
        n = sys_ss_media_id(BdId);
        if(n == 0 || n == 0x80010006)
        {
            SetFontSize(10, 16);
            DrawFormatString(x + 32, y2, "BD ID: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                                         BdId[0], BdId[1], BdId[2],  BdId[3],  BdId[4],  BdId[5],  BdId[6],  BdId[7],
                                         BdId[8], BdId[9], BdId[10], BdId[11], BdId[12], BdId[13], BdId[14], BdId[15]);
        }
    }
    else if((BdId[0] | BdId[1] | BdId[2]  | BdId[3]  | BdId[4]  | BdId[5]  | BdId[6]  | BdId[7] |
             BdId[8] | BdId[9] | BdId[10] | BdId[11] | BdId[12] | BdId[13] | BdId[14] | BdId[15]))
    {
        SetFontSize(10, 16);
        DrawFormatString(x + 32, y2, "BD ID: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                                     BdId[0], BdId[1], BdId[2],  BdId[3],  BdId[4],  BdId[5],  BdId[6],  BdId[7],
                                     BdId[8], BdId[9], BdId[10], BdId[11], BdId[12], BdId[13], BdId[14], BdId[15]);

    }


    SetFontSize(8, 10);

    utf8_to_ansi(directories[currentgamedir].path_name, temp_buffer, 128);
    temp_buffer[128] = 0;

    DrawFormatString(x + 8, y + 3 * 150 - 6, "%s", temp_buffer);
//

    SetCurrentFont(FONT_TTF);

    // draw game name

    DrawBox(x, y + 3 * 150 + 7, 0, box_width, 40, BLACK);

    SetFontColor(WHITE, BLACK);

    i = selected;

    if(Png_offset[i])
    {
        u32 str_color = WHITE;

        if((directories[currentgamedir].flags  & GAMELIST_FILTER) == BDVD_FLAG)
        {
            if(strncmp((char *) string_title_utf8, bluray_game, 64))
            {
                strncpy((char *) string_title_utf8, bluray_game, 128);
                update_title_utf8 = true;
            }
            str_color = 0x00ff00ff;
        }
        else
        {
            if(strncmp((char *) string_title_utf8, directories[currentgamedir].title, 64))
            {
                strncpy((char *) string_title_utf8, directories[currentgamedir].title, 128);
                update_title_utf8 = true;
            }
        }

        if(update_title_utf8)
        {
            width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, 16, 24);
            update_title_utf8 = false;
        }

        tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
            32, 768 * 2,
            TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox((848 - width_title_utf8) / 2, y + 3 * 150 + 8, 0, 768, 32, str_color);
    }

    tiny3d_Flip();
    ps3pad_read();

    if((old_pad & BUTTON_SELECT) && (new_pad & BUTTON_TRIANGLE))
    {
        if (Png_offset[BIG_PICT])
        {
            Png_offset[BACKGROUND_PICT] = Png_offset[BIG_PICT];
            Png_datas[BACKGROUND_PICT] = Png_datas[BIG_PICT];
        }
    }

    if(new_pad & BUTTON_CROSS_)
    {
        switch(select_option)
        {
            case 0: // Config. Game
                if(mode_homebrew >= HOMEBREW_MODE) break;
                menu_screen = SCR_MENU_GAME_OPTIONS_CONFIG;
                select_option = 0;

                // load game config
                load_gamecfg(currentgamedir);
                return;

            case 1: // Copy Game
                if(mode_homebrew >= HOMEBREW_MODE) break;
                if(test_ftp_working()) break;
                 i = selected;

                 if(Png_offset[i])
                 {
                    pause_music(1);

                    copy_from_selection(currentgamedir);

                    pause_music(0);

                    currentgamedir = currentdir = 0;

                    return_to_game_list(false);
                 }
                 return;

            case 2: // Delete Game
                 i = selected;
                 if(test_ftp_working()) break;

                 if(Png_offset[i])
                 {
                    pause_music(1);

                    delete_game(currentgamedir);

                    pause_music(0);

                    currentgamedir = currentdir = 0;

                    return_to_game_list(false);
                 }
                 return;
            case 3: // Fix File Permissions
                 i = selected;

                 if(Png_offset[i])
                 {
                    pause_music(1);

                    // sys8_perm_mode(1);
                    DCls();
                    FixDirectory(directories[currentgamedir].path_name, 0);
                    DCls();
                    // sys8_perm_mode(0);

                    msgDialogAbort();
                    msgDialogClose(0);

                    pause_music(0);

                    DrawDialogOK(language[DRAWGMOPT_FIXCOMPLETE]);
                 }
                 break;
            case 4: // Test & Fix Game
                 i = selected;

                 if(Png_offset[i])
                 {
                    pause_music(1);

                    DCls();
                    test_game(currentgamedir);
                    DCls();

                    pause_music(0);
                 }
                 break;
            case 5: // Build ISO
                {
                if(mode_homebrew >= HOMEBREW_MODE) break;

                    i = selected;

                    if(Png_offset[i])
                    {
                        menu_screen = SCR_TOOL_BUILD_ISO;
                        select_option = 0;
                    }
                }
                break;

            case 6: // Copy to Favorites
                if(TestFavouritesExits(directories[currentgamedir].title_id))
                {
                        DeleteFavouritesIfExits(directories[currentgamedir].title_id);

                        sprintf(tmp_path, "%s/config/", self_path);
                        SaveFavourites(tmp_path, mode_homebrew);

                        if(mode_favourites && !havefavourites)
                        {
                            mode_favourites = 0; get_games();

                            return_to_game_list(false);
                            return;
                        }

                        get_games();
                }
                else
                {
                    mode_favourites = currentgamedir  | 0x10000;

                    int r = get_icon(path_name, currentgamedir);

                    if(r == GET_ICON_FROM_ISO)
                    {
                        int fd = ps3ntfs_open(path_name, O_RDONLY, 0);
                        if(fd >= 0)
                        {
                            u32 flba;
                            u64 size;
                            char *mem = NULL;
                            int re;

                            if((directories[currentgamedir].flags & (PSP_FLAG)) == (PSP_FLAG))
                                re = get_iso_file_pos(fd, "/PSP_GAME/ICON0.PNG", &flba, &size);
                            else
                                re = get_iso_file_pos(fd, "/PS3_GAME/ICON0.PNG;1", &flba, &size);

                            if(!re && (mem = memalign(32, size + 128)) != 0)
                            {
                                re = ps3ntfs_read(fd, (void *) mem, size);
                                ps3ntfs_close(fd);
                                if(re == size)
                                {
                                    memset(&my_png_datas, 0, sizeof(PngDatas));
                                    my_png_datas.png_in = mem;
                                    my_png_datas.png_size = size;
                                    if(LoadTexturePNG(NULL, BIG_PICT) == SUCCESS) r = ICON_LOAD_SUCCESS;
                                }
                                free(mem);
                            }
                            else
                                ps3ntfs_close(fd);
                        }
                    }

                    if(r == ICON_LOAD_SUCCESS) ;
                    else if(!strncmp(path_name + strlen(path_name) -4, ".JPG", 4) || !strncmp(path_name + strlen(path_name) -4, ".jpg", 4))
                        LoadTextureJPG(path_name, BIG_PICT);
                    else
                        LoadTexturePNG(path_name, BIG_PICT);

                    get_games();

                    return_to_game_list(false);
                    return;
                }
                break;

            case 7: // Update
               if(get_net_status() != SUCCESS) break;

               if(game_update(directories[currentgamedir].title_id) > 0)
               {
                    //sprintf(tmp_path, "%s/PKG", self_path);
                    file_manager(updates_path, NULL);

                    return_to_game_list(true);
                }
                break;

            default:
               break;
        }
    }
    else if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))
    {
        Png_offset[BIG_PICT] = 0;

        return_to_game_list(false);
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        select_option--;
        frame_count = 32;

        if((directories[currentgamedir].flags & D_FLAG_BDVD) && (select_option == 2 || select_option == 6)) select_option--;
        if(!copy_flag && select_option == 1) select_option--;

        if(directories[currentgamedir].title_id[0] == 0 && (select_option == 0 || select_option == 5 || select_option == 7)) select_option--;

        if(mode_homebrew >= HOMEBREW_MODE)
        {
            if(select_option < 2) select_option= -1;
            if(select_option == 5) select_option= 4;
            if(select_option == 7) select_option= 6;
        }

        if(select_option < 0) select_option = 7;
    }
    else if(new_pad & BUTTON_DOWN)
    {
        select_option++;
        frame_count = 32;

        if(!copy_flag && select_option == 1) select_option++;

        if((directories[currentgamedir].flags & D_FLAG_BDVD) &&  (select_option == 2 || select_option == 6)) select_option++;

        if(mode_homebrew == HOMEBREW_MODE)
        {
            if(select_option < 2) select_option = 2;
            if(select_option == 5) select_option = 6;
            if(select_option == 7) select_option= 8;
        }

        if(select_option > 7) select_option = 0;

        if(directories[currentgamedir].title_id[0] == 0 && (select_option == 0 || select_option == 5 || select_option == 7))
        {
            select_option++;

            if(!copy_flag && select_option == 1) select_option++;

            if((directories[currentgamedir].flags & D_FLAG_BDVD) && select_option == 2) select_option++;
        }
    }
}

void draw_iso_options(float x, float y, int index)
{

    int i, o, max_op;

    float y2;
    int is_ntfs_dev = NTFS_DEVICE_UNMOUNT;

    int selected = select_px + select_py * cols;

    SetCurrentFont(FONT_TTF);

    is_ntfs_dev = NTFS_Test_Device(directories[currentgamedir].path_name + 1);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWGMOPT_OPTS]);

    SetCurrentFont(FONT_BUTTON);

    SetFontSize(16, 20);

    draw_app_version(x, y);

    utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 64);

    if((directories[currentgamedir].flags & (PS1_FLAG)) || (directories[currentgamedir].flags & (PS2_FLAG)))
        parse_iso_titleid(directories[currentgamedir].path_name, temp_buffer);

    temp_buffer[64] = 0;
    DrawFormatString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

    y += 24;

    if(bShowPIC1)
    {
        if(Png_offset[BIG_PICT])
            tiny3d_SetTextureWrap(0, Png_offset[BIG_PICT], Png_datas[BIG_PICT].width,
                                  Png_datas[BIG_PICT].height, Png_datas[BIG_PICT].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        else
            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DEFAULT_BACKGROUND], Png_res[IMG_DEFAULT_BACKGROUND].width,
                                  Png_res[IMG_DEFAULT_BACKGROUND].height, Png_res[IMG_DEFAULT_BACKGROUND].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox(x, y, 0, box_width, box_height, WHITE);

        DrawBox(x, y, 0, box_width, box_height, BLACK);
    }

    y2 = y + 8;

    SetFontSize(12, 16);

    SetFontColor(WHITE, BLACK);

    y2 = y + 32;

    if(!TestFavouritesExits(directories[currentgamedir].title_id))
        DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_CPYTOFAV],
                        (directories[currentgamedir].flags & D_FLAG_BDVD) ? DISABLED : (flash && (select_option == 0)));
    else
        DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_DELFMFAV],
                        (directories[currentgamedir].flags & D_FLAG_BDVD) ? DISABLED : (flash && (select_option == 0)));

    max_op = o = 1;

    if(is_ntfs_dev >= 0)
    {
        y2 += 48;

        sprintf(temp_buffer, language[DRAWGMOPT_UNMTDEV], is_ntfs_dev);
        DrawButton1_UTF8(x + 32, y2, 320, temp_buffer, (directories[currentgamedir].flags & D_FLAG_BDVD) ? DISABLED : (flash && (select_option == o)));

        max_op++; o++;
    }

    y2+= 48;

    if((select_option == o) &&(directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG)) select_option++;
    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_TSTGAME],
                    ((directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG)) ? DISABLED : (flash && (select_option == o)));
    max_op++; o++;

    y2+= 48;

    if(select_option == o &&(directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG)) select_option++;
    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_EXTRACTISO],
                    ((directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG)) ? DISABLED : (flash && (select_option == o)));
    max_op++; o++;

    y2+= 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_CPYGAME],(flash && (select_option == o)));
    max_op++; o++;

    y2+= 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_DELGAME], ((flash && (select_option == o)) ? 1 : 0));
    max_op++; o++;

    y2+= 48;

    if(select_option == o &&(directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG)) select_option++;
    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_GAMEUPDATE],
                    ((directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG)) ? DISABLED : (flash && (select_option == o)));
    max_op++; o++;

    y2+= 48;


//
    SetFontSize(8, 10);

    utf8_to_ansi(directories[currentgamedir].path_name, temp_buffer, 128);
    temp_buffer[128] = 0;

    DrawFormatString(x + 8, y + 3 * 150 - 6, "%s", temp_buffer);
//

    SetCurrentFont(FONT_TTF);

    // draw game name

    DrawBox(x, y + 3 * 150 + 7, 0, box_width, 40, BLACK);

    SetFontColor(WHITE, BLACK);

    i = selected;

    if(Png_offset[i])
    {
        u32 str_color = WHITE;

        if((directories[currentgamedir].flags  & GAMELIST_FILTER) == BDVD_FLAG)
        {
            if(strncmp((char *) string_title_utf8, bluray_game, 64))
            {
                strncpy((char *) string_title_utf8, bluray_game, 128);
                update_title_utf8 = true;
            }
            str_color = 0x00ff00ff;
        }
        else
        {
            if(strncmp((char *) string_title_utf8, directories[currentgamedir].title, 64))
            {
                strncpy((char *) string_title_utf8, directories[currentgamedir].title, 128);
                update_title_utf8 = true;
            }
        }

        if(update_title_utf8)
        {
            width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, 16, 24);
            update_title_utf8 = false;
        }

        tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
            32, 768 * 2,
            TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        DrawTextBox((848 - width_title_utf8) / 2, y + 3 * 150 + 8, 0, 768, 32, str_color);
    }

    tiny3d_Flip();
    ps3pad_read();

    if((old_pad & BUTTON_SELECT) && (new_pad & BUTTON_TRIANGLE))
    {
        if (Png_offset[BIG_PICT])
        {
            Png_offset[BACKGROUND_PICT] = Png_offset[BIG_PICT];
            Png_datas[BACKGROUND_PICT] = Png_datas[BIG_PICT];
        }
    }

    if(new_pad & BUTTON_CROSS_)
    {
        int select_option2 = select_option;

        if((is_ntfs_dev < 0) && (select_option2 > 0)) select_option2++;

        switch(select_option2)
        {
            case 0: // Copy to Favorites
                if(TestFavouritesExits(directories[currentgamedir].title_id))
                {
                        DeleteFavouritesIfExits(directories[currentgamedir].title_id);

                        sprintf(tmp_path, "%s/config/", self_path);
                        SaveFavourites(tmp_path, mode_homebrew);

                        if(mode_favourites && !havefavourites)
                        {
                            mode_favourites = 0; get_games();

                            return_to_game_list(false);
                            return;
                        }

                        get_games();
                }
                else
                {
                    mode_favourites = currentgamedir  | 0x10000;

                    int r = get_icon(path_name, currentgamedir);

                    if(r == GET_ICON_FROM_ISO)
                    {
                        int fd = ps3ntfs_open(path_name, O_RDONLY, 0);
                        if(fd >= 0)
                        {
                            u32 flba;
                            u64 size;
                            char *mem = NULL;
                            int re;

                            if((directories[currentgamedir].flags & (PSP_FLAG)) == (PSP_FLAG))
                                re = get_iso_file_pos(fd, "/PSP_GAME/ICON0.PNG", &flba, &size);
                            else
                                re = get_iso_file_pos(fd, "/PS3_GAME/ICON0.PNG;1", &flba, &size);

                            if(!re && (mem = memalign(32, size + 128)) != 0)
                            {
                                re = ps3ntfs_read(fd, (void *) mem, size);
                                ps3ntfs_close(fd);
                                if(re == size)
                                {
                                    memset(&my_png_datas, 0, sizeof(PngDatas));
                                    my_png_datas.png_in = mem;
                                    my_png_datas.png_size = size;
                                    if(LoadTexturePNG(NULL, BIG_PICT) == SUCCESS) r = ICON_LOAD_SUCCESS;
                                }
                                free(mem);
                            }
                            else
                                ps3ntfs_close(fd);
                        }
                    }

                    if(r == ICON_LOAD_SUCCESS) ;
                    else if(!strncmp(path_name + strlen(path_name) -4, ".JPG", 4) || !strncmp(path_name + strlen(path_name) -4, ".jpg", 4))
                        LoadTextureJPG(path_name, BIG_PICT);
                    else
                        LoadTexturePNG(path_name, BIG_PICT);

                    get_games();

                    return_to_game_list(false);
                    return;
                }
                break;

            case 1: // Unmount NTFS device
                NTFS_UnMount(is_ntfs_dev);

                return_to_game_list(true);
                return;

            case 2: // Test & Fix
                 i = selected;

                 if(Png_offset[i])
                 {
                    pause_music(1);

                    char game_update_path[0x420];
                    sprintf(game_update_path, "/dev_hdd0/game/%c%c%c%c%s", directories[currentgamedir].title_id[0], directories[currentgamedir].title_id[1],
                            directories[currentgamedir].title_id[2], directories[currentgamedir].title_id[3], &directories[currentgamedir].title_id[5]);

                    sysFSStat s;

                    if(!sysLv2FsStat(game_update_path, &s))
                        patch_error_09(game_update_path, !(old_pad & BUTTON_SELECT));

                    //test_game(currentgamedir);
                    patchps3iso(directories[currentgamedir].path_name, 0);

                    pause_music(0);
                 }
                 break;
            case 3: // Extract ISO
                {
                if(mode_homebrew >= HOMEBREW_MODE) break;

                i = selected;

                if(Png_offset[i])
                {
                    // get device list
                    list_box_devices[0] = 0;
                    max_list_box_devices = 1;

                    int i, k;
                    for(i = 0; i < 8 ; i++)
                    {
                        if(mounts[i])
                        {
                            for (k = 0; k < mountCount[i]; k++)
                            {
                                if(max_list_box_devices < 16 && !strncmp((mounts[i]+k)->name, "ntfs", 4))
                                    {list_box_devices[max_list_box_devices]= 128 + ((mounts[i]+k)->name[4] - 48); max_list_box_devices++;}
                            }
                        }
                    }

                    for(i = 0; i < 8 ; i++)
                    {
                        sprintf(filename, "/dev_usb00%c", 48+i);
                        if(max_list_box_devices < 16 && file_exists(filename))
                            {list_box_devices[max_list_box_devices]= 64 + i; max_list_box_devices++;}
                    }

                    menu_screen = SCR_TOOL_EXTRACT_ISO;
                    select_option = 0;
                }
                }
                break;

            ///
            case 4: // Copy Game
                if(mode_homebrew >= HOMEBREW_MODE) break;

                i = selected;

                if(Png_offset[i])
                {
                    // get device list

                    max_list_box_devices = 0;

                    if(strncmp(directories[currentgamedir].path_name, "/dev_hdd0", 9))
                    {
                        list_box_devices[0] = 0;
                        max_list_box_devices = 1;
                    }

                    int i, k;
                    for(i = 0; i < 8 ; i++)
                    {
                        if(mounts[i])
                        {
                            for (k = 0; k < mountCount[i]; k++)
                            {
                                if(max_list_box_devices < 16 && !strncmp((mounts[i]+k)->name, "ntfs", 4)
                                    && strncmp(&directories[currentgamedir].path_name[1], (mounts[i]+k)->name, 5))
                                    {list_box_devices[max_list_box_devices]= 128 + ((mounts[i]+k)->name[4] - 48); max_list_box_devices++;}
                            }
                        }
                    }

                    for(i = 0; i < 8 ; i++)
                    {
                        sprintf(filename, "/dev_usb00%c", 48+i);
                        if(max_list_box_devices < 16 && file_exists(filename) &&
                            strncmp(directories[currentgamedir].path_name, filename, 11))
                            {list_box_devices[max_list_box_devices]= 64 + i; max_list_box_devices++;}
                    }

                    if(max_list_box_devices > 0)
                    {
                        menu_screen = SCR_TOOL_COPY_ISO;
                        select_option = 0;
                    }
                }
                break;

            case 5: // Delete Game
                i = selected;

                if(Png_offset[i])
                {
                   if(!strncmp(directories[currentgamedir].path_name, "/dev_hdd0", 9))
                       sprintf(temp_buffer, "%s\n\n%s HDD0?", directories[currentgamedir].title, language[GAMEDELSL_WANTDELETE]);
                   else if(!strncmp(directories[currentgamedir].path_name, "/dev_usb", 8))
                       sprintf(temp_buffer, "%s\n\n%s USB00%c?", directories[currentgamedir].title, language[GAMEDELSL_WANTDELETE], directories[currentgamedir].path_name[10]);
                   else if(!strncmp(directories[currentgamedir].path_name, "/ntfs", 5))
                       sprintf(temp_buffer, "%s\n\n%s NTFS%c?", directories[currentgamedir].title, language[GAMEDELSL_WANTDELETE], directories[currentgamedir].path_name[5]);
                   else if(!strncmp(directories[currentgamedir].path_name, "/ext", 4))
                       sprintf(temp_buffer, "%s\n\n%s EXT%c?", directories[currentgamedir].title, language[GAMEDELSL_WANTDELETE], directories[currentgamedir].path_name[4]);
                   else break;

                   if(DrawDialogYesNo(temp_buffer) == YES)
                   {
                       pause_music(1);

                       delps3iso(directories[currentgamedir].path_name);

                       pause_music(0);

                       return_to_game_list(true);
                   }
                }
                return;

            case 6: // Game Update
               if(get_net_status() != SUCCESS) break;

               if(game_update(directories[currentgamedir].title_id) > 0)
               {
                    //sprintf(tmp_path, "%s/PKG", self_path);
                    file_manager(updates_path, NULL);

                    return_to_game_list(true);
                }
                break;

            default:
                break;
        }
       // return_to_game_list(true);; return;
    }
    else if(new_pad & (BUTTON_TRIANGLE | BUTTON_CIRCLE_))
    {
        Png_offset[BIG_PICT] = 0;

        return_to_game_list(false);
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, max_op - 1);

        if((directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG))
        {
            if(select_option >= 5) select_option = 4;
            if(select_option == 2) select_option = 0;
        }
    }
    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, max_op - 1, 0);

        if((directories[currentgamedir].flags & (PS2_FLAG)) == (PS2_FLAG))
        {
            if(select_option >= 5) select_option = 0;
        }
    }
}

void draw_configs(float x, float y, int index)
{

    int i;

    float y2, x2;

    int selected;
    selected = select_px + select_py * cols;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWGMCFG_CFGS]);

    i = selected;

    SetCurrentFont(FONT_BUTTON);

    SetFontSize(16, 20);

    utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 64);
    temp_buffer[64] = 0;

    DrawFormatString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

    draw_app_version(x, y);

    y += 24;

    if(bShowPIC1)
    {
        if(Png_offset[BIG_PICT])
            tiny3d_SetTextureWrap(0, Png_offset[BIG_PICT], Png_datas[BIG_PICT].width,
                                  Png_datas[BIG_PICT].height, Png_datas[BIG_PICT].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        else
            tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DEFAULT_BACKGROUND], Png_res[IMG_DEFAULT_BACKGROUND].width,
                                  Png_res[IMG_DEFAULT_BACKGROUND].height, Png_res[IMG_DEFAULT_BACKGROUND].wpitch,
                                  TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
    }

    DrawTextBox(x, y, 0, box_width, box_height, WHITE);

    DrawBox(x, y, 0, box_width, box_height, BLACK);

    x2 = x;
    y2 = y + 32;


#ifdef CONFIG_USE_SYS8PERMH4
    x2 = DrawButton1_UTF8(x + 32, y2, 320, "Fix Permissions", (flash && (select_option == 0))) + 16; // do no translate this (3.44)

    x2 = DrawButton2_UTF8(x2, y2, 0, " Default "      , (game_cfg.perm == 0) ) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, " PS jailbreak " , (game_cfg.perm == 1)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, " v4 Perms (F1) ", (game_cfg.perm == 2)) + 8;

    y2+= 48;
#endif

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMCFG_DSK], (flash && (select_option == 0)))  + 16;
    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_NO] , (game_cfg.useBDVD == 0)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_YES], (game_cfg.useBDVD == 1)) + 8;

    y2+= 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMCFG_DIRECTBOOT], (flash && (select_option == 1))) + 16;

    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_NO] , (game_cfg.direct_boot == 0)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_YES], (game_cfg.direct_boot == 1)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, "With BR"              , (game_cfg.direct_boot == 2)) + 8;
    if(manager_cfg.global_wm_autoplay)
        x2 = DrawButton2_UTF8(x2, y2, 0, "Global Auto-Play [wMM]", true) + 8;
    else
        x2 = DrawButton2_UTF8(x2, y2, 0, "Auto-Play [wMM]"  , (game_cfg.direct_boot == 3)) + 8;

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMCFG_EXTBOOT], (flash && (select_option == 2)))  + 16;

    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_ON] , (payload_mode >= ZERO_PAYLOAD) ? (game_cfg.ext_ebootbin != 0) : -1 ) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_OFF], (game_cfg.ext_ebootbin == 0)) + 8;

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMCFG_BDEMU], (flash && (select_option == 3)))  + 16;

    if(manager_cfg.global_bd_mirror)
    {
        x2 = DrawButton2_UTF8(x2, y2, 0, "Global BD Mirror"     , true)  + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, "LIBFS"                , false) + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_OFF], false) + 8;
    }
    else if(directories[currentgamedir].flags & D_FLAG_HDD0)
    {
        x2 = DrawButton2_UTF8(x2, y2, 0, "BD Mirror"            , (game_cfg.bdemu == 1)) + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, "LIBFS"                , (game_cfg.bdemu >  1)) + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_OFF], (game_cfg.bdemu == 0)) + 8;
    }
    else
    {
        x2 = DrawButton2_UTF8(x2, y2, 0, "BD Mirror"            , (game_cfg.bdemu_ext == 1)) + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, "LIBFS"                , (game_cfg.bdemu_ext >  1)) + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_OFF], (game_cfg.bdemu_ext == 0)) + 8;
    }

    y2+= 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMCFG_EXTHDD0GAME], (flash && (select_option == 4)))  + 16;

    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_ON] , (payload_mode >= ZERO_PAYLOAD) ? (game_cfg.exthdd0emu != 0): -1) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_OFF], (game_cfg.exthdd0emu == 0)) + 8;

    y2+= 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMCFG_SAVECFG], (flash && (select_option == 5)))  + 16;
    y2+= 48;


    SetCurrentFont(FONT_TTF);

    // draw game name

    i = selected;

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);

    SetFontColor(WHITE, BLACK);

    if(Png_offset[i])
    {
        u32 str_color = WHITE;

        if((directories[currentgamedir].flags & GAMELIST_FILTER) == BDVD_FLAG)
        {
            if(strncmp((char *) string_title_utf8, bluray_game, 64))
            {
                strncpy((char *) string_title_utf8, bluray_game, 128);
                update_title_utf8 = true;
            }
            str_color = 0x00ff00ff;
        }
        else
        {
            if(strncmp((char *) string_title_utf8, directories[currentgamedir].title, 64))
            {
                strncpy((char *) string_title_utf8, directories[currentgamedir].title, 128);
                update_title_utf8 = true;
            }
        }

        if(update_title_utf8)
        {
            width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, 16, 24);
            update_title_utf8 = false;
        }

        tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
                32, 768 * 2,
                TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        DrawTextBox((848 - width_title_utf8) / 2, y + 3 * 150 , 0, 768, 32, str_color);
    }


    tiny3d_Flip();
    ps3pad_read();

    if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))
    {
        // auto-save on exit if file exists or any setting is a non-default value
        sprintf(tmp_path, "%s/config/%s.cfg", self_path, directories[currentgamedir].title_id);
        if((game_cfg.useBDVD | game_cfg.direct_boot | game_cfg.bdemu | game_cfg.bdemu_ext | game_cfg.exthdd0emu | game_cfg.ext_ebootbin) > 0 || file_exists(tmp_path))
        {
            save_gamecfg();
        }

        menu_screen = SCR_MENU_GAME_OPTIONS;
        select_option = 0;
        return;
    }
    else if(new_pad & (BUTTON_START))
    {
        select_option = 5;
    }


    if(new_pad & (BUTTON_CROSS_ | BUTTON_START))
    {
        switch(select_option)
        {
            //removed sys8 calls not supported yet on 3.55
#ifdef CONFIG_USE_SYS8PERMH4
            case 0:
                ROT_INC(game_cfg.perm, 2, 0);
                break;
#endif
            case 0:
                ROT_INC(game_cfg.useBDVD, 1, 0);
                break;
            case 1:
                if(game_cfg.direct_boot == 3 && (new_pad & BUTTON_CROSS_))
                    manager_cfg.global_wm_autoplay ^= 1;
                else
                    ROT_INC(game_cfg.direct_boot, 3, 0);
                break;
            case 2:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_INC(game_cfg.ext_ebootbin, 1, 0);
                break;
            case 3:
                if(directories[currentgamedir].flags & D_FLAG_HDD0)
                {
                    if(game_cfg.bdemu == 1 && (new_pad & BUTTON_CROSS_))
                        manager_cfg.global_bd_mirror ^= 1;
                    else
                        ROT_INC(game_cfg.bdemu, 2, 0)
                }
                else
                {
                    if(game_cfg.bdemu_ext == 1 && (new_pad & BUTTON_CROSS_))
                        manager_cfg.global_bd_mirror ^= 1;
                    else
                        ROT_INC(game_cfg.bdemu_ext, 2, 0)
                }
                break;
            case 4:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_INC(game_cfg.exthdd0emu, 1, 0);
                break;
            case 5:
                if(save_gamecfg() == SUCCESS)
                {
                    return_to_game_list(false);
                    return;
                }

                break;
            default:
                menu_screen = SCR_MENU_GAME_OPTIONS;
                select_option = 0;
                return;
        }
    }

    if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, 5);
    }

    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, 5, 0);
    }

    else if(new_pad & BUTTON_LEFT)
    {
        switch(select_option)
        {
            //removed sys8 calls not supported yet on 3.55
#ifdef CONFIG_USE_SYS8PERMH4
            case 0:
                ROT_DEC(game_cfg.perm, 0, 2);
                break;
#endif
            case 0:
                ROT_DEC(game_cfg.useBDVD, 0, 1);
                break;
            case 1:
                ROT_DEC(game_cfg.direct_boot, 0, 3);
                break;
            case 2:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_DEC(game_cfg.ext_ebootbin, 0, 1);
                break;
            case 3:
                manager_cfg.global_bd_mirror = 0;
                if(directories[currentgamedir].flags & D_FLAG_HDD0)
                    ROT_DEC(game_cfg.bdemu, 0, 2)
                else
                    ROT_DEC(game_cfg.bdemu_ext, 0, 2)
                break;
            case 4:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_DEC(game_cfg.exthdd0emu, 0, 1);
                break;
            default:
                break;
        }
     }

     else if(new_pad & BUTTON_RIGHT)
     {
        switch(select_option)
        {
            //removed sys8 calls not supported yet on 3.55
#ifdef CONFIG_USE_SYS8PERMH4
            case 0:
                ROT_INC(game_cfg.perm, 2, 0);
                break;
#endif
            case 0:
                ROT_INC(game_cfg.useBDVD, 1, 0);
                break;
            case 1:
                ROT_INC(game_cfg.direct_boot, 3, 0);
                break;
            case 2:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_INC(game_cfg.ext_ebootbin, 1, 0);
                break;
            case 3:
                manager_cfg.global_bd_mirror = 0;
                if(directories[currentgamedir].flags & D_FLAG_HDD0)
                    ROT_INC(game_cfg.bdemu, 2, 0)
                else
                    ROT_INC(game_cfg.bdemu_ext, 2, 0)
                break;
            case 4:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_INC(game_cfg.exthdd0emu, 1, 0);
                break;
            default:
                break;
        }
     }
}

void draw_gbloptions(float x, float y)
{

    float y2 = 0;
    static int enable_event_flag = 0;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWGLOPT_OPTS]);

    DrawFormatString(710 - x, y, ((manager_cfg.opt_flags & OPTFLAGS_PLAYMUSIC) && (song_selected >= 0)) ? "Song: music%i" : "", song_selected + 1);

    y += 24;

    DrawBox(x, y, 0, box_width, box_height, BLACK);

    y2 = y + 32 - 24;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWGLOPT_TOOLS], (flash && (select_option == -1)));
    y2+= 48;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWGLOPT_REFRESH], (flash && (select_option == 0)));
    y2+= 48;

    sprintf(temp_buffer, "%s: %s", language[DRAWGLOPT_CHANGEGUI], (gui_mode == MODE_COVERFLOW) ? "Coverflow" :
                                                                  (gui_mode == MODE_GRID_3x2)  ? "Grid 3x2"  :
                                                                  (gui_mode == MODE_GRID_4x2)  ? "Grid 4x2"  :
                                                                  (gui_mode == MODE_GRID_3x3)  ? "Grid 3x3"  :
                                                                  (gui_mode == MODE_GRID_4x3)  ? "Grid 4x3"  :
                                                                  (gui_mode == MODE_GRID_5x3)  ? "Grid 5x3"  :
                                                                  (gui_mode == MODE_GRID_6x3)  ? "Grid 6x3"  :
                                                                  (gui_mode == MODE_GRID_4x4)  ? "Grid 4x4"  :
                                                                  (gui_mode == MODE_GRID_5x4)  ? "Grid 5x4"  :
                                                                  (gui_mode == MODE_GRID_6x4)  ? "Grid 6x4"  :
                                                                  (gui_mode == MODE_GRID_5x5)  ? "Grid 5x5"  :
                                                                  (gui_mode == MODE_GRID_6x5)  ? "Grid 6x5"  :
                                                                  (gui_mode == MODE_GRID_7x6)  ? "Grid 7x6"  :
                                                                  (gui_mode == MODE_GRID_8x5)  ? "Grid 8x5"  :
                                                                  (gui_mode == MODE_GRID_8x6)  ? "Grid 8x6"  : "XMB-Like");

    if(cover_mode) strcat(temp_buffer, cover_mode == 1 ? " + Cover" : cover_mode == 2 ? " + Box" : " + Box (ISO)");

    DrawButton1_UTF8(box_left, y2, 520, temp_buffer, (flash && (select_option == 1)));
    y2+= 48;

    if(background_sel == 0)
        sprintf(temp_buffer, "%s [FX %i]", language[DRAWGLOPT_CHANGEBCK], background_fx);
    else if(background_sel == 1)
    {
        if(bk_picture == BG_NONE)
          sprintf(temp_buffer, language[DRAWGLOPT_BCKPICNONE]);
        else if(bk_picture == BG_RANDOM)
          sprintf(temp_buffer, language[DRAWGLOPT_BCKPICRAND]);
        else if(bk_picture == BG_PIC1)
          sprintf(temp_buffer, language[DRAWGLOPT_BCKPIC1]);
        else
          sprintf(temp_buffer, "%s PICT%i.JPG", language[DRAWGLOPT_BCKPICT], bk_picture - 3);
    }
    else
        sprintf(temp_buffer, "%s [Color %i]", language[DRAWGLOPT_CHANGEBCK], background_sel - 1);

    DrawButton1_UTF8(box_left, y2, 520, temp_buffer, (flash && (select_option == 2)));
    y2+= 48;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWGLOPT_SCRADJUST], (flash && (select_option == 3)));
    y2+= 48;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWGLOPT_CHANGEDIR], (flash && (select_option == 4)));
    y2+= 48;

    DrawButton1_UTF8(box_left, y2, 520, (manager_cfg.opt_flags & OPTFLAGS_PLAYMUSIC)? language[DRAWGLOPT_SWMUSICOFF] : language[DRAWGLOPT_SWMUSICON] , (flash && (select_option == 5)));
    y2+= 48;

    bool bSelected = (flash && (select_option == 6));

    if(!bAllowNetGames &&  !(net_option == 0 || (net_option >= 7 && net_option <= 13))) net_option = 0;

    if (net_option == 0)
        DrawButton1_UTF8(box_left, y2, 520, (*ftp_ip_str) ? ftp_ip_str : language[DRAWGLOPT_INITFTP], bSelected);
    else
        DrawButton1_UTF8(box_left, y2, 520, (net_option ==  1) ? "Mount /net_host0/PKG as /dev_bdvd" :
                                                   (net_option ==  2) ? "Mount /net_host0/VIDEO as /dev_bdvd" :
                                                   (net_option ==  3) ? "Mount /net_host1/PKG as /dev_bdvd" :
                                                   (net_option ==  4) ? "Mount /net_host1/VIDEO as /dev_bdvd" :
                                                   (net_option ==  5) ? "Refresh My Games (XML)" :
                                                   (net_option ==  6) ? "webMAN Setup" :
                                                   (net_option ==  7) ? "Download Latest webMAN" :
                                                   (net_option ==  8) ? ((use_cobra && !use_mamba) ? "Disable Cobra" : "Enable Cobra") :
                                                   (net_option ==  9) ? "Showtime" :
                                                   (net_option == 10) ? "Internet Browser" :
                                                   (net_option == 11) ? (file_exists("/dev_blind") ? "/dev_blind : ON" : "/dev_blind : OFF") :
                                                   (net_option == 12) ? language[DRAWSCREEN_SHUTDOWN] :
                                                   (net_option == 13) ? language[DRAWSCREEN_RESTART]  : "", bSelected);

    y2+= 48;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWTOOLS_COVERSDOWN], (flash && (select_option == 7)));
    y2+= 48;

    if(select_option >= 7)
        DrawButton1_UTF8(box_left, y2, 520, language[DRAWGLOPT_CREDITS], (flash && (select_option == 8)));

    y2+= 48;

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);
    //y2+= 48;

    // draw sys version
    SetCurrentFont(FONT_TTF);
    SetFontColor(0xccccffff, BLACK);
    SetFontAutoCenter(1);

    if(select_option == 1)
    {
        SetFontSize(14, 20);

        if(show_custom_icons)
            DrawFormatString(500, y2 - 28, "SQUARE  Toggle Covers        R1/L1 - Showtime Icon: ON        <  >  Select GUI mode");
        else
            DrawFormatString(500, y2 - 28, "SQUARE  Toggle Covers        R1/L1 - Showtime Icon: OFF       <  >  Select GUI mode");
    }
    else if(select_option == 2)
    {
        SetFontSize(16, 20);
        if(background_sel == 0)
            DrawFormatString(500, y2 - 28, "X  Change Background           <  >  Select FX type");
        else if(background_sel == 1)
            DrawFormatString(500, y2 - 28, "X  Change Background           <  >  Select Picture");
        else
            DrawFormatString(500, y2 - 28, "X  Change Background           <  >  Select Color");
    }
    else if(select_option == 6)
    {
        SetFontSize(16, 20);
        DrawFormatString(500, y2 - 28, "<  >  Select network option");
    }
    else if(select_option < 7)
    {
        SetFontSize(16, 20);

        if(enable_event_flag && !use_cobra)
        {
            if(lv2peek(0x80000000000004E8ULL))
            {
                u32 eid = (u32) syscall_40(4, 0);

                DrawFormatString(0, y2 - 28, "Event ID: %x / VSH ID %x", eid, manager_cfg.event_flag);

                if(eid != 0)
                {
                    eid -= 0x100;

                    if(eid != manager_cfg.event_flag)
                    {
                        manager_cfg.event_flag = eid;
                        SaveManagerCfg();
                    }
                }
            }
        }
        else
            DrawFormatString(0, y2 - 28, "%x%s  -  %s", ((firmware>>4) & 0xFFFF), ((firmware & 0xF) == 0xE ? "DEH": (firmware & 0xF) == 0xD ? "DEX": "CEX"), payload_str);
    }

    SetFontAutoCenter(0);

    tiny3d_Flip();
    ps3pad_read();

    if(new_pad & BUTTON_L3) {select_option = 0; new_pad = BUTTON_CROSS_;}
    if((new_pad & BUTTON_SELECT) && select_option < 1 && !use_cobra) enable_event_flag ^= 1;

    if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))
    {
exit_gbloptions:
       if(refresh_gui)
       {
           refresh_gui = false;

           get_grid_dimensions(true);

           manager_cfg.cover_mode = cover_mode;
           manager_cfg.gui_mode = ((sort_mode & 0xf)<<4) | (gui_mode & 0xf);
           SaveManagerCfg();

           load_background_picture();

           get_games();
           load_gamecfg(RESET_GAME_INFO); // force refresh game info
        }

        enable_event_flag = 0;
        return_to_game_list(false);
        return;
    }

    if(enable_event_flag) return;

    if((new_pad & BUTTON_CROSS_) ||
       ((new_pad & (BUTTON_LEFT | BUTTON_RIGHT)) &&
        ((select_option <= 1) || ((select_option == 2) && (background_sel > 1)) || (select_option == 5)) ))
    {
        if((new_pad & BUTTON_LEFT) && (select_option < 1)) goto exit_gbloptions;

        switch(select_option)
        {
            case -1: // Tools menu

                menu_screen = SCR_MENU_TOOLS;
                select_option = 0;
                return;

            case 0: // Select Game List

                menu_screen = SCR_MENU_GAME_LIST;
                select_option = 0;
                return;

            case 1: // Change Current GUI
                if(new_pad & BUTTON_CROSS_) goto exit_gbloptions;

                if(new_pad & (BUTTON_LEFT)) gui_mode--; else gui_mode++;

                if(gui_mode > GUI_MODES)
                {
                    gui_mode = 0; // MODE_XMB-like
                    ROT_INC(cover_mode, 3, 0);
                }
                else if(gui_mode < 0)
                {
                    gui_mode = GUI_MODES;
                    cover_mode ^= 1;
                    ROT_INC(cover_mode, 3, 0);
                }

                if(gui_mode > MODE_GRID_6x5 && bk_picture == BG_PIC1) bk_picture = BG_NONE;

                refresh_gui = true;
                break;
            case 2: // Change Background Color
                if (new_pad & BUTTON_LEFT) background_sel--; else background_sel++;;
                if(background_sel >= MAX_COLORS) background_sel = 0;
                manager_cfg.background_sel = background_sel;
                SaveManagerCfg();
                break;

            case 3: // Video Adjust
                video_adjust();
                return_to_game_list(false);
                return;

            case 4: // Change Game Directory
                menu_screen = SCR_MAIN_GAME_LIST;
                Select_games_folder();

                if(manager_cfg.hdd_folder[0] == 0) strcpy(manager_cfg.hdd_folder, __MKDEF_MANAGER_DIR__);
                SaveManagerCfg();

                return_to_game_list(true);
                return;

            case 5: // Switch Music On/Off
                if (new_pad & BUTTON_LEFT)
                {
                    if(song_selected > 0) song_selected--; else song_selected = MAX_SONGS - 1;
                    init_music(song_selected);

                }
                else if (new_pad & BUTTON_RIGHT)
                {
                    if(song_selected < MAX_SONGS) song_selected++; else song_selected = 0;
                    init_music(song_selected);
                }
                else
                {
                    manager_cfg.opt_flags ^= OPTFLAGS_PLAYMUSIC; //change bit
                    pause_music((manager_cfg.opt_flags & OPTFLAGS_PLAYMUSIC)? 0 : 1);
                    SaveManagerCfg();
                }
                break;

            case 6: // Initialize FTP server
                switch (net_option)
                {
                  case 1: // Mount net0/PKG
                    if(bAllowNetGames && get_net_status() == SUCCESS)
                    {
                        call_webman("/mount_ps3/net0/PKG");

                        sprintf(temp_buffer, "Mounted /net_host0/PKG as /dev_bdvd\n\n%s", language[DRAWSCREEN_EXITXMB]);
                        if(DrawDialogYesNo(temp_buffer) == YES)
                            exit_to_xmb();
                        else
                            file_manager("/dev_bdvd", NULL);
                    }
                    break;
                  case 2: // Mount net0/VIDEO
                    if(bAllowNetGames && get_net_status() == SUCCESS)
                    {
                        call_webman("/mount_ps3/net0/VIDEO");

                        sprintf(temp_buffer, "Mounted /net_host0/VIDEO as /dev_bdvd");

                        if(gui_mode == MODE_XMB_LIKE || bk_picture == BG_PIC1) load_background_picture();

                        DrawDialogTimer(temp_buffer, 1200.0f);
                        game_list_category = GAME_LIST_PS3_ONLY;
                        mode_homebrew = VIDEOS_MODE;
                        mode_favourites = 0;

                        return_to_game_list(true);
                    }
                    break;
                  case 3: // Mount net1/PKG
                    if(bAllowNetGames && get_net_status() == SUCCESS)
                    {
                        call_webman("/mount_ps3/net1/PKG");

                        sprintf(temp_buffer, "Mounted /net_host0/PKG as /dev_bdvd\n\n%s", language[DRAWSCREEN_EXITXMB]);
                        if(DrawDialogYesNo(temp_buffer) == YES)
                            exit_to_xmb();
                        else
                            file_manager("/dev_bdvd", NULL);
                    }
                    break;
                  case 4: // Mount net1/VIDEO
                    if(bAllowNetGames && get_net_status() == SUCCESS)
                    {
                        call_webman("/mount_ps3/net1/VIDEO");

                        sprintf(temp_buffer, "Mounted /net_host1/VIDEO as /dev_bdvd");

                        if(gui_mode == MODE_XMB_LIKE || bk_picture == BG_PIC1) load_background_picture();

                        DrawDialogTimer(temp_buffer, 1200.0f);
                        game_list_category = GAME_LIST_PS3_ONLY;
                        mode_homebrew = VIDEOS_MODE;
                        mode_favourites = 0;

                        return_to_game_list(true);
                    }
                    break;
                  case 5: // Refresh webMAN
                    if(bAllowNetGames && get_net_status() == SUCCESS)
                    {
                        DrawDialogTimer("webMAN is refreshing My Games (XML)\n\nPlease wait ...", 1200.0f);
                        call_webman("/refresh.ps3");

                        //rescan network games
                        DrawDialogTimer("Scanning network games (webMAN)...", 1200.0f);
                        retro_mode = NET_GAMES;

                        game_list_category = 2;
                        mode_homebrew = GAMEBASE_MODE;
                        mode_favourites = 0;

                        return_to_game_list(true);
                    }
                    break;
                  case 6: // Setup webMAN
                    SaveGameList();
                    fun_exit();

                    char* launchargv[2];
                    memset(launchargv, 0, sizeof(launchargv));

                    launchargv[0] = (char*)malloc(27); strcpy(launchargv[0], "http://127.0.0.1/setup.ps3");
                    launchargv[1] = NULL;

                    sprintf(tmp_path, "%s/USRDIR/browser.self", self_path);
                    sysProcessExitSpawn2(tmp_path, (char const**)launchargv, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
                    exit(0);

                    break;

                  case 7: // Download Latest webMAN
                    if(!use_cobra)
                    {
                        DrawDialogOK(credits_str1);
                        DrawDialogOK(credits_str2);
                        DrawDialogOK(credits_str3);
                        DrawDialogOK(credits_str4);
                        break;
                    }

                    sprintf(tmp_path, "/dev_hdd0/plugins/webftp_server.sprx");
                    if(file_exists(tmp_path) == false) sprintf(tmp_path, "/dev_hdd0/plugins/webftp_server_ps3mapi.sprx");
                    if(file_exists(tmp_path) == false) sprintf(tmp_path, webman_path);

                    if(get_net_status() != SUCCESS) break;

                    if(download_file("http://www.aldostools.org/ps3/webftp_server.sprx", tmp_path, 0, NULL) == 0)
                    {
                        DrawDialogOKTimer("webMAN has been updated successfully!", 2000.0f);
                    }
                    else
                        DrawDialogOKTimer("webMAN could not be downloaded!", 2000.0f);

                    // Create boot_plugins.txt if it doesn't exist
                    if(file_exists("/dev_hdd0/boot_plugins.txt") == false && file_exists(tmp_path))
                    {
                        strcat(tmp_path, "\n");

                        FILE *fp;

                        // write plugin path
                        fp = fopen("/dev_hdd0/boot_plugins.txt", "w");
                        fputs (tmp_path, fp);
/*
                        sprintf(tmp_path, "/dev_hdd0/plugins/wm_vsh_menu.sprx");
                        if(file_exists(tmp_path))
                        {
                            strcat(tmp_path, "\n");
                            fputs (tmp_path, fp);
                        }
*/
                        fclose(fp);
                    }

                    return_to_game_list(false);
                    return;

                  case 8: // Toggle Cobra
                    if(file_exists("/dev_blind/sys") == false)
                        sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_blind", 0);

                    if(is_cobra_based())
                    {
                        if(file_exists("/dev_blind/sys/stage2.bin"))
                            sysLv2FsRename("/dev_blind/sys/stage2.bin", "/dev_blind/sys/stage2_disabled.bin");
                        else if(file_exists("/dev_blind/rebug/cobra/stage2.cex") /*&& file_exists("/dev_blind/rebug/cobra/stage2.dex")*/)
                        {
                            sysLv2FsRename("/dev_blind/rebug/cobra/stage2.cex", "/dev_blind/rebug/cobra/stage2.cex.bak");
                            sysLv2FsRename("/dev_blind/rebug/cobra/stage2.dex", "/dev_blind/rebug/cobra/stage2.dex.bak");
                        }
                        else
                            break;

                    }
                    else if(file_exists("/dev_blind/sys/stage2_disabled.bin"))
                        sysLv2FsRename("/dev_blind/sys/stage2_disabled.bin", "/dev_blind/sys/stage2.bin");
                    else if(file_exists("/dev_blind/rebug/cobra/stage2.cex.bak") /*&& file_exists("/dev_blind/rebug/cobra/stage2.dex.bak")*/)
                    {
                        sysLv2FsRename("/dev_blind/rebug/cobra/stage2.cex.bak", "/dev_blind/rebug/cobra/stage2.cex");
                        sysLv2FsRename("/dev_blind/rebug/cobra/stage2.dex.bak", "/dev_blind/rebug/cobra/stage2.dex");
                    }
                    else
                        break;

                    goto reboot;

                  case 9: // Launch Showtime
                    SaveGameList();
                    fun_exit();

                    if((old_pad & (BUTTON_SELECT | BUTTON_L2)) && file_exists("/dev_hdd0/game/HTSS00003/USRDIR/showtime.self"))
                    {
                        sysProcessExitSpawn2("/dev_hdd0/game/HTSS00003/USRDIR/showtime.self", NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
                        exit(0);
                    }
                    else
                        launch_showtime(false);

                    return;
                    break;

                  case 10: // Launch Internet Browser
                    SaveGameList();
                    fun_exit();

                    sprintf(tmp_path, "%s/USRDIR/browser.self", self_path);
                    sysProcessExitSpawn2(tmp_path, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
                    exit(0);

                    break;

                  case 11: // toggle dev_blind

                    if(file_exists("/dev_blind/sys"))
                        sys_fs_umount("/dev_blind");
                    else
                        sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_blind", 0);

                    break;

                  case 12: // Shutdown PS3
                    SaveGameList();

                    set_install_pkg = false;
                    fun_exit();

                    sys_shutdown();
                    break;

                  case 13: // Restart PS3
                  reboot:
                    SaveGameList();

                    set_install_pkg = true;

                    game_cfg.direct_boot = 0;
                    fun_exit();

                    break;

                  default: // 0

                    if(test_ftp_working()) break;

                    if ((manager_cfg.opt_flags & OPTFLAGS_FTP) == 0)
                    {
                        int r = get_net_status();

                        r = ftp_init();
                        if(r == SUCCESS)
                        {
                            ftp_inited = true;
                            if(DrawDialogYesNo(language[DRAWGLOPT_FTPINITED]) != YES)
                                break;
                        }
                        else
                        {
                            show_ftp_error(r);
                            break;
                        }
                    }
                    else
                    {
                            DrawDialogOK(language[DRAWGLOPT_FTPSTOPED]);
                            ftp_deinit();
                            ftp_inited = false;
                    }
                    manager_cfg.opt_flags ^= OPTFLAGS_FTP;
                    SaveManagerCfg();
                }

                frame_count = 0xFF; // force display temp
                break;

            case 7: // Download Covers
                return_to_game_list(false);

                if(get_net_status() != SUCCESS) break;

                int n = covers_update(0);
                if(n == -1) n = covers_update(1); // try again
                wait_event_thread();
                get_games();
                if(n == 0)    DrawDialogOKTimer("Covers downloaded successfully\n\nCarátulas descargadas exitosamente", 2000.0f);
                if(n == -1)   DrawDialogOKTimer("Some covers could not be downloaded\n\nAlgunas carátulas no pueden descargarse", 2000.0f);
                if(n == -555) DrawDialogOKTimer("Aborted by the user\n\nAbortado por el usuario", 2000.0f);
                break;

            case 8: // Credits
                DrawDialogOK(credits_str1);
                DrawDialogOK(credits_str2);
                DrawDialogOK(credits_str3);
                DrawDialogOK(credits_str4);
                break;

            default:
               break;
        }

    }

    if(new_pad & BUTTON_UP)
    {
        if(inited && (old_pad & BUTTON_SELECT))
        {
            mod_volume++;
            if(mod_volume > 63) mod_volume = 63;
            MODPlay_SetVolume( &mod_track, mod_volume, mod_volume);
        }
        else
        {
            frame_count = 32;
            ROT_DEC(select_option, -1, 7);
        }
    }
    else if(new_pad & BUTTON_DOWN)
    {
        if(inited && (old_pad & BUTTON_SELECT))
        {
            if(mod_volume > 0) mod_volume--;
            if(mod_volume < 0) mod_volume = 0;
            MODPlay_SetVolume( &mod_track, mod_volume, mod_volume); // fix the volume to 16 (max 64)
        }
        else
        {
            frame_count = 32;
            ROT_INC(select_option, 8, -1);
        }
    }

    if((new_pad & BUTTON_START) && (select_option == -1))
    {
        menu_screen = SCR_MENU_TOOLS;
        select_option = 0;
        return;
    }
    // Change Current GUI
    else if(select_option == 1)
    {
        // Toggle Showtime / Internet Browser icons
        if(new_pad & (BUTTON_L1 | BUTTON_R1))
        {
            show_custom_icons ^= 1;
            manager_cfg.show_custom_icons = show_custom_icons;

            delete_custom_icons(directories, &ndirectories);
            if(show_custom_icons) add_custom_icons(directories, &ndirectories);
            sort_entries2(directories, &ndirectories, sort_mode);

            SaveManagerCfg();

            get_games();
        }
        // Toggle Cover mode
        else if(new_pad & BUTTON_SQUARE)
        {
            ROT_INC(cover_mode, 3, 0);
            refresh_gui = true;
        }
        // Reset to default Coverflow mode
        else if(new_pad & BUTTON_SELECT)
        {
            gui_mode = MODE_COVERFLOW;
            refresh_gui = true;
        }
        // Exit Global Options menu
        else if(new_pad & BUTTON_START)
        {
            goto exit_gbloptions;
        }
    }
    // Change Background Color
    else if(select_option == 2)
    {
        // Toggle Animated Gears
        if(new_pad & BUTTON_SQUARE)
            bBackgroundGears = (bBackgroundGears) ? false : true;


        // Select animated background FXs
        if(background_sel == 0)
        {
            if(new_pad & (BUTTON_LEFT))
            {
              ROT_DEC(background_fx, 0, 7);
              manager_cfg.background_fx = background_fx;
              SaveManagerCfg();
            }
            else if(new_pad & (BUTTON_RIGHT))
            {
              ROT_INC(background_fx, 7, 0);
              manager_cfg.background_fx = background_fx;
              SaveManagerCfg();
            }
            else if(new_pad & BUTTON_SELECT)
            {
              background_fx = 0;
              load_background_picture();

              manager_cfg.background_fx = background_fx;
              SaveManagerCfg();
            }
        }
        // Select background picture
        else if(background_sel == 1)
        {
            if(new_pad & (BUTTON_LEFT))
            {
              if(bk_picture == BG_NONE)
              {
                  background_sel = 0;
              }
              else
              {
                  ROT_DEC(bk_picture, 0, 99);
                  if(gui_mode > MODE_GRID_6x5 && bk_picture == BG_PIC1) bk_picture--;

                  load_background_picture();

                  manager_cfg.bk_picture = bk_picture;
                  SaveManagerCfg();
              }
            }
            else if(new_pad & (BUTTON_RIGHT))
            {
              ROT_INC(bk_picture, 99, 0);
              if(gui_mode > MODE_GRID_6x5 && bk_picture == BG_PIC1) bk_picture++;

              load_background_picture();

              manager_cfg.bk_picture = bk_picture;
              SaveManagerCfg();
            }
            else if(new_pad & BUTTON_SELECT)
            {
              bk_picture = BG_NONE;
              load_background_picture();

              manager_cfg.bk_picture = bk_picture;
              SaveManagerCfg();
            }
        }
        else if(new_pad & BUTTON_SELECT)
        {
          background_sel = 1;
          background_fx  = 4;
          bk_picture = BG_NONE;
        }

        // Exit Global Options menu
        if(new_pad & BUTTON_START) goto exit_gbloptions;
    }
    // Network Tools
    else if(select_option == 6)
    {
        if((new_pad & BUTTON_SQUARE) && ((manager_cfg.opt_flags & OPTFLAGS_FTP) == 0))
        {
             ftp_port = (ftp_port == 21) ? 22 : (ftp_port == 22) ? 2121 : 21;
             sprintf(ftp_ip_str, "%s : %i", language[DRAWGLOPT_INITFTP], ftp_port);
        }

        if(new_pad & (BUTTON_LEFT))
        {
            ROT_DEC(net_option, 0, 13)
            if(net_option == 8 && file_exists("/dev_flash/sys/stage2.bin") == false
                               && file_exists("/dev_flash/sys/stage2_disabled.bin") == false
                               && file_exists("/dev_flash/rebug/cobra/stage2.cex") == false
                               && file_exists("/dev_flash/rebug/cobra/stage2.cex.bak") == false) net_option--;
        }
        else if(new_pad & (BUTTON_RIGHT))
        {
            ROT_INC(net_option, 13, 0)
            if(net_option == 8 && file_exists("/dev_flash/sys/stage2.bin") == false
                               && file_exists("/dev_flash/sys/stage2_disabled.bin") == false
                               && file_exists("/dev_flash/rebug/cobra/stage2.cex") == false
                               && file_exists("/dev_flash/rebug/cobra/stage2.cex.bak") == false) net_option++;
        }
        else if(new_pad & (BUTTON_SELECT | BUTTON_SQUARE))
            net_option = 0;
    }
}

void draw_toolsoptions(float x, float y)
{

    float y2;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWTOOLS_TOOLS]);

    y += 24;

    DrawBox(x, y, 0, box_width, box_height, BLACK);

    y2 = y + 12;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWTOOLS_ARCHIVEMAN], (flash && (select_option == 0)));

    y2+= 52;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWTOOLS_PKGTOOLS], (flash && (select_option == 1)));

    y2+= 52;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWTOOLS_DELCACHE], (flash && (select_option == 2)));

    y2+= 52;


    DrawButton1_UTF8(box_left, y2, 520, language[DRAWTOOLS_COPYFROM], (flash && (select_option == 3)));

    y2+= 52;

    DrawButton1_UTF8(box_left, y2, 520, (noBDVD == MODE_WITHBDVD) ? language[DRAWTOOLS_WITHBDVD] :
                                        (noBDVD == MODE_DISCLESS) ? language[DRAWTOOLS_NOBDVD2]  :
                                                                    language[DRAWTOOLS_NOBDVD], (flash && (select_option == 4)));

    y2+= 52;

    DrawButton1_UTF8(box_left, y2, 520, language[DRAWTOOLS_LANGUAGE_1 + (manager_cfg.language & 0xF)], (flash && (select_option == 5)));

    y2+= 52;

    DrawButton1_UTF8(box_left, y2, 520, "Spoof Console ID", (flash && (select_option == 6)));

    y2+= 52;

    DrawButton1_UTF8(box_left, y2, 520, "Control Fan & USB Wakeup", (flash && (select_option == 7)));



    SetCurrentFont(FONT_TTF);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);

    SetFontColor(WHITE, BLACK);

    tiny3d_Flip();
    ps3pad_read();

    if((new_pad & BUTTON_LEFT) && !(select_option == 4 || select_option == 5)) {old_pad = new_pad; new_pad = BUTTON_CIRCLE_;}

    if((new_pad & BUTTON_CROSS_) ||
       ( (select_option == 4 || select_option == 5) && (new_pad & (BUTTON_LEFT | BUTTON_RIGHT))) ||
       (!(select_option == 4 || select_option == 5) && (new_pad & BUTTON_RIGHT)) )
    {
        switch(select_option)
        {
            case 0: // file manager
                if (file_manager(NULL, NULL) == REFRESH_GAME_LIST) return_to_game_list(true);

                frame_count = 0xFF; // force display temp
                return;

            case 1: //PKG Install
                DrawDialogOKTimer("For NTFS and EXT2/3/4 devices use the File Manager\n\nPara dispositivos NTFS y EXT2/3/4 usa el Manejador Archivos\n\nPour les lecteurs NTFS et EXT2/3/4 utiliser le gestionnaire de fichiers", 3000.0f);
                draw_pkginstall(x, y);
                break;

            case 2: //Delete Cache Tool

                LoadCacheDatas();
                // draw_cachesel

                if(ncache_list > 0)
                {
                    menu_screen = SCR_TOOL_DELETE_CACHE;
                    select_option = 0;
                }

                return;

            case 3: // Copy from /dev_usb/iris to Iris
                if(test_ftp_working()) break;
                copy_usb_to_iris(self_path);
                break;

            case 4: // Set noBDVD mode
                if(test_ftp_working()) break;

                if(new_pad & BUTTON_LEFT)
                {
                    ROT_DEC(noBDVD, 0, 2);
                }
                else
                {
                    ROT_INC(noBDVD, 2, 0);
                }
                break;

            case 5: //Language
                if(new_pad & BUTTON_LEFT)
                {
                    ROT_DEC(manager_cfg.language, 0, LANGCOUNT);
                }
                else
                {
                    ROT_INC(manager_cfg.language, LANGCOUNT, 0);
                }

                sprintf(tmp_path, "%s/config/language.ini", self_path);
                open_language(manager_cfg.language, tmp_path);
                //manager_cfg.usekey = manager_cfg.usekey == 0;
                SaveManagerCfg();

                break;

            case 6: // set console id
                menu_screen = SCR_MENU_CONSOLE_ID_TOOLS;
                select_option = 0;
                return;

            case 7: // Control Fan & USB Wakeup
                draw_controlfan_options();

                return_to_game_list(false);
                return;

            default:
                break;
        }

    }
    else if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))
    {
        if (noBDVD != manager_cfg.noBDVD)
        {
            manager_cfg.noBDVD = noBDVD & 3;
            SaveManagerCfg();

            sys_fs_umount("/dev_bdvd");
            sys_fs_umount("/dev_ps2disc");

            if(noBDVD) Eject_BDVD(NOWAIT_BDVD | EJECT_BDVD);

            if(!manager_cfg.noBDVD)
                  sprintf(temp_buffer, "%s\n\n%s?", language[DRAWTOOLS_WITHBDVD], "Restarting the PS3...");
            else if(manager_cfg.noBDVD == MODE_DISCLESS)
                  sprintf(temp_buffer, "%s\n\n%s?", language[DRAWTOOLS_NOBDVD2], "Restarting the PS3...");
            else
                  sprintf(temp_buffer, "%s\n\n%s?", language[DRAWTOOLS_NOBDVD], "Restarting the PS3...");

            DrawDialogOKTimer(temp_buffer, 2500.0f);
            game_cfg.direct_boot = 0;

            ps3pad_read();

            if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE)) exit_to_xmb();
            else
            {
                unlink_secure("/dev_hdd0/tmp/turnoff");
                unlink_secure("/dev_hdd0/tmp/wm_request");

                SaveGameList();
                fun_exit();

                sys_reboot();
            }
        }

        if(old_pad & BUTTON_LEFT)
        {
            menu_screen = SCR_MENU_GLOBAL_OPTIONS;
            select_option = -1;
        }
        else
            return_to_game_list(false);
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, 7);
    }
    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, 7, 0);
    }
}

void draw_gamelist_options(float x, float y)
{

    float y2, x2 = box_left;

    int max_options = 5;

    SetCurrentFont(FONT_TTF);


    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    if(filter_by_letter)
        sprintf(temp_buffer, "%s [%c]", language[DRAWGLOPT_REFRESH], 47 + filter_by_letter);
    else
        strcpy(temp_buffer, language[DRAWGLOPT_REFRESH]);

    DrawFormatString(x, y, " %s", temp_buffer);

    y += 24;

    DrawBox(x, y, 0, box_width, box_height, BLACK);

    y2 = y + 12 * 3;

    if(opt_filter_by_device < HDD0_DEVICE || opt_filter_by_device >= 20)
        DrawButton1_UTF8(x2, y2, 520, language[GAMELIST_ALLGAMES], (flash && (select_option == 0)));
    else if(opt_filter_by_device == HDD0_DEVICE)
    {
        sprintf(temp_buffer, "%s /dev_hdd0", language[DRAWSCREEN_GAMES]);
        DrawButton1_UTF8(x2, y2, 520, temp_buffer, (flash && (select_option == 0)));
    }
    else if(opt_filter_by_device == BDVD_DEVICE)
    {
        sprintf(temp_buffer, "%s /dev_bdvd", language[DRAWSCREEN_GAMES]);
        DrawButton1_UTF8(x2, y2, 520, temp_buffer, (flash && (select_option == 0)));
    }
    else if(opt_filter_by_device > HDD0_DEVICE && opt_filter_by_device < BDVD_DEVICE)
    {
        sprintf(temp_buffer, "%s /dev_usb00%c", language[DRAWSCREEN_GAMES], 47 + opt_filter_by_device);
        DrawButton1_UTF8(x2, y2, 520, temp_buffer, (flash && (select_option == 0)));
    }
    else
    {
        sprintf(temp_buffer, "%s /ntfs%c", language[DRAWSCREEN_GAMES], 36 + opt_filter_by_device);
        DrawButton1_UTF8(x2, y2, 520, temp_buffer, (flash && (select_option == 0)));
    }

    y2+= 52;

    DrawButton1_UTF8(x2, y2, 520, language[GAMELIST_PS3GAMES], (flash && (select_option == 1)));

    y2+= 52;

    if(use_cobra && !is_mamba_v2)
        DrawButton1_UTF8(x2, y2, 520, "PlayStation® + PlayStation®2 + PSP", (flash && (select_option == 2)));
    else
        DrawButton1_UTF8(x2, y2, 520, "PlayStation®", (flash && (select_option == 2)));

    y2+= 52;

    DrawButton1_UTF8(x2, y2, 520, language[GAMELIST_VIDEOS], (flash && (select_option == 3)));

    y2+= 52;

    DrawButton1_UTF8(x2, y2, 520, "Homebrews", (flash && (select_option == 4)));

    y2+= 52;


    if(bAllowNetGames)
    {
        DrawButton1_UTF8(x2, y2, 520, language[GAMELIST_NETGAMES], (flash && (select_option == 5)));

        y2+= 52;

        max_options = 6;
    }


    bool bSelected = (flash && (select_option == max_options));

    switch(retro_mode)
    {
     case RETRO_PSX:
        DrawButton1_UTF8(x2, y2, 520, "PlayStation® (PSX)", bSelected);
        break;
     case RETRO_PS2:
        DrawButton1_UTF8(x2, y2, 520, "PlayStation®2 (PS2)", bSelected);
        break;
     case RETRO_PSP:
        DrawButton1_UTF8(x2, y2, 520, "PlayStation® Portable (PSP)", bSelected);
        break;
     case RETRO_SNES:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Super Nintendo (SNES)", bSelected);
        break;
     case RETRO_GBA:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Gameboy Advance (GBA)", bSelected);
        break;
     case RETRO_GEN:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Sega Genesis / Megadrive", bSelected);
        break;
     case RETRO_NES:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Nintendo (NES)", bSelected);
        break;
     case RETRO_MAME:
        DrawButton1_UTF8(x2, y2, 520, "Retro: MAME / Arcade Games", bSelected);
        break;
     case RETRO_FBA:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Final Burn Alpha (FBA)", bSelected);
        break;
     case RETRO_QUAKE:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Quake", bSelected);
        break;
     case RETRO_DOOM:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Doom", bSelected);
        break;
     case RETRO_PCE:
        DrawButton1_UTF8(x2, y2, 520, "Retro: PC Engine / TurboGrafx (PCE)", bSelected);
        break;
     case RETRO_GBC:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Gameboy / Gameboy Color (GBC)", bSelected);
        break;
     case RETRO_ATARI:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Atari 2600", bSelected);
        break;
     case RETRO_VBOY:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Nintendo Virtual Boy (VBOY)", bSelected);
        break;
     case RETRO_NXE:
        DrawButton1_UTF8(x2, y2, 520, "Retro: NX Engine (Cave Story)", bSelected);
        break;
     case RETRO_WSWAN:
        DrawButton1_UTF8(x2, y2, 520, "Retro: WonderSwan", bSelected);
        break;

     case RETRO_A7800:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Atari 7800", bSelected);
        break;
     case RETRO_LYNX:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Atari Lynx", bSelected);
        break;
     case RETRO_GW:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Game & Watch", bSelected);
        break;
     case RETRO_VECTX:
        DrawButton1_UTF8(x2, y2, 520, "Retro: Vectrex", bSelected);
        break;
     case RETRO_2048:
        DrawButton1_UTF8(x2, y2, 520, "Retro: 2048", bSelected);
        break;

     default:
        if(use_cobra && !is_mamba_v2)
            DrawButton1_UTF8(x2, y2, 520, "PSX + PS2 + PSP + Retro", bSelected);
        else
            DrawButton1_UTF8(x2, y2, 520, "PSX + Retro", bSelected);
        break;
    }



    SetCurrentFont(FONT_TTF);

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);

    SetFontColor(WHITE, BLACK);
    SetFontAutoCenter(1);
    SetFontSize(14, 20);

    if(select_option == max_options)
        DrawFormatString(500, 480, language[GAMELIST_STATUS1]);
    else
        DrawFormatString(500, 480, language[GAMELIST_STATUS2]);


    tiny3d_Flip();
    ps3pad_read();

    if((new_pad & BUTTON_LEFT) && !(select_option == 0 || select_option == max_options)) {old_pad = new_pad; new_pad = BUTTON_CIRCLE_;}

    if(new_pad & BUTTON_L3) //L3 = Close menu
    {
        while((new_pad | old_pad) & BUTTON_L3) ps3pad_read();

        new_pad = BUTTON_CIRCLE_;
    }
    else if(new_pad & BUTTON_R3)  //R3 = Refresh
    {
        while((new_pad | old_pad) & BUTTON_R3) ps3pad_read();

             if(mode_homebrew == VIDEOS_MODE)   select_option = 3;
        else if(mode_homebrew == HOMEBREW_MODE) select_option = 4;
        else if(game_list_category == GAME_LIST_PS3_ONLY)  select_option = 1;
        else if(game_list_category == (GAME_LIST_RETRO | GAME_LIST_NETHOST))
        {
            if(retro_mode == RETRO_PSALL) select_option = 2;
            else if(retro_mode != NET_GAMES) select_option = max_options;
            else max_options = max_options - 1;
        }
        else
            select_option = 0;

        new_pad = BUTTON_CROSS_;
    }

    static int frame = 0;
    frame++; if(frame > 1000) frame = 0;

    if(new_pad & (BUTTON_CROSS_ | BUTTON_START))
    {

        // refresh custom settings
        read_settings();

        bXMLScanOnlyNetGames = true;

        force_ntfs_mount = true; frame_count = 62; ntfs_retry = MAX_NTFS_RETRY;
        for(int i = 0; i < 8; i++) automountCount[i] = 2;

        filter_by_device = opt_filter_by_device;

        // scan game list
        switch(select_option)
        {
            case 0: // all games
                //DrawDialogTimer(language[GAMELIST_SCANNING0], 1200.0f);
                retro_mode = RETRO_ALL;
                game_list_category = GAME_LIST_ALL;
                mode_homebrew = GAMEBASE_MODE;
                mode_favourites = 0;
                break;

            case 1: // ps3 games only
                //DrawDialogTimer(language[GAMELIST_SCANNING1], 1200.0f);
                game_list_category = GAME_LIST_PS3_ONLY;
                mode_homebrew = GAMEBASE_MODE;
                mode_favourites = 0;
                break;

            case 2: // psx + ps2 + psp
                //DrawDialogTimer(language[GAMELIST_SCANNING2], 1200.0f);
                retro_mode = RETRO_PSALL;
                game_list_category = GAME_LIST_RETRO;
                mode_homebrew = GAMEBASE_MODE;
                mode_favourites = 0;
                break;

            case 3: // videos
                if(gui_mode == MODE_XMB_LIKE || bk_picture == BG_PIC1) load_background_picture();

                //DrawDialogTimer(language[GAMELIST_SCANNING3], 1200.0f);
                game_list_category = GAME_LIST_PS3_ONLY;
                mode_homebrew = VIDEOS_MODE;
                mode_favourites = 0;
                break;

            case 4: // Homebrews
                game_list_category = GAME_LIST_PS3_ONLY;
                mode_homebrew = HOMEBREW_MODE;
                mode_favourites = 0;
                break;

            default:
                if(select_option == max_options || bAllowNetGames == false)
                {
                    // retro
                    if(retro_mode > 22) retro_mode = RETRO_ALL;
                }
                else
                {
                    // net games
                    retro_mode = NET_GAMES, filter_by_device = opt_filter_by_device = LIST_ALL_DEVICES; refresh_game_list = true;

                    if(old_pad & (BUTTON_SELECT | BUTTON_L2)) bXMLScanOnlyNetGames = false;
                }
                game_list_category = (GAME_LIST_RETRO | GAME_LIST_NETHOST);
                mode_homebrew = GAMEBASE_MODE;
                mode_favourites = 0;
                break;
        }

        return_to_game_list(true);
        return;

    }
    else if(new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))
    {
        if(old_pad & BUTTON_LEFT)
            menu_screen = SCR_MENU_GLOBAL_OPTIONS;
        else
            return_to_game_list(false);

        select_option = 0;
        opt_filter_by_device = filter_by_device;
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, max_options);
        new_pad = 0;
    }
    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, max_options, 0);
        new_pad = 0;
    }
    else if(new_pad & BUTTON_SQUARE)
    {
        frame_count = 32;
        opt_filter_by_device = LIST_ALL_DEVICES;
        retro_mode = RETRO_ALL;
        filter_by_letter = 0;
    }
    else if((new_pad & BUTTON_L1) || ((frame > 10) && (old_pad & BUTTON_L1)))
    {
        frame = 0;
        ROT_DEC(filter_by_letter, 0, 43);
        if(filter_by_letter == 17) filter_by_letter = 10;
        if(filter_by_letter < 0) filter_by_letter = 43;
    }
    else if((new_pad & BUTTON_R1) || ((frame > 10) && (old_pad & BUTTON_R1)))
    {
        frame = 0;
        if(filter_by_letter == 0) filter_by_letter = 17;
        ROT_INC(filter_by_letter, 43, 0);
        if(filter_by_letter == 11) filter_by_letter = 18;
    }
    else if(select_option == 0)
    {
        if(new_pad & BUTTON_LEFT)
        {
            frame_count = 32;

            bool dev_exist = false;

            while(!dev_exist)
            {
                ROT_DEC(opt_filter_by_device, LIST_ALL_DEVICES, 19);

                if(opt_filter_by_device == LIST_ALL_DEVICES) break;

                if(opt_filter_by_device == HDD0_DEVICE)
                    sprintf(tmp_path, "/dev_hdd0");
                else if(opt_filter_by_device == BDVD_DEVICE)
                    sprintf(tmp_path, "/dev_bdvd");
                else if(opt_filter_by_device > HDD0_DEVICE && opt_filter_by_device < BDVD_DEVICE)
                    sprintf(tmp_path, "/dev_usb00%c", 47 + opt_filter_by_device);
                else if(mounts[opt_filter_by_device - 12])
                    sprintf(tmp_path, "/ntfs%c", 36 + opt_filter_by_device);
                else
                    continue;

                dev_exist = is_ntfs_path(tmp_path) || file_exists(tmp_path);
            }

        }
        else if(new_pad & BUTTON_RIGHT)
        {
            frame_count = 32;

            bool dev_exist = false;

            while(!dev_exist)
            {
                ROT_INC(opt_filter_by_device, 19, LIST_ALL_DEVICES);

                if(opt_filter_by_device == LIST_ALL_DEVICES) break;

                if(opt_filter_by_device == HDD0_DEVICE)
                    sprintf(tmp_path, "/dev_hdd0");
                else if(opt_filter_by_device == BDVD_DEVICE)
                    sprintf(tmp_path, "/dev_bdvd");
                else if(opt_filter_by_device > HDD0_DEVICE && opt_filter_by_device < BDVD_DEVICE)
                    sprintf(tmp_path, "/dev_usb00%c", 47 + opt_filter_by_device);
                else if(mounts[opt_filter_by_device - 12])
                    sprintf(tmp_path, "/ntfs%c", 36 + opt_filter_by_device);
                else
                    continue;

                dev_exist = is_ntfs_path(tmp_path) || file_exists(tmp_path);
            }
        }
    }
    else if(select_option == max_options)
    {
        if(new_pad & BUTTON_LEFT)
        {
            frame_count = 32;
            ROT_DEC(retro_mode, 0, 22);

            if(is_mamba_v2 && (retro_mode == RETRO_PS2 || retro_mode == RETRO_PSP)) retro_mode = RETRO_PSX;
        }
        else if(new_pad & BUTTON_RIGHT)
        {
            frame_count = 32;
            ROT_INC(retro_mode, 22, 0);

            if(is_mamba_v2 && (retro_mode == RETRO_PS2 || retro_mode == RETRO_PSP)) retro_mode = RETRO_SNES;
        }
    }
}

void draw_cache_external()
{
    int menu = menu_screen;

    LoadCacheDatas();

    if(ncache_list > 0)
    {
        menu_screen = SCR_MENU_TOOLS;
        select_option = 0;
    }
    else return;

    while(menu_screen != 0)
    {
        flash = (frame_count >> 5) & 1;

        frame_count++;
        cls();

        update_twat(true);
        menu_screen = 5;
        draw_cachesel(28, 0);
    }

    menu_screen = menu;
}

void draw_cachesel(float x, float y)
{

    int n;

    float y2, x2;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWCACHE_CACHE]);

    x2 = DrawFormatString(2000, 0, "hdd0: %.2fGB ", freeSpace[0]);
    x2 = 848 -(x2 - 2000) - x;
    DrawFormatString(x2, 0, "hdd0: %.2fGB ", freeSpace[0]);


    y += 24;

    DrawBox(x, y, 0, box_width, box_height, BLACK);


    x2 = x;
    y2 = y + 32;

    for(n = (select_option / 8) * 8; (n < (select_option / 8) * 8 + 8); n++)
    {
        if(n < ncache_list)
        {
            sprintf(temp_buffer, "%s (%1.2f GB)", cache_list[n].title_id, ((double) cache_list[n].size) / GIGABYTES);
            DrawButton1_UTF8(box_left, y2, 520, temp_buffer, (flash && select_option == n));
        }
        else
            DrawButton1_UTF8(box_left, y2, 520, "", -1);

        y2+= 48;
    }

    SetCurrentFont(FONT_TTF);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);


    if(flash && cache_need_free != 0)
    {
        SetFontSize(20, 20);
        SetFontColor(0xffff00ff, BLACK);
        SetFontAutoCenter(1);
        DrawFormatString(0, y + 3 * 150 + 6, language[DRAWCACHE_ERRNEEDIT], cache_need_free);
        SetFontAutoCenter(0);
    }
    else if(select_option < ncache_list)
    {
        SetFontColor(WHITE, BLACK);

        utf8_truncate(cache_list[select_option].title, temp_buffer, 65);

        temp_buffer[65] = 0;

        if(strlen(temp_buffer) < 50)
            SetFontSize(18, 32);
        else
            SetFontSize(14, 32);

        SetFontAutoCenter(1);

        DrawFormatString(0, y + 3 * 150, temp_buffer);

        SetFontAutoCenter(0);
    }

    SetFontColor(WHITE, BLACK);


    tiny3d_Flip();
    ps3pad_read();

    if(new_pad & (BUTTON_CROSS_))
    {
        if(select_option >= ncache_list) return;

        sprintf(temp_buffer, language[DRAWCACHE_ASKTODEL], cache_list[select_option].title_id);

        if(DrawDialogYesNo(temp_buffer) == YES)
        {
            sprintf(tmp_path, "%s/cache/%s", self_path, cache_list[select_option].title_id);
            DeleteDirectory(tmp_path);
            rmdir_secure(tmp_path);
            LoadCacheDatas();

            if(ncache_list >= select_option) select_option = ncache_list - 1;

            if(ncache_list <= 0)
                return_to_game_list(false);
        }
        new_pad = 0;
        return;
    }
    else if(new_pad & BUTTON_TRIANGLE)
    {
        return_to_game_list(false);
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, ncache_list - 1);
    }
    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, ncache_list - 1, 0);
    }

}
#endif

/******************************************************************************************************************************************************/
/* BDVDEMU FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

void unpatch_bdvdemu()
{
//LV2 Mount for 355 in his payload code

#ifdef PSDEBUG
    int flag = 0;
    flag =
#endif
    lv2_unpatch_bdvdemu();
}


int patch_bdvdemu(u32 flags)
{
    int n;
    int usb = DISABLED;

    static int one = 0;

    if(one) return SUCCESS; // only one time


    if(((flags & 1) && game_cfg.bdemu     == 2) ||
      (!(flags & 1) && game_cfg.bdemu_ext == 2) || (flags & D_FLAG_BDVD) )
    {
        if(noBDVD == MODE_DISCLESS && !use_cobra) flags = NTFS_FLAG; else return SUCCESS; // new to add BD-Emu 2
    }

    one = 1;

    if(noBDVD && !use_cobra && (flags & NTFS_FLAG)) flags = 1;

    flags &= GAMELIST_FILTER;

    for(n = 1; n < BDVD_DEVICE; n++)
    {
        if(flags == (1 << n)) {usb = n - 1; break;}
    }

    if(usb == DISABLED) bdvd_is_usb = false; else bdvd_is_usb = true;


    int save = noBDVD;

    if(use_cobra) noBDVD = 0;

    lv2_patch_bdvdemu(flags);

    noBDVD = save;

    return SUCCESS;
}

#ifndef LOADER_MODE
int move_origin_to_bdemubackup(char *path)
{
    if(strncmp(path, "/dev_usb0", 9) && strncmp(path, "/dev_hdd0", 9)) return 1;

    sprintf(temp_buffer, "%s/PS3_GAME/PS3PATH.BUP", path);
    sprintf(temp_buffer + 1024, "%s/PS3_GAME", path);

    if(SaveFile(temp_buffer, temp_buffer + 1024, strlen(temp_buffer + 1024)) != SUCCESS)
    {
        sprintf(temp_buffer + 1024, language[MOVEOBEMU_ERRSAVE], temp_buffer);
        DrawDialogOK(temp_buffer + 1024);

        return FAILED;
    }

    if(!strncmp(temp_buffer, "/dev_hdd0", 9)) strncpy(temp_buffer + 9, "/PS3_GAME", 16) ;
    else
        strncpy(temp_buffer + 11, "/PS3_GAME", 16);

    // sys8_perm_mode(1);
    int n = sysLv2FsRename(temp_buffer  + 1024, temp_buffer);
    // sys8_perm_mode(0);

    if(n != 0)
    {
        sprintf(temp_buffer + 256, language[MOVEOBEMU_ERRMOVE], temp_buffer);
        DrawDialogOK(temp_buffer + 256);

        return FAILED;
    }

    // PS3_GM01

    sprintf(temp_buffer, "%s/PS3_GM01/PS3PATH2.BUP", path);
    sprintf(temp_buffer + 1024, "%s/PS3_GM01", path);

    if(!file_exists(temp_buffer + 1024))
    {
        sprintf(temp_buffer, "%s/PS3_GAME/PS3PATH.BUP", path);

        if(!strncmp(temp_buffer, "/dev_hdd0", 9)) strncpy(temp_buffer + 9, "/PS3_GAME", 16);
        else
            strncpy(temp_buffer + 11, "/PS3_GAME", 16);

        goto skip1;
    }

    if(SaveFile(temp_buffer, temp_buffer + 1024, strlen(temp_buffer + 1024)) != SUCCESS)
    {
        sprintf(temp_buffer + 1024, language[MOVEOBEMU_ERRSAVE], temp_buffer);
        DrawDialogOK(temp_buffer + 1024);

        return FAILED;
    }

    if(!strncmp(temp_buffer, "/dev_hdd0", 9)) strncpy(temp_buffer + 9, "/PS3_GM01", 16);
        else
            strncpy(temp_buffer + 11, "/PS3_GM01", 16);

    // sys8_perm_mode(1);
    n = sysLv2FsRename(temp_buffer  + 1024, temp_buffer);
    // sys8_perm_mode(0);

    if(n != SUCCESS)
    {
        sprintf(temp_buffer + 256, language[MOVEOBEMU_ERRMOVE], temp_buffer);
        DrawDialogOK(temp_buffer + 256);

        return FAILED;
     }

 skip1:

    if(autolaunch < LAUNCHMODE_STARTED && bSkipBDMirrorMsg == false)
    {
        sprintf(temp_buffer + 256, language[MOVEOBEMU_MOUNTOK], temp_buffer);
        DrawDialogOKTimer(temp_buffer + 256, 1200.0f);
    }

    return SUCCESS;
}
#endif

int move_bdemubackup_to_origin(u32 flags)
{
    int n;
    int usb = -1;

    static u32 olderrflags = 0;
    static u32 olderrflags2 = 0;

    flags &= GAMELIST_FILTER;

    for(n = HDD0_DEVICE; n < BDVD_DEVICE; n++)
    {
        if(flags == (1 << n)) {usb = n - 1; break;}
    }

    if(usb < 0)
    {
        sprintf(temp_buffer, "/dev_hdd0/PS3_GAME");
        sprintf(temp_buffer + 256, "/dev_hdd0/PS3_GAME/PS3PATH.BUP");
    }
    else
    {
        sprintf(temp_buffer, "/dev_usb00%c/PS3_GAME", 48 + usb);
        sprintf(temp_buffer + 256, "/dev_usb00%c/PS3_GAME/PS3PATH.BUP", 48 + usb);
    }

    int file_size;
    char *file;
    int ret = SUCCESS;

    if(!file_exists(temp_buffer)) {ret = FAILED; goto PS3_GM01;}

    file = LoadFile(temp_buffer + 256, &file_size);

    if(!file) {ret = FAILED; goto PS3_GM01;}

    memset(temp_buffer + 1024, 0, 0x420);

    if(file_size > 0x400) file_size = 0x400;

    memcpy(temp_buffer + 1024, file, file_size);

    free(file);

    for(n = 0; n < 0x400; n++)
    {
        if(temp_buffer[1024 + n] == 0) break;
        if(((u8)temp_buffer[1024 + n]) < 32) {temp_buffer[1024 + n] = 0; break;}
    }

    if(strncmp(temp_buffer, temp_buffer + 1024, 10))  {ret = FAILED;goto PS3_GM01;} // if not /dev_usb00x return

    if(!strncmp(temp_buffer, "/dev_hdd0", 9)) ;
    else
        memcpy(temp_buffer + 1024, temp_buffer, 11);

    // sys8_perm_mode(1);
    n = sysLv2FsRename(temp_buffer, temp_buffer + 1024);
    // sys8_perm_mode(0);

    if(n != SUCCESS)
    {
        if(!(olderrflags & flags))
        {
            sprintf(temp_buffer, language[MOVETBEMU_ERRMOVE], temp_buffer + 1024);
            DrawDialogOK(temp_buffer);
            olderrflags |= flags;
        }
        ret = FAILED; goto PS3_GM01;
    }

    // PS3_GM01

    PS3_GM01:

    if(usb < 0)
    {
        sprintf(temp_buffer, "/dev_hdd0/PS3_GM01");
        sprintf(temp_buffer + 256, "/dev_hdd0/PS3_GM01/PS3PATH2.BUP");
    }
    else
    {
        sprintf(temp_buffer, "/dev_usb00%c/PS3_GM01", 48 + usb);
        sprintf(temp_buffer + 256, "/dev_usb00%c/PS3_GM01/PS3PATH2.BUP", 48 + usb);
    }

    if(!file_exists(temp_buffer)) return FAILED;

    file = LoadFile(temp_buffer + 256, &file_size);

    if(!file) return FAILED;

    memset(temp_buffer + 1024, 0, 0x420);

    if(file_size > 0x400) file_size = 0x400;

    memcpy(temp_buffer + 1024, file, file_size);

    free(file);

    for(n = 0; n< 0x400; n++)
    {
        if(temp_buffer[1024 + n] == 0) break;
        if(((u8)temp_buffer[1024 + n]) < 32) {temp_buffer[1024 + n] = 0; break;}
    }

    if(strncmp(temp_buffer, temp_buffer + 1024, 10)) return FAILED; // if not /dev_usb00x return

    if(!strncmp(temp_buffer, "/dev_hdd0", 9)) ;
    else
        memcpy(temp_buffer + 1024, temp_buffer, 11);

    // sys8_perm_mode(1);
    n = sysLv2FsRename(temp_buffer, temp_buffer + 1024);
    // sys8_perm_mode(0);

    if(n != SUCCESS)
    {
        if(!(olderrflags2 & flags))
        {
            sprintf(temp_buffer, language[MOVETBEMU_ERRMOVE], temp_buffer + 1024);
            DrawDialogOK(temp_buffer);
            olderrflags2 |= flags;
        }
        return FAILED;
    }

    return ret;
}

#ifndef LOADER_MODE
//------- idps / console id ----------
void draw_console_id_tools(float x, float y)
{
    float y2, x2;
    int r;

    SetCurrentFont(FONT_TTF);
    get_console_id_lv2();

    // header title

    DrawBox(x, y, 0, box_width, 20, BLACK);

    SetFontColor(WHITE, BLACK);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", "Spoof console id");

    y += 24;

    DrawBox(x, y, 0, box_width, box_height, BLACK);

    x2 = x;
    y2 = y + 32;

    DrawButton1_UTF8(box_left, y2, 520, "Set new PSID", (flash && (select_option == 0)));

    y2+= 48;

    DrawButton1_UTF8(box_left, y2, 520, "Set new console id", (flash && (select_option == 1)));

    y2+= 48;

    DrawButton1_UTF8(box_left, y2, 520, "Load PSID & console id", (flash && (select_option == 2)));

    y2+= 48;

    DrawButton1_UTF8(box_left, y2, 520, "Restore default PSID & console id", (flash && (select_option == 3)));


    for(int n = 0; n < 5; n++) y2+= 48;

    DrawBox(x, y + 3 * 150, 0, box_width, 40, BLACK);

    // draw sys version
    SetCurrentFont(FONT_TTF);

    SetFontColor(0xccccffff, BLACK);
    SetFontSize(18, 20);
    SetFontAutoCenter(1);

    DrawFormatString(0, y2 - 28, "%x%s  -  %s", ((firmware>>4) & 0xFFFF), ((firmware & 0xF) == 0xE ? "DEH": (firmware & 0xF) == 0xD ? "DEX": "CEX"), payload_str);


    SetFontAutoCenter(0);

    //Display Console id
    SetCurrentFont(FONT_TTF);

    SetFontColor(0xeeffaaff, BLACK);
    SetFontSize(14, 16);
    SetFontAutoCenter(1);
    get_console_id_lv2();
    DrawFormatString(500, y2 - 100, "Console id: %s", console_id );
    SetFontAutoCenter(0);

    if(strncmp(console_id, psid, 32))
    {
        SetFontAutoCenter(1);
        get_psid_lv2();
        DrawFormatString(500, y2 - 132, "PSID: %s", psid );
    }

    tiny3d_Flip();

    ps3pad_read();

    if(new_pad & BUTTON_CROSS_)
    {
        switch(select_option)
        {
            case 0: //set new psid
                if( get_psid_keyb() == SUCCESS )
                {
                    set_psid_lv2();
                    save_spoofed_psid();
                    sprintf(temp_buffer, "PSID has been set to:\n\n%s", psid);
                    DrawDialogOKTimer(temp_buffer, 3000.0f);
                }
                break;

            case 1: //set new console id
                if( get_console_id_keyb() == SUCCESS )
                {
                    set_console_id_lv2();
                    save_spoofed_console_id();
                    sprintf(temp_buffer, "Console ID has been set to:\n\n%s", console_id);
                    DrawDialogOKTimer(temp_buffer, 3000.0f);
                }
                break;

            case 2: //load console id
                r = load_spoofed_console_id();
                if(r == SUCCESS)
                {
                    sprintf(temp_buffer, "Console ID has been set to:\n\n%s", console_id);
                    DrawDialogOKTimer(temp_buffer, 3000.0f);
                }
                else if(r == 1)
                {
                    sprintf(temp_buffer, "Console ID is already set:\n\n%s", console_id);
                    DrawDialogOKTimer(temp_buffer, 3000.0f);
                }
                else
                {
                    DrawDialogOKTimer("idps file was not found in config folder.", 3000.0f);
                }

                r = load_spoofed_psid();

                if(r == SUCCESS)
                {
                    sprintf(temp_buffer, "PSID has been set to:\n\n%s", psid);
                    DrawDialogOKTimer(temp_buffer, 3000.0f);
                }
                else if(r == 1)
                {
                    sprintf(temp_buffer, "PSID is already set:\n\n%s", psid);
                    DrawDialogOKTimer(temp_buffer, 3000.0f);
                }
                else
                {
                    DrawDialogOKTimer("psid file was not found in config folder.", 3000.0f);
                }

                break;

            case 3: //restore default PSID
                sprintf(tmp_path, "%s/config/psid", self_path);
                unlink_secure(tmp_path);

                sprintf(tmp_path, "%s/config/idps", self_path);
                unlink_secure(tmp_path);

                if( strcmp(psid, default_psid) == SUCCESS && strcmp(console_id, default_console_id) == SUCCESS)
                {
                    sprintf(temp_buffer, "PSID & Console ID are already the default.");
                    DrawDialogOKTimer(temp_buffer, 3000.0f);
                    break; //default already set
                }

                strcpy( psid, default_psid );
                set_psid_lv2();

                strcpy( console_id, default_console_id );
                set_console_id_lv2();

                sprintf(temp_buffer, "The default PSID & Console ID have been restored.");
                DrawDialogOKTimer(temp_buffer, 3000.0f);
                break;

            default:
               break;
        }
    }

    if(new_pad & (BUTTON_TRIANGLE | BUTTON_CIRCLE_ | BUTTON_LEFT))
    {
        if(new_pad & BUTTON_LEFT)
        {
            menu_screen = SCR_MENU_TOOLS;
            select_option = 6;
        }
        else
            return_to_game_list(false);
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        frame_count = 32;
        ROT_DEC(select_option, 0, 3);
    }
    else if(new_pad & BUTTON_DOWN)
    {
        frame_count = 32;
        ROT_INC(select_option, 3, 0);
    }

}
#endif
//------------------------------------
