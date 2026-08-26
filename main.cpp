#include "ConfigParser.hpp"
#include "ServerManager.hpp"
#include <iostream>
#include <signal.h>

int main(int ac, char  **av)
{
	signal(SIGPIPE, SIG_IGN);

	std::string config_file = (ac > 1) ? av[1] : "";

	ConfigParser parser(config_file);
	if(!parser.parse())
	{
		std::cerr << "Failed to parse or no config file" << std::endl;
		return 1;
	}

	ServerManager manager(parser.getServers());
	if (!manager.init()) {
		std::cerr << "Failed to initialize listening sockets" << std::endl;
		return 1;
	}
	manager.run();

	return 0;
}