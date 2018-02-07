############################################
cmake_minimum_required(VERSION 2.8.9)
project(priocpp)
include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

############################################
# OS support
############################################


IF (WIN32)

	set(OSLIBS "Ws2_32" "Rpcrt4")

    find_package(gtest REQUIRED)
    find_package(openssl REQUIRED)
	find_library(CRYPTONEAT_LIBRARY NAMES cryptoneat)
	find_library(NG2HTTP_LIBRARY NAMES nghttp2)
	find_library(LIBEVENT_LIBRARY NAMES event)

	set(BUILDFLAGS "-await")
	set(DEBUGFLAGS "-DMOL_PROMISE_DEBUG")

	set(LIBS ${CRYPTONEAT_LIBRARY} ${GTEST_LIBRARIES} ${OPENSSL_LIBRARIES} ${NG2HTTP_LIBRARY}  )

 ELSEIF(UNIX)

    set(OSLIBS "pthread")

    find_package(PkgConfig)
    pkg_check_modules(GTEST REQUIRED gtest)
    pkg_check_modules(OPENSSL REQUIRED openssl)
    pkg_check_modules(CRYPTONEAT REQUIRED cryptoneat)
    pkg_check_modules(NGHTTP2 REQUIRED nghttp2)
    pkg_check_modules(LIBEVENT REQUIRED libevent_pthreads)

    set(OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIRS})
	set(LIBEVENT_LIBRARY ${LIBEVENT_LIBRARIES})

    set(BUILDFLAGS "${GTEST_CFLAGS_OTHER}")
	set(DEBUGFLAGS "-g -DMOL_PROMISE_DEBUG")

	set(LIBS ${CRYPTONEAT_LIBRARIES} ${GTEST_LIBRARIES} ${OPENSSL_LIBRARIES} ${NG2HTTP_LIBRARIES}  )

ENDIF ()


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
# defaults
############################################


set(BACKEND ${LIBEVENT_LIBRARY})

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

AddCompilerFlags(priocpp ${BUILDFLAGS})

AddCompilerFlags(priocppd ${BUILDFLAGS})
AddCompilerFlags(priocppd ${DEBUGFLAGS})

AddCompilerFlags(Tests ${BUILDFLAGS})
AddCompilerFlags(Tests ${DEBUGFLAGS})


############################################
# libs
############################################

target_link_libraries(Tests priocppd ${LIBS} ${BACKEND} ${OSLIBS} )


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
