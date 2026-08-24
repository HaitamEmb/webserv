#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ConfigParser.hpp"
#include <iostream>
#include <cassert>

void testGetRequest() {
    std::cout << "[Test 1] Testing standard GET request... ";
    
    std::string raw_get = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "\r\n";

    HttpRequest req;
    req.parse(raw_get);

    // Verify extraction works
    assert(req.getMethod() == "GET");
    assert(req.getPath() == "/index.html");
    assert(req.getVersion() == "HTTP/1.1");
    assert(req.getHeader("Host") == "localhost:8080");
    // Verify GET request is complete immediately after trailing CRLF
    assert(req.parse_complete() == true);

    std::cout << "PASSED!" << std::endl;
}

void testPartialPostRequest() {
    std::cout << "[Test 2] Testing partial/fragmented POST request... ";

    // Stage 1: Network only sends headers and part of the body
    std::string dynamic_stream = 
        "POST /submit-form HTTP/1.1\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "Hello"; // Only 5 bytes out of 11 sent so far

    HttpRequest req;
    req.parse(dynamic_stream);

    assert(req.getMethod() == "POST");
    assert(req.getPath() == "/submit-form");
    assert(req.getBody() == "Hello");
    // It must realize it's incomplete!
    assert(req.parse_complete() == false);

    // Stage 2: Network simulates receiving the rest of the payload
    dynamic_stream += " world"; // 5 + 6 = 11 bytes total
    req.parse(dynamic_stream);

    assert(req.getBody() == "Hello world");
    // Now it should clear the completeness gate
    assert(req.parse_complete() == true);

    std::cout << "PASSED!" << std::endl;
}


void testResponseGeneration() {
    std::cout << "[Test 3] Testing HttpResponse serialization...\n ";

    HttpResponse res;
    res.setStatusCode(404);
    res.setHeader("Content-Type", "text/html");
    res.setBody("<h1>404 Not Found</h1>");

    std::string raw_output = res.serializer();

    //std::cout << raw_output << std::endl;

    // Assert that status line, auto-calculated body length, and structure are perfect
    assert(raw_output.find("HTTP/1.1 404 Not Found\r\n") == 0);
    assert(raw_output.find("Content-Length: 22\r\n") != std::string::npos);
    assert(raw_output.find("\r\n\r\n<h1>404 Not Found</h1>") != std::string::npos);

    std::cout << "PASSED!" << std::endl;
}


void testConfigParser() {
    std::cout << "[Test 4] Testing ConfigParser loading default.conf... ";

    ConfigParser parser("default.conf");
    bool parsed_ok = parser.parse();
    assert(parsed_ok == true);

    std::vector<ConfigServ> servers = parser.getServers();
    assert(servers.size() == 1);

    // Vérification de la configuration serveur
    ConfigServ server = servers[0];
    assert(server.getPort() == 8080);
    //std::string hh = server.getHost();
    //int c = hh.compare("127.0.0.1");
    //std::cout << "this is the host" << hh << std::endl;
    assert(server.getHost() == "127.0.0.1");
    assert(server.getServer().size() == 2);
    assert(server.getServer()[0] == "localhost");

    // Vérification des routes (locations)
    std::vector<ConfigLoc> locations = server.getLocs();
    assert(locations.size() == 5);

    // Location /
    ConfigLoc loc1 = locations[0];
    assert(loc1.getPath() == "/");
    assert(loc1.getRoot() == "./www");
    assert(loc1.getAutoIndex() == true);
    assert(loc1.AllowedMethod("GET") == true);
    assert(loc1.AllowedMethod("DELETE") == false);
    assert(loc1.getMaxBodySize() == 2000000);

    // Location /uploads
    ConfigLoc loc2 = locations[1];
    assert(loc2.getPath() == "/uploads");
    assert(loc2.AllowedMethod("GET") == true);
    assert(loc2.AllowedMethod("DELETE") == true);
    assert(loc2.getAutoIndex() == false);

    // Location /cgi
    ConfigLoc loc3 = locations[2];
    assert(loc3.getPath() == "/cgi");
    assert(loc3.getCgiExtension() == ".py");
    assert(loc3.getCgiExecutor() == "/usr/bin/python3");

    // Location /redirect
    ConfigLoc loc4 = locations[3];
    assert(loc4.getReturnUrl() == "/");

    // Location /session
    ConfigLoc loc5 = locations[4];
    assert(loc5.getSessions() == true);

    std::cout << "PASSED!" << std::endl;
}



int main() {
    std::cout << "=== STARTING WEBSERV UNIT TESTS ===" << std::endl;
    
    testGetRequest();
    testPartialPostRequest();
    testResponseGeneration();
    testConfigParser();

    std::cout << "=== ALL TESTS PASSED SUCCESSFULLY ===" << std::endl;
    return 0;
}


//to test
//c++ -Wall -Wextra -Werror -std=c++98 test_parser.cpp HttpRequest.cpp HttpResponse.cpp -o software_test
//./software_test