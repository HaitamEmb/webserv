#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <iostream>
#include <string>
#include <map>

class HttpResponse {
	private:
		int	_status_code;
		std::string	_status_message;
		std::map<std::string, std::string> _headers;
		std::string	_body;
		std::string	getStatusMsg(int status_code) const;

	public:

		HttpResponse();
		~HttpResponse();
		HttpResponse(const HttpResponse &other);
		HttpResponse &operator=(const HttpResponse &other);

		void setStatusCode(int status_code);
		void setHeader(const std::string &key, const std::string &value);
		void setBody(const std::string &body);
		std::string serializer();
};


#endif