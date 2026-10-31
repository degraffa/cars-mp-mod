# Writes src/clamp.h: every menu/HUD read of the player count byte, cfg+0x24.
# Sites before 0x4c0000 are added by hand; see EXTRA.
from lib import *
EXTRA=[0x4a7cea]
sites=[]
for k,i in enumerate(ins):
    if '+ 0x468]' in i[3] and i[2]=='mov':
        reg=i[3].split(',')[0].strip()
        for j in ins[k+1:k+9]:
            if re.search(r'byte ptr \[%s \+ 0x24\]'%reg,j[3]): sites.append(j)
            if re.search(r'^%s,'%reg,j[3]) and j[2] in('mov','lea'): break
out=list(EXTRA)
for s in sorted(set(sites)):
    a=s[0]
    if not 0x4c0000<=a<0x540000: continue
    if s[3].startswith('byte ptr') and s[2]=='mov': continue   # writers stay
    b=d[v2o(a):v2o(a)+s[1]]
    pos=3 if s[2]=='movzx' else 2
    assert b[pos]==0x24,(hex(a),b.hex(),s[2],s[3])
    out.append(a+pos)
open(SRC+'/clamp.h','w').write('static const Clamp clamps[]={\n'+''.join(f' {{0x{a:x}}},\n' for a in sorted(out))+'};\n')
print(len(out))
