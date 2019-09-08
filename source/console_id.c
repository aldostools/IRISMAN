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

#include <lv2/process.h>

#include "main.h"
#include "console_id.h"

#include "payload355/payload_355.h"

#include "utils.h"
#include "osk_input.h"

extern u64 off_idps;
extern u64 off_idps2;
extern u64 val_idps_part1;
extern u64 val_idps_part2;

extern u64 off_psid;
extern u64 val_psid_part1;
extern u64 val_psid_part2;

extern char psid[33];
extern char console_id[33];
extern char default_console_id[33];


extern char tmp_path[MAXPATHLEN];
extern char self_path[MAXPATHLEN];

s32 open_device( u64 device_ID, u32* fd )
{
    lv2syscall4( 600, device_ID, 0, (u64)fd, 0 );
    return_to_user_prog(s32);
}

s32 read_device( u32 fd, u64 start_read_offset, u64 byte_to_read, const void* buffer, u32 *number_byte_read, u64 flags )
{
    lv2syscall7( 602, fd, 0, start_read_offset, byte_to_read, (u64)buffer, (u64)number_byte_read, flags );
    return_to_user_prog(s32);
}

s32 close_device( u32 fd)
{
    lv2syscall1( 601, fd );
    return_to_user_prog(s32);
}

void get_psid_lv2()
{
    u64 uPSID[2] = {0,0};
    lv2syscall1(872, (u64) uPSID); //PSID

    if((off_psid > 0) && ((uPSID[0] == 0 && uPSID[1] == 0) || !strncmp(console_id, psid, 32)))
    {
        uPSID[0] = lv2peek( off_psid );
        uPSID[1] = lv2peek( off_psid + 8 );
    }

    snprintf( psid, 33, "%016llX%016llX", (long long unsigned int)uPSID[0], (long long unsigned int)uPSID[1] );
}

void get_console_id_lv2()
{
    u64 uIDPS[2] = {0,0};
    lv2syscall1(870, (u64) uIDPS); //IDPS

    if((off_idps > 0) && (uIDPS[0] == 0 && uIDPS[1] == 0))
    {
        uIDPS[0] = lv2peek( off_idps );
        uIDPS[1] = lv2peek( off_idps + 8 );
    }

    snprintf( console_id, 33, "%016llX%016llX", (long long unsigned int)uIDPS[0], (long long unsigned int)uIDPS[1] );
}

void set_psid_lv2()
{
    if(is_valid_psid() == false) {DrawDialogOKTimer("Invalid PSID", 2000.0f); return;}

    u64 uPSID[2] = {0,0};
    lv2syscall1(872, (u64) uPSID); //PSID

    if((off_psid > 0) && ((uPSID[0] == 0 && uPSID[1] == 0) || !strncmp(console_id, psid, 32)))
    {
        uPSID[0] = lv2peek( off_psid );
        uPSID[1] = lv2peek( off_psid + 8 );
    }

    get_psid_val(); //val_psid_part1 & val_psid_part2 from PSID[]

    if (uPSID[0] == 0 || uPSID[1] == 0 || val_psid_part1 == 0 || val_psid_part2 == 0)
    {
        DrawDialogOKTimer("Invalid PSID", 2000.0f);
        return;
    }

    for(u64 j = 0x8000000000000000ULL; j < 0x8000000000600000ULL; j+=4)
    {
        if((peekq(j) == uPSID[0]) && (peekq(j+8) == uPSID[1]))
        {
            if(uPSID[0] != val_idps_part1) pokeq(j, val_psid_part1);
            j+=8;
            if(uPSID[1] != val_idps_part2) pokeq(j, val_psid_part2);
            j+=8;
        }
    }
}

void set_console_id_lv2()
{
    if(is_valid_idps() == 0) {DrawDialogOKTimer("Invalid IDPS", 2000.0f); return;}

    u64 uIDPS[2] = {0,0};
    lv2syscall1(870, (u64) uIDPS); //IDPS

    if(uIDPS[0] == 0 && uIDPS[1] == 0 && off_idps > 0)
    {
        uIDPS[0] = lv2peek( off_idps );
        uIDPS[1] = lv2peek( off_idps + 8 );
    }

    get_console_id_val(); //val_idps_part1 & val_idps_part2 from IDPS[]

    if (uIDPS[0] == 0 || uIDPS[1] == 0 || val_idps_part1 == 0 || val_idps_part2 == 0)
    {
        DrawDialogOKTimer("Invalid IDPS", 2000.0f);
        return;
    }

    for(u64 j = 0x8000000000000000ULL; j < 0x8000000000600000ULL; j+=4)
    {
        if((peekq(j) == uIDPS[0]) && (peekq(j+8) == uIDPS[1]))
        {
            if(uIDPS[0] != val_idps_part1) pokeq(j, val_idps_part1);
            j+=8;
            if(uIDPS[1] != val_idps_part2) pokeq(j, val_idps_part2);
            j+=8;
        }
    }
}

