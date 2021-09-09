source /home/ssgrant/.bashrc;
cd /home/ssgrant/pDPM/clover;
a=`date; pwd`;
echo $a >> memory_output.dat;
echo "double test" >> memory_output.dat;
echo iwicbV15 | sudo -S ./run_memory.sh 3 > memory_output.dat &
echo $a >> memory_output.dat;

sleep 45;
echo iwicbV15 | sudo -S killall run_memory.sh;
echo iwicbV15 | sudo -S killall init;