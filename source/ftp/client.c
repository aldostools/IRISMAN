/*
 *    client.c: handles clients, queries, and data
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
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <ppu-types.h>

#include <net/net.h>
#include <net/netctl.h>
#include <arpa/inet.h>

#include <sys/file.h>
#include <sys/thread.h>

#include "defines.h"
#include "client.h"
#include "functions.h"
#include "utils.h"
#include "ntfs.h"
#include "file_manager.h"

#define FS_S_IFMT 0170000

int cqueue = 0;
int dqueue = 0;

volatile int ftp_working;

extern u64 pad_last_time; // to prevent shutdown when FTP is used

int sys_fs_mount(char const* deviceName, char const* deviceFileSystem, char const* devicePath, int writeProt);
int sys_fs_umount(char const* devicePath);
bool isDir( char* path );

static void parse_wildcard(char *param, char *cwd, char *tmp_path, char *wcard, int split)
{
    char *pw, *ps; *wcard = 0;

    pw = strchr(param, '*');if(pw) {ps = strrchr(param, '/'); if((ps > param) && (ps < pw)) pw = ps; while(*pw == '*' || *pw == '/') *pw++ = 0; strcpy(wcard, pw); pw = strstr(wcard, "*"); if(pw) *pw = 0; if(!*wcard && !ps) strcpy(wcard, param);}

    if(*param == 0) split = 0;

    if(split == 1)
    {
        abspath(param, cwd, tmp_path);
    }

    if(split != 1 || !isDir(tmp_path)) strcpy(tmp_path, cwd);

}

void client_thread(void *conn_s_p)
{
    int conn_s = *(int *)conn_s_p;  // control connection socket
    int data_s = -1;        // data connection socket
    int pasv_s = -1;        // pasv listener socket

    int authorized = 0;     // 1 if successfully logged in
    long long rest = 0;     // for resuming transfers

    char temp[512];         // temporary storage of strings
    char expectcmd[16];     // for commands like RNFR, USER, etc.
    char cwd[256];          // current working directory

    size_t bytes;
    int itemp;

    bool is_copying = false;

    char cmd[16];
    char param[496];
    char source[256];
    char user[16];
    char tmp_path[256];
    char wcard[256];

    bytes = sprintf(temp, "220 OpenPS3FTP %s by jjolano (IRISMAN)\r\n", OFTP_VERSION);
    send(conn_s, temp, bytes, 0);

    while(appstate != 1 && (bytes = recv(conn_s, temp, 511, 0)) > 0)
    {
        // check if client sent a valid message
        char *p = strstr(temp, "\r\n");

        if(p != NULL)
        {
            strcpy(p, "\0\0");
        }
        else
        {
            break;
        }

        // experimental queue system
        // to try and prevent some io crashes
        itemp = cqueue;
        cqueue++;

        while((cqueue - 1) > itemp);

        // get command and parameter
        itemp = strsplit(temp, cmd, 15, param, 495);

        strtoupper(cmd);

        // check expected command
        if(!is_empty(expectcmd) && strcasecmp(cmd, expectcmd) != 0)
        {
            *cmd = '\0';

            bytes = ftpresp(temp, 503, "Bad command sequence");
            send(conn_s, temp, bytes, 0);
        }

        *expectcmd = '\0';

        // parse commands
        if(is_empty(cmd))
        {
            continue;
        }
        else
        if(strcasecmp(cmd, "NOOP") == 0)
        {
            ftp_working = 0;
            bytes = ftpresp(temp, 200, "NOOP command successful");
            send(conn_s, temp, bytes, 0);
        }
        else
        if((strcasecmp(cmd, "QUIT") == 0) || (strcasecmp(cmd, "BYE") == 0))
        {
            ftp_working = 0;
            bytes = ftpresp(temp, 221, "Goodbye");
            send(conn_s, temp, bytes, 0);
            break;
        }
        else
        if(strcasecmp(cmd, "CLNT") == 0)
        {
            bytes = ftpresp(temp, 200, "Cool story, bro");
            send(conn_s, temp, bytes, 0);
        }
        else
        if(strcasecmp(cmd, "FEAT") == 0)
        {
            char *feat[] =
            {
                "REST STREAM", "PASV", "PORT", "SIZE", "SITE", "APPE", "LIST", "MDTM", "MLSD",
                "MLST type*;size*;sizd*;modify*;UNIX.mode*;UNIX.uid*;UNIX.gid*;"
            };

            int feat_count = sizeof(feat) / sizeof(char *);
            int i = 0;

            bytes = sprintf(temp, "211-Features:\r\n");
            send(conn_s, temp, bytes, 0);

            for(; i < feat_count; i++)
            {
                bytes = sprintf(temp, " %s\r\n", feat[i]);
                send(conn_s, temp, bytes, 0);
            }

            bytes = ftpresp(temp, 211, "End");
            send(conn_s, temp, bytes, 0);
        }
        else
        if(strcasecmp(cmd, "HELP") == 0)
        {
            char *feat[] =
            {
                "CHMOD", "FLASH", "COPY", "PASTE", "RESTART", "RESET", "SHUTDOWN", "EXITAPP"
            };

            int feat_count = sizeof(feat) / sizeof(char *);
            int i = 0;

            bytes = sprintf(temp, "214-Help:\r\n");
            send(conn_s, temp, bytes, 0);

            for(; i < feat_count; i++)
            {
                bytes = sprintf(temp, " SITE %s\r\n", feat[i]);
                send(conn_s, temp, bytes, 0);
            }

            bytes = ftpresp(temp, 214, "End");
            send(conn_s, temp, bytes, 0);
        }
        else
        if(strcasecmp(cmd, "SYST") == 0)
        {
            bytes = ftpresp(temp, 215, "UNIX Type: L8");
            send(conn_s, temp, bytes, 0);
        }
        else
        if(strcasecmp(cmd, "ACCT") == 0)
        {
            bytes = ftpresp(temp, 202, "ACCT command ignored");
            send(conn_s, temp, bytes, 0);
        }
        else
        if(authorized == 1)
        {
            // logged in
            if(strcasecmp(cmd, "CWD") == 0 || strcasecmp(cmd, "XCWD") == 0)
            {
                if(!strcmp(param, "..")) goto cdup;

                abspath(param, cwd, temp);

                if(is_dir(temp))
                {
                    strcpy(cwd, temp);

                    bytes = ftpresp(temp, 250, "Directory change successful");
                }
                else
                {
                    bytes = ftpresp(temp, 550, "Cannot access directory");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "PWD") == 0 || strcasecmp(cmd, "XPWD") == 0)
            {
                bytes = sprintf(temp, "257 \"%s\" is the current directory\r\n", cwd);
                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "MKD") == 0 || strcasecmp(cmd, "XMKD") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, tmp_path);

                    if(ftp_working != 1) ftp_working = 2;

                    if(mkdir_secure(tmp_path) == 0)
                    {
                        bytes = sprintf(temp, "257 \"%s\" was successfully created\r\n", tmp_path);
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot create directory");
                    }
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "RMD") == 0 || strcasecmp(cmd, "XRMD") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, tmp_path);

                    if(ftp_working != 1) ftp_working = 2;

                    bytes = ftpresp(temp, 250, tmp_path);
                    send(conn_s, temp, bytes, 0);

                    if(is_ntfs_path(tmp_path))
                    {
                        if(tmp_path[strlen(tmp_path) - 1] == '/') tmp_path[strlen(tmp_path) - 1] = '\0';
                        ps3ntfs_unlink(tmp_path);
                    }
                    else if(rmdir_secure(tmp_path) == 0)
                    {
                        bytes = ftpresp(temp, 250, "Directory successfully removed");
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot access directory");
                    }
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "CDUP") == 0 || strcasecmp(cmd, "XCUP") == 0)
            {
cdup:           bytes = 0;
                int len = strlen(cwd) - 1;
                int c, i = len;

                for(; i > 0; i--)
                {
                    c = cwd[i];
                    cwd[i] = '\0';

                    if(c == '/' && i < len)
                    {
                        break;
                    }
                }

                bytes = ftpresp(temp, 200, "Directory change successful");
                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "PASV") == 0)
            {
                closesocket(data_s);
                closesocket(pasv_s);

                data_s = -1;

                struct sockaddr_in sa;
                socklen_t len = sizeof(sa);

                getsockname(conn_s, (struct sockaddr *)&sa, &len);

                // let the console choose port
                sa.sin_port = 0;

                pasv_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                if(bind(pasv_s, (struct sockaddr *)&sa, sizeof(sa)) == -1
                || listen(pasv_s, 1) == -1)
                {
                    closesocket(pasv_s);
                    pasv_s = -1;

                    bytes = ftpresp(temp, 451, "Failed to create PASV socket");
                }
                else
                {
                    rest = 0;

                    getsockname(pasv_s, (struct sockaddr *)&sa, &len);

                    bytes = sprintf(temp, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n",
                        (htonl(sa.sin_addr.s_addr) & 0xff000000) >> 24,
                        (htonl(sa.sin_addr.s_addr) & 0x00ff0000) >> 16,
                        (htonl(sa.sin_addr.s_addr) & 0x0000ff00) >>  8,
                        (htonl(sa.sin_addr.s_addr) & 0x000000ff),
                        (htons(sa.sin_port) & 0xff00) >> 8,
                        (htons(sa.sin_port) & 0x00ff));
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "PORT") == 0)
            {
                if(itemp == 1)
                {
                    closesocket(data_s);
                    closesocket(pasv_s);

                    data_s = -1;
                    pasv_s = -1;

                    short unsigned int a[4], p[2];
                    int i;

                    i = sscanf(param, "%3hu,%3hu,%3hu,%3hu,%3hu,%3hu", &a[0], &a[1], &a[2], &a[3], &p[0], &p[1]);

                    if(i == 6)
                    {
                        struct sockaddr_in sa;
                        memset(&sa, 0, sizeof(sa));

                        sa.sin_family = AF_INET;
                        sa.sin_port = htons(ftp_port(p[0], p[1]));
                        sa.sin_addr.s_addr = htonl(
                            ((unsigned char)(a[0]) << 24) +
                            ((unsigned char)(a[1]) << 16) +
                            ((unsigned char)(a[2]) <<  8) +
                            ((unsigned char)(a[3])));

                        data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                        if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                        {
                            closesocket(data_s);
                            data_s = -1;

                            bytes = ftpresp(temp, 451, "Failed to create PORT socket");
                        }
                        else
                        {
                            rest = 0;

                            bytes = ftpresp(temp, 200, "PORT command successful");
                        }
                    }
                    else
                    {
                        bytes = ftpresp(temp, 501, "Invalid PORT connection information");
                    }
                }
                else
                {
                    bytes = ftpresp(temp, 501, "No PORT connection information");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "ABOR") == 0)
            {
                closesocket(data_s);
                closesocket(pasv_s);

                data_s = -1;
                pasv_s = -1;

                bytes = ftpresp(temp, 225, "ABOR command successful");
                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "LIST") == 0)
            {
              list:
                parse_wildcard(param, cwd, tmp_path, wcard, itemp);

                if(is_ntfs_path(tmp_path))
                {
                    DIR_ITER *fd = NULL;

                    fd = ps3ntfs_diropen(tmp_path);
                    if(fd != NULL)
                    {
                        if(data_s == -1)
                        {
                            if(pasv_s > 0)
                            {
                                // passive
                                data_s = accept(pasv_s, NULL, NULL);

                                closesocket(pasv_s);
                                pasv_s = -1;
                            }
                            else
                            {
                                // legacy
                                struct sockaddr_in sa;
                                socklen_t len = sizeof(sa);

                                getpeername(conn_s, (struct sockaddr *)&sa, &len);
                                sa.sin_port = htons(20);

                                data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                                if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                                {
                                    closesocket(data_s);
                                    data_s = -1;
                                }
                            }

                            if(data_s == -1)
                            {
                                bytes = ftpresp(temp, 425, "No data connection");
                                send(conn_s, temp, bytes, 0);

                                continue;
                            }
                            else
                            {
                                bytes = ftpresp(temp, 150, "Opening data connection");
                                send(conn_s, temp, bytes, 0);
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 125, "Accepted data connection");
                            send(conn_s, temp, bytes, 0);
                        }

                        struct stat stat;
                        sysFSDirent entry;

                        while(ps3ntfs_dirnext(fd, entry.d_name, &stat) == 0)
                        {
                            pad_last_time = 0;

                            abspath(entry.d_name, tmp_path, temp);

                            bytes = sprintf(temp, "%s%s%s%s%s%s%s%s%s%s   1 nobody   nobody   %10llu Jan  1 00:00 %s\r\n",
                                fis_dir(stat) ? "d" : "-",
                                ((stat.st_mode & S_IRUSR) != 0) ? "r" : "-",
                                ((stat.st_mode & S_IWUSR) != 0) ? "w" : "-",
                                ((stat.st_mode & S_IXUSR) != 0) ? "x" : "-",
                                ((stat.st_mode & S_IRGRP) != 0) ? "r" : "-",
                                ((stat.st_mode & S_IWGRP) != 0) ? "w" : "-",
                                ((stat.st_mode & S_IXGRP) != 0) ? "x" : "-",
                                ((stat.st_mode & S_IROTH) != 0) ? "r" : "-",
                                ((stat.st_mode & S_IWOTH) != 0) ? "w" : "-",
                                ((stat.st_mode & S_IXOTH) != 0) ? "x" : "-",
                                (unsigned long long)stat.st_size, entry.d_name);

                            if(send(data_s, temp, bytes, 0) < 0) break;
                        }

                        bytes = ftpresp(temp, 226, "Transfer complete");
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot access NTFS/ext directory");
                    }

                    ps3ntfs_dirclose(fd);
                }
                else
                {
                    s32 fd;

                    if(sysLv2FsOpenDir(tmp_path, &fd) == 0)
                    {
                        if(data_s == -1)
                        {
                            if(pasv_s > 0)
                            {
                                // passive
                                data_s = accept(pasv_s, NULL, NULL);

                                closesocket(pasv_s);
                                pasv_s = -1;
                            }
                            else
                            {
                                // legacy
                                struct sockaddr_in sa;
                                socklen_t len = sizeof(sa);

                                getpeername(conn_s, (struct sockaddr *)&sa, &len);
                                sa.sin_port = htons(20);

                                data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                                if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                                {
                                    closesocket(data_s);
                                    data_s = -1;
                                }
                            }

                            if(data_s == -1)
                            {
                                bytes = ftpresp(temp, 425, "No data connection");
                                send(conn_s, temp, bytes, 0);

                                continue;
                            }
                            else
                            {
                                bytes = ftpresp(temp, 150, "Opening data connection");
                                send(conn_s, temp, bytes, 0);
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 125, "Accepted data connection");
                            send(conn_s, temp, bytes, 0);
                        }

                        sysFSStat stat;
                        sysFSDirent entry;
                        u64 read;

                        while(sysLv2FsReadDir(fd, &entry, &read) == 0 && read > 0)
                        {
                            if(*wcard && strcasestr(entry.d_name, wcard) == NULL) continue;

                            pad_last_time = 0;

                            if(strcmp2(tmp_path, "/") == 0 &&
                              (strcmp2(entry.d_name, "app_home") == 0 ||
                               strcmp2(entry.d_name, "host_root") == 0))
                            {
                                continue;
                            }

                            // experimental queue system
                            // to try and prevent some io crashes
                            itemp = dqueue;
                            dqueue++;

                            while((dqueue - 1) > itemp);

                            abspath(entry.d_name, tmp_path, temp);
                            sysLv2FsStat(temp, &stat);

                            char entry_date[14];
                            strftime(entry_date, 13, "%b %e %H:%M", localtime(&stat.st_mtime));

                            bytes = sprintf(temp, "%s%s%s%s%s%s%s%s%s%s 1 root  root  %13llu %s %s\r\n",
                                fis_dir(stat) ? "d" : "-",
                                ((stat.st_mode & S_IRUSR) != 0) ? "r" : "-",
                                ((stat.st_mode & S_IWUSR) != 0) ? "w" : "-",
                                ((stat.st_mode & S_IXUSR) != 0) ? "x" : "-",
                                ((stat.st_mode & S_IRGRP) != 0) ? "r" : "-",
                                ((stat.st_mode & S_IWGRP) != 0) ? "w" : "-",
                                ((stat.st_mode & S_IXGRP) != 0) ? "x" : "-",
                                ((stat.st_mode & S_IROTH) != 0) ? "r" : "-",
                                ((stat.st_mode & S_IWOTH) != 0) ? "w" : "-",
                                ((stat.st_mode & S_IXOTH) != 0) ? "x" : "-",
                                (unsigned long long)stat.st_size, entry_date, entry.d_name);

                            send(data_s, temp, bytes, 0);

                            dqueue--;
                        }

                        if(!strcmp(tmp_path, "/"))
                        {
                            for(int k = 0, i = 0; i < 8; i++)
                            {
                                if(mounts[i])
                                {
                                    bytes = sprintf(temp, "%s%s%s%s%s%s%s%s%s%s   1 nobody   nobody   %10llu Jan  1 00:00 ntfs%i:\r\n",
                                        fis_dir(stat) ? "d" : "-",
                                        ((stat.st_mode & S_IRUSR) != 0) ? "r" : "-",
                                        ((stat.st_mode & S_IWUSR) != 0) ? "w" : "-",
                                        ((stat.st_mode & S_IXUSR) != 0) ? "x" : "-",
                                        ((stat.st_mode & S_IRGRP) != 0) ? "r" : "-",
                                        ((stat.st_mode & S_IWGRP) != 0) ? "w" : "-",
                                        ((stat.st_mode & S_IXGRP) != 0) ? "x" : "-",
                                        ((stat.st_mode & S_IROTH) != 0) ? "r" : "-",
                                        ((stat.st_mode & S_IWOTH) != 0) ? "w" : "-",
                                        ((stat.st_mode & S_IXOTH) != 0) ? "x" : "-",
                                        (unsigned long long)stat.st_size, k++);

                                    if(send(data_s, temp, bytes, 0) < 0) break;
                                }
                            }
                        }

                        uint32_t blockSize;
                        uint64_t freeSize;
                        memset(param, 0, 495);
                        strcpy(param, tmp_path);
                        if(strchr(param+1, '/')) param[strchr(param+1, '/')-param]=0;

                        sysFsGetFreeSize(param, &blockSize, &freeSize);
                        freeSize = (((u64)blockSize * freeSize));

                        sprintf(param, "Transfer complete [ %i MB free ]", (int)freeSize>>20);
                        bytes = ftpresp(temp, 226, param);
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot access directory");
                    }

                    sysLv2FsCloseDir(fd);
                }

                send(conn_s, temp, bytes, 0);

                closesocket(data_s);
                data_s = -1;
            }
            else
            if(strcasecmp(cmd, "MLSD") == 0)
            {
                s32 fd;

                parse_wildcard(param, cwd, tmp_path, wcard, itemp);

                if(sysLv2FsOpenDir(tmp_path, &fd) == 0)
                {
                    if(data_s == -1)
                    {
                        if(pasv_s > 0)
                        {
                            // passive
                            data_s = accept(pasv_s, NULL, NULL);

                            closesocket(pasv_s);
                            pasv_s = -1;
                        }
                        else
                        {
                            // legacy
                            struct sockaddr_in sa;
                            socklen_t len = sizeof(sa);

                            getpeername(conn_s, (struct sockaddr *)&sa, &len);
                            sa.sin_port = htons(20);

                            data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                            if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                            {
                                closesocket(data_s);
                                data_s = -1;
                            }
                        }

                        if(data_s == -1)
                        {
                            bytes = ftpresp(temp, 425, "No data connection");
                            send(conn_s, temp, bytes, 0);

                            continue;
                        }
                        else
                        {
                            bytes = ftpresp(temp, 150, "Opening data connection");
                            send(conn_s, temp, bytes, 0);
                        }
                    }
                    else
                    {
                        bytes = ftpresp(temp, 125, "Accepted data connection");
                        send(conn_s, temp, bytes, 0);
                    }

                    sysFSStat stat;
                    sysFSDirent entry;
                    u64 read; char dirtype[2]; dirtype[1] = '\0';

                    bool is_root  = (strcmp2(tmp_path, "/") == 0);

                    while(sysLv2FsReadDir(fd, &entry, &read) == 0 && read > 0)
                    {
                        pad_last_time = 0;

                        if(is_root && (strcmp2(entry.d_name, "app_home") == 0 || strcmp2(entry.d_name, "host_root") == 0)) continue;

                        // experimental queue system
                        // to try and prevent some io crashes
                        itemp = dqueue;
                        dqueue++;

                        while((dqueue - 1) > itemp);

                        abspath(entry.d_name, tmp_path, temp);
                        sysLv2FsStat(temp, &stat);

                        char entry_date[15];
                        strftime(entry_date, 14, "%Y%m%d%H%M%S", localtime(&stat.st_mtime));

                        if(strcmp2(entry.d_name, ".") == 0)
                        {
                            *dirtype = 'c';
                        }
                        else
                        if(strcmp2(entry.d_name, "..") == 0)
                        {
                            *dirtype = 'p';
                        }
                        else
                        {
                            *dirtype = '\0';
                        }

                        bytes = sprintf(temp, "type=%s%s;siz%s=%llu;modify=%s;UNIX.mode=0%i%i%i;UNIX.uid=nobody;UNIX.gid=nobody; %s\r\n",
                            dirtype, fis_dir(stat) ? "dir" : "file",
                            fis_dir(stat) ? "d" : "e", (unsigned long long)stat.st_size, entry_date,
                            (((stat.st_mode & S_IRUSR) != 0) * 4 + ((stat.st_mode & S_IWUSR) != 0) * 2 + ((stat.st_mode & S_IXUSR) != 0) * 1),
                            (((stat.st_mode & S_IRGRP) != 0) * 4 + ((stat.st_mode & S_IWGRP) != 0) * 2 + ((stat.st_mode & S_IXGRP) != 0) * 1),
                            (((stat.st_mode & S_IROTH) != 0) * 4 + ((stat.st_mode & S_IWOTH) != 0) * 2 + ((stat.st_mode & S_IXOTH) != 0) * 1),
                            entry.d_name);

                        if(send(data_s, temp, bytes, 0) < 0) break;

                        dqueue--;
                    }

                    bytes = ftpresp(temp, 226, "Transfer complete");
                }
                else
                {
                    bytes = ftpresp(temp, 550, "Cannot access directory");
                }

                sysLv2FsCloseDir(fd);

                send(conn_s, temp, bytes, 0);

                closesocket(data_s);
                data_s = -1;
            }
            else
            if(strcasecmp(cmd, "MLST") == 0)
            {
                s32 fd;

                parse_wildcard(param, cwd, tmp_path, wcard, itemp);

                if(sysLv2FsOpenDir(tmp_path, &fd) == 0)
                {
                    bytes = sprintf(temp, "250-Directory Listing:\r\n");
                    send(conn_s, temp, bytes, 0);

                    sysFSStat stat;
                    sysFSDirent entry;
                    u64 read; char dirtype[2]; dirtype[1] = '\0';

                    bool is_root  = (strcmp2(tmp_path, "/") == 0);

                    while(sysLv2FsReadDir(fd, &entry, &read) == 0 && read > 0)
                    {
                        pad_last_time = 0;

                        if(is_root && (strcmp2(entry.d_name, "app_home") == 0 || strcmp2(entry.d_name, "host_root") == 0)) continue;

                        // experimental queue system
                        // to try and prevent some io crashes
                        itemp = dqueue;
                        dqueue++;

                        while((dqueue - 1) > itemp);

                        abspath(entry.d_name, tmp_path, temp);
                        sysLv2FsStat(temp, &stat);

                        char entry_date[15];
                        strftime(entry_date, 14, "%Y%m%d%H%M%S", localtime(&stat.st_mtime));

                        if(strcmp2(entry.d_name, ".") == 0)
                        {
                            *dirtype = 'c';
                        }
                        else
                        if(strcmp2(entry.d_name, "..") == 0)
                        {
                            *dirtype = 'p';
                        }
                        else
                        {
                            *dirtype = '\0';
                        }

                        bytes = sprintf(temp, "type=%s%s;siz%s=%llu;modify=%s;UNIX.mode=0%i%i%i;UNIX.uid=nobody;UNIX.gid=nobody; %s\r\n",
                            dirtype, fis_dir(stat) ? "dir" : "file",
                            fis_dir(stat) ? "d" : "e", (unsigned long long)stat.st_size, entry_date,
                            (((stat.st_mode & S_IRUSR) != 0) * 4 + ((stat.st_mode & S_IWUSR) != 0) * 2 + ((stat.st_mode & S_IXUSR) != 0) * 1),
                            (((stat.st_mode & S_IRGRP) != 0) * 4 + ((stat.st_mode & S_IWGRP) != 0) * 2 + ((stat.st_mode & S_IXGRP) != 0) * 1),
                            (((stat.st_mode & S_IROTH) != 0) * 4 + ((stat.st_mode & S_IWOTH) != 0) * 2 + ((stat.st_mode & S_IXOTH) != 0) * 1),
                            entry.d_name);

                        if(send(conn_s, temp, bytes, 0) < 0) break;

                        dqueue--;
                    }

                    bytes = ftpresp(temp, 250, "End");
                }
                else
                {
                    bytes = ftpresp(temp, 550, "Cannot access directory");
                }

                sysLv2FsCloseDir(fd);

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "NLST") == 0)
            {
                if(strcmp(param, "-l") == 0) {*param = 0; goto list;}

                parse_wildcard(param, cwd, tmp_path, wcard, itemp);

                if(is_ntfs_path(tmp_path))
                {
                    DIR_ITER *fd = NULL;

                    fd = ps3ntfs_diropen(tmp_path);
                    if(fd != NULL)
                    {
                        if(data_s == -1)
                        {
                            if(pasv_s > 0)
                            {
                                // passive
                                data_s = accept(pasv_s, NULL, NULL);

                                closesocket(pasv_s);
                                pasv_s = -1;
                            }
                            else
                            {
                                // legacy
                                struct sockaddr_in sa;
                                socklen_t len = sizeof(sa);

                                getpeername(conn_s, (struct sockaddr *)&sa, &len);
                                sa.sin_port = htons(20);

                                data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                                if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                                {
                                    closesocket(data_s);
                                    data_s = -1;
                                }
                            }

                            if(data_s == -1)
                            {
                                bytes = ftpresp(temp, 425, "No data connection");
                                send(conn_s, temp, bytes, 0);

                                continue;
                            }
                            else
                            {
                                bytes = ftpresp(temp, 150, "Opening data connection");
                                send(conn_s, temp, bytes, 0);
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 125, "Accepted data connection");
                            send(conn_s, temp, bytes, 0);
                        }

                        struct stat stat;
                        sysFSDirent entry;

                        while(ps3ntfs_dirnext(fd, entry.d_name, &stat) == 0)
                        {
                            pad_last_time = 0;
                            bytes = sprintf(temp, "%s\r\n", entry.d_name);
                            if(send(data_s, temp, bytes, 0) < 0) break;
                        }

                        bytes = ftpresp(temp, 226, "Transfer complete");
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot access NTFS/ext directory");
                    }

                    ps3ntfs_dirclose(fd);
                }
                else
                {
                    s32 fd;

                    if(sysLv2FsOpenDir(tmp_path, &fd) == 0)
                    {
                        if(data_s == -1)
                        {
                            if(pasv_s > 0)
                            {
                                // passive
                                data_s = accept(pasv_s, NULL, NULL);

                                closesocket(pasv_s);
                                pasv_s = -1;
                            }
                            else
                            {
                                // legacy
                                struct sockaddr_in sa;
                                socklen_t len = sizeof(sa);

                                getpeername(conn_s, (struct sockaddr *)&sa, &len);
                                sa.sin_port = htons(20);

                                data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                                if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                                {
                                    closesocket(data_s);
                                    data_s = -1;
                                }
                            }

                            if(data_s == -1)
                            {
                                bytes = ftpresp(temp, 425, "No data connection");
                                send(conn_s, temp, bytes, 0);

                                continue;
                            }
                            else
                            {
                                bytes = ftpresp(temp, 150, "Opening data connection");
                                send(conn_s, temp, bytes, 0);
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 125, "Accepted data connection");
                            send(conn_s, temp, bytes, 0);
                        }

                        sysFSDirent entry;
                        u64 read;

                        while(sysLv2FsReadDir(fd, &entry, &read) == 0 && read > 0)
                        {
                            pad_last_time = 0;
                            bytes = sprintf(temp, "%s\r\n", entry.d_name);
                            if(send(data_s, temp, bytes, 0) < 0) break;
                        }

                        if(!strcmp(tmp_path, "/"))
                        {
                            for(int k = 0, i = 0; i < 8; i++)
                            {
                                if(mounts[i])
                                {
                                    if(!strncmp(mounts[i]->name,"ntfs", 4))
                                        bytes = sprintf(temp, "ntfs%i:\r\n", k++);
                                    else
                                        bytes = sprintf(temp, "ext%i:\r\n", k++);

                                    if(send(data_s, temp, bytes, 0) < 0) break;
                                }
                            }
                        }

                        bytes = ftpresp(temp, 226, "Transfer complete");
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot access directory");
                    }

                    sysLv2FsCloseDir(fd);
                }

                send(conn_s, temp, bytes, 0);

                closesocket(data_s);
                data_s = -1;
            }
            else
            if(strcasecmp(cmd, "STOR") == 0 || strcasecmp(cmd, "APPE") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, temp);

                    if(is_ntfs_path(temp))
                    {
                        s32 fd;
                        fd = ps3ntfs_open(temp, O_WRONLY | O_CREAT | (strcasecmp(cmd, "APPE") == 0 ? O_APPEND : O_TRUNC), 0);

                        if(fd >= 0)
                        {
                            ftp_working = 1;
                            if(data_s == -1)
                            {
                                if(pasv_s > 0)
                                {
                                    // passive
                                    data_s = accept(pasv_s, NULL, NULL);

                                    closesocket(pasv_s);
                                    pasv_s = -1;
                                }
                                else
                                {
                                    // legacy
                                    struct sockaddr_in sa;
                                    socklen_t len = sizeof(sa);

                                    getpeername(conn_s, (struct sockaddr *)&sa, &len);
                                    sa.sin_port = htons(20);

                                    data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                                    if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                                    {
                                        closesocket(data_s);
                                        data_s = -1;
                                    }
                                }

                                if(data_s == -1)
                                {
                                    bytes = ftpresp(temp, 425, "No data connection");
                                    send(conn_s, temp, bytes, 0);

                                    continue;
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 150, "Opening data connection");
                                    send(conn_s, temp, bytes, 0);
                                }
                            }
                            else
                            {
                                bytes = ftpresp(temp, 125, "Accepted data connection");
                                send(conn_s, temp, bytes, 0);
                            }

                            char *databuf = malloc(OFTP_DATA_BUFSIZE);

                            if(databuf == NULL)
                            {
                                bytes = ftpresp(temp, 451, "Cannot allocate memory");
                            }
                            else
                            {
                                int err = 0;
                                u64 written, read;

                                if(strcasecmp(cmd, "STOR") == 0)
                                {
                                    //sysLv2FsFtruncate(fd, rest);
                                    ps3ntfs_seek64(fd, rest, SEEK_SET);
                                }

                                while((read = (u64)recv(data_s, databuf, OFTP_DATA_BUFSIZE, MSG_WAITALL)) > 0)
                                {
                                    // experimental queue system
                                    // to try and prevent some io crashes
                                    itemp = dqueue;
                                    dqueue++;

                                    while((dqueue - 1) > itemp);

                                    pad_last_time = 0;

                                    written = ps3ntfs_write(fd, databuf, (int) read);

                                    if(written < read)
                                    {
                                        err = 1;
                                        dqueue--;
                                        break;
                                    }

                                    dqueue--;
                                }

                                free(databuf);

                                if(err == 1)
                                {
                                    bytes = ftpresp(temp, 451, "Block write error");
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 226, "Transfer complete");
                                }
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 550, "Cannot access file");
                        }

                        ps3ntfs_close(fd);

                        ftp_working = 2;
                    }
                    else
                    {
                        s32 fd;

                        if(sysLv2FsOpen(temp, SYS_O_WRONLY | SYS_O_CREAT | (strcasecmp(cmd, "APPE") == 0 ? SYS_O_APPEND : 0), &fd, 0644, NULL, 0) == 0)
                        {
                            ftp_working = 1;
                            if(data_s == -1)
                            {
                                if(pasv_s > 0)
                                {
                                    // passive
                                    data_s = accept(pasv_s, NULL, NULL);

                                    closesocket(pasv_s);
                                    pasv_s = -1;
                                }
                                else
                                {
                                    // legacy
                                    struct sockaddr_in sa;
                                    socklen_t len = sizeof(sa);

                                    getpeername(conn_s, (struct sockaddr *)&sa, &len);
                                    sa.sin_port = htons(20);

                                    data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                                    if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                                    {
                                        closesocket(data_s);
                                        data_s = -1;
                                    }
                                }

                                if(data_s == -1)
                                {
                                    bytes = ftpresp(temp, 425, "No data connection");
                                    send(conn_s, temp, bytes, 0);

                                    continue;
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 150, "Opening data connection");
                                    send(conn_s, temp, bytes, 0);
                                }
                            }
                            else
                            {
                                bytes = ftpresp(temp, 125, "Accepted data connection");
                                send(conn_s, temp, bytes, 0);
                            }

                            char *databuf = malloc(OFTP_DATA_BUFSIZE);

                            if(databuf == NULL)
                            {
                                bytes = ftpresp(temp, 451, "Cannot allocate memory");
                            }
                            else
                            {
                                int err = 0;
                                u64 pos, written, read;

                                if(strcasecmp(cmd, "STOR") == 0)
                                {
                                    sysLv2FsFtruncate(fd, rest);
                                    sysLv2FsLSeek64(fd, rest, SEEK_SET, &pos);
                                }

                                while((read = (u64)recv(data_s, databuf, OFTP_DATA_BUFSIZE, MSG_WAITALL)) > 0)
                                {
                                    // experimental queue system
                                    // to try and prevent some io crashes
                                    itemp = dqueue;
                                    dqueue++;

                                    while((dqueue - 1) > itemp);

                                    pad_last_time = 0;

                                    if(sysLv2FsWrite(fd, databuf, read, &written) != 0 || written < read)
                                    {
                                        err = 1;
                                        dqueue--;
                                        break;
                                    }

                                    dqueue--;
                                }

                                //sysLv2FsFsync(fd);
                                free(databuf);

                                if(err == 1)
                                {
                                    bytes = ftpresp(temp, 451, "Block write error");
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 226, "Transfer complete");
                                }
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 550, "Cannot access file");
                        }

                        sysLv2FsClose(fd);
                        ftp_working = 2;
                    }
                }
                else
                {
                    bytes = ftpresp(temp, 501, "No file specified");
                }

                send(conn_s, temp, bytes, 0);

                closesocket(data_s);
                data_s = -1;
            }
            else
            if(strcasecmp(cmd, "RETR") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, temp);

                    if(is_ntfs_path(temp))
                    {
                        sysLv2FsChmod(temp, FS_S_IFMT | 0777);

                        int fd = ps3ntfs_open(temp, O_RDONLY, 0);

                        if(fd >= 0)
                        {
                            ftp_working = 1;
                            if(data_s == -1)
                            {
                                if(pasv_s > 0)
                                {
                                    // passive
                                    data_s = accept(pasv_s, NULL, NULL);

                                    closesocket(pasv_s);
                                    pasv_s = -1;
                                }
                                else
                                {
                                    // legacy
                                    struct sockaddr_in sa;
                                    socklen_t len = sizeof(sa);

                                    getpeername(conn_s, (struct sockaddr *)&sa, &len);
                                    sa.sin_port = htons(20);

                                    data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                                    if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                                    {
                                        closesocket(data_s);
                                        data_s = -1;
                                    }
                                }

                                if(data_s == -1)
                                {
                                    bytes = ftpresp(temp, 425, "No data connection");
                                    send(conn_s, temp, bytes, 0);

                                    continue;
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 150, "Opening data connection");
                                    send(conn_s, temp, bytes, 0);
                                }
                            }
                            else
                            {
                                bytes = ftpresp(temp, 125, "Accepted data connection");
                                send(conn_s, temp, bytes, 0);
                            }

                            char *databuf = malloc(OFTP_DATA_BUFSIZE);

                            if(databuf == NULL)
                            {
                                bytes = ftpresp(temp, 451, "Cannot allocate memory");
                            }
                            else
                            {
                                int err = 0;
                                u64 read;

                                ps3ntfs_seek64(fd, rest, SEEK_SET);

                                while(true)
                                {
                                    read = ps3ntfs_read(fd, databuf, OFTP_DATA_BUFSIZE);
                                    if(read <= 0) break;

                                    // experimental queue system
                                    // to try and prevent some io crashes
                                    itemp = dqueue;
                                    dqueue++;

                                    while((dqueue - 1) > itemp);

                                    pad_last_time = 0;

                                    if((u64)send(data_s, databuf, (size_t)read, 0) < read)
                                    {
                                        err = 1;
                                        dqueue--;
                                        break;
                                    }

                                    dqueue--;
                                }

                                free(databuf);

                                if(err == 1)
                                {
                                    bytes = ftpresp(temp, 451, "Block read/send error");
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 226, "Transfer complete");
                                }
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 550, "Cannot access file");
                        }

                        ps3ntfs_close(fd);
                    }
                    else
                    {
                        s32 fd;

                        if(sysLv2FsOpen(temp, SYS_O_RDONLY, &fd, 0, NULL, 0) == 0)
                        {
                            ftp_working = 1;
                            if(data_s == -1)
                            {
                                if(pasv_s > 0)
                                {
                                    // passive
                                    data_s = accept(pasv_s, NULL, NULL);

                                    closesocket(pasv_s);
                                    pasv_s = -1;
                                }
                                else
                                {
                                    // legacy
                                    struct sockaddr_in sa;
                                    socklen_t len = sizeof(sa);

                                    getpeername(conn_s, (struct sockaddr *)&sa, &len);
                                    sa.sin_port = htons(20);

                                    data_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

                                    if(connect(data_s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
                                    {
                                        closesocket(data_s);
                                        data_s = -1;
                                    }
                                }

                                if(data_s == -1)
                                {
                                    bytes = ftpresp(temp, 425, "No data connection");
                                    send(conn_s, temp, bytes, 0);

                                    continue;
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 150, "Opening data connection");
                                    send(conn_s, temp, bytes, 0);
                                }
                            }
                            else
                            {
                                bytes = ftpresp(temp, 125, "Accepted data connection");
                                send(conn_s, temp, bytes, 0);
                            }

                            char *databuf = malloc(OFTP_DATA_BUFSIZE);

                            if(databuf == NULL)
                            {
                                bytes = ftpresp(temp, 451, "Cannot allocate memory");
                            }
                            else
                            {
                                int err = 0;
                                u64 pos, read;

                                sysLv2FsLSeek64(fd, rest, SEEK_SET, &pos);

                                while(sysLv2FsRead(fd, databuf, OFTP_DATA_BUFSIZE, &read) == 0 && read > 0)
                                {
                                    // experimental queue system
                                    // to try and prevent some io crashes
                                    itemp = dqueue;
                                    dqueue++;

                                    while((dqueue - 1) > itemp);

                                    pad_last_time = 0;

                                    if((u64)send(data_s, databuf, (size_t)read, 0) < read)
                                    {
                                        err = 1;
                                        dqueue--;
                                        break;
                                    }

                                    dqueue--;
                                }

                                free(databuf);

                                if(err == 1)
                                {
                                    bytes = ftpresp(temp, 451, "Block read/send error");
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 226, "Transfer complete");
                                }
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 550, "Cannot access file");
                        }

                        sysLv2FsClose(fd);
                    }

                    ftp_working = 2;
                }
                else
                {
                    bytes = ftpresp(temp, 501, "No file specified");
                }

                send(conn_s, temp, bytes, 0);

                closesocket(data_s);
                data_s = -1;
            }
            else
            if(strcasecmp(cmd, "TYPE") == 0)
            {
                bytes = ftpresp(temp, 200, "TYPE command successful");
                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "STRU") == 0)
            {
                if(strcasecmp(cmd, "F") == 0)
                {
                    bytes = ftpresp(temp, 200, "STRU command successful");
                }
                else
                {
                    bytes = ftpresp(temp, 504, "STRU command failed");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "MODE") == 0)
            {
                if(strcasecmp(cmd, "S") == 0)
                {
                    bytes = ftpresp(temp, 200, "MODE command successful");
                }
                else
                {
                    bytes = ftpresp(temp, 504, "MODE command failed");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "REST") == 0)
            {
                if(itemp == 1)
                {
                    rest = atoll(param);

                    if(rest >= 0)
                    {
                        bytes = sprintf(temp, "350 Restarting at %llu\r\n", (unsigned long long)rest);
                    }
                    else
                    {
                        rest = 0;
                        bytes = ftpresp(temp, 501, "Invalid restart point");
                    }
                }
                else
                {
                    bytes = sprintf(temp, "350 Restarting at %llu\r\n", (unsigned long long)rest);
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "DELE") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, temp);

                    if(ftp_working != 1) ftp_working = 2;

                    if(unlink_secure(temp) == 0)
                    {
                        bytes = ftpresp(temp, 250, "File successfully removed");
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot remove file");
                    }
                }
                else
                {
                    bytes = ftpresp(temp, 501, "No file specified");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "RNFR") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, tmp_path);

                    if(file_exists(tmp_path))
                    {
                        strcpy(expectcmd, "RNTO");
                        bytes = ftpresp(temp, 350, "RNFR accepted - ready for destination");
                    }
                    else
                    {
                        *tmp_path = '\0';
                        bytes = ftpresp(temp, 550, "RNFR failed - file does not exist");
                    }
                }
                else
                {
                    bytes = ftpresp(temp, 501, "No file specified");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "RNTO") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, temp);

                    if(ftp_working != 1) ftp_working = 2;

                    if(rename_secure(tmp_path, temp) >= 0)
                    {
                        bytes = ftpresp(temp, 250, "File successfully renamed");
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot rename file");
                    }
                }
                else
                {
                    bytes = ftpresp(temp, 501, "No file specified");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "SITE") == 0)
            {
                if(itemp == 1)
                {
                    char param2[480];
                    itemp = strsplit(param, cmd, 15, param2, 479);

                    strtoupper(cmd);

                    if(strcasecmp(cmd, "CHMOD") == 0)
                    {
                        if(itemp == 1)
                        {
                            char perms[5], filename[256];
                            itemp = strsplit(param2, perms + 1, 3, filename, 256);

                            if(itemp == 1)
                            {
                                perms[0] = '0';

                                abspath(filename, cwd, temp);

                                if(ftp_working != 1) ftp_working = 2;

                                if(sysLv2FsChmod(temp, strtol(perms, NULL, 8)) == 0)
                                {
                                    bytes = ftpresp(temp, 250, "Successfully set file permissions");
                                }
                                else
                                {
                                    bytes = ftpresp(temp, 550, "Cannot set file permissions");
                                }
                            }
                            else
                            {
                                bytes = ftpresp(temp, 501, "Invalid CHMOD command syntax");
                            }
                        }
                        else
                        {
                            bytes = ftpresp(temp, 501, "Invalid CHMOD command syntax");
                        }
                    }
                    else if((strcasecmp(cmd, "QUITMM") == 0) || (strcasecmp(cmd, "EXITAPP") == 0))
                    {
                        appstate = 1;

                        bytes = ftpresp(temp, 221, "Exiting...");
                        exit(0);
                    }
                    else if((strcasecmp(cmd, "RESTART") == 0) || (strcasecmp(cmd, "REBOOT") == 0))
                    {
                        appstate = 1;

                        bytes = ftpresp(temp, 221, "Rebooting...");
                        sys_reboot();
                        exit(0);
                    }
                    else if((strcasecmp(cmd, "RESET") == 0))
                    {
                        appstate = 1;

                        bytes = ftpresp(temp, 221, "Resetting...");
                        sys_soft_reboot();
                        exit(0);
                    }
                    else if(strcasecmp(cmd, "SHUTDOWN") == 0)
                    {
                        appstate = 1;

                        bytes = ftpresp(temp, 221, "Shooting down...");
                        sys_shutdown();
                        exit(0);
                    }

                    else if(strcasecmp(cmd, "FLASH") == 0)
                    {
                        // mount/umount /dev_blind
                        if(file_exists("/dev_blind"))
                        {
                            sys_fs_umount("/dev_blind");
                        }
                        else
                        {
                            sys_fs_mount("CELL_FS_IOS:BUILTIN_FLSH1", "CELL_FS_FAT", "/dev_blind", 0);
                        }

                        bytes = ftpresp(temp, 200, file_exists("/dev_blind") ? "/dev_blind has been mounted" : "/dev_blind is unmounted");
                    }
                    else if(strcasecmp(cmd, "COPY") == 0)
                    {
                        abspath(param2, cwd, source);

                        if(is_ntfs_path(source))
                        {
                            memset(source, 0, 255);
                            bytes = ftpresp(temp, 504, "Remote copy is unavailable for ntfs/ext");
                        }
                        else if(file_exists(param2))
                        {
                            sprintf(param, "Copy %s", source);
                            bytes = ftpresp(temp, 200, param);
                        }
                        else
                        {
                            memset(source, 0, 255);
                            sprintf(param, "%s does not exist", strlen(param2) ? param2 : "Source");
                            bytes = ftpresp(temp, 550, param);
                        }
                    }

                    else if(strcasecmp(cmd, "PASTE") == 0)
                    {
                        if(is_copying)
                        {
                            bytes = ftpresp(temp, 451, "Another remote copy operation is in progress");
                        }
                        else if(is_ntfs_path(param2))
                        {
                            bytes = ftpresp(temp, 504, "Remote paste is unavailable for ntfs/ext");
                        }
                        else if((strlen(source) > 0) && file_exists(source))
                        {
                            is_copying = true;

                            abspath(param2, cwd, tmp_path);
                            strncpy(param2, tmp_path, 255);

                            bytes = ftpresp(temp, 350, "Copying...");
                            send(conn_s, temp, bytes, 0);

                            if(is_dir(source))
                                CopyDirectory(source, param2, tmp_path);
                            else
                                CopyFile(source, tmp_path);

                            if(file_exists(tmp_path))
                                bytes = ftpresp(temp, 250, "Copy completed");
                            else
                                bytes = ftpresp(temp, 550, "Copy failed");

                            is_copying = false;
                        }
                        else
                        {
                            sprintf(param, "%s does not exist", strlen(source) ? source : "Source");
                            bytes = ftpresp(temp, 550, param);
                        }
                    }
                    else
                    {
                        bytes = ftpresp(temp, 504, "Unrecognized SITE command");
                    }
                }
                else
                {
                    bytes = ftpresp(temp, 501, "No SITE command specified");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "SIZE") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, temp);

                    sysFSStat stat;
                    if(sysLv2FsStat(temp, &stat) == 0)
                    {
                        bytes = sprintf(temp, "213 %llu\r\n", (unsigned long long)stat.st_size);
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot access file");
                    }
                }
                else
                {
                    bytes = ftpresp(temp, 550, "No file specified");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "MDTM") == 0)
            {
                if(itemp == 1)
                {
                    abspath(param, cwd, temp);

                    sysFSStat stat;
                    if(sysLv2FsStat(temp, &stat) == 0)
                    {
                        char entry_date[15];
                        strftime(entry_date, 14, "%Y%m%d%H%M%S", localtime(&stat.st_mtime));
                        bytes = ftpresp(temp, 213, entry_date);
                    }
                    else
                    {
                        bytes = ftpresp(temp, 550, "Cannot access file");
                    }
                }
                else
                {
                    bytes = ftpresp(temp, 550, "No file specified");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "ALLO") == 0)
            {
                bytes = ftpresp(temp, 202, "ALLO command successful");
                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "USER") == 0 || strcasecmp(cmd, "PASS") == 0)
            {
                bytes = ftpresp(temp, 230, "You are already logged in");
                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "OPTS") == 0 || strcasecmp(cmd, "HELP") == 0
            || strcasecmp(cmd, "REIN") == 0 || strcasecmp(cmd, "ADAT") == 0
            || strcasecmp(cmd, "AUTH") == 0 || strcasecmp(cmd, "CCC") == 0
            || strcasecmp(cmd, "CONF") == 0 || strcasecmp(cmd, "ENC") == 0
            || strcasecmp(cmd, "EPRT") == 0 || strcasecmp(cmd, "EPSV") == 0
            || strcasecmp(cmd, "LANG") == 0 || strcasecmp(cmd, "LPRT") == 0
            || strcasecmp(cmd, "LPSV") == 0 || strcasecmp(cmd, "MIC") == 0
            || strcasecmp(cmd, "PBSZ") == 0 || strcasecmp(cmd, "PROT") == 0
            || strcasecmp(cmd, "SMNT") == 0 || strcasecmp(cmd, "STOU") == 0
            || strcasecmp(cmd, "XRCP") == 0 || strcasecmp(cmd, "XSEN") == 0
            || strcasecmp(cmd, "XSEM") == 0 || strcasecmp(cmd, "XRSQ") == 0
            || strcasecmp(cmd, "STAT") == 0)
            {
                bytes = ftpresp(temp, 502, "Command not implemented");
                send(conn_s, temp, bytes, 0);
            }
            else
            {
                bytes = ftpresp(temp, 500, "Unrecognized command");
                send(conn_s, temp, bytes, 0);
            }
        }
        else
        {
            // not logged in
            if(strcasecmp(cmd, "USER") == 0)
            {
                if(itemp == 1 && strlen(param) <= 15)
                {
                    strcpy(user, param);
                    strcpy(expectcmd, "PASS");

                    bytes = sprintf(temp, "331 Username %s OK. Password required\r\n", user);
                }
                else
                {
                    bytes = ftpresp(temp, 501, "Invalid username");
                }

                send(conn_s, temp, bytes, 0);
            }
            else
            if(strcasecmp(cmd, "PASS") == 0)
            {
                if(itemp == 1)
                {
                    if(is_empty(passwd) || ((strcasecmp(user, OFTP_LOGIN_USERNAME) == 0 || strcasecmp(user, "anonymous") == 0) && strcmp(param, passwd) == 0))
                    {
                        authorized = 1;
                        strcpy(cwd, "/");

                        bytes = ftpresp(temp, 230, "Successfully logged in");
                    }
                    else
                    {
                        bytes = ftpresp(temp, 430, "Invalid username or password");
                    }

                    send(conn_s, temp, bytes, 0);
                }
                else
                {
                    bytes = ftpresp(temp, 501, "Invalid password");
                    send(conn_s, temp, bytes, 0);
                }
            }
            else
            {
                bytes = ftpresp(temp, 530, "Not logged in");
                send(conn_s, temp, bytes, 0);
            }
        }

        cqueue--;
    }

    closesocket(conn_s);
    closesocket(data_s);
    closesocket(pasv_s);

    sysThreadExit(0);
}
