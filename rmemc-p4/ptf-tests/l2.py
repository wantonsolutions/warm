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

swports = []

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
        ingress_port = swports[0]
        egress_port  = swports[1]
        mac_da       = "00:11:11:11:11:11"
        #mac_da       = "00:55:55:55:55:55"
        

        print("Populating table entries")

        # self.entries dictionary will contain all installed entry handles
        self.entries={}
        self.entries["forward"] = []
        self.entries["forward"].append(
            self.client.forward_table_add_with_set_egr(
                self.sess_hdl, self.dev_tgt,
                basic_switching_forward_match_spec_t(
                    ethernet_dstAddr=macAddr_to_string(mac_da)),
                basic_switching_set_egr_action_spec_t(
                    action_egress_spec=egress_port)))

        print("Table forward: %s => set_egr(%d)" %
              (mac_da, egress_port))

        self.conn_mgr.complete_operations(self.sess_hdl)
        print("Sending packet with DST MAC=%s into port %d" %
              (mac_da, ingress_port))
        pkt = simple_tcp_packet(eth_dst=mac_da,
                                eth_src='00:11:11:11:11:11',
                                ip_dst='10.0.0.1',
                                ip_id=101,
                                ip_ttl=64,
                                ip_ihl=5)
        send_packet(self, ingress_port, pkt)
        print("Expecting packet on port %d" % egress_port)
        verify_packets(self, pkt, [egress_port])

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
