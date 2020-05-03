//fm.h - simple file manager
#ifndef __FM__
#define __FM__

#define KBSZ    (1024)          //KB: 1024
#define MBSZ    (1048576)       //MB: 1024*1024
#define GBSZ    (1073741824)    //GB: 1024*1024*1024

typedef enum {
    FS_TNONE = 0,
    FS_TSYS,
    FS_TNTFS,
    FS_TEXT,
    FS_TFAT,
} FS_TYPE;

struct fm_file {
    char *name;
    char dir;
    unsigned long size;
    //
    char selected;
    //
    struct fm_file *prev;
    struct fm_file *next;
};

struct fm_panel {
    char *path;                 //current path - full path, including drive identifier
    struct fm_file *entries;    //entries
    struct fm_file *history;    //browsing history: dirs which were traversed
    struct fm_file *current;    //current entry/selection
    unsigned int current_idx;   //index of currently selected item, for scrolling
    //
    int x, y, w, h;             //position+dimentions
    char active;                //panel is active or not
    char fs_type;               //file system type for panel
    //
    int files;
    int dirs;
    int sels;
    unsigned long long fsize;
};

struct fm_job {
    char *spath;                //current path - full path, including drive identifier
    char *dpath;                //current path - full path, including drive identifier
    struct fm_file *entries;    //entries
    //
    char stype;                 //file system type for source
    char dtype;                 //file system type for destination
    //
    int files;
    int dirs;
    unsigned long long fsize;
};

int fm_entry_add (struct fm_file **entries, char *fn, char dir, unsigned long fsz);
int fm_entry_pull (struct fm_file **entries);

//list file management job
int fm_job_list (char *path);
int fm_job_add (struct fm_job *p, char *fn, char dir, unsigned long fsz);
//clear file management job
int fm_job_clear (struct fm_job *job);
//copy files/dirs from source to destination
int fm_job_copy (struct fm_panel *p, char *src, char *dst, int (*ui_render)(int dt));
//remove files/dirs from source
int fm_job_delete (struct fm_panel *p, char *src, int (*ui_render)(int dt));
int fm_job_rename (char *path, char *old, char *new);
//
int fm_job_newdir (char *path, char *new);
//draw (4) status messages
int fm_status_draw (int dat);
//set status message for index
int fm_status_set (char *sm, int idx, int col);
void fm_toggle_selection (struct fm_panel *p);
void fm_panel_select_all (struct fm_panel *p, char sel);

int fm_panel_enter (struct fm_panel *p);
int fm_panel_exit (struct fm_panel *p);
int fm_panel_reload (struct fm_panel *p);

int fm_panel_clear (struct fm_panel *p);
int fm_panel_scan (struct fm_panel *p, char *path);

int fm_panel_init (struct fm_panel *p, int x, int y, int w, int h, char act);
int fm_panel_draw (struct fm_panel *p);
//scroll current item up/dn
int fm_panel_scroll (struct fm_panel *p, int dn);
//locate item and set as current
int fm_panel_locate (struct fm_panel *p, char *path);

//add file/dir entry to panel
int fm_panel_add (struct fm_panel *p, char *fn, char dir, unsigned long fsz);

//remove file/dir entry from panel
int fm_panel_del (struct fm_panel *p, char *fn);

int fm_fname_get (struct fm_file *link, int cw, char *out);

#endif
