#!/usr/bin/env python3
"""Check the production persistent-layout path. No UIKit runtime is simulated."""
from pathlib import Path
import hashlib, json, re
root=Path(__file__).resolve().parents[1]
h=(root/'ALB5RightLayout.h').read_text(); t=(root/'Tweak.x').read_text()

def require(ok,message):
    if not ok: raise SystemExit('TEST19 right persistence: '+message)

def body(text,marker):
    start=text.index('{',text.index(marker))
    masked=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',lambda m:' '*len(m.group()),text)
    depth=0
    for i in range(start,len(text)):
        if masked[i]=='{': depth+=1
        elif masked[i]=='}':
            depth-=1
            if not depth: return text[start+1:i]
    raise SystemExit('unbalanced body '+marker)

begin=body(h,'static void ALB6BeginRightGeometry(UIView *stack) {')
local=body(h,'static void ALB10RightLocalLayout(')
apply=body(h,'static void ALB5ApplyRightLayout(UIView *stack) {')
write=body(h,'static void ALB19RightWriteTransform(')
require('ALB5RestoreRightLayout(' not in begin and 'CATransaction' not in begin,
        'outer layout still restores transforms or changes native animation transactions')
require('ALB9RightWriteGeometry' not in local and 'layout();' in local,
        'button layout still performs a transform reset')
require('ALB19RightTransformsDiffer(view.layer.affineTransform,transform)' in write,
        'identical output still reaches the native transform setter')
require('ALB6RightEdit *record=objc_getAssociatedObject(v,&kALB8RightTransformKey);' in apply,
        'existing presentation lease not reused')
require(apply.index('ALB19RightConvertRect(v,v.bounds,v.superview,stack)') < apply.index('record.shift=shift'),
        'new map overwrites old map before recovering native inputs')
require('if (!requested) { ALB5RestoreRightLayout(stack); return; }' in apply,
        'resetting settings no longer restores native state')
require('if ([edits containsObject:old]) continue;' in apply,
        'stable roles are being restored along with stale roles')
for method in ('Frame','Bounds','Center'):
    require(t.count('if (ALB19RightSkipNative'+method+'((UIView *)self,')==2,
            'missing native no-op guard '+method)
require('UIApplication.sharedApplication.applicationState!=UIApplicationStateActive' in apply,
        'inactive scene still processes plugin reflow')
point=body(h,'static CGPoint ALB19RightConvertPoint(')
require('ALB19RightUnmapPoint' in point and 'ALB6RightMapPoint' in point,
        'source/destination projection conversion is incomplete')
require('.transform=' not in point and 'ALB5RestoreRightLayout' not in point,
        'coordinate read writes view transforms')
for name in ('ALB5RightLayout.h','ALBNativeInteraction.h'):
    masked=re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',lambda m:' '*len(m.group()),(root/name).read_text())
    stack=[]; pairs={')':'(',']':'[','}':'{'}
    for c in masked:
        if c in '([{': stack.append(c)
        elif c in ')]}': require(bool(stack) and stack.pop()==pairs[c],'delimiter mismatch '+name)
    require(not stack,'unclosed delimiter '+name)
# Reverse only the four intentional geometry edits in this header; every
# native touch handler, clipping lease and material/configuration body must
# still reconstruct the exact uploaded TEST18 source.
i=(root/'ALBNativeInteraction.h').read_text()
# TEST28's message-only inset proxy stays in the navigation bar so its visible
# and tappable rectangles coincide. Undo exactly this opt-in before retaining
# the existing byte-for-byte right-column touch/material contract below.
message_only = [
 ('    if (self.receivingOwnTouch && [objc_getAssociatedObject(self.button,\n'
  '        NSSelectorFromString(@"ALB28.KeepProxyInParent")) boolValue]) return YES;\n', ''),
 ('        BOOL keepProxyInParent = [objc_getAssociatedObject(button,\n'
  '            NSSelectorFromString(@"ALB28.KeepProxyInParent")) boolValue];\n', ''),
 ('if (!keepProxyInParent && !bridge.rightColumn &&', 'if (!bridge.rightColumn &&'),
]
for current, previous in message_only:
    require(i.count(current)==1, 'message inset opt-in changed beyond its reviewed scope')
    i=i.replace(current,previous)
message=(root/'MessageTopGlassV3.xm').read_text()
require('objc_setAssociatedObject(proxy, NSSelectorFromString(@"ALB28.KeepProxyInParent"), @YES,' in message,
        'message proxy does not opt into navigation-owned hit geometry')
