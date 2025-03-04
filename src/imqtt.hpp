#include <iostream>
#include <string>
#include <list>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <vector>
#include "MQTTClient.h"

struct MQTTMessage {
	std::string topic;
	std::string message;
};

class Observer {
public:
	virtual void update(std::string topic, std::string message) = 0;
};

class IMQTT {
public:
	virtual void connect() = 0;
	//virtual void disconnect() = 0;
	//virtual void subscribe(std::string topic) = 0;
	//virtual void unsubscribe(std::string topic) = 0;
	virtual void publish(std::string topic, std::string message) = 0;
	virtual void add_observer(Observer* observer) = 0;
	virtual void remove_observer(Observer* observer) = 0;
	virtual void notify_observers(std::string topic, std::string message) = 0;
};

class MQTT : public IMQTT {
public:
	MQTT(std::string address, std::string client_id);
	~MQTT();
	void connect();
	//void disconnect();
	//void subscribe(std::string topic);
	//void unsubscribe(std::string topic);
	void publish(std::string topic, std::string message);
	void add_observer(Observer* observer);
	void remove_observer(Observer* observer);
	void notify_observers(std::string topic, std::string message);

	static int message_arrived(void* context, char* topicName, int topicLen, MQTTClient_message* message);
	//static void connlost(void* context, char* cause);
	void connlost();
private:
	std::string m_address;
	std::string m_client_id;
	MQTTClient m_client;
	std::list<Observer*> m_observers;
	bool m_running;

	std::mutex m_mutex;
	std::condition_variable m_condition;
	std::thread receive_thread;
	std::thread process_thread;

	std::vector<MQTTMessage> m_messages;
};

class MQTTObserver : public Observer {
public:
	MQTTObserver(std::string name, IMQTT* mqtt);
	void update(std::string topic, std::string message);

private:
	std::string m_name;
	IMQTT* m_mqtt;
};
