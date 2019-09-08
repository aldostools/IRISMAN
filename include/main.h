#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

typedef struct PngDatas {

	void * png_in;		// ignored except if char *filename == NULL in LoadPNG()
	uint32_t png_size;  // ignored except if char *filename == NULL  in LoadPNG()

	void * bmp_out;		// internally allocated (bmp 32 bits color ARGB format)

	int	wpitch;			// output width pitch in bytes
	int width;			// output
	int height;			// output

} PngDatas;

typedef struct JpgDatas {

	void * jpg_in;		// ignored except if char *filename == NULL in LoadPNG()
	uint32_t jpg_size;  // ignored except if char *filename == NULL  in LoadPNG()

	void * bmp_out;		// internally allocated (bmp 32 bits color ARGB format)

	int	wpitch;			// output width pitch in bytes
	int width;			// output
	int height;			// output

} JpgDatas;

int LoadPNG(PngDatas *png, const char *filename);
int LoadJPG(JpgDatas *jpg, char *filename);


// manager config options
#define OPTFLAGS_FTP                    (1 << 0)
#define OPTFLAGS_PLAYMUSIC              (1 << 1)


#define AUTO_BUTTON_REP(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 10) {v = 5; new_pad |= b;} \
                                 } else v = 1;

#define AUTO_BUTTON_REP3(v, b) if(v && (old_pad & b)) { \
                                 v++; \
                                 if(v > 10) {v = 10; new_pad |= b;} \
                                 } else v = 1;

enum screens_types
{
    SCR_MAIN_GAME_LIST = 0,
    SCR_MENU_GAME_OPTIONS = 1,
    SCR_MENU_GAME_OPTIONS_CONFIG = 2,
    SCR_MENU_ISO_OPTIONS = 128,
    SCR_MENU_GLOBAL_OPTIONS = 3,
    SCR_MENU_TOOLS = 4,
    SCR_MENU_GAME_LIST = 123,
    SCR_MENU_CONSOLE_ID_TOOLS = 222,
    SCR_MENU_PSX_OPTIONS = 444,
    SCR_MENU_PSX_VIDEO_OPTIONS = 445,
    SCR_TOOL_DELETE_CACHE = 5,
    SCR_TOOL_BUILD_ISO = 777,
    SCR_TOOL_EXTRACT_ISO = 778,
    SCR_TOOL_COPY_ISO = 779,
};

enum ResourceImages
{
    IMG_DEFAULT_BACKGROUND = 16,
    IMG_FTP_ICON = 4,
    IMG_MISSING_ICON = 2,
    IMG_DIRECT_ICON = 3,

    IMG_PS1_DISC = 5,
    IMG_PS1_ISO = 6,
    IMG_PS2_ISO = 13,
    IMG_PSP_ISO = 17,
    IMG_RETRO_ICON = 18,
    IMG_BLURAY_DISC = 0,
    IMG_DVD_DISC = 15,
    IMG_MOVIE_ICON = 20,
    IMG_USB_ICON = 1,
    IMG_USB_ICON2 = 14,
    IMG_NETHOST = 19,

    // File Manager Icons
    IMG_FOLDER_ICON = 7,
    IMG_FILE_ICON = 8,
    IMG_PKG_ICON = 9,
    IMG_SELF_ICON = 10,
    IMG_IMAGE_ICON = 11,
    IMG_ISO_ICON = 12,
};

enum RetroModes
{
    RETRO_ALL   = 0,
    RETRO_PSX   = 1,
    RETRO_PS2   = 2,
    RETRO_PSP   = 3,
    RETRO_SNES  = 4,
    RETRO_GBA   = 5,
    RETRO_GEN   = 6,
    RETRO_NES   = 7,
    RETRO_MAME  = 8,
    RETRO_FBA   = 9,
    RETRO_QUAKE = 10,
    RETRO_DOOM  = 11,
    RETRO_PCE   = 12,
    RETRO_GBC   = 13,
    RETRO_VBOY  = 14,
    RETRO_NXE   = 15,
    RETRO_ATARI = 16,
    RETRO_WSWAN = 17,
    RETRO_A7800 = 18,
    RETRO_LYNX  = 19,
    RETRO_GW    = 20,
    RETRO_VECTX = 21,
    RETRO_2048  = 22,
    RETRO_PSALL = 23,
    NET_GAMES   = 24,

};

enum game_list_categories
{
    GAME_LIST_ALL      = 0,
    GAME_LIST_PS3_ONLY = 1,  // PS3 / Movies
    GAME_LIST_RETRO    = 2,  // Homebrews / Retro / NET
    GAME_LIST_HOMEBREW = 2,
    GAME_LIST_NETHOST  = 2,
};

void load_gamecfg (int current_dir);
void read_settings();

// FLAGS
#define HDD0_FLAG         (1)
#define BDVD_FLAG         (1<<11)    //2048
#define NTFS_FLAG         (1<<15)
#define USB_FLAG          (0x3ff<<1) //2046

#define PS3_FOLDER_FLAG   0
#define HOMEBREW_FLAG     (1<<31)

#define ISO_FLAGS         ((1<<23) | (1<<24))
#define GAME_FLAGS        ((1<<23) | (1<<24) | (1<<25))

#define PS1_FLAG          (1<<23)
#define PS2_FLAG          ISO_FLAGS
#define PS3_FLAG          (1<<24)
#define PSP_FLAG          GAME_FLAGS
#define RETRO_FLAG        GAME_FLAGS
#define PS2_CLASSIC_FLAG  GAME_FLAGS
#define IS_PS2_FLAG       (1<<24)

