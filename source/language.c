/*
 * Language IrisManager by D_Skywalk
 *
 * Copyright (c) 2011 David Colmenero Aka D_Skywalk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *   Simple code for play with languages
 *    for ps3 scene ;)
 *
**/
#include "loader.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "language.h"
#include "utils.h"

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
#else

#include "language_ini_en_bin.h"
#include "language_ini_sp_bin.h"
#include "language_ini_fr_bin.h"
#include "language_ini_it_bin.h"
#include "language_ini_nw_bin.h"
#include "language_ini_gm_bin.h"
#include "language_ini_por_bin.h"
#include "language_ini_ps_bin.h"
#include "language_ini_chs_bin.h"
#include "language_ini_cht_bin.h"
#include "language_ini_fi_bin.h"
#include "language_ini_gr_bin.h"
#include "language_ini_kr_bin.h"
#include "language_ini_da_bin.h"
#include "language_ini_pl_bin.h"

#endif

#define LANGFILE_VERSION 2

typedef struct lngstr
{
  u32 code;
  char * strname;
  char * strdefault;
} t_lngstr;

t_lngstr lang_strings[] =
{
    //MAIN
    // VIDEO - ADJUST
    {VIDEOADJUST_POSITION, "VIDEOADJUST_POSITION"    , "Use LEFT (-X) / RIGHT (+X) / UP (-Y) / DOWN (+Y) to adjust the screen" },
    {VIDEOADJUST_SCALEINFO, "VIDEOADJUST_SCALEINFO"  , "Video Scale X: %i Y: %i" },
    {VIDEOADJUST_EXITINFO, "VIDEOADJUST_EXITINFO"    , "Press CROSS to save settings and exit" },
    {VIDEOADJUST_DEFAULTS, "VIDEOADJUST_DEFAULTS"    , "Press CIRCLE to default values" },

    //SELECT - GAME FOLDER
    {GAMEFOLDER_WANTUSE, "GAMEFOLDER_WANTUSE"           , "Do you want to use" },
    {GAMEFOLDER_TOINSTALLNTR, "GAMEFOLDER_TOINSTALLNTR" , "to install the game?" },
    {GAMEFOLDER_USING, "GAMEFOLDER_USING"               , "Using" },
    {GAMEFOLDER_TOINSTALL, "GAMEFOLDER_TOINSTALL"       , "to install the game" },

    //DRAW SCREEN1
    { DRAWSCREEN_FAVSWAP, "DRAWSCREEN_FAVSWAP"           , "Favorites Swap" },
    { DRAWSCREEN_FAVINSERT, "DRAWSCREEN_FAVINSERT"       , "Favorites Insert" },
    { DRAWSCREEN_FAVORITES, "DRAWSCREEN_FAVORITES"       , "Favorites" },
    { DRAWSCREEN_PAGE, "DRAWSCREEN_PAGE"                 , "Page" },
    { DRAWSCREEN_GAMES,"DRAWSCREEN_GAMES"                , "Games" },
    { DRAWSCREEN_PLAY, "DRAWSCREEN_PLAY"                 , "Play" },
    { DRAWSCREEN_SOPTIONS, "DRAWSCREEN_SOPTIONS"         , "TRIANGLE: Game Options" },
    { DRAWSCREEN_SDELETE, "DRAWSCREEN_SDELETE"           , "TRIANGLE: Delete Favorite" },
    { DRAWSCREEN_STGLOPT, "DRAWSCREEN_STGLOPT"           , "START: Global Options" },
    { DRAWSCREEN_EXITXMB, "DRAWSCREEN_EXITXMB"           , "Exit to XMB?" },
    { DRAWSCREEN_RESTART, "DRAWSCREEN_RESTART"           , "Do you want to Restart the PS3?"},
    { DRAWSCREEN_SHUTDOWN, "DRAWSCREEN_SHUTDOWN"         , "Do you want to turn off the PS3?"},
    { DRAWSCREEN_CANRUNFAV, "DRAWSCREEN_CANRUNFAV"       , "Cannot run this favorite" },
    { DRAWSCREEN_MARKNOTEXEC, "DRAWSCREEN_MARKNOTEXEC"   , "Marked as non executable. Trying to install in HDD0 cache" },
    { DRAWSCREEN_MARKNOTEX4G, "DRAWSCREEN_MARKNOTEX4G"   , "Marked as not executable - Contains splited files >= 4GB" },
    { DRAWSCREEN_GAMEINOFMNT, "DRAWSCREEN_GAMEINOFMNT"   , "I cannot find one folder to mount /dev_hdd0/game from USB" },
    { DRAWSCREEN_GAMEIASKDIR, "DRAWSCREEN_GAMEIASKDIR"   , "Do you want to create in /dev_usb00" },
    { DRAWSCREEN_GAMEICANTFD, "DRAWSCREEN_GAMEICANTFD"   , "I cannot find an USB device to mount /dev_hdd0/game from USB" },
    { DRAWSCREEN_GAMEIWLAUNCH, "DRAWSCREEN_GAMEIWLAUNCH" , "Do you want to launch the Game?" },
    { DRAWSCREEN_EXTEXENOTFND, "DRAWSCREEN_EXTEXENOTFND" , "external executable not found" },
    { DRAWSCREEN_EXTEXENOTCPY, "DRAWSCREEN_EXTEXENOTCPY" , "Use 'Copy EBOOT.BIN from USB' to import them." },
    { DRAWSCREEN_REQBR, "DRAWSCREEN_REQBR"               , "Required BR-Disc, Retry?" },
    { DRAWSCREEN_PARCONTROL, "DRAWSCREEN_PARCONTROL"     , "Locked by Parental Control"},

    //DRAW OPTIONS
    { DRAWGMOPT_OPTS, "DRAWGMOPT_OPTS"                 , "Options" },
    { DRAWGMOPT_CFGGAME, "DRAWGMOPT_CFGGAME"           , "Config. Game" },
    { DRAWGMOPT_CPYGAME, "DRAWGMOPT_CPYGAME"           , "Copy Game" },
    { DRAWGMOPT_DELGAME, "DRAWGMOPT_DELGAME"           , "Delete Game" },
    { DRAWGMOPT_FIXGAME, "DRAWGMOPT_FIXGAME"           , "Fix File Permissions" },
    { DRAWGMOPT_TSTGAME, "DRAWGMOPT_TSTGAME"           , "Test Game" },
    { DRAWGMOPT_CPYEBOOTGAME, "DRAWGMOPT_CPYEBOOTGAME" , "Copy EBOOT.BIN from USB" },
    { DRAWGMOPT_CPYTOFAV, "DRAWGMOPT_CPYTOFAV"         , "Copy to Favourites" },
    { DRAWGMOPT_DELFMFAV, "DRAWGMOPT_DELFMFAV"         , "Delete from Favourites" },
    { DRAWGMOPT_UNMTDEV, "DRAWGMOPT_UNMTDEV"           , "Unmount USB00%i Device" },

    { DRAWGMOPT_EXTRACTISO, "DRAWGMOPT_EXTRACTISO" , "Extract ISO" },
    { DRAWGMOPT_BUILDISO, "DRAWGMOPT_BUILDISO"     , "Build ISO" },
    { DRAWGMOPT_MKISO, "DRAWGMOPT_MKISO"           , "Select a device to Build the ISO" },
    { DRAWGMOPT_XTISO, "DRAWGMOPT_XTISO"           , "Select a device to Extract the ISO" },
    { DRAWGMOPT_CPYISO, "DRAWGMOPT_CPYISO"         , "Select a device to Copy the ISO" },

    { DRAWGMOPT_FIXCOMPLETE, "DRAWGMOPT_FIXCOMPLETE" , "Fix Permissions Done!" },
    { DRAWGMOPT_CPYOK, "DRAWGMOPT_CPYOK"             , "copied successfully" },
    { DRAWGMOPT_CPYERR, "DRAWGMOPT_CPYERR"           , "Error copying" },
    { DRAWGMOPT_CPYNOTFND, "DRAWGMOPT_CPYNOTFND"     , "not found" },
    { DRAWGMOPT_GAMEUPDATE, "DRAWGMOPT_GAMEUPDATE"   , "Game Update" },

    //DRAW_PSX
    { DRAWPSX_EMULATOR,  "DRAWPSX_EMULATOR"   , "Emulator" },
    { DRAWPSX_VIDEOPS,   "DRAWPSX_VIDEOPS"    , "PSX Video Options" },
    { DRAWPSX_SAVEASK,   "DRAWPSX_SAVEASK"    , "Save PSX options?" },
    { DRAWPSX_SAVED,     "DRAWPSX_SAVED"      , "PSX Options Saved" },
    { DRAWPSX_VIDEOTHER, "DRAWPSX_VIDEOTHER"  , "Video / Others" },
    { DRAWPSX_VIDEOMODE, "DRAWPSX_VIDEOMODE"  , "Video Mode" },
    { DRAWPSX_VIDEOASP,  "DRAWPSX_VIDEOASP"   , "Video Aspect (480/576)" },
    { DRAWPSX_FULLSCR,   "DRAWPSX_FULLSCR"    , "Full Screen" },
    { DRAWPSX_SMOOTH,    "DRAWPSX_SMOOTH"     , "Smoothing" },
    { DRAWPSX_EXTROM,    "DRAWPSX_EXTROM"     , "External ROM" },
    { DRAWPSX_FORMAT,    "DRAWPSX_FORMAT"     , "Format Internal_MC" },
    { DRAWPSX_ASKFORMAT, "DRAWPSX_ASKFORMAT"  , "Do you want you format Internal_MC?\n\nYou will LOSE the saves in this operation" },
    { DRAWPSX_ERRWRITING,"DRAWPSX_ERRWRITING" , "Error writing the file (Device full?)" },

    { DRAWPSX_BUILDISO,  "DRAWPSX_BUILDISO"   , "Building custom ISO..." },
    { DRAWPSX_ASKCHEATS, "DRAWPSX_ASKCHEATS"  , "PSX Cheat disc found\n\nDo you want to use it?" },
    { DRAWPSX_ERRCHEATS, "DRAWPSX_ERRCHEATS"  , "PSX Disc for Cheats cannot be launched\n\nwithout a PSX game" },
    { DRAWPSX_ERRSECSIZE,"DRAWPSX_ERRSECSIZE" , "Error: Different sector size in ISO files" },
    { DRAWPSX_ERRUNKSIZE,"DRAWPSX_ERRUNKSIZE" , "Error: Unknown Sector Size" },
    { DRAWPSX_DISCEJECT ,"DRAWPSX_DISCEJECT"  , "PSX CD Ejected" },
    { DRAWPSX_DISCORDER , "DRAWPSX_DISCORDER" , "Select Disc Order" },
    { DRAWPSX_PRESSOB   ,"DRAWPSX_PRESSOB"    , "Press CIRCLE to change the order" },
    { DRAWPSX_PRESSXB   ,"DRAWPSX_PRESSXB"    , "Press CROSS to launch the game" },
    { DRAWPSX_CHEATMAKE ,"DRAWPSX_CHEATMAKE"  , "PSX Cheat disc found, but different sector size\n\nDo you want to build one compatible?" },
    { DRAWPSX_COPYMC    ,"DRAWPSX_COPYMC"     , "Copying Memory Card to HDD0 device..." },
    { DRAWPSX_ERRCOPYMC ,"DRAWPSX_ERRCOPYMC"  , "Error copying the Memory Card to HDD0 device" },
    { DRAWPSX_PUTFNAME  ,"DRAWPSX_PUTFNAME"   , "Put a Folder Name:" },
    { DRAWPSX_FMUSTB    ,"DRAWPSX_FMUSTB"     , "Folder Name must be >=3 chars" },
    { DRAWPSX_PUTADISC  ,"DRAWPSX_PUTADISC"   , "Put a PSX disc and press YES to continue or NO to abort\n\nDisc to copy: " },
    { DRAWPSX_UNREC     ,"DRAWPSX_UNREC"      , "Unrecognized disc" },
    { DRAWPSX_ERROPENING,"DRAWPSX_ERROPENING" , "Error opening BDVD drive" },
    { DRAWPSX_ASKEFOLDER,"DRAWPSX_ASKEFOLDER" , "Folder Exits\n\nContinue?" },
    { DRAWPSX_ISOEXITS  ,"DRAWPSX_ISOEXITS"   , "exists\n\nSkip?" },

    //DRAW CONFIGS
    { DRAWGMCFG_CFGS, "DRAWGMCFG_CFGS"               , "Config. Game" },
    { DRAWGMCFG_DSK, "DRAWGMCFG_DSK"                 , "Requires Disc" },
    { DRAWGMCFG_NO, "DRAWGMCFG_NO"                   , "No" },
    { DRAWGMCFG_YES,"DRAWGMCFG_YES"                  , "Yes" },
    { DRAWGMCFG_UPD, "DRAWGMCFG_UPD"                 , "Online Updates" },
    { DRAWGMCFG_ON, "DRAWGMCFG_ON"                   , "On" },
    { DRAWGMCFG_OFF, "DRAWGMCFG_OFF"                 , "Off" },
    { DRAWGMCFG_EXTBOOT, "DRAWGMCFG_EXTBOOT"         , "Extern EBOOT.BIN" },
    { DRAWGMCFG_BDEMU, "DRAWGMCFG_BDEMU"             , "BD Emu (for USB)" },
    { DRAWGMCFG_EXTHDD0GAME, "DRAWGMCFG_EXTHDD0GAME" , "Ext /dev_hdd0/game" },
    { DRAWGMCFG_SAVECFG, "DRAWGMCFG_SAVECFG"         , "Save Config" },
    { DRAWGMCFG_DIRECTBOOT, "DRAWGMCFG_DIRECTBOOT"   , "Direct Boot" },

    //DRAW GLOBAL OPTIONS
    { DRAWGLOPT_OPTS, "DRAWGLOPT_OPTS"               , "Global Options" },
    { DRAWGLOPT_SCRADJUST, "DRAWGLOPT_SCRADJUST"     , "Video Adjust" },
    { DRAWGLOPT_CHANGEGUI, "DRAWGLOPT_CHANGEGUI"     , "Change Current GUI" },
    { DRAWGLOPT_CHANGEBCK, "DRAWGLOPT_CHANGEBCK"     , "Change Background Color" },
    { DRAWGLOPT_BCKPICNONE, "DRAWGLOPT_BCKPICNONE"	 , "Background Picture : None" },
    { DRAWGLOPT_BCKPICRAND, "DRAWGLOPT_BCKPICRAND"	 , "Background Picture : Random" },
    { DRAWGLOPT_BCKPIC1, "DRAWGLOPT_BCKPIC1"		 , "Background Picture : PIC1.PNG" },
    { DRAWGLOPT_BCKPICT, "DRAWGLOPT_BCKPICT"		 , "Background Picture :" },
    { DRAWGLOPT_CHANGEDIR, "DRAWGLOPT_CHANGEDIR"     , "Change Game Directory" },
    { DRAWGLOPT_SWMUSICOFF, "DRAWGLOPT_SWMUSICOFF"   , "Switch Music Off" },
    { DRAWGLOPT_SWMUSICON, "DRAWGLOPT_SWMUSICON"     , "Switch Music On" },
    { DRAWGLOPT_INITFTP, "DRAWGLOPT_INITFTP"         , "Initialize FTP server" },
    { DRAWGLOPT_TOOLS, "DRAWGLOPT_TOOLS"             , "Tools" },
    { DRAWGLOPT_REFRESH, "DRAWGLOPT_REFRESH"         , "Game List" },
    { DRAWGLOPT_CREDITS, "DRAWGLOPT_CREDITS"         , "Credits" },
    { DRAWGLOPT_FTPINITED, "DRAWGLOPT_FTPINITED"     , "Server FTP initialized\nDo you want auto-enable FTP service on init?" },
    { DRAWGLOPT_FTPARINITED, "DRAWGLOPT_FTPARINITED" , "Server FTP already initialized" },
    { DRAWGLOPT_FTPSTOPED, "DRAWGLOPT_FTPSTOPED"     , "Server FTP Stoped\nRemoved FTP service on init." },

    //DRAW TOOLS
    { DRAWTOOLS_TOOLS, "DRAWTOOLS_TOOLS"         , "Tools" },
    { DRAWTOOLS_DELCACHE, "DRAWTOOLS_DELCACHE"      , "Delete Cache Tool" },
   // { DRAWTOOLS_SECDISABLE, "DRAWTOOLS_SECDISABLE"    , "Press To Disable Syscall Security" },
   // { DRAWTOOLS_SECENABLE, "DRAWTOOLS_SECENABLE"     , "Press To Enable Syscall Security" },
    { DRAWTOOLS_LANGUAGE_1, "DRAWTOOLS_LANGUAGE_1"     , "English" },
    { DRAWTOOLS_LANGUAGE_2, "DRAWTOOLS_LANGUAGE_2"     , "Español" },
    { DRAWTOOLS_LANGUAGE_3, "DRAWTOOLS_LANGUAGE_3"     , "Français" },
    { DRAWTOOLS_LANGUAGE_4, "DRAWTOOLS_LANGUAGE_4"     , "Italiano" },
    { DRAWTOOLS_LANGUAGE_5, "DRAWTOOLS_LANGUAGE_5"     , "Norsk" },
    { DRAWTOOLS_LANGUAGE_6, "DRAWTOOLS_LANGUAGE_6"     , "Deutsch" },
    { DRAWTOOLS_LANGUAGE_7, "DRAWTOOLS_LANGUAGE_7"     , "Português" },
    { DRAWTOOLS_LANGUAGE_8, "DRAWTOOLS_LANGUAGE_8"     , "Persian"},
    { DRAWTOOLS_LANGUAGE_9, "DRAWTOOLS_LANGUAGE_9"     , "Chinese Simplified"},
    { DRAWTOOLS_LANGUAGE_10, "DRAWTOOLS_LANGUAGE_10"   , "Chinese Traditional"},
    { DRAWTOOLS_LANGUAGE_11, "DRAWTOOLS_LANGUAGE_11"   , "Ελληνικός"},
    { DRAWTOOLS_LANGUAGE_12, "DRAWTOOLS_LANGUAGE_12"   , "Suomi"},
    { DRAWTOOLS_LANGUAGE_13, "DRAWTOOLS_LANGUAGE_13"   , "한국어"},
    { DRAWTOOLS_LANGUAGE_14, "DRAWTOOLS_LANGUAGE_14"   , "Dansk"},
    { DRAWTOOLS_LANGUAGE_15, "DRAWTOOLS_LANGUAGE_15"   , "Polski"},
    { DRAWTOOLS_LANGUAGE_16, "DRAWTOOLS_LANGUAGE_16"   , "Custom (from file)"},

    { DRAWTOOLS_COPYFROM, "DRAWTOOLS_COPYFROM"     , "Copy from /dev_usb/iris to Iris folder"},
    { DRAWTOOLS_WITHBDVD, "DRAWTOOLS_WITHBDVD"     , "With BDVD Controller"},
    { DRAWTOOLS_NOBDVD,   "DRAWTOOLS_NOBDVD"       , "Without BDVD Device"},
    { DRAWTOOLS_NOBDVD2,  "DRAWTOOLS_NOBDVD2"      , "Cobra / Disc-less payload"},


    { DRAWTOOLS_PKGTOOLS, "DRAWTOOLS_PKGTOOLS"     , ".PKG Install" },
    { DRAWTOOLS_ARCHIVEMAN, "DRAWTOOLS_ARCHIVEMAN" , "File Manager" },
    { DRAWTOOLS_COVERSDOWN, "DRAWTOOLS_COVERSDOWN" , "Covers Download" },

    //MAIN - OTHERS
    { DRAWCACHE_CACHE, "DRAWCACHE_CACHE"           , "Delete Cache Tool" },
    { DRAWCACHE_ERRNEEDIT, "DRAWCACHE_ERRNEEDIT"   , "You need %1.2f GB free to install" },
    { DRAWCACHE_ASKTODEL, "DRAWCACHE_ASKTODEL"     , "Delete %s Cache?" },
    { PATCHBEMU_ERRNOUSB, "PATCHBEMU_ERRNOUSB"     , "BDEMU is only for USB devices" },
    { MOVEOBEMU_ERRSAVE, "MOVEOBEMU_ERRSAVE"       , "Error Saving:\n%s" },
    { MOVEOBEMU_ERRMOVE, "MOVEOBEMU_ERRMOVE"       , "Error Moving To:\n%s/PS3_GAME exists" },
    { MOVEOBEMU_MOUNTOK, "MOVEOBEMU_MOUNTOK"       , "BDEMU mounted in:\n%s/PS3_GAME" },
    { MOVETBEMU_ERRMOVE, "MOVETBEMU_ERRMOVE"       , "Error Moving To:\n%s exists" },

    //MAIN - GLOBAL
    { GLOBAL_RETURN, "GLOBAL_RETURN"          , "Return" },
    { GLOBAL_SAVED, "GLOBAL_SAVED"            , "File Saved" },


    //UTILS
    //FAST COPY ADD
    { FASTCPADD_FAILED, "FASTCPADD_FAILED"             , "Failed in fast_copy_process() ret" },
    { FASTCPADD_ERRTMFILES, "FASTCPADD_ERRTMFILES"     , "Too much files" },
    { FASTCPADD_FAILEDSTAT, "FASTCPADD_FAILEDSTAT"     , "Failed in stat()" },
    { FASTCPADD_ERROPEN, "FASTCPADD_ERROPEN"           , "Error Opening0 (write)" },
    { FASTCPADD_COPYING, "FASTCPADD_COPYING"           , "Copying" },
    { FASTCPADD_FAILFASTFILE, "FASTCPADD_FAILFASTFILE" , "Failed in fast_files(fast_num_files).mem" },

    //FAST COPY PROCESS
    { FASTCPPRC_JOINFILE, "FASTCPPRC_JOINFILE"      , "Joining file" },
    { FASTCPPRC_COPYFILE, "FASTCPPRC_COPYFILE"      , "Copying. File" },
    { FASTCPPTC_OPENERROR, "FASTCPPTC_OPENERROR"    , "Error!!!!!!!!!!!!!!!!!!!!!!!!!\nFiles Opened %i\n Waiting 20 seconds to display fatal error" },

    //GAME TEST FILES
    { GAMETESTS_FOUNDINSTALL, "GAMETESTS_FOUNDINSTALL" , "Found %s\n\nDo you want to install?" },
    { GAMETESTS_BIGFILE, "GAMETESTS_BIGFILE"           , "Big file" },
    { GAMETESTS_TESTFILE, "GAMETESTS_TESTFILE"         , "Test File" },
    { GAMETESTS_CHECKSIZE, "GAMETESTS_CHECKSIZE"       , "Checking Size of File" },

    //GAME DELETE FILES
    { GAMEDELFL_DELETED, "GAMEDELFL_DELETED"        , "Deleted" },
    { GAMEDELFL_DELETING, "GAMEDELFL_DELETING"      , "Deleting... File" },

    //GAME COPY
    { GAMECPYSL_GSIZEABCNTASK, "GAMECPYSL_GSIZEABCNTASK" , "Get Size: Aborted - Continue the copy?" },
    { GAMECPYSL_STARTED, "GAMECPYSL_STARTED"             , "Starting... \n copy" },
    { GAMECPYSL_SPLITEDHDDNFO, "GAMECPYSL_SPLITEDHDDNFO" , "%s\n\nSplit game copied in HDD0 (non bootable)" },
    { GAMECPYSL_SPLITEDUSBNFO, "GAMECPYSL_SPLITEDUSBNFO" , "%s\n\nSplit game copied in USB00%c (non bootable)" },
    { GAMECPYSL_DONE, "GAMECPYSL_DONE"                   , "Done! Files Copied" },
    { GAMECPYSL_FAILDELDUMP, "GAMECPYSL_FAILDELDUMP"     , "Delete failed dump in" },

    //GAME CACHE COPY
    { GAMECHCPY_ISNEEDONEFILE, "GAMECHCPY_ISNEEDONEFILE" , "Sorry, but you needs to install at least a bigfile" },
    { GAMECHCPY_NEEDMORESPACE, "GAMECHCPY_NEEDMORESPACE" , "You have %.2fGB free and you needs %.2fGB\n\nPlease, delete Cache Entries" },
    { GAMECHCPY_NOSPACE, "GAMECHCPY_NOSPACE"             , "Sorry, you have %.2fGB free\n\nand you needs %.2fGB" },
    { GAMECHCPY_CACHENFOSTART, "GAMECHCPY_CACHENFOSTART" , "Cache Files: %.2fGB - Total Files: %.2fGB\n you save %.2fGB on HDD0 (%.2fGB Total)\n\nPress any button to Start" },
    { GAMECHCPY_FAILDELFROM, "GAMECHCPY_FAILDELFROM"     , "Delete Cache failed dump from" },

    //GAME DELETE
    { GAMEDELSL_WANTDELETE, "GAMEDELSL_WANTDELETE" , "Do you want to delete from" },
    { GAMEDELSL_STARTED, "GAMEDELSL_STARTED"       , "Starting... \n delete" },
    { GAMEDELSL_DONE, "GAMEDELSL_DONE"             , "Done!  Files Deleted" },

    //GAME TEST
    // warning! don't translate GAMETSTSL_FINALNFO2 from english
    { GAMETSTSL_FINALNFO2, "GAMETSTSL_FINALNFO2"  , "Directories: %i Files: %i\nBig files: %i Split files: %i" },
    { GAMETSTSL_TESTED, "GAMETSTSL_TESTED"        , "Files Tested" },

    //GLOBAL UTILS
    { GLUTIL_SPLITFILE, "GLUTIL_SPLITFILE"           , "Split file" },
    { GLUTIL_WROTE, "GLUTIL_WROTE"                   , "Wrote" },
    { GLUTIL_TIME, "GLUTIL_TIME"                     , "Time" },
    { GLUTIL_TIMELEFT, "GLUTIL_TIMELEFT"             , "Time Left" },
    { GLUTIL_HOLDTRIANGLEAB, "GLUTIL_HOLDTRIANGLEAB" , "Hold TRIANGLE to Abort" },
    { GLUTIL_HOLDTRIANGLESK, "GLUTIL_HOLDTRIANGLESK" , "Hold TRIANGLE to Skip" },
    { GLUTIL_ABORTEDUSER, "GLUTIL_ABORTEDUSER"       , "Aborted by user" },
    { GLUTIL_ABORTED, "GLUTIL_ABORTED"               , "Aborted!!!" },
    { GLUTIL_XEXIT, "GLUTIL_XEXIT"                   , "Press CROSS to Exit" },
    { GLUTIL_WANTCPYFROM, "GLUTIL_WANTCPYFROM"       , "Do you want to copy from" },
    { GLUTIL_WTO, "GLUTIL_WTO"                       , "to" },

    // INSTALL .PKG
    { PKG_HEADER, "PKG_HEADER", "Install .PKG Utility -     Use CROSS to select and CIRCLE to exit"},
    { PKG_INSERTUSB, "PKG_INSERTUSB", "Insert the USB mass storage device"},
    { PKG_ERRTOBIG, "PKG_ERRTOBIG", ".PKG size too big or disk space small"},
    { PKG_WANTINSTALL, "PKG_WANTINSTALL", "Do you want to Install this .PKG file?"},
    { PKG_ERRALREADY, "ERRALREADY", "Error: .PKG already in the stack"},
    { PKG_ERRFULLSTACK, "PKG_ERRFULLSTACK", "Error: stack is full (max 16 entries)"},
    { PKG_ERRBUILD, "PKG_ERRBUILD", "Error Building .PKG process"},
    { PKG_COPYING, "PKG_COPYING", "Copying .PKG file to Iris Manager folder..."},
    { PKG_ERROPENING, "PKG_ERROPENING", "Error Opening .PKG file"},
    { PKG_ERRCREATING, "PKG_ERRCREATING", "Error Creating .PKG file"},
    { PKG_ERRREADING, "PKG_ERRREADING", "Error Reading .PKG file"},
    { PKG_ERRLICON, "PKG_ERRLICON", "Error Loading ICON file"},
    { PKG_ERRMOVING, "PKG_ERRMOVING", "Error moving .PKG"},

     // generic
    { OUT_OFMEMORY, "OUT_OFMEMORY", "Out of Memory"},
    { OPERATION_DONE, "OPERATION_DONE"     , "Done!" },
    { PLUG_STORAGE1, "PLUG_STORAGE1" ,
        "Remember you to plug an USB storage massive device to create the fake disc event\n\n"
        "Recuerda enchufar un dispositivo de almacenamiento masivo para crear el evento del falso disco" },
    { PLUG_STORAGE2, "PLUG_STORAGE2" , "Fake Disc Inserted\n\nFalso Disco Insertado" },


    //GAME LIST
    { GAMELIST_ALLGAMES,  "GAMELIST_ALLGAMES"  , "All Games" },
    { GAMELIST_PS3GAMES,  "GAMELIST_PS3GAMES"  , "PS3 Games" },
    { GAMELIST_VIDEOS,    "GAMELIST_VIDEOS"    , "Videos" },
    { GAMELIST_NETGAMES,  "GAMELIST_NETGAMES"  , "Network Games (via webMAN)" },
    { GAMELIST_STATUS1,   "GAMELIST_STATUS1"   , "SQUARE  All Retro games       <  >  Select Retro mode       X  Select mode" },
    { GAMELIST_STATUS2,   "GAMELIST_STATUS2"   , "X  Select Game List mode             O  Go Back" },
    { GAMELIST_SCANNING0, "GAMELIST_SCANNING0" , "Scanning all games..." },
    { GAMELIST_SCANNING1, "GAMELIST_SCANNING1" , "Scanning PS3 games..." },
    { GAMELIST_SCANNING2, "GAMELIST_SCANNING2" , "Scanning PlayStation® games..." },
    { GAMELIST_SCANNING3, "GAMELIST_SCANNING3" , "Scanning Videos..." },
    { GAMELIST_SCANNING4, "GAMELIST_SCANNING4" , "Scanning Retro games..." },
    { GAMELIST_SCANNING5, "GAMELIST_SCANNING5" , "Scanning network games (webMAN)..." },


    { LANGSTRINGS_COUNT, "", ""}
};

