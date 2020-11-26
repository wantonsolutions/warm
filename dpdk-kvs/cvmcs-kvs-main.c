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


#include "cvmcs-kvs-main.h"

#include "cuckoohash.h"


extern CVMX_SHARED cvmcs_component_t *lio_components[CVMCS_MAX_COMPONENTS];
extern CVMX_SHARED cvm_per_core_stats_t *per_core_stats;

CVMX_SHARED cvmcs_kvs_global_ctx_t *kvs_ctx;


int cvmcs_kvs_mem_init(void)
{
    /* Initialize memory */
    return 0;
}

static int cvmcs_set_kvs_grpmsk(void)
{
    // TODO: Does nothing for now
	// uint64_t grp_mask[4] = { 0, 0, 0, 0 };
	// int c = 0;

    // grp_mask[0] = (1ull << LINUX_POW_GROUP);

	// for (c = 0; c < CVMCS_FIRST_CORE; c++) {
	// 	printf("Core %d global grp_mask 0x%lx\n", c, grp_mask[0]);
	// 	cvmx_pow_set_xgrp_mask(c, 0x3, grp_mask);
	// }

	return 0;
}

int cvmcs_kvs_nic_run_spoofcheck(cvmx_wqe_t *wqe)
{
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);
	struct ethhdr *eth = (struct ethhdr *)CVMCS_NIC_METADATA_PACKET_START(mdata);

	if (!octnic->port[mdata->from_ifidx].linfo.macaddr_spoofchk) {
		return 0;
	}

	/* check source mac address */
	return memcmp(&eth->h_source, ((u8 *)&octnic->port[mdata->from_ifidx].user_set_macaddr + 2), ETH_ALEN);
}

int cvmcs_kvs_mdata_host_init(cvmx_wqe_t *wqe)
{
	uint64_t nextptr;
    cvmx_buf_ptr_pki_t  *pki_lptr;
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);

	/* Return if mdata is already initialized */
	if (CVMCS_NIC_METADATA_IS_MDATA_INIT(mdata))
		return 0;

	memset(mdata, 0, sizeof(cvmcs_nic_metadata_t));

	mdata->from_port = cvmx_wqe_get_port(wqe);

	/* Packet came from one of DPI ports */
	mdata->from_interface = METADATA_PORT_DPI;
	mdata->from_ifidx = get_vnic_port_id(mdata->from_port);
	mdata->gmx_id = octnic->port[mdata->from_ifidx].gmxport_id;
	mdata->gmx_port = octnic->port[mdata->from_ifidx].linfo.gmxport;

	memcpy(&mdata->front,
			(cvmx_raw_inst_front_t *)cvmx_phys_to_ptr(cvmx_wqe_get_pki_pkt_ptr(wqe).addr),
			CVM_RAW_FRONT_SIZE);

	if (mdata->front.irh.s.rflag)
		mdata->front_size = CVM_RAW_FRONT_SIZE;
	else
		mdata->front_size = CVM_RAW_FRONT_SIZE-16;

	/* Strip front data */
	pki_lptr = (cvmx_buf_ptr_pki_t *)&wqe->packet_ptr;
	nextptr = *((uint64_t *)CVM_DRV_GET_PTR(pki_lptr->addr - 8));
	pki_lptr->addr += mdata->front_size;
	pki_lptr->size -= mdata->front_size;
	*((uint64_t *)CVM_DRV_GET_PTR(pki_lptr->addr - 8)) = nextptr;

	cvmx_wqe_set_len(wqe, (cvmx_wqe_get_len(wqe) - mdata->front_size));

	mdata->packet_start = (uint8_t *)PACKET_START(wqe);

	/* Set initialization flag to check for to avoid re-initialization */
	mdata->flags |= METADATA_FLAGS_MDATA_INIT;

    // DO WE NEED IT?
	//needed for PF's vlan offload for now.  when coming on VF
	//this info will be 0.
	// mdata->outer_vlanTCI =  (mdata->front.irh.s.priority << 13) |
	// 				mdata->front.irh.s.vlan;
	// cvmcs_kvs_insert_vlan_tag(wqe);
	// mdata->outer_vlanTCI = 0;

	return 0;
}

