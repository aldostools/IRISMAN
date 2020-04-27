/*******************************************************************************************************************************************************/
/* CONSOLE DEBUG                                                                                                                                       */
/*******************************************************************************************************************************************************/

#define CONSOLE_WIDTH       (100)
#define CONSOLE_HEIGHT      (27)

static char dbg_str1[128];
static char dbg_str2[128];

static u32 dbg_data[128 * (CONSOLE_HEIGHT + 1)];
static char dbg_string[1024];

int con_x = 0, con_y =0;

void DCls()
{
    con_x = 0; con_y =0;
    dbg_str1[0] = dbg_str2[0] = 0;
    memset((void *) dbg_data, 0, 128 * CONSOLE_HEIGHT * 4);
}

static char buff[4096];

void DbgHeader(char *str)
{
    strncpy(dbg_str1, str, 128);
    dbg_str1[127] = 0;
}

void DbgMess(char *str)
{
    strncpy(dbg_str2, str, 128);
    dbg_str2[127] = 0;
}


void DbgDraw()
{
    int n;

    cls2();


    SetFontColor(0x0fcf2fff, 0x00000000);
    SetFontAutoCenter(0);
    SetCurrentFont(FONT_TTF/*BUTTON*/);
    SetFontSize(14, 16);


    for(n = 0; n < CONSOLE_HEIGHT; n++)
    {
       UTF32_to_UTF8(&dbg_data[128 * n], (void *) dbg_string);
       DrawString(4, 56 + n * 16, dbg_string);
    }

    SetFontColor(0xffffffff, 0x00000000);
    SetCurrentFont(FONT_TTF);

    SetFontSize(14, 32);

    SetFontAutoCenter(1);
    DrawString(0, 16, dbg_str1);


    DrawString(0, 480, dbg_str2);

    SetFontAutoCenter(0);

    SetCurrentFont(FONT_BUTTON);
}

void DPrintf(char *format, ...)
{
    char *str = (char *) buff;
    va_list opt;

    va_start(opt, format);
    vsprintf( (void *) buff, format, opt);
    va_end(opt);

    while(*str)
    {
        u32 ttf_char;
        int n, m;

        if(*str & 128)
        {
            m = 1;

            if((*str & 0xf8) == 0xf0)
            {
                // 4 bytes
                ttf_char = (u32) (*(str++) & 3);
                m = 3;
            }
            else if((*str & 0xE0) == 0xE0)
            {
                // 3 bytes
                ttf_char = (u32) (*(str++) & 0xf);
                m = 2;
            }
            else if((*str & 0xE0) == 0xC0)
            {
                // 2 bytes
                ttf_char = (u32) (*(str++) & 0x1f);
                m = 1;
            }
            else {str++; continue;} // error!

             for(n = 0; n < m; n++)
             {
                if(!*str) break; // error!

                if((*str & 0xc0) != 0x80) break; // error!
                ttf_char = (ttf_char <<6) |((u32) (*(str++) & 63));
             }

            if((n != m) && !*str) break;

        } else ttf_char = (u32) *(str++);

        if(ttf_char == '\n')
        {
            con_y++;
            con_x = 0;

            if(con_y >= CONSOLE_HEIGHT)
            {
                con_y = CONSOLE_HEIGHT - 1;
                memcpy((void *) dbg_data, (void *) (dbg_data + 128), 128 * (CONSOLE_HEIGHT -1) * 4);
                dbg_data[128 * (CONSOLE_HEIGHT -1)] = 0;
            }
            else dbg_data[128 * con_y + con_x] = 0;
        }
        else
        {
            if(con_x < CONSOLE_WIDTH)
            {
                dbg_data[128 * con_y + con_x] = ttf_char;
                dbg_data[128 * con_y + con_x + 1] = 0;
                con_x++;
            }
            else
            {
                con_y++;
                con_x = 0;
                if(con_y >= CONSOLE_HEIGHT)
                {
                    con_y = CONSOLE_HEIGHT - 1;
                    memcpy((void *) dbg_data, (void *) (dbg_data + 128), 128 * (CONSOLE_HEIGHT -1) * 4);
                    dbg_data[128 * (CONSOLE_HEIGHT -1)] = 0;
                }

                dbg_data[128 * con_y + con_x] = ttf_char;
                dbg_data[128 * con_y + con_x + 1] = 0;
                con_x++;
            }
        }
    }

    DbgDraw();
    tiny3d_Flip();
}

char *getfilename_part(char *path)
{
    int len = strlen(path) - 1;

    while(len > 0) {if(path[len] == '/') {len++; break;}; len--;}

    return &path[len];
}
