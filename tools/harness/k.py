# usage: k.py shot Return w3 Up:0.5 ...   Sends keys by XTEST, w3 waits 3s, then saves out/shot.png.
# Needs python-xlib, and tools/harness/run.sh running.
import os,subprocess,sys,time
from Xlib import X,display,XK
from Xlib.ext import xtest
OUT=os.path.join(os.path.dirname(os.path.abspath(__file__)),'out')
d=display.Display(open(OUT+'/disp').read().strip())
def key(n,hold=.12):
    kc=d.keysym_to_keycode(XK.string_to_keysym(n))
    xtest.fake_input(d,X.KeyPress,kc); d.sync(); time.sleep(hold); xtest.fake_input(d,X.KeyRelease,kc); d.sync(); time.sleep(.5)
for a in sys.argv[2:]:
    if a[0]=='w' and a[1:].replace('.','').isdigit(): time.sleep(float(a[1:]))
    else:
        p=a.split(':'); key(p[0],float(p[1]) if len(p)>1 else .12)
env=dict(os.environ,GAMESCOPE_WAYLAND_DISPLAY='gamescope-0')
out=f'{OUT}/{sys.argv[1]}.png'
subprocess.run(['gamescopectl','screenshot',out],env=env,timeout=10)
time.sleep(1.2)
print(out if os.path.exists(out) else 'no screenshot')
