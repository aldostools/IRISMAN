CELL_MK_DIR = $(CELL_SDK)/samples/mk
include $(CELL_MK_DIR)/sdk.makedef.mk

BUILD_TYPE     	= release

PRX_DIR		= .
INSTALL			= cp
PEXPORTPICKUP		= ppu-lv2-prx-exportpickup
PRX_LDFLAGS_EXTRA	= -L . -Wl,--strip-unused-data
CIPHER 		= ../sprx_cipher2/sprx_cipher2.exe

CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrti.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='crtbegin.o)
CRT_TAIL                += $(shell ppu-lv2-gcc -print-file-name'='crtend.o)
CRT_HEAD                += $(shell ppu-lv2-gcc -print-file-name'='ecrtn.o)

PPU_SRCS = printf.c libc.c asmcode.s main.c
PPU_PRX_TARGET = sm.prx
PPU_PRX_LDFLAGS += $(PRX_LDFLAGS_EXTRA)
PPU_PRX_STRIP_FLAGS = -s
PPU_PRX_LDLIBS 	+= -lfs_stub -lio_stub -lvshtask_export_stub -lstdc_export_stub
PPU_CFLAGS += -Os -ffunction-sections -fdata-sections -fno-builtin-printf -nodefaultlibs -std=gnu99 -Wno-shadow -Wno-unused-parameter

ifeq ($(BUILD_TYPE), debug)
PPU_CFLAGS += -DDEBUG 
endif

CLEANFILES = $(PRX_DIR)/$(PPU_SPRX_TARGET)

all:
	$(MAKE) $(PPU_OBJS_DEPENDS)
	$(PPU_PRX_STRIP) --strip-debug --strip-section-header $(PPU_PRX_TARGET)
	$(MAKE_FSELF) $(PPU_PRX_TARGET) $(PPU_SPRX_TARGET)
	scetool --sce-type=SELF --compress-data=TRUE --skip-sections=FALSE --key-revision=07 --self-ctrl-flags=4000000000000000000000000000000000000000000000000000000000000002 --self-auth-id=1010000001000003 --self-add-shdrs=TRUE --self-vendor-id=01000002 --self-app-version=0001000000000000 --self-type=APP --self-fw-version=0003005000000000 --encrypt "sm.prx" "sm.sprx"

include $(CELL_MK_DIR)/sdk.target.mk
