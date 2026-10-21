#!/usr/bin/env python3
"""TEST42 FIX2 READBACK: source contract for carrier-derived native state.

ALBNativeCarrierState.m is read-only: it derives the native minimized state
from resident carrier visibility and observes _setMinimized: only to record
the last requested direction, forwarding the unchanged call. It writes no
UIKit state (the Placement-A safe-area experiment is removed) and never
forces layout. Not an iOS compile or device test.
"""
from pathlib import Path
import re
from check_logos_boundaries import mask_literals

ROOT = Path(__file__).resolve().parents[1]
SOURCE = 'ALBNativeCarrierState.m'
COMMENTS = re.compile(r'/\*[\s\S]*?\*/|//[^\n]*')
FORBIDDEN = [
    (r'\.(?:frame|bounds|center|transform|alpha|hidden|opacity|mask|cornerRadius|layoutMargins|'
     r'directionalLayoutMargins|additionalSafeAreaInsets|userInteractionEnabled|tabBarMinimizeBehavior|'
     r'selectedItem|items|effect|image|sublayerTransform|insetsLayoutMarginsFromSafeArea)\s*=(?!=)',
     'forbidden property write'),
    (r'\bset(?:Frame|Bounds|Center|Transform|Alpha|Hidden|Opacity|Mask|CornerRadius|NeedsLayout|'
     r'NeedsDisplay|Value|LayoutMargins|DirectionalLayoutMargins|AdditionalSafeAreaInsets)\b', 'setter call'),
    (r'\b(?:layoutIfNeeded|layoutSubviews|addSubview|insertSubview|removeFromSuperview|bringSubviewToFront|'
     r'addGestureRecognizer|removeGestureRecognizer|object_setIvar|method_setImplementation|'
     r'method_exchangeImplementations|performSelector|NSInvocation|MSHookMessageEx|sendEvent|'
     r'postNotificationName|animateWithDuration|performWithoutAnimation|CADisplayLink|NSUserDefaults|'
     r'additionalSafeAreaInsets)\b', 'mutation / animation / experiment API'),
    (r'placementA|nativePlacementA', 'Placement-A remnant'),
]
PASS_THROUGH = [
    '((void (*)(id, SEL, T))original)(receiver, selector, value); })',
    '((void (*)(id, SEL, T, U))original)(receiver, selector, value, animated); })',
]

def problems(source: str) -> list[str]:
    found = []
    code = mask_literals(source)
    uncommented = COMMENTS.sub(' ', source)
    for pattern, label in FORBIDDEN:
        match = re.search(pattern, code if label != 'Placement-A remnant' else uncommented)
        if match:
            found.append(f'{label}: {match.group(0)!r}')
    if 'objc_msgSend' in code:
        found.append('objc_msgSend is not needed by the readback')
    if len(re.findall(r'class_replaceMethod\(', code)) != 1 or 'class_replaceMethod(UITabBar.class, selector' not in uncommented:
        found.append('only UITabBar setter observers may be installed')
    observed = set(re.findall(r'ALBCarrierStateObserveSetter\(@"([^"]+)"\)', uncommented))
    if observed != {'_setMinimized:', '_setMinimized:animated:'}:
        found.append(f'observed setters must be exactly _setMinimized: / _setMinimized:animated:, got {sorted(observed)}')
    for call in PASS_THROUGH:
        if call not in uncommented:
            found.append('missing unchanged pass-through call: ' + call)
    will = uncommented[uncommented.find('static void ALBCarrierStateWillSetMinimized('):]
    will = will[:will.find('\n}\n')]
    body = [line.strip() for line in will.splitlines()[1:] if line.strip()]
    if body != ['if (!NSThread.isMainThread || !ALBCarrierStateIsOurBar(receiver)) return;',
                'gALBCarrierStateLastSetterBar = (UITabBar *)receiver;',
                'gALBCarrierStateLastSetterValue = value;',
                'gALBCarrierStateLastSetterTime = CACurrentMediaTime();']:
        found.append('setter observer may only record the last setter bar/value/time')
    if 'int32_t ALBNativeCarrierMinimizedState(UITabBar *bar) {' not in uncommented:
        found.append('ALBNativeCarrierMinimizedState missing')
    return found


def main() -> None:
    source = (ROOT / SOURCE).read_text()
    errors = problems(source)
    tweak = COMMENTS.sub(' ', (ROOT / 'Tweak.x').read_text())
    state = tweak[tweak.find('int32_t ALB15NativeMinimizedState(UIViewController *controller) {'):]
    state = state[:state.find('\n}\n')]
    if state.find('ALBNativeCarrierMinimizedState(owner.tabBar)') < 0 or \
            state.find('ALBNativeCarrierMinimizedState(owner.tabBar)') > state.find('_isMinimized'):
        errors.append('carrier-derived state must be consulted first by ALB15NativeMinimizedState')
    for name in ('ALB5Settings.h', 'Tweak.x', 'AwemeAlbumMorphBar.swift'):
        if re.search(r'nativePlacementA|placementA|ALBNativePlacement', (ROOT / name).read_text()):
            errors.append('Placement-A remnant in ' + name)
    if (ROOT / 'ALBNativePlacement.m').exists():
        errors.append('ALBNativePlacement.m must be removed')
    makefile = (ROOT / 'Makefile').read_text()
    files = re.search(r'^AwemeLiquidBar_FILES\s*=\s*(.+)$', makefile, re.M).group(1).split()
    if SOURCE not in files or 'ALBNativePlacement.m' in files:
        errors.append('readback source must be compiled and the experiment source must not')
    workflow = (ROOT / '.github/workflows/build.yml').read_text()
    if not re.search(r'for SOURCE in [^\n]*\bALBNativeCarrierState\.m\b[^\n]*; do', workflow):
        errors.append('readback source missing from the iOS 15 availability gate')
    if errors:
        raise SystemExit('Native state readback contract failed:\n- ' + '\n- '.join(errors))

    anchor = '    gALBCarrierStateLastSetterTime = CACurrentMediaTime();\n'
    broken = {
        'safe-area write': source.replace(anchor, anchor + '    ((UITabBarController *)nil).additionalSafeAreaInsets = UIEdgeInsetsZero;\n'),
        'tab bar alpha write': source.replace(anchor, anchor + '    ((UIView *)receiver).alpha = 1;\n'),
        'forced layout': source.replace(anchor, anchor + '    [(UIView *)receiver layoutIfNeeded];\n'),
        'extra observer work': source.replace(anchor, anchor + '    gALBCarrierStateSettled = -1;\n'),
        'changed argument': source.replace('original)(receiver, selector, value, animated); })',
                                           'original)(receiver, selector, value, NO); })'),
        'display link': source.replace(anchor, anchor + '    (void)[CADisplayLink class];\n'),
        'setting gate': source.replace(anchor, anchor + '    (void)NSUserDefaults.standardUserDefaults;\n'),
        'experiment event': source.replace('bottom.nativestate.readback', 'bottom.placementA.readback'),
    }
    for label, text in broken.items():
        if text == source:
            raise SystemExit('Self-test mutation did not apply: ' + label)
        if not problems(text):
            raise SystemExit('Self-test failed to reject: ' + label)
    print(f'READBACK: read-only carrier state contract passed ({len(broken)} rejected mutations); '
          'no SDK/device claim.')


if __name__ == '__main__':
    main()
