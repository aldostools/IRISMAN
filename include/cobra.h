#ifndef _COBRA_H
#define _COBRA_H

#define SYSCALL8_OPCODE_GET_VERSION             0x7000

#define SYSCALL8_OPCODE_READ_COBRA_CONFIG		0x7050
#define SYSCALL8_OPCODE_WRITE_COBRA_CONFIG		0x7051

#define SYSCALL8_OPCODE_GET_DISC_TYPE           0x7020
#define SYSCALL8_OPCODE_READ_PS3_DISC           0x7021
#define SYSCALL8_OPCODE_FAKE_STORAGE_EVENT      0x7022
#define SYSCALL8_OPCODE_GET_EMU_STATE           0x7023
#define SYSCALL8_OPCODE_MOUNT_PS3_DISCFILE      0x7024
#define SYSCALL8_OPCODE_MOUNT_DVD_DISCFILE      0x7025
#define SYSCALL8_OPCODE_MOUNT_BD_DISCFILE       0x7026
#define SYSCALL8_OPCODE_MOUNT_PSX_DISCFILE      0x7027
#define SYSCALL8_OPCODE_MOUNT_PS2_DISCFILE      0x7028
#define SYSCALL8_OPCODE_MOUNT_DISCFILE_PROXY    0x6808
#define SYSCALL8_OPCODE_UMOUNT_DISCFILE         0x702C
#define SYSCALL8_OPCODE_MOUNT_ENCRYPTED_IMAGE   0x702D

#define SYSCALL8_OPCODE_LOAD_VSH_PLUGIN         0x1EE7
#define SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN       0x364F

#define SYSCALL8_OPCODE_GET_ACCESS              0x8097
#define SYSCALL8_OPCODE_REMOVE_ACCESS           0x8654

#define BDVD_DRIVE        0x101000000000006ULL
#define USB_MASS_STORAGE_1(n)    (0x10300000000000AULL+n) /* For 0-5 */
#define USB_MASS_STORAGE_2(n)    (0x10300000000001FULL+(n-6)) /* For 6-127 */
#define USB_MASS_STORAGE(n)    (((n) < 6) ? USB_MASS_STORAGE_1(n) : USB_MASS_STORAGE_2(n))

#define MAX_PATH        0x420

#define ATA_HDD             0x101000000000007ULL
#define BDVD_DRIVE          0x101000000000006ULL
#define PATA0_HDD_DRIVE     0x101000000000008ULL
#define PATA0_BDVD_DRIVE    BDVD_DRIVE
#define PATA1_HDD_DRIVE     ATA_HDD
#define PATA1_BDVD_DRIVE    0x101000000000009ULL
#define BUILTIN_FLASH       0x100000000000001ULL
#define MEMORY_STICK        0x103000000000010ULL
#define SD_CARD             0x103000100000010ULL
#define COMPACT_FLASH       0x103000200000010ULL

#define USB_MASS_STORAGE_1(n)	(0x10300000000000AULL+n) /* For 0-5 */
#define USB_MASS_STORAGE_2(n)	(0x10300000000001FULL+(n-6)) /* For 6-127 */

#define HDD_PARTITION(n)		(ATA_HDD | ((uint64_t)n<<32))
#define FLASH_PARTITION(n)	(BUILTIN_FLASH | ((uint64_t)n<<32))

#define DEVICE_TYPE_PS3_DVD   0xFF70
#define DEVICE_TYPE_PS3_BD    0xFF71
#define DEVICE_TYPE_PS2_CD    0xFF60
#define DEVICE_TYPE_PS2_DVD   0xFF61
#define DEVICE_TYPE_PSX_CD    0xFF50
#define DEVICE_TYPE_BDROM     0x40
#define DEVICE_TYPE_BDMR_SR   0x41 /* Sequential record */
#define DEVICE_TYPE_BDMR_RR   0x42 /* Random record */
#define DEVICE_TYPE_BDMRE     0x43
#define DEVICE_TYPE_DVD       0x10 /* DVD-ROM, DVD+-R, DVD+-RW etc, they are differenced by booktype field in some scsi command */
#define DEVICE_TYPE_CD        0x08 /* CD-ROM, CD-DA, CD-R, CD-RW, etc, they are differenced somehow with scsi commands */
#define DEVICE_TYPE_USB       0x00

