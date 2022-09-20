#!/bin/bash


export HOSTS=("yeti5")

#create agg file
rm agg_read_latency
rm agg_write_latency

for host in ${HOSTS[@]}; do
    mkdir -p $host
    rm $host/*
    #read latency
    scp $host:/tmp/read_latency* ./$host
    #write latency
    scp $host:/tmp/write_latency* ./$host

    cat $host/read_latency* >> agg_read_latency
    cat $host/write_latency* >> agg_write_latency

done

mv agg_write_latency write
mv agg_read_latency read

python3 plotter.py read write > latency_result_latest.dat