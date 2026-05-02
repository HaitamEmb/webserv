#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <err.h>
#include <unistd.h>

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
	recv(clientSocket, buff, sizeof(buff), MSG_DONTWAIT);
	std::cout << "Message from client: " << buff << std::endl;
	//close
	close(server_socket);
}