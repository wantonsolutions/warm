# Short Description

A basic run consisting of two client machines. This basic test is to demonstrate
that the code works with two clients up to 32 threads each. The server is
running on a single thread the workload is YCSB-A 1024 keys.  


## Results

The program scales up to 32 threads. At which point it starts to tank due to backup on the switch.
