#include "gfx.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <math.h>
#include "utils.h"
#include "ttf_render.h"
#include "main.h"

extern int background_sel;
extern int background_fx;
extern bool bBackgroundGears;

struct {
    float x, y, dx, dy, r, rs;

} m_twat[6];

void DrawAdjustBackground(u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_LINE_LOOP);

    tiny3d_VertexPos(0  , 0  , 65535);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(847, 0  , 65535);

    tiny3d_VertexPos(847, 511, 65535);

    tiny3d_VertexPos(0  , 511, 65535);
    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_LINES);

    tiny3d_VertexPos(0        , 0        , 65535);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(120      , 120      , 65535);

    tiny3d_VertexPos(847      , 0        , 65535);
    tiny3d_VertexPos(847 - 120, 120      , 65535);

    tiny3d_VertexPos(847, 511, 65535);
    tiny3d_VertexPos(847 - 120, 511 - 120, 65535);

    tiny3d_VertexPos(0  , 511, 65535);
    tiny3d_VertexPos(120      , 511 - 120, 65535);
    tiny3d_End();
}

void DrawBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_End();
}


void DrawTextBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);


    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.0f);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexTexture(0.99f, 0.0f);

    tiny3d_VertexPos(x + w, y + h, z);
    tiny3d_VertexTexture(0.99f, 0.99f);

    tiny3d_VertexPos(x    , y + h, z);
    tiny3d_VertexTexture(0.0f , 0.99f);

    tiny3d_End();
}


void DrawBoxLine(float x, float y, float z, float w, float h, u32 rgba, u32 rgba2)
{
    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_LINE_STRIP);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba2);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_VertexPos(x    , y    , z);

    tiny3d_End();
}


void DrawTextBoxLine(float x, float y, float z, float w, float h, u32 rgba, u32 rgba2)
{
    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.0f);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexTexture(0.99f, 0.0f);

    tiny3d_VertexPos(x + w, y + h, z);
    tiny3d_VertexTexture(0.99f, 0.99f);

    tiny3d_VertexPos(x    , y + h, z);
    tiny3d_VertexTexture(0.0f , 0.99f);

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_LINE_STRIP);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba2);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_VertexPos(x    , y    , z);

    tiny3d_End();
}

#define QUADPOLY(x1, y1, z1, w1, x2, y2, z2, w2, color, color2) \
{\
    tiny3d_VertexPos(x1     , y1, z1); \
    tiny3d_VertexColor(color);         \
    tiny3d_VertexPos(x1 + w1, y1, z1); \
    tiny3d_VertexPos(x2 + w2, y2, z2); \
    tiny3d_VertexColor(color2);        \
    tiny3d_VertexPos(x2     , y2, z2); \
}

