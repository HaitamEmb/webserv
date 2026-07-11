#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <iostream>
#include <cstring>
#include <map>

class HttpRequest {

	private:
		std::string _method;
		std::string _path;
		std::string _body;
		std::string _version;
		std::map<std::string, std::string> _headers;

		void parseRequest(const std::string &line);
		void parseHeader(const std::string &line);
		
	public:
		HttpRequest();
		~HttpRequest();
		HttpRequest(HttpRequest const &other);
		HttpRequest &operator=(HttpRequest const &other);
		void parse(const std::string &buff);

		//getters
		std::string getMethod();
		std::string getVersion();
		std::string getBody();
		std::string getHeader();
		std::string getPath();

};


#endif