"""The published manifest must identify immutable, byte-exact firmware files."""
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]


def package(product,size):
    raw=bytearray(size)
    for i in range(20):
        raw[i*48+47]=(ord(product[i])+i+1)&255 if i<len(product) else 125
    return raw


class PrepareSiteTests(unittest.TestCase):
    def test_each_package_has_a_content_address_and_matching_manifest(self):
        with tempfile.TemporaryDirectory(prefix='noct-site-') as directory:
            work=Path(directory);output=work/'site';source=work/'input.fwsc';urls=[]
            for size in (1200,1300,1300):
                raw=package('FM-1_90625000701',size);source.write_bytes(raw)
                subprocess.run([sys.executable,str(ROOT/'tools/prepare-site.py'),
                    '--firmware',str(source),'--output',str(output)],check=True,capture_output=True)
                meta=json.loads((output/'firmware.json').read_text(encoding='utf8'))['builds'][0]
                digest=hashlib.sha256(raw).hexdigest()
                self.assertEqual(meta['bytes'],size)
                self.assertEqual(meta['sha256'],digest)
                self.assertEqual(meta['url'],f'./firmware/noct-usb-{digest}.fwsc')
                self.assertEqual((output/meta['url']).read_bytes(),raw)
                self.assertTrue((output/'releases.js').is_file())
                urls.append(meta['url'])
            self.assertNotEqual(urls[0],urls[1])
            self.assertEqual(urls[1],urls[2])
            self.assertTrue((output/urls[0]).is_file())

    def test_no_firmware_keeps_installation_unavailable(self):
        with tempfile.TemporaryDirectory(prefix='noct-site-empty-') as directory:
            output=Path(directory)/'site'
            subprocess.run([sys.executable,str(ROOT/'tools/prepare-site.py'),
                '--output',str(output)],check=True,capture_output=True)
            manifest=json.loads((output/'firmware.json').read_text(encoding='utf8'))
            self.assertEqual(manifest['builds'],[])


if __name__=='__main__':
    unittest.main(verbosity=2)