void DrawTextBoxCover(float x, float y, float z, float w, float h, u32 rgba, int type)
{
    u32 color = 0xc0c0c0c0;

    if(type == 2) color = 0x0020c0c0;

    if(type == 1 || type == 3)
    {
        u32 color2 = 0x000000ff;

        if(type == 3) color2 = 0x0020c0ff;

        tiny3d_SetPolygon(TINY3D_QUADS);

        QUADPOLY(x + 0.0f, y, z, w,
                 x + 0.0f, y + h, z, w, 0xc0c0c060, 0xc0c0c060);

        QUADPOLY(x + 0.0f, y + 2.0f, z, w / 10.0f,
                 x + 0.0f, y + h - 2.0f, z, w / 10.0f, color2, color2);
        tiny3d_End();

        x+= w/10.0f;

        w-= w/10.0f;
    }
    else
    {
        tiny3d_SetPolygon(TINY3D_QUADS);
        QUADPOLY(x + 4.5f, y + 0.0f, z, w - 9.0f,
                 x + 1.5f, y + 1.0f, z, w - 3.0f, color, color);
        QUADPOLY(x + 1.5f, y + 1.0f, z, w - 3.0f,
                 x + 1.5f, y + 1.5f, z, w - 3.0f, color, color);

        QUADPOLY(x + 1.5f, y + 1.5f, z, w - 3.0f,
                 x + 1.0f, y + 1.5f, z, w - 2.0f, color, color);

        QUADPOLY(x + 1.0f, y + 1.5f, z, w - 2.0f,
                 x + 0.0f, y + 4.5f, z, w - 0.0f, color, color);

        //

        QUADPOLY(x + 0.0f, y + 4.5f, z, w - 0.0f,
                 x + 0.0f, y + h - 4.5f, z, w - 0.0f, color, color);

        float y2 = y + h - 4.5f;

        //

        QUADPOLY(x + 0.0f, y2 + 0.0f, z, w - 0.0f,
                 x + 1.0f, y2 + 3.0f, z, w - 2.0f, color, color);

        QUADPOLY(x + 1.0f, y2 + 3.0f, z, w - 2.0f,
                 x + 1.5f, y2 + 3.0f, z, w - 3.0f, color, color);

        QUADPOLY(x + 1.5f, y2 + 3.0f, z, w - 3.0f,
                 x + 1.5f, y2 + 3.5f, z, w - 3.0f, color, color);

        QUADPOLY(x + 1.5f, y2 + 3.5f, z, w - 3.0f,
                 x + 4.5f, y2 + 4.5f, z, w - 9.0f, color, color);

        tiny3d_End();
    }

    float w2 = w;

    if(!type)
    {
        y += h / 40.0f * 3.0;
        h -= h / 10;
        w -= w / 45.0f;
    }
    else if(type == 1 || type == 3)
    {
        y += 2.0f;
        h -= 4.0f;
        w -= 2.0f;
    }
    else
    {
        y += h / 40.0f;
        h -= h / 20.0f;
        w -= w / 45.0f;
    }

    // texture

    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.0f);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexTexture(0.99f, 0.0f);

    tiny3d_VertexPos(x + w, y + h, z);
    tiny3d_VertexTexture(0.99f, 0.99f);

    tiny3d_VertexPos(x    , y + h, z);
    tiny3d_VertexTexture(0.0f , 0.99f);

    tiny3d_End();

    if(type) return;

    tiny3d_SetPolygon(TINY3D_LINES);

    tiny3d_VertexPos(x     , y - 3.0f, z);
    tiny3d_VertexColor(0xa0a0a0c0);
    tiny3d_VertexPos(x + w2, y - 3.0f, z);

    tiny3d_VertexPos(x     , y - 1.0f, z);
    tiny3d_VertexPos(x + w2, y - 1.0f, z);

    //tiny3d_VertexPos(x + w + 2.0f , y       , z);
    //tiny3d_VertexPos(x + w + 2.0f , y + h   , z);

    tiny3d_End();
}

void DrawTextBoxCoverShadow(float x, float y, float z, float w, float h, u32 rgba, int type)
{
    u32 color = 0x60606090;

    if(type == 2) color = 0x00106090;

    if(type == 1 || type == 3)
    {
        u32 color2 = 0x00000060;

        if(type == 3) color2 = 0x0020c060;

        tiny3d_SetPolygon(TINY3D_QUADS);

        QUADPOLY(x + 0.0f, y, z, w,
                 x - 1.0f, y + h + 2.0f, z, w, 0xc0c0c060, 0x20202000);

        QUADPOLY(x + 0.0f, y + 2.0f, z, w/10.0f,
                 x - 1.0f, y + h, z, w/10.0f + 1.0f, color2, 0x20202000);

        tiny3d_End();

        x += w/10.0f;

        w -= w/10.0f;

    }
    else
    {
        tiny3d_SetPolygon(TINY3D_QUADS);

        QUADPOLY(x + 4.5f, y + 0.0f, z, w - 9.0f,
                 x + 1.5f, y + 1.0f, z, w - 3.0f, color, color);

        QUADPOLY(x + 1.5f, y + 1.0f, z, w - 3.0f,
                 x + 1.5f, y + 1.5f, z, w - 3.0f, color, color);

        QUADPOLY(x + 1.5f, y + 1.5f, z, w - 3.0f,
                 x + 1.0f, y + 1.5f, z, w - 2.0f, color, color);

        QUADPOLY(x + 1.0f, y + 1.5f, z, w - 2.0f,
                 x + 0.0f, y + 4.5f, z, w - 0.0f, color, color);


        QUADPOLY(x + 0.0f, y + 4.5f, z, w - 0.0f,
                 x - 1.0f, y + h - 4.5f, z, w + 2.0f, color, 0x20202000);

        float y2 = y + h - 4.5f;

        QUADPOLY(x - 1.0f, y2 + 0.0f, z, w + 2.0f,
                 x + 0.0f, y2 + 3.0f, z, w - 0.0f, 0x20202000, 0x20202000);

        QUADPOLY(x + 0.0f, y2 + 3.0f, z, w - 0.0f,
                 x + 0.5f, y2 + 3.0f, z, w - 1.0f, 0x20202000, 0x20202000);

        QUADPOLY(x + 0.5f, y2 + 3.0f, z, w - 1.0f,
                 x + 0.5f, y2 + 3.5f, z, w - 1.0f, 0x20202000, 0x20202000);

        QUADPOLY(x + 0.5f, y2 + 3.5f, z, w - 1.0f,
                 x + 3.5f, y2 + 4.5f, z, w - 7.0f, 0x20202000, 0x20202000);

        tiny3d_End();
    }

    if(!type)
    {
        y += h / 40.0f * 3.0;
        h -= h / 10;
        w -= w / 45.0f;
    }
    else if(type == 1 || type == 3)
    {
        y += 2.0f;
        h -= 4.0f;
        w -= 2.0f;
    }
    else
    {
        y += h / 40.0f;
        h -= h / 20.0f;
        w -= w / 45.0f;
    }

    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.99f);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.99f, 0.99f);

    rgba = 0x20202000;
    tiny3d_VertexPos(x + w + 2, y + h, z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.99f, 0.00f);

    tiny3d_VertexPos(x - 1   , y + h, z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.00f);

    tiny3d_End();
}

