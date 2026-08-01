#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "ConfigServ.hpp"
#include <string>
#include <vector>

class ConfigParser {
	private:
		std::string _file_path;
		std::vector<ConfigServ> _servers;
		std::string _trim(const std::string &str);
		void _parseServer(std::ifstream& file);
		void _parseLocation(std::ifstream &file, ConfigLoc &location, const std::string &path);
	public:
		ConfigParser(const std::string &config_file_path);
		~ConfigParser();
		bool parse();
		std::vector<ConfigServ> getServers() const;
};

#endif