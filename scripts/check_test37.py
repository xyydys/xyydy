#!/usr/bin/env python3
"""TEST37 source integration / preservation, not UIKit or FPS verification."""
from pathlib import Path
import hashlib
import json
from check_logos_boundaries import mask_literals

r=Path(__file__).resolve().parents[1]
t=(r/'Tweak.x').read_text()
b=(r/'ALB5BottomGeometry.m').read_text()
s=(r/'ALBCenterMorphSurface.swift').read_text()
g=(r/'ALBCenterMorphGeometry.swift').read_text()
top=(r/'ALBTopNative/ALBTopNativeGroup.m').read_text()
audit=json.loads((r/'validation/test37_repair_audit.json').read_text())

def definition(source, signature):
    start=source.index(signature)
    opening=source.index('{',start)
    while ';' in source[start:opening]:
        start=source.index(signature,start+len(signature))
        opening=source.index('{',start)
    clean=mask_literals(source);depth=0
    for i in range(opening,len(source)):
        depth+=(clean[i]=='{')-(clean[i]=='}')
        if depth==0:return source[start:i+1]
    raise AssertionError(signature)

def sha(value): return hashlib.sha256(value.encode()).hexdigest()

# Exact TEST9 behavior, not another replica of its interaction.
slot=t[t.index('@interface AWEHomeTopSlotView'):t.index('static NSString *AWEHomeActionSymbolKey')]
assert sha(slot)==audit['test9_slot_sha256']
for signature, wanted in audit['test9_functions_sha256'].items():
    assert sha(definition(t,signature))==wanted,signature
binding=definition(t,'static void AWEConfigureTopSlot(UIView *container,').split('    if (@available(iOS 26.0, *)) {\n        slot.photosGlassHost.hidden = YES;',1)[1].split('    if (directSearchPassthrough) {\n        AWEClearTopMenuMorph(slot);',1)[0]
assert sha(binding)==audit['test9_native_binding_sha256']
assert 'ALBTopNativeIconButton' not in t+top+(r/'ALB5Settings.h').read_text()
assert 'ALB32TopMenuAnchorButton' not in top
assert 'performPrimaryAction' not in top
configuration=definition(t,'static void AWEConfigureAdaptiveSystemGlassIconButton(')
assert 'tintedGlass = topControl ? YES : NO;' in configuration
assert sha(definition(top,'- (void)applyAppearance {'))==audit['middle_appearance_sha256']

# Foreground lease covers every sibling, leaves selection masks/animations alone,
# and thaws behind the cover before the expanded renderer becomes visible.
lease=b.split('// TEST37: treat ordinary content',1)[1].split('// Search has no selected/unselected pair.',1)[0]
assert 'layer.mask' not in lease and 'removeAnimation' not in b
assert 'geometry.freezesLayout=YES' in lease and 'geometry.hidesForeground=YES' in lease
assert 'geometry.locksGeometry=NO; geometry.freezesLayout=NO' in lease
capture=definition(b,'NSDictionary *ALB27CaptureCenterChrome(')
for name in ['SelectedContentView','DestOutView','LiquidLens','ContentView']:
    assert name in capture
assert '[part isDescendantOfView:primary]' in capture
assert 'searchGlyphs' in capture and 'incomplete-native-foreground-group' in capture
assert 'snapshotViewAfterScreenUpdates' not in b
center=s.split('final class ALBCenterMorphSurface',1)[1].split('final class ALBBottomMaterialMode',1)[0]
assert 'UIVisualEffectView(effect:' not in center and 'UIGlassEffect(style:' not in center
apply=definition(center,'private func apply(progress value:')
assert 'primary.place(pose.primary, in: host, hidden: atEnd)' in apply
assert 'translationOnly: true' in apply
assert 'opacity: atEnd ? 0' not in apply
assert 'if !translationOnly {' in center
assert 'width: expandedSearch.width, height: expandedSearch.height' in g
assert 'min(searchRect.width, searchRect.height) * 0.90' in center
assert 'host.convert(avatarRect, to: destination)' in apply
assert 'nativeSearchTapped' in center and 'tap.cancelsTouchesInView = false' in center
restore=center.split('    func restore() {\n        recordDiagnostic',1)[1].split('    override func didMoveToWindow',1)[0]
for before,after in [('primary.restore(); search.restore()', 'ALB37ThawForeground(branch)'),
                     ('ALB37ThawForeground(branch)', 'nativeBar?.layoutIfNeeded()'),
                     ('nativeBar?.layoutIfNeeded()', 'ALB33EndForegroundLease(branch)')]:
    assert restore.index(before)<restore.index(after)
