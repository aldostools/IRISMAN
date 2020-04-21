#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>

#include <sysutil/osk.h>
#include <sysutil/sysutil.h>
#include <sys/memory.h>
#include <ppu-lv2.h>

#include <tiny3d.h>
#include "libfont2.h"
#include "pad.h"

#include "util.h"

#define OSKDIALOG_FINISHED          0x503
#define OSKDIALOG_UNLOADED          0x504
#define OSKDIALOG_INPUT_ENTERED     0x505
#define OSKDIALOG_INPUT_CANCELED    0x506

#define SUCCESS 	1
#define FAILED	 	0

volatile int osk_event = 0;
volatile int osk_unloaded = 0;
int osk_action = SUCCESS;

static sys_mem_container_t container_mem;

static oskCallbackReturnParam OutputReturnedParam;
static oskParam DialogOskParam;
static oskInputFieldInfo inputFieldInfo;

extern void cls();
extern void Draw_scene();

static void my_eventHandle(u64 status, u64 param, void * userdata) {

    switch((u32) status) {

	case OSKDIALOG_INPUT_CANCELED:
		osk_event= OSKDIALOG_INPUT_CANCELED;
		break;

    case OSKDIALOG_UNLOADED:
		osk_unloaded= 1;
		break;

    case OSKDIALOG_INPUT_ENTERED:
		osk_event=OSKDIALOG_INPUT_ENTERED;
		break;

	case OSKDIALOG_FINISHED:
		osk_event=OSKDIALOG_FINISHED;
		break;

    default:
        break;
    }

	
}

static int osk_level = 0;

static void OSK_exit(void)
{
    if(osk_level == 2) {
        oskAbort();
        oskUnloadAsync(&OutputReturnedParam);
        
        osk_event = 0;
        osk_action=FAILED;
    }

    if(osk_level >= 1) {
        sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
        sysMemContainerDestroy(container_mem);
    }

}

int Get_OSK_String(char *caption, char *str, int len)
{

    int ret=SUCCESS;

    u16 * message = NULL;
    u16 * OutWcharTex = NULL;
    u16 * InWcharTex = NULL;

    if(len > 256) len = 256;

    osk_level = 0;
    atexit(OSK_exit);

    if(sysMemContainerCreate(&container_mem, 8*1024*1024) < 0) return FAILED;

    osk_level = 1;

    message = malloc(strlen(caption)*2+32);
    if(!message) {ret=FAILED; goto end;}

    OutWcharTex = malloc(0x420*2);
    if(!OutWcharTex) {ret=FAILED; goto end;}

    InWcharTex = malloc(0x420*2);
    if(!InWcharTex) {ret=FAILED; goto end;}

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

    if(oskSetKeyLayoutOption (OSK_10KEY_PANEL | OSK_FULLKEY_PANEL)<0) {ret=FAILED; goto end;}

    DialogOskParam.firstViewPanel = OSK_PANEL_TYPE_ALPHABET_FULL_WIDTH;
    DialogOskParam.allowedPanels = (OSK_PANEL_TYPE_ALPHABET | OSK_PANEL_TYPE_NUMERAL);

    if(oskAddSupportLanguage ( OSK_PANEL_TYPE_ALPHABET )<0) {ret=FAILED; goto end;}

    if(oskSetLayoutMode( OSK_LAYOUTMODE_HORIZONTAL_ALIGN_CENTER )<0) {ret=FAILED; goto end;}

    oskPoint pos = {0.0, 0.0};

    DialogOskParam.controlPoint = pos;
    DialogOskParam.prohibitFlags = OSK_PROHIBIT_RETURN;
    if(oskSetInitialInputDevice(OSK_DEVICE_PAD)<0) {ret=FAILED; goto end;}

    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    sysUtilRegisterCallback(SYSUTIL_EVENT_SLOT0, my_eventHandle, NULL);

    osk_action = SUCCESS;
    osk_unloaded = false;

    if(oskLoadAsync(container_mem, (const void *) &DialogOskParam, (const void *)  &inputFieldInfo)<0) {ret=FAILED; goto end;}

    osk_level = 2;

    while(!osk_unloaded)
    {
        cls();
		
		Draw_scene();
		
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
                oskUnloadAsync(&OutputReturnedParam);
                osk_event = 0;
                break;

            default:
                break;
        }

    }

    usleep(150000); // unnecessary but...

    if(OutputReturnedParam.res == OSK_OK && osk_action == SUCCESS)
		UTF16_to_UTF8((u16 *) OutWcharTex, (u8 *) str);
    else ret=FAILED;

end:

    sysUtilUnregisterCallback(SYSUTIL_EVENT_SLOT0);
    sysMemContainerDestroy(container_mem);

    osk_level = 0;
    if(message) free(message);
    if(OutWcharTex) free(OutWcharTex);
    if(InWcharTex) free(InWcharTex);

    return ret;
}