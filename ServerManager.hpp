#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP


#include "ConfigServ.hpp"
#include "Client.hpp"
#include <vector>
#include <map>
#include <poll.h>

class ServerManager {
	private:
		std::vector<ConfigServ> _configs;
		std::vector<int> _listen_fds;
		std::vector<struct pollfd> _poll_fds;
		std::map<int, Client*> _clients;
		std::map<int, ConfigServ> _listen_config_map;

		void _setupListenSockets();
		void _setNonBlocking(int fd);
		void _acceptNewConnection(int listen_fd);
		void _readClientData(int client_fd, size_t poll_index);
		void _writeClientData(int client_fd, size_t poll_index);
		void _closeConnection(int client_fd, size_t poll_index);
		bool _isListenning(int fd) const;
	public:
		ServerManager(const std::vector<ConfigServ> &configs);
		~ServerManager();
		
		void init();
		void run();
};



#endif