#!/bin/bash

byobu new-session -d -s $USER

FIRST_PANE='clover'
#FIRST_PANE='rdma_bench'

byobu rename-window -t $USER:0 'manual-control-ssh'

if [ "$FIRST_PANE" == 'clover' ]; then
	byobu send-keys "ssh b09-27" C-m
	byobu send-keys "clear" C-m
	byobu send-keys "./go.sh" C-m

	#spawn the middle box on the right
	byobu split-window -h
	byobu send-keys "ssh yak2" C-m
	byobu send-keys "clear" C-m
	byobu send-keys "./run.sh" C-m

	#Launch Client 1
	byobu select-pane -t 0
	byobu split-window -v
	byobu send-keys "ssh yeti5" C-m
	byobu send-keys "clear" C-m
	byobu send-keys "./go.sh" C-m

	#currently commented for a single client
	#Launch Client 2
	# byobu split-window -v
	# byobu send-keys "ssh yeti0" C-m
	# byobu send-keys "clear" C-m
	# byobu send-keys "./go.sh" C-m

	#Launch Client 3
	# byobu split-window -v
	# byobu send-keys "ssh yeti1" C-m
	# byobu send-keys "clear" C-m
	# byobu send-keys "./go.sh" C-m

	#lauch memory server
	byobu select-pane -t 4
	byobu split-window -v
	byobu send-keys "ssh yak1" C-m
	byobu send-keys "./go.sh" C-m

	#launch meta data server
	byobu split-window -v
	byobu send-keys "ssh yak0" C-m
	byobu send-keys "clear" C-m
	byobu send-keys "./go.sh" C-m

elif [ "$FIRST_PANE" == 'rdma_bench' ]; then

	byobu send-keys "ssh yak1" C-m
	byobu send-keys "cd ~/warm/spark-nic/rdma" C-m
	byobu send-keys "echo 'im the server' " C-m
	byobu send-keys "clear" C-m

	byobu split-window -v
	byobu send-keys "ssh yak0" C-m
	byobu send-keys "cd ~/warm/spark-nic/rdma" C-m
	byobu send-keys "clear" C-m
fi


#start a second named window
byobu new-window -t $USER:1 -n control-local
byobu select-window -t $USER:1
byobu send-keys "cd ~/warm/control" C-m

#start a second named window
byobu new-window -t $USER:2 -n result-moniter
byobu select-window -t $USER:2
byobu send-keys "cd ~/warm/control" C-m
byobu send-keys "tail -f results.dat" C-m

#start a second named window
byobu new-window -t $USER:3 -n presentation
byobu select-window -t $USER:2
byobu send-keys "cd ~/warm/presentation" C-m



# Attach to the session you just created
# (flip between windows with alt -left and right)
byobu attach-session -t $USER
