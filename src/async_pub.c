#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <MQTTAsync.h>
#include <string.h>

#include "../include/status.h"

#define QOS 1
#define TOPIC "hello"
#define ADDRESS "localhost:1883"
#define MSG "This is a test message"
#define CLIENTID "TempPublisher"
#define TIMEOUT 10000L

void print_mqtt_err(char* msg, int code) {
  fprintf(stderr, "ERR: %s.\nReturn code: %d\nExiting.\n", msg, code);
}

void connlost(void* context, char* cause) {
  MQTTAsync client = (MQTTAsync) context;
  MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
  int rc;

  opts.cleansession = 1;
  opts.keepAliveInterval = 20;

  fprintf(stderr, "Connection lost.");
  if (cause) fprintf(stderr, " Reason: %s\n", cause);
  puts("Reconnecting.");

  if ((rc = MQTTAsync_connect(client, &opts)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not restart connection process", rc);
    set_fin();
  }
}

void on_disconnect(void* context, MQTTAsync_successData* res) {
  puts("Disconnected.");
  set_fin();
}

void on_disconnect_failure(void* context, MQTTAsync_failureData* res) {
  print_mqtt_err("Could not disconnect gracefully", res->code);
  set_fin();
}

void on_send_failure(void* context, MQTTAsync_failureData* res) {
  MQTTAsync client = (MQTTAsync) context;
  MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
  int rc;

  opts.onSuccess = on_disconnect;
  opts.onFailure = on_disconnect_failure;
  opts.context = client;

  if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not begin disconnection process", rc);
    exit(EXIT_FAILURE); // TODO: Proper cleanup
  }
}

void on_send(void* context, MQTTAsync_successData* res) {
  MQTTAsync client = (MQTTAsync) context;
  MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
  int rc;

  printf("Message with token value %d delivery confirmed.\n", res->token);
  opts.onSuccess = on_disconnect;
  opts.onFailure = on_disconnect_failure;
  opts.context = client;
  if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS) { 
    print_mqtt_err("Could not begin disconnection process", rc);
    exit(EXIT_FAILURE);
  }
}

void on_connect(void* context, MQTTAsync_successData* res) {
  MQTTAsync client = (MQTTAsync) context;
  MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
  MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
  int rc;

  puts("Connected.");
  opts.onSuccess = on_send;
  opts.onFailure = on_send_failure;
  opts.context = client;

  pubmsg.payload = MSG;
  pubmsg.payloadlen = (int) strlen(MSG);
  pubmsg.qos = QOS;
  pubmsg.retained = 0; // TODO: Read docs on this.
  
  if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not start sending process", rc);
    exit(EXIT_FAILURE);
  }
}

void on_connect_failure(void* context, MQTTAsync_failureData* res) {
  print_mqtt_err("Could not connect to broker", (res) ? res->code : 0);
  set_fin();
}

int arrived(void* context, char* topic, int len, MQTTAsync_message* m) { return 1; }

int main(int argc, char** argv) {
  MQTTAsync client;
  MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
  int rc;

  if ((rc = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not create client", rc);
    exit(EXIT_FAILURE);
  }
  
  puts("Created MQTT client. Setting callbacks.");
  if ((rc = MQTTAsync_setCallbacks(client, client, connlost, arrived, NULL)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not set callbacks on client", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }

  puts("Callbacks configured. Connecting.");
  opts.keepAliveInterval = 20;
  opts.cleansession = 1;
  opts.onSuccess = on_connect;
  opts.onFailure = on_connect_failure;
  opts.context = client;
  if ((rc = MQTTAsync_connect(client, &opts)) != MQTTASYNC_SUCCESS) {
    print_mqtt_err("Could not begin connection process", rc);
    MQTTAsync_destroy(&client);
    exit(EXIT_FAILURE);
  }

  puts("Publishing message.");
  while (!get_fin())
    usleep(TIMEOUT);

  MQTTAsync_destroy(&client);
  return 0;
}
