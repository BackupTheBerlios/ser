#!/bin/bash

loop=5
max_speed=30
dst_sip_addr="sip:gh@iptel.org"
alarm=0

while [ 0 ]
do
	line=`wx200 --gust --C fesarius.fokus.gmd.de`
	echo "wind info: " $line
	speed=`echo $line | cut -d\  -f1 | sed -e 's/\.//' `
	echo "speed = "$speed
	if [ $speed -gt $max_speed ] && [ $alarm -eq 0 ]
	then
		echo "Very strong wind!! -> sending alert message!"
		cmd=":t_uac_from:bash\nMESSAGE\nsip:meteo@iptel.org\n"
		cmd=$cmd$dst_sip_addr"\nContent-Type: text/plain\n"
		cmd=$cmd"Contact: iptel.org:9\n"
		cmd=$cmd"\nIptel.org weather center: WARNING!!\n"
		cmd=$cmd"Very strong winds in the area: "$line".\n.\n\n"
		echo -e $cmd > /tmp/fifo
		alarm=1
	fi
	if [ $speed -lt $max_speed ] && [ $alarm -eq 1 ]
	then
		alarm=0
	fi

	
	sleep $loop
done




