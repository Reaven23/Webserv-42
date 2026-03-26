export function buildRawQuery(testCase) {
  const { method, route, headers, version } = testCase;
  const host = Object.keys(headers).find((header) => header === "Host");

  return `${method} ${route} ${version}\r\n${host}: ${headers.Host}\r\n\r\n`;
}

function parseResponseFirstLine(line) {
  const split = line.split(" ");
  const [version, statusCode, ...reason] = split;

  return {
    version,
    statusCode,
    reason: reason.join(" "),
  };
}

function parseHeader(line) {
  const [headerName, headerValue] = line.split(": ");

  return {
    [headerName]: headerValue,
  };
}

export function parseResponse(response) {
  const lines = response.split("\r\n").filter((line) => line);

  return lines.reduce(
    (acc, curr, index) => {
      if (index === 0) return { ...acc, ...parseResponseFirstLine(curr) };

      if (curr.includes(":"))
        return { ...acc, headers: { ...acc.headers, ...parseHeader(curr) } };

      return { ...acc, body: acc.body + curr };
    },
    { body: "", headers: {} },
  );
}
