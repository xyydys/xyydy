#!/usr/bin/env python3
"""Production policy tests and integration/ownership guards; no UIKit rendering claim."""
from pathlib import Path
import shutil, subprocess, tempfile
from check_logos_boundaries import mask_literals
r = Path(__file__).resolve().parents[1]
t = (r/'Tweak.x').read_text(); m = (r/'ALBUnifiedMaterial.m').read_text()
sw = (r/'AwemeAlbumMorphBar.swift').read_text()
def body(source, signature):
    start = source.index('{', source.index(signature)); code = mask_literals(source); depth = 0
    for end in range(start, len(source)):
        depth += (code[end] == '{') - (code[end] == '}')
        if depth == 0: return source[start+1:end]
    raise AssertionError(signature)
def require(ok, message):
    if not ok: raise SystemExit('material: '+message)
require('ALBUnifiedMaterial.m' in (r/'Makefile').read_text(), 'central owner is not built')
require('ALBMaterialApplyBar(nativeBar, YES);' in t, 'bottom policy missing')
require('ALBMaterialApplyBar(bar, NO);' in t, 'top policy missing')
require('ALBMaterialStyle(container, YES)' in t, 'top uses stale bottom traits')
require('ALBMaterialApplyButton(button, feedMaterial)' in t, 'side buttons do not share policy')
require('ALBBottomMaterialMode' not in sw, 'duplicate Swift material writer returned')
require('glassButtonConfiguration' in m and 'clearGlassButtonConfiguration' in m, 'native button styles removed')
button = mask_literals(body(m, 'void ALBMaterialApplyButton('))
for token in ['addTarget:', 'removeTarget:', '.menu', 'hitTest:', 'addGestureRecognizer:', 'removeFromSuperview', '.transform', '.frame']:
    require(token not in button, 'material changes button interaction/geometry: '+token)
effect = body(m, 'UIVisualEffect *ALBMaterialMakeEffect(')
require('ALBMaterialRegular ? 0 : 1' in effect, 'Regular/Clear recipe differs from lab')
require('UIAccessibilityIsReduceTransparencyEnabled' not in effect, 'accessibility silently switches glass to blur')
require('ALBMaterialMode() != ALBMaterialRegular' in body(m, 'static UIColor *ALBMaterialTint('), 'Clear gained a tint')
require('[layer.delegate isKindOfClass:UIView.class]' in m and 'ALBIsLens' in m, 'view/lens backing is not excluded')
require('fallback.userInteractionEnabled = NO' in m and 'fallback.contentView.userInteractionEnabled = NO' in m, 'renderer intercepts taps')
require('fallback.frame = carrier.bounds' in m, 'renderer uses scaled window coordinates')
require('unresolved-native-renderer' in m and 'native26-clear-effect-fallback' in m, 'missing coverage is reported as success')
require('ALBMaterialSnapshot()' in t, 'reproduction export lacks actual coverage')
require('performAsCurrentTraitCollection:' in m, 'material factory can inherit stale application traits')
issue = body(sw, 'private func issueNativeRequest(animated: Bool)')
wait = body(sw, 'private func waitForNativeTransitionToSettle()')
require('nativeAnimationRunningSince != nil' in issue and 'nativeRequestWaitingForSettle = true' in issue, 'reversal can enter mid-spring')
require('CATransaction' not in mask_literals(issue) and 'layoutIfNeeded' not in wait and 'settleNativeLayoutAfterAnimation' not in sw, 'forced completion layout returned')
require('ALBShouldWaitForNativeSettle' in wait and 'hasNativeGeometryAnimation' in wait, 'native completion does not observe actual motion')
for file in ['ALBUnifiedMaterial.m','ALBUnifiedMaterial.h','ALBMaterialPolicy.h','ALBTopMaterial.h']:
    code = mask_literals((r/file).read_text()); stack = []; pairs = {')':'(',']':'[','}':'{'}
    for char in code:
        if char in '([{': stack.append(char)
        elif char in pairs: require(bool(stack) and stack.pop() == pairs[char], 'unbalanced '+file)
    require(not stack, 'unclosed '+file)
cc = shutil.which('clang') or shutil.which('cc')
if not cc: raise SystemExit('A C compiler is required for material policy tests')
with tempfile.TemporaryDirectory(prefix='alb-material-') as folder:
    binary = Path(folder)/'policy'
    subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',str(r/'Tests/material_policy_test.c'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
print('Unified material ownership/coverage/transition guards passed; UIKit and device behavior unverified.')
