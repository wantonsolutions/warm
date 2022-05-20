#!/bin/bash

rm clean_1.dat
rm clean_2.dat

scp yeti5:/home/ssgrant/pDPM/clover/clean_1.dat clean_1.dat
scp yeti0:/home/ssgrant/pDPM/clover/clean_2.dat clean_2.dat

res1=`tail -1 clean_1.dat`
res2=`tail -1 clean_2.dat`

echo $res1
echo $res2

if [ -z "$res1" ] || [ -z "$res2" ]; then
    echo "Experiment Failed"
else
    echo "Experiment Complete"
    echo $res1 >> results.dat
    echo $res2 >> results.dat

    echo $res1 >> latest.dat
    echo $res2 >> latest.dat
fi


