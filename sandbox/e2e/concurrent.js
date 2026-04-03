import { Socket } from "net";
import { PORT, HOST, CYAN, GREEN, RESET } from "./constants.js";
import { buildRawQuery, parseResponse } from "./helpers.js";
import { logError } from "./logError.js";

const log = (...args) => console.log("  ", ...args);

const testCases = [
  {
    name: "Multiple concurrent GET requests should respond with 200",
    requests: [
      {
        name: "GET request to '/' should respond with 200",
        route: "/",
        method: "GET",
        headers: {
          Host: HOST,
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
      {
        name: "GET request to '/' should respond with 200",
        route: "/",
        method: "GET",
        headers: {
          Host: HOST,
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
      {
        name: "GET request to '/' should respond with 200",
        route: "/",
        method: "GET",
        headers: {
          Host: HOST,
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
    ],
  },
  {
    name: "Multiple concurrent GET requests (slow/fast) should respond with 200",
    requests: [
      {
        name: "GET request to '/' should respond with 200",
        route: "/",
        method: "GET",
        headers: {
          Host: HOST,
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
        isSlow: true,
      },
      {
        name: "GET request to '/' should respond with 200",
        route: "/",
        method: "GET",
        headers: {
          Host: HOST,
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
      {
        name: "GET request to '/' should respond with 200",
        route: "/",
        method: "GET",
        headers: {
          Host: HOST,
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
    ],
  },
];

export function test(testCase, result) {
  const { expected, ...testInput } = testCase;
  const { statusCode, reason, body, headers } = result;

  let pass = true;

  if (statusCode !== expected.status) {
    logError({
      name: testCase.name,
      input: testInput,
      errorProperty: "status",
      errorValue: statusCode,
      expectedValue: expected.status,
    });

    pass = false;
  } else if (expected.reason && reason !== expected.reason) {
    logError({
      name: testCase.name,
      input: testInput,
      errorProperty: "reason",
      errorValue: reason,
      expectedValue: expected.reason,
    });

    pass = false;
  } else if (expected.headers) {
    Object.entries(expected.headers).forEach(([key, value]) => {
      const targetHeader = headers[key];

      if (!targetHeader || targetHeader !== value) {
        logError({
          name: testCase.name,
          input: testInput,
          errorProperty: key,
          errorValue: targetHeader,
          expectedValue: expected.headers[key],
        });

        pass = false;
        return;
      }
    });
  } else if (expected.body && body !== expected.body) {
    logError({
      name: testCase.name,
      input: testInput,
      errorProperty: "body",
      errorValue: body,
      expectedValue: expected.body,
    });

    pass = false;
  }

  return pass;
}

async function executeWithDelay(testCase, delay) {
  return new Promise((resolve) => {
    const client = new Socket();

    const rawQuery = buildRawQuery(testCase);

    let slowTimer = null;
    let slowedQuery = null;
    if (testCase.isSlow) slowedQuery = rawQuery.slice(0, rawQuery.length - 2);

    client.connect(PORT, HOST, () => {
      setTimeout(() => client.write(slowedQuery ?? rawQuery), delay);
      if (testCase.isSlow)
        slowTimer = setTimeout(() => client.write("\r\n"), 5000);
    });

    let response = "";
    client.on("data", (chunk) => {
      response += chunk.toString();
    });

    client.on("end", () => {
      slowTimer && clearTimeout(slowTimer);
      const result = parseResponse(response);

      resolve(test(testCase, result));
    });
  });
}

async function run() {
  log(`\n${CYAN}##### - Starting concurrent requests suite... - #####${RESET}`);

  let failsCount = 0;
  for (const testCase of testCases) {
    const results = await Promise.all(
      testCase.requests.map((request, i) => executeWithDelay(request, i * 10)),
    );

    let pass = true;
    results.forEach((result) => {
      if (!result) pass = false;
    });

    pass && log(`${GREEN}[PASS] \u2705 - ${testCase.name}${RESET}`);
    failsCount += pass ? 0 : 1;
  }

  return {
    failed: failsCount,
    passed: testCases.length - failsCount,
    total: testCases.length,
  };
}

export default run;
