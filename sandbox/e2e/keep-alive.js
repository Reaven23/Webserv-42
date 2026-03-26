import { Socket } from "net";
import { buildRawQuery } from "./helpers.js";
import { HOST, PORT, CYAN, GREEN, RESET } from "./constants.js";
import { logError } from "./logError.js";

const log = (...args) => console.log("  ", ...args);

const testCases = [
  {
    name: "GET request with 'Connection: keep-alive' should not be closed",
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
];

async function execute(testCase) {
  const { expected, ...testInput } = testCase;

  return new Promise((resolve) => {
    const client = new Socket();

    const rawQuery = buildRawQuery(testCase);

    client.connect(PORT, HOST, () => client.write(rawQuery));

    let isConnectionClosed = false;
    let hasResponded = false;

    let response = "";
    client.on("data", (chunk) => {
      response += chunk;
      hasResponded = true;
    });

    client.on("end", () => {
      isConnectionClosed = true;
    });

    setTimeout(() => {
      if (!hasResponded || isConnectionClosed) {
        logError({
          name: testCase.name,
          input: testInput,
          errorProperty: "keep-alive",
          errorValue: isConnectionClosed
            ? "connection closed"
            : "no response from server",
          expectedValue: "connection still open after response",
        });
        resolve(false);
      }

      if (response.includes("\r\n\r\n")) {
        hasResponded = false;
        response = "";
        client.write(rawQuery);

        if (!hasResponded || isConnectionClosed) {
          logError({
            name: testCase.name,
            input: testInput,
            errorProperty: "keep-alive",
            errorValue: isConnectionClosed
              ? "connection closed"
              : "no response from server",
            expectedValue: "connection still open after response",
          });
          resolve(false);
        }
      }

      log(`${GREEN}[PASS] \u2705 - ${testCase.name}${RESET}`);
      client.destroy();
      resolve(true);
    }, 2000);
  });
}

async function run() {
  log(
    `\n${CYAN}##### - Starting 'Connection: keep-alive' suite... - #####${RESET}`,
  );

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
