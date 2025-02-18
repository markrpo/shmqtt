#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>

#include "MQTTAsync.h"  

#define BROKER_ADDRESS  "tcp://localhost:1883"
#define CLIENT_ID       "async_c_client"
#define TOPIC           "test/async"
#define QOS             1

std::mutex mtx;
std::condition_variable cv;
int disconnect = 1;

struct context_struct {
    MQTTAsync client;
    int* pcontext;
};

void onConnect(void* context, MQTTAsync_successData* response) {
    printf("Connected to broker\n");
	std::unique_lock<std::mutex> lck(mtx);
	disconnect = 0;
	std::cout << "Sending message" << std::endl;
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    char payload[] = "Async message!";

    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts);

	// subscribe to a topic
	MQTTAsync_responseOptions sub_opts = MQTTAsync_responseOptions_initializer;
	int rc;
	rc = MQTTAsync_subscribe(client, "hola", QOS, &sub_opts);

}

void onSend(void* context, MQTTAsync_successData* response) {
    printf("Published, token: %d)\n", response->token);
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
	std::cout << "Reconnecting in onConnectFailure" << std::endl;
	sleep(5);

	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions connOpts = MQTTAsync_connectOptions_initializer;
	connOpts.onSuccess = onConnect;
	connOpts.onFailure = onConnectFailure;
	connOpts.context = client;
	connOpts.keepAliveInterval = 20;
	connOpts.cleansession = 1;

	MQTTAsync_connect(client, &connOpts);
	
}


void onconnLost(void* context, char* cause) {
	if (!context) {
		fprintf(stderr, "Context is null!\n");
	return;
	}
    printf("Connection lost, cause: %s\n", cause);
	std::unique_lock<std::mutex> lck(mtx);
	disconnect = 1;
	std::cout << "Reconnecting in onconnLost" << std::endl;

	context_struct* contexted = (context_struct*)context; // we cast the context to a context_struct pointer
	// we can not do MQTTAsync client = (context_struct*)context->client; because -> has higher precedence than (context_struct*) and context is a void pointer, so we need to cast it to a context_struct pointer first and then access the client
	MQTTAsync client = contexted->client; // we access the client
	int* pcontext = contexted->pcontext; // we access the context
	MQTTAsync_connectOptions connOpts = MQTTAsync_connectOptions_initializer;
	connOpts.onSuccess = onConnect;
	connOpts.onFailure = onConnectFailure;
	connOpts.context = client;
	connOpts.keepAliveInterval = 20;
	connOpts.cleansession = 1;
	
	std::cout << "Reconnecting" << std::endl;

	int rc = MQTTAsync_connect(client, &connOpts);
	std::cout << "Connected" << std::endl;

	cv.notify_all();
}

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
    printf("Message arrived\n");
    printf("Topic: %s\n", topicName);
    printf("Message: %.*s\n", message->payloadlen, (char*)message->payload);
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
	// to acces the context we need to cast it to the correct type
	int* pcontext = (int*)context; // we cast the context to an int pointer
	std::cout << "Context: " << *pcontext << std::endl; // we print the context
    return 1;
}

void deliveryComplete(void* context, MQTTAsync_token token) {
    printf("Delivery complete\n");
}

int main() {
    MQTTAsync client;
    MQTTAsync_createOptions createOpts = MQTTAsync_createOptions_initializer;
	// conn_opts.http_proxy = "http://proxy:8080";
    MQTTAsync_connectOptions connOpts = MQTTAsync_connectOptions_initializer;
    int rc;
    // sendWhileDisconnected: 0 (default) doesn't allow sending messages while disconnected and messages are queued
    createOpts.sendWhileDisconnected = 0;
    rc = MQTTAsync_createWithOptions(&client, BROKER_ADDRESS, CLIENT_ID,
                                     MQTTCLIENT_PERSISTENCE_NONE, NULL, &createOpts);
    if (rc != MQTTASYNC_SUCCESS) {
        fprintf(stderr, "Error creating client: %d\n", rc);
        exit(EXIT_FAILURE);
    }

    connOpts.onSuccess = onConnect;
	connOpts.onFailure = onConnectFailure;
    connOpts.context = client; 
    connOpts.keepAliveInterval = 20;
    connOpts.cleansession = 1;

	context_struct contexted;
	contexted.client = client;
	contexted.pcontext = new int(10); // we create a new int pointer and assign it the value 10
	
    MQTTAsync_setCallbacks(client, &contexted, onconnLost, messageArrived, deliveryComplete); // we pass the client as context in onConnect, the second parameter is to pass the context, in case we want to use it in the callback, in the third parameter we pass the messageArrived function, in the fourth parameter we pass the connLost function, and in the fifth parameter we pass the deliveryComplete function

    rc = MQTTAsync_connect(client, &connOpts);

	std::cout << "Connected" << std::endl;
    while (1) {
		std::unique_lock<std::mutex> lck(mtx);
		cv.wait(lck, []{return disconnect;}); // we wait until the disconnect variable is set to 1
		std::cout << "Reconnecting" << std::endl;
		cv.wait(lck, []{return !disconnect;}); // until the disconnect variable is set to 0
    }

    MQTTAsync_disconnect(client, NULL);
    MQTTAsync_destroy(&client);
    return EXIT_SUCCESS;
}
