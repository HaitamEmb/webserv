#!/usr/bin/env python3
import os

print("Content-Type: text/plain")
print()
print("Hello from CGI")
print("method=" + os.environ.get("REQUEST_METHOD", ""))
print("query=" + os.environ.get("QUERY_STRING", ""))
