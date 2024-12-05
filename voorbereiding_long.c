#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>

#define ADDRESS     "tcp://192.168.1.27:1883"
#define CLIENTID    "cara"
#define TOPIC       "ewoud"
#define QOS         1
#define TIMEOUT     10000L

typedef struct {
    char device[50];
    long double min;
    long double max;
    long double sum;
    long long count;
} DeviceStats;

DeviceStats devices[100]; // Assuming a maximum of 100 different devices
int device_count = 0;

void delivered(void *context, MQTTClient_deliveryToken dt) { }

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    char *payload = (char *)message->payload;
    char device[50];
    long double number;

    sscanf(payload, "%[^;];%Lf", device, &number);

    FILE *file;
    int device_index = -1;
    for (int i = 0; i < device_count; i++) {
        if (strcmp(devices[i].device, device) == 0) {
            device_index = i;
            break;
        }
    }

    if (device_index == -1) {
        device_index = device_count++;
        strcpy(devices[device_index].device, device);
        devices[device_index].min = number;
        devices[device_index].max = number;
        devices[device_index].sum = number;
        devices[device_index].count = 1;
    } else {
        if (number < devices[device_index].min) devices[device_index].min = number;
        if (number > devices[device_index].max) devices[device_index].max = number;
        devices[device_index].sum += number;
        devices[device_index].count += 1;
    }

    file = fopen(device, "a");
    if (file != NULL) {
        fprintf(file, "Device: %s, Number: %Lf\n", device, number);
        fclose(file);
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    if (MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL) != MQTTCLIENT_SUCCESS) {
        printf("Failed to create client\n");
        return EXIT_FAILURE;
    }
    if ((rc = MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to set callbacks, return code %d\n", rc);
        return EXIT_FAILURE;
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        return EXIT_FAILURE;
    }

    if ((rc = MQTTClient_subscribe(client, TOPIC, QOS)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to subscribe, return code %d\n", rc);
        return EXIT_FAILURE;
    }

    printf("Press 's' to stop the program\n");
    char c;
    while ((c = getchar()) != 's') { }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    for (int i = 0; i < device_count; i++) {
        printf("Device: %s\n", devices[i].device);
        printf("Minimum: %Lf\n", devices[i].min);
        printf("Maximum: %Lf\n", devices[i].max);
        printf("Average: %Lf\n", devices[i].sum / devices[i].count);
        printf("\n");
    }

    return EXIT_SUCCESS;
}
