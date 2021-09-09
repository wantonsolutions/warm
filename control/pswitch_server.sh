source /home/ssgrant/.bashrc;
cd /home/ssgrant/warm/rmemc-dpdk;
./run.sh &
sleep 45;
echo iwicbV15 | sudo -S killall rmemc-dpdk