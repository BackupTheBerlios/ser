#!/bin/sh
#
# $Id: harv_ser.sh,v 1.17 2002/12/10 07:58:01 jiri Exp $
#
# tool for post-processesing captured SIP messages 
#
# call it without parameters to harvest the youngest
# log file or with "all" parameter to harvest all
# 
# you need to capture SIP messages first; you
# may for example run an init.d job such as
# ngrep -t port 5060 2>&1 | rotatelogs /var/log/sip 86400&
# caution: if you do that you best set up a crond daemon
# which deletes the files too -- they become huge
# very quickly
#
# note that the tool has no notion of messages and transactions
# yet; a consuquence of the former is that number of clients
# which do not identify themselves using User-Agent HF is 
# unknown (only lines which include it are processed);
# a consequence is also that relayed messages are
# counted twice (incoming, outgoing), and INVITEs are not
# correlated with BYEs
#


LOGDIR=/var/log

#####################

if [ "$1" = "all" ] ; then
	CURRENT=`ls -t $LOGDIR/sip.*`
else
	CURRENT=`ls -t $LOGDIR/sip.* | head -1`
fi
echo "Log: `ls -l $CURRENT`"
echo "Date: `date`"

#cat $CURRENT | ./ser_harvest.awk 

AWK_PG='
BEGIN {

	IGNORECASE=1;

    rpl100=0; rpl180=0; rpl183=0; rpl1xx=0;
    rpl200=0; rpl202=0; rpl2xx=0;
    rpl300=0; rpl302=0; rpl3xx=0;
    rpl400=0; rpl401=0; rpl403=0; rpl404=0; rpl405=0;
        rpl406=0;rpl407=0;rpl408=0;rpl410=0; rpl415=0;
        rpl476=0;rpl481=0;rpl483=0;rpl486=0;rpl478=0;rpl487=0;
		rpl488=0;rpl489=0;
        rpl4xx=0;
	rpl479=0;
    rpl500=0;rpl501=0;rpl502=0;rpl503=0;rpl5xx=0;
    rpl603=0;rpl6xx=0;

	hint_imgw=0;
	hint_voicemail=0;
	hint_battest=0;
	hint_usrloc=0;
	hint_outbound=0;
	hint_sms=0;
	hint_gw=0;
	hint_off_voicemail=0;


    cancel=0;invite=0;ack=0; info=0;register=0;bye=0;
    options=0;
    message=0; subscribe=0; notify=0;

	ua_snom=0;
	ua_msn=0;
	ua_mitel=0;
	ua_pingtel=0;
	ua_ser=0;
	ua_osip=0;
	ua_linphone=0;
	ua_kphone=0;
	ua_sjphone=0;
	ua_ubiquity=0;
	ua_3com=0;
	ua_ipdialog=0;
	ua_epygi=0;
	ua_jasomi=0;
	ua_cisco=0;
	ua_insipid=0;
	ua_hotsip=0;
	ua_mxsf=0;
	ua_grandstream=0;
	ua_tellme=0;
	ua_pocketsipm=0;
	ua_estara=0;
	ua_vovida=0;
	ua_jsip=0;
	ua_nortel=0;
	ua_polycom=0;
	ua_csco=0;
	ua_leader=0;
	ua_nebula=0;
	ua_yamaha=0;
	ua_magicppc=0;
	ua_scs=0;
	ua_edgeaccess=0;
	ua_xx=0;

	server_cisco=0
	server_ser=0
	server_intertex=0
	server_hotsip=0
	server_3com=0
	server_epygi=0;
	server_leader=0;
	server_ims=0;
	server_csco=0;
	server_sapphire=0;
	server_lucent=0;
	server_snom=0;
	server_edgeaccess=0;
	server_xx=0

}

{ua=0; request=0;reply=0;server=0}

