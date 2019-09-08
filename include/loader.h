//#define SHOWTIME_LOADER 1 //uncomment for Showtime Loader
//#define BDUMOUNT_LOADER 1 //uncomment for BD Unmount Loader
//#define LASTPLAY_LOADER 1 //uncomment for lastPLAY Loader

//#define PRXMAMBA_LOADER 1 //uncomment for PRX+MAMBA Loader (obsolete: use NzV Mamba/PRX Loader)

#if defined(LASTPLAY_LOADER) || defined(SHOWTIME_LOADER) || defined(BDUMOUNT_LOADER) || defined(PRXMAMBA_LOADER)
  #define LOADER_MODE     1
#else
  #undef  LOADER_MODE
#endif
