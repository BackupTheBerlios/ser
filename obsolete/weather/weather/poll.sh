#!/bin/sh
#
# $Id: poll.sh,v 1.2 2002/09/05 21:40:16 jku Exp $
#
# demo app -- it polls local weatherstation using the
# wx2000 utility http://wx2000.sourceforge.net/ 
# (to get it running, you need to compile it with
# an apropriate /dev/tty which is read-write-able)
# if a value is returns, it is send via IM

SIP_TO="gh@iptel.org"
# -x reads all accumulated reports and removes them
#    from weather station
# -g reads the recent value and leaves it in the station
#OPTIONS="-g"
OPTIONS="-x"
WX="/home/jiri/tmp/weather/wx2000-0.3/wx2000 $OPTIONS"
SER_FIFO=/tmp/ser_fifo
REPLY_FIFO=hh

#####################################################

# make sure ser has a place to write to about replies
# so that it does not complains so much

touch /tmp/$REPLY_FIFO



MSG=`$WX | grep -v "^NO" | awk '
{
	print "blocknr " $1
	print "time " $2
	print "date " $3
	print "temp1 " $4
	print "humidity1 " $5
	print "new1 " $6
	print "temp_i " $7
	print "hum_i " $8
	print "pressure " $9
	print "newin " $10
	print "wind_speed " $11
	print "rain " $12
	print "new_rain " $13
}' | egrep "temp1|wind_speed"`

if [ -z "$MSG" ] ; then
	# do nothing
	echo nothing > /dev/null
else
	cat > $SER_FIFO <<EOF
:t_uac_from:$REPLY_FIFO
MESSAGE

sip:$SIP_TO
P-originator: weather-service
Contact: <sip:devnull@127.0.0.1:9>
Content-Type: text/plain; charset=UTF-8

Current Weather: `date`
$MSG

.

EOF
fi


