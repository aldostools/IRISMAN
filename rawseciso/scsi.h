#ifndef __SCSI_H__
#define __SCSI_H__

#define SCSI_CMD_TEST_UNIT_READY		0x00
#define SCSI_CMD_REQUEST_SENSE			0x03
#define SCSI_CMD_FORMAT_UNIT			0x04
#define SCSI_CMD_FORMAT_INQUIRY			0x12
#define SCSI_CMD_START_STOP_UNIT		0x1B
#define SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL	0x1E
#define SCSI_CMD_READ_FORMAT_CAPACITIES		0x23
#define SCSI_CMD_READ_CAPACITY			0x25
#define SCSI_CMD_READ_10			0x28
#define SCSI_CMD_WRITE_10			0x2A
#define SCSI_CMD_SEEK_10			0x2B
#define SCSI_CMD_WRITE_AND_VERIFY_10		0x2E
#define SCSI_CMD_VERIFY_10			0x2F
#define SCSI_CMD_SYNCHRONIZE_CACHE		0x35
#define SCSI_CMD_WRITE_BUFFER			0x3B
#define SCSI_CMD_READ_BUFFER			0x3C
#define SCSI_CMD_READ_TOC_PMA_ATIP		0x43
#define SCSI_CMD_GET_CONFIGURATION		0x46
#define SCSI_CMD_GET_EVENT_STATUS_NOTIFICATION	0x4A
#define SCSI_CMD_READ_DISC_INFORMATION		0x51
#define SCSI_CMD_READ_TRACK_INFORMATION		0x52
#define SCSI_CMD_RESERVE_TRACK			0x53
#define SCSI_CMD_SEND_OPC_INFORMATION		0x54
#define SCSI_CMD_MODE_SELECT_10			0x55
#define SCSI_CMD_REPAIR_TRACK			0x58
#define SCSI_CMD_MODE_SENSE_10			0x5A
#define SCSI_CMD_CLOSE_TRACK_SESSION		0x5B
#define SCSI_CMD_READ_BUFFER_CAPACITY		0x5C
#define SCSI_CMD_SEND_CUE_SHEET			0x5D
#define SCSI_CMD_REPORT_LUNS			0xA0
#define SCSI_CMD_BLANK				0xA1
#define SCSI_CMD_SECURITY_PROTOCOL_IN		0xA2
#define SCSI_CMD_SEND_KEY			0xA3
#define SCSI_CMD_REPORT_KEY			0xA4
#define SCSI_CMD_LOAD_UNLOAD_MEDIUM		0xA6
#define SCSI_CMD_SET_READ_AHEAD			0xA7
#define SCSI_CMD_READ_12			0xA8
#define SCSI_CMD_WRITE_12			0xAA
#define SCSI_CMD_READ_MEDIA_SERIAL_NUMBER	0xAB
#define SCSI_CMD_GET_PERFORMANCE		0xAC
#define SCSI_CMD_READ_DISC_STRUCTURE		0xAD
#define SCSI_CMD_SECURITY_PROTOCOL_OUT		0xB5
#define SCSI_CMD_SET_STREAMING			0xB6
#define SCSI_CMD_READ_CD_MSF			0xB9
#define SCSI_CMD_SET_CD_SPEED			0xBB
#define SCSI_CMD_MECHANISM_STATUS		0xBD
#define SCSI_CMD_READ_CD 			0xBE
#define SCSI_CMD_SEND_DISC_STRUCTURE		0xBF
#define SCSI_CMD_READ_2064			0xD1 /* Not reall name. Not standard cmd? */

#define itob(i)               ((i)/10*16 + (i)%10)

enum DvdBookType
{
	BOOKTYPE_DVDROM,
	BOOKTYPE_DVDRAM,
	BOOKTYPE_DVDMR,
	BOOKTYPE_DVDMRW,
	BOOKTYPE_HDDVDROM,
	BOOKTYPE_HDDVDRAM,
	BOOKTYPE_HDDVDR,
	BOOKTYPE_DVDPRW = 9,
	BOOKTYPE_DVDPR,
	BOOKTYPE_DVDPRWDL = 13,
	BOOKTYPE_DVDPRDL	
};

typedef struct _ScsiCmdTestUnitReady
{
	uint8_t opcode;
	uint8_t reserved[4];
	uint8_t control;
} __attribute__((packed)) ScsiCmdTestUnitReady;

enum 
{
	FORMAT_TOC,
	FORMAT_SESSION_INFO,
	FORMAT_FULL_TOC,
	FORMAT_PMA,
	FORMAT_ATIP,
	FORMAT_CDTEXT
};

typedef struct _ScsiCmdReadTocPmaAtip
{
	uint8_t opcode;
	uint8_t rv_msf;
	uint8_t rv_format;
	uint8_t reserved[3];
	uint8_t track_session_num;
	uint16_t alloc_length;
	uint8_t control;	
} __attribute__((packed)) ScsiCmdReadTocPmaAtip;

typedef struct _ScsiTocResponse
{
	uint16_t toc_length;
	uint8_t first_track;
	uint8_t last_track;
} __attribute__((packed)) ScsiTocResponse;

