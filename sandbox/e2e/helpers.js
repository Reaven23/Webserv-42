export function buildRawQuery(testCase) {
  const { method, route, headers, version, body } = testCase;

  let query = `${method} ${route} ${version}\r\n`;

  Object.entries(headers).forEach(([header, value]) => {
    query += `${header}: ${value}\r\n`;
  });

  query += "\r\n";
  if (body) query += body;

  return query;
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
