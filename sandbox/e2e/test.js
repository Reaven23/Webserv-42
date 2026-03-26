import { Socket } from "net";
import { HOST, PORT, GREEN, RESET } from "./constants.js";
import { logError } from "./logError.js";
import { buildRawQuery, parseResponse } from "./helpers.js";

const log = (...args) => console.log("  ", ...args);

function test(testCase, result) {
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

  pass && log(`${GREEN}[PASS] \u2705 - ${testCase.name}${RESET}`);

  return pass;
}

export function execute(testCase) {
  return new Promise((resolve) => {
    const client = new Socket();

    const rawQuery = buildRawQuery(testCase);

    client.connect(PORT, HOST, () => client.write(rawQuery));

    let response = "";
    client.on("data", (chunk) => {
      response += chunk.toString();
    });

    client.on("end", () => {
      const result = parseResponse(response);

      resolve(test(testCase, result));
    });
  });
}
