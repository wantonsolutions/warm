#!/bin/bash

byobu new-session -d -s $USER

byobu rename-window -t $USER:0 'memcached'
byobu send-keys "ssh b09-27" C-m
byobu send-keys "clear" C-m
byobu send-keys "./go.sh" C-m

#spawn yeti 5 on the right when we have two clients
#byobu split-window -h
#byobu send-keys "ssh yeti5" C-m
#byobu send-keys "clear" C-m
#byobu send-keys "./go.sh" C-m

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

#byobu select-pane -t 3
#byobu send-keys "ssh yak1" C-m
#byobu send-keys "clear" C-m


#byobu select-pane -t 2
#byobu split-window -v

#byobu send-keys "ssh yak1" C-m
#byobu send-keys "clear" C-m

#byobu split-window -v
#byobu select-pane -t 4

#byobu send-keys "ssh yak0" C-m
#byobu send-keys "clear" C-m











# Attach to the session you just created
# (flip between windows with alt -left and right)
byobu attach-session -t $USER
