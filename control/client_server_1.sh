source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
echo iwicbV15 | sudo -S rm /tmp/read_latency*
echo iwicbV15 | sudo -S rm /tmp/write_latency*
echo iwicbV15 | sudo -S ./run_client.sh 1 > output_1.dat &
sleep 20;
echo iwicbV15 | sudo -S killall run_client.sh;
echo iwicbV15 | sudo -S killall init;
cat output_1.dat;
cat output_1.dat | grep @TAG@ > clean_1.dat;
