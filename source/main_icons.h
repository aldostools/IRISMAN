int LoadPNG(PngDatas *png, const char *filename)
{
    int ret;
    pngData png2;

    if(filename)
    {
        if(is_ntfs_path((char *) filename))
        {
            int file_size = 0;
            char *buff = LoadFile((char *) filename, &file_size);

            if(!buff) return FAILED;

            ret = pngLoadFromBuffer((const void *) buff, file_size, &png2);

            free(buff);
        }
        else
            ret = pngLoadFromFile(filename, &png2);
    }
    else
        ret = pngLoadFromBuffer((const void *) png->png_in, png->png_size, &png2);

    png->bmp_out = png2.bmp_out;
    png->wpitch  = png2.pitch;
    png->width   = png2.width;
    png->height  = png2.height;

    return ret;
}

int LoadJPG(JpgDatas *jpg, char *filename)
{
    int ret;

    jpgData jpg2;

    if(filename)
    {
        if(is_ntfs_path(filename))
        {
            int file_size = 0;
            char *buff = LoadFile((char *) filename, &file_size);

            if(!buff) return FAILED;

            ret = jpgLoadFromBuffer((const void *) buff, file_size, &jpg2);

            free(buff);
        }
        else
            ret = jpgLoadFromFile(filename, &jpg2);
    }
    else ret = jpgLoadFromBuffer((const void *) jpg->jpg_in, jpg->jpg_size, &jpg2);

    jpg->bmp_out = jpg2.bmp_out;
    jpg->wpitch  = jpg2.pitch;
    jpg->width   = jpg2.width;
    jpg->height  = jpg2.height;

    return ret;
}

char tmp_path[MAXPATHLEN];
char path_name[MAXPATHLEN];

const char folder_mode[2][16] = {{"/"},{"/PS3_GAME/"}};

static PngDatas my_png_datas;

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
void Load_PNG_resources() {}
void get_games() {}
int LoadTexturePNG(char * filename, int index) {return SUCCESS;}
int LoadTextureJPG(char * filename, int index) {return SUCCESS;}
int get_icon(char * path, const int num_dir) {return SUCCESS;}
void get_games_2(void *empty) {}
void get_games_3(u64 var) {}
#else
void Load_PNG_resources()
{

    int i;

    for(i = 0; i < MAX_RESOURCES; i++) Png_res[i].png_in = NULL;
    for(i = 0; i < MAX_PICTURES; i++) {Png_iscover[i] = Png_offset[i] = 0; Png_index[i] = i;}

    // datas for PNG from memory

    Png_res[IMG_BLURAY_DISC].png_in   = (void *) bluray_png_bin;
    Png_res[IMG_BLURAY_DISC].png_size = bluray_png_bin_size;

    Png_res[IMG_USB_ICON].png_in   = (void *) usb_png_bin;
    Png_res[IMG_USB_ICON].png_size = usb_png_bin_size;

    Png_res[IMG_MISSING_ICON].png_in   = (void *) missing_png_bin;
    Png_res[IMG_MISSING_ICON].png_size = missing_png_bin_size;

    Png_res[IMG_DIRECT_ICON].png_in   = (void *) direct_png_bin;
    Png_res[IMG_DIRECT_ICON].png_size = direct_png_bin_size;

    Png_res[IMG_FTP_ICON].png_in   = (void *) ftp_png_bin;
    Png_res[IMG_FTP_ICON].png_size = ftp_png_bin_size;

    Png_res[IMG_PS1_DISC].png_in   = (void *) psone_png_bin;
    Png_res[IMG_PS1_DISC].png_size = psone_png_bin_size;

    Png_res[IMG_PS1_ISO].png_in   = (void *) psoneiso_png_bin;
    Png_res[IMG_PS1_ISO].png_size = psoneiso_png_bin_size;

    // file manager icons

    Png_res[IMG_FOLDER_ICON].png_in   = (void *) folder_png_bin;
    Png_res[IMG_FOLDER_ICON].png_size = folder_png_bin_size;

    Png_res[IMG_FILE_ICON].png_in   = (void *) file_png_bin;
    Png_res[IMG_FILE_ICON].png_size = file_png_bin_size;

    Png_res[IMG_PKG_ICON].png_in   = (void *) pkg_png_bin;
    Png_res[IMG_PKG_ICON].png_size = pkg_png_bin_size;

    Png_res[IMG_SELF_ICON].png_in   = (void *) self_png_bin;
    Png_res[IMG_SELF_ICON].png_size = self_png_bin_size;

    Png_res[IMG_IMAGE_ICON].png_in   = (void *) img_png_bin;
    Png_res[IMG_IMAGE_ICON].png_size = img_png_bin_size;

    Png_res[IMG_ISO_ICON].png_in   = (void *) iso_png_bin;
    Png_res[IMG_ISO_ICON].png_size = iso_png_bin_size;

    // end file manager icons

    Png_res[IMG_PS2_ISO].png_in   = (void *) pstwoiso_png_bin;
    Png_res[IMG_PS2_ISO].png_size = pstwoiso_png_bin_size;

    Png_res[IMG_USB_ICON2].png_in   = (void *) usb_png2_bin;
    Png_res[IMG_USB_ICON2].png_size = usb_png2_bin_size;

    Png_res[IMG_DVD_DISC].png_in   = (void *) dvd_png_bin;
    Png_res[IMG_DVD_DISC].png_size = dvd_png_bin_size;

    // load PNG from memory

    for(i = 0; i < 16; i++)
        if(Png_res[i].png_in != NULL) LoadPNG(&Png_res[i], NULL);

    // Default Background Picture
    if(bShowPIC1)
    {
        Png_res[IMG_DEFAULT_BACKGROUND].png_in   = (void *) background_jpg_bin;
        Png_res[IMG_DEFAULT_BACKGROUND].png_size = background_jpg_bin_size;
        LoadJPG((JpgDatas *) &Png_res[IMG_DEFAULT_BACKGROUND], NULL);
    }

    // UMD
    Png_res[IMG_PSP_ISO].png_in   = (void *) pspiso_png_bin;
    Png_res[IMG_PSP_ISO].png_size = pspiso_png_bin_size;
    LoadPNG(&Png_res[IMG_PSP_ISO], NULL);

    // retro
    Png_res[IMG_RETRO_ICON].png_in   = (void *) retro_png_bin;
    Png_res[IMG_RETRO_ICON].png_size = retro_png_bin_size;
    LoadPNG(&Png_res[IMG_RETRO_ICON], NULL);

    // film PNG
    Png_res[IMG_MOVIE_ICON].png_in   = (void *) film_png_bin;
    Png_res[IMG_MOVIE_ICON].png_size = film_png_bin_size;
    LoadPNG(&Png_res[IMG_MOVIE_ICON], NULL);

    // net_host overlay
    Png_res[IMG_NETHOST].png_in   = (void *) nethost_png_bin;
    Png_res[IMG_NETHOST].png_size = nethost_png_bin_size;
    LoadPNG(&Png_res[IMG_NETHOST], NULL);
}

