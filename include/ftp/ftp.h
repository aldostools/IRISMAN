#ifndef FTP_H
#define FTP_H

#include "utils.h"

extern int ftp_perms;
extern char ftp_ip_str[256];
extern int ftp_port;

int get_ftp_activity();

int ftp_net_init();
int ftp_net_deinit();
int ftp_net_status();

int ftp_init();
void ftp_deinit();

#endif
