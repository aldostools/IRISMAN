#include <sys/socket.h>
#include <netinet/in.h>

#define HTML_RECV_SIZE	2048
#define HTML_RECV_LAST	2045 // HTML_RECV_SIZE-3
#define HTML_RECV_LASTP	2042 // HTML_RECV_LAST-3

static int connect_to_webman(void)
{
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		return -1;
	}

	struct sockaddr_in sin;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0x7F000001;	//127.0.0.1 (localhost)
	sin.sin_port = htons(80);			//http port (80)

	struct timeval tv;
	tv.tv_usec = 0;

	tv.tv_sec = 3;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		return -1;
	}

	tv.tv_sec = 60;
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	return s;
}

static void sclose(int *socket_e)
{
	//if(*socket_e != -1)
	{
		shutdown(*socket_e, SHUT_RDWR);
		//socketclose(*socket_e);
		*socket_e = -1;
	}
}

static void wm_plugin_action(const char * action)
{
	int s = connect_to_webman();
	if(s >= 0)
	{
		char proxy_action[HTML_RECV_SIZE];
		memcpy(proxy_action, "GET ", 4);

		u32 pa = 4;

		if(*action == 'G') action += 4;  // skip GET
		if(*action != '/') action += 16; // using http://127.0.0.1/ or http://localhost/

		if(*action == '/')
		{
			u8 is_path = !strstr(action, ".ps") && !strstr(action, "_ps");

			for(;*action && (pa < HTML_RECV_LAST); action++, pa++)
			{
				if(*action == 0x20)
					proxy_action[pa] = 0x2B;
				else if((*action == 0x2B) && is_path)
				{
					if(pa > HTML_RECV_LASTP) break;
					memcpy(proxy_action + pa, "%2B", 3); pa += 2; //+
				}
				else
					proxy_action[pa] = *action;
			}

			memcpy(proxy_action + pa, "\r\n\0", 3); pa +=2;
			send(s, proxy_action, pa, 0);
		}
		sclose(&s);
	}
}
