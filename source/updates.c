#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include "http/https.h"
#include "utils.h"
#include <sys/file.h>

#include "language.h"
#include "main.h"
#include "gfx.h"

#include "sysregistry.h"
#include "ttf_render.h"

#define FS_S_IFMT 0170000

// ----------------------------------------------------
#include <arpa/inet.h>

#define ssend(socket, str)

int closesocket(int socket);

void call_webman(const char *cmd)
{
	struct sockaddr_in sin;
	int s, len;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0x7F000001;	//127.0.0.1 (localhost)
	sin.sin_port = htons(80);			//http port (80)
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		return;
	}

	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		return;
	}


	char http[strlen(cmd) + 24];

	len = sprintf(http, "GET %s HTTP/1.0\r\n", cmd);

	send(s, http, len, 0);

	sleep(1);

	shutdown(s, SHUT_RDWR);
	closesocket(s);
}
// ----------------------------------------------------

extern volatile int dialog_action ;
void my_dialog(msgButton button, void *userdata);

void get_games();
int get_icon(char * path, const int num_dir);
int LoadTextureJPG(char * filename, int index);

extern char temp_buffer[8192];
extern int firmware;

extern char self_path[MAXPATHLEN];
extern char covers_path[MAXPATHLEN];
extern char updates_path[MAXPATHLEN];

extern bool show_http_errors;

#define MEM_MESSAGE_OFFSET  0x400
#define TEMP_PATH1_OFFSET   0x1400
#define TEMP_PATH2_OFFSET   0x1800

#define MEM_MESSAGE     temp_buffer + MEM_MESSAGE_OFFSET
#define TEMP_PATH1      temp_buffer + TEMP_PATH1_OFFSET
#define TEMP_PATH2      temp_buffer + TEMP_PATH2_OFFSET

static msgType mdialogprogress =   MSG_DIALOG_SINGLE_PROGRESSBAR | MSG_DIALOG_MUTE_ON;

static volatile int progress_action = 0;

static void progress_callback(msgButton button, void *userdata)
{
    switch(button)
    {
        case MSG_DIALOG_BTN_OK:
            progress_action = 1;
            break;

        case MSG_DIALOG_BTN_NO:
        case MSG_DIALOG_BTN_ESCAPE:
            progress_action = 2;
            break;

        case MSG_DIALOG_BTN_NONE:
            progress_action = -1;
            break;

        default:
            break;
    }
}


static void update_bar(u32 cpart)
{
    msgDialogProgressBarInc(MSG_PROGRESSBAR_INDEX0, (u32) cpart);
    sysUtilCheckCallback();tiny3d_Flip();
}


static void single_bar(char *caption)
{
    progress_action = 0;

    msgDialogOpen2(mdialogprogress, caption, progress_callback, (void *) 0xadef0044, NULL);

    msgDialogProgressBarReset(MSG_PROGRESSBAR_INDEX0);

    sysUtilCheckCallback();tiny3d_Flip();
}

int param_sfo_app_ver(char * path, char *app_ver)
{
    if(is_ntfs_path(path)) return -1;

    s32 fd;
    u64 bytes;
    u64 position = 0LL;

    unsigned char *mem = NULL;

    if(!sysLv2FsOpen(path, 0, &fd, S_IRWXU | S_IRWXG | S_IRWXO, NULL, 0))
    {
        unsigned len, pos, str;

        sysLv2FsLSeek64(fd, 0, 2, &position);
        len = (u32) position;

        if(len > 0x4000) {sysLv2FsClose(fd); return -2;}

        mem = (unsigned char *) malloc(len + 16);
        if(!mem) {sysLv2FsClose(fd); return -2;}

        memset(mem, 0, len + 16);

        sysLv2FsLSeek64(fd, 0, 0, &position);

        if(sysLv2FsRead(fd, mem, len, &bytes)!=0) bytes =0LL;

        len = (u32) bytes;

        sysLv2FsClose(fd);

        str = (mem[8] + (mem[9]<<8));
        pos = (mem[0xc] + (mem[0xd]<<8));

        int indx = 0;

        while(str < len)
        {
            if(mem[str] == 0) break;

            if(!strcmp((char *) &mem[str], "APP_VER"))
            {
                strncpy(app_ver, (char *) &mem[pos], 5);
                app_ver[5] = 0;
                break;
            }

            while(mem[str]) str++; str++;

            pos  += (mem[0x1c + indx] + (mem[0x1d + indx]<<8));
            indx += 16;
        }

        if(mem) free(mem);

        return 0;
    }

    return -1;
}

#include "event_threads.h"


static volatile struct f_async {
    int flags;
    FILE *fp;
    void * mem;
    int ret;
    int size;

} my_f_async;

#define ASYNC_ENABLE 128
#define ASYNC_ERROR 16

static void my_func_async(struct f_async * v)
{
    int ret = -1;

    if(v && v->flags & ASYNC_ENABLE) {
        v->ret = -1;
        int flags = v->flags;
        if(v->mem)
        {
            ret = (fwrite(v->mem, 1, v->size, v->fp) != v->size);
            v->ret = -ret;

            free(v->mem); v->mem = 0;
        }

        if(ret) flags|= ASYNC_ERROR;

        flags &= ~ASYNC_ENABLE;

        v->flags = flags;
    }
}
static int use_async_fd = 128;