void DrawTextBoxShadow(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.99f);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.99f, 0.99f);

    rgba = 0x20202000;
    tiny3d_VertexPos(x + w + 2, y + h, z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.99f, 0.00f);

    tiny3d_VertexPos(x - 1   , y + h, z);
    tiny3d_VertexColor(rgba);
    tiny3d_VertexTexture(0.0f , 0.00f);

    tiny3d_End();
}

void DrawBoxShadow(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_QUADS);


    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexColor(rgba);

    rgba = 0x20202000;
    tiny3d_VertexPos(x + w + 2, y + h, z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x - 1   , y + h, z);
    tiny3d_VertexColor(rgba);

    tiny3d_End();
}

void DrawLineBox(float x, float y, float z, float w, float h, u32 rgba)
{
    tiny3d_SetPolygon(TINY3D_LINE_LOOP);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);

    tiny3d_VertexPos(x + w, y + h, z);

    tiny3d_VertexPos(x    , y + h, z);

    tiny3d_End();
}

static void DrawBox2(float x, float y, float z, float w, float h, u32 rgba, u32 rgba2)
{
    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , z);
    tiny3d_VertexColor(rgba2);

    tiny3d_VertexPos(x + w, y + h, z);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x    , y + h, z);
    tiny3d_VertexColor(rgba2);

    tiny3d_End();
}

/*

float DrawButton1(float x, float y, float w, char * t, int select)
{
    int len = strlen(t);
    u32 rgba = 0xffffffff, brgba = 0x000000ff;

    if(w < (len + 2) * 12) w = (len + 2) * 12;

    SetFontAutoCenter(0);
    SetCurrentFont(FONT_BUTTON);
    SetFontSize(12, 32);

    if(select == 1) {rgba = 0x000000ff; brgba = 0xffffffff;}
        else
    if(select == -1) {rgba = 0x0000006f; brgba = 0xffffff6f;}

    SetFontColor(rgba, 0x0);

    DrawBox(x, y, 0.0f, w, 40, brgba);
    DrawLineBox(x, y, 0.0f, w, 40, rgba);

    DrawString(x + (w - len * 12) / 2, y + 4, t);

    return x + w;

}

float DrawButton2(float x, float y, float w, char * t, int select)
{
    int len = strlen(t);
    u32 rgba = 0xffffffff, brgba = 0x0000c080;

    if(w < (len + 2) * 10) w = (len + 2) * 10;

    SetFontAutoCenter(0);
    SetCurrentFont(FONT_BUTTON);
    SetFontSize(10, 32);

    if(select == 2) {rgba = 0xffffffff; brgba = 0xc01000ff;}
        else
    if(select == 1) {rgba = 0xffffffff; brgba = 0x00c000ff;}
        else
    if(select == -1) {rgba = 0xffffff6f; brgba = 0x00c0006f;}

    SetFontColor(rgba, 0x0);

    DrawBox(x, y, 0.0f, w, 40, brgba);
    DrawLineBox(x, y, 0.0f, w, 40, 0xc0c0c0ff);

    DrawString(x + (w - len * 10) / 2, y + 4, t);

    return x + w;

}

*/

