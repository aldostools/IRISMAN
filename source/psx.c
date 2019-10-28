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

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <lv2/process.h>
#include <lv2/sysfs.h>
#include <ppu-lv2.h>
#include <sys/stat.h>
#include <lv2/sysfs.h>

#include <sysmodule/sysmodule.h>
#include <pngdec/pngdec.h>
#include <jpgdec/jpgdec.h>

#include <io/pad.h>
#include "osk_input.h"

#include <tiny3d.h>
#include "libfont2.h"
#include "language.h"
#include "syscall8.h"
#include "payload.h"

#include "main.h"
#include "psx.h"
#include "gfx.h"
#include "utils.h"
#include "storage.h"

#include "psx_storage_bin.h"
#include "syscall8.h"

#include "ttf_render.h"
#include "controlfan.h"

#include "cobra.h"
#include "ntfs.h"
#include "modules.h"

int NTFS_Test_Device(char *name);

static u8 region_psx = 0;

#define EMU_PSX_MULTI (EMU_PSX + 16)

#define GIGABYTES 1073741824.0

//int cobra_mount_psx_disc_image(char *file, TrackDef *tracks, unsigned int num_tracks);

typedef struct
{
  uint64_t device;
    uint32_t emu_mode;
    uint32_t num_sections;
    uint32_t num_tracks;
    // sections after
    // sizes after
    // tracks after
} __attribute__((packed)) rawseciso_args;

typedef struct
{
  uint64_t device;
    uint32_t emu_mode;         //   16 bits    16 bits    32 bits
    uint32_t discs_desc[8][2]; //    parts  |   offset | toc_filesize (LBA format)   -> if parts == 0  use files
} __attribute__((packed)) psxseciso_args;


extern u16 * ttf_texture;
extern bool update_title_utf8;
extern u8 string_title_utf8[128];
extern int width_title_utf8;
extern u8 bShowPIC1;

bool test_ftp_working();

#define FS_S_IFMT 0170000

void DrawDialogOKTimer(char * str, float milliseconds);

//int syscall36(char * path);
int sys_set_leds(u64 color, u64 state);
u64 lv2peek(u64 addr);
//u64 lv2poke(u64 addr, u64 value);

extern int num_box;
extern int cols;
extern int noBDVD;

extern u64 syscall_base;

void load_psx_payload();

int n_psx_vm1 = 0;
char *psx_vm1= NULL;
int sel_psx_vm1 = 0;
int sel2_psx_vm1 = 0;

char *PSX_LAST_PATH = NULL;

extern char temp_buffer[8192];
extern char tmp_path[MAXPATHLEN];

extern int flash;

extern int select_px;
extern int select_py;
extern int select_option;
extern u64 frame_count;

extern int menu_screen;
extern int mode_favourites;

extern t_directories directories[MAX_DIRECTORIES];

extern int ndirectories;

extern int currentdir;
extern int currentgamedir;

extern int mode_homebrew;

extern char * language[];
extern char self_path[MAXPATHLEN];
extern char path_name[MAXPATHLEN];

extern u32 default_psxoptions;

// no inline for psx.c
int get_icon(char * path, const int num_dir);
void copy_PSX_game_from_CD();

void get_games();

void pause_music(int pause);

int LoadTexturePNG(char * filename, int index);
int LoadTextureJPG(char * filename, int index);

int copy_async_gbl(char *path1, char *path2, u64 size, char *progress_string1, char *progress_string2); // updates.c (it can copy to ntfs devices)

void fun_exit();

int build_MC(char *path);

int get_psx_memcards(void)
{
    DIR  *dir;

    n_psx_vm1 = sel_psx_vm1 = sel2_psx_vm1 =0;
    psx_vm1 = malloc(256 * 16);
    if(psx_vm1)
    {
        strncpy(psx_vm1, "No Memory Card", 256); n_psx_vm1++;

        dir = opendir("/dev_hdd0/savedata/vmc");
        if(dir)
        {
            struct dirent *entry;

            while((entry = readdir(dir)) != NULL)
            {
                if(entry->d_type & DT_DIR) continue;

                if(strlen(entry->d_name) < 4 || strcmp(entry->d_name + strlen(entry->d_name) - 4, ".VM1")) continue;

                strncpy(&psx_vm1[n_psx_vm1<<8], entry->d_name, 256);

                n_psx_vm1++; if(n_psx_vm1 > 15) break;
            }
            closedir(dir);
        }

        return SUCCESS;
    }

    return FAILED;
}

static bool psx_modified = false;

psx_opt psx_options;

void draw_psx_options(float x, float y, int index)
{
    int i, n;

    float y2, x2;
    int selected;

    selected = select_px + select_py * cols;

    char *mc_name = NULL;
    int is_ntfs = 0;

    if(!strncmp(directories[currentgamedir].path_name, "/ntfs", 5)  || !strncmp(directories[currentgamedir].path_name, "/ext", 4))
    {
        is_ntfs = 1;
        if((psx_options.flags & PSX_PAYLOAD) == PSX_PAYLOAD) psx_options.flags ^= PSX_PAYLOAD;
    }

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    DrawFormatString(x, y, " PSX %s", language[DRAWGMOPT_OPTS]);

    utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 12);

    if(temp_buffer[4] != '_')
    {
        int flen = strlen(directories[currentgamedir].path_name) - 4;

        if(flen >= 0 && strcasestr(".iso|.bin|.mdf|.img", directories[currentgamedir].path_name + flen) != NULL)
        {
           parse_iso_titleid(directories[currentgamedir].path_name, temp_buffer);
           temp_buffer[12] = 0;
           strncpy(directories[currentgamedir].title_id, temp_buffer, 12);
        }
        else
           memset(temp_buffer, 12, 0);
    }

    if(!(directories[currentgamedir].flags & BDVD_FLAG) &&
        ((select_option == 0 && strcmp(psx_options.mc1, "No Memory Card") && strcmp(psx_options.mc1, "Internal_MC.VM1")) ||
         (select_option == 1 && strcmp(psx_options.mc2, "No Memory Card") && strcmp(psx_options.mc2, "Internal_MC.VM1"))))
    {
        SetFontSize(18, 22);
        utf8_truncate("Press [] to copy MC as Internal_MC", temp_buffer, 64);

        x2 = DrawFormatString(1024, y, "%s", temp_buffer);
        DrawFormatString(848 - x2 + 1024 - x, y, "%s", temp_buffer);

        if(select_option == 0) mc_name = psx_options.mc1; else mc_name = psx_options.mc2;
    }
    else
    {
        mc_name = NULL;

        if(temp_buffer[4] == '_')
        {
            sprintf(temp_buffer, "%c%c%c%c-%c%c%c%c%c", temp_buffer[0], temp_buffer[1], temp_buffer[2], temp_buffer[3],
                                                        temp_buffer[5], temp_buffer[6], temp_buffer[7], temp_buffer[9], temp_buffer[10]);

            DrawFormatString(848 - x - strlen(temp_buffer) * 8 - 60, y, temp_buffer);
        }
    }

    SetCurrentFont(FONT_TTF);

    SetFontSize(16, 20);


    y += 24;

    if(bShowPIC1)
    {
        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DEFAULT_BACKGROUND], Png_res[IMG_DEFAULT_BACKGROUND].width,
                        Png_res[IMG_DEFAULT_BACKGROUND].height, Png_res[IMG_DEFAULT_BACKGROUND].wpitch,
                            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
    }

    DrawTextBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0xffffffff);

    DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

    x2 = x;
    y2 = y + 8;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, "MC Slot 1 ", (flash && select_option == 0)) + 8;

    if(!(directories[currentgamedir].flags & BDVD_FLAG) && !strcmp(psx_options.mc1, "No Memory Card")) strncpy(psx_options.mc1, "Internal_MC.VM1", 256);
    if((directories[currentgamedir].flags & BDVD_FLAG) && !strcmp(psx_options.mc1, "Internal_MC.VM1")) strncpy(psx_options.mc1, "No Memory Card", 256);

    utf8_truncate(psx_options.mc1, temp_buffer + 1024, 32);
    sprintf(temp_buffer, " %s ", temp_buffer + 1024);
    x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, "MC Slot 2 ", (flash && select_option == 1)) + 8;
    utf8_truncate(psx_options.mc2, temp_buffer + 1024, 32);
    sprintf(temp_buffer, " %s ", temp_buffer + 1024);
    x2 = DrawButton2_UTF8(x2, y2, 0, temp_buffer, 1) + 8;

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWPSX_EMULATOR], (flash && select_option == 2)) + 8;

    if(!is_ntfs && use_cobra && noBDVD == MODE_DISCLESS)
    {
        x2 = DrawButton2_UTF8(x2, y2, 0, " ps1_emu "   , ((psx_options.flags & PSX_EMULATOR) == 0)) + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, " ps1_netemu ", ((psx_options.flags & PSX_EMULATOR) == 1)) + 8;

        x2 = DrawButton2_UTF8(x2, y2, 0, " old_emu "   , ((psx_options.flags & PSX_EMULATOR) == 2)) + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, " old_netemu ", ((psx_options.flags & PSX_EMULATOR) == 3)) + 8;
    }
    else
    {
        x2 = DrawButton2_UTF8(x2, y2, 0, " ps1_emu "   , ((psx_options.flags & 0x1) == 0)) + 8;
        x2 = DrawButton2_UTF8(x2, y2, 0, " ps1_netemu ", ((psx_options.flags & 0x1) == 1)) + 8;
    }

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWPSX_VIDEOTHER], (flash && select_option == 3)) + 8;


    y2 += 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_CPYGAME], (flash && select_option == 4));

    y2 += 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_DELGAME],
                    (directories[currentgamedir].flags & BDVD_FLAG) ? DISABLED  : ((flash && select_option == 5) ? 1 : 0));

    y2 += 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_FIXGAME],
                    (directories[currentgamedir].flags & BDVD_FLAG) ? DISABLED :(flash && select_option == 6));

    y2 += 48;

    if(!TestFavouritesExits(directories[currentgamedir].title_id))
        DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_CPYTOFAV],
                        (directories[currentgamedir].flags & BDVD_FLAG) ? DISABLED : (flash && select_option == 7));
    else
        DrawButton1_UTF8(x + 32, y2, 320, language[DRAWGMOPT_DELFMFAV],
                        (directories[currentgamedir].flags & BDVD_FLAG) ? DISABLED : (flash && select_option == 7));

    y2 += 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[GLOBAL_RETURN], (flash && select_option == 8));

    y2 += 48;

//
    SetFontSize(8, 10);

    utf8_to_ansi(directories[currentgamedir].path_name, temp_buffer, 128);
    temp_buffer[128] = 0;

    DrawFormatString(x + 8, y + 3 * 150 - 6, "%s", temp_buffer);