static int download_update(char *url, char *file, int mode, u64 *size)
{
    void *ssl_p = NULL;
    void *cert_buffer = NULL;
    int cert_size = 0;

    int flags = 0;
    int ret = 0;
    void *http_p = NULL;
    void *uri_p = NULL;
    httpUri uri;
    s32 transID = 0;
    s32 clientID;
    int pool_size = 0;
    int recv = -1;
    u64 length = 0;
    int code = 0;
    FILE *fp = NULL;
    float parts = 0;
    float cpart;

    use_async_fd = 128;
    my_f_async.flags = 0;

    if(mode == 2)
    {
        if(size)
            sprintf(MEM_MESSAGE, "File Size: %u MB\n%s", (u32) (*size/0x100000ULL), strrchr(file, '/') + 1);
        else
            sprintf(MEM_MESSAGE, "File: %s", strrchr(file, '/') + 1);

        single_bar(MEM_MESSAGE);
    }

    http_p = malloc(0x10000);
    if(!http_p) {if(show_http_errors) DrawDialogOKTimer("Error malloc", 2000.0f); ret= -1; goto err;}

    ssl_p = malloc(0x40000);
    if(!ssl_p) {if(show_http_errors) DrawDialogOKTimer("Error malloc", 2000.0f); ret= -2; goto err;}

    ret = httpInit(http_p, 0x10000);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpInit", 2000.0f); goto err;}
    flags|= 1;

    // SSL
    ret = sslInit(ssl_p, 0x40000);
    if(ret < 0) goto err;
    flags|= 2;

    ret = sslCertificateLoader(SSL_LOAD_CERT_SCE, NULL, 0, &cert_size);
    if(ret < 0) goto err;

    cert_buffer = malloc(cert_size);
    if(!cert_buffer) {ret = -3; goto err;}

    ret = sslCertificateLoader(SSL_LOAD_CERT_SCE, cert_buffer, cert_size, NULL);
    if(ret < 0) goto err;

    httpsData caList;

    caList.ptr = cert_buffer;
    caList.size = cert_size;

    ret = httpsInit(1, (const httpsData *) &caList);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpsInit", 2000.0f); goto err;}
    flags|= 4;
    //

    ret = httpCreateClient(&clientID);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpCreateClient", 2000.0f); goto err;}
    flags|= 8;

    httpClientSetConnTimeout(clientID, 10000000);

    httpClientSetUserAgent(clientID, "Mozilla/5.0 (Windows NT 6.1; rv:21.0) Gecko/20100101 Firefox/27.0");

    ret = httpUtilParseUri(NULL, url, NULL, 0, &pool_size);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error init httpUtilParseUri", 2000.0f); goto err;}

    uri_p = malloc(pool_size);
    if (!uri_p)  {if(show_http_errors) DrawDialogOKTimer("Error malloc", 2000.0f); goto err;}

    ret = httpUtilParseUri(&uri, url, uri_p, pool_size, NULL);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpUtilParseUri", 2000.0f); goto err;}

    ret = httpCreateTransaction(&transID, clientID, HTTP_METHOD_GET, &uri);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpCreateTransaction", 2000.0f); goto err;}

    ret = httpSendRequest(transID, NULL, 0, NULL);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpSendRequest", 2000.0f); goto err;}

    ret = httpResponseGetStatusCode(transID, &code);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpResponseGetStatusCode", 2000.0f); goto err;}

    //if (code == 404) {ret = -4; goto err;}
    //if (code == 403) {ret = -4; goto err;}
    if (code >= 400 && code < 600) {ret = -4; goto err;} // 4xx (client error) / 5xx (server error)

    ret = httpResponseGetContentLength(transID, &length);
    if (ret < 0) {
        if (ret == HTTP_STATUS_CODE_No_Content) {
            length = 0ULL;
            ret = 0;
        } else {if(show_http_errors) DrawDialogOKTimer("Error httpResponseGetContentLength", 2000.0f); goto err;}
    }

    if(size) *size = length;

    if(mode == 1) goto err; // get only the size

    char buffer[16384];

    if(mode == 2) {
        parts = (length == 0) ? 0.0f : 100.0f / ((double) length / (double) sizeof(buffer));
        cpart = 0;
    }

    fp = fopen(file, "wb");
    if(!fp) {if(show_http_errors) DrawDialogOKTimer("Error saving file", 2000.0f); goto err;}

    int acum = 0;
    while (recv != 0 && length != 0ULL)
    {
        int n;

        for(n = 0; n < 5; n++)
        {
            memset(buffer, 0x0, sizeof(buffer));
            if (httpRecvResponse(transID, buffer, sizeof(buffer) - 1, &recv) < 0)
            {
                if(n == 4) {fclose(fp); ret = -5; goto err;}
                else sleep(1);
            }
            else break;
        }

        if (recv == 0) break;
        if (recv > 0) {
            //if(fwrite(buffer, 1, recv, fp) != recv) {fclose(fp); fp = NULL; ret = -6; goto err;}
            ///////////////////

loop_write:
            if(use_async_fd == 128)
            {
                use_async_fd = 1;
                my_f_async.flags = 0;
                my_f_async.size = recv;
                my_f_async.fp = fp;
                my_f_async.mem = malloc(recv);
                if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);
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
                        if(fp) fclose(fp);
                        fp = NULL; ret = -6;
                        goto err;
                    }

                    my_f_async.flags = 0;
                    my_f_async.size = recv;
                    my_f_async.fp = fp;
                    my_f_async.mem = malloc(recv);

                    if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);

                    my_f_async.ret = -1;
                    my_f_async.flags = ASYNC_ENABLE;
                    event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
                }
                else goto loop_write;
            }
            ///////////////////
            length -= recv;
            acum+= recv;
        }

        if(mode == 2 && progress_action == 2) {ret = -0x555; goto err;}

        pad_last_time = 0;

        if(mode == 2)
        {
            if(acum >= sizeof(buffer))
            {
                acum-= sizeof(buffer);
                cpart += parts;
                if(cpart >= 1.0f)
                {
                    update_bar((u32) cpart);
                    cpart-= (float) ((u32) cpart);
                }
            }
        }
    }

    ret = 0;

