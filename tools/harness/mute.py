# Keeps the game muted during test runs.
import subprocess,json,time
while True:
    try:
        for s in json.loads(subprocess.run(['pactl','-f','json','list','sink-inputs'],capture_output=True,text=True,timeout=5).stdout or '[]'):
            p=s.get('properties',{})
            t=' '.join(str(p.get(k,'')) for k in('application.process.binary','application.name','node.name'))
            if 'Cars' in t and not s.get('mute'):
                subprocess.run(['pactl','set-sink-input-mute',str(s['index']),'1'],timeout=5)
    except Exception: pass
    time.sleep(0.5)
