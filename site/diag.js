// SPDX-License-Identifier: MPL-2.0
// Read-only diagnostics: fm1-base answers F0 7D 4C 47 F7 ("LG") with one SysEx frame per log line
// (F0 7D 4C 47 <ASCII> F7): the last crash, then the Bluetooth log, the oldest first. Nothing is written.
export const LOG_REQUEST = [0xF0, 0x7D, 0x4C, 0x47, 0xF7];

export function logLine(data) {
  if (!data || data.length < 5 || data[0] !== 0xF0 || data[1] !== 0x7D || data[2] !== 0x4C || data[3] !== 0x47 ||
      data[data.length - 1] !== 0xF7) return null;
  let text = '';
  for (let i = 4; i < data.length - 1; i++) text += String.fromCharCode(data[i] & 0x7F);
  return text;
}

// The HCI reason of the newest "disconnected" line, e.g. 0x3d from "computer: disconnected 3d x4".
export function lastDisconnect(lines) {
  for (let i = lines.length - 1; i >= 0; i--) {
    const m = /disconnected\b(.*)$/.exec(lines[i]);
    if (!m) continue;
    const word = m[1].split(/[\s:]+/).find(w => /^[0-9a-f]{1,4}$/i.test(w));
    return word ? parseInt(word, 16) : -1;
  }
  return -1;
}

export function readDeviceLog(input, output, ms = 1500) {
  return new Promise(resolve => {
    const lines = [];
    const take = event => { const line = logLine(event.data); if (line !== null) lines.push(line); };
    input.addEventListener('midimessage', take);
    output.send(LOG_REQUEST);
    setTimeout(() => { input.removeEventListener('midimessage', take); resolve(lines); }, ms);
  });
}
