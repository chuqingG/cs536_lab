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
#include <regex.h>
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
	char ip[20];
	char port[10];
	// char name[32];
} client_t;
client_t *clients[MAX];

int read_line(int fd,char *buf, int size){
	//return the current idx 
	int i = 0;
	char ch;
	for(i = 0;i < size;++i){
		int n = recv(fd, &ch, 1, 0);
		if(n == 1){
			buf[i] = ch;
			if(ch == '\n') 
				break;
		}
		else{
			perror("read a char");
			exit(EXIT_FAILURE);
		}
	}
	return i + 1;
}

int copy_line(const char *src, char *buf){
	//return the length of the copied line
	int i = 0;
	for(i = 0; src[i] != '\0'; ++i){
		buf[i] = src[i];
		if(src[i] == '\n') 
			break;
	}
	buf[i+1] = '\0';
	return i + 1;
}

int get_rel_path(const char *src, char *buf){
	//return the relative path to buf
	int i;
	for(i = 4; src[i] != ' '; ++i){
		buf[i - 4] = src[i];
		if(src[i] == '\r' || src[i] == '\n') 
			return -1;
		else if(src[i] == '\0'){
			return -2;
		}
	}
	return 0;
}

int read_file(char* des, char* path){
	FILE* pfile;
	pfile = fopen(path, "rb");
	if(pfile == NULL){
		perror("open file");
		exit(EXIT_FAILURE);
	} 
	fseek(pfile, 0, SEEK_END);
	int length = ftell(pfile);
	rewind(pfile);
	length = fread(des, 1, length, pfile);
	des[length] = '\0';
	fclose(pfile);
	return length;
}

int send_picture(client_t *cli){
	// FILE *fp;
    char readBuf[960*320 + 1024] = {0}; //600k

	int length = read_file(readBuf, "www/purdue.jpeg");
    
    /* send data */
    char* p_bufs = (char *)malloc(strlen(readBuf) + 1024);
    if (NULL == p_bufs){
        perror("malloc buffer");
		exit(EXIT_FAILURE);
    }  
    int response_len = sprintf(p_bufs,	"HTTP/1.1 200 OK\r\n"
						"Connection: keep-alive\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "Content-Length: %d\r\n\r\n",
                        length);

    memcpy(p_bufs+response_len, readBuf, length);
    response_len += length;
    
    send(cli->fd, p_bufs, response_len, 0);
	printf("message-to-client:%s,%s\nHTTP/1.1 200 OK\n",
			cli->ip,cli->port);
    free(p_bufs);
    return 0;
}

int read_file_fixed_size(char* des, char* path){
	FILE* pfile;
	pfile = fopen(path, "rb");
	if(pfile == NULL){
		perror("open file");
		exit(EXIT_FAILURE);
	} 
	fseek(pfile, 0, SEEK_END);
	int length = ftell(pfile);
	rewind(pfile);
	length = fread(des, 1, length, pfile);
	des[length] = '\0';
	fclose(pfile);
	return length;
}

