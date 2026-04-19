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
      if (stat.isFile()) {
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

const body = "<html><body>";
body += "<h1>Clear Upload</h1>";
body += "<p>Removed " + removed.length + " file(s)</p>";
if (removed.length > 0) {
  body += "<ul>";
  for (let i = 0; i < removed.length; i++) {
    body += "<li>" + removed[i] + "</li>";
  }
  body += "</ul>";
}
if (errors.length > 0) {
  body += "<p>Errors:</p><ul>";
  for (let i = 0; i < errors.length; i++) {
    body += "<li>" + errors[i] + "</li>";
  }
  body += "</ul>";
}
body += "</body></html>";

process.stdout.write("Content-Type: text/html\r\n");
process.stdout.write("Content-Length: " + body.length + "\r\n");
process.stdout.write("\r\n");
process.stdout.write(body);
