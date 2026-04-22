#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

const uploadDir = path.join(__dirname, "..", "..", "upload");
const removed = [];
const errors = [];

try {
  const files = fs.readdirSync(uploadDir);
  for (let i = 0; i < files.length; i++) {
    const filePath = path.join(uploadDir, files[i]);
    try {
      const stat = fs.statSync(filePath);
      if (stat.isFile() && !files[i].startsWith(".")) {
        fs.unlinkSync(filePath);
        removed.push(files[i]);
      }
    } catch (e) {
      errors.push(files[i] + ": " + e.message);
    }
  }
} catch (e) {
  errors.push("readdir: " + e.message);
}

let body = `<!doctype html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>Clear Upload</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: "Segoe UI", -apple-system, sans-serif; background: #0a0a0f; color: #c9d1d9; min-height: 100vh; }
    header { background: linear-gradient(135deg, #0d1117 0%, #161b22 100%); padding: 40px 24px 32px; text-align: center; border-bottom: 1px solid #21262d; }
    header h1 { font-size: 2rem; font-weight: 700; letter-spacing: -0.5px; }
    header h1 span { color: #58a6ff; }
    header p { color: #8b949e; margin-top: 8px; font-size: 0.95rem; }
    .container { max-width: 960px; margin: 28px auto; padding: 0 20px; }
    .card { background: #0d1117; border: 1px solid #21262d; border-radius: 12px; padding: 24px; margin-bottom: 16px; }
    .card h2 { font-size: 1.05rem; font-weight: 600; margin-bottom: 14px; display: flex; align-items: center; gap: 10px; }
    .card p { font-size: 0.88rem; line-height: 1.6; color: #8b949e; }
    .tag { display: inline-block; padding: 2px 10px; border-radius: 12px; font-size: 0.7rem; font-weight: 700; letter-spacing: 0.5px; }
    .tag-cgi { background: #0d2a2a; color: #56d4d4; border: 1px solid #1a4040; }
    ul { list-style: none; padding: 0; margin-top: 12px; }
    ul li { padding: 8px 12px; border-radius: 6px; margin-bottom: 4px; font-size: 0.85rem; background: #161b22; border: 1px solid #21262d; color: #c9d1d9; }
    .error-item { background: #2d0a0a; border-color: #4a1515; color: #f85149; }
    .count { font-size: 0.88rem; color: #8b949e; margin-bottom: 10px; }
    .count strong { color: #3fb950; }
    .count.none strong { color: #484f58; }
    a { color: #58a6ff; text-decoration: none; }
    a:hover { text-decoration: underline; }
    footer { text-align: center; padding: 28px 20px; color: #30363d; font-size: 0.8rem; margin-top: 20px; border-top: 1px solid #161b22; }
    footer span { color: #484f58; }
  </style>
</head>
<body>
  <header>
    <h1><span>web</span>serv</h1>
    <p>CGI &mdash; clearUpload.js</p>
  </header>
  <div class="container">
    <div class="card">
      <h2><span>&#x1F5D1;</span> Clear Upload <span class="tag tag-cgi">.js</span></h2>
      <p class="count ${removed.length === 0 ? "none" : ""}">Removed <strong>${removed.length}</strong> file(s)</p>`;

if (removed.length > 0) {
  body += `<ul>`;
  for (let i = 0; i < removed.length; i++)
    body += `<li>${removed[i]}</li>`;
  body += `</ul>`;
}
if (errors.length > 0) {
  body += `<ul style="margin-top:10px">`;
  for (let i = 0; i < errors.length; i++)
    body += `<li class="error-item">${errors[i]}</li>`;
  body += `</ul>`;
}

body += `
    </div>
    <p style="font-size:0.85rem"><a href="/">&larr; Back to dashboard</a></p>
  </div>
  <footer><span>webserv</span> &middot; 42 project</footer>
</body>
</html>`;

process.stdout.write("Content-Type: text/html\r\n");
process.stdout.write("Content-Length: " + body.length + "\r\n");
process.stdout.write("\r\n");
process.stdout.write(body);
