/*******************************************************************************************************************************************************/
/* sys8 path table                                                                                                                                     */
/*******************************************************************************************************************************************************/


static char *table_compare[19];
static char *table_replace[19];

static int ntable = 0;

void reset_sys8_path_table()
{
    while(ntable > 0)
    {
        if(table_compare[ntable - 1]) free(table_compare[ntable - 1]);
        if(table_replace[ntable - 1]) free(table_replace[ntable - 1]);

        ntable --;
    }
}

void add_sys8_path_table(char * compare, char * replace)
{
    if(ntable >= 16) return;

    table_compare[ntable] = malloc(strlen(compare) + 1);
    if(!table_compare[ntable]) return;
    strncpy(table_compare[ntable], compare, strlen(compare) + 1);

    table_replace[ntable] = malloc(strlen(replace) + 1);
    if(!table_replace[ntable]) return;
    strncpy(table_replace[ntable], replace, strlen(replace) + 1);

    ntable++;

    table_compare[ntable] = NULL;
}

void add_sys8_bdvd(char * bdvd, char * app_home)
{
    static char compare1[]="/dev_bdvd";
    static char compare2[]="/app_home";
    static char replace1[MAX_PATH_LEN];
    static char replace2[MAX_PATH_LEN];
    int pos = 17;

    table_compare[pos] = NULL;
    table_compare[pos + 1] = NULL;

    if(bdvd)
    {
        strncpy(replace1, bdvd, MAX_PATH_LEN);
        table_compare[pos] = compare1;
        table_replace[pos] = replace1;
        pos++;
    }

    if(app_home)
    {
        strncpy(replace2, app_home, MAX_PATH_LEN);
        table_compare[pos] = compare2;
        table_replace[pos] = replace2;
        pos++;
    }

}

void build_sys8_path_table()
{

    path_open_entry *pentries;

    int entries = 0;

    int arena_size = 0;

    int n, m;

    sys8_path_table(0LL);

    if(ntable <= 0 && !table_compare[17] && !table_compare[18]) return;
    if(ntable <= 0) {table_compare[0] = 0; ntable = 0;}

    while(ntable > 0 && table_compare[entries] != NULL) entries++;

    // /dev_bdvd & /app_home entries
    if(table_compare[17])
    {
        table_compare[entries] = table_compare[17];
        table_replace[entries] = table_replace[17];
        entries++;
    }

    if(table_compare[18])
    {
        table_compare[entries] = table_compare[18];
        table_replace[entries] = table_replace[18];
        entries++;
    }

    table_compare[entries] = NULL;

    entries = 0;

    while(table_compare[entries] != NULL)
    {
        int l = strlen(table_compare[entries]);

        arena_size += MAX_PATH_LEN;
        for(m = 0x80; m <= MAX_PATH_LEN; m += 0x20)
            if(l < m) {arena_size += m; break;}

        entries++;
    }


    if(!entries) return;

    char * datas = memalign(16, arena_size + sizeof(path_open_entry) * (entries + 2));

    if(!datas) return;

    u64 dest_table_addr = 0x80000000007FE000ULL - (u64)((arena_size + sizeof(path_open_entry) * (entries + 1) + 15) & ~15);

    u32 arena_offset = (sizeof(path_open_entry) * (entries + 1));

    pentries = (path_open_entry *) datas;

    for(n = 0; n < entries; n++)
    {
        int l = strlen(table_compare[n]);

        int size = 0;
        for(m = 0x80; m <= MAX_PATH_LEN; m += 0x20)
            if(l < m) {size += m; break;}

        pentries->compare_addr = dest_table_addr + (u64) (arena_offset);

        pentries->replace_addr = dest_table_addr + (u64) (arena_offset + size);

        strncpy(&datas[arena_offset], table_compare[n], size);
        strncpy(&datas[arena_offset + size], table_replace[n], MAX_PATH_LEN);

        pentries->compare_len = strlen(&datas[arena_offset]);
        pentries->replace_len = strlen(&datas[arena_offset + size]);

        arena_offset += size + MAX_PATH_LEN;
        pentries ++;
    }

    pentries->compare_addr = 0ULL;

    sys8_memcpy(dest_table_addr, (u64) datas, (u64) (arena_size + sizeof(path_open_entry) * (entries + 1)));

    free(datas);

    reset_sys8_path_table();

    // set the path table
    sys8_path_table( dest_table_addr);
}
