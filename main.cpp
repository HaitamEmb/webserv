#include "ConfigParser.hpp"
#include "ServerManager.hpp"
#include <iostream>

int main(int ac, char  **av)
{
	std::string config_file = (ac > 1) ? av[1] : "default.conf";

	ConfigParser parser(config_file);
	if(!parser.parse())
	{
		std::cerr << "Failed to parse config file" << std::endl;
		return 1;
	}

	ServerManager manager(parser.getServers());
	manager.init();
	manager.run();

	return 0;
}