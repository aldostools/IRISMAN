#include <sdk_version.h>
#include <cellstatus.h>
#include <cell/cell_fs.h>

#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/memory.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "storage.h"
#include "syscall8.h"
#include "scsi.h"

SYS_MODULE_INFO(RAWSECISO, 0, 1, 0);
SYS_MODULE_START(rawseciso_start);
SYS_MODULE_STOP(rawseciso_stop);

#ifdef DEBUG
#define THREAD_NAME "rawseciso_thread"
#define STOP_THREAD_NAME "rawseciso_stop_thread"
#else
#define THREAD_NAME ""
#define STOP_THREAD_NAME ""
#endif

#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#define ABS(a)		(((a) < 0) ? -(a) : (a))

#define CD_CACHE_SIZE			(64)

enum EMU_TYPE
{
	EMU_OFF = 0,
	EMU_PS3,
	EMU_PS2_DVD,
	EMU_PS2_CD,
	EMU_PSX,
	EMU_BD,
	EMU_DVD,
	EMU_MAX,
};

#define EMU_PSX_MULTI (EMU_PSX + 16)

enum STORAGE_COMMAND
{
	CMD_READ_ISO,
	CMD_READ_DISC,
	CMD_READ_CD_ISO_2352,
	CMD_FAKE_STORAGE_EVENT,
	CMD_GET_PSX_VIDEO_MODE
};

typedef struct
{
	uint64_t device;
	uint32_t emu_mode;
	uint32_t num_sections;
	uint32_t num_tracks;
	// sections after
	// sizes after
	// tracks after
} __attribute__((packed)) rawseciso_args;

/* new for PSX with multiple disc support (derivated from Estwald's PSX payload):

in 0x80000000000000D0, u64 (8 bytes) countaining ISOs sector size (for alls)

tracks index is from lv2peek(0x8000000000000050)

max 8 disc, 4*u32 (16 bytes) containing

0 -> u32 lv2 address for next entry (if 0 indicates no track info: you must generate it!)
1 -> u32 to 0 (reserved)
2 -> u32 lv2 address for track table info
3 -> u32 with size of track table info

table info: (from 0x80000000007E0000)
u8 datas

0, 1 -> countain an u16 with size - 2 of track info block
2 -> to 1
3 -> to 1
// first track
4 -> 0
5 -> mode 0x14 for data, 0x10 for audio
6 -> track number (from 1)
7 -> 0
8, 9, 10, 11 -> countain an u32 with the LBA

....
....

// last track datas (skiped in rawseciso)

0,
0x14,
0xAA,
0,

Ejection / insertion test (to change the disc):

if psxseciso_args->device == 0
test if virtual directory "/psx_cdrom0" is present

virtual disc file names (use this name when psxseciso_args->discs_desc[x][0]==0):

"/psx_d0"
"/psx_d1"
"/psx_d2"
"/psx_d3"
"/psx_d4"
"/psx_d5"
"/psx_d6"
"/psx_d7"

when psxseciso_args->discs_desc[x][0]!=0 it describe a list of sectors/numbers sectors arrays

discs_desc[x][0] = (parts << 16) | offset: parts number of entries, offset: in u32 units from the end of this packet
discs_desc[x][1] = toc_filesize (in LBA!. Used for files/sectors arrays isos to know the toc)

sector array sample:

u32 *datas = &psxseciso_args->discs_desc[8][0]; // datas starts here

u32 *sectors_array1 = datas + offset;
u32 *nsectors_array1 = datas + offset + parts;

offset += parts * 2; // to the next disc sector array datas...

*/

typedef struct
{
	uint64_t device;
	uint32_t emu_mode;		 //   16 bits	16 bits	32 bits
	uint32_t discs_desc[8][2]; //	parts  |   offset | toc_filesize   -> if parts == 0  use files. Offset is in u32 units from the end of this packet. For 8 discs
	// sector array 1
	// numbers sector array 1
	//...
	// sector array 8
	// numbers sector array 8
} __attribute__((packed)) psxseciso_args;

volatile int eject_running = 1;

uint32_t real_disctype;
ScsiTrackDescriptor tracks[64];
int emu_mode, num_tracks;
sys_event_port_t result_port;


static int mode_file = 0, cd_sector_size_param = 0;

#define CD_SECTOR_SIZE_2048			2048

static uint64_t size_sector = 512;
static uint32_t CD_SECTOR_SIZE_2352 = 2352;

static sys_device_info_t disc_info;

uint64_t usb_device = 0ULL;

static sys_ppu_thread_t thread_id = -1, thread_id2 = -1;
static int ntfs_running = 1;

static volatile int do_run = 0;

sys_device_handle_t handle = -1;
sys_event_queue_t command_queue = -1;

static uint32_t *sections, *sections_size;
static uint32_t num_sections;

