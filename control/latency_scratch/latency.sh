#!/bin/bash

mkdir -p yeti0
mkdir -p yeti5

rm yeti0/*
rm yeti5/*

#read latency
scp yeti0:/tmp/read_latency* ./yeti0
scp yeti5:/tmp/read_latency* ./yeti5

#write_latency
scp yeti0:/tmp/write_latency* ./yeti0
scp yeti5:/tmp/write_latency* ./yeti5

#create agg file
rm agg_read_latency
cat yeti0/read_latency* >> agg_read_latency
cat yeti5/read_latency* >> agg_read_latency

rm agg_write_latency
cat yeti0/write_latency* >> agg_write_latency
cat yeti5/write_latency* >> agg_write_latency

mv agg_write_latency write
mv agg_read_latency read

python3 plotter.py read write > latency_result_latest.dat