//

    // draw game name

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    i = selected;

    u32 str_color = 0xffffffff;

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

    SetFontAutoCenter(0);

    tiny3d_Flip();

    ps3pad_read();

    if(mc_name && (new_pad & BUTTON_SQUARE))
    {
        s64 lenfile;
        struct stat s;
        sprintf(temp_buffer, "/dev_hdd0/savedata/vmc/%s", mc_name);

        if(stat(temp_buffer, &s) >= 0 && s.st_size >= 0x20000)
        {
            lenfile =  s.st_size;

            int flen = strlen(directories[currentgamedir].path_name) - 4;

            if(flen < 0 || strcasestr(".iso|.bin|.mdf|.img", directories[currentgamedir].path_name + flen) == NULL)
                sprintf(temp_buffer + 1024, "%s/%s", directories[currentgamedir].path_name, "Internal_MC.VM1");
            else
            {
                char output[256];
                strcpy(output, directories[currentgamedir].path_name);
                if(output[strlen(output) - 1] == '0') output[strlen(output) - 6] = 0; else output[strlen(output) - 4] = 0;
                sprintf(temp_buffer + 1024, "%s%s", output, ".VM1");
            }

            if(stat(temp_buffer + 1024, &s) >= 0)
            {
                if(DrawDialogYesNo("Internal_MC already exits\n\nDo you want to overwrite the file?") == YES)
                {
                    unlink_secure(temp_buffer + 1024);
                } else return; // cancel
            }

            if(copy_async_gbl(temp_buffer, temp_buffer + 1024, lenfile, "Copying Memory Card as Internal_MC.VM1...", NULL) < 0)
            {
                unlink_secure(temp_buffer + 1024); // delete possible truncated file
                DrawDialogOKTimer("Error copying the Memory Card as Internal_MC.VM1", 2000.0f);
            }
        }
        else
            DrawDialogOKTimer("Error: Memory Card size < 128KB", 2000.0f);

    }

    if(new_pad & (BUTTON_CROSS_))
    {
        switch(select_option)
        {
            case 0: // MC Slot 0
                psx_modified = true;
                sel_psx_vm1++; if(sel_psx_vm1 >= n_psx_vm1) sel_psx_vm1 = 0;

                if(sel_psx_vm1!=0 && !strcmp(psx_options.mc1, &psx_vm1[sel_psx_vm1<<8])) sel_psx_vm1++;
                if(sel_psx_vm1 >= n_psx_vm1) sel_psx_vm1 = 0;

                if(sel_psx_vm1!=0 && !strcmp(psx_options.mc2, &psx_vm1[sel_psx_vm1<<8])) sel_psx_vm1++;
                if(sel_psx_vm1 >= n_psx_vm1) sel_psx_vm1 = 0;

                memcpy(psx_options.mc1, &psx_vm1[sel_psx_vm1<<8], 256);
                if(!strcmp(psx_options.mc1, "No Memory Card")) strncpy(psx_options.mc1, "Internal_MC.VM1", 256);
                break;

            case 1: // MC Slot 1
                psx_modified = true;
                sel2_psx_vm1++; if(sel2_psx_vm1 >= n_psx_vm1) sel2_psx_vm1 = 0;

                if(sel2_psx_vm1!=0 && !strcmp(psx_options.mc2, &psx_vm1[sel2_psx_vm1<<8])) sel2_psx_vm1++;
                if(sel2_psx_vm1 >= n_psx_vm1) sel2_psx_vm1 = 0;

                if(sel2_psx_vm1!=0 && !strcmp(psx_options.mc1, &psx_vm1[sel2_psx_vm1<<8])) sel2_psx_vm1++;
                if(sel2_psx_vm1 >= n_psx_vm1) sel2_psx_vm1 = 0;

                memcpy(psx_options.mc2, &psx_vm1[sel2_psx_vm1<<8], 256);
                break;

            case 2: // PSX Emulator

                psx_modified = true;
                n = (psx_options.flags & PSX_EMULATOR) + 1;

                if(!is_ntfs && use_cobra && noBDVD == MODE_DISCLESS) {
                    if(n > 3) n = 0;
                } else {
                    if(n > 1) n = 0;
                }
                psx_options.flags = (n & PSX_EMULATOR) | (psx_options.flags & 0xfffffffc);
                break;

            case 3: // Video / Others
                menu_screen = SCR_MENU_PSX_VIDEO_OPTIONS;
                select_option = 0;
                return;

            case 4: // Copy Game
                if(mode_homebrew == 1) break;
                if(test_ftp_working()) break;
                if(psx_modified && DrawDialogYesNo(language[DRAWPSX_SAVEASK]) == YES)
                {
                    if(SavePSXOptions(PSX_LAST_PATH) == 0)
                    {
                        //sprintf(temp_buffer, language[DRAWPSX_SAVED]);
                        //DrawDialogOKTimer(temp_buffer, 1500.0f);
                    }
                }
                if(psx_vm1) free(psx_vm1); psx_vm1 = NULL;
                Png_offset[num_box] = 0;

                if(directories[currentgamedir].flags & BDVD_FLAG)
                {
                    pause_music(1);
                    copy_PSX_game_from_CD();
                    pause_music(0);

                    forcedevices = 1;

                    return_to_game_list(true);
                }
                else
                {
                    i = selected;

                    if(Png_offset[i])
                    {
                        pause_music(1);

                        copy_from_selection(currentgamedir);

                        pause_music(0);

                        return_to_game_list(true);
                    }
                }
                return;

            case 5: // Delete Game
                if(test_ftp_working()) break;

                if(psx_vm1) free(psx_vm1); psx_vm1 = NULL;
                Png_offset[num_box] = 0;

                 i = selected;

                 if(Png_offset[i])
                 {
                    pause_music(1);

                    delete_game(currentgamedir);

                    pause_music(0);

                    return_to_game_list(true);;
                 }
                 return;
            case 6: // Fix File Permissions
                 i = selected;

                 if(Png_offset[i])
                 {
                    pause_music(1);

                    // sys8_perm_mode(1);
                    FixDirectory(directories[currentgamedir].path_name, 0);
                    // sys8_perm_mode(0);

                    msgDialogAbort();
                    msgDialogClose(0);

                    pause_music(0);

                    DrawDialogOKTimer(language[DRAWGMOPT_FIXCOMPLETE], 1500.0f);
                 }
                 break;

            case 7: // Delete from Favourites / Copy to Favourites

                if(TestFavouritesExits(directories[currentgamedir].title_id))
                {
                    DeleteFavouritesIfExits(directories[currentgamedir].title_id);

                    sprintf(temp_buffer, "%s/config/", self_path);
                    SaveFavourites(temp_buffer, mode_homebrew);

                    if(mode_favourites && !havefavourites)
                    {
                        mode_favourites = 0;

                        get_games();

                        return_to_game_list(false);;

                        return;
                    }

                    get_games();
                }
                else
                {
                    mode_favourites = currentgamedir  | 65536;

                    get_icon(path_name, currentgamedir);
                    if(!strncmp(path_name + strlen(path_name) -4, ".JPG", 4) || !strncmp(path_name + strlen(path_name) -4, ".jpg", 4))
                        LoadTextureJPG(path_name, num_box);
                    else
                        if(LoadTexturePNG(path_name, num_box) < 0) ;

                    get_games();

                    return_to_game_list(false);

                    return;
                }
                break;

            case 8: // Return
                if(psx_modified && DrawDialogYesNo(language[DRAWPSX_SAVEASK]) == YES)
                {
                    SavePSXOptions(PSX_LAST_PATH);
                }

                if(psx_vm1) free(psx_vm1); psx_vm1 = NULL;
                Png_offset[num_box] = 0;

                return_to_game_list(false);
                return;

            default:
               break;
        }
    }
    else if(new_pad & BUTTON_LEFT)
    {
         switch(select_option)
         {
            case 0: // MC Slot 0
                psx_modified = true;
                sel_psx_vm1--; if(sel_psx_vm1 < 0) sel_psx_vm1 = n_psx_vm1 - 1;

                if(sel_psx_vm1!=0 && !strcmp(psx_options.mc1, &psx_vm1[sel_psx_vm1<<8])) sel_psx_vm1--;
                if(sel_psx_vm1 < 0) sel_psx_vm1 = n_psx_vm1 - 1;

                if(sel_psx_vm1!=0 && !strcmp(psx_options.mc2, &psx_vm1[sel_psx_vm1<<8])) sel_psx_vm1--;
                if(sel_psx_vm1 < 0) sel_psx_vm1 = n_psx_vm1 - 1;

                memcpy(psx_options.mc1, &psx_vm1[sel_psx_vm1<<8], 256);
                if(!strcmp(psx_options.mc1, "No Memory Card")) strncpy(psx_options.mc1, "Internal_MC.VM1", 256);
                break;

            case 1: // MC Slot 1
                psx_modified = true;
                sel2_psx_vm1--; if(sel2_psx_vm1 < 0) sel2_psx_vm1 = n_psx_vm1 - 1;

                if(sel2_psx_vm1!=0 && !strcmp(psx_options.mc2, &psx_vm1[sel2_psx_vm1<<8])) sel2_psx_vm1--;
                if(sel2_psx_vm1 < 0) sel2_psx_vm1 = n_psx_vm1 - 1;

                if(sel2_psx_vm1!=0 && !strcmp(psx_options.mc1, &psx_vm1[sel2_psx_vm1<<8])) sel2_psx_vm1--;
                if(sel2_psx_vm1 < 0) sel2_psx_vm1 = n_psx_vm1 - 1;

                memcpy(psx_options.mc2, &psx_vm1[sel2_psx_vm1<<8], 256);
                break;

            case 2: // PSX Emulator
                psx_modified = true;
                n= (psx_options.flags & PSX_EMULATOR) - 1;

                if(!is_ntfs && use_cobra && noBDVD == MODE_DISCLESS)
                {
                    if(n < 0) n = 3;
                } else {
                    if(n < 0) n = 1;
                }

                psx_options.flags = (n & PSX_EMULATOR) | (psx_options.flags & 0xfffffffc);
                break;
        }
    }
    else if(new_pad & BUTTON_RIGHT)
    {
         switch(select_option)
         {
            case 0: // MC Slot 0
                psx_modified = true;
                sel_psx_vm1++; if(sel_psx_vm1 >= n_psx_vm1) sel_psx_vm1 = 0;

                if(sel_psx_vm1 != 0 && !strcmp(psx_options.mc1, &psx_vm1[sel_psx_vm1<<8])) sel_psx_vm1++;
                if(sel_psx_vm1 >= n_psx_vm1) sel_psx_vm1 = 0;

                if(sel_psx_vm1 != 0 && !strcmp(psx_options.mc2, &psx_vm1[sel_psx_vm1<<8])) sel_psx_vm1++;
                if(sel_psx_vm1 >= n_psx_vm1) sel_psx_vm1 = 0;

                memcpy(psx_options.mc1, &psx_vm1[sel_psx_vm1<<8], 256);
                if(!strcmp(psx_options.mc1, "No Memory Card")) strncpy(psx_options.mc1, "Internal_MC.VM1", 256);
                break;

            case 1: // MC Slot 1
                psx_modified = true;
                sel2_psx_vm1++; if(sel2_psx_vm1 >= n_psx_vm1) sel2_psx_vm1 = 0;

                if(sel2_psx_vm1 != 0 && !strcmp(psx_options.mc2, &psx_vm1[sel2_psx_vm1<<8])) sel2_psx_vm1++;
                if(sel2_psx_vm1 >= n_psx_vm1) sel2_psx_vm1 = 0;

                if(sel2_psx_vm1 != 0 && !strcmp(psx_options.mc1, &psx_vm1[sel2_psx_vm1<<8])) sel2_psx_vm1++;
                if(sel2_psx_vm1 >= n_psx_vm1) sel2_psx_vm1 = 0;

                memcpy(psx_options.mc2, &psx_vm1[sel2_psx_vm1<<8], 256);
                break;

            case 2: // PSX Emulator
                psx_modified = true;
                n= (psx_options.flags & PSX_EMULATOR) + 1;

                if(!is_ntfs && use_cobra && noBDVD == MODE_DISCLESS)
                {
                    if(n > 3) n = 0;
                } else {
                    if(n > 1) n = 0;
                }
                psx_options.flags = (n & PSX_EMULATOR) | (psx_options.flags & 0xfffffffc);
                break;
         }
    }
    else if(new_pad & (BUTTON_TRIANGLE | BUTTON_CIRCLE_))
    {
        if(old_pad & BUTTON_SELECT)
        { /* cancel without any change */ }
        else if(psx_modified && DrawDialogYesNo(language[DRAWPSX_SAVEASK]) == YES)
        {
            if(SavePSXOptions(PSX_LAST_PATH) == 0)
            {
                //sprintf(temp_buffer, language[DRAWPSX_SAVED]);
                //DrawDialogOKTimer(temp_buffer, 1500.0f);
            }
         }

        if(psx_vm1) free(psx_vm1); psx_vm1 = NULL;
        Png_offset[num_box] = 0;

        return_to_game_list(false);
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        select_option--;
        while((directories[currentgamedir].flags & BDVD_FLAG) && select_option >= 5 &&  select_option <= 7) select_option--;

        frame_count = 32;
        if(select_option < 0) select_option = 8;
    }
    else if(new_pad & BUTTON_DOWN)
    {
        select_option++;
        while((directories[currentgamedir].flags & BDVD_FLAG) && select_option >= 5 &&  select_option <= 7) select_option++;

        frame_count = 32;
        if(select_option > 8) select_option = 0;
    }
}

// PSX video options (at the moment...)