float DrawButton1_UTF8(float x, float y, float w, char * t, int select)
{
    int len = 0;
    u32 rgba = 0xffffffff, brgba = 0x10101088;
    u32 brgba2 = 0x404040ff;

    set_ttf_window(0, 0, 848, 512, WIN_SKIP_LF);
    len = display_ttf_string(0, 0, t, 0, 0, 16, 32);

    if(w < (len + 24)) w = (len + 24);

    if(select ==  1) {brgba2 = 0xf0f0f0ff; brgba = 0x505050ee;}
    else
    if(select == -1) {brgba2 = brgba = 0xffffff6f; rgba = 0x0000006f;}

    DrawBox2(x, y, 0.0f, w, 40, brgba, brgba2);
    DrawLineBox(x, y, 0.0f, w, 40, rgba);

    //DrawString(x + (w - len * 12) / 2, y + 4, t);
    display_ttf_string(x + (w - len) / 2, y + 4, t, rgba, 0, 16, 32);

    return x + w;

}

float DrawButton2_UTF8(float x, float y, float w, char * t, int select)
{
    int len = 0;
    u32 rgba = 0xffffffff, brgba = 0x0030d044;
    u32 brgba2 = 0x00308044;

    set_ttf_window(0, 0, 848, 512, WIN_SKIP_LF);
    len = display_ttf_string(0, 0, t, 0, 0, 14, 32);

    if(w < (len + 20)) w = (len + 20);

    if(select ==  2) {rgba = 0xffffffff; brgba = 0xd01000ff; brgba2 = 0x801000ff;}
    else
    if(select ==  1) {rgba = 0xffffffff; brgba = 0x00d000ff; brgba2 = 0x008030ff;}
    else
    if(select == -1) {rgba = 0xffffff6f; brgba = 0x0040006f; brgba2 = 0x00d0006f;}

    DrawBox2(x, y, 0.0f, w, 40, brgba, brgba2);
    DrawLineBox(x, y, 0.0f, w, 40, 0xc0c0c0ff);

    //DrawString(x + (w - len * 10) / 2, y + 4, t);
    display_ttf_string(x + (w - len) / 2, y + 4, t, rgba, 0, 14, 32);

    return x + w;

}


void init_twat()
{
    m_twat[0].r = m_twat[1].r = m_twat[2].r = 0.0f;
    m_twat[3].r = m_twat[4].r = m_twat[5].r = 0.0f;
}

void update_twat(bool background)
{
    float x = 840.0f - 170.0f, y = 512.0f - 180.0f;

    if(background && Png_offset[BIG_PICT + 1])
    {
        int i = BIG_PICT + 1;
        tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width,
                              Png_datas[i].height, Png_datas[i].wpitch,
                              TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox(-1, -1, 128000.0f, 850, 514, (background_sel & 1) ? 0xffffffff : 0xcfcfcfff);
    }

    if(!bBackgroundGears) return;

    m_twat[0].r += .01f;
    m_twat[1].r -= .01f;
    m_twat[2].r += .01f;

    draw_twat(x + 70.0f, y - 70.0f, m_twat[0].r + 0.196349540875f);
    draw_twat(x, y, m_twat[1].r);
    draw_twat(x + 70.0f, y + 70.0f, m_twat[2].r + 0.196349540875f);


    x = 240.0f; y = 512.0f - 120.0f;
    draw_twat2(x + 35.0f, y - 35.0f, m_twat[0].r + 0.196349540875f);
    draw_twat2(x, y, m_twat[1].r);
    draw_twat2(x + 35.0f, y + 35.0f, m_twat[2].r + 0.196349540875f);

    x = 848 - 280.0f; y = 512.0f - 120.0f;
    draw_twat2(x, y + 35.0f, m_twat[2].r + 0.196349540875f);


    x = 840.0f - 170.0f; y = 180.0f + 50.0f;
    draw_twat(x + 70.0f, y - 70.0f, m_twat[1].r);


    x = 848.0f / 2.0f - 70.0f; y = 512.0f / 2.0f;

    draw_twat(x + 70.0f, y - 70.0f, m_twat[0].r + 0.196349540875f);
    draw_twat(x, y, m_twat[1].r);
    draw_twat(x + 70.0f, y + 70.0f, m_twat[2].r + 0.196349540875f);

    x = 100.0f; y = 200.0f;

    m_twat[3].r -= .01f;
    m_twat[4].r += .01f;
    m_twat[5].r -= .01f;

    draw_twat(x, y - 70.0f, m_twat[3].r + 0.196349540875f);
    draw_twat(x + 70.0f, y, m_twat[4].r);
    draw_twat(x, y + 70.0f, m_twat[5].r + 0.196349540875f);

    x = 848 - 280.0f; y = 140.0f;

    draw_twat2(x, y - 35.0f, m_twat[3].r + 0.196349540875f);
    draw_twat2(x + 35.0f, y, m_twat[4].r);
    draw_twat2(x, y + 35.0f, m_twat[5].r + 0.196349540875f);

    x = 215.0f + 60.0f; y = 140.0f;

    draw_twat2(x, y - 35.0f, m_twat[3].r + 0.196349540875f);

    x = 100.0f; y = 512.0f - 170.0f - 40.0f;
    draw_twat(x, y + 70.0f, m_twat[4].r);
}

