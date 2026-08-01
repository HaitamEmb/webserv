#include "ConfigParser.hpp"

#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

ConfigParser::ConfigParser(const std::string &config_file_path) : _file_path(config_file_path) {};
ConfigParser::~ConfigParser() {};

std::string ConfigParser::_trim(const std::string &str)
{
	size_t first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}

bool ConfigParser::parse()
{
	std::ifstream file(_file_path.c_str());
	if (!file.is_open())
	{
		std::cerr << "Error: Couldn't Open config file " << _file_path << std::endl;
		return false;
	}

	std::string line;
	while(std::getline(file, line))
	{
		size_t comment_pos = line.find('#');
		if (comment_pos != std::string::npos)
			line = line.substr(0, comment_pos);
		std::string trimmed = _trim(line);
		if (trimmed.empty())
			continue;
		if (trimmed == "server {" || trimmed == "server{")
			_parseServer(file);	
	}
	file.close();
	return !_servers.empty();
}

std::string clean_token(std::string str)
{
	size_t first = str.find_first_not_of(";");
	if(first == std::string::npos)
	{
		return "";
	}
	size_t last = str.find_last_not_of(";");
	return str.substr(first, (last - first + 1));

}

void ConfigParser::_parseServer(std::ifstream &file)
{
	ConfigServ server;
	std::string line;

	while(std::getline(file, line))
	{
		size_t cmt_pos = line.find("#");
		if (cmt_pos != std::string::npos)
			line = line.substr(0, cmt_pos);
		std::string trimmed = _trim(line);
		if (trimmed.empty())
			continue;
		if (trimmed == "}")
		{
			_servers.push_back(server);
			return;
		}

		std::stringstream ss(trimmed);
		std::string key;
		ss >> key;

		if (key == "listen")
		{
			int port;
			ss >> port;
			server.setPort(port);
		}
		else if (key == "host")
		{	
			std::string host;
			ss >> host;
			server.setHost(clean_token(host));
		} else if (key == "server_name")
		{
			std::string name;
			while (ss >> name)
				server.addServer(clean_token(name));
		}
		else if (key == "location")
		{
			std::string path;
			std::string bracket;
			ss >> path >> bracket;

			ConfigLoc location;
			_parseLocation(file, location, path);
			server.addLoc(location);
		}
	}
}

void ConfigParser::_parseLocation(std::ifstream &file, ConfigLoc &location, const std::string &path)
{
	location.setPath(path);
	std::string line;
	
	while(std::getline(file, line))
	{
		size_t cmt_pos = line.find("#");
		if (cmt_pos != std::string::npos)
			line = line.substr(0, cmt_pos);
		std::string trimmed = _trim(line);
		if(trimmed.empty())
			continue;
		if (trimmed == "}")
			return;
		std::stringstream ss(trimmed);
		std::string key;

		ss >> key;
		if (key == "root")
		{
			std::string root;
			ss >> root;
			location.setRoot(clean_token(root));
		}else if (key == "index")
		{
			std::string index;
			ss >> index;
			location.setIndex(clean_token(index));
		}else if (key == "autoindex"){
			std::string value;
			ss >> value;
			location.setAutoIndex(clean_token(value) == "on");
		}else if (key == "methods") {
			std::string method;
			while (ss >> method)
				location.addMethod(clean_token(method));
		}else if (key == "client_max_body_size") {
			size_t size;
			ss >> size;
			location.setMaxBodySize(size);
		}
	}
}

std::vector<ConfigServ> ConfigParser::getServers() const {
	return _servers;
}