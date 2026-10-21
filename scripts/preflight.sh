#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
# The old opaque-settings / disabled-tracking gates were superseded by the
# user's explicit glass restoration. Compilation remains authoritative.
python3 - <<'CHECK'
from pathlib import Path
import re
r=Path('.')
control=dict(line.split(': ',1) for line in (r/'control').read_text().splitlines() if ': ' in line)
if control.get('Package') != 'com.xyy.awemeliquidbar': raise SystemExit('Unexpected package identity')
if control.get('Version') != '2.3': raise SystemExit('Unexpected package version')
m=(r/'Makefile').read_text()
# No project-wide C++ dialect may enter the Swift Objective-C importer.
if re.search(r'^AwemeLiquidBar_(?:CFLAGS|CCFLAGS|SWIFTFLAGS).*?-std=(?:gnu\+\+|c\+\+)',m,re.M):
    raise SystemExit('C++ dialect flag leaked into mixed-language target')
if (r/'AudioMeter').exists(): raise SystemExit('Retired AudioMeter directory is present')
sources=re.search(r'^AwemeLiquidBar_FILES\s*=\s*(.+)$',m,re.M).group(1).split()
headers=['ALBReproCapture.h','ALBReproCaptureSignal.h','ALB22NativeEndpoint.h','ALB22NativeMethodPolicy.h','AwemeLiquidBar-Bridging-Header.h','ALBNativeState.h','ALBNativeInteraction.h','ALBSettingsGlass.h','ALBSidePalette.h','ALBAlbumVideoExporter.h','ALBScrollIntent.h','ALBNativeMinimize.h','ALBGlassMaterials.h','ALBRootScroll.h','ALBClearContrast.h','ALBAvatarMenuTouch.h']
for name in sources+headers:
    if not (r/name).is_file() or (r/name).stat().st_size==0: raise SystemExit('Missing source: '+name)
for name in sources:
    for header in re.findall(r'^#(?:include|import) "([^"]+)"',(r/name).read_text(),re.M):
        if not ((r/name).parent/header).is_file() and not (r/header).is_file(): raise SystemExit('Missing local include: '+header)
# Guard TEST17 ownership paths from accidental merges with old hot-loop code.
t=(r/'Tweak.x').read_text();g=(r/'ALB5BottomGeometry.m').read_text()
if 'gAWEAlbumMorphCompact?ALB5Number(@"top.compactScale"' in t: raise SystemExit('Top scale is coupled to bottom state again')
if 'ALB10RightAlphaChanged(((UIView *)self).alpha,nativeAlpha)' not in t: raise SystemExit('Missing native-alpha comparison hook')
# TEST37 foreground covers are temporary, separately owned and never applied
# to the two material roots. The retired root/platter clipping stays forbidden.
geometry=g.split('// TEST37: treat ordinary content',1)[0]
if 'compactMasks' in g or '.layer.mask' in geometry: raise SystemExit('Retired material clipping mask returned')
for name in sources:
    text=(r/name).read_text()
    if 'DKAudioTap' in text or 'ALB5WaveRing' in text: raise SystemExit('Retired waveform source: '+name)
print('2.3 source/package preflight passed; compile and device testing still required.')
CHECK

python3 scripts/check_auth51.py
python3 scripts/check_native_contract.py
python3 scripts/check_test42.py
python3 scripts/check_test43.py
python3 scripts/check_native_state.py
python3 scripts/check_logos_boundaries.py

# Run every SDK-independent CI source gate locally too. Keep Swift/SDK gates in CI.
python3 scripts/check_watchdog_guards.py
python3 scripts/check_right_persistence.py
python3 scripts/check_test24.py
python3 scripts/test_bottom_scale_feedback.py
python3 scripts/check_test29.py
python3 scripts/check_test33.py
python3 scripts/check_test34.py
python3 scripts/check_test35.py
python3 scripts/check_test36.py
python3 scripts/test_profile_scroll_preservation.py
python3 scripts/check_repro_capture.py

python3 scripts/check_unified_material.py
