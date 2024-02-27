/***********************************************************************************************************/
/* main.c - mount game                                                                                     */
/***********************************************************************************************************/
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
                        sysProcessExitSpawn2("/dev_hdd0/game/HTSS00003/USRDIR/showtime.self", NULL, NULL, NULL, 0, 1200, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
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

                if(directories[currentgamedir].flags & (BDVD_FLAG | PS3_FLAG | PS1_FLAG))
                {
                    if(!strcmpext(directories[currentgamedir].path_name, ".zip") ||
                       !strcmpext(directories[currentgamedir].path_name, ".rar") ||
                       !strcmpext(directories[currentgamedir].path_name, ".7z"))
                    {
                        char *pos = strrchr(directories[currentgamedir].path_name, '/');
                        if(pos)
                        {
                            *pos = 0;
                            sprintf(tmp_path, "%s", directories[currentgamedir].path_name);
                            extract_file(tmp_path, tmp_path, pos + 1); // extract & mount iso
                        }
                        return r;
                    }
                }

                if((directories[currentgamedir].flags & (BDVD_FLAG | HOMEBREW_FLAG | PS1_FLAG)) == PS1_FLAG)
                {
                    if(!(directories[currentgamedir].flags & PS3_FLAG))
                    {
                        if (strstr(directories[currentgamedir].path_name, "/PSXISO") != NULL)
                            launch_iso_game(directories[currentgamedir].path_name, 0); // mount_game.h
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
                    mount_iso_game(); // Mount using Cobra - main.c
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
                            launch_iso_game(directories[currentgamedir].path_name, EMU_BD); // launch BD Video (mount_game.h)
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
                                launch_iso_build(temp_buffer + 2048, temp_buffer, true); // mount_game.h
                            }
                            else
                            {
                                launch_iso_game(directories[currentgamedir].path_name, EMU_DVD); // launch DVD Video (mount_game.h)
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