static uint64_t discsize=0;
static int is_cd2352 = 0;
static uint8_t *cd_cache=0;


int rawseciso_start(uint64_t arg);
int rawseciso_stop(void);

static inline uint64_t lv2peek(uint64_t lv2addr)
{
	system_call_1(6, lv2addr);
	return (uint64_t)p1;
}

static inline uint64_t sys8_memcpy(uint64_t dst, uint64_t src, uint64_t size)
{
	system_call_4(8, 2ULL, dst, src, size);
	return (uint64_t)p1;
}


static inline int sysUsleep(uint32_t useconds)
{
	system_call_1(141, useconds);
	return (int)p1;
}

static inline int sys_shutdown(void)
{
	system_call_4(379,0x1100,0,0,0);
	return (int)p1;
}

static inline void _sys_ppu_thread_exit(uint64_t val)
{
	system_call_1(41, val);
}

static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
	system_call_1(461, (uint64_t)(uint32_t)addr);
	return (int)p1;
}

static int fake_eject_event(void)
{
	sys_storage_ext_fake_storage_event(4, 0, BDVD_DRIVE);
	return sys_storage_ext_fake_storage_event(8, 0, BDVD_DRIVE);
}

static int fake_insert_event(uint64_t disctype)
{
	uint64_t param = (uint64_t)(disctype) << 32ULL;
	sys_storage_ext_fake_storage_event(7, 0, BDVD_DRIVE);
	return sys_storage_ext_fake_storage_event(3, param, BDVD_DRIVE);
}

static int sys_set_leds(uint64_t color, uint64_t state)
{
	system_call_2(386,  (uint64_t) color, (uint64_t) state);
	return (int)p1;
}

static inline void get_next_read(uint64_t discoffset, uint64_t bufsize, uint64_t *offset, uint64_t *readsize, int *idx, uint64_t size_sector)
{
	uint64_t base = 0;
	*idx = -1;
	*readsize = bufsize;
	*offset = 0;

	for(uint32_t i = 0; i < num_sections; i++)
	{
		uint64_t last;
		uint64_t sz;

		if(i == 0 && mode_file > 1)
			sz = (((uint64_t)sections_size[i]) * CD_SECTOR_SIZE_2048);
		else
			sz = (((uint64_t)sections_size[i]) * size_sector);

		last = base + sz;

		if(discoffset >= base && discoffset < last)
		{
			uint64_t maxfileread = last-discoffset;

			if(bufsize > maxfileread)
				*readsize = maxfileread;
			else
				*readsize = bufsize;

			*idx = i;
			*offset = discoffset-base;
			return;
		}

		base += sz;
	}

	// We can be here on video blu-ray
	DPRINTF("Offset or size out of range  %lx%08lx   %lx!!!!!!!!\n", discoffset>>32, discoffset, bufsize);
}

uint8_t last_sect_buf[4096] __attribute__((aligned(16)));
uint32_t last_sect = 0xffffffff;