ua==0 && /User-Agent:.*snom/ {
	ua_snom++
	ua=1
}
ua==0 && /User-Agent:.*Windows RTC/ {
	ua_msn++
	ua=1
}
ua==0 && /User-Agent:.*Mitel/ {
	ua_mitel++
	ua=1
}
ua==0 && /User-Agent:.*Pingtel/ {
	ua_pingtel++
	ua=1
}
ua==0 && /User-Agent:.*Sip EXpress/ {
	ua_ser++
	ua=1
}
ua==0 && /User-Agent:.*oSIP-ua/ {
	ua_osip++
	ua=1
}
ua==0 && /User-Agent:.*oSIP\/Linphone/ {
	ua_linphone++
	ua=1
}
ua==0 && /User-Agent:.*3Com/ {
	ua_3com++
	ua=1
}
ua==0 && /User-Agent:.*ipDialog/ {
	ua_ipdialog++
	ua=1
}
ua==0 && /User-Agent:.*UbiquityUserAgent/ {
	ua_ubiquity++
	ua=1
}
ua==0 && /User-Agent:.*EPYGI/ {
	ua_epygi++
	ua=1
}
ua==0 && /User-Agent:.*Jasomi/ {
	ua_jasomi++
	ua=1
}
ua==0 && /User-Agent:.*Cisco/ {
	ua_cisco++
	ua=1
}
ua==0 && /User-Agent:.*Insipid/ {
	ua_insipid++
	ua=1
}
ua==0 && /User-Agent:.*mxsf/ {
	ua_mxsf++
	ua=1
}
ua==0 && /User-Agent:.*Hotsip/ {
	ua_hotsip++
	ua=1
}
ua==0 && /User-Agent:.*GrandStream/ {
	ua_grandstream++
	ua=1
}
ua==0 && /User-Agent:.*Tellme/ {
	ua_tellme++
	ua=1
}
ua==0 && /User-Agent:.*PocketSipM/ {
	ua_pocketsipm++
	ua=1
}
ua==0 && /User-Agent:.*eStara/ {
	ua_estara++
	ua=1
}
ua==0 && /User-Agent:.*vovida/ {
	ua_vovida++
	ua=1
}
ua==0 && /User-Agent:.*jSIP/ {
	ua_jsip++
	ua=1
}
ua==0 && /User-Agent:.*Nortel/ {
	ua_nortel++
	ua=1
}
ua==0 && /User-Agent:.*Polycom/ {
	ua_polycom++
	ua=1
}
ua==0 && /User-Agent:.*CSCO/ {
	ua_csco++
	ua=1
}
ua==0 && /User-Agent:.*LeaderSIP/ {
	ua_leader++
	ua=1
}
ua==0 && /User-Agent:.*Nebula/ {
	ua_nebula++
	ua=1
}
ua==0 && /User-Agent:.*YAMAHA/ {
	ua_yamaha++
	ua=1
}
ua==0 && /User-Agent:.*MagicPPC/ {
	ua_magicppc++
	ua=1
}
ua==0 && /User-Agent:.*SJPhone/ {
	ua_sjphone++
	ua=1
}
ua==0 && /User-Agent:.*KPhone/ {
	ua_kphone++
	ua=1
}
ua==0 && /User-Agent:.*SCS/ {
	ua_scs++
	ua=1
}
ua==0 && /User-Agent:.*EdgEAccEss/ {
	ua_edgeaccess++
	ua=1
}


 { comment="hack to deal with old version of ngrep (breaking in columns)"
		 c="skip lines which words which frequently appeared on broken "
		 c="columns. should not affect non-broken logs"
 }


ua==0 && /(CANCEL|REGISTER|SUBSCRIBE|ACK|BYE|INVITE|REFER|OPTIONS|NOTIFY|sip-cc).*User-Agent:/ {
	ua=1
}

ua==0 && /User-Agent:/ {
	ua_xx++
	print
}


server==0 && /Server:.*Cisco/ {
	server_cisco++
	server=1
}
server==0 && /Server:.*Sip EXpress/ {
	server_ser++
	server=1
}
server==0 && /Server:.*Intertex/ {
	server_intertex++
	server=1
}
server==0 && /Server:.*HotSip/ {
	server_hotsip++
	server=1
}
server==0 && /Server:.*3Com/ {
	server_3com++
	server=1
}
server==0 && /Server:.*EPYGI/ {
	server_epygi++
	server=1
}
server==0 && /Server:.*LeaderSIP_UA/ {
	server_leader++
	server=1
}
server==0 && /Server:.*IMS/ {
	server_ims++
	server=1
}
server==0 && /Server:.*CSCO/ {
	server_csco++
	server=1
}
server==0 && /Server:.*sapphire/ {
	server_sapphire++
	server=1
}
server==0 && /Server:.*snom/ {
	server_snom++
	server=1
}
server==0 && /Server:.*Lucent/ {
	server_lucent++
	server=1
}
server==0 && /Server:.*EdgEAccEss/ {
	server_edgeaccess++
	server=1
}
server==0 && /Server:/ {
	server_xx++
	print
}