int send_big_picture_with_head(int fd){
	// FILE *fp;
    char read_buf[102400 + 10] = {0}; 
	FILE* pfile;
	pfile = fopen("www/bigpicture.jpeg", "rb");
	if(pfile == NULL){
		perror("open file");
		exit(EXIT_FAILURE);
	}
	int seg_len = 102400;

	// send header and first part
	seg_len = fread(read_buf, 1, seg_len, pfile);
	char* send_buf = (char *)malloc(strlen(read_buf) + 1024);
    if (NULL == send_buf){
        perror("malloc buffer");
		exit(EXIT_FAILURE);
    }
	int response_len = sprintf(send_buf,	"HTTP/1.1 200 OK\r\n"
						"Connection: keep-alive\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "Transfer-Encoding: chunked\r\n\r\n");

    memcpy(send_buf + response_len, read_buf, seg_len);
    send(fd, send_buf, response_len + seg_len, 0);

	/* send the remaining data */
	while(seg_len < 102400){
		seg_len = fread(read_buf, 1, seg_len, pfile);
		send(fd, read_buf, seg_len, 0);
	}
	// int length = read_file(readBuf, "www/bigpicture.jpeg");
    
    free(send_buf);
    return 0;
}

int min_(int x, int y){
	if (x < y)
		return x;
	else	
		return y;
}

int send_big_picture(client_t *cli){
	FILE *f = fopen("www/bigpicture.jpeg", "rb");
	if (f == NULL){
		perror("open file");
		exit(EXIT_FAILURE);
	}
	fseek(f, 0, SEEK_END);
	int length = ftell(f);
	if (length <= 0){
		perror("seek end");
		exit(EXIT_FAILURE);
	}
	rewind(f);

	// send head 
	char *head_buf = (char *)malloc(length + 1024);
	int head_len = sprintf(head_buf, "HTTP/1.1 200 OK\r\n"
							"Content-Type: image/jpeg\r\n"
							"Accept-Ranges:bytes\r\n\r\n");
	send(cli->fd, head_buf, head_len, 0);	
	printf("message-to-client:%s,%s\nHTTP/1.1 200 OK\n",
			cli->ip,cli->port);
	// send chunked body
	int seg_len = 102400;
	char read_buf[102400 + 100] = {0};
	// printf("frame count:%d", (int)length / seg_len);
	for (int i = 0; i < length; i += seg_len){
		int size = min_(seg_len, length - i);
		size = fread(read_buf, 1, size, f);
		send(cli->fd, read_buf, size, 0);
	}
	free(head_buf);
	fclose(f);
}

int send_video(client_t *cli, const char *path){
	FILE *f = fopen(path, "rb");
	if (f == NULL){
		perror("open file");
		exit(EXIT_FAILURE);
	}
	fseek(f, 0, SEEK_END);
	int length = ftell(f);
	if (length <= 0){
		perror("seek end");
		exit(EXIT_FAILURE);
	}
	rewind(f);

	// send head 
	char *head_buf = (char *)malloc(length + 1024);
	int head_len = sprintf(head_buf, "HTTP/1.1 200 OK\r\n"
							"Content-Type: video/mp4\r\n"
							"Accept-Ranges:bytes\r\n\r\n");
	send(cli->fd, head_buf, head_len, 0);	
	printf("message-to-client:%s,%s\nHTTP/1.1 200 OK\n",
			cli->ip,cli->port);
	// send chunked body
	int seg_len = 102400;
	char read_buf[102400 + 100] = {0};
	// printf("frame count:%d", (int)length / seg_len);
	for (int i = 0; i < length; i += seg_len){
		int size = min_(seg_len, length - i);
		size = fread(read_buf, 1, size, f);
		send(cli->fd, read_buf, size, 0);
	}
	free(head_buf);
	fclose(f);
}

int send_html(client_t *cli, char* path){
    char file_buf[1024] = {0}; 
	int length = read_file(file_buf, path);
    
    char* response_buf = (char *)malloc(strlen(file_buf) + 1024);
    if (NULL == response_buf){
        perror("malloc buffer");
		exit(EXIT_FAILURE);
    }  
    int response_len = sprintf(response_buf,
						"HTTP/1.1 200 OK\r\n"
						"Connection: keep-alive\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %d\r\n\r\n",
                        length);

    memcpy(response_buf + response_len, file_buf, length);
    send(cli->fd, response_buf, response_len + length, 0);
	printf("message-to-client:%s,%s\nHTTP/1.1 200 OK\n",
			cli->ip,cli->port);
    free(response_buf);
    return 0;
}

int send_404(client_t *cli){

    char* response_buf = (char *)malloc(1024);
      
    int response_len = sprintf(response_buf,
						"HTTP/1.1 404 Not Found\r\n\r\n"
						"<!DOCTYPE html><html><body><h1>404 Not Found</h1>"
						"<p>The requested URL was not found on this server.</p></body></html>");
	printf("message-to-client:%s,%s\nHTTP/1.1 404 Not Found\n",
			cli->ip,cli->port);
    send(cli->fd, response_buf, response_len, 0);
    free(response_buf);
    return 0;
}

int send_400(client_t *cli){

    char* response_buf = (char *)malloc(1024);
      
    int response_len = sprintf(response_buf,
						"HTTP/1.1 400 Bad Request\r\n\r\n"
						"<!DOCTYPE html><html><body><h1>400 Bad Request</h1>"
						"<p>Please check the syntax of your request.</p></body></html>");

    printf("message-to-client:%s,%s\nHTTP/1.1 400 Bad Request\n",
			cli->ip,cli->port);
    send(cli->fd, response_buf, response_len, 0);
    free(response_buf);
    return 0;
}

int send_505(client_t *cli){

    char* response_buf = (char *)malloc(1024);
      
    int response_len = sprintf(response_buf,
						"HTTP/1.1 505 HTTP Version Not Supported\r\n\r\n"
						"<!DOCTYPE html><html><body><h1>505 HTTP Version Not Supported</h1>"
						"<p>Incorrect http version was given.</p></body></html>");

    printf("message-to-client:%s,%s\nHTTP/1.1 505 HTTP Version Not Supported\n",
			cli->ip,cli->port);
    send(cli->fd, response_buf, response_len, 0);
    free(response_buf);
    return 0;
}

int check_syntax_error(char* request){
	int status;
	int cflags = REG_EXTENDED | REG_NEWLINE;
	regmatch_t pmatch;
	const size_t nmatch = 1;
	regex_t reg, end_reg;
	const char *pattern = "^[a-zA-Z]+(-[a-zA-Z]+)*:";
	regcomp(&reg, pattern, cflags);

	int i = 0;
    int length = strlen(request);
    while(i < length - 2){
        status = regexec(&reg, request + i, nmatch, &pmatch, 0);
        if(status == REG_NOMATCH)
		    break;
	    else if (status == 0){
            char out[40] = {0};
            strncpy(out, request + i + pmatch.rm_so, pmatch.rm_eo - pmatch.rm_so);
            out[pmatch.rm_eo - pmatch.rm_so - 1] = '\0';
            i += pmatch.rm_eo;
	    	// printf("%s\n", out);
			
            if (strcmp(out, "Host") && strcmp(out, "Connection") &&
				strcmp(out, "Upgrade-Insecure-Requests") && 
				strcmp(out, "User-Agent") && strcmp(out, "Accept") && 
				strcmp(out, "Accept-Encoding") && strcmp(out, "Accept-Language") &&
				strcmp(out, "Cache-Control") && strcmp(out, "Referer")){
				return 1;
			}
	    }
    }
	regfree(&reg);
	return 0;
}

int check_version_error(char* request){
	//return 1 if error
	int status;
	int cflags = REG_EXTENDED | REG_NEWLINE;
	regmatch_t pmatch;
	const size_t nmatch = 1;
	regex_t reg, end_reg;
	const char *pattern = "HTTP/[0-9]\\.[0-9]*";
	regcomp(&reg, pattern, cflags);

	char head[50] = {0};
	copy_line(request, head);
	// printf("%s\n", head);
	status = regexec(&reg, head, nmatch, &pmatch, 0);
    if(status == REG_NOMATCH)
	    return 1;
	else if (status == 0){
        char out[10] = {0};
        strncpy(out, head + pmatch.rm_eo - 3, 3);
        out[3] = '\0';
		// printf("%s\n", out);
		
        if (strcmp(out, "1.1")){
			return 1;
		}
	}
	regfree(&reg);
	return 0;
}

void get_request_parser(char *request, client_t *cli){
	char rel_path[128] = {0};
	int flag = get_rel_path(request, rel_path);
	// printf("relative path: %s\n", rel_path);
	if(check_syntax_error(request)){
		send_400(cli);
	} else if(check_version_error(request)){
		send_505(cli);
	} else if(!strcmp(rel_path, "/text.html") || !strcmp(rel_path, "/www/text.html")){
		send_html(cli, "www/text.html");
	} else if (!strcmp(rel_path, "/picture.html") || !strcmp(rel_path, "/www/picture.html")){
		send_html(cli, "www/picture.html");
	} else if (!strcmp(rel_path, "/bigpicture.html") || !strcmp(rel_path, "/www/bigpicture.html")){
		send_html(cli, "www/bigpicture.html");
	} else if (!strcmp(rel_path, "/video.html") || !strcmp(rel_path, "/www/video.html")){
		send_html(cli, "www/video.html");
	} else if (!strcmp(rel_path, "/video.mp4") || !strcmp(rel_path, "/www/video.mp4")){
		send_video(cli, "www/video.mp4");
	} else if (!strcmp(rel_path, "/purdue.jpeg") || !strcmp(rel_path, "/www/purdue.jpeg")){
		send_picture(cli);
	} else if (!strcmp(rel_path, "/bigpicture.jpeg") || !strcmp(rel_path, "/www/bigpicture.jpeg")){
		send_big_picture(cli);
	}else{
		send_404(cli);
	}
	return;
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

	int valread = recv(cli->fd, cli_msg, 1024, 0);
	char firstline[128] = { 0 };
	int t = copy_line(cli_msg, firstline);
	printf("message-from-client:%s,%s\n%s", addr_buf, port_buf, firstline);
	// char head[50] = {0};

	strcpy(cli->ip, addr_buf);
	strcpy(cli->port, port_buf);
	
	memcpy(response_msg, header, strlen(header) * sizeof(char));
	memcpy(response_msg + strlen(header), addr_buf, strlen(addr_buf) * sizeof(char));
	memcpy(response_msg + strlen(header) + strlen(addr_buf) + 1, port_buf, strlen(port_buf) * sizeof(char));
	response_msg[strlen(header) + strlen(addr_buf)] = ',';
	response_msg[strlen(header) + strlen(addr_buf) + strlen(port_buf) + 1] = '\n';
	memcpy(response_msg + idx, cli_msg, sizeof(cli_msg));
	
	// printf("%s\n", response_msg);

	//If recv http request
	if(!strncmp(cli_msg, "GET ", 4)){
		get_request_parser(cli_msg, cli);
	}else{
		send(cli->fd, response_msg, strlen(response_msg), 0);
	}
	
	// while(send(cli->fd, response_msg, 1, 0) >= 0){
	// 	// polling for closing connection
	// 	sleep(1);
	// 	printf("sleep\n");
	// }
	// closing the connected socket
	sleep(600);
	printf("close-client:%s,%s\n", addr_buf, port_buf);
	pthread_mutex_lock(&clients_mutex);
	
	close(cli->fd);
	pthread_mutex_lock(&clients_mutex);
	// Dequeue(cli->uid);
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
		// int cli_idx = Inqueue(cli);
		pthread_create(&tid, NULL, handle_connection, cli);
		sleep(1);
		
	}
	// closing the listening socket
	shutdown(server_fd, SHUT_RDWR);
	return 0;
}
