#include "HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse() : _status_code(200), _status_message("OK") {
	setHeader("Server", "Webserv/1.0");
	setHeader("Connection", "close");
}

HttpResponse::~HttpResponse(){};

HttpResponse::HttpResponse(const HttpResponse &other) {
	*this = other;
}

HttpResponse &HttpResponse::operator=(const HttpResponse &other) {
	if (this != &other) {
		_status_code = other._status_code;
		_status_message = other._status_message;
		_headers = other._headers;
		_body = other._body;
	}
	return *this;
}

void HttpResponse::setHeader(const std::string &key, const std::string &value) {
	_headers[key] = value;
}

void HttpResponse::setBody(const std::string &body){
	_body = body;
	std::stringstream ss;
	ss << _body.size();
	setHeader("Content-Length", ss.str());
};

void HttpResponse::setCookie(const std::string &name, const std::string &value) {
	setHeader("Set-Cookie", name + "=" + value + "; Path=/; HttpOnly");
}

std::string HttpResponse::getBody() const { return _body; }

void HttpResponse::setStatusCode(int code){
	_status_code = code;
	_status_message = getStatusMsg(code);
};

std::string HttpResponse::serializer() {
	std::stringstream response;
	response << "HTTP/1.1 " << _status_code << " " << _status_message <<"\r\n";

	std::map<std::string, std::string>::const_iterator it;
	for (it = _headers.begin(); it != _headers.end(); ++it)
		response << it->first << ": " << it->second << "\r\n";
	response << "\r\n";
	response << _body;
	return response.str();
};

std::string HttpResponse::getStatusMsg(int code) const
{
	switch(code) {
		case 200 : return "OK";
		case 201 : return "Created";
		case 301 : return "Moved Permanently";
		case 400 : return "Bad Request";
		case 403 : return "Forbidden";
		case 404 : return "Not Found";
		case 405 : return "Method not Allowed";
		case 413 : return "Payload Too Large";
		case 501 : return "Not Implemented";
		case 502 : return "Bad Gateway";
		case 504 : return "Gateway Timeout";
		case 500 : return "Internal Server Error";
		default  : return "Unknown Error";
	}
}
