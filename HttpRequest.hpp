#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <map>

class HttpRequest {

	private:
		std::string _method;
		std::string _path;
		std::string _body;
		std::string _version;
		std::map<std::string, std::string> _headers;
		bool _is_parsed;

		void parseRequest(const std::string &line);
		void parseHeader(const std::string &line);
		
	public:
		HttpRequest();
		~HttpRequest();
		HttpRequest(HttpRequest const &other);
		HttpRequest &operator=(HttpRequest const &other);
		void parse(const std::string &buff);
		bool parse_complete();

		//getters
		std::string getMethod();
		std::string getVersion();
		std::string getBody();
		std::string getHeader(const std::string &key);
		std::string getPath();

};


#endif