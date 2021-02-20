source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
echo iwicbV15 | sudo -S ./run_memory.sh 3 &
sleep 30;
echo iwicbV15 | sudo -S killall run_memory.sh;
echo iwicbV15 | sudo -S killall init;