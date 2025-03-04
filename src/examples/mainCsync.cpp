#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include "MQTTClient.h" 

#define BROKER_ADDRESS  "tcp://localhost:1883"
#define CLIENT_ID       "CLientC"
#define TOPIC           "test/topic"
#define QOS             1
#define TIMEOUT         10000L

int messageArrived(void* context, char* topicName, int topicLen, MQTTClient_message* msg) {
	printf("Mensaje recibido [%s]: %.*s\n", 
	topicName, msg->payloadlen, (char*)msg->payload);
	MQTTClient_freeMessage(&msg);
	MQTTClient_free(topicName);
	return 1;
}

void connlost(void *context, char *cause) {
	printf("\nConnection lost exiting\n");
	printf("Causa: %s\n", cause);
	exit(EXIT_FAILURE);
}

int main() {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    int rc;

    rc = MQTTClient_create(&client, BROKER_ADDRESS, CLIENT_ID,
                           MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Error creating client: %d\n", rc);
        exit(EXIT_FAILURE);
    }
	
	rc = MQTTClient_setCallbacks(client, NULL, connlost, messageArrived, NULL);
	if (rc != MQTTCLIENT_SUCCESS) {
		fprintf(stderr, "Error setting callbacks: %d\n", rc);
		MQTTClient_destroy(&client);
		exit(EXIT_FAILURE);
	}

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Connection error: %d\n", rc);
        MQTTClient_destroy(&client);
        exit(EXIT_FAILURE);
    }

    char payload[] = "Hello World from C!";
    pubmsg.payload = payload;
    pubmsg.payloadlen = strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    rc = MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
        fprintf(stderr, "Error at publish: %d\n", rc);
        MQTTClient_disconnect(client, 1000);
        MQTTClient_destroy(&client);
        exit(EXIT_FAILURE);
    }

    printf("Mensaje publicado (token: %d)\n", token);

    MQTTClient_waitForCompletion(client, token, TIMEOUT);

	// to keep the connection alive subscribe to a topic and wait for a message
	
	rc = MQTTClient_subscribe(client, "test/topic", 1);
	if (rc != MQTTCLIENT_SUCCESS) {
		fprintf(stderr, "Error at subscribe: %d\n", rc);
		MQTTClient_disconnect(client, 1000);
		MQTTClient_destroy(&client);
		exit(EXIT_FAILURE);
	}
	while(1){
		std::cout << "Waiting for messages..." << std::endl;
		MQTTClient_yield(); 
	}
	
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);

    return EXIT_SUCCESS;
}
