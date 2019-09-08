/***********************************************************************************************************/
/* NTFS                                                                                                    */
/***********************************************************************************************************/

const DISC_INTERFACE *disc_ntfs[8]= {
    &__io_ntfs_usb000,
    &__io_ntfs_usb001,
    &__io_ntfs_usb002,
    &__io_ntfs_usb003,
    &__io_ntfs_usb004,
    &__io_ntfs_usb005,
    &__io_ntfs_usb006,
    &__io_ntfs_usb007
};

// mounts from /dev_usb000 to 007
ntfs_md *mounts[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
int mountCount[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int automountCount[8] = {0, 0, 0, 0, 0, 0, 0, 0};


u32 ports_cnt = 0;
u32 old_ports_cnt = 0;
int ntfs_mount_delay = 2;

int NTFS_Event_Mount(int id)
{
    int r = 0;

    ports_cnt &= ~(1<<id);
    if(PS3_NTFS_IsInserted(id)) ports_cnt |= 1<<id;

    if( ((ports_cnt>>id) & 1) && !((old_ports_cnt>>id) & 1)) automountCount[id] = ntfs_mount_delay; // enable delay event

    if(automountCount[id] > 0)
    {   // if delay counter ticks...
        automountCount[id]--; if(automountCount[id] == 0) {r = NTFS_DEVICE_MOUNT; ntfs_mount_delay = (7 * 60);}// mount device (7 seconds)
    }

    if( !((ports_cnt>>id) & 1) && ((old_ports_cnt>>id) & 1))
    {   // unmount device
        automountCount[id] = 0; r = NTFS_DEVICE_UNMOUNT;
    }

    old_ports_cnt = ports_cnt;

    return r;
}

int NTFS_UnMount(int id)
{
    int ret = 0;

    if (mounts[id])
    {
        int k;
        for (k = 0; k < mountCount[id]; k++)
            if((mounts[id]+k)->name[0])
                {ret = NTFS_DEVICE_UNMOUNT; ntfsUnmount((mounts[id]+k)->name, true); (mounts[id]+k)->name[0] = 0;}

        free(mounts[id]);
        mounts[id]= NULL;
        mountCount[id] = 0;
    }

    PS3_NTFS_Shutdown(id);

    return ret;
}

void NTFS_UnMountAll(void)
{
    int i;

    for(i = 0; i < 8; i++)
    {
        NTFS_UnMount(i);
    }
}

int NTFS_Test_Device(char *name)
{
    int k, i;

    for(k = 0; k < 8; k++)
    {
        for (i = 0; i < mountCount[k]; i++)
        if(!strncmp((mounts[k]+i)->name, name, 5 - ((mounts[k]+i)->name[0] == 'e')))
            return ((mounts[k] + i)->interface->ioType & 0xff) - '0';
    }

    return NTFS_DEVICE_UNMOUNT;
}
