// Client side C/C++ program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <regex.h>
#include <pthread.h>
#include <sys/types.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <arpa/inet.h>

pthread_mutex_t requests_mutex = PTHREAD_MUTEX_INITIALIZER;
static _Atomic unsigned int req_count = 0;
#define PORT 12000
#define MAX 32
#define HTTP2
#define MULTSOCK

typedef struct{
	int uid;
	char src[128];
	char server_ip[32];
	int sock;
} request_pack;
request_pack *requests[MAX];

int recv_line(int fd,char *buf, int size){
	//return the current idx 
	int i = 0;
	char ch;
	for(i = 0;i < size;++i){
		int n = recv(fd, &ch, 1, 0);
		if(n == 1){
			buf[i] = ch;
			if(ch == '\n') 
				break;
		} else{
			return i;
		}
	}
	return i + 1;
}


int Inqueue(request_pack *rq){
	pthread_mutex_lock(&requests_mutex);
	int i;
	for(i = 0; i < MAX; ++i)
		if(!requests[i]){
			requests[i] = rq;
			requests[i]->uid = req_count++;
			break;
		}
	pthread_mutex_unlock(&requests_mutex);
	return i;
}

void Dequeue(int uid){
	pthread_mutex_lock(&requests_mutex);
	for(int i=0; i < MAX; ++i){
		if(requests[i]){
			if(requests[i]->uid == uid){
				requests[i] = NULL;
				break;
			}
		}
	}
	pthread_mutex_unlock(&requests_mutex);
}

int html_source_parser(const char* html){
	// printf("The return value:\n%s\n", html);
	int status;
	int cflags = REG_EXTENDED | REG_NEWLINE;
	regmatch_t pm;
	const size_t nmatch = 1;
	regex_t reg;
	const char *pattern = "src=\"(/[a-zA-Z0-9]+)*[a-zA-Z0-9]+\\.[a-zA-Z0-9]+\"";
	regcomp(&reg, pattern, cflags);
	// printf("%s\n", html);
	int i = 0;
    int length = strlen(html);
    while(i < length){
        status = regexec(&reg, html + i, nmatch, &pm, 0);
        if(status == REG_NOMATCH)
		    break;
	    else if (status == 0){
            char out[40] = {0};
			out[0] = '/';
            strncpy(out + 1, html + i + pm.rm_so + 5, pm.rm_eo - pm.rm_so - 5);
            out[pm.rm_eo - pm.rm_so - 5] = '\0';
            i += pm.rm_eo;
	    	// printf("%s\n", out);
			request_pack* req = (request_pack *)malloc(sizeof(request_pack));
			req->uid = req_count++;
			strcpy(req->src, out);
			Inqueue(req); 
	    }
    }
	regfree(&reg);
	return 0;
}

int frame_parser(const char* line){
	// return 1 if end with 1
	int status;
	int cflags = REG_EXTENDED | REG_NEWLINE;
	regmatch_t pm;
	const size_t nmatch = 1;
	regex_t reg;
	const char *pattern = "Frame_([0-9]+0)*1$";
	regcomp(&reg, pattern, cflags);

	int i = 0;
    status = regexec(&reg, line, nmatch, &pm, 0);
    if(status == REG_NOMATCH)
	    return 0;
	else if (status == 0){
		// char out[40] = {0};
		// strncpy(out, line + pm.rm_so, pm.rm_eo - pm.rm_so);
		// printf("%s\n", out);
        return 1;
	}
	regfree(&reg);
	return 0;
}

