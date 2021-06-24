source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
make clean; make -j 30

cd /home/ssgrant/warm/rmem-dpdk;
make clean; make -j 30
