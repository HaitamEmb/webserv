#include "HttpRequest.hpp"
#include <sstream>
#include <iostream>

HttpRequest::HttpRequest() : _is_parsed(false){};
HttpRequest::~HttpRequest() {};

HttpRequest::HttpRequest(HttpRequest const &other)
{
	*this = other;
}

HttpRequest &HttpRequest::operator=(HttpRequest const &other)
{
	if (this != &other)
	{
		this->_body = other._body;
		this->_headers = other._headers;
		this->_method = other._method;
		this->_path = other._path;
		this->_version = other._version;
	}
	return *this;
	
}
void HttpRequest::parse(const std::string &buff)
{
	std::string line;
	std::size_t pos = 0;
	std::size_t prev = 0;
	bool is_first_line = true;

	while((pos = buff.find("\r\n", prev)) != std::string::npos)
	{
		line = buff.substr(prev, pos - prev);
		prev = pos + 2;
		if (line.empty())
			break;
		if (is_first_line)
		{
			parseRequest(line);
			is_first_line = false;
		}
		else
			parseHeader(line);
	}

	if (prev < buff.size())
		_body = buff.substr(prev);

	std::size_t header_end = buff.find("\r\n\r\n");
	if (header_end == std::string::npos)
	{
		_is_parsed = false;
		return;
	}
	if (_method == "GET" || _method == "DELETE")
	{
		_is_parsed = true;
		return;
	}
	if (_method == "POST")
	{
		std::string content_len = getHeader("Content-Length");
		if (content_len.empty())
		{
			_is_parsed = true;
			return ;
		}
		std::size_t bodysize = std::atoi(content_len.c_str());
		if (_body.size() >= bodysize)
			_is_parsed = true;
		else
			_is_parsed = false;
	}
}

bool HttpRequest::parse_complete() {return _is_parsed;};

std::string HttpRequest::getBody() {return _body; };
std::string HttpRequest::getPath() {return _path; };
std::string HttpRequest::getVersion() {return _version; };
std::string HttpRequest::getMethod() {return _method; };
std::string HttpRequest::getHeader(const std::string &key) {
	std::map<std::string, std::string>::const_iterator it = _headers.find(key);
	if (it != _headers.end()){
		return it->second;
	}
	return "";
};

void HttpRequest::parseRequest(const std::string &line)
{
	std::stringstream ss(line);
	ss >> _method >> _path >> _version;
}

void HttpRequest::parseHeader(const std::string &line)
{
	std::size_t idx = line.find(':');
	if (idx == std::string::npos)
		return;
	std::string key = line.substr(0, idx);
	std::string value = line.substr(idx + 1);

	if (!value.empty() && value[0] == ' ')
		value.erase(0, 1);
	_headers[key] = value;
}