void draw_twat(float x, float y, float angle)
{
    int n;

    float ang, angs = 6.2831853071796 / 16, angs2 = 6.2831853071796 / 32;

    MATRIX matrix;

    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x , y , 65535.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLES);

    ang = 0.0f;

    for(n = 0; n < 16; n++)
    {
        tiny3d_VertexPos(40.0f *sinf(ang), 40.0f *cosf(ang), 1000);
        tiny3d_VertexColor(0xffffff18);
        tiny3d_VertexPos(58.0f *sinf(ang+angs/2), 58.0f *cosf(ang+angs/2), 1000);
        tiny3d_VertexColor(0xffffff18);
        tiny3d_VertexPos(40.0f *sinf(ang+angs), 40.0f *cosf(ang+angs), 1000);
        tiny3d_VertexColor(0xffffff18);

        ang += angs;
    }

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_POLYGON);

    ang = 0.0f;

    tiny3d_VertexPos(0.0f * sinf(ang), 0.0f * cosf(ang), 1000);
    tiny3d_VertexColor(0xffffff18);
    for(n = 0; n < 33; n++)
    {
        tiny3d_VertexPos(30.0f * sinf(ang), 30.0f * cosf(ang), 1000);
        if(n & 1) tiny3d_VertexColor(0x80ffff18); else tiny3d_VertexColor(0xffffff18);
        ang += angs2;
    }

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity
}

void draw_twat2(float x, float y, float angle)
{
    int n;

    float ang, angs = 6.2831853071796 / 16, angs2 = 6.2831853071796 / 32;

    MATRIX matrix;

    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x , y , 65535.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLES);

    ang = 0.0f;

    for(n = 0; n < 16; n++)
    {
        tiny3d_VertexPos(20.0f *sinf(ang), 20.0f *cosf(ang), 1000);
        tiny3d_VertexColor(0xffffff18);
        tiny3d_VertexPos(28.0f *sinf(ang+angs/2), 28.0f *cosf(ang + angs/2), 1000);
        tiny3d_VertexColor(0xffffff18);
        tiny3d_VertexPos(20.0f *sinf(ang+angs), 20.0f *cosf(ang + angs), 1000);
        tiny3d_VertexColor(0xffffff18);

        ang += angs;
    }

    tiny3d_End();

    tiny3d_SetPolygon(TINY3D_POLYGON);

    ang = 0.0f;

    tiny3d_VertexPos(0.0f * sinf(ang), 0.0f * cosf(ang), 1000);
    tiny3d_VertexColor(0xffffff18);
    for(n = 0; n < 33; n++)
    {
        tiny3d_VertexPos(15.0f * sinf(ang), 15.0f * cosf(ang), 1000);
        if(n & 1) tiny3d_VertexColor(0x80ffff18); else tiny3d_VertexColor(0xffffff18);
        ang += angs2;
    }

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity

}

// GUI alternative

static struct {
    float x, y;
    float dx, dy;
    float rad;
    float rot;
    int sides;
    u32 color;
    u32 color2;
} bubble[32];

int GFX1_mode = 0;
int GFX1_counter = 0;