void draw_psx_options2(float x, float y, int index)
{
    int n;

    float y2, x2;

    SetCurrentFont(FONT_TTF);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);
    SetFontSize(18, 20);
    SetFontAutoCenter(0);

    DrawFormatString(x, y, " %s", language[DRAWPSX_VIDEOPS]);


    SetFontSize(16, 20);

    y += 24;

    if(bShowPIC1)
    {
        tiny3d_SetTextureWrap(0, Png_res_offset[IMG_DEFAULT_BACKGROUND], Png_res[IMG_DEFAULT_BACKGROUND].width,
                        Png_res[IMG_DEFAULT_BACKGROUND].height, Png_res[IMG_DEFAULT_BACKGROUND].wpitch,
                            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
    }

    DrawTextBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0xffffffff);

    DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

    x2 = x;
    y2 = y + 8;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWPSX_VIDEOMODE], (flash && select_option == 0)) + 8;

    x2 = DrawButton2_UTF8(x2, y2, 0, " Default "   , ((psx_options.video & PSX_VIDEO_MODE) == 0)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, " Region Disc", ((psx_options.video & PSX_VIDEO_MODE) == 1)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, " 480 "       , ((psx_options.video & PSX_VIDEO_MODE) == 2)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, " 576 "       , ((psx_options.video & PSX_VIDEO_MODE) == 3)) + 8;

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWPSX_VIDEOASP], (flash && select_option == 1)) + 8;

    x2 = DrawButton2_UTF8(x2, y2, 0, " Auto ", ((psx_options.video & PSX_VIDEO_ASPECT_RATIO) == 0x0))  + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, " 4:3 " , ((psx_options.video & PSX_VIDEO_ASPECT_RATIO) == 0x10)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, " 16:9 ", ((psx_options.video & PSX_VIDEO_ASPECT_RATIO) == 0x20)) + 8;

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWPSX_FULLSCR], (flash && select_option == 2)) + 8;

    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_YES], ((psx_options.flags & PSX_VIDEO_FULLSCREEN) == 0x0)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_NO] , ((psx_options.flags & PSX_VIDEO_FULLSCREEN) == PSX_VIDEO_FULLSCREEN)) + 8;

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWPSX_SMOOTH], (flash && select_option == 3)) + 8;

    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_YES], ((psx_options.flags & PSX_VIDEO_SMOOTHING) == 0x0)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_NO] , ((psx_options.flags & PSX_VIDEO_SMOOTHING) == PSX_VIDEO_SMOOTHING)) + 8;

    y2 += 48;

    x2 = DrawButton1_UTF8(x + 32, y2, 320, language[DRAWPSX_EXTROM], (flash && select_option == 4)) + 8;

    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_YES], ((psx_options.flags & PSX_EXTERNAL_ROM) == PSX_EXTERNAL_ROM)) + 8;
    x2 = DrawButton2_UTF8(x2, y2, 0, language[DRAWGMCFG_NO], ((psx_options.flags  & PSX_EXTERNAL_ROM) == 0x0)) + 8;

    y2 += 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[DRAWPSX_FORMAT], (flash && select_option == 5));

    y2 += 48;

    DrawButton1_UTF8(x + 32, y2, 320, language[GLOBAL_RETURN], (flash && select_option == 6));


//
    SetFontSize(8, 10);

    utf8_to_ansi(directories[currentgamedir].path_name, temp_buffer, 128);
    temp_buffer[128] = 0;

    DrawFormatString(x + 8, y + 3 * 150 - 6, "%s", temp_buffer);
//

    // draw game name

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    u32 str_color = 0xffffffff;

    if((directories[currentgamedir].flags  & GAMELIST_FILTER) ==  BDVD_FLAG)
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

    SetFontAutoCenter(0);

    tiny3d_Flip();

    ps3pad_read();

    if(new_pad & (BUTTON_CROSS_))
    {
        switch(select_option)
        {
            case 0: // Video Mode
                psx_modified = true;
                n= (psx_options.video & 0xf) + 1;
                if(n > 3) n = 0;
                psx_options.video = (n & PSX_VIDEO_MODE) | (psx_options.video & 0xfffffff0);
                break;

            case 1: // Video Aspect
                psx_modified = true;
                n= ((psx_options.video>>4) & 0xf) +1;
                if(n > 2) n = 0;
                n<<=4;
                psx_options.video = (n & PSX_VIDEO_ASPECT_RATIO) | (psx_options.video & 0xfffffff0f);
                break;

            case 2: // Full Screen
                psx_modified = true;
                psx_options.flags ^= PSX_VIDEO_FULLSCREEN;
                break;

            case 3: // Smoothing
                psx_modified = true;
                psx_options.flags ^= PSX_VIDEO_SMOOTHING;
                break;

            case 4: // External ROM
                psx_modified = true;
                psx_options.flags ^= PSX_EXTERNAL_ROM;
                break;

            case 5: // Format Internal_MC
                if(!((directories[currentgamedir].flags  & GAMELIST_FILTER) ==  BDVD_FLAG) &&
                    DrawDialogYesNo(language[DRAWPSX_ASKFORMAT]) == YES)
                {

                    int flen = strlen(directories[currentgamedir].path_name) - 4;

                    if(flen < 0 || strcasestr(".iso|.bin|.mdf|.img", directories[currentgamedir].path_name + flen) == NULL)
                        sprintf(temp_buffer, "%s/%s", directories[currentgamedir].path_name, "Internal_MC.VM1");
                    else
                    {
                        char output[256];
                        strcpy(output, directories[currentgamedir].path_name);
                        if(output[strlen(output) - 1] == '0') output[strlen(output) - 6] = 0; else output[strlen(output) - 4] = 0;
                        sprintf(temp_buffer, "%s%s", output, ".VM1");
                    }

                    if(build_MC(temp_buffer) == 0) DrawDialogOKTimer(language[OPERATION_DONE], 1500.0f);
                        else DrawDialogOKTimer(language[DRAWPSX_ERRWRITING], 2000.0f);

                }
                return;

            case 6: // Return
                menu_screen = SCR_MENU_PSX_OPTIONS;
                select_option = 3;
                return;

            default:
               break;
        }

       // return_to_game_list(false); return;
    }

    if(new_pad & BUTTON_LEFT)
    {
         switch(select_option)
         {
            case 0: // Video Mode
                psx_modified = true;
                n = (psx_options.video & 0xf) - 1;
                if(n < 0) n = 3;
                psx_options.video = (n & PSX_VIDEO_MODE) | (psx_options.video & 0xfffffff0);
                break;

            case 1: // Video Aspect
                psx_modified = true;
                n = ((psx_options.video>>4) & 0xf) - 1;
                if(n < 0) n = 2;
                n<<=4;
                psx_options.video = (n & PSX_VIDEO_ASPECT_RATIO) | (psx_options.video & 0xfffffff0f);
                break;

            case 2: // Full Screen
                psx_modified = true;
                psx_options.flags ^= PSX_VIDEO_FULLSCREEN;
                break;

            case 3: // Smoothing
                psx_modified = true;
                psx_options.flags ^= PSX_VIDEO_SMOOTHING;
                break;

            case 4: // External ROM
                psx_modified = true;
                psx_options.flags ^= PSX_EXTERNAL_ROM;
                break;
         }
    }

    if(new_pad & BUTTON_RIGHT)
    {
         switch(select_option)
         {
            case 0: // Video Mode
                psx_modified = true;
                n = (psx_options.video & 0xf) + 1;
                if(n > 3) n = 0;
                psx_options.video = (n & PSX_VIDEO_MODE) | (psx_options.video & 0xfffffff0);
                break;

            case 1: // Video Aspect
                psx_modified = true;
                n = ((psx_options.video>>4) & 0xf) + 1;
                if(n > 2) n = 0;
                n<<=4;
                psx_options.video = (n & PSX_VIDEO_ASPECT_RATIO) | (psx_options.video & 0xfffffff0f);
                break;

            case 2: // Full Screen
                psx_modified = true;
                psx_options.flags ^= PSX_VIDEO_FULLSCREEN;
                break;

            case 3: // Smoothing
                psx_modified = true;
                psx_options.flags ^= PSX_VIDEO_SMOOTHING;
                break;

            case 4: // External ROM
                psx_modified = true;
                psx_options.flags ^= PSX_EXTERNAL_ROM;
                break;
         }
    }

    if(new_pad & (BUTTON_TRIANGLE | BUTTON_CIRCLE_))
    {
        menu_screen = SCR_MENU_PSX_OPTIONS;
        select_option = 3;
        return;
    }
    else if(new_pad & BUTTON_UP)
    {
        select_option--;
        if((directories[currentgamedir].flags & BDVD_FLAG) && select_option == 7) select_option--;

        frame_count = 32;

        if(select_option < 0) select_option = 6;
    }
    else if(new_pad & BUTTON_DOWN)
    {
        select_option++;

        frame_count = 32;

        if(select_option > 6) select_option = 0;
    }

}

static unsigned char cd_datas[12] = {
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00
};

u32 get_sectorsize(char *path)
{
    u32 sector_size = 0;
    u32 total_size;
    u8 * mem = malloc(2448 * 3);
    FILE *fp;

    if(!mem) return sector_size;

    fp = fopen(path, "rb");
    if(!fp) return sector_size;

    fseek(fp, 0, SEEK_END);

    total_size = ftell(fp);

    fseek(fp, 0, SEEK_SET);


    if(fread((void *) mem,  1, 2448 * 3, fp) == 2448 * 3)
    {
        if(!memcmp(mem, (char *) cd_datas, 12))
        {
            if(     !memcmp(mem + 2352, (char *) cd_datas, 12) && !memcmp(mem + 2352 * 2, (char *) cd_datas, 12) &&
                !(total_size % 2352)) sector_size = 2352; // match!
            else if(!memcmp(mem + 2336, (char *) cd_datas, 12) && !memcmp(mem + 2336 * 2, (char *) cd_datas, 12) &&
                !(total_size % 2336)) sector_size = 2336; // match!
            else if(!memcmp(mem + 2368, (char *) cd_datas, 12) && !memcmp(mem + 2368 * 2, (char *) cd_datas, 12) &&
                !(total_size % 2368)) sector_size = 2368; // match!
            else if(!memcmp(mem + 2448, (char *) cd_datas, 12) && !memcmp(mem + 2448 * 2, (char *) cd_datas, 12) &&
                !(total_size % 2448)) sector_size = 2448; // match!
            else if(!memcmp(mem + 2328, (char *) cd_datas, 12) && !memcmp(mem + 2328 * 2, (char *) cd_datas, 12) &&
                !(total_size % 2328)) sector_size = 2328; // match!
            else if(!memcmp(mem + 2340, (char *) cd_datas, 12) && !memcmp(mem + 2340 * 2, (char *) cd_datas, 12) &&
                !(total_size % 2340)) sector_size = 2340; // match!
            else if(!memcmp(mem + 2048, (char *) cd_datas, 12) && !memcmp(mem + 2048 * 2, (char *) cd_datas, 12) &&
                !(total_size % 2048)) sector_size = 2048; // match!
        }
        else
        {
            // sector size of 2048 bytes?
            if(!(total_size % 2048)) sector_size = 2048; // match!
        }
    }

    fclose(fp);
    free(mem);

    return sector_size;
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


static msgType mdialogprogress = MSG_DIALOG_SINGLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;

int build_iso(char *path, u32 sector_size)
{
    FILE *fp;
    FILE *fp2;
    int ret = FAILED;
    struct stat s;

    u32 szsect = get_sectorsize(path);

    if(szsect <= 2048) return FAILED;

    if(stat(path, &s) < 0) s.st_size = 0;

    fp = fopen(path, "rb");
    if(!fp) return FAILED;

    sprintf(path + strlen(path)-4,"_%u.img", sector_size);
    fp2 = fopen(path, "wb");
    if(!fp2) {fclose(fp2); return -2;}

    u8 * mem = malloc(4096);

    if(!mem) {fclose(fp);fclose(fp2); return -3;}

    msgDialogOpen2(mdialogprogress, language[DRAWPSX_BUILDISO], progress_callback, (void *) 0xadef0044, NULL);

    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    sysUtilCheckCallback();tiny3d_Flip();

    float parts = 100.0f / (((double) s.st_size)/ (double) szsect);
    float cpart = 0;

    while(fread((void *) mem,  1, szsect, fp) == szsect)
    {
        ret = fwrite((void *) mem + 24 * (sector_size == 2048),  1, sector_size, fp2);
        if(ret != sector_size)
        {
            fclose(fp2); fp2 = NULL;
            unlink_secure(path);
            ret = FAILED;
            break;
        }
        else ret = SUCCESS;

        cpart += parts;
        if(cpart >= 1.0f)
        {
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) cpart);
            cpart-= (float) ((u32) cpart);
            sysUtilCheckCallback();tiny3d_Flip();
        }

    }

    msgDialogAbort();
    usleep(250000);
    if(fp) fclose(fp);
    if(fp2) fclose(fp2);
    free(mem);
    return ret;
}


int are_using_cheats = 0;


