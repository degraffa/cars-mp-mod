# Reads live game memory through /proc while it runs. Add what you need to inspect.
import struct,subprocess
pid=int(subprocess.check_output(['pgrep','-x','Cars.exe']).split()[0])
f=open(f'/proc/{pid}/mem','rb',buffering=0)
def rd(a,n=4): f.seek(a); return f.read(n)
def u32(a): return struct.unpack('<I',rd(a))[0]
g=u32(0x779f34); c=u32(g+0x468)
print('cfg',hex(c),'players',rd(c+0x24,1)[0],'mode',u32(c+0x10))