err:
    free(uri_p);
    uri_p = NULL;

    if(my_f_async.flags & ASYNC_ENABLE)
    {
        wait_event_thread();

        if(my_f_async.flags  & ASYNC_ERROR)
        {
            if(fp) fclose(fp);
            fp = NULL; ret = -6;
        }

        my_f_async.flags = 0;
    }

    event_thread_send(0x555ULL, (u64) 0, 0);

    if(mode == 2) msgDialogAbort();

    if(fp)
    {
        fclose(fp);
        if(ret < 0) unlink_secure(file);
    }

    if(transID) httpDestroyTransaction(transID);
    if(flags & 8) httpDestroyClient(clientID);
    if(flags & 4) httpsEnd();
    if(flags & 2) sslEnd();
    if(flags & 1) httpEnd();
    if(http_p) free(http_p);
    if(ssl_p)  free(ssl_p);
    if(cert_buffer) free(cert_buffer);
    if(uri_p) free(uri_p);
    return ret;
}

int download_file(char *url, char *file, int mode, u64 *size)
{
    int flags = 0;
    int ret = 0;
    void *http_p = NULL;
    void *uri_p = NULL;
    httpUri uri;
    s32 transID = 0;
    s32 clientID;
    int pool_size = 0;
    int recv = -1;
    u64 length = 0;
    int code = 0;
    FILE *fp = NULL;
    float parts = 0;
    float cpart;

    bool show_http_errors_original = show_http_errors, is_local = false;

    if(!url || *url == 0) show_http_errors = false; else
    if(!memcmp(url, "http://127.0.0.1/", 17) || !memcmp(url, "http://localhost/", 17)) {show_http_errors = false; is_local = true;}

    int no_len_flag = (mode & 128) != 0;
    mode &= 127;

    use_async_fd = 128;
    my_f_async.flags = 0;

    if(!file || *file == 0) show_http_errors = false; else
    if(mode == 2)
    {
        if(size)
            sprintf(MEM_MESSAGE, "File Size: %u MB\n%s", (u32) (*size/0x100000ULL), strrchr(file, '/') + 1);
        else
            sprintf(MEM_MESSAGE, "File: %s", strrchr(file, '/') + 1);

        single_bar(MEM_MESSAGE);
    }

    http_p = malloc(0x10000);
    if(!http_p) {if(show_http_errors) DrawDialogOKTimer("Error malloc", 2000.0f); ret= -1; goto err;}

    ret = httpInit(http_p, 0x10000);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpInit", 2000.0f); goto err;}
    flags|= 1;

    ret = httpCreateClient(&clientID);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpCreateClient", 2000.0f); goto err;}
    flags|= 2;

    httpClientSetConnTimeout(clientID, is_local ? 500000 : 20000000);

    httpClientSetUserAgent(clientID, TITLE_APP);

    ret = httpUtilParseUri(NULL, url, NULL, 0, &pool_size);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error init httpUtilParseUri", 2000.0f); goto err;}

    uri_p = malloc(pool_size);
    if (!uri_p)  {if(show_http_errors) DrawDialogOKTimer("Error malloc", 2000.0f); goto err;}

    ret = httpUtilParseUri(&uri, url, uri_p, pool_size, NULL);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpUtilParseUri", 2000.0f); goto err;}

    ret = httpCreateTransaction(&transID, clientID, HTTP_METHOD_GET, &uri);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpCreateTransaction", 2000.0f); goto err;}
    flags|= 4;

    ret = httpSendRequest(transID, NULL, 0, NULL);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpSendRequest", 2000.0f); goto err;}
    flags|= 8;

    ret = httpResponseGetStatusCode(transID, &code);
    if (ret < 0) {if(show_http_errors) DrawDialogOKTimer("Error httpResponseGetStatusCode", 2000.0f); goto err;}

    //if (code == 404) {ret = -4; goto err;}
    //if (code == 403) {ret = -4; goto err;}
    if (code >= 400 && code < 600) {ret = -4; goto err;} // 4xx (client error) / 5xx (server error)

    ret = httpResponseGetContentLength(transID, &length);

    if (ret < 0)
    {
        if(!no_len_flag)
        {
            if (ret == HTTP_STATUS_CODE_No_Content)
            {
                length = 0ULL;
                ret = 0;
            }
            else goto err;
        }
        else
        {
            if(size) *size = 0;
            length = 65536; // set length to a default value
        }
    } else if(size) *size = length;

    if(mode == 1) goto err; // get only the size

    char buffer[16384];

    if(mode == 2)
    {
        parts = (length == 0) ? 0.0f : 100.0f / ((double) length / (double) sizeof(buffer));
        cpart = 0;
    }

    if(!file) goto err;

    fp = fopen(file, "wb");
    if(!fp) {if(show_http_errors) DrawDialogOKTimer("Error saving file", 2000.0f); goto err;}

    if(!strncmp(file, "/dev_hdd0", 9)) sysFsChmod(file, FS_S_IFMT | 0777);

    int acum = 0;
    int acum2 = 0;

    while (recv != 0 && length != 0ULL)
    {
        int n;

        for(n = 0; n < 5; n++)
        {
            memset(buffer, 0x0, sizeof(buffer));
            if (httpRecvResponse(transID, buffer, sizeof(buffer) - 1, &recv) < 0)
            {
                if(n == 4) {fclose(fp); ret = -5; goto err;}
                else sleep(1);
            } else break;
        }

        if(recv == 0) break;

        if(recv > 0)
        {
            //if(fwrite(buffer, 1, recv, fp) != recv) {fclose(fp); fp = NULL; ret = -6; goto err;}
            ///////////////////
loop_write:
            if(use_async_fd == 128)
            {
                use_async_fd = 1;
                my_f_async.flags = 0;
                my_f_async.size = recv;
                my_f_async.fp = fp;
                my_f_async.mem = malloc(recv);
                if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);
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
                        if(fp) fclose(fp);
                        fp = NULL;
                        if(ret < 0) unlink_secure(file);
                        ret = -6; goto err;
                    }

                    my_f_async.flags = 0;
                    my_f_async.size = recv;
                    my_f_async.fp = fp;
                    my_f_async.mem = malloc(recv);

                    if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);

                    my_f_async.ret = -1;
                    my_f_async.flags = ASYNC_ENABLE;
                    event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
                }
                else goto loop_write;
            }
            ///////////////////
            length -= recv;
            acum+= recv;
            acum2+= recv;
        }

        if(mode == 2 && progress_action == 2) {ret = -0x555; goto err;}

        pad_last_time = 0;

        if(mode == 2)
        {
            if(acum >= sizeof(buffer))
            {
                acum-= sizeof(buffer);
                cpart += parts;
                if(cpart >= 1.0f)
                {
                    update_bar((u32) cpart);
                    cpart-= (float) ((u32) cpart);
                }
            }
        }
    }


    ret = 0;

    if(no_len_flag)
    {
        // return size readed
        if(size) *size = (u64) acum2;
    }

