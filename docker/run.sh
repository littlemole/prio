!/bin/bash

cd /opt/workspace/priocpp
make clean
make -e
make -e test
make -e install