int cvmcs_kvs_find_idx(cvmx_wqe_t * wqe)
{
	int port = cvmx_wqe_get_port(wqe);

	if (OCTEON_IS_MODEL(OCTEON_CN78XX)) {
		if (port == 0xA00 || port == 0xC00)
			return octnic->gmx_ids[((port >> 8) & 0xf) - 8];
	} else if (OCTEON_IS_MODEL(OCTEON_CN73XX)) {
		if (port == 0xA00 || port == 0xA10)
			return octnic->gmx_ids[(port >> 4) & 0xf];
		else if (port == 0x800)
			return octnic->gmx_ids[0];
		else if (port == 0x900)
			return octnic->gmx_ids[1];
	} else if (OCTEON_IS_MODEL(OCTEON_CN68XX)) {
		if ((port == 0x840 || port == 0x940 ||
		     port == 0xB40 || port == 0xC40))
			return octnic->gmx_ids[((port >> 8) & 0xf) - 8];
	} else if (OCTEON_IS_MODEL(OCTEON_CN66XX)) {
		if ((port == 0) || (port == 16))
			return octnic->gmx_ids[(port >> 4)];
	}
	return -1;
}

int cvmcs_kvs_mdata_wire_init(cvmx_wqe_t *wqe)
{
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);
	
	/* Return if mdata is already initialized */
	if (CVMCS_NIC_METADATA_IS_MDATA_INIT(mdata))
		return 0;

	memset(mdata, 0, sizeof(cvmcs_nic_metadata_t));

	mdata->from_port = cvmx_wqe_get_port(wqe);
	mdata->gmx_port = mdata->from_port;
	mdata->front_size = 0;
    mdata->from_ifidx = GMX_PF_INDEX(cvmcs_kvs_find_idx(wqe));
    mdata->from_interface = METADATA_PORT_GMX;

	mdata->packet_start = (uint8_t *)PACKET_START(wqe);

	/* Set initialization flag to check for to avoid re-initialization */
	mdata->flags |= METADATA_FLAGS_MDATA_INIT;
    return 0;
}

int cvmcs_kvs_parse_mdata(cvmx_wqe_t *wqe)
{
	uint16_t l2proto, l2hlen;
	uint8_t l3proto;
	void  *outer_l3hdr, *outer_l4hdr;
	void  *inner_l2hdr = NULL, *inner_l3hdr, *inner_l4hdr;
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);
	struct ethhdr *eth;

	eth = (struct ethhdr *)CVMCS_NIC_METADATA_PACKET_START(mdata);

	mdata->outer_l2offset = 0;
	mdata->inner_l2offset = 0;
	mdata->inner_l3offset = 0;
	mdata->inner_l4offset = 0;
	
	/*reset packet header flags, we will compute them again*/
	/*TODO METADATA_FLAGS_DUP_WQE was set for some packets.
	 * Need to debug*/
#ifdef TRITON_COMPONENTS
    	mdata->flags &= (METADATA_FLAGS_PTP_HEADER | METADATA_FLAGS_MDATA_INIT | METADATA_FLAGS_CSUM_L3 | METADATA_FLAGS_CSUM_L4);
#else
	mdata->flags &= (METADATA_FLAGS_PTP_HEADER | METADATA_FLAGS_CSUM_L3 | METADATA_FLAGS_CSUM_L4); 
