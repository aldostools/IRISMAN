/*
    (c) 2011 Hermes/Estwald <www.elotrolado.net>
    IrisManager (HMANAGER port) (c) 2011 D_Skywalk <http://david.dantoine.org>

    HMANAGER4 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    HMANAGER4 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with HMANAGER4.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
    (c) 2013 Estwald/Hermes <www.elotrolado.net>

    MAKEPS3ISO, EXTRACTPS3ISO and PATCHPS3ISO is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MAKEPS3ISO, EXTRACTPS3ISO and PATCHPS3ISO is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MAKEPS3ISO, EXTRACTPS3ISO and PATCHPS3ISO.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "iso.h"
#include "main.h"
#include "utils.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <sys/file.h>

//#define ALIGNED32SECTORS 1

#define NOPS3_UPDATE 1

#define SUCCESS 0
#define FAILED -1

#define ERROR_OUT_OF_MEMORY          -1
#define ERROR_TOO_MUCH_DIR_ENTRIES   -444
#define ERROR_FILE_NAME_TOO_LONG     -555
#define ERROR_OPENING_INPUT_FILE     -666
#define ERROR_READING_INPUT_FILE     -668
#define ERROR_WRITING_OUTPUT_FILE    -667
#define ERROR_INPUT_FILE_NOT_EXISTS  -669
#define ERROR_CREATING_SPLIT_FILE    -777
#define ERROR_ABORTED_BY_USER        -999

#define TICKS_PER_SEC 0x4c1a6bdULL

#define IS_DIRECTORY  1

static u16 wstring[1024];
static char temp_string[1024];

static inline u64 get_ticks(void)
{
    u64 ticks;
    asm volatile("mftb %0" : "=r" (ticks));
    return ticks;
}

#define SWAP16(x) (x)
#define SWAP32(x) (x)

#define SECTOR_FILL    2047
#define SECTOR_SIZE    2048
#define SECTOR_SIZELL  2048LL

int nfiles = 0;

static int get_input_char()
{
    pad_last_time = 0;

    while(true)
    {
        tiny3d_Flip();
        ps3pad_read();

        tiny3d_Project2D();
        DbgDraw();

        if(new_pad & BUTTON_CROSS_)    return 1;   // YES
        if(new_pad & BUTTON_TRIANGLE) return -1;  // NO
        if(new_pad & BUTTON_CIRCLE_)   return 0;   // CANCEL
    }

    return 0;
}

static int get_input_abort()
{
    pad_last_time = 0;

    ps3pad_read();

    if(new_pad & (BUTTON_TRIANGLE | BUTTON_CIRCLE_)) return 1;

    return 0;
}

static void press_button_to_continue()
{
    DPrintf("\n\n                    > > >  Press a button to continue  < < <\n\n");
    get_input_char();

    DCls();
    cls(); tiny3d_Flip();
}

int UTF8_to_UTF16(u8 *stb, u16 *stw);
int UTF16_to_UTF8(u16 *stw, u8 *stb);

static void utf8_to_ansiname(char *utf8, char *ansi, int len)
{
    u8 *ch= (u8 *) utf8;
    u8 c;
    int is_space = 1;

    char *a = ansi;

    *ansi = 0;

    while(*ch != 0 && len > 0)
    {
        // 3, 4 bytes utf-8 code
        if(((*ch & 0xF1) == 0xF0 || (*ch & 0xF0) == 0xe0) && (*(ch+1) & 0xc0) == 0x80)
        {
            if(!is_space)
            {
                *ansi++=' '; // ignore
                len--;
                is_space = 1;
            }

            ch += 2+1*((*ch & 0xF1) == 0xF0);
        }
        else
        // 2 bytes utf-8 code
        if((*ch & 0xE0) == 0xc0 && (*(ch+1) & 0xc0) == 0x80)
        {
            c = (((*ch & 3)<<6) | (*(ch+1) & 63));

            if(c >= 0xC0 && c <= 0xC5) c='A';
            else if(c == 0xc7) c = 'C';
            else if(c >= 0xc8 && c <= 0xcb) c = 'E';
            else if(c >= 0xcc && c <= 0xcf) c = 'I';
            else if(c == 0xd1) c = 'N';
            else if(c >= 0xd2 && c <= 0xd6) c = 'O';
            else if(c >= 0xd9 && c <= 0xdc) c = 'U';
            else if(c == 0xdd) c = 'Y';
            else if(c >= 0xe0 && c <= 0xe5) c = 'a';
            else if(c == 0xe7) c = 'c';
            else if(c >= 0xe8 && c <= 0xeb) c = 'e';
            else if(c >= 0xec && c <= 0xef) c = 'i';
            else if(c == 0xf1) c = 'n';
            else if(c >= 0xf2 && c <= 0xf6) c = 'o';
            else if(c >= 0xf9 && c <= 0xfc) c = 'u';
            else if(c == 0xfd || c == 0xff) c = 'y';
            else if(c>127) c=*(++ch+1); //' ';

            if(!is_space || c!= 32)
            {
               *ansi++=c;
                len--;
                if(c == 32) is_space = 1; else is_space = 0;
            }

            ch++;

        }
        else
        {
            if(*ch<32) *ch=32;

            if(!is_space || *ch!= 32) {
               *ansi++=*ch;

                len--;

                if(*ch == 32) is_space = 1; else is_space = 0;
            }
        }

        ch++;
    }

    while(len > 0)
    {
        *ansi++=0;
        len--;
    }

    if(a[0] == 0 || a[0] == ' ') strcpy(a, "PS3");
}


extern int firmware;

#define ISODCL(from, to) (to - from + 1)

static void setdaterec(unsigned char *p,int dd,int mm,int aa,int ho,int mi,int se)
{
    *p++=(unsigned char) ((aa-1900) & 255);*p++=(char) (mm & 15) ;*p++=(char) (dd & 31);*p++=(char) ho;*p++=(char) mi;*p++=(char) se;*p++=(char) 0;

}

static void set731(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>24) & 0xff);
}

static void set733(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>24) & 0xff);
    *p++=((n>>24) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static void set732(unsigned char *p,int n)
{
    *p++=((n>>24) & 0xff);*p++=((n>>16) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static void set721(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);
}

static void set722(unsigned char *p,int n)
{
    *p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static void set723(unsigned char *p,int n)
{
    *p++=(n & 0xff);*p++=((n>>8) & 0xff);*p++=((n>>8) & 0xff);*p++=(n & 0xff);
}

static int isonum_731 (unsigned char * p)
{
    return ((p[0] & 0xff)
         | ((p[1] & 0xff) << 8)
         | ((p[2] & 0xff) << 16)
         | ((p[3] & 0xff) << 24));
}

/*
static int isonum_732 (unsigned char * p)
{
    return ((p[3] & 0xff)
         | ((p[2] & 0xff) << 8)
         | ((p[1] & 0xff) << 16)
         | ((p[0] & 0xff) << 24));
}
*/

static int isonum_733 (unsigned char * p)
{
    return (isonum_731 (p));
}


static int isonum_721 (unsigned char * p)
{
    return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

/*
static int isonum_723 (unsigned char * p)
{
    return (isonum_721 (p));
}
*/

struct iso_primary_descriptor {
    unsigned char type                      [ISODCL (  1,   1)]; /* 711 */
    unsigned char id                        [ISODCL (  2,   6)];
    unsigned char version                   [ISODCL (  7,   7)]; /* 711 */
    unsigned char unused1                   [ISODCL (  8,   8)];
    unsigned char system_id                 [ISODCL (  9,  40)]; /* aunsigned chars */
    unsigned char volume_id                 [ISODCL ( 41,  72)]; /* dunsigned chars */
    unsigned char unused2                   [ISODCL ( 73,  80)];
    unsigned char volume_space_size         [ISODCL ( 81,  88)]; /* 733 */
    unsigned char unused3                   [ISODCL ( 89, 120)];
    unsigned char volume_set_size           [ISODCL (121, 124)]; /* 723 */
    unsigned char volume_sequence_number    [ISODCL (125, 128)]; /* 723 */
    unsigned char logical_block_size        [ISODCL (129, 132)]; /* 723 */
    unsigned char path_table_size           [ISODCL (133, 140)]; /* 733 */
    unsigned char type_l_path_table         [ISODCL (141, 144)]; /* 731 */
    unsigned char opt_type_l_path_table     [ISODCL (145, 148)]; /* 731 */
    unsigned char type_m_path_table         [ISODCL (149, 152)]; /* 732 */
    unsigned char opt_type_m_path_table     [ISODCL (153, 156)]; /* 732 */
    unsigned char root_directory_record     [ISODCL (157, 190)]; /* 9.1 */
    unsigned char volume_set_id             [ISODCL (191, 318)]; /* dunsigned chars */
    unsigned char publisher_id              [ISODCL (319, 446)]; /* achars */
    unsigned char preparer_id               [ISODCL (447, 574)]; /* achars */
    unsigned char application_id            [ISODCL (575, 702)]; /* achars */
    unsigned char copyright_file_id         [ISODCL (703, 739)]; /* 7.5 dchars */
    unsigned char abstract_file_id          [ISODCL (740, 776)]; /* 7.5 dchars */
    unsigned char bibliographic_file_id     [ISODCL (777, 813)]; /* 7.5 dchars */
    unsigned char creation_date             [ISODCL (814, 830)]; /* 8.4.26.1 */
    unsigned char modification_date         [ISODCL (831, 847)]; /* 8.4.26.1 */
    unsigned char expiration_date           [ISODCL (848, 864)]; /* 8.4.26.1 */
    unsigned char effective_date            [ISODCL (865, 881)]; /* 8.4.26.1 */
    unsigned char file_structure_version    [ISODCL (882, 882)]; /* 711 */
    unsigned char unused4                   [ISODCL (883, 883)];
    unsigned char application_data          [ISODCL (884, 1395)];
    unsigned char unused5                   [ISODCL (1396, 2048)];
};

struct iso_directory_record {
    unsigned char length                    [ISODCL ( 1,  1)]; /* 711 */
    unsigned char ext_attr_length           [ISODCL ( 2,  2)]; /* 711 */
    unsigned char extent                    [ISODCL ( 3, 10)]; /* 733 */
    unsigned char size                      [ISODCL (11, 18)]; /* 733 */
    unsigned char date                      [ISODCL (19, 25)]; /* 7 by 711 */
    unsigned char flags                     [ISODCL (26, 26)];
    unsigned char file_unit_size            [ISODCL (27, 27)]; /* 711 */
    unsigned char interleave                [ISODCL (28, 28)]; /* 711 */
    unsigned char volume_sequence_number    [ISODCL (29, 32)]; /* 723 */
    unsigned char name_len                  [1]; /* 711 */
    unsigned char name                      [1];
};

struct iso_path_table{
    unsigned char  name_len[2]; /* 721 */
    char extent[4];             /* 731 */
    char parent[2];             /* 721 */
    char name[1];
};

struct psp_file_entry_descriptor {
    unsigned char filler1[2];
    unsigned char lba_msd[4];
    unsigned char lba_lsd[4];
    unsigned char size_msd[4];
    unsigned char size_lsd[4];
    unsigned char filler2[14];
    unsigned char name_len[1];
    unsigned char file_name[23];
};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void get_rand(void *bfr, u32 size)
{
    int n;

    if (size == 0)
        return;

    srand(get_ticks());

    for(n = 0; n < size; n++)
        *(((char *) bfr) + n) = rand() & 0xFF;
}

u64 get_disk_free_space(char *path)
{

    struct statvfs svfs;
    u32 blockSize;
    u64 freeSize = 0;
    int is_ntfs = 0;

    if(!strncmp(path, "/ntfs", 5) || !strncmp(path, "/ext", 4)) is_ntfs = 1;

    if(!is_ntfs)
    {
        if(sysFsGetFreeSize(path, &blockSize, &freeSize) !=0) return (u64) (-1LL);
        return (((u64)blockSize * freeSize));
    }
    else
    {
        if(ps3ntfs_statvfs((const char *) path, &svfs) != 0) return (u64) (-1LL);
    }

    return ( ((u64)svfs.f_bsize * svfs.f_bfree));


}

#ifdef USE_64BITS_LSEEK
int get_iso_file_pos(int fd, char *path, u32 *flba, u64 *size)
#else
int get_iso_file_pos(FILE *fp, unsigned char *path, u32 *flba, u64 *size)
#endif
{
    static struct iso_primary_descriptor sect_descriptor;
    struct iso_directory_record * idr;
    static int folder_split[64][3];
    int nfolder_split = 0;

    u32 file_lba = 0xffffffff;

    u8 *sectors = NULL;

    #ifdef USE_64BITS_LSEEK
    if(fd <= 0 || !size || !flba) return FAILED;
    #else
    if(!fp || !size || !flba) return FAILED;
    #endif

    *size = 0;

    folder_split[nfolder_split][0] = 0;
    folder_split[nfolder_split][1] = 0;
    folder_split[nfolder_split][2] = 0;
    int i = 0;
    int len = strlen(path);

    while(path[i] != 0 && i < len)
    {
        if(path[i] == '/')
        {
            folder_split[nfolder_split][2] = i - folder_split[nfolder_split][1];
            while(path[i] =='/' && i < len) i++;
            if(folder_split[nfolder_split][2] == 0) {folder_split[nfolder_split][1] = i; continue;}
            folder_split[nfolder_split][0] = 1;
            nfolder_split++;
            folder_split[nfolder_split][0] = 0;
            folder_split[nfolder_split][1] = i;
            folder_split[nfolder_split][2] = 0;

        } else i++;
    }

    folder_split[nfolder_split][0] = 0;
    folder_split[nfolder_split][2] = i - folder_split[nfolder_split][1];
    nfolder_split++;

    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, 0x8800LL, SEEK_SET) != 0x8800LL) goto err;
    if(ps3ntfs_read(fd, (void *) &sect_descriptor, sizeof(struct iso_primary_descriptor)) != sizeof(struct iso_primary_descriptor)) goto err;
    #else
    if(fseek(fp, 0x8800, SEEK_SET) != 0) goto err;
    if(fread((void *) &sect_descriptor, 1, sizeof(struct iso_primary_descriptor), fp) != sizeof(struct iso_primary_descriptor)) goto err;
    #endif

    if((sect_descriptor.type[0] != 2 && sect_descriptor.type[0] != 0xFF) || strncmp((void *) sect_descriptor.id, "CD001", 5)) goto err;

    // PSP ISO
    if(sect_descriptor.type[0] == 0xFF)
    {
        unsigned char dir_entry[0x38];
        char entry_name[0x20];

        #ifdef USE_64BITS_LSEEK
        if(ps3ntfs_seek64(fd, 0xB860LL, SEEK_SET) != 0xB860LL) goto err;
        #else
        if(fseek(fp, 0xB860, SEEK_SET) != 0) goto err;
        #endif

        int c;

        for(int i = 0; i < 8; i++)
        {
            #ifdef USE_64BITS_LSEEK
            if(ps3ntfs_read(fd, (void *) &dir_entry, 0x38) != 0x38) goto err;
            #else
            if(fread((void *) &dir_entry, 1, 0x38, fp) != 0x38) goto err;
            #endif

            for(c = 0; c < dir_entry[0x20]; c++) entry_name[c] = dir_entry[0x21 + c];
            for( ; c < 0x20; c++) entry_name[c] = 0;

            if(strstr(path, entry_name))
            {
                file_lba = isonum_731(&dir_entry[2]);
                *flba = file_lba;

                *size = isonum_731(&dir_entry[10]);

                #ifdef USE_64BITS_LSEEK
                if(ps3ntfs_seek64(fd, ((s64) file_lba) * SECTOR_SIZELL, SEEK_SET) != ((s64) file_lba) * SECTOR_SIZELL) goto err;
                #else
                if(fseek(fp, file_lba * SECTOR_SIZE, SEEK_SET) != 0) goto err;
                #endif

                return SUCCESS;
            }
        }

        return -4;
    }

    u32 lba0 = isonum_731(&sect_descriptor.type_l_path_table[0]); // lba
    if(!lba0) return -3;

    u32 size0 = isonum_733(&sect_descriptor.path_table_size[0]); // size
    if(!size0) return -3;

    int size1 = ((size0 + SECTOR_FILL) / SECTOR_SIZE) * SECTOR_SIZE;
    if(size1 < 0 || size1 > 0x400000) return -3; // size larger than 4MB

    sectors = malloc(size1 + SECTOR_SIZE);
    if(!sectors) return -3;

    memset(sectors, 0, size1 + SECTOR_SIZE);

    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) lba0) * SECTOR_SIZELL, SEEK_SET) != ((s64) lba0) * SECTOR_SIZELL) goto err;
    if(ps3ntfs_read(fd, (void *) sectors, size1) != size1) goto err;
    #else
    if(fseek(fp, lba0 * SECTOR_SIZE, SEEK_SET) != 0) goto err;
    if(fread((void *) sectors, 1, size1, fp) != size1) goto err;
    #endif

    u32 p = 0;

    u32 lba_folder = 0xffffffff;
    u32 lba  = 0xffffffff;

    int nsplit = 0;
    int last_parent = 1;
    int cur_parent = 1;

    while(p < size0)
    {
        if(nsplit >= nfolder_split) break;
        if(folder_split[nsplit][0] == 0 && nsplit != 0) {lba_folder = lba; break;}
        if(folder_split[nsplit][2] == 0) continue;

        u32 snamelen = isonum_721(&sectors[p]);
        if(snamelen == 0) p = ((p / SECTOR_SIZE) * SECTOR_SIZE) + SECTOR_SIZE; //break;
        p += 2;
        lba = isonum_731(&sectors[p]);
        p += 4;
        u32 parent_name = isonum_721(&sectors[p]);
        p += 2;

        memset(wstring, 0, 512 * 2);
        memcpy(wstring, &sectors[p], snamelen);

        len = UTF16_to_UTF8(wstring, (u8 *) temp_string);

        if(cur_parent == 1 && folder_split[nsplit][0] == 0 && nsplit == 0) {lba_folder = lba; break;}

        if(last_parent == parent_name && len == folder_split[nsplit][2] &&
           !strncmp((void *) temp_string, &path[folder_split[nsplit][1]], folder_split[nsplit][2]))
        {

            //DPrintf("p: %s %u %u\n", &path[folder_split[nsplit][1]], folder_split[nsplit][2], snamelen);
            last_parent = cur_parent;

            nsplit++;
            if(folder_split[nsplit][0] == 0) {lba_folder = lba; break;}
        }

        p += snamelen;
        cur_parent++;
        if(snamelen & 1) {p++;}
    }

    if(lba_folder == 0xffffffff) goto err;

    memset(sectors, 0, 4096);

    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) lba_folder) * SECTOR_SIZELL, SEEK_SET) != ((s64) lba_folder) * SECTOR_SIZELL) goto err;
    if(ps3ntfs_read(fd, (void *) sectors, SECTOR_SIZE) != SECTOR_SIZE) goto err;
    #else
    if(fseek(fp, lba_folder * SECTOR_SIZE, SEEK_SET) != 0) goto err;
    if(fread((void *) sectors, 1, SECTOR_SIZE, fp) != SECTOR_SIZE) goto err;
    #endif

    int size_directory = -1;
    int p2 = 0;

    p = 0;
    while(true)
    {
        if(nsplit >= nfolder_split) break;
        idr = (struct iso_directory_record *) &sectors[p];

        if(size_directory == -1)
        {
            if((int) idr->name_len[0] == 1 && idr->name[0] == 0 && lba == isonum_731((void *) idr->extent) && idr->flags[0] == 0x2)
            {
                size_directory = isonum_733((void *) idr->size);
            }
        }

        if(idr->length[0] == 0 && sizeof(struct iso_directory_record) + p > SECTOR_SIZE)
        {
            lba_folder++;

            #ifdef USE_64BITS_LSEEK
            if(ps3ntfs_seek64(fd, ((s64) lba_folder) * SECTOR_SIZELL, SEEK_SET) != ((s64) lba_folder) * SECTOR_SIZELL) goto err;
            if(ps3ntfs_read(fd, (void *) sectors, SECTOR_SIZE) != SECTOR_SIZE) goto err;
            #else
            if(fseek(fp, lba_folder * SECTOR_SIZE, SEEK_SET) != 0) goto err;
            if(fread((void *) sectors, 1, SECTOR_SIZE, fp) != SECTOR_SIZE) goto err;
            #endif

            p = 0; p2 = (p2 & ~SECTOR_FILL) + SECTOR_SIZE;

            idr = (struct iso_directory_record *) &sectors[p];
            if((int) idr->length[0] == 0) break;
            if((size_directory == -1 && idr->length[0] == 0) || (size_directory != -1 && p2 >= size_directory)) break;
            continue;
        }

        if((size_directory == -1 && idr->length[0] == 0) || (size_directory != -1 && p2 >= size_directory)) break;

        if((int) idr->length[0] == 0) break;

        memset(wstring, 0, 512 * 2);
        memcpy(wstring, (char *) idr->name, idr->name_len[0]);

        len = UTF16_to_UTF8(wstring, (u8 *) temp_string);

        if(len == folder_split[nsplit][2] &&
          !strncmp((char *) temp_string, &path[folder_split[nsplit][1]], (int) folder_split[nsplit][2]))
        {
            if(file_lba == 0xffffffff) file_lba = isonum_733(&idr->extent[0]);

            *size+= (u64) (u32) isonum_733(&idr->size[0]);

        } else if(file_lba != 0xffffffff) break;

        p += idr->length[0]; p2 += idr->length[0];
    }

    *flba = file_lba;

    if(file_lba == 0xffffffff) goto err;

    #ifdef USE_64BITS_LSEEK
    if(ps3ntfs_seek64(fd, ((s64) file_lba) * SECTOR_SIZELL, SEEK_SET) != ((s64) file_lba) * SECTOR_SIZELL) goto err;
    #else
    if(fseek(fp, file_lba * SECTOR_SIZE, SEEK_SET) != 0) goto err;
    #endif

    if(sectors) free(sectors);

    return SUCCESS;

