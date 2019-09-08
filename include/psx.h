#ifndef PSX_H
#define PSX_H

enum PsxVideoOptionsFlags
{
    PSX_VIDEO_MODE = 0xf,
    PSX_VIDEO_ASPECT_RATIO = 0xf0,
};

enum PsxOptionsFlags
{
    PSX_PAYLOAD  = 0x2,
    PSX_EMULATOR = 0x3,
    PSX_VIDEO_SMOOTHING = 0x4,
    PSX_VIDEO_FULLSCREEN = 0x8,
    PSX_EXTERNAL_ROM = 0x10,
};

typedef struct {
    u32 version;
    char mc1[256];
    char mc2[256];
    u32 video;
    u32 flags;

} psx_opt;

extern psx_opt psx_options;

void unload_psx_payload();

void LoadPSXOptions(char *path);
int SavePSXOptions(char *path);

void draw_psx_options(float x, float y, int index);
void draw_psx_options2(float x, float y, int index);

int get_psx_memcards(void);
void psx_launch(void);

int psx_iso_prepare(char *path, char *name, char *isopath);
int psx_cd_with_cheats(void);

void Reset1_BDVD(void);
void Reset2_BDVD(void);

extern u8 psx_id[32];

int get_disc_ready(void);
u8 get_psx_region_cd(void);
u8 get_psx_region_file(char *path);


#define STOP_BDVD  0
#define START_BDVD 1
#define EJECT_BDVD 2
#define LOAD_BDVD  3
#define NOWAIT_BDVD 128

void Eject_BDVD(int mode);

#endif