#endif


	if (eth->h_proto == ETH_P_8021Q){
		struct vlan_hdr *vh = (struct vlan_hdr *)eth;
		mdata->flags |= METADATA_FLAGS_VLAN;
		l2proto = vh->proto;
		l2hlen = VLAN_ETH_HLEN;
	} else {
		l2proto = eth->h_proto;
		l2hlen = ETH_HLEN;
	}

	if ((l2proto != ETH_P_IP) && (l2proto != ETH_P_IPV6)) {
		/* Not IP Protocol */
		return 0;
	}

	outer_l3hdr = (void *)((uint8_t *)eth + l2hlen);
	mdata->outer_l3offset = mdata->outer_l2offset + l2hlen;

	if (l2proto == ETH_P_IP) {
		struct iphdr *iph4 = (struct iphdr *)outer_l3hdr;

		mdata->flags |= METADATA_FLAGS_IPV4;

		if ((iph4->frag_off & IP_MF) || (iph4->frag_off & IP_OFFSET))
			mdata->flags |= METADATA_FLAGS_IP_FRAG;

		if (iph4->ihl > 5)
			mdata->flags |= METADATA_FLAGS_IP_OPTS_OR_EXTH;

		outer_l4hdr = (void *)((uint8_t *)iph4 + (iph4->ihl << 2));
		mdata->outer_l4offset = mdata->outer_l3offset + (iph4->ihl << 2);

		l3proto = iph4->protocol;

	} else {
        printf("IPv6 not supported! Anil - 2019 Dec 20.\n");
        return 1;
	} 

	if (IPPROTO_UDP == l3proto) {
		// struct udphdr *udp = (struct udphdr *)outer_l4hdr;

	} else if (IPPROTO_TCP == l3proto) {
		struct tcphdr *tcp = (struct tcphdr *)outer_l4hdr;

		mdata->flags |= METADATA_FLAGS_TCP;
		if (tcp->syn)
			mdata->flags |= METADATA_FLAGS_TCP_SYN;
	}

	if (mdata->flags & METADATA_FLAGS_TUNNEL) {

		eth = (struct ethhdr *)inner_l2hdr;

		if (eth->h_proto == ETH_P_8021Q){
			struct vlan_hdr *vh = (struct vlan_hdr *)eth;
			mdata->flags |= METADATA_FLAGS_INNER_VLAN;
			mdata->inner_vlanTCI = vh->vlan_TCI;
			l2proto = vh->proto;
			l2hlen = VLAN_ETH_HLEN;
		} else {
			l2proto = eth->h_proto;
			l2hlen = ETH_HLEN;
		}

		if ((l2proto != ETH_P_IP) && (l2proto != ETH_P_IPV6)) {
			/* Not IP Protocol */
			return 0;
		}

		inner_l3hdr = (void *)((uint8_t *)eth + l2hlen);
		mdata->inner_l3offset = mdata->inner_l2offset + l2hlen;

		inner_l4hdr = NULL;

		if (ETH_P_IP == l2proto) {
			struct iphdr *iph4 = (struct iphdr *)inner_l3hdr;

			mdata->flags |= METADATA_FLAGS_INNER_IPV4;
			if ((iph4->frag_off & IP_MF) || (iph4->frag_off & IP_OFFSET))
				mdata->flags |= METADATA_FLAGS_INNER_IP_FRAG;

			if (iph4->ihl > 5)
				mdata->flags |= METADATA_FLAGS_INNER_IP_OPTS_OR_EXTH;


			l3proto = iph4->protocol;

			inner_l4hdr = (void *)iph4 + (iph4->ihl << 2);
			mdata->inner_l4offset =
				mdata->inner_l3offset + (iph4->ihl << 2);
		} else if (ETH_P_IPV6 == l2proto) {
            printf("IPv6 not supported! Anil - 2019 Dec 20.\n");
            return 1;
		}

		if (IPPROTO_TCP == l3proto) {
			mdata->flags |= METADATA_FLAGS_INNER_TCP;
		} else if (IPPROTO_UDP == l3proto) {
			mdata->flags |= METADATA_FLAGS_INNER_UDP;
		}

		mdata->header_len = mdata->inner_l4offset +
			(((struct tcphdr *)inner_l4hdr)->doff << 2);
	} else {
		mdata->header_len = mdata->outer_l4offset +
			(((struct tcphdr *)outer_l4hdr)->doff << 2);
	}

	return 0;
}

int cvmcs_kvs_mdata_offload(cvmx_wqe_t * wqe, int *l3_offset, int *l4_offset)
{
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);

	if (CVMCS_NIC_METADATA_IS_TUNNEL(mdata)) {
		*l3_offset = CVMCS_NIC_METADATA_INNER_L3_OFFSET(mdata);
		if (CVMCS_NIC_METADATA_IS_INNER_TCP(mdata) || 
		    CVMCS_NIC_METADATA_IS_INNER_UDP(mdata)) {
			if (!CVMCS_NIC_METADATA_IS_INNER_IP_FRAG(mdata))
				*l4_offset = CVMCS_NIC_METADATA_INNER_L4_OFFSET(mdata);
		}
	} else {
		*l3_offset = CVMCS_NIC_METADATA_L3_OFFSET(mdata);
		if (CVMCS_NIC_METADATA_IS_TCP(mdata)||
		    CVMCS_NIC_METADATA_IS_UDP(mdata)) {
			if (!CVMCS_NIC_METADATA_IS_IP_FRAG(mdata))
				*l4_offset = CVMCS_NIC_METADATA_L4_OFFSET(mdata);
		}
	}

	return 0;
}