err:
    if(sectors) free(sectors);

    return -4;
}

int create_fake_file_iso(char *path, char *filename, u64 size)
{
    u8 *mem = create_fake_file_iso_mem(filename, size);
    if(!mem) return ERROR_OUT_OF_MEMORY;

    int ret = SUCCESS;

    FILE *fp2 = fopen(path, "wb");

    if(fp2)
    {
        fwrite((void *) mem, 1, sizeof(build_iso_data), fp2);
        fclose(fp2);
    }
    else ret = FAILED;

    free(mem);

    return ret;
}

u8 *create_fake_file_iso_mem(char *filename, u64 size)
{
    int len_string;

    u8 *mem = malloc(sizeof(build_iso_data));
    if(!mem) return NULL;

    u16 *string = (u16 *) malloc(256);
    if(!string) {free(mem); return NULL;}

    #define MAX_NAME_LEN	110

    char name[MAX_NAME_LEN + 1];
    strncpy(name, filename, MAX_NAME_LEN);
    name[MAX_NAME_LEN] = 0;

    if(strlen(filename) > MAX_NAME_LEN)
    {
        // break the string
        int pos = MAX_NAME_LEN - 1 - strlen(get_extension(filename));
        while(pos > 0 && (name[pos] & 192) == 128) pos--; // skip UTF extra codes
        strcpy(&name[pos], get_extension(filename));
    }

    len_string = UTF8_to_UTF16((u8 *) name, string);

    memcpy(mem, build_iso_data, sizeof(build_iso_data));

    struct iso_primary_descriptor *ipd  = (struct iso_primary_descriptor *) &mem[0x8000];
    struct iso_primary_descriptor *ipd2 = (struct iso_primary_descriptor *) &mem[0x8800];
    struct iso_directory_record   *idr  = (struct iso_directory_record *)   &mem[0xB840];
    struct iso_directory_record   *idr2 = (struct iso_directory_record *)   &mem[0xC044];

    u32 last_lba = isonum_733 (ipd->volume_space_size);

    u64 size0 = size;

    while(size)
    {
        u8 flags = 0;

        if(size > 0xFFFFF800ULL) {flags = 0x80; size0 = 0xFFFFF800ULL;} else size0 = size;
        idr->name_len[0] = strlen(name);
        memcpy(idr->name, name, idr->name_len[0]);
        idr->length[0] = (idr->name_len[0] + sizeof(struct iso_directory_record) + 1) & ~1;
        idr->ext_attr_length[0] = 0;

        set733(idr->extent, last_lba);
        set733(idr->size, size0);

        idr->date[0] = 0x71; idr->date[1] = 0x0B;
        idr->date[2] = 0x0A; idr->date[3] = 0x0D;
        idr->date[4] = 0x38; idr->date[5] = 0x00;
        idr->date[6] = 0x04;
        idr->flags[0] = flags;
        idr->file_unit_size[0] = 0;
        idr->interleave[0] = 0;

        set723(idr->volume_sequence_number, 1);

        idr = (struct iso_directory_record *) (((char *) idr) + idr->length[0]);

        /////////////////////

        idr2->name_len[0] = len_string * 2;

        memcpy(idr2->name, string, idr2->name_len[0]);

        idr2->length[0] = (idr2->name_len[0] + sizeof(struct iso_directory_record) + 1) & ~1;
        idr2->ext_attr_length[0] = 0;
        set733(idr2->extent, last_lba);
        set733(idr2->size, size0);
        idr2->date[0] = 0x71; idr2->date[1] = 0x0B;
        idr2->date[2] = 0x0A; idr2->date[3] = 0x0D;
        idr2->date[4] = 0x38; idr2->date[5] = 0x00;
        idr2->date[6] = 0x04;
        idr2->flags[0] = flags;
        idr2->file_unit_size[0] = 0;
        idr2->interleave[0] = 0;
        set723(idr2->volume_sequence_number, 1);

        idr2 = (struct iso_directory_record *) (((char *) idr2) + idr2->length[0]);

        /////////////////////
        last_lba += (size0 + 0x7ffULL)/ 0x800ULL;
        size-= size0;
    }

    last_lba += (size + SECTOR_FILL) / SECTOR_SIZE;
    set733(ipd->volume_space_size, last_lba);
    set733(ipd2->volume_space_size, last_lba);

    free(string);
    return mem;
}

/***********************************************************************************************************/
/* MAKEPS3ISO - EXTRACTPS3ISO - PATCHPS3ISO                                                                */
/***********************************************************************************************************/

static int param_patched = 0;
static int self_sprx_patched = 0;

static int cur_isop = -1;

static int lpath;
static int wpath;

static u32 llba0 = 0; // directory path0
static u32 llba1 = 0; // directory path1
static u32 wlba0 = 0; // directory pathw0
static u32 wlba1 = 0; // directory pathw1

static u32 dllba = 0; // dir entries
static u32 dwlba = 0; // dir entriesw
static u32 dlsz = 0; // dir entries size (sectors)
static u32 dwsz = 0; // dir entriesw size (sectors)
static u32 flba = 0; // first lba for files
static u32 toc = 0;  // TOC of the iso

static char iso_split = 0;
static char output_name[0x420];
static char output_name2[0x420];

static int pos_lpath0 = 0;
static int pos_lpath1 = 0;
static int pos_wpath0 = 0;
static int pos_wpath1 = 0;

static int dd = 1, mm = 1, aa = 2013, ho = 0, mi = 0, se = 2;

static u8 *sectors = NULL;
static u8 *sectors3 = NULL;


#define MAX_ISO_PATHS 4096

typedef struct {
    u32 ldir;
    u32 wdir;
    u32 llba;
    u32 wlba;
    int parent;
    char *name;

} _directory_iso;

typedef struct {
    int parent;
    char *name;

} _directory_iso2;

typedef struct {
    u32 size;
    char path[0x420];

} _split_file;

static _split_file *split_file = NULL;

static int fd_split = -1;
static int fd_split0 = -1;

static int split_index = 0;
static int split_files = 0;

static _directory_iso *directory_iso = NULL;
static _directory_iso2 *directory_iso2 = NULL;

static char dbhead[64];

static void memcapscpy(void *dest, void *src, int size)
{
    char *d = dest;
    char *s = src;
    char c;

    int n;

    for(n = 0; n < size; n++) {c = *s++; *d++ = (char) toupper((int) c);}
}

static int iso_parse_param_sfo(char * file, char *title_id, char *title_name)
{
    int fd;
    int bytes;
    int ct = 0;

    fd = ps3ntfs_open(file, O_RDONLY, 0766);

    if(fd >= 0)
    {
        int len, pos, str;
        unsigned char *mem=NULL;

        len = (int) ps3ntfs_seek64(fd, 0, SEEK_END);

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {ps3ntfs_close(fd); return -2;}

        memset(mem, 0, len + 16);

        ps3ntfs_seek64(fd, 0, SEEK_SET);

        bytes = ps3ntfs_read(fd, (void *) mem, len);

        ps3ntfs_close(fd);

        if(bytes != len)
        {
            free(mem);
            return -2;
        }

        str = (mem[8]   + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0;

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strncmp((char *) &mem[str], "TITLE", 6))
            {
                strncpy(title_name, (char *) &mem[pos], 63);
                title_name[63] = 0;
                ct++;
            }
            else if(!strncmp((char *) &mem[str], "TITLE_ID", 9))
            {
                memcpy(title_id, (char *) &mem[pos], 4);
                title_id[4] = '-';
                strncpy(&title_id[5], (char *) &mem[pos + 4], 58);
                title_id[63] = 0;
                ct++;
            }

            if(ct >= 2)
            {
                free(mem);
                return SUCCESS;
            }

            while(mem[str]) str++; str++;
            pos  += (mem[0x1c+indx]+(mem[0x1d+indx]<<8));
            indx += 16;
        }

        if(mem) free(mem);
    }

    return FAILED;
}

