CLIB_PATH=/home/zerons/workspace/clib
SRCINV_ROOT=/home/zerons/workspace/srcinv

GCC_VER_MAJ:=$(shell expr `gcc -dumpversion | cut -f1 -d.`)
GCC_PLUGIN_INC=/usr/lib/gcc/x86_64-linux-gnu/$(GCC_VER_MAJ)/plugin/include

SELF_CFLAGS=-g -O0 -Wall
#SELF_CFLAGS+=-DUSE_NCURSES
SELF_CFLAGS+=-DCONFIG_DEBUG_MODE=1

SELF_CFLAGS+=-DCONFIG_ANALYSIS_THREAD=0x8
#SELF_CFLAGS+=-DCONFIG_THREAD_STACKSZ=0x10*1024*1024

#SELF_CFLAGS+=-DCONFIG_ID_VALUE_BITS=28
#SELF_CFLAGS+=-DCONFIG_ID_TYPE_BITS=4

#SELF_CFLAGS+=-DCONFIG_SRC_BUF_START=0x100000000
#SELF_CFLAGS+=-DCONFIG_SRC_BUF_BLKSZ=0x10000000
#SELF_CFLAGS+=-DCONFIG_SRC_BUF_END=0x300000000
#SELF_CFLAGS+=-DCONFIG_RESFILE_BUF_START=0x1000000000
#SELF_CFLAGS+=-DCONFIG_RESFILE_BUF_SIZE=0x80000000
#SELF_CFLAGS+=-DCONFIG_SIBUF_LOADED_MAX=0x100000000

#SELF_CFLAGS+=-DCONFIG_SI_PATH_MAX=128
#SELF_CFLAGS+=-DCONFIG_SRC_ID_LEN=4

#SELF_CFLAGS+=-DCONFIG_MAX_OBJS_PER_FILE=0x800000
#SELF_CFLAGS+=-DCONFIG_MAX_SIZE_PER_FILE=0x8000000

#SELF_CFLAGS+=-DCONFIG_SAVED_SRC="src.saved"

#SELF_CFLAGS+=-Wno-packed-not-aligned
SELF_CFLAGS+=-fno-omit-frame-pointer

CFLAGS=-std=gnu11 $(SELF_CFLAGS) $(EXTRA_CFLAGS)
CPPFLAGS=-std=gnu++11 $(SELF_CFLAGS) $(EXTRA_CFLAGS)

ARCH=$(shell getconf LONG_BIT)
CLIB_INC=$(CLIB_PATH)/include
CLIB_LIB=$(CLIB_PATH)/lib
CLIB_SO=clib$(ARCH)
SRCINV_BIN=$(SRCINV_ROOT)/bin
SRCINV_INC=$(SRCINV_ROOT)/include
