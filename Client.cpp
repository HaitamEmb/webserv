#include "Client.hpp"
#include <unistd.h>

Client::Client(int fd, const ConfigServ &config) : _socket_fd(fd), _state(READING_REQUEST), _bytes_sent(0), _cgi(NULL), _last_activity(std::time(NULL)), assigned_config(config){

}

Client::~Client() { delete _cgi; };

int Client::getFd() const {return _socket_fd;};
void Client::setState(ClientState state) { _state = state;};
ClientState Client::getState() const {return _state;};
CgiHandler *Client::getCgi() const { return _cgi; }
void Client::setCgi(CgiHandler *cgi) { _cgi = cgi; }
void Client::touch() { _last_activity = std::time(NULL); }
std::time_t Client::getLastActivity() const { return _last_activity; }

void Client::appendtoReadBuff(const char *data, size_t len)
{
	_read_buff.append(data, len);
}

std::string &Client::getReadBuff()
{
	return _read_buff;
}

void Client::setWriteBuff(const std::string &data)
{
	_write_buff = data;
	_bytes_sent = 0;
}

const char *Client::getWriteData() const {
	return _write_buff.c_str() + _bytes_sent;
}

size_t Client::getRemainingBytes() const {
	return _write_buff.size() - _bytes_sent;
}

void Client::advanceWrite(size_t bytes_written)
{
	_bytes_sent += bytes_written;
}
