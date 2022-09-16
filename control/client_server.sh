source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
echo iwicbV15 | sudo -S rm /tmp/read_latency*
echo iwicbV15 | sudo -S rm /tmp/write_latency*
echo iwicbV15 | sudo -S ./run_client.sh %%ID%% %%NR_CN%% > output_%%ID%%.dat 2> client_%%ID%%.log &
sleep %%SLEEP%%;
echo iwicbV15 | sudo -S killall run_client.sh;
echo iwicbV15 | sudo -S killall init;
cat output_%%ID%%.dat;
cat output_%%ID%%.dat | grep @TAG@ > clean_%%ID%%.dat;