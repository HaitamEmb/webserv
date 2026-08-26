#ifndef SERVERMANAGER_HPP
#define SERVERMANAGER_HPP


#include "ConfigServ.hpp"
#include "Client.hpp"
#include <vector>
#include <map>
#include <poll.h>
#include <signal.h>

class ServerManager {
	private:
		std::vector<ConfigServ> _configs;
		std::vector<int> _listen_fds;
		std::vector<struct pollfd> _poll_fds;
		std::map<int, Client*> _clients;
		std::map<int, ConfigServ> _listen_config_map;
		std::map<int, std::vector<ConfigServ> > _listen_configs;
		std::map<int, int> _client_listen_map;
		std::map<int, Client*> _cgi_input_clients;
		std::map<int, Client*> _cgi_output_clients;
		static volatile sig_atomic_t _stop_requested;

		bool _setupListenSockets();
		void _setNonBlocking(int fd);
		void _acceptNewConnection(int listen_fd);
		void _readClientData(int client_fd, size_t poll_index);
		void _writeClientData(int client_fd);
		void _closeConnection(int client_fd);
		bool _isListenning(int fd) const;
		void _removePollFd(size_t index);
		void _startCgi(Client *client, size_t client_index);
		void _abortCgi(Client *client);
		void _handleCgiInput(int fd, size_t poll_index);
		void _handleCgiOutput(int fd, size_t poll_index);
		void _selectClientConfig(Client *client);
		static void _handleSignal(int signal_number);
	public:
		ServerManager(const std::vector<ConfigServ> &configs);
		~ServerManager();
		
		bool init();
		void run();
};



#endif