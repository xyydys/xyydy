#!/usr/bin/env python3
"""Nine-regression integration guards. Not an SDK compile or device claim."""
from pathlib import Path
import hashlib, json
from check_logos_boundaries import mask_literals
r=Path(__file__).resolve().parents[1]
support=(r/'ALBBottomSupport.swift').read_text()
sw=(r/'AwemeAlbumMorphBar.swift').read_text()
t=(r/'Tweak.x').read_text()
top=(r/'ALBTopNative/ALBTopNativeGroup.m').read_text()
b=(r/'ALB5BottomGeometry.m').read_text()
root=(r/'ALBRootScroll.h').read_text()
settings=(r/'ALB5Settings.h').read_text()
# TEST42 FIX8 (user request): the centered-avatar compact scheme is deleted.
# No capture, lease or snapshot of the native bar may come back.
assert not (r/'ALBCenterMorphSurface.swift').exists() and not (r/'ALBCenterMorphGeometry.swift').exists()
for retired in ['ALB27CaptureCenterChrome','ALB32ApplyCenterGeometryLease','ALB33BeginForegroundLease','centerHitView']:
    assert retired not in b, retired
assert 'snapshotViewAfterScreenUpdates' not in b
assert 'NSInteger ALB15CompactStyle(void) { return 1; }' in b
# TEST37 restores TEST9's actual UIButton source, not a second menu anchor.
assert 'ALB24ColumnMenuRow' not in t and 'elementWithUncachedProvider:' not in t
assert 'ALBTopNativeIconButton' not in t and 'ALB32TopMenuAnchorButton' not in top
assert 'button.menu = menu;' in t and 'button.showsMenuAsPrimaryAction = YES;' in t
# Appearance distinguishes visible Home app routes, without material replacement.
resolve=t.split('static UIUserInterfaceStyle AWEAlbumMorphResolvedInterfaceStyle(\n',1)[1].split('static void AWEScheduleStableHomeChromeStyle',1)[0]
assert 'route != AWEBottomRouteShop && route != AWEBottomRouteGrid' in resolve
assert 'AWEAppResolvedAppearanceStyle(window)' in resolve
commit=t.split('static void AWEScheduleStableHomeChromeStyle(',1)[1].split('static void AWEApplyAlbumMorphResolvedAppearance(',1)[0]
assert 'style = UIUserInterfaceStyleDark;' not in commit
assert 'ALBBottomMaterialMode' not in support
assert 'ALBMaterialApplyBar(nativeBar, YES)' in t
# Restart-safe geometry is owned above the controller, not inferred from frame.
assert t.count('overlay.rootViewController = [ALB33BottomOverlayController new]')==2
assert 'overlay.rootViewController = gAWEAlbumMorphController' not in t
assert 'canvas.transform = target' in sw and 'view.transform = target' not in sw
assert '[controller isKindOfClass:ALB33BottomOverlayController.class]' in t
# Hidden action sources and leased gestures remain eligible; no cross-item scan.
actions=t.split('static NSArray<UIView *> *ALB33AllHomeActionButtons(',1)[1].split('static BOOL AWERecognizerHasBusinessTarget',1)[0]
assert 'AWESortedVisibleHomeActionButtons' not in actions
assert 'ALB33ForwardLeasedActionFromSource' in actions
assert 'ordered.count==4' in actions and 'AWEViewIsDescendantOfView(owner, stack)' in actions
# No centered surface remains to own input.
assert 'centeredSurface' not in sw and 'applyCenteredMode' not in sw
# Both gestures honor the same opt-out, and never bind the app collection.
assert 'direction > 0 || ALB5Bool(@"collapse.expandOnDown", YES)' in root
assert 'next || ALB5Bool(@"collapse.expandOnDown", YES)' in root
assert '中间头像球' not in settings and '@"key":@"collapse.nativeStyle"' not in settings
assert 'ALBBindMinimizeSources(' not in sw and 'setContentScrollView:feed' not in t
assert 'AWEAlbumMorphRegisterContentScrollView' not in t
assert 'if (ALB34ProfileScrollChainBusy(view)) return NO;' in t
# Runtime preservation is audited against TEST36; the requested top-button
# path is now deliberately changed, while the middle material body is retained.
audit=json.loads((r/'validation/test37_repair_audit.json').read_text())
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/test42_preservation.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/material2_changes.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((r/'validation/auth51_changes.json').read_text())['intentional_changes'])
for n,h in audit['expected_unchanged_sha256'].items():
 assert hashlib.sha256((r/n).read_bytes()).hexdigest()==h, n
print('TEST33 retained action, route, scroll, scaling and material contracts passed; no SDK/device claim.')
