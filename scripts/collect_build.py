"""Export the exact dylib in the package, without installation or unsafe extraction."""
from pathlib import Path
import io, shutil, subprocess, tarfile, hashlib, json, datetime
root=Path(__file__).resolve().parent.parent
fields=dict(line.split(': ',1) for line in (root/'control').read_text().splitlines() if ': ' in line)
candidates=[]
for path in (root/'packages').glob('*.deb'):
    package=subprocess.check_output(['dpkg-deb','-f',str(path),'Package'],text=True).strip()
    version=subprocess.check_output(['dpkg-deb','-f',str(path),'Version'],text=True).strip()
    if package==fields['Package'] and version==fields['Version']: candidates.append(path)
if not candidates: raise SystemExit('没有找到本次版本对应的 deb 包')
package=max(candidates,key=lambda p:p.stat().st_mtime_ns)
archive=subprocess.check_output(['dpkg-deb','--fsys-tarfile',str(package)])
with tarfile.open(fileobj=io.BytesIO(archive),mode='r:') as tar:
    members=[m for m in tar.getmembers() if m.isfile() and m.name.endswith('/AwemeLiquidBar.dylib')]
    if len(members)!=1: raise SystemExit('deb 中没有唯一的 AwemeLiquidBar.dylib')
    binary=tar.extractfile(members[0]).read()
out=root/'out'; out.mkdir(exist_ok=True)
(out/'AwemeLiquidBar.dylib').write_bytes(binary)
shutil.copy2(package,out/package.name)
print(f'已生成：{out / "AwemeLiquidBar.dylib"}')
print(f'已生成：{out / package.name}')

# TEST18: pair the exact packaged binary with the source/build identity and
# architecture-specific link maps. No crash logs or personal data are uploaded.
def capture(command):
    try:
        result = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, check=False)
        return result.stdout.strip() if result.returncode == 0 else None
    except OSError:
        return None

uuid_report = capture(['xcrun', 'dwarfdump', '--uuid', str(out/'AwemeLiquidBar.dylib')])
if uuid_report:
    (out/'AwemeLiquidBar_UUID.txt').write_text(uuid_report+'\n')
maps=[]
for index, linkmap in enumerate(sorted((root/'.theos'/'obj').rglob('*.linkmap'))):
    destination = out/f'{index:02d}_{linkmap.name}'
    shutil.copy2(linkmap,destination)
    maps.append(destination.name)
source_files=[p for p in root.rglob('*') if p.is_file() and
              p.suffix in {'.x','.xm','.m','.h','.swift'} and
              not any(part.startswith('.') or part=='docs' for part in p.relative_to(root).parts)]
manifest = {
    'package':fields['Package'], 'version':fields['Version'],
    'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'git_commit':capture(['git','rev-parse','HEAD']),
    'packaged_dylib_sha256':hashlib.sha256(binary).hexdigest(),
    'packaged_dylib_uuid_report':uuid_report,
    'link_maps':maps,
    'source_sha256':{str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in sorted(source_files)},
    'note':'Use the packaged binary UUID, not the source ZIP name, to match an .ips file.'
}
(out/'build_identity.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
if not uuid_report or not maps:
    print('注意：UUID 或 link map 未导出完整，不能进行完整地址映射。')
else:
    print(f'已导出 UUID 和 {len(maps)} 份 link map。')
