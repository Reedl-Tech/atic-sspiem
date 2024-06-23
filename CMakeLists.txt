cmake_minimum_required(VERSION 3.0)

# set project name from current directory
get_filename_component(BASENAME ${CMAKE_CURRENT_LIST_DIR} NAME)
project(${BASENAME})

set (CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0") # Add debug information

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

set(SRC_LIST 
	src/atic-ictrl-cmd.c
	src/atic-ictrl-cmd.h
	src/atic-ictrl.c
	src/atic-ictrl.h
	src/atic-utils.c
	src/atic-utils.h
	src/core.c
	src/core.h
	src/debug.h
	src/hardware.c
	src/hardware.h
	src/intrface.c
	src/intrface.h
	src/main.c
	src/opcode.h
	src/reedl-generic.h
	src/SSPIEm.c
	src/SSPIEm.h
	src/util.c
	src/util.h
)


add_executable (${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} Threads::Threads)

install(TARGETS sspiem-ictrl DESTINATION bin)
