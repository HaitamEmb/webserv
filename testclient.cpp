#include <sys/socket.h>
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
	serverAdd.sin_addr.s_addr = INADDR_ANY;

	connect(client_socket, (struct sockaddr*)&serverAdd, sizeof(serverAdd));
	const char* m = "Hello, server!";
	send(client_socket, m, strlen(m), 0);
	close(client_socket);
}