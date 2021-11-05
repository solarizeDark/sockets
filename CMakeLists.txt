cmake_minimum_required(VERSION 3.10)

project(fechat)

file(GLOB QUEUE_INCLUDE "queue/include/*.h") 
file(GLOB QUEUE_SRC 	"queue/src/*.c")

set(QUEUE_FILES ${QUEUE_INCLUDE} ${QUEUE_SRC})

file(GLOB SERVER_INCLUDE "server/include/*.h") 
file(GLOB SERVER_SRC 	 "server/src/*.c")

set(SERVER_FILES ${SERVER_INCLUDE} ${SERVER_SRC})

set(SOURCES ${QUEUE_FILES} ${SERVER_FILES})

add_executable(server ${SOURCES})

target_link_libraries(server pthread)
