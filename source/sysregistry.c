/*
    (c) 2013 Hermes/Estwald <www.elotrolado.net>
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

#include "utils.h"
#include <sys/systime.h>

struct _PS3TimeZone {

    int ftime;
    char *name;

} PS3TimeZone[128] = {

{-1100, "American Samoa"},
{-1100, "Midway Islands"},
{-1000, "Hawaii"},
{-900, "Alaska"},
{-800, "US, Canada (Pacific Time)"},
{-800, "Tijuana"},
{-700, "US, Canada (Mountain Time)"},
{-700, "Chihuahua"},
{-600, "San Jose"},
{-600, "US, Canada (Central Time)"},
{-600, "Mexico City"},
{-500, "Quito"},
{-500, "US, Canada (Eastern Time)"},
{-500, "Panama City"},
{-500, "Bogota"},
{-500, "Lima"},
{-400, "Caracas"},
{-400, "Santiago"},
{-400, "Canada (Atlantic)"},
{-400, "Puerto Rico"},
{-400, "La Paz"},
{-350, "Newfoundland"},
{-300, "Sao Paulo"},
{-300, "Buenos Aires"},
{-100, "Azores"},
{0, "Casablanca"},
{0, "Dublin"},
{0, "Lisbon"},
{0, "Reykjavik"},
{0, "London"},
{0, "Amsterdan"},
{100, "Algiers"},
{100, "Andorra"},
{100, "Vienna"},
{100, "Windhoek"},
{100, "Oslo"},
{100, "Copenhage"},
{100, "Zagreb"},
{100, "Sarajevo"},
{100, "San Marino"},
{100, "Skopje"},
{100, "Stockholm"},
{100, "Tunis"},
{100, "Tirana"},
{100, "Paris"},
{100, "Valleta"},
{100, "Vaduz"},
{100, "Budapest"},
{100, "Bratislava"},
{100, "Prague"},
{100, "Brussels"},
{100, "Belgrade"},
{100, "Berlin"},
{100, "Bern"},
{100, "Madrid"},
{100, "Monaco"},
{100, "Ljubljana"},
{100, "Luxembourg"},
{100, "Rome"},
{100, "Warsaw"},
{200, "Athens"},
{200, "Amman"},
{200, "Istanbul"},
{200, "Jerusalem"},
{200, "Cairo"},
{200, "Kiev"},
{200, "Sofia"},
{200, "Damascus"},
{200, "Tallinn"},
{200, "Nicosia"},
{200, "Vilnius"},
{200, "Bucharest"},
{200, "Beirut"},
{200, "Helsinki"},
{200, "Minsk"},
{200, "Johannesburg"},
{200, "Riga"},
{300, "Kuwait City"},
{300, "Baghdad"},
{300, "Manama"},
{300, "Moskow"},
{300, "Riyadh"},
{350, "Tehran"},
{400, "Abu Dhabi"},
{400, "Yerevan"},
{400, "Tbilisi"},
{400, "Baku"},
{400, "Muscat"},
{450, "Kabul"},
{500, "Karachi"},
{500, "Tashkent"},
{500, "Bishkek"},
{550, "Calcutta"},
{575, "Kathmandu"},
{600, "Astana"},
{600, "Dhaka"},
{700, "Bangkok"},
{800, "Kuala Lumpur"},
{800, "Singapore"},
{800, "Taipei"},
{800, "Perth"},
{800, "Beijing"},
{800, "Hong Kong"},
{900, "Seoul"},
{900, "Tokyo"},
{950, "Adelaide"},
{1000, "Sydney"},
{1200, "Wellington"},
{1200, "Suva"},
{1300, "Independent State of Samoa"},
};

int sys_language = 0;
int sys_timezone = 29;
int sys_dateformat = 1;
int sys_summer = 0;
int sys_parental_level = 9;
int sys_button_layout = 1; //0 = O is enter, 1 = X is enter

int read_from_registry()
{
    int file_size;
    int n, ret = -2;

    u16 str_offset;
    u16 str_len;
    u16 data_len;
    int found = 0;


    u8 * mem = (u8 *) LoadFile("/dev_flash2/etc/xRegistry.sys", &file_size);

    if(!mem) return -1;

    n = 0xfff0;
    if(mem[n] != 0x4D || mem[n + 1] != 0x26) goto exitloop;
    n += 2;

    while(n < file_size - 4)
    {
        if(mem[n + 0] == 0xAA && mem[n + 1] == 0xBB && mem[n + 2] == 0xCC && mem[n + 3] == 0xDD) break;
        str_offset= ((mem[n+2]<<8) | mem[n+3]) + 0x10;

        data_len= (mem[n+6]<<8) | mem[n+7];

        str_len = (mem[str_offset + 2]<<8) | mem[str_offset + 3];

        if(str_len == 24 && !strcmp((char *) &mem[str_offset + 5], "/setting/system/language"))
        {
            memcpy(&ret, &mem[n+9], 4); sys_language = ret;
            found++; if(found >= 6) break;
        }

        if(str_len == 28 && !strcmp((char *) &mem[str_offset + 5], "/setting/dateTime/summerTime"))
        {
            memcpy(&ret, &mem[n+9], 4);  sys_summer = ret;
            found++; if(found >= 6) break;
        }

        if(str_len == 26 && !strcmp((char *) &mem[str_offset + 5], "/setting/dateTime/timeZone"))
        {
            memcpy(&ret, &mem[n+9], 4);  sys_timezone = ret;
            found++; if(found >= 6) break;
        }

        if(str_len == 28 && !strcmp((char *) &mem[str_offset + 5], "/setting/dateTime/dateFormat"))
        {
            memcpy(&ret, &mem[n+9], 4);  sys_dateformat = ret;
            found++; if(found >= 6) break;
        }

        if(str_len == 27 && !strcmp((char *) &mem[str_offset + 5], "/setting/parental/gameLevel"))
        {
            memcpy(&ret, &mem[n+9], 4);  sys_parental_level = ret;
            found++; if(found >= 6) break;
        }

        if(str_len == 28 && !strcmp((char *) &mem[str_offset + 5], "/setting/system/buttonAssign"))
        {
            memcpy(&ret, &mem[n+9], 4);  sys_button_layout = ret;
            found++; if(found >= 6) break;
        }

        n += 10 + data_len;
    }


exitloop:
    if(found == 6) ret = 0; else ret = -2;
    free(mem);
    return ret;
}


void PS3GetDateTime(u32 * hh, u32 * mm, u32 * ss, u32 * day, u32 * month, u32 * year)
{
    u64 sec, nsec;
    int n;

    if(!hh || !mm || !ss || !day || !month || !year) return;

    if(sysGetCurrentTime(&sec, &nsec) < 0) sec = 0ULL;

    sec += (s64) (3600LL * (s64) PS3TimeZone[sys_timezone % 110].ftime)/100LL; // time zone correction

    if(sys_summer) sec += 3600ULL;

    int bi = 0;

    *hh = (sec / 3600ULL) % 24;

    *mm = (sec / 60ULL) % 60;

    *ss = (sec % 60);

    sec = (sec / 86400ULL); // total days

    *year = sec / 365ULL;

    *day = ((u32) sec) - (*year * 365) - ((*year + 1)/4); // days in the year

    int dd = *(day);

    if(dd < 0)
    {
        (*year) --;

        *day = ((u32) sec) - (*year * 365) - ((*year + 1)/4); // days in the year
    }

    *year += 1970;

    if((*year % 4) == 0) bi = 1;

    static u32 day_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    *month = 1;

    for(n= 0; n < 12; n++)
    {
        if(*day < day_month[n] + ((n==1) && bi))
        {
            *day += 1;
            *month = n + 1;
            break;
        }

        *day -= day_month[n] + ((n==1) && bi);
    }

}
