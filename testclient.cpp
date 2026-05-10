#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <err.h>
#include <unistd.h>
#include <cstring> //to check

int main()
{
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAdd;
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_port = htons(8080);
	inet_pton(AF_INET, "127.0.0.1", &serverAdd.sin_addr);	
	serverAdd.sin_addr.s_addr = INADDR_ANY;

	connect(client_socket, (struct sockaddr*)&serverAdd, sizeof(serverAdd));
	const char* m = "Hello world!";
	send(client_socket, m, strlen(m), 0);
	close(client_socket);
}