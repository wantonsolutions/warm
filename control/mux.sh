#!/bin/bash

byobu new-session -d -s $USER

byobu rename-window -t $USER:0 'manual-control-ssh'
byobu send-keys "ssh b09-27" C-m
byobu send-keys "clear" C-m
byobu send-keys "./go.sh" C-m


#spawn the middle box on the right
byobu split-window -h
byobu send-keys "ssh yak2" C-m
byobu send-keys "clear" C-m
byobu send-keys "./run.sh" C-m

sleep 0.5

byobu select-pane -t 0
byobu split-window -v
byobu send-keys "ssh yak1" C-m
byobu send-keys "clear" C-m
byobu send-keys "./go.sh" C-m


byobu select-pane -t 0
byobu split-window -v
byobu send-keys "ssh yak1" C-m
byobu send-keys "clear" C-m
byobu send-keys "./go.sh" C-m

byobu select-pane -t 2
byobu split-window -v
byobu send-keys "ssh yak0" C-m
byobu send-keys "clear" C-m
byobu send-keys "./go.sh" C-m

#start a second named window
byobu new-window -t $USER:1 -n control-local
byobu select-window -t $USER:1
byobu send-keys "cd ~/warm/control" C-m



# Attach to the session you just created
# (flip between windows with alt -left and right)
byobu attach-session -t $USER
