#/bin/bash
#created by Cesc Santasusana (c) 2005

#The only input needed is $1 = name of the configuration file (minus the .conf extension)

#automatically create a certificate from the specified configuration file
#(create the certificate, the request to send to the CA, then the
#certificate, then delete the request).

#Create private key as RSA 1024 with Triple DES encryption
openssl genrsa -des3 -out $1_prik.key 1024

echo " "
echo "Private Key created"
echo " "

#To make a request to a CA to create a certificate ....
openssl req -config $1.conf -new -key $1_prik.key -out $1_req.csr -outform PEM
echo " "
echo "Certificate request created successfully, sending to CA for signing"
echo " "

#copy request to CA's certs directory
cp $1_req.csr ../ca/certs

cd ../ca/certs

#execute script to sign the request
echo "Executing request for signature .... "
./sign_request.sh $1

cp $1_cert.crt ../../users/

cd ../../users

echo "Done!!"
echo " "



