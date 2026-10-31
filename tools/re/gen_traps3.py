# Writes src/traps3.h: mov r32,[x+y] in functions that first add 0x30 to an index.
# Seeds are functions in 0x458000-0x475000 that use the constant 0x30, plus two found by hand.
from lib import *
def fn(a):
    k=bisect.bisect_left(addrs,a);j=k
    while not (ins[j][2]=='int3' and ins[j+1][2]!='int3'): j-=1
    e=k
    while not (ins[e][2]=='int3' and ins[e+1][2]=='int3'): e+=1
    return j+1,e
seeds=[i[0] for i in ins if 0x458000<=i[0]<0x475000 and (
   (i[2]=='mov' and re.search(r', 0x30$',i[3])) or (i[2]=='add' and re.search(r'^e(ax|bx|cx|dx|si|di|bp), 0x30$',i[3])))]+[0x4c8507,0x4ca91a]
fns={}
for a in seeds:
    s,e=fn(a); fns[s]=e
out=set()
for s,e in fns.items():
    for i in ins[s:e]:
        b=d[v2o(i[0]):v2o(i[0])+i[1]]
        if i[1]==3 and b[0]==0x8b and b[1]&0xc7==0x04 and (b[2]>>6)==0 and (b[2]&7)!=5 and ((b[2]>>3)&7)!=4 and (b[2]&7)!=4:
            out.add(i[0])
out=sorted(out)
open(SRC+'/traps3.h','w').write('static const DWORD traps3[]={'+','.join(hex(a) for a in out)+'};\n')
print(len(out))
