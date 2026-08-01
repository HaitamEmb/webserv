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

//implement other functions here
