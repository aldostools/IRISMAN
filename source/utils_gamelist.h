void sort_entries(t_directories *list, int *max)
{
    int n,m;
    int fi = (*max);
    t_directories swap;

    for(n = 0; n < (fi - 1); n++)
        for(m = n + 1; m < fi; m++)
        {
            if((strcasecmp(list[n].title, list[m].title) > 0  && ((list[n].flags | list[m].flags) & D_FLAG_BDVD) == 0) ||
              ((list[m].flags & D_FLAG_BDVD) && n == 0))
            {
                    swap = list[n], list[n] = list[m], list[m] = swap;
            }
        }
}

void sort_entries2(t_directories *list, int *max, u32 mode)
{
    int n, m;
    int fi = (*max);
    t_directories swap;

    if(mode > 2) mode = 0;

    if(mode_homebrew != GAMEBASE_MODE)
    {
        for(n = 0; n < (fi - 1); n++)
            for(m = n + 1; m < fi ;m++)
            {
                if( (((list[n].flags & D_FLAG_HOMEB_MKV) != 0) &&  (list[m].flags & D_FLAG_HOMEB_MKV) == 0) ||
                    (((list[n].flags & D_FLAG_HOMEB_MKV) != 0) == ((list[m].flags & D_FLAG_HOMEB_MKV) != 0) && strcasecmp(list[n].title, list[m].title) > 0) )
                {
                        swap = list[n], list[n] = list[m], list[m] = swap;
                }
            }

        return;
    }

    if(mode == 0) {sort_entries(list, max); return;}

    for(n = 0; n < (fi - 1); n++)
        for(m = n + 1; m < fi; m++)
        {
            if((mode == 1 && (list[n].flags & (PS1_FLAG)) != (list[m].flags & (PS1_FLAG)) && (list[n].flags & (PS1_FLAG)) != 0) ||
               (mode == 2 && (list[n].flags & (PS1_FLAG)) != (list[m].flags & (PS1_FLAG)) && (list[n].flags & (PS1_FLAG)) == 0) ||
               (mode      && (list[n].flags & (PS1_FLAG)) == (list[m].flags & (PS1_FLAG)) && strcasecmp(list[n].title, list[m].title) > 0 &&
                                                                                         ((list[n].flags | list[m].flags) & D_FLAG_BDVD) == 0) ||
               ((list[m].flags & D_FLAG_BDVD) && n == 0))
            {
                    swap = list[n], list[n] = list[m], list[m] = swap;
            }
        }
}

bool icon_exists(char *title_id, t_directories *list, int max)
{
    int i;

    for(i = 0; i < max; i++)
    {
        if(strncmp(list[i].title_id, title_id, 9) == SUCCESS) break;
    }

    return (i < max);
}

bool add_homebrew_icon(t_directories *list, int *max, char *title_id, char *spath)
{
    if(icon_exists(title_id, list, (*max)) == false)
    {
        char filepath[MAXPATHLEN];
        char title_name[64] = {};

        if(strncmp(spath, "/dev_", 5) == SUCCESS)
            strcpy(filepath, spath);
        else if(strncmp(title_id, "BLES80616", 9) == SUCCESS)
        {
            sprintf(filepath, "%s/USRDIR/prepNTFS.self", self_path);
            sprintf(title_name, "Prepare NTFS Drives for webMAN");
        }
        else if(strncmp(title_id, "PRXLOADER", 9) == SUCCESS)
        {
            sprintf(filepath, "%s/USRDIR/prxloader.self", self_path);
            sprintf(title_name, "PRX Loader");
        }
        else if(strncmp(title_id, "CDGPLAYER", 9) == SUCCESS)
        {
            sprintf(filepath, "%s/USRDIR/CDGPlayer.self", self_path);
            sprintf(title_name, "CDG Player");
        }
        else if(strncmp(title_id, "GMPADTEST", 9) == SUCCESS)
        {
            sprintf(filepath, "%s/USRDIR/Testpad.self", self_path);
            sprintf(title_name, "GamePad Test");
        }
        else
            sprintf(filepath, "/dev_hdd0/game/%s/USRDIR/%s", title_id, spath);

        if(file_exists(filepath) && ndirectories < MAX_DIRECTORIES)
        {
            int ret = SUCCESS;

            if(title_name[0] == 0)
            {
                char paramsfo[MAXPATHLEN];
                sprintf(paramsfo, "/dev_hdd0/game/%s/PARAM.SFO", title_id);
                ret = parse_param_sfo(paramsfo, title_name);
            }

            if(ret == SUCCESS)
            {
                list[*max].flags = HOMEBREW_FLAG;
                list[*max].splitted = 0;

                sprintf(list[*max].title_id, title_id);
                sprintf(list[*max].path_name, filepath);

                sprintf(list[*max].title, title_name);
                if(list[*max].title[0]) (*max) ++;
                return true;
            }
        }
    }

    return false;
}

