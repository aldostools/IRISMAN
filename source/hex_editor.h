#define FD_LV1 -1
#define FD_LV2 -2

static u64 begin_lv1 = 0x0;
static u64 begin_lv2 = 0x0;
static u64 size_lv1 = 0x10000000ULL;
static u64 size_lv2 = 0x800000ULL;


int read_LV1(u64 pos, char *mem, int size)
{
    int n = 0;
    u64 temp;

    lv1_reg regs_i, regs_o;

    if(begin_lv1 > pos) return (int) 0x8001002B;

    if(pos + ((u64) size) > size_lv1)  size -= (int) (pos + ((u64) size) - size_lv1);

    if(firmware < 0x421C) return (int) 0x80010009;

    temp = pos;

    if(temp & 7)
    {
        n= 8 - (pos & 7);
        if(n > size) n = size;
        temp&= ~7ULL;
        regs_i.reg3 = temp;
        regs_i.reg11 = 0xB6;
        sys8_lv1_syscall(&regs_i, &regs_o);
        memcpy(&mem[0], ((char *) &regs_o.reg4) + (pos & 7), n);

        temp += 8;
    }

    if(n < size)
    {
        for(; n < size; n += 8)
        {
            regs_i.reg3 = temp;
            regs_i.reg11 = 0xB6;
            sys8_lv1_syscall(&regs_i, &regs_o);
            memcpy(&mem[n], ((char *) &regs_o.reg4), ((n+8) > size) ? (size - n) : 8);
            temp += 8;
        }
    }

    return SUCCESS;
}

int read_LV2(u64 pos, char *mem, int size)
{
    int n = 0;
    u64 temp;
    u64 temp2;

    if(begin_lv2 > pos) return (int) 0x8001002B;

    if(pos + ((u64) size) > size_lv2)  size -= (int) (pos + ((u64) size) - size_lv2);

    temp = pos;

    if(temp & 7)
    {
        n= 8 - (pos & 7);
        if(n > size) n = size;
        temp&= ~7ULL;
        temp2 = lv2peek(0x8000000000000000ULL + temp);

        memcpy(&mem[0], ((char *) &temp2) + (pos & 7), n);

        temp += 8;
    }

    if(n < size)
    {
        for(; n < size; n += 8)
        {
            temp2 = lv2peek(0x8000000000000000ULL + temp);
            memcpy(&mem[n], ((char *) &temp2), ((n+8) > size) ? (size - n) : 8);
            temp += 8;
        }
    }

    return SUCCESS;
}

int write_LV1(u64 pos, char *mem, int size)
{
    int n = 0;
    u64 temp;

    lv1_reg regs_i, regs_o;

    if(begin_lv1 > pos) return (int) 0x8001002B;

    if(pos + ((u64) size) > size_lv1)  size -= (int) (pos + ((u64) size) - size_lv1);

    if(firmware < 0x421C) return (int) 0x80010009;

    temp = pos;

    if(temp & 7)
    {
        n= 8 - (pos & 7);
        if(n > size) n = size;
        temp&= ~7ULL;
        regs_i.reg3 = temp;
        regs_i.reg11 = 0xB6;
        sys8_lv1_syscall(&regs_i, &regs_o);
        memcpy(((char *) &regs_o.reg4) + (pos & 7), &mem[0], n);

        regs_i.reg4 = regs_o.reg4;
        regs_i.reg11 = 0xB7;
        sys8_lv1_syscall(&regs_i, &regs_o);

        temp += 8;
    }

    if(n < size)
    {
        for(; n < size; n += 8)
        {
            regs_i.reg3 = temp;
            regs_i.reg11 = 0xB6;
            sys8_lv1_syscall(&regs_i, &regs_o);
            memcpy(((char *) &regs_o.reg4), &mem[n], ((n+8) > size) ? (size - n) : 8);
            regs_i.reg4 = regs_o.reg4;
            regs_i.reg11 = 0xB7;
            sys8_lv1_syscall(&regs_i, &regs_o);
            temp += 8;
        }
    }

    return SUCCESS;
}

int write_LV2(u64 pos, char *mem, int size)
{
    int n = 0;
    u64 temp;
    u64 temp2;

    temp = pos;

    if(begin_lv2 > pos) return (int) 0x8001002B;

    if(pos + ((u64) size) > size_lv2)  size -= (int) (pos + ((u64) size) - size_lv2);

    if(temp & 7)
    {
        n= 8 - (pos & 7);
        if(n > size) n = size;
        temp &= ~7ULL;

        temp2 = lv2peek(0x8000000000000000ULL + temp);
        memcpy(((char *) &temp2) + (pos & 7), &mem[0], n);
        lv2poke(0x8000000000000000ULL + temp, temp2);

        temp += 8;
    }

    if(n < size)
    {
        for(; n < size; n += 8)
        {
            temp2 = lv2peek(0x8000000000000000ULL + temp);
            memcpy(((char *) &temp2), &mem[n], ((n+8) > size) ? (size - n) : 8);
            lv2poke(0x8000000000000000ULL + temp, temp2);
            temp += 8;
        }
    }

    return SUCCESS;
}

static int load_hex(bool is_ntfs, s32 fd, u64 pos, void *buffer, u64 readed)
{
    int ret;
    u64 temp = 0;

    if(fd == FD_LV1)
    {
        ret = read_LV1(pos, buffer, (int) readed);

        if(ret != 0)
        {
            sprintf(MEM_MESSAGE, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);
        }

        return ret;
    }
    else if(fd == FD_LV2)
    {

        ret = read_LV2(pos, buffer, (int) readed);

        if(ret != 0)
        {
            sprintf(MEM_MESSAGE, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);
        }

        return ret;
    }

    if(is_ntfs)
    {
        temp = ps3ntfs_seek64(fd, pos, 0);
        if(temp < 0) ret = FAILED; else ret = SUCCESS;
    }
    else
        ret = sysLv2FsLSeek64(fd, pos, 0, &temp);

    if(ret < 0 || pos != temp)
    {
        if(ret == 0) ret = (int) 0x8001001E;
        sprintf(MEM_MESSAGE, "Lseek Error: 0x%08x\n\n%s", ret, getlv2error(ret));
        DrawDialogOK(MEM_MESSAGE);

    }
    else
    {
        temp = 0;
        if(is_ntfs)
        {
            ret = ps3ntfs_read(fd, buffer, readed);
            temp = (u64) ret; if(ret > 0) ret = 0;
        }
        else
            ret = sysLv2FsRead(fd, buffer, readed, &temp);

        if(ret < 0 || readed != temp)
        {
            if(ret == 0) ret = (int) 0x8001002B;
            sprintf(MEM_MESSAGE, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);
        }
    }

    return ret;
}

