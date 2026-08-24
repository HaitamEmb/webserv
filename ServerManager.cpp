#include "ServerManager.hpp"
#include "RequestRouter.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <ctime>

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

bool ServerManager::_setupListenSockets()
{
	for (size_t i = 0; i < _configs.size(); ++i)
	{
		int shared_fd = -1;
		for (std::map<int, ConfigServ>::iterator existing = _listen_config_map.begin(); existing != _listen_config_map.end(); ++existing) {
			if (existing->second.getHost() == _configs[i].getHost()
				&& existing->second.getPort() == _configs[i].getPort()) {
				shared_fd = existing->first;
				break;
			}
		}
		if (shared_fd >= 0) {
			if (_configs[i].getServer().empty() || _listen_configs[shared_fd][0].getServer().empty()) return false;
			_listen_configs[shared_fd].push_back(_configs[i]);
			continue;
		}
		int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (listen_fd < 0)
		{
			std::cerr << "Socket Creation Failed : " << _configs[i].getPort() << std::endl;
			return false; 
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
			return false;
		}

		if(listen(listen_fd, 128) < 0)
		{
			std::cerr << "Listenning failed on port: " << _configs[i].getPort() << std::endl;
			close(listen_fd);
			return false;
		}

		_listen_fds.push_back(listen_fd);
		_listen_config_map[listen_fd] = _configs[i];
		_listen_configs[listen_fd].push_back(_configs[i]);

		struct pollfd pfd;
		pfd.fd = listen_fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		_poll_fds.push_back(pfd);

		std::cout << "[ServerManager] listening on http://" << _configs[i].getHost() 
			<< ":" << _configs[i].getPort() << std::endl;
		
	}
	return true;
}

bool ServerManager::_isListenning(int fd) const {
	for (size_t i = 0; i < _listen_fds.size(); i++)
	{
		if(_listen_fds[i] == fd) return true;
	}
	return false;
}

bool ServerManager::init() {
	if (_setupListenSockets()) return true;
	for (size_t i = 0; i < _listen_fds.size(); ++i) close(_listen_fds[i]);
	_listen_fds.clear();
	_poll_fds.clear();
	_listen_config_map.clear();
	_listen_configs.clear();
	return false;
}

