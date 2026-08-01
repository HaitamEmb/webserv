#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ConfigServ.hpp"
#include <string>

enum ClientState {
	READING_REQUEST,
	WRITING_RESPONSE,
	DISCONNECTED
};

class Client {
	private:
		int _socket_fd;
		ClientState _state;
		std::string _read_buff;
		std::string _write_buff;
		size_t _bytes_sent;
	public:
		HttpRequest request;
		HttpResponse response;
		ConfigServ assigned_config;

		Client(int fd, const ConfigServ &config);
		~Client();

		//getters and setters
		int getFd() const;
		void setState(ClientState state);
		const char *getWriteData() const;
		ClientState getState() const;
		size_t getRemainingBytes()const;

		void appendtoReadBuff(const char *data, size_t len);
		void setWriteBuff(const std::string &data);
		void advanceWrite(size_t bytes_written);
		std::string &getReadBuff();
		
};
#endif