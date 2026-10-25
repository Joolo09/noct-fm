"""Source distribution with pinned, vendored dependencies and their licences."""
from pathlib import Path
import hashlib
import zipfile

root = Path(__file__).resolve().parents[1]
dist = root/'dist'
dist.mkdir(exist_ok=True)
target = dist/'NOCT-FM-alpha-source.zip'
with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED) as archive:
    for directory in ('firmware', 'integrations', 'tests', 'upstream'):
        for p in sorted((root/directory).rglob('*')):
            if not p.is_file(): continue
            rel = p.relative_to(root)
            if any(part in ('.git', 'build', '__pycache__') for part in rel.parts): continue
            archive.write(p, Path('NOCT-FM')/rel)
    for name in ('README.md', 'build.sh', '.gitignore'):
        archive.write(root/name, Path('NOCT-FM')/name)
    for name in ('build-host.ps1', 'cc.cmd', 'make-preview.py', 'package-source.py'):
        archive.write(root/'tools'/name, Path('NOCT-FM/tools')/name)
    for name in ('noct-fm.gif', 'fl-studio.png', 'ableton-live.png'):
        archive.write(root/'preview'/name, Path('NOCT-FM/preview')/name)
digest = hashlib.sha256(target.read_bytes()).hexdigest()
(dist/'SHA256SUMS.txt').write_text(f'{digest}  {target.name}\n')
print(f'{target}: {target.stat().st_size} bytes\nSHA256 {digest}')
