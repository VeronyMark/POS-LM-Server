#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "Simulacia.h"
#include "buffer.h"


typedef struct sprava {
    int row;
    int column;
    int numberAnts;
    int numberSteps;
    int type;
    int logic;
    int solutions;
} SPRAVA;

GENERATE_BUFFER( SPRAVA , sprava)

typedef struct thread_data {

    struct buffer_sprava buf;

    pthread_mutex_t mutex;
    pthread_cond_t is_full;
    pthread_cond_t is_empty;

    struct Simulacia s;
    short port;
} THREAD_DATA;

void thread_data_init(struct thread_data* data, int buffer_capacity,
                      short port) {
    buffer_sprava_init(&data->buf, buffer_capacity);
    pthread_mutex_init(&data->mutex, NULL);
    pthread_cond_init(&data->is_full, NULL);
    pthread_cond_init(&data->is_empty, NULL);
    data->port = port;
}

void thread_data_destroy(struct thread_data* data) {
    buffer_sprava_destroy(&data->buf);
    pthread_mutex_destroy(&data->mutex);
    pthread_cond_destroy(&data->is_full);
    pthread_cond_destroy(&data->is_empty);
    data->port = 0;
}


void Sprava_serialize(const struct Plocha *plocha, char *output) {
    int i;
   // printf("som v sprava serialize");
    snprintf(output, 1024, "\n");
    for (i = 0; i < (plocha->sirka * 2) - 1; i++) {
        snprintf(output + strlen(output), 1024 - strlen(output), "-");
    }
    snprintf(output + strlen(output), 1024 - strlen(output), "\n");

    for (i = 0; i < plocha->sirka * plocha->vyska; i++) {
        if (i > 0 && i % plocha->sirka == 0) {
            snprintf(output + strlen(output), 1024 - strlen(output), "\n");
        }
        if (i % plocha->sirka != 0) {
            snprintf(output + strlen(output), 1024 - strlen(output), "|");
        }

        int cislo = getFarba(&plocha->plocha[i]);
        snprintf(output + strlen(output), 1024 - strlen(output), "%d", cislo);
    }

    snprintf(output + strlen(output), 1024 - strlen(output), "\n");
    for (i = 0; i < (plocha->sirka * 2) - 1; i++) {
        snprintf(output + strlen(output), 1024 - strlen(output), "-");
    }
    //printf("koncim serialize");

}

void tryVypisPlochu(const struct Plocha *plocha, char *output) {
    //printf("SOM V TRY VYPIS PLOCHU");
    Sprava_serialize(plocha, output);
}

_Bool try_deserializate(SPRAVA *spravaOdKlienta, char *buf) {
    if (sscanf(buf, "%d;%d;%d;%d;%d;%d;%d",
               &spravaOdKlienta->row,
               &spravaOdKlienta->column,
               &spravaOdKlienta->numberAnts,
               &spravaOdKlienta->numberSteps,
               &spravaOdKlienta->type,
               &spravaOdKlienta->logic,
               &spravaOdKlienta->solutions) == 7) {
        return true;
    }
    return false;
}


//prijatie
void* process_client_data(void* thread_data) {
    struct thread_data* data = (struct thread_data*)thread_data;

    struct sprava clientSprava;

    char buffer[1024];

    bzero(buffer, 256);
    int n = read(data->port, buffer, 255);

    if (n < 0) {
        perror("Error reading from socket");
    }
    printf("Here is the message: %s\n", buffer);

    if (try_deserializate(&clientSprava, &buffer)) {

        simuluj(&data->s, clientSprava.row, clientSprava.column, clientSprava.numberAnts, clientSprava.numberSteps,
                clientSprava.type, clientSprava.logic, clientSprava.solutions);

    } else {
        printf("Nedokazali sme identifikovat parametre");
    }
}

void* consume(void* thread_data) {
    struct thread_data *data = (struct thread_data *) thread_data;

    char outputBuffer[1024];
    tryVypisPlochu(&data->s.plocha, outputBuffer);

    //const char* msg = "I got your message";
    write(data->port, outputBuffer, strlen(outputBuffer));


}



int main(int argc, char *argv[]) {
    int sockfd, newsockfd;
    socklen_t cli_len;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "usage %s port\n", argv[0]);
        return 1;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return 1;
    }

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket address");
        return 2;
    }

    listen(sockfd, 5);
    cli_len = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
    if (newsockfd < 0) {
        perror("ERROR on accept");
        return 3;
    }

    pthread_t th_listen;
    THREAD_DATA data;
    thread_data_init(&data,7, newsockfd);
    //active_socket_init(&my_socket);

    pthread_create(&th_listen, NULL, process_client_data, &data);
    pthread_join(th_listen, NULL);

    consume(&data);
    vycisti(&data.s);
    close(sockfd);
    thread_data_destroy(&data);
}