static void DrawBubble(float x, float y, float layer, float dx, float dy, u32 rgba, u32 rgba2, float angle, int sides)
{
    dx /= 2.0f; dy /= 2.0f;

    int n;

    MATRIX matrix;

    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x, y, 0.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLE_FAN);

    tiny3d_VertexPos(0, 0, layer);
    tiny3d_VertexColor(rgba);

    float ang = 0.0f, ang2 = 6.2831853071796 / (float) sides;

    for(n = 0; n <= sides; n++)
    {
        tiny3d_VertexPos(dx * sinf(ang), dy * cosf(ang), layer);
        tiny3d_VertexColor(rgba2);
        ang+= ang2;
    }

    tiny3d_End();

    // hack for Tiny3D (force to change the shader)
    tiny3d_SetPolygon(TINY3D_POINTS);
        tiny3d_VertexPos(0.0f, 0.0f, 0.0f);
        tiny3d_VertexColor(0);
        tiny3d_VertexTexture(0.0f , 0.0f);

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity
}


static void DrawGFX1(float x, float y, float layer, float dx, float dy, u32 rgba, u32 rgba2, float angle)
{
    dx /= 2.0f; dy /= 2.0f;

    int n;

    MATRIX matrix;

    // rotate and translate the sprite
    matrix = MatrixRotationZ(angle);
    matrix = MatrixMultiply(matrix, MatrixTranslation(x, y, 0.0f));

    // fix ModelView Matrix
    tiny3d_SetMatrixModelView(&matrix);

    tiny3d_SetPolygon(TINY3D_TRIANGLE_FAN);

    tiny3d_VertexPos(0, 0, layer);
    tiny3d_VertexColor(rgba);

    float ang = 0.0f, ang2 = 6.2831853071796 / 20.0;

    for(n = 0; n <= 20; n++)
    {
        float m = (float) ((1 + rand() % 10))/10.0f;

        tiny3d_VertexPos(dx * sinf(ang) * m, dy * cosf(ang) * m, layer);
        tiny3d_VertexColor(rgba2);
        ang+= ang2;
    }

    tiny3d_End();

    // hack for Tiny3D (force to change the shader)
    tiny3d_SetPolygon(TINY3D_POINTS);
        tiny3d_VertexPos(0.0f, 0.0f, 0.0f);
        tiny3d_VertexColor(0);
        tiny3d_VertexTexture(0.0f , 0.0f);

    tiny3d_End();

    tiny3d_SetMatrixModelView(NULL); // set matrix identity
}

