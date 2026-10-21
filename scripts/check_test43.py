#!/usr/bin/env python3
"""TEST42 FIX3 source guards for the four targeted repairs; not an SDK/device test."""
from pathlib import Path

r = Path(__file__).resolve().parents[1]


def load():
    return {name: (r / name).read_text() for name in
            ('Tweak.x', 'AwemeAlbumMorphBar.swift', 'ALBTopMaterial.h', 'ALBSidePalette.h')}


def problems(src):
    t, sw, m, p = src['Tweak.x'], src['AwemeAlbumMorphBar.swift'], src['ALBTopMaterial.h'], src['ALBSidePalette.h']
    found = []
    # 1. First-entry Favorites: the pager's own layout applies the existing
    #    inner-grid extension to its visible page; no timer/polling added.
    pager = t.split('%hook AWEListKitTabContentCollectionView', 1)[1].split('%end', 1)[0]
    if 'ALB43RevealVisibleProfilePages((UICollectionView *)self);' not in pager:
        found.append('pager layout does not reveal its visible profile page')
    reveal = t.split('static void ALB43RevealVisibleProfilePages(', 1)[1].split('\n}\n', 1)[0]
    for need in ('pager.visibleCells', 'ALB36ProfileViewportDidLayout(grid);', 'ALB37ExtendProfileInnerList(grid);',
                 'AWECurrentMainTabIndex()!=4'):
        if need not in reveal:
            found.append('favorites reveal missing: ' + need)
    if 'dispatch_after' in reveal or 'NSTimer' in reveal or 'CADisplayLink' in reveal:
        found.append('favorites reveal uses a delay/poll')
    # 2. Shared top/bottom material: the compact top native UITabBar follows
    #    the bottom material policy. It is never independently forced Clear.
    if 'syncMaterial' in m + t or 'effect copy' in m:
        found.append('side-button material copy returned')
    if 'ALB43ApplyTopPlatterClear' in t or 'ALB50ApplyTopClear' in t:
        found.append('retired forced-top-Clear path returned')
    if 'ALB50ApplyTopSharedBottomMaterial((UITabBar *)self);' not in t:
        found.append('top native bar does not apply shared bottom material in layout')
    shared = t.split('static void ALB50ApplyTopSharedBottomMaterial(', 1)[1].split('\n}\n', 1)[0]
    for need in ('ALB5ApplySharedBottomSurface(bar);', 'ALBMaterialApplyBar(bar, NO);'):
        if need not in shared: found.append('shared top material missing: '+need)
    # 3. FIX8 (user request): the centered-ball scheme is deleted outright.
    if (r / 'ALBCenterMorphSurface.swift').exists() or 'centeredSurface' in sw or 'applyCenteredMode' in sw:
        found.append('centered compact scheme returned')
    # 4. Double-tap panel: bounded read-only diagnostic on every failed forward.
    for kind in ('AWEHomeActionKindFavorite,@"favorite"', 'AWEHomeActionKindShare,@"share"'):
        if 'ALB43RecordPanelActionMiss(stack,' + kind + ');' not in p:
            found.append('panel miss not recorded: ' + kind)
    miss = t.split('static void ALB43RecordPanelActionMiss(', 1)[1].split('\n}\n', 1)[0]
    if 'i < 400 && others.count < 16' not in miss or 'videoButtons.count >= 10' not in miss:
        found.append('panel diagnostic is not bounded')
    for forbidden in ('sendActionsForControlEvents', 'setState:', 'accessibilityActivate', 'addGestureRecognizer'):
        if forbidden in miss:
            found.append('panel diagnostic mutates: ' + forbidden)
    # FIX8 (user request): the middle group is the native UITabBar group again
    # (native lens/selection); the FIX6/FIX7 separated build is removed.
    if 'ALB45LayoutSeparatedTopGroup' in t or 'group.nativeGroup.hidden = YES' in t:
        found.append('native middle group is hidden behind a separated build')
    if 'ALBMaterialSnapshot()' not in t:
        found.append('top glass structure collection missing')
    # FIX6: right actions identified by runtime identity, ordered natively.
    if 'AWEHomeActionKind runtime=ALB44HomeActionKindFromRuntime(view);' not in t or \
            'AWEHomeActionKind kind = ALB44HomeActionKindFromRuntime(button);' not in t:
        found.append('right action matching does not use runtime identity first')
    if 'CGFloat ay=ALB44NativeMidY(a, stack);' not in t:
        found.append('panel ordering still uses plugin-transformed geometry')
    # FIX6: music artwork follows the avatar proportion rule.
    if 'ALBInstallNativeArtwork(plate, artwork, artworkSide, @"right.music.native");' not in t:
        found.append('music artwork size not derived like the avatar')
    # FIX6: left menu appears without Douyin's late entrance view.
    if 'BOOL ownerlessMenu = kind == AWEHomeTopActionKindMenu && AWETopMenuMorphEnabled();' not in t:
        found.append('left menu still waits for the Douyin entrance view')
    # FIX7: panel actions reach UIAction / nil-target handlers via UIControl.
    forward = t.split('static BOOL AWEForwardExactNativeTapFromView(UIView *sourceView,\n                                              NSString *context) {\n    if (!sourceView', 1)[1]
    forward = forward.split('\n}\n', 1)[0]
    if 'if (ALB46SendControlEventHandlers(sourceView, context)) return YES;' not in forward or \
            forward.index('ALB46SendControlEventHandlers') > forward.index('accessibilityActivate'):
        found.append('panel forward skips UIControl event handlers')
    if '[context hasPrefix:@"RIGHT_"] ? sourceView.superview : nil' not in forward:
        found.append('ancestor recognizer route is not limited to the right column')
    if '(nearTitles && (badgeClass || redDot))' not in t:
        found.append('category badges show through the top group')
    if 'ALBMaterialApplyBar(nativeBar, YES);' not in t or 'ALB50ApplyBottomClearFromController(tabBar)' not in sw:
        found.append('shared bottom material path missing')
    if 'ALBMaterialStyle(container, YES)' not in t:
        found.append('top uses a stale Light/Dark cache')
    if 'settleNativeLayoutAfterAnimation' in sw or 'hasNativeGeometryAnimation' not in sw:
        found.append('native transition still snaps layout at completion')
    if 'ALB49TraceNativeScale(tabBar, $0)' not in sw:
        found.append('native scale traces missing')
    # The experimental second top body glass is deleted outright.
    settings = (r/'ALB5Settings.h').read_text()
    if 'top.bodyGlass' in settings or 'ALB51ApplyTopBodyGlass' in t or 'AwemeLiquidBar.TopGroup.BodyGlass' in t:
        found.append('experimental top body glass returned')
    # 2.2: the failed FIX9 layer-hiding experiment is removed.
    if 'ALB48ApplyTopClearGroup' in t:
        found.append('failed top layer-hiding experiment returned')
    # FIX8: Douyin fires Favorite/Share from AWEFeedVideoButton's own touch
    # handling (device: handlers=0, touchesEnded overridden).
    if 'if ([context hasPrefix:@"RIGHT_"] && ALB47DeliverPanelTap(sourceView, context)) return YES;' not in forward or \
            forward.index('ALB47DeliverPanelTap') > forward.index('accessibilityActivate'):
        found.append('panel does not deliver the tap to the native touch handling')
    deliver = t.split('static BOOL ALB47DeliverPanelTap(', 1)[1].split('\n}\n', 1)[0]
    if 'ALB47OverridesTouchesEnded(button)' not in deliver or 'touchesCancelled' not in deliver:
        found.append('panel touch delivery is not limited/cancel-safe')
    # FIX8: a native minimize reversal waits for UIKit's running animation.
    issue = sw.split('private func issueNativeRequest(animated: Bool) {', 1)[1].split('\n    }\n', 1)[0]
    if 'nativeRequestWaitingForSettle = true' not in issue or 'nativeAnimationRunningSince != nil' not in issue:
        found.append('native minimize can reverse mid-animation (overshoot)')
    return found


