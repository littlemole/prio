# prio
promising reactive io

# install

## native linux
```bash
    git clone https://github.com/littlemole/prio.git
    make 
    make test
    make install
```
self documention makefile, run *make help* for details

## cmake support

building with cmake is supported assuming build happens in a dedicated build directory different from the root src directory.

### default build CXX=g++ and BACKEND=libevent

```bash
    git clone https://github.com/littlemole/prio.git
    cd prio
    mkdir build
    cd build
    cmake ..
```

### build with different BACKEND=boost_asio

```bash
    git clone https://github.com/littlemole/prio.git
    cd prio
    mkdir build
    cd build
    cmake .. -DWITH_LIBEVENT=Off
```
### build for CXX=clang++-5.0 and default BACKEND

```bash
    git clone https://github.com/littlemole/prio.git
    cd prio
    mkdir build
    cd build
    cmake .. -DCMAKE_CXX_COMPILER="clang++-5.0" -DCMAKE_CXX_FLAGS="-std=c++14 -stdlib=libc++"
```
note this requires not only clang++-5.0 toolchain installed,
but also dependent cpp libraries built against clang++ and libc++.
for this project that means gtest and cryptoneat libraries
have to be built using clang++ toolchain.

### build for CXX=clang++-5.0 and BACKEND=boost_asio

just add both options from above to calling cmake.


## docker
