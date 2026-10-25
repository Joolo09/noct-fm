#!/usr/bin/env python3
# SPDX-License-Identifier: MPL-2.0
"""Package the static installer; publish a firmware only after a successful build."""
import argparse
import hashlib
import json
import shutil
import zipfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--firmware', type=Path)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
if args.output.resolve() == (root/'site').resolve():
    parser.error('Output must differ from site source')
shutil.copytree(root/'site', args.output, dirs_exist_ok=True)
shutil.copyfile(root/'LICENSE',args.output/'LICENSE')
downloads=args.output/'downloads'
downloads.mkdir(exist_ok=True)
with zipfile.ZipFile(downloads/'NOCT-DAW-scripts.zip','w',zipfile.ZIP_DEFLATED) as archive:
    for file in sorted((root/'integrations').rglob('*.py')):
        if '__pycache__' not in file.parts:
            archive.write(file,file.relative_to(root/'integrations'))
    archive.write(root/'README.md','README.md')
if args.firmware:
    data = args.firmware.read_bytes()
    if len(data) < 960 or len(data) > 2*1024*1024:
        raise SystemExit('Invalid package size')
    product = ''.join(chr((data[i*48+47]-i-1)&255) for i in range(20) if data[i*48+47]!=125)
    if not product.startswith('FM-1_90625'):
        raise SystemExit('Not a NOCT package')
    directory=args.output/'firmware'
    directory.mkdir(exist_ok=True)
    filename='noct-usb-alpha.fwsc'
    (directory/filename).write_bytes(data)
    version={'FM-1_90625000300':'NOCT MIDI 0.3.0 / USB','FM-1_90625000200':'NOCT MIDI 0.2.0 / USB'}.get(product,'NOCT FM / USB alpha')
    manifest={'schema':1,'builds':[{'version':version,'product':product,
        'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'url':f'./firmware/{filename}'}]}
    (args.output/'firmware.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'Installer ready: {args.output} / firmware: {bool(args.firmware)}')
