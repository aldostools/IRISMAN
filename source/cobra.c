/* cobre.c minimal cobralib support for Iris Manager */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/systime.h>
#include <lv2/sysfs.h>
#include <errno.h>
#include "syscall8.h"
#include "cobra.h"

#if 0
#include "file_manager.h"
#include "utils.h"

#define PSPL_LAMBDA_NONCE   0x0ab40b3bbd1f1a7bULL

extern char self_path[MAXPATHLEN];
extern char psp_launcher_path[MAXPATHLEN];

// Tags directly in little endian, no swap needed
// The tags the 3.55 emulator_drm directly supports
static uint32_t emulator_supported_tags355[] =
{
    0x00000008, /* 1.XX */
    0x7C16CBC0, /* 2.XX */
    0x03FD0480, /* 2.71 */
    0xF00516D9, /* 2.80 */
    0xF00616D9, /* 3.00 */
    0xF00A16D9,
    0xF00B16D9,
};

// Keys we add supports
static uint8_t key_D91608F0[] = {0x5C, 0x77, 0x0C, 0xBB, 0xB4, 0xC2, 0x4F, 0xA2, 0x7E, 0x3B, 0x4E, 0xB4, 0xB4, 0xC8, 0x70, 0xAF};
static uint8_t key_D91609F0[] = {0xD0, 0x36, 0x12, 0x75, 0x80, 0x56, 0x20, 0x43, 0xC4, 0x30, 0x94, 0x3E, 0x1C, 0x75, 0xD1, 0xBF};
static uint8_t key_D91611F0[] = {0x61, 0xB0, 0xC0, 0x58, 0x71, 0x57, 0xD9, 0xFA, 0x74, 0x67, 0x0E, 0x5C, 0x7E, 0x6E, 0x95, 0xB9};
static uint8_t key_D91612F0[] = {0x9E, 0x20, 0xE1, 0xCD, 0xD7, 0x88, 0xDE, 0xC0, 0x31, 0x9B, 0x10, 0xAF, 0xC5, 0xB8, 0x73, 0x23};
static uint8_t key_D91613F0[] = {0xEB, 0xFF, 0x40, 0xD8, 0xB4, 0x1A, 0xE1, 0x66, 0x91, 0x3B, 0x8F, 0x64, 0xB6, 0xFC, 0xB7, 0x12};
static uint8_t key_D91614F0[] = {0xFD, 0xF7, 0xB7, 0x3C, 0x9F, 0xD1, 0x33, 0x95, 0x11, 0xB8, 0xB5, 0xBB, 0x54, 0x23, 0x73, 0x85};
static uint8_t key_D91615F0[] = {0xC8, 0x03, 0xE3, 0x44, 0x50, 0xF1, 0xE7, 0x2A, 0x6A, 0x0D, 0xC3, 0x61, 0xB6, 0x8E, 0x5F, 0x51};
static uint8_t key_D91616F0[] = {0x53, 0x03, 0xB8, 0x6A, 0x10, 0x19, 0x98, 0x49, 0x1C, 0xAF, 0x30, 0xE4, 0x25, 0x1B, 0x6B, 0x28};
static uint8_t key_D91617F0[] = {0x02, 0xFA, 0x48, 0x73, 0x75, 0xAF, 0xAE, 0x0A, 0x67, 0x89, 0x2B, 0x95, 0x4B, 0x09, 0x87, 0xA3};
static uint8_t key_D91618F0[] = {0x96, 0x96, 0x7C, 0xC3, 0xF7, 0x12, 0xDA, 0x62, 0x1B, 0xF6, 0x9A, 0x9A, 0x44, 0x44, 0xBC, 0x48};
static uint8_t key_D91619F0[] = {0xE0, 0x32, 0xA7, 0x08, 0x6B, 0x2B, 0x29, 0x2C, 0xD1, 0x4D, 0x5B, 0xEE, 0xA8, 0xC8, 0xB4, 0xE9};
static uint8_t key_D9161AF0[] = {0x27, 0xE5, 0xA7, 0x49, 0x52, 0xE1, 0x94, 0x67, 0x35, 0x66, 0x91, 0x0C, 0xE8, 0x9A, 0x25, 0x24};
static uint8_t key_D91620F0[] = {0x52, 0x1C, 0xB4, 0x5F, 0x40, 0x3B, 0x9A, 0xDD, 0xAC, 0xFC, 0xEA, 0x92, 0xFD, 0xDD, 0xF5, 0x90};
static uint8_t key_D91621F0[] = {0xD1, 0x91, 0x2E, 0xA6, 0x21, 0x14, 0x29, 0x62, 0xF6, 0xED, 0xAE, 0xCB, 0xDD, 0xA3, 0xBA, 0xFE};
static uint8_t key_D91622F0[] = {0x59, 0x5D, 0x78, 0x4D, 0x21, 0xB2, 0x01, 0x17, 0x6C, 0x9A, 0xB5, 0x1B, 0xDA, 0xB7, 0xF9, 0xE6};
static uint8_t key_D91623F0[] = {0xAA, 0x45, 0xEB, 0x4F, 0x62, 0xFB, 0xD1, 0x0D, 0x71, 0xD5, 0x62, 0xD2, 0xF5, 0xBF, 0xA5, 0x2F};
static uint8_t key_D91624F0[] = {0x61, 0xB7, 0x26, 0xAF, 0x8B, 0xF1, 0x41, 0x58, 0x83, 0x6A, 0xC4, 0x92, 0x12, 0xCB, 0xB1, 0xE9};
static uint8_t key_D91628F0[] = {0x49, 0xA4, 0xFC, 0x66, 0xDC, 0xE7, 0x62, 0x21, 0xDB, 0x18, 0xA7, 0x50, 0xD6, 0xA8, 0xC1, 0xB6};
static uint8_t key_D91680F0[] = {0x2C, 0x22, 0x9B, 0x12, 0x36, 0x74, 0x11, 0x67, 0x49, 0xD1, 0xD1, 0x88, 0x92, 0xF6, 0xA1, 0xD8};
static uint8_t key_D91681F0[] = {0x52, 0xB6, 0x36, 0x6C, 0x8C, 0x46, 0x7F, 0x7A, 0xCC, 0x11, 0x62, 0x99, 0xC1, 0x99, 0xBE, 0x98};

