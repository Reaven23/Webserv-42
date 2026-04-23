#!/usr/bin/env python3
import os
import sys
import urllib.parse

# ── Read POST body if present ──
content_length = os.environ.get("CONTENT_LENGTH", "")
post_body = ""
if content_length.isdigit() and int(content_length) > 0:
    post_body = sys.stdin.read(int(content_length))

# ── Parse query string and POST data ──
method = os.environ.get("REQUEST_METHOD", "GET")
query_string = os.environ.get("QUERY_STRING", "")
query_params = urllib.parse.parse_qs(query_string)
post_params = urllib.parse.parse_qs(post_body)

# ── Collect CGI environment variables ──
cgi_vars = [
    "REQUEST_METHOD", "QUERY_STRING", "CONTENT_TYPE", "CONTENT_LENGTH",
    "SCRIPT_NAME", "SCRIPT_FILENAME", "SERVER_NAME", "SERVER_PORT",
    "SERVER_PROTOCOL", "GATEWAY_INTERFACE",
]

# ── Build HTML ──
h = ""

def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")

h += """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>CGI Echo</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: 'Segoe UI', sans-serif; background: #0a0a0f; color: #c9d1d9; padding: 32px 20px; }
  .wrap { max-width: 720px; margin: 0 auto; }
  h1 { font-size: 1.5rem; color: #58a6ff; margin-bottom: 4px; }
  h1 span { color: #c9d1d9; font-weight: 400; }
  .subtitle { color: #484f58; font-size: 0.85rem; margin-bottom: 24px; }
  h2 { font-size: 0.95rem; color: #8b949e; margin: 20px 0 8px; text-transform: uppercase; letter-spacing: 1px; font-weight: 600; }
  table { width: 100%; border-collapse: collapse; background: #0d1117; border: 1px solid #21262d; border-radius: 8px; overflow: hidden; }
  th, td { text-align: left; padding: 8px 14px; font-size: 0.84rem; border-bottom: 1px solid #21262d; }
  th { background: #161b22; color: #58a6ff; font-weight: 600; width: 40%; }
  td { color: #c9d1d9; font-family: 'SF Mono', monospace; word-break: break-all; }
  .empty { color: #484f58; font-style: italic; }
  .method { display: inline-block; padding: 2px 10px; border-radius: 12px; font-size: 0.75rem; font-weight: 700; }
  .m-GET { background: #0d2818; color: #3fb950; }
  .m-POST { background: #2a1a00; color: #d29922; }
  .m-DELETE { background: #2d0a0a; color: #f85149; }
  pre { background: #161b22; border: 1px solid #21262d; border-radius: 8px; padding: 14px; font-size: 0.82rem; color: #e6edf3; overflow-x: auto; white-space: pre-wrap; word-break: break-all; }
  form { background: #0d1117; border: 1px solid #21262d; border-radius: 8px; padding: 16px; margin-top: 8px; display: flex; gap: 8px; align-items: center; flex-wrap: wrap; }
  input[type="text"] { flex: 1; min-width: 200px; padding: 8px 12px; border-radius: 6px; border: 1px solid #30363d; background: #0a0a0f; color: #c9d1d9; font-size: 0.85rem; }
  button { padding: 8px 18px; border-radius: 6px; border: 1px solid #2ea043; background: #238636; color: #fff; font-weight: 600; font-size: 0.85rem; cursor: pointer; }
  button:hover { background: #2ea043; }
</style>
</head>
<body>
<div class="wrap">
"""

h += '<h1><span class="method m-' + esc(method) + '">' + esc(method) + '</span> <span>CGI Echo</span></h1>\n'
h += '<p class="subtitle">This script displays everything the server passed to the CGI process.</p>\n'

# ── Environment variables table ──
h += "<h2>CGI Environment</h2>\n"
h += "<table>\n"
for var in cgi_vars:
    val = os.environ.get(var, "")
    display = esc(val) if val else '<span class="empty">not set</span>'
    h += "<tr><th>" + var + "</th><td>" + display + "</td></tr>\n"
h += "</table>\n"

# ── Query string parameters ──
h += "<h2>Query String Parameters</h2>\n"
if query_params:
    h += "<table>\n"
    for key, values in sorted(query_params.items()):
        for v in values:
            h += "<tr><th>" + esc(key) + "</th><td>" + esc(v) + "</td></tr>\n"
    h += "</table>\n"
else:
    h += '<p class="empty" style="font-size:0.85rem; margin: 4px 0;">No query parameters.</p>\n'

# ── POST body ──
h += "<h2>Request Body</h2>\n"
if post_body:
    if post_params:
        h += "<table>\n"
        for key, values in sorted(post_params.items()):
            for v in values:
                h += "<tr><th>" + esc(key) + "</th><td>" + esc(v) + "</td></tr>\n"
        h += "</table>\n"
    else:
        h += "<pre>" + esc(post_body[:4096]) + "</pre>\n"
        if len(post_body) > 4096:
            h += '<p class="empty" style="font-size:0.8rem;">... truncated (' + str(len(post_body)) + ' bytes total)</p>\n'
else:
    h += '<p class="empty" style="font-size:0.85rem; margin: 4px 0;">No body received.</p>\n'

# ── Try it forms ──
h += "<h2>Try It</h2>\n"
h += '<form method="GET" action="echo.py">\n'
h += '  <input type="text" name="message" placeholder="GET query param (message=...)">\n'
h += "  <button type=\"submit\">Send GET</button>\n"
h += "</form>\n"
h += '<form method="POST" action="echo.py">\n'
h += '  <input type="text" name="data" placeholder="POST body param (data=...)">\n'
h += '  <button type="submit">Send POST</button>\n'
h += "</form>\n"

h += "</div>\n</body>\n</html>"

# ── Output CGI response ──
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("Content-Length: " + str(len(h)) + "\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(h)
