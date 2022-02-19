# PSPD - 2021/2 - Laboratório 2 (TCP/UDP)

## Integrantes

- João Luis Baraky - 18/0033646
- Rhuan Carlos Queiroz - 180054848

## Execução

Para executar o programa, é necessário primeiro compilar os arquivos:

`$ gcc server.c cJSON.c -o server`

`$ gcc client.c cJSON.c -o client -lm -pthread`

Depois disso, basta rodar o(s) servidor(es) e depois o(s) cliente(s):

`$ ./server <ip>:<porta>`

`$ ./client <ip_servidor>:<porta_servidor> [<ip_servidor_n>:<porta_servidor_n>]` 

## Utilização

A utilização é de forma automática, sem interação com o usuário

