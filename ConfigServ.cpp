#include "ConfigServ.hpp"
#include <algorithm>

ConfigServ::ConfigServ() : _port(80), _host("0.0.0.0") {};
ConfigServ::~ConfigServ() {};

ConfigServ::ConfigServ(const ConfigServ &other)
{
	*this = other;
}

ConfigServ &ConfigServ::operator=(const ConfigServ &other)
{
	if (this != &other)
	{
		this->_host = other._host;
		this->_locations = other._locations;
		this->_port = other._port;
		this->_servers = other._servers;
		this->_error_pages = other._error_pages;
	}
	return *this;
		
}

void ConfigServ::setPort(int port) {
	_port = port;
};
void ConfigServ::setHost(const std::string &host) {
	_host = host;
};
void ConfigServ::addServer(const std::string &server) {
	_servers.push_back(server);
};
void ConfigServ::addLoc(const ConfigLoc &loc) {
	 _locations.push_back(loc);
};
void ConfigServ::setErrorPage(int code, const std::string &path) { _error_pages[code] = path; }

int ConfigServ::getPort() const {return _port;}
std::string ConfigServ::getHost() const { return _host;}
std::vector<std::string> ConfigServ::getServer() const {return _servers;}
const std::vector<ConfigLoc> &ConfigServ::getLocs() const {return _locations;}
std::string ConfigServ::getErrorPage(int code) const {
	std::map<int, std::string>::const_iterator it = _error_pages.find(code);
	return it == _error_pages.end() ? "" : it->second;
}