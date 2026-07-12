#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
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

int main() {
    std::cout << "=== STARTING HTTP PARSER UNIT TESTS ===" << std::endl;
    
    testGetRequest();
    testPartialPostRequest();
    testResponseGeneration();

    std::cout << "=== ALL TESTS PASSED SUCCESSFULLY ===" << std::endl;
    return 0;
}


//to test
//c++ -Wall -Wextra -Werror -std=c++98 test_parser.cpp HttpRequest.cpp HttpResponse.cpp -o software_test
//./software_test