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

#include "cvmx-config.h"
#include "global-config.h"
#include "cvmcs-common.h"
#include "cvmcs-nic.h"
#include "cvmcs-nic-component.h"
#include "cvmcs-nic-hybrid.h"
#include "cvmip.h"
#include "cvmcs-nic-udp.h"
#include "cvmcs-nic-tunnel.h"
#include "cvmx-rng.h"

#include "cvmcs-nic-mdata.h"
#include "cvmcs-kvs-util.h"

#include "cvmx-pcieepx-defs.h"
#include "cvmx-pcie.h"
#include "cvmcs-nic-rss.h"
#include "cvmcs-nic-switch.h"
#include <errno.h>


extern CVMX_SHARED cvm_per_core_stats_t *per_core_stats;

/*to wire*/
void cvmcs_kvs_update_tx_fwd_stats(uint8_t from_if, int from_idx, int to_idx, int len, int fromq_idx)
{
	if (from_if == METADATA_PORT_DPI) {
		if (!((from_idx >= 0) && (from_idx < MAX_OCTEON_NIC_PORTS))) {
			printf("%s: Index is not valid: from_idx:%d \n", __func__,from_idx);
			return;
		}
		per_core_stats[cvmx_get_core_num()].link_stats[from_idx].fromhost.fw_total_fwd += 1;
		per_core_stats[cvmx_get_core_num()].link_stats[from_idx].fromhost.fw_total_fwd_bytes += len;
		/*queue stat fromhost*/
		if((fromq_idx >= 0) && (fromq_idx < MAX_IOQS_PER_NICIF)) {
			per_core_stats[cvmx_get_core_num()].perq_stats[from_idx].fromhost.fw_total_fwd[fromq_idx] += 1;
			per_core_stats[cvmx_get_core_num()].perq_stats[from_idx].fromhost.fw_total_fwd_bytes[fromq_idx] += len;
		}
	} else {
		if (!((to_idx >= 0) && (to_idx < MAX_OCTEON_NIC_PORTS))) {
			printf("%s: Index is not valid: to_idx:%d\n", __func__, to_idx);
			return;
		}
		per_core_stats[cvmx_get_core_num()].link_stats[to_idx].fromwire.fw_total_fwd += 1;
	}
}

void cvmcs_kvs_update_tx_err_stats(uint8_t from_if, int from_idx, int to_idx)
{
	if (from_if == METADATA_PORT_DPI) {
		if (!((from_idx >= 0) && (from_idx < MAX_OCTEON_NIC_PORTS))) {
			printf("%s: Index is not valid: from_idx:%d \n", __func__,from_idx);
			return;
		}
		per_core_stats[cvmx_get_core_num()].link_stats[from_idx].fromhost.fw_err_pko += 1;
	} else {
		if (!((to_idx >= 0) && (to_idx < MAX_OCTEON_NIC_PORTS))) {
			printf("%s: Index is not valid: to_idx:%d\n", __func__, to_idx);
			return;
		}
		per_core_stats[cvmx_get_core_num()].link_stats[to_idx].fromwire.fw_err_pko += 1;
	}
}

/*to host*/
void cvmcs_kvs_update_rx_fwd_stats(uint8_t from_if, int from_idx, int to_idx, int len, int fromq_idx, int toq_idx)
{
	if (from_if == METADATA_PORT_DPI) {
		if (!((from_idx >= 0) && (from_idx < MAX_OCTEON_NIC_PORTS) && (to_idx >= 0) && (to_idx < MAX_OCTEON_NIC_PORTS))) {
			printf("%s: Index is not valid: from_idx:%d to_idx%d\n", __func__,from_idx,to_idx);
			return;
		}
		per_core_stats[cvmx_get_core_num()].link_stats[from_idx].fromhost.fw_total_fwd += 1;
		per_core_stats[cvmx_get_core_num()].link_stats[from_idx].fromhost.fw_total_fwd_bytes += len;

		per_core_stats[cvmx_get_core_num()].link_stats[to_idx].fromwire.fw_total_fwd += 1;
		/*queue stat fromhost*/
		if ((fromq_idx >= 0) && (fromq_idx < MAX_IOQS_PER_NICIF)) {
			per_core_stats[cvmx_get_core_num()].perq_stats[from_idx].fromhost.fw_total_fwd[fromq_idx] += 1;
			per_core_stats[cvmx_get_core_num()].perq_stats[from_idx].fromhost.fw_total_fwd_bytes[fromq_idx] += len;
		}
	} else {
		if (!((to_idx >= 0) && (to_idx < MAX_OCTEON_NIC_PORTS))) {
			printf("%s: Index is not valid: to_idx:%d\n", __func__, to_idx);
			return;
		}
		per_core_stats[cvmx_get_core_num()].link_stats[to_idx].fromwire.fw_total_fwd += 1;
	}
	/*queue stat fromwire*/
	if ((toq_idx >= 0) && (toq_idx < MAX_IOQS_PER_NICIF)) {
		per_core_stats[cvmx_get_core_num()].perq_stats[to_idx].fromwire.fw_total_fwd[toq_idx] += 1;
		per_core_stats[cvmx_get_core_num()].perq_stats[to_idx].fromwire.fw_total_fwd_bytes[toq_idx] += len;
	} 
	else {
		printf("%s: toq_idx is invalid: %d\n",  __func__, toq_idx);
	}

}

void cvmcs_kvs_update_rx_err_stats(uint8_t from_if, int from_idx, int to_idx)
{
	if (from_if == METADATA_PORT_DPI) {
		if (!((from_idx >= 0) && (from_idx < MAX_OCTEON_NIC_PORTS))) {
			printf("%s: Index is not valid: from_idx:%d \n", __func__,from_idx);
			return;
		}
		per_core_stats[cvmx_get_core_num()].link_stats[from_idx].fromhost.fw_err_pko += 1;
	} else {
		if (!((to_idx >= 0) && (to_idx < MAX_OCTEON_NIC_PORTS))) {
			printf("%s: Index is not valid: to_idx:%d\n", __func__, to_idx);
			return;
		}
		per_core_stats[cvmx_get_core_num()].link_stats[to_idx].fromwire.fw_err_pko += 1;
	}
}
