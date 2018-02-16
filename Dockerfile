# This is a comment
FROM ubuntu:16.04
MAINTAINER me <little.mole@oha7.org>

# std dependencies
RUN DEBIAN_FRONTEND=noninteractive apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get upgrade -y
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
  git sudo joe wget netcat psmisc \
  build-essential g++ cmake pkg-config valgrind \
  libgtest-dev  openssl libssl-dev libevent-dev uuid-dev \
  nghttp2 libnghttp2-dev


# clang++-5.0 dependencies
RUN echo "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-5.0 main" >> /etc/apt/sources.list
RUN wget -O - http://apt.llvm.org/llvm-snapshot.gpg.key|apt-key add -

RUN DEBIAN_FRONTEND=noninteractive apt-get update

RUN DEBIAN_FRONTEND=noninteractive apt-get install -y \
  clang-5.0 lldb-5.0 lld-5.0 libc++-dev libc++abi-dev \
  libboost-dev libboost-system-dev 


# hack for gtest with clang++-5.0
RUN ln -s /usr/include/c++/v1/cxxabi.h /usr/include/c++/v1/__cxxabi.h
RUN ln -s /usr/include/libcxxabi/__cxxabi_config.h /usr/include/c++/v1/__cxxabi_config.h

ARG CXX=g++
ENV CXX=${CXX}

ARG BACKEND=
ENV BACKEND=${BACKEND}

# compile gtest with given compiler
RUN  cd /usr/src/gtest && \
  if [ "$CXX" = "g++" ] ; then \
  cmake .; \
  else \
  cmake -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_CXX_FLAGS="-std=c++14 -stdlib=libc++" . ; \
  fi && \
  make && \
  ln -s /usr/src/gtest/libgtest.a /usr/lib/libgtest.a

RUN cd /usr/local/src && \
  git clone https://github.com/littlemole/repro.git && \
  cd repro && \
  make clean && \
  make CXX=${CXX} && \
  make CXX=${CXX} test && \
  make CXX=${CXX} install 

RUN cd /usr/local/src && \
  git clone https://github.com/littlemole/cryptoneat.git && \
  cd cryptoneat && \
  make clean && \
  make CXX=${CXX} && \
  make CXX=${CXX} test && \
  make CXX=${CXX} install 

RUN mkdir -p /usr/local/src/priocpp
ADD . /usr/local/src/priocpp

RUN /usr/local/src/priocpp/docker/run.sh

