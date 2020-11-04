source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
echo iwicbV15 | sudo -S ./run_client.sh 1 > output.dat &
sleep 30;
echo iwicbV15 | sudo -S killall run_client.sh;
echo iwicbV15 | sudo -S killall init;
cat output.dat
cat output.dat | grep @TAG@ > clean.dat
