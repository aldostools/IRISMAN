#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <tiny3d.h>
#include "libfont2.h"

#include "gfx.h"
#include "utils.h"
#include "main.h"

#include <sysutil/osk.h>
#include "sysutil/sysutil.h"
#include <sys/memory.h>
#include <ppu-lv2.h>

#include "sysregistry.h"
#include "file_manager.h"

int osk_action = 0;

#define OSKDIALOG_FINISHED          0x503
#define OSKDIALOG_UNLOADED          0x504
#define OSKDIALOG_INPUT_ENTERED     0x505
#define OSKDIALOG_INPUT_CANCELED    0x506

volatile int osk_event = 0;
volatile bool osk_unloaded = false;

static sys_mem_container_t container_mem;

static oskCallbackReturnParam OutputReturnedParam;
static oskParam DialogOskParam;
static oskInputFieldInfo inputFieldInfo;

static void my_eventHandle(u64 status, u64 param, void * userdata)
{
    switch((u32) status)
    {
        case OSKDIALOG_INPUT_CANCELED:
            osk_event = OSKDIALOG_INPUT_CANCELED;
            break;

        case OSKDIALOG_UNLOADED:
            osk_unloaded = true;
            break;

        case OSKDIALOG_INPUT_ENTERED:
            osk_event = OSKDIALOG_INPUT_ENTERED;
            break;

        case OSKDIALOG_FINISHED:
            osk_event = OSKDIALOG_FINISHED;
            break;

        default:
            break;
    }
}

void UTF32_to_UTF8(u32 *stw, u8 *stb)
{
  u16 h[2];

    while(stw[0])
    {
        if((stw[0] & 0xFF80) == 0) {
            *(stb++) = stw[0] & 0xFF;   // utf8 0xxxxxxx
        }
        else if((stw[0] & 0xF800) == 0)
        {
            // utf8 110yyyyy 10xxxxxx
            *(stb++) = ((stw[0]>>6) & 0xFF) | 0xC0; *(stb++) = (stw[0] & 0x3F) | 0x80;
        }
        else if((stw[0] & 0xFFFF0000) != 0)
        {
            // utf16 110110ww wwzzzzyy 110111yy yyxxxxxx (wwww = uuuuu - 1)
            // utf8  1111000uu 10uuzzzz 10yyyyyy 10xxxxxx
            h[0] = stw[0]>>10;
            h[1] = stw[0] & 0x3ff;

            *(stb++)= (((h[0] + 64)>>8) & 0x3) | 0xF0; *(stb++)= (((h[0]>>2) + 16) & 0x3F) | 0x80;
            *(stb++)= ((h[0]>>4) & 0x30) | 0x80 | ((h[1]<<2) & 0xF); *(stb++)= (h[1] & 0x3F) | 0x80;
        }
        else
        {
            // utf8 1110zzzz 10yyyyyy 10xxxxxx
            *(stb++)= ((stw[0]>>12) & 0xF) | 0xE0; *(stb++)= ((stw[0]>>6) & 0x3F) | 0x80; *(stb++)= (stw[0] & 0x3F) | 0x80;
        }

        stw++;
    }

    *stb = 0;
}

void UTF16_to_UTF8(u16 *stw, u8 *stb)
{
    while(stw[0])
    {
        if((stw[0] & 0xFF80) == 0)
        {
            *(stb++) = stw[0] & 0xFF;   // utf16 00000000 0xxxxxxx utf8 0xxxxxxx
        }
        else if((stw[0] & 0xF800) == 0)
        {
            // utf16 00000yyy yyxxxxxx utf8 110yyyyy 10xxxxxx
            *(stb++) = ((stw[0]>>6) & 0xFF) | 0xC0; *(stb++) = (stw[0] & 0x3F) | 0x80;
        }
        else if((stw[0] & 0xFC00) == 0xD800 && (stw[1] & 0xFC00) == 0xDC00)
        {
            // utf16 110110ww wwzzzzyy 110111yy yyxxxxxx (wwww = uuuuu - 1)
            // utf8  1111000uu 10uuzzzz 10yyyyyy 10xxxxxx
            *(stb++)= (((stw[0] + 64)>>8) & 0x3) | 0xF0; *(stb++)= (((stw[0]>>2) + 16) & 0x3F) | 0x80;
            *(stb++)= ((stw[0]>>4) & 0x30) | 0x80 | ((stw[1]<<2) & 0xF); *(stb++)= (stw[1] & 0x3F) | 0x80;
            stw++;
        }
        else
        {
            // utf16 zzzzyyyy yyxxxxxx utf8 1110zzzz 10yyyyyy 10xxxxxx
            *(stb++)= ((stw[0]>>12) & 0xF) | 0xE0; *(stb++)= ((stw[0]>>6) & 0x3F) | 0x80; *(stb++)= (stw[0] & 0x3F) | 0x80;
        }

        stw++;
    }

    *stb = 0;
}

