############################################
cmake_minimum_required(VERSION 2.8.9)
project(priocpp)
include(CTest)
############################################

############################################
# macros
############################################

macro(AddDebugFlags target)
    get_target_property(CF ${target} COMPILE_FLAGS)
    if(CF STREQUAL "CF-NOTFOUND")
        SET(CF "") # set to empty string
    else()
        SET(CF "${CF} ") # a space to cleanly separate from existing content
    endif()

    SET(CF "${CF} -DMOL_PROMISE_DEBUG -g" )
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${CF} )
endmacro()

############################################
# defaults
############################################

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(LIBEVENT "event" "event_pthreads")
IF (WIN32)
    set(LIBEVENT "event")
    find_package(gtest REQUIRED)
    find_package(openssl REQUIRED)
	find_library(CRYPTONEAT_LIBRARY NAMES cryptoneat)
	find_library(NG2HTTP_LIBRARY NAMES nghttp2)
	find_library(LIBEVENT_LIBRARY NAMES event)
	set(LIBEVENT ${LIBEVENT_LIBRARY})
ENDIF ()

set(BACKEND ${LIBEVENT})

option(WITH_LIBEVENT "use libevent (default:on)" ON)
if(WITH_LIBEVENT)
    add_definitions(-DPROMISE_USE_LIBEVENT)
else()
    add_definitions(-DPROMISE_USE_BOOST_ASIO)
    set(BACKEND "boost_system")
endif()


############################################
# sources
############################################

include_directories(include)
include_directories(${OPENSSL_INCLUDE_DIR})

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/priocpp/*.h")

############################################
# targets
############################################

add_library(priocpp  STATIC ${SOURCES})
add_library(priocppd STATIC ${SOURCES})

add_executable(Tests t/test.cpp)

AddDebugFlags(priocppd)
AddDebugFlags(Tests)


############################################
# libs
############################################

IF (UNIX)
    target_link_libraries(Tests gtest priocppd nghttp2 pthread ${BACKEND} cryptoneat ssl crypto pthread uuid)
ELSEIF (WIN32)
    target_link_libraries(Tests ${GTEST_LIBRARY} priocppd ${NG2HTTP_LIBRARY} ${BACKEND} ${CRYPTONEAT_LIBRARY} ${OPENSSL_LIBRARIES} Ws2_32 Rpcrt4 )
ENDIF ()


############################################
# tests
############################################

add_test(NAME AllTests COMMAND Tests)

file(COPY pem DESTINATION .)
 
############################################
# install rules
############################################
 
install(TARGETS priocpp DESTINATION lib)
install(TARGETS priocppd DESTINATION lib)

install(FILES ${HEADERS} DESTINATION include/priocpp)

install(FILES priocpp.pc DESTINATION lib/pkgconfig)
