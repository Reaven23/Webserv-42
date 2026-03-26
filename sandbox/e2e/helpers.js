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
  const separatorIndex = response.indexOf("\r\n\r\n");
  const headPart = response.substring(0, separatorIndex);
  const body = response.substring(separatorIndex + 4);

  const headlines = headPart.split("\r\n");
  const result = {
    ...parseResponseFirstLine(headlines[0]),
    headers: {},
    body,
  };

  for (let i = 1; i < headlines.length; i++) {
    Object.assign(result.headers, parseHeader(headlines[i]));
  }

  return result;
}
