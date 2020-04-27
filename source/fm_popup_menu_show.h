// called from file_manager.c
{
    int py = 0;
    int max_menu2 = 8; bool is_dir=false;
    if((!fm_pane && (path1[1] == 0)) || (fm_pane && (path2[1] == 0))) {max_menu2 = 7;}
    else if(!fm_pane &&
            (strcmp(path1, "/dev_hdd0/game") == SUCCESS ||
             strstr(path1, "/GAME") != NULL ||
             strcmp(entries1[sel1].d_name, "game") == SUCCESS ||
             strcmp(entries1[sel1].d_name, "GAMES") == SUCCESS ||
             strcmp(entries1[sel1].d_name, "GAMEZ") == SUCCESS ||
             (strstr(path1, "/PS3ISO") != NULL &&
             (!strcmpext(entries1[sel1].d_name, ".iso") || !strcmpext(entries1[sel1].d_name, ".iso.0")))
           )) max_menu2 = 9;
    else if(fm_pane &&
            (strcmp(path2, "/dev_hdd0/game") == SUCCESS ||
             strstr(path2, "/GAME") != NULL ||
             strcmp(entries2[sel2].d_name, "game") == SUCCESS ||
             strcmp(entries2[sel2].d_name, "GAMES") == SUCCESS ||
             strcmp(entries2[sel2].d_name, "GAMEZ") == SUCCESS ||
             (strstr(path2, "/PS3ISO")  != NULL &&
             (!strcmpext(entries2[sel2].d_name, ".iso") || !strcmpext(entries2[sel2].d_name, ".iso.0")))
           )) max_menu2 = 9;
    else if( (!fm_pane && use_cobra && (entries1[sel1].d_type & IS_DIRECTORY) && strstr(path1, "/dev_bdvd")==NULL && !is_ntfs_path(path1))   ||
             ( fm_pane && use_cobra && (entries2[sel2].d_type & IS_DIRECTORY) && strstr(path2, "/dev_bdvd")==NULL && !is_ntfs_path(path2)) ) {max_menu2 = 9; is_dir=true;}
    else if( (!fm_pane && use_cobra && is_ntfs_path(path1) && !(entries1[sel1].d_type & IS_DIRECTORY)) || ( fm_pane && use_cobra && is_ntfs_path(path2) && !(entries2[sel2].d_type & IS_DIRECTORY) ) ) {max_menu2 = 9;}

    //else if(!(!fm_pane && (entries1[sel1].d_type & IS_DIRECTORY)) || !(fm_pane && (entries2[sel2].d_type & IS_DIRECTORY))) {max_menu2 = 9;}


    DrawBox((848 - 224)/2, (512 - (24 * max_menu2 + 1))/2 - 20 - 20, 0, 224, (24 * max_menu2 + 1) + 40, GRAY);
    DrawBox((848 - 216)/2, (512 - (24 * (max_menu2 + 1)))/2 - 20, 0, 216, (24 * (max_menu2 + 1)), POPUPMENUCOLOR);
    set_ttf_window((848 - 200)/2, (512 - (24 * (max_menu2 + 1)) - 20)/2, 200, (24 * (max_menu2 + 1)), 0);

    if(new_pad & BUTTON_CROSS_) frame = 300;

    if(ROOT_MENU)
    {
        if(use_cobra == false || bAllowNetGames == false) mount_option = 0;

        bool blink = (set_menu2 == 1  && (frame & BLINK));

        display_ttf_string(0, py, "exFAT Manager",      (set_menu2 == 7  && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, (mount_option == 1) ? "Mount /net_host0" :
                                  (mount_option == 2) ? "Mount /net_host0/PKG" :
                                  (mount_option == 3) ? "Mount /net_host0/VIDEO" :
                                  (mount_option == 4) ? "Mount /net_host1" :
                                  (mount_option == 5) ? "Mount /net_host1/PKG" :
                                  (mount_option == 6) ? "Mount /net_host1/VIDEO" :
                                  (mount_option == 7) ? "Unmount /dev_bdvd" :
                                  !dev_blind ? "Mount /dev_blind" : "Unmount /dev_blind", blink ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, "LV2 Dump",          (set_menu2 == 2  && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, "LV1 Dump",          (set_menu2 == 3  && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, "RAM Editor",        (set_menu2 == 4  && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, "LV2 Editor",        (set_menu2 == 5  && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, (exit_option == 1) ? "Exit to XMB" : (exit_option == 2) ? "Restart the PS3" : "Exit File Manager",
                                                       (set_menu2 == 6  && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;
    }
    else
    {
        display_ttf_string(0, py, "New Folder",        (set_menu2 == 1 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, "Rename",            (set_menu2 == 2 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        if((allow_shadow_copy == 1) && !strncmp(path1, "/dev_hdd0", 9) && !strncmp(path1, path2, 9))
        {
            display_ttf_string(0, py, "Shadow Copy",   (set_menu2 == 3 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;
        }
        else if(allow_shadow_copy == 2 && ((!fm_pane && isDir(path1)) || (fm_pane && isDir(path2))))
        {
            display_ttf_string(0, py, "Zip Folder",    (set_menu2 == 3 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;
        }
        else
        {
            if(allow_shadow_copy == 1) allow_shadow_copy = 0;
            display_ttf_string(0, py, "Copy",          (set_menu2 == 3  && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;
        }

        display_ttf_string(0, py, "Move",              (set_menu2 == 4 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, "Delete",            (set_menu2 == 5 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, "Paste to New File", (set_menu2 == 6 && (frame & BLINK)) ? INVISIBLE : copy_len ? WHITE : GRAY, 0, 16, 24); py += 24;

        display_ttf_string(0, py, ((!fm_pane && (entries1[sel1].d_type & IS_DIRECTORY)) || ( fm_pane && (entries2[sel2].d_type & IS_DIRECTORY))) ?
                                  "Build ISO from Folder" : "Build ISO from File",
                                                       (set_menu2 == 7 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        display_ttf_string(0, py, ((!fm_pane && (entries1[sel1].d_type & IS_DIRECTORY)) || ( fm_pane && (entries2[sel2].d_type & IS_DIRECTORY))) ?
                                  "Get Folder Info" : "Get File Info",
                                                       (set_menu2 == 8 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

        if(is_dir)
            {display_ttf_string(0, py, (mnt_mode == 1) ? "Mount + Exit to XMB" : (mnt_mode == 2) ? "Fix Permissions" : "Mount Folder",
                                                       (set_menu2 == 9 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;}
        else if(max_menu2 >= 9)
        {
            //if(mnt_mode > 1) mnt_mode = 0;

            if( (!fm_pane && (!strcmpext(entries1[sel1].d_name, ".iso") || !strcmpext(entries1[sel1].d_name, ".iso.0"))) ||
                ( fm_pane && (!strcmpext(entries2[sel2].d_name, ".iso") || !strcmpext(entries2[sel2].d_name, ".iso.0"))) )
                {display_ttf_string(0, py, "Fix Game", (set_menu2 == 9 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;}
            else if((!fm_pane && use_cobra && is_ntfs_path(path1)) || ( fm_pane && use_cobra && is_ntfs_path(path2)))
                {display_ttf_string(0, py, (mnt_mode == 1) ? "Mount + Exit to XMB" : (mnt_mode == 2) ? "Make Fake ISO" : "Mount as /dev_bdvd",
                                                       (set_menu2 == 9 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;}

            //else if(!(!fm_pane && (entries1[sel1].d_type & IS_DIRECTORY)) || !(fm_pane && (entries2[sel2].d_type & IS_DIRECTORY)))
            //    {display_ttf_string(0, py, (mnt_mode == 1) ? "Mount + Exit to XMB" : (mnt_mode == 2) ? "Make Fake ISO" : "Mount as /dev_bdvd",
            //                                           (set_menu2 == 9 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;}

            else
                {display_ttf_string(0, py, "Fix Game", (set_menu2 == 9 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;}
        }
    }
}