void add_custom_icons(t_directories *list, int *max)
{
    char stself[MAXPATHLEN];

    //-- add Internet Browser icon
    if(mode_homebrew == HOMEBREW_MODE || game_list_category == GAME_LIST_ALL)
    {
        sprintf(stself, "%s/USRDIR/browser.self", self_path);
        if(add_homebrew_icon(list, max, "NETBROWSE", stself)) sprintf(list[*max].title, "Internet Browser");
    }

    //-- add Showtime icon
    if(mode_homebrew == VIDEOS_MODE || mode_homebrew == HOMEBREW_MODE || game_list_category == GAME_LIST_ALL)
    {
        sprintf(stself, "%s/USRDIR/SHOWTIME.SELF", self_path);
        if(file_exists(stself))
            add_homebrew_icon(list, max, "HTSS00003", stself); // Showtime
        else
            add_homebrew_icon(list, max, "HTSS00003", "showtime.self");

    }

    if(mode_homebrew == HOMEBREW_MODE)
    {
        char title_id[10];

        int custom_homebrews_len = strlen(custom_homebrews);

        for(int i = 0; i < custom_homebrews_len; i+=10)
        {
            strncpy(title_id, custom_homebrews + i, 9);

            if(strncmp(title_id, "BLES80608", 9) == SUCCESS)
            {
                if(!use_mamba) add_homebrew_icon(list, max, title_id, "RELOAD.SELF"); // multiMAN
            }
            else if(strncmp(title_id, "BLES80616", 9) == SUCCESS)
                add_homebrew_icon(list, max, title_id, "prepNTFS.self");
            else if(strncmp(title_id, "SSNE10000", 9) == SUCCESS)
                add_homebrew_icon(list, max, title_id, "cores/snes9x_next_libretro_ps3.SELF"); // RetroArch
            else if(strncmp(title_id, "IRISMAN00", 9) == SUCCESS) ;
            else if(strncmp(title_id, "IMANAGER4", 9) == SUCCESS)
                add_homebrew_icon(list, max, title_id, "iris_manager.self"); // Iris Manager
            else if(strncmp(title_id, "MAME90000", 9) == SUCCESS)
                add_homebrew_icon(list, max, title_id, "frontend.self");
            else if(strncmp(title_id, "PSID81257", 9) == SUCCESS)
                add_homebrew_icon(list, max, title_id, "ps1_emu.self");
            else
                add_homebrew_icon(list, max, title_id, "RELOAD.SELF"); // default RELOAD.SELF
        }
    }
    //-- end of custom icons
}

int delete_custom_icons(t_directories *list, int *max)
{
    int n, deleted = 0;

    n = 0;
    while(n < (*max) && (*max) > 0)
    {
        if((list[n].title_id[0] > 0) && (strstr(custom_homebrews, list[n].title_id) != NULL))
        {
            deleted++;

            if((*max) > 1)
            {
                list[n] = list[(*max) - 1];

                (*max) --;
            }
            else if((*max) == 1)
            {
                (*max) --;

                break;
            }

            if(deleted == 2) break;
        }
        else
            n++;
    }

    if((*max) >= MAX_DIRECTORIES) return deleted;

    list[(*max)].flags = 0;
    list[(*max)].title[0] = 0;
    list[(*max)].title_id[0] = 0;
    list[(*max)].path_name[0] = 0;

    for(n = (*max); n < MAX_DIRECTORIES; n++)
        list[n] = list[(*max)];

    return deleted;
}

int delete_entries(t_directories *list, int *max, u32 flag)
{
    int n, deleted = 0;

    n = 0;
    flag &= GAMELIST_FILTER; // filter entries
    while(n < (*max) && (*max) > 0)
    {
        if((list[n].flags & flag) || (list[n].flags == flag) || list[n].path_name[0] == 0)
        {
            deleted++;

            if((*max) > 1)
            {
                list[n] = list[(*max) - 1];

                (*max) --;
            }
            else if((*max) == 1)
            {
                (*max) --;

                break;
            }
        }
        else
            n++;
    }

    if((*max) >= MAX_DIRECTORIES) return deleted;

    list[(*max)].flags = 0;
    list[(*max)].title[0] = 0;
    list[(*max)].title_id[0] = 0;
    list[(*max)].path_name[0] = 0;

    for(n = (*max); n < MAX_DIRECTORIES; n++)
        list[n] = list[(*max)];

    return deleted;
}

void fill_psx_iso_entries_from_device(char *path, u32 flag, t_directories *list, int *max)
{
    if(*max >= MAX_DIRECTORIES || scan_canceled) return;

    sysFSDirent dir;
    DIR_ITER *pdir = NULL;
    struct stat st;

    bool is_ntfs = is_ntfs_path((char *) path);

    if(!is_ntfs) sysFsChmod(path, FS_S_IFDIR | 0777);

    pdir = ps3ntfs_diropen(path); if(!pdir) return;

    while(ps3ntfs_dirnext(pdir, dir.d_name, &st) == SUCCESS)
    {
        if(*max >= MAX_DIRECTORIES) break;

        if(ps3pad_poll()) break;

        if(dir.d_name[0]=='.' && (dir.d_name[1] == 0 || dir.d_name[1] == '.')) continue;

        if(!S_ISDIR(st.st_mode)) continue;

        if(filter_by_letter)
        {
           char c1[1], c2[1]; sprintf(c1, "%c", dir.d_name[0]); sprintf(c2, "%c", (47 + filter_by_letter)); if(strcasecmp(c1, c2)) continue;
        }

        list[*max].flags = flag | PS1_FLAG;
        list[*max].splitted = 0;

        sprintf(list[*max].path_name, "%s/%s", path, dir.d_name);

        dir.d_name[63]=0;
        sprintf(list[*max].title, "%s", dir.d_name);
        sprintf(list[*max].title_id, "%s", dir.d_name);

        if(!(flag & HDD0_FLAG))
        {
            // is not HDD
            char file2[MAX_PATH_LEN];

            sprintf(file2, "%s/VM1/%s/Internal_MC.VM1", self_path, list[*max].title);
            if(file_exists(file2))
            {
                u64 size = get_filesize(file2);

                if(size > 0 && file_exists(list[*max].path_name))
                {
                    char file3[MAX_PATH_LEN];
                    sprintf(file3, "%s/Internal_MC.VM1", list[*max].path_name);
                    unlink_secure(file3);

                    if(copy_async_gbl(file2, file3, size, "Copying Memory Card to USB device...", NULL) < 0)
                    {
                        unlink_secure(file3); // delete possible truncated file
                        DrawDialogOK("Error copying the Memory Card to USB device");
                    }
                    else unlink_secure(file2);

                    sprintf(file2,"%s/VM1/%s", self_path, list[*max].title);
                    rmdir_secure(file2);
                }
            }
        }

        if(list[*max].title[0]) (*max) ++;
    }

    ps3ntfs_dirclose(pdir);
}

