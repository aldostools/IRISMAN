volatile int dialog_action = 0;

static msgType mdialogyesno = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO  | MSG_DIALOG_DEFAULT_CURSOR_NO;
static msgType mdialogyesno2 = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DISABLE_CANCEL_ON;
static msgType mdialogyesno3 = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO;
static msgType mdialogok = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_OK;
static msgType mdialog = MSG_DIALOG_NORMAL | MSG_DIALOG_DISABLE_CANCEL_ON;

static msgType mdialogprogress = MSG_DIALOG_SINGLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON | MSG_DIALOG_DISABLE_CANCEL_ON | MSG_DIALOG_BKG_INVISIBLE;
static msgType mdialogprogress2 = MSG_DIALOG_DOUBLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;

void wait_dialog()
{
    while(!dialog_action)
    {
        sysUtilCheckCallback();
        tiny3d_Flip();
    }

    msgDialogAbort();
    usleep(100000);
}

void my_dialog(msgButton button, void *userdata)
{
    switch(button)
    {
        case MSG_DIALOG_BTN_YES:
            dialog_action = 1;
            break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
        case MSG_DIALOG_BTN_NONE:
            dialog_action = 2;
            break;
        default:
            break;
    }
}

void my_dialog2(msgButton button, void *userdata)
{
    switch(button)
    {
        case MSG_DIALOG_BTN_OK:
        case MSG_DIALOG_BTN_ESCAPE:
        case MSG_DIALOG_BTN_NONE:
            dialog_action = 1;
            break;
        default:
            break;
    }
}

void DrawDialogOKTimer(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogok, str, my_dialog2, (void*) 0x0000aaab, NULL );
    msgDialogClose(milliseconds);

    wait_dialog();
}


void DrawDialogOK(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogok, str, my_dialog2, (void*) 0x0000aaab, NULL );

    wait_dialog();
}


void DrawDialogTimer(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialog, str, my_dialog2, (void*) 0x0000aaab, NULL );
    msgDialogClose(milliseconds);

    wait_dialog();
}

int DrawDialogYesNoTimer(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno, str, my_dialog, (void*)  0x0000aaaa, NULL );
    msgDialogClose(milliseconds);

    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNo(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno, str, my_dialog, (void*)  0x0000aaaa, NULL );

    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNoDefaultYes(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno3, str, my_dialog, (void*)  0x0000aaaa, NULL );

    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNoTimer2(char * str, float milliseconds)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno2, str, my_dialog, (void*)  0x0000aaaa, NULL );
    msgDialogClose(milliseconds);

    wait_dialog();

    return dialog_action;
}

int DrawDialogYesNo2(char * str)
{
    dialog_action = 0;

    msgDialogOpen2(mdialogyesno2, str, my_dialog, (void*)  0x0000aaaa, NULL );

    wait_dialog();

    return dialog_action;
}

static volatile int progress_action2 = 0;

static float bar1_countparts = 0.0f, bar2_countparts = 0.0f;

char progress_bar_title[256];

static void progress_callback(msgButton button, void *userdata)
{
    switch(button)
    {
        case MSG_DIALOG_BTN_OK:
            progress_action2 = 1;
            break;
        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
            progress_action2 = 2;
            break;
        case MSG_DIALOG_BTN_NONE:
            progress_action2 = -1;
            break;
        default:
            break;
    }
}

extern bool scan_canceled;

bool ps3pad_poll()
{
    pad_last_time = 0;
    ps3pad_read();

    if((old_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE)) || (new_pad & (BUTTON_CIRCLE_ | BUTTON_TRIANGLE))) {scan_canceled = true; return true;}

    return false;
}

