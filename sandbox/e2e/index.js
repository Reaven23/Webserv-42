import { CYAN, GREEN, RED, RESET, BOLD } from "./constants.js";
import runTests200 from "./200.js";
import runTests400 from "./400.js";
import runTests404 from "./404.js";

const log = (...args) => console.log("  ", ...args);

async function start() {
  const results200 = await runTests200();
  const results400 = await runTests400();
  const results404 = await runTests404();

  const totals = [results200, results400, results404].reduce(
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
