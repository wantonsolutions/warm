# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Simple PTF test for basic_switching.p4
"""

import pd_base_tests

from ptf import config
from ptf.testutils import *
from ptf.thriftutils import *

from basic_switching.p4_pd_rpc.ttypes import *
from res_pd_rpc.ttypes import *
#from scapy.layers.l2 import Ether

import scapy.contrib
import ptf
#from scapy import *

ROCE_PORT=4791

RC_SEND=0x04
RC_WRITE_ONLY=0x0A
RC_READ_REQUEST=0x0C
RC_READ_RESPONSE=0x10
RC_ACK=0x11
RC_ATOMIC_ACK=0x12
RC_CNS=0x13
ECN_OPCODE=0x81

#DATA_LENGTH=8192 #1024 * 8
#DATA_LENGTH=1024 #1024 * 8
#DATA_LENGTH=2048 #1024 * 8
DATA_LENGTH=4096 #1024 * 8
DATA_LENGTH=8000 #1024 * 8
#DATA_LENGTH=8191 #1024 * 8

# class Disney(Packet):
#     name = "DisneyPacket "
#     fields_desc=[ 
#                  ByteEnumField("opcode",0, 
#                     { }
#                  XByteField("minnie",3) ,
#                  IntEnumField("donald" , 1 ,
#                       { 1: "happy", 2: "cool" , 3: "angry" } ) ]

class RoceV2(Packet):
    name = "RoceV2Packet"
    fields_desc=[
        ByteEnumField("opcode",0, 
            { RC_SEND: "RC_SEND", RC_WRITE_ONLY: "RC_WRITE_ONLY", RC_READ_REQUEST: "RC_READ_REQUEST", RC_READ_RESPONSE: "RC_READ_RESPONSE", RC_ACK: "RC_ACK", RC_ATOMIC_ACK: "RC_ATOMIC_ACK", RC_CNS: "RC_CSN", ECN_OPCODE: "ECN_OPCODE"} ),
        BitField("solicited_event", 0 , 1),
        BitField("migration_request", 0, 1),
        BitField("pad_count", 0, 2),
        BitField("transport_header_version",0,4),
        BitField("partition_key", 0, 16),
        BitField("reserved",0,6),
        BitField("becn", 0 ,1),
        BitField("fecn", 0 ,1),
        BitField("dest_qp",0,24),
        BitField("ack",0,1),
        BitField("reserved",0,7),
        BitField("packet_sequence_number",0,24),
             ]

class ReadRequest(Packet):
    name = "ReadRequest"
    fields_desc=[
        BitField("virtual_address", 0, 64),
        BitField("rkey", 0, 32),
        BitField("dma_length", 0, 32),
        BitField("icrc",0,32),
    ]

class ReadResponse(Packet):
    name = "ReadResponse"
    fields_desc=[
        BitField("reserved",0,1),
        BitField("opcode",0,2),
        BitField("credit_count",0,5),
        BitField("sequence_number",0,24),
        BitField("ptr",0,64),
        BitField("data",0,DATA_LENGTH),
        BitField("icrc",0,32),
    ]

class WriteRequest(Packet):
    name = "WriteRequest"
    fields_desc=[
        BitField("virtual_address", 0, 64),
        BitField("rkey", 0, 32),
        BitField("dma_length", 0, 32),
        BitField("ptr",0,64),
        BitField("data",0,DATA_LENGTH),
        BitField("icrc",0,32),
    ]

class Ack(Packet):
    name = "RoceV2Ack"
    fields_desc=[
        BitField("reserved",0,1),
        BitField("opcode",0,2),
        BitField("credit_count",0,5),
        BitField("sequence_number",0,24),
        BitField("icrc",0,32),
    ]

class AtomicRequest(Packet):
    name = "RoceAtomicRequest"
    fields_desc=[
        BitField("virtual_address", 0, 64),
        BitField("rkey", 0, 32),
        BitField("swap_or_add",0,64),
        BitField("compare",0,64),
        BitField("icrc",0,32),
    ]

class AtomicResponse(Packet):
    name = "RoceAtomicResponse"
    fields_desc=[
        BitField("reserved",0,1),
        BitField("opcode",0,2),
        BitField("credit_count",0,5),
        BitField("sequence_number",0,24),
        BitField("original_remote_data",0,64),
        BitField("icrc",0,32),
    ]

bind_layers(UDP, RoceV2, dport=ROCE_PORT)

bind_layers(RoceV2, ReadRequest, opcode=RC_READ_REQUEST)
bind_layers(RoceV2, ReadResponse, opcode=RC_READ_RESPONSE)
bind_layers(RoceV2, WriteRequest, opcode=RC_WRITE_ONLY)
bind_layers(RoceV2, Ack, opcode=RC_ACK)
bind_layers(RoceV2, AtomicRequest, opcode=RC_CNS)
bind_layers(RoceV2, AtomicResponse, opcode=RC_ATOMIC_ACK)



#client
yeti_5_mac="ec:0d:9a:68:21:a0"
yeti_5_port=0

#memory
yak_0_mac="ec:0d:9a:68:21:d0"
yak_0_port=1

#meta
yak_1_mac="ec:0d:9a:68:21:cc"
yak_1_port=2

#this set of forwarding tripples allows each of the client, memory, and metadata servers to send to one another
#ingress port, egress port, mac_destination
forwarding_tripples=[
    (yeti_5_port,yak_0_port,yak_0_mac),
    (yak_1_port,yak_0_port,yak_0_mac),
    (yak_0_port,yeti_5_port,yeti_5_mac),
    (yak_1_port,yeti_5_port,yeti_5_mac),
    (yak_0_port,yak_1_port,yak_1_mac),
    (yeti_5_port,yak_1_port,yak_1_mac),
]

mac_forwarding_tripples=[
    (yak_0_port,yak_0_mac),
    (yeti_5_port,yeti_5_mac),
    (yak_1_port,yak_1_mac),
]

ingress_ports=[yeti_5_port,yak_0_port,yak_1_port]



swports = []

pkt_str = b"\xEC\x0D\x9A\x68\x21\xCC\xEC\x0D\x9A\x68\x21\xA0\x08\x00\x45\x02\x04\x3C\x68\xD6\x40\x00\x40\x11\x4A\x6A\xC0\xA8\x01\x11\xC0\xA8\x01\x0D\xF0\x12\x12\xB7\x04\x28\x00\x00\x0A\x40\xFF\xFF\x00\x00\x45\x20\x80\x00\x0C\x71\x00\x00\x56\x0D\x3F\x37\x65\x00\x00\x04\xA9\x88\x00\x00\x04\x00\x00\x08\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
#pkt = Ether(pkt_str)
#pkt.show()


print config

for device, port, ifname in config["interfaces"]:
    swports.append(port)
    swports.sort()

if swports == []:
    swports = [0, 4]


class L2Test(pd_base_tests.ThriftInterfaceDataPlane):
    def __init__(self):
        pd_base_tests.ThriftInterfaceDataPlane.__init__(self,
                                                        ["bs"])

    # The setUp() method is used to prepare the test fixture. Typically
    # you would use it to establich connection to the Thrift server.
    #
    # You can also put the initial device configuration there. However,
    # if during this process an error is encountered, it will be considered
    # as a test error (meaning the test is incorrect),
    # rather than a test failure
    def setUp(self):
        pd_base_tests.ThriftInterfaceDataPlane.setUp(self)

        self.sess_hdl = self.conn_mgr.client_init()
        self.dev      = 0
        self.dev_tgt  = DevTarget_t(self.dev, hex_to_i16(0xFFFF))

        print("\nConnected to Device %d, Session %d" % (
            self.dev, self.sess_hdl))

    # This method represents the test itself. Typically you would want to
    # configure the device (e.g. by populating the tables), send some
    # traffic and check the results.
    #
    # For more flexible checks, you can import unittest module and use
    # the provided methods, such as unittest.assertEqual()
    #
    # Do not enclose the code into try/except/finally -- this is done by
    # the framework itself
    def runTest(self):
        # Test Parameters

        for tup in mac_forwarding_tripples:
            ingress_port = tup[0]
            egress_port  = tup[0]
            mac_da       = tup[1]
            

            print("Populating table entries")

            # self.entries dictionary will contain all installed entry handles

            self.entries={}
            self.entries["forward"] = []
            self.entries["forward"].append(
                # self.client.forward_table_add_with_dec_ttl(
                self.client.forward_table_add_with_set_egr(
                    self.sess_hdl, self.dev_tgt,
                    basic_switching_forward_match_spec_t(
                        ethernet_dstAddr=macAddr_to_string(mac_da)),
                    basic_switching_set_egr_action_spec_t(
                        action_egress_spec=egress_port)))

            self.entries["update"] = []
            self.entries["update"].append(
                self.client.update_table_add_with_dec_ttl(
                    self.sess_hdl, self.dev_tgt,
                    basic_switching_forward_match_spec_t(
                        ethernet_dstAddr=macAddr_to_string(mac_da))))

            print("Table forward: %s => set_egr(%d)" %
                (mac_da, egress_port))


        ingress_port = yeti_5_port
        self.conn_mgr.complete_operations(self.sess_hdl)
        print("Sending packet with DST MAC=%s into port %d" %
              (mac_da, ingress_port))


        #first example
        pkt = Ether(pkt_str)
        #pkt.show()

        egress_port = yak_1_port
        send_packet(self, ingress_port, pkt)

        pkt2=pkt.copy()
        pkt2[IP].ttl = pkt2[IP].ttl-5
        #pkt2[RoceV2].partition_key=0
        #print("Expecting packet on port %d" % egress_port)

        # print(pkt[TCP].ttl = 63)
        #ptk2[RoceV2].partition_key=0
        #pkt2.show()



        verify_packets(self, pkt2, [egress_port])


        # timeout = 2
        # result = dp_poll(self, device_number=0, port_number=egress_port, timeout=timeout, exp_pkt=pkt2)
        # pkt_result = result.packet
        # pkt_result.show()
        # print(pkt_result[RoceV2].partition_key)
        # pkt2.show()

    # Use this method to return the DUT to the initial state by cleaning
    # all the configuration and clearing up the connection
    def tearDown(self):
        try:
            print("Clearing table entries")
            for table in self.entries.keys():
                delete_func = "self.client." + table + "_table_delete"
                for entry in self.entries[table]:
                    exec delete_func + "(self.sess_hdl, self.dev, entry)"
        except:
            print("Error while cleaning up. ")
            print("You might need to restart the driver")
        finally:
            self.conn_mgr.complete_operations(self.sess_hdl)
            self.conn_mgr.client_cleanup(self.sess_hdl)
            print("Closed Session %d" % self.sess_hdl)
            pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