static int process_read_iso_cmd(uint8_t *buf, uint64_t offset, uint64_t size)
{
	uint64_t remaining;
	int x;

	//DPRINTF("read iso: %p %lx %lx\n", buf, offset, size);
	remaining = size;

	while(remaining > 0)
	{
		uint64_t pos, readsize;
		int idx;
		int ret;
		//uint8_t tmp[4096];
		uint32_t sector;
		uint32_t r;

		if(!ntfs_running) return -1;

		get_next_read(offset, remaining, &pos, &readsize, &idx, size_sector);

		if(idx == -1 || sections[idx] == 0xFFFFFFFF)
		{
			memset(buf, 0, readsize);
			buf += readsize;
			offset += readsize;
			remaining -= readsize;
			continue;
		}

		if(pos % size_sector)
		{
			uint64_t csize;

			sector = sections[idx] + (pos / size_sector);
			for(x = 0; x < 16; x++)
			{
				r = 0;
				if(last_sect == sector)
				{
					ret = 0; r = 1;
				}
				else
					ret = sys_storage_read(handle, 0, sector, 1, last_sect_buf, &r, 0);

				if(ret == 0 && r == 1)
					last_sect = sector;
				else
					last_sect = 0xffffffff;

				if(ret != 0 || r != 1)
				{
					if(emu_mode == EMU_PSX_MULTI) return (int) 0x8001000A; // EBUSY

					if(ret == (int) 0x80010002 || ret == (int) 0x8001002D)
					{
						if(handle != (sys_device_handle_t) -1) sys_storage_close(handle); handle = -1;

						while(ntfs_running)
						{
							if(sys_storage_get_device_info(usb_device, &disc_info) == 0)
							{
								ret = sys_storage_open(usb_device, 0, &handle, 0);
								if(ret == 0) break;

								handle = -1; sysUsleep(500000);
							}
							else sysUsleep(7000000);
						}
						x= -1; continue;
					}


					if(x == 15 || !ntfs_running)
					{
						DPRINTF("sys_storage_read failed: %x 1 -> %x\n", sector, ret);
						return -1;
					}
					else sysUsleep(100000);
				}
				else break;
			}

			csize = size_sector - (pos % size_sector);

			if(csize > readsize) csize = readsize;

			memcpy(buf, last_sect_buf+(pos % size_sector), csize);
			buf += csize;
			offset += csize;
			pos += csize;
			remaining -= csize;
			readsize -= csize;
		}

		if(readsize > 0)
		{
			uint32_t n = readsize / size_sector;

			if(n > 0)
			{
				uint64_t s;

				sector = sections[idx] + (pos / size_sector);
				for(x = 0; x < 16; x++)
				{
					r = 0;
					ret = sys_storage_read(handle, 0, sector, n, buf, &r, 0);

					if(ret == 0 && r == n)
						last_sect = sector + n - 1;
					else
						last_sect = 0xffffffff;

					if(ret != 0 || r != n)
					{
						if(emu_mode == EMU_PSX_MULTI) return (int) 0x8001000A; // EBUSY

						if(ret == (int) 0x80010002 || ret == (int) 0x8001002D)
						{
							if(handle != (sys_device_handle_t) -1) sys_storage_close(handle); handle = -1;

							while(ntfs_running)
							{
								if(sys_storage_get_device_info(usb_device, &disc_info) == 0)
								{
									ret = sys_storage_open(usb_device, 0, &handle, 0);
									if(ret == 0) break;

									handle = -1; sysUsleep(500000);
								}
								else sysUsleep(7000000);
							}
							x= -1; continue;
						}

						if(x == 15 || !ntfs_running)
						{
							DPRINTF("sys_storage_read failed: %x %x -> %x\n", sector, n, ret);
							return -1;
						}
						else sysUsleep(100000);
					}
					else break;
				}

				s = n * size_sector;
				buf += s;
				offset += s;
				pos += s;
				remaining -= s;
				readsize -= s;
			}

			if(readsize > 0)
			{
				sector = sections[idx] + pos / size_sector;
				for(x = 0; x < 16; x++)
				{
					r = 0;
					if(last_sect == sector)
					{
						ret = 0; r = 1;
					}
					else
						ret = sys_storage_read(handle, 0, sector, 1, last_sect_buf, &r, 0);

					if(ret == 0 && r == 1)
						last_sect = sector;
					else
						last_sect = 0xffffffff;

					if(ret != 0 || r != 1)
					{
						if(emu_mode == EMU_PSX_MULTI) return (int) 0x8001000A; // EBUSY

						if(ret == (int) 0x80010002 || ret == (int) 0x8001002D)
						{
							if(handle != (sys_device_handle_t) -1) sys_storage_close(handle); handle = -1;

							while(ntfs_running)
							{
								if(sys_storage_get_device_info(usb_device, &disc_info) == 0)
								{
									ret = sys_storage_open(usb_device, 0, &handle, 0);
									if(ret == 0) break;

									handle = -1; sysUsleep(500000);
								}
								else sysUsleep(7000000);
							}
							x= -1; continue;
						}

						if(x == 15 || !ntfs_running)
						{
							DPRINTF("sys_storage_read failed: %x 1 -> %x\n", sector, ret);
							return -1;
						}
						else sysUsleep(100000);
					}
					else break;
				}

				memcpy(buf, last_sect_buf, readsize);
				buf += readsize;
				offset += readsize;
				remaining -= readsize;
			}
		}
	}

	return 0;
}

int last_index = -1;
static int discfd = -1;

static int process_read_file_cmd(uint8_t *buf, uint64_t offset, uint64_t size)
{
	uint64_t remaining;

	char *path_name = (char *) sections;

	if(mode_file > 1)
	{
		last_index = -1;
		path_name-= 0x200;
	}

	remaining = size;

	while(remaining > 0)
	{
		uint64_t pos, p, readsize;
		int idx;
		int ret;

		get_next_read(offset, remaining, &pos, &readsize, &idx, CD_SECTOR_SIZE_2048);

		if(idx == -1 || path_name[0x200 * idx] == 0)
		{
			memset(buf, 0, readsize);
			buf += readsize;
			offset += readsize;
			remaining -= readsize;
			continue;
		}
		else
		{
			if(idx != last_index)
			{
				if(discfd != -1) cellFsClose(discfd);

				while(ntfs_running)
				{
					ret = cellFsOpen(&path_name[0x200 * idx], CELL_FS_O_RDONLY, &discfd, NULL, 0);
					if(ret == 0) break;

					discfd = -1;
					sysUsleep(5000000);
				}

				last_index = idx;
			}

			p = 0;
			ret = cellFsLseek(discfd, pos, SEEK_SET, &p);
			if(!ret) ret = cellFsRead(discfd, buf, readsize, &p);
			if(ret != 0)
			{
				last_index = -1;

				continue;
			}
			else if(readsize != p)
			{
				if((offset + readsize) < discsize) return -1;
			}

			buf += readsize;
			offset += readsize;
			remaining -= readsize;
		}
	}

	return 0;
}

