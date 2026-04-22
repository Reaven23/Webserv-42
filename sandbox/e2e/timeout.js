import { Socket } from "net";
import { HOST, PORT, CYAN, GREEN, RESET } from "./constants.js";
import { logError } from "./logError.js";

const log = (...args) => console.log("  ", ...args);

const testCases = [
  {
    name: "Incomplete GET request should timeout after 60s",
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
    duration: 62000,
  },
];

async function execute(testCase) {
  const { name, ...testInput } = testCase;
  return new Promise((resolve) => {
    const client = new Socket();

    const incompleteRequest = "GET / HTTP/1.1\r\nHost: localhost\r\n";
    let resolved = false;

    const start = Date.now();
    client.connect(PORT, HOST, () => client.write(incompleteRequest));

    const failTimer = setTimeout(() => {
      if (resolved) return;
      resolved = true;

      logError({
        name: testCase.name,
        input: testInput,
        errorProperty: "",
        errorValue: "connection still open",
        expectedValue: `connection closed after ${testCase.duration / 1000}s`,
      });

      client.destroy();
      resolve(false);
    }, testCase.duration * 2);

    client.on("end", () => {
      if (resolved) return;
      resolved = true;
      clearTimeout(failTimer);

      const elapsed = Date.now() - start;

      if (elapsed > testCase.duration) {
        logError({
          name: name,
          input: testInput,
          errorProperty: "",
          errorValue: `connection closed after ${elapsed / 1000}s`,
          expectedValue: `connection closed after ${testCase.duration / 1000}s`,
        });

        resolve(false);
      } else {
        log(`${GREEN}[PASS] \u2705 - ${testCase.name}${RESET}`);
        resolve(true);
      }
    });
  });
}

async function run() {
  log(`\n${CYAN}##### - Starting timeout suite... - #####${RESET}`);

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