/* Direction: 0 - to wire, 1 - to host */
int
cvmcs_kvs_send_to_pko3(cvmx_wqe_t * wqe, int dir, int port, int queue,
		       int l3_offset, int l4_offset, int flags,
			vnic_port_info_t * nicport)
{
	cvmx_pko_send_hdr_t hdr_s;
	cvmx_pko_query_rtn_t pko_status;
	cvmx_buf_ptr_pki_t *tmp_lptr;
	unsigned node, nwords;
	unsigned scr_base = cvmx_pko3_lmtdma_scr_base();
	cvmx_pko_buf_ptr_t send;
	unsigned dq = queue;
	//cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);
	uint32_t new_tag, next_tag;

	node = dq >> 10;
	dq &= (1 << 10) - 1;
	new_tag = (cvmx_wqe_get_tag(wqe) ^ dq); // ^ CVMCS_NIC_METADATA_FW_CRC(mdata)); FIX ME?
	cvmx_pow_tag_sw_full(wqe, new_tag,
			CVMX_POW_TAG_TYPE_ATOMIC, cvmx_wqe_get_grp(wqe));

	/* Fill in header */
	hdr_s.u64 = 0;
	hdr_s.s.total = cvmx_wqe_get_len(wqe);
	hdr_s.s.df = 0;
	hdr_s.s.ii = 0;
	if (OCTEON_IS_MODEL(OCTEON_CN78XX_PASS1_X))
		hdr_s.s.n2 = 0;	/* L2 allocate everything */
	else
		hdr_s.s.n2 = 1;	/* No L2 allocate works faster */
	hdr_s.s.aura = cvmx_wqe_get_aura(wqe);
#ifdef __LITTLE_ENDIAN_BITFIELD
	hdr_s.s.le = 1;
#endif

	if (l3_offset) {
		if (flags & METADATA_FLAGS_TUNNEL) {
			if (flags & METADATA_FLAGS_INNER_IPV4)
				hdr_s.s.ckl3 = 1;
			hdr_s.s.l3ptr = l3_offset;
			hdr_s.s.l4ptr = l4_offset;
			if (flags & METADATA_FLAGS_INNER_TCP)
				hdr_s.s.ckl4 = CKL4ALG_TCP;
			if (flags & METADATA_FLAGS_INNER_UDP)
				hdr_s.s.ckl4 = CKL4ALG_UDP;

		} else {
			if (flags & METADATA_FLAGS_IPV4)
				hdr_s.s.ckl3 = 1;
			hdr_s.s.l3ptr = l3_offset;
			hdr_s.s.l4ptr = l4_offset;
			if (flags & METADATA_FLAGS_TCP)
				hdr_s.s.ckl4 = CKL4ALG_TCP;
			if (flags & METADATA_FLAGS_UDP)
				hdr_s.s.ckl4 = CKL4ALG_UDP;
		}
	}

	nwords = 0;
	cvmx_scratch_write64(scr_base + sizeof(uint64_t) * (nwords++),
			     hdr_s.u64);

	tmp_lptr = (cvmx_buf_ptr_pki_t *) & wqe->packet_ptr;
	send.u64 = 0;
	send.s.addr = tmp_lptr->addr;
	send.s.size = tmp_lptr->size;

	if (cvmx_wqe_get_bufs(wqe) > 1)
		send.s.subdc3 = CVMX_PKO_SENDSUBDC_LINK;
	else
		send.s.subdc3 = CVMX_PKO_SENDSUBDC_GATHER;
	cvmx_scratch_write64(scr_base + sizeof(uint64_t) * (nwords++),
			     send.u64);

	CVMX_SYNCWS;

	next_tag = cvmx_pow_tag_compose(cvmx_pow_tag_get_sw_bits(new_tag) + 1, 
								cvmx_pow_tag_get_hw_bits(new_tag));

	/* Do LMTDMA */
	pko_status = cvmcs_pko3_lmtdma(node, dq, nwords, true, false);
	if (cvmx_unlikely(pko_status.s.dqstatus != PKO_DQSTATUS_PASS)) {
		cvmx_pow_tag_sw(next_tag, CVMX_POW_TAG_TYPE_ORDERED);
		return -1;
	}
	cvmx_pow_tag_sw(next_tag, CVMX_POW_TAG_TYPE_ORDERED);
	return 0;
}