replacements={
 'ALB19RightSetBounds(button,bounds);':'if (!CGRectEqualToRect(button.bounds,bounds)) button.bounds=bounds;',
 'ALB19RightSetCenter(button,center);':'if (!CGPointEqualToPoint(button.center,center)) button.center=center;',
 'CGRect frame = host == slot ? slot.bounds :\n        ALB19RightConvertRect(slot,slot.bounds,host,slot.superview);':'CGRect frame = host == slot ? slot.bounds : [slot convertRect:slot.bounds toView:host];',
 'CGRect local = ALB19RightConvertRect(coordinates,frame,parent,coordinates);':'CGRect local = [coordinates convertRect:frame toView:parent];',
}
for a,b in replacements.items():
    require(i.count(a)==1,'unexpected interaction edit count')
    i=i.replace(a,b)
require(hashlib.sha256(i.encode()).hexdigest()=='3667b60fd60152987cc2421350a3f9b410050e211c5b69734acc2506be3c73ae',
        'native touch/material implementation changed outside four geometry helpers')
audit=json.loads((root/'validation/test29_preservation_audit.json').read_text())
audit['expected_unchanged_sha256'].update(json.loads((root/'validation/material2_changes.json').read_text())['intentional_changes'])
audit['expected_unchanged_sha256'].update(json.loads((root/'validation/auth51_changes.json').read_text())['intentional_changes'])
for name,expected in audit['expected_unchanged_sha256'].items():
    require(hashlib.sha256((root/name).read_bytes()).hexdigest()==expected,'preserved file changed: '+name)
# TEST32 changes only tail-role selection and the shared count/name label.
# Reverse that exact center helper replacement before removing the three
# TEST29 count-anchor additions. The final TEST28 FIX1 hash is unchanged.
current_center='''static CGPoint ALB6RightCountCenter(UIView *share,UIView *stack) {
    return ALB6RightCenter(ALB32RightTailLabel(share,stack) ?: share,stack);
}
'''
previous_center='''static CGPoint ALB6RightCountCenter(UIView *share,UIView *stack) {
    CGPoint result=ALB6RightCenter(share,stack);
    CGFloat bottom=-CGFLOAT_MAX;
    NSMutableArray<UIView *> *queue=[NSMutableArray arrayWithObject:share];
    for (NSUInteger i=0;i<queue.count && i<60;i++) {
        UIView *v=queue[i];
        if (ALBIsOwnedView(v) || v.hidden) continue;
        if ([v isKindOfClass:UILabel.class]) {
            UILabel *label=(UILabel *)v;
            NSString *text=label.attributedText.string ?: label.text;
            if ([text rangeOfCharacterFromSet:NSCharacterSet.decimalDigitCharacterSet].location!=NSNotFound && text.length) {
                CGPoint point=ALB6RightCenter(v,stack);
                if (point.y>bottom) { result=point; bottom=point.y; }
            }
        }
        [queue addObjectsFromArray:v.subviews];
    }
    return result;
}
'''
require(h.count(current_center)==1,'count-center helper changed beyond shared label lookup')
tail_label=body(h,'static UILabel *ALB32RightTailLabel(')
require('ALB7RightNativeAlpha(view)<=.001' in tail_label and
        'text.length && CGRectGetWidth(label.bounds)>0 && CGRectGetHeight(label.bounds)>0' in tail_label,
        'tail label lookup no longer handles native visibility and zero-count names')
require('ALB6RightNativeVisible(anchors[role] ?: roots[role],stack)' in apply and
        'ALB32RightLastActionIndex((unsigned)order.count,actionMask,hiddenMask,nativeInvisibleMask)' in apply,
        'hidden-share tail no longer accounts for native-hidden rows')
restored=h
restored=restored.replace(current_center,previous_center)
for first,last in [
    ('// Share is the visual bottom reference', '// TEST23:'),
    ('static UILabel *ALB32RightTailLabel(', 'static CGAffineTransform ALB6RightViewTransform('),
    ('        // Fix only the hidden-Share tail.', '        NSArray<ALB6RightEdit *> *previous=')]:
    begin=restored.index(first); end=restored.index(last,begin)
    restored=restored[:begin]+restored[end:]
# TEST38 intentionally extends the presentation map with an X offset and makes
# music part of the common rail scale. The exact old-header hash therefore no
# longer applies; interaction/material implementation hashes above remain exact.
require('ALB6RightMapPointOffset' in h and 'ALB19RightUnmapPointOffset' in h,
        'music alignment offset is not represented in the persistent projection')
require('record.xShift=xShift;record.shift=shift;record.scale=scale' in h,
        'projection lease does not retain the horizontal alignment offset')
require('CGFloat scale=columnScale;' in apply and 'CGPoint rolePivot=pivot;' in apply,
        'music is not part of the common whole-rail scale')
require('right.musicScale' not in apply and 'right.shareMusicGap' not in apply and 'right.musicGap' in apply,
        'legacy music parameters remain active or current fixed-anchor gap is missing')
require('shifts[@"music"]=@0;' in apply and 'ALB42RightUpperGapShift(musicTop,rowBottom,musicGap,columnScale)' in apply,
        'gap control no longer preserves music position')
print('TEST42 right persistence / material / touch-handler source checks passed; not an iOS build or device test.')
