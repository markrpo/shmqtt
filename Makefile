USER_ID := $(shell id -u)
SUDO := $(if $(filter 0,$(USER_ID)),,sudo)

bootstrap_lib:
	@$(SUDO) apt-get update
	@$(SUDO) apt-get install -y cmake g++
	@$(SUDO) apt-get install -y libpaho-mqtt-dev libpaho-mqtt3as-dev libpaho-mqtt3c-dev libpaho-mqtt3c
	@$(SUDO) apt-get install -y mosquitto
	# if mqttpp does not work, use git clone https://github.com/eclipse/paho.mqtt.cpp.git
	# cd paho.mqtt.cpp
	# mkdir build
	# cd build
	# cmake ..
	# make
	# sudo make install
	
	@$(SUDO) apt install -y build-essential cmake git libssl-dev

.PHONY: bootstrap_lib