void UTF8_to_UTF16(u8 *stb, u16 *stw)
{
   int n, m;
   u32 UTF32;
   while(*stb)
   {
       if(*stb & 128)
       {
            m = 1;

            if((*stb & 0xf8) == 0xf0)
            {
                // 4 bytes
                UTF32 = (u32) (*(stb++) & 3);
                m = 3;
            }
            else if((*stb & 0xE0) == 0xE0)
            {
                // 3 bytes
                UTF32 = (u32) (*(stb++) & 0xf);
                m = 2;
            }
            else if((*stb & 0xE0) == 0xC0)
            {
                // 2 bytes
                UTF32 = (u32) (*(stb++) & 0x1f);
                m = 1;
            }
            else {stb++; continue;} // error!

            for(n = 0; n < m; n++)
            {
                if(!*stb) break; // error!

                if((*stb & 0xc0) != 0x80) break; // error!
                UTF32 = (UTF32 <<6) |((u32) (*(stb++) & 63));
            }

            if((n != m) && !*stb) break;

        } else UTF32 = (u32) *(stb++);

        if(UTF32<65536)
            *stw++= (u16) UTF32;
        else
        {
            //110110ww wwzzzzyy 110111yy yyxxxxxx
            *stw++= (((u16) (UTF32>>10)) & 0x3ff) | 0xD800;
            *stw++= (((u16) (UTF32)) & 0x3ff) | 0xDC00;
        }
   }

   *stw++ = 0;
}

/*
void UTF8_to_Ansi(char *utf8, char *ansi, int len)
{
u8 *ch= (u8 *) utf8;
u8 c;

    *ansi = 0;

    while(*ch!=0 && len>0){

        // 3, 4 bytes utf-8 code
        if(((*ch & 0xF1)==0xF0 || (*ch & 0xF0)==0xe0) && (*(ch+1) & 0xc0)==0x80){

        *ansi++=' '; // ignore
        len--;
        ch+=2+1*((*ch & 0xF1)==0xF0);

        }
        else
        // 2 bytes utf-8 code
        if((*ch & 0xE0)==0xc0 && (*(ch+1) & 0xc0)==0x80){

        c= (((*ch & 3)<<6) | (*(ch+1) & 63));

        *ansi++=c;
        len--;
        ch++;

        } else {

            if(*ch<32) *ch=32;
            *ansi++=*ch;
            len--;
        }

        ch++;
    }

    while(len>0) {
        *ansi++=0;
        len--;
    }
}

*/

static int osk_level = 0;

static void OSK_exit(void)
{
    if(osk_level == 2)
    {
        oskAbort();
        oskUnloadAsync(&OutputReturnedParam);
        osk_event  = 0;
        osk_action = FAILED;
    }

    if(osk_level >= 1)
    {
        sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
        sysMemContainerDestroy(container_mem);
    }
}

