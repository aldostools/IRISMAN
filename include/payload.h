#ifndef _PAYLOAD_H
#define _PAYLOAD_H

extern int (*lv2_unpatch_bdvdemu)(void);
extern int (*lv2_patch_bdvdemu)(uint32_t flags);
extern int (*lv2_patch_storage)(void);
extern int (*lv2_unpatch_storage)(void);

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
