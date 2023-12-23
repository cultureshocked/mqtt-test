#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <MQTTAsync.h>

#include "../include/status.h"

#define ADDRESS "localhost:1883"
#define CLIENTID "TemporaryClientName"
#define TIMEOUT 10000L
#define TOPICS {"hello", "world", "hello/world"}
#define QOS {1, 1, 1}
#define TOPIC_LEN 3

void on_connect(void* context, MQTTAsync_successData* res);
void on_connect_failure(void* context, MQTTAsync_failureData* res);

// Helper
void print_mqtt_err(char* msg, int return_code) {
  fprintf(stderr, "ERR: %s.\nReturn code: %d\nExiting.\n", msg, return_code);
}

// Reconnect
void connlost(void* context, char* cause) {
  MQTTAsync client = (MQTTAsync) context;
  MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
  int rc;

  opts.cleansession = 1;
  opts.keepAliveInterval = 20;

  fprintf(stderr, "\nConnection lost.\n\n");
  if (cause) fprintf(stderr, "Reason: %s\n\n", cause);
  puts("Reconnecting...");

  if ((rc = MQTTAsync_connect(client, &opts)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not start connection process", rc);
    set_fin();
  }
}

// Handles incoming (subscribed) messages
int printmsg(void* context, char* topic, int topic_len, MQTTAsync_message* msg) {
  printf("Message: { topic: \"%s\", data: \"%.*s\" }\n", topic, msg->payloadlen, (char*) msg->payload);
  MQTTAsync_freeMessage(&msg);
  MQTTAsync_free(topic);
  return 1;
}

// Disconnect handlers
void on_disconnect(void* context, MQTTAsync_successData* res) {
  puts("Disconnected.");
  set_dc();
}

void on_disconnect_failure(void* context, MQTTAsync_failureData* res) {
  print_mqtt_err("Could not disconnect gracefully", res->code);
  set_dc();
}

// Subscribe handlers
void on_subscribe(void* context, MQTTAsync_successData* res) {
  printf("Subscribed to requested topics.\n");
  set_sub();
}

void on_subscribe_failure(void* context, MQTTAsync_failureData* res) {
  print_mqtt_err("Could not subscribe to all requested topics", res->code);
  set_fin();
}

// Connection handlers
void on_connect(void* context, MQTTAsync_successData* res) {
  static char* topic_strs[] = TOPICS;
  static char** topics = topic_strs;
  static int qos[] = QOS;
  MQTTAsync client = (MQTTAsync) context;
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  int rc;

  puts("Connected.");
  printf("Using client ID %s\n", CLIENTID);
  puts("Topics:");
  for (int i = 0; i < TOPIC_LEN; ++i) {
    printf("Name: \"%s\" with QoS %d\n", topics[i], qos[i]);
  }

  opts.onSuccess = on_subscribe;
  opts.onFailure = on_subscribe_failure;
  opts.context = client;

  puts("Press Q<Enter> to quit.");
  if ((rc = MQTTAsync_subscribeMany(context, TOPIC_LEN, topics, qos, &opts)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not subscribe to topics(s)", rc);
    set_fin();
  }
}

void on_connect_failure(void* context, MQTTAsync_failureData* res) {
  print_mqtt_err("Could not connect to server", res->code);
  set_fin();
}

int main(int argc, char** argv) {
  MQTTAsync client;
  MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
  MQTTAsync_disconnectOptions dc_opts = MQTTAsync_disconnectOptions_initializer;
  int rc, ch;

  puts("Creating client.");
  if ((rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not create client", rc);
    exit(EXIT_FAILURE);
  }

  puts("Client created. Configuring callbacks.");
  if ((rc = MQTTAsync_setCallbacks(client, client, connlost, printmsg, NULL)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not set callbacks on client.", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }

  puts("Callbacks set. Connecting.");

  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.onSuccess = on_connect;
  conn_opts.onFailure = on_connect_failure;
  conn_opts.context = client;

  if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not begin connection process", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }

  puts("Connected.");

  while (!get_sub() && !get_fin()) 
    usleep(TIMEOUT);

  if (get_fin()) {
    MQTTAsync_destroy(&client);
    return 0;
  }

  while ((ch = getchar())) 
    if ((ch & 0xDF) == 'Q') break;

  dc_opts.onSuccess = on_disconnect;
  dc_opts.onFailure = on_disconnect_failure;

  puts("Disconnecting.");
  if ((rc = MQTTAsync_disconnect(client, &dc_opts)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not begin disconnection process.", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }

  while (!get_dc())
    usleep(TIMEOUT);

  MQTTAsync_destroy(&client);
  return 0;
}