int psx_indx = 0;

uint32_t *psx_isos_desc;

char psx_filename[8][8]= {
	"/psx_d0",
	"/psx_d1",
	"/psx_d2",
	"/psx_d3",
	"/psx_d4",
	"/psx_d5",
	"/psx_d6",
	"/psx_d7"
};

static int process_read_psx_cmd(uint8_t *buf, uint64_t offset, uint64_t size, uint32_t ssector)
{
	uint64_t remaining, sect_size;
	int ret, rel;

	uint32_t lba = offset + 150, lba2;

	sect_size = lv2peek(0x80000000000000D0ULL);
	offset *= sect_size;
	//size   *= sector;

	remaining = size;

	if(!psx_isos_desc[psx_indx * 2 + 0])
	{
		if(discfd == -1)
		{
			while(ntfs_running)
			{
				ret = cellFsOpen(&psx_filename[psx_indx][0], CELL_FS_O_RDONLY, &discfd, NULL, 0);
				if(ret == 0) break;

				discfd = -1;
				sysUsleep(5000000);
			}
		}
	}

	while(remaining > 0)
	{
		uint64_t pos, p, readsize = (uint64_t)CD_SECTOR_SIZE_2352;


			p = 0;
			rel = 0;

			pos = offset;
			if(ssector == CD_SECTOR_SIZE_2048) { if(sect_size >= CD_SECTOR_SIZE_2352) pos += 24ULL; readsize = CD_SECTOR_SIZE_2048;}
			else
			{
				if(sect_size == CD_SECTOR_SIZE_2048)
				{
					rel = 24; readsize = CD_SECTOR_SIZE_2048;
					memset(&buf[0], 0x0, 24);
					memset(&buf[1], 0xff, 10);
					buf[0x12] = buf[0x16]= 8;
					buf[0xf] = 2;

					buf[0xe] = lba % 75;

					lba2 = lba / 75;
					buf[0xd] = lba2 % 60;
					lba2 /= 60;
					buf[0xc] = lba2;
				}
			}

			if(!psx_isos_desc[psx_indx * 2 + 0])
			{
				ret = cellFsLseek(discfd, pos, SEEK_SET, &p);
				if(!ret) ret = cellFsRead(discfd, buf + rel, readsize, &p);
				if(ret != 0)
				{
					if(ret == (int) 0x8001002B) return (int) 0x8001000A; // EBUSY

					return 0;
				}
				else if(readsize != p)
				{
					if((offset + readsize) < discsize) return 0;
				}
			}
			else
			{
				ret = process_read_iso_cmd(buf + rel, pos, readsize);
				if(ret) return ret;
			}


			buf += ssector;
			offset += sect_size;
			remaining --;
			lba++;
	}

	return 0;
}

static inline void my_memcpy(uint8_t *dst, uint8_t *src, int size)
{
	for(int i = 0; i < size; i++) dst[i] = src[i];
}


static uint32_t cached_cd_sector=0x80000000;

static int process_read_cd_2048_cmd(uint8_t *buf, uint32_t start_sector, uint32_t sector_count)
{
	uint32_t capacity = sector_count * CD_SECTOR_SIZE_2048;
	uint32_t fit = capacity/CD_SECTOR_SIZE_2352;
	uint32_t rem = (sector_count-fit);
	uint32_t i;
	uint8_t *in = buf;
	uint8_t *out = buf;

	if(fit > 0)
	{
		process_read_iso_cmd(buf, start_sector*CD_SECTOR_SIZE_2352, fit*CD_SECTOR_SIZE_2352);

		for(i = 0; i < fit; i++)
		{
			my_memcpy(out, in + 24, CD_SECTOR_SIZE_2048);
			in += CD_SECTOR_SIZE_2352;
			out += CD_SECTOR_SIZE_2048;
			start_sector++;
		}
	}

	for(i = 0; i < rem; i++)
	{
		process_read_iso_cmd(out, (start_sector*CD_SECTOR_SIZE_2352) + 24, CD_SECTOR_SIZE_2048);
		out += CD_SECTOR_SIZE_2048;
		start_sector++;
	}

	return 0;
}

