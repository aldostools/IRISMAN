int param_sfo_util(char * path, int patch_app)
{
    bool patched = false;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;
    char file[MAX_PATH_LEN];
    char file2[MAX_PATH_LEN];

    u8 * app_ver = NULL;

    char str_version[8];

    unsigned char *mem = NULL;
    unsigned char *mem2 = NULL;

    int n;
    char * version = LoadFile("/dev_flash/vsh/etc/version.txt", &n);

    if(!version) return -2;

    char *v = strstr(version, "release:");

    if(v) {memcpy(str_version, v + 8, 7); str_version[7] = 0;}

    free(version);

    if(!v) return -3;


    sprintf(file, "%s/PS3_GAME/PARAM.SFO", path);
    sprintf(file2, "%s/PS3_GAME/_PARAM.SFO", path);

    if(!sysLv2FsOpen(file2, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        unsigned len, pos, str;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        mem2 = (unsigned char *) malloc(len + 16);
        if(!mem2) {sysLv2FsClose(fd); return -2;}

        memset(mem2, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem2, len, &bytes) != 0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem2[8] + (mem2[9]<<8));
        pos = (mem2[0xc] + (mem2[0xd]<<8));

        int indx = 0;

        while(str < len)
        {
            if(mem2[str] == 0) break;

            if(!strcmp((char *) &mem2[str], "APP_VER")) app_ver = &mem2[pos];

            while(mem2[str] && str < len) str++; str++;
            pos += (mem2[0x1c + indx] + (mem2[0x1d + indx]<<8));
            indx += 16;
        }

    }

    n = sysLv2FsOpen(file, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0);

    if(n) {n = sysLv2FsOpen(file2, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0); patched = true;}

    if(!n)
    {
        unsigned len, pos, str;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes) != 0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem[8] + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0;

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strcmp((char *) &mem[str], "PS3_SYSTEM_VER"))
            {
                if(strcmp((char *) &mem[pos], str_version) > 0)
                {
                    memcpy(&mem[pos], str_version, 8);
                    patched = true;
                }
            }
            else
            if(!strcmp((char *) &mem[str], "APP_VER"))
            {
               u8 old =  mem[pos + 1];

               if(app_ver)
               {
                   mem[pos + 1] = app_ver[1];
                   if(mem[pos + 1] == '9') mem[pos + 1] = '1';
               } else mem[pos + 1] = '1';

               if(patch_app)
               {
                    mem[pos + 1] = '9';
               }

               if(old != mem[pos + 1]) patched = true;
            }

            while(mem[str] && str < len) str++; str++;
            pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
            indx += 16;
        }

        if(patched)
        {
            sysLv2FsRename(file, file2);
            SaveFile(file, (char *) mem, len);
        }

        if(mem) free(mem);
        if(mem2) free(mem2);

        return SUCCESS;
    }

    return FAILED;
}

int param_sfo_patch_category_to_cb(char * path_src, char *path_dst)
{
    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;
    char file[MAX_PATH_LEN];
    char file2[MAX_PATH_LEN];

    unsigned char *mem2 = NULL;

    sprintf(file, "%s", path_src);
    sprintf(file2, "%s", path_dst);

    if(!sysLv2FsOpen(file, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        unsigned len, pos, str;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        mem2 = (unsigned char *) malloc(len + 16);
        if(!mem2)
        {
            sysLv2FsClose(fd);
            return -2;
        }

        memset(mem2, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem2, len, &bytes)!=0)
            bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem2[8] + (mem2[9]<<8));
        pos = (mem2[0xc] + (mem2[0xd]<<8));
        int indx = 0;

        while(str < len)
        {
            if(mem2[str] == 0)
                break;

            if(!strcmp((char *) &mem2[str], "CATEGORY"))
            {
                mem2[pos] = 'H';
                mem2[pos+1] = 'G';
            }

            if(!strcmp((char *) &mem2[str], "APP_VER"))
            {
                mem2[pos] = '9'; mem2[pos + 1] = '9'; mem2[pos + 3] = '9'; mem2[pos + 4] = '9';
            }

            while(mem2[str] && str < len) str++; str++;
            pos  += (mem2[0x1c + indx] + (mem2[0x1d + indx]<<8));
            indx += 16;
        }

        if(SaveFile(file2, (char *) mem2, len))
        {
            free(mem2);
            return -2;
        }

        if(mem2) free(mem2);

        return SUCCESS;
    }

    return FAILED;
}