char * language[LANGSTRINGS_COUNT];

int reverse_language = 0;

#if defined(LOADER_MODE) || defined(LASTPLAY_LOADER)
void open_language(int lang, char * filename) {}
int get_system_language(void) {return 0;}
void close_language(void) {}
#else
static int lang_inited = 0;

void open_language(int lang, char * filename)
{

    int n;//, version;
    struct stat s;

    int elements = sizeof(lang_strings) / sizeof(t_lngstr);

    char get_reverse[64] = "";

    reverse_language = 0;

    for (n = 0; n < LANGSTRINGS_COUNT; n++)
    {
        if(!lang_inited)
            language[n] = (char*) malloc(MAX_CFGLINE_LEN);
        strncpy(language[n], "***ERROR****", MAX_CFGLINE_LEN - 1);
    }

    lang_inited = 1;

    char * file_external = NULL;
    int file_size = 0;

    if(lang >= LANGCOUNT) { // custom language (external filename)
        if(!stat(filename, &s))
            file_external = LoadFile(filename, &file_size);

        if(!file_external) lang = 0;

        getConfigMemValueString((char *) file_external, file_size, "Language", "REVERSE", get_reverse, 63, "OFF");
        if(!strcasecmp((const char *) get_reverse, "on")) reverse_language = 1;

        for (n = 0; n < elements; n++)
        {
            if(lang_strings[n].code == LANGSTRINGS_COUNT) break;

            // from external file
            strncpy(language[lang_strings[n].code], lang_strings[n].strdefault, MAX_CFGLINE_LEN - 1);
            getConfigMemValueString((char *) file_external, file_size, "Language",
                lang_strings[n].strname, language[lang_strings[n].code], MAX_CFGLINE_LEN - 1, lang_strings[n].strdefault);
        }

    }
    else
    {
        char *file_bin = (char *) language_ini_en_bin;
        file_size = language_ini_en_bin_size;

        switch(lang)
        {
            case 1: // sp - DRAWTOOLS_LANGUAGE_2: Español
                file_bin = (char *) language_ini_sp_bin;
                file_size = language_ini_sp_bin_size;
                break;
            case 2: // fr - DRAWTOOLS_LANGUAGE_3: Français
                file_bin = (char *) language_ini_fr_bin;
                file_size = language_ini_fr_bin_size;
                break;
            case 3: // it - DRAWTOOLS_LANGUAGE_4: Italiano
                file_bin = (char *) language_ini_it_bin;
                file_size = language_ini_it_bin_size;
                break;
            case 4: // nw - DRAWTOOLS_LANGUAGE_5: Norsk
                file_bin = (char *) language_ini_nw_bin;
                file_size = language_ini_nw_bin_size;
                break;
            case 5: // gm - DRAWTOOLS_LANGUAGE_6: Deutsch
                file_bin = (char *) language_ini_gm_bin;
                file_size = language_ini_gm_bin_size;
                break;
            case 6: // por - DRAWTOOLS_LANGUAGE_7: Português
                file_bin = (char *) language_ini_por_bin;
                file_size = language_ini_por_bin_size;
                break;
            case 7: // ps - DRAWTOOLS_LANGUAGE_8: Persian
                file_bin = (char *) language_ini_ps_bin;
                file_size = language_ini_ps_bin_size;
                break;
            case 8: // chs - DRAWTOOLS_LANGUAGE_9: Chinese Simplified
                file_bin = (char *) language_ini_chs_bin;
                file_size = language_ini_chs_bin_size;
                break;
            case 9: // cht - DRAWTOOLS_LANGUAGE_10: Chinese Traditional
                file_bin = (char *) language_ini_cht_bin;
                file_size = language_ini_cht_bin_size;
                break;
            case 10: // gr - DRAWTOOLS_LANGUAGE_11: Ελληνικός
                file_bin = (char *) language_ini_gr_bin;
                file_size = language_ini_gr_bin_size;
                break;
            case 11: // fi - DRAWTOOLS_LANGUAGE_12: Suomi
                file_bin = (char *) language_ini_fi_bin;
                file_size = language_ini_fi_bin_size;
                break;
            case 12: // kr - DRAWTOOLS_LANGUAGE_12: 한국어
                file_bin = (char *) language_ini_kr_bin;
                file_size = language_ini_kr_bin_size;
                break;
            case 13: // da - DRAWTOOLS_LANGUAGE_14: Dansk
                file_bin = (char *) language_ini_da_bin;
                file_size = language_ini_da_bin_size;
                break;
            case 14: // da - DRAWTOOLS_LANGUAGE_15: Polski
                file_bin = (char *) language_ini_pl_bin;
                file_size = language_ini_da_bin_size;
                break;
            default: // en - DRAWTOOLS_LANGUAGE_1
                file_bin = (char *) language_ini_en_bin;
                file_size = language_ini_en_bin_size;
                break;
        }

        getConfigMemValueString((char *) file_bin, file_size, "Language", "REVERSE", get_reverse, 63, "OFF");
        if(!strcasecmp((const char *) get_reverse, "on")) reverse_language = 1;

        for (n = 0; n < elements; n++)
        {
            if(lang_strings[n].code == LANGSTRINGS_COUNT) break;

            // from external file
            getConfigMemValueString((char *) file_bin, file_size, "Language",
                    lang_strings[n].strname, language[lang_strings[n].code], MAX_CFGLINE_LEN - 1, lang_strings[n].strdefault);
        }
    }

    if(file_external) free(file_external);

    return;
}