static int save_hex(bool is_ntfs, s32 fd, u64 pos, void *buffer, u64 readed)
{
    int ret;
    u64 temp = 0;

    if(fd == FD_LV1)
    {
        ret = write_LV1(pos, buffer, (int) readed);

        if(ret != 0)
        {
            sprintf(MEM_MESSAGE, "Write Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);
        }

        return ret;
    }
    else if(fd == FD_LV2)
    {
        ret = write_LV2(pos, buffer, (int) readed);

        if(ret != 0)
        {
            sprintf(MEM_MESSAGE, "Write Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);
        }

        return ret;
    }

    if(is_ntfs)
    {
        temp = ps3ntfs_seek64(fd, pos, 0);
        if(temp < 0) ret = FAILED; else ret = SUCCESS;
    }
    else
        ret = sysLv2FsLSeek64(fd, pos, 0, &temp);

    if(ret < 0 || pos != temp) {
        if(ret == 0) ret = (int) 0x8001001E;
        sprintf(MEM_MESSAGE, "Lseek Error: 0x%08x\n\n%s", ret, getlv2error(ret));
        DrawDialogOK(MEM_MESSAGE);

    }
    else
    {
        temp = 0;
        if(is_ntfs)
        {
            ret = ps3ntfs_write(fd, buffer, readed);
            temp = (u64) ret; if(ret>0) ret = 0;
        }
        else
            ret = sysLv2FsWrite(fd, buffer, readed, &temp);

        if(ret < 0 || readed != temp)
        {
            if(ret == 0) ret = (int) 0x8001002B;
            sprintf(MEM_MESSAGE, "Write Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);
        }
    }

    return ret;
}

int memcmp_case(char * p1, char *p2, int len)
{
    int n;
    char a, b;

    for(n = 0; n < len; n++)
    {
        a = *p1++; b = *p2++;
        if(a >= 'A' && a <= 'Z') a += 32;
        if(b >= 'A' && b <= 'Z') b += 32;
        if(a != b) return FAILED;
    }

    return SUCCESS;
}

static int find_mode = FIND_HEX_MODE;

static int find_in_file(bool is_ntfs, s32 fd, u64 pos, u64 size, u64 *found, void * str, int len, int s)
{

    u64 temp;
    u64 readed = 0;
    u32 n;
    int ret = 0;
    int flag = 2;
    u64 pos0 = pos;

    *found = TEXTNOTFOUND;

    char *mem =  malloc(0x8208);
    if(!mem) return (int) 0x80010004;
    memset(mem, 0, 0x8200);

    single_bar("finding in file...");

    float parts;

    if(s < 0) parts = 100.0f / ((double) pos / (double) 0x8000);
    else parts = 100.0f / ((double) (size - pos) / (double) 0x8000);

    float cpart = (s >= 0) ? parts * ((double) pos / (double) 0x8000) : 0;

    while((s >= 0 && pos < size) || (s < 0 && pos >= 0 && flag))
    {
        if(fd == FD_LV1 || fd == FD_LV2)
        {
            ret = 0;
            if(fd == FD_LV1) {if(begin_lv1 > pos) ret =(int) 0x8001001E; if(pos >= size_lv1) temp = 0; else  temp = pos;}
            if(fd == FD_LV2) {if(begin_lv2 > pos) ret =(int) 0x8001001E; if(pos >= size_lv2) temp = 0; else  temp = pos;}
        }
        else
        {
            if(is_ntfs)
                {temp = ps3ntfs_seek64(fd, pos, 0); if(temp < 0) ret = FAILED; else ret = SUCCESS;}
            else
                ret = sysLv2FsLSeek64(fd, pos, 0, &temp);
        }

        if(ret < 0 || pos != temp)
        {
            if(ret == 0) ret = (int) 0x8001001E;
            msgDialogAbort();
            sprintf(MEM_MESSAGE, "Lseek Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);
            goto skip;

        }

        readed = size - pos; if(readed > 0x8200ULL) readed = 0x8200ULL;

        if(fd == FD_LV1)
        {
            ret = read_LV1(pos, mem, (int) readed);
            temp = readed;
        }
        else if(fd == FD_LV2)
        {
            ret = read_LV2(pos, mem, (int) readed);
            temp = readed;
        }
        else
        {
            if(is_ntfs)
            {
                ret = ps3ntfs_read(fd, mem, readed);
                temp = (u64) ret; if(ret > 0) ret = 0;
            }
            else
                ret =sysLv2FsRead(fd, mem, readed, &temp);
        }

        if(ret < 0 || readed != temp)
        {
            if(ret == 0) ret = 0x8001000C;
            msgDialogAbort();
            sprintf(MEM_MESSAGE, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);

            goto skip;
        }

        readed-= 0x200ULL * (readed == 0x8200ULL); // resta area solapada de busqueda

        for(n = 0; n < (u32) readed; n++)
        {
            if(find_mode == FIND_CASE_INSENSITIVE_MODE)
            {
                if(((pos + (u64) n) < pos0 || s >= 0) && !memcmp_case(mem + n, str, len))
                {
                    *found = pos + (u64) n;
                    goto skip;
                }
            }
            else if(((pos + (u64) n) < pos0 || s >= 0) && !memcmp(mem + n, str, len))
            {
                *found = pos + (u64) n;
                goto skip;
            }
        }

        if(s >= 0)
        {
            pos += readed;
        }
        else
        {
            if(pos < readed) {pos = 0ULL; flag--;} else  pos -= readed;
        }

        if(progress_action == 2) {ret = 0x8001000C; goto skip;}

        cpart += parts;
        if(cpart >= 1.0f)
        {
            update_bar((u32) cpart);
            cpart-= (float) ((u32) cpart);
        }
    }

skip:
    if(mem) free(mem);
    msgDialogAbort();

    return ret;
}

static char help2[] = {
    "HELP - [ Hex Editor]\n"
    "\n"
    "CROSS   - Select the option\n"
    "UP/DOWN - Select option\n"
    "\n"
    "START   - Open/Close this menu\n"
    "CIRCLE  - Exit\n"
};

static char help3[] = {
    "SELECT   - Show help window\n"
    "CROSS    - Increase the selected nibble\n"
    "TRIANGLE - Decrease the selected nibble\n"
    "L2/R2    - Decrease/increase the selected byte (pressing to auto-repeat)\n"
    "\n"
    "SQUARE   - Mark Area / Undo the changes in selected byte \n"
    "SELECT + SQUARE - Undo the current windows changes\n"
    "\n"
    "START    - Opens menu selector (go to, find..)\n"
    "CIRCLE   - Exit from Hex Editor\n"
    "\n"
    "UP/DOWN/LEFT/RIGHT - Move the cursor\n"
    "\n"
    "L1/R1 - Move back/forward from the file\n"
    "L3/R3 - Find back/forward\n"
    "\n"
    "Special Note: Changes in the window must be saved to use some actions with implicit changes"
    "in the editor window. Pressing CIRCLE you can save or discard the window changes.\n"
};

static char help4[] = {
    "SELECT   - Show this help window\n"
    "\n"
    "CROSS    - Set the hex number\n"
    "SQUARE   - Delete the last digit\n"
    "\n"
    "UP/DOWN/LEFT/RIGHT - Move the cursor to the keyboard\n"
    "L2/R2    - Move back/forward from the number window\n"
    "\n"
    "START    - Go to the Address\n"
    "\n"
    "CIRCLE   - Close the window\n"
    "\n"
    "Special Note: jump to the absolute file address"
};

static char help5[] = {
    "SELECT   - Show this help window\n"
    "\n"
    "CROSS    - Set the hex number\n"
    "SQUARE   - Delete the last digit\n"
    "\n"
    "UP/DOWN/LEFT/RIGHT - Move the cursor to the keyboard\n"
    "L2/R2    - Move back/forward from the number window\n"
    "L1/R1    - Decrease/increase the number of bytes to find\n"
    "\n"
    "START    - Find Hex values in file (forward)\n"
    "\n"
    "START (pressing) - Open/close this help window\n"
    "\n"
    "CIRCLE   - Close the window\n"
    "\n"
    "Special Note: Find Hex values in the file. You can use L3/R3 from    "
    "the Hex Editor window to find the previous/next result from the     current file position\n"
};

enum hex_modes
{
    HEX_EDIT_FILE = 0,
    HEX_EDIT_RAM  = 1,
    HEX_EDIT_LV2  = 2,
};

static int mark_flag = 0;
static u64 mark_ini = 0ULL;
static u32 mark_len = 0x0;

static void *copy_mem = NULL;
static u32 copy_len = 0;

static u64 lv1_pos = 0ULL;
static u64 lv2_pos = 0ULL;

void draw_hex_editor()
{
    int n, m;

    int px = 0, py = 0;
    frame++; if(frame > 300) frame = 0;

    tiny3d_Flip();
    ps3pad_read();

    tiny3d_Project2D();
    cls2();
    update_twat(false);

    DrawBox(0, 0, 0, 848, 32, BLUE5);
    DrawBox2(0, 32, 0, 848, 448);

    DrawBox(0, 480, 0, 848, 32, BLUE5);


    SetFontColor(WHITE, 0x0);

    SetCurrentFont(FONT_BUTTON);

    SetFontSize(8, 16);

    py = 40;

    #define START_X 80

    px = START_X;  DrawString(px, py, "      Offset ");
    px = START_X + 16 + 17 *8 + 4;

    for(m = 0; m < 16; m++)
    {
        if(m == 8) px += 8;
        DrawFormatString(px, py,  "%2X", m);
        px += 24;
    }

    py += 24;

    // 0x180
    for(n = 0; n < 24; n++)
    {
       px = START_X;
       u32 color = WHITE;

       SetFontSize(8, 16);
       SetFontColor(color, 0x0);

       // draw hex
       px = DrawFormatString(px, py, " %08X", (u32) (((pos + (u64) (n<<4)))>>32));
       px = DrawFormatString(px, py,  "%08X", (u32)   (pos + (u64) (n<<4))); px+= 16;

       // draw hex
       for(m = 0; m < 16; m++)
       {
           int sel = 0;
           if(m == 8) px += 8;
           px += 8;

           sel = mark_flag == 2 && (pos + (u64) ((n<<4) + m)) >= mark_ini && (pos + (u64) ((n<<4) + m)) < (mark_ini + (u64) mark_len);

           if(temp_buffer[HEX_EDIT + (n<<4) + m] == temp_buffer[HEX_READ + (n<<4) + m])
               color = WHITE;
           else
               color = GREEN;

           // first nibble
           if(((n<<4) + m) >= readed) color = GRAY;

           if((e_x) == (m<<1) && e_y == n)
           {
               if(frame & BLINK)
                   SetFontColor(color, (sel) ? CYAN2 : ((color == GREEN) ? BLUE5 : MAGENTA));
               else
                   SetFontColor(BLACK, color);
           }
           else
               SetFontColor(color, (sel) ? CYAN2 : ((color == GREEN) ? BLUE5 : INVISIBLE));

           px = DrawFormatString(px, py, "%X", (temp_buffer[HEX_EDIT + (n<<4) + m])>>4);

           // second nibble
           if(((n<<4) + m) >= readed) color = GRAY;

           if((e_x) == (m<<1)+1 && e_y == n)
           {
              if(frame & BLINK)
                  SetFontColor(color, (sel) ? CYAN2 : ((color == GREEN) ? BLUE5 : MAGENTA));
              else
                  SetFontColor(BLACK, color);
           }
           else   SetFontColor(color, (sel) ? CYAN2 : ((color == GREEN) ? BLUE5 : INVISIBLE));

           px = DrawFormatString(px, py, "%X", temp_buffer[HEX_EDIT + (n<<4) + m] & 0xF);
       }

       px += 16;

       SetFontColor(WHITE, INVISIBLE);

       // draw chars
       for(m = 0; m < 16; m++)
       {
            u8 ch = temp_buffer[HEX_EDIT + (n<<4) + m];
            int sel = 0;

            sel = mark_flag == 2 && (pos + (u64) ((n<<4) + m)) >= mark_ini && (pos + (u64) ((n<<4) + m)) < (mark_ini + (u64) mark_len);

            if(temp_buffer[HEX_EDIT + (n<<4) + m] == temp_buffer[HEX_READ + (n<<4) + m]) color = WHITE;
            else color = GREEN;

            if(((n<<4) + m) >= readed) color = GRAY;

            if((e_x>>1) == m && e_y == n)
            {
                if(frame & BLINK)
                    SetFontColor(color, (sel) ? CYAN2 : ((color == GREEN) ? BLUE5 : MAGENTA));
                else
                    SetFontColor(BLACK, color);
            }
            else    SetFontColor(color, (sel) ? CYAN2 : ((color == GREEN) ? BLUE5 : INVISIBLE));


            px = DrawFormatString(px, py, "%c", ch == 0 ? '.' : (ch < 32 ? '?' : (char) ch));
       }

       py += 16;
    }

    py += 2;
    SetFontColor(WHITE, INVISIBLE);
    SetFontAutoCenter(1);

    SetFontSize(8, 16);
    DrawFormatString(px, py, "Size: %08X%08X (%1.0f)", (u32) (hex_filesize>>32), (u32) (hex_filesize), (double) (hex_filesize));
    DrawFormatString(px, py + 14, "File at %1.4f%%", 100.0f * (double) (pos + (u64) ((e_y * 0x10) + (e_x>>1))) / (double) hex_filesize);
    SetFontAutoCenter(0);


    set_ttf_window(8, 0, 752, 32, WIN_AUTO_LF);
    display_ttf_string(0, 0, (char *) hex_path, GREEN, 0, 12, 20);
}

void hex_editor(int hex_mode, char *path, s64 size)
{
    int n, m;
    int px, py;

    int help = 0;

    s32 fd = FAILED;
    u64 temp;

    bool read_only = false;

    pos = 0;
    readed = 0;
    e_x = 0, e_y = 0;

    int ret;

    bool modified = false;

    enum function_menu_options
    {
        HEX_EDIT_MODE    = 0,
        HEX_GOTO_ADDRESS = 1,
        HEX_FIND_VALUE   = 2,
    };

    int function_menu = HEX_EDIT_MODE;
    int enable_menu = 0;
    int start_status = 0;

    int auto_up = 0, auto_down = 0;
    int auto_left = 0, auto_right = 0;
    int auto_l2 = 0, auto_r2 = 0;
    int auto_l1 = 0, auto_r1 = 0;
    int auto_l11 = 0, auto_r11 = 0;

    int f_key = 0;
    int f_pos = 0;
    int f_len = 8;
    static u8 find[512];
    int find_len = 4;

    bool is_ntfs = false;

    mark_flag = 0;
    mark_ini = 0ULL;
    mark_len = 0x0;

    memset((char *) find, 0, 512);

    if(hex_mode == HEX_EDIT_FILE)
    {
        is_ntfs = is_ntfs_path(path);

        hex_filesize = size;

        if(hex_filesize == 0ULL) return; // ignore zero files

        if(!is_ntfs)
        {
            ret = sysLv2FsOpen(path, SYS_O_RDWR, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
            if(ret != SUCCESS)
            {
                read_only = true;
                ret = sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0);
            }
        }
        else
        {
            fd = ps3ntfs_open(path, O_RDWR, 0);
            if(fd < 0)
            {
                read_only = true;
                fd = ps3ntfs_open(path, O_RDONLY, 0);
            }
            if(fd < 0) ret = FAILED; else ret = SUCCESS;
        }

        if(ret != SUCCESS) {sprintf(MEM_MESSAGE, "Error: Cannot read %s", path); DrawDialogOK(MEM_MESSAGE); return;}
    }
    else if(hex_mode == HEX_EDIT_RAM)
    {
        fd = FD_LV1; pos = lv1_pos;
        hex_filesize = size_lv1 = 0x10000000ULL;
        begin_lv1 = 0;
    }
    else if(hex_mode == HEX_EDIT_LV2)
    {
        fd = FD_LV2; pos = lv2_pos;
        hex_filesize = size_lv2 = 0x800000ULL;
        begin_lv2 = 0;
    }

read_file:

    memset(MEM_HEX_EDIT, 0, 0x180);

    if(fd == FD_LV1 || fd == FD_LV2)
    {
        ret = 0;
        if(fd == FD_LV1) {if(begin_lv1 > pos) ret =(int) 0x8001001E; if(pos >= size_lv1) temp = 0; else  temp = pos;}
        if(fd == FD_LV2) {if(begin_lv2 > pos) ret =(int) 0x8001001E; if(pos >= size_lv2) temp = 0; else  temp = pos;}
    }
    else if(is_ntfs)
    {
        temp = ps3ntfs_seek64(fd, pos, 0);
        if(temp < 0) ret = FAILED; else ret = SUCCESS;
    }
    else
        ret = sysLv2FsLSeek64(fd, pos, 0, &temp);

    if(ret != SUCCESS || pos != temp)
    {
        if(ret == 0) ret = (int) 0x8001001E;
        sprintf(MEM_MESSAGE, "Lseek Error: 0x%08x\n\n%s", ret, getlv2error(ret));
        DrawDialogOK(MEM_MESSAGE);
        readed = 0;
    }
    else
    {
        readed = hex_filesize - pos;
        if(readed > 0x180ULL) readed = 0x180ULL;
        temp = 0;

        if(fd == FD_LV1)
        {
            ret = read_LV1(pos, MEM_HEX_EDIT, (int) readed);
            temp = readed;
        }
        else if(fd == FD_LV2)
        {
            ret = read_LV2(pos, MEM_HEX_EDIT, (int) readed);
            temp = readed;
        }
        else if(is_ntfs)
        {
            ret = ps3ntfs_read(fd, MEM_HEX_EDIT, readed);
            temp = (u64) ret; if(ret>0) ret = 0;
        }
        else
            ret = sysLv2FsRead(fd, MEM_HEX_EDIT, readed, &temp);

        if(ret < 0 || readed != temp)
        {
            if(ret == 0) ret = (int) 0x8001002B;
            sprintf(MEM_MESSAGE, "Read Error: 0x%08x\n\n%s", ret, getlv2error(ret));
            DrawDialogOK(MEM_MESSAGE);
            readed = temp;
        }
    }

    memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);

    strncpy(hex_path, path, strlen(path));

    while(true)
    {

        draw_hex_editor();

        if(function_menu == HEX_GOTO_ADDRESS || function_menu == HEX_FIND_VALUE)
        {
            px = 64;
            py = 64 + 16;

            SetFontSize(8, 16);
            DrawBox(px, py, 0, 4 * 40 + 4, 4 * 40 + 8 + 32, GRAY);

            SetFontColor(BLACK, INVISIBLE);
            if(function_menu == HEX_GOTO_ADDRESS)
                DrawFormatString(px + 4, py + 4, "Go to Address");
            else
                DrawFormatString(px + 4, py + 4, "Find Hex Values");

            SetFontColor(BLACK, WHITE);

            py += 16;

            DrawFormatString(px + 20, py + 4, "                ");

            for(n = 0; n < f_len; n++)
            {
                if(f_pos == n  && (frame & BLINK))
                    SetFontColor(BLACK, 0x00bfcfff);
                else
                    SetFontColor(BLACK, WHITE);

                if(function_menu == HEX_GOTO_ADDRESS)
                    DrawFormatString(px + 20 + (15 - f_len) * 8 + n * 8, py + 4, "%X", (n & 1) ? (temp_buffer[0x1000 + (n>>1)] & 15) : (temp_buffer[0x1000 + (n>>1)] >> 4));
                else
                    DrawFormatString(px + 20 + n * 8, py + 4, "%X", (n & 1) ? (find[n>>1] & 15) : (find[n>>1] >> 4));
            }

            py += 20;

            SetFontSize(32, 32);

            for(n = 0; n < 4; n++)
            {
                for(m = 0; m < 4; m++)
                {
                    if((f_key & 3) == m && ((f_key>>2) & 3) == n && (frame & BLINK))
                        SetFontColor(BLACK, 0x00bfcfff);
                    else
                        SetFontColor(BLACK, WHITE);

                    DrawFormatString(px + 4 + m * 40, py + 4 + n * 40, "%X", ((n<<2) | m));
                }
            }

            SetFontColor(WHITE, INVISIBLE);
            SetFontSize(8, 16);
        }

        if(enable_menu && function_menu == HEX_EDIT_MODE)
        {
            int py = 0;

            if(new_pad & BUTTON_CROSS_) frame = 300;

            DrawBox((848 - 224)/2, (512 - 248 - 24)/2, 0, 224, 248 - 24, GRAY);
            DrawBox((848 - 216)/2, (512 - 240 - 24)/2, 0, 216, 240 - 24, POPUPMENUCOLOR);
            set_ttf_window((848 - 200)/2, (512 - 240 - 24)/2, 200, 240 - 24, 0);

            display_ttf_string(0, py, "Go to Address",         (enable_menu == 1 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

            display_ttf_string(0, py, "Find Hex",              (enable_menu == 2 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

            display_ttf_string(0, py, "Find String",           (enable_menu == 3 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

            display_ttf_string(0, py, "Find String (no case)", (enable_menu == 4 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

            display_ttf_string(0, py, "Find Hex from Data",    (enable_menu == 5 && (frame & BLINK)) ? INVISIBLE : copy_len ? WHITE: GRAY, 0, 16, 24); py += 24;

            display_ttf_string(0, py, "Mark Begin",            (enable_menu == 6 && (frame & BLINK)) ? INVISIBLE : WHITE, 0, 16, 24); py += 24;

            display_ttf_string(0, py, "Mark End",              (enable_menu == 7 && (frame & BLINK)) ? INVISIBLE : (mark_flag == 1) ? WHITE : GRAY, 0, 16, 24); py += 24;

            display_ttf_string(0, py, "Copy Mark Area",        (enable_menu == 8 && (frame & BLINK)) ? INVISIBLE : (mark_flag == 2) ? WHITE : GRAY, 0, 16, 24); py += 24;

            display_ttf_string(0, py, "Paste Copied Data",     (enable_menu == 9 && (frame & BLINK)) ? INVISIBLE : copy_len ? WHITE: GRAY, 0, 16, 24); py += 24;

        }

        modified = (!read_only && memcmp(MEM_HEX_READ, MEM_HEX_EDIT, 0x180));

        if(help)
        {
            DrawBox((848 - 624)/2, (512 - 424)/2, 0, 624, 424, GRAY);
            DrawBox((848 - 616)/2, (512 - 416)/2, 0, 616, 416, POPUPMENUCOLOR);
            set_ttf_window((848 - 600)/2, (512 - 416)/2, 600, 416, WIN_AUTO_LF);

            if(enable_menu && function_menu == HEX_EDIT_MODE)    display_ttf_string(0, 0, help2, WHITE, 0, 16, 24); else
            if(               function_menu == HEX_GOTO_ADDRESS) display_ttf_string(0, 0, help4, WHITE, 0, 16, 24); else
            if(               function_menu == HEX_FIND_VALUE)   display_ttf_string(0, 0, help5, WHITE, 0, 16, 24); else
                                                                 display_ttf_string(0, 0, help3, WHITE, 0, 16, 24);
        }


        if(help && (new_pad & BUTTON_START))
        {
            help = 0;
            start_status = 0;
        }

        if(start_status <= 180 && (old_pad & BUTTON_SELECT))
        {
            start_status++;
            if(start_status > 180) help = 1;

            if(new_pad & BUTTON_START)
            {
                //cancel without any change
                if(hex_mode == HEX_EDIT_RAM) lv1_pos = pos; else
                if(hex_mode == HEX_EDIT_LV2) lv2_pos = pos;

                break;
            }
        }
        if(enable_menu && (new_pad & BUTTON_SELECT)) help = 1;



        if(help && (new_pad & BUTTON_CIRCLE_))  {help ^= 1; start_status = 0; new_pad ^= BUTTON_CIRCLE_;}
        if(help && (new_pad & BUTTON_TRIANGLE)) {help ^= 1; start_status = 0; new_pad ^= BUTTON_TRIANGLE;}

        if(help) continue;


        if(!enable_menu && (new_pad & BUTTON_CIRCLE_))
        {
            if(old_pad & BUTTON_SELECT)
            {
                //cancel without any change
                if(hex_mode == HEX_EDIT_RAM) lv1_pos = pos; else
                if(hex_mode == HEX_EDIT_LV2) lv2_pos = pos;

                break;
            }
            else
            {
                if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                {
                    save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                    memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                    modified = false;
                }

                if((modified == false) || DrawDialogYesNo("Exit from Hex Editor?") == YES)
                {
                    if(hex_mode == HEX_EDIT_RAM) lv1_pos = pos; else
                    if(hex_mode == HEX_EDIT_LV2) lv2_pos = pos;

                    break;
                }
            }
        }

        if(!enable_menu)
        {
            AUTO_BUTTON_REP2(auto_up, BUTTON_UP)
            AUTO_BUTTON_REP2(auto_down, BUTTON_DOWN)
            AUTO_BUTTON_REP2(auto_left, BUTTON_LEFT)
            AUTO_BUTTON_REP2(auto_right, BUTTON_RIGHT)
            AUTO_BUTTON_REP2(auto_l2, BUTTON_L2)
            AUTO_BUTTON_REP2(auto_r2, BUTTON_R2)
            AUTO_BUTTON_REP2(auto_l1, BUTTON_L1)
            AUTO_BUTTON_REP2(auto_r1, BUTTON_R1)

            if((new_pad & BUTTON_R3) && find_len)
            {
                if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                {
                    save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                    memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                    modified = false;
                }
                if(!modified)
                {
                    u64 found;

                    found = pos + (u64) ((e_y * 0x10) + (e_x>>1) + 1);
                    if(found >= hex_filesize) found = 0ULL;
                    find_in_file(is_ntfs, fd, found, hex_filesize, &found, find, find_len, 1);

                    enable_menu = function_menu = HEX_EDIT_MODE;

                    if(found == TEXTNOTFOUND)
                    {
                        if(find_mode == FIND_HEX_MODE)
                            DrawDialogOKTimer("Hex String not found", 2000.0f);
                        else
                            DrawDialogOKTimer("String not found", 2000.0f);
                    }
                    else
                    {
                        pos = found & ~15ULL;
                        e_y = 0;
                        e_x = (found & 15) << 1;
                        goto read_file;
                    }
                }
            }

            if((new_pad & BUTTON_L3) && find_len)
            {
                if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                {
                    save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                    memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                    modified = false;
                }
                if(!modified)
                {
                    u64 found;

                    found = pos + (u64) ((e_y * 0x10) + (e_x>>1));
                    if(found >= hex_filesize) found = 0ULL;
                    find_in_file(is_ntfs, fd, found, hex_filesize, &found, find, find_len, -1);

                    enable_menu = function_menu = HEX_EDIT_MODE;

                    if(found == TEXTNOTFOUND)
                    {
                        if(find_mode == FIND_HEX_MODE)
                            DrawDialogOKTimer("Hex String not found", 2000.0f);
                        else
                            DrawDialogOKTimer("String not found", 2000.0f);
                    }
                    else
                    {
                        pos = found & ~15ULL;
                        e_y = 0;
                        e_x = (found & 15) << 1;
                        goto read_file;
                    }
                }
            }

            if(new_pad & BUTTON_UP)
            {
                auto_up = 1;
                e_y--;
                if(e_y < 0)
                {
                    e_y = 0;

                    if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                    {
                        save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                        memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                        modified = false;
                    }

                    if(!modified)
                    {
                        if(pos >= 16ULL)  pos -= 16ULL;
                        else
                        {
                            if(hex_filesize>= 16ULL)
                                pos = (hex_filesize - 16ULL) & ~(15ULL);
                            else
                                pos = 0;
                        }
                        goto read_file;
                    }
                }
            }
            else if(new_pad & BUTTON_DOWN)
            {
                auto_down = 1;
                e_y++;
                if(e_y > 23)
                {
                    e_y = 23;

                    if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                    {
                        save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                        memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                        modified = false;
                    }

                    if(!modified)
                    {
                        if(pos + 16ULL < hex_filesize)  pos += 16ULL;
                        else pos = 0ULL;
                        goto read_file;
                    }
                }
            }
            else if(new_pad & BUTTON_LEFT)
            {
                auto_left = 1;
                e_x--;
                if(e_x < 0)
                {
                    e_x = 0x1f;
                    e_y--;
                    if(e_y < 0)
                    {
                        e_y = 0;
                        if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                        {
                            save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                            memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                            modified = false;
                        }

                        if(!modified)
                        {
                            if(pos >= 16ULL)  pos -= 16ULL;
                            else
                            {
                                if(hex_filesize >= 16ULL)
                                    pos = (hex_filesize - 16ULL) & ~(15ULL);
                                else
                                    pos = 0;
                            }
                            goto read_file;
                        }
                    }
                }
            }
            else if(new_pad & BUTTON_RIGHT)
            {
                auto_right = 1;
                e_x++;
                if(e_x > 0x1f)
                {
                    e_x = 0;
                    e_y++;
                    if(e_y > 23)
                    {
                        e_y = 23;
                        if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                        {
                            save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                            memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                            modified = false;
                        }
                        if(!modified)
                        {
                            if(pos + 16ULL < hex_filesize)  pos += 16ULL;
                            else pos = 0ULL;
                            goto read_file;
                        }
                    }
                }
            }

            if((new_pad & BUTTON_L1)) // scroll up
            {
                u64 incre = 0x80ULL;

                if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                {
                    save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                    memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                    modified = false;
                }

                auto_l11++;
                start_status = 0;
                if((old_pad & BUTTON_SELECT))
                    incre = 0x1000ULL;
                else
                    incre <<= 4 * (auto_l11 / 10);

                if(incre > 0x6400000ULL) incre = 0x6400000ULL;

                auto_l1 = 1;
                if(pos >= incre)  pos -= incre;
                else
                {
                    if(hex_filesize >= 16ULL)
                        pos = (hex_filesize - 16ULL) & ~(15ULL);
                    else pos = 0;
                }
                goto read_file;

            }
            else if(!(old_pad & BUTTON_L1)) auto_l11 = 0;

            if((new_pad & BUTTON_R1)) // scroll down
            {
                u64 incre = 0x80ULL;

                if(modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
                {
                    save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                    memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                    modified = false;
                }

                auto_r11++;
                start_status = 0;
                if((old_pad & BUTTON_SELECT))
                    incre = 0x1000ULL;
                else
                    incre <<= 4 * (auto_r11 / 10);

                if(incre > 0x6400000ULL) incre = 0x6400000ULL;

                auto_r1 = 1;
                if(pos + incre < hex_filesize)  pos += incre;
                else pos = 0ULL;

                goto read_file;
            } else if(!(old_pad & BUTTON_R1)) auto_r11 = 0;

        /**********************************************************************************************************/
        /* MODIFICATION AREA                                                                                      */
        /**********************************************************************************************************/

             // byte ++
            if((new_pad & BUTTON_R2) && !read_only)
            {
               u8 * p = (u8 *) &temp_buffer[HEX_EDIT + (e_y * 0x10) + (e_x>>1)];
                if(((e_y * 0x10) + (e_x>>1)) < readed)
                {
                    p[0] = (p[0] + 1);
                }
                auto_r2 = 1;
            }

            // byte --
            if((new_pad & BUTTON_L2) && !read_only)
            {
                u8 * p = (u8 *) &temp_buffer[HEX_EDIT + (e_y * 0x10) + (e_x>>1)];
                if(((e_y * 0x10) + (e_x>>1)) < readed)
                {
                    p[0] = (p[0] - 1);
                }

                auto_l2 = 1;
            }

            // nibble ++
            if((new_pad & BUTTON_CROSS_) && !read_only)
            {
               u8 * p = (u8 *) &temp_buffer[HEX_EDIT + (e_y * 0x10) + (e_x>>1)];
                if(((e_y * 0x10) + (e_x>>1)) < readed)
                {
                    if(e_x & 1)
                        p[0] = ((p[0] + 1) & 0xf) | (p[0] & 0xf0);
                    else
                        p[0] = ((p[0] + 0x10) & 0xf0) | (p[0] & 0xf);
                }
            }

            // nibble --
            if((new_pad & BUTTON_TRIANGLE) && !read_only)
            {
                u8 * p = (u8 *) &temp_buffer[HEX_EDIT + (e_y * 0x10) + (e_x>>1)];
                if(((e_y * 0x10) + (e_x>>1)) < readed)
                {
                    if(e_x & 1)
                        p[0] = ((p[0] - 1) & 0xf) | (p[0] & 0xf0);
                    else
                        p[0] = ((p[0] - 0x10) & 0xf0) | (p[0] & 0xf);
                }
            }

            // undo all
            if(((old_pad & BUTTON_SELECT) && (new_pad & BUTTON_SQUARE)) && !read_only)
            {
                memcpy(MEM_HEX_EDIT, MEM_HEX_READ, 0x180);
            }
            // undo one
            else if((new_pad & BUTTON_SQUARE) && !read_only)
            {
                if(((e_y * 0x10) + (e_x>>1)) < readed)
                {
                    if(temp_buffer[HEX_EDIT + (e_y * 0x10) + (e_x>>1)] == temp_buffer[HEX_READ + (e_y * 0x10) + (e_x>>1)])
                        {function_menu = (mark_flag == 1 && (pos + (u64) ((e_y * 0x10) + (e_x>>1))) >= mark_ini) ? 7 : 6; enable_menu = help = start_status = 0; goto edit_options;}
                    else
                        temp_buffer[HEX_EDIT + (e_y * 0x10) + (e_x>>1)] = temp_buffer[HEX_READ + (e_y * 0x10) + (e_x>>1)];
                }
            }


        } // enable menu off
        else
        {// enable menu on
            if(function_menu == HEX_FIND_VALUE)
            {
                if(new_pad & BUTTON_R1)
                {
                    //auto_r1 = 1;
                    memset((char *) &find[f_len>>1], 0, 8);
                    f_len+=2;
                    if(f_len > 16) f_len = 16;
                    if(f_pos >= f_len) f_pos = (f_len) - 1;
                }
                else if(new_pad & BUTTON_L1)
                {
                    //auto_l1 = 1;
                    memset((char *) &find[f_len>>1], 0, 8);
                    f_len-=2;
                    if(f_len < 2) f_len = 2;
                    if(f_pos >= f_len) f_pos = f_len - 1;
                }
            }

            if(function_menu == HEX_GOTO_ADDRESS || function_menu == HEX_FIND_VALUE)
            {
                if(new_pad & BUTTON_R2)
                {
                    //auto_r2 = 1;
                    f_pos++;
                    if(f_pos >= f_len) f_pos = (f_len) - 1;
                }
                else if(new_pad & BUTTON_L2)
                {
                    //auto_l2 = 1;
                    f_pos--;
                    if(f_pos < 0) f_pos = 0;
                }
                else if(new_pad & BUTTON_UP)
                {
                    //auto_up = 1;
                    if((f_key & 12) == 0) f_key|=12; else f_key-=4;
                }
                else if(new_pad & BUTTON_DOWN)
                {
                    //auto_down = 1;
                    if((f_key & 12) == 12) f_key=12; else f_key+=4;
                }
                else if(new_pad & BUTTON_LEFT)
                {
                    //auto_left = 1;
                    if((f_key & 3) == 0) f_key|=3; else f_key--;
                }
                else if(new_pad & BUTTON_RIGHT)
                {
                    //auto_right = 1;
                    if((f_key & 3) == 3) f_key^=3; else f_key++;
                }
                else if(new_pad & BUTTON_CROSS_)
                {
                    u8 * p = (function_menu == HEX_GOTO_ADDRESS) ? (u8 *) &temp_buffer[0x1000 + (f_pos >> 1)] : &find[f_pos >> 1];
                    if(f_pos & 1)
                        p[0] = (f_key) | (p[0] & 0xf0);
                    else
                        p[0] = ((f_key<<4) & 0xf0) | (p[0] & 0xf);

                    if(function_menu == HEX_GOTO_ADDRESS && f_len < 16 && f_pos == (f_len - 1)) f_len++;
                    f_pos++;
                    if(f_pos >= f_len) f_pos = (f_len) - 1;
                }
            }

            if(function_menu == HEX_GOTO_ADDRESS && (new_pad & BUTTON_SQUARE))
            {
                f_len--;
                if(f_len < 1) f_len = 1;
                else {
                    u8 * p = (u8 *) &temp_buffer[0x1000 + (f_len >> 1)];
                    // clear the nibble
                    if(f_len & 1)
                        p[0] = (p[0] & 0xf0);
                    else
                        p[0] = (p[0] & 0xf);
                }
                if(f_pos >= f_len) f_pos = (f_len) - 1;
            }
            else if((function_menu == HEX_GOTO_ADDRESS || function_menu == HEX_FIND_VALUE) && (new_pad & BUTTON_START))
            {
                switch(function_menu)
                {
                    case 1:
                    {
                        pos = 0ULL;
                        memcpy(((char *) &pos), &temp_buffer[0x1000], (f_len+1)/2);

                        //if(!(f_len & 1))
                        pos>>=(u64) ((16 - f_len) <<2);

                        if(pos >= hex_filesize) pos = 0ULL;

                        e_y = 0; e_x = 0;
                        pos &= ~15ULL;
                        enable_menu = function_menu = HEX_EDIT_MODE;
                        goto read_file;
                        break;
                    }
                    case 2:
                    {
                        u64 found;

                        find_len = f_len / 2;

                        found = pos + (u64) ((e_y * 0x10) + (e_x>>1) + 1);
                        if(found >= hex_filesize) found = 0ULL;
                        find_mode = FIND_HEX_MODE;
                        find_in_file(is_ntfs, fd, found, hex_filesize, &found, find, find_len, 1);

                        enable_menu = function_menu = HEX_EDIT_MODE;

                        if(found == TEXTNOTFOUND)
                        {
                            DrawDialogOK("Hex string not found");
                        }
                        else
                        {
                            pos = found & ~15ULL;
                            e_y = 0;
                            e_x = (found & 15) << 1;
                            goto read_file;
                        }
                        break;
                    }
                }
            }

            if(function_menu == HEX_EDIT_MODE)
            {
                if(new_pad & BUTTON_UP)
                {
                    if(enable_menu > 1) enable_menu--;  else {enable_menu = 9;}
                }
                else if(new_pad & BUTTON_DOWN)
                {
                    if(enable_menu < 9) enable_menu++;  else {enable_menu = 1;}
                }
                else if(new_pad & BUTTON_CROSS_)
                {
                    function_menu = enable_menu;
edit_options:
                    switch(function_menu)
                    {
                        case 1:
                            memset(&temp_buffer[0x1000], 0, 8);
                            f_len = 1;
                            break;

                        case 2:
                            f_len = 8;
                            if(find_len > 8) find_len = 8;
                            break;
                        case 3:
                            memset(find, 0, 512); find_len = 0;
                            if(Get_OSK_String("Find String", (char *) find, 250) == SUCCESS)
                            {
                                u64 found;

                                find_len = strlen((char *) find);

                                if(find_len != 0)
                                {
                                    found = pos + (u64) ((e_y * 0x10) + (e_x>>1) + 1);
                                    if(found >= hex_filesize) found = 0ULL;
                                    find_mode = FIND_TEXT_MODE;
                                    find_in_file(is_ntfs, fd, found, hex_filesize, &found, find, find_len, 1);

                                    enable_menu = function_menu = HEX_EDIT_MODE;

                                    if(found == TEXTNOTFOUND)
                                    {
                                        DrawDialogOK("String not found");
                                    }
                                    else
                                    {
                                        pos = found & ~15ULL;
                                        e_y = 0;
                                        e_x = (found & 15) << 1;
                                        goto read_file;
                                    }
                                }
                            }
                            enable_menu = function_menu = HEX_EDIT_MODE;

                            break;
                        case 4:
                            memset(find, 0, 512); find_len = 0;
                            if(Get_OSK_String("Find String (no case sensitive)", (char *) find, 250) == SUCCESS)
                            {
                                u64 found;

                                find_len = strlen((char *) find);

                                if(find_len != 0)
                                {
                                    found = pos + (u64) ((e_y * 0x10) + (e_x>>1) + 1);
                                    if(found >= hex_filesize) found = 0ULL;
                                    find_mode = FIND_CASE_INSENSITIVE_MODE;
                                    find_in_file(is_ntfs, fd, found, hex_filesize, &found, find, find_len, 1);

                                    enable_menu = function_menu = HEX_EDIT_MODE;

                                    if(found == TEXTNOTFOUND)
                                    {
                                        DrawDialogOK("String (no case sensitive) not found");
                                    }
                                    else
                                    {
                                        pos = found & ~15ULL;
                                        e_y = 0;
                                        e_x = (found & 15) << 1;
                                        goto read_file;
                                    }
                                }
                            }
                            enable_menu = function_menu = HEX_EDIT_MODE;

                            break;
                        case 5:
                            if(copy_mem && copy_len)
                            {
                                find_len = (copy_len > 512) ? 512 : copy_len;
                                memcpy(find, copy_mem, find_len);

                                u64 found;

                                if(find_len != 0)
                                {
                                    found = pos + (u64) ((e_y * 0x10) + (e_x>>1) + 1);
                                    if(found >= hex_filesize) found = 0ULL;
                                    find_mode = FIND_HEX_MODE;
                                    find_in_file(is_ntfs, fd, found, hex_filesize, &found, find, find_len, 1);

                                    enable_menu = function_menu = HEX_EDIT_MODE;

                                    if(found == TEXTNOTFOUND)
                                    {
                                        DrawDialogOK("Hex string not found");
                                    }
                                    else
                                    {
                                        pos = found & ~15ULL;
                                        e_y = 0;
                                        e_x = (found & 15) << 1;
                                        goto read_file;
                                    }
                                }
                            }
                            enable_menu = function_menu = HEX_EDIT_MODE;

                            break;

                        case 6:
                            enable_menu = function_menu = HEX_EDIT_MODE;

                            if(pos + (u64) ((e_y * 0x10) + (e_x>>1)) >= hex_filesize)
                            {
                                DrawDialogOKTimer("Mark is out of filesize / memory", 2000.0f);
                            }
                            else
                            {
                                mark_ini = pos + (u64) ((e_y * 0x10) + (e_x>>1));
                                mark_flag = 1;
                            }
                            break;

                        case 7:
                            enable_menu = function_menu = HEX_EDIT_MODE;

                            if(!mark_flag) mark_ini = 0;

                            if((pos + (u64) ((e_y * 0x10) + (e_x>>1))) >= mark_ini &&
                               (pos + (u64) ((e_y * 0x10) + (e_x>>1))) < (mark_ini + 0x100000ULL))
                            {
                                if(pos + (u64) ((e_y * 0x10) + (e_x>>1)) >= hex_filesize)
                                {
                                    DrawDialogOKTimer("Mark is out of file size / memory: truncating to the end position", 2000.0f);
                                    mark_len = hex_filesize - mark_ini + 1ULL;
                                }
                                else
                                    mark_len = (pos + (u64) ((e_y * 0x10) + (e_x>>1))) - mark_ini + 1ULL;

                                mark_flag = 2;
                            }
                            else DrawDialogOKTimer("Mark position out of the range\nyou can select a block of 1 MB max from Mark Begin", 2000.0f);
                            break;

                        case 8:
                            enable_menu = function_menu = HEX_EDIT_MODE;

                            if(mark_flag == 2) {
                                if(copy_mem) free(copy_mem);
                                copy_mem = malloc(mark_len);
                                copy_len = 0;
                                if(!copy_mem) DrawDialogOKTimer("Out of memory from copy function", 2000.0f);
                                else {copy_len = mark_len;

                                    if(load_hex(is_ntfs, fd, mark_ini, copy_mem, mark_len) == 0) {
                                        sprintf(TEMP_PATH2, "Copied %d bytes", copy_len);
                                        DrawDialogOKTimer(TEMP_PATH2, 2000.0f);
                                    }
                                }
                            } else DrawDialogOKTimer("Nothing to Copy", 2000.0f);


                            break;

                        case 9:
                            enable_menu = function_menu = HEX_EDIT_MODE;

                            if(copy_len == 0 || !copy_mem) {DrawDialogOKTimer("Paste buffer is empty", 2000.0f);}
                            //else if(fd != FD_LV1 && fd != FD_LV2) DrawDialogOKTimer("Paste is Only supported to memory for now", 2000.0f);
                            else {
                                u64 my_pos = (pos + (u64) ((e_y * 0x10) + (e_x>>1)));
                                int my_len = copy_len;

                                if((my_pos + (u64) my_len) > hex_filesize)
                                {
                                     my_len = (u32) (hex_filesize - my_pos);
                                     sprintf(MEM_MESSAGE, "Paste buffer exceeds %d bytes the file / memory\n\nWant you write %d bytes from 0x%08X%08X ?", copy_len - my_len, my_len, (u32) (my_pos>>32), (u32) my_pos);
                                }
                                else
                                {
                                    sprintf(MEM_MESSAGE, "Do you want to write %d bytes from 0x%08X%08X ?", my_len, (u32) (my_pos>>32), (u32) my_pos);
                                }

                                if(DrawDialogYesNo(MEM_MESSAGE) == YES)
                                {
                                    int ret = 0;

                                    ret = save_hex(is_ntfs, fd, my_pos, copy_mem, my_len);

                                    if(ret == 0)
                                    {
                                        sprintf(MEM_MESSAGE, "%d bytes written", my_len);
                                        DrawDialogOKTimer(MEM_MESSAGE, 2000.0f);
                                    }

                                    goto read_file;
                                }
                            }

                            break;

                        case 10:
                            enable_menu = function_menu = HEX_EDIT_MODE;

                            f_key = 0;
                            f_pos = 0;
                            f_len = (find_len > 8) ? 16 : find_len * 2;

                            break;
                        default:
                            enable_menu = function_menu = HEX_EDIT_MODE;
                            break;
                    }
                }
            } // func menu 0

        } // enable menu on

        if((enable_menu == 0 && (new_pad & BUTTON_START)) ||
           (enable_menu != 0 && (new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))))
        {
            enable_menu = !enable_menu;
            function_menu = HEX_EDIT_MODE;

            f_key = 0;
            f_pos = 0;
            f_len = (find_len > 8) ? 16 : find_len * 2;

            if(enable_menu && modified && DrawDialogYesNo("Do you want to save the changes?") == YES)
            {
                save_hex(is_ntfs, fd, pos, MEM_HEX_EDIT, readed);
                memcpy(MEM_HEX_READ, MEM_HEX_EDIT, 0x180);
                modified = false;
            }

            // if modified undo all changes
            if(modified) {memcpy(MEM_HEX_EDIT, MEM_HEX_READ, 0x180); modified = false;}
        }

    }

    if(fd >= SUCCESS)
    {
        if(is_ntfs) ps3ntfs_close(fd); else  sysLv2FsClose(fd);
    }

    fd = FAILED;
    frame = 0;
}
