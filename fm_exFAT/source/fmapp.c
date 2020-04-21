#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

//#define _FPS

#include <io/pad.h>
#include <osk_input.h>
#define SUCCESS 	1
#define FAILED	 	0

#include <tiny3d.h>
#include <libfont.h>

// font 0: 224 chr from 32 to 255, 16 x 32 pix 2 bit depth
#include "font.h"

// font 1: 224 chr from 32 to 255, 16 x 32 pix 2 bit depth
#include "font_b.h"

// font 2: 255 chr from 0 to 254, 8 x 8 pix 1 bit depth
extern unsigned char msx[];

#include "console.h"

#include "ff.h"
#include "fflib.h"
#include "fm.h"
#include "util.h"
#include "fsutil.h"
#include "pad.h"

#include "types.h"

#include "ver.h"

typedef struct {
    int device;
    void *dirStruct;
} DIR_ITER;

#include "iosupport.h"
#include "storage.h"
#include <malloc.h>
#include <sys/file.h>
#include <lv2/mutex.h>
#include <sys/errno.h>

#include <sys/file.h>
#include <ppu-lv2.h>
#include <sys/stat.h>
#include <lv2/sysfs.h>

#include <sysutil/disc.h>
#include <sysutil/sysutil.h>
#include <sysmodule/sysmodule.h>
//app
int fmapp_init (int dt);      //1
//int app_input (int dt);     //2
int fmapp_update (int dt);    //3
int fmapp_render (int dt);    //4
int fmapp_cleanup (int dt);   //5
//
struct fm_panel lp, rp;
//

#ifdef LIBBUILD
extern void LoadTexture();
#else
void LoadTexture()
{
    u32 * texture_mem = tiny3d_AllocTexture(64*1024*1024); // alloc 64MB of space for textures (this pointer can be global)

    u32 * texture_pointer; // use to asign texture space without changes texture_mem

    if(!texture_mem) return; // fail!

    texture_pointer = texture_mem;

    ResetFont();
    texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) font  , (u8 *) texture_pointer, 32, 255, 16, 32, 2, BIT0_FIRST_PIXEL);
    texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) font_b, (u8 *) texture_pointer, 32, 255, 16, 32, 2, BIT0_FIRST_PIXEL);
    texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) msx   , (u8 *) texture_pointer,  0, 254,  8,  8, 1, BIT7_FIRST_PIXEL);

    // here you can add more textures using 'texture_pointer'. It is returned aligned to 16 bytes
}
#endif

//app
struct fm_panel *app_active_panel ()
{
    if (lp.active)
        return &lp;
    return &rp;
}

struct fm_panel *app_inactive_panel ()
{
    if (lp.active)
        return &rp;
    return &lp;
}
//restore app state after other module was executed
int _fmapp_restore (char init)
{
    if (init)
        initConsole ();
    //DbgHeader("FATFS EXFAT Example");
    //DbgMess("Press o/circle to exit");
    NPrintf("\n");
    return 0;
}//1st

