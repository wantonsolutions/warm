# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
APP = rmemc-dpdk

# all source are stored in SRCS-y
SRCS-y := rmemc-dpdk.c roce_v2.c measurement.c print_helpers.c packet_templates.c

# Build using pkg-config variables if possible
ifeq ($(shell pkg-config --exists libdpdk && echo 0),0)


else # Build using legacy build system

ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

$(info $$RTE_SDK is [${RTE_SDK}])

# Default target, detect a build directory, by looking for a path with a .config
RTE_TARGET ?= $(notdir $(abspath $(dir $(firstword $(wildcard $(RTE_SDK)/*/.config)))))


include $(RTE_SDK)/mk/rte.vars.mk

EXTRA_CFLAGS += -O3 -Wfatal-errors $(STEWFLAGS)
EXTRA_LDFLAGS+=-lz

STEWFLAGS := -DMITSUME_BENCHMARK_THREAD_NUM=${CLOVER_THREADS} \
		-DMITSUME_YCSB_KEY_RANGE=${CLOVER_KEY_RANGE} \
   		-DMITSUME_YCSB_OP_MODE=${CLOVER_YCSB_OP_MODE} \
		-DMITSUME_BENCHMARK_SIZE=${CLOVER_PAYLOAD_SIZE} \

CFLAGS += $(EXTRA_CFLAGS) $(WERROR_FLAGS) $(STEWFLAGS)

# workaround for a gcc bug with noreturn attribute
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=12603
# ifeq ($(CONFIG_RTE_TOOLCHAIN_GCC),y)
# CFLAGS_main.o += -Wno-return-type 
# endif


include $(RTE_SDK)/mk/rte.extapp.mk
endif