/** 
 *
 * \brief Send the packet to the BGX  over pci queue.
 * @param wqe Work to send to host
 * @param ifidx physical interface index the packet belongs to
 * @param l3_offset Starting of the L3 header
 * @param l4_offset Starting of the L4 header
 * @param flags  Packet metadata flags. See cvmcs-nic-mdata.h for various
 * 		 possible metadata flags
 * @return 0 on success, -1 on failure
 */
int kvs_send_packet_to_phys_port(cvmx_wqe_t * wqe, int ifidx, int l3_offset,
				int l4_offset, int flags)
{
	int ret = -1;
	vnic_port_info_t *nicport = &octnic->port[ifidx];
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);
	int len = cvmx_wqe_get_len(wqe);
	uint8_t from_if = mdata->from_interface;
	int from_idx = mdata->from_ifidx;
	int fromq_idx = 0;

	int port = nicport->linfo.gmxport;
	int queue = cvmx_pko_get_base_queue(nicport->linfo.gmxport);
	

	if(from_if == METADATA_PORT_DPI) {
		fromq_idx = (mdata->from_port-0x100) - octnic->port[mdata->from_ifidx].iq_base;
		 if(!((fromq_idx >= 0) && (fromq_idx < MAX_IOQS_PER_NICIF))) {
			printf("%s fromq_idx is invalid:%d\n", __func__, fromq_idx);
		}
	}

	if (cvmx_unlikely(port == -1 || queue == -1)) {
		cvm_free_wqe_wrapper(wqe);
		return ret;
	}

	ret =
	    cvmcs_kvs_send_to_pko3(wqe, 0, port, queue, l3_offset, l4_offset,
					flags, nicport);
	if(ret != 0) {
		kvsprintf("%s: core %d, PKO3 send on ifidx %d, phys port %d, queue %d, Failed\n", __func__,
		  cvmx_get_core_num(), ifidx, port, queue);
		cvm_free_wqe_wrapper(wqe);
		cvmcs_kvs_update_tx_err_stats(from_if, from_idx, ifidx);
		return -1;
	}

#ifdef OVS_IPSEC
	if((cvmx_wqe_get_pki_pkt_ptr(wqe).packet_outside_wqe))
	{
		cvmx_wqe_free(wqe);
		DBG("Freeing WQE %p\n", wqe);
	}
#endif


	cvmcs_kvs_update_tx_fwd_stats(from_if, from_idx, ifidx, len, fromq_idx);

	return ret;
}

int kvs_global_init(void)
{
	const cvmx_bootmem_named_block_desc_t *desc;
	uint64_t def_pki_tag_secret = 0x161756F9C6A94F22;

	printf("Initializing KVS component SE First core %d and Last core %d\n",
		CVMCS_FIRST_CORE,CVMCS_LAST_CORE);

	/* Allocating/finding of global kvsitch context for live upgrade
         * All global  variables should be part of this context */
	if (booting_for_the_first_time) {

		kvs_ctx = cvmx_bootmem_alloc_named(sizeof (cvmcs_kvs_global_ctx_t), 
					CVMX_CACHE_LINE_SIZE, "__kvs_global_ctx");
		memset(kvs_ctx, 0, sizeof(cvmcs_kvs_global_ctx_t));
	} else {
		desc = cvmx_bootmem_find_named_block("__kvs_global_ctx");
		if (desc)
			kvs_ctx = cvmx_phys_to_ptr(desc->base_addr);
	}

	if (!kvs_ctx) {
		printf("Error in %s the kvs context\n", 
			booting_for_the_first_time ? "allocation":"discovering");
		return -1;
	}

	if (!booting_for_the_first_time) {
		printf("Booting the ovs app for live upgrade\n");  
		return 0;
	}

	cvmcs_set_kvs_grpmsk();

	if (cvmcs_kvs_mem_init())
		return -1;

    cvmx_rng_enable();

	/* Not sure if we need it, but keeping it anyway */
	if(!def_pki_tag_secret) {
		def_pki_tag_secret = cvmx_rng_get_random64();
		// printf("Using Hardware generated value 0x%llx as tag sauce\n", def_pki_tag_secret);
	}
	cvmx_write_csr_node(cvmx_get_node_num(), CVMX_PKI_TAG_SECRET, def_pki_tag_secret);

	return 0;
}


