#include "cJSON.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#define SIZE 10


typedef struct result {
    float minor;
    float major;
} result;

typedef struct worker {
    char host[16];
    char port[6];
    float *v;
    int n;
} worker;

worker str_to_worker(char data[]) {
    worker w;
    strcpy(w.host, strtok(data, ":"));
    strcpy(w.port, strtok(NULL, ":"));

    return w;
}

worker* get_workers(char * v[], int n) {
    worker *w = (worker*) malloc(sizeof(worker) * n);

    for (int i = 0; i < n; i++) {
        w[i] = str_to_worker(v[i]);
    }

    return w;
}

cJSON *build_array(float v[], int n) {
    cJSON *json = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    cJSON *num;

    for (int i = 0; i < n; i++) {
        num = cJSON_CreateNumber(v[i]);
        cJSON_AddItemToArray(arr, num);
    }
    cJSON_AddItemToObject(json, "array", arr);
    return json;
}

result parse_response(char response[]) {
    result res;
    cJSON *json = cJSON_Parse(response);
    res.minor = cJSON_GetObjectItem(json, "minor")->valuedouble;
    res.major = cJSON_GetObjectItem(json, "major")->valuedouble;

    return res;
}

result get_major_minor(worker w) {
	struct  sockaddr_in server;
	int sd;
    char *request = cJSON_PrintUnformatted(build_array(w.v, w.n));
    char response[50];

    memset(response, '\0', sizeof(response));
	memset((char *)&server,0,sizeof(server));
	
	server.sin_family      = AF_INET;
	server.sin_addr.s_addr = inet_addr(w.host);
	server.sin_port        = htons(atoi(w.port));

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0) {
		fprintf(stderr, "Criacao do socket falhou!\n");
		exit(1); }

	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) < 0) {
		fprintf(stderr,"Tentativa de conexao falhou!\n");
		exit(1); }

    send(sd,request,strlen(request),0);
    recv(sd,response,sizeof(response),0);
	close (sd);

    result res = parse_response(response);
    return res;
}

void* get_major_minor_thread(void* arg) {
    worker * w = (worker*) arg;
    result * res = (result*)malloc(sizeof(result));
    result r = get_major_minor(*w);
    res->minor = r.minor;
    res->major = r.major;

    pthread_exit(res);
}

int main(int argc,char * argv[]) {
    int n;
    int hosts_size = argc - 1;

    float v[SIZE];

	for (int i = 0; i < SIZE; i++) {
		v[i] = (i - (SIZE/2)) * (i - (SIZE/2));
	}

    for (int i = 0; i < SIZE; i++) {
        v[i] = sqrt(v[i]);
    }

	
  	if(argc<2)  {
        printf("Uso correto: %s <host>:<port> [ ... <host>:<port> ]\n", argv[0]);
        exit(1);
    }

    worker * w = get_workers(argv + 1, hosts_size);


    int items_per_thread = SIZE / hosts_size;
    int items_left = SIZE % hosts_size;

    for (int i = 0; i < hosts_size; i++) {
        if (i == hosts_size - 1)
            w[i].n = items_per_thread + items_left;
        else
            w[i].n = items_per_thread;

        w[i].v = (float*)malloc(sizeof(float) * w[i].n);

        for( int j = 0; j < w[i].n; j++)
            w[i].v[j] = v[i * items_per_thread + j];
    }

    pthread_t * threads = malloc(sizeof(pthread_t) * hosts_size);

    for (int i = 0; i < hosts_size; i++) {
        pthread_create(&threads[i], NULL, get_major_minor_thread, &w[i]);
    }

    result **results = (result**)malloc(sizeof(result*) * hosts_size);
    for (int i = 0; i < hosts_size; i++) {
        pthread_join(threads[i], (void **) &results[i]);
    }

    worker final_worker;

    final_worker.n = hosts_size*2;
    final_worker.v = (float*)malloc(sizeof(worker) * (hosts_size*2));
    strcpy(final_worker.host, w[0].host);
    strcpy(final_worker.port, w[0].port);

    for(int i = 0; i < hosts_size; i++) {
        final_worker.v[i*2] = results[i]->minor;
        final_worker.v[i*2 + 1] = results[i]->major;
    }

    result res = get_major_minor(final_worker);
    printf("$ minor: %f major: %f\n", res.minor, res.major);


    free(final_worker.v);
    free(threads);
    free(results);

    for (int i = 0; i < hosts_size; i++) {
        free(w[i].v);
    }
    free(w);

	return (0);
}

