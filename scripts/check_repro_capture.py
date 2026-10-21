#!/usr/bin/env python3
"""Capture integration/source boundaries. NOT an Apple SDK or device test."""
from pathlib import Path
import hashlib
import json
import re
from check_logos_boundaries import mask_literals

root = Path(__file__).resolve().parents[1]
def need(ok, message):
    if not ok:
        raise SystemExit('TEST30 CAPTURE: ' + message)

header = (root/'ALBReproCapture.h').read_text()
settings = (root/'ALBSettingsGlass.h').read_text()
signal = (root/'ALBReproCaptureSignal.h').read_text()
tweak = (root/'Tweak.x').read_text()
code = mask_literals(header)
# 2.2 (user request): the floating reproduction-capture overlay is removed. The
# C API stays (inert) so call sites compile; nothing records, draws or writes.
for name in ['ALBReproIsRecording', 'ALBReproIsEnabled', 'ALBReproRecordEvent',
             'ALBReproScheduleSnapshot', 'ALBReproRecordTouch', 'ALBReproSetEnabled',
             'ALBReproRestoreIfEnabled', 'ALBReproConfigure']:
    need(name+'(' in signal and name+'(' in header, 'missing ABI: '+name)
need('extern "C"' in signal, 'ObjC++ / Swift C linkage is missing')
need(tweak.count('#include "ALBReproCapture.h"') == 1, 'collector implementation must be included once')
need('ALBReproCaptureSignal.h' in (root/'AwemeLiquidBar-Bridging-Header.h').read_text(), 'Swift import missing')
need('BOOL ALBReproIsRecording(void) { return NO; }' in header and 'BOOL ALBReproIsEnabled(void) { return NO; }' in header,
     'reproduction capture can still record')
for retired in ['UIWindow', 'ALBRPManager', 'writeToFile', 'NSFileHandle', 'addSubview']:
    need(retired not in code, 'floating capture machinery returned: '+retired)
need('ALBRP' not in mask_literals(tweak), 'Tweak.x still references capture overlay classes')
need('专项复现采集悬浮窗' not in tweak and 'reproCaptureSwitchChanged' not in tweak, 'capture overlay switch still shown')
need('交互诊断日志' in tweak and 'exportProbeLogs:' in tweak, 'everyday diagnostic log/export removed')
for name, source in [('settings',settings)]:
    need(not re.search(r'preferredCornerRadius\s*\?:',source), name+' uses a scalar as a nullable object')
fixture=(root/'Tests/Support/repro_capture_compile.mm').read_text()
need('ALBReproCapture.h' in fixture and 'ALBSettingsGlass.h' in fixture,
     'SDK fixture does not cover both affected production headers')
need('%hook UIApplication' in tweak and 'return %orig(action, target, sender, event);' in tweak,
     'passive action forwarding is missing')
for name in ['ALBReproCapture.h','ALBReproCaptureSignal.h','ALBAvatarMenuTouch.h','ALBSettingsGlass.h']:
    clean = mask_literals((root/name).read_text()); stack=[]; pairs={')':'(',']':'[','}':'{'}
    for char in clean:
        if char in '([{': stack.append(char)
        elif char in ')]}': need(bool(stack) and stack.pop()==pairs[char], 'unbalanced source: '+name)
    need(not stack, 'unclosed source: '+name)

audit=json.loads((root/'validation/test37_repair_audit.json').read_text())
# TEST38 intentionally changes right-side projection geometry and extends its
# SDK-free persistence fixture. Capture ABI/collector files remain byte-exact.
audit['expected_unchanged_sha256'].update(json.loads((root/'validation/test42_preservation.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((root/'validation/material2_changes.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((root/'validation/auth51_changes.json').read_text())['intentional_changes'])
for name, wanted in audit['expected_unchanged_sha256'].items():
    need(hashlib.sha256((root/name).read_bytes()).hexdigest()==wanted, 'unexpected runtime change: '+name)
workflow=(root/'.github/workflows/build.yml').read_text()
need('Tests/Support/repro_capture_compile.mm' in workflow and 'Typecheck bottom Swift' in workflow,
     'real SDK collector/Swift gates missing')
need('Version: 2.3' in (root/'control').read_text(), 'package version mismatch')
print('2.3 CAPTURE: floating overlay removed, inert ABI, everyday diagnostics kept, preservation checks passed; no SDK/device claim.')