badge=apply.split('if item.badge {',1)[1].split('continue',1)[0]
assert 'item.view.center = item.start' in badge
assert '1 - progress / 0.14' in badge and 'contentTravel' not in badge
assert 'let travel = pose.contentTravel' in apply
# Search covers only image/label leaves, so native button press remains routable.
publicCover=definition(b,'void ALB37CoverSearchGlyph(')
assert 'UIImageView.class' in publicCover and 'UILabel.class' in publicCover
assert 'geometry.hidesForeground' not in publicCover
assert 'ALB37UncoverSearchGlyph(glyph)' in restore

# Outer sticky list still cannot enter the broad frame/inset rewrite path.
adjust=definition(t,'static BOOL AWEBottomV15AdjustView(')
assert adjust.index('if (route == AWEBottomRouteProfile) return NO;')<adjust.index('CGRect wr =')
inner=definition(t,'static void ALB37ExtendProfileInnerList(')
assert 'ALB37IsInnerProfileGrid(list)' in inner and 'ALB34ProfileScrollChainBusy(list)' in inner
assert 'ALB37CompensatedBottomInset' in inner
assert 'contentSize=' not in inner and 'inset.top=' not in inner
assert '%hook UIScrollView\n' not in t and '@"profile.collection.layout"' not in t
for name,wanted in audit['expected_unchanged_sha256'].items():
    assert hashlib.sha256((r/name).read_bytes()).hexdigest()==wanted,name
assert sha(t.split('#pragma mark - Local account authorization gate',1)[1].split('#pragma mark - Transition diagnostics',1)[0])==audit['auth47_gate_sha256']
# Scope fence: package must not transplant TEST9's unrelated old layout,
# screenshot capture, tab actions, profile coordinator or appearance code.
normalized=t
for signature in audit['tweak_allowed_definitions']:
    fragment=definition(normalized,signature)
    normalized=normalized.replace(fragment,'/* RELEASE_EDIT '+signature+' */',1)
a=normalized.index('@interface AWEHomeTopSlotView')
boundary=normalized.index('static NSString *AWEHomeActionSymbolKey',a)
normalized=normalized[:a]+'/* TOP_SOURCE */\n'+normalized[boundary:]
a=normalized.index('// TEST37: extend only the inner vertical grid')
boundary=normalized.index('\nstatic void AWEExtendRouteContentBehindBottomBar',a)
normalized=normalized[:a]+'/* PROFILE_INNER_VIEWPORT */\n'+normalized[boundary:]
normalized=normalized.replace("""    // Only the known inner profile grid; no offset hook or scheduled scan.
    if (gAWEAlbumMorphVisible && AWECurrentMainTabIndex()==4)
        ALB37ExtendProfileInnerList((UICollectionView *)self);
""",'')
normalized=normalized.replace('TEST37 FIX1','TEST37 REPAIR').replace('TEST37-FIX1','TEST37-REPAIR')
normalized=normalized.replace('TEST37','RELEASE').replace('test37','release')
assert sha(normalized)==audit['tweak_outside_requested_sections_sha256']
assert '2.0.10-test37-fix1' in (r/'control').read_text()
print('TEST37: exact TEST9 button/gesture source, original bottom material ownership, full foreground lease, translation-only search, one compact glass, stationary badge fade and compensated inner Favorites contracts passed. Source checks only.')
