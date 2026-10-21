#!/usr/bin/env python3
"""User-requested capsule isolation, avatar attachment and old no-jump boundary.
Source integration only: no UIKit rendering or device-performance claim.
"""
from pathlib import Path
import hashlib
import json
from check_logos_boundaries import mask_literals

r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.x').read_text()
b = (r/'ALB5BottomGeometry.m').read_text()
sw = (r/'AwemeAlbumMorphBar.swift').read_text()

def body(source, signature):
    opening = source.index('{', source.index(signature))
    code = mask_literals(source)
    depth = 0
    for i in range(opening, len(source)):
        depth += (code[i] == '{') - (code[i] == '}')
        if depth == 0:
            return source[opening+1:i]
    raise AssertionError(signature)

adjust = body(t, 'static BOOL AWEBottomV15AdjustView(')
assert adjust.index('if (route == AWEBottomRouteProfile) return NO;') < adjust.index('CGRect wr =')
route = body(t, 'static void AWEExtendRouteContentBehindBottomBar(')
profile_branch=route.index('if (route == AWEBottomRouteProfile) {')
profile_return=route.index('return;',profile_branch)
assert route.index('ALB24ClearNativeBottomBackdrops(window,route)') < profile_branch < profile_return
assert route.index('AWEBottomV22ClearShopNativeBarFast',profile_branch,profile_return) < route.index('ALB36RevealProfileBottom',profile_branch,profile_return)
assert profile_return < route.index('NSMutableArray<UIView *> *queue')
assert '%hook UIScrollView\n' not in t
assert '@"profile.layout"' not in t and '@"profile.collection.layout"' not in t

# TEST42 FIX8 (user request): the centered morph and its native-foreground
# capture/lease/avatar attachment are deleted outright.
assert not (r/'ALBCenterMorphSurface.swift').exists()
assert 'snapshotViewAfterScreenUpdates' not in b
for retired in ['ALB27SnapshotForeground', 'ALB33BeginForegroundLease', 'searchArtworkHost']:
    assert retired not in b, retired
assert 'surface.restore()' not in sw and 'centeredSurface' not in sw

audit = json.loads((r/'validation/test37_repair_audit.json').read_text())
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/test42_preservation.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/material2_changes.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/auth51_changes.json').read_text())['intentional_changes'])
for name, expected in audit['expected_unchanged_sha256'].items():
    assert hashlib.sha256((r/name).read_bytes()).hexdigest() == expected, name
print('TEST35: no lens snapshots; complete selection lease; native platter foreground attachment; historical profile exclusion; TEST9 native top button source. Source checks only.')
