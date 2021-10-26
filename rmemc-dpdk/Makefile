# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
APP = rmemc-dpdk

# all source are stored in SRCS-y
SRCS-y := rmemc-dpdk.c roce_v2.c measurement.c print_helpers.c packet_templates.c

# Build using pkg-config variables if possible
ifeq ($(shell pkg-config --exists libdpdk && echo 0),0)

all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PKGCONF ?= pkg-config

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O3 -v $(shell $(PKGCONF) --cflags libdpdk)
#CFLAGS += -O0 -ggdb -v $(shell $(PKGCONF) --cflags libdpdk)
#EXTRA_CFLAGS += -O0 -ggdb  -Wfatal-errors $(STEWFLAGS)
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)
LDFLAGS_STATIC = -Wl,-Bstatic $(shell $(PKGCONF) --static --libs libdpdk)

STEWFLAGS := -DMITSUME_BENCHMARK_THREAD_NUM=${CLOVER_THREADS} \
		-DMITSUME_YCSB_KEY_RANGE=${CLOVER_KEY_RANGE} \
   		-DMITSUME_YCSB_OP_MODE=${CLOVER_YCSB_OP_MODE} \
		-DMITSUME_BENCHMARK_SIZE=${CLOVER_PAYLOAD_SIZE} \

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED) $(STEWFLAGS)

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC) $(STEWFLAGS)

build:
	@mkdir -p $@

.PHONY: clean

clean:
	echo "cleaning"
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	rm -fr build
	test -d build && rmdir -p build || true

else # Build using legacy build system

ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

$(info $$RTE_SDK is [${RTE_SDK}])

# Default target, detect a build directory, by looking for a path with a .config
RTE_TARGET ?= $(notdir $(abspath $(dir $(firstword $(wildcard $(RTE_SDK)/*/.config)))))


include $(RTE_SDK)/mk/rte.vars.mk

EXTRA_CFLAGS += -O3 -g -DNDEBUG -Wfatal-errors $(STEWFLAGS)
#EXTRA_CFLAGS+=-O0 -ggdb  -Wfatal-errors
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
