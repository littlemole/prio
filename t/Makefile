DEBUG = -g -DMOL_PROMISE_DEBUG
release : override DEBUG = -O3

LIBEVENT=$(shell pkg-config libevent_pthreads --libs)
PKG_INC=$(shell pkg-config cryptoneat --cflags)
PKG_LIB=$(shell pkg-config cryptoneat --libs)

# if not g++ we assume clang++
DEFAULT_LIBS = $(TS) -stdlib=libc++ -fcoroutines-ts  -lc++abi -std=c++17
DEFAULT_OPTS = $(TS) -std=c++17 -stdlib=libc++ -fcoroutines-ts -D_RESUMABLE_FUNCTIONS_SUPPORTED 

ifeq ($(CXX),g++)
DEFAULT_OPTS = -std=c++17
DEFAULT_LIBS = 
endif

#backend switch libevent/boost_asio
PROMISE_IMPL = -DPROMISE_USE_LIBEVENT
PROMISE_3DPARTY = $(LIBEVENT) $(DEFAULT_LIBS)

ifeq ($(BACKEND),boost_asio)
        PROMISE_IMPL = -DPROMISE_USE_BOOST_ASIO
        PROMISE_3DPARTY = -L/opt/boost/lib -lboost_system $(DEFAULT_LIBS)
endif

#final cpp options
OPTIONS = -g -fpic -Wno-write-strings -pthread -D_REENTRANT $(DEFAULT_OPTS) $(PKG_INC)
CPPFLAGS = -Wall -I/opt/boost/include -I../include $(DEBUG) $(OPTIONS) $(PROMISE_IMPL)

# target
LIB = ../src/build/libpriocppd.a
release : override LIB = ../src/build/libpriocpp.a

TEST_SRC   = .
TEST_BUILD = ./build
TEST_LIBS  = $(LIB) /usr/lib/libgtest.a -lpthread  $(PROMISE_3DPARTY) $(PKG_LIB)


# Objects for the test executable
TEST_SRCFILESABS = $(shell ls $(TEST_SRC)/*.cpp)
TEST_SRCFILES =  $(notdir $(TEST_SRCFILESABS))
TEST_OBJFILES = $(TEST_SRCFILES:%.cpp=$(TEST_BUILD)/%.o)
TEST_BINS = $(TEST_SRCFILES:%.cpp=$(TEST_BUILD)/%.bin)

#################################################
# rule to compile all (default rule)
#################################################

all: $(TEST_BINS)

#################################################
# rules to compile .o files from .cpp source
#################################################

$(TEST_BUILD)/%.o: $(TEST_SRC)/%.cpp 
	-mkdir -p $(TEST_BUILD)
	$(CXX) -c $^ -o $@ $(CPPFLAGS) 


#################################################
# rules to compile the artifacts from .o files
#################################################

$(TEST_BUILD)/%.bin: $(TEST_BUILD)/%.o 
	$(CXX) $(LIB_OBJFILES) $^ $(TEST_LIBS) -o $@ 
			
#################################################
# make clean
#################################################

clean:
	-find -name "*.o" -exec rm {} \;
	-find -name "*.bin" -exec rm {} \;
	-find -name "*~" -exec rm {} \;


release : all