int get_field_param_sfo(char *file, char *fieldname, char *value, int field_len)
{
    if(is_ntfs_path(file) || strlen(file) <= 4) return FAILED;
    if(strncmp(file + strlen(file) - 4, ".SFO", 4)) return FAILED;
    if(!file_exists(file)) return FAILED;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    unsigned pos, str, len = 0;
    unsigned char *mem = NULL;

    if(!sysLv2FsOpen(file, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes) != 0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem[8] + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0, fieldname_len;

        fieldname_len = strlen(fieldname);

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strncmp((char *) &mem[str], fieldname, fieldname_len + 1))
            {
                memcpy(value, (char *) &mem[pos], field_len);
                value[field_len] = 0;

                free(mem);
                return SUCCESS;
            }
            while(mem[str] && str < len) str++; str++;
            pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
            indx += 16;
        }


        if(strcmp(fieldname, "APP_VER") == SUCCESS)
        {
            str = (mem[8] + (mem[9]<<8));
            pos = (mem[0xc] + (mem[0xd]<<8));

            indx = 0;

            while(str < len)
            {
                if(mem[str] == 0) break;

                if(!strncmp((char *) &mem[str], "VERSION", 8))
                {
                    memcpy(value, (char *) &mem[pos], 5);
                    value[field_len] = 0;

                    free(mem);
                    return SUCCESS;
                }
                while(mem[str] && str < len) str++; str++;
                pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
                indx += 16;
            }
        }

        if(mem) free(mem);
    }

    return FAILED;
}

