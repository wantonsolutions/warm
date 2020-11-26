/**********************************************************************
* Author: Cavium, Inc.
*
* Contact: support@cavium.com
*          Please include "LiquidIO" in the subject.
*
* Copyright (c) 2017 Cavium, Inc.  All rights reserved.
* All information contained herein is, and remains the property of the Cavium, Inc.
* The source code herein is proprietary and confidential and may contain trade secrets. 
* Any copying, modification, or public disclosure of the source code herein is strictly
* prohibited without express permission of Cavium, Inc. The receipt or possession of
* this source code does not covey or imply any rights to reproduce, disclose, or distribute
* its contents, in whole or in part.  
*
* This file is distributed in the hope that it will be useful, but
* AS-IS and WITHOUT ANY WARRANTY; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, TITLE, or
* NONINFRINGEMENT. 
*
* This file may also be available under a different license from Cavium.
* Contact Cavium, Inc. for more information
**********************************************************************/


#ifndef __CVMCS_KVS_UTIL_H
#define __CVMCS_KVS_UTIL_H

#include "cvmcs-nic.h"

extern CVMX_SHARED uint64_t secs_from_boot;

typedef uint8_t uint8_all_alias __attribute__ ((may_alias));
typedef uint16_t uint16_all_alias __attribute__ ((may_alias));

typedef struct resphdr {
	uint64_t identifier;
	struct {
		uint32_t status;
		uint32_t resplen;
	} s;
} resphdr_t;

typedef struct reqhdr {
	uint32_t opcode:8;
	uint32_t identifier:8;
	uint32_t dlen:16;
	uint64_t respptr;
	uint32_t resplen;
} reqhdr_t;

enum {
	CVMCS_KVS_DMAC_FILTER_NONE,
	CVMCS_KVS_DMAC_FILTER_ADD,
	CVMCS_KVS_DMAC_FILTER_DEL,
};

typedef struct cvmcs_KVS_dmac_filter_req_s {
	uint8_t cmd;
	uint8_t resv[3];
	uint32_t ipd_port;
	uint64_t dmac;
} cvmcs_KVS_dmac_filter_req_t;

/* Standard TCP flags */
#define TCP_FIN 0x001
#define TCP_SYN 0x002
#define TCP_RST 0x004
#define TCP_PSH 0x008
#define TCP_ACK 0x010
#define TCP_URG 0x020
#define TCP_ECE 0x040
#define TCP_CWR 0x080
#define TCP_NS  0x100


#define kvsprintf(args...) if (nic_verbose) printf(args)

#define HEXTODEC(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]
#define HEXDEC_FMT "%u.%u.%u.%u "

/* both set/get APIs are relative to current timestamp*/
static inline uint64_t cvmcs_get_secs()
{
	return secs_from_boot;
}

static inline uint64_t cvmcs_secs_after(int secs)
{
	return (secs_from_boot + secs);
}

#endif