/P-hint: IMGW/ {
	hint_imgw++
}
/P-hint: VOICEMAIL/ {
	hint_voicemail++
}
/P-hint: BATTEST/ {
	hint_battest++
}
/P-hint: USRLOC/ {
	hint_usrloc++
}
/P-hint: OUTBOUND/ {
	hint_outbound++
}
/P-hint: SMS/ {
	hint_sms++
}
/P-hint: GATEWAY/ {
	hint_gw++
}
/P-hint: OFFLINE-VOICEMAIL/ {
	hint_off_voicemail++
}



/SIP\/2\.0 [0-9][0-9][0-9]/ {
	reply=1
}

/[A-Z]* sip.* SIP\/2\.0/ {
	request=1
}

reply==0 && request=0 {
	comment="optimization--skip now"
	next
}


/SIP\/2\.0 100/ {
    rpl100++
    next
}
/SIP\/2\.0 180/ {
    rpl180++
    next
}
/SIP\/2\.0 183/ {
    rpl183++
    next
}
/SIP\/2\.0 1[0-9][0-9]/ {
    print
    rpl1xx=0
    next
}


/SIP\/2\.0 200/ {
    rpl200++
    next
}
/SIP\/2\.0 202/ {
    rpl202++
    next
}
/SIP\/2\.0 2[0-9][0-9]/ {
    print
    rpl2xx++
    next
}

/SIP\/2\.0 300/ {
    rpl300++
    next
}
/SIP\/2\.0 302/ {
    rpl302++
    next
}
/SIP\/2\.0 3[0-9][0-9]/ {
    print
    rpl3xx++
    next
}

/SIP\/2\.0 400/ {
    rpl400++
    next
}
/SIP\/2\.0 401/ {
    rpl401++
    next
}
/SIP\/2\.0 403/ {
    rpl403++
    next
}
/SIP\/2\.0 404/ {
    rpl404++
    next
}
/SIP\/2\.0 405/ {
    rpl405++
    next
}
/SIP\/2\.0 406/ {
    rpl406++
    next
}
/SIP\/2\.0 407/ {
    rpl407++
    next
}
/SIP\/2\.0 408/ {
    rpl408++
    next
}
/SIP\/2\.0 410/ {
    rpl410++
    next
}
/SIP\/2\.0 415/ {
    rpl415++
    next
}
/SIP\/2\.0 478/ {
    rpl478++
    next
}
/SIP\/2\.0 476/ {
    rpl476++
    next
}
/SIP\/2\.0 481/ {
    rpl481++
    next
}
/SIP\/2\.0 483/ {
    rpl483++
    next
}
/SIP\/2\.0 486/ {
    rpl486++
    next
}
/SIP\/2\.0 487/ {
    rpl487++
    next
}
/SIP\/2\.0 488/ {
    rpl488++
    next
}
/SIP\/2\.0 489/ {
    rpl489++
    next
}
/SIP\/2\.0 479/ {
    rpl479++
    next
}
/SIP\/2\.0 4[0-9][0-9]/ {
    print
    rpl4xx++
    next
}


/SIP\/2\.0 500/ {
    rpl500++
    next
}
/SIP\/2\.0 501/ {
    rpl501++
    next
}
/SIP\/2\.0 502/ {
    rpl502++
    next
}
/SIP\/2\.0 503/ {
    rpl503++
    next
}
/SIP\/2\.0 5[0-9][0-9]/ {
    print
    rpl5xx++
    next
}

/SIP\/2\.0 603/{
	rpl603++
	next
}
/SIP\/2\.0 6[0=9][0-9]/ {
    print
    rpl6xx++
    next
}


/CANCEL sip/ {
    cancel++
    next
}
/INVITE sip/ {
    invite++
    next
}
/ACK sip/ {
    ack++
    next
}
/BYE sip/ {
    bye++
    next
}
/OPTIONS sip/ {
    options++
    next
}
/INFO sip/ {
    info++
    next
}
/MESSAGE sip/ {
    message++
    next
}
/SUBSCRIBE sip/ {
    subscribe++
    next
}
/NOTIFY sip/ {
    notify++
    next
}
/REGISTER sip/ {
	register++
	next
}

