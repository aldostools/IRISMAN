/*
   TINY3D - font library / (c) 2010 Hermes  <www.elotrolado.net>

*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "libfont2.h"
#include "ttf_render.h"

struct t_font_description
{
    int w, h, bh;

    u8 first_char;
    u8 last_char;

    u32 rsx_text_offset;
    u32 rsx_bytes_per_char;
    u32 color_format;

    short fw[256]; // chr width
    short fy[256]; // chr y correction
};

static struct t_font_datas
{

    int number_of_fonts;

    int current_font;

    struct t_font_description fonts[9];

    int sx, sy;

    u32 color, bkcolor;

    int autocenter;
    int autonewline;

    u32 rsx_text_bk_offset;
    int enable_doubletextures;

    float screen_w, screen_h;

    int mod_x, mod_y;

    float X,Y,Z;

} font_datas;


void ResetFont()
{
    font_datas.current_font = font_datas.number_of_fonts =0;

    font_datas.color = 0xffffffff;
    font_datas.bkcolor = 0;
    font_datas.autocenter = 0;
    font_datas.X = font_datas.Y = font_datas.Z = 0.0f;
    font_datas.autonewline = 0;

    font_datas.sx = font_datas.sy = 8;

    font_datas.rsx_text_bk_offset = 0;

    font_datas.enable_doubletextures = 0;

    font_datas.screen_w = 848.0f;
    font_datas.screen_h = 512.0f;

    font_datas.mod_x = font_datas.mod_y = 32;

}

u8 * AddFontFromBitmapArray(u8 *font, u8 *texture, u8 first_char, u8 last_char, int w, int h, int bits_per_pixel, int byte_order)
{
    int n, a, b;
    u8 i;

    if(font_datas.number_of_fonts >= 8) return texture;

    font_datas.fonts[font_datas.number_of_fonts].w = w;
    font_datas.fonts[font_datas.number_of_fonts].h = h;
    font_datas.fonts[font_datas.number_of_fonts].bh = h;
    font_datas.fonts[font_datas.number_of_fonts].color_format = TINY3D_TEX_FORMAT_A4R4G4B4; //TINY3D_TEX_FORMAT_A8R8G8B8;
    font_datas.fonts[font_datas.number_of_fonts].first_char = first_char;
    font_datas.fonts[font_datas.number_of_fonts].last_char  = last_char;
    font_datas.autocenter =0;

    font_datas.color = 0xffffffff;
    font_datas.bkcolor = 0x0;

    font_datas.sx = w;
    font_datas.sy = h;

    font_datas.Z = 0.0f;

    for(n = 0; n < 256; n++)
    {
        font_datas.fonts[font_datas.number_of_fonts].fw[n] = 0;
        font_datas.fonts[font_datas.number_of_fonts].fy[n] = 0;
    }

    if(!font_datas.rsx_text_bk_offset)
    {
        texture = (u8 *) ((((long) texture) + 15) & ~15);
        font_datas.rsx_text_bk_offset = tiny3d_TextureOffset(texture);
        memset(texture, 255, 8 * 8 * 2);
        texture += 8 * 8 * 2;
        texture = (u8 *) ((((long) texture) + 15) & ~15);
    }

    for(n = first_char; n <= last_char; n++)
    {
        font_datas.fonts[font_datas.number_of_fonts].fw[n] = w;

        texture = (u8 *) ((((long) texture) + 15) & ~15);

        if(n == first_char) font_datas.fonts[font_datas.number_of_fonts].rsx_text_offset = tiny3d_TextureOffset(texture);

        if(n == first_char+1) font_datas.fonts[font_datas.number_of_fonts].rsx_bytes_per_char = tiny3d_TextureOffset(texture)
            - font_datas.fonts[font_datas.number_of_fonts].rsx_text_offset;

        for(a = 0; a < h; a++)
        {
            for(b = 0; b < w; b++)
            {
                i = font[(b * bits_per_pixel)/8];

                if(byte_order)
                    i = (i << ((b & (7/bits_per_pixel)) * bits_per_pixel))>> (8-bits_per_pixel);
                else
                    i >>= (b & (7/bits_per_pixel)) * bits_per_pixel;

                i = (i & ((1 << bits_per_pixel)-1)) * 255 / ((1 << bits_per_pixel)-1);

                if(i)
                {
                    //TINY3D_TEX_FORMAT_A1R5G5B5
                    //i>>=3;
                    //*((u16 *) texture) = (1<<15) | (i<<10) | (i<<5) | (i);
                    //TINY3D_TEX_FORMAT_A4R4G4B4
                    i>>=4;
                    *((u16 *) texture) = (i<<12) | 0xfff;
                }
                else
                {
                    texture[0] = texture[1] = 0x0; //texture[2] = 0x0;
                    //texture[3] = 0x0; // alpha
                }
                texture += 2;
            }

            font += (w * bits_per_pixel) / 8;
        }
    }

    texture = (u8 *) ((((long) texture) + 15) & ~15);

    font_datas.number_of_fonts++;

    return texture;
}

u8 * AddFontFromTTF(u8 *texture, u8 first_char, u8 last_char, int w, int h,
    void (* ttf_callback) (u8 chr, u8 * bitmap, short *w, short *h, short *y_correction))
{
    int n, a, b;
    u8 i;
    u8 *font;
    static u8 letter_bitmap[257 * 256];

    int bits_per_pixel = 8;

    if(font_datas.number_of_fonts >= 8) return texture;

    if(h < 8) h = 8;
    if(w < 8) w = 8;
    if(h > 256) h = 256;
    if(w > 256) w = 256;

    font_datas.fonts[font_datas.number_of_fonts].w = w;
    font_datas.fonts[font_datas.number_of_fonts].h = h;
    font_datas.fonts[font_datas.number_of_fonts].bh = h+4;
    font_datas.fonts[font_datas.number_of_fonts].color_format = TINY3D_TEX_FORMAT_A4R4G4B4;
    font_datas.fonts[font_datas.number_of_fonts].first_char = first_char;
    font_datas.fonts[font_datas.number_of_fonts].last_char  = last_char;
    font_datas.autocenter =0;

    font_datas.color = 0xffffffff;
    font_datas.bkcolor = 0x0;

    font_datas.sx = w;
    font_datas.sy = h;

    font_datas.Z = 0.0f;

    for(n = 0; n < 256; n++)
    {
        font_datas.fonts[font_datas.number_of_fonts].fw[n] = 0;
        font_datas.fonts[font_datas.number_of_fonts].fy[n] = 0;
    }

    if(!font_datas.rsx_text_bk_offset)
    {
        texture = (u8 *) ((((long) texture) + 15) & ~15);
        font_datas.rsx_text_bk_offset = tiny3d_TextureOffset(texture);
        memset(texture, 255, 8 * 8 * 2);
        texture += 8 * 8 * 2;
        texture = (u8 *) ((((long) texture) + 15) & ~15);
    }

    for(n = first_char; n <= last_char; n++)
    {
        short hh = h;

        font = letter_bitmap;

        font_datas.fonts[font_datas.number_of_fonts].fw[n] = (short) w;

        ttf_callback((u8) (n & 255), letter_bitmap, &font_datas.fonts[font_datas.number_of_fonts].fw[n], &hh,  &font_datas.fonts[font_datas.number_of_fonts].fy[n]);

        // letter background correction
        if((hh + font_datas.fonts[font_datas.number_of_fonts].fy[n]) > font_datas.fonts[font_datas.number_of_fonts].bh)
            font_datas.fonts[font_datas.number_of_fonts].bh = hh + font_datas.fonts[font_datas.number_of_fonts].fy[n];

        texture = (u8 *) ((((long) texture) + 15) & ~15);

        if(n == first_char) font_datas.fonts[font_datas.number_of_fonts].rsx_text_offset = tiny3d_TextureOffset(texture);

        if(n == first_char+1) font_datas.fonts[font_datas.number_of_fonts].rsx_bytes_per_char = tiny3d_TextureOffset(texture)
            - font_datas.fonts[font_datas.number_of_fonts].rsx_text_offset;

        for(a = 0; a < h; a++)
        {
            for(b = 0; b < w; b++)
            {
                i = font[(b * bits_per_pixel)/8];

                i >>= (b & (7/bits_per_pixel)) * bits_per_pixel;

                i = (i & ((1 << bits_per_pixel)-1)) * 255 / ((1 << bits_per_pixel)-1);

                if(i)
                {
                    //TINY3D_TEX_FORMAT_A4R4G4B4
                    i>>=4;
                    *((u16 *) texture) = (i<<12) | 0xfff;
                }
                else
                {
                    texture[0] = texture[1] = 0x0; //texture[2] = 0x0;
                    //texture[3] = 0x0; // alpha
                }

                texture += 2;
            }

            font += (w * bits_per_pixel) / 8;
        }
    }

    texture = (u8 *) ((((long) texture) + 15) & ~15);

    font_datas.number_of_fonts++;

    return texture;
}

void SetCurrentFont(int nfont)
{
    if(nfont < -1 || nfont >= font_datas.number_of_fonts) nfont = 0;
    if(nfont == -1) nfont = 8;

    font_datas.current_font = nfont;
}

void SetFontSize(int sx, int sy)
{
    if(sx < 8) sx = 8;
    if(sy < 8) sy = 8;

    font_datas.sx = sx;
    font_datas.sy = sy;
}

void SetFontColor(u32 color, u32 bkcolor)
{
    font_datas.color   = color;
    font_datas.bkcolor = bkcolor;
}

void SetFontTextureMethod(int method)
{
    font_datas.enable_doubletextures = method;
}

void SetDoubleTextureModule(int module_x, int module_y)
{
    font_datas.mod_x = module_x;
    font_datas.mod_y = module_y;
}

void SetFontAutoCenter(int on_off)
{
    font_datas.autocenter  = on_off;
    font_datas.autonewline = 0;
}

void SetFontAutoNewLine(int width)
{
    font_datas.autonewline = width;
    font_datas.autocenter  = 0;
}

void SetFontZ(float z)
{
    font_datas.Z  = z;
}

float GetFontX()
{
    return font_datas.X;
}

float GetFontY()
{
    return font_datas.Y;
}

void SetFontScreenLimits(float width, float height)
{
    font_datas.screen_w = width;
    font_datas.screen_h = height;
}

static int WidthFromStr(u8 * str)
{
    int w = 0;

    while(*str)
    {
        w += font_datas.sx * font_datas.fonts[font_datas.current_font].fw[*str++] / font_datas.fonts[font_datas.current_font].w;
    }

    return w;
}

#define MOD_FLOAT(x,y) (((float)(((u32) (x)) % y)) / (float) y)

void DrawChar(float x, float y, float z, u8 chr)
{
    float dx  = font_datas.sx, dy = font_datas.sy;
    float dx2 = (font_datas.sx * font_datas.fonts[font_datas.current_font].fw[chr]) / font_datas.fonts[font_datas.current_font].w;
    float dy2 = (float) (font_datas.sy * font_datas.fonts[font_datas.current_font].bh) / (float) font_datas.fonts[font_datas.current_font].h;

    if(font_datas.number_of_fonts <= 0) return;

    if(chr < font_datas.fonts[font_datas.current_font].first_char) return;

    if(font_datas.bkcolor)
    {
        tiny3d_SetTextureWrap(0, font_datas.rsx_text_bk_offset, 8,
        8, 8 * 2,
        TINY3D_TEX_FORMAT_A4R4G4B4, TEXTWRAP_CLAMP, TEXTWRAP_CLAMP, TEXTURE_LINEAR);


        tiny3d_SetPolygon(TINY3D_QUADS);

        tiny3d_VertexPos(x     , y     , z);
        tiny3d_VertexColor(font_datas.bkcolor);
        tiny3d_VertexTexture(0.0f, 0.0f);

        tiny3d_VertexPos(x + dx2, y     , z);

        tiny3d_VertexPos(x + dx2, y + dy2, z);

        tiny3d_VertexPos(x     , y + dy2, z);

        tiny3d_End();
    }

    y += (float) (font_datas.fonts[font_datas.current_font].fy[chr] * font_datas.sy) / (float) (font_datas.fonts[font_datas.current_font].h);

    if(chr > font_datas.fonts[font_datas.current_font].last_char) return;

    // Load sprite texture
    tiny3d_SetTextureWrap(0, font_datas.fonts[font_datas.current_font].rsx_text_offset + font_datas.fonts[font_datas.current_font].rsx_bytes_per_char
        * (chr - font_datas.fonts[font_datas.current_font].first_char), font_datas.fonts[font_datas.current_font].w,
        font_datas.fonts[font_datas.current_font].h, font_datas.fonts[font_datas.current_font].w *
        ((font_datas.fonts[font_datas.current_font].color_format == TINY3D_TEX_FORMAT_A8R8G8B8) ? 4 : 2),
        font_datas.fonts[font_datas.current_font].color_format, TEXTWRAP_CLAMP, TEXTWRAP_CLAMP, TEXTURE_LINEAR);

    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x     , y     , z);
    tiny3d_VertexColor(font_datas.color);
    tiny3d_VertexTexture(0.0f, 0.0f);

    switch(font_datas.enable_doubletextures)
    {
        case 1:
            tiny3d_VertexTexture2(0.0f, 0.0f);
            break;
        case 2:
            tiny3d_VertexTexture2((x)/848.0f, (y)/512.0f);
            break;
        case 3:
            tiny3d_VertexTexture2(MOD_FLOAT(x, font_datas.mod_x), MOD_FLOAT(y, font_datas.mod_y));
            break;
        default:
            break;
    }

    tiny3d_VertexPos(x + dx, y     , z);
    tiny3d_VertexTexture(0.95f, 0.0f);

    switch(font_datas.enable_doubletextures) {
        case 1:
            tiny3d_VertexTexture2(0.95f, 0.0f);
            break;
        case 2:
            tiny3d_VertexTexture2((x+dx)/848.0f, y/512.0f);
            break;
        case 3:
            tiny3d_VertexTexture2(MOD_FLOAT(x+dx, font_datas.mod_x), MOD_FLOAT(y, font_datas.mod_y));
            break;
        default:
            break;
    }

    tiny3d_VertexPos(x + dx, y + dy, z);
    tiny3d_VertexTexture(0.95f, 0.95f);

    switch(font_datas.enable_doubletextures) {
        case 1:
            tiny3d_VertexTexture2(0.95f, 0.95f);
            break;
        case 2:
            tiny3d_VertexTexture2((x+dx)/848.0f, (y+dy)/512.0f);
            break;
        case 3:
            tiny3d_VertexTexture2(MOD_FLOAT(x+dx, font_datas.mod_x), MOD_FLOAT(y+dy,font_datas.mod_y));
            break;
        default:
            break;
    }

    tiny3d_VertexPos(x     , y + dy, z);
    tiny3d_VertexTexture(0.0f, 0.95f);

    switch(font_datas.enable_doubletextures) {
        case 1:
            tiny3d_VertexTexture2(0.0f, 0.95f);
            break;
        case 2:
            tiny3d_VertexTexture2(x/848.0f, (y+dy)/512.0f);
            break;
        case 3:
            tiny3d_VertexTexture2(MOD_FLOAT(x, font_datas.mod_x), MOD_FLOAT(y+dy, font_datas.mod_y));
            break;
        default:
            break;
    }

    tiny3d_End();

}

static int i_must_break_line(char *str, float x)
{
    int xx = 0;

    while(*str)
    {
        if(((u8)*str) <= 32) break;
        xx += font_datas.sx * font_datas.fonts[font_datas.current_font].fw[((u8)*str)] / font_datas.fonts[font_datas.current_font].w;
        str++;
    }


    if(*str && (x+xx) >= font_datas.autonewline) return 1;

    return 0;
}

float DrawString(float x, float y, char *str)
{

    if(font_datas.current_font == 8)
    {
        int len;

        set_ttf_window(0, 0, 848, 512, font_datas.autonewline ? WIN_AUTO_LF : 0);
        len = display_ttf_string(0, 0, str, 0, 0, font_datas.sx, font_datas.sy);

        if(font_datas.autocenter)
        {
            x = (font_datas.screen_w - len) / 2;
        }

        if(font_datas.bkcolor)
        {
            tiny3d_SetTextureWrap(0, font_datas.rsx_text_bk_offset, 8,
            8, 8 * 2,
            TINY3D_TEX_FORMAT_A4R4G4B4, TEXTWRAP_CLAMP, TEXTWRAP_CLAMP, TEXTURE_LINEAR);


            tiny3d_SetPolygon(TINY3D_QUADS);

            tiny3d_VertexPos(x     , y     , font_datas.Z);
            tiny3d_VertexColor(font_datas.bkcolor);
            tiny3d_VertexTexture(0.0f, 0.0f);

            tiny3d_VertexPos(x + (float) len, y     , font_datas.Z);

            tiny3d_VertexPos(x + (float) len, y + (float) Y_ttf/*font_datas.sy*/ , font_datas.Z);

            tiny3d_VertexPos(x              , y + (float) Y_ttf/*font_datas.sy*/, font_datas.Z);

            tiny3d_End();
        }

        Z_ttf = font_datas.Z;
        x = (float) display_ttf_string(x, y, str, font_datas.color, 0, font_datas.sx, font_datas.sy);
        Z_ttf = 0.0f;

        font_datas.X = x; font_datas.Y = Y_ttf;
        return x;
    }

    if(font_datas.autocenter)
    {
        x = (font_datas.screen_w - WidthFromStr((u8 *) str)) / 2;
    }

    while (*str)
    {
        if(*str == '\n')
        {
            x = 0.0f;
            y += font_datas.sy * font_datas.fonts[font_datas.current_font].bh / font_datas.fonts[font_datas.current_font].h;
            str++;
            continue;
        }
        else
        {
            if(font_datas.autonewline && i_must_break_line(str, x))
            {
                x = 0.0f;
                y += font_datas.sy * font_datas.fonts[font_datas.current_font].bh / font_datas.fonts[font_datas.current_font].h;
            }
        }

        DrawChar(x, y, font_datas.Z, (u8) *str);
        x += font_datas.sx * font_datas.fonts[font_datas.current_font].fw[((u8)*str)] / font_datas.fonts[font_datas.current_font].w;
        str++;
    }

    font_datas.X = x; font_datas.Y = y;

    return x;
}

