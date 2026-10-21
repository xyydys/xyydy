#!/usr/bin/env python3
"""TEST38 scoped source contracts. Not an iOS render/device test."""
from pathlib import Path
import hashlib, json, re
r=Path(__file__).resolve().parents[1]
t=(r/'Tweak.x').read_text(); right=(r/'ALB5RightLayout.h').read_text()
settings=(r/'ALB5Settings.h').read_text(); surface=(r/'ALBCenterMorphSurface.swift').read_text()
interaction=(r/'ALBNativeInteraction.h').read_bytes()
audit=json.loads((r/'validation/test37_repair_audit.json').read_text())

def req(ok,msg):
    if not ok: raise SystemExit('TEST38: '+msg)

def body(text, marker):
    start=text.index('{',text.index(marker)); depth=0
    clean=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',lambda m:' '*len(m.group()),text)
    for i in range(start,len(text)):
        if clean[i]=='{': depth+=1
        elif clean[i]=='}':
            depth-=1
            if depth==0:return text[start:i+1]
    raise SystemExit('unbalanced '+marker)

# Top: implementation/action/menu/geometry remains TEST9-era; only the material
# selector is no longer force-switched to tinted regular glass.
config=body(t,'static void AWEConfigureAdaptiveSystemGlassIconButton(')
req('tintedGlass = topControl ? YES : NO;' not in config,'top side material is still forced tinted')
req('if (playbackControl) tintedGlass = NO;' in config,'playback clear-material behavior changed')
topslot=body(t,'static void AWEConfigureTopSlot(UIView *container,')
req('AWEHomeTintedGlassEnabled(),' in topslot,'top slot no longer obtains the neutral UIKit material from the existing caller')
req('AWELeaseSharedSearchGestureToView(' in topslot and 'button.menu = menu;' in t,
    'top search/menu behavior source changed')

# Compact avatar: never enter the private search/platter lens subtree at endpoint.
apply=body(surface,'private func apply(progress value:')
req('let destination: UIView = self' in apply,'compact avatar is not pinned to the foreground sibling plane')
req('avatarHost ?? search.view' not in apply,'compact avatar still reparents into the native lens hierarchy')

# Favorites/profile: clear only native carrier backgrounds before reveal; profile
# scrolling remains on the TEST37 viewport contract.
clear=body(t,'static void AWEBottomV22ClearShopNativeBarFast(')
req('route!=AWEBottomRouteProfile' in clear,'profile route is not admitted to carrier background cleanup')
extend=body(t,'static void AWEExtendRouteContentBehindBottomBar(')
profile=extend.split('if (route == AWEBottomRouteProfile) {',1)[1].split('    if (route == AWEBottomRouteGrid)',1)[0]
req('AWEBottomV22ClearShopNativeBarFast' in profile and 'ALB36RevealProfileBottom' in profile,
    'profile branch does not clear the carrier before revealing content')
req('setContentOffset' not in profile and '.frame=' not in profile and 'contentInset' not in profile,
    'profile background fix changes scroll geometry')

# Right: one scale, exact Share/music gap offset, vertical-axis alignment in the
# persistent projection, and no touch/deformation implementation changes.
req('right.musicScale' not in settings+right,'legacy music-only scale remains active')
req('right.shareMusicGap' in settings+right,'Share/music distance control missing')
req('右侧整体缩放（含音乐）' in settings,'whole-rail scale is not exposed in settings')
req('ALB6RightMapPointOffset' in right and 'ALB19RightUnmapPointOffset' in right,
    'horizontal alignment is not reversible in the persistent projection')
right_apply=body(right,'static void ALB5ApplyRightLayout(UIView *stack) {')
req('CGFloat scale=columnScale;' in right_apply and 'CGPoint rolePivot=pivot;' in right_apply,
    'music is not using the same scale/pivot as the right rail')
req('musicXShift=axisFinalX-musicFinalX;' in right_apply,'music is not aligned to the action column')
req('shareMusicGap/MAX(.0001,columnScale)' in right_apply,'Share/music gap is not compensated to an on-screen delta')
req('record.xShift=xShift;record.shift=shift;record.scale=scale' in right_apply,
    'new alignment is not retained by the native-transform projection lease')
req(hashlib.sha256(interaction).hexdigest()==audit['expected_unchanged_sha256']['ALBNativeInteraction.h'],
    'native press/artwork interaction implementation changed')
req('plate.contentHorizontalAlignment = UIControlContentHorizontalAlignmentCenter;' in t and
    'plate.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;' in t,
    'music artwork is not explicitly centered inside its glass button')

req('2.0.10-test38' in (r/'control').read_text(),'package revision mismatch')
print('TEST38 scoped avatar/background/top-material/right-layout contracts passed. Source checks only.')
