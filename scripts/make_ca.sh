#!/bin/sh

set -e

#
# https://jamielinux.com/docs/openssl-certificate-authority/index.html
#


rm -rf db
mkdir -p db/newcerts
touch db/index.txt
echo 1000 > db/serial


OPENSSL_CONF=db/openssl.conf #{{{1
cat >$OPENSSL_CONF <<OPENSSL_CONF
[ ca ]
default_ca = CA_default

[ CA_default ]
dir = db
new_certs_dir = \$dir/newcerts
database = \$dir/index.txt
serial = \$dir/serial
RANDFILE = \$dir/.rnd
default_md = sha256
name_opt = ca_default
cert_opt = ca_default
preserve = no
policy = policy_match

[ policy_match ]
C = match
ST = match
O = match
OU = optional
CN = supplied
emailAddress = optional

[ policy_anything ]
C = optional
ST = match
O = optional
OU = optional
CN = supplied
emailAddress = optional

[ req ]
default_bits = 2048
default_md = sha256
distinguished_name = req_distinguished_name
string_mask = utf8only
x509_extensions = v3_ca

[ req_distinguished_name ]
C = Country Name
ST = State
O = Organization
OU = Organizational Unit
CN = Common Name
emailAddress = Email Address

[ v3_ca ]
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
basicConstraints = critical, CA:true
keyUsage = critical, digitalSignature, keyCertSign, cRLSign

[ v3_intermediate_ca ]
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid:always,issuer
basicConstraints = critical, CA:true, pathlen:0
keyUsage = critical, digitalSignature, keyCertSign, cRLSign

[ server_cert ]
basicConstraints = CA:FALSE
nsCertType = server
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer:always
keyUsage = critical, digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth

[ client_cert ]
basicConstraints = CA:FALSE
nsCertType = client, email
subjectKeyIdentifier = hash
authorityKeyIdentifier = keyid,issuer
keyUsage = critical, nonRepudiation, digitalSignature, keyEncipherment
extendedKeyUsage = clientAuth, emailProtection
OPENSSL_CONF

echo '*** Root key ***' #{{{1
rm -f ca.key.pem
openssl genrsa -aes256 \
  -out ca.key.pem \
  -passout pass:CaPassword \
  4096
chmod 400 ca.key.pem

echo '*** Root certificate ***' #{{{1
rm -f ca.pem
openssl req -new -x509 -batch -config $OPENSSL_CONF \
  -out ca.pem \
  -subj '/C=EE/ST=Estonia/O=SAL/OU=SAL CA/CN=SAL Root CA' \
  -days 7300 \
  -sha256 \
  -extensions v3_ca \
  -passin pass:CaPassword \
  -key ca.key.pem

echo '*** Intermediate key ***' #{{{1
rm -f intermediate.key.pem
openssl genrsa -aes256 \
  -out intermediate.key.pem \
  -passout pass:IntermediatePassword \
  4096
chmod 400 intermediate.key.pem

echo '*** Intermediate certificate request ***' #{{{1
rm -f intermediate.csr.pem
openssl req -new -sha256 -batch -config $OPENSSL_CONF \
  -out intermediate.csr.pem \
  -subj '/C=EE/ST=Estonia/O=SAL/OU=SAL CA/CN=SAL Intermediate CA' \
  -passin pass:IntermediatePassword \
  -key intermediate.key.pem

echo '*** Intermediate certificate ***' #{{{1
rm -f intermediate.pem
openssl ca -batch -config $OPENSSL_CONF \
  -in intermediate.csr.pem \
  -out intermediate.pem \
  -extensions v3_intermediate_ca \
  -days 7300 \
  -notext \
  -cert ca.pem \
  -passin pass:CaPassword \
  -keyfile ca.key.pem
chmod 444 intermediate.pem
rm -f intermediate.csr.pem

echo '*** Server key ***' #{{{1
rm -f server.key.pem
# omit -aes256 for password-less key
openssl genrsa -aes256 \
  -out server.key.pem \
  -passout pass:ServerPassword \
  2048
chmod 400 server.key.pem

echo '*** Server certificate request ***' #{{{1
rm -f server.csr.pem
openssl req -new -sha256 -batch -config $OPENSSL_CONF \
  -out server.csr.pem \
  -subj '/C=EE/ST=Estonia/O=SAL/OU=SAL Test/CN=test.sal.ee' \
  -passin pass:ServerPassword \
  -key server.key.pem

echo '*** Server certificate ***' #{{{1
rm -f server.pem
openssl ca -batch -config $OPENSSL_CONF \
  -in server.csr.pem \
  -out server.pem \
  -extensions server_cert \
  -days 7270 \
  -notext \
  -cert intermediate.pem \
  -passin pass:IntermediatePassword \
  -keyfile intermediate.key.pem
chmod 444 server.pem
rm -f server.csr.pem
echo '*** Client key ***' #{{{1
rm -f client.key.pem
# omit -aes256 for password-less key
openssl genrsa -aes256 \
  -out client.key.pem \
  -passout pass:ClientPassword \
  2048
chmod 400 client.key.pem

echo '*** Client certificate request ***' #{{{1
rm -f client.csr.pem
openssl req -new -sha256 -batch -config $OPENSSL_CONF \
  -out client.csr.pem \
  -subj '/C=EE/ST=Estonia/O=SAL/OU=SAL Test/CN=test.sal.ee/emailAddress=test@sal.ee' \
  -passin pass:ClientPassword \
  -key client.key.pem

echo '*** Client certificate ***' #{{{1
rm -f client.pem
openssl ca -batch -config $OPENSSL_CONF \
  -in client.csr.pem \
  -out client.pem \
  -extensions client_cert \
  -days 7270 \
  -notext \
  -cert intermediate.pem \
  -passin pass:IntermediatePassword \
  -keyfile intermediate.key.pem
chmod 444 client.pem
rm -f client.csr.pem
