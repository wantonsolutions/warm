#!/bin/bash
i=1
clientSource=`cat client_server.sh`
clientSource=$(sed "s|%%ID%%|$i|g" <<< $clientSource)
echo $clientSource