int LoadTexturePNG(char * filename, int index)
{

    u32 * texture_pointer2 = (u32 *) (png_texture + (index >= num_box ? num_box : Png_index[index]) * 4096 * 1024); // 4 MB reserved for PNG index
    //u32 * texture_pointer2 = (u32 *) (png_texture + (index >= BIG_PICT ? BIG_PICT : Png_index[index]) * 4096 * 1024); // 4 MB reserved for PNG index

    if(index == num_box || index == BIG_PICT) texture_pointer2 += 2048 * 1200; // reserves 2048 x 1200 x 4 for background picture

    // here you can add more textures using 'texture_pointer'. It is returned aligned to 16 bytes

    if(filename) memset(&my_png_datas, 0, sizeof(PngDatas));
    if(LoadPNG(&my_png_datas, filename) != SUCCESS) memset(&my_png_datas, 0, sizeof(PngDatas));

    my_png_datas.png_in = NULL;
    my_png_datas.png_size = 0;

    Png_offset[index] = 0;
    memcpy(&Png_datas[index], &my_png_datas, sizeof(PngDatas));

    if(my_png_datas.bmp_out)
    {
        if((index < num_box && my_png_datas.wpitch * my_png_datas.height > 4096 * 1024) ||
           (index > BIG_PICT && my_png_datas.wpitch * my_png_datas.height > 8192 * 1200))
        {   // too big!
            memset(texture_pointer2, 0, 64 * 64 * 4);
            my_png_datas.wpitch = 64 * 4;
            my_png_datas.height = my_png_datas.width = 64;
        }
        else
            memcpy(texture_pointer2, my_png_datas.bmp_out, my_png_datas.wpitch * my_png_datas.height);

        free(my_png_datas.bmp_out);

        my_png_datas.bmp_out= texture_pointer2;

        memcpy(&Png_datas[index], &my_png_datas, sizeof(PngDatas));
        Png_offset[index] = tiny3d_TextureOffset(my_png_datas.bmp_out);      // get the offset (RSX use offset instead address)

        return SUCCESS;
    }
    else
    {
         // fake PNG
        my_png_datas.bmp_out= texture_pointer2;

        int n;
        u32 * text = texture_pointer2;

        my_png_datas.width = my_png_datas.height = 64;

        my_png_datas.wpitch = my_png_datas.width * 4;

        for (n = 0; n < my_png_datas.width * my_png_datas.height; n++) *text++ = 0xff000000;

        memcpy(&Png_datas[index], &my_png_datas, sizeof(PngDatas));

        Png_offset[index] = tiny3d_TextureOffset(my_png_datas.bmp_out);
    }

    return FAILED;
}

int LoadTextureJPG(char * filename, int index)
{

    u32 * texture_pointer2 = (u32 *) (png_texture + (index >= num_box ? num_box : Png_index[index]) * 4096 * 1024); // 4 MB reserved for PNG index
    //u32 * texture_pointer2 = (u32 *) (png_texture + (index >= BIG_PICT ? BIG_PICT : Png_index[index]) * 4096 * 1024); // 4 MB reserved for PNG index

    if(index == num_box || index == BIG_PICT) texture_pointer2 += 2048 * 1200; // reserves 2048 x 1200 x 4 for background picture

    // here you can add more textures using 'texture_pointer'. It is returned aligned to 16 bytes

    memset(&my_png_datas, 0, sizeof(PngDatas));

    if(LoadJPG((JpgDatas *)&my_png_datas, filename) != SUCCESS) memset(&my_png_datas, 0, sizeof(PngDatas));

    Png_offset[index] = 0;
    memcpy(&Png_datas[index], &my_png_datas, sizeof(PngDatas));

    if(my_png_datas.bmp_out)
    {
        if((index < num_box && my_png_datas.wpitch * my_png_datas.height > 4096 * 1024) ||
           (index > BIG_PICT && my_png_datas.wpitch * my_png_datas.height > 8192 * 1200))
        {   // too big!
            memset(texture_pointer2, 0, 64 * 64 * 4);
            my_png_datas.wpitch = 64 * 4;
            my_png_datas.height = my_png_datas.width = 64;
        }
        else
            memcpy(texture_pointer2, my_png_datas.bmp_out, my_png_datas.wpitch * my_png_datas.height);

        free(my_png_datas.bmp_out);

        my_png_datas.bmp_out = texture_pointer2;

        memcpy(&Png_datas[index], &my_png_datas, sizeof(PngDatas));
        Png_offset[index] = tiny3d_TextureOffset(my_png_datas.bmp_out);      // get the offset (RSX use offset instead address)

        return SUCCESS;
    }
    else
    {
         // fake PNG
        my_png_datas.bmp_out= texture_pointer2;

        int n;
        u32 * text = texture_pointer2;

        my_png_datas.width = my_png_datas.height = 64;

        my_png_datas.wpitch = my_png_datas.width * 4;

        for (n = 0; n < my_png_datas.width * my_png_datas.height; n++) *text++ = 0xff000000;

        memcpy(&Png_datas[index], &my_png_datas, sizeof(PngDatas));

        Png_offset[index] = tiny3d_TextureOffset(my_png_datas.bmp_out);
    }

    return FAILED;
}

