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

void arg_parser(char* url, char* port_str, char* addr, char* path){
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
	}

	status = regexec(&port_reg, url, nmatch, &port_pm, 0);
	if(status == REG_NOMATCH){
		perror("invalid url");
		exit(EXIT_FAILURE);
	} else if (status == 0){
        strncpy(port_str, url + port_pm.rm_so + 1, port_pm.rm_eo - port_pm.rm_so - 1);
        port_str[port_pm.rm_eo - port_pm.rm_so - 1] = '\0';
	}
	strcpy(path, url + port_pm.rm_eo);
	regfree(&ip_reg);
	regfree(&port_reg);
	return;
}

void send_get(int fd, char* path, char* ip){
    char send_buf[256] = {0};
 
    int send_len = sprintf(send_buf,
						"GET %s HTTP/1.1\r\n"
						"Host: %s\r\n\r\n", path, ip);
	send(fd, send_buf, send_len, 0);
	printf("Send request:\n%s", send_buf);
}

int main(int argc, char const* argv[])
{	
	char ipaddr[20] = {0};
	char port[10] = {0};
	char path[30] = {0};
	arg_parser(argv[1], port, ipaddr, path);
	int sock = 0, client_fd;
	struct sockaddr_in serv_addr;

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

	send_get(sock, path, ipaddr);
	int valread = read(sock, receive_msg, 1024);
	printf("Get response:\n%s", receive_msg);
	
	// closing the connected socket
	sleep(10);
	close(client_fd);
	return 0;
}
