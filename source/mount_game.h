/***********************************************************************************************************/
/* COBRA ISO                                                                                               */
/***********************************************************************************************************/

void fun_exit();
extern bool is_ps3hen;

bool is_retro_file(char *rom_path, char *rom_file)
{
    if(strlen(retro_root_path) > 0 && strcasestr(rom_path, retro_root_path) == NULL) return false;

    sprintf(rom_extension, "%s", get_extension(rom_file));
    if(strlen(rom_extension) < 2) return false;

    strtoupper(rom_extension);
    strcat(rom_extensions, " ");

    return (strlen(rom_extension) > 0 && strcasestr(rom_extensions, rom_extension) != NULL);
}

#ifdef LOADER_MODE
void launch_ps2classic(char *ps2iso_path, char *ps2iso_title) {}
void launch_retro(char *rom_path) {}
void launch_video(char *videofile) {}
#else
void launch_ps2classic(char *ps2iso_path, char *ps2iso_title)
{
    if(file_exists(PS2_CLASSIC_PLACEHOLDER))
    {
        int ret = FAILED;

        if(strcmp(temp_buffer, PS2_CLASSIC_ISO_PATH))
        {
            sprintf(MEM_MESSAGE, "%s\n\nDo you want to copy the PS2 image to PS2 Classics Placeholder?", ps2iso_title);

            unlink_secure("/dev_hdd0/tmp/loadoptical"); //Cobra 8.x

            if(DrawDialogYesNo(MEM_MESSAGE) == YES)
            {
                sprintf(temp_buffer, "%s", ps2iso_path);

                unlink_secure(PS2_CLASSIC_ISO_PATH);
                ret = CopyFile(temp_buffer, PS2_CLASSIC_ISO_PATH);

                if(ret == SUCCESS)
                {
                    if(firmware>=0x465C)
                        SaveFile("/dev_hdd0/classic_ps2", (char *)temp_buffer, 0);

                    temp_buffer[strlen(temp_buffer) - 4] = 0;
                    strcat(temp_buffer, ".png");
                    if(file_exists(temp_buffer))
                    {
                        unlink_secure(PS2_CLASSIC_ISO_ICON);
                        ret = CopyFile(temp_buffer, PS2_CLASSIC_ISO_ICON);
                    }
                    else
                    {
                        temp_buffer[strlen(temp_buffer) - 4] = 0;
                        strcat(temp_buffer, ".PNG");
                        if(file_exists(temp_buffer))
                        {
                            unlink_secure(PS2_CLASSIC_ISO_ICON);
                            ret = CopyFile(temp_buffer, PS2_CLASSIC_ISO_ICON);
                        }
                        else
                        {
                            temp_buffer[strlen(temp_buffer) - 8] = 0;
                            strcat(temp_buffer, ".png");
                            if(file_exists(temp_buffer))
                            {
                                unlink_secure(PS2_CLASSIC_ISO_ICON);
                                ret = CopyFile(temp_buffer, PS2_CLASSIC_ISO_ICON);
                            }
                            else
                            {
                                temp_buffer[strlen(temp_buffer) - 4] = 0;
                                strcat(temp_buffer, ".PNG");
                                if(file_exists(temp_buffer))
                                {
                                    unlink_secure(PS2_CLASSIC_ISO_ICON);
                                    ret = CopyFile(temp_buffer, PS2_CLASSIC_ISO_ICON);
                                }
                            }
                        }
                    }
                }
            }
        }
        else
            ret = SUCCESS;

        if(ret)
            DrawDialogOKTimer("ERROR: PS2 image could not be copied to PS2 Classics Placeholder", 5000.0f);
        else if(DrawDialogYesNoDefaultYes("Do you want to exit to XMB to launch the mounted game with PS2 Classics Placeholder?") == YES)
        {
            // Save game list
            fun_exit();
            SaveGameList();

            exit(0);
        }
    }
    else
        DrawDialogOKTimer("PS2 Classics Placeholder must be installed", 3000.0f);
}

