cmake_minimum_required(VERSION 3.10)
project(shmqtt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find libpahoC 
find_path(PAHO_MQTT_C_INCLUDE_DIR
	NAMES MQTTAsync.h
	PATHS /usr/include /usr/local/include
	)
find_library(PAHO_MQTT_C_LIBRARIES
	NAMES paho-mqtt3a
	PATHS /usr/lib /usr/local/lib
	)


# Set sources and headers files
set(SOURCES src/main.cpp)
set(HEADERS)

# Add executable
add_executable(shmqtt ${SOURCES} ${HEADERS})

# Include directories PRIVATE means only this target can see the include directories
target_include_directories(shmqtt PRIVATE include ${PAHO_MQTT_C_INCLUDE_DIR})

# Link libraries
target_link_libraries(shmqtt PRIVATE  ${PAHO_MQTT_C_LIBRARIES})
