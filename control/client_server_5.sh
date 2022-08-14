source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
echo iwicbV15 | sudo -S rm /tmp/read_latency*
echo iwicbV15 | sudo -S rm /tmp/write_latency*
echo iwicbV15 | sudo -S ./run_client.sh 5 > output_5.dat &
sleep 25;
echo iwicbV15 | sudo -S killall run_client.sh;
echo iwicbV15 | sudo -S killall init;
cat output_5.dat;
cat output_5.dat | grep @TAG@ > clean_5.dat;
