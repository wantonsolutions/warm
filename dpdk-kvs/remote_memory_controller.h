#ifndef RMEMC_H
#define RMEMC_H

#include "packets.h"

//Setup specific hard coded IP's
#define YAK0_IP 0xC01A8C0
#define YAK1_IP 0xD01A8C0
#define YAK2_IP 0xE01A8C0


void rmem_switch(struct rte_mbuf *pkt);

#endif