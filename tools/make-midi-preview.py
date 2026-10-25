"""Capture NOCT 0.2's actual MIDI UI and arp modifier in the host simulator."""
import os
import subprocess
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
out=root/'preview';out.mkdir(exist_ok=True)
exe=root/'firmware/app/build/host'/('noct-usb_host.exe' if os.name=='nt' else 'noct-usb_host')
commands=['wait 800']
for i in range(100):
    if i==10:commands+=['tap SEL','key 7 down']
    if 12<=i<24:commands+=['spin K1 2','spin K2 -1','spin K3 1']
    if i==26:commands+=['key 7 up','tap SEL','tap SEQ']
    if i==45:commands+=['tap EDIT']
    if i==50:commands+=['press SEQ','spin K2 4','spin K3 1']
    if i==60:commands+=['release SEQ']
    if i==65:commands+=['tap PLAY','tap SEQ']
    if i==70:commands+=['tap ARP','key 0 down','key 4 down','key 7 down']
    if i==85:commands+=['key 0 up','key 4 up','key 7 up','press ARP','spin K2 1']
    if i==95:commands+=['release ARP']
    commands+=['wait 80',f'shot anim-midi-{i:02}.png']
script=out/'animation-midi.ctl';script.write_text('\n'.join(commands)+'\n')
subprocess.run([str(exe),str(script),str(out)],check=True)
frames=[]
font=ImageFont.truetype(str(root/'upstream/base/assets/fonts/BarlowSemiCondensed-Medium.ttf'),21)
for i in range(100):
    canvas=Image.new('RGB',(600,644),'#090c19');draw=ImageDraw.Draw(canvas)
    draw.rounded_rectangle((48,48,552,552),radius=16,fill='#141b2c',outline='#30394e',width=2)
    screen=Image.open(out/f'anim-midi-{i:02}.png').convert('RGB')
    canvas.paste(screen.resize((480,480),Image.Resampling.NEAREST),(60,60))
    draw.text((60,579),'NOCT MIDI 0.3 / ONE FIXED PRESET',font=font,fill='#c1cbe0')
    draw.text((60,608),'C renderer / chords + generator + arp',font=font,fill='#7c8ba7')
    frames.append(canvas)
frames[0].save(out/'noct-fm.gif',save_all=True,append_images=frames[1:],duration=80,loop=0,optimize=False)
frames[0].save(root/'site/still.png')
(root/'site/noct-fm.gif').write_bytes((out/'noct-fm.gif').read_bytes())
