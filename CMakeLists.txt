cmake_minimum_required(VERSION 3.10)

project(fechat)

file(GLOB FDS_ARRAY_INCLUDE "fds_array/include/*.h") 
file(GLOB FDS_ARRAY_SRC 	"fds_array/src/*.c")

set(FDS_ARRAY_FILES ${FDS_ARRAY_INCLUDE} ${FDS_ARRAY_SRC})

file(GLOB QUEUE_INCLUDE "queue/include/*.h") 
file(GLOB QUEUE_SRC 	"queue/src/*.c")

set(QUEUE_FILES ${QUEUE_INCLUDE} ${QUEUE_SRC})

file(GLOB SERVER_INCLUDE "server/include/*.h") 
file(GLOB SERVER_SRC 	 "server/src/*.c")

set(SERVER_FILES ${SERVER_INCLUDE} ${SERVER_SRC})

set(SOURCES ${QUEUE_FILES} ${SERVER_FILES} ${FDS_ARRAY_FILES})

set(CMAKE_BUILD_TYPE Debug)

add_executable(server ${SOURCES})

target_link_libraries(server pthread)
