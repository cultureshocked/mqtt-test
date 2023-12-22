#include <stdio.h>
#include <MQTTAsync.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "../include/status.h"

#define ADDRESS "localhost:1883"
#define CLIENTID "TemporaryClientName"
#define TOPIC "hello"
#define PAYLOAD "Test Message."
#define QOS 1
#define TIMEOUT 10000L

void on_connect(void* context, MQTTAsync_successData* res);
void on_connect_failure(void* context, MQTTAsync_failureData* res);

// Handles dropped connections
void connlost(void* context, char* cause) {
  MQTTAsync client = (MQTTAsync) context;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  int rc;

  fprintf(stderr, "\nConnection lost.\n\n");

  if (cause)
    fprintf(stderr, "Reason: %s\n", cause);

  puts("Reconnecting...");
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = on_connect;
  conn_opts.onFailure = on_connect_failure;

  if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "ERR: Could not start connection process.\nReturn code: %d\nExiting.\n", rc);
    set_fin();
  }
}

// Prints out messages received
int arrived(void* context, char* topic, int topic_len, MQTTAsync_message* msg) {
  printf("Message: { topic: %s; msg: %.*s }\n", topic, msg->payloadlen, (char*) msg->payload);
  MQTTAsync_freeMessage(&msg);
  MQTTAsync_free(topic);
  return 1;
}

// Handler for failed disconnection
void on_disconnect_failure(void* context, MQTTAsync_failureData* res) {
  fprintf(stderr, "ERR: Disconnection failed.\nReturn code: %d\nExiting.\n", res->code);
  set_dc();
}

// Handler for disconnection
void on_disconnect(void* context, MQTTAsync_successData* res) {
  printf("Disconnected.\n");
  set_dc();
}

// Handler for subscription
void on_subscribe(void* context, MQTTAsync_successData* res) {
  printf("Subscribed.\n");
  set_sub();
}

// Handler for failed subscription
void on_subscribe_failure(void* context, MQTTAsync_failureData* res) {
  fprintf(stderr, "ERR: Could not subscribe.\nReturn code: %d\nExiting.\n", res->code);
  set_fin();
}

// Handler for failed connection
void on_connect_failure(void* context, MQTTAsync_failureData* res) {
  fprintf(stderr, "ERR: Could not connect to MQTT broker.\nReturn code: %d\nExiting.\n", res->code);
  set_fin();
}

//Handler for connection
void on_connect(void* context, MQTTAsync_successData* res) {
  MQTTAsync client = (MQTTAsync) context;
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  int rc;

  puts("Connected.");
  printf("Subscribing to topic: %s\n", TOPIC);
  printf("Using client ID: %s\n", CLIENTID);
  printf("Using QoS level: %d\n", QOS);
  printf("Press Q<Enter> to quit.\n\n");

  opts.onSuccess = on_subscribe;
  opts.onFailure = on_subscribe_failure;
  opts.context = client;
  if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "ERR: Could not subscribe to the selected topic.\nReturn code: %d\nExiting.\n", rc);
    set_fin();
  }
}

int main(int argc, char** argv) {
  MQTTAsync client;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  MQTTAsync_disconnectOptions dc_opts = MQTTAsync_disconnectOptions_initializer;
  int rc, ch;
  
  puts("Creating client.");
  if ((rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "ERR: Could not create client.\nReturn code: %d\nExiting.\n", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }
  puts("Client created. Adding callbacks.");

  if ((rc = MQTTAsync_setCallbacks(client, client, connlost, arrived, NULL)) != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "ERR: Could not set callback functions.\nReturn code: %d\nExiting.\n", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }

  puts("Calbacks set. Connecting.");

  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = on_connect;
  conn_opts.onFailure = on_connect_failure;
  conn_opts.context = client;

  if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "ERR: Could not connect.\nReturn code: %d\nExiting.\n", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }

  puts("Connected. Entering status loop.");

  while (!get_sub() && !get_fin()) 
    usleep(TIMEOUT);

  puts("Checking if finished.");
  if (get_fin())
    return 0;

  puts("Looping for exit input Q.");
  while ((ch = getchar()))
    if ((ch & 0xDF) == 'Q') break;

  dc_opts.onFailure = on_disconnect_failure;
  dc_opts.onSuccess = on_disconnect;

  if ((rc = MQTTAsync_disconnect(client, &dc_opts)) != MQTTASYNC_SUCCESS) {
    fprintf(stderr, "ERR: Could not start disconnection process.\nReturn code: %d\nExiting.\n", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }

  while (!get_dc())
    usleep(TIMEOUT);

  MQTTAsync_destroy(&client);
  return 0;
}