int fill_iso_entries_from_device(char *path, u32 flag, t_directories *list, int *max, unsigned long ioType)
{
    if(*max >= MAX_DIRECTORIES || scan_canceled) return 0;

    char *mem = malloc(1024);

    if(!mem) return 0;

    bool is_ps3_iso = (strstr(path, "/PS3ISO")!=NULL);
    bool is_psx_iso = (strstr(path, "/PSXISO")!=NULL);
    bool is_psp_iso = (strstr(path, "/PSPISO")!=NULL);
    bool is_ps2_iso = (strstr(path, "/PS2ISO")!=NULL);

    bool is_bd_iso  = (strstr(path, "/BDISO" )!=NULL);
    bool is_dvd_iso = (strstr(path, "/DVDISO")!=NULL);

    bool is_psp = (flag & (PSP_FLAG | RETRO_FLAG)) == (PSP_FLAG | RETRO_FLAG);
    bool is_retro = is_psp && (retro_root_path[0] > 0) && (strstr(path, retro_root_path) != NULL);
    bool is_ps2_classic = is_psp && !is_retro &&
                          ((ps2classic_path[0] > 0) && strstr(path, ps2classic_path) != NULL);

    char wm_path[MAX_PATH_LEN];
    bool use_wmtmp =  file_exists("/dev_hdd0/tmp/wmtmp");
    bool use_wmtmp_ps3 = ( use_wmtmp && is_ps3_iso );

    bool is_rx_video = (strcmp(path, "/dev_hdd0/game/RXMOV")==SUCCESS); if(is_rx_video) strcpy(path, "/dev_hdd0/game\0");

    sysFSDirent dir;
    DIR_ITER *pdir = NULL;
    struct stat st;

    bool is_ntfs = is_ntfs_path((char *) path);

    pdir = ps3ntfs_diropen(path); if(!pdir) goto exit_function;

    if(!is_ntfs) sysFsChmod(path, FS_S_IFDIR | 0777);

    while(ps3ntfs_dirnext(pdir, dir.d_name, &st) == SUCCESS)
    {
        if(*max >= MAX_DIRECTORIES) break;

        if(ps3pad_poll()) break;

        if(dir.d_name[0]=='.' && (dir.d_name[1]==0 || dir.d_name[1]=='.')) continue;

        if(S_ISDIR(st.st_mode))
        {
            if(is_retro) continue;
            if(is_rx_video && strncmp(dir.d_name, "RXMOV", 5)!=SUCCESS) continue;

            int len = strlen(path);
            strcat(path,"/");
            strcat(path, dir.d_name);
            fill_iso_entries_from_device(path, flag, list, max, ioType);
            path[len] = 0;
            continue;
        }

        if(is_retro)
        {
            if(roms_count >= max_roms) break;
            if(!is_retro_file(path, dir.d_name)) continue;
        }
        else if(flag & (D_FLAG_HOMEB))
        {
            if(is_audiovideo(get_extension(dir.d_name)) == false &&
               strcmpext(dir.d_name, ".iso") && strcmpext(dir.d_name, ".iso.0")) continue;
        }
        else if(is_ps2_classic)
        {
            if(strcmpext(dir.d_name, ".bin.enc")) continue;
        }
        else if(((flag & (PS2_FLAG)) == (PS2_FLAG)) || ((flag & (PSP_FLAG)) == (PSP_FLAG)))
        {
            int flen = strlen(dir.d_name) - 4;

            if(flen < 0) continue;

            if(strcasestr(".iso|.bin|.mdf|.img|.zip|.rar", dir.d_name + flen) == NULL)
            {
                if(strcmpext(dir.d_name, ".7z")) continue;
            }
        }
        else if(flag & (PS1_FLAG))
        {
            int flen = strlen(dir.d_name) - 4;

            if(flen < 0) continue;

            if(strcasestr(".iso|.bin|.mdf|.img|.zip|.rar", dir.d_name + flen) == NULL)
            {
                if(strcmpext(dir.d_name, ".7z")) continue;
            }
        }
        else if(strcmpext(dir.d_name, ".iso") && strcmpext(dir.d_name, ".iso.0") && strcmpext(dir.d_name, ".zip")  && strcmpext(dir.d_name, ".rar") && strcmpext(dir.d_name, ".7z")) continue;

        sprintf(list[*max].path_name, "%s/%s", path, dir.d_name);

        char name[MAX_PATH_LEN];

        if(is_ps3_iso) list[*max].flags = flag | PS3_FLAG;
        else if(is_psx_iso || ((flag & (ISO_FLAGS)) == (PS1_FLAG))) list[*max].flags = flag | (PS1_FLAG);
        else if(is_retro) {list[*max].flags = flag | (RETRO_FLAG); roms_count++;}
        else if(is_ps2_classic) list[*max].flags = flag | (PS2_CLASSIC_FLAG);
        else if(is_psp || is_psp_iso) list[*max].flags = flag | (PSP_FLAG);
        else if(flag & (D_FLAG_HOMEB)) list[*max].flags = flag;
        else if(is_ps2_iso) list[*max].flags = flag | PS2_FLAG;
        else
        {
            int fd = ps3ntfs_open(list[*max].path_name, O_RDONLY, 0777);
            if(fd >= SUCCESS)
            {
                if(ps3ntfs_seek64(fd, 0x810LL, SEEK_SET) != 0x810LL) {ps3ntfs_close(fd); continue;}
                if(ps3ntfs_read(fd, (void *) mem + 256, 10)<10) continue;
                mem[256 + 10] = 0;

                if(ps3ntfs_seek64(fd, 0x8000LL, SEEK_SET) != 0x8000LL) {ps3ntfs_close(fd); continue;}
                if(ps3ntfs_read(fd, (void *) mem + 256, 256)<256) continue;

                if(ps3ntfs_seek64(fd, 0x9320LL, SEEK_SET) != 0x9320LL) {ps3ntfs_close(fd); continue;}
                if(ps3ntfs_read(fd, (void *) mem + 512, 256)<256) continue;

                ps3ntfs_close(fd);

                if(!memcmp((void *) &mem[0x28], "PS3VOLUME", 9)) list[*max].flags = flag | PS3_FLAG;
                else if(!memcmp((void *) &mem[8], "PLAYSTATION", 11) || !memcmp((void *) &mem[512], "PLAYSTATION", 11))
                {
                    list[*max].flags = flag | PS2_FLAG;
                    if(!memcmp((void *) &mem[8], "PLAYSTATION", 11)) memcpy((void *) mem + 256, (void *) &mem[0x28], 10);
                    else memcpy((void *) mem + 256, (void *) &mem[0x220], 10);
                    mem[266] = 0;
                }
                else continue;

            }
            else continue;
        }

        strcpy(name, dir.d_name);

        // remove file extension
        if(!strcmpext(name, ".iso.0"))
            name[strlen(name) - 6] = 0;
        else if(is_ps2_classic && !strcmpext(name, ".bin.enc"))
            name[strlen(name) - 8] = 0;
        else if(strlen(name) > 5 && name[strlen(name) - 5] == '.')
            name[strlen(name) - 5] = 0;  // .????
        else if(strlen(name) > 3 && name[strlen(name) - 3] == '.')
            name[strlen(name) - 3] = 0;  // .??
        else if(strlen(name) > 4)
            name[strlen(name) - 4] = 0;  // ISO/MKV/MP4/MP3/AVI/MPG/FLV/WMV/ASF/etc.

        if(is_ps2_classic && strcmp(name, "ISO") == 0)
        {
            if(strstr(path, "[PS2"))
                strcpy(name, strstr(path, "[PS2"));
            else if(strstr(path, "PS2ISO/"))
                strcpy(name, strstr(path, "PS2ISO/")+7);

            for(u16 p = strlen(name); p > 0; p--) if(name[p]=='/') name[p]=0;
        }

        // cache ICON0 and SFO for webMAN
        if(use_wmtmp_ps3)
        {
            sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s.PNG", name);
            if(file_exists(wm_path)==false)
              ExtractFileFromISO(list[*max].path_name, "/PS3_GAME/ICON0.PNG;1", wm_path);
            sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s.SFO", name);
            if(file_exists(wm_path)==false)
              ExtractFileFromISO(list[*max].path_name, "/PS3_GAME/PARAM.SFO;1", wm_path);
        }
        else if(use_wmtmp && (is_psx_iso || is_dvd_iso || is_bd_iso))
        {
            sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s.jpg", name);
            if(file_exists(wm_path)==false)
            {
                char image_file[MAX_PATH_LEN];
                sprintf(image_file, "%s/%s.jpg", path, name);
                CopyFile(image_file, wm_path);
                if(file_exists(wm_path)==false)
                {
                    sprintf(image_file, "%s/%s.JPG", path, name);
                    CopyFile(image_file, wm_path);
                    if(file_exists(wm_path)==false)
                    {
                        sprintf(wm_path, "/dev_hdd0/tmp/wmtmp/%s.PNG", name);
                        sprintf(image_file, "%s/%s.png", path, name);
                        CopyFile(image_file, wm_path);
                        if(file_exists(wm_path)==false)
                        {
                            sprintf(image_file, "%s/%s.PNG", path, name);
                            CopyFile(image_file, wm_path);
                        }
                    }
                }
            }
        }

        if(use_cobra && is_ntfs && (is_ps3_iso || is_psx_iso || is_dvd_iso || is_bd_iso))
        {
            if(is_ps3_iso) sprintf(extension, "%s", "PS3ISO");
            if(is_psx_iso) sprintf(extension, "%s", "PSXISO");
            if(is_dvd_iso) sprintf(extension, "%s", "DVDISO");
            if(is_bd_iso ) sprintf(extension, "%s", "BDISO" );

            sprintf(ntfs_path, "/dev_hdd0/tmp/wmtmp/%s.ntfs[%s]", name, extension);
            if(file_exists(ntfs_path) == false)
            {
                snprintf(ntfs_path, MAX_PATH_LEN, "%s/%s", path, dir.d_name);

                if(file_exists(ntfs_path)==false) goto cont;

                parts = ps3ntfs_file_to_sectors(ntfs_path, sections, sections_size, MAX_SECTIONS, 1);

                if (parts == MAX_SECTIONS)
                {
                    goto cont;
                }
                else if (parts > 0)
                {
                    uint8_t plugin_args[0x10000];

                    num_tracks = 1;
                    if(is_ps3_iso) emu_mode = EMU_PS3; else
                    if(is_bd_iso ) emu_mode = EMU_BD;  else
                    if(is_dvd_iso) emu_mode = EMU_DVD; else
                    if(is_psx_iso)
                    {
                        emu_mode = EMU_PSX;
                        cue=0;
                        int fd;
                        ntfs_path[strlen(ntfs_path)-3]='C'; ntfs_path[strlen(ntfs_path)-2]='U'; ntfs_path[strlen(ntfs_path)-1]='E';
                        fd = ps3ntfs_open(ntfs_path, O_RDONLY, 0);
                        if(fd<0)
                        {
                            ntfs_path[strlen(ntfs_path)-3]='c'; ntfs_path[strlen(ntfs_path)-2]='u'; ntfs_path[strlen(ntfs_path)-1]='e';
                            fd = ps3ntfs_open(ntfs_path, O_RDONLY, 0);
                        }

                        if(fd >= 0)
                        {
                            int r = ps3ntfs_read(fd, (char *)cue_buf, sizeof(cue_buf));
                            ps3ntfs_close(fd);

                            if (r > 0)
                            {
                                char dummy[64];

                                if (cobra_parse_cue(cue_buf, r, tracks, 100, &num_tracks, dummy, sizeof(dummy)-1) != 0)
                                {
                                    num_tracks=1;
                                    cue = 0;
                                }
                                else
                                    cue = 1;
                            }
                        }
                    }

                    p_args = (rawseciso_args *)plugin_args; memset(p_args, 0, 0x10000);
                    p_args->device = USB_MASS_STORAGE((ioType & 0xff) - '0');
                    p_args->emu_mode = emu_mode;
                    p_args->num_sections = parts;

                    memcpy(plugin_args + sizeof(rawseciso_args), sections, parts * sizeof(uint32_t));
                    memcpy(plugin_args + sizeof(rawseciso_args) + (parts * sizeof(uint32_t)), sections_size, parts * sizeof(uint32_t));

                    if (emu_mode == EMU_PSX)
                    {
                        int max = MAX_SECTIONS - ((num_tracks*sizeof(ScsiTrackDescriptor)) / 8);

                        if (parts == max)
                        {
                            continue;
                        }

                        p_args->num_tracks = num_tracks;
                        scsi_tracks = (ScsiTrackDescriptor *)(plugin_args+sizeof(rawseciso_args)+(2*parts*sizeof(uint32_t)));

                        if (!cue)
                        {
                            scsi_tracks[0].adr_control = 0x14;
                            scsi_tracks[0].track_number = 1;
                            scsi_tracks[0].track_start_addr = 0;
                        }
                        else
                        {
                            for (u8 j = 0; j < num_tracks; j++)
                            {
                                scsi_tracks[j].adr_control = (tracks[j].is_audio) ? 0x10 : 0x14;
                                scsi_tracks[j].track_number = j+1;
                                scsi_tracks[j].track_start_addr = tracks[j].lba;
                            }
                        }
                    }

                    FILE *flistW;
                    sprintf(ntfs_path, "/dev_hdd0/tmp/wmtmp/%s.ntfs[%s]", name, extension);
                    flistW = fopen(ntfs_path, "wb");
                    if(flistW!=NULL)
                    {
                        fwrite(plugin_args, sizeof(plugin_args), 1, flistW);
                        fclose(flistW);
                        sysFsChmod(ntfs_path, 0666);
                    }
                }
            }
        }
cont:
        strncpy(list[*max].title, name, 63);
        list[*max].title[63] = 0;
        list[*max].title_id[0] = 0;
        list[*max].title_id[12] = 0;

        if(flag & (D_FLAG_HOMEB))
        {
            strncpy(list[*max ].title_id, name, 63); // for BD/DVD/MKV
        }
        else if(list[*max].flags & (PS1_FLAG))
        {   // PS2/PS1/PSP
            if(is_psp) goto parse_param_sfo;
            if(is_ps2_classic) goto add_file_to_list;

            parse_iso_titleid(list[*max].path_name, list[*max].title_id);
        }
        else
        {
parse_param_sfo:
            if(is_psp)
                strncpy(list[*max].title_id, mem + 0x373, 63);
            else
                strncpy(list[*max].title_id, mem + 256, 63);

            sysLv2FsChmod(list[*max].path_name, FS_S_IFMT | 0777);

            list[*max].title_id[0] = 0;

            int fd = ps3ntfs_open(list[*max].path_name, O_RDONLY, 0);
            if(fd >= SUCCESS)
            {
                u32 flba;
                u64 size;
                int re;
                char *mem = NULL;

                if(is_psp)
                    re = get_iso_file_pos(fd, "/PSP_GAME/PARAM.SFO", &flba, &size);
                else
                    re = get_iso_file_pos(fd, "/PS3_GAME/PARAM.SFO;1", &flba, &size);

                if(!re && (mem = malloc(size + 16)) != NULL)
                {
                    memset(mem, 0, size + 16);

                    re = ps3ntfs_read(fd, (void *) mem, size);

                    if(re == size)
                    {
                        if(is_psp)
                        {
                            if(mem_parse_param_sfo((u8 *) mem, size, "DISC_ID", list[*max].title_id) == SUCCESS) list[*max].title_id[12] = 0;
                            if(mem_parse_param_sfo((u8 *) mem, size, "TITLE",   list[*max].title   ) == SUCCESS)
                            {
                                if(!strncmp(list[*max].title, "Patched & Uploaded", 18)) strncpy(list[*max].title, name, 63);
                            }
                        }
                        else
                        {
                            if(mem_parse_param_sfo((u8 *) mem, size, "TITLE_ID", list[*max].title_id) == SUCCESS) list[*max].title_id[12] = 0;
                            if(mem_parse_param_sfo((u8 *) mem, size, "TITLE",    list[*max].title   ) == SUCCESS) list[*max].title[63] = 0;
                        }
                    }
                    free(mem);
                }
                ps3ntfs_close(fd);
            }
        }

add_file_to_list:

        if(filter_by_letter)
        {
            char c1[1], c2[1]; sprintf(c1, "%c", list[*max].title[0]); sprintf(c2, "%c", (47 + filter_by_letter)); if(strcasecmp(c1, c2)) continue;
        }


        list[*max].title_id[63] = 0;
        list[*max].splitted = 0;

        if(list[*max].title[0]) (*max) ++;
        if(*max >= MAX_DIRECTORIES) break;
    }

    ps3ntfs_dirclose(pdir);

exit_function:

    free(mem);
    return (*max);
}

