# Static analysis helpers. Reads the unwrapped bak/Cars.exe that ships in the game folder.
# python3 -i tools/re/lib.py, then show(0x5567a0), xrefs(0x779f34), findstr(b'Player')
import bisect,os,pickle,re,capstone
HERE=os.path.dirname(os.path.abspath(__file__))
SRC=os.path.join(HERE,'..','..','src')
GAME=os.environ.get('CARS_DIR',os.path.expanduser('~/.local/share/Steam/steamapps/common/Cars'))
d=open(GAME+'/bak/Cars.exe','rb').read()
BASE=0x400000
secs=[('.text',0x1000,0x30fa00,0x400),('.rdata',0x311000,0x5c200,0x30fe00),('.data',0x36e000,0x1ce00,0x36c000)]
def v2o(va):
    r=va-BASE
    for n,v,s,o in secs:
        if v<=r<v+s: return r-v+o
def o2v(o):
    for n,v,s,p in secs:
        if p<=o<p+s: return o-p+v+BASE
def disasm():
    md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
    md.skipdata=True
    return list(md.disasm_lite(d[0x400:0x400+0x30f8a9],BASE+0x1000))
PKL=os.path.join(HERE,'ins.pkl')
if os.path.exists(PKL): ins=pickle.load(open(PKL,'rb'))
else:
    ins=disasm(); pickle.dump(ins,open(PKL,'wb'))
addrs=[i[0] for i in ins]
def istr(va):
    o=v2o(va)
    if o is None: return None
    return d[o:d.find(b'\0',o)].decode('latin1')
def findstr(b):
    r=[];p=0
    while True:
        p=d.find(b,p)
        if p<0: return r
        r.append(o2v(p));p+=1
def xrefs(va):
    h='0x%x'%va
    return [i for i in ins if re.search(re.escape(h)+r'\b',i[3])]
def show(a,before=0,after=30):
    k=bisect.bisect_left(addrs,a)
    for i in ins[max(0,k-before):k+after]:
        c=''
        for m in re.findall(r'0x[0-9a-f]{6,}',i[3]):
            v=int(m,16)
            if 0x711000<=v<0x76e000+0x1ce00:
                s=istr(v)
                if s and len(s)>2 and s.isprintable(): c=' ; "%s"'%s[:60]
        print('%08x %s %s%s'%(i[0],i[2],i[3],c))
