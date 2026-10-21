#!/usr/bin/env python3
"""Three-regression source integration / preservation checks; not UIKit tests."""
from pathlib import Path
import hashlib
import json
from check_logos_boundaries import mask_literals

r = Path(__file__).resolve().parents[1]
top = (r/'ALBTopNative/ALBTopNativeGroup.m').read_text()
bottom = (r/'ALB5BottomGeometry.m').read_text()
swift = (r/'AwemeAlbumMorphBar.swift').read_text()

def body(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    masked = mask_literals(source)
    depth = 0
    for i in range(opening, len(source)):
        depth += (masked[i] == '{') - (masked[i] == '}')
        if depth == 0:
            return source[opening+1:i]
    raise AssertionError('unclosed source: ' + signature)

# TEST37 retires the separate-anchor lifecycle entirely. UIKit presents its
# UIButton's own menu; exact TEST9 source equality is checked by check_test37.
assert '@implementation ALBTopNativeIconButton' not in top
assert 'nativeAnchorWillEnd:' not in top

# TEST42 FIX8 (user request): the centered avatar / search-to-avatar morph is
# deleted. The native layout keeps its per-pass work without a surface branch.
assert not (r/'ALBCenterMorphSurface.swift').exists()
assert 'searchArtworkHost' not in bottom and 'ALB32ApplyCenterGeometryLease' not in bottom
layout = body(swift, 'override func viewDidLayoutSubviews()')
assert 'queueNativeRefresh()' in layout and 'applyCompactUnreadBadgeScale(in: tabBar)' in layout
assert 'centeredSurface' not in layout

audit = json.loads((r/'validation/test37_repair_audit.json').read_text())
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/test42_preservation.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/material2_changes.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/auth51_changes.json').read_text())['intentional_changes'])
for name, expected in audit['expected_unchanged_sha256'].items():
    assert hashlib.sha256((r/name).read_bytes()).hexdigest() == expected, name
print('TEST34 retained avatar attachment, material ownership and layout-cost guards passed; no SDK/device claim.')
