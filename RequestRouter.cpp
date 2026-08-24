#include "RequestRouter.hpp"
#include "Autoindex.hpp"
#include "CgiHandler.hpp"
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <map>

static std::map<std::string, int> sessions;
static unsigned long session_counter = 0;

const ConfigLoc *RequestRouter::_matchLocation(const std::string& path, const ConfigServ &config) {
	const ConfigLoc* best_match = NULL;
	size_t longest_match_len = 0;

	const std::vector<ConfigLoc> &locations = config.getLocs();
	for (size_t i = 0; i < locations.size(); ++i) {
		const std::string& loc_path = locations[i].getPath();
		if (path.find(loc_path) == 0
			&& (loc_path == "/" || path.length() == loc_path.length()
			|| path[loc_path.length()] == '/'))
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

HttpResponse RequestRouter::_generateErrorResponse(int code, const std::string& message, const ConfigServ *config) {
	HttpResponse res;
	res.setStatusCode(code);
	res.setHeader("Content-Type", "text/html");
	if (config != NULL && !config->getErrorPage(code).empty()) {
		std::ifstream error_file(config->getErrorPage(code).c_str(), std::ios::binary);
		if (error_file.is_open()) {
			std::stringstream error_body;
			error_body << error_file.rdbuf();
			res.setBody(error_body.str());
			return res;
		}
	}
	
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
	if (req.getPath().find("..") != std::string::npos)
		return _generateErrorResponse(400, "Bad Request");
	const ConfigLoc* loc = _matchLocation(req.getPath(), config);
	if (!loc)
		return _generateErrorResponse(404, "Not Found", &config);
	if (!loc->getReturnUrl().empty()) {
		HttpResponse redirect;
		redirect.setStatusCode(301);
		redirect.setHeader("Location", loc->getReturnUrl());
		redirect.setBody("<h1>301 Moved Permanently</h1>");
		return redirect;
	}

	if (!loc->AllowedMethod(req.getMethod()))
		return _generateErrorResponse(405, "Method Not Allowed", &config);

	if (req.getMethod() == "POST" && req.getBody().size() > loc->getMaxBodySize())
		return _generateErrorResponse(413, "Payload Too Large", &config);

	std::string relative_path = req.getPath().substr(loc->getPath().length());
	if (!relative_path.empty() && relative_path[0] == '/') 
		relative_path = relative_path.substr(1);
	
	std::string full_path = loc->getRoot();
	if (!full_path.empty() && full_path[full_path.length() - 1] != '/') 
		full_path += "/";
	full_path += relative_path;

	if (req.getMethod() == "GET") {
		HttpResponse response = _handleGet(req, *loc, config);
		if (loc->getSessions()) {
			std::string cookie = req.getHeader("Cookie");
			size_t start = cookie.find("WEBSESSID=");
			std::string id;
			if (start != std::string::npos) {
				start += 10;
				size_t end = cookie.find(';', start);
				id = cookie.substr(start, end == std::string::npos ? std::string::npos : end - start);
			}
			if (id.empty() || sessions.find(id) == sessions.end()) {
				std::stringstream generated;
				generated << std::time(NULL) << "-" << ++session_counter;
				id = generated.str();
				sessions[id] = 0;
				response.setCookie("WEBSESSID", id);
			}
			++sessions[id];
			std::stringstream body;
			body << response.getBody() << "\nSession requests: " << sessions[id] << "\n";
			response.setBody(body.str());
		}
		return response;
	} else if (req.getMethod() == "POST") {
		return _handlePost(req, *loc);
	} else if (req.getMethod() == "DELETE") {
		return _handleDelete(full_path, config);
	}

	return _generateErrorResponse(501, "Not Implemented", &config);
}

CgiHandler *RequestRouter::createCgi(const HttpRequest &req, const ConfigServ &config) {
	const ConfigLoc *loc = _matchLocation(req.getPath(), config);
	if (loc == NULL)
		return NULL;
	std::string extension = loc->getCgiExtension();
	std::string executor = loc->getCgiExecutor();
	const std::vector<std::pair<std::string, std::string> > &handlers = loc->getCgiHandlers();
	for (size_t i = 0; i < handlers.size(); ++i)
		if (req.getPath().size() >= handlers[i].first.size()
			&& req.getPath().substr(req.getPath().size() - handlers[i].first.size()) == handlers[i].first) {
			extension = handlers[i].first;
			executor = handlers[i].second;
			break;
		}
	if (extension.empty() || executor.empty() || req.getPath().size() < extension.size()
		|| req.getPath().substr(req.getPath().size() - extension.size()) != extension) return NULL;
	std::string relative_path = req.getPath().substr(loc->getPath().length());
	if (!relative_path.empty() && relative_path[0] == '/') relative_path = relative_path.substr(1);
	std::string full_path = loc->getRoot();
	if (!full_path.empty() && full_path[full_path.length() - 1] != '/') full_path += "/";
	full_path += relative_path;
	return new CgiHandler(req, *loc, full_path, executor);
}

bool RequestRouter::bodyTooLarge(const HttpRequest &req, const ConfigServ &config) {
	const ConfigLoc *loc = _matchLocation(req.getPath(), config);
	if (loc == NULL) return false;
	std::string length = req.getHeader("Content-Length");
	if (!length.empty() && std::strtoul(length.c_str(), NULL, 10) > loc->getMaxBodySize()) return true;
	return req.getBody().size() > loc->getMaxBodySize();
}

HttpResponse RequestRouter::_handlePost(const HttpRequest &req, const ConfigLoc &loc) {
	if (!loc.getUpload())
		return _generateErrorResponse(405, "Method Not Allowed");

	std::string directory = loc.getUploadPath();
	if (directory.empty())
		directory = loc.getRoot();
	std::string body = req.getBody();
	std::string filename = req.getHeader("X-Filename");
	std::string content_type = req.getHeader("Content-Type");
	if (filename.empty() && content_type.find("multipart/form-data") == 0) {
		size_t boundary_pos = content_type.find("boundary=");
		if (boundary_pos != std::string::npos) {
			std::string boundary = "--" + content_type.substr(boundary_pos + 9);
			size_t name_pos = body.find("filename=\"");
			size_t data_pos = body.find("\r\n\r\n", name_pos);
			if (name_pos != std::string::npos && data_pos != std::string::npos) {
				name_pos += 10;
				size_t name_end = body.find('"', name_pos);
				filename = body.substr(name_pos, name_end - name_pos);
				data_pos += 4;
				size_t data_end = body.find("\r\n" + boundary, data_pos);
				if (data_end != std::string::npos) body = body.substr(data_pos, data_end - data_pos);
			}
		}
	}
	if (filename.empty())
		filename = "upload.bin";
	for (size_t i = 0; i < filename.size(); ++i) {
		if (filename[i] == '/' || filename[i] == '\\')
			return _generateErrorResponse(400, "Invalid Filename");
	}
	if (filename == "." || filename == "..")
		return _generateErrorResponse(400, "Invalid Filename");
	if (directory.empty() || filename.empty())
		return _generateErrorResponse(500, "Upload Location Not Configured");
	if (directory[directory.size() - 1] != '/')
		directory += "/";
	std::ofstream file((directory + filename).c_str(), std::ios::binary);
	if (!file.is_open())
		return _generateErrorResponse(500, "Unable to Store Upload");
	file.write(body.data(), body.size());
	if (!file.good())
		return _generateErrorResponse(500, "Unable to Store Upload");
	HttpResponse res;
	res.setStatusCode(201);
	res.setHeader("Content-Type", "text/plain");
	res.setBody("Upload stored successfully\n");
	return res;
}

HttpResponse RequestRouter::_handleGet(const HttpRequest& req, const ConfigLoc &loc, const ConfigServ &config) {
	std::string relative_path = req.getPath().substr(loc.getPath().length());
	if (!relative_path.empty() && relative_path[0] == '/') relative_path = relative_path.substr(1);

	std::string full_path = loc.getRoot();
	if (!full_path.empty() && full_path[full_path.length() - 1] != '/') 
		full_path += "/";
	full_path += relative_path;

	struct stat path_stat;
	if (stat(full_path.c_str(), &path_stat) != 0) {
		return _generateErrorResponse(404, "Not Found", &config);
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
				return _generateErrorResponse(500, "Internal Server Error", &config);
			HttpResponse res;
			res.setStatusCode(200);
			res.setHeader("Content-Type", "text/html");
			res.setBody(listing);
			return res;
		}
		else
			return _generateErrorResponse(403, "Forbidden", &config);
	}

	std::ifstream file(full_path.c_str(), std::ios::binary);
	if (!file.is_open()) {
		return _generateErrorResponse(403, "Forbidden", &config);
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	HttpResponse res;
	res.setStatusCode(200);
	res.setHeader("Content-Type", _getMimeType(full_path));
	res.setBody(buffer.str());
	return res;
}

HttpResponse RequestRouter::_handleDelete(const std::string& full_path, const ConfigServ &config) {
	struct stat path_stat;
	if (stat(full_path.c_str(), &path_stat) != 0) {
		return _generateErrorResponse(404, "Not Found", &config);
	}

	if (std::remove(full_path.c_str()) == 0) {
		HttpResponse res;
		res.setStatusCode(200);
		res.setHeader("Content-Type", "text/html");
		res.setBody("<h1>File successfully deleted</h1>");
		return res;
	}

	return _generateErrorResponse(500, "Internal Server Error", &config);
}