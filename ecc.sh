#!/bin/sh
openssl ecparam -name prime256v1 -out ecc.pem
openssl req -x509 -sha256 -nodes -days 32767 -newkey ec:ecc.pem -keyout ecc.key -out ecc.crt
