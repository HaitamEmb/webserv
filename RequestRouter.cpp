#include "RequestRouter.hpp"
#include "Autoindex.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>

const ConfigLoc *RequestRouter::_matchLocation(const std::string& path, const ConfigServ &config) {
	const ConfigLoc* best_match = NULL;
	size_t longest_match_len = 0;

	std::vector<ConfigLoc> locations = config.getLocs();
	for (size_t i = 0; i < locations.size(); ++i) {
		const std::string& loc_path = locations[i].getPath();
		if (path.find(loc_path) == 0) 
		{
		if (loc_path.length() > longest_match_len) 
		{
			longest_match_len = loc_path.length();
			best_match = &locations[i];
		}
		}
	}
	return best_match;
}

HttpResponse RequestRouter::_generateErrorResponse(int code, const std::string& message) {
	HttpResponse res;
	res.setStatusCode(code);
	res.setHeader("Content-Type", "text/html");
	
	std::stringstream ss;
	ss << "<html><head><title>" << code << " " << message << "</title></head>"
	<< "<body><center><h1>" << code << " " << message << "</h1></center><hr><center>Webserv/1.0</center></body></html>";
	
	res.setBody(ss.str());
	return res;
}

std::string RequestRouter::_getMimeType(const std::string& path) {
	if (path.find(".html") != std::string::npos || path.find(".htm") != std::string::npos) return "text/html";
	if (path.find(".css") != std::string::npos) return "text/css";
	if (path.find(".js") != std::string::npos) return "text/javascript";
	if (path.find(".png") != std::string::npos) return "image/png";
	if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos) return "image/jpeg";
	if (path.find(".json") != std::string::npos) return "application/json";
	return "text/plain";
}

HttpResponse RequestRouter::routeRequest(const HttpRequest& req, const ConfigServ &config) 
{
	const ConfigLoc* loc = _matchLocation(req.getPath(), config);
	if (!loc)
		return _generateErrorResponse(404, "Not Found");

	if (!loc->AllowedMethod(req.getMethod()))
		return _generateErrorResponse(405, "Method Not Allowed");

	if (req.getMethod() == "POST" && req.getBody().size() > loc->getMaxBodySize())
		return _generateErrorResponse(413, "Payload Too Large");

	std::string relative_path = req.getPath().substr(loc->getPath().length());
	if (!relative_path.empty() && relative_path[0] == '/') 
		relative_path = relative_path.substr(1);
	
	std::string full_path = loc->getRoot();
	if (!full_path.empty() && full_path[full_path.length() - 1] != '/') 
		full_path += "/";
	full_path += relative_path;

	if (req.getMethod() == "GET") {
		return _handleGet(req, *loc);
	} else if (req.getMethod() == "DELETE") {
		return _handleDelete(full_path);
	}

	return _generateErrorResponse(501, "Not Implemented");
}

HttpResponse RequestRouter::_handleGet(const HttpRequest& req, const ConfigLoc &loc) {
	std::string relative_path = req.getPath().substr(loc.getPath().length());
	if (!relative_path.empty() && relative_path[0] == '/') relative_path = relative_path.substr(1);

	std::string full_path = loc.getRoot();
	if (!full_path.empty() && full_path[full_path.length() - 1] != '/') 
		full_path += "/";
	full_path += relative_path;

	struct stat path_stat;
	if (stat(full_path.c_str(), &path_stat) != 0) {
		return _generateErrorResponse(404, "Not Found");
	}

	if (S_ISDIR(path_stat.st_mode)) {
		std::string index_path = full_path;
		if (index_path[index_path.length() - 1] != '/') 
			index_path += "/";
		index_path += loc.getIndex();

		struct stat index_stat;
		
		if (stat(index_path.c_str(), &index_stat) == 0 && !S_ISDIR(index_stat.st_mode))
			full_path = index_path;
		else if (loc.getAutoIndex())
		{
			std::string listing = Autoindex::generateListing(full_path, req.getPath());
			if (listing.empty())
				return _generateErrorResponse(500, "Internal Server Error");
			HttpResponse res;
			res.setStatusCode(200);
			res.setHeader("Content-Type", "text/html");
			res.setBody(listing);
			return res;
		}
		else
			return _generateErrorResponse(403, "Forbidden");
	}

	std::ifstream file(full_path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		return _generateErrorResponse(403, "Forbidden");
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	HttpResponse res;
	res.setStatusCode(200);
	res.setHeader("Content-Type", _getMimeType(full_path));
	res.setBody(buffer.str());
	return res;
}

HttpResponse RequestRouter::_handleDelete(const std::string& full_path) {
	struct stat path_stat;
	if (stat(full_path.c_str(), &path_stat) != 0) {
		return _generateErrorResponse(404, "Not Found");
	}

	if (std::remove(full_path.c_str()) == 0) {
		HttpResponse res;
		res.setStatusCode(200);
		res.setHeader("Content-Type", "text/html");
		res.setBody("<h1>File successfully deleted</h1>");
		return res;
	}

	return _generateErrorResponse(500, "Internal Server Error");
}