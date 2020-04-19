//fsutil.h
#ifndef __FSUTIL__
#define __FSUTIL__

#define FS_S_IFMT 0170000
#define FS_S_IFDIR 0040000

#define S_ISDIR(m)      (((m)&_IFMT) == _IFDIR)

int fs_path_scan (struct fm_panel *p);

int fs_job_scan (struct fm_job *p);

int fs_get_fstype (char *path, int *npo);

int fatfs_init();

//automounter logic
int rootfs_probe ();

#endif