void get_console_id_eid5()
{
    u32 source, read;
    u64 offset_c;
    u64 buffer[ 0x40 ];
    int ret = 666;

    ret = open_device( 0x100000000000004ull, &source );

    if( ret != SUCCESS ) //PS3 has nand
    {
        offset_c = 0x20D;  //0x20d * 0x200
        close_device( source );
        open_device( 0x100000000000001ull, &source );
    }
    else            //PS3 has nor
        offset_c = 0x181;  //0x181 * 0x200

    read_device( source, offset_c, 0x1, buffer, &read, 0x22 );

    snprintf( console_id, 33, "%016llX%016llX", (long long unsigned int)buffer[ 0x3a ], (long long unsigned int)buffer[ 0x3b ] );

    close_device( source );
}

void get_psid_val()
{
    char tmp[ 17 ], tmp2[ 17 ];
    int i, y;

    for( i = 0, y = 0; i <= 31; i++ )
    {
        if( i > 15 )
        {
            tmp2[ y ] = psid[ i ];
            y++;
        }
        else
            tmp[ i ] = psid[ i ];
    }
    tmp[ 16 ] = '\0';

    val_psid_part1 = string_to_ull( tmp );
    val_psid_part2 = string_to_ull( tmp2 );
}

void get_console_id_val()
{
    char tmp[ 17 ], tmp2[ 17 ];
    int i, y;

    for( i = 0, y = 0; i <= 31; i++ )
    {
        if( i > 15 )
        {
            tmp2[ y ] = console_id[ i ];
            y++;
        }
        else
            tmp[ i ] = console_id[ i ];
    }
    tmp[ 16 ] = '\0';

    val_idps_part1 = string_to_ull( tmp );
    val_idps_part2 = string_to_ull( tmp2 );
}

int get_psid_keyb()
{
    if( Get_OSK_String_no_lang("PSID:", psid, 32) == SUCCESS )
    {
        if(is_valid_psid())
            return SUCCESS;
        else
            return FAILED;
    }
    else
        return 1;
}

int get_console_id_keyb()
{
    if( Get_OSK_String_no_lang("Console ID:", console_id, 32) == SUCCESS )
    {
        if(is_valid_idps())
            return SUCCESS;
        else
            return FAILED;
    }
    else
        return 1;
}

bool is_valid_psid()
{
    for(int i = 0; i <= 31; i++)
      if (is_hex_char(psid[i]) == false) return false;
    return true;
}

int is_valid_idps()
{
    if( (console_id[0] == '0' && console_id[1] == '0' && console_id[2] == '0' && console_id[3] == '0' &&
         console_id[4] == '0' && console_id[5] == '0' && console_id[6] == '0' && console_id[7] == '1' &&
         console_id[8] == '0' && console_id[9] == '0' && console_id[10]== '8' &&
         is_hex_char(console_id[11]) != 0 &&
         console_id[12]== '0' && console_id[13]== '0' && console_id[14]== '0') &&
         is_hex_char(console_id[15]) != 0 &&
        (console_id[16]== '0' || console_id[16]== '1' || console_id[16]== 'F' || console_id[16]== 'f' ) &&
        (console_id[17]== '0' || console_id[17]== '4' ))
    {
        for(int i = 18; i <= 31; i++)
        {
          if (is_hex_char(console_id[i]) == 0) return false;
        }
        return true;
    }
    return false;
}

bool is_hex_char(char c)
{
  if(c == 'A' || c == 'a' || c == '1' || c == '6' ||
     c == 'B' || c == 'b' || c == '2' || c == '7' ||
     c == 'C' || c == 'c' || c == '3' || c == '8' ||
     c == 'D' || c == 'd' || c == '4' || c == '9' ||
     c == 'E' || c == 'e' || c == '5' || c == '0' ||
     c == 'F' || c == 'f' ) return true;
  return false;
}

int save_spoofed_psid()
{
    sprintf(tmp_path, "%s/config/psid", self_path);
    return SaveFile(tmp_path, (char *) &psid, sizeof(psid) - 1);
}

int load_spoofed_psid()
{
    sprintf(tmp_path, "%s/config/psid", self_path);

    int file_size;
    char *file = LoadFile(tmp_path, &file_size);

    if(!file) return FAILED;
    if(file_size < 32) {free(file); return FAILED;}

    char tmp_psid[33];
    memcpy(tmp_psid, file, 32);
    tmp_psid[32] = 0;

    if( strcmp(psid, tmp_psid) == SUCCESS )
    {
        free(file);
        return 1;
    }

    memcpy(psid, file, 32);
    psid[32] = 0;

    set_psid_lv2();
    free(file);
    return SUCCESS;
}

int save_spoofed_console_id()
{
    sprintf(tmp_path, "%s/config/idps", self_path);
    return SaveFile(tmp_path, (char *) &console_id, sizeof(console_id) - 1);
}

int load_spoofed_console_id()
{
    sprintf(tmp_path, "%s/config/idps", self_path);

    int file_size;
    char *file = LoadFile(tmp_path, &file_size);

    if(!file) return FAILED;
    if(file_size < 32) {free(file); return FAILED;}

    char tmp_console_id[33];
    memcpy(tmp_console_id, file, 32);
    tmp_console_id[32] = 0;

    if( strcmp(console_id, tmp_console_id) == SUCCESS )
    {
        free(file);
        return 1;
    }

    memcpy(console_id, file, 32);
    console_id[32] = 0;

    set_console_id_lv2();
    free(file);
    return SUCCESS;
}
