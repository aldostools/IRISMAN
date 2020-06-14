// called from file_manager.c
{

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

    if(new_pad & BUTTON_UP)
        ROT_DEC(set_menu2, 1, max_menu2)
    else if(new_pad & BUTTON_DOWN)
        ROT_INC(set_menu2, max_menu2, 1)
    else if((set_menu2 == 1) && ROOT_MENU)
    {
        if(new_pad & BUTTON_LEFT)
            ROT_DEC(mount_option, 0, 7)
        else if(new_pad & BUTTON_RIGHT)
            ROT_INC(mount_option, 7, 0)
        else if(new_pad & BUTTON_SELECT)
            mount_option = 0;
    }
    else if((set_menu2 == 6) && ROOT_MENU)
    {
        if(new_pad & BUTTON_LEFT)
            ROT_DEC(exit_option, 0, 2)
        else if(new_pad & BUTTON_RIGHT)
            ROT_INC(exit_option, 2, 0)
        else if(new_pad & BUTTON_SELECT)
            exit_option = 0;
    }
    else if(set_menu2 == 9)
    {
        if(new_pad & BUTTON_LEFT)
            ROT_DEC(mnt_mode, 0, 2)
        else if(new_pad & BUTTON_RIGHT)
            ROT_INC(mnt_mode, 2, 0)
        else if(new_pad & BUTTON_SELECT)
            mnt_mode = 0;
    }
    else
    if((set_menu2 == 3) && (!ROOT_MENU) && ((new_pad & BUTTON_LEFT) || (new_pad & BUTTON_RIGHT)))
    {
        ROT_INC(allow_shadow_copy, 2, 0);
    }

    if(new_pad & BUTTON_CROSS_)
    {
       char buffer1[256];
       frame = 300; //force immediate refresh

       if(options_locked)
       {
           if(set_menu2 != 7)
           {
               DrawDialogOKTimer("Locked by Parental Control", 2000.0f);
               set_menu2 = 666; // for skip
           }
       }

       if(ROOT_MENU)
       {
           if(set_menu2 == 1)
           {
               if(mount_option)
               {
                   if(bAllowNetGames && get_net_status() == SUCCESS)
                   {
                       if(mount_option == 1)
                       {
                           call_webman("/mount_ps3/net0/.");
                           DrawDialogTimer("Mounted /net_host0 as local /dev_bdvd", 2000.0f);
                       }
                       else if(mount_option == 2)
                       {
                           call_webman("/mount_ps3/net0/PKG");
                           DrawDialogTimer("Mounted /net_host0/PKG as local /dev_bdvd", 2000.0f);
                       }
                       else if(mount_option == 3)
                       {
                           call_webman("/mount_ps3/net0/VIDEO");
                           DrawDialogTimer("Mounted /net_host0/VIDEO as local /dev_bdvd", 2000.0f);
                       }
                       else if(mount_option == 4)
                       {
                           call_webman("/mount_ps3/net1/.");
                           DrawDialogTimer("Mounted /net_host1 as local /dev_bdvd", 2000.0f);
                       }
                       else if(mount_option == 5)
                       {
                           call_webman("/mount_ps3/net1/PKG");
                           DrawDialogTimer("Mounted /net_host1/PKG as local /dev_bdvd", 2000.0f);
                       }
                       else if(mount_option == 6)
                       {
                           call_webman("/mount_ps3/net1/VIDEO");
                           DrawDialogTimer("Mounted /net_host1/VIDEO as local /dev_bdvd", 2000.0f);
                       }
                       else if(mount_option == 7)
                       {
                           call_webman("/mount.ps3/unmount");
                           DrawDialogTimer("Unmounted /dev_bdvd", 2000.0f);

                           if(!strncmp(path1, "/dev_bdvd", 9)) {path1[1] = 0; nentries1 = 0;}
                           if(!strncmp(path2, "/dev_bdvd", 9)) {path2[1] = 0; nentries2 = 0;}
                       }

                       if(mount_option != 7 && file_exists("/dev_bdvd"))
                       {
                           if(!fm_pane)
                           {
                               sprintf(path1, "/dev_bdvd");
                               nentries1 = 0;
                           }
                           else
                           {
                               sprintf(path2, "/dev_bdvd");
                               nentries2 = 0;
                           }
                       }
                   }
               }
               else
               {
                   // mount/umount /dev_blind
                   if(dev_blind)
                   {
                       sys_fs_umount("/dev_blind");
                       sys_fs_umount("/dev_habib");
                       sys_fs_umount("/dev_rebug");
                       sys_fs_umount("/dev_rewrite");

                       dev_blind = false;

                       // return to root
                       if(!strncmp(path1, "/dev_blind", 10) ||
                          !strncmp(path1, "/dev_habib", 10) ||
                          !strncmp(path1, "/dev_rebug", 10) ||
                          !strncmp(path1, "/dev_rewrite", 12)) path1[1] = 0;

                       if(!strncmp(path2, "/dev_blind", 10) ||
                          !strncmp(path2, "/dev_habib", 10) ||
                          !strncmp(path2, "/dev_rebug", 10) ||
                          !strncmp(path2, "/dev_rewrite", 12)) path2[1] = 0;
                   }
                   else
                       dev_blind = (sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_blind", 0) == SUCCESS);
               }
               nentries1 = pos1 = sel1 = 0;
               nentries2 = pos2 = sel2 = 0;
           } // mount/umount /dev_blind
           else if(set_menu2 == 2 || set_menu2 == 3)
           {   // lv2 / lv1 dump
               sprintf(temp_buffer, "/%s", !fm_pane ? entries1[sel1].d_name : entries2[sel2].d_name);

               if(strncmp(temp_buffer, "/dev_flash", 10) == SUCCESS || strstr("/host_root|/app_home|/dev_bdvd|/dev_blind|/dev_habib|/dev_rewrite|/dev_rebug|/dev_ps2disc", temp_buffer) != NULL)
                   strcpy(temp_buffer, "/dev_hdd0");

               int flen = strlen(temp_buffer);

               int ret = level_dump(temp_buffer, 0x800400ULL, (set_menu2 == 3) ? 1 : 2);

               if(ret < 0)
               {
                   sprintf(MEM_MESSAGE, "Error in %s dump: 0x%08x\n\n%s", (set_menu2 == 3) ? "LV1" : "LV2", ret, getlv2error(ret));
                   DrawDialogOK(MEM_MESSAGE);
               }
               else
               if(!fm_pane)
               {
                   update_device_sizes |= 1;
                   nentries1 = pos1 = sel1 = 0;
                   strncpy(path1, temp_buffer, flen);
               }
               else
               {
                   update_device_sizes |= 2;
                   nentries2 = pos2 = sel2 = 0;
                   strncpy(path2, temp_buffer, flen);
               }
           }   // lv2 / lv1 dump
           else if(set_menu2 == 4)
           {   // RAM area editor
               hex_editor(HEX_EDIT_RAM, "RAM Area Editor", 0x10000000ULL);
               frame = set_menu2 = 0;
               continue;
           }   // RAM area editor
           else if(set_menu2 == 5)
           {   // LV2 editor
               hex_editor(HEX_EDIT_LV2, "LV2 Editor", 0x800000ULL);
               frame = set_menu2 = 0;
               continue;
           }   // LV2 Editor
           else if(set_menu2 == 6)
           {
               if(exit_option == 1)
               {
                   if(DrawDialogYesNo("Exit to XMB?") == YES) {unlink_secure("/dev_hdd0/tmp/wm_request"); SaveGameList(); fun_exit(); exit(0);}
               }
               else
               if(exit_option == 2)
               {
                   if(DrawDialogYesNo("Restart the PS3?") == YES) {unlink_secure("/dev_hdd0/tmp/wm_request"); fun_exit(); set_install_pkg = true; SaveGameList(); sys_reboot();}
               }
               else
               {
                   // Return to Game List
                   set_menu2 = 0;
                   break;
               }
               continue;
           }   // Exit File Manager
           else if(set_menu2 == 7)
           {
               // exFAT File Manager
               extern s32 fmapp_run();
               fmapp_run();

               set_menu2 = 0;
               continue;
           }
       }
       else if(set_menu2 == 1)
       {
           // new folder
           sprintf(buffer1, "%s", "New");

           if(Get_OSK_String("New Folder", buffer1, 256) == SUCCESS)
           {
                if(buffer1[0] == 0) {set_menu2 = 0; continue;}

                sprintf(MEM_MESSAGE, "Do you want to create the new folder %s\non %s ?", buffer1, !fm_pane ? path1 : path2);

                ps3pad_read();

                if((old_pad | BUTTON_L2) || DrawDialogYesNo(MEM_MESSAGE) == YES)
                {
                    exitcode = REFRESH_GAME_LIST;

                    if(!fm_pane)
                       sprintf(TEMP_PATH, "%s/%s", path1, buffer1);
                    else
                       sprintf(TEMP_PATH, "%s/%s", path2, buffer1);

                    int ret;

                    ret = mkdir_secure(TEMP_PATH);

                    if(ret < 0)
                    {
                       sprintf(MEM_MESSAGE, "New folder error: 0x%08x\n\n%s", ret, getlv2error(ret));
                       DrawDialogOK(MEM_MESSAGE);
                    }

                    if(strcmp(path1, path2) == SUCCESS)
                       {update_device_sizes |= 1|2; pos1 = sel1 = nentries1 = pos2 = sel2 = nentries2 = 0;}
                    else if(fm_pane)
                       {update_device_sizes |= 2; pos2 = sel2 = nentries2 = 0;}
                    else
                       {update_device_sizes |= 1; pos1 = sel1 = nentries1 = 0;}

                    frame = 300; //force immediate refresh
                }
           }

       } // new folder
       else if(set_menu2 == 2)
       {
           if(!fm_pane)
              strcpy(buffer1, entries1[sel1].d_name);
           else
              strcpy(buffer1, entries2[sel2].d_name);

           if(Get_OSK_String("Rename", buffer1, 256) == SUCCESS)
           {
                if(buffer1[0] == 0 || (!fm_pane && !strcmp(buffer1, entries1[sel1].d_name))
                                   || ( fm_pane && !strcmp(buffer1, entries2[sel2].d_name))) {set_menu2 = 0; continue;}

                sprintf(MEM_MESSAGE, "Do you want to rename %s\nto %s ?",
                                     !fm_pane ? entries1[sel1].d_name : entries2[sel2].d_name, buffer1);

                ps3pad_read();

                if((old_pad | BUTTON_L2) || DrawDialogYesNo(MEM_MESSAGE) == YES)
                {
                    exitcode = REFRESH_GAME_LIST;

                    if(!fm_pane)
                    {
                       sprintf(TEMP_PATH1, "%s/%s", path1, entries1[sel1].d_name);
                       sprintf(TEMP_PATH2, "%s/%s", path1, buffer1);
                    }
                    else
                    {
                       sprintf(TEMP_PATH1, "%s/%s", path2, entries2[sel2].d_name);
                       sprintf(TEMP_PATH2, "%s/%s", path2, buffer1);
                    }

                    int ret = rename_secure(TEMP_PATH1, TEMP_PATH2);

                    if(ret < 0)
                    {
                       sprintf(MEM_MESSAGE, "Rename error: 0x%08x\n\n%s", ret, getlv2error(ret));
                       DrawDialogOK(MEM_MESSAGE);
                    }

                    if(strcmp(path1, path2) == SUCCESS)
                       {update_device_sizes |= 1|2; pos1 = sel1 = nentries1 = pos2 = sel2 = nentries2 = 0;}
                    else if(fm_pane)
                       {update_device_sizes |= 2; pos2 = sel2 = nentries2 = 0;}
                    else
                       {update_device_sizes |= 1; pos1 = sel1 = nentries1 = 0;}

                    frame = 300; //force immediate refresh
                }
           }

       } // rename
       else if(set_menu2 == 3)
       {
           if(allow_shadow_copy == 2)
   		{
   			char *base_path, *dir_name, *dest_path;
   			if(!fm_pane)
   			{
   				base_path = path1;
   				dir_name =  entries1[sel1].d_name;
   				dest_path = (old_pad & BUTTON_SELECT) ? path1 : path2;
   			}
   			else
   			{
   				base_path = path2;
   				dir_name =  entries2[sel2].d_name;
   				dest_path = (old_pad & BUTTON_SELECT) ? path2 : path1;
   			}

   			if(strlen(base_path) > 10)
   			{
   				sprintf(TEMP_PATH1, "%s/%s/", base_path, dir_name);
   				if(isDir(TEMP_PATH1) == false) continue;

   				sprintf(MEM_MESSAGE, "Do you want to zip %s folder into %s?", dir_name, dest_path);
   				if(DrawDialogYesNo(MEM_MESSAGE) == YES)
   				{
   					sprintf(TEMP_PATH2, "%s/%s.zip", dest_path, dir_name);

   					msgDialogAbort();
   					sprintf(MEM_MESSAGE, "Zipping %s/%s\nTo: %s", base_path, dir_name, TEMP_PATH2);
   					DrawDialogTimer(MEM_MESSAGE, 500.0f);

   					zip_directory(base_path, TEMP_PATH1, TEMP_PATH2);

   					{update_device_sizes |= 1|2; pos1 = sel1 = nentries1 = pos2 = sel2 = nentries2 = 0;}
   					frame = 300; //force immediate refresh
   				}
   			}
   			continue;
   		}

           // copy
           int files;
           int ret = 0;

           if(!fm_pane)
           {
               if(test_mark_flags(entries1, nentries1, &files))
               {
                   // multiple
                   if((path2[1] == 0) || !strcmp(path1, path2)) {set_menu2 = 0; continue;}

                   ret = copy_file_manager(path1, path2, entries1, nentries1, -1, free_device2);
                   exitcode = REFRESH_GAME_LIST;
               }
               else
               {
                   if(!strcmp(entries1[sel1].d_name, "..") || (path2[1] == 0) || !strcmp(path1, path2)) {set_menu2 = 0; continue;}

                   ret = copy_file_manager(path1, path2, entries1, nentries1, sel1, free_device2);
                   exitcode = REFRESH_GAME_LIST;
               }
           }
           else
           {
               if(test_mark_flags(entries2, nentries2, &files))
               {
                   // multiple
                   if((path1[1] == 0) || !strcmp(path1, path2)) {set_menu2 = 0; continue;}

                   ret = copy_file_manager(path2, path1, entries2, nentries2, -1, free_device1);
                   exitcode = REFRESH_GAME_LIST;
               }
               else
               {
                   if(!strcmp(entries2[sel2].d_name, "..") || (path1[1] == 0) || !strcmp(path1, path2)) {set_menu2 = 0; continue;}

                   ret = copy_file_manager(path2, path1, entries2, nentries2, sel2, free_device1);
                   exitcode = REFRESH_GAME_LIST;
               }
           }

           if(ret < 0)
           {
               sprintf(MEM_MESSAGE, "Copy error: 0x%08x\n\n%s", ret, getlv2error(ret));
               DrawDialogOK(MEM_MESSAGE);
           }

           if(strcmp(path1, path2) == SUCCESS)
               {update_device_sizes |= 1|2; pos1 = sel1 = nentries1 = pos2 = sel2 = nentries2 = 0;}
           else if(!fm_pane)
               {update_device_sizes |= 2; pos2 = sel2 = nentries2 = 0;}
           else
               {update_device_sizes |= 1; pos1 = sel1 = nentries1 = 0;}

           frame = 300; //force immediate refresh

       } // copy
       else if(set_menu2 == 4)
       {
           // move
           int files;
           int ret = 0;

           if(!fm_pane)
           {
               if(test_mark_flags(entries1, nentries1, &files))
               {
                   // multiple
                   if((path2[1] == 0) || !strcmp(path1, path2))  {set_menu2 = 0; continue;}

                   ret = move_file_manager(path1, path2, entries1, nentries1, -1, free_device2);
                   exitcode = REFRESH_GAME_LIST;
               }
               else
               {
                   if(!strcmp(entries1[sel1].d_name, "..") || (path2[1] == 0)  || !strcmp(path1, path2)) {set_menu2 = 0; continue;}

                   ret = move_file_manager(path1, path2, entries1, nentries1, sel1, free_device2);
                   exitcode = REFRESH_GAME_LIST;
               }
           }
           else
           {
               if(test_mark_flags(entries2, nentries2, &files))
               {
                   // multiple
                   if((path1[1] == 0) || !strcmp(path1, path2)) {set_menu2 = 0; continue;}

                   ret = move_file_manager(path2, path1, entries2, nentries2, -1, free_device1);
                   exitcode = REFRESH_GAME_LIST;
               }
               else
               {
                   if(!strcmp(entries2[sel2].d_name, "..") || (path1[1] == 0)  || !strcmp(path1, path2)) {set_menu2 = 0; continue;}

                   ret = move_file_manager(path2, path1, entries2, nentries2, sel2, free_device1);
                   exitcode = REFRESH_GAME_LIST;
               }
           }

            if(ret < 0)
            {
               sprintf(MEM_MESSAGE, "Move error: 0x%08x\n\n%s", ret, getlv2error(ret));
               DrawDialogOK(MEM_MESSAGE);
            }

            update_device_sizes |= 1|2;
            nentries1 = pos1 = sel1 = 0;
            nentries2 = pos2 = sel2 = 0;
            frame = 300; //force immediate refresh

       } // move
       else if(set_menu2 == 5)
       {
           // delete
           sysFSDirent *entries;
           int nentries, sel;
           char *path;

           int files;
           int ret = 0;
           int cfiles = 0;

           if(!fm_pane)
           {
               entries = entries1;
               nentries = nentries1;
               sel = sel1;
               path = path1;
               update_device_sizes |= 1;
           }
           else
           {
               entries = entries2;
               nentries = nentries2;
               sel = sel2;
               path = path2;
               update_device_sizes |= 2;
           }

           if(test_mark_flags(entries, nentries, &files))
           {
               // multiple
               sprintf(MEM_MESSAGE, "Do you want to delete the selected Files and Folders?\n\n(%i) Items", files);

               if(DrawDialogYesNo(MEM_MESSAGE) == YES)
               {
                   exitcode = REFRESH_GAME_LIST;

                   single_bar("Deleting...");

                   float parts = 100.0f / (float) files;
                   float cpart = 0;

                   for(n = 0; n < nentries; n++)
                   {
                       if(!(entries[n].d_type & IS_MARKED)) continue; // skip no marked
                       if(progress_action == 2) break;

                       cpart += parts;
                       if(cpart >= 1.0f)
                       {
                           update_bar((u32) cpart);
                           cpart-= (float) ((u32) cpart);
                       }
                       cfiles++;

                       sprintf(temp_buffer, "%s/%s", path, entries[n].d_name);

                       if(entries[n].d_type & IS_DIRECTORY)
                       {
                           DeleteDirectory(temp_buffer);
                           if(is_ntfs_path(temp_buffer))
                               ret = ps3ntfs_unlink(temp_buffer);
                           else
                               ret = rmdir_secure(temp_buffer);

                       }
                       else
                       {
                           if(is_ntfs_path(temp_buffer))
                               ret = ps3ntfs_unlink(temp_buffer);
                           else
                               ret = unlink_secure(temp_buffer);
                       }

                       if(ret < 0) break;
                   }
                   sysUtilCheckCallback(); tiny3d_Flip();
                   msgDialogAbort();
                   usleep(250000);

                   if(ret < 0)
                   {
                       sprintf(MEM_MESSAGE, "Delete error: 0x%08x\n\n%s", ret, getlv2error(ret));
                       DrawDialogOK(MEM_MESSAGE);
                   }

                   if(strcmp(path1, path2) == SUCCESS)
                      {update_device_sizes |= 1|2; pos1 = sel1 = nentries1 = pos2 = sel2 = nentries2 = 0;}
                   else if(fm_pane)
                      {update_device_sizes |= 2; pos2 = sel2 = nentries2 = 0;}
                   else
                      {update_device_sizes |= 1; pos1 = sel1 = nentries1 = 0;}

                   frame = 300; //force immediate refresh
               }
           }
           else
           {
               if(!strcmp(entries[sel].d_name, "..")) {set_menu2 = 0; continue;}

               sprintf(MEM_MESSAGE, "Do you want to delete %s?", entries[sel].d_name);

               if(DrawDialogYesNo(MEM_MESSAGE) == YES)
               {
                   sprintf(temp_buffer, "%s/%s", path, entries[sel].d_name);

                   if(entries[sel].d_type & IS_DIRECTORY)
                   {
                       DeleteDirectory(temp_buffer);
                       if(is_ntfs_path(temp_buffer))
                           ret = ps3ntfs_unlink(temp_buffer);
                       else
                           ret = rmdir_secure(temp_buffer);
                   }
                   else
                   {
                       if(is_ntfs_path(temp_buffer))
                           ret = ps3ntfs_unlink(temp_buffer);
                       else
                           ret = unlink_secure(temp_buffer);
                   }

                   if(ret < 0)
                   {
                       sprintf(MEM_MESSAGE, "Delete error: 0x%08x\n\n%s", ret, getlv2error(ret));
                       DrawDialogOK(MEM_MESSAGE);
                   }

                   if(strcmp(path1, path2) == SUCCESS)
                      {update_device_sizes |= 1|2; pos1 = sel1 = nentries1 = pos2 = sel2 = nentries2 = 0;}
                   else if(fm_pane)
                      {update_device_sizes |= 2; pos2 = sel2 = nentries2 = 0;}
                   else
                      {update_device_sizes |= 1; pos1 = sel1 = nentries1 = 0;}

                   frame = 300; //force immediate refresh
               }
           }

       } // delete
       else if(set_menu2 == 6)
       {
           // Paste to New File
           if(copy_len == 0 || !copy_mem) {DrawDialogOKTimer("Paste buffer is empty", 2000.0f); set_menu2 = 0; continue;}

           sprintf(buffer1, "%s", "Newfile");

           if(Get_OSK_String("Paste to New File", buffer1, 256) == SUCCESS)
           {
                if(buffer1[0] == 0) {DrawDialogOKTimer("Invalid filename", 2000.0f); set_menu2 = 0; continue;}

                ps3pad_read();

                sprintf(MEM_MESSAGE, "Do you want to create the new file %s.bin\non %s ?", buffer1, !fm_pane ? path1 : path2);

                if((old_pad | BUTTON_L2) || DrawDialogYesNo(MEM_MESSAGE) == YES)
                {
                    exitcode = REFRESH_GAME_LIST;

                    if(!fm_pane)
                    {
                       update_device_sizes |= 1;
                       sprintf(temp_buffer, "%s/%s.bin", path1, buffer1);
                    }
                    else
                    {
                       update_device_sizes |= 2;
                       sprintf(temp_buffer, "%s/%s.bin", path2, buffer1);
                    }

                    s32 fd = FAILED;

                    int ret;

                    is_ntfs = is_ntfs_path(temp_buffer);

                    if(is_ntfs) {fd = ps3ntfs_open(temp_buffer, O_WRONLY | O_CREAT | O_TRUNC, 0);if(fd < 0) ret = FAILED; else ret = SUCCESS;}
                    else
                       ret = sysLv2FsOpen(temp_buffer, SYS_O_WRONLY | SYS_O_CREAT | SYS_O_TRUNC, &fd, 0777, NULL, 0);

                    if(ret == SUCCESS && fd >= SUCCESS)
                    {
                       if(!is_ntfs) sysLv2FsChmod(temp_buffer, FS_S_IFMT | 0777);
                       ret = save_hex(is_ntfs, fd, 0LL, copy_mem, copy_len);
                       if(fd >= SUCCESS) {if(is_ntfs) ps3ntfs_close(fd); else sysLv2FsClose(fd);}
                    }

                    if(ret != SUCCESS)
                    {
                       sprintf(MEM_MESSAGE, "New file error: 0x%08x\n\n%s", ret, getlv2error(ret));
                       DrawDialogOK(MEM_MESSAGE);
                    }
                    else
                    {
                       sprintf(temp_buffer, "%d Bytes written", copy_len);
                       DrawDialogOKTimer(temp_buffer, 2000.0f);
                    }

                    if(strcmp(path1, path2) == SUCCESS)
                       {update_device_sizes |= 1|2; pos1 = sel1 = nentries1 = pos2 = sel2 = nentries2 = 0;}
                    else if(fm_pane)
                       {update_device_sizes |= 2; pos2 = sel2 = nentries2 = 0;}
                    else
                       {update_device_sizes |= 1; pos1 = sel1 = nentries1 = 0;}

                    frame = 300; //force immediate refresh
                }
           }

       } // Paste to New File
       else if(set_menu2 == 7)
       {
           // Build ISO from file
           if(( fm_pane == 0 && ((path1[1] == 0) || strcmp(entries1[sel1].d_name, "..") == SUCCESS)) ||
               (fm_pane == 1 && ((path2[1] == 0) || strcmp(entries2[sel2].d_name, "..") == SUCCESS))) ;
           else if((fm_pane == 0 && (entries1[sel1].d_type & IS_DIRECTORY)) ||
                   (fm_pane == 1 && (entries2[sel2].d_type & IS_DIRECTORY)))
           {
               if(!fm_pane)
                   sprintf(TEMP_PATH1, "%s/%s", path1, entries1[sel1].d_name);
               else
                   sprintf(TEMP_PATH1, "%s/%s", path2, entries2[sel2].d_name);

               menu_screen = SCR_TOOL_BUILD_ISO;
               while(menu_screen != SCR_MENU_GAME_OPTIONS)
               {
                   cls();
                   update_twat(false);
                   draw_device_mkiso(0, 0, -1, TEMP_PATH1);
               }

               menu_screen = SCR_MAIN_GAME_LIST;
               exitcode = REFRESH_GAME_LIST;
               frame = 0;
          }
          else
          {
               sprintf(TEMP_PATH1, "%s/USRDIR/TEMP/showtime.iso", self_path);

               if(!fm_pane)
                   sprintf(TEMP_PATH2, "%s/%s", path1, entries1[sel1].d_name);
               else
                   sprintf(TEMP_PATH2, "%s/%s", path2, entries2[sel2].d_name);

               launch_iso_build(TEMP_PATH1, TEMP_PATH2, true);
          }
       } // Build ISO from file
       else if(set_menu2 == 8)
       {
           // Get file / folder info
           int nfiles = 0, nfolders = 0;
           u64 size = 0ULL;

           set_menu2 = 0;

           if((!fm_pane && (entries1[sel1].d_type & IS_DIRECTORY) && strcmp(entries1[sel1].d_name, "..") != 0) ||
              ( fm_pane && (entries2[sel2].d_type & IS_DIRECTORY) && strcmp(entries2[sel2].d_name, "..") != 0))
               DrawDialogTimer("Scanning folder ...", 500.0f);

           if(!fm_pane)
               sprintf(TEMP_PATH, "%s/%s", path1, entries1[sel1].d_name);
           else
               sprintf(TEMP_PATH, "%s/%s", path2, entries2[sel2].d_name);

           if((!fm_pane && (path1[1] != 0) && strcmp(entries1[sel1].d_name, "..")) ||
              ( fm_pane && (path2[1] != 0) && strcmp(entries2[sel2].d_name, ".."))
             )
           {
               CountFiles(TEMP_PATH, &nfiles, &nfolders, &size);

               if(!((!fm_pane && (entries1[sel1].d_type & IS_DIRECTORY)) || (fm_pane && (entries2[sel2].d_type & IS_DIRECTORY)))) nfiles = 1;

               if(size == 1LL)
                   sprintf(MEM_MESSAGE, "%s\n\n%i Files, %i Folders\n\nTotal Size: 1 Byte", TEMP_PATH, nfiles, nfolders);
               else if(size < 1024LL)
                   sprintf(MEM_MESSAGE, "%s\n\n%i Files, %i Folders\n\nTotal Size: %i Bytes", TEMP_PATH, nfiles, nfolders, (int) size);
               else
                   if(size < 0x100000LL)
                       sprintf(MEM_MESSAGE, "%s\n\n%i Files, %i Folders\n\nTotal Size: %1.1f KB (%1.0f Bytes)", TEMP_PATH, nfiles, nfolders, (double) (size  / 1024LL), (double) size);
                   else if(size < 0x40000000LL)
                       sprintf(MEM_MESSAGE, "%s\n\n%i Files, %i Folders\n\nTotal Size: %1.1f MB (%1.0f Bytes)", TEMP_PATH, nfiles, nfolders, (double) (size / 0x100000LL), (double) size);
                   else
                       sprintf(MEM_MESSAGE, "%s\n\n%i Files, %i Folders\n\nTotal Size: %1.2f GB (%1.0f Bytes)", TEMP_PATH, nfiles, nfolders, ((double) size) / GIGABYTES, (double) size);

               DrawDialogOK(MEM_MESSAGE);
           }
       } // Getfile / folder info
       else if(set_menu2 == 9 && max_menu2 >= 9)
       {
            // Fix game / Mount folder
            if(!fm_pane)
                sprintf(temp_buffer, "%s/%s", path1, entries1[sel1].d_name);
            else
                sprintf(temp_buffer, "%s/%s", path2, entries2[sel2].d_name);

            if(mnt_mode == 2)
            {
                if((fm_pane == 0 && (entries1[sel1].d_type & IS_DIRECTORY)) ||
                   (fm_pane == 1 && (entries2[sel2].d_type & IS_DIRECTORY)))
                {
                   pause_music(1);

                   // sys8_perm_mode(1);
                   DCls();
                   FixDirectory(temp_buffer, 0);
                   DCls();
                   // sys8_perm_mode(0);

                   msgDialogAbort();
                   msgDialogClose(0);

                   pause_music(0);
                }
                else if(!fm_pane && use_cobra && is_ntfs_path(path1))
                {
                    DrawDialogTimer("Creating fake ISO ...", 500.0f);

                    mkdir_secure("/dev_hdd0/tmp/wmtmp");

                    for(int s = 0; s < nentries1; s++)
                    {
                        if((entries1[s].d_type & IS_MARKED) || (selcount1==0 && s==sel1))
                        {
                            int flen=strlen(entries1[s].d_name)-4;
                            if((entries1[s].d_type & IS_DIRECTORY) || (flen>0 && strcasestr(".iso|.bin|.mdf|.img|so.0", entries1[s].d_name + flen)!=NULL)) ;
                            else
                            {
                                sprintf(temp_buffer, "%s/%s", path1, entries1[s].d_name);
                                sprintf(TEMP_PATH1, "/dev_hdd0/tmp/wmtmp/%s.iso", entries1[s].d_name);
                                launch_iso_build(TEMP_PATH1, temp_buffer, false);
                            }
                        }
                    }
                    strcpy(path1, "/dev_hdd0/tmp/wmtmp\0"); nentries1 = pos1 = sel1 = 0; update_device_sizes = 1|2;

                    if(get_net_status() != SUCCESS) continue;
                    call_webman("/refresh.ps3");
                }
                else if(fm_pane && use_cobra && is_ntfs_path(path2))
                {
                    DrawDialogTimer("Creating fake ISO ...", 500.0f);

                    mkdir_secure("/dev_hdd0/tmp/wmtmp");

                    for(int s = 0; s < nentries2; s++)
                    {
                        if((entries2[s].d_type & IS_MARKED) || (selcount2==0 && s==sel2))
                        {
                            int flen=strlen(entries2[s].d_name)-4;
                            if((entries2[s].d_type & IS_DIRECTORY) || (flen>0 && strcasestr(".iso|.bin|.mdf|.img|so.0", entries2[s].d_name + flen)!=NULL)) ;
                            else
                            {
                                sprintf(temp_buffer, "%s/%s", path2, entries2[s].d_name);
                                sprintf(TEMP_PATH2, "/dev_hdd0/tmp/wmtmp/%s.iso", entries2[s].d_name);
                                launch_iso_build(TEMP_PATH2, temp_buffer, false);
                            }
                        }
                    }
                    strcpy(path2, "/dev_hdd0/tmp/wmtmp\0"); nentries2 = pos2 = sel2 = 0; update_device_sizes = 1|2;

                    if(get_net_status() != SUCCESS) continue;
                    call_webman("/refresh.ps3");
                }
                else
                    sysLv2FsChmod(temp_buffer, FS_S_IFMT | 0777);

                mnt_mode  = 1;
                set_menu2 = 0;
                continue;
            }

            if( (!fm_pane && (!strcmpext(entries1[sel1].d_name, ".iso") || !strcmpext(entries1[sel1].d_name, ".iso.0"))) ||
                ( fm_pane && (!strcmpext(entries2[sel2].d_name, ".iso") || !strcmpext(entries2[sel2].d_name, ".iso.0"))) )
            {
                if((!fm_pane && selcount1<2) || (fm_pane && selcount2<2))
                    patchps3iso(temp_buffer, 0);
                else if(fm_pane == 0)
                {
                    for(int s = 0; s < nentries1; s++)
                    {
                        if((entries1[s].d_type & IS_MARKED) &&
                           (!strcmpext(entries1[s].d_name, ".iso") || !strcmpext(entries1[s].d_name, ".iso.0")))
                        {
                            sprintf(temp_buffer, "%s/%s", path1, entries1[s].d_name);
                            patchps3iso(temp_buffer, 1);
                        }
                    }
                }
                else if(fm_pane == 1)
                {
                    for(int s = 0; s < nentries2; s++)
                    {
                        if((entries2[s].d_type & IS_MARKED) &&
                           (!strcmpext(entries2[s].d_name, ".iso") || !strcmpext(entries2[s].d_name, ".iso.0")))
                        {
                            sprintf(temp_buffer, "%s/%s", path2, entries2[s].d_name);
                            patchps3iso(temp_buffer, 1);
                        }
                    }
                }
            }

            //else if(use_cobra && !((!fm_pane && (entries1[sel1].d_type & IS_DIRECTORY)) || (fm_pane && (entries2[sel2].d_type & IS_DIRECTORY))))
            else if(use_cobra && ( (!fm_pane && is_ntfs_path(path1)) || (fm_pane && is_ntfs_path(path2)) ))
            {
                sprintf(TEMP_PATH1, "%s/USRDIR/TEMP/pkg.iso", self_path);

                if(!fm_pane)
                    sprintf(TEMP_PATH2, "%s/%s", path1, entries1[sel1].d_name);
                else
                    sprintf(TEMP_PATH2, "%s/%s", path2, entries2[sel2].d_name);

                int flen = strlen(TEMP_PATH2) - 4;

                cobra_send_fake_disc_eject_event();
                usleep(4000);
                cobra_umount_disc_image();

                cobra_unset_psp_umd();
                sys_map_path((char*)"/dev_bdvd", NULL);
                sys_map_path((char*)"/app_home", NULL);

                if(flen >= 0 && (strcasestr(".iso|.bin|.mdf|.img|so.0", TEMP_PATH2 + flen) != NULL))
                    launch_iso_game(TEMP_PATH2, DETECT_EMU_TYPE);
                else
                {
                    unlink_secure(TEMP_PATH1);
                    launch_iso_build(TEMP_PATH1, TEMP_PATH2, false);

                    if(!fm_pane)
                       {strcpy(path1, "/dev_bdvd\0"); nentries1 = pos1 = sel1 = 0;}
                    else
                       {strcpy(path2, "/dev_bdvd\0"); nentries2 = pos2 = sel2 = 0;}
                }

                if((mnt_mode == 1) || (old_pad & BUTTON_SELECT)) {SaveGameList(); fun_exit(); exit(0);}
            }
            else
            if(is_dir)
            {
                cobra_send_fake_disc_eject_event();
                usleep(4000);
                cobra_umount_disc_image();

                sys_map_path("/dev_bdvd", temp_buffer);
                sys_map_path("/app_home", temp_buffer);

                cobra_send_fake_disc_insert_event();

                if(!fm_pane)
                   {strcpy(path1, "/dev_bdvd\0"); nentries1 = pos1 = sel1 = 0;}
                else
                   {strcpy(path2, "/dev_bdvd\0"); nentries2 = pos2 = sel2 = 0;}

                if(mnt_mode == 1 || (old_pad & BUTTON_SELECT)) {SaveGameList(); fun_exit(); exit(0);}
            }
            else
            {
               pause_music(1);
               patch_error_09(temp_buffer, 0);

               DCls();
               FixDirectory(temp_buffer, 0);
               DCls();

               msgDialogAbort();
               msgDialogClose(0);

               pause_music(0);
            }

       } // fix game

       set_menu2 = 0;
       continue;
    }
}
