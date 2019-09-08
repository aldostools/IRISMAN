#ifndef OPF_DEFINES_H
#define OPF_DEFINES_H

#define OFTP_VERSION        "v2.3"

//#define OFTP_PASSWORD_FILE  "/dev_hdd0/game/IRISMAN00/USRDIR/ftp_pwd.txt"

#define OFTP_LOGIN_USERNAME "root"

#define OFTP_DATA_BUFSIZE   32768
#define OFTP_LISTEN_BACKLOG 32

#define OFTP_ERRMSG_THREAD  "An error occured when starting the main server thread.\n\nOpenPS3FTP cannot continue operation. Please restart OpenPS3FTP.\n\nIf this problem still occurs, please try rebooting your console or updating to the latest version of OpenPS3FTP."
#define OFTP_ERRMSG_NETWORK "No active network connection was detected. OpenPS3FTP will now exit."

// macros
#define NIPQUAD(addr) ((unsigned char *)&addr)[0], ((unsigned char *)&addr)[1], ((unsigned char *)&addr)[2], ((unsigned char *)&addr)[3]
#define FD(socket) ((socket)&~SOCKET_FD_MASK)
//#define closesocket(socket) netClose(FD(socket))

// PSL1GHT v2's net/socket.h doesn't seem to have this, but sprx/libnet/socket.c does
int closesocket(int socket);

extern char passwd[64];
extern int appstate;

#endif /* OPF_DEFINES_H */

