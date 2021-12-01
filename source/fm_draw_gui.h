static char help1[] = {
    "HELP - [ File Manager ]\n"
    "\n"
    "SELECT + START - Exit\n"
    "CROSS - Action for files/folders (Opens Hex Editor if selected)\n"
    "\n"
    "TRIANGLE - Opens menu selector (from the device)\n"
    "SQUARE - Single item selection\n"
    "SELECT + SQUARE - Select/Deselect all files/folders\n"
    "\n"
    "UP/DOWN - Move the cursor\n"
    "L1/R1 - Move the cursor by page\n"
    "LEFT/RIGHT - Switch window.\n"
    "SELECT+LEFT/RIGHT - Open current directory in the other window\n"
    "\n"
    "L2+R2 - Switch the window split mode (Vertical/Horizontal)\n"
    "L3/R3 - Changes to different frequently used paths\n"
};

static void display_icon(int x, int y, int z, int icon)
{
    tiny3d_SetTextureWrap(0, Png_res_offset[7 + icon], Png_res[7 + icon].width,
                          Png_res[7 + icon].height, Png_res[7 + icon].wpitch,
                          TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

    DrawTextBox(x+2, y, z, 18, 18, WHITE);
}

void draw_file_manager()
{
        tiny3d_Flip();
        ps3pad_read();

        tiny3d_Project2D();
        cls2();
        update_twat(false);

        //// Begin drawing File Manager screen ////

        tiny3d_SetMatrixModelView(is_vsplit ? &mat_win1 : &mat_unit);
        DrawBox(0, 0, 0, 816, is_vsplit ? 48 : 32, BLUE5);
        DrawBox(816, 0, 0, 32, is_vsplit ? 48 : 32, (!fm_pane && (frame & BLINK_SLOW)) ? YELLOW : BLACK);
        set_ttf_window(8, 0, is_vsplit ? 590 : 592, is_vsplit ? 48 : 32, WIN_AUTO_LF);
        display_ttf_string(0, 0, (char *) path1, WHITE, 0, is_vsplit ? 16 : 12, 24);

        set_ttf_window(600, 0, 200, 32, WIN_AUTO_LF);
        if(free_device1 < 0x40000000LL)
            sprintf(temp_buffer, "%1.1f MB FREE", ((double) free_device1) / 0x100000LL);
        else
            sprintf(temp_buffer, "%1.2f GB FREE", ((double) free_device1) / GIGABYTES);

        if(free_device1 < GIGABYTES && strncmp(path1, "/dev_hdd0", 9) == SUCCESS)
            display_ttf_string(0, 0, (char *) temp_buffer, (frame & BLINK_SLOW) ? RED1 : GRAY, 0, 24, 32);
        else
            display_ttf_string(0, 0, (char *) temp_buffer, GRAY, 0, 24, 32);

        if(is_vsplit)
        {
            set_ttf_window(600, 29, 200, 24, WIN_AUTO_LF);
            if(selcount1 <= 0)
                sprintf(temp_buffer, "%i Items", nentries1);
            else if(selsize1 < 0x100000LL)
                sprintf(temp_buffer, "%i of %i (%1.1f KB)", selcount1, nentries1, (double) (selsize1  / 1024LL));
            else if(selsize1 < 0x40000000LL)
                sprintf(temp_buffer, "%i of %i (%1.1f MB)", selcount1, nentries1, (double) (selsize1 / 0x100000LL));
            else
                sprintf(temp_buffer, "%i of %i (%1.2f GB)", selcount1, nentries1, ((double) selsize1) / GIGABYTES);

            display_ttf_string(0, 0, (char *) temp_buffer, GRAY, 0, 20, 24);
        }

        set_ttf_window(816, 0, 36, 32, WIN_AUTO_LF);
        display_ttf_string(4, 0, (char *) "A", RED1, 0, 32, 32);

        DrawBox2(0, is_vsplit ? 48 : 32, 0, 848, is_vsplit ? (512 - 48) * 3/2: 256 - 32 /*, 0x2080c0ff*/);

        if(is_vsplit)
        {
            DrawLineBox(0, 0, 0, 848, 48, 0x2000ffff);

            DrawLineBox(-1, 48, 0, 848, 24 * 24 + 16, 0x2000ffff);
        }

        tiny3d_SetMatrixModelView(is_vsplit ? &mat_win2 : &mat_unit);
        DrawBox(0, 256, 0, 816, is_vsplit ? 48 : 32, BLUE5);
        DrawBox(816, 256, 0, 32, is_vsplit ? 48 : 32, (fm_pane && (frame & BLINK_SLOW)) ? YELLOW : BLACK);
        set_ttf_window(8, 256, is_vsplit ? 590 : 592, is_vsplit ? 48 : 32, WIN_AUTO_LF);
        display_ttf_string(0, 0, (char *) path2, WHITE, 0, is_vsplit ? 16 : 12, 24);

        set_ttf_window(600, 256, 200, 32, WIN_AUTO_LF);

        if(free_device2 < 0x40000000LL)
            sprintf(temp_buffer, "%1.1f MB FREE", ((double) free_device2) / 0x100000LL);
        else
            sprintf(temp_buffer, "%1.2f GB FREE", ((double) free_device2) / GIGABYTES);

        if(free_device2 < GIGABYTES && strncmp(path2, "/dev_hdd0", 9) == SUCCESS)
            display_ttf_string(0, 0, (char *) temp_buffer, (frame & BLINK_SLOW) ? RED1 : GRAY, 0, 24, 32);
        else
            display_ttf_string(0, 0, (char *) temp_buffer, GRAY, 0, 24, 32);

        if(is_vsplit)
        {
            set_ttf_window(600, 285, 200, 24, WIN_AUTO_LF);
            if(selcount2 <= 0)
                sprintf(temp_buffer, "%i Items", nentries2);
            else if(selsize2 < 0x100000LL)
                sprintf(temp_buffer, "%i of %i (%1.1f KB)", selcount2, nentries2, (double) (selsize2  / 1024LL));
            else if(selsize2 < 0x40000000LL)
                sprintf(temp_buffer, "%i of %i (%1.1f MB)", selcount2, nentries2, (double) (selsize2 / 0x100000LL));
            else
                sprintf(temp_buffer, "%i of %i (%1.2f GB)", selcount2, nentries2, ((double) selsize2) / GIGABYTES);

            display_ttf_string(0, 0, (char *) temp_buffer, GRAY, 0, 20, 24);
        }

        set_ttf_window(816, 256, 36, 32, WIN_AUTO_LF);
        display_ttf_string(4, 0, (char *) "B", RED1, 0, 32, 32);

        DrawBox2(0, (is_vsplit ? 48 : 32) + 256 , 0, 848, is_vsplit? (512 - 48) * 3/2 : 256 - 32/*, 0x2080c0ff*/);

        if(is_vsplit)
        {
            DrawLineBox(0, 256, 0, 848, 48, 0x2000ffff);

            DrawLineBox(-1, 48 + 256, 0, 848, 24 * 24 + 16, 0x2000ffff);
        }

        tiny3d_SetMatrixModelView(is_vsplit ? &mat_win1 : &mat_unit);

        set_ttf_window(24, is_vsplit ? 48 : 32, 848-24, is_vsplit ? 656 - 48 : 256 - 32, 0);

        if(nentries1)
        {
            int py = 0;

            if(sel1 > nentries1) sel1 = nentries1 > 0 ? nentries1 - 1: 0;

            if((sel1 >= pos1) && (frame & BLINK) && !fm_pane)
                DrawBox(0, py + (is_vsplit ? 48 : 32) + 24 * (sel1 - pos1), 0, 848, 24, CURSORCOLOR);
            else
                DrawBox(0, py + (is_vsplit ? 48 : 32) + 24 * (sel1 - pos1), 0, 848, 24, INVISIBLE);

            for(int n = 0; n < (is_vsplit? 24 : 9); n++)
            {
                if(pos1 + n >= nentries1) break;

                u32 color = WHITE;

                if(entries1_size[pos1 + n] < 0)
                {
                    struct stat s;
                    sprintf(temp_buffer, "%s/%s", path1, entries1[pos1 + n].d_name);
                    if(ps3ntfs_stat(temp_buffer, &s) == SUCCESS) entries1_size[pos1 + n] = s.st_size; else entries1_size[pos1 + n] = 0;
                }

                stat1.st_size = entries1_size[pos1 + n];
                stat1.st_mode = entries1[pos1 + n].d_type;

                if(entries1[pos1 + n].d_type & IS_DIRECTORY)
                {
                    if(entries1[pos1 + n].d_type & IS_NOT_AVAILABLE) color = YELLOW;
                    else color = YELLOW2;
                    display_icon(0, py + (is_vsplit ? 50 : 34), 0, FILE_TYPE_FOLDER);
                }
                else
                {
                    int type = entries1_type[pos1 + n];

                    if(type < 1)
                    {
                        type = FILE_TYPE_NORMAL;
                        char *ext = get_extension(entries1[pos1 + n].d_name);
                        if(!strcasecmp(ext, ".pkg")) type = FILE_TYPE_PKG; else
                        if(!strcasecmp(ext, ".self")) type = FILE_TYPE_SELF; else
                        if(!strcasecmp(ext, ".png")) type = FILE_TYPE_PNG; else
                        if(!strcasecmp(ext, ".jpg")) type = FILE_TYPE_JPG; else
                        if(!strcasecmp(ext, ".zip")) type = FILE_TYPE_ZIP; else
                        if(!strcasecmp(ext, ".lua")) type = FILE_TYPE_LUA; else
                        if(strcasestr(".iso|.bin|.img|.mdf|.iso.0", ext) != NULL) type = FILE_TYPE_ISO; else
                        if(is_audiovideo(ext)) type = FILE_TYPE_ISO;

                        if(type == FILE_TYPE_ISO && strcmp(entries1[pos1 + n].d_name, "EBOOT.BIN") == SUCCESS)
                        {
                            color = CYAN;
                            entries1_type[pos1 + n] = FILE_TYPE_BIN;
                        }
                        else
                            entries1_type[pos1 + n] = type;
                    }
                    else if(type == FILE_TYPE_BIN)
                    {
                        color = CYAN;
                        type = FILE_TYPE_ISO;
                    }
                    if (type == FILE_TYPE_JPG) type = FILE_TYPE_PNG;

                    display_icon(0, py + (is_vsplit ? 50 : 34), 0, type);
                }

                if(entries1[pos1 + n].d_type & IS_MARKED)
                    DrawBox(0, py + (is_vsplit ? 52 : 36), 0, 848, 16, 0x800080a0);

                int dx = 0;

                double fsize = 0;

                if(/*sel1 == (pos1 + n) && */stat1.st_mode != 0xffffffff)
                {
                    if(stat1.st_mode == IS_DIRECTORY)
                    {
                        if((path1[1] == 0))
                        {
                            if(entries1_size[pos1 + n] > 0x10000) //if size > 64kb (1 cluster or more) use cached free space
                                fsize = (double) stat1.st_size;
                            else if(entries1[pos1 + n].d_name[0] == 'd' || entries1[pos1 + n].d_name[0] == 'n' || entries1[pos1 + n].d_name[0] == 'e') //dev_*, ntfs, ext
                            {
                                sprintf(TEMP_PATH, "/%s", entries1[pos1 + n].d_name);
                                fsize = (double) get_free_space(TEMP_PATH, true);
                                entries1_size[pos1 + n] = (s64) fsize;
                            }
                        }
                    }
                    else
                        fsize = (double) stat1.st_size;

                    if(stat1.st_mode == IS_DIRECTORY && fsize == 0)
                    { /* skip folders */}
                    else if(fsize < 1024LL)
                        sprintf(temp_buffer, "%1.0f B", fsize);
                    else if(fsize < 0x100000LL)
                        sprintf(temp_buffer, "%1.1f KB", fsize / 1024LL);
                    else if(fsize < 0x40000000LL)
                        sprintf(temp_buffer, "%1.1f MB", fsize / 0x100000LL);
                    else
                        sprintf(temp_buffer, "%1.2f GB", fsize / GIGABYTES);

                    dx = display_ttf_string(0, py, (char *) temp_buffer, 0, 0, is_vsplit ? 24 : 16, 24);
                }

                //set_ttf_window(24, is_vsplit ? 48 : 32, 848 - (dx + 24), 256 - 32, 0);
                set_ttf_window(24, is_vsplit ? 48 : 32, 848 - (dx + 24), is_vsplit ? 656 - 48 : 256 - 32, 0);

                int dxx = display_ttf_string(0, py, (char *) entries1[pos1 + n].d_name, color, 0, is_vsplit ? 24 : 16, 24);

                if((path1[1] == 0) && !strncmp( (char *) entries1[pos1 + n].d_name, "ntfs", 4))
                {
                    sprintf(MEM_MESSAGE, MSG_HOW_TO_UNMOUNT_DEVICE, NTFS_Test_Device((char *) entries1[pos1 + n].d_name));
                    display_ttf_string(dxx, py, MEM_MESSAGE, YELLOW, 0, is_vsplit ? 24 : 16, 24);
                }
                else
                if((path1[1] == 0) && !strncmp( (char *) entries1[pos1 + n].d_name, "ext", 3))
                {
                    sprintf(MEM_MESSAGE, MSG_HOW_TO_UNMOUNT_DEVICE, NTFS_Test_Device((char *) entries1[pos1 + n].d_name));
                    display_ttf_string(dxx, py, MEM_MESSAGE, YELLOW, 0, is_vsplit ? 24 : 16, 24);
                }

                if(stat1.st_mode == IS_DIRECTORY && fsize == 0)
                {
                    // don't show size for folders
                }
                else if(/*sel1 == (pos1 + n) && */stat1.st_mode != 0xffffffff)
                {
                    set_ttf_window(848 - dx, (is_vsplit ? 48 : 32), dx, is_vsplit ? 656 - 48 : 256 - 32, 0);
                    display_ttf_string(0, py, (char *) temp_buffer, 0xffffffff, 0, is_vsplit ? 24 : 16, 24);
                }

                py += 24;

            }
        }

        tiny3d_SetMatrixModelView(is_vsplit ? &mat_win2 : &mat_unit);

        set_ttf_window(24, (is_vsplit ? 48 : 32 ) + 256, 848 - 24, is_vsplit ? 656 - 48 : 256 - 32, 0);

        if(nentries2)
        {
            int py = 0;

            if(sel2 > nentries2) sel2 = nentries2 > 0 ? nentries2 - 1: 0;

            if((sel2 >= pos2) && (frame & BLINK) && fm_pane)
                DrawBox(0, py + (is_vsplit ? 48 : 32) + 256 + 24 * (sel2 - pos2), 0, 848, 24, CURSORCOLOR);
            else
                DrawBox(0, py + (is_vsplit ? 48 : 32) + 256 + 24 * (sel2 - pos2), 0, 848, 24, INVISIBLE);

            for(int n = 0; n < (is_vsplit? 24 : 9); n++)
            {
                if(pos2 + n >= nentries2) break;

                u32 color = WHITE;

                if(entries2_size[pos2 + n] < 0)
                {
                    struct stat s;
                    sprintf(temp_buffer, "%s/%s", path2, entries2[pos2 + n].d_name);
                    if(ps3ntfs_stat(temp_buffer, &s) == SUCCESS) entries2_size[pos2 + n] = s.st_size; else entries2_size[pos2 + n] = 0;
                }

                stat2.st_size = entries2_size[pos2 + n];
                stat2.st_mode = entries2[pos2 + n].d_type;

                if(entries2[pos2 + n].d_type & IS_DIRECTORY)
                {
                    if(entries2[pos2 + n].d_type & IS_NOT_AVAILABLE) color = YELLOW;
                    else color = YELLOW2;
                    display_icon(0, py + (is_vsplit ? 50 : 34) + 256 , 0, FILE_TYPE_FOLDER);
                }
                else
                {
                    int type = entries2_type[pos2 + n];

                    if(type < 1)
                    {
                        type = FILE_TYPE_NORMAL;
                        char *ext = get_extension(entries2[pos2 + n].d_name);
                        if(!strcasecmp(ext, ".pkg")) type = FILE_TYPE_PKG; else
                        if(!strcasecmp(ext, ".self")) type = FILE_TYPE_SELF; else
                        if(!strcasecmp(ext, ".png")) type = FILE_TYPE_PNG; else
                        if(!strcasecmp(ext, ".jpg")) type = FILE_TYPE_JPG; else
                        if(!strcasecmp(ext, ".zip")) type = FILE_TYPE_ZIP; else
                        if(!strcasecmp(ext, ".lua")) type = FILE_TYPE_LUA; else
                        if(strcasestr(".iso|.bin|.img|.mdf|.iso.0", ext) != NULL) type = FILE_TYPE_ISO; else
                        if(is_audiovideo(ext)) type = FILE_TYPE_ISO;

                        if(type == FILE_TYPE_ISO && strcmp(entries2[pos2 + n].d_name, "EBOOT.BIN") == SUCCESS)
                        {
                            color = CYAN;
                            entries2_type[pos2 + n] = FILE_TYPE_BIN;
                        }
                        else
                            entries2_type[pos2 + n] = type;
                    }
                    else if(type == FILE_TYPE_BIN)
                    {
                        color = CYAN;
                        type = FILE_TYPE_ISO;
                    }
                    if (type == FILE_TYPE_JPG) type = FILE_TYPE_PNG;

                    display_icon(0, py + (is_vsplit ? 50 : 34) + 256, 0, type);
                }

                if(entries2[pos2 + n].d_type & IS_MARKED)
                    DrawBox(0, py + (is_vsplit ? 52 : 36) + 256, 0, 848, 16, 0x800080a0);

                int dx = 0;

                double fsize = 0;

                if(/*sel2 == (pos2 + n) && */stat2.st_mode != 0xffffffff)
                {
                    if(stat2.st_mode == IS_DIRECTORY)
                    {
                        if((path2[1] == 0))
                        {
                            if(entries2_size[pos2 + n] > 0x10000) //if size > 64kb (1 cluster or more) use cached free space
                                fsize = (double) stat2.st_size;
                            else if(entries2[pos2 + n].d_name[0] == 'd' || entries2[pos2 + n].d_name[0] == 'n' || entries2[pos2 + n].d_name[0] == 'e') //dev_*, ntfs, ext
                            {
                                sprintf(TEMP_PATH, "/%s", entries2[pos2 + n].d_name);
                                fsize = (double) get_free_space(TEMP_PATH, true);
                                entries2_size[pos2 + n] = (s64) fsize;
                            }
                        }
                    }
                    else
                        fsize = (double) stat2.st_size;

                    if(stat2.st_mode == IS_DIRECTORY && fsize == 0)
                    { /* skip folders */}
                    else if(fsize < 1024LL)
                        sprintf(temp_buffer, "%1.0f B", fsize);
                    else if(fsize < 0x100000LL)
                        sprintf(temp_buffer, "%1.1f KB", fsize / 1024LL);
                    else if(fsize < 0x40000000LL)
                        sprintf(temp_buffer, "%1.1f MB", fsize / 0x100000LL);
                    else
                        sprintf(temp_buffer, "%1.2f GB", fsize / GIGABYTES);

                    dx = display_ttf_string(0, py, (char *) temp_buffer, 0, 0, is_vsplit ? 24 : 16, 24);
                }

                //set_ttf_window(24, 256 + 32, 848 - (dx + 24), 256 - 32, 0);
                set_ttf_window(24, (is_vsplit ? 48 : 32) + 256, 848 - (dx + 24), is_vsplit ? 656 - 48 : 256 - 32, 0);

                int dxx = display_ttf_string(0, py, (char *) entries2[pos2 + n].d_name, color, 0, is_vsplit ? 24 : 16, 24);

                if((path2[1] == 0) && !strncmp( (char *) entries2[pos2 + n].d_name, "ntfs", 4))
                {
                    sprintf(MEM_MESSAGE, MSG_HOW_TO_UNMOUNT_DEVICE, NTFS_Test_Device((char *) entries2[pos2 + n].d_name));
                    display_ttf_string(dxx, py, MEM_MESSAGE, YELLOW, 0, is_vsplit ? 24 : 16, 24);
                }
                else
                if((path2[1] == 0) && !strncmp( (char *) entries2[pos2 + n].d_name, "ext", 3))
                {
                    sprintf(MEM_MESSAGE, MSG_HOW_TO_UNMOUNT_DEVICE, NTFS_Test_Device((char *) entries2[pos2 + n].d_name));
                    display_ttf_string(dxx, py, MEM_MESSAGE, YELLOW, 0, is_vsplit ? 24 : 16, 24);
                }

                if(stat2.st_mode == IS_DIRECTORY && fsize == 0)
                {
                    // don't show size for folders
                }
                else if(/*sel2 == (pos2 + n) && */stat2.st_mode != 0xffffffff)
                {
                    set_ttf_window(848 - dx, (is_vsplit ? 48 : 32) + 256, dx, is_vsplit ? 656 - 48 : 256 - 32, 0);
                    display_ttf_string(0, py, (char *) temp_buffer, 0xffffffff, 0, is_vsplit ? 24 : 16, 24);
                }

                py += 24;

            }
        }

        tiny3d_SetMatrixModelView(&mat_unit);

        if(is_vsplit)
        {
            DrawBox(0, 512 - 32, 0, 848, 32, BLUE5);

            DrawLineBox(10, 512 - 32, 0, 848, 32, 0x2000ffff);

            set_ttf_window(0, 512 - 32, 480, 32, WIN_AUTO_LF);
            display_ttf_string(0, 0, (char *) "- File Manager", 0x208098cf, 0, 32, 32);

            // display temperature
            static u32 temp = 0;
            static u32 temp2 = 0;
            static char temp_disp[64];

            if(temp == 0 || (frame & 0x1f) == 0x0 )
            {
                sys_game_get_temperature(0, &temp);
                sys_game_get_temperature(1, &temp2);
                sprintf(temp_disp, "Temp CPU: %iºC RSX: %iºC", temp, temp2);
            }

            set_ttf_window(848 - 260, 512 - 30, 300, 32, WIN_AUTO_LF);
            display_ttf_string(0, 0, temp_disp, 0xFFFFFF55, 0, 16, 32);
        }


        //// End drawing File Manager screen ////
}