static int process_read_cd_2352_cmd(uint8_t *buf, uint32_t sector, uint32_t remaining)
{
	int cache = 0;

	if(remaining <= CD_CACHE_SIZE)
	{
		int dif = (int)cached_cd_sector-sector;

		if(ABS(dif) < CD_CACHE_SIZE)
		{
			uint8_t *copy_ptr = NULL;
			uint32_t copy_offset = 0;
			uint32_t copy_size = 0;

			if(dif > 0)
			{
				if(dif < (int)remaining)
				{
					copy_ptr = cd_cache;
					copy_offset = dif;
					copy_size = remaining-dif;
				}
			}
			else
			{

				copy_ptr = cd_cache+((-dif)*CD_SECTOR_SIZE_2352);
				copy_size = MIN((int)remaining, CD_CACHE_SIZE+dif);
			}

			if(copy_ptr)
			{
				memcpy(buf+(copy_offset*CD_SECTOR_SIZE_2352), copy_ptr, copy_size*CD_SECTOR_SIZE_2352);

				if(remaining == copy_size)
				{
					return 0;
				}

				remaining -= copy_size;

				if(dif <= 0)
				{
					uint32_t newsector = cached_cd_sector + CD_CACHE_SIZE;
					buf += ((newsector-sector)*CD_SECTOR_SIZE_2352);
					sector = newsector;
				}
			}
		}

		cache = 1;
	}

	if(!cache)
	{
		return process_read_iso_cmd(buf, sector*CD_SECTOR_SIZE_2352, remaining*CD_SECTOR_SIZE_2352);
	}

	if(!cd_cache)
	{
		sys_addr_t addr;

		int ret = sys_memory_allocate(192*1024, SYS_MEMORY_PAGE_SIZE_64K, &addr);
		if(ret != 0)
		{
			DPRINTF("sys_memory_allocate failed: %x\n", ret);
			return ret;
		}

		cd_cache = (uint8_t *)addr;
	}

	if(process_read_iso_cmd(cd_cache, sector*CD_SECTOR_SIZE_2352, CD_CACHE_SIZE*CD_SECTOR_SIZE_2352) != 0)
		return -1;

	memcpy(buf, cd_cache, remaining*CD_SECTOR_SIZE_2352);
	cached_cd_sector = sector;
	return 0;
}

static void get_psx_track_datas(void)
{
	uint32_t track_datas[4];
	uint8_t buff[CD_SECTOR_SIZE_2048];
	uint64_t lv2_addr = 0x8000000000000050ULL;
	lv2_addr+= 16ULL * psx_indx;

	sys8_memcpy((uint64_t)(uint32_t) track_datas, lv2_addr, 16ULL);

	int k = 4;
	num_tracks = 0;

	if(!track_datas[0])
	{
		tracks[num_tracks].adr_control = 0x14;
		tracks[num_tracks].track_number = 1;
		tracks[num_tracks].track_start_addr = 0;
		num_tracks++;

	}
	else
	{
		lv2_addr = 0x8000000000000000ULL + (uint64_t) track_datas[2];
		sys8_memcpy((uint64_t)(uint32_t) buff, lv2_addr, track_datas[3]);

		while(k < (int) track_datas[3])
		{
			tracks[num_tracks].adr_control = (buff[k + 1] != 0x14) ? 0x10 : 0x14;
			tracks[num_tracks].track_number = num_tracks+1;
			tracks[num_tracks].track_start_addr = ((uint32_t) buff[k + 4] << 24) | ((uint32_t) buff[k + 5] << 16) |
												  ((uint32_t) buff[k + 6] << 8)  | ((uint32_t) buff[k + 7]);
			num_tracks++;
			k+= 8;
		}

		if(num_tracks > 1) num_tracks--;
	}

	discsize = ((uint64_t) psx_isos_desc[psx_indx * 2 + 1]) * (uint64_t)CD_SECTOR_SIZE_2352;

	num_sections = (psx_isos_desc[psx_indx * 2 + 0] >> 16) & 0xffff;
	sections = &psx_isos_desc[16] + (psx_isos_desc[psx_indx * 2 + 0] & 0xffff);
	sections_size = sections + num_sections;
}

static int ejected_disc(void)
{
	static int ejected = 0;
	static int counter = 0;
	int fd = -1;

	if(usb_device != 0ULL)
	{
		int r = sys_storage_get_device_info(usb_device, &disc_info);
		if(r == 0)
		{
			counter++;

			if(ejected && counter > 100)
			{
				if(handle != (sys_device_handle_t) -1) sys_storage_close(handle); handle = -1;

				if(sys_storage_open(usb_device, 0, &handle, 0) == 0)
				{
					sys_set_leds(1 , 1);
					ejected = 0;
					counter = 0;
					return 1;

				}
				else
				{
					handle = -1;
					return -1;
				}
			}
			else
				return 2;
		}
		else if(r == (int) 0x80010002)
		{
			sys_set_leds(1 , 2);

			if(!ejected)
			{
				counter = 0;

				ejected = 1;
				return 0;
			}
			else
				return -1;
		}
		else
		{
			ejected = 1;
			return -128;
		}
	}


	if(cellFsOpendir("/psx_cdrom0", &fd)==0)
	{
		cellFsClosedir(fd);
		if(ejected)
		{
			sys_set_leds(1 , 1);
			ejected = 0;
			return 1;
		}
		else
			return 2;

	}
	else
	{
		sys_set_leds(1 , 2);

		if(!ejected)
		{
			ejected = 1;
			return 0;
		}
		else
			return -1;
	}

	return -2;
}