void launch_retro(char *rom_path)
{
     char emu_path[MAXPATHLEN];

     sprintf(emu_path, "%s/USRDIR/EBOOT.BIN", retroarch_path);
     if(file_exists(emu_path) == false)
     {
         DrawDialogOKTimer("ERROR: RetroArch is not installed.\nCannot launch the selected Retro game.", 3000.0f);
         return;
     }

     char src_path[MAXPATHLEN];
     char dst_path[MAXPATHLEN];
     char libretro_rom_path[MAXPATHLEN + 32];

     sprintf(emu_path, "%s/USRDIR/RETRO.self", self_path);
     if(file_exists(emu_path) == false)
     {
         DrawDialogOKTimer("ERROR: RETRO.self was not found.\nCannot launch the selected Retro game.", 3000.0f);
         return;
     }

     // Clean temporary roms path
     sprintf(dst_path, "%s/USRDIR/cores/roms", self_path);
     DeleteDirectory(dst_path);
     mkdir_secure(dst_path);

     // Get rom path
     sprintf(src_path, "%s", rom_path);

     if(is_ntfs_path(rom_path))
     {
         sprintf(dst_path, "%s/USRDIR/cores/roms/%s", self_path, get_filename(src_path));

         unlink_secure(dst_path);
         CopyFile(src_path, dst_path);
     }
     else
         sprintf(dst_path, "%s", rom_path);

     if(file_exists(dst_path))
     {
         // Copy prboom.wad to rom path
         if(strlen(retro_doom_path) > 0 && strstr(rom_path, retro_doom_path))
         {
             char wad_path[MAXPATHLEN];
             strcpy(wad_path, dst_path);

             int p = strlen(wad_path);
             while (p > 0 && wad_path[p] != '/') p--;

             strcpy(wad_path + p, "/prboom.wad");

             if(file_exists((char *)wad_path) == false)
             {
                 sprintf(libretro_rom_path, "%s/USRDIR/cores/system/prboom.wad", self_path);
                 if(file_exists((char *)libretro_rom_path)) CopyFile(libretro_rom_path, wad_path);
             }
         }

        // Set rom path
        sprintf(libretro_rom_path, "libretro_rom_path = \"%s\"\n", dst_path);

        // Select template
        if(strcasestr(src_path, retro_snes_path) != NULL || strcasestr(src_path, "/ROMS/snes/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/snes-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_gba_path) != NULL || strcasestr(src_path, "/ROMS/gba/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/gba-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_gen_path) != NULL || strcasestr(src_path, "/ROMS/gen/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/gen-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_nes_path) != NULL || strcasestr(src_path, "/ROMS/fceu/") != NULL)
        {
            if(!strcmpext(src_path, ".fds"))
                sprintf(src_path, "%s/USRDIR/cores/fds-retroarch.cfg", self_path);
            else
                sprintf(src_path, "%s/USRDIR/cores/nes-retroarch.cfg", self_path);
        }
        else if(strcasestr(src_path, retro_mame_path) != NULL || strcasestr(src_path, "/ROMS/mame/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/mame-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_fba_path) != NULL || strcasestr(src_path, "/ROMS/fba/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/fba-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_doom_path) != NULL || strcasestr(src_path, "/ROMS/prb/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/doom-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_quake_path) != NULL || strcasestr(src_path, "/ROMS/pak/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/quake-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_pce_path) != NULL || strcasestr(src_path, "/ROMS/pce/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/pce-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_gb_path) != NULL || strcasestr(src_path, "/ROMS/gb") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/gbc-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_gbc_path) != NULL || strcasestr(src_path, "/ROMS/snes/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/gbc-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_atari_path) != NULL || strcasestr(src_path, "/ROMS/atari/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/atari-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_vb_path) != NULL || strcasestr(src_path, "/ROMS/voy/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/vb-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_nxe_path) != NULL || strcasestr(src_path, "/ROMS/nxe/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/nxe-retroarch.cfg", self_path);
        else if(strcasestr(src_path, retro_wswan_path) != NULL || strcasestr(src_path, "/ROMS/wsw/") != NULL)
            sprintf(src_path, "%s/USRDIR/cores/wswan-retroarch.cfg", self_path);
        else
        {
            DrawDialogOKTimer("ERROR: Invalid path for Retro game.", 3000.0f);
            return;
        }

        // Remove old retroarch.cfg
        sprintf(dst_path, "%s/USRDIR/retroarch.cfg", self_path);
        unlink_secure(dst_path);

        // Create new retroarch.cfg
        FILE *fp;

        // write rom path
        fp = fopen(dst_path, "w");
        fputs (libretro_rom_path, fp);
        fclose(fp);

        // Load template
        int size;
        char *template_cfg = LoadFile(src_path, &size);

        if(!template_cfg)
        {
            DrawDialogOKTimer("ERROR: Could not load the config file for the emulator.", 3000.0f);
            return;
        }

        // append template
        fp = fopen(dst_path, "a");
        fputs (template_cfg, fp);
        fclose(fp);

        free(template_cfg);

        // Save game list
        fun_exit();
        SaveGameList();

        // call emulator
        sysProcessExitSpawn2(emu_path, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
        exit(0);
     }
     else
         DrawDialogOKTimer("ERROR: Cannot launch the selected Retro game.", 3000.0f);
}

void launch_video(char *videofile)
{
    char flist[MAXPATHLEN];

    sprintf(flist, "%s/USRDIR/TEMP", self_path);
    mkdir_secure(flist);

    sprintf(flist, "%s/USRDIR/TEMP/SHOWTIME.TXT", self_path);
    unlink_secure(flist);

    if(is_ntfs_path(videofile)) return;

    char my_video_file[1024];
    snprintf(my_video_file, 1020, videofile);

    char filename[1024];
    sprintf(filename, "file://%s", my_video_file);

    FILE *fd;

    fd = fopen(flist, "w");
    fputs (filename, fd);
    fclose(fd);

    launch_showtime(true);
}
#endif

void launch_showtime(bool playmode)
{
    char stself[1024];

    if(!playmode)
    {
        sprintf(stself, "%s/USRDIR/TEMP/SHOWTIME.TXT", self_path);
        unlink_secure(stself);
    }

    sprintf(stself, "%s/USRDIR/SHOWTIME.SELF", self_path);

    if(file_exists(stself) == false) sprintf(stself, "/dev_hdd0/game/IRISMAN00/USRDIR/USRDIR/SHOWTIME.SELF");
    if(file_exists(stself) == false) sprintf(stself, "%s/USRDIR/sys/SHOWTIME.SELF", MM_PATH);

    if(file_exists(stself))
    {
        reset_sys8_path_table();
        add_sys8_path_table(MM_PATH, self_path);
        build_sys8_path_table();

        fun_exit();
        SaveGameList();

        sysProcessExitSpawn2((const char*)stself, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
        exit(0);
    }

    if(file_exists(MOVIAN))
    {
        fun_exit();
        SaveGameList();

        sysProcessExitSpawn2((const char*)MOVIAN, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
        exit(0);
    }

    if(file_exists(SHOWTIME))
    {
        fun_exit();
        SaveGameList();

        sysProcessExitSpawn2((const char*)SHOWTIME, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
        exit(0);
    }
}

#ifdef LOADER_MODE
void launch_luaplayer(char *lua_path) {}
int mount_psp_iso(char *path) {return FAILED;}
int launch_iso_game(char *path, int mtype) {return FAILED;}
int launch_iso_game_mamba(char *path, int mtype) {return FAILED;}
int launch_iso_build(char *iso_path, char *src_path, bool run_showtime) {return FAILED;}
#else
void launch_luaplayer(char *lua_path)
{
    sysFSStat stat;

    char luaplayer[1024];
    sprintf(luaplayer, "%s/USRDIR/LuaPlayer.self", self_path);

    if(sysLv2FsStat(luaplayer, &stat) == SUCCESS)
    {
        char temp_lua[1024];
        sprintf(temp_lua, "%s/USRDIR/app.lua", self_path);

        if(strcmp(lua_path, temp_lua))
        {
            unlink_secure(temp_lua);
            CopyFile(lua_path, temp_lua);
        }

        fun_exit();
        SaveGameList();

        sysProcessExitSpawn2((const char*)luaplayer, NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
        exit(0);
    }
}

int mount_psp_iso(char *path)
{
    char icon_path[MAXPATHLEN];
    sprintf(icon_path, "%s/PIC1.PNG", psp_launcher_path);

    unlink_secure((char*) icon_path);
    cobra_unset_psp_umd();

    sprintf(icon_path, "%s/USRDIR/icons/PSP_ICON.PNG", self_path);

    int ret = cobra_set_psp_umd2(path, NULL, (char*)icon_path, 2);

    if(ret)
    {
        DrawDialogOKTimer("PSP image could not be mounted", 2000.0f);
        return FAILED;
    }
    else
    {
        DrawDialogOKTimer("Use PSP Remaster Launcher to play the mounted game", 2500.0f);
        cobra_send_fake_disc_insert_event();

        // Save game list
        fun_exit();
        SaveGameList();

        exit(0);
    }
}

int launch_iso_game(char *path, int mtype)
{
    int type = EMU_BD;

    if(is_audiovideo(get_extension(path)))
    {
        launch_video(path);

        //ntfs
        if(is_audiovideo(path) || !strcmpext(path, ".iso") || !strcmpext(path, ".iso.0"))
            type = EMU_BD;
        else
            return FAILED;
    }

    if (use_cobra)
    {
        cobra_umount_disc_image();
        usleep(4000);
        cobra_send_fake_disc_eject_event();
    }

    if((use_cobra && !is_mamba_v2) &&
       (mtype == EMU_PSP || strstr(path, "/PSPISO/") != NULL || strstr(path, "/ISO/") != NULL) &&
       !strcasecmp(path + strlen(path) - 4, ".iso"))
    {
        mount_psp_iso(path);
    }

    int flen = strlen(path) - 4;

    if((strstr(path, "/PSXISO/") != NULL || strstr(path, "/PSXGAMES/") != NULL) &&
       flen >= 0 && (strcasestr(".iso|.bin|.mdf|.img", path + flen) != NULL))
    {
        ps3pad_read();
        if((use_cobra || use_mamba) && (old_pad & BUTTON_SELECT))
        {
            mtype = EMU_PSX; // Mount using Cobra method if launched with SELECT+X
            goto mount_iso;
        }
        else if(is_ps3hen || is_ntfs_path(path))
        {
            mtype = EMU_PSX; // Mount using Cobra method if PSX ISO is on NTFS/ext
        }
        else
        {
            // Launch PSX ISO directly

            reset_sys8_path_table();

            //syscall36("/dev_bdvd");
            add_sys8_bdvd(NULL, NULL);

            if(lv2peek(0x80000000000004E8ULL) && !use_cobra) syscall_40(1, 0); // disables PS3 Disc-less

            // load PSX options
            sprintf(TEMP_PATH1, "%s", path);
            LoadPSXOptions(TEMP_PATH1);

            if(psx_iso_prepare(TEMP_PATH1, NULL, TEMP_PATH1) == 0)
            {
                return FAILED;
            }

            psx_launch();
            return FAILED;
        }
    }

    if(strstr(path, "/PSXGAMES/") || strstr(path, "/PSXISO/"))
    {
        return launch_iso_game_mamba(path, EMU_PSX);
    }

    if(strstr(path, "/BDISO/"))  return launch_iso_game_mamba(path, EMU_BD);
    if(strstr(path, "/DVDISO/")) return launch_iso_game_mamba(path, EMU_DVD);

    if(use_mamba || mtype == EMU_BD || mtype == EMU_PSX)
    {
        return launch_iso_game_mamba(path, mtype);
    }

mount_iso: ;
    int is_ps2_game = 0;

    uint8_t *plugin_args = malloc(0x20000);

    if(plugin_args)
    {
        struct stat s;

        sprintf((char *) plugin_args, PLUGIN_ISO, self_path);

        if(stat((char *) plugin_args, &s) != SUCCESS) {free(plugin_args); return FAILED;}

        if(mtype >= 0)
        {
            type = mtype;
            is_ps2_game = (type == EMU_PS2_DVD);
        }
        else if(strstr(path, "/PS3ISO"))  type = EMU_PS3;
        else if(strstr(path, "/PS2ISO")) {type = EMU_PS2_DVD; is_ps2_game = 1;}
        else if(strstr(path, "/PSPISO"))  type = EMU_PSP;
        else if(strstr(path, "/PSXISO"))  type = EMU_PSX;
        else if(strstr(path, "/DVDISO"))  type = EMU_DVD;
        else if(strstr(path, "/BDISO"))   type = EMU_BD;
        else
        {
            FILE *fp = NULL;

            if(stat(path, &s) == SUCCESS) fp = fopen(path, "rb");

            if(fp)
            {
                fseek(fp, 0x8000, SEEK_SET);
                fread((void *) plugin_args, 1, 256, fp);

                fseek(fp, 0x9320, SEEK_SET);
                fread((void *) plugin_args + 256, 1, 256, fp);

                fclose(fp);

                if(!memcmp((void *) &plugin_args[8], "PSP GAME", 8)) type = EMU_PSP;
                else if(!memcmp((void *) &plugin_args[1], "BEA01", 5)) type = EMU_BD;
                else if(!memcmp((void *) &plugin_args[0x28], "PS3VOLUME", 9)) type = EMU_PS3;
                else if(!memcmp((void *) &plugin_args[8], "PLAYSTATION", 11) || !memcmp((void *) &plugin_args[256], "PLAYSTATION", 11) )
                {
                    if(is_ntfs_path(path))
                        type = EMU_PSX;
                    else
                    {
                        type = EMU_PS2_DVD; is_ps2_game = 1;
                    }
                }
            }
        }

        if(is_ntfs_path(path))
        {
            uint32_t *sections = malloc(MAX_SECTIONS * sizeof(uint32_t));
            uint32_t *sections_size = malloc(MAX_SECTIONS * sizeof(uint32_t));

            if(plugin_args && sections && sections_size && (type == EMU_PS3 || type == EMU_DVD || type == EMU_BD || type == EMU_PSX))
            {
                rawseciso_args *p_args;

                memset(sections, 0, MAX_SECTIONS * sizeof(uint32_t));
                memset(sections_size, 0, MAX_SECTIONS * sizeof(uint32_t));

                memset(plugin_args, 0, 0x10000);

                int parts = ps3ntfs_file_to_sectors(path, sections, sections_size, MAX_SECTIONS, 1);

                if(!strcmpext(path, ".iso.0"))
                {
                    sprintf(TEMP_PATH2, "%s", path);
                    temp_buffer[TEMP_PATH2_OFFSET + strlen(TEMP_PATH2) - 1] = 0;

                    for (int o = 1; o < 64; o++)
                    {
                        if(parts >= MAX_SECTIONS) break;

                        sprintf(TEMP_PATH1, "%s%i", TEMP_PATH2, o);
                        if(file_exists(TEMP_PATH1) == false) break;

                        parts += ps3ntfs_file_to_sectors(TEMP_PATH1, sections + parts, sections_size + parts, MAX_SECTIONS - parts, 1);
                    }
                }

                if(parts>0 && parts < MAX_SECTIONS)
                {
                    p_args = (rawseciso_args *)plugin_args;
                    p_args->device = USB_MASS_STORAGE(NTFS_Test_Device(&path[1]));
                    p_args->emu_mode = type;
                    p_args->num_sections = parts;
                    p_args->num_tracks = 0;


                    memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * sizeof(uint32_t));
                    memcpy(plugin_args + sizeof(rawseciso_args) + (parts * sizeof(uint32_t)), sections_size, parts * sizeof(uint32_t));

                    cobra_unload_vsh_plugin(0);

                    sprintf(TEMP_PATH, PLUGIN_ISO, self_path);

                    int r = cobra_load_vsh_plugin(0, TEMP_PATH, plugin_args, 0x10000);
                    if (r == 0) {SaveGameList(); fun_exit(); exit(0);}

                    sprintf(MEM_MESSAGE, "error %X loading sprx_iso plugin", r);
                    DrawDialogOK(MEM_MESSAGE);

                }
                else
                {
                    if(parts >= MAX_SECTIONS) DrawDialogOKTimer(".ISO is very fragmented", 2000.0f);
                }

                if(plugin_args) free(plugin_args); plugin_args = NULL;
                if(sections) free(sections);
                if(sections_size) free(sections_size);
            }

        }
        else if((type == EMU_PS3 || type == EMU_DVD || type == EMU_BD || type == EMU_PSX) ||
                (type == EMU_PS2_DVD && strncmp(path, "/dev_usb", 8)))
        {
            if(plugin_args) free(plugin_args); plugin_args = NULL;

            if(is_ps2_game) {unlink_secure("/dev_hdd0/classic_ps2"); unlink_secure("/dev_hdd0/tmp/loadoptical");} //Cobra 8.x

            if(use_mamba)
            {
                //if(is_ps2_game) return FAILED;

                goto mount_with_mamba;
            }

            int ret = FAILED;

            char *files[32];
            int nfiles = 1;

            files[0] = path;
            files[1] = NULL;

            if(!strcmpext(path, ".iso.0"))
            {
                sprintf(TEMP_PATH2, "%s", path);
                temp_buffer[TEMP_PATH2_OFFSET + strlen(TEMP_PATH2) - 1] = 0;

                for (int o = 1; o < 64; o++)
                {
                    files[o] = malloc(1024);
                    if(!files[o]) break;

                    sprintf(files[o], "%s%i", TEMP_PATH2, o);
                    if(file_exists(files[o]) == false) break;

                    nfiles++;
                }
            }

            if(type == EMU_PS3)
                ret = cobra_mount_ps3_disc_image(files, nfiles);
            else if(type == EMU_PS2_DVD)
                ret = cobra_mount_ps2_disc_image(files, nfiles, (TrackDef *) TEMP_PATH1, 1);
            else if(type == EMU_PSX)
                ret = cobra_mount_psx_disc_image(path, (TrackDef *) TEMP_PATH1, 1);
            else if(type == EMU_PSP)
                ret = mount_psp_iso(path);
            else if(type == EMU_DVD)
                ret = cobra_mount_dvd_disc_image(files, nfiles);
            else if(type == EMU_BD)
                ret = cobra_mount_bd_disc_image(files, nfiles);
            else ret = FAILED;

            if (ret == SUCCESS)
            {
                cobra_send_fake_disc_insert_event();
                {SaveGameList(); fun_exit(); exit(0);}
            }

            for(int o = 1; o < 64; o++) if(files[o]) free(files[o]);
        }
        else
mount_with_mamba:
        {
            char *sections = malloc(64 * 0x200);
            uint32_t *sections_size = malloc(64 * sizeof(uint32_t));

            if(plugin_args && sections && sections_size)
            {
                rawseciso_args *p_args;

                memset(sections, 0, 64 * 0x200);
                memset(sections_size, 0, 64 * sizeof(uint32_t));

                memset(plugin_args, 0, 0x10000);

                int parts = 0;

                if(!strcmpext(path, ".iso.0"))
                {
                    for(int o = 0; o < 64; o++)
                    {
                        struct stat s;

                        sprintf(TEMP_PATH2, "%s", path);
                        temp_buffer[TEMP_PATH2_OFFSET + strlen(TEMP_PATH2) - 1] = 0;

                        sprintf(&sections[0x200 * o], "%s%i", TEMP_PATH2, o);

                        if(stat(&sections[0x200 * o], &s) != SUCCESS) {memset(&sections[0x200 * o], 0, 0x200); break;}
                        sections_size[o] = s.st_size / 2048ULL;

                        parts++;
                    }
                }
                else
                {
                    parts = 1;

                    strncpy(&sections[0], path, 0x1ff);
                    sections[0x1ff] = 0;

                    if(stat(&sections[0], &s) != SUCCESS) goto skip_load;
                    sections_size[0] = s.st_size / 2048ULL;
                }

                p_args = (rawseciso_args *)plugin_args;
                p_args->device = USB_MASS_STORAGE(NTFS_Test_Device(&path[1]));
                p_args->emu_mode = type | 1024;
                p_args->num_sections = parts;
                p_args->num_tracks = 0;

                memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * 0x200);
                memcpy(plugin_args + sizeof(rawseciso_args) + (parts * 0x200), sections_size, parts * sizeof(uint32_t));

                cobra_unload_vsh_plugin(0);

                sprintf(TEMP_PATH1, PLUGIN_ISO, self_path);
                int r = cobra_load_vsh_plugin(0, TEMP_PATH1, plugin_args, 0x10000);
                if (r == 0) {SaveGameList(); fun_exit(); exit(0);}

                sprintf(MEM_MESSAGE, "error %X loading sprx_iso plugin", r);
                DrawDialogOK(MEM_MESSAGE);
            }


 skip_load:
            if(plugin_args) free(plugin_args); plugin_args = NULL;
            if(sections) free(sections);
            if(sections_size) free(sections_size);
        }
    }

    if(plugin_args) free(plugin_args); plugin_args = NULL;

    return FAILED;

}

int launch_iso_game_mamba(char *path, int mtype)
{
    int type = EMU_DVD;

    int is_ps2_game = 0;

    uint8_t *plugin_args = malloc(0x20000);

    if(plugin_args)
    {
        struct stat s;

        sprintf((char *) plugin_args, PLUGIN_ISO, self_path);

        if(stat((char *) plugin_args, &s) != SUCCESS) {free(plugin_args); return FAILED;}

        if(mtype >= 0)
        {
            type = mtype;
            is_ps2_game = (type == EMU_PS2_DVD);
        }
        else if(strstr(path, "/PS3ISO"))  type = EMU_PS3;
        else if(strstr(path, "/PS2ISO")) {type = EMU_PS2_DVD; is_ps2_game = 1;}
        else if(strstr(path, "/PSPISO"))  type = EMU_PSP;
        else if(strstr(path, "/PSXISO"))  type = EMU_PSX;
        else if(strstr(path, "/DVDISO"))  type = EMU_DVD;
        else if(strstr(path, "/BDISO"))   type = EMU_BD;
        else
        {
            FILE *fp = NULL;

            if(stat(path, &s) == SUCCESS) fp = fopen(path, "rb");

            if(fp)
            {
                fseek(fp, 0x8000, SEEK_SET);
                fread((void *) plugin_args, 1, 256, fp);

                fseek(fp, 0x9320, SEEK_SET);
                fread((void *) plugin_args + 256, 1, 256, fp);

                fclose(fp);

                if(!memcmp((void *) &plugin_args[8], "PSP GAME", 8)) type = EMU_PSP;
                else if(!memcmp((void *) &plugin_args[1], "BEA01", 5)) type = EMU_BD;
                else if(!memcmp((void *) &plugin_args[0x28], "PS3VOLUME", 9)) type = EMU_PS3;
                else if(!memcmp((void *) &plugin_args[8], "PLAYSTATION", 11) || !memcmp((void *) &plugin_args[256], "PLAYSTATION", 11) )
                {
                    if(is_ntfs_path(path))
                        type = EMU_PSX;
                    else
                    {
                        type = EMU_PS2_DVD; is_ps2_game = 1;
                    }
                }
            }
        }

        if(is_ntfs_path(path))
        {
            uint32_t *sections = malloc(MAX_SECTIONS * sizeof(uint32_t));
            uint32_t *sections_size = malloc(MAX_SECTIONS * sizeof(uint32_t));

            if(plugin_args && sections && sections_size && (type == EMU_PS3 || type == EMU_DVD || type == EMU_BD || type == EMU_PSX))
            {
                rawseciso_args *p_args;

                memset(sections, 0, MAX_SECTIONS * sizeof(uint32_t));
                memset(sections_size, 0, MAX_SECTIONS * sizeof(uint32_t));

                memset(plugin_args, 0, 0x10000);

                int parts = ps3ntfs_file_to_sectors(path, sections, sections_size, MAX_SECTIONS, 1);

                if(!strcmpext(path, ".iso.0"))
                {
                    sprintf(TEMP_PATH2, "%s", path);
                    temp_buffer[TEMP_PATH2_OFFSET + strlen(TEMP_PATH2) - 1] = 0;

                    for (int o = 1; o < 64; o++)
                    {
                        if(parts >= MAX_SECTIONS) break;

                        sprintf(TEMP_PATH1, "%s%i", TEMP_PATH2, o);
                        if(file_exists(TEMP_PATH1) == false) break;

                        parts += ps3ntfs_file_to_sectors(TEMP_PATH1, sections + parts, sections_size + parts, MAX_SECTIONS - parts, 1);
                    }
                }

                if(parts>0 && parts < MAX_SECTIONS)
                {
                    p_args = (rawseciso_args *)plugin_args;
                    p_args->device = USB_MASS_STORAGE(NTFS_Test_Device(&path[1]));
                    p_args->emu_mode = type;
                    p_args->num_sections = parts;
                    p_args->num_tracks = 0;

                    memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * sizeof(uint32_t));
                    memcpy(plugin_args + sizeof(rawseciso_args) + (parts * sizeof(uint32_t)), sections_size, parts * sizeof(uint32_t));

                    cobra_unload_vsh_plugin(0);

                    sprintf(TEMP_PATH1, PLUGIN_ISO, self_path);

                    if (cobra_load_vsh_plugin(0, TEMP_PATH1, plugin_args, 0x10000) == 0) {SaveGameList(); fun_exit(); exit(0);}
                }
                else if(parts >= MAX_SECTIONS) DrawDialogOKTimer(".ISO is very fragmented", 2000.0f);

                if(plugin_args) free(plugin_args); plugin_args = NULL;
                if(sections) free(sections);
                if(sections_size) free(sections_size);
            }
        }
        else if(type == EMU_PS3 || (type == EMU_PS2_DVD && strncmp(path, "/dev_usb", 8))
             || type == EMU_DVD || type == EMU_BD)
        {
            if(is_ps2_game)
            {
                unlink_secure("/dev_hdd0/classic_ps2"); unlink_secure("/dev_hdd0/tmp/loadoptical"); //Cobra 8.x

                if(plugin_args) free(plugin_args); plugin_args = NULL;

                //if(use_mamba) return FAILED;

                int ret;

                char *files[32];
                int nfiles = 1;

                files[0] = path;
                files[1] = NULL;

                if(!strcmpext(path, ".iso.0"))
                {
                    sprintf(TEMP_PATH2, "%s", path);
                    temp_buffer[TEMP_PATH2_OFFSET + strlen(TEMP_PATH2) - 1] = 0;

                    for (int o = 1; o < 64; o++)
                    {
                        files[o] = malloc(1024);
                        if(!files[o]) break;

                        sprintf(files[o], "%s%i", TEMP_PATH2, o);
                        if(file_exists(files[o]) == false) break;

                        nfiles++;
                    }
                }

                if(type == EMU_PS3)
                    ret = cobra_mount_ps3_disc_image(files, nfiles);
                else if(type == EMU_PS2_DVD)
                    ret = cobra_mount_ps2_disc_image(files, nfiles, (TrackDef *) TEMP_PATH1, 1);
                else if(type == EMU_PSX)
                    ret = cobra_mount_psx_disc_image(path, (TrackDef *) TEMP_PATH1, 1);
                else if(type == EMU_PSP)
                    ret = mount_psp_iso(path);
                else if(type == EMU_DVD)
                    ret = cobra_mount_dvd_disc_image(files, nfiles);
                else if(type == EMU_BD)
                    ret = cobra_mount_bd_disc_image(files, nfiles);
                else ret = FAILED;

                if (ret == SUCCESS)
                {
                    cobra_send_fake_disc_insert_event();
                    {SaveGameList(); fun_exit(); exit(0);}
                }

                for(int o = 1; o < 64; o++) if(files[o]) free(files[o]);
            }
            else
            {
                char *sections = malloc(64 * 0x200);
                uint32_t *sections_size = malloc(64 * sizeof(uint32_t));

                if(plugin_args && sections && sections_size)
                {
                    rawseciso_args *p_args;

                    memset(sections, 0, 64 * 0x200);
                    memset(sections_size, 0, 64 * sizeof(uint32_t));

                    memset(plugin_args, 0, 0x10000);

                    int parts = 0;

                    if(!strcmpext(path, ".iso.0"))
                    {
                        for(int o = 0; o < 64; o++)
                        {
                            struct stat s;

                            sprintf(TEMP_PATH2, "%s", path);
                            temp_buffer[TEMP_PATH2_OFFSET + strlen(TEMP_PATH2) - 1] = 0;

                            sprintf(&sections[0x200 * o], "%s%i", TEMP_PATH2, o);

                            if(stat(&sections[0x200 * o], &s) != SUCCESS) {memset(&sections[0x200 * o], 0, 0x200); break;}
                            sections_size[o] = s.st_size / 2048ULL;

                            parts++;
                        }
                    }
                    else
                    {
                        parts = 1;

                        strncpy(&sections[0], path, 0x1ff);
                        sections[0x1ff] = 0;

                        if(stat(&sections[0], &s) != SUCCESS) goto skip_load;
                        sections_size[0] = s.st_size / 2048ULL;
                    }

                    p_args = (rawseciso_args *)plugin_args;
                    p_args->device = USB_MASS_STORAGE(NTFS_Test_Device(&path[1]));
                    p_args->emu_mode = type | 1024;
                    p_args->num_sections = parts;
                    p_args->num_tracks = 0;


                    memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * 0x200);
                    memcpy(plugin_args + sizeof(rawseciso_args) + (parts * 0x200), sections_size, parts * sizeof(uint32_t));

                    cobra_unload_vsh_plugin(0);

                    sprintf(TEMP_PATH1, PLUGIN_ISO, self_path);
                    int r = 0;
                    if ((r = cobra_load_vsh_plugin(0, TEMP_PATH1, plugin_args, 0x10000)) == 0) {SaveGameList(); fun_exit(); exit(0);}

                    sprintf(MEM_MESSAGE, "error %X", r);
                    DrawDialogOK(MEM_MESSAGE);
                }


            skip_load:

                if(plugin_args) free(plugin_args); plugin_args = NULL;
                if(sections) free(sections);
                if(sections_size) free(sections_size);
            }

        }
    }

    if(plugin_args) free(plugin_args); plugin_args = NULL;

    return FAILED;
}

int launch_iso_build(char *iso_path, char *src_path, bool run_showtime)
{
    int type = EMU_DVD; bool is_ntfs_file = strstr(iso_path, "/dev_hdd0/tmp/wmtmp");

    if(run_showtime && is_audiovideo(get_extension(src_path))) launch_video(src_path);

    if(is_audiovideo(get_extension(src_path)) || strstr(src_path, ".pkg") || is_ntfs_file)
    {
        if(run_showtime) launch_video(src_path);

        //ntfs
        type = EMU_BD;
    }

    sprintf(TEMP_PATH, "%s/USRDIR/TEMP", self_path);
    mkdir_secure(TEMP_PATH);

    if(use_cobra)
    {
        cobra_umount_disc_image();
        usleep(4000);
        cobra_send_fake_disc_eject_event();
    }

    int iso_path_len = strlen(iso_path) - 4; if(iso_path_len < 0) return FAILED;

    uint8_t *plugin_args = malloc(0x20000);

    if(plugin_args)
    {
        sprintf((char*)plugin_args, PLUGIN_ISO, self_path);

        if(file_exists((char*)plugin_args) == false) {free(plugin_args); DrawDialogOK("ERROR: ISO Plugin not found"); return FAILED;}

        if(file_exists(src_path) == false) {free(plugin_args); sprintf(MEM_MESSAGE, "ERROR: %s could not be found", src_path); DrawDialogOK(MEM_MESSAGE); return FAILED;}

        u64 size;
        size = get_filesize(src_path);

        char filename[MAXPATHLEN]; //name only
        sprintf(filename, "%s", get_filename(src_path));;
        create_fake_file_iso(iso_path, filename, size);

        if(file_exists(iso_path) == false) {free(plugin_args); sprintf(MEM_MESSAGE, "ERROR: %s could not be found", iso_path); DrawDialogOK(MEM_MESSAGE); return FAILED;}

        int r = FAILED;

        if(is_ntfs_path(src_path))
        {
            uint32_t *sections      = malloc(MAX_SECTIONS * sizeof(uint32_t));
            uint32_t *sections_size = malloc(MAX_SECTIONS * sizeof(uint32_t));

            if(plugin_args && sections && sections_size)
            {
                rawseciso_args *p_args;

                memset(sections, 0, MAX_SECTIONS * sizeof(uint32_t));
                memset(sections_size, 0, MAX_SECTIONS * sizeof(uint32_t));

                memset(plugin_args, 0, 0x10000);

                // create file section
                strncpy((char *) sections, iso_path, 0x1ff);
                ((char *) sections)[0x1ff] = 0;

                size = get_filesize(iso_path);
                if (size == 0) goto skip_load_ntfs;
                sections_size[0] = size / 2048ULL;
                sections[0x200/4] = 0;

                int parts = 1;

                if(parts < MAX_SECTIONS)
                    parts += ps3ntfs_file_to_sectors(src_path, sections + parts + 0x200/4, sections_size + parts, MAX_SECTIONS - parts - 0x200/4, 1);

                if (parts > 0 && parts < (MAX_SECTIONS - 0x200/4))
                {
                    p_args = (rawseciso_args *)plugin_args;
                    p_args->device = USB_MASS_STORAGE(NTFS_Test_Device(&src_path[1]));
                    p_args->emu_mode = type | 0x800;
                    p_args->num_sections = parts;
                    p_args->num_tracks = 0;

                    memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * sizeof(uint32_t) + 0x200);
                    memcpy(plugin_args + sizeof(rawseciso_args) + (parts*sizeof(uint32_t) + 0x200), sections_size, parts * sizeof(uint32_t));

                    if(is_ntfs_file)
                    {
                        // save sectors file
                        iso_path[iso_path_len] = 0; strcat(iso_path, ".ntfs[BDFILE]");

                        int fd = ps3ntfs_open(iso_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                        if(fd >= 0)
                        {
                            if(ps3ntfs_write(fd, (void *) plugin_args, 0x10000)==0x10000) r = SUCCESS;
                            ps3ntfs_close(fd);
                        }
                    }
                    else
                    {
                        cobra_unload_vsh_plugin(0);
                        sprintf(TEMP_PATH, PLUGIN_ISO, self_path);
                        r = cobra_load_vsh_plugin(0, TEMP_PATH, plugin_args, 0x10000);
                    }
                }
                else
                {
                    if(parts >= (MAX_SECTIONS - 0x200/4)) DrawDialogOKTimer(".ISO is very fragmented", 2000.0f);
                }
           skip_load_ntfs:
                if(sections) free(sections);
                if(sections_size) free(sections_size);
            }
        }
        else
        {
            char *sections = malloc(64 * 0x200);
            uint32_t *sections_size = malloc(64 * sizeof(uint32_t));

            if(plugin_args && sections && sections_size)
            {
                rawseciso_args *p_args;

                memset(sections, 0, 64 * 0x200);
                memset(sections_size, 0, 64 * sizeof(uint32_t));

                memset(plugin_args, 0, 0x10000);

                int parts = 2;

                strncpy(&sections[0], iso_path, 0x1ff);
                sections[0x1ff] = 0;

                size = get_filesize(iso_path);
                if (size == 0) goto skip_load;
                sections_size[0] = size / 2048ULL;

                if(file_exists(src_path) == false) goto skip_load;
                strncpy(&sections[0x200], src_path, 0x1ff);
                sections[0x1ff] = 0;
                sections_size[1] = (size + 2047ULL) / 2048ULL;

                p_args = (rawseciso_args *)plugin_args;
                p_args->device = USB_MASS_STORAGE(NTFS_Test_Device(&iso_path[1]));
                p_args->emu_mode = type | 1024;
                p_args->num_sections = parts;
                p_args->num_tracks = 0;

                memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * 0x200);
                memcpy(plugin_args + sizeof(rawseciso_args) + (parts * 0x200), sections_size, parts * sizeof(uint32_t));

                if(is_ntfs_file)
                {
                    // save sectors file
                    iso_path[iso_path_len] = 0; strcat(iso_path, ".ntfs[BDFILE]");

                    int fd = ps3ntfs_open(iso_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    if(fd >= 0)
                    {
                        if(ps3ntfs_write(fd, (void *) plugin_args, 0x10000)==0x10000) r = SUCCESS;
                        ps3ntfs_close(fd);
                    }
                }
                else
                {
                    cobra_unload_vsh_plugin(0);
                    sprintf(TEMP_PATH, PLUGIN_ISO, self_path);
                    r = cobra_load_vsh_plugin(0, TEMP_PATH, plugin_args, 0x10000);
                }
            }

    skip_load:
            if(sections) free(sections);
            if(sections_size) free(sections_size);
        }

        if(plugin_args) free(plugin_args); plugin_args = NULL;

        if(r == SUCCESS)
        {
            char name[65];
            strncpy(name, filename, 64);
            name[64] = 0;

            if(strlen(src_path) > 64)
            {
                // break the string
                int pos = 63 - strlen(get_extension(filename));
                while(pos > 0 && (name[pos] & 192) == 128) pos--; // skip UTF extra codes
                strcpy(&name[pos], get_extension(filename));
            }

            if(run_showtime) {sprintf(TEMP_PATH, "/dev_bdvd/%s", name); launch_video(TEMP_PATH);}
        }

        if(r == 0) return SUCCESS;

        sprintf(MEM_MESSAGE, "error %X loading sprx_iso plugin", r);
        DrawDialogOK(MEM_MESSAGE);

    }
    else
        DrawDialogOK("ERROR: allocating 128KB memory");


    return FAILED;
}
#endif
