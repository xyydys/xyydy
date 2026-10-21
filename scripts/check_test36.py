#!/usr/bin/env python3
"""Foreground ownership and clip-only profile repair. Source checks, not UIKit."""
from pathlib import Path
import hashlib
import json
from check_logos_boundaries import mask_literals

r = Path(__file__).resolve().parents[1]
b = (r/'ALB5BottomGeometry.m').read_text()
sw = (r/'AwemeAlbumMorphBar.swift').read_text()
t = (r/'Tweak.x').read_text()

def body(source, signature):
    start = source.index('{', source.index(signature))
    code = mask_literals(source)
    depth = 0
    for i in range(start, len(code)):
        depth += (code[i] == '{') - (code[i] == '}')
        if depth == 0:
            return source[start+1:i]
    raise AssertionError(signature)

# TEST42 FIX8 (user request): the centered morph's foreground leases are
# deleted; the syncing path no longer has a surface to release.
for retired in ['ALB33BeginForegroundLease', 'ALB36ReapplyForegroundCover', 'ALB42CenterForegroundViews',
                'ALB27CaptureCenterChrome']:
    assert retired not in b, retired
sync = body(sw, 'func syncSelectedOriginalIndex(')
assert sync.index('guard selectedTab !== target') < sync.index('selectedTab = target')
assert 'resetCenteredMode()' not in sw
route = body(t, 'static void AWEExtendRouteContentBehindBottomBar(')
profile_branch=route.index('if (route == AWEBottomRouteProfile) {')
profile_return=route.index('return;',profile_branch)
assert route.index('AWEBottomV22ClearShopNativeBarFast',profile_branch,profile_return) < route.index('ALB36RevealProfileBottom(window)',profile_branch,profile_return) < profile_return
for signature in ['static void ALB36OpenProfileClipChain(', 'static void ALB36RevealProfileBottom(',
                  'static void ALB36ProfileViewportDidLayout(']:
    code = mask_literals(body(t, signature))
    for forbidden in ['contentOffset', 'contentInset', 'contentSize', '.frame=', '.frame =',
                      '.bounds=', '.bounds =', 'reloadData', 'setNeedsLayout', 'dispatch_async']:
        assert forbidden not in code, (signature, forbidden)
assert 'ALB36RestoreProfileClips();' in route

audit = json.loads((r/'validation/test37_repair_audit.json').read_text())
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/test42_preservation.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/material2_changes.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/auth51_changes.json').read_text())['intentional_changes'])
for name, expected in audit['expected_unchanged_sha256'].items():
    assert hashlib.sha256((r/name).read_bytes()).hexdigest() == expected, name
print('TEST36 retained restore-before-reveal and native-state ownership guards passed; foreground masks superseded by TEST37. Source checks only.')
