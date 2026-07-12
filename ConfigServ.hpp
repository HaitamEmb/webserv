#ifndef CONFIGSERV_HPP
#define CONFIGSERV_HPP

#include <iostream>
#include <vector>
#include <string>
#include "ConfigLoc.hpp"

class ConfigServ
{
	private:
		int	_port;
		std::string _host;
		std::vector<std::string> _servers;
		std::vector<ConfigLoc> _locations;


	public:
		ConfigServ();
		~ConfigServ();
		ConfigServ(const ConfigServ &other);
		ConfigServ &operator=(const ConfigServ &other);
		void setPort(int port);
		void setHost(const std::string &host);
		void addServer(const std::string& name);
		void addLoc(const ConfigLoc &location);

		int getPort() const;
		std::string getHost() const;
		std::vector<std::string> getServer() const;
		std::vector<ConfigLoc> getLocs() const;

};


#endif