int fmapp_init (int dt)
{
    fm_panel_init (&lp, 0, 0, PANEL_W*8, PANEL_H*8, 1);
    fm_panel_init (&rp, PANEL_W*8, 0, PANEL_W*8, PANEL_H*8, 0);
    //debug console
    initConsole ();
    //fatfs test
    //fatfs_init ();
    fflib_init();
    //
    tiny3d_Init (1024*1024);
    //
	ioPadInit (7);
    //
	// Load texture
    LoadTexture ();
    //
    //DbgHeader("FATFS EXFAT Example");
    //
    fm_panel_scan (&lp, NULL);
    fm_panel_scan (&rp, NULL);
    //
    fm_status_set ("SQUARE   - Copy content", 0, 0xffff00FF);
    fm_status_set ("TRIANGLE - Erase content", 1, 0xff0000FF);
    fm_status_set ("LEFT / RIGHT - Switch active panel", 2, 0x00ff00FF);
    fm_status_set ("L1 / R1  - Navigate folders as well as CROSS and CIRCLE", 3, 0xeeeeeeFF);
    //
    return 1;
}
//2nd input - skip, we read in app_update
//3rd
// --automount logic, every X update: X = 60fps * 4sec = 240 frames
#define AUTOMOUNT_FREQ  (3 * 60)
static int amount_k = 0;
int fmapp_update(int dat)
{
    //2 input
    ps3pad_read ();
    //mount monitoring logic: every 4 sec
    if (!app_active_panel ()->path)
    {
        if (amount_k <= 0)
        {
            //probe rootfs devices
            if (rootfs_probe ())
            {
                fm_panel_scan (&lp, NULL);
                fm_panel_scan (&rp, NULL);
            }
            amount_k = AUTOMOUNT_FREQ;
        }
        amount_k --;
    }
    //go up
    if (NPad (BUTTON_CIRCLE))
    {
        //fm_panel_exit (app_active_panel ());
        if (fm_panel_exit (app_active_panel ()))
        {
            //really quit?
            //if (1 == YesNoDialog ("Quit the file manager?"))
                return -1;
        }
    }
    //activate left panel
    else if (NPad (BUTTON_LEFT))
    {
        if(lp.active)
            fm_panel_exit (app_active_panel ());
        else
        {
            lp.active = 1;
            rp.active = 0;
        }
    }
    //activate right panel
    else if (NPad (BUTTON_RIGHT))
    {
        lp.active = 0;
        rp.active = 1;
    }
    //scroll panel up
    else if (APad (BUTTON_UP))
    {
        fm_panel_scroll (app_active_panel (), FALSE);
    }
    //scroll panel dn
    else if (APad (BUTTON_DOWN))
    {
        fm_panel_scroll (app_active_panel (), TRUE);
    }
    //enter dir
    else if (NPad (BUTTON_R1))
    {
        fm_panel_enter (app_active_panel ());
    }
    //exit dir
    else if (NPad (BUTTON_L1))
    {
        fm_panel_exit (app_active_panel ());
    }

    //rename
    else if (NPad (BUTTON_START))
    {
        char sp[CBSIZE];
        struct fm_panel *ps = app_active_panel ();
        if (ps->path && ps->current)
        {
            fm_status_set ("", 1, 0xffeeeeFF);
            //check if we're allowed to rename item
            if(strlen(ps->path) < 6 || strstr(ps->path, "/dev_flash/") || strstr(ps->path, "/dev_bdvd/"))
            {
                snprintf (sp, CBSIZE, "Rename is not allowed in %s", ps->path);
                fm_status_set (sp, 0, 0xffeeeeFF);
            }
            else
            {
                snprintf (sp, CBSIZE, "%s", ps->current->name);
                if(Get_OSK_String("Rename", sp, CBSIZE) == 0)
                {
                    //rename
                    char lp[CBSIZE];
                    snprintf (lp, CBSIZE, "rename %s to %s", ps->current->name, sp);
                    fm_status_set (lp, 0, 0xffeeeeFF);
                    //
                    fm_job_rename (ps->path, ps->current->name, sp);
                    //reload for content refresh
                    fm_panel_reload (ps);
                    fm_panel_locate (ps, sp);
                }
            }
        }
    }
    //new dir
    else if (NPad (BUTTON_SELECT))
    {
        char sp[CBSIZE];
        struct fm_panel *ps = app_active_panel ();
        if (ps->path)
        {
            fm_status_set ("", 1, 0xffeeeeFF);
            //check if we're allowed to create dir?!
            if(strlen(ps->path) < 6 || strstr(ps->path, "/dev_flash/") || strstr(ps->path, "/dev_bdvd/"))
            {
                snprintf (sp, CBSIZE, "New folder is not allowed in %s", ps->path);
                fm_status_set (sp, 0, 0xffeeeeFF);
            }
            else
            {
                snprintf (sp, CBSIZE, "New folder");
                if(Get_OSK_String("New folder", sp, 255) == 0)
                {
                    //new dir
                    char lp[CBSIZE];
                    snprintf (lp, CBSIZE, "new dir %s", sp);
                    fm_status_set (lp, 0, 0xffeeeeFF);
                    fm_job_newdir (ps->path, sp);
                    //reload for content refresh
                    fm_panel_reload (ps);
                    fm_panel_locate (ps, sp);
                }
            }
        }
    }

    //cross - action: enter dir
    else if (NPad (BUTTON_CROSS))
    {
        fm_panel_enter (app_active_panel ());
    }
    else if (NPad (BUTTON_R3))
    {
        return -1;
    }
    //files delete
    else if (NPad (BUTTON_TRIANGLE))
    {
        char sp[CBSIZE];
        struct fm_panel *ps = app_active_panel ();
        if (ps->path)
        {
            snprintf (sp, CBSIZE, "%s/%s", ps->path, ps->current->name);
            fm_job_delete (sp, &fmapp_render);
            //reload for content refresh
            fm_panel_reload (ps);
        }
    }
    //files copy
    else if (NPad (BUTTON_SQUARE))
    {
        char sp[CBSIZE];
        char dp[CBSIZE];
        struct fm_panel *ps = app_active_panel ();
        struct fm_panel *pd = app_inactive_panel ();
        if (ps->path && pd->path)
        {
            snprintf (sp, CBSIZE, "%s/%s", ps->path, ps->current->name);
            snprintf (dp, CBSIZE, "%s/", pd->path);
            fm_job_copy (sp, dp, &fmapp_render);
            //reload inactive panel for content refresh
            fm_panel_reload (app_inactive_panel ());
        }
    }
    //
    return 0;
}
//4th
int fmapp_render(int dat)
{
    #if 1
    /* DRAWING STARTS HERE */
    tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);

    // Enable alpha Test
    tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

    // Enable alpha blending.
    tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
        TINY3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | TINY3D_BLEND_FUNC_DST_ALPHA_ZERO,
        TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
    #endif
    // change to 2D context ( virtual size of the screen is 848.0 x 512.0)
    fm_panel_draw (&lp);
    fm_panel_draw (&rp);
    //
    fm_status_draw (dat);
    //
#ifdef _FPS
    char sfps[8];
    snprintf (sfps, 7, "%dfps", fps_update ());
    SetFontColor (0xff0000ff, 0x00000000);
    SetFontAutoCenter (0);
    DrawString (800, 0, sfps);
#endif
#ifdef SWVER
    char swver[8];
    snprintf (swver, 7, "%s", SWVER);
    SetFontColor (0xff0000ff, 0x00000000);
    SetFontAutoCenter (0);
    DrawString (800, 504, swver);
#endif
    //
    tiny3d_Flip ();

    return 1;
}
//5th
int fmapp_cleanup(int dat)
{
#ifndef LIBBUILD
    ioPadEnd();
#endif
    return 1;
}

s32 fmapp_run()
{
    //1 init
	fmapp_init (0);
    _fmapp_restore (0);
	// Ok, everything is setup. Now for the main loop.
	while(1)
    {
        //3
        if (fmapp_update (0) == -1)
            break;
		//4
        //cls ();
        fmapp_render (0);
	}
    //5
    fmapp_cleanup (0);
    //
    return 0;
}
