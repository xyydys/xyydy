#!/usr/bin/env python3
"""AwemeLiquidBar 3.0 static preflight (no SDK needed).

1. Every source the Makefile compiles exists.
2. Logos %hook / %group blocks are balanced in every .x / .xm source.
3. Every C function the Swift bridging header imports is defined in an
   Objective-C / Logos source.
4. The Objective-C declarations of the Swift classes match the selectors
   the Swift code exports (a mismatch compiles and then crashes at runtime).
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
errors = []


def read(name):
    with open(os.path.join(ROOT, name), encoding='utf-8') as handle:
        return handle.read()


makefile = read('Makefile')
files = re.search(r'^AwemeLiquidBar_FILES\s*=\s*(.+)$', makefile, re.M).group(1).split()
for name in files:
    if not os.path.isfile(os.path.join(ROOT, name)):
        errors.append('Makefile lists a missing source: %s' % name)

for name in files:
    if not name.endswith(('.x', '.xm')):
        continue
    text = read(name)
    opened = len(re.findall(r'^\s*%(hook|group|subclass)\b', text, re.M))
    closed = len(re.findall(r'^\s*%end\b', text, re.M))
    if opened != closed:
        errors.append('%s: %d %%hook/%%group but %d %%end' % (name, opened, closed))

objc_sources = ''.join(read(n) for n in files if n.endswith(('.x', '.xm', '.m', '.mm')))
bridge = read('AwemeLiquidBar-Bridging-Header.h')
for match in re.finditer(r'FOUNDATION_EXPORT\s+[^;]*?\b(\w+)\s*\(', bridge):
    function = match.group(1)
    if not re.search(r'^[A-Za-z_][\w\s\*]*\b%s\s*\([^;]*\)\s*\{' % function, objc_sources, re.M):
        errors.append('bridging header function without a definition: %s' % function)

swift = ''.join(read(n) for n in files if n.endswith('.swift'))
tweak = read('Tweak.x')


def objc_selectors(cls):
    block = re.search(r'@interface %s\b.*?@end' % cls, tweak, re.S)
    if not block:
        return None
    body = block.group(0)
    selectors = set()
    for method in re.finditer(r'^[-+]\s*\([^)]*\)\s*([^;]+);', body, re.M):
        parts = re.findall(r'(\w+)\s*:', method.group(1))
        selectors.add(''.join(p + ':' for p in parts) if parts else method.group(1).strip())
    for prop in re.finditer(r'^@property\s*\(([^)]*)\)\s*[^;]*?(\w+)\s*;', body, re.M):
        attributes, name = prop.group(1), prop.group(2)
        selectors.add(name)
        if 'readonly' not in attributes:
            selectors.add('set' + name[0].upper() + name[1:] + ':')
    return selectors


def swift_class_body(objc_name):
    start = re.search(r'@objc\(%s\)\s*\n[^\n]*class\s+\w+' % objc_name, swift)
    if not start:
        return ''
    index, depth, begun = start.end(), 0, False
    for position in range(index, len(swift)):
        character = swift[position]
        if character == '{':
            depth += 1
            begun = True
        elif character == '}':
            depth -= 1
            if begun and depth == 0:
                return swift[index:position]
    return ''


def swift_selectors(body):
    selectors = set(re.findall(r'@objc\(([\w:]+)\)', body))
    for match in re.finditer(r'@objc\s+(?:public\s+|open\s+)?(?:private\(set\)\s+)?(?:public\s+)?(?:weak\s+)?var\s+(\w+)', body):
        name = match.group(1)
        selectors.add(name)
        selectors.add('set' + name[0].upper() + name[1:] + ':')
    for match in re.finditer(r'@objc\s+(?:public\s+|open\s+)?func\s+(\w+)\(\)', body):
        selectors.add(match.group(1))
    return selectors


for cls in ['ALB3TopChromeView', 'ALB3BarGlassButton', 'ALB3Edge', 'AWEAlbumMorphTabController']:
    declared = objc_selectors(cls)
    if declared is None:
        errors.append('Tweak.x does not declare %s' % cls)
        continue
    exported = swift_selectors(swift_class_body(cls))
    if not exported:
        errors.append('no Swift class exported as %s' % cls)
        continue
    for selector in sorted(declared - exported):
        errors.append('%s: Tweak.x declares -%s, Swift does not export it' % (cls, selector))

if errors:
    print('preflight FAILED')
    for line in errors:
        print('  - ' + line)
    sys.exit(1)
print('preflight OK: %d sources, Logos balanced, bridge and Swift selectors match' % len(files))
