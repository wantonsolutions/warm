import os

clear_all()

dpmap = {}
mapping_file = os.environ['OPERA_MAPPING_PATH'] if 'OPERA_MAPPING_PATH' in os.environ else '/tmp/dp_mappings_identity.csv'
with open(mapping_file, 'r') as f:
    lines = f.read().strip().splitlines()
    for l in lines:
        row = l.split(",")
        dpmap[int(row[0])] = int(row[1])
    f.close()

ip_dsts = {
    "10.0.0.1"  : 0, 
    "10.0.0.2"  : 4
}

for dst in ip_dsts:
    p4_pd.forward_table_add_with_set_egr(
        p4_pd.forward_match_spec_t(
            ipv4_dstAddr=ipv4Addr_to_i32(dst)
        ),
        p4_pd.set_egr_action_spec_t(dpmap[ip_dsts[dst]])
    )

conn_mgr.complete_operations()