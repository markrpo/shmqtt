#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "MQTTAsync.h"  

#define BROKER_ADDRESS  "tcp://localhost:1883"
#define CLIENT_ID       "async_c_client"
#define TOPIC           "test/async"
#define QOS             1

void onConnect(void* context, MQTTAsync_successData* response) {
    printf("Connected to broker\n");
    
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
	if (rc != MQTTASYNC_SUCCESS) {
		fprintf(stderr, "Error subscribing: %d\n", rc);
		exit(EXIT_FAILURE);
	}
	

}

void onSend(void* context, MQTTAsync_successData* response) {
    printf("Published, token: %d)\n", response->token);
}

void onconnLost(void* context, char* cause) {
    printf("Connection lost, cause: %s\n", cause);
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

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Connect failed, rc: %d\n", response->code);
    exit(EXIT_FAILURE);
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
	int context = 0;

    // sendWhileDisconnected: 0 (default) doesn't allow sending messages while disconnected and messages are queued
    createOpts.sendWhileDisconnected = 0;
    rc = MQTTAsync_createWithOptions(&client, BROKER_ADDRESS, CLIENT_ID,
                                     MQTTCLIENT_PERSISTENCE_NONE, NULL, &createOpts);
    if (rc != MQTTASYNC_SUCCESS) {
        fprintf(stderr, "Error creating client: %d\n", rc);
        exit(EXIT_FAILURE);
    }

    connOpts.onSuccess = onConnect;
	// connOpts.onFailure = onConnectFailure;
    connOpts.context = client; 
    connOpts.keepAliveInterval = 20;
    connOpts.cleansession = 1;

    MQTTAsync_setCallbacks(client, &context, onconnLost, messageArrived, deliveryComplete); // we pass the client as context in onConnect, the second parameter is to pass the context, in case we want to use it in the callback, in the third parameter we pass the messageArrived function, in the fourth parameter we pass the connLost function, and in the fifth parameter we pass the deliveryComplete function

    rc = MQTTAsync_connect(client, &connOpts);
    if (rc != MQTTASYNC_SUCCESS) {
        fprintf(stderr, "Error starting connection: %d\n", rc);
        MQTTAsync_destroy(&client);
        exit(EXIT_FAILURE);
    }

    printf("Waiting CTRL + C to exit\n");
    while (1) {
        sleep(1);
    }

    MQTTAsync_disconnect(client, NULL);
    MQTTAsync_destroy(&client);
    return EXIT_SUCCESS;
}
