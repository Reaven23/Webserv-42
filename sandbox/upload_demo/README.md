# Upload Demo

Mini site for testing GET / POST / DELETE handlers.

## Launch

```bash
# Build (in Docker)
docker run --rm -it -v "$PWD":/work -w /work ubuntu:22.04 \
  bash -lc "apt-get update && apt-get install -y build-essential && make re -j"

# Create uploads dir (needed once)
mkdir -p sandbox/upload_demo/uploads

# Run
docker run --rm -it --name webserv42 \
  -v "$PWD":/work -w /work -p 8080:8080 \
  ubuntu:22.04 bash -lc "./webserv ./sandbox/upload_demo/test.conf"
```

Open http://localhost:8080/ in your browser.

## What you can test

| Feature | How |
|---------|-----|
| Static page (GET) | Open http://localhost:8080/ |
| Upload file (POST) | Use the forms on the page |
| Browse uploads (GET + autoindex) | Click "Ouvrir /uploads/" |
| Delete file (DELETE) | curl commands on the page |
| 404 error | Click a broken link |
| 405 Method Not Allowed | POST to / (only GET allowed) |
| 413 Payload Too Large | POST > 10KB body |
| 403 Path traversal | /%2e%2e/etc/passwd |
| 400 Null byte | /%00test |
