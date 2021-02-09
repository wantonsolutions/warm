source /home/ssgrant/.bashrc;
cd /home/ssgrant/warm/rmemc-dpdk;
./run.sh &
sleep 30;
echo iwicbV15 | sudo -S killall rmemc-dpdk