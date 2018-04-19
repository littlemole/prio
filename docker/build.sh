#!/bin/bash
set -e

echo "**********************************"
echo "building $1 for $BACKEND with" 
echo "$CXX using $BUILDCHAIN"
echo "**********************************"

function cmake_build {

    MODE="$1"
    
    mkdir -p $MODE
    cd $MODE

    if [ "$BACKEND" == "libevent" ]
    then
        cmake .. -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_BUILD_TYPE=$MODE
    else
        cmake .. -DCMAKE_CXX_COMPILER=$CXX -DWITH_LIBEVENT=Off -DCMAKE_BUILD_TYPE=$MODE
    fi
     
    make
    
    if [ "$SKIPTEST" == "true" ]
    then
    	echo "skipping tests for $1 ..."
    else
        ctest
    fi
    
    make install  
    cd ..
}

cd /usr/local/src/$1

if [ "$BUILDCHAIN" == "make" ] 
then
    if [ "$SKIPTEST" == "true" ]
    then
    	echo "skipping tests for $1 ..."
    else
	    make clean
	    make -e test
    fi
    make clean
    make -e install
else

    cmake_build "Debug"
    cmake_build "Release"
fi

