#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <err.h>
#include <unistd.h>
#include <cstring>

int main()
{
	//we create socket
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
		err(2, "SERVER RETURNED -1");

	sockaddr_in serverAdd;
	serverAdd.sin_family = AF_INET;
	serverAdd.sin_port = htons(8080);	
	serverAdd.sin_addr.s_addr = INADDR_ANY;
	
	//bind socket
	bind(server_socket, (struct sockaddr*)&serverAdd, sizeof(serverAdd));
	//listen to data on 8080 port
	listen(server_socket, 5);
	//client accepting
	int clientSocket = accept(server_socket, 0, 0);
	//data receipt
	char buff[1024] = {0};
	const char *hellohttp = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
	//now recv just recieve the message and put it in buff, later will become an HTTP request
	//the HTTP request need to be parsed
	//extracting Method, Path, and Headers.
	recv(clientSocket, buff, sizeof(buff), 0);
	send(clientSocket, hellohttp, strlen(hellohttp), 0);
	std::cout << "Message from client: " << buff << std::endl;
	//close
	close(server_socket);
}