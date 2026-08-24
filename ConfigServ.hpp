#ifndef CONFIGSERV_HPP
#define CONFIGSERV_HPP

#include <iostream>
#include <vector>
#include <string>
#include "ConfigLoc.hpp"
#include <map>

class ConfigServ
{
	private:
		int	_port;
		std::string _host;
		std::vector<std::string> _servers;
		std::vector<ConfigLoc> _locations;
		std::map<int, std::string> _error_pages;


	public:
		ConfigServ();
		~ConfigServ();
		ConfigServ(const ConfigServ &other);
		ConfigServ &operator=(const ConfigServ &other);
		void setPort(int port);
		void setHost(const std::string &host);
		void addServer(const std::string& name);
		void addLoc(const ConfigLoc &location);
		void setErrorPage(int code, const std::string &path);

		int getPort() const;
		std::string getHost() const;
		std::vector<std::string> getServer() const;
		const std::vector<ConfigLoc> &getLocs() const;
		std::string getErrorPage(int code) const;

};


#endif