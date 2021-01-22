#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <fcntl.h>
#include <string.h> 
#define PORT 9999
#define MAX 20000
int main(int argc, char const *argv[]){ 
	printf("%02x", '\0');
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	unsigned char buffer[100000] = {0}; 
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	if(inet_pton(AF_INET, "10.0.0.21", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	}

	// Enviar Client Greeting
	buffer[0] = 0x05;
	buffer[1] = 0x01;
	buffer[2] = 0x00;
	send(sock, buffer, 3, 0);
	memset(buffer, 0, sizeof(buffer));

	// Receber Server Choice
	recv(sock, buffer, 2, 0);
	printf("Server Choice: | %x | %x |\n", buffer[0], buffer[1]);
	memset(buffer, 0, sizeof(buffer));

	// Enviar Client connection request
	// buffer[0] = 0x05; // Versao 5
	// buffer[1] = 0x01; // establish a TCP/IP stream connection
	// buffer[2] = 0x00; // Reserved
	// buffer[3] = 0x01; // É ip
	// buffer[4] = 10; // 192
	// buffer[5] =	0; // 168
	// buffer[6] = 0; // 0
	// buffer[7] = 22; // 70

	buffer[0] = 0x04; // Versao 5
	buffer[1] = 0x01; // establish a TCP/IP stream connection
	buffer[4] = 10; // 192
	buffer[5] =	0; // 168
	buffer[6] = 0; // 0
	buffer[7] = 25; // 70

	// 8888 = 0x22 0xB8
	unsigned short port = 6666;
	buffer[2] = port >> 8;
	buffer[3] = port;
	send(sock, buffer, 8, 0);
	memset(buffer, 0, sizeof(buffer));

	// Receber Response packet from server
	int n = recv(sock, buffer, 2, 0);

	printf("Response packet from server: |");
	for(int i=0; i<n; i++){
		printf(" %x |", buffer[i]);
	}
	printf("\n");

	n = 0;
	int fd = open("test.txt",O_RDONLY);

	while((n=read(fd,buffer,MAX))>0){
		printf("%d\n",n);
		send(sock, buffer, n, 0);
		memset(buffer,0,MAX);
	}
	sleep(10);
		close(sock);
	close(fd);
	

	

	return 0; 
} 
