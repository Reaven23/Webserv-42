import { CYAN, GREEN, RED, RESET, BOLD } from "./constants.js";
import runTests200 from "./200.js";
import runTests400 from "./400.js";
import runTests404 from "./404.js";
import runTestsKeepAlive from "./keep-alive.js";
import runTestsTimeout from "./timeout.js";

const log = (...args) => console.log("  ", ...args);
const all = process.argv.includes("--all");

async function start() {
  const results = [
    await runTests200(),
    await runTests400(),
    await runTests404(),
  ];

  if (all) {
    results.push(await runTestsKeepAlive());
    results.push(await runTestsTimeout());
  }

  const totals = results.reduce(
    (acc, curr) => ({
      failed: acc.failed + curr.failed,
      passed: acc.passed + curr.passed,
      total: acc.total + curr.total,
    }),
    {
      failed: 0,
      passed: 0,
      total: 0,
    },
  );

  log(`\n${CYAN}##### - Tests results - #####${RESET}`);
  if (totals.failed) {
    log(
      `${BOLD}${RED}\u274C Tests failed: ${totals.failed}/${totals.total} ${RESET}`,
    );
  }
  log(
    `${BOLD}${GREEN}\u2705 Tests passed: ${totals.passed}/${totals.total} ${RESET}`,
  );
}

start().catch((e) => console.error(e));
