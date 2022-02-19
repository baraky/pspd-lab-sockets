#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "cJSON.h"

#define QLEN 5  
#define MAX_SIZE 500 

typedef struct response {
    float major;
    float minor;
} response;

typedef struct request {
    float *v;
    int n;
} request;

typedef struct server_info {
    char host[16];
    char port[6];
} server_info;

request parse_request(char *message) {
    request rq;
    cJSON *json = cJSON_Parse(message);
    cJSON *arr = cJSON_GetObjectItem(json, "array");
    rq.n = cJSON_GetArraySize(arr);
    rq.v = (float*)malloc(sizeof(float) * rq.n);

    for (int i = 0; i < rq.n; i++) {
        rq.v[i] = cJSON_GetArrayItem(arr, i)->valuedouble;
    }

    return rq;
}

cJSON* build_response(response rs) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "minor", cJSON_CreateNumber(rs.minor));
    cJSON_AddItemToObject(json, "major", cJSON_CreateNumber(rs.major));
    return json;
}

response get_minor_major(request rq) {
    response rs;
    float min, max;
    min = max = rq.v[0];
    for (int i = 1; i < rq.n; i++) {
        if (rq.v[i] < min) {
            min = rq.v[i];
        }
        if (rq.v[i] > max) {
            max = rq.v[i];
        }
    }

    rs.minor = min;
    rs.major = max;

    return rs;
}

int handle_request(int sd, struct sockaddr_in client)  {
   char buffin[MAX_SIZE];
   char* buffout;
   response rs;
   request rq;

   memset(&buffin, 0x0, sizeof(buffin));

   recv(sd, &buffin, sizeof(buffin),0);
   printf("[%s:%u] => %s\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port), buffin);

   rq = parse_request(buffin);
   rs = get_minor_major(rq);
   buffout = cJSON_PrintUnformatted(build_response(rs));
   send(sd, buffout, strlen(buffout), 0);

   printf("Encerrando conexao com %s:%u ...\n\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
   close (sd);
   free(rq.v);
 }

int setup_server(server_info srv) {
   struct sockaddr_in server;  /* endereco do servidor   */
   int sd;

   memset((char *)&server,0,sizeof(server)); /* limpa variavel server    */
   server.sin_family 		= AF_INET;           	/* familia TCP/IP   */
   server.sin_addr.s_addr 	= inet_addr(srv.host); 	/* endereco IP      */
   server.sin_port 		= htons(atoi(srv.port)); /* PORTA	    */

   /* Cria socket */
   sd = socket(AF_INET, SOCK_STREAM, 0);
   if (sd < 0) {
     fprintf(stderr, "Falha ao criar socket!\n");
     exit(1); }

   /* liga socket a porta e ip */
   if (bind(sd, (struct sockaddr *)&server, sizeof(server)) < 0) {
     fprintf(stderr,"Ligacao Falhou!\n");
     exit(1); }

   /* Ouve porta */
   if (listen(sd, QLEN) < 0) {
     fprintf(stderr,"Falhou ouvindo porta!\n");
     exit(1); }

   printf("\t\t| Servidor ouvindo no IP %s porta %s | \n\n", srv.host, srv.port);

   return sd;
}

void start_server(int sd) {
   struct sockaddr_in client;
   int cli_sd;
   int socklen = sizeof(client);

   while (1) {
        if ( (cli_sd=accept(sd, (struct sockaddr *)&client, &socklen)) < 0) {
		    fprintf(stdout, "Falha na conexao\n");
		    exit(1);
        }

        fprintf(stdout, "Cliente %s: %u conectado.\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port)); 
        handle_request(cli_sd, client);
   } 
}

server_info get_server_info(char data[]) {
    server_info server;
    strcpy(server.host, strtok(data, ":"));
    strcpy(server.port, strtok(NULL, ":"));

    return server;
}

int main(int argc, char *argv[]) {
   int sd;

    if (argc != 2) {
        printf("Uso correto: %s <host>:<port>", argv[0]);
        exit(1);
    }

    sd = setup_server(get_server_info(argv[1]));

    start_server(sd);

    close(sd);
}