typedef struct
{
    uint32_t tag;
    uint8_t code;
    uint8_t *keys;
} PSPKey;

static PSPKey psp_extra_keys[] =
{
    { 0xF00816D9, 0x5D, key_D91608F0 },
    { 0xF00916D9, 0x5D, key_D91609F0 },
    { 0xF01116D9, 0x5D, key_D91611F0 },
    { 0xF01216D9, 0x5D, key_D91612F0 },
    { 0xF01316D9, 0x5D, key_D91613F0 },
    { 0xF01416D9, 0x5D, key_D91614F0 },
    { 0xF01516D9, 0x5D, key_D91615F0 },
    { 0xF01616D9, 0x5D, key_D91616F0 },
    { 0xF01716D9, 0x5D, key_D91617F0 },
    { 0xF01816D9, 0x5D, key_D91618F0 },
    { 0xF01916D9, 0x5D, key_D91619F0 },
    { 0xF01A16D9, 0x5D, key_D9161AF0 },
    { 0xF02016D9, 0x5D, key_D91620F0 },
    { 0xF02116D9, 0x5D, key_D91621F0 },
    { 0xF02216D9, 0x5D, key_D91622F0 },
    { 0xF02316D9, 0x5D, key_D91623F0 },
    { 0xF02416D9, 0x5D, key_D91624F0 },
    { 0xF02816D9, 0x5D, key_D91628F0 },
    { 0xF08016D9, 0x5D, key_D91680F0 },
    { 0xF08116D9, 0x5D, key_D91681F0 },
};

#define NUM_SUPPORTED_TAGS  (sizeof(emulator_supported_tags355)/sizeof(uint32_t))
#define NUM_EXTRA_KEYS      (sizeof(psp_extra_keys)/sizeof(PSPKey))

#define SYSCALL8_OPCODE_SET_PSP_DECRYPT_OPTIONS     0x9002
#define SYSCALL8_OPCODE_SET_PSP_UMDFILE             0x9003
#define SYSCALL8_OPCODE_PSP_CHANGE_EMU              0x9752
#define SYSCALL8_OPCODE_MOUNT_ENCRYPTED_IMAGE       0x702D

enum PSPEmu
{
    EMU_AUTO,
    EMU_355,
    EMU_400
};


