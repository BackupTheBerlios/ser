#/bin/bash
#created by Cesc Santasusana (c) 2005

#input parameter is $1 = name of the certificate (without the last part, after the _....)


openssl ca -config ./sign_defaults.conf -out $1_cert.crt -infiles $1_req.csr
echo " "
echo " "
echo " "
echo "Verifying newly signed certificate against rootCA Certificate:"
echo " "
echo " "
echo " "
openssl verify -CAfile ../rootca.cert.pem $1_cert.crt
echo " "
echo " "

