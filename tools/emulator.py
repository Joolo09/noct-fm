# SPDX-License-Identifier: MPL-2.0
"""NOCT interactive emulator. Runs the actual compiled C app, never real MIDI.
Start: python tools/emulator.py   (build the host first)
"""
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import argparse
import collections
import json
import queue
import re
import subprocess
import tempfile
import threading
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=8765)
args = parser.parse_args()
HOST = ROOT / 'firmware/app/build/host/noct-usb_host.exe'
if not HOST.exists():
    HOST = HOST.with_suffix('')
if not HOST.exists():
    raise SystemExit('Build first: sh build.sh --host --no-ble')
BUTTONS = {'HOME','SEL','ARP','SEQ','EDIT','GLO','SAVE','FX','ENV','LFO','PLAY','REC','OCT-','OCT+'}
ENCODERS = {'SELECT','ALGO','PRESET','K1','K2','K3','K4'}
pending = queue.Queue()
lock = threading.Lock()
snapshot = {'notes': [], 'events': [], 'ready': False}
screen = b''
events = collections.deque(maxlen=120)
directory = tempfile.TemporaryDirectory(prefix='noct-emulator-')
work = Path(directory.name)
process = subprocess.Popen([str(HOST), '-', str(work)], stdin=subprocess.PIPE,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                           text=True, bufsize=1)

def exchange(commands):
    global snapshot, screen
    process.stdin.write('\n'.join(commands + ['snapshot', 'out', 'shot screen.png', 'echo __READY__']) + '\n')
    process.stdin.flush()
    state = None
    while True:
        line = process.stdout.readline()
        if not line:
            raise RuntimeError('C emulator exited')
        if line.strip() == '__READY__':
            break
        if line.startswith('@STATE '):
            state = json.loads(line[7:])
        match = re.search(r'out ([0-9A-F]{2}) ([0-9A-F]{2}) ([0-9A-F]{2}) -> USB', line)
        if match:
            events.append({'id': time.monotonic_ns(), 'data': [int(v,16) for v in match.groups()]})
    with lock:
        snapshot = {**state, 'events': list(events), 'ready': True}
        screen = (work/'screen.png').read_bytes()

def worker():
    try:
        exchange(['wait 800'])
        previous = time.monotonic()
        while process.poll() is None:
            time.sleep(.06)
            now = time.monotonic()
            commands = ['wait ' + str(min(200, max(1, int((now-previous)*1000))))]
            previous = now
            while True:
                try:
                    commands += pending.get_nowait()
                except queue.Empty:
                    break
            exchange(commands)
    except Exception as exc:
        with lock:
            snapshot.update(error=str(exc), ready=False)

def action(body):
    op = body.get('op')
    name = body.get('name')
    if op in ('tap','press','release') and name in BUTTONS:
        return [f'{op} {name}']
    if op == 'spin' and name in ENCODERS:
        return [f'spin {name} {max(-128,min(128,int(body.get("value",1))))}']
    if op == 'key':
        key = int(body.get('value',-1))
        if 0 <= key < 27 and body.get('down') in (True,False):
            return [f'key {key} {"down" if body["down"] else "up"}']
    if op == 'daw':
        return ['simclock 1','midi USB FA 00 00'] if body.get('value') else ['midi USB FC 00 00','simclock 0']
    if op == 'clock':
        return ['simclock 1' if body.get('value') else 'simclock 0']
    if op == 'tempo':
        tempo=max(200,min(9990,int(float(body.get('value',120))*10)))
        return [f'midi USB BF 72 {tempo>>7:02X}', f'midi USB BF 73 {tempo&127:02X}']
    if op == 'reset':
        return ['reboot','wait 800']
    raise ValueError('Unsupported emulator control')

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass
    def respond(self, data, kind, code=200):
        self.send_response(code)
        self.send_header('Content-Type',kind)
        self.send_header('Cache-Control','no-store')
        self.send_header('Content-Length',str(len(data)))
        self.end_headers()
        self.wfile.write(data)
    def do_GET(self):
        path=self.path.split('?')[0]
        if path == '/state':
            with lock:
                data=json.dumps(snapshot).encode()
            return self.respond(data,'application/json')
        if path == '/screen.png':
            with lock:
                data=screen
            return self.respond(data,'image/png')
        if path == '/':
            return self.respond((ROOT/'emulator/index.html').read_bytes(),'text/html; charset=utf-8')
        self.respond(b'Not found','text/plain',404)
    def do_POST(self):
        if self.path != '/action' or int(self.headers.get('Content-Length','0')) > 2048:
            return self.respond(b'Invalid request','text/plain',400)
        # Only same-origin local UI can operate this simulated device.
        origin=self.headers.get('Origin','')
        if origin and origin != f'http://127.0.0.1:{args.port}':
            return self.respond(b'Forbidden origin','text/plain',403)
        try:
            body=json.loads(self.rfile.read(int(self.headers.get('Content-Length','0'))))
            pending.put(action(body))
            self.respond(b'{"ok":true}','application/json')
        except (ValueError,TypeError):
            self.respond(b'Invalid control','text/plain',400)

threading.Thread(target=worker,daemon=True).start()
print(f'NOCT C emulator: http://127.0.0.1:{args.port}',flush=True)
try:
    ThreadingHTTPServer(('127.0.0.1',args.port),Handler).serve_forever()
except KeyboardInterrupt:
    pass
finally:
    process.terminate()
    process.wait(timeout=5)
    directory.cleanup()
