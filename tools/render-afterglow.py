# SPDX-License-Identifier: MPL-2.0
"""Capture actual C frames: six scenes, contact sheet and animated GIFs."""
from pathlib import Path
import os
import subprocess
from PIL import Image, ImageDraw, ImageFont

root=Path(__file__).resolve().parents[1]
output=root/'test-output/afterglow'
output.mkdir(parents=True,exist_ok=True)
host=root/('firmware/app/build/host/noct-usb_host.exe' if os.name=='nt' else 'firmware/app/build/host/noct-usb_host')
commands=['wait 1400','splash','shot splash.png','wait 300']
frames=[]
def capture(page,extra=None):
    for i in range(24):
        if extra and i in extra:commands.extend(extra[i])
        name=f'{page}-{i:02}.png';commands.extend(['wait 100',f'shot {name}']);frames.append(name)
capture('daw',{8:['key 7 down','key 11 down','key 14 down'],16:['key 7 up','key 11 up','key 14 up']})
commands.extend(['tap SEL','key 7 down','wait 900'])
capture('chords',{12:['spin K1 1']})
commands.extend(['key 7 up','tap SEL','tap SEQ','spin K1 2','wait 2300'])
capture('compose',{18:['tap EDIT']})
commands.extend(['spin PRESET 1'])
capture('rhythm',{10:['spin K1 2']})
commands.extend(['tap SEQ','tap ARP','key 7 down','key 11 down','key 14 down'])
capture('arp',{12:['spin K2 2']})
commands.extend(['key 7 up','key 11 up','key 14 up','tap ARP','tap GLO','simclock 1','midi USB FA 00 00'])
capture('sync',{10:['simclock 0','wait 650'],18:['simclock 1']})
commands.extend(['midi USB FC 00 00','simclock 0','expect active 0'])
script=output/'animation.ctl';script.write_text('\n'.join(commands)+'\n',encoding='utf8')
subprocess.run([str(host),str(script),str(output)],check=True)
images=[Image.open(output/name).convert('RGB') for name in frames]
preview=root/'preview';preview.mkdir(exist_ok=True)
images[0].save(preview/'noct-fm.gif',save_all=True,append_images=images[1:],duration=100,loop=0,disposal=2)
large=[im.resize((720,720),Image.Resampling.NEAREST) for im in images]
large[0].save(preview/'afterglow.gif',save_all=True,append_images=large[1:],duration=100,loop=0,disposal=2)
sheet=Image.new('RGB',(816,654),(9,11,23));draw=ImageDraw.Draw(sheet)
font=ImageFont.load_default(size=16)
draw.text((24,18),'NOCT 0.6 / AFTERGLOW / REAL C FRAMES',font=font,fill=(190,160,255))
for i,page in enumerate(['daw','chords','compose','rhythm','arp','sync']):
    x=24+(i%3)*264;y=68+(i//3)*288
    sheet.paste(Image.open(output/f'{page}-08.png').convert('RGB'),(x,y))
    draw.text((x,y+247),page.upper(),font=font,fill=(183,190,219))
sheet.save(preview/'afterglow-scenes.png')
(root/'site/noct-fm.gif').write_bytes((preview/'noct-fm.gif').read_bytes())
Image.open(output/'daw-08.png').save(root/'site/still.png')
print('Captured',len(images),'native C frames; animation:',(preview/'afterglow.gif').stat().st_size,'bytes')