static int calc_entries(char *path, int parent)
{
    DIR  *dir;
    int len_string;
    struct stat s;

    int cldir = 0;
    int ldir = sizeof(struct iso_directory_record) + 6; // ..
    ldir = (ldir + 7) & ~7;
    ldir += sizeof(struct iso_directory_record) + 6; // .
    ldir += ldir & 1;

    int cwdir = 0;
    int wdir = sizeof(struct iso_directory_record) + 6; // ..
    wdir = (wdir + 7) & ~7;
    wdir += sizeof(struct iso_directory_record) + 6; // .
    wdir += wdir & 1;

    cldir = ldir;
    cwdir = wdir;

    lpath+= (lpath & 1);
    wpath+= (wpath & 1);

    int cur = cur_isop;

    if(cur >= MAX_ISO_PATHS) return ERROR_TOO_MUCH_DIR_ENTRIES;

    directory_iso[cur].ldir = ldir;
    directory_iso[cur].wdir = wdir;
    directory_iso[cur].parent = parent;
    if(!cur)
    {
        directory_iso[cur].name = malloc(16);
        if(!directory_iso[cur].name) return ERROR_OUT_OF_MEMORY;
        directory_iso[cur].name[0] = 0;
    }


    int cur2 = cur;
    int nfolders = 0, mfolders, nentries, progress, pcount;

    nentries = 0; progress = 100; pcount = 0;

    cur_isop++;

    bool is_dir_entry;

    int len = strlen(path);

    // calc folders, subfolders, multi-part files & files
    #ifdef NOPS3_UPDATE
    if(len >= 11 && strstr(path + len - 11, "/PS3_UPDATE")) {dir = NULL; nfiles++;}else
    #endif
    dir = opendir(path);

    if(dir)
    {
        int l = len + 1;
        strcat(path + len, "/");

        // files & multi-part files
        while(true)
        {
            struct dirent *entry = readdir(dir);

            if(!entry || !entry->d_name[0]) break;
            if(entry->d_name[0] == '.' && (entry->d_name[1] == '.' || entry->d_name[1] == 0)) continue;

            if(get_input_abort()) {closedir(dir); return ERROR_ABORTED_BY_USER;}

            nentries++;
            if(nentries >= progress) {DPrintf(pcount ? "." : ">> Still reading... Please wait ."); pcount++; progress = nentries + 100;}

            sprintf(path + l, entry->d_name);

            if(stat(path, &s) != SUCCESS) continue; //{closedir(dir); DPrintf("Not found: %s\n", path); path[len] = 0; return ERROR_INPUT_FILE_NOT_EXISTS;}

            is_dir_entry = entry->d_type & IS_DIRECTORY;

            if(is_dir_entry) {nfolders++; continue;}

            int lname = entry->d_namlen; //strlen(entry->d_name);

            if(lname >= 6 && !strncmp(&entry->d_name[lname - 6], ".66600", 6))
            {
                lname -= 6;
                if(lname > 222) {closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                memcpy(temp_string, entry->d_name, lname);
                temp_string[lname] = 0;

                // check multi-part file name length
                len_string = UTF8_to_UTF16((u8 *) temp_string, wstring);
                if(len_string > 222) {closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                // build size of .666xx files
                u64 size = s.st_size;
                nfiles++;

                // calc multi-part file size
                for(int n = 1; n < 100; n++)
                {
                    path[l] = 0;
                    memcpy(path + l, entry->d_name, lname);

                    sprintf(&path[l + lname], ".666%2.2u", n);

                    if(stat(path, &s) != SUCCESS) {s.st_size = size; break;}

                    size += s.st_size;
                }

                path[l] = 0;
            }
            else if(lname >= 6 && !strncmp(&entry->d_name[lname - 6], ".666", 4)) continue; // ignore .666xx files

            else
            {
                if(lname > 222) {path[len] = 0; closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                // check file name length
                len_string = UTF8_to_UTF16((u8 *) entry->d_name, wstring);
                if(len_string > 222) {closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                nfiles++;
            }

            int parts = s.st_size ? (int) ((((u64) s.st_size) + 0xFFFFF7FFULL) / 0xFFFFF800ULL) : 1;

            for(int n = 0; n < parts; n++)
            {
                int add;

                add = sizeof(struct iso_directory_record) + lname - 1 + 8; // add ";1"
                add+= add & 1;
                cldir += add;

                if(cldir > SECTOR_SIZE)
                {
                    ldir = (ldir & ~SECTOR_FILL) + SECTOR_SIZE;
                    cldir = add;
                }
                else if(cldir == SECTOR_SIZE)
                {
                    cldir = 0;
                }

                ldir += add;
                //ldir += ldir & 1;

                add = sizeof(struct iso_directory_record) + len_string * 2 + 9; //- 1 + 4 + 6;  // add "\0;\01"
                add+= (add & 1);
                cwdir += add;

                if(cwdir > SECTOR_SIZE)
                {
                    wdir = (wdir & ~SECTOR_FILL) + SECTOR_SIZE;
                    cwdir = add;
                }
                else if(cwdir == SECTOR_SIZE)
                {
                    cwdir = 0;
                }

                wdir += add;
                //wdir += wdir & 1;
            }
        }

        closedir (dir);
        path[len] = 0;

        // directories
        if(nfolders > 0)
        {
            mfolders = nfolders; nentries = 0; progress = 100;

            #ifdef NOPS3_UPDATE
            if(len >= 11 && strstr(path + len - 11, "/PS3_UPDATE")) dir = NULL; else
            #endif
            dir = opendir(path);

            if(dir)
            {
                int l = len + 1;
                strcat(path + len, "/");

                while(true)
                {
                    struct dirent *entry = readdir (dir);

                    if(!entry || !entry->d_name[0]) break;
                    if(entry->d_name[0] == '.' && (entry->d_name[1] == '.' || entry->d_name[1] == 0)) continue;

                    if(get_input_abort()) {closedir(dir); return ERROR_ABORTED_BY_USER;}

                    nentries++;
                    if(nentries >= progress) {DPrintf("."); pcount++; progress = nentries + 100;}

                    path[l] = 0;
                    strcat(path + l, entry->d_name);

                    is_dir_entry = entry->d_type & IS_DIRECTORY;

                    if(is_dir_entry)
                    {
                        int lname = entry->d_namlen; //strlen(entry->d_name);
                        if(lname > 222) {closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                        // check directory name length
                        len_string = UTF8_to_UTF16((u8 *) entry->d_name, wstring);
                        if(len_string > 222) {closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                        lpath += sizeof(struct iso_path_table) + lname - 1;
                        lpath += (lpath & 1);

                        int add;

                        add = sizeof(struct iso_directory_record) + lname + 5; //- 1 + 6;
                        add+= add & 1;
                        cldir += add;

                        if(cldir > SECTOR_SIZE)
                        {
                            ldir = (ldir & ~SECTOR_FILL) + SECTOR_SIZE;
                            cldir = add;
                        }
                        else if(cldir == SECTOR_SIZE)
                        {
                            cldir = 0;
                        }

                        ldir += add;
                        //ldir += ldir & 1;

                        wpath += sizeof(struct iso_path_table) + len_string * 2 - 1;
                        wpath += (wpath & 1);

                        add = sizeof(struct iso_directory_record) + len_string * 2 + 5; //- 1 + 6;
                        add+= add & 1;
                        cwdir += add;

                        if(cwdir > SECTOR_SIZE)
                        {
                            wdir= (wdir & ~SECTOR_FILL) + SECTOR_SIZE;
                            cwdir = add;
                        }
                        else if(cwdir == SECTOR_SIZE)
                        {
                            cwdir = 0;
                        }

                        wdir += add;
                        //wdir += wdir & 1;

                        mfolders--;
                        if(mfolders <= 0) break;
                    }
                }

                closedir (dir);
            }
            path[len] = 0;


            directory_iso[cur].ldir = (ldir + SECTOR_FILL) / SECTOR_SIZE;
            directory_iso[cur].wdir = (wdir + SECTOR_FILL) / SECTOR_SIZE;


            // directories (recursive add)
            #ifdef NOPS3_UPDATE
            if(len >= 11 && strstr(path + len - 11, "/PS3_UPDATE")) dir = NULL; else
            #endif
            dir = opendir(path);

            if(dir)
            {
                mfolders = nfolders; nentries = 0; progress = 100; pcount = 0;

                int l = len + 1;
                strcat(path + len, "/");

                while(true)
                {
                    struct dirent *entry = readdir(dir);

                    if(!entry || !entry->d_name[0]) break;
                    if(entry->d_name[0] == '.' && (entry->d_name[1] == '.' || entry->d_name[1] == 0)) continue;

                    if(get_input_abort()) {closedir(dir); return ERROR_ABORTED_BY_USER;}

                    nentries++;
                    if(nentries >= progress) {DPrintf("."); pcount++; progress = nentries + 100;}

                    path[l] = 0;
                    strcat(path + l, entry->d_name);

                    is_dir_entry = entry->d_type & IS_DIRECTORY;

                    if(!is_dir_entry) continue;

                    DPrintf("%s\n", path);

                    directory_iso[cur_isop].name = malloc(entry->d_namlen + 1);
                    if(!directory_iso[cur_isop].name) {closedir(dir); return FAILED;}
                    strcpy(directory_iso[cur_isop].name, entry->d_name);

                    int ret = calc_entries(path, cur2 + 1);

                    if(ret < 0) {closedir(dir); return ret;}

                    mfolders--;
                    if(mfolders <= 0) break;
                }
                closedir (dir);
            }
            path[len] = 0;
        }
        else
        {
            directory_iso[cur].ldir = (ldir + SECTOR_FILL) / SECTOR_SIZE;
            directory_iso[cur].wdir = (wdir + SECTOR_FILL) / SECTOR_SIZE;
        }
    }

    if(pcount) DPrintf(".\n");

    if(cur == 0)
    {
        llba0 = 20;
        llba1 = llba0 + ((lpath + SECTOR_FILL) / SECTOR_SIZE);
        wlba0 = llba1 + ((lpath + SECTOR_FILL) / SECTOR_SIZE);
        wlba1 = wlba0 + ((wpath + SECTOR_FILL) / SECTOR_SIZE);
        dllba = wlba1 + ((wpath + SECTOR_FILL) / SECTOR_SIZE);
        if(dllba < 32) dllba = 32;

        int n;

        int m, l;

        // searching...

        for(n = 1; n < cur_isop - 1; n++)
            for(m = n + 1; m < cur_isop; m++)
            {
                if(directory_iso[n].parent > directory_iso[m].parent) {
                    directory_iso[cur_isop] = directory_iso[n]; directory_iso[n] = directory_iso[m]; directory_iso[m] = directory_iso[cur_isop];
                    for(l = n; l < cur_isop; l++)
                    {
                        if(n + 1 == directory_iso[l].parent)
                            directory_iso[l].parent = m + 1;
                        else if(m + 1 == directory_iso[l].parent)
                            directory_iso[l].parent = n + 1;
                    }
                }
            }

        for(n = 0; n < cur_isop; n++)
        {
            dlsz += directory_iso[n].ldir;
            dwsz += directory_iso[n].wdir;
        }

        #ifdef ALIGNED32SECTORS
        dwlba = ((dllba + dlsz) + 31) & ~31;

        flba = ((dwlba + dwsz) + 31) & ~31;
        #else
        dwlba = (dllba + dlsz);
        flba = (dwlba + dwsz);
        #endif

        u32 lba0 = dllba;
        u32 lba1 = dwlba;

        for(n = 0; n < cur_isop; n++)
        {
            directory_iso[n].llba = lba0;
            directory_iso[n].wlba = lba1;
            lba0 += directory_iso[n].ldir;
            lba1 += directory_iso[n].wdir;

        }
    }

    return SUCCESS;
}

static int fill_dirpath(void)
{
    int n;
    struct iso_path_table *iptl0;
    struct iso_path_table *iptl1;
    struct iso_path_table *iptw0;
    struct iso_path_table *iptw1;

    for(n = 0; n < cur_isop; n++)
    {
        iptl0 = (void *) &sectors[pos_lpath0];
        iptl1 = (void *) &sectors[pos_lpath1];
        iptw0 = (void *) &sectors[pos_wpath0];
        iptw1 = (void *) &sectors[pos_wpath1];

        if(!n)
        {
            set721((void *) iptl0->name_len, 1);
            set731((void *) iptl0->extent, directory_iso[n].llba);
            set721((void *) iptl0->parent, directory_iso[n].parent);
            iptl0->name[0] = 0;
            pos_lpath0 += sizeof(struct iso_path_table) - 1 + 1;
            pos_lpath0 += pos_lpath0 & 1;
            iptl0 = (void *) &sectors[pos_lpath0];

            set721((void *) iptl1->name_len, 1);
            set732((void *) iptl1->extent, directory_iso[n].llba);
            set722((void *) iptl1->parent, directory_iso[n].parent);
            iptl1->name[0] = 0;
            pos_lpath1 += sizeof(struct iso_path_table) - 1 + 1;
            pos_lpath1 += pos_lpath1 & 1;
            iptl1 = (void *) &sectors[pos_lpath1];

            set721((void *) iptw0->name_len, 1);
            set731((void *) iptw0->extent, directory_iso[n].wlba);
            set721((void *) iptw0->parent, directory_iso[n].parent);
            iptw0->name[0] = 0;
            pos_wpath0 += sizeof(struct iso_path_table) - 1 + 1;
            pos_wpath0 += pos_wpath0 & 1;
            iptw0 = (void *) &sectors[pos_wpath0];

            set721((void *) iptw1->name_len, 1);
            set732((void *) iptw1->extent, directory_iso[n].wlba);
            set722((void *) iptw1->parent, directory_iso[n].parent);
            iptw1->name[0] = 0;
            pos_wpath1 += sizeof(struct iso_path_table) - 1 + 1;
            pos_wpath1 += pos_wpath1 & 1;
            iptw1 = (void *) &sectors[pos_wpath1];
            continue;

        }

        //////
        int len_string;
        len_string = UTF8_to_UTF16((u8 *) directory_iso[n].name, wstring);

        set721((void *) iptl0->name_len, len_string);
        set731((void *) iptl0->extent, directory_iso[n].llba);
        set721((void *) iptl0->parent, directory_iso[n].parent);
        memcapscpy(&iptl0->name[0], directory_iso[n].name, len_string);
        pos_lpath0 += sizeof(struct iso_path_table) - 1 + len_string;
        pos_lpath0 += pos_lpath0 & 1;
        iptl0 = (void *) &sectors[pos_lpath0];

        set721((void *) iptl1->name_len, len_string);
        set732((void *) iptl1->extent, directory_iso[n].llba);
        set722((void *) iptl1->parent, directory_iso[n].parent);
        memcapscpy(&iptl1->name[0], directory_iso[n].name, len_string);
        pos_lpath1 += sizeof(struct iso_path_table) - 1 + len_string;
        pos_lpath1 += pos_lpath1 & 1;
        iptl1 = (void *) &sectors[pos_lpath1];

        len_string *= 2;
        set721((void *) iptw0->name_len, len_string);
        set731((void *) iptw0->extent, directory_iso[n].wlba);
        set721((void *) iptw0->parent, directory_iso[n].parent);
        memcpy(&iptw0->name[0], wstring, len_string);
        pos_wpath0 += sizeof(struct iso_path_table) - 1 + len_string;
        pos_wpath0 += pos_wpath0 & 1;
        iptw0 = (void *) &sectors[pos_wpath0];

        set721((void *) iptw1->name_len, len_string);
        set732((void *) iptw1->extent, directory_iso[n].wlba);
        set722((void *) iptw1->parent, directory_iso[n].parent);
        memcpy(&iptw1->name[0], wstring, len_string);
        pos_wpath1 += sizeof(struct iso_path_table) - 1 + len_string;
        pos_wpath1 += pos_wpath1 & 1;
        iptw1 = (void *) &sectors[pos_wpath1];

        //////

    }

    return SUCCESS;
}

static int fill_entries(char *path1, char *path2, int level)
{
    DIR  *dir;

    int n;
    int len_string;

    int len1 = strlen(path1);
    int len2 = strlen(path2);

    struct iso_directory_record *idrl = (void *) &sectors[directory_iso[level].llba * SECTOR_SIZE];
    struct iso_directory_record *idrw = (void *) &sectors[directory_iso[level].wlba * SECTOR_SIZE];
    struct iso_directory_record *idrl0 = idrl;
    struct iso_directory_record *idrw0 = idrw;


    struct tm *tm;
    struct stat s;

    memset((void *) idrl, 0, SECTOR_SIZE);
    memset((void *) idrw, 0, SECTOR_SIZE);

    u32 count_sec1 = 1, count_sec2 = 1, max_sec1, max_sec2;

    int first_file = 1;

    int aux_parent = directory_iso[level].parent - 1;


    if(level != 0)
    {
        strcat(path2 + len2, "/");
        strcat(path2 + len2, directory_iso[level].name);
        strcat(path1 + len1, path2);
    }
    else
    {
        path2[0] = 0; fill_dirpath();
    }


    //DPrintf("q %s LBA %X\n", path2, directory_iso[level].llba);

    if(stat(path1, &s) != SUCCESS) {DPrintf("Not found: %s\n", path1); return ERROR_INPUT_FILE_NOT_EXISTS;}

    tm = localtime(&s.st_mtime);
    dd = tm->tm_mday;
    mm = tm->tm_mon + 1;
    aa = tm->tm_year + 1900;
    ho = tm->tm_hour;
    mi = tm->tm_min;
    se = tm->tm_sec;

    idrl->length[0] = sizeof(struct iso_directory_record) + 6;
    idrl->length[0] += idrl->length[0] & 1;
    idrl->ext_attr_length[0] = 0;
    set733((void *) idrl->extent, directory_iso[level].llba);
    set733((void *) idrl->size, directory_iso[level].ldir * SECTOR_SIZE);
    setdaterec(idrl->date, dd, mm, aa, ho, mi, se);
    idrl->flags[0] = 0x2;
    idrl->file_unit_size[0] = 0x0;
    idrl->interleave[0] = 0x0;
    set723(idrl->volume_sequence_number, 1);
    idrl->name_len[0] = 1;
    idrl->name[0] = 0;
    idrl = (void *) ((char *) idrl) + idrl->length[0];

    max_sec1 = directory_iso[level].ldir;

    idrw->length[0] = sizeof(struct iso_directory_record) + 6;
    idrw->length[0] += idrw->length[0] & 1;
    idrw->ext_attr_length[0] = 0;
    set733((void *) idrw->extent, directory_iso[level].wlba);
    set733((void *) idrw->size, directory_iso[level].wdir * SECTOR_SIZE);
    setdaterec(idrw->date, dd, mm, aa, ho, mi, se);
    idrw->flags[0] = 0x2;
    idrw->file_unit_size[0] = 0x0;
    idrw->interleave[0] = 0x0;
    set723(idrw->volume_sequence_number, 1);
    idrw->name_len[0] = 1;
    idrw->name[0] = 0;
    idrw = (void *) ((char *) idrw) + idrw->length[0];

    max_sec2 = directory_iso[level].wdir;

    if(level)
    {
        int len = strlen(path1);
        strcat(path1 + len, "/..");
        if(stat(path1, &s) != SUCCESS) {DPrintf("Not found: %s\n", path1); return ERROR_INPUT_FILE_NOT_EXISTS;}
        path1[len] = 0;

        tm = localtime(&s.st_mtime);
        dd = tm->tm_mday;
        mm = tm->tm_mon + 1;
        aa = tm->tm_year + 1900;
        ho = tm->tm_hour;
        mi = tm->tm_min;
        se = tm->tm_sec;
    }

    idrl->length[0] = sizeof(struct iso_directory_record) + 6;
    idrl->length[0] += idrl->length[0] & 1;
    idrl->ext_attr_length[0] = 0;
    set733((void *) idrl->extent, directory_iso[!level ? 0 : aux_parent].llba);
    set733((void *) idrl->size, directory_iso[!level ? 0 : aux_parent].ldir * SECTOR_SIZE);
    setdaterec(idrl->date, dd, mm, aa, ho, mi, se);
    idrl->flags[0] = 0x2;
    idrl->file_unit_size[0] = 0x0;
    idrl->interleave[0] = 0x0;
    set723(idrl->volume_sequence_number, 1);
    idrl->name_len[0] = 1;
    idrl->name[0] = 1;
    idrl = (void *) ((char *) idrl) + idrl->length[0];

    idrw->length[0] = sizeof(struct iso_directory_record) + 6;
    idrw->length[0] += idrw->length[0] & 1;
    idrw->ext_attr_length[0] = 0;
    set733((void *) idrw->extent, directory_iso[!level ? 0 : aux_parent].wlba);
    set733((void *) idrw->size, directory_iso[!level ? 0 : aux_parent].wdir * SECTOR_SIZE);
    setdaterec(idrw->date, dd, mm, aa, ho, mi, se);
    idrw->flags[0] = 0x2;
    idrw->file_unit_size[0] = 0x0;
    idrw->interleave[0] = 0x0;
    set723(idrw->volume_sequence_number, 1);
    idrw->name_len[0] = 1;
    idrw->name[0] = 1;
    idrw = (void *) ((char *) idrw) + idrw->length[0];

    int len = strlen(path1);
    DPrintf("%s\n", path1);

    // files
    #ifdef NOPS3_UPDATE
    if(len >= 11 && strstr(path1 + len - 11, "/PS3_UPDATE")) dir = NULL; else
    #endif
    dir = opendir(path1);

    if(dir)
    {
        int l = len + 1;
        strcat(path1 + len, "/");

        while(true)
        {
            struct dirent *entry = readdir (dir);

            if(!entry || !entry->d_name[0]) break;
            if(entry->d_name[0] == '.' && (entry->d_name[1] == '.' || entry->d_name[1] == 0)) continue;

            if(get_input_abort()) {closedir(dir); return ERROR_ABORTED_BY_USER;}

            path1[l] = 0;
            strcat(path1 + l, entry->d_name);

            if(stat(path1, &s) != SUCCESS) continue; //{closedir(dir); DPrintf("Not found: %s\n", path1); path1[len] = 0; return ERROR_INPUT_FILE_NOT_EXISTS;}

            if(S_ISDIR(s.st_mode)) continue;

            int lname = entry->d_namlen; //strlen(entry->d_name);

            if(lname >= 6 && !strcmp(&entry->d_name[lname -6], ".66600"))
            {
                lname -= 6;
                if(lname > 222) {closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                memcpy(temp_string, entry->d_name, lname);
                temp_string[lname] = 0;

                // check multi-part file name length
                len_string = UTF8_to_UTF16((u8 *) temp_string, wstring);
                if(len_string > 222) {closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                // build size of .666xx files
                u64 size = s.st_size;

                // calc multi-part file size
                for(int n = 1; n < 100; n++)
                {
                    path1[l] = 0;
                    memcpy(path1 + l, entry->d_name, lname);

                    sprintf(&path1[l + lname], ".666%2.2u", n);

                    if(stat(path1, &s) != SUCCESS) {s.st_size = size; break;}

                    size += s.st_size;
                }

                path1[len] = 0;
            }
            else if(lname >= 6 && !strncmp(&entry->d_name[lname -6], ".666", 4)) continue; // ignore .666xx files

            else
            {
                if(lname > 222) {path1[len] = 0; closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                // check file name length
                len_string = UTF8_to_UTF16((u8 *) entry->d_name, wstring);
                if(len_string > 222) {closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}
            }


            #ifdef ALIGNED32SECTORS
            if(first_file) flba = (flba + 31) & ~31;
            #endif

            first_file = 0;

            int parts = s.st_size ? (u32) ((((u64) s.st_size) + 0xFFFFF7FFULL)/0xFFFFF800ULL) : 1;

            int n;

            tm = gmtime(&s.st_mtime);
            time_t t = mktime(tm);
            tm = localtime(&t);
            dd = tm->tm_mday;
            mm = tm->tm_mon + 1;
            aa = tm->tm_year + 1900;
            ho = tm->tm_hour;
            mi = tm->tm_min;
            se = tm->tm_sec;

            for(n = 0; n < parts; n++)
            {
                u32 fsize;
                if(parts > 1 && (n + 1) != parts) {fsize = 0xFFFFF800; s.st_size-= fsize;}
                else fsize = s.st_size;

                int add;

                add = sizeof(struct iso_directory_record) - 1 + (lname + 8);
                add+= (add & 1);

                // align entry data with sector

                int cldir = (((int) (s64)idrl) - ((int) (s64)idrl0)) & SECTOR_FILL;

                cldir += add;

                if(cldir > SECTOR_SIZE)
                {
                    //DPrintf("gapl1 lba 0x%X %s/%s %i\n", directory_iso[level].llba, path1, entry->d_name, cldir);
                    //getchar();

                    count_sec1++;
                    if(count_sec1 > max_sec1)
                    {
                        closedir (dir);
                        DPrintf("ERROR: too much entries in directory:\n%s\n", path1);
                        return ERROR_TOO_MUCH_DIR_ENTRIES;
                    }

                    idrl = (void *) ((char *) idrl) + (add - (cldir - SECTOR_SIZE));

                    memset((void *) idrl, 0, SECTOR_SIZE);
                }

                idrl->length[0] = add;
                idrl->length[0] += idrl->length[0] & 1;
                idrl->ext_attr_length[0] = 0;
                set733((void *) idrl->extent, flba);
                set733((void *) idrl->size, fsize);

                //DPrintf("a %i/%i/%i %i:%i:%i %s LBA: %X\n", dd, mm, aa, ho, mi, se, entry->d_name, flba);
                setdaterec(idrl->date, dd, mm, aa, ho, mi, se);
                idrl->flags[0] = ((n + 1) != parts) ? 0x80 : 0x0; // fichero
                idrl->file_unit_size[0] = 0x0;
                idrl->interleave[0] = 0x0;
                set723(idrl->volume_sequence_number, 1);
                idrl->name_len[0] = lname + 2;
                memcapscpy(idrl->name, entry->d_name, lname);
                idrl->name[lname + 0] = ';';
                idrl->name[lname + 1] = '1';
                idrl = (void *) ((char *) idrl) + idrl->length[0];

                //

                add = sizeof(struct iso_directory_record) + len_string * 2 + 9; //- 1 + 4 + 6;
                add+= (add & 1);

                // align entry data with sector

                int cwdir = (((int) (s64)idrw) - ((int) (s64)idrw0)) & SECTOR_FILL;

                cwdir += add;

                if(cwdir > SECTOR_SIZE)
                {
                    //DPrintf("gapw1 lba 0x%X %s/%s %i\n", directory_iso[level].wlba, path1, entry->d_name, cwdir);
                    //getchar();

                    count_sec2++;
                    if(count_sec2 > max_sec2)
                    {
                        closedir (dir);
                        DPrintf("ERROR: too much entries in directory:\n%s\n", path1);
                        return ERROR_TOO_MUCH_DIR_ENTRIES;
                    }

                    idrw = (void *) ((char *) idrw) + (add - (cwdir - SECTOR_SIZE));

                    memset((void *) idrw, 0, SECTOR_SIZE);
                }

                idrw->length[0] = add;
                idrw->length[0] += idrw->length[0] & 1;
                idrw->ext_attr_length[0] = 0;
                set733((void *) idrw->extent, flba);
                set733((void *) idrw->size, fsize);

                setdaterec(idrw->date, dd, mm, aa, ho, mi, se);
                idrw->flags[0] = ((n + 1) != parts) ? 0x80 : 0x0; // fichero
                idrw->file_unit_size[0] = 0x0;
                idrw->interleave[0] = 0x0;
                set723(idrw->volume_sequence_number, 1);
                idrw->name_len[0] = len_string * 2 + 4;
                memcpy(idrw->name, wstring, len_string * 2);
                idrw->name[len_string * 2 + 0] = 0;
                idrw->name[len_string * 2 + 1] = ';';
                idrw->name[len_string * 2 + 2] = 0;
                idrw->name[len_string * 2 + 3] = '1';
                idrw = (void *) ((char *) idrw) + idrw->length[0];

                flba+= ((fsize + SECTOR_FILL) & ~SECTOR_FILL) / SECTOR_SIZE;
            }

        }

        closedir (dir);
    }
    path1[len] = 0;


    int l = len + 1;
    strcat(path1 + len, "/");

    // folders
    for(n = 1; n < cur_isop; n++)
        if(directory_iso[n].parent == level + 1)
        {
            len_string = sprintf(path1 + l, "%s", directory_iso[n].name);

            if(stat(path1, &s) != SUCCESS) {DPrintf("Not found: %s\n", path1); path1[len] = 0; return ERROR_INPUT_FILE_NOT_EXISTS;}

            tm = localtime(&s.st_mtime);
            dd = tm->tm_mday;
            mm = tm->tm_mon + 1;
            aa = tm->tm_year + 1900;
            ho = tm->tm_hour;
            mi = tm->tm_min;
            se = tm->tm_sec;

            //DPrintf("dir %i/%i/%i %i:%i:%i %s\n", dd, mm, aa, ho, mi, se, directory_iso[n].name);

            int add;

            add = sizeof(struct iso_directory_record) + len_string + 5; //- 1 + 6;
            add+= (add & 1);

            // align entry data with sector

            int cldir = (((int)(s64)idrl) - (int) (s64)idrl0) & SECTOR_FILL;

            cldir += add;

            if(cldir > SECTOR_SIZE)
            {
                //DPrintf("gapl0 lba 0x%X %s/%s %i\n", directory_iso[level].llba, path1, directory_iso[n].name, cldir);
                //getchar();

                count_sec1++;
                if(count_sec1 > max_sec1)
                {
                    closedir (dir);
                    DPrintf("ERROR: too much entries in directory:\n%s\n", path1);
                    return ERROR_TOO_MUCH_DIR_ENTRIES;
                }

                idrl = (void *) ((char *) idrl) + (add - (cldir - SECTOR_SIZE));

                memset((void *) idrl, 0, SECTOR_SIZE);
            }

            idrl->length[0] = add;
            idrl->length[0] += idrl->length[0] & 1;
            idrl->ext_attr_length[0] = 0;
            set733((void *) idrl->extent, directory_iso[n].llba);
            set733((void *) idrl->size, directory_iso[n].ldir * SECTOR_SIZE);
            setdaterec(idrl->date, dd, mm, aa, ho, mi, se);
            idrl->flags[0] = 0x2;
            idrl->file_unit_size[0] = 0x0;
            idrl->interleave[0] = 0x0;
            set723(idrl->volume_sequence_number, 1);
            idrl->name_len[0] = len_string;
            memcapscpy(idrl->name, directory_iso[n].name, len_string);
            idrl = (void *) ((char *) idrl) + idrl->length[0];

            //
            len_string = UTF8_to_UTF16((u8 *) directory_iso[n].name, wstring);

            add = sizeof(struct iso_directory_record) + len_string * 2 + 5; //- 1 + 6;
            add+= (add & 1);

            // align entry data with sector

            int cwdir = (((int) (s64)idrw) - ((int) (s64)idrw0)) & SECTOR_FILL;

            cwdir += add;

            if(cwdir > SECTOR_SIZE)
            {
                //DPrintf("gapw0 lba 0x%X %s/%s %i\n", directory_iso[level].wlba, path1, directory_iso[n].name, cwdir);
                //getchar();

                count_sec2++;
                if(count_sec2 > max_sec2)
                {
                    closedir (dir);
                    DPrintf("ERROR: too much entries in directory:\n%s\n", path1);
                    return ERROR_TOO_MUCH_DIR_ENTRIES;
                }

                idrw = (void *) ((char *) idrw) + (add - (cwdir - SECTOR_SIZE));

                memset((void *) idrw, 0, SECTOR_SIZE);
            }

            idrw->length[0] = add;
            idrw->length[0] += idrw->length[0] & 1;
            idrw->ext_attr_length[0] = 0;
            set733((void *) idrw->extent, directory_iso[n].wlba);
            set733((void *) idrw->size, directory_iso[n].wdir * SECTOR_SIZE);
            setdaterec(idrw->date, dd, mm, aa, ho, mi, se);
            idrw->flags[0] = 0x2;
            idrw->file_unit_size[0] = 0x0;
            idrw->interleave[0] = 0x0;
            set723(idrw->volume_sequence_number, 1);
            idrw->name_len[0] = len_string * 2;
            memcpy(idrw->name, wstring, len_string * 2);
            idrw = (void *) ((char *) idrw) + idrw->length[0];

    }

    path1[len1] = 0;

    // iteration
    for(n = 1; n < cur_isop; n++)
    {
        if(directory_iso[n].parent == level + 1)
        {
            int ret = fill_entries(path1, path2, n);
            if(ret < 0) {path2[len2] = 0; return ret;}
        }
    }

    path2[len2] = 0;

    return SUCCESS;

}

#define SPLIT_LBA 0x1FFFE0


static int write_split0(int *fd, u32 lba, u8 *mem, int sectors, int sel)
{
    char filename[0x420];

    if(!iso_split)
    {
        if(ps3ntfs_write(*fd, (void *) mem, (int) sectors * SECTOR_SIZE) != sectors * SECTOR_SIZE) return ERROR_WRITING_OUTPUT_FILE;
        return SUCCESS;
    }

    int cur = lba / SPLIT_LBA;
    int cur2 = (lba + sectors) / SPLIT_LBA;

    if(cur == cur2 && (iso_split - 1) == cur)
    {
        if(ps3ntfs_write(*fd, (void *) mem, (int) sectors * SECTOR_SIZE) != sectors * SECTOR_SIZE) return ERROR_WRITING_OUTPUT_FILE;
        return SUCCESS;
    }

    u32 lba2 = lba + sectors;
    u32 pos = 0;

    for(; lba < lba2; lba++)
    {
        int cur = lba / SPLIT_LBA;

        if(iso_split - 1 != cur)
        {
            if (*fd >= 0) ps3ntfs_close(*fd); *fd = -1;

            if(sel == 0) return SUCCESS;

            if(iso_split == 1)
            {
                sprintf(filename, "%s.0", output_name);
                ps3ntfs_unlink(filename);
                rename(output_name, filename);
            }

            iso_split = cur + 1;

            sprintf(filename, "%s.%i", output_name, iso_split - 1);

            *fd = ps3ntfs_open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0766);
            if(*fd < 0) return ERROR_WRITING_OUTPUT_FILE;
        }

        if(ps3ntfs_write(*fd, (void *) mem + pos, SECTOR_SIZE) != SECTOR_SIZE) return ERROR_WRITING_OUTPUT_FILE;
        pos += SECTOR_SIZE;
    }

    return SUCCESS;
}

#include "event_threads.h"


static volatile struct f_async {
    int flags;
    int *fd;
    void * mem;
    u32 ret;
    u32 lba;
    u32 nsectors;
    int sel;

} my_f_async;

#define ASYNC_ENABLE 128
#define ASYNC_ERROR 16

static void my_func_async(struct f_async * v)
{
    int ret = -1;

    if(v && v->flags & ASYNC_ENABLE)
    {
        v->ret = -1;
        int flags = v->flags;
        if(v->mem)
        {
             ret = write_split0(v->fd, v->lba, (u8 *) v->mem, v->nsectors, v->sel);
             v->ret = ret;

            free(v->mem); v->mem = 0;
        }

        if(ret) flags|= ASYNC_ERROR;

        flags &= ~ASYNC_ENABLE;

        v->flags = flags;
    }
}
static int use_async_fd = 128;

static int write_split(int *fd, u32 lba, u8 *mem, int sectors, int sel)
{
    loop_write:

    if(use_async_fd == 128)
    {
        use_async_fd = 1;
        my_f_async.flags = 0;
        my_f_async.sel = sel;
        my_f_async.lba = lba;
        my_f_async.nsectors = sectors;
        my_f_async.fd = fd;
        my_f_async.mem = malloc(sectors * SECTOR_SIZE);
        if(my_f_async.mem) memcpy(my_f_async.mem, (void *) mem, sectors * SECTOR_SIZE);
        my_f_async.ret = -1;
        my_f_async.flags = ASYNC_ENABLE;
        event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
    }
    else
    {

        if(!(my_f_async.flags & ASYNC_ENABLE))
        {

            if(my_f_async.flags & ASYNC_ERROR)
            {
                return my_f_async.ret;
            }

            my_f_async.flags = 0;
            my_f_async.sel = sel;
            my_f_async.lba = lba;
            my_f_async.nsectors = sectors;
            my_f_async.fd = fd;
            my_f_async.mem = malloc(sectors * SECTOR_SIZE);
            if(my_f_async.mem) memcpy(my_f_async.mem, (void *) mem, sectors * SECTOR_SIZE);
            my_f_async.ret = -1;
            my_f_async.flags = ASYNC_ENABLE;
            event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
        }
        else
        {
            //wait_event_thread();
            goto loop_write;
        }
    }

    return SUCCESS;
}

static int build_file_iso(int *fd, char *path1, char *path2, int level)
{
    DIR  *dir;
    struct stat s;
    struct stat z;

    int n;
    int first_file = 1;
    int len1 = strlen(path1);
    int len2 = strlen(path2);

    stat(output_name, &z);
    sprintf(dbhead, "%s - done %2.1f %%", "MAKEPS3ISO Utility", (double)((double)z.st_size * 100.0f / (double)(toc * 2048ULL)));

    DbgHeader(dbhead);

    if(level != 0)
    {
        strcat(path2 + len2, "/");
        strcat(path2 + len2, directory_iso[level].name);
        strcat(path1 + len1, path2);
    }
    else
        path2[0] = 0;

    if(level)
        DPrintf("</%s>\n", directory_iso[level].name);
    else
        DPrintf("</>\n");

    //DPrintf("q %s LBA %X\n", path2, directory_iso[level].llba);

    int len = strlen(path1);

    // files
    #ifdef NOPS3_UPDATE
    if(len >= 11 && strstr(path1 + len - 11, "/PS3_UPDATE")) dir = NULL; else
    #endif
    dir = opendir(path1);

    if(dir)
    {
        int l = len + 1;
        strcat(path1 + len, "/");

        while(true)
        {
            struct dirent *entry = readdir (dir);

            if(!entry || !entry->d_name[0]) break;
            if(entry->d_name[0] == '.' && (entry->d_name[1] == '.' || entry->d_name[1] == 0)) continue;

            if(get_input_abort()) {closedir(dir); return ERROR_ABORTED_BY_USER;}

            path1[l] = 0;
            strcat(path1 + l, entry->d_name);

            if(stat(path1, &s) != SUCCESS) continue; //{closedir(dir); DPrintf("Not found: %s\n", path1); path1[len] = 0; return ERROR_INPUT_FILE_NOT_EXISTS;}

            if(S_ISDIR(s.st_mode)) continue;

            int is_file_split = 0;

            int lname = entry->d_namlen; //strlen(entry->d_name);

            if(lname >= 6 && !strcmp(&entry->d_name[lname -6], ".66600"))
            {
                lname -= 6;
                if(lname > 222) {path1[len] = 0; closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}

                is_file_split = 1;

                memcpy(temp_string, entry->d_name, lname);
                temp_string[lname] = 0;

                // build size of .666xx files
                u64 size = s.st_size;

                // calc multi-part file size
                for(int n = 1; n < 100; n++)
                {
                    path1[l] = 0;
                    memcpy(path1 + l, entry->d_name, lname);

                    sprintf(&path1[l + lname], ".666%2.2u", n);

                    if(stat(path1, &s) != SUCCESS) {s.st_size = size; break;}

                    size += s.st_size;
                }

                path1[l] = 0;
                strcat(path1 + l, entry->d_name); // restore .66600 file
            }
            else if(lname >= 6 && !strncmp(&entry->d_name[lname -6], ".666", 4)) continue; // ignore .666xx files

            else
            {
                if(lname > 222) {path1[len] = 0; closedir(dir); return ERROR_FILE_NAME_TOO_LONG;}
            }


            int  fd2 = ps3ntfs_open(path1, O_RDONLY, 0766);
            path1[l] = 0;

            if(fd2 < 0) {closedir(dir); return ERROR_OPENING_INPUT_FILE;}

            u32 flba0 = flba;

            #ifdef ALIGNED32SECTORS
            if(first_file) flba= (flba + 31) & ~31;
            #endif

            first_file = 0;

            if(flba0 != flba)
            {
                //DPrintf("gap: %i\n", (flba - flba0));

                int f = (flba - flba0);
                int f2 = 0;
                int z = 0;

                memset(sectors, 0, ((f > 128) ? 128 : f ) * SECTOR_SIZE);

                while(f > 0)
                {
                    if(f > 128) f2 = 128; else f2 = f;

                    int ret = write_split(fd, flba + z, sectors, f2, 1);
                    if(ret < 0)
                    {
                        ps3ntfs_close(fd2); closedir(dir); return ret;
                    }

                    f -= f2;
                    z += f2;
                }
            }

            stat(output_name, &z);
            sprintf(dbhead, "%s - done %2.1f %%", "MAKEPS3ISO Utility", (double)((double)z.st_size * 100.0f / (double)(toc * 2048ULL)));

            DbgHeader(dbhead);

            if(is_file_split)
            {
                if(s.st_size < 1024ULL)
                    DPrintf("  -> %s LBA %u size %u Bytes\n", temp_string, flba, (u32) s.st_size);
                else if(s.st_size < 0x100000LL)
                    DPrintf("  -> %s LBA %u size %u KB\n", temp_string, flba, (u32) (s.st_size/1024));
                else
                    DPrintf("  -> %s LBA %u size %u MB\n", temp_string, flba, (u32) (s.st_size/0x100000LL));
            }
            else
            {
                if(s.st_size < 1024ULL)
                    DPrintf("  -> %s LBA %u size %u Bytes\n", entry->d_name, flba, (u32) s.st_size);
                else if(s.st_size < 0x100000LL)
                    DPrintf("  -> %s LBA %u size %u KB\n", entry->d_name, flba, (u32) (s.st_size/1024));
                else
                    DPrintf("  -> %s LBA %u size %u MB\n", entry->d_name, flba, (u32) (s.st_size/0x100000LL));
            }

            u32 count = 0, percent = (u32) (s.st_size / 0x40000ULL);
            if(percent == 0) percent = 1;

            int cx = con_x, cy = con_y;

            u64 t_one, t_two;

            t_one = get_ticks();

            if(get_input_abort())
            {
                ps3ntfs_close(fd2); closedir(dir); return ERROR_ABORTED_BY_USER;
            }

            while(s.st_size > 0)
            {
                u32 fsize;
                u32 lsize;

                con_x = cx; con_y = cy;

                t_two = get_ticks();

                if(((t_two - t_one) >= TICKS_PER_SEC/2))
                {
                    t_one = t_two;
                    DPrintf("*** Writing... %u %%", count * 100 / percent);
                    con_x = cx; con_y = cy;

                    if(get_input_abort())
                    {
                        ps3ntfs_close(fd2); closedir(dir); return ERROR_ABORTED_BY_USER;
                    }
                }

                if(s.st_size > 0x40000) fsize = 0x40000;
                else fsize = (u32) s.st_size;

                count++;

                if(fsize < 0x40000) memset(sectors, 0, 0x40000);

                if(is_file_split)
                {
                    int read = ps3ntfs_read(fd2, (void *) sectors, (int) fsize);
                    if(read < 0)
                    {
                        ps3ntfs_close(fd2); closedir(dir); return ERROR_READING_INPUT_FILE;
                    }
                    else if(read < fsize)
                    {
                        ps3ntfs_close(fd2);

                        path1[l] = 0;
                        memcpy(path1 + l, entry->d_name, lname);

                        sprintf(&path1[l + lname], ".666%2.2u", is_file_split);
                        //DPrintf("split: %s\n", path1);
                        is_file_split++;

                        fd2 = ps3ntfs_open(path1, O_RDONLY, 0766);
                        path1[len] = 0;

                        if(fd2 < 0) {closedir(dir); return ERROR_OPENING_INPUT_FILE;}

                        if(ps3ntfs_read(fd2, (void *) (sectors + read), (int) (fsize - read)) != (fsize - read))
                        {
                            ps3ntfs_close(fd2); closedir(dir); return ERROR_READING_INPUT_FILE;
                        }
                    }

                }
                else if(ps3ntfs_read(fd2, (void *) sectors, (int) fsize) != fsize)
                {
                    ps3ntfs_close(fd2); closedir(dir); return ERROR_READING_INPUT_FILE;
                }

                lsize = (fsize + SECTOR_FILL) & ~SECTOR_FILL;

                int ret = write_split(fd, flba, sectors, lsize / SECTOR_SIZE, 1);

                if(ret < 0)
                {
                    DPrintf("\n");
                    ps3ntfs_close(fd2); closedir(dir); return ret;
                }

                flba += lsize / SECTOR_SIZE;
                //DPrintf("flba %i\n", flba * SECTOR_SIZE);

                s.st_size-= fsize;
            }

            con_x = cx; con_y = cy;
            DPrintf("                             ");
            con_x = cx; con_y = cy;

            ps3ntfs_close(fd2);
        }

        closedir (dir);
    }

    path1[len1] = 0;

    // iteration
    for(n = 1; n < cur_isop; n++)
    {
        if(directory_iso[n].parent == level + 1)
        {
            int ret = build_file_iso(fd, path1, path2, n);
            if(ret < 0) {path2[len2] = 0; return ret;}
        }
    }

    path2[len2] = 0;

    if(level == 0)
    {
        int ret;

        if(toc != flba)
        {
            //DPrintf("End gap: %i\n", (toc - flba));

            int f = (toc - flba);
            int f2 = 0;
            int z = 0;

            memset(sectors, 0, ((f > 128) ? 128 : f ) * SECTOR_SIZE);

            while(f > 0)
            {
                if(f > 128) f2 = 128; else f2 = f;

                ret = write_split(fd, flba + z, sectors, f2, 1);

                if(ret < 0) return ret;

                f-= f2;
                z+= f2;
            }
        }

        ret = 0;

        if(my_f_async.flags & ASYNC_ENABLE){
            wait_event_thread();
            if(my_f_async.flags  & ASYNC_ERROR) {

                ret = my_f_async.ret;
            }
            my_f_async.flags = 0;
        }

        event_thread_send(0x555ULL, (u64) 0, 0);

        sprintf(dbhead, "%s - 100%c", "MAKEPS3ISO Utility", '%');

        DbgHeader(dbhead);

        return ret;
    }


    return SUCCESS;

}

void fixpath(char *p)
{
    u8 * pp = (u8 *) p;

    if(*p == '"')
    {
        p[strlen(p) -1] = 0;
        memcpy(p, p + 1, strlen(p));
    }

    #ifdef __CYGWIN__
    if(p[0]!=0 && p[1] == ':')
    {
        p[1] = p[0];
        memmove(p + 9, p, strlen(p) + 1);
        memcpy(p, "/cygdrive/", 10);
    }
    #endif

    while(*pp)
    {
        if(*pp == '"') {*pp = 0; break;}
        else
        if(*pp == '\\') *pp = '/';
        else
        if(*pp > 0 && *pp < 32) {*pp = 0; break;}
        pp++;
    }

}

void fixtitle(char *p)
{
    while(*p)
    {
        if(*p & 128) *p = 0;
        else
        if(*p == ':' || *p == '*' || *p == '?' || *p == '"' || *p == '<' || *p == '>' || *p == '|') *p = '_';
        else
        if(*p == '\\' || *p == '/') *p = '-';
        else
        if(((u8)*p) > 0 && ((u8)*p) < 32) {*p = 0; break;}
        p++;
    }
}

int makeps3iso(char *g_path, char *f_iso, int split)
{

    struct stat s;
    char path1[0x420];
    char path2[0x420];
    char title_id[64];
    u64 t_start, t_finish;

    int ask_del = 0;

    use_async_fd = 128;
    my_f_async.flags = 0;

    DCls();

    sprintf(dbhead, "MAKEPS3ISO Utility");

    DbgHeader(dbhead);

    DPrintf("\nMAKEPS3ISO (c) 2013, Estwald (Hermes)\n\n");

    strcpy(path1, g_path);

    // libc test
    if(sizeof(s.st_size) != 8)
    {
        DPrintf("ERROR: stat st_size must be a 64 bit number!  (size %i)\n", (int) sizeof(s.st_size));
        press_button_to_continue();
        return FAILED;
    }


    fixpath(path1);

    if(stat(path1, &s) < 0 || !(S_ISDIR(s.st_mode)))
    {
        DPrintf("ERROR: Invalid Path!\n");
        press_button_to_continue();
        return FAILED;
    }

    strcpy(path2, path1);
    strcat(path2, "/PS3_GAME/PARAM.SFO");

    if(iso_parse_param_sfo(path2, title_id, output_name) < 0)
    {
        if(strstr(path1, "/GAME") == NULL)
        {
            path2[64] = 0;
            strcpy(path2, strrchr(path1, '/') + 1);
            path2[40] = 0;
        }
        else
        {
            DPrintf("ERROR: PARAM.SFO not found!\n");
            press_button_to_continue();
            return FAILED;
        }
    }
    else
    {
        utf8_to_ansiname(output_name, path2, 32);
        path2[32]= 0;
        fixtitle(path2);
        strcat(path2, " [");
        strcat(path2, title_id);
        strcat(path2, "]");
    }

    if(f_iso) strcpy(output_name, f_iso); else output_name[0] = 0;


    fixpath(output_name);

    // create path for get free space from destination file
    char dest_path[0x420];

    strcpy(dest_path, output_name);

    u64 avail = get_disk_free_space(dest_path);

    int nlen = strlen(output_name);

    if(nlen < 1) {strcpy(output_name, path2);/*DPrintf("ISO name too short!\n"); press_button_to_continue(); return FAILED;*/}
    else
    {
         if(stat(output_name, &s) == 0 && (S_ISDIR(s.st_mode)))
         {
            strcat(output_name, "/"); strcat(output_name, path2);
         }
    }

    nlen = strlen(output_name);

    if(nlen < 4 || (strcmp(&output_name[nlen - 4], ".iso") && strcmp(&output_name[nlen - 4], ".ISO")))
    {
        strcat(output_name + nlen, ".iso");
    }


    if(!stat(output_name, &s))
    {
        DPrintf("\n%s\nFile Exists. Do you want to Overwrite it? (CROSS - Yes/ TRIANGLE - No):\n", output_name);

        if(get_input_char() <= 0) return FAILED;
    }
    else
        DPrintf("\nCreate ISO\n%s\n", output_name);

    strcpy(output_name2, output_name);


    if(split == 2)
    {
        DPrintf("\nSplit in 4GB parts? (CROSS - Yes/ TRIANGLE - No):\n");

        int c;
        c = get_input_char();

        if(c ==  1) iso_split = 1; else
        if(c == -1) iso_split = 0; else {cls(); tiny3d_Flip(); return FAILED;}

    }
    else
        iso_split = (split != 0);

    if(iso_split) DPrintf("YES - Using Split File Mode\n"); else DPrintf("NO - Using Single File Mode\n");


    t_start = get_ticks();

    cur_isop = 0;

    directory_iso = malloc((MAX_ISO_PATHS + 1) * sizeof(_directory_iso));

    if(!directory_iso)
    {
        DPrintf("Out of Memory (directory_iso mem)\n");
        press_button_to_continue();
        return FAILED;
    }

    memset(directory_iso, 0, (MAX_ISO_PATHS + 1) * sizeof(_directory_iso));

    lpath = sizeof(struct iso_path_table);
    wpath = sizeof(struct iso_path_table);

    llba0 = 0;
    llba1 = 0;
    wlba0 = 0;
    wlba1 = 0;

    dllba = 0; // dir entries
    dwlba = 0; // dir entriesw
    dlsz = 0; // dir entries size (sectors)
    dwsz = 0; // dir entriesw size (sectors)
    flba = 0; // first lba for files

    u32 flba2 = 0;

    int ret;

    DPrintf("\nDetermining size of directory entries... Please wait...\n\nNOTICE: This process may take several minutes if there is a large number of files.\n\n");

    nfiles = 0;
    ret = calc_entries(path1, 1);

    if(ret < 0 )
    {
        switch(ret)
        {
            case ERROR_OUT_OF_MEMORY:
                DPrintf("Out of Memory (calc_entries())\n");
                goto err;
            case ERROR_TOO_MUCH_DIR_ENTRIES:
                DPrintf("Too much folders (max %i) (calc_entries())\n", MAX_ISO_PATHS);
                goto err;
            case ERROR_FILE_NAME_TOO_LONG:
                DPrintf("Folder Name Too Long (calc_entries())\n");
                goto err;
            case ERROR_INPUT_FILE_NOT_EXISTS:
                DPrintf("Error Input File Not Exists (calc_entries())\n");
            case ERROR_OPENING_INPUT_FILE:
                DPrintf("Error Opening Input File (calc_entries())\n");
                goto err;
            case ERROR_WRITING_OUTPUT_FILE:
                DPrintf("Error Writing Output File (calc_entries())\n");
                goto err;
            case ERROR_READING_INPUT_FILE:
                DPrintf("Error Reading Input File (calc_entries())\n");
                goto err;
            case ERROR_CREATING_SPLIT_FILE:
                DPrintf("Error Creating Split file (calc_entries())\n");
                goto err;
            case ERROR_ABORTED_BY_USER:
                DPrintf("Process aborted by user\n");
                goto err;

        }
    }

    /*
    DPrintf("llba0 0x%X - offset: 0x%X\n", llba0, llba0 * SECTOR_SIZE);
    DPrintf("llba1 0x%X - offset: 0x%X\n", llba1, llba1 * SECTOR_SIZE);
    DPrintf("wlba0 0x%X - offset: 0x%X\n", wlba0, wlba0 * SECTOR_SIZE);
    DPrintf("wlba1 0x%X - offset: 0x%X\n", wlba1, wlba1 * SECTOR_SIZE);

    DPrintf("\ndllba0 0x%X - offset: 0x%X, size 0x%X\n", dllba, dllba * SECTOR_SIZE, dlsz * SECTOR_SIZE);
    DPrintf("dwlba0 0x%X - offset: 0x%X, size 0x%X\n", dwlba, dwlba * SECTOR_SIZE, dwsz * SECTOR_SIZE);
    DPrintf("flba0 0x%X - offset: 0x%X\n", flba, flba * SECTOR_SIZE);
    */

    flba2 = flba;

    sectors = malloc((flba > 128) ? flba * SECTOR_SIZE + SECTOR_SIZE : 128 * SECTOR_SIZE + SECTOR_SIZE);

    if(!sectors)
    {
        DPrintf("Out of Memory (sectors mem)\n");
        press_button_to_continue();
        goto err;
    }

    memset(sectors, 0, flba * SECTOR_SIZE);

    pos_lpath0 = llba0 * SECTOR_SIZE;
    pos_lpath1 = llba1 * SECTOR_SIZE;
    pos_wpath0 = wlba0 * SECTOR_SIZE;
    pos_wpath1 = wlba1 * SECTOR_SIZE;

    DPrintf("\nComputing LBA for %i directory entries... Please wait...\n", nfiles);

    path2[0] = 0;
    ret = fill_entries(path1, path2, 0);

    if(ret < 0 )
    {
        switch(ret)
        {
            case ERROR_OUT_OF_MEMORY:
                DPrintf("Out of Memory (fill_entries())\n");
                goto err;
            case ERROR_FILE_NAME_TOO_LONG:
                DPrintf("File Name Too Long (fill_entries())\n");
                goto err;
            case ERROR_INPUT_FILE_NOT_EXISTS:
                DPrintf("Error Input File Not Exists (fill_entries())\n");
            case ERROR_OPENING_INPUT_FILE:
                DPrintf("Error Opening Input File (fill_entries())\n");
                goto err;
            case ERROR_WRITING_OUTPUT_FILE:
                DPrintf("Error Writing Output File (fill_entries())\n");
                goto err;
            case ERROR_READING_INPUT_FILE:
                DPrintf("Error Reading Input File (fill_entries())\n");
                goto err;
            case ERROR_CREATING_SPLIT_FILE:
                DPrintf("Error Creating Split file (fill_entries())\n");
                goto err;
        }
    }

    #ifdef ALIGNED32SECTORS
    flba= (flba + 31) & ~31;
    #endif
    toc = flba;

    if((((u64)toc) * 2048ULL) > (avail - 0x100000ULL))
    {
        DPrintf("ERROR: Insufficient Disk Space in Destination\n");
        goto err;
    }


    DPrintf("\n");
    DPrintf("Creating ISO... Please wait...\n");
    DPrintf(output_name);
    DPrintf("\n");

    sectors[0x3] = 1; // one range
    set732((void *) &sectors[0x8], 0); // first unencrypted sector
    set732((void *) &sectors[0xC], toc - 1); // last unencrypted sector

    strcpy((void *) &sectors[0x800], "PlayStation3");

    memset((void *) &sectors[0x810], 32, 0x20);
    memcpy((void *) &sectors[0x810], title_id, 10);

    get_rand(&sectors[0x840], 0x1B0);
    get_rand(&sectors[0x9F0], 0x10);


    struct iso_primary_descriptor *isd = (void  *) &sectors[0x8000];
    struct iso_directory_record * idr;

    isd->type[0]=1;
    memcpy(&isd->id[0],"CD001",5);
    isd->version[0]=1;

    memset(&isd->system_id[0],32, 32);
    memcpy(&isd->volume_id[0],"PS3VOLUME                       ",32);

    set733((void *) &isd->volume_space_size[0], toc);
    set723(&isd->volume_set_size[0],1);
    set723(&isd->volume_sequence_number[0],1);
    set723(&isd->logical_block_size[0], SECTOR_SIZE);

    set733((void *) &isd->path_table_size[0], lpath);
    set731((void *) &isd->type_l_path_table[0], llba0); // lba
    set731((void *) &isd->opt_type_l_path_table[0], 0); //lba
    set732((void *) &isd->type_m_path_table[0], llba1); //lba
    set732((void *) &isd->opt_type_m_path_table[0], 0);//lba

    idr = (struct iso_directory_record *) &isd->root_directory_record;
    idr->length[0]=34;
    idr->ext_attr_length[0]=0;
    set733((void *) &idr->extent[0], directory_iso[0].llba); //lba
    set733((void *) &idr->size[0], directory_iso[0].ldir * SECTOR_SIZE); // tamaño
    //setdaterec(&idr->date[0],dd,mm,aa,ho,mi,se);
    struct iso_directory_record * aisdr = (void *) &sectors[directory_iso[0].llba * SECTOR_SIZE];
    memcpy(idr->date, aisdr->date, 7);

    idr->flags[0]=2;
    idr->file_unit_size[0]=0;
    idr->interleave[0]=0;
    set723(&idr->volume_sequence_number[0],1); //lba
    idr->name_len[0]=1;
    idr->name[0]=0;

    memset(&isd->volume_set_id[0],32,128);
    memcpy(&isd->volume_set_id[0],"PS3VOLUME", 9);
    memset(&isd->publisher_id[0],32,128);
    memset(&isd->preparer_id[0],32,128);
    memset(&isd->application_id[0],32,128);
    memset(&isd->copyright_file_id[0],32,37);
    memset(&isd->abstract_file_id[0],32,37);
    memset(&isd->bibliographic_file_id,32,37);

    unsigned dd1,mm1,aa1,ho1,mi1,se1;

    time_t t;
    struct tm *tm;

    time(&t);

    tm = localtime(&t);
    dd = tm->tm_mday;
    mm = tm->tm_mon + 1;
    aa = tm->tm_year + 1900;
    ho = tm->tm_hour;
    mi = tm->tm_min;
    se = tm->tm_sec;

    dd1=dd;mm1=mm;aa1=aa;ho1=ho;mi1=mi;se1=se;
    if(se1>59) {se1=0;mi1++;}
    if(mi1>59) {mi1=0;ho1++;}
    if(ho1>23) {ho1=0;dd1++;}
    char fecha[64];
    sprintf(fecha,"%4.4u%2.2u%2.2u%2.2u%2.2u%2.2u00",aa1,mm1,dd1,ho1,mi1,se1);

    memcpy(&isd->creation_date[0], fecha,16);
    memcpy(&isd->modification_date[0],"0000000000000000",16);
    memcpy(&isd->expiration_date[0],"0000000000000000",16);
    memcpy(&isd->effective_date[0],"0000000000000000",16);
    isd->file_structure_version[0]=1;

    int len_string;

    isd = (void  *) &sectors[0x8800];

    isd->type[0] = 2;
    memcpy(&isd->id[0],"CD001",5);
    isd->version[0]=1;
    len_string = UTF8_to_UTF16((u8 *) "PS3VOLUME", wstring);

    memset(&isd->system_id[0],0, 32);
    memset(&isd->volume_id[0],0, 32);
    memcpy(&isd->volume_id[0], wstring, len_string * 2);

    set733((void *) &isd->volume_space_size[0],toc);
    set723(&isd->volume_set_size[0],1);
    isd->unused3[0] = 0x25;
    isd->unused3[1] = 0x2f;
    isd->unused3[2] = 0x40;
    set723(&isd->volume_sequence_number[0],1);
    set723(&isd->logical_block_size[0], SECTOR_SIZE);

    set733((void *) &isd->path_table_size[0], wpath);
    set731((void *) &isd->type_l_path_table[0], wlba0); // lba
    set731((void *) &isd->opt_type_l_path_table[0], 0); //lba
    set732((void *) &isd->type_m_path_table[0], wlba1); //lba
    set732((void *) &isd->opt_type_m_path_table[0], 0);//lba

    idr = (struct iso_directory_record *) &isd->root_directory_record;
    idr->length[0]=34;
    idr->ext_attr_length[0]=0;
    set733((void *) &idr->extent[0], directory_iso[0].wlba); //lba
    set733((void *) &idr->size[0], directory_iso[0].wdir * SECTOR_SIZE); // tamaño
    //setdaterec(&idr->date[0],dd,mm,aa,ho,mi,se);
    aisdr = (void *) &sectors[directory_iso[0].wlba * SECTOR_SIZE];
    memcpy(idr->date, aisdr->date, 7);

    idr->flags[0]=2;
    idr->file_unit_size[0]=0;
    idr->interleave[0]=0;
    set723(&idr->volume_sequence_number[0],1); //lba
    idr->name_len[0]=1;
    idr->name[0]=0;

    memset(&isd->volume_set_id[0],0,128);
    memcpy(&isd->volume_set_id[0], wstring, len_string * 2);
    memset(&isd->publisher_id[0],0,128);
    memset(&isd->preparer_id[0],0,128);
    memset(&isd->application_id[0],0,128);
    memset(&isd->copyright_file_id[0],0,37);
    memset(&isd->abstract_file_id[0],0,37);
    memset(&isd->bibliographic_file_id,0,37);
    memcpy(&isd->creation_date[0], fecha,16);
    memcpy(&isd->modification_date[0],"0000000000000000",16);
    memcpy(&isd->expiration_date[0],"0000000000000000",16);
    memcpy(&isd->effective_date[0],"0000000000000000",16);
    isd->file_structure_version[0]=1;

    isd = (void  *) &sectors[0x9000];
    isd->type[0] = 255;
    memcpy(&isd->id[0],"CD001",5);

    int fd2 = -1;

    if(get_input_abort())
    {
        ask_del = 1;
        goto err;
    }
    else
    {
        fd2 = ps3ntfs_open(output_name, O_WRONLY | O_CREAT | O_TRUNC, 0766);
    }

    if(fd2 >= 0)
    {
        ask_del = 1;

        ps3ntfs_write(fd2, (void *) sectors, (int) flba2 * SECTOR_SIZE);
        flba = flba2;
        int ret = build_file_iso(&fd2, path1, path2, 0);

        if(my_f_async.flags & ASYNC_ENABLE)
        {
            wait_event_thread();
            if(my_f_async.flags  & ASYNC_ERROR)
            {
                if(ret == 0) ret = my_f_async.ret;
            }
            my_f_async.flags = 0;
        }

        event_thread_send(0x555ULL, (u64) 0, 0);

        if(/*iso_split < 2 && */ret != ERROR_CREATING_SPLIT_FILE)
        {
            if(fd2 >= 0) ps3ntfs_close(fd2); fd2 = -1;
        }

        if(ret < 0 )
        {
            switch(ret)
            {
                case ERROR_OUT_OF_MEMORY:
                    DPrintf("Out of Memory (build_file_iso())\n");
                    goto err;
                case ERROR_FILE_NAME_TOO_LONG:
                    DPrintf("File Name Too Long (build_file_iso())\n");
                    goto err;
                case ERROR_INPUT_FILE_NOT_EXISTS:
                    DPrintf("Error Input File Not Exists (build_file_iso())\n");
                case ERROR_OPENING_INPUT_FILE:
                    DPrintf("Error Opening Input File (build_file_iso())\n");
                    goto err;
                case ERROR_WRITING_OUTPUT_FILE:
                    DPrintf("Error Writing Output File (build_file_iso())\n");
                    goto err;
                case ERROR_READING_INPUT_FILE:
                    DPrintf("Error Reading Input File (build_file_iso())\n");
                    goto err;
                case ERROR_CREATING_SPLIT_FILE:
                    DPrintf("Error Creating Split file (build_file_iso())\n");
                    goto err;
                case ERROR_ABORTED_BY_USER:
                    DPrintf("\nERROR: Aborted by User\n\n");
                    goto err;

            }
        }
    }
    else
    {
        DPrintf("Error Creating ISO file %s\n", output_name);
        goto err;
    }

    int n;
    for(n = 0; n < cur_isop; n++)
        if(directory_iso[n].name) {free(directory_iso[n].name); directory_iso[n].name = NULL;}

    free(directory_iso); directory_iso = NULL;
    free(sectors);  sectors = NULL;

    t_finish = get_ticks();

    DPrintf("\n>> Build ISO completed.\n\nISO TOC: %i\n\n", toc);
    DPrintf("Total Time (HH:MM:SS): %2.2u:%2.2u:%2.2u.%u\n\n", (u32) (t_finish - t_start)/(TICKS_PER_SEC * 3600),
           (u32) ((t_finish - t_start) / (TICKS_PER_SEC  * 60)) % 60, (u32) ((t_finish - t_start)/(TICKS_PER_SEC)) % 60,
           (u32) ((t_finish - t_start) / (TICKS_PER_SEC / 100)) % 100);

    press_button_to_continue();
    return SUCCESS;

err:
    if(directory_iso) {
        int n;

        for(n = 0; n < cur_isop; n++)
            if(directory_iso[n].name) {free(directory_iso[n].name); directory_iso[n].name = NULL;}

        free(directory_iso); directory_iso = NULL;
    }

    if(sectors) free(sectors); sectors = NULL;

    if(ask_del)
    {
        DPrintf("\nDo you want to delete the partial ISO?  (CROSS - Yes/ TRIANGLE - No):\n");
        if(get_input_char() == 1) {
            int n;
            DPrintf("Yes\n");
            ps3ntfs_unlink(output_name2);

            for(n = 0; n < 64; n++)
            {
                sprintf(output_name, "%s.%i", output_name2, n);

                if(!ps3ntfs_stat(output_name, &s))
                    ps3ntfs_unlink(output_name);
                else
                    break;
            }

        }
        else
            DPrintf("No\n");

    }

    press_button_to_continue();
    return FAILED;
}

/*************************************************************************************/

static void get_iso_path(char *path, int indx)
{
    char aux[0x420];

    path[0] = 0;

    if(!indx) {path[0] = '/'; path[1] = 0; return;}

    while(true)
    {
        strcpy(aux, directory_iso2[indx].name);
        strcat(aux, path);
        strcpy(path, aux);

        indx = directory_iso2[indx].parent - 1;
        if(indx == 0) break;
    }

}

static int read_split(u64 position, u8 *mem, int size)
{
    int n;

    if(!split_file[1].size)
    {
        if(fd_split0 < 0) fd_split0 = ps3ntfs_open(split_file[0].path, O_RDONLY, 0766);
        if(fd_split0 < 0) return ERROR_OPENING_INPUT_FILE;

        if(ps3ntfs_seek64(fd_split0, position, SEEK_SET) < SUCCESS)
        {
            DPrintf("ERROR: in ISO file fseek\n\n");

            return ERROR_READING_INPUT_FILE;
        }

        if(ps3ntfs_read(fd_split0, (void *) mem, size) != size) return ERROR_WRITING_OUTPUT_FILE;

        return SUCCESS;
    }

    u64 relpos0 = 0;
    u64 relpos1 = 0;

    for(n = 0; n < 64; n++)
    {
        if(!split_file[n].size) return ERROR_INPUT_FILE_NOT_EXISTS;
        if(position < (relpos0 + (u64) split_file[n].size))
        {
            relpos1 = relpos0 + (u64) split_file[n].size;
            break;
        }

        relpos0 += split_file[n].size;
    }

    if(fd_split < 0) split_index = 0;

    if(n == 0)
    {
        if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}
        split_index = 0;
        fd_split = fd_split0;
    }
    else
    {
        if(n != split_index)
        {
            if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}

            split_index = n;

            fd_split = ps3ntfs_open(split_file[split_index].path, O_RDONLY, 0766);
            if(fd_split < 0) return ERROR_OPENING_INPUT_FILE;
        }
    }

    //int cur = lba / SPLIT_LBA;
    //int cur2 = (lba + sectors) / SPLIT_LBA;

    if(ps3ntfs_seek64(fd_split, (position - relpos0), SEEK_SET) < SUCCESS)
    {
        DPrintf("ERROR: in ISO file fseek\n\n");

        return ERROR_READING_INPUT_FILE;
    }

    if(position >= relpos0 && (position + size) < relpos1)
    {
        if(ps3ntfs_read(fd_split, (void *) mem, (int) size) != size) return ERROR_WRITING_OUTPUT_FILE;

        return SUCCESS;
    }

    int lim = (int) (relpos1 - position);

    if(ps3ntfs_read(fd_split, (void *) mem, (int) lim) != lim) return ERROR_WRITING_OUTPUT_FILE;

    mem += lim; size-= lim;

    if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}

    split_index++;

    fd_split = ps3ntfs_open(split_file[split_index].path, O_RDONLY, 0766);
    if(fd_split < 0) return ERROR_OPENING_INPUT_FILE;

    if(ps3ntfs_read(fd_split, (void *) mem, (int) size) != size) return ERROR_WRITING_OUTPUT_FILE;

    return SUCCESS;
}

int extractps3iso(char *f_iso, char *g_path, int split)
{

    struct stat s;
    int n;

    char path1[0x420];
    char path2[0x420];
    char path3[0x420];

    int len_path2;

    int fd2 = -1;

    u8 *sectors2 = NULL;

    char string[0x420];
    char string2[0x420];
    u16 wstring[1024];

    struct iso_primary_descriptor sect_descriptor;
    struct iso_directory_record * idr;
    int idx = -1;

    directory_iso2 = NULL;

    fd_split = -1;
    fd_split0 = -1;
    split_index = 0;
    split_files = 0;

    u64 t_start, t_finish;

    DCls();

    sprintf(dbhead, "EXTRACTPS3ISO Utility");

    DbgHeader(dbhead);

    DPrintf("\nEXTRACTPS3ISO (c) 2013, Estwald (Hermes)\n\n");

    // libc test
    if(sizeof(s.st_size) != 8)
    {
        DPrintf("ERROR: stat st_size must be a 64 bit number!  (size %i)\n", (int) sizeof(s.st_size));
        press_button_to_continue();
        return FAILED;
    }

    split_file = malloc(sizeof(_split_file) * 64);
    if(!split_file)
    {
        DPrintf("ERROR: out of memory! (split_file)\n");
        press_button_to_continue();
        return FAILED;
    }

    strcpy(path1, f_iso);

    if(path1[0] == 0)
    {
        free(split_file); split_file = NULL;
        DPrintf("Error: ISO file don't exists!\n");
        press_button_to_continue();
        return FAILED;
    }

    fixpath(path1);
    if(!is_ntfs_path(path1)) sysLv2FsChmod(path1, FS_S_IFMT | 0777);

    n = strlen(path1);


    if(n >= 4 && !strcasecmp(&path1[n - 4], ".iso"))
    {
        sprintf(split_file[0].path, "%s", path1);
        if(stat(split_file[0].path, &s) < SUCCESS)
        {
            free(split_file); split_file = NULL;
            DPrintf("Error: ISO file don't exists!\n");
            press_button_to_continue();
            return FAILED;
        }

        split_file[0].size = s.st_size;
        split_file[1].size = 0; // split off
    }
    else if(n >= 6 && !strcasecmp(&path1[n - 6], ".iso.0"))
    {
        int m;

        for(m = 0; m < 64; m++)
        {
            strcpy(string2, path1);
            string2[n - 2] = 0;
            sprintf(split_file[m].path, "%s.%i", string2, m);
            if(stat(split_file[m].path, &s) < SUCCESS) break;
            split_file[m].size = s.st_size;
        }

        for(; m < 64; m++)
        {
            split_file[m].size = 0;
        }
    }
    else
    {
        free(split_file); split_file = NULL;
        DPrintf("Error: file must be with .iso or .iso.0 extension\n");
        press_button_to_continue();
        return FAILED;
    }

    path2[0] = 0;
    strcpy(path2, g_path);

    fixpath(path2);

    if(path2[0] == 0)
    {
        free(split_file); split_file = NULL;
        DPrintf("Error: Invalid game path\n");
        press_button_to_continue();
        return FAILED;
    }
    else if(stat(path2, &s) == 0)
    {
        strcat(path2, "/");

        char * o = strrchr(path1, '/');
        if(!o)
            strcat(path2, path1);
        else
            strcat(path2, o + 1);

        n= strlen(path2);

        if(!strcmp(&path2[n - 2], ".0")) path2[n - 6] = 0; else path2[n - 4] = 0;

        fixpath(path2);

        DPrintf("path: %s\n", path2);
    }

    strcpy(path3, path2);

    split_files = split;

    if(split_files) DPrintf("Using Split File Mode\n");

    DPrintf("\n");


    len_path2 = strlen(path2);

    ps3ntfs_mkdir(path2, 0766); // make directory

    u64 avail = get_disk_free_space(path2);

    int fd = ps3ntfs_open(path1, O_RDONLY, 0766);
    if(fd < 0)
    {
        free(split_file); split_file = NULL;
        DPrintf("ERROR: Cannot open ISO file\n");
        press_button_to_continue();
        return FAILED;
    }

    t_start = get_ticks();


    if(ps3ntfs_seek64(fd, 0x8800, SEEK_SET) < 0)
    {
        DPrintf("ERROR: in sect_descriptor fseek\n\n");
        goto err;
    }

    if(ps3ntfs_read(fd, (void *) &sect_descriptor, SECTOR_SIZE) != SECTOR_SIZE)
    {
        DPrintf("ERROR: reading sect_descriptor\n\n");
        goto err;
    }

    if(!(sect_descriptor.type[0] == 2 && !strncmp((void *) &sect_descriptor.id[0], "CD001",5)))
    {
        DPrintf("ERROR: UTF16 descriptor not found\n");
        goto err;
    }

    u32 toc = isonum_733(&sect_descriptor.volume_space_size[0]);

    if((((u64)toc) * 2048ULL) > (avail - 0x100000ULL))
    {
        DPrintf("ERROR: Insufficient Disk Space in Destination\n");
        goto err;
    }

    u32 lba0 = isonum_731(&sect_descriptor.type_l_path_table[0]); // lba
    u32 size0 = isonum_733(&sect_descriptor.path_table_size[0]); // size
    //DPrintf("lba0 %u size %u %u\n", lba0, size0, ((size0 + SECTOR_FILL) / SECTOR_SIZE) * SECTOR_SIZE);

    if(ps3ntfs_seek64(fd, lba0 * SECTOR_SIZE, SEEK_SET) < 0)
    {
        DPrintf("ERROR: in path_table fseek\n\n");
        goto err;
    }

    directory_iso2 = malloc((MAX_ISO_PATHS + 1) * sizeof(_directory_iso2));

    if(!directory_iso2)
    {
        DPrintf("ERROR: in directory_is malloc()\n\n");
        goto err;
    }

    memset(directory_iso2, 0, (MAX_ISO_PATHS + 1) * sizeof(_directory_iso2));

    sectors = malloc(((size0 + SECTOR_FILL) / SECTOR_SIZE) * SECTOR_SIZE);

    if(!sectors)
    {
        DPrintf("ERROR: in sectors malloc()\n\n");
        goto err;
    }

    sectors2 = malloc(SECTOR_SIZE * 2);

    if(!sectors2)
    {
        DPrintf("ERROR: in sectors2 malloc()\n\n");
        goto err;
    }

    sectors3 = malloc(128 * SECTOR_SIZE);

    if(!sectors3)
    {
        DPrintf("ERROR: in sectors3 malloc()\n\n");
        goto err;
    }

    if(ps3ntfs_read(fd, (void *) sectors, size0) != size0)
    {
        DPrintf("ERROR: reading path_table\n\n");
        goto err;
    }


    u32 p = 0;

    string2[0] = 0;

    fd_split = -1;
    fd_split0 = -1;

    split_index = 0;


    idx = 0;

    directory_iso2[idx].name = NULL;

    int len, len2;

    while(p < size0)
    {
        u32 lba;

        u32 snamelen = isonum_721(&sectors[p]);
        if(snamelen == 0) p= ((p/SECTOR_SIZE) * SECTOR_SIZE) + SECTOR_SIZE;
        p += 2;
        lba = isonum_731(&sectors[p]);
        p += 4;
        u32 parent =isonum_721(&sectors[p]);
        p += 2;

        memset(wstring, 0, 512 * 2);
        memcpy(wstring, &sectors[p], snamelen);

        len = UTF16_to_UTF8(wstring, (u8 *) string);

        if(idx >= MAX_ISO_PATHS)
        {
            DPrintf("Too much folders (max %i)\n\n", MAX_ISO_PATHS);
            goto err;
        }

        directory_iso2[idx].name = malloc(len + 2);
        if(!directory_iso2[idx].name)
        {
            DPrintf("ERROR: in directory_iso2.name malloc()\n\n");
            goto err;
        }

        strcpy(directory_iso2[idx].name, "/");
        strcat(directory_iso2[idx].name, string);

        directory_iso2[idx].parent = parent;

        get_iso_path(string2, idx);

        strcat(path2, string2);

        ps3ntfs_mkdir(path2, 0766); // make directory

        path2[len_path2] = 0;

        DPrintf("</%s>\n", string);

        u32 file_lba = 0, old_file_lba = 0;
        u64 file_size = 0;

        char file_aux[0x420];

        file_aux[0] = 0;

        int q2 = 0;
        int size_directory = 0;

        len2 = strlen(string2);

        while(true)
        {
            if(ps3ntfs_seek64(fd, lba * SECTOR_SIZE, SEEK_SET) < SUCCESS)
            {
                DPrintf("ERROR: in directory_record fseek\n\n");
                goto err;
            }

            memset(sectors2 + SECTOR_SIZE, 0, SECTOR_SIZE);

            if(ps3ntfs_read(fd, (void *) sectors2, SECTOR_SIZE) != SECTOR_SIZE)
            {
                DPrintf("ERROR: reading directory_record sector\n\n");
                goto err;
            }

            int q = 0;

            if(q2 == 0)
            {
                idr = (struct iso_directory_record *) &sectors2[q];
                if((int) idr->name_len[0] == 1 && idr->name[0] == 0 && lba == isonum_731((void *) idr->extent) && idr->flags[0] == 0x2)
                {
                    size_directory = isonum_733((void *) idr->size);
                }
                else
                {
                    DPrintf("ERROR: Bad first directory record! (LBA %i)\n\n", lba);
                    goto err;
                }
            }

            int signal_idr_correction = 0;

            while(true)
            {
                if(signal_idr_correction)
                {
                    signal_idr_correction = 0;
                    q-= SECTOR_SIZE; // sector correction
                    // copy next sector to first
                    memcpy(sectors2, sectors2 + SECTOR_SIZE, SECTOR_SIZE);
                    memset(sectors2 + SECTOR_SIZE, 0, SECTOR_SIZE);
                    lba++;

                    q2 += SECTOR_SIZE;
                }

                if(q2 >= size_directory) goto end_dir_rec;

                idr = (struct iso_directory_record *) &sectors2[q];

                if(idr->length[0]!=0 && (idr->length[0] + q) > SECTOR_SIZE)
                {
                    DPrintf("Warning! Entry directory break the standard ISO 9660\n");
                    press_button_to_continue();

                    if(ps3ntfs_seek64(fd, lba * SECTOR_SIZE + SECTOR_SIZE, SEEK_SET) < 0)
                    {
                        DPrintf("ERROR: in directory_record fseek\n\n");
                        goto err;
                    }

                    if(ps3ntfs_read(fd, (void *) (sectors2 + SECTOR_SIZE), SECTOR_SIZE) != SECTOR_SIZE)
                    {
                        DPrintf("ERROR: reading directory_record sector\n\n");
                        goto err;
                    }

                    signal_idr_correction = 1;

                }

                if(idr->length[0] == 0 && (SECTOR_SIZE - q) > 255) goto end_dir_rec;

                if((idr->length[0] == 0 && q != 0) || q == SECTOR_SIZE)
                {
                    lba++;
                    q2 += SECTOR_SIZE;

                    if(q2 >= size_directory) goto end_dir_rec;

                    if(ps3ntfs_seek64(fd, lba * SECTOR_SIZE, SEEK_SET) < 0)
                    {
                        DPrintf("ERROR: in directory_record fseek\n\n");
                        goto err;
                    }

                    if(ps3ntfs_read(fd, (void *) (sectors2), SECTOR_SIZE) != SECTOR_SIZE)
                    {
                        DPrintf("ERROR: reading directory_record sector\n\n");
                        goto err;
                    }

                    memset(sectors2 + SECTOR_SIZE, 0, SECTOR_SIZE);

                    q = 0;
                    idr = (struct iso_directory_record *) &sectors2[q];

                    if(idr->length[0] == 0 || ((int) idr->name_len[0] == 1 && !idr->name[0])) goto end_dir_rec;

                }

                if((int) idr->name_len[0] > 1 && idr->flags[0] != 0x2 &&
                    idr->name[idr->name_len[0] - 1] == '1' && idr->name[idr->name_len[0] - 3] == ';')
                {
                    // skip directories

                    memset(wstring, 0, 512 * 2);
                    memcpy(wstring, idr->name, idr->name_len[0]);

                    len = UTF16_to_UTF8(wstring, (u8 *) string);

                    if(file_aux[0])
                    {
                        if(strcmp(string, file_aux))
                        {
                            DPrintf("ERROR: in batch file %s\n", file_aux);
                            goto err;
                        }

                        file_size += (u64) (u32) isonum_733(&idr->size[0]);
                        if(idr->flags[0] == 0x80)
                        {
                            // get next batch file
                            q+= idr->length[0];
                            continue;
                        }

                        file_aux[0] = 0; // stop batch file
                    }
                    else
                    {
                        file_lba = isonum_733(&idr->extent[0]);
                        file_size = (u64) (u32) isonum_733(&idr->size[0]);
                        if(idr->flags[0] == 0x80)
                        {
                            strcpy(file_aux, string);
                            q += idr->length[0];
                            continue;  // get next batch file
                        }
                    }

                    string[len - 2] = 0; // break ";1" string

                    strcat(string2 + len2, "/");
                    strcat(string2 + len2, string);

                    if(old_file_lba < file_lba)
                    {
                        sprintf(dbhead, "%s - done %i%c", "EXTRACTPS3ISO Utility", file_lba * 100 / toc, '%');
                        DbgHeader(dbhead);
                        old_file_lba = file_lba;
                    }

                    if(file_size < 1024ULL)
                        DPrintf("  -> %s LBA %u size %u Bytes\n", string, file_lba, (u32) file_size);
                    else if(file_size < 0x100000LL)
                        DPrintf("  -> %s LBA %u size %u KB\n", string, file_lba, (u32) (file_size/1024));
                    else
                        DPrintf("  -> %s LBA %u size %u MB\n", string, file_lba, (u32) (file_size/0x100000LL));

                    //DPrintf("f %s\n", string2);

                    // writing procedure

                    strcat(path2, string2);

                    int use_split = 0;

                    if(split_files && file_size >= 0xFFFF0001LL) {
                        use_split = 1;
                        sprintf(string, "%s.666%2.2u", path2, 0);
                        fd2 = ps3ntfs_open(string, O_WRONLY | O_CREAT | O_TRUNC, 0766);

                    } else
                        fd2 = ps3ntfs_open(path2, O_WRONLY | O_CREAT | O_TRUNC, 0766);

                    if(fd2 >= 0)
                    {
                        fd_split0 = fd;

                        u32 count = 0, percent = (u32) (file_size / 0x40000ULL);
                        if(percent == 0) percent = 1;

                        int count_split = 0;

                        int cx = con_x, cy = con_y;

                        u64 t_one, t_two;

                        t_one = get_ticks();

                        if(get_input_abort())
                        {
                            DPrintf("\nERROR: Aborted by User\n\n");
                            goto err;
                        }

                        while(file_size > 0) {
                            u32 fsize;

                            con_x = cx; con_y = cy;
                            t_two = get_ticks();

                            if(((t_two - t_one) >= TICKS_PER_SEC/2)) {
                                t_one = t_two;
                                DPrintf("*** Writing... %u %%", count * 100 / percent);
                                con_x = cx; con_y = cy;

                                if(get_input_abort()) {
                                    DPrintf("\nERROR: Aborted by User\n\n");
                                    goto err;
                                }
                            }

                            if(use_split && count_split >= 0x40000000)
                            {
                                count_split = 0;
                                ps3ntfs_close(fd2);

                                sprintf(string, "%s.666%2.2u", path2, use_split);
                                use_split++;
                                fd2 = ps3ntfs_open(string, O_WRONLY | O_CREAT | O_TRUNC, 0766);

                                if(fd2 < 0)
                                {
                                    DPrintf("\nERROR: creating extract file\n\n");
                                    goto err;
                                }
                            }

                            if(file_size > 0x40000) fsize = 0x40000;
                            else fsize = (u32) file_size;

                            count++;

                            if(use_split) count_split+= fsize;

                            if(read_split(((u64) file_lba) * 2048ULL, (void *) sectors3, (int) fsize) < 0)
                            {
                                DPrintf("\nERROR: reading ISO file\n\n");
                                goto err;
                            }

                            if(ps3ntfs_write(fd2, (void *) sectors3, (int) fsize) != fsize)
                            {
                                DPrintf("\nERROR: writing ISO file\n\n");
                                goto err;
                            }

                            file_size-= (u64) fsize;

                            file_lba += (fsize + SECTOR_FILL) / SECTOR_SIZE;
                        }

                        con_x = cx; con_y = cy;
                        DPrintf("                             ");
                        con_x = cx; con_y = cy;

                        ps3ntfs_close(fd2); fd2 = -1;
                    }
                    else
                    {
                        DPrintf("\nERROR: creating extract file\n\n");
                        goto err;
                    }

                    path2[len_path2] = 0;
                    string2[len2] = 0;
                }

                q+= idr->length[0];
            }

            lba ++;
            q2+= SECTOR_SIZE;
            if(q2 >= size_directory) goto end_dir_rec;

        }

        end_dir_rec:

        p += snamelen;
        if(snamelen & 1) p++;

        idx++;

    }

    sprintf(dbhead, "%s - done 100%c", "EXTRACTPS3ISO Utility", '%');
    DbgHeader(dbhead);

    if(fd) ps3ntfs_close(fd);
    if(fd2) ps3ntfs_close(fd2);
    if(split_index && fd_split) {ps3ntfs_close(fd_split); fd_split = -1;}
    if(sectors) free(sectors); sectors = NULL;
    if(sectors2) free(sectors2); sectors2 = NULL;
    if(sectors3) free(sectors3); sectors3 = NULL;

    for(n = 0; n <= idx; n++)
        if(directory_iso2[n].name) {free(directory_iso2[n].name); directory_iso2[n].name = NULL;}

    if(directory_iso2) free(directory_iso2); directory_iso2 = NULL;

    free(split_file); split_file = NULL;

    t_finish = get_ticks();

    DPrintf("\n>> Extract completed.\n\n");
    DPrintf("Total Time (HH:MM:SS): %2.2u:%2.2u:%2.2u.%u\n\n", (u32) (t_finish - t_start)/(TICKS_PER_SEC * 3600),
           (u32) ((t_finish - t_start) / (TICKS_PER_SEC *  60)) % 60, (u32) ((t_finish - t_start)/(TICKS_PER_SEC)) % 60,
           (u32) ((t_finish - t_start) / (TICKS_PER_SEC / 100)) % 100);

    press_button_to_continue();
    return SUCCESS;

err:

    if(fd) ps3ntfs_close(fd);
    if(fd2) ps3ntfs_close(fd2);
    if(split_index && fd_split) {ps3ntfs_close(fd_split); fd_split = -1;}

    if(sectors) free(sectors); sectors = NULL;
    if(sectors2) free(sectors2); sectors2 = NULL;
    if(sectors3) free(sectors3); sectors3 = NULL;

    for(n = 0; n <= idx; n++)
        if(directory_iso2[n].name) {free(directory_iso2[n].name); directory_iso2[n].name = NULL;}

    if(directory_iso2) free(directory_iso2); directory_iso2 = NULL;

    free(split_file); split_file = NULL;

    DPrintf("\nDelete partial GAME?  (CROSS - Yes/ TRIANGLE - No):\n");
    if(get_input_char() == 1)
    {
        DPrintf("Yes\n");
        DeleteDirectory(path3);
        ps3ntfs_unlink(path3);

    }
    else DPrintf("No\n");

    press_button_to_continue();
    return FAILED;
}

static int write_split2(u64 position, u8 *mem, int size)
{
    int n;

    if(!split_file[1].size)
    {
        if(fd_split0 < 0) fd_split0 = ps3ntfs_open(split_file[0].path, O_RDWR, 0766);
        if(fd_split0 < 0) return ERROR_OPENING_INPUT_FILE;

        if(ps3ntfs_seek64(fd_split0, position, SEEK_SET) < 0)
        {
            DPrintf("ERROR: in ISO file fseek\n\n");

            return ERROR_READING_INPUT_FILE;
        }

        if(ps3ntfs_write(fd_split0, (void *) mem, size) != size) return ERROR_WRITING_OUTPUT_FILE;

        return SUCCESS;
    }

    u64 relpos0 = 0;
    u64 relpos1 = 0;

    for(n = 0; n < 64; n++)
    {
        if(!split_file[n].size) return ERROR_INPUT_FILE_NOT_EXISTS;
        if(position < (relpos0 + (u64) split_file[n].size))
        {
            relpos1 = relpos0 + (u64) split_file[n].size;
            break;
        }

        relpos0 += split_file[n].size;
    }

    if(fd_split < 0) split_index = 0;

    if(n == 0)
    {
        if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}
        split_index = 0;
        fd_split = fd_split0;

    }
    else
    {
        if(n != split_index)
        {
            if(split_index && fd_split >= 0) {ps3ntfs_close(fd_split); fd_split = -1;}

            split_index = n;

            fd_split = ps3ntfs_open(split_file[split_index].path, O_RDWR, 0766);
            if(fd_split < 0) return ERROR_OPENING_INPUT_FILE;
        }
    }

    //int cur = lba / SPLIT_LBA;
    //int cur2 = (lba + sectors) / SPLIT_LBA;

    if(ps3ntfs_seek64(fd_split, (position - relpos0), SEEK_SET) < 0)
    {
        DPrintf("ERROR: in ISO file fseek\n\n");

        return ERROR_READING_INPUT_FILE;
    }

    if(position >= relpos0 && (position + size) < relpos1)
    {
        if(ps3ntfs_write(fd_split, (void *) mem, (int) size) != size) return ERROR_WRITING_OUTPUT_FILE;

        return SUCCESS;
    }

    int lim = (int) (relpos1 - position);

    if(ps3ntfs_write(fd_split, (void *) mem, (int) lim) != lim) return ERROR_WRITING_OUTPUT_FILE;

    mem += lim; size-= lim;

    if(split_index && fd_split > 0) {ps3ntfs_close(fd_split); fd_split = -1;}

    split_index++;

    fd_split = ps3ntfs_open(split_file[split_index].path, O_RDWR, 0766);
    if(fd_split < 0) return ERROR_OPENING_INPUT_FILE;

    if(ps3ntfs_write(fd_split, (void *) mem, (int) size) != size) return ERROR_WRITING_OUTPUT_FILE;

    return SUCCESS;
}

static int iso_param_sfo_util(u32 lba, u32 len)
{
    u32 pos, str;

    char str_version[8];

    param_patched = 0;

    u16 cur_firm = ((firmware>>12) & 0xF) * 10000 + ((firmware>>8) & 0xF) * 1000 + ((firmware>>4) & 0xF) * 100;

    sprintf(str_version, "%2.2u.%4.4u", cur_firm/10000, cur_firm % 10000 );

    unsigned char *mem = (void *) sectors3;

    u64 file_pos = ((u64) lba * 2048ULL);

    if(read_split(file_pos, (void *) mem, (int) len) < 0)
    {
        DPrintf("\nERROR: reading in ISO PARAM.SFO file\n\n");
        return FAILED;
    }

    str = (mem[8] + (mem[9]<<8));
    pos = (mem[0xc] + (mem[0xd]<<8));

    int indx = 0;

    while(str < len)
    {
        if(mem[str] == 0) break;

        if(!strcmp((char *) &mem[str], "PS3_SYSTEM_VER"))
        {
            if(strcmp((char *) &mem[pos], str_version) > 0)
            {
                DPrintf("PARAM.SFO patched to version: %s from %s\n\n", str_version, &mem[pos]);
                    memcpy(&mem[pos], str_version, 8);
                param_patched = 1;
                break;
            }
        }

        while(mem[str]) str++;str++;

        pos += (mem[0x1c+indx]+(mem[0x1d+indx]<<8));
        indx += 16;
    }

    if(param_patched)
    {
        if(write_split2(file_pos, (void *) mem, (int) len) < 0)
        {
            DPrintf("\nERROR: reading in ISO PARAM.SFO file\n\n");
            return FAILED;
        }
    }

    return SUCCESS;
}

static int iso_patch_exe_error_09(u32 lba, char *filename)
{
    if(firmware >= 0x486C) return SUCCESS;

    u16 fw_421 = 42100;
    u16 fw_486 = 48600;
    int offset_fw;
    u16 ver = 0;
    int flag = 0;

    u64 file_pos = ((u64) lba * 2048ULL);

    // open self/sprx and changes the fw version

    // set to offset position

    if(read_split(file_pos + 0xCULL, (void *) &offset_fw, (int) 4) < 0)
    {
        DPrintf("\nERROR: reading in ISO SPRX/SELF file\n\n");
        return FAILED;
    }

    u8 retried; retried = 0;
    offset_fw = SWAP32(offset_fw); offset_fw -= 0x78;

    retry_offset_iso:
    if(offset_fw < 0x90 || offset_fw > 0x800) offset_fw = strstr(filename, ".sprx") ? 0x258 : 0x428;
    offset_fw += 6;

    if(read_split(file_pos + ((u64) offset_fw), (void *) &ver, (int) 2) < 0)
    {
        DPrintf("\nERROR: reading in ISO SPRX/SELF file\n\n");
        return FAILED;
    }

    ver = SWAP16(ver); if(retried == 0 && (ver % 100) > 0) {offset_fw = (offset_fw==0x258) ? 0x278 : 0; retried = 1; goto retry_offset_iso;}

    u16 cur_firm = ((firmware>>12) & 0xF) * 10000 + ((firmware>>8) & 0xF) * 1000 + ((firmware>>4) & 0xF) * 100;

    if(firmware >= 0x421C && firmware < 0x485C && ver > fw_421 && ver <= fw_486 && ver > cur_firm)
    {
        DPrintf("Version changed from %u.%u to %u.%u in %s\n\n", ver/10000, (ver % 10000)/100, cur_firm/10000, (cur_firm % 10000)/100, filename);
        cur_firm = SWAP16(cur_firm);
        if(write_split2(file_pos + ((u64) offset_fw), (void *) &cur_firm, (int) 2) < 0)
        {
            DPrintf("\nERROR: writing ISO file\n\n");
            return FAILED;
        }

        flag = 1;
        self_sprx_patched++;

    }
    else if(ver > cur_firm)
    {
        DPrintf("\nERROR: this SELF/SPRX uses a bigger version of %u.%uC (%u.%u)\n\n", cur_firm/10000, (cur_firm % 10000)/100, ver/10000, (ver % 10000)/100);
        flag = -1; //
    }


   return flag;
}

int patchps3iso(char *f_iso, int nopause)
{

    struct stat s;
    int n;

    char path1[0x420];

    u8 *sectors2 = NULL;
    sectors3 = NULL;

    char string[0x420];
    char string2[0x420];
    u16 wstring[1024];

    int num_files = 0;
    int num_dir = 0;

    struct iso_primary_descriptor sect_descriptor;
    struct iso_directory_record * idr;
    int idx = -1;

    directory_iso2 = NULL;

    fd_split = -1;
    fd_split0 = -1;
    split_index = 0;
    param_patched = 0;
    self_sprx_patched = 0;

    u64 t_start, t_finish;

    DCls();

    if(nopause)
        DbgHeader("Patching ISO");
    else
        DbgHeader("PATCHPS3ISO Utility");

    DPrintf("\nPATCHPS3ISO (c) 2013, Estwald (Hermes)\n\n");

    // libc test
    if(sizeof(s.st_size) != 8)
    {
        DPrintf("ERROR: stat st_size must be a 64 bit number!  (size %i)\n", (int) sizeof(s.st_size));
        press_button_to_continue();
        return FAILED;
    }

    split_file = malloc(sizeof(_split_file) * 64);
    if(!split_file)
    {
        DPrintf("ERROR: out of memory! (split_file)\n");
        press_button_to_continue();
        return FAILED;
    }

    strcpy(path1, f_iso);

    if(path1[0] == 0)
    {
        free(split_file); split_file = NULL;
        DPrintf("Error: ISO file don't exists!\n");
        press_button_to_continue();
        return FAILED;
    }

    fixpath(path1);
    if(!is_ntfs_path(path1)) sysLv2FsChmod(path1, FS_S_IFMT | 0777);

    n = strlen(path1);


    if(n >= 4 && !strcasecmp(&path1[n - 4], ".iso"))
    {
        sprintf(split_file[0].path, "%s", path1);
        if(stat(split_file[0].path, &s) < SUCCESS)
        {
            free(split_file); split_file = NULL;
            DPrintf("Error: ISO file don't exists!\n");
            press_button_to_continue();
            return FAILED;
        }

        split_file[0].size = s.st_size;
        split_file[1].size = 0; // split off
    }
    else if(n >= 6 && !strcasecmp(&path1[n - 6], ".iso.0"))
    {
        int m;

        for(m = 0; m < 64; m++)
        {
            strcpy(string2, path1);
            string2[n - 2] = 0;
            sprintf(split_file[m].path, "%s.%i", string2, m);
            if(stat(split_file[m].path, &s) < SUCCESS) break;
            split_file[m].size = s.st_size;
        }

        for(; m < 64; m++)
        {
            split_file[m].size = 0;
        }
    }
    else
    {
        free(split_file); split_file = NULL;
        DPrintf("Error: file must be with .iso or .iso.0 extension\n");
        press_button_to_continue();
        return FAILED;
    }

    DPrintf("\n");

    int fd = ps3ntfs_open(path1, O_RDWR, 0766);
    if(fd < 0)
    {
        free(split_file); split_file = NULL;
        DPrintf("ERROR: Cannot open ISO file\n");
        press_button_to_continue();
        return FAILED;
    }

    t_start = get_ticks();

    if(ps3ntfs_seek64(fd, 0x8800, SEEK_SET) < 0)
    {
        DPrintf("ERROR: in sect_descriptor fseek\n\n");
        goto err;
    }

    if(ps3ntfs_read(fd, (void *) &sect_descriptor, SECTOR_SIZE) != SECTOR_SIZE)
    {
        DPrintf("ERROR: reading sect_descriptor\n\n");
        goto err;
    }

    if(!(sect_descriptor.type[0] == 2 && !strncmp((void *) &sect_descriptor.id[0], "CD001",5)))
    {
        DPrintf("ERROR: UTF16 descriptor not found\n\n");
        goto err;
    }

    toc = isonum_733((void *) &sect_descriptor.volume_space_size[0]);
    u32 lba0 = isonum_731(&sect_descriptor.type_l_path_table[0]); // lba
    u32 size0 = isonum_733(&sect_descriptor.path_table_size[0]); // tamaño
    //DPrintf("lba0 %u size %u %u\n", lba0, size0, ((size0 + SECTOR_FILL) / SECTOR_SIZE) * SECTOR_SIZE);

    if(ps3ntfs_seek64(fd, lba0 * SECTOR_SIZE, SEEK_SET) < 0)
    {
        DPrintf("ERROR: in path_table fseek\n\n");
        goto err;
    }

    directory_iso2 = malloc((MAX_ISO_PATHS + 1) * sizeof(_directory_iso2));

    if(!directory_iso2)
    {
        DPrintf("ERROR: in directory_is malloc()\n\n");
        goto err;
    }

    memset(directory_iso2, 0, (MAX_ISO_PATHS + 1) * sizeof(_directory_iso2));

    sectors = malloc(((size0 + SECTOR_FILL) / SECTOR_SIZE) * SECTOR_SIZE);

    if(!sectors)
    {
        DPrintf("ERROR: in sectors malloc()\n\n");
        goto err;
    }

    sectors2 = malloc(SECTOR_SIZE * 2);

    if(!sectors2)
    {
        DPrintf("ERROR: in sectors2 malloc()\n\n");
        goto err;
    }

    sectors3 = malloc(128 * SECTOR_SIZE);

    if(!sectors3)
    {
        DPrintf("ERROR: in sectors3 malloc()\n\n");
        goto err;
    }

    if(ps3ntfs_read(fd, (void *) sectors, size0) != size0)
    {
        DPrintf("ERROR: reading path_table\n\n");
        goto err;
    }

    u32 p = 0;

    string2[0] = 0;

    fd_split = -1;
    fd_split0 = -1;

    split_index = 0;

    idx = 0;

    directory_iso2[idx].name = NULL;

    int len, len2;

    while(p < size0)
    {
        u32 lba;

        u32 snamelen = isonum_721(&sectors[p]);
        if(snamelen == 0) p= ((p/SECTOR_SIZE) * SECTOR_SIZE) + SECTOR_SIZE;
        p += 2;
        lba = isonum_731(&sectors[p]);
        p += 4;
        u32 parent =isonum_721(&sectors[p]);
        p += 2;

        memset(wstring, 0, 512 * 2);
        memcpy(wstring, &sectors[p], snamelen);

        len = UTF16_to_UTF8(wstring, (u8 *) string);

        if(idx >= MAX_ISO_PATHS)
        {
            DPrintf("Too much folders (max %i)\n\n", MAX_ISO_PATHS);
            goto err;
        }

        directory_iso2[idx].name = malloc(len + 2);
        if(!directory_iso2[idx].name)
        {
            DPrintf("ERROR: in directory_iso2.name malloc()\n\n");
            goto err;
        }

        strcpy(directory_iso2[idx].name, "/");
        strcat(directory_iso2[idx].name, string);

        directory_iso2[idx].parent = parent;

        get_iso_path(string2, idx);

        DPrintf("</%s>\n", string);

        u32 file_lba = 0;
        u64 file_size = 0;

        char file_aux[0x420];

        file_aux[0] = 0;

        int q2 = 0;
        int size_directory = 0;

        len2 = strlen(string2);

        while(true)
        {
            if(ps3ntfs_seek64(fd, lba * SECTOR_SIZE, SEEK_SET) < SUCCESS)
            {
                DPrintf("ERROR: in directory_record fseek\n\n");
                goto err;
            }

            memset(sectors2 + SECTOR_SIZE, 0, SECTOR_SIZE);

            if(ps3ntfs_read(fd, (void *) sectors2, SECTOR_SIZE) != SECTOR_SIZE)
            {
                DPrintf("ERROR: reading directory_record sector\n\n");
                goto err;
            }

            int q = 0;

            if(q2 == 0)
            {
                idr = (struct iso_directory_record *) &sectors2[q];
                if((int) idr->name_len[0] == 1 && idr->name[0] == 0 && lba == isonum_731((void *) idr->extent) && idr->flags[0] == 0x2)
                {
                    size_directory = isonum_733((void *) idr->size);
                }
                else
                {
                    DPrintf("ERROR: Bad first directory record! (LBA %i)\n\n", lba);
                    goto err;
                }
            }

            int signal_idr_correction = 0;

            while(true)
            {
                if(signal_idr_correction)
                {
                    signal_idr_correction = 0;
                    q-= SECTOR_SIZE; // sector correction
                    // copy next sector to first
                    memcpy(sectors2, sectors2 + SECTOR_SIZE, SECTOR_SIZE);
                    memset(sectors2 + SECTOR_SIZE, 0, SECTOR_SIZE);
                    lba++;

                    q2 += SECTOR_SIZE;
                }

                if(q2 >= size_directory) goto end_dir_rec;

                idr = (struct iso_directory_record *) &sectors2[q];

                if(idr->length[0]!=0 && (idr->length[0] + q) > SECTOR_SIZE)
                {
                    DPrintf("Warning! Entry directory break the standard ISO 9660\n");
                    press_button_to_continue();

                    if(ps3ntfs_seek64(fd, lba * SECTOR_SIZE + SECTOR_SIZE, SEEK_SET) < 0)
                    {
                        DPrintf("ERROR: in directory_record fseek\n\n");
                        goto err;
                    }

                    if(ps3ntfs_read(fd, (void *) (sectors2 + SECTOR_SIZE), SECTOR_SIZE) != SECTOR_SIZE)
                    {
                        DPrintf("ERROR: reading directory_record sector\n\n");
                        goto err;
                    }

                    signal_idr_correction = 1;
                }

                if(idr->length[0] == 0 && (SECTOR_SIZE - q) > 255) goto end_dir_rec;

                if((idr->length[0] == 0 && q != 0) || q == SECTOR_SIZE)
                {
                    lba++;
                    q2 += SECTOR_SIZE;

                    if(q2 >= size_directory) goto end_dir_rec;

                    if(ps3ntfs_seek64(fd, lba * SECTOR_SIZE, SEEK_SET) < 0)
                    {
                        DPrintf("ERROR: in directory_record fseek\n\n");
                        goto err;
                    }

                    if(ps3ntfs_read(fd, (void *) (sectors2), SECTOR_SIZE) != SECTOR_SIZE)
                    {
                        DPrintf("ERROR: reading directory_record sector\n\n");
                        goto err;
                    }

                    memset(sectors2 + SECTOR_SIZE, 0, SECTOR_SIZE);

                    q = 0;
                    idr = (struct iso_directory_record *) &sectors2[q];

                    if(idr->length[0] == 0 || ((int) idr->name_len[0] == 1 && !idr->name[0])) goto end_dir_rec;
                }

                if((int) idr->name_len[0] > 1 && idr->flags[0] != 0x2 &&
                    idr->name[idr->name_len[0] - 1] == '1' && idr->name[idr->name_len[0] - 3] == ';')
                {
                    // skip directories

                    if(idr->flags[0] != 0x80) num_files++;

                    memset(wstring, 0, 512 * 2);
                    memcpy(wstring, idr->name, idr->name_len[0]);

                    len = UTF16_to_UTF8(wstring, (u8 *) string);

                    if(file_aux[0])
                    {
                        if(strcmp(string, file_aux))
                        {
                            DPrintf("ERROR: in batch file %s\n", file_aux);
                            goto err;
                        }

                        file_size += (u64) (u32) isonum_733(&idr->size[0]);
                        if(idr->flags[0] == 0x80)
                        {
                            // get next batch file
                            q+= idr->length[0];
                            continue;
                        }

                        file_aux[0] = 0; // stop batch file
                    }
                    else
                    {
                        file_lba = isonum_733(&idr->extent[0]);
                        file_size = (u64) (u32) isonum_733(&idr->size[0]);
                        if(idr->flags[0] == 0x80)
                        {
                            strcpy(file_aux, string);
                            q += idr->length[0];
                            continue;  // get next batch file
                        }
                    }

                    string[len - 2] = 0; // break ";1" string

                    strcat(string2 + len2, "/");
                    strcat(string2 + len2, string);

                    if(file_size < 1024ULL)
                        DPrintf("  -> %s LBA %u size %u Bytes\n", string, file_lba, (u32) file_size);
                    else if(file_size < 0x100000LL)
                        DPrintf("  -> %s LBA %u size %u KB\n", string, file_lba, (u32) (file_size/1024));
                    else
                        DPrintf("  -> %s LBA %u size %u MB\n", string, file_lba, (u32) (file_size/0x100000LL));

                    //DPrintf("f %s\n", string2);

                    // writing procedure;


                    fd_split0 = fd;

                    if(get_input_abort())
                    {
                        DPrintf("\nERROR: Aborted by User\n\n");
                        goto err;
                    }

                    if(!strcmp(string, "PARAM.SFO"))
                    {
                        iso_param_sfo_util(file_lba, file_size);
                        goto next_file;
                    }

                    int ext = strlen(string) - 4;

                    if(ext <= 1) {goto next_file;}

                    if((strcmp(string, "EBOOT.BIN") == 0) ||
                       ((string[ext-1] == '.') &&
                        ((string[ext] == 's' && string[ext+1] == 'p' && string[ext+2] == 'r' && string[ext+3] == 'x') ||
                         (string[ext] == 'S' && string[ext+1] == 'P' && string[ext+2] == 'R' && string[ext+3] == 'X') ||
                         (string[ext] == 's' && string[ext+1] == 'e' && string[ext+2] == 'l' && string[ext+3] == 'f') ||
                         (string[ext] == 'S' && string[ext+1] == 'E' && string[ext+2] == 'L' && string[ext+3] == 'F'))))
                    {
                        if(iso_patch_exe_error_09(file_lba, string) < 0) goto err;
                    }

                next_file:
                    string2[len2] = 0;

                }

                q += idr->length[0];
            }

            lba ++;
            q2 += SECTOR_SIZE;
            if(q2 >= size_directory) goto end_dir_rec;
        }

        end_dir_rec:

        p += snamelen;
        if(snamelen & 1) p++;

        idx++;
        num_dir++;
    }

    if(fd) ps3ntfs_close(fd);
    if(split_index && fd_split) {ps3ntfs_close(fd_split); fd_split = -1;}
    if(sectors) free(sectors); sectors = NULL;
    if(sectors2) free(sectors2); sectors2 = NULL;
    if(sectors3) free(sectors3); sectors3 = NULL;

    for(n = 0; n <= idx; n++)
        if(directory_iso2[n].name) {free(directory_iso2[n].name); directory_iso2[n].name = NULL;}

    if(directory_iso2) free(directory_iso2); directory_iso2 = NULL;
    free(split_file); split_file = NULL;

    t_finish = get_ticks();

    DPrintf("\n>> Patch ISO completed.\n\n");
    DPrintf("Total Time (HH:MM:SS): %2.2u:%2.2u:%2.2u.%u\nPARAM.SFO patched: %c\nSPRX/SELF patched: %i\n\n", (u32) (t_finish - t_start)/(TICKS_PER_SEC * 3600),
           (u32) ((t_finish - t_start) / (TICKS_PER_SEC *  60)) % 60, (u32) ((t_finish - t_start)/(TICKS_PER_SEC)) % 60,
           (u32) ((t_finish - t_start) / (TICKS_PER_SEC / 100)) % 100, param_patched ? 'Y' : 'N', self_sprx_patched);

    u64 file_size = ((u64) toc) * 2048ULL;
    if(file_size < 1024ULL)
        DPrintf("Total ISO Size %u Bytes\n", (u32) file_size);
    else if(file_size < 0x100000LL)
        DPrintf("Total ISO Size %u KB\n", (u32) (file_size/1024));
    else
        DPrintf("Total ISO Size %u MB\nTotal folders %i\nTotal files %i\n", (u32) (file_size/0x100000LL), num_dir - 1, num_files);

    if(nopause) return SUCCESS;

    press_button_to_continue();
    return SUCCESS;

err:

    if(fd) ps3ntfs_close(fd);
    if(split_index && fd_split) {ps3ntfs_close(fd_split); fd_split = -1;}

    if(sectors ) free(sectors ); sectors  = NULL;
    if(sectors2) free(sectors2); sectors2 = NULL;
    if(sectors3) free(sectors3); sectors3 = NULL;

    for(n = 0; n <= idx; n++)
        if(directory_iso2[n].name) {free(directory_iso2[n].name); directory_iso2[n].name = NULL;}

    if(directory_iso2) free(directory_iso2); directory_iso2 = NULL;
    free(split_file); split_file = NULL;

    press_button_to_continue();
    return FAILED;
}

int delps3iso(char *f_iso)
{
    int len, n;
    struct stat s;

    strcpy(output_name, f_iso);

    len = strlen(output_name);

    DCls();

    DbgHeader("DELPS3ISO Utility");

    DPrintf("\nDeleting...\n\n");

    if(len >= 6 && !strcasecmp(&output_name[len - 6], ".iso.0"))
    {
        output_name[len - 2] = 0;

        for(n = 0; n < 64; n++)
        {
            sprintf(output_name2, "%s.%i", output_name, n);

            if(!ps3ntfs_stat(output_name2, &s))
            {
                DPrintf("%s - Deleted\n", output_name2);
                ps3ntfs_unlink(output_name2);
            }
            else
                break;
        }

    }
    else
    {
        DPrintf("%s - Deleted\n", output_name);
        ps3ntfs_unlink(output_name);
    }

    press_button_to_continue();
    return SUCCESS;
}
