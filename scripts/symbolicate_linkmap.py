#!/usr/bin/env python3
"""Resolve one image's .ips frames with a matching binary and same-build link map.
Link maps give function ranges, not dSYM source lines or a timeline/profile.
Usage: python3 scripts/symbolicate_linkmap.py crash.ips binary.dylib arch.linkmap
"""
from pathlib import Path
import argparse, json, re, struct, sys

def slices(data):
    magic=struct.unpack_from('>I',data)[0]
    if magic in (0xcafebabe,0xcafebabf):
        count=struct.unpack_from('>I',data,4)[0]
        if count>32: raise ValueError('implausible Mach-O slice count')
        stride=32 if magic==0xcafebabf else 20
        for i in range(count):
            pos=8+i*stride
            cpu,sub=struct.unpack_from('>II',data,pos)
            off,size=struct.unpack_from('>QQ' if stride==32 else '>II',data,pos+8)
            if off+size>len(data): raise ValueError('truncated Mach-O slice')
            yield cpu,sub,data[off:off+size]
    else:
        cpu,sub=struct.unpack_from('<II',data,4)
        yield cpu,sub,data

def identities(data):
    import uuid
    for cpu,sub,chunk in slices(data):
        if struct.unpack_from('<I',chunk)[0]!=0xfeedfacf: continue
        ncmds=struct.unpack_from('<I',chunk,16)[0]; pos=32; uid=None; base=0
        for _ in range(ncmds):
            cmd,size=struct.unpack_from('<II',chunk,pos)
            if size<8 or pos+size>len(chunk): raise ValueError('invalid load command')
            if cmd==0x1b: uid=str(uuid.UUID(bytes=chunk[pos+8:pos+24]))
            if cmd==0x19 and chunk[pos+8:pos+24].rstrip(b'\0')==b'__TEXT':
                base=struct.unpack_from('<Q',chunk,pos+24)[0]
            pos+=size
        arch='arm64e' if cpu==0x100000c and (sub&0xffffff)==2 else 'arm64' if cpu==0x100000c else str(cpu)
        yield {'uuid':uid,'arch':arch,'text_vmaddr':base}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('ips',type=Path);ap.add_argument('binary',type=Path);ap.add_argument('linkmap',type=Path)
    args=ap.parse_args(); raw=args.ips.read_text()
    decoder=json.JSONDecoder(); first,end=decoder.raw_decode(raw)
    rest=raw[end:].strip(); report=json.loads(rest) if rest else first
    images=report['usedImages']; name=args.binary.name
    image_index=next(i for i,v in enumerate(images) if v.get('name')==name)
    image=images[image_index]; uid=image['uuid'].lower()
    match=next((v for v in identities(args.binary.read_bytes()) if v['uuid']==uid),None)
    if match is None: raise ValueError('binary UUID does not match the crash image')
    text=args.linkmap.read_text(errors='replace')
    if '# Arch: '+match['arch'] not in text: raise ValueError('wrong architecture link map')
    live=text.split('# Symbols:',1)[1].split('# Dead Stripped Symbols:',1)[0]
    symbols=[]
    for line in live.splitlines():
        m=re.match(r'^(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+\[\s*(\d+)\]\s+(.*)',line)
        if m: symbols.append((int(m[1],16),int(m[2],16),int(m[3]),m[4]))
    print('Matched binary UUID:',uid,'architecture:',match['arch'])
    print('Use a link map from THIS build; a link map has no independent UUID.')
    for tid,thread in enumerate(report['threads']):
        if not any(f.get('imageIndex')==image_index for f in thread.get('frames',[])): continue
        print('\nThread',tid,thread.get('queue',thread.get('name','')))
        for fi,frame in enumerate(thread['frames']):
            idx=frame.get('imageIndex');off=frame.get('imageOffset',0)
            im=images[idx] if isinstance(idx,int) and idx<len(images) else {}
            label=frame.get('symbol','<no symbol>')
            if idx==image_index:
                addr=off+match['text_vmaddr']
                found=[s for s in symbols if s[0]<=addr<s[0]+s[1]]
                label=' | '.join(f'{s[3]} + {addr-s[0]} [object {s[2]}]' for s in found) or '<unmapped>'
            print(f'{fi:3} {im.get("name","?")} +0x{off:x}: {label}')

if __name__=='__main__':
    try: main()
    except (OSError,ValueError,KeyError,StopIteration,struct.error,IndexError) as exc:
        print('Symbolication failed:',exc,file=sys.stderr);sys.exit(1)