static char buff[4096];

float DrawFormatString(float x, float y, char *format, ...)
{
    char *str = (char *) buff;
    va_list opt;

    va_start(opt, format);
    vsprintf( (void *) buff, format, opt);
    va_end(opt);

    if(font_datas.current_font == 8)
    {
        int len;

        set_ttf_window(0, 0, 848, 512, font_datas.autonewline ? WIN_AUTO_LF : 0);
        len = display_ttf_string(0, 0, str, 0, 0, font_datas.sx, font_datas.sy);

        if(font_datas.autocenter)
        {
            x = (font_datas.screen_w - len) / 2;
        }

        if(font_datas.bkcolor) {

            tiny3d_SetTextureWrap(0, font_datas.rsx_text_bk_offset, 8,
            8, 8 * 2,
            TINY3D_TEX_FORMAT_A4R4G4B4, TEXTWRAP_CLAMP, TEXTWRAP_CLAMP, TEXTURE_LINEAR);


            tiny3d_SetPolygon(TINY3D_QUADS);

            tiny3d_VertexPos(x     , y     , font_datas.Z);
            tiny3d_VertexColor(font_datas.bkcolor);
            tiny3d_VertexTexture(0.0f, 0.0f);

            tiny3d_VertexPos(x + (float) len, y     , font_datas.Z);

            tiny3d_VertexPos(x + (float) len, y + (float) Y_ttf/*font_datas.sy*/ , font_datas.Z);

            tiny3d_VertexPos(x              , y + (float) Y_ttf/*font_datas.sy*/, font_datas.Z);

            tiny3d_End();
        }

        Z_ttf = font_datas.Z;
        x = (float) display_ttf_string(x, y, str, font_datas.color, 0, font_datas.sx, font_datas.sy);
        Z_ttf = 0.0f;

        font_datas.X = x; font_datas.Y = Y_ttf;
        return x;
    }

    if(font_datas.autocenter)
    {
        x = (font_datas.screen_w - WidthFromStr((u8 *) str)) / 2;
    }

    while (*str)
    {
        if(*str == '\n')
        {
            x = 0.0f;
            y += font_datas.sy * font_datas.fonts[font_datas.current_font].bh / font_datas.fonts[font_datas.current_font].h;
            str++;
            continue;
        }
        else
        {
            if(font_datas.autonewline && i_must_break_line(str, x))
            {
                x = 0.0f;
                y += font_datas.sy * font_datas.fonts[font_datas.current_font].bh / font_datas.fonts[font_datas.current_font].h;
            }
        }

        DrawChar(x, y, font_datas.Z, (u8) *str);

        x += font_datas.sx * font_datas.fonts[font_datas.current_font].fw[((u8)*str)] / font_datas.fonts[font_datas.current_font].w;
        str++;
    }

    font_datas.X = x; font_datas.Y = y;

    return x;
}
