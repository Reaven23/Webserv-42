import { HOST, CYAN, RESET } from "./constants.js";
import { execute } from "./test.js";

const log = (...args) => console.log("  ", ...args);

const testCases = [
  {
    name: "GET request to '/prout' should respond 404",
    route: "/prout",
    method: "GET",
    headers: {
      Host: HOST,
    },
    version: "HTTP/1.1",
    expected: {
      status: "404",
      reason: "Not Found",
      body: "Not Found",
      headers: {
        "Content-Type": "text/html",
      },
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
