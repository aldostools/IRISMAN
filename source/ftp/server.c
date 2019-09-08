/*
 *    server.c: handles socket connections
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
#include <ppu-types.h>

#include <net/net.h>
#include <net/netctl.h>
#include <arpa/inet.h>

#include <sys/thread.h>
#include <sys/systime.h>

#include "defines.h"
#include "server.h"
#include "client.h"
#include "ftp.h"

void listener_thread(void *unused)
{
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));

    sa.sin_family = AF_INET;
    sa.sin_port = htons(ftp_port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    int list_s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(bind(list_s, (struct sockaddr *)&sa, sizeof(sa)) == -1
    || listen(list_s, OFTP_LISTEN_BACKLOG) == -1)
    {
        appstate = 1;
    }

    int conn_s;
    sys_ppu_thread_t id;

    while(appstate != 1)
    {
        if((conn_s = accept(list_s, NULL, NULL)) > 0)
        {
            sysThreadCreate(&id, client_thread, (void *)&conn_s, 1337, 0x2000, THREAD_JOINABLE, "client");
            sysThreadYield();
        } else sysUsleep(250000);
    }

    closesocket(list_s);

    sysThreadExit(0);
}
