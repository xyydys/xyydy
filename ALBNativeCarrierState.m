// TEST42 FIX2 READBACK: carrier-derived native minimized state (read-only).
//
// -[UITabBar _isMinimized] is unavailable on the tested iOS 26.6 device, and
// the old fallbacks read "expanded" during the ~0.8 s native carrier overlap,
// which re-issued _setMinimized:YES. ALB15NativeMinimizedState consults
// ALBNativeCarrierMinimizedState first.
//
// Contract (enforced by scripts/check_native_state.py):
// - Writes no UIKit state at all: no frame/bounds/center/transform/hidden/
//   alpha, no safe-area insets, no layout.
// - The _setMinimized: observer only records the direction and time of the
//   last call, then calls the original once with the unchanged receiver,
//   selector and argument.
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <string.h>
#import "ALBReproCaptureSignal.h"
#include "ALB22NativeMethodPolicy.h"

static NSString *const kALBCarrierStateBarIdentifier = @"AwemeLiquidBar.AlbumMorph.SystemTabBar";
static const CFTimeInterval kALBCarrierStateRecentSetterSeconds = 1.5;
static const NSUInteger kALBCarrierStateLensScanBudget = 48;

#pragma mark - Read-only carrier state

static BOOL ALBCarrierStateIsOurBar(UIView *view) {
    return [view isKindOfClass:UITabBar.class] &&
        [view.accessibilityIdentifier isEqualToString:kALBCarrierStateBarIdentifier];
}
static BOOL ALBCarrierStateIsPlatter(UIView *view) {
    NSString *name = NSStringFromClass(object_getClass(view));
    return [name rangeOfString:@"TabBarPlatterView"].location != NSNotFound &&
        [name rangeOfString:@"ContentView"].location == NSNotFound;
}
static BOOL ALBCarrierStateHasLens(UIView *root) {
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:root.subviews];
    for (NSUInteger index = 0; index < queue.count && index < kALBCarrierStateLensScanBudget; ++index) {
        NSString *name = NSStringFromClass(object_getClass(queue[index]));
        if ([name isEqualToString:@"_UILiquidLensView"] || [name hasSuffix:@"._UILiquidLensView"]) return YES;
        [queue addObjectsFromArray:queue[index].subviews];
    }
    return NO;
}

static __weak UITabBar *gALBCarrierStateCarrierBar;
static __weak UIView *gALBCarrierStateExpandedCarrier;
static __weak UIView *gALBCarrierStateMinimizedCarrier;

// Device evidence (PROBE2): both primary carriers are resident direct
// children of UITabBar; only the expanded one hosts the LiquidLens.
static BOOL ALBCarrierStatePrimaryCarriers(UITabBar *bar, UIView **expanded, UIView **minimized) {
    UIView *cachedExpanded = gALBCarrierStateExpandedCarrier, *cachedMinimized = gALBCarrierStateMinimizedCarrier;
    if (gALBCarrierStateCarrierBar != bar || cachedExpanded.superview != bar || cachedMinimized.superview != bar) {
        cachedExpanded = nil; cachedMinimized = nil;
        for (UIView *child in bar.subviews) {
            if (!ALBCarrierStateIsPlatter(child)) continue;
            if (ALBCarrierStateHasLens(child)) { if (!cachedExpanded) cachedExpanded = child; }
            else if (!cachedMinimized) cachedMinimized = child;
        }
        gALBCarrierStateCarrierBar = bar;
        gALBCarrierStateExpandedCarrier = cachedExpanded;
        gALBCarrierStateMinimizedCarrier = cachedMinimized;
    }
    if (expanded) *expanded = cachedExpanded;
    if (minimized) *minimized = cachedMinimized;
    return cachedExpanded && cachedMinimized;
}
static BOOL ALBCarrierStateVisible(UIView *view) {
    return view && !view.hidden && view.alpha > 0.01;
}

static __weak UITabBar *gALBCarrierStateLastSetterBar;
static BOOL gALBCarrierStateLastSetterValue;
static CFTimeInterval gALBCarrierStateLastSetterTime;
static int32_t gALBCarrierStateSettled = -1;
static int32_t gALBCarrierStateLastReported = -2;

