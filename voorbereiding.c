#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>

#define ADRESS "tcp://192.168.1.27:1883"
#define MAX_NUMBERS 1000 // Maximum number of numbers to store

// Struct to store numbers and their statistics
typedef struct {
    int count;
    double numbers[MAX_NUMBERS];
    double min;
    double max;
    double avg;
} Stats;

void calculate_stats(Stats *stats) {
    if (stats->count == 0) return;

    stats->min = stats->numbers[0];
    stats->max = stats->numbers[0];
    double sum = 0.0;
    for (int i = 0; i < stats->count; ++i) {
        if (stats->numbers[i] < stats->min) stats->min = stats->numbers[i];
        if (stats->numbers[i] > stats->max) stats->max = stats->numbers[i];
        sum += stats->numbers[i];
    }
    stats->avg = sum / stats->count;
}

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *msg) {
    Stats *stats = (Stats *)userdata;
    char *token;
    char *payload_copy = strdup((char *)msg->payload);

    // Extract number from payload
    token = strtok(payload_copy, ";");
    token = strtok(NULL, ";"); // Second token is the number
    if (token) {
        double number = atof(token);
        if (stats->count < MAX_NUMBERS) {
            stats->numbers[stats->count++] = number;
            calculate_stats(stats);
            FILE *file = fopen("numbers.txt", "a");
            if (file) {
                fprintf(file, "%lf\n", number);
                fclose(file);
            }
        }
    }
    free(payload_copy);
}

int main(int argc, char *argv[]) {
    struct mosquitto *mosq;
    Stats stats = {0};

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, &stats);
    if (!mosq) {
        fprintf(stderr, "Error: Could not create Mosquitto instance\n");
        return 1;
    }

    mosquitto_message_callback_set(mosq, message_callback);

    if (mosquitto_connect(mosq, ADRESS, 1883, 60) != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Error: Could not connect to Broker\n");
        return 1;
    }

    mosquitto_subscribe(mosq, NULL, "your_topic", 0);

    mosquitto_loop_start(mosq);

    printf("Press Enter to quit...\n");
    getchar();

    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    printf("Min: %lf\n", stats.min);
    printf("Max: %lf\n", stats.max);
    printf("Avg: %lf\n", stats.avg);

    return 0;
}
