/*
 *    functions.c: some functions used throughout OpenPS3FTP
 *    Copyright (C) 2011 John Olano (jjolano)
 *
 *    This file is part of OpenPS3FTP.
 *
 *    OpenPS3FTP is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    OpenPS3FTP is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with OpenPS3FTP.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ppu-types.h>

#include <sys/file.h>

#include "defines.h"
#include "functions.h"
#include "ntfs.h"

void abspath(const char *relpath, const char *cwd, char *abspath)
{
    if(relpath[0] == '/')
    {
        strcpy(abspath, relpath);
    }
    else
    {
        strcpy(abspath, cwd);

        if(cwd[strlen(cwd) - 1] != '/')
        {
            strcat(abspath, "/");
        }

        strcat(abspath, relpath);
    }
}

int exists(const char *path)
{
    if(!strncmp(path, "/ntfs", 5) || !strncmp(path, "/ext", 4))
    {
        struct stat st;
        return ps3ntfs_stat(path, &st);
    }

    sysFSStat stat;
    return sysLv2FsStat(path, &stat);
}

int is_dir(const char *path)
{
    if(!strncmp(path, "/ntfs", 5) || !strncmp(path, "/ext", 4))
    {
        struct stat st;
        return ps3ntfs_stat(path, &st) == 0 && fis_dir(st);
    }

    sysFSStat stat;
    return (sysLv2FsStat(path, &stat) == 0 && fis_dir(stat));
}

int strpos(const char *haystack, int needle)
{
    char *p = strchr(haystack, needle);
    return p ? (p - haystack) : -1;
}

int strsplit(const char *str, char *left, int lmaxlen, char *right, int rmaxlen)
{
    int sp = strpos(str, ' ');
    int len = strlen(str);

    int lmax = low((sp > 0) ? sp : len, lmaxlen);
    strncpy(left, str, lmax);
    left[lmax] = '\0';

    if(sp > 0 && len > sp)
    {
        int rmax = low((len - sp - 1), rmaxlen);
        strncpy(right, str + sp + 1, rmax);
        right[rmax] = '\0';

        return 1;
    }

    return 0;
}

void strreplace(char *str, int oldc, int newc)
{
    char *pos;
    while((pos = strchr(str, oldc)) != NULL)
    {
        *pos = newc;
    }
}

void strtoupper(char *str)
{
    do if(*str > 96 && *str < 123) *str &= 223;
    while(*str++);
}
