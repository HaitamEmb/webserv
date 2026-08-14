#ifndef REQUEST_ROUTER_HPP
#define REQUEST_ROUTER_HPP

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerManager.hpp"
#include "ConfigLoc.hpp"
#include <string>


class RequestRouter {
	private:
		static const ConfigLoc *_matchLocation(const std::string &path, const ConfigServ &config);
		static HttpResponse _handleGet(const HttpRequest &req, const ConfigLoc &loc);
		static HttpResponse _handleDelete(const std::string &full_path);
		static HttpResponse _generateErrorResponse(int code, const std::string &message);
		static std::string _getMimeType(const std::string &path);
	public:
		static HttpResponse routeRequest(const HttpRequest &req, const ConfigServ &config);
};

#endif