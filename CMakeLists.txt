############################################
# preamble
############################################
cmake_minimum_required(VERSION 2.8.9)
cmake_policy(SET CMP0054 NEW)
project(priocpp)
include(CTest)

############################################
# c++ std
############################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

############################################
# debug mode produces libs with d suffix
############################################

if(NOT CMAKE_DEBUG_POSTFIX)
  set(CMAKE_DEBUG_POSTFIX d)
endif()

############################################
# basic dependecies
############################################

find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)


############################################
# clang support
############################################

set(LINKERFLAGS "")
set(STDLIB "")
set(FLAGS "")
set(INC_DIRS include)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(STDLIB "c++abi")
    set(LINKERFLAGS "-stdlib=libc++ -fcoroutines-ts  -std=c++14")
    set(FLAGS "-stdlib=libc++ -fcoroutines-ts -D_RESUMABLE_FUNCTIONS_SUPPORTED")
endif()


############################################
# OS support (Linux/Windows)
############################################

IF (WIN32)

	set(OSLIBS "Ws2_32" "Rpcrt4")
	set(BUILDFLAGS "-await ${FLAGS}")

	find_library(LIBEVENT_LIBRARY NAMES event)
	set(LIBS ${GTEST_LIBRARIES} ${OPENSSL_LIBRARIES} ${STDLIB}  )

message("++++++++++++++++++++++")
message("${CMAKE_BUILD_TYPE}")


	if(${CMAKE_BUILD_TYPE} MATCHES "Release")
		find_library(CRYPTONEAT_LIBRARY NAMES cryptoneat)
		message("Release:${CRYPTONEAT_LIBRARY}")
		set(LIBS ${LIBS} debug ${CMAKE_PROJECT_NAME} optimized ${CMAKE_PROJECT_NAME} ${CRYPTONEAT_LIBRARY} )
    else()
		find_library(CRYPTONEAT_LIBRARY NAMES cryptoneat)
		find_library(CRYPTONEATD_LIBRARY NAMES cryptoneatd)
		message("${CRYPTONEAT_LIBRARY}")
		message("${CRYPTONEATD_LIBRARY}")

		set(LIBS ${LIBS} debug ${CMAKE_PROJECT_NAME} optimized ${CMAKE_PROJECT_NAME} debug ${CRYPTONEATD_LIBRARY} optimized  ${CRYPTONEAT_LIBRARY} )
	endif()

message("++++++++++++++++++++++")


 ELSEIF (UNIX)

    set(OSLIBS "pthread")

    find_package(PkgConfig)
    pkg_check_modules(CRYPTONEAT REQUIRED cryptoneat)
    pkg_check_modules(LIBEVENT REQUIRED libevent_pthreads)

    set(LIBEVENT_LIBRARY ${LIBEVENT_LIBRARIES})

    set(BUILDFLAGS " ${FLAGS}")

    set(LIBS debug ${CMAKE_PROJECT_NAME} optimized ${CMAKE_PROJECT_NAME} cryptoneat ${GTEST_LIBRARIES} ${OPENSSL_LIBRARIES} ${STDLIB} )

ENDIF ()


############################################
# set linker and compiler flags
############################################

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${LINKERFLAGS}" )
set(CMAKE_CXX_FLAGS_DEBUG  	"${CMAKE_CXX_FLAGS_DEBUG} -DMOL_PROMISE_DEBUG")
set(CMAKE_CXX_FLAGS  		"${CMAKE_CXX_FLAGS} ${BUILDFLAGS}")


############################################
# backend: libevent or boost_asio
############################################


option(WITH_LIBEVENT "use libevent (default:off)" OFF)
if(WITH_LIBEVENT)
    add_definitions(-DPROMISE_USE_LIBEVENT)    
    set(BACKEND ${LIBEVENT_LIBRARY})
else()
    add_definitions(-DPROMISE_USE_BOOST_ASIO)
if (WIN32)
find_package( Boost COMPONENTS system date_time  REQUIRED )
    set(BACKEND "Boost::system" "Boost::date_time")
else()
    set(CMAKE_EXE_LINKER_FLAGS  "-L/opt/boost/lib ${CMAKE_EXE_LINKER_FLAGS}" )
    set(INC_DIRS "/opt/boost/include"  "${INC_DIRS}" )
    set(BACKEND "boost_system")
endif()
endif()


############################################
# include directories
############################################

include_directories(${INC_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} ${CRYPTONEAT_INCLUDE_DIR} )


############################################
# sources
############################################

file(GLOB SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/priocpp/*.h")

############################################
# lib target
############################################

add_library(priocpp  STATIC ${SOURCES})


############################################
# tests
############################################

add_executable(SimpleTest t/test.cpp)
add_executable(HttpTest t/http.cpp)

############################################
# libs for tests
############################################

target_link_libraries(SimpleTest ${LIBS} ${BACKEND} ${OSLIBS} )
target_link_libraries(HttpTest ${LIBS} ${BACKEND} ${OSLIBS} )

############################################
# run tests
############################################

add_test(NAME Simple_Test COMMAND SimpleTest)
add_test(NAME Http_Test COMMAND HttpTest)

file(COPY pem DESTINATION .)
 
############################################
# install rules
############################################
 
install(TARGETS priocpp DESTINATION lib)

install(DIRECTORY include/priocpp DESTINATION include)

install(FILES priocpp.pc DESTINATION lib/pkgconfig)


