# usage: vpad.py N   Creates N uinput pads. Needs python-evdev and write access to /dev/uinput.
# Start it before the game. Then write lines to out/vpad.fifo:
#   "0 0x130"          tap BTN_SOUTH on pad 0
#   "0 0x130 2"        hold it for 2s
#   "axis 0 0x00 -32768"  set ABS_X on pad 0
import os,sys,time
from evdev import UInput,ecodes as e,AbsInfo
OUT=os.path.join(os.path.dirname(os.path.abspath(__file__)),'out')
os.makedirs(OUT,exist_ok=True)
N=int(sys.argv[1])
cap={e.EV_KEY:[e.BTN_SOUTH,e.BTN_EAST,e.BTN_NORTH,e.BTN_WEST,e.BTN_TL,e.BTN_TR,e.BTN_SELECT,e.BTN_START],
     e.EV_ABS:[(c,AbsInfo(0,-32768,32767,16,128,0)) for c in (e.ABS_X,e.ABS_Y,e.ABS_RX,e.ABS_RY)]}
pads=[UInput(cap,name=f'CarsMP Test Pad {i}',vendor=0x1234,product=0x5678+i,version=1,bustype=e.BUS_USB) for i in range(N)]
ff=OUT+'/vpad.fifo'
if not os.path.exists(ff): os.mkfifo(ff)
while True:
    for l in open(ff):
        a=l.split()
        if a[0]=='quit': sys.exit()
        if a[0]=='axis':
            p=pads[int(a[1])]; p.write(e.EV_ABS,int(a[2],0),int(a[3])); p.syn(); continue
        p=pads[int(a[0])]; c=int(a[1],0); h=float(a[2]) if len(a)>2 else .15
        p.write(e.EV_KEY,c,1); p.syn(); time.sleep(h); p.write(e.EV_KEY,c,0); p.syn()