u8 * build_from_cue(char *path, int *len, u32 last_sec)
{
    int n, nlen;
    int m;

    int track = 0;
    int minutes = 0;
    int seconds = 0;
    u32 frames = 0;
    int mode = 0;
    int pregap = 0;

    u8 *mem = malloc(2048);

    if(!mem) {*len = 0; return NULL;}
    memcpy(temp_buffer + 4096, path, strlen(path) - 4);
    temp_buffer[4096 + strlen(path) - 4] = 0;

    sprintf(temp_buffer + 5120, "%s.toc", temp_buffer + 4096);

    char * mdata= LoadFile(temp_buffer + 5120, &nlen); // file is TOC Raw

    if(mdata)
    {
        if(nlen > 2048) nlen = 2048;
        memcpy(mem, mdata, nlen);
        *len = nlen; free(mdata);
        return mem;
    }


    sprintf(temp_buffer + 5120, "%s.cue", temp_buffer + 4096);

    mdata = LoadFile(temp_buffer + 5120, &nlen);

    if(!mdata)
    {
        sprintf(temp_buffer + 5120, "%s.CUE", temp_buffer + 4096);
        mdata = LoadFile(temp_buffer + 5120, &nlen);
    }

    if(!mdata)
    {
        // create fake cue info

        m= 4;

        mem[m++] = 0;
        mem[m++] = 0x14;
        mem[m++] = 1;
        mem[m++] = 0;

        frames = 0;

        mem[m++] = (frames>>24) & 0xff;
        mem[m++] = (frames>>16) & 0xff;
        mem[m++] = (frames>>8) & 0xff;
        mem[m++] = (frames) & 0xff;

        mem[m++] = 0;
        mem[m++] = 0x14;
        mem[m++] = 0xAA;
        mem[m++] = 0;

        frames = last_sec;

        mem[m++] = (frames>>24) & 0xff;
        mem[m++] = (frames>>16) & 0xff;
        mem[m++] = (frames>>8) & 0xff;
        mem[m++] = (frames) & 0xff;

        m -= 2;
        mem[0] = (m>>8) & 0xFF;
        mem[1] = m & 0xFF;
        mem[2]= 1;
        mem[3]= 1;

        *len = m + 2;

        return mem;
    }

    n = 0;

    m = 4;

    int control = 3;
    int error = 0;

    int last_track = 0;

    while(n < nlen)
    {
        if(error) break;

        if((n + 5) < nlen && !strncmp((char *) &mdata[n], "TRACK", 5))
        {
            n+= 5;

            if(control!=3) {error=6; break;}
            track = 0;
            while(mdata[n] ==  ' ' || mdata[n] == 9) n++;

            while(mdata[n]>='0' && mdata[n] <='9') {track= (track*10) + mdata[n]- 48;n++;}

            control = 0;
        }
        else if((n + 5) < nlen && !strncmp((char *) &mdata[n], "AUDIO", 5))
        {
            n+= 5;
            mode = 0x10;
            if(control==1) error = 1;
            else if(control) error = 2;
            control = 1;
        }
        else if((n + 6) < nlen && !strncmp((char *) &mdata[n], "PREGAP", 6))
        {
            n+= 6;
            int pminutes, pseconds, pframes = 0;

            while(mdata[n] ==  ' ' || mdata[n] == 9) n++;

            pminutes = pseconds = pframes= 0;

            while(mdata[n]>='0' && mdata[n] <='9') {pminutes= (pminutes*10) + mdata[n]- 48;n++;}
            if(mdata[n] == ':') n++; else {error = 7;break;}
            while(mdata[n]>='0' && mdata[n] <='9') {pseconds= (pseconds*10) + mdata[n]- 48;n++;}
            if(mdata[n] == ':') n++; else {error = 7;break;}
            while(mdata[n]>='0' && mdata[n] <='9') {pframes= (pframes*10) + mdata[n] - 48;n++;}

            pregap = (pminutes * 60 + pseconds) * 75 + pframes;
        }
        else if((n + 5) < nlen && !strncmp((char *) &mdata[n], "MODE2", 5))
        {
            n+= 5;
            mode = 0x14;
            if(control==1) error = 1;
            else if(control) error = 2;
            control = 1;
        }
        else if((n + 5) < nlen && !strncmp((char *) &mdata[n], "INDEX", 5))
        {
            n+= 5;
            int index = 0;
            if(control!=1) {error = 3;break;}

            while(mdata[n] ==  ' ' || mdata[n] == 9) n++;
            while(mdata[n]>='0' && mdata[n] <='9') {index= (index*10) + mdata[n]- 48;n++;}

            if(index == 1)
            {
                while(mdata[n] ==  ' ' || mdata[n] == 9) n++;

                minutes = seconds = frames= 0;

                while(mdata[n] >= '0' && mdata[n] <= '9') {minutes= (minutes*10) + mdata[n]- 48;n++;}
                if(mdata[n] == ':') n++; else {error = 4;break;}
                while(mdata[n] >= '0' && mdata[n] <= '9') {seconds= (seconds*10) + mdata[n]- 48;n++;}
                if(mdata[n] == ':') n++; else {error = 4;break;}
                while(mdata[n] >= '0' && mdata[n] <= '9') {control=2;frames= (frames*10) + mdata[n] - 48;n++;}

                if(control != 2) {error = 4;break;}
            }
        }

        if(control == 2)
        {
            if(track <= last_track) {error= 5; control = 0;break;}
            last_track = track;
            mem[m++] = 0;
            mem[m++] = mode;
            mem[m++] = track;
            mem[m++] = 0;

            frames += (minutes * 60 + seconds) * 75 + pregap;

            mem[m++] = (frames>>24) & 0xff;
            mem[m++] = (frames>>16) & 0xff;
            mem[m++] = (frames>>8) & 0xff;
            mem[m++] = (frames) & 0xff;

           control = 3;
        }

        n++;
    }

    free(mdata);

    if(error)
    {
        free(mem); *len = 0;
        switch(error)
        {
            case 1:
                DrawDialogOKTimer("Error in cuesheet: MODE2/AUDIO must be first token after track", 2000.0f);
                break;
            case 2:
                DrawDialogOKTimer("Error in cuesheet: detected two MODES in the track", 2000.0f);
                break;
            case 3:
                DrawDialogOKTimer("Error in cuesheet: detected INDEX 01 before MODE", 2000.0f);
                break;
            case 4:
                DrawDialogOKTimer("Error in cuesheet: error in INDEX 01", 2000.0f);
                break;
            case 5:
                DrawDialogOKTimer("Error in cuesheet: invalid track order", 2000.0f);
            case 6:
                DrawDialogOKTimer("Error in cuesheet: detected incomplete track", 2000.0f);
                break;
            case 7:
                DrawDialogOKTimer("Error in cuesheet: error in PREGAP", 2000.0f);
                break;
        }

        return NULL;
    }

    mem[m++] = 0;
    mem[m++] = 0x14;
    mem[m++] = 0xAA;
    mem[m++] = 0;

    frames = last_sec;

    mem[m++] = (frames>>24) & 0xff;
    mem[m++] = (frames>>16) & 0xff;
    mem[m++] = (frames>>8) & 0xff;
    mem[m++] = (frames) & 0xff;

    m -= 2;
    mem[0] = (m>>8) & 0xFF;
    mem[1] = m & 0xFF;
    mem[2]= 1;
    mem[3]= last_track;

    *len = m + 2;

    return mem;
}

int build_MC(char *path)
{
    int n;
    int ret = SUCCESS;

    u8 *mem = malloc(0x20000);

    if(mem)
    {
        memset(mem,    0x00, 0x1200);
        memset(mem + 0x1200, 0xFF, 0x20000 - 0x1200);
        mem[0] =  'M'; mem[1] =  'C'; mem[0x7F] =  0x0E;

        for(n = 0x80; n < 0x1200; n += 0x80)
        {
            if(n < 0x800)
            {
                mem[n + 0x00] = 0xA0;
                mem[n + 0x08] = 0xFF;
                mem[n + 0x09] = 0xFF;
                mem[n + 0x7F] = 0xA0;
            }
            else
            {
                mem[n + 0x00] = 0xFF;
                mem[n + 0x01] = 0xFF;
                mem[n + 0x02] = 0xFF;
                mem[n + 0x03] = 0xFF;
                mem[n + 0x08] = 0xFF;
                mem[n + 0x09] = 0xFF;
            }
        }

        if(SaveFile(path, (char *) mem, 0x20000))
        {
            ret = FAILED;
            unlink_secure(path);
        }

        free(mem);
    }
    else
        ret = FAILED;

    return ret;
}