void GFX1_background()
{
    //
    static int one = 1;
    int n;

    if(one)
    {
        srand(1);

        for(n = 0; n < 32; n++) {
            bubble[n].x = rand() % 848;
            bubble[n].y = rand() % 512;
            bubble[n].rad = (4 + (rand() % 56));

            do
            {
                bubble[n].dx = (((float) (rand() % 32) - 16.0f)/32.0f) * 2.0f;
            }
            while(!bubble[n].dx);

            do
            {
                bubble[n].dy = (((float) (rand() % 32) - 16.0f)/32.0f) * 2.0f;
            }
            while(!bubble[n].dy);

            bubble[n].sides = 5 + (rand() & 0x7);

            bubble[n].rot = 0;
            switch(n & 3)
            {
                case 0:
                    bubble[n].color = 0x80008f30;
                    bubble[n].color2 = 0x40008030;
                    break;

                case 1:
                    bubble[n].color = 0x0010ff30;
                    bubble[n].color2 = 0x00208030;
                    break;

                case 2:
                    bubble[n].color = 0x8f600030;
                    bubble[n].color2 = 0x40200030;
                    break;

                case 3:
                    bubble[n].color = 0x80108f30;
                    bubble[n].color2 = 0x30200030;
                    break;
            }
        }
        one = 0;
    }

    static float rot = 0.0f;

    static u32 counter = 0;
    counter++;
    u32 col1 = (((counter>>4) & 0xf) * 0x40 / 0xf);
    u32 col2 = (((counter>>4) & 0xf) * 0x30 / 0xf);
    u32 col3 = ((0xf - ((counter>>4) & 0xf)) * 0x40 / 0xf);
    u32 col4 = ((0xf - ((counter>>4) & 0xf)) * 0x30 / 0xf);
    //u32 col5 = (((counter>>4) & 0xf) * 0x30 / 0xf) << 24;

    u32 col5 = (counter>>4); col5 = (col5 & 0x10) ? 0xf - (col5 & 0xf) : (col5 & 0xf);

    col5 = (col5 * 0x30 / 0xf) << 24;

    if(Png_offset[BIG_PICT + 1])
    {
        int i = BIG_PICT + 1;
        tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width,
                                             Png_datas[i].height, Png_datas[i].wpitch,
                                             TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

        DrawTextBox(-1, -1, 1000, 850, 514, (background_sel & 1) ? 0xffffffff : 0xcfcfcfff);
    }
    else
    {
        if(background_sel & 1) DrawBubble(256, 128, 1000, 256 * 7, 256 * 7, 0x052010ff | col5, 0x030010ff | col5, rot, 8);
        else DrawBubble(256, 128, 1000, 256 * 7, 256 * 7, 0x050020ff | col5, 0x031010ff | col5, rot, 8);
    }

    int sel = background_fx >> 1;

    if(!(sel & 2))
    {
        if(counter & 0x100)
        {
            DrawGFX1(128, 128 - 64, 1000, 256 * 2, (256 - (counter & 0x1f)) * 2, 0xc0004000 + col1, 0x40001000 + col2, rot);
            DrawGFX1(848 - 128, 256 + 128, 1000, 256 * 2, 256 * 2, 0xc000c000 + col1, 0x40004000 + col2, -rot);
            DrawGFX1(128 + 128, 256 + 128, 1000, 256 * 2, 256 * 2, 0xc0c00000 + col3, 0x40100000 + col4, rot);
            DrawGFX1(848 - 128 - 128, 256 - 128, 1000, 256 * 2, (256 - (counter & 0x1f)) * 2, 0x40c00000 + col3, 0x40400000 + col4, -rot);
        }
        else
        {
            DrawGFX1(128, 128, 1000, 256 * 2, (256 - (counter & 0x1f)) * 2, 0xc000c000 + col1, 0x40004000 + col2, rot);
            DrawGFX1(848 - 128, 256 + 128, 1000, 256 * 2, 256 * 2, 0xc0004000 + col1, 0x40001000 + col2, -rot);
            DrawGFX1(128 + 128 - 32, 256 + 128 + 16, 1000, 256 * 2, 256 * 2, 0x40c00000 + col3, 0x40400000 + col4, rot);
            DrawGFX1(848 - 128 - 128, 256 - 192, 1000, 256 * 2, (256 - (counter & 0x1f)) * 2, 0xc0c00000 + col3, 0x40100000 + col4, -rot);
        }
    }

    rot+= 0.01f;
    if(rot > 6.2831853071796) rot -= 6.2831853071796;

    for(n = 0; n < 32; n++)
    {
        int sides = bubble[n].sides > 11 ? 20 : bubble[n].sides;

        if(sel & 1) sides = (n & 3) + 3;

        DrawBubble(bubble[n].x, bubble[n].y, 1000, bubble[n].rad, bubble[n].rad, bubble[n].color, bubble[n].color2, bubble[n].rot, sides);

        if(GFX1_mode == 1) {bubble[n].x+= -fabs(bubble[n].dx * 20.0f); bubble[n].y+= bubble[n].dy * 5.0f;  bubble[n].rot += .075f;}
        else if(GFX1_mode == 2) {bubble[n].x+= fabs(bubble[n].dx * 20.0f); bubble[n].y+= bubble[n].dy * 5.0f; bubble[n].rot += -.075f;}
        else
        {
            bubble[n].x+= bubble[n].dx;
            bubble[n].y+= bubble[n].dy;
            bubble[n].rot += -bubble[n].dx/32.0f;
        }

        if(bubble[n].rot < -6.2831853071796) bubble[n].rot += 6.2831853071796;
        if(bubble[n].rot >  6.2831853071796) bubble[n].rot -= 6.2831853071796;

        if(bubble[n].x + bubble[n].rad/2.0f < -10.0f || bubble[n].x > 860.0f || bubble[n].y + bubble[n].rad/2.0f < -10.0f || bubble[n].y > 520.0f)
        {
            bubble[n].sides = 5 + (rand() & 0x7);
            if(GFX1_mode == 1) bubble[n].x = 848;
            else if(GFX1_mode == 2) bubble[n].x = 0;
            else bubble[n].x = 848/2 + (n-16) * 16; bubble[n].y = 512/2;
        }
    }

    if(!GFX1_counter) GFX1_mode= 0; else GFX1_counter --;

}