int edit_title_param_sfo(char * file)
{
    int ret = FAILED;

    bool more = false;

    char sfo[MAX_PATH_LEN]; // sfo path + file name

    char title_name[64];

    char title_id[10] = "         ";
    char ps3_sys_ver[8] = "00.0000";

    char sub_title[128];
    char savedata_directory[64];

    if(!strncmp(file, "/bdvd", 5) || is_ntfs_path(file) || strlen(file) <= 4)
    {
        DrawDialogOKTimer("PARAM.SFO cannot be edited on this device", 3000.0f);
        return FAILED;
    }
    if(strncmp(file + strlen(file) - 4, ".SFO", 4)) return FAILED;
    if(!file_exists(file)) return FAILED;

    strcpy(sfo, file);

    ret = parse_param_sfo(sfo, title_name);

    if(Get_OSK_String("New Title", title_name, 63) == SUCCESS)
    {
        ps3pad_poll();
        if(old_pad & (BUTTON_SELECT | BUTTON_L1 | BUTTON_R1 | BUTTON_L2 | BUTTON_R2))
        {
            if(strcmp((const char *) game_category, "SD") == SUCCESS)
            {
                ret = get_field_param_sfo(sfo, "SUB_TITLE", sub_title, 127);
                if(Get_OSK_String("Subtitle", sub_title, 127) != SUCCESS) return FAILED;

                ret = get_field_param_sfo(sfo, "SAVEDATA_DIRECTORY", savedata_directory, 63);
                if(Get_OSK_String("Save Data Directory", savedata_directory, 63) != SUCCESS) return FAILED;
            }
            else
            {
                ret = get_field_param_sfo(sfo, "TITLE_ID", title_id, 9);
                if(Get_OSK_String("Title ID", title_id, 9) != SUCCESS) return FAILED;

                ret = get_field_param_sfo(sfo, "PS3_SYSTEM_VER", ps3_sys_ver, 7);
                if(Get_OSK_String("PS3 System Version", ps3_sys_ver, 7) != SUCCESS) return FAILED;
            }

            more = true;
        }

        Lv2FsFile fd;
        u64 bytes;
        u64 position = 0LL;

        if(!sysLv2FsOpen(sfo, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
        {
            unsigned len, pos, str;
            unsigned char *mem = NULL;

            sysLv2FsLSeek64(fd, 0, 2, &position);
            len = (u32) position;

            if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

            mem = (unsigned char *) malloc(len + 16);
            if(!mem) {sysLv2FsClose(fd); return -2;}

            memset(mem, 0, len + 16);

            sysLv2FsLSeek64(fd, 0, 0, &position);

            if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes = 0LL;

            len = (u32) bytes;

            sysLv2FsClose(fd);

            str = (mem[8] + (mem[9]<<8));
            pos = (mem[0xc] + (mem[0xd]<<8));

            int indx = 0;
            int ct = 0;

            while(str < len)
            {
                if(mem[str] == 0) break;

                if(!strncmp((char *) &mem[str], "TITLE", 6))
                {
                    title_name[63] = 0;
                    for(int i = 0; title_name[i] && (i < 64); i++) mem[pos + i] = title_name[i];
                    ct++;
                }
                else if(more)
                {
                    if(!strncmp((char *) &mem[str], "TITLE_ID", 8))
                    {
                        title_id[9] = 0;
                        for(int i = 0; title_id[i] && (i < 10); i++) mem[pos + i] = title_id[i];
                        ct++;
                    }
                    else if(!strncmp((char *) &mem[str], "PS3_SYSTEM_VER", 14))
                    {
                        ps3_sys_ver[7] = 0;
                        for(int i = 0; ps3_sys_ver[i] && (i < 7); i++) mem[pos + i] = ps3_sys_ver[i];
                        ct++;
                    }
                    else if(!strncmp((char *) &mem[str], "SUB_TITLE", 9))
                    {
                        sub_title[127] = 0;
                        for(int i = 0; sub_title[i] && (i < 128); i++) mem[pos + i] = sub_title[i];
                        ct++;
                    }
                    else if(!strncmp((char *) &mem[str], "SAVEDATA_DIRECTORY", 18))
                    {
                        savedata_directory[63] = 0;
                        for(int i = 0; savedata_directory[i] && (i < 64); i++) mem[pos + i] = savedata_directory[i];
                        ct++;
                    }
                }

                if(ct >= (more ? 3 : 1)) break;

                while(mem[str] && str < len) str++; str++;
                pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
                indx += 16;
            }

            ret = FAILED;

            if(mem)
            {
                ret = SaveFile(sfo, (char *) mem, len);
                free(mem);
            }

            return ret;
        }
        else
            DrawDialogOKTimer("Error opening PARAM.SFO", 3000.0f);
    }

    return FAILED;
}

int parse_param_sfo(char * file, char *title_name)
{
    if(is_ntfs_path(file) || strlen(file) <= 4) return FAILED;
    if(strncmp(file + strlen(file) - 4, ".SFO", 4)) return FAILED;
    if(!file_exists(file)) return FAILED;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    if(!sysLv2FsOpen(file, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0))
    {
        unsigned len, pos, str;
        unsigned char *mem = NULL;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes = 0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem[8] + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0;
        int ct = 0;

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strncmp((char *) &mem[str], "TITLE", 6))
            {
                strncpy(title_name, (char *) &mem[pos], 63);
                title_name[63] = 0;
                ct++;
            }
            else if(!strncmp((char *) &mem[str], "CATEGORY", 8))
            {
                memcpy((char *) game_category, (char *) &mem[pos], 2);
                game_category[2] = 0;
                ct++;
            }

            if(ct >= 2)
            {
                free(mem);
                return SUCCESS;
            }

            while(mem[str] && str < len) str++; str++;
            pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
            indx += 16;
        }

        if(mem) free(mem);
    }

    return FAILED;
}

int mem_parse_param_sfo(u8 *mem, u32 len, char *field, char *value)
{
    u32 pos, str;

    if(mem[0] !=0 || mem[1] != 0x50 || mem[2] != 0x53 || mem[3] != 0x46) return -1; // unknown file!

    str = (mem[8] + (mem[9]<<8));
    pos = (mem[0xc] + (mem[0xd]<<8));

    int indx = 0;

    while(str < len)
    {
        if(mem[str] == 0) break;

        if(!strcmp((char *) &mem[str], field))
        {
            strncpy(value, (char *) &mem[pos], 63);
            if(!strncmp(field, "TITLE_ID", 8) && value[4] != 0x2D)
            {
                sprintf(value, "%c%c%c%c-%c%c%c%c%c", value[0], value[1], value[2], value[3],
                                                      value[4], value[5], value[6], value[7], value[8]);
            }
            value[63] = 0;
            return SUCCESS;
        }

        while(mem[str] && str < len) str++; str++;
        pos  += (mem[0x1c+indx] + (mem[0x1d+indx]<<8));
        indx += 16;
    }

    return FAILED;

}