int Get_OSK_String(char *caption, char *str, int len)
{
    int ret = SUCCESS;

    u16 * message = NULL;
    u16 * OutWcharTex = NULL;
    u16 * InWcharTex = NULL;

    if(len > 256) len = 256;

    osk_level = 0;
    atexit(OSK_exit);

    int lang= sys_language;

    if(sysMemContainerCreate(&container_mem, 8 * 1024 * 1024) < 0) return FAILED;

    osk_level = 1;

    message = malloc(strlen(caption)*2+32);
    if(!message) goto end;

    OutWcharTex = malloc(0x420*2);
    if(!OutWcharTex) goto end;

    InWcharTex = malloc(0x420*2);
    if(!InWcharTex) goto end;

    //memset(message, 0, 64);
    UTF8_to_UTF16((u8 *) caption, (u16 *) message);
    UTF8_to_UTF16((u8 *) str, (u16 *) InWcharTex);

    inputFieldInfo.message =  (u16 *) message;
    inputFieldInfo.startText = (u16 *) InWcharTex;
    inputFieldInfo.maxLength = len;

    OutputReturnedParam.res = OSK_NO_TEXT; //OSK_OK;
    OutputReturnedParam.len = len;

    OutputReturnedParam.str = (u16 *) OutWcharTex;

    memset(OutWcharTex, 0, 1024);

    if(oskSetKeyLayoutOption (OSK_10KEY_PANEL | OSK_FULLKEY_PANEL)<0) {ret= -2; goto end;}

    DialogOskParam.firstViewPanel = OSK_PANEL_TYPE_ALPHABET_FULL_WIDTH;
    DialogOskParam.allowedPanels = (OSK_PANEL_TYPE_ALPHABET | OSK_PANEL_TYPE_NUMERAL);

    switch(lang)
    {
        case LANG_GERMAN:
            DialogOskParam.allowedPanels |=  OSK_PANEL_TYPE_GERMAN | OSK_PANEL_TYPE_ENGLISH  |
            OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_ENGLISH:
        case LANG_ENGLISHUK:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_SPANISH:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_SPANISH | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_FRENCH:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_FRENCH | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_ITALIAN:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_ITALIAN | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_DUTCH:
            DialogOskParam.allowedPanels |=  OSK_PANEL_TYPE_DUTCH | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_PORTUGUESE:
        case LANG_PORTUGUESEB:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_PORTUGUESE | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_RUSSIAN:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_RUSSIAN | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_JAPANESE:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_JAPANESE | OSK_PANEL_TYPE_ENGLISH;
            break;
        case LANG_KOREAN:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_KOREAN | OSK_PANEL_TYPE_ENGLISH;
            break;
        case LANG_CHINESE:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_TRADITIONAL_CHINESE | OSK_PANEL_TYPE_ENGLISH;
            break;
        case LANG_CHINESES:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_SIMPLIFIED_CHINESE | OSK_PANEL_TYPE_ENGLISH;
            break;
        case LANG_FINNISH:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_FINNISH | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_SPANISH |
            OSK_PANEL_TYPE_RUSSIAN | OSK_PANEL_TYPE_FRENCH | OSK_PANEL_TYPE_GERMAN;
            break;
        case LANG_SWEDISH:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_SWEDISH | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_DANISH:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_DANISH | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_NORWEGIAN:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_NORWEGIAN | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        case LANG_POLISH:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_LATIN | OSK_PANEL_TYPE_ENGLISH | OSK_PANEL_TYPE_JAPANESE;
            break;
        default:
            DialogOskParam.allowedPanels |= OSK_PANEL_TYPE_ENGLISH;
            break;
    }

    if(oskAddSupportLanguage (DialogOskParam.allowedPanels)<0) {ret= -3; goto end;}

    if(oskSetLayoutMode( OSK_LAYOUTMODE_HORIZONTAL_ALIGN_CENTER )<0) {ret= -4; goto end;}

    oskPoint pos = {0.0, 0.0};

    DialogOskParam.controlPoint = pos;
    DialogOskParam.prohibitFlags = OSK_PROHIBIT_RETURN;
    if(oskSetInitialInputDevice(OSK_DEVICE_PAD)<0) {ret= -5; goto end;}

    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, my_eventHandle, NULL);

    osk_action = 0;
    osk_unloaded = false;

    if(oskLoadAsync(container_mem, (const void *) &DialogOskParam, (const void *)  &inputFieldInfo)<0) {ret= -6; goto end;}

    osk_level = 2;

    while(!osk_unloaded)
    {
        if(len == 250)
            draw_hex_editor();
        else
            draw_file_manager();
/*
        else
        {
            float x = 28, y = 0;

            cls();

            update_twat(true);


            SetCurrentFont(FONT_TTF);

            // header title

            DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

            SetFontColor(0xffffffff, 0x00000000);

            SetFontSize(18, 20);

            SetFontAutoCenter(0);

            DrawFormatString(x, y - 2, "%s", caption);

            y += 24;

            DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

            tiny3d_Flip();
            ps3pad_read();
        }
*/

        switch(osk_event)
        {
            case OSKDIALOG_INPUT_ENTERED:
                oskGetInputText(&OutputReturnedParam);
                osk_event = 0;
                break;

            case OSKDIALOG_INPUT_CANCELED:
                oskAbort();
                oskUnloadAsync(&OutputReturnedParam);
                osk_event  = 0;
                osk_action = FAILED;
                break;

            case OSKDIALOG_FINISHED:
                if(osk_action != FAILED) osk_action = 1;
                oskUnloadAsync(&OutputReturnedParam);
                osk_event = 0;
                break;

            default:
                break;
        }

    }

    usleep(150000); // unnecessary but...

    if(OutputReturnedParam.res == OSK_OK && osk_action == 1)
        UTF16_to_UTF8((u16 *) OutWcharTex, (u8 *) str);
    else
        ret= -666;