void arg_parser(const char* url, char* port_str, char* addr, char* path){
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

void send_get(int fd, const char* path, const char* ip){
    char send_buf[256] = {0};

#ifndef HTTP2
    int send_len = sprintf(send_buf,
						"GET %s HTTP/1.1\r\n"
						"Host: %s\r\n\r\n", path, ip);
#else
	int send_len = sprintf(send_buf,
						"GET %s HTTP/2.0\r\n"
						"Host: %s\r\n\r\n", path, ip);
#endif
	send(fd, send_buf, send_len, 0);
	// printf("Send request:\n%s", send_buf);
}

void handle_subrequest(request_pack* request){
	// printf("Send object %d request\n", request->uid);
	send_get(request->sock, request->src, request->server_ip);
	// char receive_msg[1024] = { 0 };
	// int read_len = read(request->fd, receive_msg, 1024);
	// printf("Object %d get response:\n%-105.100s", request->uid, receive_msg);
#ifdef MULTSOCK
	sleep(2);
	char receive_msg[1024] = { 0 };
	while(1){
		int read_len = recv_line(request->sock, receive_msg, 1024);
		receive_msg[read_len] = '\0';
		// printf("len %d\n", read_len);
		if(read_len <= 0)
			break;
		if(frame_parser(receive_msg) == 1){
			printf("%s", receive_msg);
		}
	}
	Dequeue(request->uid);
	return;
#endif
}

void send_resource_request_and_read_result(int sock, const char* server_port, const char* server_ip, 
											struct sockaddr_in* serv_addr){
	int i;
	pthread_t tid;
#ifndef MULTSOCK
	for(i = 0; i < MAX; ++i){
		if(!requests[i]){
			break;
		}
		strcpy(requests[i]->server_ip, server_ip);
		requests[i]->sock = sock;
		pthread_create(&tid, NULL, handle_subrequest, requests[i]);
	}
#else
	for(i = 0; i < MAX; i++){
		if(!requests[i]){
			break;
		}

		int sock_i = 0, client_fd;
		// struct sockaddr_in serv_addr;

		char receive_msg[2048 + 10] = { 0 };
		if ((sock_i = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("\n Socket creation error \n");
			return -1;
		}

		struct timeval tv_out;
		tv_out.tv_sec = 1;
		tv_out.tv_usec = 0;
		int opt = 1;
		if (setsockopt(sock_i, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out))) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		struct sockaddr_in serv_addr_i;
		serv_addr_i.sin_family = AF_INET;
		serv_addr_i.sin_port = htons(atoi(server_port));

		if (inet_pton(AF_INET, server_ip, &serv_addr_i.sin_addr) <= 0) {
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		if ((client_fd = connect(sock_i, (struct sockaddr*)&serv_addr_i,
				sizeof(serv_addr_i))) < 0) {
			perror("connect");
			exit(EXIT_FAILURE);
		}

		strcpy(requests[i]->server_ip, server_ip);
		requests[i]->sock = sock_i;
		pthread_create(&tid, NULL, handle_subrequest, requests[i]);
	}
	sleep(1);
	pthread_join(tid, NULL);
#endif
#ifndef MULTSOCK
	sleep(1);
	char receive_msg[1024] = { 0 };
	while(1){
		int read_len = recv_line(sock, receive_msg, 1024);
		receive_msg[read_len] = '\0';
		// printf("len %d\n", read_len);
		if(read_len <= 0)
			break;
		if(frame_parser(receive_msg) == 1){
			printf("%s", receive_msg);
		}
	}
#else

#endif
	return;
}

int main(int argc, char const* argv[])
{	
	char ipaddr[20] = {0};
	char port[10] = {0};
	char path[30] = {0};
	arg_parser(argv[1], port, ipaddr, path);
	int sock = 0, client_fd;
	struct sockaddr_in serv_addr;

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	char receive_msg[2048 + 10] = { 0 };
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n Socket creation error \n");
		return -1;
	}

	struct timeval tv_out;
	tv_out.tv_sec = 1;
	tv_out.tv_usec = 0;
	int opt = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out))) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	serv_addr.sin_family = AF_INET;
	// serv_addr.sin_addr.s_addr = inet_addr("10.145.21.35");
	serv_addr.sin_port = htons(atoi(port));

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
	int valread = recv(sock, receive_msg, 2048, 0);
	// printf("Get response:\n%s", receive_msg);
	if(html_source_parser(receive_msg)){
		printf("Parse html Failed \n");
		return -1;
	}
	send_resource_request_and_read_result(sock, port, ipaddr, &serv_addr);
	// closing the connected socket
	clock_gettime(CLOCK_MONOTONIC, &end);
	printf("Time elapsed: %ld.%09ld seconds\n",
             end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);
	sleep(60);
	close(client_fd);
	return 0;
}