typedef struct _ScsiTrackDescriptor
{
	uint8_t reserved;
	uint8_t adr_control;
	uint8_t track_number;
	uint8_t reserved2;
	uint32_t track_start_addr;
} __attribute__((packed)) ScsiTrackDescriptor;

typedef struct _ScsiFullTrackDescriptor
{
	uint8_t session_number;
	uint8_t adr_control;
	uint8_t tno;
	uint8_t point;
	uint8_t min;
	uint8_t sec;
	uint8_t frame;
	uint8_t zero;
	uint8_t pmin;
	uint8_t psec;
	uint8_t pframe;
} __attribute__((packed)) ScsiFullTrackDescriptor;

typedef struct _ScsiCmdGetConfiguration
{
	uint8_t opcode;
	uint8_t rv_rt;
	uint16_t starting_feature_number;
	uint8_t reserved[3];
	uint16_t alloc_length;
	uint8_t control;
} __attribute__((packed)) ScsiCmdGetConfiguration;

typedef struct _ScsiCmdGetEventStatusNotification
{
	uint8_t opcode;
	uint8_t rv_polled;
	uint8_t reserved[2];
	uint8_t notification_class_request;
	uint8_t reserved2[2];
	uint16_t alloc_lenght;
	uint8_t control;
} __attribute__((packed)) ScsiCmdGetEventStatusNotification;

typedef struct _ScsiEventHeader
{
	uint16_t event_length;
	uint8_t nea_rv_nc;
	uint8_t supported_event_class;
} __attribute__((packed)) ScsiEventHeader;

typedef struct _ScsiMediaEventResponse
{
	ScsiEventHeader event_header;
	uint8_t rv_ec;
	uint8_t media_status;
	uint8_t start_slot;
	uint8_t end_slot;
} __attribute__((packed)) ScsiMediaEventResponse;

typedef struct _ScsiCmdReadCd
{
	uint8_t opcode;
	uint8_t rv_est_raddr;
	uint32_t lba;
	uint8_t length[3];
	uint8_t misc;
	uint8_t rv_scsb;
	uint8_t control;
} __attribute__((packed)) ScsiCmdReadCd;

typedef struct _SubChannelQ
{
	uint8_t control_adr;
	uint8_t track_number;
	uint8_t index_number;
	uint8_t min;
	uint8_t sec;
	uint8_t frame;
	uint8_t zero;
	uint8_t amin;
	uint8_t asec;
	uint8_t aframe;
	uint16_t crc;
	uint8_t pad[3];
	uint8_t p;
} __attribute__((packed)) SubChannelQ;

typedef struct _ScsiCmdReadDiscInformation
{
	uint8_t opcode;
	uint8_t reserved[6];
	uint16_t alloc_length;
	uint8_t control;
} __attribute__((packed)) ScsiCmdReadDiscInformation;

typedef struct _ScsiReadDiscInformationResponse
{
	uint16_t length;
	uint8_t  misc;
	uint8_t first_track;
	uint8_t num_sessions_lb;
	uint8_t first_track_lastsession_lb;
	uint8_t last_track_lastsession_lb;
	uint8_t misc2;
	uint8_t disctype;
	uint8_t num_sessions_mb;
	uint8_t first_track_lastsession_mb;
	uint8_t last_track_lastsession_mb;
	uint32_t disc_identification;
	uint32_t last_session_leadin;
	uint32_t last_session_leadout;
	uint8_t barcode[8];
	uint8_t reserved;
	uint8_t num_opc;
} __attribute__((packed)) ScsiReadDiscInformationResponse;

typedef struct _ScsiCmdReadDiscStructure
{
	uint8_t opcode;
	uint8_t rv_mediatype;
	uint32_t address;
	uint8_t layer_num;
	uint8_t format;
	uint16_t alloc_length;
	uint8_t reserved;
	uint8_t control;
} __attribute__((packed)) ScsiCmdReadDiscStructure;

typedef struct _ScsiReadDiscStructureFormat0Response
{
	uint16_t length;
	uint8_t reserved[2];
	uint8_t disccategory_partversion;
	uint8_t discsize_maximumrate;
	uint8_t misc;
	uint8_t density;
	uint8_t zero;
	uint8_t start_sector[3];
	uint8_t zero2;
	uint8_t end_sector[3];
	uint8_t zero3;
	uint8_t end_sector_layer0[3];
	uint8_t reserved2;	
} __attribute__((packed)) ScsiReadDiscStructureFormat0Response;

typedef struct _ScsiRead10
{
	uint8_t opcode;
	uint8_t dpo_fua;
	uint32_t lba;
	uint8_t rv;
	uint16_t length;
	uint8_t control;
} __attribute__((packed)) ScsiRead10;

typedef struct _ScsiRead2064
{
	uint8_t opcode;
	uint8_t unk;
	uint32_t lba;
	uint8_t unk2[2];
	uint16_t length;
	uint8_t control;
} __attribute__((packed)) ScsiRead2064;

#endif /* __SCSI_H__ */