u8 custom_disc_info[16] = {0x00, 0x20, 0x0E, 0x01, 0x01, 0x01, 0x16, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


int psx_cd_with_cheats(void)
{
    struct stat s;
    int cur_sz = 0;
    DIR *dir;

    strncpy((char *) temp_buffer, "/dev_hdd0/PSXGAMES/CHEATS", 1024);

    dir = opendir(temp_buffer);
    if(!dir)
    {
        strncpy((char *) temp_buffer, "/dev_usb/PSXGAMES/CHEATS", 1024);
        dir = opendir(temp_buffer);
    }

    if(!dir) return FAILED;
    if(dir) {
        struct dirent *entry;

        while((entry = readdir(dir)) !=  NULL)
        {
            if(entry->d_type & DT_DIR) continue;

            int flen = strlen(entry->d_name) - 4;

            if(flen < 0 || strcasestr(".iso|.bin|.mdf|.img", entry->d_name + flen) == NULL) continue;

            sprintf(temp_buffer + 1024, "%s/%s", temp_buffer, entry->d_name);

            if(stat(temp_buffer + 1024, &s) < 0) continue;

            int s_z =  get_sectorsize(temp_buffer + 1024);

            if(s_z >= 2352)
            {
                memcpy(temp_buffer + 2048,  temp_buffer + 1024, 1024);
                cur_sz = s_z;
                break;
            }

            if(s_z > 2048 || cur_sz == 0)
            {
                memcpy(temp_buffer +2048,  temp_buffer + 1024, 1024);
                cur_sz = s_z;
            }
        }
        closedir(dir);
    }

    if(cur_sz && DrawDialogYesNo(language[DRAWPSX_ASKCHEATS]) == YES)
    {
        int n;

        load_psx_payload();

        sys8_pokeinstr(0x80000000000000D0, cur_sz);

        reset_sys8_path_table();

        // index TOC table
        u64 lv2_addr = 0x8000000000000050ULL;

        for(n = 0; n < 8; n++)
        {
            sprintf(temp_buffer + 1024, "/psx_d%c", 48 + n);
            add_sys8_path_table(temp_buffer + 1024, temp_buffer +2048);
            sys8_memset(lv2_addr, 0, 0x10);
            lv2_addr+= 0x10ULL;
        }

        dir = opendir("/dev_usb");
        if(dir)
        {
            add_sys8_path_table("/psx_cdrom0", "/dev_usb");
            closedir(dir);
        }
        else
        {
            dir = opendir("/dev_bdvd");
            if(dir)
            {
                add_sys8_path_table("/psx_cdrom0", "//dev_bdvd");
                closedir(dir);
            }
            else
            {
                dir = opendir("/dev_hdd0");
                if(dir)
                {
                    add_sys8_path_table("/psx_cdrom0", "/dev_hdd0");
                    closedir(dir);
                }
            }
        }

        sprintf(temp_buffer, "%s/ps1_emu.self", self_path);
        if(!stat(temp_buffer, &s))
            add_sys8_path_table("/dev_flash/ps1emu/ps1_emu.self", temp_buffer);

        sprintf(temp_buffer, "%s/ps1_netemu.self", self_path);
        if(!stat(temp_buffer, &s))
            add_sys8_path_table("/dev_flash/ps1emu/ps1_netemu.self", temp_buffer);

        sprintf(temp_buffer, "%s/ps1_rom.bin", self_path);
        if((psx_options.flags & PSX_EXTERNAL_ROM) && !stat(temp_buffer, &s))
            add_sys8_path_table("/dev_flash/ps1emu/ps1_rom.bin", temp_buffer);

        //build_sys8_path_table();

        if(!noBDVD || (use_cobra && noBDVD == MODE_DISCLESS))
            sys8_pokeinstr(0x8000000000001830ULL, (u64) 3); // enable emulation mode 3
        else
            sys8_pokeinstr(0x8000000000001830ULL, (u64)((1ULL<<32) | 3ULL));

        return SUCCESS;
    }

    reset_sys8_path_table();
    return FAILED;

}

uint8_t *plugin_args = NULL;

int psx_iso_prepare(char *path, char *name, char *isopath)
{
    DIR  *dir;
    int n, m, nfiles = 0;
    char *files[9];
    u8 *file_datas[9];
    int file_ndatas[9];

    int forced_no_cd = 0;
    u32 sector_size = 0;
    are_using_cheats = 0;

    if(plugin_args) {free(plugin_args); plugin_args = NULL;}

    if((psx_options.flags & PSX_PAYLOAD) == PSX_PAYLOAD)
    {
        // disable PSX payload alternative
        if(!strncmp(path, "/ntfs", 5)  || !strncmp(path, "/ext", 4)) psx_options.flags ^= PSX_PAYLOAD;
    }

    {
        // test old_psxemu use (payload) when CD is ejected under cobra plugin
        struct stat s;

        ps3pad_read();

        sprintf(temp_buffer, "%s/ps1_rom.bin", self_path);
        if((old_pad & BUTTON_L2) && (psx_options.flags & PSX_EXTERNAL_ROM) && !stat(temp_buffer, &s) && use_cobra && noBDVD == MODE_DISCLESS)
        {
            psx_options.flags |= 2;
            if(!strncmp(path, "/ntfs", 5)  || !strncmp(path, "/ext", 4)) forced_no_cd = 2; else forced_no_cd = 1;// force old_psxemu without CD
        }
    }

    if((psx_options.flags & PSX_PAYLOAD) || !(use_cobra && noBDVD == MODE_DISCLESS))
    {
        load_psx_payload();
    }

    for(n = 0; n < 9; n++) if(!(files[n] = malloc(MAXPATHLEN))) return 0;

    if(isopath != NULL)
    {
        sprintf(files[nfiles], "%s", isopath);

        n = strlen(path) - 1; while(n > 1 && path[n] != '/') n--; path[n++] = 0;

        sector_size = get_sectorsize(files[nfiles]);
        nfiles++;
        goto skip_scan_folder_isos;
    }

    if(strstr(path, "/PSXGAMES/CHEATS"))
    {
          DrawDialogOK(language[DRAWPSX_ERRCHEATS]);
          nfiles = 0; return nfiles;
    }

    // scan folder isos
    dir = opendir(path);
    if(dir)
    {
        struct dirent *entry;
        struct stat s;

        while((entry = readdir(dir)) !=  NULL)
        {
            if(entry->d_type & DT_DIR) continue;

            int flen = strlen(entry->d_name) - 4;

            if(flen < 0 || strcasestr(".iso|.bin|.mdf|.img", entry->d_name + flen) == NULL) continue;

            sprintf(files[nfiles], "%s/%s", path, entry->d_name);

            if(stat(files[nfiles], &s) < 0)
            {
                sprintf(temp_buffer, "File access error:\n\n%s", entry->d_name);
                DrawDialogOK(temp_buffer);
                nfiles = 0; break;
            }

            if(nfiles == 0) sector_size = get_sectorsize(files[nfiles]);
            else if(sector_size != get_sectorsize(files[nfiles]))
            {
                sprintf(temp_buffer, language[DRAWPSX_ERRSECSIZE]);
                DrawDialogOK(temp_buffer);
                nfiles = 0; break;
            }

            nfiles++; if(nfiles > 7) break;
        }
        closedir(dir);


skip_scan_folder_isos:

        if(sector_size == 0)
        {
            DrawDialogOK(language[DRAWPSX_ERRUNKSIZE]);
            nfiles = 0;
            reset_sys8_path_table();
            goto end;
        }

        if(nfiles > 0)
        {
           struct stat s;
           sys8_pokeinstr(0x80000000000000D0, sector_size);

           reset_sys8_path_table();

           sprintf(temp_buffer, "%s/ps1_rom.bin", self_path);
           if((psx_options.flags & PSX_EXTERNAL_ROM) && !stat(temp_buffer, &s))
           {
                add_sys8_path_table("/dev_flash/ps1emu/ps1_rom.bin", temp_buffer);

                u64 value = lv2peek(0x8000000000001820ULL);
                if((value == 0x45505331454D5531ULL) && forced_no_cd)
                {
                    if(!noBDVD || (use_cobra && noBDVD == MODE_DISCLESS))
                        sys8_pokeinstr(0x8000000000001830ULL, (u64) 2); // disable CD
                    else
                        sys8_pokeinstr(0x8000000000001830ULL, (u64)((1ULL<<32) | 2ULL)); // disable CD

                    DrawDialogOKTimer(language[DRAWPSX_DISCEJECT], 1500.0f);
                }
           }

        }
        else {reset_sys8_path_table(); goto end;}

        for(n = 0; n < nfiles - 1; n++)
        {
            for(m = n; m < nfiles; m++)
            {
                if(strcmp(files[m], files[n]) < 0)
                {
                    memcpy(files[8], files[m], MAXPATHLEN);
                    memcpy(files[m], files[n], MAXPATHLEN);
                    memcpy(files[n], files[8], MAXPATHLEN);
                }
            }
        }

        if(nfiles > 1)
        {
            int select_option = 0;
            while(true)
            {
                float x= 28, y = 0;
                float y2;

                flash = (frame_count >> 5) & 1;

                frame_count++;
                cls();

                update_twat(true);

                SetCurrentFont(FONT_TTF);

                // header title

                DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

                SetFontColor(0xffffffff, 0x00000000);
                SetFontSize(18, 20);
                SetFontAutoCenter(0);

                DrawFormatString(x, y, " %s", language[DRAWPSX_DISCORDER]);

                if(strncmp((char *) string_title_utf8, name, 64))
                {
                    strncpy((char *) string_title_utf8, name, 128);
                    update_title_utf8 = true;
                }

                if(update_title_utf8)
                {
                    width_title_utf8 = Render_String_UTF8(ttf_texture, 768, 32, string_title_utf8, 16, 24);
                    update_title_utf8 = false;
                }

                tiny3d_SetTextureWrap(0, tiny3d_TextureOffset(ttf_texture), 768,
                    32, 768 * 2, TINY3D_TEX_FORMAT_A4R4G4B4,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                DrawTextBox((848 - width_title_utf8) - x, y - 2 , 0, 768, 32, 0xffffffff);
                y += 24;

                DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);


                y2 = y + 32;

                for(n = 0; n < nfiles; n++)
                {
                    utf8_truncate(((char *) files[n]) + strlen(path) + 1, temp_buffer, 40);
                    DrawButton1_UTF8((848 - 720) / 2, y2, 720, temp_buffer, (flash && select_option == n));
                    y2 += 48;
                }

                for(; n < 8; n++) {DrawButton1_UTF8((848 - 720) / 2, y2, 720, "", DISABLED); y2 += 48;}

                SetCurrentFont(FONT_TTF);
                SetFontColor(0xffffffff, 0x00000000);
                SetFontSize(16, 20);

                SetFontAutoCenter(1);

                DrawFormatString(0, 150 * 3 + y - 8, "%s", language[DRAWPSX_PRESSOB]);

                DrawFormatString(0, 150 * 3 + y + 16 - 8, "%s", language[DRAWPSX_PRESSXB]);
                SetFontAutoCenter(0);

                tiny3d_Flip();

                ps3pad_read();

                if(new_pad & (BUTTON_CROSS_ | BUTTON_START)) break;

                if(new_pad & BUTTON_CIRCLE_) return 0;

                if(new_pad & (BUTTON_TRIANGLE | BUTTON_DOWN | BUTTON_R1))
                {
                    memcpy(files[8], files[0], MAXPATHLEN);
                    for(n = 0; n < nfiles - 1; n++) memcpy(files[n], files[n+1], MAXPATHLEN);
                    memcpy(files[nfiles - 1],  files[8], MAXPATHLEN);
                }
                if(new_pad & (BUTTON_UP | BUTTON_L1))
                {
                    if((nfiles > 0) && (nfiles < 8))
                    {
                        memcpy(files[8], files[nfiles - 1], MAXPATHLEN);
                        for(n = nfiles - 1; n > 0; n--) memcpy(files[n], files[n-1], MAXPATHLEN);
                        memcpy(files[0],  files[8], MAXPATHLEN);
                    }
                }
            }
        }

        if(nfiles > 0)
        {
            /////////////////////////////////////
            // cheats
            /////////////////////////////////////
            if(nfiles < 8 && forced_no_cd != 2)
            {
                strcpy((char *) tmp_path, "/dev_hdd0/PSXGAMES/CHEATS");
                dir = opendir(tmp_path);
                if(!dir)
                {
                    strcpy((char *) tmp_path, "/dev_usb000/PSXGAMES/CHEATS");
                    dir = opendir(tmp_path);
                }

                if(dir)
                {
                    struct dirent *entry;
                    struct stat s;
                    while((entry = readdir(dir)) !=  NULL)
                    {
                        if(entry->d_type & DT_DIR) continue;

                        int flen = strlen(entry->d_name) - 4;

                        if(flen < 0 || strcasestr(".iso|.bin|.mdf|.img", entry->d_name + flen) == NULL) continue;

                        sprintf(files[nfiles], "%s/%s", tmp_path, entry->d_name);

                        if(stat(files[nfiles], &s) < 0) continue;

                        int s_z =  get_sectorsize(files[nfiles]);

                        if(s_z >= 2352) {memcpy(files[8], files[nfiles], MAXPATHLEN);are_using_cheats = 2;} // possible candidate

                        if(sector_size == s_z)
                        {
                            if(DrawDialogYesNo(language[DRAWPSX_ASKCHEATS]) == YES)
                            {
                                nfiles++;
                                int m;
                                are_using_cheats = 1;
                                for(m = 0; m < nfiles - 1; m++)
                                {
                                    // rotate cheat iso to the first
                                    memcpy(files[8], files[0], MAXPATHLEN);
                                    for(n = 0; n < nfiles - 1; n++) memcpy(files[n], files[n+1], MAXPATHLEN);
                                    memcpy(files[nfiles - 1],  files[8], MAXPATHLEN);
                                }
                            }
                            else are_using_cheats = 0;

                            break;
                        }

                    }

                    closedir(dir);
                }

                //////////////////////
                if(are_using_cheats == 2)
                {
                    // try build one
                    if(DrawDialogYesNo(language[DRAWPSX_CHEATMAKE]) == YES)
                    {
                        if(build_iso(files[8], sector_size) == 0)
                        {
                            memcpy(files[nfiles], files[8], MAXPATHLEN); // files[8] return no have the same name!
                            nfiles++;
                            int m;
                            are_using_cheats = 1;
                            for(m = 0; m < nfiles - 1; m++)
                            {
                                // rotate cheat iso to the first
                                memcpy(files[8], files[0], MAXPATHLEN);
                                for(n = 0; n < nfiles - 1; n++) memcpy(files[n], files[n+1], MAXPATHLEN);
                                memcpy(files[nfiles - 1],  files[8], MAXPATHLEN);
                            }
                        }
                     }
                     else are_using_cheats =0;
                }
                //////////////////////
            }

            /////////////////////////////////////

            // table TOC datas
            u64 lv2_addr = 0x80000000007E0000ULL;
            u32 lv2_table[8][4];

            for(n = 0; n < nfiles; n++)
            {
                struct stat s;
                stat(files[n], &s);

                lv2_table[n][0] = 0;
                lv2_table[n][1] = 0;
                lv2_table[n][2] = 0;
                lv2_table[n][3] = 0;

                file_datas[n] = build_from_cue(files[n], &file_ndatas[n], s.st_size/sector_size);

                if(file_datas[n])
                {
                    lv2_table[n][2] = (u32) lv2_addr;
                    lv2_table[n][3] = file_ndatas[n];
                    // copy full TOC datas
                    sys8_memcpyinstr(lv2_addr, (u64) file_datas[n], (u64) file_ndatas[n]);
                    lv2_addr+= (u64) ((file_ndatas[n] + 15) & ~15);
                    lv2_table[n][0] = (u32) lv2_addr;

                    custom_disc_info[6] =*(((u8 *) file_datas[n])+ 3); // copy last track
                    // copy disck info datas
                    sys8_memcpyinstr(lv2_addr, (u64) custom_disc_info, 16ULL);
                    lv2_addr+= 16;
                    free(file_datas[n]);
                }
            }

            if(are_using_cheats == 1) region_psx = get_psx_region_file(files[1]);
            else region_psx = get_psx_region_file(files[0]);

            if(forced_no_cd == 2)
            {
                // NTFS & EXTx (copy partial ISO to HDD0)
                sprintf(files[7], "%s/temp_psx.iso", self_path);

                if(copy_async_gbl(files[0], files[7], 32 * sector_size, "creating temp_psx", NULL) < 0)
                {
                    DrawDialogOK("Error creating temp_psx");
                    goto end;
                }
            }

            if(!forced_no_cd && use_cobra && noBDVD == MODE_DISCLESS && (psx_options.flags & PSX_PAYLOAD) != PSX_PAYLOAD)
            {
                // use plugin
                plugin_args = malloc(0x20000);

                psxseciso_args *p_args;

                memset(plugin_args, 0, 0x10000);

                p_args = (psxseciso_args *)plugin_args;
                p_args->device = 0ULL;
                p_args->emu_mode = EMU_PSX_MULTI;

                int max_parts = (0x10000 - sizeof(psxseciso_args)) / 8;

                uint32_t *sections = malloc(max_parts * sizeof(uint32_t));
                uint32_t *sections_size = malloc(max_parts * sizeof(uint32_t));

                u32 offset = 0;

                for(n = 0; n < nfiles; n++)
                {
                    if(stat(files[n], &s)) s.st_size = 0x40000000;
                    if(!strncmp(files[n], "/ntfs", 5) || !strncmp(files[n], "/ext", 4))
                    {
                        if(p_args->device == 0) p_args->device = USB_MASS_STORAGE(NTFS_Test_Device(((char *) files[n])+1));

                        memset(sections, 0, max_parts * sizeof(uint32_t));
                        memset(sections_size, 0, max_parts * sizeof(uint32_t));
                        int parts = ps3ntfs_file_to_sectors(files[n], sections, sections_size, max_parts, 1);
                        if(parts <=0 || parts >= max_parts)
                        {
                            if(sections) free(sections);
                            if(sections_size) free(sections_size);
                            if(plugin_args) free(plugin_args); plugin_args = NULL;

                            sprintf(temp_buffer, "Too much parts in PSX iso:\n%s", files[n]);
                            DrawDialogOK(temp_buffer);
                            nfiles = 0;
                            reset_sys8_path_table();
                            goto end;
                        }

                        //sprintf(temp_buffer, "parts %i\n%s", parts, files[n]);
                        //DrawDialogOK(temp_buffer);

                        p_args->discs_desc[n][0] = (parts << 16) | offset;

                        memcpy((void *) (((u32 *) &p_args->discs_desc[8][0]) + offset), sections, parts * sizeof(uint32_t));
                        memcpy((void *) (((u32 *) &p_args->discs_desc[8][0]) + offset + parts), sections_size, parts * sizeof(uint32_t));

                        offset += parts * 2;
                        max_parts -= parts;

                    }
                    else p_args->discs_desc[n][0] = 0;

                    p_args->discs_desc[n][1] = (s.st_size / sector_size);

                }

                for(; n < 8; n++)
                {
                    p_args->discs_desc[n][0] = p_args->discs_desc[n % nfiles][0];
                    p_args->discs_desc[n][1] = p_args->discs_desc[n % nfiles][1];

                }

                if(sections) free(sections);
                if(sections_size) free(sections_size);
            }
            else
            {
                if(plugin_args) free(plugin_args); plugin_args = NULL;
            }

            // index TOC table
            lv2_addr = 0x8000000000000050ULL;

            for(n = 0; n < 8; n++)
            {
                sprintf(files[8], "/psx_d%c", 48 + n);

                if(forced_no_cd == 2) add_sys8_path_table(files[8], files[7]); // NTFS-EXTx without CD
                else add_sys8_path_table(files[8], files[n % nfiles]);

                sys8_memcpyinstr(lv2_addr, (u64) &lv2_table[n  % nfiles][0], 0x10);
                lv2_addr+= 0x10ULL;
            }

            if(forced_no_cd == 2) {plugin_args = NULL; goto use_hdd0;}

            dir = opendir("/dev_usb");
            if(dir) {
                add_sys8_path_table("/psx_cdrom0", "/dev_usb");
                closedir(dir);
            }
            else
            {
                psxseciso_args *p_args;
                p_args = (psxseciso_args *)plugin_args;

                if(p_args && p_args->device == 0 && use_cobra && noBDVD == MODE_DISCLESS)
                {
                    // use NTFS/EXTx if it is connected
                    for(n = 0; n < 8; n++)
                    {
                        if(PS3_NTFS_IsInserted(n))
                        {
                            p_args->device = USB_MASS_STORAGE(n);
                            goto use_hdd0;
                        }
                    }
                }

                if(p_args) goto use_hdd0;

                dir = opendir("/dev_bdvd");
                if(dir)
                {
                    add_sys8_path_table("/psx_cdrom0", "//dev_bdvd");
                    closedir(dir);
                }
                else
                {
    use_hdd0:
                    dir = opendir("/dev_hdd0");
                    if(dir)
                    {
                        add_sys8_path_table("/psx_cdrom0", "/dev_hdd0");
                        closedir(dir);
                    }
                }
            }

            if(!strcmp(psx_options.mc1, "No Memory Card") || !strcmp(psx_options.mc1, "Internal_MC.VM1"))
            {
                struct stat s;

                int flen = strlen(directories[currentgamedir].path_name) - 4;

                if(flen < 0 || strcasestr(".iso|.bin|.mdf|.img", directories[currentgamedir].path_name + flen) == NULL)
                    sprintf(files[8], "%s/%s", path, "Internal_MC.VM1");
                else
                {
                    char output[256];
                    strcpy(output, directories[currentgamedir].path_name);
                    if(output[strlen(output) - 1] == '0') output[strlen(output) - 6] = 0; else output[strlen(output) - 4] = 0;
                    sprintf(files[8], "%s%s", output, ".VM1");
                }

                if(stat(files[8], &s) < 0)
                {
                    /// create one empty memory card :)
                    if(build_MC(files[8])) strncpy(psx_options.mc1, "No Memory Card", 256);
                }

                if(stat(files[8], &s) >= 0 && s.st_size >= 0x20000)
                {
                    if(!strncmp(path, "/dev_usb", 8) || !strncmp(path, "/ntfs", 5) || !strncmp(path, "/ext", 4)) { // from USB move the MC to the Iris folder VM1

                        sprintf(temp_buffer, "%s/VM1", self_path);
                        mkdir_secure(temp_buffer);

                        sprintf(temp_buffer + 1024, "%s/VM1/%s", self_path, name);
                        mkdir_secure(temp_buffer + 1024);

                        sprintf(temp_buffer, "%s/VM1/%s/Internal_MC.VM1", self_path, name);

                        unlink_secure(temp_buffer);

                        if(copy_async_gbl(files[8], temp_buffer, s.st_size, language[DRAWPSX_COPYMC], NULL) < 0)
                        {
                            unlink_secure(temp_buffer); // delete possible truncated file
                            strncpy(psx_options.mc1, "No Memory Card", 256);
                            DrawDialogOK(language[DRAWPSX_ERRCOPYMC]);

                        }
                        else
                            memcpy(files[8], temp_buffer, MAXPATHLEN);
                    }
                }
                else strncpy(psx_options.mc1, "No Memory Card", 256);

                add_sys8_path_table("/dev_hdd0/savedata/vmc/Internal_MC.VM1", files[8]);
            }

            sprintf(temp_buffer, "%s/ps1_emu.self", self_path);
            if(!stat(temp_buffer, &s))
                add_sys8_path_table("/dev_flash/ps1emu/ps1_emu.self", temp_buffer);

            sprintf(temp_buffer, "%s/ps1_netemu.self", self_path);
            if(!stat(temp_buffer, &s))
                add_sys8_path_table("/dev_flash/ps1emu/ps1_netemu.self", temp_buffer);

            //build_sys8_path_table();
        }
    }

 end:
    for(n = 0; n < 9; n++) if(files[n]) free(files[n]);

    return nfiles;
}

void unload_psx_payload()
{
    volatile u64 value = lv2peek(0x8000000000001820ULL);

    if(value == 0x45505331454D5531ULL)
    {
        // new method
        sys8_pokeinstr(0x8000000000001830ULL, (u64) 1); // disable emulation

        if(!syscall_base)
        {
            DrawDialogOKTimer("syscall_base is empty!", 2000.0f);
            return;
        }

        value = lv2peek(0x80000000000018A0ULL); // 604 (send cmd syscall base)
        sys8_pokeinstr(syscall_base + (u64) (604 * 8), value);
        value = lv2peek(0x80000000000018A8ULL); // 600 (open syscall base)
        sys8_pokeinstr(syscall_base + (u64) (600 * 8), value);
        usleep(1000); // very important!
    }
}

void load_psx_payload()
{
    int n;
    static int one = 0;

    if(one) return;

    one = 1;

    // 0x1720 -> 0x180e bytes free in 4.31
    u64 addr = 0x8000000000001820ULL;
    u64 addr2, addr3, /*addr4,*/ toc, addrt;
    u64 base1, base2/*, base3*/;

#if 0
    int m;
    sprintf(temp_buffer, "%s/psx_storage.bin", self_path);
    char * mdata= LoadFile(temp_buffer, &m);
    if(!mdata) return;
#endif
/*
    if(lv2peek(addr) != 0x45505331454D5531ULL)
    {
        for(n = 0; n < psx_storage_bin_size + 8; n+= 8)
        {
            if(lv2peek(addr + (u64) n))
            {
                DrawDialogOKTimer("Error: The LV2 space for psx_storage is not empty\n\nExiting to the XMB", 2000.0f);
                exit(0);
            }
        }
    }
*/
    base1 = lv2peek(syscall_base + (u64) (600 * 8));
    toc   = lv2peek(base1 + 0x8ULL);
    addr2 = lv2peek(base1);
    base2 = lv2peek(syscall_base + (u64) (604 * 8));
    addr3 = lv2peek(base2);

    // to be sure code is written ...

    #if 1
    sys8_memcpyinstr(addr, // copy psx_storage routines
              (u64) psx_storage_bin,
              psx_storage_bin_size);
    #else
    sys8_memcpyinstr(addr, // copy psx_storage routines
              (u64) mdata,
              m);
    #endif

    sys8_pokeinstr(0x8000000000001828ULL, syscall_base);
    sys8_pokeinstr(0x8000000000001840ULL, toc);
    sys8_pokeinstr(0x8000000000001850ULL, addr2); // 600
    sys8_pokeinstr(0x8000000000001860ULL, toc);
    sys8_pokeinstr(0x8000000000001870ULL, addr3); // 604
    sys8_pokeinstr(0x8000000000001880ULL, toc);

    if(base1) {
        sys8_pokeinstr(0x80000000000018A0ULL, base2); // 604 (send cmd syscall base)
        sys8_pokeinstr(0x80000000000018A8ULL, base1); // 600 (open syscall base)
    }


    addrt = addr + 0x18ULL;
    sys8_pokeinstr(syscall_base + (u64) (600 * 8),addrt);

    addrt = addr + 0x38ULL;
    sys8_pokeinstr(syscall_base + (u64) (604 * 8), addrt);

    usleep(10000);

   if(!noBDVD || (use_cobra && noBDVD == MODE_DISCLESS))
       sys8_pokeinstr(0x8000000000001830ULL, (u64) 0); // enable emulation
   else
       sys8_pokeinstr(0x8000000000001830ULL, (u64)((1ULL<<32))); // enable emulation

}

void psx_launch(void)
{
    u32 p[10] __attribute__((unused));
    char *arg[10];
    int k;

    struct stat s;
    if(!stat("/psx_d0", &s) || (plugin_args && use_cobra && noBDVD == MODE_DISCLESS))
    {
        //psx_options.flags   |= 3;
        directories[currentgamedir].flags|= (region_psx & 0xff)<<16; // get region data from second disc

    }
    else if(directories[currentgamedir].flags & BDVD_FLAG)
    {
        struct stat s;

        reset_sys8_path_table();

        sprintf(temp_buffer, "%s/ps1_rom.bin", self_path);
        if((psx_options.flags & PSX_EXTERNAL_ROM) && !stat(temp_buffer, &s))
            add_sys8_path_table("/dev_flash/ps1emu/ps1_rom.bin", temp_buffer);
    }

    //syscall36(self_path); // to avoid stupid check in ps1_netemu

    add_sys8_bdvd(self_path, NULL);
    build_sys8_path_table();

    if(!test_controlfan_compatibility())
    {
        sys_set_leds(2, 0);
        sys_set_leds(0, 0);
        sys_set_leds(1, 1);
    }
    else
        set_fan_mode(-1);

    if(!(psx_options.flags & PSX_PAYLOAD) && plugin_args && use_cobra && noBDVD == MODE_DISCLESS)
    {

        cobra_send_fake_disc_eject_event();
        cobra_umount_disc_image();

        cobra_unload_vsh_plugin(0);

        sprintf(temp_buffer + 2048, PLUGIN_ISO, self_path);

        if (cobra_load_vsh_plugin(0, temp_buffer + 2048, plugin_args, 0x10000) == 0)
        {
            use_cobra = 2; /*exit(0);*/
        }
        else
        {
            use_cobra = 2; exit(0);
        }
    }

    if((psx_options.flags & PSX_PAYLOAD) == PSX_PAYLOAD) psx_options.flags ^= PSX_PAYLOAD; //  dont use flag 2 here

    // check empty mc

    if(strlen(psx_options.mc1) < 4 || strcmp(psx_options.mc1 + strlen(psx_options.mc1) - 4, ".VM1")) psx_options.mc1[0] = 0;
    if(strlen(psx_options.mc2) < 4 || strcmp(psx_options.mc2 + strlen(psx_options.mc2) - 4, ".VM1")) psx_options.mc2[0] = 0;
    if(!strcmp(psx_options.mc1, psx_options.mc2)) {psx_options.mc2[0] = ' '; psx_options.mc2[0] = 0;}

    // building args

    k = 0;

    arg[k] = malloc(256); p[k] = (u32)(u64) arg[k]; strncpy(arg[k], psx_options.mc1, 256); k++;
    arg[k] = malloc(256); p[k] = (u32)(u64) arg[k]; strncpy(arg[k], psx_options.mc2, 256); k++;
    arg[k] = malloc( 5);  p[k] = (u32)(u64) arg[k]; strncpy(arg[k], "0082", 5);  k++;   // region
    arg[k] = malloc( 5);  p[k] = (u32)(u64) arg[k];

    if((psx_options.flags & 0x1) == 0) strncpy(arg[k], "1200", 5); else strncpy(arg[k], "1600", 5);

    k++;

    if((psx_options.flags & PSX_EMULATOR) != 0)
    {
        arg[k] = malloc(16); p[k] = (u32)(u64) arg[k]; strncpy(arg[k], "", 16); k++;
        arg[k] = malloc( 2); p[k] = (u32)(u64) arg[k]; strncpy(arg[k], "1", 2); k++;
    }

    // full screen  on/off  = 2/1
    arg[k] = malloc( 2); p[k] = (u32)(u64) arg[k];
    if((psx_options.flags & PSX_VIDEO_FULLSCREEN) == 0) strncpy(arg[k], "2", 2); else strncpy(arg[k], "1", 2); k++;

    // smoothing    on/off  = 1/0
    arg[k] = malloc( 2); p[k] = (u32)(u64) arg[k];
    if((psx_options.flags & PSX_VIDEO_SMOOTHING) == 0) strncpy(arg[k], "1", 2); else strncpy(arg[k], "0", 2); k++;

    p[k] = 0;arg[k] =NULL;

    cls2();
    tiny3d_Flip();
    cls2();
    tiny3d_Flip();

    if(lv2_patch_storage) sys_storage_reset_bd();

    if((psx_options.video & PSX_VIDEO_MODE) != 0)
    {
        videoConfiguration vconfig;
        memset(&vconfig, 0, sizeof(videoConfiguration));

        if((psx_options.video & PSX_VIDEO_MODE) == 1)
        {
            if(((directories[currentgamedir].flags>>16) & 0x1f) != 0x11)
                vconfig.resolution = VIDEO_RESOLUTION_480;
            else
                vconfig.resolution = VIDEO_RESOLUTION_576;
        }
        else if((psx_options.video & PSX_VIDEO_MODE) == 2)
            vconfig.resolution = VIDEO_RESOLUTION_480;
        else
            vconfig.resolution = VIDEO_RESOLUTION_576;

        vconfig.format = VIDEO_BUFFER_FORMAT_XRGB;
        vconfig.pitch = 2880;

        switch(psx_options.video & PSX_VIDEO_ASPECT_RATIO)
        {
            case 0x10:
                Video_aspect = VIDEO_ASPECT_4_3;
                break;
            case 0x20:
                Video_aspect = VIDEO_ASPECT_16_9;
                break;
            default:
                Video_aspect = VIDEO_ASPECT_AUTO;
                break;
        }

        vconfig.aspect = Video_aspect;

        videoConfigure(0, &vconfig, NULL, 0);
    }

    unlink_secure("/dev_hdd0/tmp/wm_request");
    fun_exit();

    if((psx_options.flags & 0x1) == 0)
        sysProcessExitSpawn2((const char*) "/dev_flash/ps1emu/ps1_emu.self", (const char**) arg, NULL, NULL, 0,
            1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
    else
        sysProcessExitSpawn2((const char*) "/dev_flash/ps1emu/ps1_netemu.self", (const char**) arg, NULL, NULL, 0,
            1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
}

void Reset1_BDVD(void)
{
    int ret;
    int n;

    ret = sys_storage_reset_bd();

    if(ret < 0 && ((u32) ret) == 0x8001000A)
    {
        for(n = 0; n < 500000; n++)
        {
           ret = sys_storage_reset_bd();
           if(((u32) ret) != 0x8001000A) break;
        }
    }

    // old method

    if(ret == 0)
    {
        ret = sys_storage_authenticate_bd();
       if(ret < 0 && ((u32) ret) == 0x8001000A)
       {
            for(n = 0; n < 500000; n++)
            {
               ret = sys_storage_authenticate_bd();
               if(((u32) ret) != 0x8001000A) break;
            }
        }
    }

    sys_storage_ctrl_bd(0x3f);
}


void Reset2_BDVD(void)
{
    int ret;
    int n;

    ret = sys_storage_reset_bd();

    if(ret < 0 && ((u32) ret) == 0x8001000A)
    {
        for(n = 0; n < 500000; n++)
        {
           ret = sys_storage_reset_bd();
           if(((u32) ret) != 0x8001000A) break;
        }
    }

 // new method
    if(ret == 0)
    {
        ret = sys_storage_ctrl_bd(0x43);
       if(ret < 0 && ((u32) ret) == 0x8001000A)
       {
            for(n = 0; n < 500000; n++)
            {
               ret = sys_storage_ctrl_bd(0x43);
               if(((u32) ret) != 0x8001000A) break;
            }
        }
    }

    sys_storage_ctrl_bd(0x3f);
}


// from Multiman modified by Estwald

static u8 data_cd001[8] = {
    0x01, 0x43, 0x44, 0x30, 0x30, 0x31, 0x01, 0x00
};

u8 psx_id[32];

u8 get_psx_region_file(char *path)
{
    u8 ret = 0; // 0xXY X=1/2 PS1/PS2, Y=1/2/3 PAL/USA/JAP, 0=NA

    int indx = 0;
    u8* read_buffer = (unsigned char *) memalign(16, 4096);

#ifdef PSDEBUG
    u64 disc_size = 0;
#endif
    device_info_t disc_info;

    FILE *fp;

    if(!(disc_info.sector_size = get_sectorsize(path)))
    {
        free(read_buffer);
        return 0;
    }

    fp = fopen(path, "rb");

    if (fp != NULL)
    {
        fseek(fp, 0, SEEK_END);

        disc_info.total_sectors = ftell(fp);

        if(disc_info.sector_size == 0) disc_info.sector_size = 2352;
       // if(disc_info.total_sectors % 2352) goto error; // invalid sector size

        disc_info.total_sectors/=disc_info.sector_size;

        fseek(fp, 0, SEEK_SET);

        if(disc_info.sector_size > 2048) indx = 24;

#ifdef PSDEBUG
        disc_size = disc_info.sector_size * disc_info.total_sectors;
#endif

        fseek(fp, disc_info.sector_size * 4, SEEK_SET); // read sector 4 for License message
        if(fread((void *) read_buffer, 1, disc_info.sector_size, fp) == disc_info.sector_size)
        {
                if( read_buffer[0x20 + indx] == 0x53 ) ret = 0x13; else //Sony
                if( read_buffer[0x3c + indx] == 0x45 ) ret = 0x11; else //Europe
                if( read_buffer[0x3c + indx] == 0x41 ) ret = 0x12;      //America
        }

        fseek(fp, disc_info.sector_size * 16, SEEK_SET); // read sector 16
        if(fread((void *) read_buffer, 1, disc_info.sector_size, fp) == disc_info.sector_size)
        {
                if(!memcmp((char *) &read_buffer[indx], data_cd001, 8) && !strncmp((char *) &read_buffer[indx + 8], "PLAYSTATION ", 12) &&
                    !strncmp((char *) &read_buffer[indx + 0x400], "CD-XA001", 8))
                {
                    if(ret == 0) ret = 0x11;
                    strncpy((char *) psx_id, (char*) &read_buffer[indx + 0x28], 32);
                }
        }
    }

    if(ret == 0) strncpy((char *) psx_id, "PSX GAME UNKNOWN", 32);
    if(fp) fclose(fp);
    free(read_buffer);
    return ret;
}

int get_disc_ready(void)
{
    int rr;
    int dev_id;

    u64 disc_size = 0;
    device_info_t disc_info;

    rr = sys_storage_open(BD_DEVICE, &dev_id);
    if(!rr)
        while((rr = sys_storage_get_device_info(BD_DEVICE, &disc_info)) == 0x8001000A) usleep(20000);

    disc_size = disc_info.sector_size * disc_info.total_sectors;

    sys_storage_close(dev_id);

    if(disc_size && !rr) return FAILED;

    return SUCCESS;
}

u8 get_psx_region_cd(void)
{
    u8 ret = 0; // 0xXY X=1/2 PS1/PS2, Y=1/2/3 PAL/USA/JAP, 0=NA

    int rr;
    int indx = 0;
    int dev_id;
    u8* read_buffer = (unsigned char *) memalign(16, 4096);
    u32 readlen = 0;
    u64 disc_size = 0;
    device_info_t disc_info;

    rr=sys_storage_open(BD_DEVICE, &dev_id);
    if(!rr)
        while((rr=sys_storage_get_device_info(BD_DEVICE, &disc_info)) == 0x8001000A) usleep(20000);

    if(disc_info.sector_size > 2048) indx = 24;

    disc_size = disc_info.sector_size * disc_info.total_sectors;

    if(disc_size && !rr) // read sector 4 for License message
    {
        while((rr=sys_storage_read(dev_id, 4, 1, read_buffer, &readlen)) == 0x8001000A) usleep(20000);
        if(!rr && readlen==1)
        {
            if( read_buffer[0x20 + indx] == 0x53 ) ret = 0x13; //Sony
            if( read_buffer[0x3c + indx] == 0x45 ) ret = 0x11; //Europe
            if( read_buffer[0x3c + indx] == 0x41 ) ret = 0x12; //America

        }

        while(( rr=sys_storage_read(dev_id, 16, 1, read_buffer, &readlen)) == 0x8001000A) usleep(20000);
        if(!rr && readlen==1)
        {
            if(!memcmp((char *) &read_buffer[indx], data_cd001, 8) &&
               !strncmp((char *) &read_buffer[indx + 8], "PLAYSTATION ", 12) &&
               !strncmp((char *) &read_buffer[indx + 0x400], "CD-XA001", 8))
            {
                if(ret == 0) ret = 0x11;
                strncpy((char *) psx_id, (char*) &read_buffer[indx + 0x28], 32);
            }
        }
    }
    if(ret == 0) strncpy((char *) psx_id, "PSX GAME UNKNOWN", 32);
    rr=sys_storage_close(dev_id);
    free(read_buffer);

    return ret;
}

static int bdvd_id = -1;

int open_bdvd(u64 *total_sector, u32 *sector_size)
{

    device_info_t disc_info;

    if(bdvd_id < 0 && sys_storage_open(BD_DEVICE, &bdvd_id) < 0) return FAILED;

    if(sys_storage_get_device_info(BD_DEVICE, &disc_info) < 0)
    {
        sys_storage_close(bdvd_id); bdvd_id = -1;
        return -2;
    }

    *total_sector = disc_info.total_sectors;
    *sector_size  = disc_info.sector_size;

    return SUCCESS;
}

int close_bdvd(void)
{
    int ret = SUCCESS;

    if(bdvd_id >= 0)
    {
        ret = sys_storage_close(bdvd_id);
        bdvd_id = -1;
    }

    return ret;
}

static struct lv2_atapi_cmnd_block atapi_cmd;

int read_raw_sector_bdvd(u32 sector, void *buffer, u32 sector_size, int n_sectors)
{
    int ret;

    if(bdvd_id < 0) return FAILED;

    memset(&atapi_cmd, 0, sizeof(struct lv2_atapi_cmnd_block));

    atapi_cmd.pkt[0] = GPCMD_READ_CD;
    atapi_cmd.pkt[0xa] = 2;

    memcpy(&atapi_cmd.pkt[5], &n_sectors, 4); // only 3 LSB bytes (pkt[5] is overwrite by sector)
    memcpy(&atapi_cmd.pkt[2], &sector, 4);

    atapi_cmd.pkt[9] = 0xf8; // sync field: 1 , Header code: 11 User data: 1 EDC & ECC: 1 Error Flag: 00 Reserved: 0

    atapi_cmd.pktlen = 12;
    atapi_cmd.blocks = n_sectors;
    atapi_cmd.block_size = sector_size;
    atapi_cmd.proto = 3;
    atapi_cmd.in_out = 1;


    while(true)
    {
        ret = sys_storage_send_device_cmd(bdvd_id, 1, &atapi_cmd, sizeof(struct lv2_atapi_cmnd_block), buffer, sector_size * (u32) n_sectors);
        if(((u32) ret) != 0x8001000A) break;
    }

    return ret;
}

int get_toc_bdvd(void *buffer, u32 size)
{
    int ret;

    if(bdvd_id < 0) return FAILED;

    memset(&atapi_cmd, 0, sizeof(struct lv2_atapi_cmnd_block));

    atapi_cmd.pkt[0] = 0x43;
    atapi_cmd.pkt[2] = 0;
    atapi_cmd.pkt[6] = 1;

    atapi_cmd.pkt[7] = (size>>8) & 0xFF;
    atapi_cmd.pkt[8] = size & 0xFF;


    atapi_cmd.pktlen = 12;
    atapi_cmd.blocks = 1;
    atapi_cmd.block_size = size;
    atapi_cmd.proto = 3;
    atapi_cmd.in_out = 1;

    while(true)
    {
        ret = sys_storage_send_device_cmd(bdvd_id, 1, &atapi_cmd, sizeof(struct lv2_atapi_cmnd_block), buffer, size);
        if(((u32) ret) != 0x8001000A) break;
    }

    return ret;
}


int load_unload_bdvd(int mode)
{
    int ret = -2;

    if(bdvd_id < 0) return ret;

    memset(&atapi_cmd, 0, sizeof(struct lv2_atapi_cmnd_block));

    atapi_cmd.pkt[0] = 0x1b;
    atapi_cmd.pkt[1] = (mode & 128) != 0;
    atapi_cmd.pkt[4] =(mode & 3);


    atapi_cmd.pktlen = 12;
    atapi_cmd.blocks = 0;
    atapi_cmd.block_size = 0;
    atapi_cmd.proto = 0;
    atapi_cmd.in_out = 1;

    while(true)
    {
        ret = sys_storage_send_device_cmd(bdvd_id, 1, &atapi_cmd, sizeof(struct lv2_atapi_cmnd_block), NULL, 0);
        if(((u32) ret) != 0x8001000A) break;
    }

    return ret;
}

void Eject_BDVD(int mode)
{
    if(bdvd_id < 0 && sys_storage_open(BD_DEVICE, &bdvd_id) < 0) return;

#ifdef PSDEBUG
    int ret=
#endif
    load_unload_bdvd(mode);

    close_bdvd();
}

void LoadPSXOptions(char *path)
{   int size;
    char *mem = NULL;

    psx_modified = false;
    memset(&psx_options, 0, sizeof(psx_opt));

    psx_options.version = 1;
    strncpy(psx_options.mc1, "No Memory Card", 256);
    strncpy(psx_options.mc2, "No Memory Card", 256);

    psx_options.flags = default_psxoptions;

    PSX_LAST_PATH = path;

    if(path)
    {
        int flen = strlen(path) - 4;

        if(flen < 0 || strcasestr(".iso|.bin|.mdf|.img", path + flen) == NULL)
            sprintf(tmp_path, "%s/psx_config.cfg", path);
        else
        {
            strcpy(tmp_path, path);
            strcpy(tmp_path + flen, ".cfg");
        }

        mem = LoadFile(tmp_path, &size);
    }

    if(!mem || size != sizeof(psx_opt))
    {   // get psx options by default
        if(mem) free(mem);

        sprintf(tmp_path, "%s/USRDIR/psx_config.bin", self_path);

        if(file_exists(tmp_path)==false)
        {
            sprintf(tmp_path, "%s/config/psx_config.bin", self_path);
        }

        if(!file_exists(tmp_path)) return;

        if(path) psx_modified = true;
        mem = LoadFile(tmp_path, &size);
    }

    if(mem && size == sizeof(psx_opt)) memcpy(&psx_options, mem, sizeof(psx_opt));
    if(mem) free(mem);
}

int SavePSXOptions(char *path)
{
    psx_modified = false;

    sprintf(temp_buffer, "%s/USRDIR/psx_config.bin", self_path);

    if(path)
    {
        int flen = strlen(path) - 4;

        if(flen < 0 || strcasestr(".iso|.bin|.mdf|.img", path + flen) == NULL)
            sprintf(temp_buffer, "%s/psx_config.cfg", path);
        else
        {
            strcpy(temp_buffer, path);
            strcpy(temp_buffer + flen, ".cfg");
        }
    }
    else if(file_exists(temp_buffer)==false)
        sprintf(temp_buffer, "%s/config/psx_config.bin", self_path);

    return SaveFile(temp_buffer, (char *) &psx_options, sizeof(psx_opt));
}

static int mode_dump = 0;

void copy_PSX_game_from_CD()
{
    char output[256];
    int disc = 1;
    int aborted = 0;
    u64 total_sector;
    u32 sector_size;

    u32 sector = 0;

    char * buffer = memalign(16, 1024 * 0x940);

    FILE *fp = NULL;

    while(true)
    {
        memset(temp_buffer, 0, 512);
        memset(output, 0, 64);
        if(Get_OSK_String_no_lang(language[DRAWPSX_PUTFNAME], temp_buffer, 63) == SUCCESS)
        {
            utf8_truncate(temp_buffer, output, 63);
            if(strlen(output)<3) DrawDialogOKTimer(language[DRAWPSX_FMUSTB], 2000.0f);
            else break;
        }
        else goto end;
    }


    for(disc = 1; disc < 9; disc++)
    {
        int ret;
        int f;

        sprintf(temp_buffer + 1024, "%s#%i",language[DRAWPSX_PUTADISC], disc);
        while((ret = DrawDialogYesNo2(temp_buffer + 1024)) == YES)
        {
            //Reset_BDVD(); //  in the callback!

            for(f = 0; f < 15; f++)
            {
                if(get_disc_ready()) break;
                sleep(1);
            }

            if(get_psx_region_cd() & 0x10) break;  else DrawDialogOKTimer(language[DRAWPSX_UNREC], 2000.0f);
        }

        if(ret != 1) goto end;

        ret = open_bdvd(&total_sector, &sector_size);
        if(ret < 0) {DrawDialogOKTimer(language[DRAWPSX_ERROPENING], 2000.0f); goto end;}

        mkdir_secure("/dev_hdd0/PSXGAMES");

        sprintf(temp_buffer + 1024, "/dev_hdd0/PSXGAMES/%s", temp_buffer);

        if(mkdir_secure(temp_buffer + 1024) < 0)
        {
            if(disc == 0 && DrawDialogYesNo(language[DRAWPSX_ASKEFOLDER]) == 2) goto end;
        }



        if(get_toc_bdvd(buffer, 4) == 0) { // get and save toc
            int nlen = ((buffer[0]<<8) | buffer[1]) + 2;
            if(get_toc_bdvd(buffer, nlen) == 0)
            {
                sprintf(temp_buffer + 1024, "/dev_hdd0/PSXGAMES/%s/disc%i.toc", temp_buffer, disc);
                SaveFile(temp_buffer + 1024, buffer, nlen);
            }

        }

        sprintf(temp_buffer + 1024, "/dev_hdd0/PSXGAMES/%s/disc%i.img", temp_buffer, disc);

        struct stat s;

        if(stat(temp_buffer + 1024, &s) >= 0)
        {
            sprintf(temp_buffer + 2048, "disc%i.img %s", disc, language[DRAWPSX_ISOEXITS]);
            if(DrawDialogYesNo2(temp_buffer + 2048) == YES)
            {
                close_bdvd();
                continue;
            }
        }

        DCls();
        DbgDraw();
        tiny3d_Flip();
        DbgDraw();
        tiny3d_Flip();

        u32 blockSize;
        u64 freeSize;
        u64 free_hdd0;
        sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
        free_hdd0 = ( ((u64)blockSize * freeSize));

        if((((s64) total_sector * 0x930LL) + 0x40000000LL) >= (s64) free_hdd0)
        {
            sprintf(temp_buffer + 1024, "%s\n\n%s%1.2f GB", "Error: no space in HDD0 to copy it", "You need ",
                ((double) (((s64) total_sector * 0x930LL) + 0x40000000LL - free_hdd0)) / GIGABYTES);
            DrawDialogOK(temp_buffer + 1024);
            goto end;
        }

        fp = fopen(temp_buffer + 1024, "wb");
        if(!fp) goto end;

        sector = 0;
        while(true)
        {

            s64 to_read;
            to_read = ((s64) total_sector) - ((s64)  (sector + 16));

            if(to_read > 1024) to_read = 1024;
            if(to_read <= 0) break; // dump ends

            sprintf(temp_buffer + 1024, "Disc #%i. Dumping sector: %u of %lu", disc, sector, total_sector);
            DbgHeader(temp_buffer + 1024);
            DbgMess("Press TRIANGLE or CIRCLE to abort");

            DbgDraw();
            tiny3d_Flip();
            pad_last_time = 0;
            ps3pad_read();

            if(new_pad & (BUTTON_TRIANGLE | BUTTON_CIRCLE_)) {aborted = 1; break;}

            if(read_raw_sector_bdvd(sector, buffer, 0x940, (u32) to_read) >= 0)
            {
                int n;

                if(!mode_dump)
                    for(n = 0; n < (u32) to_read; n++) memcpy(buffer + 0x930 * n, buffer + 0x940 * n, 0x930); // convert 0x940 to 0x930 (2352) bytes

                if(fwrite((void *) buffer, 0x930 + 0x10 * mode_dump, (u32) to_read, fp) != (u32) to_read)
                {
                    DPrintf("Error writing sectors %u to %u\n", sector, sector + (u32) to_read - 1);
                    goto end;
                }
            }
            else
            {
                int n;
                int f;

                // fine read
                for(n = 0; n < (u32) to_read; n++)
                {
                    for(f = 0; f < 5; f++)
                    {
                        // retry
                        if(read_raw_sector_bdvd(sector + n, buffer + 0x940 * n, 0x940, 1) < 0)
                        {
                            if(f==4) DPrintf("Error reading sector %u\n", sector + n);
                        }
                        else break;
                    }
                }

                // writing data always
                if(!mode_dump)
                    for(n = 0; n < (u32) to_read; n++) memcpy(buffer + 0x930 * n, buffer + 0x940 * n, 0x930); // convert 0x940 to 0x930 (2352) bytes

                if(fwrite((void *) buffer, 0x930 + 0x10 * mode_dump, (u32) to_read, fp) != (u32) to_read)
                {
                    DPrintf("Error writing sectors %u to %u\n", sector, sector + (u32) to_read - 1);
                    goto end;
                }

            }

            sector+= (u32) to_read;
        }

        if(aborted) break;

        if(fp) fclose(fp); fp = NULL;

        while(true)
        {
            sprintf(temp_buffer + 1024, "Disc #%i. Copied %u sectors", disc, sector);
            DbgHeader(temp_buffer + 1024);
            DbgMess("Press CROSS to continue");

            DbgDraw();
            tiny3d_Flip();

            ps3pad_read();

            if(new_pad & BUTTON_CROSS_) {break;}
        }

        if(DrawDialogYesNo("Do you want to copy a New Disc?") != YES) break;
        close_bdvd();

    }


   end:
    if(fp) fclose(fp);

    free(buffer);

    close_bdvd();

    if(aborted)
    {
        if((disc > 1) && DrawDialogYesNo("Do you want to delete all discs copied to hdd?") == YES)
        {
            sprintf(temp_buffer + 1024, "/dev_hdd0/PSXGAMES/%s", temp_buffer);
            DeleteDirectory(temp_buffer + 1024);
            rmdir_secure(temp_buffer + 1024);
        }
        else
        {
            sprintf(temp_buffer + 1024, "/dev_hdd0/PSXGAMES/%s/disc%i.img", temp_buffer, disc);
            unlink_secure(temp_buffer + 1024);
        }
    }
}