def main():
    src = load()
    errors = problems(src)
    if errors:
        raise SystemExit('2.3 (TEST42 FIX3-FIX9) guards failed:\n- ' + '\n- '.join(errors))
    mutations = {
        'no pager reveal': ('Tweak.x', 'ALB43RevealVisibleProfilePages((UICollectionView *)self);', ''),
        'shared top path missing': ('Tweak.x', '        ALB50ApplyTopSharedBottomMaterial((UITabBar *)self);\n', ''),
        'no favorite record': ('ALBSidePalette.h',
                               'ALB43RecordPanelActionMiss(stack,AWEHomeActionKindFavorite,@"favorite");', ''),
        'no control handlers': ('Tweak.x', '    if (ALB46SendControlEventHandlers(sourceView, context)) return YES;\n', ''),
        'no panel touch': ('Tweak.x', '    if ([context hasPrefix:@"RIGHT_"] && ALB47DeliverPanelTap(sourceView, context)) return YES;\n', ''),
        'reverse mid-animation': ('AwemeAlbumMorphBar.swift', '            nativeRequestWaitingForSettle = true\n', ''),
    }
    for label, (name, old, new) in mutations.items():
        broken = dict(src)
        broken[name] = src[name].replace(old, new, 1)
        if broken[name] == src[name] or not problems(broken):
            raise SystemExit('Self-test failed to reject: ' + label)
    print(f'2.3 (TEST42 FIX3-FIX9) targeted repair guards passed ({len(mutations)} rejected mutations); no SDK/device claim.')


if __name__ == '__main__':
    main()