enum DiscType
{
    DISC_TYPE_NONE, /* No disc inserted */
    DISC_TYPE_PS3_BD, /* A PS3 game or game hybrid BD */
    DISC_TYPE_PS3_DVD, /* A PS3 DVD. This concept only exists in debug consoles */
    DISC_TYPE_PS2_DVD,
    DISC_TYPE_PS2_CD,
    DISC_TYPE_PSX_CD,
    DISC_TYPE_BDROM, /* Original non PS3 BD */
    DISC_TYPE_BDMR_SR, /* BD-R sequential record */
    DISC_TYPE_BDMR_RR, /* BD-R random record */
    DISC_TYPE_BDMRE, /* BD-RE */
    DISC_TYPE_DVD, /* Any kind of DVD (ROM, +-R, +-RW, etc) that is not a PS game. To distinguish between types, use cobra_get_disc_phys_info to check booktype */
    DISC_TYPE_CD, /* Any kind of CD (ROM, CDDA, -R, -RW) that is not a PS game. No idea how to distinguish the type :) */
    DISC_TYPE_UNKNOWN /* You shouldn't see this value. There is a posibility of this value to be reported on SCDA, since I don't have any, I haven't been able to verify */
};

enum PS2EmuType
{
    PS2_EMU_HW, // the so called "full" bc models, CECHA and CECHB
    PS2_EMU_GX, // the so called partial bc models, CECHC and CECHE
    PS2_EMU_SW, // the non bc models
};

enum DVDRegion
{
    DVD_VIDEO_REGION_1 = 1,
    DVD_VIDEO_REGION_2 = 2,
    DVD_VIDEO_REGION_3 = 4,
    DVD_VIDEO_REGION_4 = 8,
    DVD_VIDEO_REGION_5 = 16,
    DVD_VIDEO_REGION_6 = 32,
    DVD_VIDEO_REGION_MAX = DVD_VIDEO_REGION_6
};

enum BDRegion
{
    BD_VIDEO_REGION_A = 1,
    BD_VIDEO_REGION_B = 2,
    BD_VIDEO_REGION_C = 4,
    BD_VIDEO_REGION_MAX = BD_VIDEO_REGION_C
};

typedef struct
{
    int size;
    int disc_emulation;
    char firstfile_path[MAX_PATH];
} __attribute__((packed)) sys_emu_state_t;

enum DiscEmu
{
    EMU_OFF = 0,
    EMU_PS3,
    EMU_PS2_DVD,
    EMU_PS2_CD,
    EMU_PSX,
    EMU_BD,
    EMU_DVD,
    EMU_PSP,
    EMU_MAX,
};

/* cobra.c minimal cobralib support for Iris Manager */

typedef struct
{
    uint32_t lba;
    int is_audio;
} TrackDef;


typedef struct _ScsiTrackDescriptor
{
    uint8_t reserved;
    uint8_t adr_control;
    uint8_t track_number;
    uint8_t reserved2;
    uint32_t track_start_addr;
} __attribute__((packed)) ScsiTrackDescriptor;


typedef struct
{
    uint16_t size; // size of structure, this will be set automatically by the library
    uint16_t checksum; // Only for core, don't mess with this
    uint8_t bd_video_region; // One of BDRegion, or 0 for default
    uint8_t dvd_video_region; // One of DVDRegion or 0 for default
    uint8_t ps2softemu; // Non-zero to show ps2 discs in non bc consoles
    uint32_t spoof_version; // version in BCD, eg. 0x0400 -> 4.00, 0x0420 -> 4.20 Max value: 0x0999
    uint32_t spoof_revision; // revision nuber (decimal), e.g: 53642. MAx value: 99999
} __attribute__((packed)) CobraConfig;

int is_cobra_based(void);

int cobra_send_fake_disc_eject_event(void);
int cobra_send_fake_disc_insert_event(void);

int cobra_mount_ps3_disc_image(char *files[], unsigned int num);
int cobra_mount_ps2_disc_image(char *files[], int num, TrackDef *tracks, unsigned int num_tracks);
int cobra_mount_psx_disc_image(char *file, TrackDef *tracks, unsigned int num_tracks);
int cobra_mount_dvd_disc_image(char *files[], unsigned int num);
int cobra_mount_bd_disc_image(char *files[], unsigned int num);

int cobra_umount_disc_image(void);
char *build_blank_iso(char *title_id);

int cobra_load_vsh_plugin(unsigned int slot, char *path, void *arg, uint32_t arg_size);
int cobra_unload_vsh_plugin(unsigned int slot);

int cobra_set_psp_umd2(char *path, char *umd_root, char *icon_save_path, uint64_t options);
int cobra_unset_psp_umd(void);

int cobra_read_config(CobraConfig *cfg);
int cobra_write_config(CobraConfig *cfg);
int cobra_get_ps2_emu_type(void);

int sys_map_path(char *oldpath, char *newpath);

int cobra_get_disc_type(unsigned int *real_disctype, unsigned int *effective_disctype, unsigned int *iso_disctype);
int cobra_disc_auth(void);

void reset_usb_ports(char *_path);
int wm_cobra_map_game(char *path, char *title_id);
#endif