int get_system_language(void)
{
    s32 ret = - 1;
    int reg = -1;
    u32 val_lang = 0x66;
    int ret_lang = 0;

    ret = sysFsOpen( "/dev_flash2/etc/xRegistry.sys", SYS_O_RDONLY, &reg, NULL, 0);
    if( ret == 0 )
    {
        int entry_name = 0x10002;

        while(true)
        {
            u64 pos;
            sysFsLseek( reg, (s64)entry_name, 0, &pos );

            //Leggo offset stringa
            u16 off_string = 0;
            u64 read;
            sysFsRead( reg, &off_string, 2, &read );

            //Leggo lunghezza stringa
            u16 len_string = 0;
            sysFsLseek( reg, (s64)off_string + (s64)0x12, 0, &pos );
            sysFsRead( reg, &len_string, 2, &read );

            //Avanzo di uno e leggo stringa
            char string[ 256 ];
            memset( string, 0, sizeof(string) );
            sysFsLseek( reg, (s64)off_string + (s64)0x15, 0, &pos );
            sysFsRead( reg, string, len_string, &read );

            //Comparo stringa
            if( strcmp(string, "/setting/system/language") == 0 )
            {
                sysFsLseek( reg, (s64)entry_name + (s64)0x7, 0, &pos );
                sysFsRead( reg, &val_lang, 4, &read );
                break;
            }

            //Non è uguale, avanzo l'offset da entry_name di 4 e leggo lunghezza data
            u16 len_data = 0;
            entry_name += 4;
            sysFsLseek( reg, (s64)entry_name, 0, &pos );
            sysFsRead( reg, &len_data, 2, &read );

            //Vado al prossimo offset stringa
            entry_name = entry_name + 6 + len_data;

            if( off_string == 0xCCDD ) break;
        }
    }

    switch( val_lang )
    {
        case 0x0:
        //  strcpy( lang, "ger_language.ini" );
            ret_lang = 5;
            break;
        case 0x1:
        //  strcpy( lang, "eng-us_language.ini" );
            ret_lang = 0;
            break;
        case 0x2:
        //  strcpy( lang, "spa_language.ini" );
            ret_lang = 1;
            break;
        case 0x3:
        //  strcpy( lang, "fre_language.ini" );
            ret_lang = 2;
            break;
        case 0x4:
        //  strcpy( lang, "ita_language.ini" );
            ret_lang = 3;
            break;
        case 0x5:
        //  strcpy( lang, "dut_language.ini" ); //Olandese
            ret_lang = 0;
            break;
        case 0x6:
        //  strcpy( lang, "por-por_language.ini" );
            ret_lang = 6;
            break;
        case 0x7:
        //  strcpy( lang, "rus_language.ini" );
            ret_lang = 0;
            break;
        case 0x8:
        //  strcpy( lang, "jap_language.ini" );
            ret_lang = 0;
            break;
        case 0x9:
        //  strcpy( lang, "kor_language.ini" );
            ret_lang = 12;
            break;
        case 0xA:
        //  strcpy( lang, "chi-tra_language.ini" );
            ret_lang = 9;
            break;
        case 0xB:
        //  strcpy( lang, "chi-sim_language.ini" );
            ret_lang = 8;
            break;
        case 0xC:
        //  strcpy( lang, "fin_language.ini" );
            ret_lang = 11;
            break;
        case 0xD:
        //  strcpy( lang, "swe_language.ini" );
            ret_lang = 0;
            break;
        case 0xE:
        //  strcpy( lang, "dan_language.ini" );
            ret_lang = 13;
            break;
        case 0xF:
        //  strcpy( lang, "nor_language.ini" );
            ret_lang = 4;
            break;
        case 0x10:
        //  strcpy( lang, "pol_language.ini" );
            ret_lang = 0;
            break;
        case 0x11:
        //  strcpy( lang, "por-bra_language.ini" );
            ret_lang = 6;
            break;
        case 0x12:
        //  strcpy( lang, "eng-uk_language.ini" );
            ret_lang = 0;
            break;
        default:
        //  strcpy( lang, "language.ini" );
            ret_lang = 0;
            break;
    }
    sysFsClose( reg );

    return ret_lang;
}

void close_language(void)
{
    int n;

    if(!lang_inited) return;

    //free memory
    for (n = 0; n < LANGSTRINGS_COUNT; n++)
    {
       free(language[n]);
    }

    lang_inited = 0;
}
#endif