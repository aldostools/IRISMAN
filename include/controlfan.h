#ifndef CONTROLFAN_H
#define CONTROLFAN_H

int get_controlfan_offsets();

int test_controlfan_compatibility();

void set_fan_mode(int mode);
void set_device_wakeup_mode(u32 flags);
void set_usleep_sm_main(u32 us);

void load_controlfan_config();

void draw_controlfan_options();

#endif