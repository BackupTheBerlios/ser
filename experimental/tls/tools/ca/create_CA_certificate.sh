#/bin/bash
#created by Cesc Santasusana (c) 2005

#input parameter is the configuration file name (without the .conf extension)
#The self-signed cert is valid for 10 years ...

echo " "
openssl req -config $1.conf -x509 -days 3652 -newkey rsa:1024 -out $1.cert.pem -outform PEM
echo " "
echo " "