/** 
 *
 * \brief Send the packet to the host over pci queue.
 * @param wqe Work to send to host
 * @param ifidx physical interface index the packet belongs to
 * @param rxq_idx PF/VF queue that packet needs to be sent to
 * @param l3_offset Starting of the L3 header
 * @param l4_offset Starting of the L4 header
 * @param flags  Packet metadata flags. See cvmcs-nic-mdata.h for various
 * 		 possible metadata flags
 * @return 0 on success, -1 on failure
 */
int kvs_send_packet_to_pci_port(cvmx_wqe_t * wqe, int ifidx, int rxq_idx, int l3_offset,
				int l4_offset, int flags)
{
	union octeon_rh *rh;
	int port, queue, rxq;
	int ret = 0;
	uint64_t nextptr;
	cvmx_buf_ptr_pki_t *pki_lptr;
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);
	uint8_t from_if = mdata->from_interface;
	int from_idx = mdata->from_ifidx;
	uint32_t hash = (uint32_t)-1; /* All ones !*/
	uint32_t hashtype = 0;
	int len = 0;
	int fromq_idx = -1;
	int offset = 0;

	vnic_port_info_t *nicport = &octnic->port[ifidx];

	if (!nicport->state.rx_on) {
		kvsprintf("%s: RX is OFF for %d\n", __func__, ifidx);
		cvm_free_wqe_wrapper(wqe);
		return -1;
	}

	if(from_if == METADATA_PORT_DPI) {
		fromq_idx = (mdata->from_port-0x100) - octnic->port[mdata->from_ifidx].iq_base;
		 if(!((fromq_idx >= 0) && (fromq_idx < MAX_IOQS_PER_NICIF))) {
			printf("%s fromq_idx is invalid:%d\n", __func__, fromq_idx);
		}
	}

	nextptr = *((uint64_t *)
		    cvmx_phys_to_ptr(cvmx_wqe_get_pki_pkt_ptr(wqe).addr - 8));
	pki_lptr = (cvmx_buf_ptr_pki_t *) & wqe->packet_ptr;
	pki_lptr->addr -= OCT_RH_SIZE + 8;
	pki_lptr->size += OCT_RH_SIZE + 8;
	*(uint64_t *) cvmx_phys_to_ptr(pki_lptr->addr - 8) = nextptr;
	rh = (union octeon_rh *)cvmx_phys_to_ptr(pki_lptr->addr);
	cvmx_wqe_set_len(wqe, (cvmx_wqe_get_len(wqe) + OCT_RH_SIZE + 8));

	len = cvmx_wqe_get_len(wqe);

	rxq = OCT_NIC_OQ_NUM(nicport, rxq_idx);

	rh->u64 = 0;
	rh->r_dh.opcode = OPCODE_NIC;
	rh->r_dh.subcode = OPCODE_NIC_NW_DATA;
	rh->r_dh.csum_verified = CNNIC_CSUM_VERIFIED;
	rh->r_dh.encap_on = 0;
	rh->r_dh.has_hwtstamp = 0;
	rh->r_dh.len = 0;
	offset += OCT_RH_SIZE;

	*(uint32_t *)(rh+1) = hash;
        *(((uint32_t *)(rh+1)) + 1) = hashtype;
	rh->r_dh.has_hash = 0x1; /* indicate hash */
	rh->r_dh.len += 1;
	offset += 8;
	len += 8;

	//Vlan offload support.
	//VLAN is already stripped and present in metadata.
	//currently only supported for PF.  
	if (CVMCS_NIC_METADATA_VLAN_TCI(mdata)) {
		rh->r_dh.priority = CVMCS_NIC_METADATA_PRIORITY(mdata);
		rh->r_dh.vlan = CVMCS_NIC_METADATA_VLAN_ID(mdata);
	}
	port = cvm_pci_get_oq_pkoport(rxq);
	queue = cvm_pci_get_oq_pkoqueue(rxq);

	if (cvmx_unlikely(port == -1 || queue == -1)) {
		cvm_free_wqe_wrapper(wqe);
		return -1;
	}

	if(!(CVMCS_NIC_METADATA_CSUM_L3(mdata) || CVMCS_NIC_METADATA_CSUM_L4(mdata)))
	{
		l3_offset = l4_offset = 0;
	} else {
		l3_offset += offset;
		l4_offset += offset;
	} 

	ret = cvmcs_kvs_send_to_pko3(wqe, 1, port, queue, l3_offset,
					l4_offset, flags, nicport);

	if(ret != 0) {
		kvsprintf("%s:core %d,  PKO3 send on ifidx %d, pci port %d, queue %d, Failed \n",
		  __func__, cvmx_get_core_num(), ifidx, port, queue);
		cvm_free_wqe_wrapper(wqe);
		cvmcs_kvs_update_rx_err_stats(from_if, from_idx, ifidx);
		return -1;
	}

	cvmcs_kvs_update_rx_fwd_stats(from_if, from_idx, ifidx, len, fromq_idx, rxq_idx);
	return ret;
}

