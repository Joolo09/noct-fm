"""Capture an animated preview from NOCT's C renderer, never a UI approximation."""
from pathlib import Path
import subprocess
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
out = root/'preview'
out.mkdir(exist_ok=True)
lines = ['fixed', 'wait 800']
for daw in range(2):
    if daw:
        lines += ['spin PRESET 1']
    for i in range(38):
        if i == 10: lines += ['key 7 down']
        if 12 <= i < 23: lines += ['spin K1 3', 'spin K2 -2', 'spin K3 1']
        if i == 25: lines += ['key 7 up']
        lines += ['wait 80', f'shot anim-{daw}-{i:02}.png']
script = out/'animation.ctl'
script.write_text('\n'.join(lines)+'\n')
exe = root/'firmware/app/build/host/noct_host.exe'
if not exe.exists(): exe = exe.with_suffix('')
subprocess.run([str(exe), str(script), str(out)], check=True)
frames = []
for daw in range(2):
    for i in range(38):
        screen = Image.open(out/f'anim-{daw}-{i:02}.png').convert('RGB')
        canvas = Image.new('RGB', (600, 644), '#090c19')
        draw = ImageDraw.Draw(canvas)
        draw.rounded_rectangle((48, 48, 552, 552), radius=16, fill='#141b2c', outline='#30394e', width=2)
        canvas.paste(screen.resize((480, 480), Image.Resampling.NEAREST), (60, 60))
        font = ImageFont.truetype(str(root/'upstream/base/assets/fonts/BarlowSemiCondensed-Medium.ttf'), 21)
        draw.text((60, 579), 'NOCT FM / AFTER HOURS, IN CONTROL', font=font, fill='#c1cbe0')
        draw.text((60, 608), 'C renderer capture / 240 x 240', font=font, fill='#7c8ba7')
        frames.append(canvas)
frames[0].save(out/'noct-fm.gif', save_all=True, append_images=frames[1:], duration=80, loop=0, optimize=False)
print(out/'noct-fm.gif')
