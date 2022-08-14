source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
echo iwicbV15 | sudo -S rm /tmp/read_latency*
echo iwicbV15 | sudo -S rm /tmp/write_latency*
echo iwicbV15 | sudo -S ./run_client.sh 6 > output_6.dat &
sleep 25;
echo iwicbV15 | sudo -S killall run_client.sh;
echo iwicbV15 | sudo -S killall init;
cat output_6.dat;
cat output_6.dat | grep @TAG@ > clean_6.dat;