static int kvs_per_core_init(void)
{
	int core = cvmx_get_core_num();
	printf("Initializing KVS component on core %d\n", core);


    /* TODO: Setitng per core group masks. Need to do this eventually */
	// if (OCTEON_IS_MODEL(OCTEON_CN73XX)) {
	// 	uint64_t grp_mask[4] = { 0, 0, 0, 0 };

	// 	/*Accept from all groups other than LINUX, 73xx has only 64 groups */
	// 	grp_mask[0] = ~((1ull << LINUX_POW_DATA_GROUP) 
	// 			 | (1ull << LINUX_POW_CTRL_GROUP));

	// 	printf("Core %d grp_mask 0x%lx\n", core, grp_mask[0]);
	// 	cvmx_pow_set_xgrp_mask(core, 0x3, grp_mask);
    // }
    // else{
    //     printf("Not our model! Anil - 2019 Dec 20.\n");
    //     return 1;
    // }

	return 0;
}

static int kvs_cmd_init(void)
{
	// nic_cmdl_register("command", command_hook);
	return 0;
}

/**
 * \brief Handles any control commands from the host. 
 *
 * @param wqe Work received
 * @param subcode subcode of the request
 * @return 0 On success
 */
int kvs_process_host_message(cvmx_wqe_t * wqe, int subcode)
{
	/* Host packets with opcode set to OPCODE_KVS are sent to this routine.
		This can be used for control-path between host app and its nic offload */
    printf("Host message received, nothing to do.\n");
	switch (subcode) {
	default:
		break;
	}

	/* Return code does not matter */
	return 0;
}


/**
 * \brief Process the packets received from the host
 *
 * @param wqe Work
 * @return COMP_CONSUMED is the packet was successfully switched in the
 *         FastPath.
 *         COMP_NOT_HANDLED if the packet was not handled and should 
 *         be sent to slowPath for further processing.
 */
