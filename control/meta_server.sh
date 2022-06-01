source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
echo iwicbV15 | sudo -S ./run_ms.sh 0 &
sleep 10;
echo iwicbV15 | sudo -S killall run_ms.sh;
echo iwicbV15 | sudo -S killall init;

