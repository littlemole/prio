#!/bin/bash

build/ssl-echo$1.bin &

sleep 1

openssl s_client -connect localhost:9876 <<EOF
huhu
helo
quit
EOF


killall -INT ssl-echo$1.bin
echo "success"