err:
    free(uri_p);
    uri_p = NULL;

    if(my_f_async.flags & ASYNC_ENABLE)
    {
        wait_event_thread();

        if(my_f_async.flags  & ASYNC_ERROR)
        {
            if(fp) fclose(fp);
            fp = NULL; ret = -6;
        }

        my_f_async.flags = 0;
    }

    event_thread_send(0x555ULL, (u64) 0, 0);

    if(mode == 2) msgDialogAbort();

    if(fp)
    {
        struct stat s;
        fclose(fp);
        if(ret == 0 && !stat(file, &s) && s.st_size == 0) ret = -4;
        if(ret < 0) unlink_secure(file);
    }

    if(transID) httpDestroyTransaction(transID);
    if(flags & 2) httpDestroyClient(clientID);
    if(flags & 1) httpEnd();
    if(http_p) free(http_p);
    if(uri_p) free(uri_p);

    show_http_errors = show_http_errors_original;

    return ret;
}

extern char hdd_folder[64];

static int game_up_mode = 0;

int cover_update(char *title_id)
{
    int ret;
    struct stat s;

    char id[10];
    memcpy(id, title_id, 4);
    id[4] = title_id[5]; id[5] = title_id[6]; id[6] = title_id[7]; id[7] = title_id[8]; id[8] = title_id[9]; id[9] = 0;

    {
        // checking ID from gametdb list
        sprintf(TEMP_PATH2, "%s/config/ps3tdb.txt", self_path);
        int file_size = 0;
        u8 *mem = (u8 *) LoadFile(TEMP_PATH2, &file_size);

        if(mem && file_size != 0)
        {
            int n, f = 0;

            for(n = 0; n < file_size - 9; n++)
            {
                if(!memcmp((char *) &mem[n], id, 9)) {f = 1; break;}
            }

            free(mem);

            if(!f) {return -1;}
        }
    }

    // covers
    sprintf(TEMP_PATH1, "%s", id);
    ret = get_icon(TEMP_PATH1, 0);

    if(ret < 0 || !game_up_mode)
    {
        if(ret != 0) game_up_mode = 1;

        if(stat(temp_buffer, &s))
        {
            mkdir_secure(temp_buffer);

            if(stat(temp_buffer, &s))
            {
                sprintf(temp_buffer, "%s", covers_path);
                sprintf(TEMP_PATH1, "%s%s.JPG", covers_path, id);
                mkdir_secure(temp_buffer);
            }
        }

        char region[17][4] = {
                                 "EN",
                                 "ES",
                                 "DE",
                                 "FR",
                                 "IT",
                                 "US",
                                 "AU",
                                 "NL",
                                 "PT",
                                 "SE",
                                 "DK",
                                 "NO",
                                 "FI",
                                 "TR",
                                 "KO",
                                 "RU",
                                 "JA",
                             };

        u8 *mem = NULL;

        sprintf(temp_buffer, "http://www.gametdb.com/PS3/%s.html", id);
        sprintf(TEMP_PATH2, "%s/tmp_cover.html", self_path);

        ret = download_file(temp_buffer, TEMP_PATH2, 128, NULL);
        if(ret) {sleep(1); ret = download_file(temp_buffer, TEMP_PATH2, 128, NULL);} // try again

        //sprintf(MEM_MESSAGE, "err: %i %x\n", ret, ret);
        //DrawDialogOKTimer(MEM_MESSAGE, 2000.0f);

        if(ret == 0)
        {
            int file_size = 0;
            u8 *mem = (u8 *) LoadFile(TEMP_PATH2, &file_size);

            if(mem && file_size != 0)
            {
                int n, m, l;
                mem[file_size - 1] = 0;

                unlink_secure(TEMP_PATH2); // remove temp html page

                n = -1;

                switch(sys_language)
                {
                    case LANG_GERMAN:
                        n = 2; break;
                    case LANG_ENGLISH:
                    case LANG_ENGLISHUK:
                        n = (id[2] == 'U') * 5; break;
                    case LANG_SPANISH:
                        n = 1; break;
                    case LANG_FRENCH:
                        n = 3; break;
                    case LANG_ITALIAN:
                        n = 4; break;
                    case LANG_DUTCH:
                        n = 7; break;
                    case LANG_PORTUGUESEB:
                    case LANG_PORTUGUESE:
                        n = 8; break;
                    case LANG_RUSSIAN:
                        n = 15; break;
                    case LANG_JAPANESE:
                        n = 16; break;
                    case LANG_KOREAN:
                        n = 14; break;
                    case LANG_FINNISH:
                        n = 12; break;
                    case LANG_SWEDISH:
                        n = 9; break;
                    case LANG_DANISH:
                        n = 2; break;
                    case LANG_NORWEGIAN:
                        n = 11; break;
                }

                // find from LANGUAGE cover link
                if(n >= 0)
                {
                    sprintf(temp_buffer, "http://art.gametdb.com/ps3/coverM/%s/%s.jpg", &region[n][0], id);

                    l = strlen(temp_buffer);
                    for(m = 0; m < file_size - l; m++)
                    {
                        if(!memcmp(mem + m, temp_buffer, l)) goto match;
                    }
                }

                // find JAPAN cover link
                if(id[2] == 'J')
                {
                    sprintf(temp_buffer, "http://art.gametdb.com/ps3/coverM/JA/%s.jpg", id);

                    l = strlen(temp_buffer);
                    for(m = 0; m < file_size - l; m++)
                    {
                        if(!memcmp(mem + m, temp_buffer, l)) goto match;
                    }
                }

                // find USA cover link
                if(id[2] == 'U')
                {
                    sprintf(temp_buffer, "http://art.gametdb.com/ps3/coverM/US/%s.jpg", id);

                    l = strlen(temp_buffer);
                    for(m = 0; m < file_size - l; m++)
                    {
                        if(!memcmp(mem + m, temp_buffer, l)) goto match;
                    }
                }

                // find a valid cover link
                for (n = 0; n < 17; n++)
                {
                    sprintf(temp_buffer, "http://art.gametdb.com/ps3/coverM/%s/%s.jpg", &region[n][0], id);
                    l = strlen(temp_buffer);
                    for(m = 0; m < file_size - l; m++)
                    {
                        if(!memcmp(mem + m, temp_buffer, l)) goto match;
                    }
                }

                free(mem);

                return -1;
            }
        }

        match:

        if(mem) free(mem);

        if(!game_up_mode) {
            sprintf(TEMP_PATH2, "%s/tmp_cover.png", self_path);
            ret = download_file(temp_buffer, TEMP_PATH2, 0, NULL);

            if(ret == 0)
            {
                if(LoadTextureJPG(TEMP_PATH2, 1) < 0)
                {
                    unlink_secure(TEMP_PATH2);
                    return -2;
                }
                else
                {
                    LoadTextureJPG(TEMP_PATH1, 0);

                    dialog_action = 0;

                    msgType mdialogyesno3 = MSG_DIALOG_NORMAL | MSG_DIALOG_BTN_TYPE_YESNO | MSG_DIALOG_DISABLE_CANCEL_ON | MSG_DIALOG_DEFAULT_CURSOR_NO | MSG_DIALOG_BKG_INVISIBLE;
                    msgDialogOpen2(mdialogyesno3, "Replace Left Cover with Right Cover?\n\nReemplazar Carátula Izquierda por Carátula Derecha?", my_dialog, (void*)  0x0000aaaa, NULL );

                    while(!dialog_action)
                    {
                        cls();

                        set_ttf_window(848/2 - 200 - 32, (512 - 230)/2 - 32, 200, 32, WIN_AUTO_LF);
                        display_ttf_string(0, 0, (char *) "       OLD COVER", 0xffffffff, 0, 16, 32);

                        if(Png_offset[0])
                        {
                            tiny3d_SetTextureWrap(0, Png_offset[0], Png_datas[0].width,
                                                 Png_datas[0].height, Png_datas[0].wpitch,
                                                 TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                            DrawTextBox(848/2 - 200 - 32, (512 - 230)/2 , 0, 200, 230, 0x8f8f8fff);
                        }

                        set_ttf_window(848/2 + 32, (512 - 230)/2 - 32, 200, 32, WIN_AUTO_LF);
                        display_ttf_string(0, 0, (char *) "       NEW COVER", 0xffffffff, 0, 16, 32);

                        if(Png_offset[1])
                        {
                            tiny3d_SetTextureWrap(0, Png_offset[1], Png_datas[1].width,
                                                 Png_datas[1].height, Png_datas[1].wpitch,
                                                 TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                            DrawTextBox(848/2 + 32, (512 - 230)/2 , 0, 200, 230, 0x8f8f8fff);
                        }

                        sysUtilCheckCallback();
                        tiny3d_Flip();
                    }

                    msgDialogAbort();
                    usleep(100000);

                    if(dialog_action != 1)
                    {
                        wait_event_thread();
                        get_games();
                        unlink_secure(TEMP_PATH2); return -1;
                    }

                    unlink_secure(TEMP_PATH1);
                    sysLv2FsRename(TEMP_PATH2, TEMP_PATH1);

                    return 0;
                }
            }

        }
        else
        {
            ret = download_file(temp_buffer, TEMP_PATH1, 0, NULL);

            if(ret == 0)
            {
                JpgDatas jpg;

                memset(&jpg, 0, sizeof(JpgDatas));

                if(LoadJPG(&jpg, TEMP_PATH1) < 0)
                {
                    if(jpg.bmp_out) free(jpg.bmp_out);
                    unlink_secure(TEMP_PATH1);
                    return -2;
                }
                else
                {
                    if(jpg.bmp_out) free(jpg.bmp_out);
                    return 0;
                }
            }
        }
    }

    return -1;
}


int covers_update(int pass)
{
    int n;
    int count = 0;

    float parts = 0;
    float cpart;
    int ret = 0, result = 0;

    for(n = 0; n < ndirectories; n++)
    {
        if((directories[n].flags & (HOMEBREW_FLAG | PS1_FLAG)) == 0)  count++;
    }

    if(count == 0)
    {
        if(show_http_errors) DrawDialogOKTimer("PS3 Games Not Found", 2000.0f);
        return 0;
    }

    parts = (count == 0) ? 0.0f : 100.0f / ((double) count);
    cpart = 0;

    // Download ps3tdb.txt from www.gametdb.com
    sprintf(temp_buffer, "http://www.gametdb.com/ps3tdb.txt");
    sprintf(TEMP_PATH2, "%s/config/ps3tdb.txt", self_path);

    for(n = 0; n < 10; n++)
    {
        if(n) sleep(1);
        ret = download_file(temp_buffer, TEMP_PATH2, 0, NULL);
        if(ret == 0) break; else {unlink_secure(TEMP_PATH2);}
    }
    //

    if(pass == 0)
        single_bar("Downloading Covers...");
    else
        single_bar("Downloading Covers #2...");

    for(n = 0; n < ndirectories; n++)
    {
        if((directories[n].flags & (HOMEBREW_FLAG | PS1_FLAG)) != 0)  continue;

        if(progress_action == 2) {result = -555; break;}

        game_up_mode = 1;
        ret = cover_update(directories[n].title_id);
        game_up_mode = 0;

        if(ret == -2) result = -1;

        cpart += parts;
        if(cpart >= 1.0f)
        {
            update_bar((u32) cpart);
            cpart-= (float) ((u32) cpart);
        }
    }

    msgDialogAbort();

    return result;
}

int locate_xml(u8 *mem, int pos, int size, char *key, int *last)
{
    int start = 0;
    int end = 0;

    int sig = 0;

    *last = 0;

    while(pos < size)
    {
        if(mem[pos] == '"' || sig)
        {
            if(mem[pos] == '"') sig^=1;
            pos++; continue;
        }

        if(mem[pos] == '<') return -1;
        if(!strncmp((char *) &mem[pos], "/>", 2) || mem[pos] == '>') return -2;

        if(!strncmp((char *) &mem[pos], key, strlen(key)))
        {
            pos += strlen(key); break;
        }

        pos++;
    }

    while(pos < size)
    {
        if(mem[pos] == '<') return -1;
        if(!strncmp((char *) &mem[pos], "/>", 2) || mem[pos] == '>') return -2;

        if(mem[pos] == '=')
        {
            pos++; break;
        }

        pos++;
    }

    while(pos < size)
    {
        if(mem[pos] == '<') return -1;
        if(!strncmp((char *) &mem[pos], "/>", 2) || mem[pos] == '>') return -2;

        if(mem[pos] == '"')
        {
            start = pos;
            pos++;
            break;
        }

        pos++;
    }

    while(pos < size)
    {
        if(mem[pos] == '<') return -1;
        if(!strncmp((char *) &mem[pos], "/>", 2) || mem[pos] == '>') return -2;

        if(mem[pos] == '"')
        {
            end = pos;
            break;
        }

        pos++;
    }

    if(pos >= size) return -3;

    *last = end;
    return start;
}

int game_update(char *title_id)
{
    char id[10];

    char version[6];
    char ver_app[6];
    char system[8];

    int list[128][2];

    int max_list = 0;

    int ret;

    memcpy(id, title_id, 4);
    id[4] = title_id[5]; id[5] = title_id[6]; id[6] = title_id[7]; id[7] = title_id[8]; id[8] = title_id[9]; id[9] = 0;

    game_up_mode = 0;

    // Download ps3tdb.txt from www.gametdb.com
    sprintf(temp_buffer, "http://www.gametdb.com/ps3tdb.txt");
    sprintf(TEMP_PATH2, "%s/config/ps3tdb.txt", self_path);

    for(int n = 0; n < 10; n++)
    {
        if(n) sleep(1);
        ret = download_file(temp_buffer, TEMP_PATH2, 0, NULL);
        if(ret == 0) break; else {unlink_secure(TEMP_PATH2);}
    }
    //

    ret = cover_update(title_id);

    if(ret == 0)
    {
        wait_event_thread();
        get_games();

        if(show_http_errors) DrawDialogOKTimer("Cover Downloaded", 2000.0f);
    }
    else if (ret == -2) if(show_http_errors) DrawDialogOKTimer("Invalid Cover", 2000.0f);

    sprintf(temp_buffer, "Do you want you update the game %s?", title_id);
    if(DrawDialogYesNoDefaultYes(temp_buffer) != YES) return 0;

    strcpy(ver_app, "00.00");

    sprintf(temp_buffer, "/dev_hdd0/game/%s/PARAM.SFO", id);
    param_sfo_app_ver(temp_buffer, ver_app);

    sprintf(temp_buffer, "https://a0.ww.np.dl.playstation.net/tpl/np/%s/%s-ver.xml", id, id);
    sprintf(TEMP_PATH1, "%s/temp.xml", self_path);

    ret = download_update(temp_buffer, TEMP_PATH1, 0, NULL);

    if(ret < 0)
    {
        sprintf(temp_buffer, "Error 0x%x downloading XML", ret);
        DrawDialogOK(temp_buffer);
        return 0;
    }

    int file_size = 0;
    u8 *mem = (u8 *) LoadFile(TEMP_PATH1, &file_size);

    if(!mem || file_size== 0)
    {
        DrawDialogOK("No updates were found for this game");
        return 0;
    }

    int n = 0;
    int m, l;
    int k = 0;

    while(n < file_size)
    {
        if(mem[n] != '<') {n++; continue;}
        if(!strncmp((char *) &mem[n], "/>", 2) || mem[n] == '>') {n++; continue;}

        if(strncmp((char *) &mem[n], "<package ", 9)) {n++; continue;}

        n+= 9;

        strcpy(version, "00.00");
        strcpy(system,  "00.0000");

        m = locate_xml(mem, n, file_size, "version", &l);
        if(m < 0) goto no_ver; // not found
        if(l > k) k = l;

        m++; l-= m;
        if(l<=0) goto no_ver; // empty

        strncpy(version, (char *) &mem[m], 5);
        version[5] = 0;

no_ver:
        m = locate_xml(mem, n, file_size, "url", &l);
        if(m < 0) continue;
        if(l > k) k = l;

        m++; l -= m;
        if(l <= 0) continue; // empty
        if(l > 1023) l = 1023;

        strncpy(temp_buffer, (char *) &mem[m], l);
        temp_buffer[l] = 0;

        list[max_list][0] = m;
        list[max_list][1] = l;

        m = locate_xml(mem, n, file_size, "ps3_system_ver", &l);
        if(m < 0) goto no_system; // not found
        if(l > k) k = l;

        m++; l-= m;
        if(l <= 0) goto no_system; // empty

        strncpy(system, (char *) &mem[m], 7);
        system[7] = 0;

no_system:

        if(strcmp(version, ver_app)<=0) {n = k; continue;}

        char * o = strrchr(temp_buffer, '/');
        if(o)
        {
            sprintf(MEM_MESSAGE, "Do you want to download this update?\n\n"
                                 "Version: %s for System Ver %s\n\n%s", version, system, o + 1);

            if(DrawDialogYesNo2(MEM_MESSAGE) == YES)
            {
                max_list++; if(max_list >=128) break;
            }
            else break; // to avoid download the next package
        }

        n = k;
    }

    int downloaded = 0;

    if(max_list > 0)
    {
        if(show_http_errors) DrawDialogOKTimer("Downloading the updates...\n\n"
                                               "Wait to finish", 2000.0f);

        //sprintf(TEMP_PATH1, "%s/PKG", self_path);
        sprintf(TEMP_PATH1, updates_path);
        mkdir_secure(TEMP_PATH1);

        for(n = 0; n < max_list; n++)
        {
            struct stat s;
            u64 pkg_size = 0;

            strncpy(temp_buffer, (char *) &mem[list[n][0]], list[n][1]);
            temp_buffer[list[n][1]] = 0;

            char *o = strrchr(temp_buffer, '/');
            if (!o) continue;

            //sprintf(TEMP_PATH1, "%s/PKG%s", self_path, o);
            sprintf(TEMP_PATH1, "%s%s", updates_path, o);
            if(!stat(TEMP_PATH1, &s)) {downloaded++; continue;} // if exist skip

            // get size
            ret = download_update(temp_buffer, TEMP_PATH1, 1, &pkg_size);

            if(ret < 0)
            {
                free(mem);

                sprintf(temp_buffer, "Error 0x%x downloading XML", ret);
                DrawDialogOK(temp_buffer);

                return downloaded;
            }

            {
                u32 blockSize;
                u64 freeSize;
                u64 free_hdd0;
                sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
                free_hdd0 = ( ((u64)blockSize * freeSize));
                if((pkg_size + 0x40000000LL) >= (s64) free_hdd0)
                {
                    free(mem);

                    sprintf(MEM_MESSAGE, "%s\n\n%s\n\n%s%1.2f GB", "Error: There is no free space in HDD0 to copy it", TEMP_PATH1, "You need ",
                                         ((double) (pkg_size + 0x40000000LL - free_hdd0)) / (GIGABYTES));

                    DrawDialogOK(MEM_MESSAGE);

                    return downloaded;
                }
            }

            // download
            ret = download_update(temp_buffer, TEMP_PATH1, 2, &pkg_size);

            if(ret == -0x555)
            {
                free(mem);

                DrawDialogOK(language[GLUTIL_ABORTEDUSER]);
                return downloaded;
            }
            else if(ret < 0)
            {
                free(mem);

                sprintf(MEM_MESSAGE, "Error 0x%x downloading XML", ret);
                DrawDialogOK(MEM_MESSAGE);

                return downloaded;
            }
            else
                downloaded++;
        }
    }
    else
        DrawDialogOK("No updates were found for this game");

    free(mem);

    return downloaded;
}

int copy_async_gbl(char *path1, char *path2, u64 size, char *progress_string1, char *progress_string2)
{
  int ret = 0;

  FILE *fp = NULL;
  FILE *fp2 = NULL;
  float parts = 0;
  float cpart;
  char buffer[16384];

  use_async_fd = 128;
  my_f_async.flags = 0;

  single_bar(progress_string1);

  parts = (size == 0) ? 0.0f : 100.0f / ((double) size / (double) sizeof(buffer));
  cpart = 0;

  fp = fopen(path1, "rb");
  if(!fp) {ret = -1; goto err;}

  fp2 = fopen(path2, "wb");
  if(!fp2) {ret = -2; goto err;}

  int acum = 0;
  while (size != 0ULL)
  {
    int recv = (size > 16384) ? 16384 : size;

    recv = fread(buffer, 1, recv, fp);
    if (recv <= 0) break;
    if (recv > 0)
    {

loop_write:
        if(use_async_fd == 128)
        {
            use_async_fd = 1;
            my_f_async.flags = 0;
            my_f_async.size = recv;
            my_f_async.fp = fp2;
            my_f_async.mem = malloc(recv);
            if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);
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
                    if(fp2) fclose(fp2);
                    fp2 = NULL; ret = -6;
                    goto err;
                }

                my_f_async.flags = 0;
                my_f_async.size = recv;
                my_f_async.fp = fp2;
                my_f_async.mem = malloc(recv);

                if(my_f_async.mem) memcpy(my_f_async.mem, (void *) buffer, recv);

                my_f_async.ret = -1;
                my_f_async.flags = ASYNC_ENABLE;
                event_thread_send(0x555ULL, (u64) my_func_async, (u64) &my_f_async);
            }
            else goto loop_write;
        }
        ///////////////////
        size -= recv;
        acum+= recv;
    }

    if(progress_action == 2) {ret = -0x555; goto err;}

    pad_last_time = 0;

    if(acum >= sizeof(buffer))
    {
        acum-= sizeof(buffer);
        cpart += parts;
        if(cpart >= 1.0f)
        {
            update_bar((u32) cpart);
            cpart-= (float) ((u32) cpart);
        }
    }
  }


  ret = 0;

err:

    if(my_f_async.flags & ASYNC_ENABLE)
    {
        wait_event_thread();
        if(my_f_async.flags  & ASYNC_ERROR)
        {
            if(fp2) fclose(fp2);
            fp2 = NULL; ret = -6;
        }
        my_f_async.flags = 0;
    }

    event_thread_send(0x555ULL, (u64) 0, 0);

    msgDialogAbort();

    if(fp) fclose(fp);

    if(fp2)
    {
        fclose(fp2);
        if(ret < 0) unlink_secure(path2);
    }

    return ret;
}
