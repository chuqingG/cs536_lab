// Client side C/C++ program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>
#define PORT 12000

void arg_parser(char* url, char* port_str, char* addr){
	int status;
	int cflags = REG_EXTENDED | REG_NEWLINE;
	regmatch_t ip_pm, port_pm;
	const size_t nmatch = 1;
	regex_t ip_reg, port_reg;
	const char *ip_pattern = "[0-9]+(\\.[0-9]+)+";
	const char *port_pattern = ":[0-9]+";
	regcomp(&ip_reg, ip_pattern, cflags);
	regcomp(&port_reg, port_pattern, cflags);

	status = regexec(&ip_reg, url, nmatch, &ip_pm, 0);
	if(status == REG_NOMATCH){
		perror("invalid url");
		exit(EXIT_FAILURE);
	} else if (status == 0){
        strncpy(addr, url + ip_pm.rm_so, ip_pm.rm_eo - ip_pm.rm_so);
        addr[ip_pm.rm_eo - ip_pm.rm_so] = '\0';
		printf("%s\n", addr);
	}

	status = regexec(&port_reg, url, nmatch, &port_pm, 0);
	if(status == REG_NOMATCH){
		perror("invalid url");
		exit(EXIT_FAILURE);
	} else if (status == 0){
        strncpy(port_str, url + port_pm.rm_so + 1, port_pm.rm_eo - port_pm.rm_so - 1);
        port_str[port_pm.rm_eo - port_pm.rm_so - 1] = '\0';
		printf("%s\n", port_str);
	}
	regfree(&ip_reg);
	regfree(&port_reg);
	return;
}

int main(int argc, char const* argv[])
{	
	char ipaddr[20] = {0};
	char port[10] = {0};
	arg_parser(argv[1], port, ipaddr);
	int sock = 0, client_fd;
	struct sockaddr_in serv_addr;
	// printf("Input lowercase sentence:\n");
	// char sentence[256];
	// fgets(sentence, sizeof(sentence), stdin);
	char receive_msg[1024] = { 0 };
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	// serv_addr.sin_addr.s_addr = inet_addr("10.145.21.35");
	serv_addr.sin_port = htons(atoi(port));

	// Convert IPv4 and IPv6 addresses from text to binary
	// form
	if (inet_pton(AF_INET, ipaddr, &serv_addr.sin_addr)
		<= 0) {
		printf(
			"\nInvalid address/ Address not supported \n");
		return -1;
	}

	if ((client_fd
		= connect(sock, (struct sockaddr*)&serv_addr,
				sizeof(serv_addr)))
		< 0) {
		printf("\nConnection Failed \n");
		return -1;
	}

	char sentence[256];
	strcpy(sentence, argv[3]); //message
	send(sock, argv[3], strlen(argv[3]), 0);
	int valread = read(sock, receive_msg, 1024);
	printf("%s\n", receive_msg);
	
	// closing the connected socket
	sleep(10);
	close(client_fd);
	return 0;
}
