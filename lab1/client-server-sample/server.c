// Server side C/C++ program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>

#define PORT 12000
int main(int argc, char const* argv[])
{
	int server_fd, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	// Forcefully attaching socket to the port 12000
	if (setsockopt(server_fd, SOL_SOCKET,
				SO_REUSEADDR, &opt,
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

		// Get client information from socket 
		struct sockaddr_in cli_addr;
		int len = sizeof(cli_addr);
		if(getpeername(new_socket, (struct sockaddr *)&cli_addr, &len)){
			perror("get client information");
			exit(EXIT_FAILURE);
		}
		char *addr_buf = malloc(INET6_ADDRSTRLEN);
		char *port_buf = malloc(10);
        memset(addr_buf, 0, INET6_ADDRSTRLEN);
		memset(port_buf, 0, 10);
    
        if((inet_ntop(cli_addr.sin_family, &cli_addr.sin_addr, addr_buf, INET6_ADDRSTRLEN)) == NULL){
        	fprintf(stderr,"convert client information failed, fd=%d\n", new_socket);        
        	exit(EXIT_FAILURE);
        }
		// printf( "ip：%s\n", addr_buf);
		sprintf(port_buf, "%d", (int)ntohs(cli_addr.sin_port));
		// itoa((int)ntohs(cli_addr.sin_port), port_buf, 10);
		// printf( "port：%s\n", port_buf);
		
		char header[30] = "message-from-client:";
		char cli_msg[1024] = { 0 };
		char response_msg[1024] = { 0 };
		int idx = strlen(header) + strlen(addr_buf) + strlen(port_buf) + 2; // for , and \n

		valread = read(new_socket, cli_msg, 1024);

		//build the response message
		memcpy(response_msg, header, strlen(header) * sizeof(char));
		memcpy(response_msg + strlen(header), addr_buf, strlen(addr_buf) * sizeof(char));
		memcpy(response_msg + strlen(header) + strlen(addr_buf) + 1, port_buf, strlen(port_buf) * sizeof(char));
		response_msg[strlen(header) + strlen(addr_buf)] = ',';
		response_msg[strlen(header) + strlen(addr_buf) + strlen(port_buf) + 1] = '\n';
		memcpy(response_msg + idx, cli_msg, sizeof(cli_msg));

		printf("%s\n", response_msg);
		send(new_socket, response_msg, strlen(response_msg), 0);
		
		while(send(new_socket, response_msg, 1, 0) >= 0){
			//polling
		}
		// closing the connected socket
		printf("close-client:%s,%s\n", addr_buf, port_buf);
		close(new_socket);
	}
	// closing the listening socket
	shutdown(server_fd, SHUT_RDWR);
	return 0;
}