end:

    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    sysMemContainerDestroy(container_mem);

    osk_level = 0;
    if(message) free(message);
    if(OutWcharTex) free(OutWcharTex);
    if(InWcharTex) free(InWcharTex);

    return ret;
}

int Get_OSK_String_no_lang(char *caption, char *str, int len)
{

    int ret = SUCCESS;

    u16 * message = NULL;
    u16 * OutWcharTex = NULL;
    u16 * InWcharTex = NULL;

    if(len > 256) len = 256;

    osk_level = 0;
    atexit(OSK_exit);

    if(sysMemContainerCreate(&container_mem, 8*1024*1024) < 0) return FAILED;

    osk_level = 1;

    message = malloc(strlen(caption)*2+32);
    if(!message) goto end;

    OutWcharTex = malloc(0x420*2);
    if(!OutWcharTex) goto end;

    InWcharTex = malloc(0x420*2);
    if(!InWcharTex) goto end;

    //memset(message, 0, 64);
    UTF8_to_UTF16((u8 *) caption, (u16 *) message);
    UTF8_to_UTF16((u8 *) str, (u16 *) InWcharTex);

    inputFieldInfo.message =  (u16 *) message;
    inputFieldInfo.startText = (u16 *) InWcharTex;
    inputFieldInfo.maxLength = len;

    OutputReturnedParam.res = OSK_NO_TEXT; //OSK_OK;
    OutputReturnedParam.len = len;

    OutputReturnedParam.str = (u16 *) OutWcharTex;

    memset(OutWcharTex, 0, 1024);

    if(oskSetKeyLayoutOption (OSK_10KEY_PANEL | OSK_FULLKEY_PANEL)<0) {ret= -2; goto end;}

    DialogOskParam.firstViewPanel = OSK_PANEL_TYPE_ALPHABET_FULL_WIDTH;
    DialogOskParam.allowedPanels = (OSK_PANEL_TYPE_ALPHABET | OSK_PANEL_TYPE_NUMERAL);

    if(oskAddSupportLanguage ( OSK_PANEL_TYPE_ALPHABET )<0) {ret= -3; goto end;}

    if(oskSetLayoutMode( OSK_LAYOUTMODE_HORIZONTAL_ALIGN_CENTER )<0) {ret= -4; goto end;}

    oskPoint pos = {0.0, 0.0};

    DialogOskParam.controlPoint = pos;
    DialogOskParam.prohibitFlags = OSK_PROHIBIT_RETURN;
    if(oskSetInitialInputDevice(OSK_DEVICE_PAD)<0) {ret= -5; goto end;}

    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, my_eventHandle, NULL);

    osk_action = 0;
    osk_unloaded = false;

    if(oskLoadAsync(container_mem, (const void *) &DialogOskParam, (const void *)  &inputFieldInfo)<0) {ret= -6; goto end;}

    osk_level = 2;

    while(!osk_unloaded)
    {
        float x = 28, y = 0;

        cls();

        update_twat(true);


        SetCurrentFont(FONT_TTF);

        // header title

        DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

        SetFontColor(0xffffffff, 0x00000000);

        SetFontSize(18, 20);

        SetFontAutoCenter(0);

        DrawFormatString(x, y - 2, "%s", caption);

        y += 24;

        DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

        tiny3d_Flip();
        ps3pad_read();

        switch(osk_event)
        {
            case OSKDIALOG_INPUT_ENTERED:
                oskGetInputText(&OutputReturnedParam);
                osk_event = 0;
                break;

            case OSKDIALOG_INPUT_CANCELED:
                oskAbort();
                oskUnloadAsync(&OutputReturnedParam);
                osk_event  = 0;
                osk_action = FAILED;
                break;

            case OSKDIALOG_FINISHED:
                if(osk_action != FAILED) osk_action = 1;
                oskUnloadAsync(&OutputReturnedParam);
                osk_event = 0;
                break;

            default:
                break;
        }

    }

    usleep(150000); // unnecessary but...

    if(OutputReturnedParam.res == OSK_OK && osk_action == 1)
        UTF16_to_UTF8((u16 *) OutWcharTex, (u8 *) str);
    else
        ret= -666;

end:

    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    sysMemContainerDestroy(container_mem);

    osk_level = 0;
    if(message) free(message);
    if(OutWcharTex) free(OutWcharTex);
    if(InWcharTex) free(InWcharTex);

    return ret;
}
