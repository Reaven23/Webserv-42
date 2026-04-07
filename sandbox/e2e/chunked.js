import { Socket } from "net";
import { execute } from "./test.js";
import { HOST, CYAN, RESET, PORT } from "./constants.js";
import { parseResponse } from "./helpers.js";
import { test } from "./test.js";

const log = (...args) => console.log("  ", ...args);

const testCases = [
  {
    name: "POST chunked request (all chunks at once) should respond 200",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    body: "5\r\nHello\r\n6\r\n World\r\n0\r\n\r\n",
    expected: {
      status: "200",
    },
    executor: execute,
  },
  {
    name: "POST chunked request with streaming chunks should respond 200",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    chunks: ["5\r\nHello\r\n", "6\r\n World\r\n"],
    expected: {
      status: "200",
    },
    executor: chunkExecutor,
  },
  {
    name: "POST chunked request with one chunk should respond 200",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    chunks: ["5\r\nProut\r\n"],
    expected: {
      status: "200",
    },
    executor: chunkExecutor,
  },
  {
    name: "POST chunked request with empty body should respond 200",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    chunks: ["0\r\n\r\n"],
    expected: {
      status: "200",
    },
    executor: chunkExecutor,
  },
  {
    name: "POST chunked request with misformatted chunk should respond 400",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    chunks: ["5\r\nHello", "6\r\n World\r\n"],
    expected: {
      status: "400",
    },
    executor: chunkExecutor,
  },
  {
    name: "POST chunked request with invalid chunk size (not hex) should respond 400",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    chunks: ["5\r\nHello", "prout\r\n World\r\n"],
    expected: {
      status: "400",
    },
    executor: chunkExecutor,
  },
  {
    name: "POST chunked request with invalid chunk size (too big) should respond 400",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    chunks: ["5\r\nHello", "10\r\n World\r\n"],
    expected: {
      status: "400",
    },
    executor: chunkExecutor,
  },
  {
    name: "POST chunked request with invalid chunk size (too small) should respond 400",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    chunks: ["5\r\nHello", "1\r\n World\r\n"],
    expected: {
      status: "400",
    },
    executor: chunkExecutor,
  },
  {
    name: "POST chunked request with missing terminating chunk should timeout",
    route: "/upload",
    method: "POST",
    headers: {
      Host: HOST,
      "Transfer-Encoding": "chunked",
    },
    version: "HTTP/1.1",
    chunks: ["5\r\nHello", "1\r\n World\r\n"],
    expected: {},
    executor: chunkExecutor,
    isMissingChunkTest: true,
  },
];

function chunkExecutor(testCase) {
  return new Promise((resolve) => {
    const client = new Socket();
    const { method, route, headers, version, chunks, isMissingChunkTest } =
      testCase;

    let head = `${method} ${route} ${version}\r\n`;
    Object.entries(headers).forEach(([key, value]) => {
      head += `${key}: ${value}\r\n`;
    });
    head += "\r\n";

    client.connect(PORT, HOST, () => {
      client.write(head);

      let i = 0;
      function sendNext() {
        if (i < chunks.length) {
          const chunk = chunks[i++];
          client.write(chunk);
          setTimeout(sendNext, 2000);
        } else if (!isMissingChunkTest) {
          client.write("0\r\n\r\n");
        }
      }
      sendNext();
    });

    let response = "";
    client.on("data", (data) => {
      response += data.toString();
    });
    client.on("end", () => {
      const result = parseResponse(response);
      resolve(test(testCase, result));
    });
  });
}

async function run() {
  log(`\n${CYAN}##### - Starting chunked requests suite... - #####${RESET}`);

  let failsCount = 0;
  for (const testCase of testCases) {
    failsCount += (await testCase.executor(testCase)) ? 0 : 1;
  }

  return {
    failed: failsCount,
    passed: testCases.length - failsCount,
    total: testCases.length,
  };
}

export default run;
