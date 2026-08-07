#include "ServerManager.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

ServerManager::ServerManager(const std::vector<ConfigServ> &configs) : _configs(configs) {};

ServerManager::~ServerManager() {
	for (size_t i = 0; i < _listen_fds.size(); i++) {
		close(_listen_fds[i]);
	}
	std::map<int, Client*>::iterator it;
	for (it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
}

void ServerManager::_setNonBlocking(int fd) {
	fcntl(fd, F_SETFL, O_NONBLOCK);
}

void ServerManager::_setupListenSockets()
{
	for (size_t i = 0; i < _configs.size(); ++i)
	{
		int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (listen_fd < 0)
		{
			std::cerr << "Socket Creation Failed : " << _configs[i].getPort() << std::endl;
			continue; 
		}
		int opt = 1;
		setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		_setNonBlocking(listen_fd);

		struct sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(_configs[i].getPort());
		addr.sin_addr.s_addr = inet_addr(_configs[i].getHost().c_str());

		if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		{
			std::cerr << "Binding Failed at port: " << _configs[i].getPort() << std::endl;
			close(listen_fd);
			continue;
		}

		if(listen(listen_fd, 128) < 0)
		{
			std::cerr << "Listenning failed on port: " << _configs[i].getPort() << std::endl;
			close(listen_fd);
			continue;
		}

		_listen_fds.push_back(listen_fd);
		_listen_config_map[listen_fd] = _configs[i];

		struct pollfd pfd;
		pfd.fd = listen_fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		_poll_fds.push_back(pfd);

		std::cout << "[ServerManager] listening on http://" << _configs[i].getHost() 
			<< ":" << _configs[i].getPort() << std::endl;
		
	}
}

bool ServerManager::_isListenning(int fd) const {
	for (size_t i = 0; i < _listen_fds.size(); i++)
	{
		if(_listen_fds[i] == fd) return true;
	}
	return false;
}

void ServerManager::init() {
	_setupListenSockets();
}

void ServerManager::run() {
	std::cout << "[ServerManager] Starting Main loop..." << std::endl;
	while(true) {
		int poll_count = poll(_poll_fds.data(), _poll_fds.size(), -1);
		if (poll_count < 0)
		{
			std::cerr << "Poll error: " << std::strerror(errno) << std::endl;
			break;
		}

		for (size_t i = 0; i < _poll_fds.size(); ++i)
		{
			if (_poll_fds[i].revents == 0) continue;
			int current_fd = _poll_fds[i].fd;

			if(_isListenning(current_fd)){
				if(_poll_fds[i].revents & POLLIN) {
					_acceptNewConnection(current_fd);
				}
			}
			else
			{
				if (_poll_fds[i].revents & POLLIN)
				{
					_readClientData(current_fd, i);
				}
				if (_poll_fds[i].revents & POLLOUT)
					_writeClientData(current_fd, i);
				if (_poll_fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
					_closeConnection(current_fd, i);
			}
		}
	}
}
//implement other functions here

void ServerManager::_acceptNewConnection(int listen_fd)
{
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);

	int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd < 0)
		return;
	_setNonBlocking(client_fd);
	Client *new_client = new Client(client_fd, _listen_config_map[listen_fd]);
	_clients[client_fd] = new_client;

	struct pollfd pfd;
	pfd.fd = client_fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_poll_fds.push_back(pfd);

	std::cout << "[ServerManager] New client connected on socket: " << client_fd << std::endl;

}

void ServerManager::_readClientData(int client_fd, size_t poll_index) {
	char buffer[4096];
	ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
	if(bytes_read <= 0)
	{
		_closeConnection(client_fd, poll_index);
		return;
	}

	Client *client = _clients[client_fd];
	client->appendtoReadBuff(buffer, bytes_read);

	client->request.parse(client->getReadBuff());

	if (client->request.parse_complete())
	{

		//testing response generation	
		std::cout << "[ServerManager] full request received from socket: " << client_fd << std::endl;
		client->response.setStatusCode(200);
		client->response.setHeader("Content-Type", "text/html");
		client->response.setBody("<h1>Hello from Webserv poll loop!</h1>");
		client->setWriteBuff(client->response.serializer());
		client->setState(WRITING_RESPONSE);
		_poll_fds[poll_index].events = POLLOUT;
	}
}

void ServerManager::_writeClientData(int client_fd, size_t poll_index)
{
	Client *client = _clients[client_fd];

	ssize_t bytes_sent = send(client_fd, client->getWriteData(), client->getRemainingBytes(), 0);
	if (bytes_sent <= 0)
	{
		_closeConnection(client_fd, poll_index);
		return;
	}
	client->advanceWrite(bytes_sent);
	if(client->getRemainingBytes() == 0)
	{
		std::cout << "[ServerManager] Response sent to socket: " << client_fd << std::endl;
		_closeConnection(client_fd, poll_index);
	}
}

void ServerManager::_closeConnection(int client_fd, size_t poll_index)
{
	std::cout << "[Server Manager] Closing socket: " << client_fd << std::endl;
	close(client_fd);
	delete _clients[client_fd];
	_clients.erase(client_fd);
	_poll_fds.erase(_poll_fds.begin() + poll_index);
}
