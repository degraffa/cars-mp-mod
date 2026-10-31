# Writes src/traps.h: mov r32,[base+idx*scale+0x30/0x3c], the camera and view array reads.
from lib import *
out=[]
for i in ins:
    b=d[v2o(i[0]):v2o(i[0])+i[1]]
    if 0x430000<=i[0]<0x560000 and i[1]==4 and b[0]==0x8b and b[1]&0xc7==0x44 and b[3] in(0x30,0x3c) and (b[2]>>6) in (0,2):
        if (b[2]>>3)&7==4 or b[2]&7==4: continue
        out.append(i[0])
open(SRC+'/traps.h','w').write('static const DWORD traps[]={'+','.join(hex(a) for a in out)+'};\n')
print(len(out))