void ServerManager::run() {
	std::cout << "[ServerManager] Starting Main loop..." << std::endl;
	while(true) {
		int poll_count = poll(_poll_fds.data(), _poll_fds.size(), 1000);
		if (poll_count < 0)
		{
			std::cerr << "Poll error: " << std::strerror(errno) << std::endl;
			break;
		}
		std::time_t now = std::time(NULL);
		for (std::map<int, Client*>::iterator timeout_it = _clients.begin(); timeout_it != _clients.end(); ) {
			std::map<int, Client*>::iterator current = timeout_it++;
			if (current->second->getCgi() != NULL) current->second->getCgi()->reap();
			if (current->second->getCgi() != NULL && current->second->getCgi()->isComplete()) {
				current->second->response = current->second->getCgi()->getResponse();
				current->second->setWriteBuff(current->second->response.serializer());
				delete current->second->getCgi();
				current->second->setCgi(NULL);
				for (size_t finish_index = 0; finish_index < _poll_fds.size(); ++finish_index)
					if (_poll_fds[finish_index].fd == current->first) _poll_fds[finish_index].events = POLLOUT;
			}
			if (now - current->second->getLastActivity() > 30) {
				for (size_t timeout_index = 0; timeout_index < _poll_fds.size(); ++timeout_index)
					if (_poll_fds[timeout_index].fd == current->first) { _closeConnection(current->first); break; }
			}
		}

		for (size_t i = 0; i < _poll_fds.size(); ++i)
		{
			if (_poll_fds[i].revents == 0) continue;
			int current_fd = _poll_fds[i].fd;
			if (_cgi_input_clients.find(current_fd) != _cgi_input_clients.end()) {
				_handleCgiInput(current_fd, i);
				continue;
			}
			if (_cgi_output_clients.find(current_fd) != _cgi_output_clients.end()) {
				_handleCgiOutput(current_fd, i);
				continue;
			}

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
					if (_clients.find(current_fd) == _clients.end())
					{
						if (i > 0) --i;
						continue;
					}
				}
				if (_poll_fds[i].revents & POLLOUT)
					_writeClientData(current_fd);
				if (_poll_fds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
					_closeConnection(current_fd);
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
	_client_listen_map[client_fd] = listen_fd;

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
		_closeConnection(client_fd);
		return;
	}

	Client *client = _clients[client_fd];
	client->appendtoReadBuff(buffer, bytes_read);
	client->touch();

	client->request.parse(client->getReadBuff());
	_selectClientConfig(client);
	if (RequestRouter::bodyTooLarge(client->request, client->assigned_config)) {
		client->response.setStatusCode(413);
		client->response.setHeader("Content-Type", "text/html");
		client->response.setBody("<h1>413 Payload Too Large</h1>");
		client->setWriteBuff(client->response.serializer());
		client->setState(WRITING_RESPONSE);
		_poll_fds[poll_index].events = POLLOUT;
		return;
	}

	if (client->request.parse_complete())
	{

		//testing response generation	
		std::cout << "[ServerManager] full request received from socket: " << client_fd << std::endl;
		CgiHandler *cgi = RequestRouter::createCgi(client->request, client->assigned_config);
		if (cgi != NULL) {
			client->setCgi(cgi);
			_startCgi(client, poll_index);
			return;
		}
		client->response = RequestRouter::routeRequest(client->request, client->assigned_config);
		// client->response.setStatusCode(200);
		// client->response.setHeader("Content-Type", "text/html");
		// client->response.setBody("<h1>Hello from Webserv poll loop!</h1>");
		client->setWriteBuff(client->response.serializer());
		client->setState(WRITING_RESPONSE);
		_poll_fds[poll_index].events = POLLOUT;
	}
}

void ServerManager::_selectClientConfig(Client *client) {
	std::map<int, int>::const_iterator listen_it = _client_listen_map.find(client->getFd());
	if (listen_it == _client_listen_map.end()) return;
	std::map<int, std::vector<ConfigServ> >::const_iterator configs_it = _listen_configs.find(listen_it->second);
	if (configs_it == _listen_configs.end()) return;
	std::string host = client->request.getHeader("Host");
	size_t colon = host.find(':');
	if (colon != std::string::npos) host = host.substr(0, colon);
	const std::vector<ConfigServ> &configs = configs_it->second;
	for (size_t i = 0; i < configs.size(); ++i) {
		std::vector<std::string> names = configs[i].getServer();
		for (size_t j = 0; j < names.size(); ++j)
			if (names[j] == host) { client->assigned_config = configs[i]; return; }
	}
}

void ServerManager::_removePollFd(size_t index) {
	_poll_fds.erase(_poll_fds.begin() + index);
}

void ServerManager::_startCgi(Client *client, size_t client_index) {
	if (!client->getCgi()->start()) {
		client->response = RequestRouter::routeRequest(client->request, client->assigned_config);
		client->setWriteBuff(client->response.serializer());
		_poll_fds[client_index].events = POLLOUT;
		return;
	}
	if (client->getCgi()->getInputFd() >= 0) {
		struct pollfd input;
		input.fd = client->getCgi()->getInputFd(); input.events = POLLOUT; input.revents = 0;
		_poll_fds.push_back(input);
		_cgi_input_clients[input.fd] = client;
	}
	struct pollfd output;
	output.fd = client->getCgi()->getOutputFd(); output.events = POLLIN; output.revents = 0;
	_poll_fds.push_back(output);
	_cgi_output_clients[output.fd] = client;
	_poll_fds[client_index].events = 0;
}

void ServerManager::_handleCgiInput(int fd, size_t poll_index) {
	Client *client = _cgi_input_clients[fd];
	client->getCgi()->handleInput();
	if (client->getCgi()->getInputFd() < 0) {
		_cgi_input_clients.erase(fd);
		_removePollFd(poll_index);
	}
}

void ServerManager::_handleCgiOutput(int fd, size_t poll_index) {
	Client *client = _cgi_output_clients[fd];
	client->getCgi()->handleOutput();
	if (client->getCgi()->isComplete()) {
		int input_fd = client->getCgi()->getInputFd();
		if (input_fd >= 0) {
			_cgi_input_clients.erase(input_fd);
			for (size_t input_index = 0; input_index < _poll_fds.size(); ++input_index)
				if (_poll_fds[input_index].fd == input_fd) { _removePollFd(input_index); break; }
		}
		_cgi_output_clients.erase(fd);
		_removePollFd(poll_index);
		client->response = client->getCgi()->getResponse();
		client->setWriteBuff(client->response.serializer());
		delete client->getCgi();
		client->setCgi(NULL);
		for (size_t i = 0; i < _poll_fds.size(); ++i)
			if (_poll_fds[i].fd == client->getFd()) _poll_fds[i].events = POLLOUT;
	}
}

void ServerManager::_writeClientData(int client_fd)
{
	Client *client = _clients[client_fd];

	ssize_t bytes_sent = send(client_fd, client->getWriteData(), client->getRemainingBytes(), 0);
	if (bytes_sent <= 0)
	{
			_closeConnection(client_fd);
		return;
	}
	client->advanceWrite(bytes_sent);
	if(client->getRemainingBytes() == 0)
	{
		std::cout << "[ServerManager] Response sent to socket: " << client_fd << std::endl;
			_closeConnection(client_fd);
	}
}

void ServerManager::_closeConnection(int client_fd)
{
	std::cout << "[Server Manager] Closing socket: " << client_fd << std::endl;
	Client *client = _clients[client_fd];
	for (size_t i = _poll_fds.size(); i > 0; --i) {
		size_t index = i - 1;
		int fd = _poll_fds[index].fd;
		if ((_cgi_input_clients.find(fd) != _cgi_input_clients.end() && _cgi_input_clients[fd] == client)
			|| (_cgi_output_clients.find(fd) != _cgi_output_clients.end() && _cgi_output_clients[fd] == client)) {
			_cgi_input_clients.erase(fd);
			_cgi_output_clients.erase(fd);
			_removePollFd(index);
		}
	}
	close(client_fd);
	delete client;
	_clients.erase(client_fd);
	_client_listen_map.erase(client_fd);
	for (size_t i = 0; i < _poll_fds.size(); ++i)
		if (_poll_fds[i].fd == client_fd) { _removePollFd(i); break; }
}