int kvs_process_host_packet(cvmx_wqe_t * wqe)
{
    printf("Host packet received in KVS component\n");

	vnic_port_info_t *nicport;
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);
    int l3_off = 0, l4_off = 0;

    if (!OCTEON_IS_MODEL(OCTEON_CN78XX) && !OCTEON_IS_MODEL(OCTEON_CN73XX))
    	return COMP_NOT_HANDLED;

	/* Init metadata before main processing  */
	cvmcs_kvs_mdata_host_init(wqe);

	/* Don't touch the PF traffic */
	if (ISPF(mdata->from_ifidx))
	{
    	printf("PF packet received in KVS component\n");
		return COMP_NOT_HANDLED;
	}

	printf("VF packet received in KVS component, sending it to wire myself\n");

	/* Packet was not handled in this component and needs further processing
		When we are *not* returning this return code, we need to be sure that packet is 
		definitely ours or bad things will happen */
	// if (<not my packet>)
	//	return COMP_NOT_HANDLED;

    /* Don't do spoof-checking for now */
	// if (cvmcs_nic_run_spoofcheck(wqe)) {
	// 	DBG("spoof src mac detected: from_ifidx=%d, mymac=%lx\n",
	// 	    mdata->from_ifidx,  octnic->port[mdata->from_ifidx].user_set_macaddr);
	// 	per_core_stats[cvmx_get_core_num()].link_stats[mdata->from_ifidx].fromhost.fw_err_drop += 1;
	// 	per_core_stats[cvmx_get_core_num()].vf_stats[mdata->from_ifidx].spoofmac_cnt += 1;
	// 	return COMP_DROP;
	// }

	nicport = &octnic->port[mdata->from_ifidx];
	if (cvmx_unlikely(nicport->linfo.link.s.link_up == 0)) {
		//cvmx_atomic_add_u64(&nicport->stats.fromhost.fw_err_link, 1);
		return COMP_DROP;
	}

	cvmcs_kvs_parse_mdata(wqe);

    cvmcs_kvs_mdata_offload(wqe, &l3_off, &l4_off);
    kvs_send_packet_to_phys_port(wqe, mdata->from_ifidx, 
            l3_off, l4_off, mdata->flags);

    /* Packet was consumed, no need to do further processing */
    return COMP_CONSUMED;
}


/**
 * \brief Process the packets received from the wire over
 * BGX.
 *
 * @param wqe Work
 * @return COMP_CONSUMED is the packet was successfully switched in the
 *         FastPath.
 *         COMP_NOT_HANDLED if the packet was not handled.
 */
int kvs_process_wire_packet(cvmx_wqe_t ** wqe_ref, int * pci_idx)
{
    printf("Wire packet received in KVS component\n");
	//return COMP_NOT_HANDLED;

	int err = 0, qidx;
	cvmx_wqe_t *wqe = *wqe_ref;
	cvmcs_nic_metadata_t *mdata = CVMCS_NIC_METADATA(wqe);
	int err_code = cvmx_wqe_get_rcv_err(wqe);
	pkt_proc_flags_t flags;
	memset(&flags,0,sizeof(flags));

	/* Init metadata before main processing */
	cvmcs_kvs_mdata_wire_init(wqe);

	if (mdata->from_ifidx == -1) {
		printf(" %s : invalid ifidx = %d, port %d\n",
			  __func__, mdata->from_ifidx, mdata->from_port);
		cvmx_helper_dump_packet(wqe);
		return COMP_DROP;
	}

	if (cvmx_unlikely(err_code)) {
		if (cvmcs_nic_opcode_to_stats(mdata->from_ifidx, err_code)) {
			printf("L2/L1 error from port %d. Error code=%x\n",
					cvmx_wqe_get_port(wqe), err_code);
			return COMP_DROP;;
		}
	}

	if (!octnic->port[mdata->from_ifidx].state.active) {
		return COMP_DROP;
    }

	per_core_stats[cvmx_get_core_num()].link_stats[mdata->from_ifidx].fromwire.fw_total_rcvd += 1;

	err = cvmcs_nic_validate_rx_frame_len(wqe, mdata->from_ifidx); 
	if (err) {
		/* Drop the packet */
		cvm_free_wqe_wrapper(wqe);
		return err;
	}

    // TODO: Steer packets to proper VFs and Queues
    qidx = 0;

    //Start Doing KV stuff here
    
    printf("Packet received: length correct. sending to if: %d, queue: %d\n", mdata->from_ifidx, qidx);
	kvs_send_packet_to_pci_port(wqe, mdata->from_ifidx, qidx,
			CVMCS_NIC_METADATA_L3_OFFSET(mdata), 
			CVMCS_NIC_METADATA_L4_OFFSET(mdata),
			mdata->flags);

	return COMP_CONSUMED;
}


CVMX_SHARED cvmcs_component_t kvs_comp = {
	.name = "kvs",
	.opcode = OPCODE_KVS,
	.global_init = kvs_global_init,
	.per_core_init = kvs_per_core_init,
	.cmd_init = kvs_cmd_init,
	.from_host_msg_cb = kvs_process_host_message,
	.from_host_packet_cb = kvs_process_host_packet,
	.from_wire_packet_cb = kvs_process_wire_packet,
};