static void eject_thread(uint64_t arg)
{
	while(eject_running)
	{
		int ret;

		if(emu_mode == EMU_PSX_MULTI)
		{
			ret = ejected_disc();

			if(ret == 0)
			{
				sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);
				fake_eject_event();
				sys_storage_ext_umount_discfile();

				if(real_disctype != 0)
				{
					fake_insert_event(real_disctype);
				}

				if(discfd != -1) cellFsClose(discfd); discfd = -1;
				if(handle != (sys_device_handle_t) -1) sys_storage_close(handle); handle = -1;

				if(command_queue != (sys_event_queue_t)-1)
				{
					eject_running = 2;
					sys_event_queue_t command_queue2 = command_queue;
					command_queue = (sys_event_queue_t) -1;

					if(sys_event_queue_destroy(command_queue2, SYS_EVENT_QUEUE_DESTROY_FORCE) != 0)
					{
						DPRINTF("Failed in destroying command_queue\n");
					}
				}



				while(do_run!=0) sysUsleep(100000);

				psx_indx = (psx_indx + 1) & 7;

			}
			else if(ret == 1)
			{
				get_psx_track_datas();

				sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);

				if(real_disctype != 0)
				{
					fake_eject_event();
				}

				sys_event_queue_attribute_t queue_attr;
				sys_event_queue_attribute_initialize(queue_attr);
				ret = sys_event_queue_create(&command_queue, &queue_attr, 0, 1);
				if(ret == 0) {eject_running = 1; ret = sys_storage_ext_mount_discfile_proxy(result_port, command_queue, emu_mode & 0xF, discsize, 256*1024, (num_tracks | cd_sector_size_param), tracks);}

				if(ret != 0) psx_indx = (psx_indx - 1) & 7;
				else {fake_insert_event(real_disctype);}
			}
		}


		sysUsleep(70000);
	}

	DPRINTF("Exiting eject thread!\n");
	sys_ppu_thread_exit(0);
}

