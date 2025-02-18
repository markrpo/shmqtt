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

bool reconnect(MQTTClient client) {
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	int rc = MQTTClient_connect(client, &conn_opts);
	if (rc != MQTTCLIENT_SUCCESS) {
		fprintf(stderr, "Connection error: %d\n", rc);
		return false;
	}
	return 1;
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
	
	/*if you set callbacks, you can't use the function MQTTClient_receive()
	rc = MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL);
	if (rc != MQTTCLIENT_SUCCESS) {
		fprintf(stderr, "Error setting callbacks: %d\n", rc);
		MQTTClient_destroy(&client);
		exit(EXIT_FAILURE);
	}
	*/

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
		// MQTTClient_yield();
		// Other option is to use the function MQTTClient_receive():
		char* topicName;
		int topicLen;
		MQTTClient_message* message;
		int msgLen;
		MQTTProperties msgProps = MQTTProperties_initializer;

		rc = MQTTClient_receive(client, &topicName, &topicLen, &message, 2000); // 2 seconds timeout
		if (rc == MQTTCLIENT_SUCCESS && message != NULL) {
			printf("Mensaje recibido [%s]: %.*s\n", topicName, message->payloadlen, (char*)message->payload);
			MQTTClient_freeMessage(&message);
			MQTTClient_free(topicName);
		}
		if (rc != MQTTCLIENT_SUCCESS) {
			fprintf(stderr, "Error at receive: %d\n", rc);
			if (MQTTClient_isConnected(client)) {
				std::cout << "Client is connected" << std::endl; //Caution here, this means that rc != MQTTCLIENT_SUCCESS but the client is still connected this must be handled properly
			}
			else {
				fprintf(stderr, "Connection lost trying to reconnect...\n");
				while (!reconnect(client)) {
					std::cout << "Reconnecting..." << std::endl;
					sleep(1);
				}
			}
		}
	}
	
    MQTTClient_disconnect(client, 1000);
    MQTTClient_destroy(&client);

    return EXIT_SUCCESS;
}
