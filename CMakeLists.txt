cmake_minimum_required(VERSION 3.10)
project(shmqtt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find libpahoC 
find_path(PAHO_MQTT_CA_INCLUDE_DIR
	NAMES MQTTAsync.h
	PATHS /usr/include /usr/local/include
	)
find_library(PAHO_MQTT_CA_LIBRARIES
	NAMES paho-mqtt3a
	PATHS /usr/lib /usr/local/lib
	)

find_path(PAHO_MQTT_C_INCLUDE_DIR
	NAMES MQTTClient.h
	PATHS /usr/include /usr/local/include
	)

find_library(PAHO_MQTT_C_LIBRARIES
	NAMES paho-mqtt3c
	PATHS /usr/lib /usr/local/lib
	)

# Check if libpahoC is found
if (PAHO_MQTT_CA_INCLUDE_DIR AND PAHO_MQTT_CA_LIBRARIES)
	message(STATUS "Found libpaho-mqtt3a")
	message(STATUS "Include dir: ${PAHO_MQTT_CA_INCLUDE_DIR}")
	message(STATUS "Library: ${PAHO_MQTT_CA_LIBRARIES}")
else()
	message(FATAL_ERROR "libpaho-mqtt3a not found")
endif()

# Set sources and headers files
set(SOURCES src/mainCasync.cpp)
set(HEADERS)

# Add executable
add_executable(shmqtt ${SOURCES} ${HEADERS})

# Include directories PRIVATE means only this target can see the include directories
target_include_directories(shmqtt PRIVATE include ${PAHO_MQTT_CA_INCLUDE_DIR})

# Link libraries
target_link_libraries(shmqtt PRIVATE  ${PAHO_MQTT_CA_LIBRARIES})