static void rawseciso_thread(uint64_t arg)
{
	rawseciso_args *args;
	psxseciso_args *psx_args;

	sys_event_queue_attribute_t queue_attr;

	int ret; cd_sector_size_param = 0;

	args = (rawseciso_args *)(uint32_t)arg;
	psx_args = (psxseciso_args *)(uint32_t)arg;

	DPRINTF("Hello VSH\n");

	num_sections = 0;
	CD_SECTOR_SIZE_2352 = 2352;

	emu_mode = args->emu_mode & 1023;

	if(emu_mode != EMU_PSX_MULTI)
	{
		num_sections = args->num_sections;
		sections = (uint32_t *)(args+1);

		mode_file = (args->emu_mode & 3072) >> 10;

		if(mode_file == 0)
			sections_size = sections + num_sections;
		else if(mode_file == 1)
			sections_size = sections + num_sections * 0x200/4;
		else
		{
			sections += 0x200/4;
			sections_size = sections + num_sections;
		}

		discsize = 0;

		for(uint32_t i = 1 * (mode_file > 1); i < num_sections; i++)
		{
			discsize += sections_size[i];
		}
	}
	else
		mode_file = 0;

	if(mode_file != 1)
	{
		size_sector = 512ULL;
		if(args->device != 0)
		{
			for(int i = 0; i < 16; i++)
			{
				if(sys_storage_get_device_info(args->device, &disc_info) == 0)
				{
					size_sector  = (uint32_t) disc_info.sector_size;
					break;
				}

				sysUsleep(500000);
			}
		}

	}
	else
		size_sector = CD_SECTOR_SIZE_2048;

	if(emu_mode == EMU_PSX_MULTI)
	{
		psx_isos_desc = &psx_args->discs_desc[0][0];

		is_cd2352 = 2;
	}
	else if(emu_mode == EMU_PSX)
	{   // old psx support
		num_tracks = args->num_tracks;

		if(num_tracks > 0xFF)
		{
			CD_SECTOR_SIZE_2352 = (num_tracks & 0xFFF00)>>4;
			if(CD_SECTOR_SIZE_2352 > 2448) CD_SECTOR_SIZE_2352 = (num_tracks & 0xFFF00)>>8;
		}

		if(CD_SECTOR_SIZE_2352 != 2352 && CD_SECTOR_SIZE_2352 != 2048 && CD_SECTOR_SIZE_2352 != 2336 && CD_SECTOR_SIZE_2352 != 2448 && CD_SECTOR_SIZE_2352 != 2328 && CD_SECTOR_SIZE_2352 != 2340 && CD_SECTOR_SIZE_2352 != 2368) CD_SECTOR_SIZE_2352 = 2352;

		if(CD_SECTOR_SIZE_2352 & 0xf) cd_sector_size_param = CD_SECTOR_SIZE_2352<<8;
		if(CD_SECTOR_SIZE_2352 != 2352) cd_sector_size_param = CD_SECTOR_SIZE_2352<<4;

		num_tracks &= 0xff;

		if(num_tracks)
			memcpy((void *) tracks, (void *) ((ScsiTrackDescriptor *)(sections_size + num_sections)), num_tracks * sizeof(ScsiTrackDescriptor));
		else
		{
			tracks[num_tracks].adr_control = 0x14;
			tracks[num_tracks].track_number = 1;
			tracks[num_tracks].track_start_addr = 0;
			num_tracks++;
		}

		is_cd2352 = 1;

		discsize = discsize * size_sector;

		if(discsize % CD_SECTOR_SIZE_2352)
		{
			discsize = discsize - (discsize % CD_SECTOR_SIZE_2352);
		}

	}
	else
	{
		num_tracks = 0;
		discsize = discsize * size_sector + ((mode_file > 1) ? (uint64_t) (CD_SECTOR_SIZE_2048 * sections[0] ) : 0ULL) ;
		is_cd2352 = 0;
	}


	DPRINTF("discsize = %lx%08lx\n", discsize>>32, discsize);

	if(mode_file != 1)
	{
		usb_device = args->device;

		if(usb_device != 0)
		{
			ret = sys_storage_open(usb_device, 0, &handle, 0);
			if(ret != 0)
			{
				DPRINTF("sys_storage_open failed: %x\n", ret);
				sys_memory_free((sys_addr_t)args);
				sys_ppu_thread_exit(ret);
			}
		}
	}

	ret = sys_event_port_create(&result_port, 1, SYS_EVENT_PORT_NO_NAME);
	if(ret != 0)
	{
		DPRINTF("sys_event_port_create failed: %x\n", ret);
		if(handle != (sys_device_handle_t) -1) sys_storage_close(handle);
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(ret);
	}

	sys_event_queue_attribute_initialize(queue_attr);
	ret = sys_event_queue_create(&command_queue, &queue_attr, 0, 1);
	if(ret != 0)
	{
		DPRINTF("sys_event_queue_create failed: %x\n", ret);
		sys_event_port_destroy(result_port);
		if(handle != (sys_device_handle_t) -1) sys_storage_close(handle);
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(ret);
	}

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);

	if(real_disctype != 0)
	{
		fake_eject_event();
	}

	if(emu_mode == EMU_PSX_MULTI)
	{
		get_psx_track_datas();

		sys_ppu_thread_create(&thread_id2, eject_thread, 0, 3000, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME"_eject");
	}

	ret = sys_storage_ext_mount_discfile_proxy(result_port, command_queue, emu_mode & 0xF, discsize, 256*1024, (num_tracks | cd_sector_size_param), tracks);
	DPRINTF("mount = %x\n", ret);

	fake_insert_event(real_disctype);

	if(ret != 0)
	{
		sys_event_port_disconnect(result_port);
		// Queue destroyed in stop thread sys_event_queue_destroy(command_queue);
		sys_event_port_destroy(result_port);
		if(handle != (sys_device_handle_t) -1) sys_storage_close(handle);
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(0);
	}

	while(ntfs_running)
	{
		sys_event_t event;

		do_run = 0;

		if(command_queue == (sys_event_queue_t)-1)
		{
			if(!ntfs_running) break;

			sysUsleep(100000);

			continue;
		}

		ret = sys_event_queue_receive(command_queue, &event, 0);
		if(ret != 0)
		{
			if((command_queue == (sys_event_queue_t)-1 || eject_running == 2) && ret !=0)
			{
				if(!ntfs_running) break;

				sysUsleep(100000);

				continue;
			}
			if(ret != (int) 0x80010013) sys_shutdown();
			//DPRINTF("sys_event_queue_receive failed: %x\n", ret);
			break;
		}

		if(!ntfs_running) break;

		do_run = 1;

		void *buf = (void *)(uint32_t)(event.data3>>32ULL);
		uint64_t offset = event.data2;
		uint32_t size = event.data3&0xFFFFFFFF;

		switch (event.data1)
		{
			case CMD_READ_ISO:
			{
				if(is_cd2352)
				{
					if(is_cd2352 == 1) ret = process_read_cd_2048_cmd(buf, offset / CD_SECTOR_SIZE_2048, size / CD_SECTOR_SIZE_2048);
					else ret = process_read_psx_cmd(buf, offset / CD_SECTOR_SIZE_2048, size / CD_SECTOR_SIZE_2048, CD_SECTOR_SIZE_2048);
				}
				else
				{
					if(mode_file == 0)
						ret = process_read_iso_cmd(buf, offset, size);
					else if(mode_file == 1)
						ret = process_read_file_cmd(buf, offset, size);
					else
					{
						if(offset < (((uint64_t)sections_size[0]) * CD_SECTOR_SIZE_2048))
						{
							uint32_t rd = sections_size[0] * CD_SECTOR_SIZE_2048;

							if(rd > size) rd = size;
							ret = process_read_file_cmd(buf, offset, rd);
							if(ret == 0)
							{
								size-= rd;
								offset+= rd;
								buf = ((char *) buf) + rd;

								if(size != 0)
									ret = process_read_iso_cmd(buf, offset, size);
							}
						}
						else
							ret = process_read_iso_cmd(buf, offset, size);

						if(discfd != -1) cellFsClose(discfd); discfd = -1;
					}
				}
			}
			break;

			case CMD_READ_CD_ISO_2352:
			{

				if(is_cd2352 == 1) ret = process_read_cd_2352_cmd(buf, offset/CD_SECTOR_SIZE_2352, size/CD_SECTOR_SIZE_2352);
				else ret = process_read_psx_cmd(buf, offset/CD_SECTOR_SIZE_2352, size/CD_SECTOR_SIZE_2352, CD_SECTOR_SIZE_2352);

			}
			break;
		}

		while(ntfs_running)
		{
			ret = sys_event_port_send(result_port, ret, 0, 0);
			if(ret == 0) break;

			if(ret == (int) 0x8001000A)
			{	   // EBUSY
					sysUsleep(100000);
					continue;
			}

			DPRINTF("sys_event_port_send failed: %x\n", ret);
			break;
		}

		if(ret != 0) break;
	}

	do_run = 0;

	eject_running = 0;

	sys_memory_free((sys_addr_t)args);

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);
	fake_eject_event();
	sys_storage_ext_umount_discfile();

	if(real_disctype != 0)
	{
		fake_insert_event(real_disctype);
	}

	if(cd_cache)
	{
		sys_memory_free((sys_addr_t)cd_cache);
	}

	if(handle != (sys_device_handle_t) -1) sys_storage_close(handle);

	if(discfd != -1) cellFsClose(discfd);

	sys_event_port_disconnect(result_port);
	if(sys_event_port_destroy(result_port) != 0)
	{
		DPRINTF("Error destroyng result_port\n");
	}

	// queue destroyed in stop thread

	DPRINTF("Exiting main thread!\n");
	sys_ppu_thread_exit(0);
}

