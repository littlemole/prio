############################################
cmake_minimum_required(VERSION 2.8.9)
project(priocpp)
include(CTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


############################################
# macros
############################################

macro(AddCompilerFlags target flags)
    get_target_property(CF ${target} COMPILE_FLAGS)
    if(CF STREQUAL "CF-NOTFOUND")
        SET(CF "") # set to empty string
    else()
        SET(CF "${CF} ") # a space to cleanly separate from existing content
    endif()

    SET(CF "${CF} ${flags}" )
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${CF} )
endmacro()


############################################
# clang support
############################################

set(LINKERFLAGS "")
set(STDLIB "")
set(FLAGS "")

message(" COMPILER: ${CMAKE_CXX_COMPILER_ID}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(STDLIB "c++abi")
    set(LINKERFLAGS "-stdlib=libc++ -fcoroutines-ts  -std=c++14")
    set(FLAGS "-stdlib=libc++ -fcoroutines-ts -D_RESUMABLE_FUNCTIONS_SUPPORTED")
endif()

set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${LINKERFLAGS}" )


############################################
# OS support
############################################

find_package(GTest REQUIRED)
find_package(OpenSSL REQUIRED)

IF (WIN32)

	set(OSLIBS "Ws2_32" "Rpcrt4")

	find_library(CRYPTONEAT_LIBRARY NAMES cryptoneat)
	find_library(LIBEVENT_LIBRARY NAMES event)

	set(BUILDFLAGS "-await ${FLAGS}")
	set(DEBUGFLAGS "-DMOL_PROMISE_DEBUG")

	set(LIBS ${CRYPTONEAT_LIBRARY} ${GTEST_LIBRARIES} ${OPENSSL_LIBRARIES} ${STDLIB}  )

 ELSEIF(UNIX)

    set(OSLIBS "pthread")

    find_package(PkgConfig)
    pkg_check_modules(CRYPTONEAT REQUIRED cryptoneat)
    pkg_check_modules(LIBEVENT libevent_pthreads)

	set(LIBEVENT_LIBRARY ${LIBEVENT_LIBRARIES})

    set(BUILDFLAGS " ${FLAGS}")
    set(RELEASEFLAGS " -O3 ")
    set(DEBUGFLAGS "-g -DMOL_PROMISE_DEBUG")

    set(LIBS ${CRYPTONEAT_LIBRARIES} ${GTEST_LIBRARIES} ${OPENSSL_LIBRARIES} ${STDLIB} )

ENDIF ()


############################################
# backend: libevent or boost_asio
############################################


option(WITH_LIBEVENT "use libevent (default:on)" ON)
if(WITH_LIBEVENT)
    add_definitions(-DPROMISE_USE_LIBEVENT)    
    set(BACKEND ${LIBEVENT_LIBRARY})
else()
    add_definitions(-DPROMISE_USE_BOOST_ASIO)
    set(BACKEND "boost_system")
endif()


############################################
# sources
############################################

include_directories(include)
include_directories(${OPENSSL_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} ${CRYPTONEAT_INCLUDE_DIRS} )

file(GLOB SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "include/priocpp/*.h")

message("HEADERS: ${HEADERS}")
############################################
# targets
############################################

add_library(priocpp  STATIC ${SOURCES})
add_library(priocppd STATIC ${SOURCES})


add_executable(SimpleTest t/test.cpp)
add_executable(HttpTest t/http.cpp)

AddCompilerFlags(priocpp ${BUILDFLAGS})

AddCompilerFlags(priocppd ${BUILDFLAGS})
AddCompilerFlags(priocppd ${DEBUGFLAGS})

AddCompilerFlags(SimpleTest ${BUILDFLAGS})
AddCompilerFlags(SimpleTest ${DEBUGFLAGS})

AddCompilerFlags(HttpTest ${BUILDFLAGS})
AddCompilerFlags(HttpTest ${DEBUGFLAGS})

############################################
# libs
############################################

target_link_libraries(SimpleTest priocppd ${LIBS} ${BACKEND} ${OSLIBS} )
target_link_libraries(HttpTest priocppd ${LIBS} ${BACKEND} ${OSLIBS} )


############################################
# tests
############################################

add_test(NAME Simple_Test COMMAND SimpleTest)
add_test(NAME Http_Test COMMAND HttpTest)

file(COPY pem DESTINATION .)
 
############################################
# install rules
############################################
 
install(TARGETS priocpp DESTINATION lib)
install(TARGETS priocppd DESTINATION lib)

install(DIRECTORY include/priocpp DESTINATION include)

install(FILES priocpp.pc DESTINATION lib/pkgconfig)
