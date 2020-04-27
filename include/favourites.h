/*******************************************************************************************************************************************************/
/* Favourites                                                                                                                                          */
/*******************************************************************************************************************************************************/

extern int num_box;

int havefavourites = 0;
tfavourites2 favourites;

static tfavourites2 favourites_gamebase;
static tfavourites2 favourites_homebrew;
static tfavourites2 favourites_films;

void LoadFavourites(char * path, int mode)
{
    int n, mode_flag;
    tfavourites2 * pfavourites = NULL;

    if(mode == GAMEBASE_MODE)
    {
        pfavourites = &favourites_gamebase;
        mode_flag = 0;
        strcat(path, "favourites.bin");
    }
    else if(mode == HOMEBREW_MODE)
    {
        pfavourites = &favourites_homebrew;
        mode_flag = 0xffff0000;
        strcat(path, "favourites2.bin");
    }
    else
    {
        pfavourites = &favourites_films;
        mode_flag = 0xffff0000;
        strcat(path, "favourites3.bin");
    }

    memset(pfavourites, 0, sizeof(*pfavourites));

    int file_size;
    char *file = LoadFile(path, &file_size);

    if(file)
    {
        tfavourites2 *fav = (tfavourites2 *) file;
        if(file_size == sizeof(tfavourites) && fav->version == 100)
            memcpy(pfavourites, file, sizeof(tfavourites));
        if(file_size == sizeof(*pfavourites) && fav->version == 101)
            memcpy(pfavourites, file, sizeof(*pfavourites));
        free(file);
    }

    for(n = 0; n < 48; n++)
    {
        pfavourites->list[n].index  = -1;
        pfavourites->list[n].flags &=  mode_flag;
    }

}

void SaveFavourites(char * path, int mode)
{
    favourites.version = 101;
    if(mode == GAMEBASE_MODE)
    {
        strcat(path, "favourites.bin");
        favourites_gamebase = favourites;
    }
    else if (mode == HOMEBREW_MODE)
    {
        strcat(path, "favourites2.bin");
        favourites_homebrew = favourites;
    }
    else return;

    SaveFile(path, (void *) &favourites, sizeof(favourites)); // use favourites_gamebase?
}

void GetFavourites(int mode)
{
    if(mode > HOMEBREW_MODE)
        favourites = favourites_films;
    else if(mode == HOMEBREW_MODE)
        favourites = favourites_homebrew;
    else
        favourites = favourites_gamebase;
}

void SetFavourites(int mode)
{
    if(mode > HOMEBREW_MODE)
        favourites_films = favourites;
    else if(mode == HOMEBREW_MODE)
        favourites_homebrew = favourites;
    else
        favourites_gamebase = favourites;
}

void UpdateFavourites(t_directories *list, int nlist)
{
    int n, m;

    havefavourites = 0;

    for(m = 0; m < num_box; m++)
    {
        favourites.list[m].index = -1;

        for(n = 0; n < nlist; n++)
        {
            if(favourites.list[m].title_id[0] !=0 && !strncmp(list[n].title_id, favourites.list[m].title_id, 64))
            {
                if((/*1*/ favourites.list[m].index < 0) ||
                    /*2*/((favourites.list[m].flags & NTFS_FLAG) != NTFS_FLAG &&
                    ((favourites.list[m].flags & GAMELIST_FILTER) > (list[n].flags & GAMELIST_FILTER)
                    || (list[n].flags & NTFS_FLAG) == NTFS_FLAG)) ||
                    /*3*/((favourites.list[m].flags & NTFS_FLAG) == NTFS_FLAG && (list[n].flags & D_FLAG_HDD0) == D_FLAG_HDD0))
                {
                    //strncpy(favourites.list[m].title_id, list[n].title_id, 64);
                    //strncpy(favourites.list[m].title, list[n].title, 64);
                    favourites.list[m].index = n;
                    favourites.list[m].flags = list[n].flags;
                    havefavourites = 1;
                }
            }
        }
    }
}


int TestFavouritesExits(char *id)
{
    int m;
    for(m = 0; m < 48; m++)
    {
        if(!strncmp(favourites.list[m].title_id, id, 64)) return true;
    }

    return false;
}

void AddFavourites(int indx, t_directories *list, int position_list)
{
    strncpy(favourites.list[indx].title_id, list[position_list].title_id, 64);
    strncpy(favourites.list[indx].title, list[position_list].title, 64);
    favourites.list[indx].index = position_list;
    favourites.list[indx].flags = list[position_list].flags;
    havefavourites = 1;
}

void DeleteFavouritesIfExits(char *id)
{
    int m;
    for(m = 0; m < 48; m++)
    {
        if(!strcmp(favourites.list[m].title_id, id))
        {
            memset(favourites.list[m].title_id, 0, 64);
            memset(favourites.list[m].title, 0, 64);
            favourites.list[m].index = -1;
        }
    }

    havefavourites = 0;

    for(m = 0; m < 48; m++)
    {
        if(favourites.list[m].index >= 0) {havefavourites = 1; break;}
    }

    return;
}