#define PS1_BIT        23

#define HDD0_DEVICE    0
#define BDVD_DEVICE    11

// HOMEBREW / BDISO / DVDISO / MKV
#define D_FLAG_HOMEB_DPL   (31)
#define D_FLAG_HOMEB       (1<<31)
#define D_FLAG_HOMEB_BD    (1<<24)
#define D_FLAG_HOMEB_DVD   (1<<23)
#define D_FLAG_HOMEB_MKV   (D_FLAG_HOMEB_BD | D_FLAG_HOMEB_DVD)
#define D_FLAG_HOMEB_GROUP (D_FLAG_HOMEB | D_FLAG_HOMEB_MKV)

// DEVICES
#define D_FLAG_HDD0 1
#define D_FLAG_BDVD (1<<11)   //2048
#define D_FLAG_NTFS (1<<15)
#define D_FLAG_USB (0x3ff<<1) //2046

// TYPES
#define D_FLAG_PS3_ISO  (1<<24)
#define D_FLAG_PS2_ISO  (D_FLAG_PS3_ISO | (1<<23))
#define D_FLAG_PSX_ISO  (1<<23) // also can be used to identify PS2 game!
#define D_FLAG_MASK_ISO (D_FLAG_PS3_ISO | D_FLAG_HOMEB | D_FLAG_BDVD)

/*

EXAMPLES:

PS3 GAME from BDVD: D_FLAG_BDVD
PS3 GAME from HDD0 (jailbreak) : D_FLAG_HDD0
PS3 GAME from USBx (jailbreak) : (1 << (x+1))

PS3 GAME from HDD0 (ISO) : D_FLAG_PS3_ISO | D_FLAG_HDD0
PS3 GAME from USBx (ISO) : D_FLAG_PS3_ISO | (1 << (x+1))
PS3 GAME from NTFS (ISO) : D_FLAG_PS3_ISO | D_FLAG_NTFS (from USB device with NTFS/EXTx partition)

PS2 GAME from HDD0 (ISO) : D_FLAG_PS2_ISO | D_FLAG_HDD0 (only supported in CFW 4.46 Cobra)

PS1 GAME from BDVD : D_FLAG_PSX_ISO | D_FLAG_BDVD
PS1 GAME from HDD0 (ISO) : D_FLAG_PSX_ISO | D_FLAG_HDD0
PS1 GAME from USBx (ISO) : D_FLAG_PSX_ISO | (1 << (x+1))
PS1 GAME from NTFS (ISO) : D_FLAG_PSX_ISO | D_FLAG_NTFS (from USB device with NTFS/EXTx partition)

Homebrew Mode:

PS3 Homebrew/PS3 : D_FLAG_HOMEB_DPL | (1 << (x+1)) (from USBx depacked in path /dev_usb00x/game/xxxxx)

BD ISO   : D_FLAG_HOMEB_DPL | D_FLAG_HOMEB_BD  | x (from /BDISO  , x = D_FLAG_HDD0 or D_FLAG_NTFS or (1 << (usb_port+1)))
DVD ISO  : D_FLAG_HOMEB_DPL | D_FLAG_HOMEB_DVD | x (from /BDVDISO, x = D_FLAG_HDD0 or D_FLAG_NTFS or (1 << (usb_port+1)))
MKV file : D_FLAG_HOMEB_DPL | D_FLAG_HOMEB_MKV | x (from /MKV    , x = D_FLAG_HDD0 or D_FLAG_NTFS or (1 << (usb_port+1)))

*/

#define SUCCESS 0
#define FAILED -1

#define DISABLED -1

#define YES 1

#define REFRESH_GAME_LIST    1

#define NTFS_DEVICE_MOUNT    1
#define NTFS_DEVICE_UNMOUNT -1

#define MODE_WITHBDVD    0
#define MODE_NOBDVD      1
#define MODE_DISCLESS    2

#define MM_PATH                  "/dev_hdd0/game/BLES80608"
#define PS2_CLASSIC_PLACEHOLDER  "/dev_hdd0/game/PS2U10000/USRDIR"
#define PS2_CLASSIC_ISO_PATH     "/dev_hdd0/game/PS2U10000/USRDIR/ISO.BIN.ENC"
#define PS2_CLASSIC_ISO_ICON     "/dev_hdd0/game/PS2U10000/ICON0.PNG"

#define BIG_PICT  48
#define BACKGROUND_PICT  (BIG_PICT + 1)
#define MAX_PICTURES     (BIG_PICT + 2)
#define MAX_RESOURCES    24

#define ROMS_MAXPATHLEN  64

#define GIGABYTES 1073741824.0

extern u16 BUTTON_CROSS_;
extern u16 BUTTON_CIRCLE_;

extern int scr_grid_games;
extern int scr_grid_w;
extern int scr_grid_h;

extern u8 * png_texture;
extern PngDatas Png_datas[MAX_PICTURES];
extern u32 Png_offset[MAX_PICTURES];
extern int Png_iscover[MAX_PICTURES];

extern PngDatas Png_res[MAX_RESOURCES];
extern u32 Png_res_offset[MAX_RESOURCES];

void SaveGameList();
int get_net_status();
void return_to_game_list(bool update);

u64 lv2peek(u64 addr);
u64 lv2poke(u64 addr, u64 value);
#endif

