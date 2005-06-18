#/bin/bash
#created by Cesc Santasusana (c) 2005

#use with care ... it resets the database and the serial of the CA ...
rm -f index*
touch index.txt
rm serial*
echo '01' > serial

cd certs
rm -f *pem
rm -f *csr
rm -f *crt
