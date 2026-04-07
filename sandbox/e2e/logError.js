import { BOLD, RED, RESET } from "./constants.js";

const error = (...args) => console.error("  ", ...args);

export function logError({
  name,
  input,
  errorProperty,
  errorValue,
  expectedValue,
}) {
  error(`${BOLD}${RED}[FAIL] \u274C - ${name}${RESET}`);
  error(`${RED}Expected ${errorProperty}: '${expectedValue}'${RESET}`);
  error(`${RED}Received ${errorProperty}: '${errorValue}'${RESET}`);
  error(`${RED}Test input: ${JSON.stringify(input, null, 2)}${RESET}\n`);
}
