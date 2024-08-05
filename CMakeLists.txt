cmake_minimum_required(VERSION 3.0)

# set project name from current directory
get_filename_component(BASENAME ${CMAKE_CURRENT_LIST_DIR} NAME)
project(${BASENAME})

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g3 -O0") # Add debug information

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(PkgConfig REQUIRED)

set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

set(SRC_LIST 
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
	src/reedl-ictrl-cmd.c
	src/reedl-ictrl-cmd.h
	src/reedl-ictrl.c
	src/reedl-ictrl.h
	src/reedl-utils.c
	src/reedl-utils.h
	src/SSPIEm.c
	src/SSPIEm.h
	src/util.c
	src/util.h
)

add_executable (${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} Threads::Threads)

install(TARGETS atic-sspiem DESTINATION bin)
