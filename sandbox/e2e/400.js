import { HOST, CYAN, RESET } from "./constants.js";
import { execute } from "./test.js";

const log = (...args) => console.log("  ", ...args);

const testCases = [
  {
    name: "GET request with invalid HTTP version format (HTP/1.1) should return 400 Bad Request",
    route: "/",
    method: "GET",
    headers: {
      Host: HOST,
    },
    version: "HTP/1.1",
    expected: {
      status: "400",
      reason: "Bad Request",
      body: "Bad request",
    },
  },
  {
    name: "GET request with invalid HTTP/2.0 version should return 400 Bad Request",
    route: "/",
    method: "GET",
    headers: {
      Host: HOST,
    },
    version: "HTTP/2.0",
    expected: {
      status: "400",
      reason: "Bad Request",
      body: "Bad request",
    },
  },
  {
    name: "GET request with invalid URI (empty) should return 400 Bad Request",
    route: "",
    method: "GET",
    headers: {
      Host: HOST,
    },
    version: "HTTP/1.1",
    expected: {
      status: "400",
      reason: "Bad Request",
      body: "Bad request",
    },
  },
];

async function run() {
  log(`\n${CYAN}##### - Starting 400 suite... - #####${RESET}`);

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