// 1 minimized, 0 expanded, -1 unknown. While both carriers are visible (the
// ~0.8 s native overlap) the direction comes from the last observed setter, or
// for a UIKit-initiated transition, away from the last settled state.
FOUNDATION_EXPORT int32_t ALBNativeCarrierMinimizedState(UITabBar *bar);
int32_t ALBNativeCarrierMinimizedState(UITabBar *bar) {
    if (!NSThread.isMainThread || !ALBCarrierStateIsOurBar(bar)) return -1;
    UIView *expanded = nil, *minimized = nil;
    if (!ALBCarrierStatePrimaryCarriers(bar, &expanded, &minimized)) return -1;
    BOOL expandedVisible = ALBCarrierStateVisible(expanded), minimizedVisible = ALBCarrierStateVisible(minimized);
    BOOL recent = gALBCarrierStateLastSetterBar == bar &&
        CACurrentMediaTime() - gALBCarrierStateLastSetterTime < kALBCarrierStateRecentSetterSeconds;
    int32_t state = -1;
    NSString *basis = @"unknown";
    if (minimizedVisible && !expandedVisible) {
        if (recent && !gALBCarrierStateLastSetterValue) { state = 0; basis = @"settled-minimized+pending-expand"; }
        else { state = 1; gALBCarrierStateSettled = 1; basis = @"settled-minimized"; }
    } else if (expandedVisible && !minimizedVisible) {
        if (recent && gALBCarrierStateLastSetterValue) { state = 1; basis = @"settled-expanded+pending-minimize"; }
        else { state = 0; gALBCarrierStateSettled = 0; basis = @"settled-expanded"; }
    } else if (expandedVisible && minimizedVisible) {
        if (recent) { state = gALBCarrierStateLastSetterValue ? 1 : 0; basis = @"overlap+setter"; }
        else { state = gALBCarrierStateSettled == 1 ? 0 : 1; basis = @"overlap+uikit-initiated"; }
    }
    if (state != gALBCarrierStateLastReported) {
        gALBCarrierStateLastReported = state;
        if (ALBReproIsRecording()) ALBReproRecordEvent(@"bottom.nativestate.readback",
            @{@"state": @(state), @"basis": basis, @"expandedVisible": @(expandedVisible),
              @"minimizedVisible": @(minimizedVisible), @"monotonic": @(CACurrentMediaTime())});
    }
    return state;
}

#pragma mark - Setter observer

// Runs immediately before the original setter, in the same call.
static void ALBCarrierStateWillSetMinimized(id receiver, BOOL value) {
    if (!NSThread.isMainThread || !ALBCarrierStateIsOurBar(receiver)) return;
    gALBCarrierStateLastSetterBar = (UITabBar *)receiver;
    gALBCarrierStateLastSetterValue = value;
    gALBCarrierStateLastSetterTime = CACurrentMediaTime();
}

// Each variant forwards the unchanged receiver, selector and argument bits.
#define ALBCARRIERSTATE_PASS_THROUGH_1(T) imp_implementationWithBlock(^(id receiver, T value) { \
    ALBCarrierStateWillSetMinimized(receiver, value != 0); \
    ((void (*)(id, SEL, T))original)(receiver, selector, value); })
#define ALBCARRIERSTATE_PASS_THROUGH_2(T, U) imp_implementationWithBlock(^(id receiver, T value, U animated) { \
    ALBCarrierStateWillSetMinimized(receiver, value != 0); \
    ((void (*)(id, SEL, T, U))original)(receiver, selector, value, animated); })

static void ALBCarrierStateObserveSetter(NSString *name) {
    SEL selector = NSSelectorFromString(name);
    Method method = class_getInstanceMethod(UITabBar.class, selector);
    if (!method) return;
    const char *types = method_getTypeEncoding(method);
    ALB23NativeBoolABI abi;
    if (!ALB23ParseNativeBoolABI(types, method_getNumberOfArguments(method), true, &abi)) return;
    IMP original = method_getImplementation(method);
    IMP replacement = NULL;
    if (abi.arguments == 3) {
        replacement = abi.value == 'B' ? ALBCARRIERSTATE_PASS_THROUGH_1(bool) : ALBCARRIERSTATE_PASS_THROUGH_1(signed char);
    } else if (abi.value == 'B' && abi.animated == 'B') {
        replacement = ALBCARRIERSTATE_PASS_THROUGH_2(bool, bool);
    } else if (abi.value == 'B') {
        replacement = ALBCARRIERSTATE_PASS_THROUGH_2(bool, signed char);
    } else if (abi.animated == 'B') {
        replacement = ALBCARRIERSTATE_PASS_THROUGH_2(signed char, bool);
    } else {
        replacement = ALBCARRIERSTATE_PASS_THROUGH_2(signed char, signed char);
    }
    class_replaceMethod(UITabBar.class, selector, replacement, types);
}

__attribute__((constructor)) static void ALBCarrierStateInstall(void) {
    // After all load-time constructors (including the observation probe).
    dispatch_async(dispatch_get_main_queue(), ^{
        static BOOL installed;
        if (installed) return;
        installed = YES;
        ALBCarrierStateObserveSetter(@"_setMinimized:");
        ALBCarrierStateObserveSetter(@"_setMinimized:animated:");
    });
}