END {
	print "## Reply Codes"
    print "100 (trying): " rpl100 
	print "180 (ringing): " rpl180 
	print "183: (early media)" rpl183 
	print "1xx: " rpl1xx
    print "200 (ok): " rpl200 
	print "202 (accepted): " rpl202 
	print "2xx: " rpl2xx
    print "300 (Multiple Choices): " rpl300 
	print "302 (Moved Temporarily): " rpl302 
	print "3xx: " rpl3xx
    print "400 (Bad Request): " rpl400 
	print "401 (Unauthorized): " rpl401 
	print "403 (Forbidden): " rpl403 
	print "404 (Not Found):" rpl404 
	print "405 (Method not allowed): " rpl405 
	print "406 (Not Acceptable): " rpl406
	print "407 (Proxy Authentication Required):" rpl407 
	print "408 (Request Timeout): " rpl408  
	print "410 (Gone): " rpl410
	print "415 (Unsupported Media): " rpl415
	print "476 (no recursive registrations): " rpl476 
	print "478 (Unresolveable): " rpl478 
	print "479 (private IP): " rpl479 
	print "481 (Call/Transaction does not exist): " rpl481 
	print "483 (Too Many Hops): " rpl483 
	print "486 (Busy Here): " rpl486 
	print "487 (Request Terminated): " rpl487
	print "488 (Not Acceptable): " rpl488
	print "489 (Bad Event): " rpl489
	print "4xx: " rpl4xx  
    print "500 (Server Internal Error): " rpl500 
	print "501 (Not Implemented): " rpl501 
	print "502 (Bad Gateway): " rpl502 
	print "503 (Service Unavailabl): " rpl503 
	print "5xx: " rpl5xx
    print "603 (Decline): " rpl603 
	print "6xx: " rpl6xx

	print "## Request Methods"
    print "INVITE: " invite " CANCEL: " cancel " ACK: " ack
    print "REGISTER: " register " BYE: " bye " OPTIONS: " options " INFO: " info
    print "MESSAGE: " message " SUBSCRIBE: " subscribe " NOTIFY: " notify

	print "## Outbound Routes"
	print "To imgw: " hint_imgw " To voicemail: " hint_voicemail
	print "To bat: " hint_battest " To UsrLoc: " hint_usrloc
	print "Outbound: " hint_outbound " To SMS: " hint_sms
	print "To PSTN: " hint_gw " To: VM on off-line" hint_off_voicemail

	print "## User Agents"
	print "Snom: " ua_snom " MSN: " ua_msn " Mitel: " ua_mitel
	print "Pingtel: " ua_pingtel " SER: " ua_ser " osip: " ua_osip
	print "linphone: " ua_linphone " ubiquity: " ua_ubiquity
	print "3com: " ua_3com " IPDialog: " ua_ipdialog " Epygi: " ua_epygi
	print "Jasomi: " ua_jasomi " Cisco: " ua_cisco " insipid: " ua_insipid
	print "Hotsip: " ua_hotsip " mxsf: " ua_mxsf " GrandStream: " ua_grandstream
	print "Tellme: " ua_tellme " PocketSipM: " ua_pocketsipm 
	print "eStara: " ua_estara " Vovida: " ua_vovida 
	print "jSIP: " ua_jsip " Nortel: " ua_nortel " Polycom: " ua_polycom
	print "Leader: " ua_leader " csco: " ua_csco " Nebula: " ua_nebula
	print "MagicPPC: " ua_magicppc " SCS: " ua_scs 
	print "SJPhone: " ua_sjphone " KPhone: " ua_kphone
	print "Yamaha: " ua_yamaha 
	print "EdgeAccess: " ua_edgeaccess
	print "UFO: " ua_xx

	print "## Servers"
	print "Cisco: " server_cisco " ser: " server_ser 
	print "Intertex: " server_intertex " Hotsip: " server_hotsip
	print "3com: " server_3com " EPYGI: " server_epygi " Leader: " server_leader
	print "IMS (Nortel): " server_ims " CSCO: " server_csco
	print "sapphire: " server_sapphire
	print "snom: " server_snom
	print "lucent: " server_lucent
	print "edgeAccess: " server_edgeaccess
	print "UFO: " server_xx
}
'


cat $CURRENT | awk "$AWK_PG"
