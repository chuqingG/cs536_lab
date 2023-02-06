// Server side C/C++ program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#define MAX 32
#define PORT 12000
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static _Atomic unsigned int cli_count = 0;
static int uid = 10;

typedef struct{
	struct sockaddr_in address;
	int fd;
	int uid;
	// char name[32];
} client_t;
client_t *clients[MAX];

// maybe useless...
int Inqueue(client_t *cl){
	pthread_mutex_lock(&clients_mutex);
	int i;
	for(i = 0; i < MAX; ++i)
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	pthread_mutex_unlock(&clients_mutex);
	return i;
}

void Dequeue(){
	pthread_mutex_lock(&clients_mutex);
	for(int i=0; i < MAX; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

void *handle_connection(client_t *cli){
	// Get client information from socket 
	struct sockaddr_in cli_addr;
	int len = sizeof(cli_addr);
	if(getpeername(cli->fd, (struct sockaddr *)&cli_addr, &len)){
		perror("get client information");
		exit(EXIT_FAILURE);
	}
	char *addr_buf = malloc(INET6_ADDRSTRLEN);
	char *port_buf = malloc(10);
    memset(addr_buf, 0, INET6_ADDRSTRLEN);
	memset(port_buf, 0, 10);

    if((inet_ntop(cli_addr.sin_family, &cli_addr.sin_addr, addr_buf, INET6_ADDRSTRLEN)) == NULL){
    	fprintf(stderr,"convert client information failed, fd=%d\n", cli->fd);        
    	exit(EXIT_FAILURE);
    }
	sprintf(port_buf, "%d", (int)ntohs(cli_addr.sin_port));

	//build the response message
	char header[30] = "message-from-client:";
	char cli_msg[1024] = { 0 };
	char response_msg[1024] = { 0 };
	int idx = strlen(header) + strlen(addr_buf) + strlen(port_buf) + 2; // for , and \n

	int valread = read(cli->fd, cli_msg, 1024);
	
	memcpy(response_msg, header, strlen(header) * sizeof(char));
	memcpy(response_msg + strlen(header), addr_buf, strlen(addr_buf) * sizeof(char));
	memcpy(response_msg + strlen(header) + strlen(addr_buf) + 1, port_buf, strlen(port_buf) * sizeof(char));
	response_msg[strlen(header) + strlen(addr_buf)] = ',';
	response_msg[strlen(header) + strlen(addr_buf) + strlen(port_buf) + 1] = '\n';
	memcpy(response_msg + idx, cli_msg, sizeof(cli_msg));

	// send reponse
	printf("%s\n", response_msg);
	// send(cli->fd, response_msg, strlen(response_msg), 0);

	while(send(cli->fd, response_msg, 1, 0) >= 0){
		// polling for closing connection
	}
	// closing the connected socket
	printf("close-client:%s,%s\n", addr_buf, port_buf);
	
	pthread_mutex_lock(&clients_mutex);
	close(cli->fd);
	pthread_mutex_lock(&clients_mutex);
	Dequeue(cli->uid);
    pthread_detach(pthread_self());
	free(cli);
	free(addr_buf);
	free(port_buf);
    return NULL;
}


int main(int argc, char const* argv[])
{
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	pthread_t tid;

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 12000
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
				sizeof(opt))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(atoi(argv[1]));

	// Forcefully attaching socket to the port 12000
	if (bind(server_fd, (struct sockaddr*)&address,
			sizeof(address))
		< 0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	if (listen(server_fd, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}
	printf("The server is ready to receive\n");
	while (1){
		if ((new_socket
			= accept(server_fd, (struct sockaddr*)&address,
			(socklen_t*)&addrlen)) < 0){
			perror("accept");
			exit(EXIT_FAILURE);
		}

		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = address;
		cli->fd = new_socket;
		cli->uid = uid++;
		int cli_idx = Inqueue(cli);
		pthread_create(&tid, NULL, handle_connection, cli);
		// sleep(1);
		
	}
	// closing the listening socket
	shutdown(server_fd, SHUT_RDWR);
	return 0;
}
