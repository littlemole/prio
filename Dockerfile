# This is a comment
FROM littlemole/devenv_clangpp_make
MAINTAINER me <little.mole@oha7.org>

ARG CXX=g++
ENV CXX=${CXX}

ARG BACKEND=
ENV BACKEND=${BACKEND}

ARG BUILDCHAIN=make
ENV BUILDCHAIN=${BUILDCHAIN}

RUN /usr/local/bin/install.sh repro 

RUN mkdir -p /usr/local/src/priocpp
ADD . /usr/local/src/priocpp

RUN SKIPTEST=true /usr/local/bin/build.sh priocpp 

