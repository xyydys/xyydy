#!/usr/bin/env python3
"""TEST22 source contracts plus execution of extracted tap-gating predicates.
No UIKit event delivery/rendering or private selector execution is simulated.
"""
from pathlib import Path
import hashlib,json,re,shutil,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
T=(ROOT/'Tweak.x').read_text()
TOP=(ROOT/'ALBTopNative/ALBTopNativeGroup.m').read_text()
E=(ROOT/'ALB22NativeEndpoint.h').read_text()

def require(ok,message):
    if not ok: raise SystemExit('TEST22: '+message)

def mask(text):
    return re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',lambda m:' '*len(m.group()),text)

def body(text,marker):
    pos=text.index('{',text.index(marker));m=mask(text);depth=0
    for end in range(pos,len(m)):
        depth+=(m[end]=='{')-(m[end]=='}')
        if depth==0:return text[pos+1:end]
    raise RuntimeError('Missing end of '+marker)

start=T.index('static void AWEConfigureTopSlot(', T.index('static BOOL AWETopSourceIsNearExpectedPoint('))
config=body(T[start:],'static void AWEConfigureTopSlot(')
slot=T[T.index('@implementation AWEHomeTopSlotView'):];slot=slot[:slot.index('@end')]
require('removeTarget:slot action:@selector(alb_handleTopTap:)' not in config,'native search receiver removed')
require('actionsForTarget:slot' in config and 'addTarget:slot action:@selector(alb_handleTopTap:)' in config,'missing/doubled search receiver binding')
require('AWEHideTopOwnerVisual(slot, owner, container);' in config,'asymmetric original search background returned')
require('AWERestoreTopOwnerAppearanceOnly' not in config,'search source backdrop leaks through')
require('AWELeaseSharedSearchGestureToView(' not in config+slot,'top moves original alpha recognizer onto native carrier again')
require('AWEForwardTopActionOwner(owner, self.topKind)' in slot,'original search business forwarding missing')
require('configureNativeImage:image' in config and 'gAWEAlbumMorphLastAppliedStyle' in config,'native material/appearance route missing')
require('top.sideSelectionCapsule' in config and 'topSideCapsuleChanged:' in T,'capsule setting is not wired')
require('top.sideSelectionCapsule' in (ROOT/'ALB5Settings.h').read_text(),'advanced settings missing capsule toggle')
require('_keepsSingleSelectionCapsule = YES;' in TOP,'new capsule default changed existing look')
wrapper=TOP[TOP.index('@implementation ALBTopNativeIconButton'):]
require('keepsSingleSelectionCapsule' not in body(wrapper,'- (void)configureNativeImage:'),'reconfiguration overwrites capsule preference')
require('canPreventGestureRecognizer:(UIGestureRecognizer *)other { return NO; }' in TOP and
        'canBePreventedByGestureRecognizer:(UIGestureRecognizer *)other { return NO; }' in TOP,'passive observer starts competing with native recognizers')
require('self.cancelsTouchesInView = NO;' in TOP and 'self.delaysTouchesBegan = NO;' in TOP,'observer cancels or delays native press')
require('[weakSelf restoreIdleActionLenses];' in TOP,'press does not restore saved native lens')
lens=body(TOP,'- (void)updateIdleActionLensVisibility {')
for token in ('i < 128','j < 48','0.25','hasRealContent','self.isSelectionTracking'):
    require(token in lens,'lens path guard missing '+token)
for token in ('setSelectedTab:','selectedItem =','cornerRadius','CGAffineTransform','UIVisualEffectView'):
    require(token not in lens,'lens switch changes native selection/material/geometry '+token)
require('view.layer.opacity != 0' in lens,'lens opacity is rewritten on every scan')
require('object_getIvar' in E and 'valueForKey:' not in E,'provider lookup lazily invokes KVC')
require('targets.count >= 8' in E and 'depth < 4' in E and 'emitted < 12' in E,'endpoint inspection is unbounded')
request=body(T,'int32_t AWEAlbumMorphRequestNativeMinimized(UIViewController *controller,int32_t value,int32_t animation) {')
for token in ('requestInProgress','@finally','ALB22OwnedMinimizeTargets','ALB22RememberNativeEndpoint'):
    require(token in request,'request guard/route missing '+token)
