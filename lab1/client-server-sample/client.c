// Client side C/C++ program to demonstrate Socket
// programming
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#define PORT 12000
#define TRANS

void str_trans(char* ori){
	printf("%s\n", ori);
	int len = strlen(ori);
	char *temp = (char*)malloc(len * sizeof(char));
	int j = 0;
	for(int i = 0; i < len; i++){
		char ch = ori[i];
		if(ch == '\\' && ori[i+1] == 'r'){
			temp[j++] = '\r';
			i++;
		} else if (ch == '\\' && ori[i+1] == 'n'){
			temp[j++] = '\n';
			i++;
		} else {
			temp[j++] = ch;
		}
		i++;
	}
	temp[j] = '\0';
	strcpy(ori, temp);
	printf("%s\n", ori);
}

int main(int argc, char const* argv[])
{
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
	serv_addr.sin_port = htons(atoi(argv[2]));

	// Convert IPv4 and IPv6 addresses from text to binary
	// form
	if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)
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
#ifdef TRANS
	str_trans(sentence);
#endif
	send(sock, sentence, strlen(sentence), 0);
	int valread = read(sock, receive_msg, 1024);
	printf("%s\n", receive_msg);
	
	// closing the connected socket
	sleep(60);
	close(client_fd);
	return 0;
}