// no inline!
int get_icon(char * path, const int num_dir)
{
    char titleid[12];
    int is_update = (strlen(path) == 9) && (num_dir == 0);

    if(!strncmp(directories[num_dir].title_id, NETHOST, 9))
    {
        bool is_iso = false;

        int pathlen = strlen(directories[num_dir].path_name);

        if(pathlen > 4) is_iso = (strcasecmp(directories[num_dir].path_name + pathlen - 4, ".iso")   == SUCCESS);
        if(pathlen > 6) is_iso = (strcasecmp(directories[num_dir].path_name + pathlen - 6, ".iso.0") == SUCCESS) || is_iso;

        char *p;
        p = strstr(directories[num_dir].path_name, "[B");
        if(p)
        {
            snprintf(titleid, 10, "%s", p + 1);
            sprintf(path, "%s%s.JPG", covers_path, titleid);
            if(file_exists(path)) return 1;
            sprintf(path, "%s%s.PNG", covers_path, titleid);
            if(file_exists(path)) return 1;
        }
        p = strstr(directories[num_dir].path_name, "[N");
        if(p)
        {
            snprintf(titleid, 10, "%s", p + 1);
            sprintf(path, "%s%s.JPG", covers_path, titleid);
            if(file_exists(path)) return 1;
            sprintf(path, "%s%s.PNG", covers_path, titleid);
            if(file_exists(path)) return 1;
        }

        if(is_iso) return GET_ICON_FROM_ISO;

        sprintf(path, "/dev_hdd0/tmp/wmtmp/%s.PNG", get_filename(directories[num_dir].path_name));
        if(file_exists(path)) return 2;

        sprintf(path, "/%s/PS3_GAME/ICON0.PNG", directories[num_dir].path_name);
        if(file_exists(path)) return 2;

        return SUCCESS;
    }

    if(cover_mode == 0 && ((directories[num_dir].flags & (GAME_FLAGS)) == (PS3_FLAG)) && strstr(directories[num_dir].path_name, "/PS3ISO") != NULL)
        return GET_ICON_FROM_ISO;

    if((directories[num_dir].flags & (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG)) == (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG))
    {
        bool is_retro = (strlen(retro_root_path) > 0 && strstr(directories[num_dir].path_name, retro_root_path) != NULL);
        bool is_ps2_classic = !is_retro && strlen(ps2classic_path) > 0 &&
                              (strstr(directories[num_dir].path_name, ps2classic_path) != NULL);

        if(cover_mode || is_retro || is_ps2_classic)
        {
            strcpy(path, directories[num_dir].path_name);

            int path_len = strlen(path);

            if(is_ps2_classic && path_len > 8)
            {
                path_len -= 8; path[path_len] = 0;

                strcpy(path + path_len, ".jpg");
                if(file_exists(path)) return 1;
                strcpy(path + path_len, ".png");
                if(file_exists(path)) return 2;
                strcpy(path + path_len, ".JPG");
                if(file_exists(path)) return 1;
                strcpy(path + path_len, ".PNG");
                if(file_exists(path)) return 2;

                strcpy(path, directories[num_dir].path_name);
            }

            if(path_len > 4)
            {
                path_len -= 4; path[path_len] = 0;

                strcpy(path + path_len, ".jpg");
                if(file_exists(path)) return 1;
                strcpy(path + path_len, ".png");
                if(file_exists(path)) return 2;
                strcpy(path + path_len, ".JPG");
                if(file_exists(path)) return 1;
                strcpy(path + path_len, ".PNG");
                if(file_exists(path)) return 2;

                strcpy(path, directories[num_dir].path_name);
            }

            if(is_retro && (path_len > 3) && (path[path_len - 3] == '.'))
            {
                path_len -= 3; path[path_len] = 0;

                strcpy(path + path_len, ".jpg");
                if(file_exists(path)) return 1;
                strcpy(path + path_len, ".png");
                if(file_exists(path)) return 2;
                strcpy(path + path_len, ".JPG");
                if(file_exists(path)) return 1;
                strcpy(path + path_len, ".PNG");
                if(file_exists(path)) return 2;
            }
            else
            if(is_retro && (path_len > 5) && (path[path_len - 5] == '.'))
            {
                path_len -= 5; path[path_len] = 0;

                strcpy(path + path_len, ".jpg");
                if(file_exists(path)) return 1;
                strcpy(path + path_len, ".png");
                if(file_exists(path)) return 2;
                strcpy(path + path_len, ".JPG");
                if(file_exists(path)) return 1;
                strcpy(path + path_len, ".PNG");
                if(file_exists(path)) return 2;
            }
        }

        strcpy(path, directories[num_dir].path_name);

        if(is_retro || is_ps2_classic || file_exists(path) == false) return FAILED;
        return GET_ICON_FROM_ISO;
    }

    if (!is_update)
    {
        if(strlen(directories[num_dir].title_id) == 9 && strstr("NETBROWSE|IRISMAN00|PRXLOADER|CDGPLAYER|GMPADTEST|BLES80616", directories[num_dir].title_id) != NULL)
        {
            if(strncmp(directories[num_dir].title_id, "IRISMAN00", 9) == SUCCESS)
                sprintf(path, "%s/USRDIR/icons/NETBROWSE.PNG", self_path);
            else
                sprintf(path, "%s/USRDIR/icons/%s.PNG", self_path, directories[num_dir].title_id);

            if(file_exists(path)) return SUCCESS;

            sprintf(path, "%s/USRDIR/icons/DEFAULT.PNG", self_path);
            if(file_exists(path)) return SUCCESS;

            return FAILED;
        }
        else if((strlen(directories[num_dir].title_id) == 9 && strstr(custom_homebrews, directories[num_dir].title_id) != NULL) ||
                (directories[num_dir].flags & HOMEBREW_FLAG) == HOMEBREW_FLAG)
        {
            sprintf(path, "/dev_hdd0/game/%s/ICON0.PNG", directories[num_dir].title_id);
            if(file_exists(path)) return SUCCESS;

            sprintf(path, "%s/USRDIR/icons/DEFAULT.PNG", self_path);
            if(file_exists(path)) return SUCCESS;

            return FAILED;
        }


        // add PSX/PS2 iso
        if(directories[num_dir].flags & PS1_FLAG)
        {
            int cover_type = (((directories[num_dir].flags & (PS2_FLAG)) == (PS2_FLAG)) ? 1 : 2); // PSX cover

            sprintf(path, "%s%s_COV.JPG", retro_covers_path, directories[num_dir].title_id);
            if(file_exists(path)) return cover_type;

            if(strstr(directories[num_dir].path_name, "/PSXGAMES"))
            {
                sprintf(path, "%s/cover.jpg", directories[num_dir].path_name);
                if(file_exists(path)) return 2;
                sprintf(path, "%s/cover.png", directories[num_dir].path_name);
                if(file_exists(path)) return 2;
                sprintf(path, "%s/Cover.jpg", directories[num_dir].path_name);
                if(file_exists(path)) return 2;
                sprintf(path, "%s/Cover.png", directories[num_dir].path_name);
                if(file_exists(path)) return 2;
                sprintf(path, "%s/COVER.JPG", directories[num_dir].path_name);
                if(file_exists(path)) return 2;
                sprintf(path, "%s/COVER.PNG", directories[num_dir].path_name);
                if(file_exists(path)) return 2;
            }
            else
            {
                int n = strlen(directories[num_dir].path_name)-4;
                if(n<0 || strcasestr(".iso|.bin|.mdf|.img",directories[num_dir].path_name+n)==NULL) ; // skip parse title
                else
                if(parse_iso_titleid(directories[num_dir].path_name, titleid) == SUCCESS)
                {
                    sprintf(path, "%s%s_COV.JPG", retro_covers_path, titleid);
                    if(file_exists(path))
                    {
                        strcpy(directories[num_dir].title_id, titleid);
                        return cover_type;
                    }
                }
            }

            strcpy(path, directories[num_dir].path_name);
            if(path[strlen(path) - 1] == '0') path[strlen(path) - 6] = 0; else path[strlen(path) - 4] = 0;

            int n = strlen(path);
            strcpy(path + n, ".jpg");
            if(file_exists(path)) return cover_type;
            strcpy(path + n, ".png");
            if(file_exists(path)) return cover_type;
            strcpy(path + n, ".JPG");
            if(file_exists(path)) return cover_type;
            strcpy(path + n, ".PNG");
            if(file_exists(path)) return cover_type;

            sprintf(path, "%s/COVER.JPG", directories[num_dir].path_name);
            if(file_exists(path)) return 2;

            sprintf(path, "%s/COVER.jpg", directories[num_dir].path_name);
            if(file_exists(path)) return 2;

            sprintf(path, "%s/cover.jpg", directories[num_dir].path_name);
            if(file_exists(path)) return 2;

            sprintf(path, "%s/Cover.jpg", directories[num_dir].path_name);
            if(file_exists(path)) return 2;

            return 0;
        }

        // bluray /dvd / mkv in Homebrew mode
        if((directories[num_dir].flags & D_FLAG_HOMEB_GROUP) > D_FLAG_HOMEB)
        {
            strcpy(path, directories[num_dir].path_name);
            if(path[strlen(path) - 1] == '0') path[strlen(path) - 6] = 0; else path[strlen(path) - 4] = 0;
            int n = strlen(path);
            strcpy(path + n, ".jpg");
            if(file_exists(path)) return 1;
            strcpy(path + n, ".png");
            if(file_exists(path)) return 1;
            strcpy(path + n, ".JPG");
            if(file_exists(path)) return 1;
            strcpy(path + n, ".PNG");
            if(file_exists(path)) return 1;

            return FAILED;
        }

        if(cover_mode == 0 && !((directories[num_dir].flags & (PS3_FLAG | HOMEBREW_FLAG | BDVD_FLAG)) == PS3_FLAG))
        {
            sprintf(path, "%s%sICON0.PNG", directories[num_dir].path_name, &folder_mode[!(directories[num_dir].flags & D_FLAG_HOMEB)][0]);
            return SUCCESS;
        }
    }

    if(is_update || (directories[num_dir].flags & ((PS3_FLAG) | (HOMEBREW_FLAG) | (BDVD_FLAG))) == (PS3_FLAG))
    {
        // PS3
        if((directories[num_dir].flags & ((PS3_FLAG) & (BDVD_FLAG))) == ((PS3_FLAG) & (BDVD_FLAG)))
        {
            if(cover_mode == 0)
            {
                sprintf(path, "%s/PS3_GAME/ICON0.PNG", directories[num_dir].path_name);
                if(file_exists(path)) return 2;
            }
            sprintf(titleid, "%c%c%c%c%s", directories[num_dir].title_id[0], directories[num_dir].title_id[1],
                                           directories[num_dir].title_id[2], directories[num_dir].title_id[3], &directories[num_dir].title_id[5]);
        }
        else if((strlen(path) == 9) && (num_dir == 0))
            strcpy(titleid, path);
        else
            sprintf(titleid, "%c%c%c%c%s", directories[num_dir].title_id[0], directories[num_dir].title_id[1],
                                           directories[num_dir].title_id[2], directories[num_dir].title_id[3], &directories[num_dir].title_id[5]);

        sprintf(path, "%s%s.JPG", covers_path, titleid);
        if(file_exists(path) == false) {
            sprintf(path, "%s%s.PNG", covers_path, titleid);
            if(file_exists(path) == false) {
                sprintf(path, "%s%s.jpg", covers_path, titleid);
                if(file_exists(path) == false) {
                    sprintf(path, "%s%s.png", covers_path, titleid);
                    if(file_exists(path) == false) {
                        sprintf(path, "%s/USRDIR/covers/%s.JPG", MM_PATH, titleid);
                        if(file_exists(path) == false) {
                            sprintf(path, "%s/USRDIR/covers/%s.PNG", MM_PATH, titleid);
                            if(file_exists(path) == false) {
                                sprintf(path, "%s/covers/%s.JPG", self_path, titleid);
                                if(file_exists(path) == false) {
                                    sprintf(path, "%s/covers/%s.PNG", self_path, titleid);
                                    if(file_exists(path) == false) {
                                        sprintf(path, "%s/covers/%s.PNG", self_path, directories[num_dir].title_id);
                                        if(file_exists(path) == false) {
                                            // get covers from GAMES or GAMEZ
                                            if(!strcmp(hdd_folder, "GAMES") || !strcmp(hdd_folder, "dev_hdd0_2"))
                                              sprintf(path, "/dev_hdd0/GAMES/covers/%s.JPG", titleid);
                                            else
                                              sprintf(path, "/dev_hdd0/GAMEZ/covers/%s.JPG", titleid);

                                            if(file_exists(path) == false)
                                            {
                                                strcpy(path, directories[num_dir].path_name);
                                                if(path[strlen(path) - 1] == '0') path[strlen(path) - 6] = 0; else path[strlen(path) - 4] = 0;
                                                int n = strlen(path);
                                                strcpy(path + n, ".jpg");
                                                if(file_exists(path)) return 1;
                                                strcpy(path + n, ".JPG");
                                                if(file_exists(path)) return 1;
                                                strcpy(path + n, ".png");
                                                if(file_exists(path)) return 2;
                                                strcpy(path + n, ".PNG");
                                                if(file_exists(path)) return 2;

                                                sprintf(path, "%s/PS3_GAME/ICON0.PNG", directories[num_dir].path_name);
                                                if(file_exists(path)) return 2;

                                                sprintf(path, "%s", directories[num_dir].path_name);
                                                if(file_exists(path)) return GET_ICON_FROM_ISO;
                                                return FAILED;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return 1;
    }

    sprintf(titleid, "%c%c%c%c%s", directories[num_dir].title_id[0], directories[num_dir].title_id[1],
                                   directories[num_dir].title_id[2], directories[num_dir].title_id[3], &directories[num_dir].title_id[5]);

    sprintf(path, "%s%s.JPG", covers_path, titleid);
    if(file_exists(path) == false) {
        sprintf(path, "%s%s.PNG", covers_path, titleid);
        if(file_exists(path) == false) {
            sprintf(path, "%s%s.jpg", covers_path, titleid);
            if(file_exists(path) == false) {
                sprintf(path, "%s%s.png", covers_path, titleid);
                if(file_exists(path) == false) {
                    sprintf(path, "%s/USRDIR/covers/%s.JPG", MM_PATH, titleid);
                    if(file_exists(path) == false) {
                        sprintf(path, "%s/USRDIR/covers/%s.PNG", MM_PATH, titleid);
                        if(file_exists(path) == false) {
                            sprintf(path, "%s/covers/%s.PNG", self_path, titleid);
                            if(file_exists(path) == false) {
                                sprintf(path, "%s/covers/%s.JPG", self_path, titleid);
                                if(file_exists(path) == false) {
                                    sprintf(path, "%s/COVERS/%s.PNG", self_path, titleid);
                                    if(file_exists(path) == false) {
                                        sprintf(path, "%s/COVERS/%s.JPG", self_path, titleid);
                                        if(file_exists(path) == false) {
                                            sprintf(path, "%s/COVERS/%s.PNG", self_path, directories[num_dir].title_id);
                                            // get covers from GAMES or GAMEZ
                                            if(file_exists(path) == false) {
                                                sprintf(path, "/dev_hdd0/GAMES/covers/%s.JPG", titleid);
                                                if(file_exists(path) == false) sprintf(path, "/dev_hdd0/GAMEZ/covers/%s.JPG", titleid);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if(file_exists(path) == false)
    {
        sprintf(path, "%s%sICON0.PNG", directories[num_dir].path_name, &folder_mode[!((directories[num_dir].flags>>D_FLAG_HOMEB_DPL) & 1)][0]);
        if(file_exists(path)) return SUCCESS;

        sprintf(path, "%s/USRDIR/icons/DEFAULT.PNG", self_path);
        return SUCCESS;
    }
    else
        return FAILED;
}

static volatile int break_get_games = 0;

void get_games_2(void *empty)
{
    int n, f;

    if(int_currentdir < 0 || int_currentdir >= ndirectories) int_currentdir = 0;

    switch (menu_screen)
    {
        case SCR_MENU_ISO_OPTIONS:
        case SCR_MENU_GAME_OPTIONS:
        case SCR_MENU_GAME_OPTIONS_CONFIG: break;
        default:
        Png_offset[BIG_PICT] = 0;
    }

    if(mode_favourites)
    {
        for(f = 0; f < 3; f++) // priority loop: HDD/USB/BDVD Only
        for(n = 0; n < num_box; n++)
        {
            if(break_get_games) return;

            if(favourites.list[n].index < 0 || favourites.list[n].title_id[0] == 0 || favourites.list[n].index >= ndirectories) Png_offset[n] = 0;
            else
            {
                if(!Png_offset[n])
                {

                    if(f == 0 && !(directories[favourites.list[n].index].flags & D_FLAG_HDD0)) continue; // HDD Only
                    if(f == 1 && (directories[favourites.list[n].index].flags & (D_FLAG_HDD0 | D_FLAG_BDVD))) continue; // USB Only
                    if(f == 2 && !(directories[favourites.list[n].index].flags & D_FLAG_BDVD)) continue; // BDVD Only

                    strcpy(path_name, directories[favourites.list[n].index].path_name);

                    int r = get_icon(path_name, favourites.list[n].index);

                    if(r == GET_ICON_FROM_ISO)
                    {
                        Png_iscover[n] = -1;

                        if(path_name[0] != '/') sprintf(path_name, "/%s", directories[favourites.list[n].index].path_name);

                        int fd = ps3ntfs_open(path_name, O_RDONLY, 0);
                        if(fd >= 0)
                        {
                            u32 flba;
                            u64 size;
                            int re;
                            char *mem = NULL;

                            if((directories[favourites.list[n].index].flags & (PSP_FLAG)) == (PSP_FLAG))
                                re = get_iso_file_pos(fd, "/PSP_GAME/ICON0.PNG", &flba, &size);
                            else
                                re = get_iso_file_pos(fd, "/PS3_GAME/ICON0.PNG;1", &flba, &size);

                            if(!re && (mem = malloc(size)) != NULL)
                            {
                                re = ps3ntfs_read(fd, (void *) mem, size);
                                ps3ntfs_close(fd);
                                if(re == size)
                                {
                                    memset(&my_png_datas, 0, sizeof(PngDatas));
                                    my_png_datas.png_in = mem;
                                    my_png_datas.png_size = size;
                                    if(LoadTexturePNG(NULL, n) == SUCCESS) Png_iscover[n] = 2;
                                }

                                free(mem);
                                continue;
                            }
                            else
                                ps3ntfs_close(fd);
                        }
                    }
                    else
                        Png_iscover[n] = r;

                    if(!strncmp(path_name + strlen(path_name) -4, ".JPG", 4) || !strncmp(path_name + strlen(path_name) -4, ".jpg", 4))
                        LoadTextureJPG(path_name, n);
                    else
                        LoadTexturePNG(path_name, n);
                }
            }
        }

        enable_draw_background_pic1();
        return;
    }

    int nn;

    for(f = 0; f < 3; f++) // priority loop: HDD/USB/BDVD Only
    for(n = 0; n < num_box; n++)
    {
        if(break_get_games) return;
        nn = (int_currentdir + n);

        if(nn < ndirectories)
        {
            if(!Png_offset[n])
            {
                if(f == 0 && !(directories[nn].flags & D_FLAG_HDD0)) continue; // HDD Only
                if(f == 1 && (directories[nn].flags & (D_FLAG_HDD0 | D_FLAG_BDVD))) continue; // USB Only
                if(f == 2 && !(directories[nn].flags & D_FLAG_BDVD)) continue; // BDVD Only

                strcpy(path_name, directories[nn].path_name);

                int r = get_icon(path_name, nn);

                if(r == GET_ICON_FROM_ISO)
                {
                    Png_iscover[n] = -1;

                    if(path_name[0] != '/') sprintf(path_name, "/%s", directories[nn].path_name);

                    int fd = ps3ntfs_open(path_name, O_RDONLY, 0);
                    if(fd >= 0)
                    {
                        u32 flba;
                        u64 size;
                        char *mem = NULL;
                        int re;

                        if((directories[nn].flags & (PSP_FLAG)) == (PSP_FLAG))
                            re = get_iso_file_pos(fd, "/PSP_GAME/ICON0.PNG", &flba, &size);
                        else
                            re = get_iso_file_pos(fd, "/PS3_GAME/ICON0.PNG;1", &flba, &size);

                        if(!re && (mem = malloc(size)) != NULL)
                        {
                            re = ps3ntfs_read(fd, (void *) mem, size);
                            ps3ntfs_close(fd);
                            if(re == size)
                            {
                                memset(&my_png_datas, 0, sizeof(PngDatas));
                                my_png_datas.png_in = mem;
                                my_png_datas.png_size = size;
                                if(LoadTexturePNG(NULL, n) == SUCCESS) Png_iscover[n] = 2;
                            }

                            free(mem);
                            continue;
                        }
                        else
                            ps3ntfs_close(fd);
                    }
                }
                else
                    Png_iscover[n] = r;

                if(!strncmp(path_name + strlen(path_name) -4, ".JPG", 4) || !strncmp(path_name + strlen(path_name) -4, ".jpg", 4))
                   LoadTextureJPG(path_name, n);
                else
                   LoadTexturePNG(path_name, n);
            }

        } else Png_offset[n] = 0;
    }

    enable_draw_background_pic1();
}

void get_games_3(u64 var)
{

    int n, indx;

    get_grid_dimensions(false);

    if(var == 1ULL)
    {
        indx = Png_index[0];

        for(n = 0; n < num_box; n++)
        {
            Png_iscover[n] = Png_iscover[n + 1];
            Png_offset[n] = Png_offset[n + 1];
            Png_datas[n] = Png_datas[n + 1];
            Png_index[n] = Png_index[n + 1];
            if(Png_offset[n]) ;
            else {Png_iscover[n] = Png_offset[n] = 0;}
        }

        Png_iscover[num_box - 1] = Png_offset[num_box - 1] = 0; Png_index[num_box - 1] = indx;

    }
    else
    {
        indx = Png_index[num_box - 1];
        for(n = num_box - 1; n > 0; n--)
        {
            Png_iscover[n] = Png_iscover[n - 1];
            Png_offset[n] = Png_offset[n - 1];
            Png_datas[n] = Png_datas[n - 1];
            Png_index[n] = Png_index[n - 1];
            if(Png_offset[n]) ;
            else {Png_iscover[n] = Png_offset[n] = 0;}
        }

        Png_iscover[0] = Png_offset[0] = 0;  Png_index[0] = indx;
    }

    for(n = num_box; n < BIG_PICT; n++)
        Png_iscover[n] = Png_offset[n] = 0;

    get_games_2(NULL);
 }


void get_games()
{
    break_get_games = 1; // for short wait
    wait_event_thread(); // wait previous event thread function
    break_get_games = 0;

    int_currentdir = currentdir;

    // reset icon datas
    for(int n = 0; n < BIG_PICT; n++) {Png_iscover[n] = Png_offset[n] = 0; Png_index[n] = n;}

    get_grid_dimensions(false);

    // program new event thread function
    event_thread_send(0x555ULL, (u64) get_games_2, 0);
}

void DrawCenteredBar2D(float y, float w, float h, u32 rgba)
{
    float x = (848.0f - w)/ 2.0f;

    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , 1.0f);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , 1.0f);

    tiny3d_VertexPos(x + w, y + h, 1.0f);

    tiny3d_VertexPos(x    , y + h, 1.0f);
    tiny3d_End();
}
#endif

static u32 text_size = 0;

void LoadTexture()
{
    int i;

    u32 * texture_mem = tiny3d_AllocTexture(170 * 1024 * 1024); // alloc 170MB of space for textures (this pointer can be global)

    u32 * texture_pointer; // use to asign texture space without changes texture_mem

    if(!texture_mem) return; // fail!

    texture_pointer = texture_mem;

    ResetFont();

    //debug font
    texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) font_b, (u8 *) texture_pointer, 32, 255, 16, 32, 2, BIT0_FIRST_PIXEL);

/*
    TTFLoadFont(NULL, (void *) comfortaa_ttf_bin, comfortaa_ttf_bin_size);
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 32, 32, TTF_to_Bitmap);
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 20, 20, TTF_to_Bitmap);
    TTFUnloadFont();


    //new button font
    TTFLoadFont(NULL, (void *) comfortaa_bold_ttf_bin, comfortaa_bold_ttf_bin_size);
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 24, 24, TTF_to_Bitmap);
    TTFUnloadFont();
*/

    {
        sprintf(tmp_path, "%s/font.ttf", self_path);

        if(file_exists(tmp_path) == false || TTFLoadFont(0, tmp_path, NULL, 0) != SUCCESS)
        {
            if(TTFLoadFont(0, "/dev_flash/data/font/SCE-PS3-SR-R-JPN.TTF", NULL, 0) != SUCCESS)
                if(TTFLoadFont(0, "/dev_flash/data/font/SCE-PS3-NR-R-JPN.TTF", NULL, 0) != SUCCESS)
                    exit(0);
                //TTFLoadFont(NULL, (void *) comfortaa_bold_ttf_bin, comfortaa_bold_ttf_bin_size);
        }

        TTFLoadFont(1, "/dev_flash/data/font/SCE-PS3-DH-R-CGB.TTF", NULL, 0);
        TTFLoadFont(2, "/dev_flash/data/font/SCE-PS3-SR-R-LATIN2.TTF", NULL, 0);
        TTFLoadFont(3, "/dev_flash/data/font/SCE-PS3-YG-R-KOR.TTF", NULL, 0);

    }


    Load_PNG_resources();

    for(i = 0; i < MAX_RESOURCES; i++)
    {
        if(Png_res[i].png_in == NULL) continue;

        Png_res_offset[i]  = 0;

        if(Png_res[i].bmp_out)
        {
            memcpy(texture_pointer, Png_res[i].bmp_out, Png_res[i].wpitch * Png_res[i].height);

            free(Png_res[i].bmp_out); // free the PNG because i don't need this datas

            Png_res_offset[i] = tiny3d_TextureOffset(texture_pointer);      // get the offset (RSX use offset instead address)

            texture_pointer += ((Png_res[i].wpitch * Png_res[i].height + 15) & ~15) / 4; // aligned to 16 bytes (it is u32) and update the pointer
         }
    }


    ttf_texture = (u16 *) texture_pointer;

    texture_pointer += 1024 * 16;

    texture_pointer = (u32 *) init_ttf_table((u16 *) texture_pointer);

    png_texture = (u8 *) texture_pointer;

    text_size = (u32) (u64)((png_texture + BIG_PICT * 4096 * 1024 + 1980 * 1080 * 4 + 2048 * 1200 * 4) - ((u8 *) texture_mem));
}

void get_pict(int *index)
{
    char dir[0x420];

    int fd;
    int i = *index;
    int ind = mode_favourites ? favourites.list[i].index : (int_currentdir + i);

    bSkipPIC1 = false;
    Png_offset[BIG_PICT] = 0;

    if(!mode_favourites || (mode_favourites != 0 && favourites.list[i].index >= 0))
    {
        sprintf(dir, "%s%s", directories[get_int_currentdir(i)].path_name,
             &folder_mode[!((directories[get_int_currentdir(i)].flags>>D_FLAG_HOMEB_DPL) & 1)][0]);

        if(strstr(dir, "/USRDIR")) goto get_pic1;

        bool is_retro = false;
        bool is_psp = (directories[ind].flags & (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG)) == (PSP_FLAG | RETRO_FLAG | PS2_CLASSIC_FLAG);

        if(is_psp)
        {
            is_retro = (strlen(retro_root_path) > 0 && strstr(directories[ind].path_name, retro_root_path) != NULL);
            bool is_ps2_classic = !is_retro && strlen(ps2classic_path) > 0 &&
                                  (strstr(directories[ind].path_name, ps2classic_path) != NULL);

            if(is_retro || is_ps2_classic) goto default_pict;
            goto get_pic_from_iso;
        }

        // ISOS
        if((directories[ind].flags & ((PS3_FLAG) | (HOMEBREW_FLAG) | (BDVD_FLAG))) == (PS3_FLAG))
        {
            if((directories[ind].flags & (PS2_FLAG)) == (PS2_FLAG))
            { // PS2
            }
            else
            { // PS3 / PSP
get_pic_from_iso:
                fd = ps3ntfs_open(directories[ind].path_name, O_RDONLY, 0);
                if(fd >= 0)
                {
                    u32 flba;
                    u64 size;
                    char *mem = NULL;
                    int re;

                    if(is_psp)
                    {
                               re = get_iso_file_pos(fd, "/PSP_GAME/PIC1.PNG", &flba, &size);
                        if(re) re = get_iso_file_pos(fd, "/PSP_GAME/PIC0.PNG", &flba, &size);
                        if(re) re = get_iso_file_pos(fd, "/PSP_GAME/ICON0.PNG", &flba, &size);
                        if(re) re = get_iso_file_pos(fd, "/PSP_GAME/PIC2.PNG", &flba, &size);
                    }
                    else
                    {
                               re = get_iso_file_pos(fd, "/PS3_GAME/PIC1.PNG;1", &flba, &size);
                        if(re) re = get_iso_file_pos(fd, "/PS3_GAME/PIC0.PNG;1", &flba, &size);
                        if(re) re = get_iso_file_pos(fd, "/PS3_GAME/ICON0.PNG;1", &flba, &size);
                        if(re) re = get_iso_file_pos(fd, "/PS3_GAME/PIC2.PNG;1", &flba, &size);
                    }

                    Png_offset[BIG_PICT] = 0;

                    if(!re && (mem = malloc(size)) != NULL)
                    {
                        re = ps3ntfs_read(fd, (void *) mem, size);
                        ps3ntfs_close(fd);
                        if(re == size)
                        {
                            memset(&my_png_datas, 0, sizeof(PngDatas));
                            my_png_datas.png_in = mem;
                            my_png_datas.png_size = size;
                            if(LoadTexturePNG(NULL, BIG_PICT) == SUCCESS) ;
                            else
                                Png_offset[BIG_PICT] = 0;
                        }
                        free(mem);
                    }
                    else
                        ps3ntfs_close(fd);
                }
            }       // PS3

            if(bk_picture == BG_PIC1 || gui_mode == MODE_XMB_LIKE)
            {
                if(Png_offset[BIG_PICT] == 0)
                    load_background_picture();
                else
                {
                    Png_offset[BACKGROUND_PICT] = Png_offset[BIG_PICT];
                    Png_datas[BACKGROUND_PICT] = Png_datas[BIG_PICT];
                }
            }

            return;
        }

get_pic1:

        for(i = 22; i < 30; i++) if(dir[i] == '/') dir[++i]=0;

        // GAMES / GAMEZ
        char tmp_path[0x420];
        sprintf(tmp_path, "%sPIC1.PNG", dir);
        if(file_exists(tmp_path) == false || LoadTexturePNG(tmp_path, BIG_PICT) == FAILED)
        {
            sprintf(tmp_path, "%sPIC0.PNG", dir);
            if(file_exists(tmp_path) == false || LoadTexturePNG(tmp_path, BIG_PICT) == FAILED)
            {
                sprintf(tmp_path, "%sPIC2.PNG", dir);
                if(file_exists(tmp_path) == false || LoadTexturePNG(tmp_path, BIG_PICT) == FAILED)
                {
                    sprintf(dir, "%s%s", directories[ind].path_name,
                          &folder_mode[!((directories[ind].flags>>D_FLAG_HOMEB_DPL) & 1)][0]);

                    sprintf(tmp_path, "%sPIC1.PNG", dir);
                    if(file_exists(tmp_path) == false || LoadTexturePNG(tmp_path, BIG_PICT) == FAILED)
                    {
                        sprintf(tmp_path, "%sICON0.PNG", dir);
                        if(file_exists(tmp_path) == false || LoadTexturePNG(tmp_path, BIG_PICT) == FAILED) Png_offset[BIG_PICT] = 0;
                    }
                }
            }
        }

default_pict:
        if(bk_picture == BG_PIC1 || gui_mode == MODE_XMB_LIKE)
        {
            bSkipPIC1 = true;

            if(Png_offset[BIG_PICT] == 0 || is_retro)
                load_background_picture();
            else
            {
                Png_offset[BACKGROUND_PICT] = Png_offset[BIG_PICT];
                Png_datas[BACKGROUND_PICT] = Png_datas[BIG_PICT];
            }
        }
    }
}
