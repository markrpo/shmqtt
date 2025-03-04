#include "imqtt.hpp"

MQTT::MQTT(std::string address, std::string client_id){
	m_address = address;
	m_client_id = client_id;
}

MQTT::~MQTT(){
}

void MQTT::add_observer(Observer *observer){
	std::cout << "Adding observer" << std::endl;
	m_observers.push_back(observer);
}

void MQTT::remove_observer(Observer *observer){
	m_observers.remove(observer);
}

void MQTT::notify_observers(std::string topic, std::string message){
	for(auto observer : m_observers){
		observer->update(topic, message);
	}
}

void MQTT::publish(std::string topic, std::string message){
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	pubmsg.payload = (void *)message.c_str();
	pubmsg.payloadlen = message.length();
	pubmsg.qos = 0;
	pubmsg.retained = 0;
	MQTTClient_deliveryToken token;
	int rc = MQTTClient_publishMessage(m_client, topic.c_str(), &pubmsg, &token);
	if(rc != MQTTCLIENT_SUCCESS){
		std::cout << "Failed to publish message, return code: " << rc << std::endl;
	}
}

int MQTT::message_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message){
	std::cout << "Message arrived" << std::endl;

	MQTTMessage message_struct;
	message_struct.topic = std::string(topicName);
	message_struct.message = std::string((char *)message->payload);

	MQTT *mqtt = (MQTT *)context;
	std::lock_guard<std::mutex> lock(mqtt->m_mutex);
	mqtt->m_messages.push_back(message_struct);
	mqtt->m_condition.notify_one();

	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	std::cout << "Message processed" << std::endl;
	return 1;
}

/*void MQTT::connlost(void *context, char *cause){
	std::cout << "Connection lost" << std::endl;
	std::cout << "Cause: " << cause << std::endl;
	MQTT *mqtt = (MQTT *)context;
	mqtt->m_running = false;
	mqtt->m_condition.notify_one();
	std::cout << "Connection lost processed" << std::endl;
}*/

void MQTT::connlost(){
	std::cout << "Connection lost" << std::endl;
	m_running = false;
	m_condition.notify_one();
}

void MQTT::connect(){
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_deliveryToken token;

	int rc;
	
	rc = MQTTClient_create(&m_client, m_address.c_str(), m_client_id.c_str(), MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if(rc != MQTTCLIENT_SUCCESS){
		std::cout << "Failed to create client, return code: " << rc << std::endl;
		exit(-1);
	}

	// rc = MQTTClient_setCallbacks(m_client, this, connlost, message_arrived, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	rc = MQTTClient_connect(m_client, &conn_opts);
	if(rc != MQTTCLIENT_SUCCESS){
		std::cout << "Failed to connect, return code: " << rc << std::endl;
		exit(-1);
	}

	rc = MQTTClient_subscribe(m_client, "test", 0);
	if(rc != MQTTCLIENT_SUCCESS){
		std::cout << "Failed to subscribe, return code: " << rc << std::endl;
		exit(-1);
	}

	std::cout << "Connected" << std::endl;
	m_running = true;

	process_thread = std::thread([this](){ // execute the following code in a separate thread, it can access the members of the class because this (a pointer to the class) is passed to the lambda function
										   // normally a thread is created doing: std::thread thread_name(function_name, arguments), but here we use a lambda function
										   // if the thread function returns, the thread is finished (and does not continue in the main thread)
		std::cout << "Process thread started" << std::endl;
		std::unique_lock<std::mutex> lock(m_mutex);
		while(m_running){
			m_condition.wait(lock);
			std::cout << "Process thread notified" << std::endl;
			while(!m_messages.empty()){
				MQTTMessage message = m_messages.front();
				m_messages.erase(m_messages.begin());
				lock.unlock();
				notify_observers(message.topic, message.message);
				lock.lock(); 
			}
		}
		std::cout << "Process thread stopped" << std::endl;
	});

	int topicLen;
	MQTTClient_message *message;
	char *topicName;

	while(m_running){
		std::cout << "Waiting for messages" << std::endl;
		rc = MQTTClient_receive(m_client, &topicName, &topicLen, &message, 1000);
		if(rc == MQTTCLIENT_SUCCESS && message != NULL){
			message_arrived(this, topicName, topicLen, message);
		}
		if(rc != MQTTCLIENT_SUCCESS){
			std::cout << "Failed to receive message, return code: " << rc << std::endl;
			if(MQTTClient_isConnected(m_client)){
				std::cout << "Client is connected" << std::endl;
				// When can this happen?
			}
			else{
				std::cout << "Client is not connected" << std::endl;
				connlost();
			}
		}
	}

	std::cout << "Disconnecting" << std::endl;
	// wait for the process thread to finish
	process_thread.join();

}

MQTTObserver::MQTTObserver(std::string name, IMQTT *mqtt){
	m_name = name;
	m_mqtt = mqtt;
}


void MQTTObserver::update(std::string topic, std::string message){
	std::cout << "Observer: " << m_name << std::endl;
	std::cout << "Topic: " << topic << std::endl;
	std::cout << "Message: " << message << std::endl;
	m_mqtt->publish("test2", "Hello from observer " + m_name);
}

int main(){
	MQTT mqtt("tcp://localhost:1883", "test");
	MQTTObserver observer1("Observer1", &mqtt); // MQTTObserver is an observer receives objects of type IMQTT
												// even if mqtt is of type MQTT, it is derived from IMQTT
												// so it can be passed to the constructor of MQTTObserver
	MQTTObserver observer2("Observer2", &mqtt);

	mqtt.add_observer(&observer1);
	mqtt.add_observer(&observer2);

	mqtt.connect();

	return 0;
}