for token in ('layoutIfNeeded','setContentOffset','dispatch_after','setTabs','animateWithDuration'):
    require(token not in request,'native request has new geometry/loop side effect '+token)
invoke=body(T,'static BOOL AWEInvokeAlbumMorphBoolAnimatedSelector(')
require('ALB22BoolSetterSignature' in invoke,'new provider selectors are not signature-checked')
require('AWEAlbumMorphRequestNativeMinimized(self,' in (ROOT/'AwemeAlbumMorphBar.swift').read_text(),'request not wired to Swift')
# Preserve material/geometry/auth rather than editing unrelated hot paths again.
audit=json.loads((ROOT/'validation/test22_preservation_audit.json').read_text())
for name,sha in audit['expected_unchanged_sha256'].items():
    require(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==sha,'changed stable baseline '+name)
auth_start=T.index('#pragma mark - Local account authorization gate');auth_end=T.index('#pragma mark - Transition diagnostics')
require(hashlib.sha256(T[auth_start:auth_end].encode()).hexdigest()=='a9aa5a8d5e08723a6f034372a1937658c99bc1f023f1773387c21efad42c013f','authorization changed')
# Execute the exact production expressions, substituting only view/event storage.
finish=body(TOP,'- (void)finishTouches:')
expression=re.search(r'BOOL tap = (.*?);',finish,re.S).group(1)
expression=expression.replace('self.maximumTravel','travel').replace('CGRectContainsPoint(self.view.bounds, self.touchPosition)','inside')
gate=body(TOP,'- (void)dispatchNativeActionForSequence:(NSUInteger)sequence {')
gate=mask(gate).replace('self.actionOnly','actionOnly').replace('self.enabled','enabled').replace('self.hidden','hidden').replace('self.window','window').replace('self.lastActionSequence','last')
gate=gate.replace('if (self.onSelection) self.onSelection(0);','calls++;')
require('self.' not in gate,'untranslated production action-gating expression')
c=r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
static unsigned calls; static unsigned long last;
static bool actionOnly=true,enabled=true,hidden=false,window=true;
static bool isTap(bool cancelled,bool single,double travel,double duration,bool inside) {
    return EXPRESSION;
}
static void deliver(unsigned long sequence) { GATE }
int main(void){
    assert(isTap(false,true,0,.1,true));assert(isTap(false,true,12,.65,true));
    assert(!isTap(true,true,0,.1,true));assert(!isTap(false,false,0,.1,true));
    assert(!isTap(false,true,12.01,.1,true));assert(!isTap(false,true,0,.651,true));
    assert(!isTap(false,true,0,-.1,true));assert(!isTap(false,true,0,.1,false));
    // Native delegate then observer, and observer then delegate: one delivery.
    deliver(1);deliver(1);assert(calls==1);deliver(2);deliver(2);assert(calls==2);
    // Accessibility has no physical sequence, and remains repeatable.
    deliver(0);deliver(0);assert(calls==4);
    hidden=true;deliver(3);assert(calls==4);hidden=false;
    enabled=false;deliver(3);assert(calls==4);enabled=true;
    window=false;deliver(3);assert(calls==4);window=true;
    actionOnly=false;deliver(3);assert(calls==4);actionOnly=true;
    deliver(3);assert(calls==5);
    puts("16 extracted tap/action gate scenarios passed; no UIKit events simulated.");
    return 0;
}
'''.replace('EXPRESSION',expression).replace('GATE',gate)
with tempfile.TemporaryDirectory(prefix='alb22-tap-') as d:
    d=Path(d);f=d/'test.c';f.write_text(c)
    subprocess.run([shutil.which('clang') or 'cc','-std=c11','-Wall','-Wextra','-Werror',str(f),'-o',str(d/'test')],check=True,timeout=20)
    subprocess.run([str(d/'test')],check=True,timeout=10)
print('TEST22 source contracts and preserved baseline hashes passed; not an iOS build or device validation.')
