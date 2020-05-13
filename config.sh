#!/bin/bash

# This is a simple configure script.
# Later, may change to use config tools.

DEF_GCC_PLUGIN_BASE=/usr/lib/gcc/x86_64-linux-gnu

usage()
{
	echo "Usage: $0 (CLIB_PATH) (SRCINV_ROOT) [GCC_PLUGIN_BASE]"
	echo "    GCC_PLUGIN_BASE: Default: $DEF_GCC_PLUGIN_BASE"
}

gen_file()
{
	TFILE=$4

	CONTENT=$'CLIB_PATH='$1$'\n'
	CONTENT+=$'SRCINV_ROOT='$2$'\n'
	CONTENT+=$'\n'

	CONTENT+=$'GCC_VER_MAJ:=$(shell expr `gcc -dumpversion | cut -f1 -d.`)\n'
	CONTENT+=$'GCC_PLUGIN_BASE='$3$'\n'
	CONTENT+=$'GCC_PLUGIN_INC=$(GCC_PLUGIN_BASE)/$(GCC_VER_MAJ)/plugin/include\n'
	CONTENT+=$'\n'

	CONTENT+=$'SELF_CFLAGS=-g -Wall -O3\n'
	CONTENT+=$'SELF_CFLAGS+=-DCONFIG_ANALYSIS_THREAD=0x10\n'
	CONTENT+=$'SELF_CFLAGS+=-DCONFIG_DEBUG_MODE=1\n'
	CONTENT+=$'SELF_CFLAGS+=-DHAVE_CLIB_DBG_FUNC\n'
	CONTENT+=$'#SELF_CFLAGS+=-DUSE_NCURSES\n'
	CONTENT+=$'\n'

	CONTENT+=$'#SELF_CFLAGS+=-Wno-packed-not-aligned\n'
	CONTENT+=$'#SELF_CFLAGS+=-fno-omit-frame-pointer\n'
	CONTENT+=$'SELF_CFLAGS+=-Wno-address-of-packed-member\n'
	CONTENT+=$'\n'

	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_THREAD_STACKSZ=0x20*1024*1024\n'
	CONTENT+=$'\n'

	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_ID_VALUE_BITS=28\n'
	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_ID_TYPE_BITS=4\n'
	CONTENT+=$'\n'

	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_SRC_BUF_START=0x100000000\n'
	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_SRC_BUF_BLKSZ=0x10000000\n'
	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_SRC_BUF_END=0x300000000\n'
	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_RESFILE_BUF_START=0x1000000000\n'
	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_RESFILE_BUF_SIZE=0x80000000\n'
	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_SIBUF_LOADED_MAX=0x100000000\n'
	CONTENT+=$'\n'

	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_SI_PATH_MAX=128\n'
	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_SRC_ID_LEN=4\n'
	CONTENT+=$'\n'

	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_MAX_OBJS_PER_FILE=0x800000\n'
	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_MAX_SIZE_PER_FILE=0x8000000\n'
	CONTENT+=$'\n'

	CONTENT+=$'#SELF_CFLAGS+=-DCONFIG_SAVED_SRC="src.saved"\n'
	CONTENT+=$'\n'

	CONTENT+=$'#SELF_CFLAGS+=-DGCC_CONTAIN_FREE_SSANAMES\n'
	CONTENT+=$'\n'

	CONTENT+=$'CFLAGS=-std=gnu11 $(SELF_CFLAGS) $(EXTRA_CFLAGS)\n'
	CONTENT+=$'CPPFLAGS=-std=gnu++11 $(SELF_CFLAGS) $(EXTRA_CFLAGS)\n'
	CONTENT+=$'\n'

	CONTENT+=$'ARCH=$(shell getconf LONG_BIT)\n'
	CONTENT+=$'CLIB_INC=$(CLIB_PATH)/include\n'
	CONTENT+=$'CLIB_LIB=$(CLIB_PATH)/lib\n'
	CONTENT+=$'CLIB_SO=clib$(ARCH)\n'
	CONTENT+=$'SRCINV_BIN=$(SRCINV_ROOT)/bin\n'
	CONTENT+=$'SRCINV_INC=$(SRCINV_ROOT)/include\n'

	echo "$CONTENT" > $TFILE
}

main()
{
	if [[ $# != 2 && $# != 3 ]]; then
		usage
		exit 1
	fi

	CLIB_PATH=$1
	SRCINV_ROOT=$2
	GCC_PLUGIN_BASE=$DEF_GCC_PLUGIN_BASE
	TFILE=pre_defines.makefile

	if [[ $# == 3 ]]; then
		GCC_PLUGIN_BASE=$3
	fi

	echo "Using"
	echo "    CLIB_PATH=$1"
	echo "    SRCINV_ROOT=$2"
	echo "    GCC_PLUGIN_BASE=$3"
	echo "to generate $TFILE..."
	gen_file $CLIB_PATH $SRCINV_ROOT $GCC_PLUGIN_BASE $TFILE
	echo "$TFILE ready."
	echo ""
	echo "Now, you can run 'make distclean && make && make install'"
}

main $@
