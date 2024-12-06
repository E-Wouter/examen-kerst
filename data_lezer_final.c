#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>

#define ADDRESS     "tcp://192.168.0.152:1883"
#define CLIENTID    "cara"
#define TOPIC       "P1/DM6"
#define QOS         1
#define TIMEOUT     10000L

#define DATE_TIME_LEN 18

char *payload;
int aantal_dagen = 0;

struct meter_data{
    char datum_tijd_stroom[DATE_TIME_LEN];
    int tarief_indicator; //unused
    float actueel_stroomverbruik; //unused
    float actueel_spanning; //unused
    float totaal_dagverbruik;
    float totaal_nachtverbruik;
    float totaal_dagopbrengst;
    float totaal_nachtopbrengst;
    char datum_tijd_gas[DATE_TIME_LEN];
    float totaal_gasverbruik;

};

void delivered(void *context, MQTTClient_deliveryToken dt) { }

void process_existing_data() {
    FILE *file = fopen("output.txt", "r");
    if (file == NULL) {
        printf("No existing data to process.\n");
        return;
    }

    int line;
    while((aantal_dagen) > 0) {
        char datum_tijd_stroom[DATE_TIME_LEN];
        int tarief_indicator; //unused
        float actueel_stroomverbruik; //unused
        float actueel_spanning; //unused
        float totaal_dagverbruik;
        float totaal_nachtverbruik;
        float totaal_dagopbrengst;
        float totaal_nachtopbrengst;
        char datum_tijd_gas[DATE_TIME_LEN];
        float totaal_gasverbruik;
        fscanf(payload,"%[^;];%f;%f;%f;%f;%[^';'];%f",
        datum_tijd_stroom[DATE_TIME_LEN], totaal_dagverbruik, totaal_nachtverbruik, totaal_dagopbrengst, totaal_nachtopbrengst, datum_tijd_gas[DATE_TIME_LEN], totaal_gasverbruik );
        printf("%*s\n", 10, "-");
        printf("STROOM:\n");
        printf("\t\t Totaal verbruik \t = \t %f kWh\n", totaal_dagverbruik+totaal_nachtverbruik);
        printf("\t\t Totaal opbrengst \t = \t %f kWh\n", totaal_dagopbrengst+totaal_nachtopbrengst);
        printf("GAS:\n");
        printf("\t\t Totaal verbruik \t = \t %f kWh\n", totaal_gasverbruik*11.55);
        aantal_dagen -= 1;
    }
    fclose(file);
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    aantal_dagen += 1;
    payload = (char *)message->payload;

    char datum_tijd_stroom[DATE_TIME_LEN];
    int tarief_indicator; //unused
    float actueel_stroomverbruik; //unused
    float actueel_spanning; //unused
    float totaal_dagverbruik;
    float totaal_nachtverbruik;
    float totaal_dagopbrengst;
    float totaal_nachtopbrengst;
    char datum_tijd_gas[DATE_TIME_LEN];
    float totaal_gasverbruik;

    struct meter_data m;

    sscanf(payload,"%[^;];%d;%f;%f;%f;%f;%f;%f;%[^';'];%f",
     datum_tijd_stroom[DATE_TIME_LEN], tarief_indicator, actueel_stroomverbruik, actueel_spanning, totaal_dagverbruik, totaal_nachtverbruik, totaal_dagopbrengst, totaal_nachtopbrengst, datum_tijd_gas[DATE_TIME_LEN], totaal_gasverbruik );
    printf(payload);

    m.datum_tijd_stroom[DATE_TIME_LEN] = datum_tijd_stroom[DATE_TIME_LEN];
    m.tarief_indicator = tarief_indicator;
    m.actueel_stroomverbruik = actueel_stroomverbruik;
    m.actueel_spanning = actueel_spanning;
    m.totaal_dagverbruik = totaal_dagverbruik;
    m.totaal_nachtverbruik = totaal_nachtverbruik;
    m.totaal_dagopbrengst = totaal_dagopbrengst;
    m.totaal_nachtopbrengst = totaal_nachtopbrengst;
    m.datum_tijd_gas[DATE_TIME_LEN] = datum_tijd_stroom[DATE_TIME_LEN];
    m.totaal_gasverbruik;

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    printf("Datum: %s, TOT_dag: %f\n",
     datum_tijd_stroom, totaal_dagverbruik, totaal_nachtverbruik, totaal_dagopbrengst, totaal_nachtopbrengst);
    return 1;

    FILE *file = fopen("output.txt", "a");
    if (file != NULL) {
        fprintf(file, "%s;%f;%f;%f;%f;%s;%f\n", datum_tijd_stroom, totaal_dagverbruik, totaal_nachtverbruik, totaal_dagopbrengst, totaal_nachtopbrengst);
        fclose(file);
        return 1;
    }
    else{
        printf("error, could not open file!");
    }
    return(aantal_dagen);
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    struct meter_data m;
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

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

    int lcount = 20;
    int scount = 10;

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    printf("%*s", lcount, "+");
    printf("Elektriciteit- en gas verbruik - totalen per dag\n");
    printf("%*s\n\n", lcount, "+");
    printf("STARTWAARDEN\n\n");


    printf("%*s", lcount, "-");
    printf("TOTALEN:");
    printf("%*s", lcount, "-");

    /*while((aantal_dagen) != 0){
        aantal_dagen -= 1;
        printf("%-*s\n", 10, "-");
        printf("STROOM:\n");
        printf("\t\t Totaal verbruik \t = \t %f kWh\n", m.totaal_dagverbruik+m.totaal_nachtverbruik);
        printf("\t\t Totaal opbrengst \t = \t %f kWh\n", m.totaal_dagopbrengst+m.totaal_nachtopbrengst);
        printf("GAS:\n");
        printf("\t\t Totaal verbruik \t = \t %f kWh\n", m.totaal_gasverbruik*11.55);

    }*/
    process_existing_data();

    printf("%*s", lcount, "+");
    printf("Einde van dit rapport\n");
    printf("%*s", lcount, "+");

    return EXIT_SUCCESS;
}