void fill_directory_entries_with_alt_path(char *file, int n, char *retro_path, char *alt_path, t_directories *list, int *max, u32 flag)
{
    if(scan_canceled) return;

    file[n] = 0; strcat(file, retro_path);
    fill_iso_entries_from_device(file, flag | RETRO_FLAG, list, max, 0);

    if(scan_canceled) return;

    if(roms_count < max_roms && ((strncmp(file, "/dev_hdd0", 9) == SUCCESS && strcmp(retro_path, alt_path) != SUCCESS) ||
                                 (strncmp(file, "/dev_hdd0", 9) != SUCCESS && strcasecmp(retro_path, alt_path) != SUCCESS)))
    {
        file[n] = 0; strcat(file, alt_path);
        fill_iso_entries_from_device(file, flag | RETRO_FLAG, list, max, 0);
    }
}

int fill_entries_from_device(char *path, t_directories *list, int *max, u32 flag, int sel, bool append)
{
    if(scan_canceled) return FAILED;

    DIR  *dir;
    char file[MAX_PATH_LEN];

    int n;
    int in_progress = 0;
    progress_action2 = 0;
    bar1_countparts = 0.0f;

    sysUtilCheckCallback(); tiny3d_Flip();

    if (append == false) delete_entries(list, max, flag & GAMELIST_FILTER);

    if((*max) < 0) *max = 0;
    if((*max) >= MAX_DIRECTORIES) return FAILED;

    if(sel == GAMEBASE_MODE && (use_cobra || use_mamba) && /*noBDVD == MODE_DISCLESS &&*/ append == false)
    {
        // isos
        strncpy(file, path, MAX_PATH_LEN);
        n = 1; while(file[n] != '/' && file[n] != 0) n++;

        file[n] = 0; strcat(file, "/PS3ISO\0");

        if((game_list_category == GAME_LIST_PS3_ONLY) || (game_list_category == GAME_LIST_ALL))
        {
            fill_iso_entries_from_device(file, flag, list, max, 0);

            file[n] = 0; strcat(file, "/PS3ISO [auto]\0");
            fill_iso_entries_from_device(file, flag, list, max, 0);
        }

        if((game_list_category == GAME_LIST_RETRO) || (game_list_category == GAME_LIST_ALL))
        {
            if((retro_mode == RETRO_ALL) || (retro_mode == RETRO_PSX) || (retro_mode == RETRO_PSALL))
            {
                file[n] = 0; strcat(file, "/PSXISO\0");
                fill_iso_entries_from_device(file, flag | PS1_FLAG, list, max, 0);
                //fill_psx_iso_entries_from_device(file, flag | PS1_FLAG, list, max);

                if(scan_canceled) return FAILED;

                file[n] = 0; strcat(file, "/PSXISO [auto]\0");
                fill_iso_entries_from_device(file, flag | PS1_FLAG, list, max, 0);
                //fill_psx_iso_entries_from_device(file, flag | PS1_FLAG, list, max);
            }

            if((retro_mode == RETRO_ALL) || (retro_mode == RETRO_PS2) || (retro_mode == RETRO_PSALL))
            {
                file[n] = 0; strcat(file, ps2classic_path);
                fill_iso_entries_from_device(file, flag | PS2_CLASSIC_FLAG, list, max, 0);
            }

            if(scan_canceled) return FAILED;

            if(use_cobra && !is_mamba_v2)
            {
                if((retro_mode == RETRO_ALL) || (retro_mode == RETRO_PS2) || (retro_mode == RETRO_PSALL))
                {
                    if(!strncmp(file, "/dev_hdd0", 9))
                    {
                        sprintf(file, "/dev_hdd0/PS2ISO");
                        fill_iso_entries_from_device(file, flag | PS2_FLAG, list, max, 0);

                        if(scan_canceled) return FAILED;

                        sprintf(file, "/dev_hdd0/PS2ISO [auto]");
                        fill_iso_entries_from_device(file, flag | PS2_FLAG, list, max, 0);
                    }
                }

                if(scan_canceled) return FAILED;

                if((retro_mode == RETRO_ALL) || (retro_mode == RETRO_PSP) || (retro_mode == RETRO_PSALL))
                {
                    if(strncmp(file, "/dev_hdd0", 9) != 0)
                    {
                        file[n] = 0; strcat(file, "/ISO\0");
                        fill_iso_entries_from_device(file, flag | PSP_FLAG, list, max, 0);
                    }

                    if(scan_canceled) return FAILED;

                    file[n] = 0; strcat(file, "/PSPISO\0");
                    fill_iso_entries_from_device(file, flag | PSP_FLAG, list, max, 0);
                }
            }

            if(scan_canceled) return FAILED;

            //RETRO
            char cfg_path[MAXPATHLEN];
            sprintf(cfg_path, "%s/USRDIR/cores", self_path);
            if(roms_count < max_roms && file_exists(cfg_path))
            {
                sprintf(cfg_path, "%s/USRDIR/cores/snes-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_SNES) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_snes_path, "/ROMS/snes", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/gba-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_GBA) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_gba_path, "/ROMS/vba", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/gen-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_GEN) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_gen_path, "/ROMS/gen", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/nes-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_NES) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_nes_path, "/ROMS/fceu", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/mame-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_MAME) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_mame_path, "/ROMS/mame", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/fba-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_FBA) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_fba_path, "/ROMS/fba", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/quake-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_QUAKE) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_quake_path, "/ROMS/pak", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/doom-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_DOOM) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_doom_path, "/ROMS/prb", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/pce-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_PCE) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_pce_path, "/ROMS/pce", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/gbc-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_GBC) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_gbc_path, "/ROMS/gbc", list, max, flag);

                    if(roms_count < max_roms)
                       fill_directory_entries_with_alt_path(file, n, retro_gb_path, "/ROMS/gb", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/atari-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_ATARI) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_atari_path, "/ROMS/atari", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/vb-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_VBOY) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_atari_path, "/ROMS/vboy", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/nxe-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_NXE) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_atari_path, "/ROMS/nxe", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/wswan-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_WSWAN) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_wswan_path, "/ROMS/wsw", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/a7800-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_A7800) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_a7800_path, "/ROMS/a7800", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/lynx-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_LYNX) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_lynx_path, "/ROMS/lynx", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/gw-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_GW) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_gw_path, "/ROMS/gw", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/vectrex-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_VECTX) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_vectrex_path, "/ROMS/vectrex", list, max, flag);
                }

                sprintf(cfg_path, "%s/USRDIR/cores/2048-retroarch.cfg", self_path);
                if(((retro_mode == RETRO_ALL) || retro_mode == RETRO_2048) && (roms_count < max_roms) && file_exists(cfg_path))
                {
                    fill_directory_entries_with_alt_path(file, n, retro_2048_path, "/ROMS/2048", list, max, flag);
                }

                if(roms_count) roms_count = max_roms;
            }
        }
    }

    if(scan_canceled) return FAILED;

    if(sel >= HOMEBREW_MODE && (use_cobra || use_mamba) && /*noBDVD == MODE_DISCLESS &&*/ append == false)
    {
        // isos BR-DVD
        strncpy(file, path, MAX_PATH_LEN);
        n = 1; while(file[n] != '/' && file[n]!=0) n++;

        file[n] = 0; strcat(file, "/BDISO\0");
        fill_iso_entries_from_device(file, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | (flag  & GAMELIST_FILTER), list, max, 0);

        file[n] = 0; strcat(file, "/BDISO [auto]\0");
        fill_iso_entries_from_device(file, D_FLAG_HOMEB | D_FLAG_HOMEB_BD | (flag  & GAMELIST_FILTER), list, max, 0);

        file[n] = 0; strcat(file, "/DVDISO\0");
        fill_iso_entries_from_device(file, D_FLAG_HOMEB | D_FLAG_HOMEB_DVD | (flag  & GAMELIST_FILTER), list, max, 0);

        file[n] = 0; strcat(file, "/DVDISO [auto]\0");
        fill_iso_entries_from_device(file, D_FLAG_HOMEB | D_FLAG_HOMEB_DVD | (flag  & GAMELIST_FILTER), list, max, 0);

        file[n] = 0; strcat(file, "/VIDEO\0");
        fill_iso_entries_from_device(file, D_FLAG_HOMEB | D_FLAG_HOMEB_MKV | (flag  & GAMELIST_FILTER), list, max, 0);

        file[n] = 0; strcat(file, "/MOVIES\0");
        fill_iso_entries_from_device(file, D_FLAG_HOMEB | D_FLAG_HOMEB_MKV | (flag  & GAMELIST_FILTER), list, max, 0);

        file[n] = 0; strcat(file, video_path);
        fill_iso_entries_from_device(file, D_FLAG_HOMEB | D_FLAG_HOMEB_MKV | (flag  & GAMELIST_FILTER), list, max, 0);
    }

    if(scan_canceled) return FAILED;

    // add PSX Games
    if((append == false) && (sel == GAMEBASE_MODE) && ((game_list_category == GAME_LIST_ALL) ||
                                                      ((game_list_category == GAME_LIST_RETRO) &&
                                                      ((retro_mode == RETRO_ALL) || (retro_mode == RETRO_PSX) || (retro_mode == RETRO_PSALL)))))
    {
        int n;
        strncpy(file, path, MAX_PATH_LEN);
        n = 1; while(file[n] != '/' && file[n] != 0)  n++;

        file[n] = 0; strcat(file, "/PSXGAMES\0");
        fill_psx_iso_entries_from_device(file, flag, list, max);

        msgDialogAbort();
        msgDialogClose(0);
    }

    if(*max >= MAX_DIRECTORIES)
    {
        msgDialogAbort();
        msgDialogClose(0);
        return FAILED;
    }


    if(!(flag & BDVD_FLAG) && sel != HOMEBREW_MODE &&  game_list_category == GAME_LIST_HOMEBREW) return SUCCESS;

    if(mode_homebrew == VIDEOS_MODE || is_ntfs_path(path))
    {
        msgDialogAbort();
        msgDialogClose(0);
        return SUCCESS;
    }

    dir = opendir(path);
    if(!dir)
    {
        msgDialogAbort();
        msgDialogClose(0);
        return SUCCESS;
    }

    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);
    //strcpy(progress_bar_title, language[GAMELIST_SCANNING1]);

    char pattern[128];

    if(sel == GAMEBASE_MODE)
        strcpy(pattern, "%s/PS3_GAME/PARAM.SFO\0");
    else if(sel == HOMEBREW_MODE)
        strcpy(pattern, "%s/PARAM.SFO\0");
    else
        pattern[0] = 0;

    struct stat s;

    while(true)
    {
        if(*max >= MAX_DIRECTORIES) break;

        struct dirent *entry = readdir(dir);

        if(!entry) break;

        if(ps3pad_poll()) break;

        if(entry->d_name[0] == '.' && (entry->d_name[1] == 0 || entry->d_name[1] == '.')) continue;

        if(!(entry->d_type & DT_DIR)) continue;

        if(!strncmp(entry->d_name, "covers", 6)) continue; // skip global covers folder

        in_progress++; // delayed show progress
        if (in_progress == 10)
            msgDialogOpen2(mdialogprogress, progress_bar_title, progress_callback, (void *) 0xadef0045, NULL);

        list[*max].flags = flag;

        strncpy(list[*max].title, entry->d_name, 63);
        list[*max].title[63] = 0;

        sprintf(list[*max].path_name, "%s/%s", path, entry->d_name);

        if((sel == GAMEBASE_MODE) || (sel == HOMEBREW_MODE))
        {
            // read name in PARAM.SFO
            sprintf(file, pattern, list[*max].path_name);

            //add splitted icon
            if(list[*max].title[0] == '_')
                list[*max].splitted = 1;
            else
                list[*max].splitted = 0;

            //get tittle from sfo
            parse_param_sfo(file, list[*max].title);

            // ignore some games for homebrew
            if((sel == HOMEBREW_MODE && game_category[0] != 'H' && game_category[0] != 'A') &&
                                       (game_category[0] != 'C' && game_category[1] != 'B'))
                continue;

            list[*max].title[63] = 0;

            sprintf(file, "%s/%s", list[*max].path_name, "PS3_DISC.SFB");
            if( (sel == HOMEBREW_MODE) ||
               ((sel == GAMEBASE_MODE) && (parse_ps3_disc((char *) file, list[*max].title_id)<0))) {
                    sprintf(file, pattern, list[*max].path_name);
                    parse_param_sfo_id(file, list[*max].title_id); // build de ID from param.sfo
            }
            list[*max].title_id[63] = 0;

            if(sel == GAMEBASE_MODE)
                sprintf(file, "%s/PS3_GAME/USRDIR/EBOOT.BIN", list[*max].path_name);
            else if(sel == HOMEBREW_MODE)
                sprintf(pattern, "%s/USRDIR/EBOOT.BIN", list[*max].path_name);
        }
        else
        {
            sprintf(file, "%s/EBOOT.BIN", list[*max].path_name);
        }

        if(ps3pad_poll()) break;

        if(stat(file, &s) < 0)  continue;

        if(((*max) & 4) == 4)
        {
            static char string1[256];
            msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);

            sprintf(string1, "%s", list[*max].title);
            msgDialogProgressBarSetMsg(MSG_PROGRESSBAR_INDEX0, string1);

            bar1_countparts = 5 * (double) (*max) / ((double) (*max) + 10) ;
            msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) bar1_countparts);

            DbgDraw();
            tiny3d_Flip();
        }

        if(filter_by_letter)
        {
            char c1[1], c2[1]; sprintf(c1, "%c", list[*max].title[0]); sprintf(c2, "%c", (47 + filter_by_letter)); if(strcasecmp(c1, c2)) continue;
        }

        if(list[*max].title[0]) (*max) ++;
    }

    closedir(dir);
    msgDialogAbort();
    msgDialogClose(0);
    return SUCCESS;
}
