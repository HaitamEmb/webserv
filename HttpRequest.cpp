#include "HttpRequest.hpp"
#include <sstream>
#include <iostream>

static std::string lowerString(const std::string &value)
{
	std::string result = value;
	for (size_t i = 0; i < result.size(); ++i)
		if (result[i] >= 'A' && result[i] <= 'Z') result[i] = static_cast<char>(result[i] - 'A' + 'a');
	return result;
}

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
		this->_query_string = other._query_string;
		this->_is_parsed = other._is_parsed;
	}
	return *this;
	
}
void HttpRequest::parse(const std::string &buff)
{
	if (_is_parsed) return;

	std::size_t header_end = buff.find("\r\n\r\n");
	if (header_end == std::string::npos)
		return; // Headers not fully received yet

	// 1. Only parse headers ONCE
	if (_method.empty()) {
		std::string line;
		std::size_t pos = 0;
		std::size_t prev = 0;
		bool is_first_line = true;

		while((pos = buff.find("\r\n", prev)) != std::string::npos)
		{
			if (pos == prev) break; 
			line = buff.substr(prev, pos - prev);
			prev = pos + 2;
			if (is_first_line) {
				parseRequest(line);
				is_first_line = false;
			} else {
				parseHeader(line);
			}
		}
	}

	// 2. Handle Body Requirements
	if (_method == "GET" || _method == "DELETE" || (_method != "POST" && _method != "PUT")) {
		_is_parsed = true;
		return;
	}

	if (_method == "POST" || _method == "PUT") {
		std::string content_len = getHeader("Content-Length");
		
		if (lowerString(getHeader("Transfer-Encoding")) == "chunked") {
			// CRITICAL FIX: Do NOT parse chunks on every 4KB recv chunk.
			// Wait until the final "0\r\n\r\n" termination marker has arrived!
			if (buff.find("0\r\n\r\n", header_end) == std::string::npos) {
				return; // Keep reading from the socket silently without freezing CPU
			}

			// Now parse the chunks EXACTLY ONCE
			std::string decoded;
			size_t cursor = header_end + 4;
			while (cursor < buff.size())
			{
				size_t line_end = buff.find("\r\n", cursor);
				if (line_end == std::string::npos) return;
				std::string size_text = buff.substr(cursor, line_end - cursor);
				if (size_text.empty()) return;
				for (size_t digit = 0; digit < size_text.size(); ++digit) {
					char value = size_text[digit];
					if (!((value >= '0' && value <= '9') || (value >= 'a' && value <= 'f')
						|| (value >= 'A' && value <= 'F') || value == ';')) return;
				}
				unsigned long chunk_size = std::strtoul(size_text.c_str(), NULL, 16);
				cursor = line_end + 2;
				if (chunk_size == 0)
				{
					_body = decoded;
					_is_parsed = true;
					return;
				}
				if (buff.size() < cursor + chunk_size + 2) return;
				decoded.append(buff, cursor, chunk_size);
				cursor += chunk_size + 2;
			}
			return;
		} else {
			if (content_len.empty()) {
				_is_parsed = true;
				return;
			}
			
			std::size_t bodysize = std::strtoul(content_len.c_str(), NULL, 10);
			std::size_t body_start = header_end + 4;
			
			if (buff.size() >= body_start + bodysize) {
				_body = buff.substr(body_start, bodysize);
				_is_parsed = true;
			}
		}
	}
}
bool HttpRequest::parse_complete() {return _is_parsed;};

std::string HttpRequest::getBody() const {return _body; };
std::string HttpRequest::getPath() const {return _path; };
std::string HttpRequest::getVersion() const {return _version; };
std::string HttpRequest::getMethod() const {return _method; };
std::string HttpRequest::getQueryString() const {return _query_string;};
std::string HttpRequest::getHeader(const std::string &key) const {
	std::map<std::string, std::string>::const_iterator it = _headers.find(lowerString(key));
	if (it != _headers.end()){
		return it->second;
	}
	return "";
};

void HttpRequest::parseRequest(const std::string &line)
{
	std::stringstream ss(line);
	std::string raw_target;

	ss >> _method >> raw_target >> _version;
	std::size_t query_pos = raw_target.find('?');
	if (query_pos != std::string::npos)
	{
		_path = raw_target.substr(0, query_pos);
		_query_string = raw_target.substr(query_pos + 1);
	}else
	{
		_path = raw_target;
		_query_string = "";
	}
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
	_headers[lowerString(key)] = value;
}
