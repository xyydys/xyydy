#!/usr/bin/env python3
"""Source invariants only. Passing is not an iOS compile or rendering test."""
from pathlib import Path
import hashlib, re
root=Path(__file__).resolve().parents[1]
def require(ok,message):
    if not ok: raise SystemExit('native contract: '+message)
def uncomment(text):
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', lambda m:' ' if m.group().startswith(('/', '"', "'")) else m.group(),text)
import json
expected=json.loads((root/'validation/test29_preservation_audit.json').read_text())['expected_unchanged_sha256']
expected.update(json.loads((root/'validation/material2_changes.json').read_text())['intentional_changes'])
expected.update(json.loads((root/'validation/auth51_changes.json').read_text())['intentional_changes'])
for name,wanted in expected.items():
    require(hashlib.sha256((root/name).read_bytes()).hexdigest()==wanted,'unchanged baseline differs: '+name)
t=(root/'Tweak.x').read_text();g=(root/'ALB5BottomGeometry.m').read_text();sw=(root/'AwemeAlbumMorphBar.swift').read_text()
start=t.index('#pragma mark - Local account authorization gate');end=t.index('#pragma mark - Transition diagnostics')
# auth51 explicitly adds one normal digest/tag and one special entitlement.
# The dedicated auth51 check verifies every previous row and the unchanged logic.
require(hashlib.sha256(t[start:end].encode()).hexdigest()==json.loads((root/'validation/auth51_changes.json').read_text())['authorization_core_after'],'authorization core changed')
# TEST33 explicitly shares the center-mode direction detector with native-left.
# Only the resident UIKit endpoint animates that mode; foreign scroll binding
# caused Favorites to jump to the top and is deliberately retired.
controller=sw[sw.index('final class AWEAlbumMorphTabController:'):]
require('AWEAlbumMorphRequestNativeMinimized(self,' in controller,'native intent is not dispatched')
request=controller[controller.index('private func prepareIndependentDriver('):controller.index('private func queueNativeRefresh(')]
require('.onScrollDown' in request and 'ALBBindMinimizeSources' not in request,'native mode rebinds an app list')
for token in ['CGAffineTransform','UIView.animate','setContentOffset','CADisplayLink','setTabs(']:
    require(token not in uncomment(request),'custom native-left geometry returned: '+token)
require('ALB15NativeMinimizedState(self)' in controller,'actual native readback missing')
require('compactTap' not in controller and 'expandCompactTab' not in controller,'cancelling compact tap returned')
configure=t[t.index('static void AWEConfigureTopSlot(',t.index('static BOOL AWETopSourceIsNearExpectedPoint(')):t.index('#pragma mark - Home top category visual suppression')]
# TEST42 retains the TEST9 UIButton implementation; only its material branch changes.
require('AWEConfigureAdaptiveSystemGlassIconButton(' in configure,'TEST9 native button configuration missing')
require('configureNativeImage:image' not in configure,'retired auxiliary-tab side carrier returned')
require('tintedGlass = topControl ? YES : NO;' not in t and
        'ALBMaterialMode() == ALBMaterialRegular' in t,
        'top material branch is not the scoped neutral-glass implementation')
for marker in ['@implementation AWEIOS27NativeBarHostView','@implementation AWEAlbumMorphOverlayWindow']:
    part=t[t.index(marker):t.index('@end',t.index(marker))]
    require('return gAWEAlbumMorphController.view' not in part,'host intercepts native press')
    require('nativeHit' in part,'native subtree hit route missing')
top=(root/'ALBTopNative/ALBTopNativeGroup.m').read_text()
require('ALB5ApplySharedBottomSurface(bar); ALBMaterialApplyBar(bar, NO);' in top,'shared material entry changed')
require('ALBTopNativeIconButton' not in top,'retired proxy menu carrier returned')
require('button.menu = menu;' in t and 'button.showsMenuAsPrimaryAction = YES;' in t,
        'UIKit UIButton no longer owns its menu')
require('performPrimaryAction' not in top,'manual primary-action dispatch returned')
require('TARGET = iphone:clang:latest:15.0' in (root/'Makefile').read_text(),
        'minimum deployment target changed')
# Basic lexical balance also catches accidental editing damage in Logos/ObjC.
for name in ['ALB22NativeEndpoint.h','ALB22NativeMethodPolicy.h','Tweak.x','ALB5BottomGeometry.m','ALBTopNative/ALBTopNativeGroup.m','ALB5Settings.h','ALBNativeMinimize.h','CommentGlassDYKiller.xm','ALBClearContrast.h','ALB5Options.h','ALBTopMaterial.h','ALBDanmakuGlass.h','ALBAvatarMenuTouch.h','ALBSettingsGlass.h','ALBSidePalette.h','ALBNativeInteraction.h','MessageTopGlassV3.xm']:
    code=uncomment((root/name).read_text()); stack=[];pairs={')':'(',']':'[','}':'{'}
    for c in code:
        if c in '([{': stack.append(c)
        elif c in ')]}':
            require(bool(stack) and stack.pop()==pairs[c],'unbalanced delimiters: '+name)
    require(not stack,'unclosed delimiters: '+name)
require('Version: 2.3' in (root/'control').read_text(),'package version mismatch')
require('SESSION_START build=2.3' in t,'diagnostic version mismatch')
require('LiquidBar · 2.3' in t,'settings version mismatch')
print('TEST42 native source contract passed (not an iOS compile/device test).')
