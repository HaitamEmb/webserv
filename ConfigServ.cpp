#include "ConfigServ.hpp"
#include <algorithm>

ConfigServ::ConfigServ() : _port(80), _host("0.0.0.0") {};
ConfigServ::~ConfigServ() {};

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

int ConfigServ::getPort() const {return _port;}
std::string ConfigServ::getHost() const { return _host;}
std::vector<std::string> ConfigServ::getServer() const {return _servers;}
std::vector<ConfigLoc> ConfigServ::getLocs() const {return _locations;}