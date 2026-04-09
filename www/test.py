#!/usr/bin/env python3
import os

query = os.environ.get("QUERY_STRING", "")
method = os.environ.get("REQUEST_METHOD", "GET")

body = "<html><body>"
body += "<h1>CGI Test</h1>"
body += "<p>Method: " + method + "</p>"
body += "<p>Query: " + query + "</p>"
body += "</body></html>"

print("Content-Type: text/html\r")
print("Content-Length: " + str(len(body)) + "\r")
print("\r")
print(body, end="")
