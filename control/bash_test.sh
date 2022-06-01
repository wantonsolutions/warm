#!/bin/bash

hosts=("yeti5" "yeti0")

let "i=0"
for host in ${hosts[@]}; do
    echo $host $i
    let "i=i+1"
done