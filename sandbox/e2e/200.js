import { HOST, CYAN, RESET } from "./constants.js";
import { execute } from "./test.js";

const log = (...args) => console.log("  ", ...args);

const testCases = [
  {
    name: "GET request to '/' should respond 200",
    route: "/",
    method: "GET",
    headers: {
      Host: HOST,
    },
    version: "HTTP/1.1",
    expected: {
      status: "200",
    },
  },
  {
    name: "GET request to '/' with 'Connection: keep-alive' header should respond 200",
    route: "/",
    method: "GET",
    headers: {
      Host: HOST,
      Connection: "keep-alive",
    },
    version: "HTTP/1.1",
    expected: {
      status: "200",
    },
  },
  {
    name: "GET request to '/' with 'Connection: keep-alive' and 'Accept' headers should respond 200",
    route: "/",
    method: "GET",
    headers: {
      Host: HOST,
      Connection: "keep-alive",
      Accept: "*/*",
    },
    version: "HTTP/1.1",
    expected: {
      status: "200",
    },
  },
  {
    name: "GET request to '/' with 'Connection: keep-alive', 'Accept' and 'User-Agent' headers should respond 200",
    route: "/",
    method: "GET",
    headers: {
      Host: HOST,
      Connection: "keep-alive",
      Accept: "*/*",
      "User-Agent": "node",
    },
    version: "HTTP/1.1",
    expected: {
      status: "200",
    },
  },
  {
    name: "GET request to '/' should respond with correct html",
    route: "/",
    method: "GET",
    headers: {
      Host: HOST,
      Connection: "keep-alive",
      Accept: "*/*",
      "User-Agent": "node",
    },
    version: "HTTP/1.1",
    expected: {
      status: "200",
      body: `<html>
  <body>
    <h1>Hello Webserv</h1>
  </body>
</html>
`,
    },
  },
  {
    name: "GET request to '/' should respond with 'Content-Type: text/html' header",
    route: "/",
    method: "GET",
    headers: {
      Host: HOST,
      Connection: "keep-alive",
      Accept: "*/*",
      "User-Agent": "node",
    },
    version: "HTTP/1.1",
    expected: {
      status: "200",
      headers: {
        "Content-Type": "text/html",
      },
    },
  },
];

async function run() {
  log(`\n${CYAN}##### - Starting 200 suite... - #####${RESET}`);

  let failsCount = 0;
  for (const testCase of testCases) {
    failsCount += (await execute(testCase)) ? 0 : 1;
  }

  return {
    failed: failsCount,
    passed: testCases.length - failsCount,
    total: testCases.length,
  };
}

export default run;
