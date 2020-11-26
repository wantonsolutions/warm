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


#ifndef __CVMCS_kvs_MAIN_H__
#define __CVMCS_kvs_MAIN_H__
#include <cvmx.h>
#include "cvmx-config.h"
#include "cvmx-spinlock.h"
#include "global-config.h"
#include "cvmcs-nic.h"
#include "cvmcs-nic-ip.h"
#include "cvmcs-nic-tcp.h"
#include "cvmcs-nic-udp.h"
#include "cvmcs-nic-mdata.h"
#include "cvmcs-nic-component.h"
#include "octeon-pci-console.h"
#include "cvmcs-common.h"
#include  <cvmx-atomic.h>
#include "cvmcs-nic-ip.h"
#include "cvmcs-nic-tcp.h"
#include "cvmcs-nic-udp.h"
#include "cvmcs-nic.h"
#include "liquidio_common.h"
#include "cvmcs-nic-tunnel.h"
#include "cvmcs-nic-hybrid.h"
#include "cvmcs-nic.h"
#include "cvmcs-nic-mdata.h"
#include "cvmcs-nic-switch.h"
#include "cvmcs-nic-rss.h"
#include "cvmx-rng.h"

#include "cvmcs-kvs-util.h"


#define kvs_MAX_PORTS MAX_OCTEON_NIC_PORTS
#define kvs_RIF_PORT_ID  128
#define kvs_WIRE0_RIF_PORT_ID 129
#define kvs_WIRE1_RIF_PORT_ID 130

#define CVMCS_kvs_TSO_HANDLED 2
#define CVMCS_kvs_TSO_NOT_VALID 3
#define CVMCS_kvs_NOT_HANDLED   0xBAD

#define CVM_PHYS_LOOPBACK_PORT 36
#define CVM_VIRT_LRO_CSUM_LOOPBACK_PORT  64

#define   MAX(x,y)      ((x) > (y) ? (x) : (y))

#define   IFNAMSIZ             16


/** OVS related identifiers **/
#define CVM_OVS_MSG                             0x1
#define CVM_OVS_SP_TO_FP_ZEROCOPY_TX            0x2
#define CVM_OVS_SP_TO_FP_NO_ZEROCOPY_TX         0x3
#define CVM_OVS_FPA_FREE                        0x4
#define CVM_OVS_IGNORE_ROUTE_LEARN              0x5

#define CVM_SP_CMD_WQE_TAG                      2


/*Types of the interfaces*/
#define PORT_PCI        0
#define PORT_XAUI       1
/*
 * Remote interface for commonication between host
 * and octlinux
 */
#define PORT_RIF        2

#define MAX_TUNN_PORTS	65536
/* size of the element in the bitmap array*/
#define SIZE_OF_ELEMENT_BITS 6
#define SIZE_OF_ELEMENT	(1 << SIZE_OF_ELEMENT_BITS)

#define MAX_TUNN_PORT_SIZE	(MAX_TUNN_PORTS >> SIZE_OF_ELEMENT_BITS)

typedef struct cvmcs_kvs_port {
	void *vport;
	char name[IFNAMSIZ];
	uint64_t hwaddr;
	int type;
	int ifidx;
	int port;
	int queue;
	int inited;
	uint16_t bus_devfn;
	uint16_t offset;
	uint16_t stride;
#ifdef LIVE_MIGRATE
	int bond_ifidx;
	int bond_type;
#endif
	uint8_t linkup;
	int mtu; /* mtu and effective_mtu are same for kvsitch ports */
	int max_mtu;
	int is_mtu_master;
} cvmcs_kvs_port_t;

/* Internal PCI header information structure*/
typedef struct cvmcs_kvs_pci_info {
	int ifidx;
	tx_info_t txinfo;
	union octnic_packet_params packet_params;
	int      front_size; 
} cvmcs_kvs_pci_info_t;

/* PktStats context */

typedef struct cvmcs_kvs_global_ctx {
	uint64_t total_pkts;
} cvmcs_kvs_global_ctx_t;



/* variables used to dump FP code to SP */
CVMX_SHARED extern int is_fp_stats_on;
CVMX_SHARED extern int fp_stats_offset;

int kvs_receive_packet(cvmx_wqe_t * wqe, int port);
int kvs_send_packet_to_host(cvmx_wqe_t * wqe, int inport, int oport,
			   int is_tunnel, int * ifidx);
int kvs_send_packet_to_pci_port(cvmx_wqe_t * wqe, int ifidx, int qdx, int l3_offset,
				int l4_offset, int flags);

int kvs_send_packet_to_phys_port(cvmx_wqe_t * wqe, int ifidx, int l3_offset,
				int l4_offset, int flags);

/*WQE Util functions */
static inline void *cvm_kvs_wqe_get_pkt_ptr(cvmx_wqe_t *wqe)
{
	if (OCTEON_IS_MODEL(OCTEON_CN78XX) ||
		OCTEON_IS_MODEL(OCTEON_CN73XX)) {
		return cvmx_phys_to_ptr(cvmx_wqe_get_pki_pkt_ptr(wqe).addr);
	}else {
               return cvmx_phys_to_ptr(wqe->packet_ptr.s.addr);
	}
}

static inline int cvm_kvs_wqe_get_pkt_size(cvmx_wqe_t *wqe)
{
	if (OCTEON_IS_MODEL(OCTEON_CN78XX) ||
		OCTEON_IS_MODEL(OCTEON_CN73XX)) {
		return cvmx_wqe_get_pki_pkt_ptr(wqe).size; 
	}else {
		return wqe->packet_ptr.s.size; 
	}
}

static inline void cvm_kvs_wqe_set_pkt_size(cvmx_wqe_t *wqe, int size)
{
	if (OCTEON_IS_MODEL(OCTEON_CN78XX) ||
		OCTEON_IS_MODEL(OCTEON_CN73XX)) {
		cvmx_buf_ptr_pki_t *pki_ptr = (cvmx_buf_ptr_pki_t *)&wqe->packet_ptr;
		pki_ptr->size = size;
	}else {
		wqe->packet_ptr.s.size = size;
	}
}
void cvmcs_kvs_update_tx_fwd_stats(uint8_t from_if, int from_idx, int to_idx, int len, int fromq_idx);
int kvs_process_host_packet(cvmx_wqe_t * wqe);
int kvs_process_wire_packet(cvmx_wqe_t ** wqe_ref, int * pci_idx);
void cvmcs_kvs_update_rx_err_stats(uint8_t from_if, int from_idx, int to_idx);
void cvmcs_kvs_update_rx_fwd_stats(uint8_t from_if, int from_idx, int to_idx, 
int len, int fromq_idx, int toq_idx);
void cvmcs_kvs_update_tx_err_stats(uint8_t from_if, int from_idx, int to_idx);
int cvmcs_kvs_send_to_pko3(cvmx_wqe_t * wqe, int dir, int port, int queue,
		       int l3_offset, int l4_offset, int flags,
			vnic_port_info_t * nicport);
#endif
