#!/bin/bash
#
# $Id: ws.sh,v 1.3 2002/10/15 02:34:46 jiri Exp $
#
#

loop=5
max_speed=30
SIP_DST="sip:pocketpc@192.168.0.33"
alarm=0
SER_FIFO="/tmp/ser_fifo"
SIP_FROM="sip:meteo@iptel.org"
CMD="./wx200d-1.2/wx200 --gust --C"

while [ 0 ]
do
	line=`$CMD`
	echo "wind info: " $line
	speed=`echo $line | cut -d\  -f1 | sed -e 's/\.//' `
	if [ -z "$speed" ] ; then
		echo "Sorry -- unable to determine wind speed"
		echo "Check if weather station server is running"
		exit 1
	fi
	echo "speed = "$speed
	if [ "$speed" -gt "$max_speed" ] && [ $alarm -eq 0 ]
	then
		echo "Very strong wind!! -> sending alert message!"
		cat > $SER_FIFO << EOF
:t_uac_from:null
MESSAGE
$SIP_FROM
$SIP_DST
Content-Type: text/plain
Contact: $SIP_FROM

weather alert: Very strong winds in the area: 
$line
.


EOF
		alarm=1
	fi
	if [ $speed -lt $max_speed ] && [ $alarm -eq 1 ]
	then
		alarm=0
	fi

	
	sleep $loop
done

