#!/usr/bin/env python3
"""TEST42 source integration guards; not an Apple SDK or on-device test."""
from pathlib import Path
from check_logos_boundaries import mask_literals
r=Path(__file__).resolve().parents[1]
g=(r/'ALB5BottomGeometry.m').read_text()
t=(r/'Tweak.x').read_text(); b=(r/'AwemeAlbumMorphBar.swift').read_text()
m=(r/'ALBTopMaterial.h').read_text(); rail=(r/'ALB5RightLayout.h').read_text()
# TEST42 FIX8 (user request): centered compact scheme deleted.
assert not (r/'ALBCenterMorphSurface.swift').exists()
assert 'ALB42CenterForegroundViews' not in g and 'centeredSurface' not in b
# Top and bottom now share the native material policy; side buttons keep
# their UIKit-owned interactive glass and no live-effect copying is used.
assert 'effect copy' not in m and 'syncMaterial' not in m and '.effect=' not in m
assert 'addTarget:' not in m and 'hitTest:' not in m and 'setTransform:' not in m
assert '[[ALB42TopMaterialButton alloc] initWithFrame:' in t
assert 'ALBMaterialMode() == ALBMaterialRegular' in t
assert 'ALB42RightUpperGapShift(musicTop,rowBottom,musicGap,columnScale)' in rail
assert 'shifts[@"music"]=@0;' in rail
assert 'ALB6RightNativeVisible(anchors[candidate],stack)' in rail
assert 'if (!scrolling && !CGPointEqualToPoint(list.contentOffset,offset))' in t
for name in ['ALBTopMaterial.h','ALB5RightLayout.h','ALBBottomSupport.swift','AwemeAlbumMorphBar.swift']:
    code=mask_literals((r/name).read_text()); stack=[]; pairs={')':'(',']':'[','}':'{'}
    for c in code:
        if c in '([{': stack.append(c)
        elif c in pairs: assert stack and stack.pop()==pairs[c],name
    assert not stack,name
assert 'Version: 2.3' in (r/'control').read_text()
print('TEST42 scoped integration guards passed; device rendering remains unverified.')

