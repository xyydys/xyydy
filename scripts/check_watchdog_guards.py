#!/usr/bin/env python3
"""Source checks plus extracted C control-flow tests. No UIKit/device emulation."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
comment = (root/'CommentGlassDYKiller.xm').read_text()
tweak = (root/'Tweak.x').read_text()
contrast = (root/'ALBClearContrast.h').read_text()


def require(ok, message):
    if not ok:
        raise SystemExit('TEST18 watchdog guard: '+message)


def code_only(text):
    pattern = r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: ' '*len(m.group()), text)


def body(text, marker):
    start = text.index('{', text.index(marker))
    masked = code_only(text)
    depth = 0
    for i in range(start, len(text)):
        if masked[i] == '{':
            depth += 1
        elif masked[i] == '}':
            depth -= 1
            if depth == 0:
                return text[start+1:i]
    raise SystemExit('unclosed source block: '+marker)


view_hook = comment[comment.index('%hook UIView\n'):]
view_hook = view_hook[:view_hook.index('%end')]
view_code = code_only(view_hook)
require('layoutSubviews' not in view_code, 'global UIView subtree scan returned')
require('AWECommentPlayerControllerForView' not in view_code, 'responder ancestry lookup in global geometry')
require('frame.origin.x >' not in view_code, 'player-wide positive-X veto returned')
frame = body(view_hook, '- (void)setFrame:')
require(frame.index('!gAWECommentVisible') < frame.index('objc_getAssociatedObject'), 'closed-comment fast path missing')
require(frame.index('NSValue *saved') < frame.index('AWECommentMediaGuardEnabled()'), 'ordinary views still query preferences')
for name in ('setFrame:', 'setBounds:', 'setCenter:', 'setTransform:'):
    method = body(view_hook, '- (void)'+name)
    require('UIApplicationStateActive' in method and '%orig(' in method,
            'missing active-state gate/original call: '+name)
scan = body(comment[comment.index('%hook AWEPlayInteractionViewController'):], '- (void)viewDidLayoutSubviews')
require('queue.count >= 96' in scan and 'i < 96' in scan and 'view.superview == root' in scan,
        'top-strip scan is not bounded')
snap = body(contrast, 'static UIImage *ALB8ContrastSnapshot(')
require(snap.index('queued = YES') < snap.index('dispatch_async') < snap.index('AWECreateLowResolutionBackdropSnapshot()'),
        'contrast capture is synchronous or uncoalesced')
require('@finally' in snap and 'queued = NO' in snap, 'snapshot queue lock leaks')
render = body(tweak, 'static UIImage *AWECreateLowResolutionBackdropSnapshot(void) {')
require('captureInProgress' in render and '@finally' in render and 'UIApplicationStateActive' in render,
        'snapshot capture reentry/activity barrier missing')
stack = tweak[tweak.index('%hook AWEElementStackView\n'):]
stack = stack[:stack.index('%end')]
for name in ('setFrame:', 'setBounds:'):
    require('AWELayoutBoundHomeActionSlots' not in body(stack, '- (void)'+name),
            'synchronous slot layout from native geometry setter: '+name)
layout = body(stack, '- (void)layoutSubviews')
require(layout.index('layoutInProgressKey') < layout.index('%orig') and '@finally' in layout,
        'right-stack reentry guard missing/late')
require('else if (!artwork && stack.subviews.firstObject != plate)' in tweak,
        'plate reordered on every layout pass')
audit = json.loads((root/'validation/test29_preservation_audit.json').read_text())
audit['expected_unchanged_sha256'].update(json.loads((root/'validation/material2_changes.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((root/'validation/auth51_changes.json').read_text())['intentional_changes'])
for name, expected in audit['expected_unchanged_sha256'].items():
    require(hashlib.sha256((root/name).read_bytes()).hexdigest() == expected, 'preserved file changed: '+name)

# Copy the production branch and pure rectangle predicate; substitute only the
# Objective-C environment/logging primitives with deterministic C stubs.
translated = code_only(frame)
translated = translated.replace('NSThread.isMainThread', 'is_main')
translated = translated.replace('UIApplication.sharedApplication.applicationState', 'app_state')
translated = translated.replace('NSValue *saved = objc_getAssociatedObject(self, &kAWECommentMediaBaselineFrameKey);',
                                'const Snapshot *saved = lookup_snapshot();')
translated = translated.replace('saved.CGRectValue', 'saved->frame')
translated = re.sub(r'AWECommentLogMediaGuard\([\s\S]*?;', '', translated)
translated = translated.replace('%orig(', 'original(')
predicate = body(comment, 'static BOOL AWECommentRectChangeLooksLikePanelShrink(')
fixture = r'''
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef bool BOOL;
typedef double CGFloat;
typedef struct { double x,y; } CGPoint;
typedef struct { double width,height; } CGSize;
typedef struct { CGPoint origin; CGSize size; } CGRect;
typedef struct { CGRect frame; } Snapshot;
#define NO false
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CGRectGetWidth(r) ((r).size.width)
#define CGRectGetHeight(r) ((r).size.height)
#define CGRectGetMinX(r) ((r).origin.x)
#define CGRectGetMinY(r) ((r).origin.y)
#define CGRectIsEmpty(r) ((r).size.width<=0 || (r).size.height<=0)
#define CGRectIsNull(r) (!isfinite((r).origin.x))
enum { UIApplicationStateActive=0, Inactive=1, Background=2 };
static bool gAWECommentVisible, is_main, has_snapshot, guard_enabled;
static int app_state, original_calls, lookup_calls, preference_calls;
static CGRect delivered;
static Snapshot baseline;
static const Snapshot *lookup_snapshot(void) { lookup_calls++; return has_snapshot?&baseline:NULL; }
static BOOL AWECommentMediaGuardEnabled(void) { preference_calls++; return guard_enabled; }
static void original(CGRect r) { original_calls++; delivered=r; }
static BOOL AWECommentRectChangeLooksLikePanelShrink(CGRect baseline, CGRect proposed) {
PREDICATE
}
static void frame_hook(CGRect frame) {
FRAME_BODY
}
static bool eq(CGRect a, CGRect b) { return memcmp(&a,&b,sizeof(a))==0; }
int main(void) {
    baseline.frame=(CGRect){{0,0},{390,844}};
    CGRect proposed={{24,20},{60,60}}; // Native glass subview with positive X.
    unsigned cases=0;
    for (int visible=0;visible<2;visible++)
    for (int main_thread=0;main_thread<2;main_thread++)
    for (int state=0;state<3;state++)
    for (int saved=0;saved<2;saved++)
    for (int enabled=0;enabled<2;enabled++) {
        gAWECommentVisible=visible; is_main=main_thread; app_state=state;
        has_snapshot=saved; guard_enabled=enabled;
        original_calls=lookup_calls=preference_calls=0;
        frame_hook(proposed);
        assert(original_calls==1);
        bool in_scope=visible && main_thread && state==UIApplicationStateActive;
        assert(lookup_calls==(in_scope?1:0));
        assert(preference_calls==((in_scope && saved)?1:0));
        assert(eq(delivered,(in_scope && saved && enabled)?baseline.frame:proposed));
        cases++;
    }
    gAWECommentVisible=true; is_main=true; app_state=UIApplicationStateActive;
    has_snapshot=true; guard_enabled=true; original_calls=0;
    CGRect ordinary={{2,4},{392,846}};
    frame_hook(ordinary); assert(original_calls==1 && eq(delivered,ordinary));
    printf("%u extracted frame-gate cases + no-shrink pass-through passed; no UIKit used.\n",cases);
}
'''.replace('PREDICATE', predicate).replace('FRAME_BODY', translated)
clang = shutil.which('clang') or shutil.which('cc')
require(clang, 'a C compiler is required for extracted control-flow checks')
with tempfile.TemporaryDirectory(prefix='alb18-frame-') as tmp:
    src=Path(tmp)/'frame.c'; exe=Path(tmp)/'frame'
    src.write_text(fixture)
    subprocess.run([clang,'-std=c11','-Wall','-Wextra','-Werror',str(src),'-lm','-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('TEST18 source scope / scheduling / preservation checks passed. Not an iOS build or device test.')
