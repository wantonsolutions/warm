source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
echo iwicbV15 | sudo -S memcached -u root -I 128m -m 2048 &
sleep %%SLEEP%%;
echo iwicbV15 | sudo -S killall memcached;
echo iwicbV15 | sudo -S killall init;