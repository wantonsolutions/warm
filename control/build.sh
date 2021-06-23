source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
make clean; make -j 30

cd /home/warm/rmem-dpdk;
make clean; make -j 30