int parse_iso_titleid(char * path_iso, char * title_id)
{
    if(get_filesize(path_iso) < 0x9320LL) return -2;

    Lv2FsFile fd;
    u64 bytes;
    u64 position = 0LL;

    unsigned char *mem = NULL;

    int n, flag;

    flag = FAILED;

    unsigned pos, len = (u32) 0x6000;

    bool is_ntfs = is_ntfs_path(path_iso);

    if(is_ntfs)
    {
        int fd = ps3ntfs_open(path_iso, O_RDONLY, 0777);
        if(fd >= SUCCESS)
        {
            if(ps3ntfs_seek64(fd, 0x9320LL, SEEK_SET) != 0x9320LL) {ps3ntfs_close(fd); return -2;}

            mem = (unsigned char *) malloc(len + 16);
            if(!mem) {ps3ntfs_close(fd); return -2;}
            memset(mem, 0, len + 16);

            bytes = ps3ntfs_read(fd, (void *) mem, len);

            ps3ntfs_close(fd);
        }
    }
    else
    {
        sysLv2FsChmod(path_iso, FS_S_IFMT | 0777);
        n = sysLv2FsOpen(path_iso, 0, &fd, S_IREAD | S_IRGRP | S_IROTH, NULL, 0);

        if(!n)
        {
            sysLv2FsLSeek64(fd, 0x9320LL, 0, &position);
            if(position != 0x9320LL) {sysLv2FsClose(fd); return -2;}

            mem = (unsigned char *) malloc(len + 16);
            if(!mem) {sysLv2FsClose(fd); return -2;}
            memset(mem, 0, len + 16);

            if(sysLv2FsRead(fd, mem, len, &bytes) != 0) bytes = 0LL;

            len = (u32) bytes;

            sysLv2FsClose(fd);
        }
    }


    pos = 0;
    if(bytes > 11) len = (u32) bytes - 11; else len = 0;

    while(pos < len)
    {
        if(mem[pos] == 'S' && mem[pos + 8] == '.')
        {
            if(!strncmp((char *) &mem[pos], "SLUS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCUS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLES_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCES_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLPM_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLPS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCPM_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCPS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SIPS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCPS_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLUD_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCUD_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SLED_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "SCED_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
        }
        else if(mem[pos] == 'P' && mem[pos + 8] == '.')
        {
            if(!strncmp((char *) &mem[pos], "PAPX_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "PBPX_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "PCPX_", 5))
            {
                memcpy(title_id, (char *) &mem[pos], 11);
                flag = SUCCESS;
                pos = len;
            }
        }
        else if(mem[pos] == 'N' && mem[pos + 1] == 'P')
        {
            if(!strncmp((char *) &mem[pos], "NPUZ", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPEZ", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPUH", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPEH", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPJH", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPUG", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPEG", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "NPJG", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
        }
        else if(mem[pos] == 'U')
        {
            if(!strncmp((char *) &mem[pos], "ULUS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "ULES", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "ULJS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "ULAS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCJM", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCUS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCES", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCJS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCAS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCES", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
            else if(!strncmp((char *) &mem[pos], "UCKS", 4))
            {
                memcpy(title_id, (char *) &mem[pos], 10);
                flag = SUCCESS;
                pos = len;
            }
        }

        pos++;
    }

    if(mem) free(mem);
    return flag;
}

int parse_param_sfo_id(char * file, char *title_id)
{
    char titleid[16];

    int ret = get_field_param_sfo(file, "TITLE_ID", titleid, 9);

    sprintf(title_id, "%c%c%c%c-%c%c%c%c%c", titleid[0], titleid[1], titleid[2], titleid[3],
                                             titleid[4], titleid[5], titleid[6], titleid[7], titleid[8]);

    return ret;
}

int parse_param_sfo_appver(char *file, char *app_ver)
{
    return get_field_param_sfo(file, "APP_VER", app_ver, 5);
}