LV2_SYSCALL sys_get_version(u32 *version)
{
    lv2syscall2(8, SYSCALL8_OPCODE_GET_VERSION, (u64)version);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_fake_storage_event(uint64_t event, uint64_t param, uint64_t device)
{
    lv2syscall4(8, SYSCALL8_OPCODE_FAKE_STORAGE_EVENT, event, param, device);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_ps3_discfile(unsigned int filescount, uint32_t *files)
{
    lv2syscall3(8, SYSCALL8_OPCODE_MOUNT_PS3_DISCFILE, filescount, (uint64_t)files);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_dvd_discfile(unsigned int filescount, uint32_t *files)
{
    lv2syscall3(8, SYSCALL8_OPCODE_MOUNT_DVD_DISCFILE, filescount, (uint64_t)files);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_bd_discfile(unsigned int filescount, uint32_t *files)
{
    lv2syscall3(8, SYSCALL8_OPCODE_MOUNT_BD_DISCFILE, filescount, (uint64_t)files);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_psx_discfile(char *file, unsigned int trackscount, ScsiTrackDescriptor *tracks)
{
    lv2syscall4(8, SYSCALL8_OPCODE_MOUNT_PSX_DISCFILE, (uint64_t)file, trackscount, (uint64_t)tracks);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_mount_ps2_discfile(unsigned int filescount, uint32_t *files, unsigned int trackscount, ScsiTrackDescriptor *tracks)
{
    lv2syscall5(8, SYSCALL8_OPCODE_MOUNT_PS2_DISCFILE, filescount, (uint64_t)files, trackscount, (uint64_t)tracks);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_umount_discfile(void)
{
    lv2syscall1(8, SYSCALL8_OPCODE_UMOUNT_DISCFILE);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_get_disc_type(unsigned int *real_disctype, unsigned int *effective_disctype, unsigned int *fake_disctype)
{
    lv2syscall4(8, SYSCALL8_OPCODE_GET_DISC_TYPE, (uint64_t)real_disctype, (uint64_t)effective_disctype, (uint64_t)fake_disctype);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_storage_ext_get_emu_state(sys_emu_state_t *state)
{
    lv2syscall2(8, SYSCALL8_OPCODE_GET_EMU_STATE, (uint64_t)state);
    return_to_user_prog(s32);
}


int is_cobra_based(void)
{
    u32 version = 0x99999999;

    if (sys_get_version(&version) < 0)
        return 0;

    if (version != 0x99999999) // If value changed, it is cobra
        return 1;

    return 0;
}

static inline int translate_type(unsigned int type)
{
    if (type == 0)
        return DISC_TYPE_NONE;

    else if (type == DEVICE_TYPE_PS3_BD)
        return DISC_TYPE_PS3_BD;

    else if (type == DEVICE_TYPE_PS3_DVD)
        return DISC_TYPE_PS3_DVD;

    else if (type == DEVICE_TYPE_PS2_DVD)
        return DISC_TYPE_PS2_DVD;

    else if (type == DEVICE_TYPE_PS2_CD)
        return DISC_TYPE_PS2_CD;

    else if (type == DEVICE_TYPE_PSX_CD)
        return DISC_TYPE_PSX_CD;

    else if (type == DEVICE_TYPE_BDROM)
        return DISC_TYPE_BDROM;

    else if (type == DEVICE_TYPE_BDMR_SR)
        return DISC_TYPE_BDMR_SR;

    else if (type == DEVICE_TYPE_BDMR_RR)
        return DISC_TYPE_BDMR_RR;

    else if (type == DEVICE_TYPE_BDMRE)
        return DISC_TYPE_BDMRE;

    else if (type == DEVICE_TYPE_DVD)
        return DISC_TYPE_DVD;

    else if (type == DEVICE_TYPE_CD)
        return DISC_TYPE_CD;

    return DISC_TYPE_UNKNOWN;
}

int cobra_get_disc_type(unsigned int *real_disctype, unsigned int *effective_disctype, unsigned int *iso_disctype)
{
    sys_emu_state_t emu_state;
    unsigned int rdt, edt;
    int ret;

    ret = sys_storage_ext_get_disc_type(&rdt, &edt, NULL);
    if (ret != 0)
        return ret;

    rdt = translate_type(rdt);
    edt = translate_type(edt);

    if (real_disctype)
    {
        *real_disctype = rdt;
    }

    if (effective_disctype)
    {
        *effective_disctype = edt;
    }

    if (iso_disctype)
    {
        *iso_disctype = DISC_TYPE_NONE;

        emu_state.size = sizeof(sys_emu_state_t);
        ret = sys_storage_ext_get_emu_state(&emu_state);

        if (ret == 0)
        {
            int disc_emulation = emu_state.disc_emulation;

            if (disc_emulation != EMU_OFF)
            {
                switch (disc_emulation)
                {
                    case EMU_PS3:
                        *iso_disctype = DISC_TYPE_PS3_BD;
                    break;

                    case EMU_PS2_DVD:
                        *iso_disctype = DISC_TYPE_PS2_DVD;
                    break;

                    case EMU_PS2_CD:
                        *iso_disctype = DISC_TYPE_PS2_CD;
                    break;

                    case EMU_PSX:
                        *iso_disctype = DISC_TYPE_PSX_CD;
                    break;

                    case EMU_BD:
                        if (edt != DISC_TYPE_NONE)
                            *iso_disctype = edt;
                        else
                            *iso_disctype = DISC_TYPE_BDMR_SR;
                    break;

                    case EMU_DVD:
                        *iso_disctype = DISC_TYPE_DVD;
                    break;

                    default:
                        *iso_disctype = DISC_TYPE_UNKNOWN;

                }
            }
        }
    }

    return 0;
}


static unsigned int ejected_realdisc;

int cobra_send_fake_disc_eject_event(void)
{
    sys_storage_ext_get_disc_type(&ejected_realdisc, NULL, NULL);

    sys_storage_ext_fake_storage_event(4, 0, BDVD_DRIVE);
    return sys_storage_ext_fake_storage_event(8, 0, BDVD_DRIVE);
}

int cobra_send_fake_disc_insert_event(void)
{
    uint64_t param;
    unsigned int real_disctype, effective_disctype, iso_disctype;

    cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);

    if (ejected_realdisc == 0 && real_disctype == 0 && effective_disctype == 0 && iso_disctype == 0)
    {
        //printf("Alll disc types 0, aborting\n");
        return -1;//EABORT;
    }

    param = (uint64_t)(ejected_realdisc) << 32ULL;
    sys_storage_ext_get_disc_type(&ejected_realdisc, NULL, NULL);
    sys_storage_ext_fake_storage_event(7, 0, BDVD_DRIVE);
    return sys_storage_ext_fake_storage_event(3, param, BDVD_DRIVE);
}

static uint32_t *translate_str_array(char *array[], unsigned int num)
{
    uint32_t *out = malloc(num * sizeof(uint32_t));

    for (unsigned int i = 0; i < num; i++)
    {
        out[i] = (uint32_t)(uint64_t)array[i];
    }

    return out;
}

int cobra_mount_ps3_disc_image(char *files[], unsigned int num)
{
    uint32_t *files32;
    int ret;

    if (!files) return EINVAL;

    files32 = translate_str_array(files, num);
    ret = sys_storage_ext_mount_ps3_discfile(num, files32);
    free(files32);

    return ret;
}

int cobra_mount_dvd_disc_image(char *files[], unsigned int num)
{
    uint32_t *files32;
    int ret;

    if (!files) return EINVAL;

    files32 = translate_str_array(files, num);
    ret = sys_storage_ext_mount_dvd_discfile(num, files32);
    free(files32);

    return ret;
}

int cobra_mount_bd_disc_image(char *files[], unsigned int num)
{
    uint32_t *files32;
    int ret;

    if (!files) return EINVAL;

    files32 = translate_str_array(files, num);
    ret = sys_storage_ext_mount_bd_discfile(num, files32);
    free(files32);

    return ret;
}

int cobra_umount_disc_image(void)
{
    int ret = sys_storage_ext_umount_discfile();
    if (ret == -1)
        ret = ENODEV;

    return ret;
}

int cobra_mount_ps2_disc_image(char *files[], int num, TrackDef *tracks, unsigned int num_tracks)
{
    uint32_t *files32;
    int ret;
    ScsiTrackDescriptor scsi_tracks[100];

    if (tracks)
    {
        for (int i = 0; i < num_tracks; i++)
        {
            scsi_tracks[i].adr_control = (!tracks[i].is_audio) ? 0x14 : 0x10;
            scsi_tracks[i].track_number = i+1;
            scsi_tracks[i].track_start_addr = tracks[i].lba;
        }
    }

    files32 = translate_str_array(files, num);
    ret = sys_storage_ext_mount_ps2_discfile(num, files32, num_tracks, (tracks) ? scsi_tracks : NULL);
    free(files32);

    return ret;
}

int cobra_load_vsh_plugin(unsigned int slot, char *path, void *arg, uint32_t arg_size)
{
    lv2syscall5(8, SYSCALL8_OPCODE_LOAD_VSH_PLUGIN, slot, (uint64_t)path, (uint64_t)arg, arg_size);
    return_to_user_prog(s32);
}

int cobra_unload_vsh_plugin(unsigned int slot)
{
    lv2syscall2(8, SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN, slot);
    return_to_user_prog(s32);
}

int cobra_mount_psx_disc_image(char *file, TrackDef *tracks, unsigned int num_tracks)
{
  ScsiTrackDescriptor scsi_tracks[100];

  if (!file || num_tracks >= 100) return EINVAL;

  memset(scsi_tracks, 0, sizeof(scsi_tracks));

  for (int i = 0; i < num_tracks; i++)
  {
    scsi_tracks[i].adr_control = (!tracks[i].is_audio) ? 0x14 : 0x10;
    scsi_tracks[i].track_number = i+1;
    scsi_tracks[i].track_start_addr = tracks[i].lba;
  }

  return sys_storage_ext_mount_psx_discfile(file, num_tracks, scsi_tracks);
}


// PSPISO
static inline int sys_storage_ext_mount_encrypted_image(char *image, char *mount_point, char *filesystem, uint64_t nonce)
{
    lv2syscall5(8, SYSCALL8_OPCODE_MOUNT_ENCRYPTED_IMAGE, (uint64_t)image, (uint64_t)mount_point, (uint64_t)filesystem, nonce);
    return_to_user_prog(s32);
}

static inline int sys_psp_change_emu_path(const char *path)
{
    lv2syscall2(8, SYSCALL8_OPCODE_PSP_CHANGE_EMU, (uint64_t)path);
    return_to_user_prog(s32);
}

static inline int sys_psp_set_umdfile(char *file, char *id, int prometheus)
{
    lv2syscall4(8, SYSCALL8_OPCODE_SET_PSP_UMDFILE, (uint64_t)file, (uint64_t)id, prometheus);
    return_to_user_prog(s32);
}

static inline int sys_psp_set_decrypt_options(int decrypt_patch, uint32_t tag, uint8_t *keys, uint8_t code, uint32_t tag2, uint8_t *keys2, uint8_t code2)
{
    lv2syscall8(8, SYSCALL8_OPCODE_SET_PSP_DECRYPT_OPTIONS, decrypt_patch, tag, (uint64_t)keys, code, tag2, (uint64_t)keys2, code2);
    return_to_user_prog(s32);
}

int cobra_set_psp_umd2(char *path, char *umd_root, char *icon_save_path, uint64_t options)
{
    struct stat s;

    char umd_file[256];
    char title_id[11];
    //char title_name[256];
    char *root;
    unsigned int real_disctype, effective_disctype, iso_disctype;
    int ret;

    int decrypt_patch = 1;
    uint32_t tag = 0;
    uint8_t *keys = NULL;
    uint8_t code = 0;
    int prometheus = 0;
    int emu = options & 0xF;


    char *sector = malloc(0x800);
    if(!sector) return 0;

    if (!path || !icon_save_path)
    {
        DrawDialogOKTimer("PSPISO: Invalid path", 2000.0f);
        return EINVAL;
    }

    char PSPL_ICON[MAXPATHLEN];
    sprintf(PSPL_ICON, "%s/ICON0.PNG", psp_launcher_path);

    if (stat(PSPL_ICON, &s) != 0)
    {
        DrawDialogOKTimer("PSPISO: icon not found", 2000.0f);
        return -1;//EABORT;
    }

    char PSPL_LAMBDA[MAXPATHLEN];
    sprintf(PSPL_LAMBDA, "%s/USRDIR/CONTENT/lambda.db", psp_launcher_path);

    if (stat(PSPL_LAMBDA, &s) != 0)
    {
        DrawDialogOKTimer("PSPISO: PSP Launcher database not found", 2000.0f);
        return -2;//ESYSVER;
    }

    FILE *fp = fopen(path, "rb");
    if (!fp) return EIO;

    fseek(fp, 0x8000, SEEK_SET);
    fread((void *) sector, 1, 0x800, fp);
    fclose(fp);

    if (sector[0] != 1 ||  memcmp((void *) &sector[1], "CD001", 5) != 0)
    {
        DrawDialogOKTimer("PSPISO: Invalid ISO image", 2000.0f);
        return EIO;
    }

    memset(title_id, 0, sizeof(title_id));
    memcpy(title_id, sector+0x373, 10);

    root = umd_root;

    if (!root)
    {
        char *files[1];
        int i;

        cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);

        if (iso_disctype != DISC_TYPE_NONE)
        {
            DrawDialogOKTimer("PSPISO: invalid disc type", 2000.0f);
            return EBUSY;
        }

        if (effective_disctype != DISC_TYPE_NONE)
        {
            cobra_send_fake_disc_eject_event();
        }

        files[0] = path;

        uint32_t *files32;
        files32 = translate_str_array(files, 1);

        ret = sys_storage_ext_mount_dvd_discfile(1, files32);
        if (ret != 0)
        {
            if (real_disctype != DISC_TYPE_NONE)
            {
                cobra_send_fake_disc_insert_event();
            }
            return ret;
        }

        cobra_send_fake_disc_insert_event();

        // Wait 0.5 seconds for automonter to mount iso
        for (i = 0; i < 25; i++)
        {
            struct stat s;

            if (stat("/dev_bdvd", &s) == 0)
            {
                break;
            }

            usleep(20000);
        }

        if (i == 25)
        {
            cobra_send_fake_disc_eject_event();
            sys_storage_ext_umount_discfile();

            if (real_disctype != DISC_TYPE_NONE)
                cobra_send_fake_disc_insert_event();

            return EIO;
        }

        root = "/dev_bdvd";
    }
    else
    {
        real_disctype = DISC_TYPE_NONE;
    }

    snprintf(umd_file, sizeof(umd_file), "%s/PSP_GAME/ICON0.PNG", root);

    if (CopyFile(umd_file, icon_save_path) == 0)
    {
        add_sys8_path_table(PSPL_ICON, icon_save_path);
        build_sys8_path_table();

        snprintf(umd_file, sizeof(umd_file), "%s/PSP_GAME/SYSDIR/EBOOT.OLD", root);

        if (stat(umd_file, &s) != 0)
        {
            snprintf(umd_file, sizeof(umd_file), "%s/PSP_GAME/SYSDIR/EBOOT.BIN", root);
        }
        else
        {
            prometheus = 1;
        }

        FILE *fp = fopen(umd_file, "rb");
        if (fp)
        {
            uint32_t header[0xD4/4];

            fread(header, 1, sizeof(header), fp);
            fclose(fp);

            if (header[0] == 0x7E505350)
            {
                unsigned int i;

                decrypt_patch = 0;
                for (i = 0; i < NUM_SUPPORTED_TAGS; i++)
                {
                    if (emulator_supported_tags355[i] == header[0xD0/4])
                        break;
                }

                if (i == NUM_SUPPORTED_TAGS)
                {
                    unsigned int j;

                    //DPRINTF("Tag not supported natively.\n");

                    for (j = 0; j < NUM_EXTRA_KEYS; j++)
                    {
                        if (psp_extra_keys[j].tag == header[0xD0/4])
                        {
                            tag = psp_extra_keys[j].tag;
                            code = psp_extra_keys[j].code;
                            keys = psp_extra_keys[j].keys;
                            //DPRINTF("Tag %08X found\n", psp_extra_keys[j].tag);
                            break;
                        }
                    }

                    if (j == NUM_EXTRA_KEYS)
                    {
                        DrawDialogOKTimer("PSPISO: No tag found. Game will crash.", 2000.0f);
                    }
                }
                else
                {
                    //DPRINTF("Tag supported natively.\n");
                }
            }
        }

        /*
        if (emu == EMU_AUTO)
        {
            snprintf(umd_file, sizeof(umd_file), "%s/PSP_GAME/PARAM.SFO", root);
            if (parse_param_sfo(umd_file, "TITLE", title_name) != 0)
            {
                title_name[0] = 0;
            }

            emu = get_emu(title_id, title_name);
        }
        */

        ret = 0;
    }
    else
    {
        ret = EIO;
    }

    if (!umd_root)
    {
        cobra_send_fake_disc_eject_event();
        sys_storage_ext_umount_discfile();

        if (real_disctype != DISC_TYPE_NONE)
            cobra_send_fake_disc_insert_event();
    }

    if (ret == 0)
    {
        if (emu == EMU_400)
        {
    //          if (check_lambda() < 0)
    //              return ECANCELED;

            sys_storage_ext_mount_encrypted_image(PSPL_LAMBDA, "/dev_moo", "CELL_FS_FAT", PSPL_LAMBDA_NONCE);
            sys_psp_change_emu_path("/dev_moo/pspemu");
        }

        sys_psp_set_umdfile(path, title_id, prometheus);
        sys_psp_set_decrypt_options(decrypt_patch, tag, keys, code, 0, NULL, 0);
    }

    return ret;
}

int cobra_unset_psp_umd(void)
{
    struct stat s;
    char icon_path[MAXPATHLEN];

    sprintf(icon_path, "%s/USRDIR/icons/PSP_ICON.PNG", self_path);

    if(stat(icon_path, &s) != 0)
    {
        char PSPL_ICON[MAXPATHLEN];
        sprintf(PSPL_ICON, "%s/ICON0.PNG", psp_launcher_path);

        add_sys8_path_table(PSPL_ICON, icon_path);
        build_sys8_path_table();
    }

    sys_psp_set_umdfile(NULL, NULL, 0);
    sys_psp_change_emu_path(NULL);
    sys_storage_ext_mount_encrypted_image(NULL, "/dev_moo", NULL, 0);

    return 0;
}

//---------------------------
// Used by Spoof CFW Version
//---------------------------
LV2_SYSCALL sys_read_cobra_config(CobraConfig *cfg);
LV2_SYSCALL sys_write_cobra_config(CobraConfig *cfg);
LV2_SYSCALL sys_get_hw_config(uint8_t *ret, uint8_t *config);

LV2_SYSCALL sys_read_cobra_config(CobraConfig *cfg)
{
    cfg->size = sizeof(CobraConfig);
    lv2syscall2(8, SYSCALL8_OPCODE_READ_COBRA_CONFIG, (u64)cfg);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_write_cobra_config(CobraConfig *cfg)
{
    lv2syscall2(8, SYSCALL8_OPCODE_WRITE_COBRA_CONFIG, (u64)cfg);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_get_hw_config(uint8_t *ret, uint8_t *config)
{
    lv2syscall2(393, (u64)ret, (u64)cobra_unset_psp_umd);
    return_to_user_prog(s32);
}

int cobra_read_config(CobraConfig *cfg)
{
    if (!cfg) return EINVAL;

    cfg->size = sizeof(CobraConfig);
    return sys_read_cobra_config(cfg);
}

int cobra_write_config(CobraConfig *cfg)
{
    if (!cfg)  return EINVAL;

    cfg->size = sizeof(CobraConfig);
    return sys_write_cobra_config(cfg);
}

int cobra_get_ps2_emu_type(void)
{
	int ret;
	uint8_t hw_config[8], ret2;

	ret = sys_get_hw_config(&ret2, hw_config);
	if (ret != 0)
	{
		return ret;
	}

	if (hw_config[6]&1)
	{
		ret = PS2_EMU_HW;
	}
	else if (hw_config[0]&0x20)
	{
		ret = PS2_EMU_GX;
	}
	else
	{
		ret = PS2_EMU_SW;
	}

	return ret;
}
#endif

LV2_SYSCALL sys_storage_ext_get_disc_type(unsigned int *real_disctype, unsigned int *effective_disctype, unsigned int *fake_disctype)
{
    lv2syscall4(8, SYSCALL8_OPCODE_GET_DISC_TYPE, (uint64_t)real_disctype, (uint64_t)effective_disctype, (uint64_t)fake_disctype);
    return_to_user_prog(s32);
}

LV2_SYSCALL sys_get_version(u32 *version)
{
    lv2syscall2(8, SYSCALL8_OPCODE_GET_VERSION, (u64)version);
    return_to_user_prog(s32);
}

int is_cobra_based(void)
{
    u32 version = 0x99999999;

    if (sys_get_version(&version) < 0) return 0;

    if ((version & 0xFF00FF) == 0x04000F || (version & 0xFFFFFF) == 0x03550F)  return 1;

    return 0;
}

static char *get_blank_iso_path(void)
{
    char *s = malloc(32);
    strcpy(s, "/dev_hdd0/vsh/task.dat\0");

    return s;
}

char *build_blank_iso(char *title_id)
{
    uint8_t *buf = malloc(128*1024);

    memset(buf, 0, 128*1024);

    buf[3] = 2;
    buf[0x17] = 0x3F;
    strcpy((char *)buf+0x800, "PlayStation3");
    memcpy(buf+0x810, title_id, 4);
    buf[0x814] = '-';
    memcpy(buf + 0x815, title_id + 4, 5);
    memset(buf + 0x81A, ' ', 0x16);
    buf[0x8000] = 1;
    strcpy((char *)buf+0x8001, "CD001");
    buf[0x8006] = 1;
    memset(buf+0x8008, ' ', 0x20);
    memcpy(buf+0x8028, "PS3VOLUME", 9);
    memset(buf+0x8031, ' ', 0x17);
    buf[0x8050] = buf[0x8057] = 0x40;
    buf[0x8078] = buf[0x807B] = buf[0x807C] = buf[0x807F] = 1;
    buf[0x8081] = buf[0x8082] = 8;
    buf[0x8084] = buf[0x808B] = 0xA;
    buf[0x808C] = 0x14;
    buf[0x8097] = 0x15;
    buf[0x809C] = 0x22;
    buf[0x809E] = buf[0x80A5] = 0x18;
    buf[0x80A7] = buf[0x80AC] = 8;
    buf[0x80AE] = 0x6F;
    buf[0x80AF] = 7;
    buf[0x80B0] = 0x16;
    buf[0x80B1] = 2;
    buf[0x80B2] = 0x2B;
    buf[0x80B3] = buf[0x80B5] = 2;
    buf[0x80B8] = buf[0x80BB] = buf[0x80BC] = 1;
    memcpy(buf+0x80be, "PS3VOLUME", 9);
    memset(buf+0x80C7, ' ', 0x266);
    strcpy((char *)buf+0x832d, "2011072202451800");
    strcpy((char *)buf+0x833e, "0000000000000000");
    strcpy((char *)buf+0x834f, "0000000000000000");
    strcpy((char *)buf+0x8360, "0000000000000000");
    buf[0x8371] = 1;
    buf[0x8800] = 2;
    strcpy((char *)buf+0x8801, "CD001");
    buf[0x8806] = 1;
    buf[0x8829] = 'P';
    buf[0x882B] = 'S';
    buf[0x882D] = '3';
    buf[0x882F] = 'V';
    buf[0x8831] = 'O';
    buf[0x8833] = 'L';
    buf[0x8835] = 'U';
    buf[0x8837] = 'M';
    buf[0x8839] = 'E';
    buf[0x8850] = buf[0x8857] = 0x40;
    strcpy((char *)buf+0x8858, "%/@");
    buf[0x8878] = buf[0x887B] = buf[0x887C] = buf[0x887F] = 1;
    buf[0x8881] = buf[0x8882] = 8;
    buf[0x8884] = buf[0x888B] = 0xA;
    buf[0x888C] = 0x16;
    buf[0x8897] = 0x17;
    buf[0x889C] = 0x22;
    buf[0x889E] = buf[0x88A5] = 0x19;
    buf[0x88A7] = buf[0x88AC] = 8;
    buf[0x88AE] = 0x6F;
    buf[0x88AF] = 7;
    buf[0x88B0] = 0x16;
    buf[0x88B1] = 2;
    buf[0x88B2] = 0x2B;
    buf[0x88B3] = buf[0x88B5] = 2;
    buf[0x88B8] = buf[0x88BB] = buf[0x88BC] = 1;
    buf[0x88BF] = 'P';
    buf[0x88C1] = 'S';
    buf[0x88C3] = '3';
    buf[0x88C5] = 'V';
    buf[0x88C7] = 'O';
    buf[0x88C9] = 'L';
    buf[0x88CB] = 'U';
    buf[0x88CD] = 'M';
    buf[0x88CF] = 'E';

    strcpy((char *)buf+0x8B2D, "2011072202451800");
    strcpy((char *)buf+0x8B3E, "0000000000000000");
    strcpy((char *)buf+0x8B4F, "0000000000000000");
    strcpy((char *)buf+0x8b60, "0000000000000000");
    buf[0x8B71] = 1;
    buf[0x9000] = 0xFF;
    strcpy((char *)buf+0x9001, "CD001");
    buf[0xA000] = 1;
    buf[0xA002] = 0x18;
    buf[0xA006] = 1;
    buf[0xA800] = 1;
    buf[0xA805] = 0x18;
    buf[0xA807] = 1;
    buf[0xB000] = 1;
    buf[0xB002] = 0x19;
    buf[0xB006] = 1;
    buf[0xB800] = 1;
    buf[0xB805] = 0x19;
    buf[0xB807] = 1;
    buf[0xC000] = 0x28;
    buf[0xC002] = buf[0xC009] = 0x18;
    buf[0xC00B] = buf[0xC010] = 8;
    buf[0xC012] = 0x6F;
    buf[0xC013] = 7;
    buf[0xC014] = 0x16;
    buf[0xC015] = 2;
    buf[0xC016] = 0x2B;
    buf[0xC017] = buf[0xC019] = 2;
    buf[0xC01C] = buf[0xC01F] = buf[0xC020] = 1;
    buf[0xC028] = 0x28;
    buf[0xC02A] = buf[0xC031] = 0x18;
    buf[0xC033] = buf[0xC038] = 8;
    buf[0xC03A] = 0x6F;
    buf[0xC03B] = 7;
    buf[0xC03C] = 0x16;
    buf[0xC03D] = 2;
    buf[0xC03E] = 0x2B;
    buf[0xC03F] = buf[0xC041] = 2;
    buf[0xC044] = buf[0xC047] = buf[0xC048] = buf[0xC049] = 1;
    buf[0xC800] = 0x28;
    buf[0xC802] = buf[0xC809] = 0x19;
    buf[0xC80B] = buf[0xC810] = 8;
    buf[0xC812] = 0x6F;
    buf[0xC813] = 7;
    buf[0xC814] = 0x16;
    buf[0xC815] = 2;
    buf[0xC816] = 0x2B;
    buf[0xC817] = buf[0xC819] = 2;
    buf[0xC81C] = buf[0xC81F] = buf[0xC820] = 1;
    buf[0xC828] = 0x28;
    buf[0xC82A] = buf[0xC831] = 0x19;
    buf[0xC833] = buf[0xC838] = 8;
    buf[0xC83A] = 0x6F;
    buf[0xC83B] = 7;
    buf[0xC83C] = 0x16;
    buf[0xC83D] = 2;
    buf[0xC83E] = 0x2B;
    buf[0xC83F] = buf[0xC841] = 2;
    buf[0xC844] = buf[0xC847] = buf[0xC848] = buf[0xC849] = 1;

    char *ret = get_blank_iso_path();

    FILE *f = fopen(ret, "wb");
    if (fwrite(buf, 1, 128*1024, f) != (128*1024))
    {
        fclose(f);
        free(buf);
        free(ret);
        return NULL;
    }

    fclose(f);
    free(buf);
    return ret;
}

#define SYSCALL8_OPCODE_MAP_PATHS			0x7964

int sys_map_path(char *oldpath, char *newpath)
{
#if 1
	lv2syscall2(35, (u64)oldpath, (u64)newpath);
#else
	char *paths[1]={NULL}; char *new_paths[1]={NULL};
	paths[0]=oldpath;new_paths[0]=newpath;
	lv2syscall4(8, SYSCALL8_OPCODE_MAP_PATHS, (u64)paths, (u64)new_paths, 1);
#endif
	return_to_user_prog(s32);
}

int sys_map_paths(char *paths[], char *new_paths[], unsigned int num)
{
	lv2syscall4(8, SYSCALL8_OPCODE_MAP_PATHS, (u64)paths, (u64)new_paths, num);
	return_to_user_prog(s32);
}

int wm_cobra_map_game(char *path, char *title_id)
{
	int ret;
	unsigned int real_disctype;

	ret = sys_map_path((char*)"/dev_bdvd", path);
	if (ret != 0) return ret;

	char *blank_iso = build_blank_iso(title_id);

	sys_map_path((char*)"//dev_bdvd", path);

	sys_map_path((char*)"/app_home", NULL);
	//sys_map_path("//app_home", path);

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);

	if (real_disctype == 0 && blank_iso != NULL)
	{
		cobra_send_fake_disc_eject_event();
		usleep(20000);

		char *files[32];
		int nfiles = 1;

		files[0] = blank_iso;
		files[1] = NULL;

		cobra_mount_ps3_disc_image(files, nfiles);

		{
			cobra_send_fake_disc_insert_event();
			usleep(20000);
		}
	}

	if(blank_iso) free(blank_iso);

	return 0;
}

LV2_SYSCALL sys_storage_ext_fake_storage_event(uint64_t event, uint64_t param, uint64_t device)
{
    lv2syscall4(8, SYSCALL8_OPCODE_FAKE_STORAGE_EVENT, event, param, device);
    return_to_user_prog(s32);
}

static int fake_insert_event(uint64_t devicetype, uint64_t disctype)
{
	uint64_t param = (uint64_t)(disctype) << 32ULL;
	sys_storage_ext_fake_storage_event(7, 0, devicetype);
	return sys_storage_ext_fake_storage_event(3, param, devicetype);
}

static int fake_eject_event(uint64_t devicetype)
{
	sys_storage_ext_fake_storage_event(4, 0, devicetype);
	return sys_storage_ext_fake_storage_event(8, 0, devicetype);
}

void reset_usb_ports(char *_path)
{
	// send fake eject event
	for(u8 f0=0; f0<8; f0++) fake_eject_event((f0<6)?USB_MASS_STORAGE_1(f0):USB_MASS_STORAGE_2(f0));

	sleep(1); u8 indx=0;

	if(strstr(_path, "/dev_usb00")) indx=_path[10]-'0';

	// send fake insert event for the current usb device
	fake_insert_event((indx<6)?USB_MASS_STORAGE_1(indx):USB_MASS_STORAGE_2(indx), DEVICE_TYPE_USB);

	sleep(3);

	// send fake insert event for the other usb devices
	for(u8 f0=0; f0<8; f0++)
	{
		if(f0!=indx) fake_insert_event((f0<6)?USB_MASS_STORAGE_1(f0):USB_MASS_STORAGE_2(f0), DEVICE_TYPE_USB);
	}
}