int rawseciso_start(uint64_t arg)
{
	if(arg != 0)
	{
		void *argp = (void *)(uint32_t)arg;
		sys_addr_t addr;

		if(sys_memory_allocate(64*1024, SYS_MEMORY_PAGE_SIZE_64K, &addr) == 0)
		{
			memcpy((void *)addr, argp, 64*1024);
			sys_ppu_thread_create(&thread_id, rawseciso_thread, (uint64_t)addr, -0x1d8, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME);

		}
	}
	// Exit thread using directly the syscall and not the user mode library or we will crash
	_sys_ppu_thread_exit(0);
	return SYS_PRX_RESIDENT;
}

static void rawseciso_stop_thread(uint64_t arg)
{
	uint64_t exit_code;

	DPRINTF("rawseciso_stop_thread\n");

	ntfs_running = do_run = 0;

	if(command_queue != (sys_event_queue_t)-1)
	{
		if(sys_event_queue_destroy(command_queue, SYS_EVENT_QUEUE_DESTROY_FORCE) != 0)
		{
			DPRINTF("Failed in destroying command_queue\n");
		}
	}

	if(thread_id != (sys_ppu_thread_t)-1)
	{
		sys_ppu_thread_join(thread_id, &exit_code);
	}

	eject_running = 0;

	if(thread_id2 != (sys_ppu_thread_t)-1)
	{
		sys_ppu_thread_join(thread_id2, &exit_code);
	}

	DPRINTF("Exiting stop thread!\n");
	sys_ppu_thread_exit(0);
}

static void finalize_module(void)
{
	uint64_t meminfo[5];

	sys_prx_id_t prx = prx_get_module_id_by_address(finalize_module);

	meminfo[0] = 0x28;
	meminfo[1] = 2;
	meminfo[3] = 0;

	system_call_3(482, prx, 0, (uint64_t)(uint32_t)meminfo);
}

int rawseciso_stop(void)
{
	sys_ppu_thread_t t;
	uint64_t exit_code;

	sys_ppu_thread_create(&t, rawseciso_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	sys_ppu_thread_join(t, &exit_code);

	finalize_module();
	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}
