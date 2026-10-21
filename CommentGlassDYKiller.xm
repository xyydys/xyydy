#import "ALBUnifiedMaterial.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <math.h>
#import "ALBReproCaptureSignal.h"

// Fresh implementation from the auth44 baseline.
// Structure follows DYKiller's panel-slot strategy; video geometry protection
// follows DYYY's comment-blur path. None of the previous comment-glass files
// are reused.

static const NSInteger kAWECommentGlassTag = 26082031;
static char kAWECommentSlotOriginalColorKey;
static char kAWECommentCoverOriginalColorKey;
static char kAWECommentSlotGlassKey;
static char kAWECommentInputFieldKey;
static char kAWECommentTintOverlayKey;
static char kAWECommentTopBlackOriginalColorKey;
static char kAWECommentTopBlackOriginalOpaqueKey;
static char kAWECommentGlassModeAppliedKey;
static char kAWECommentMediaBaselineFrameKey;
static char kAWECommentMediaBaselineBoundsKey;
static char kAWECommentMediaBaselineCenterKey;
static char kAWECommentMediaBaselineTransformKey;
static char kAWECommentMediaBaselineKindKey;
static char kAWECommentMediaBaselineWindowFrameKey;
static char kAWECommentMediaDismissRestoreKey;
static char kALB24MediaParentKey;
static char kALB24MediaFeedLeaseKey;
static CGSize gALB24CommentWindowSize;
static NSString * const kAWECommentGlassEnabledDefaultsKey = @"AwemeLiquidBarCommentGlassEnabledV1";
static NSString * const kAWECommentGlassModeDefaultsKey = @"AwemeLiquidBarCommentGlassModeV1";
static NSString * const kAWECommentGlassCornerRadiusDefaultsKey = @"AwemeLiquidBarCommentGlassCornerRadiusV1";
static NSString * const kAWEInternalFullscreenDefaultsKey = @"AwemeLiquidBarInternalFullscreenV1";
static NSHashTable<UIView *> *gAWECommentTopBlackViews = nil;
static BOOL gAWECommentVisible = NO;
// A dismissal completion belongs to one presentation, not whichever comment
// controller happens to be visible when the completion eventually runs.
static __weak UIViewController *gAWECommentOwner = nil;
static NSUInteger gAWECommentLifecycleGeneration = 0;
static char kAWECommentWillDisappearGenerationKey;
static __weak UIView *gAWECommentPanelSlot = nil;
static __weak UIView *gAWECommentInputContainer = nil;
static NSUInteger gAWECommentGeometryBlockCount = 0;
static NSString *gAWECommentLastSyncSignature = nil;
static NSHashTable<UIView *> *gAWECommentMediaBaselineViews = nil;
static NSUInteger gAWECommentMediaGuardCount = 0;
static NSUInteger gAWECommentMediaSnapshotGeneration = 0;
static __weak UIWindow *gAWECommentMediaBaselineWindow = nil;
static BOOL gAWECommentBaselineHadFullscreen = NO;

// Diagnostic-only, sampled on the existing comment paths. No new tree scan,
// text dump, hit-test probe, or main-thread file I/O is introduced here.
static BOOL ALBCommentCaptureSample(id owner, const void *key) {
    if (!owner || !NSThread.isMainThread || !ALBReproIsRecording()) return NO;
    CFTimeInterval now = CACurrentMediaTime();
    NSNumber *last = objc_getAssociatedObject(owner, key);
    if (last && now - last.doubleValue < 0.75) return NO;
    objc_setAssociatedObject(owner, key, @(now), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return YES;
}
static id ALBCommentCaptureColor(UIColor *color, UITraitCollection *traits) {
    if (!color) return NSNull.null;
    CGFloat r = 0, g = 0, b = 0, a = 0;
    UIColor *resolved = [color resolvedColorWithTraitCollection:traits];
    return [resolved getRed:&r green:&g blue:&b alpha:&a] ? (id)@[@(r), @(g), @(b), @(a)] : (id)@"non-rgb";
}
static NSDictionary *ALBCommentCaptureView(UIView *view) {
    if (!view) return @{@"found": @NO};
    UIVisualEffect *effect = [view isKindOfClass:UIVisualEffectView.class] ? ((UIVisualEffectView *)view).effect : nil;
    return @{@"found": @YES, @"class": NSStringFromClass(view.class),
        @"object": [NSString stringWithFormat:@"%p", view],
        @"parentClass": view.superview ? NSStringFromClass(view.superview.class) : @"nil",
        @"windowClass": view.window ? NSStringFromClass(view.window.class) : @"nil",
        @"frame": NSStringFromCGRect(view.frame), @"bounds": NSStringFromCGRect(view.bounds),
        @"windowFrame": view.window ? NSStringFromCGRect([view convertRect:view.bounds toView:view.window]) : @"unattached",
        @"alpha": @(view.alpha), @"hidden": @(view.hidden), @"opaque": @(view.opaque),
        @"layerOpacity": @(view.layer.opacity), @"radius": @(view.layer.cornerRadius),
        @"style": @(view.traitCollection.userInterfaceStyle), @"overrideStyle": @(view.overrideUserInterfaceStyle),
        @"effectClass": effect ? NSStringFromClass(effect.class) : @"nil",
        @"background": ALBCommentCaptureColor(view.backgroundColor, view.traitCollection),
        @"layerBackground": ALBCommentCaptureColor(view.layer.backgroundColor ? [UIColor colorWithCGColor:view.layer.backgroundColor] : nil, view.traitCollection)};
}
static void ALBCommentCaptureReason(NSMutableDictionary *reasons, NSString *reason) {
    if (reasons) reasons[reason] = @([reasons[reason] unsignedIntegerValue] + 1);
}
static void ALBCommentCapturePair(NSMutableArray *samples, UIView *view, NSDictionary *before, NSString *reason) {
    if (samples && samples.count < 12 && before)
        [samples addObject:@{@"reason": reason, @"before": before, @"after": ALBCommentCaptureView(view)}];
}
static void ALBCommentCaptureObserved(NSMutableArray *views, UIView *view) {
    if (!views || views.count >= 48 || [view isKindOfClass:UILabel.class]) return;
    [views addObject:ALBCommentCaptureView(view)];
}
static void ALBCommentCaptureLifecycle(UIView *view, NSString *phase) {
    if (!NSThread.isMainThread || !ALBReproIsRecording()) return;
    ALBReproRecordEvent(@"comment.lifecycle", @{@"phase": phase, @"visible": @(gAWECommentVisible),
        @"commentCount": @"unknown", @"generation": @(gAWECommentLifecycleGeneration),
        @"ownerClass": gAWECommentOwner ? NSStringFromClass(gAWECommentOwner.class) : @"nil",
        @"view": ALBCommentCaptureView(view)});
    ALBReproScheduleSnapshot([@"comment." stringByAppendingString:phase], 0.4);
}

static BOOL AWECommentGlassFeatureEnabled(void) {
    if (NSProcessInfo.processInfo.operatingSystemVersion.majorVersion < 26) return NO;
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:kAWECommentGlassEnabledDefaultsKey];
    return [stored isKindOfClass:NSNumber.class] ? [(NSNumber *)stored boolValue] : YES;
}

static NSInteger AWECommentGlassMode(void) {
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:kAWECommentGlassModeDefaultsKey];
    NSInteger mode = [stored isKindOfClass:NSNumber.class] ? [(NSNumber *)stored integerValue] : 1;
    return MIN(2, MAX(0, mode)); /* 0 clear, 1 native tint, 2 frosted material */
}

static CGFloat AWECommentGlassCornerRadius(void) {
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:kAWECommentGlassCornerRadiusDefaultsKey];
    CGFloat value = [stored isKindOfClass:NSNumber.class] ? [(NSNumber *)stored doubleValue] : 22.0;
    return MIN(40.0, MAX(16.0, value));
}

/* Fullscreen media preservation is a geometry feature, not a glass feature.
   When the user's Fullscreen Playback switch is on, opening/closing comments
   must not shrink a previously-fullscreen player even if comment glass is off. */
static BOOL AWECommentMediaGuardEnabled(void) {
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:kAWEInternalFullscreenDefaultsKey];
    return [stored isKindOfClass:NSNumber.class] ? [(NSNumber *)stored boolValue] : YES;
}

static NSString *AWECommentProbePath(void) {
    NSString *documents = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                                NSUserDomainMask,
                                                                YES) firstObject];
    return [documents stringByAppendingPathComponent:@"AwemeLiquidBar_comment_message_probe_2.0.1.log"];
}

static void AWECommentProbe(NSString *format, ...) {
    if (format.length == 0) return;
    va_list args;
    va_start(args, format);
    NSString *line = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    if (!line.length) return;
    NSString *record = [NSString stringWithFormat:@"%@ %@\n", [NSDate date], line];
    NSData *data = [record dataUsingEncoding:NSUTF8StringEncoding];
    NSString *path = AWECommentProbePath();
    @synchronized(NSFileManager.class) {
        NSFileManager *fm = NSFileManager.defaultManager;
        unsigned long long size = [[fm attributesOfItemAtPath:path error:nil] fileSize];
        if (size > 3 * 1024 * 1024) [fm removeItemAtPath:path error:nil];
        if (![fm fileExistsAtPath:path]) {
            [data writeToFile:path atomically:YES];
            return;
        }
        NSFileHandle *handle = [NSFileHandle fileHandleForWritingAtPath:path];
        if (!handle) return;
        @try {
            [handle seekToEndOfFile];
            [handle writeData:data];
            [handle closeFile];
        } @catch (__unused NSException *exception) {
            @try { [handle closeFile]; } @catch (__unused NSException *ignored) {}
        }
    }
}

static BOOL AWECommentColorIsOpaque(UIColor *color) {
    return color && color.CGColor && CGColorGetAlpha(color.CGColor) >= 0.99;
}

static BOOL AWECommentViewHasVisibleBackdrop(UIView *view) {
    if (!view) return NO;
    UIColor *bg = view.backgroundColor;
    if (bg.CGColor && CGColorGetAlpha(bg.CGColor) > 0.025) return YES;
    CGColorRef layerBG = view.layer.backgroundColor;
    return layerBG && CGColorGetAlpha(layerBG) > 0.025;
}

static void AWECommentClearVisibleBackdrop(UIView *view) {
    if (!view || !AWECommentViewHasVisibleBackdrop(view)) return;
    UIColor *remembered = objc_getAssociatedObject(view, &kAWECommentCoverOriginalColorKey);
    if (!remembered && view.backgroundColor) {
        objc_setAssociatedObject(view, &kAWECommentCoverOriginalColorKey,
                                 view.backgroundColor, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    view.backgroundColor = UIColor.clearColor;
    view.layer.backgroundColor = UIColor.clearColor.CGColor;
    view.opaque = NO;
}

static UIViewController *AWECommentControllerForView(UIView *view) {
    UIResponder *responder = view;
    NSInteger guard = 0;
    while (responder && guard++ < 80) {
        if ([responder isKindOfClass:UIViewController.class]) {
            return (UIViewController *)responder;
        }
        responder = responder.nextResponder;
    }
    return nil;
}

static BOOL AWECommentControllerNameLooksComment(UIViewController *controller) {
    UIViewController *cursor = controller;
    for (NSInteger depth = 0; cursor && depth < 10; depth++, cursor = cursor.parentViewController) {
        NSString *name = NSStringFromClass(cursor.class).lowercaseString;
        if ([name containsString:@"comment"]) return YES;
    }
    return NO;
}

static BOOL AWECommentViewIsInsideComment(UIView *view) {
    return AWECommentControllerNameLooksComment(AWECommentControllerForView(view));
}

static UIViewController *AWECommentChildControllerNamed(UIViewController *controller,
                                                         NSString *className) {
    if (!controller || className.length == 0) return nil;
    for (UIViewController *child in controller.childViewControllers) {
        if ([NSStringFromClass(child.class) isEqualToString:className]) return child;
        UIViewController *nested = AWECommentChildControllerNamed(child, className);
        if (nested) return nested;
    }
    return nil;
}

static UIView *AWECommentFindSubviewOfClass(UIView *root, Class cls, NSUInteger maxDepth) {
    if (!root || !cls) return nil;
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[root, @0]];
    NSUInteger index = 0;
    while (index < queue.count) {
        NSArray *entry = queue[index++];
        UIView *view = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        if (view != root && [view isKindOfClass:cls]) return view;
        if (depth >= maxDepth) continue;
        for (UIView *sub in view.subviews) [queue addObject:@[sub, @(depth + 1)]];
    }
    return nil;
}

static UIUserInterfaceStyle AWECommentSceneStyleForView(UIView *view) {
    UIUserInterfaceStyle style = view.window.windowScene.traitCollection.userInterfaceStyle;
    if (style == UIUserInterfaceStyleUnspecified) style = view.traitCollection.userInterfaceStyle;
    if (style == UIUserInterfaceStyleUnspecified) style = UIUserInterfaceStyleLight;
    return style;
}


static UIVisualEffect *AWECommentMakeGlassEffectForMode(NSInteger mode) {
    if (!AWECommentGlassFeatureEnabled()) return nil;
    if (mode == 2) return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
    if (ALBMaterialEnabled()) return ALBMaterialMakeEffect(YES);
    Class glassClass = NSClassFromString(@"UIGlassEffect");
    if (!glassClass) return nil;

    id effect = nil;
    if (mode == 0) {
        /* Pure-clear mode: the same clear branch used by the successful v4
           panel. It stays visually transparent and only follows system traits. */
        SEL maker = NSSelectorFromString(@"effectWithStyle:");
        if ([glassClass respondsToSelector:maker]) {
            effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(glassClass, maker, 1);
        }
    } else {
        /* Tint mode: use UIKit's normal UIGlassEffect initializer.  Its
           light/dark branch is driven only by Douyin's app/window appearance;
           no video-region probe participates in the decision. */
        effect = [[glassClass alloc] init];
    }
    if (!effect) return nil;

    SEL setInteractive = NSSelectorFromString(@"setInteractive:");
    if ([effect respondsToSelector:setInteractive]) {
        ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, setInteractive, YES);
    }
    SEL setTint = NSSelectorFromString(@"setTintColor:");
    if ([effect respondsToSelector:setTint]) {
        ((void (*)(id, SEL, id))objc_msgSend)(effect, setTint, nil);
    }
    return [effect isKindOfClass:UIVisualEffect.class] ? (UIVisualEffect *)effect : nil;
}

static BOOL AWECommentIsOurGlassTree(UIView *view) {
    UIView *cursor = view;
    for (NSInteger depth = 0; cursor && depth < 8; depth++, cursor = cursor.superview) {
        if (cursor.tag == kAWECommentGlassTag) return YES;
    }
    return NO;
}

static BOOL AWECommentIsCoverCandidate(UIView *view, UIView *slot) {
    if (!view || !slot || view == slot || view.hidden || view.alpha < 0.01) return NO;
    if (AWECommentIsOurGlassTree(view)) return NO;
    if ([view isKindOfClass:UIVisualEffectView.class] ||
        [view isKindOfClass:UILabel.class] ||
        [view isKindOfClass:UIControl.class] ||
        [view isKindOfClass:UIImageView.class]) return NO;
    if (!AWECommentColorIsOpaque(view.backgroundColor)) return NO;
    if (fabs(CGRectGetWidth(view.bounds) - CGRectGetWidth(slot.bounds)) > 1.0) return NO;
    return CGRectGetHeight(view.bounds) >= 8.0;
}

static void AWECommentRememberAndClearColor(UIView *view, const void *key) {
    if (!view) return;
    UIColor *remembered = objc_getAssociatedObject(view, key);
    if (!remembered && AWECommentColorIsOpaque(view.backgroundColor)) {
        objc_setAssociatedObject(view, key, view.backgroundColor,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (AWECommentColorIsOpaque(view.backgroundColor)) {
        view.backgroundColor = UIColor.clearColor;
        view.layer.backgroundColor = UIColor.clearColor.CGColor;
        view.opaque = NO;
    }
}

static BOOL AWECommentRectNearlyMatchesPanel(CGRect rect, CGRect panelRect) {
    if (CGRectIsNull(rect) || CGRectIsEmpty(rect) ||
        CGRectIsNull(panelRect) || CGRectIsEmpty(panelRect)) return NO;
    return fabs(CGRectGetMinX(rect) - CGRectGetMinX(panelRect)) <= 2.5 &&
           fabs(CGRectGetMinY(rect) - CGRectGetMinY(panelRect)) <= 2.5 &&
           fabs(CGRectGetWidth(rect) - CGRectGetWidth(panelRect)) <= 3.0 &&
           fabs(CGRectGetHeight(rect) - CGRectGetHeight(panelRect)) <= 3.0;
}

/*
 The rounded Liquid Glass lives inside Douyin's comment-inner controller, but
 recent builds can keep one or more same-frame backdrop/wrapper layers behind
 that inner view.  If only the glass itself is rounded, those rectangular
 carriers remain visible through the two top corner cut-outs and look like a
 pair of translucent "shadows" level with the panel top.

 Keep the cleanup geometry-scoped: only same-frame panel carriers are touched.
 Labels, controls, images and the plugin glass tree are never modified.
*/
static void AWECommentClearMatchingPanelBackdrop(UIView *view, UIView *slot) {
    if (!view || !slot || view == slot || AWECommentIsOurGlassTree(view)) return;
    if ([view isKindOfClass:UILabel.class] || [view isKindOfClass:UIControl.class] ||
        [view isKindOfClass:UIImageView.class]) return;
    CGRect rect = [view convertRect:view.bounds toView:slot];
    if (!AWECommentRectNearlyMatchesPanel(rect, slot.bounds)) return;

    if ([view isKindOfClass:UIVisualEffectView.class]) {
        UIVisualEffectView *effectView = (UIVisualEffectView *)view;
        effectView.effect = nil;
        effectView.backgroundColor = UIColor.clearColor;
        effectView.layer.backgroundColor = UIColor.clearColor.CGColor;
        effectView.opaque = NO;
        return;
    }
    if (AWECommentViewHasVisibleBackdrop(view)) {
        AWECommentClearVisibleBackdrop(view);
    }
}

static void AWECommentClearCovers(UIView *root, UIView *slot,
                                  NSUInteger depth, NSUInteger maxDepth) {
    if (!root || !slot || depth > maxDepth) return;
    for (UIView *sub in [root.subviews copy]) {
        if (AWECommentIsOurGlassTree(sub)) continue;
        AWECommentClearMatchingPanelBackdrop(sub, slot);
        if (AWECommentIsCoverCandidate(sub, slot)) {
            AWECommentRememberAndClearColor(sub, &kAWECommentCoverOriginalColorKey);
        }
        if ([sub isKindOfClass:UILabel.class] || [sub isKindOfClass:UIImageView.class]) continue;
        AWECommentClearCovers(sub, slot, depth + 1, maxDepth);
    }
}

static void AWECommentClipPanelChainToRoundedTop(UIView *slot,
                                                  UIViewController *controller,
                                                  CGFloat radius) {
    if (!slot || !slot.window || radius <= 0.0) return;
    UIWindow *window = slot.window;
    CGRect panelRect = [slot convertRect:slot.bounds toView:window];
    UIView *cursor = slot;
    for (NSInteger depth = 0; cursor && depth < 8; depth++) {
        CGRect rect = [cursor convertRect:cursor.bounds toView:window];
        if (AWECommentRectNearlyMatchesPanel(rect, panelRect)) {
            cursor.layer.cornerRadius = radius;
            cursor.layer.masksToBounds = YES;
            cursor.clipsToBounds = YES;
            cursor.layer.shadowOpacity = 0.0f;
            cursor.layer.shadowPath = nil;
            if (@available(iOS 13.0, *)) {
                cursor.layer.cornerCurve = kCACornerCurveContinuous;
                cursor.layer.maskedCorners = kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner;
            }
        }

        UIView *parent = cursor.superview;
        if (!parent) break;
        NSArray<UIView *> *siblings = [parent.subviews copy];
        NSUInteger index = [siblings indexOfObjectIdenticalTo:cursor];
        if (index != NSNotFound) {
            for (NSUInteger i = 0; i < index; i++) {
                UIView *sibling = siblings[i];
                if (AWECommentIsOurGlassTree(sibling)) continue;
                CGRect siblingRect = [sibling convertRect:sibling.bounds toView:window];
                if (!AWECommentRectNearlyMatchesPanel(siblingRect, panelRect)) continue;
                AWECommentClearMatchingPanelBackdrop(sibling, slot);
                sibling.layer.shadowOpacity = 0.0f;
                sibling.layer.shadowPath = nil;
            }
        }
        if (controller && cursor == controller.view) break;
        cursor = parent;
    }
}

static UIColor *AWECommentSystemBorderColor(UIView *view) {
    (void)view;
    return [UIColor colorWithWhite:1.0 alpha:0.20];
}

static void AWECommentApplyRoundedGlassChrome(UIVisualEffectView *glass, UIView *slot) {
    if (!glass || !slot) return;
    CGFloat radius = AWECommentGlassCornerRadius();
    glass.frame = slot.bounds;
    glass.clipsToBounds = YES;
    glass.layer.masksToBounds = YES;
    glass.layer.cornerRadius = radius;
    if (@available(iOS 13.0, *)) {
        glass.layer.cornerCurve = kCACornerCurveContinuous;
        glass.layer.maskedCorners = kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner;
    }
    glass.layer.borderWidth = 0.65;
    glass.layer.borderColor = AWECommentSystemBorderColor(slot).CGColor;

    /* Do not paint a custom light/dark overlay over the material.  Clear mode
       stays clear; tint mode is resolved only by Douyin's own app trait. */
    UIView *oldTint = objc_getAssociatedObject(slot, &kAWECommentTintOverlayKey);
    if (oldTint) {
        [oldTint removeFromSuperview];
        objc_setAssociatedObject(slot, &kAWECommentTintOverlayKey, nil,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static UIVisualEffectView *AWECommentEnsurePanelGlass(UIView *slot,
                                                       UIViewController *controller) {
    if (!slot) return nil;
    UIVisualEffectView *glass = objc_getAssociatedObject(slot, &kAWECommentSlotGlassKey);
    if (glass && glass.superview != slot) {
        glass = nil;
        objc_setAssociatedObject(slot, &kAWECommentSlotGlassKey, nil,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    AWECommentRememberAndClearColor(slot, &kAWECommentSlotOriginalColorKey);
    if (!glass) {
        glass = [[UIVisualEffectView alloc] initWithEffect:nil];
        glass.tag = kAWECommentGlassTag;
        glass.userInteractionEnabled = NO;
        glass.alpha = 1.0;
        glass.backgroundColor = UIColor.clearColor;
        glass.opaque = NO;
        glass.autoresizingMask = UIViewAutoresizingFlexibleWidth |
                                 UIViewAutoresizingFlexibleHeight;
        [slot insertSubview:glass atIndex:0];
        objc_setAssociatedObject(slot, &kAWECommentSlotGlassKey, glass,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (slot.subviews.firstObject != glass) [slot insertSubview:glass atIndex:0];
    NSInteger mode = AWECommentGlassMode();
    glass.overrideUserInterfaceStyle = (mode != 0)
        ? AWECommentSceneStyleForView(slot)
        : UIUserInterfaceStyleUnspecified;
    AWECommentApplyRoundedGlassChrome(glass, slot);
    /* Clip Douyin's same-frame panel wrappers to the identical top-corner path.
       This removes the rectangular translucent carrier that otherwise leaks
       through beside the two rounded glass corners. */
    AWECommentClipPanelChainToRoundedTop(slot, controller,
                                         AWECommentGlassCornerRadius());
    NSNumber *appliedMode = objc_getAssociatedObject(glass, &kAWECommentGlassModeAppliedKey);
    BOOL modeChanged = !appliedMode || appliedMode.integerValue != mode;
    if (modeChanged) {
        glass.effect = nil;
        objc_setAssociatedObject(glass, &kAWECommentGlassModeAppliedKey, @(mode),
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (!glass.effect) {
        UIVisualEffect *effect = AWECommentMakeGlassEffectForMode(mode);
        if (effect) {
            // DYKiller's key behavior: mount the effect view first, then
            // materialize the glass effect after geometry is valid.
            dispatch_async(dispatch_get_main_queue(), ^{
                if (glass.superview && !glass.effect && AWECommentGlassMode() == mode) {
                    glass.effect = effect;
                }
            });
        }
    }
    if (mode != 2 && ALBMaterialEnabled() && glass.effect) ALBMaterialApplyEffectView(glass, NO);
    return glass;
}

static BOOL AWECommentViewContainsInputSignals(UIView *root) {
    if (!root) return NO;
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[root, @0]];
    NSUInteger index = 0;
    NSUInteger controlCount = 0;
    while (index < queue.count && index < 100) {
        NSArray *entry = queue[index++];
        UIView *view = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        NSString *name = NSStringFromClass(view.class).lowercaseString;
        if ([view isKindOfClass:UITextView.class] || [view isKindOfClass:UITextField.class] ||
            [name containsString:@"commentinput"] || [name containsString:@"textinput"] ||
            [name containsString:@"emoticon"] || [name containsString:@"emoji"]) {
            return YES;
        }
        if ([view isKindOfClass:UIControl.class]) {
            controlCount++;
            if (controlCount >= 2) return YES;
        }
        if (depth < 4) {
            for (UIView *sub in view.subviews) [queue addObject:@[sub, @(depth + 1)]];
        }
    }
    return NO;
}

/*
 Zero-comment pages do not always expose the normal
 AWECommentInputViewSwiftImpl.CommentInputContainerView at the same depth.  In
 that state Douyin may wrap the same composer in an empty-state footer/carrier.
 Resolve only a bottom, nearly full-width host with real input/control signals;
 this keeps the fallback scoped to the composer rather than the empty-state
 illustration or the whole comment panel.
 */
static UIView *AWECommentFallbackInputContainer(UIView *panel) {
    if (!panel || CGRectGetWidth(panel.bounds) < 100.0) return nil;
    CGFloat panelW = CGRectGetWidth(panel.bounds);
    CGFloat panelH = CGRectGetHeight(panel.bounds);
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[panel, @0]];
    NSUInteger index = 0;
    UIView *best = nil;
    CGFloat bestScore = -CGFLOAT_MAX;
    while (index < queue.count && index < 260) {
        NSArray *entry = queue[index++];
        UIView *view = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        if (depth < 8) {
            for (UIView *sub in view.subviews) [queue addObject:@[sub, @(depth + 1)]];
        }
        if (view == panel || view.hidden || view.alpha < 0.02 ||
            [view isKindOfClass:UILabel.class] || [view isKindOfClass:UIImageView.class] ||
            AWECommentIsOurGlassTree(view)) continue;
        CGRect r = [view convertRect:view.bounds toView:panel];
        CGFloat w = CGRectGetWidth(r), h = CGRectGetHeight(r);
        if (w < panelW * 0.86 || w > panelW * 1.04 || h < 38.0 || h > 150.0) continue;
        if (CGRectGetMaxY(r) < panelH - 150.0 || CGRectGetMaxY(r) > panelH + 12.0) continue;
        if (!AWECommentViewContainsInputSignals(view)) continue;
        CGFloat score = w - fabs(CGRectGetMaxY(r) - panelH) * 4.0 - fabs(h - 64.0) * 1.2;
        NSString *name = NSStringFromClass(view.class).lowercaseString;
        if ([name containsString:@"input"] || [name containsString:@"comment"]) score += 100.0;
        if (score > bestScore) { bestScore = score; best = view; }
    }
    return best;
}

static BOOL AWECommentTreeLooksEmptyState(UIView *root, NSDictionary **evidence) {
    if (evidence) *evidence = @{@"source": @"no-empty-signal-matched", @"commentCount": @"unknown"};
    if (!root) return NO;
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[root, @0]];
    NSUInteger index = 0;
    while (index < queue.count && index < 420) {
        NSArray *entry = queue[index++];
        UIView *view = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        if (view.hidden || view.alpha < .02 || AWECommentIsOurGlassTree(view)) continue;
        if ([view isKindOfClass:UILabel.class]) {
            UILabel *label = (UILabel *)view;
            NSString *text = label.text ?: label.attributedText.string ?: @"";
            NSString *lower = text.lowercaseString;
            if ([text containsString:@"期待你的评论"] || [text containsString:@"发条评论参与讨论"] ||
                [text containsString:@"暂无评论"] || [text containsString:@"还没有评论"] ||
                [text containsString:@"快来抢沙发"] || [text containsString:@"暂无人评论"] ||
                [text isEqualToString:@"去评论"] || [lower containsString:@"be the first to comment"] ||
                [lower containsString:@"no comments"]) {
                if (evidence) *evidence = @{@"source": @"existing-empty-copy-rule", @"commentCount": @"unknown",
                    @"viewClass": NSStringFromClass(view.class)};
                return YES;
            }
        }
        if ([view isKindOfClass:UICollectionView.class]) {
            UICollectionView *collection = (UICollectionView *)view;
            CGRect r = [collection convertRect:collection.bounds toView:root];
            if (CGRectGetWidth(r) >= CGRectGetWidth(root.bounds) * 0.75 && CGRectGetHeight(r) >= 120.0) {
                NSInteger sections = collection.numberOfSections;
                NSInteger items = 0;
                for (NSInteger section = 0; section < sections && section < 20; section++) {
                    items += [collection numberOfItemsInSection:section];
                    if (items > 0) break;
                }
                if (items == 0) {
                    if (evidence) *evidence = @{@"source": @"existing-collection-zero-items-rule", @"commentCount": @"unknown",
                        @"viewClass": NSStringFromClass(view.class), @"collectionSections": @(sections),
                        @"scannedSections": @(MIN(sections, 20)), @"scannedItems": @0,
                        @"allSectionsInspected": @(sections <= 20)};
                    return YES;
                }
            }
        }
        if (depth < 12) {
            for (UIView *sub in view.subviews) [queue addObject:@[sub, @(depth + 1)]];
        }
    }
    return NO;
}

static UIView *AWECommentInputFieldSlot(UIView *container) {
    if (!container) return nil;
    UIView *cached = objc_getAssociatedObject(container, &kAWECommentInputFieldKey);
    if (cached && cached.superview && !cached.hidden) return cached;
    CGFloat containerWidth = CGRectGetWidth(container.bounds);
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[container, @0]];
    NSUInteger index = 0;
    UIView *best = nil;
    CGFloat bestScore = -CGFLOAT_MAX;
    while (index < queue.count) {
        NSArray *entry = queue[index++];
        UIView *view = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        if (depth < 5) {
            for (UIView *sub in view.subviews) [queue addObject:@[sub, @(depth + 1)]];
        }
        if (view == container || [view isKindOfClass:UIControl.class] ||
            [view isKindOfClass:UILabel.class] || [view isKindOfClass:UIImageView.class]) continue;
        CGRect r = [view convertRect:view.bounds toView:container];
        CGFloat w = CGRectGetWidth(r), h = CGRectGetHeight(r);
        if (w < containerWidth * 0.80 || w > containerWidth * 0.99) continue;
        if (h < 30.0 || h > 48.0) continue;
        if (CGRectGetMinX(r) < 5.0 || CGRectGetMinX(r) > 28.0) continue;
        CGFloat score = 0.0;
        score -= fabs(w - 416.0) * 0.5;
        score -= fabs(h - 36.0) * 2.0;
        if (view.layer.cornerRadius > 0.0) score += 80.0;
        if (AWECommentColorIsOpaque(view.backgroundColor)) score += 30.0;
        if (score > bestScore) { bestScore = score; best = view; }
    }
    if (best) objc_setAssociatedObject(container, &kAWECommentInputFieldKey, best,
                                       OBJC_ASSOCIATION_ASSIGN);
    return best;
}

static void AWECommentClearInputOuterBackground(UIView *container, UIView *field) {
    if (!container) return;
    static char captureKey;
    BOOL capture = ALBCommentCaptureSample(container, &captureKey);
    NSMutableArray *samples = capture ? [NSMutableArray array] : nil;
    NSDictionary *beforeContainer = capture ? ALBCommentCaptureView(container) : nil;
    container.backgroundColor = UIColor.clearColor;
    container.layer.backgroundColor = UIColor.clearColor.CGColor;
    container.opaque = NO;
    ALBCommentCapturePair(samples, container, beforeContainer, @"input-container");
    CGFloat width = CGRectGetWidth(container.bounds);
    CGRect fieldRect = field ? [field convertRect:field.bounds toView:container] : CGRectNull;
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[container, @0]];
    NSUInteger index = 0;
    while (index < queue.count) {
        NSArray *entry = queue[index++];
        UIView *view = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        if (depth < 8) {
            for (UIView *sub in view.subviews) [queue addObject:@[sub, @(depth + 1)]];
        }
        if (view == container || (field && (view == field || [view isDescendantOfView:field]))) continue;
        if ([view isKindOfClass:UILabel.class] || [view isKindOfClass:UIControl.class] ||
            [view isKindOfClass:UIImageView.class] || [view isKindOfClass:UIVisualEffectView.class]) continue;
        CGRect r = [view convertRect:view.bounds toView:container];
        CGFloat h = CGRectGetHeight(r);
        CGFloat w = CGRectGetWidth(r);
        if (!field && view.layer.cornerRadius >= 10.0 && h >= 28.0 && h <= 56.0 &&
            w >= width * 0.72 && w <= width * 0.98) continue;
        BOOL fullWidth = w >= width * 0.94;
        BOOL belowOrAroundField = CGRectIsNull(fieldRect) ||
            CGRectGetMinY(r) >= CGRectGetMaxY(fieldRect) - 6.0 ||
            (CGRectGetMinY(r) <= CGRectGetMinY(fieldRect) + 2.0 &&
             CGRectGetMaxY(r) >= CGRectGetMaxY(fieldRect) - 2.0);
        /* DYKiller treats the input container as the semantic unit: remove its
           backdrop/emoji-tab carriers but keep the rounded field itself.  Use
           visible alpha (including CALayer backgrounds), not only 0.99 opaque,
           because Douyin's emoji suggestion strip is translucent. */
        if (fullWidth && h >= 8.0 && h <= MAX(120.0, CGRectGetHeight(container.bounds)) &&
            belowOrAroundField && AWECommentViewHasVisibleBackdrop(view)) {
            NSDictionary *before = (samples && samples.count < 12) ? ALBCommentCaptureView(view) : nil;
            AWECommentClearVisibleBackdrop(view);
            ALBCommentCapturePair(samples, view, before, @"input-backing-carrier");
        }
    }
    if (capture) ALBReproRecordEvent(@"comment.input.clear", @{@"container": ALBCommentCaptureView(container),
        @"field": ALBCommentCaptureView(field), @"scanned": @(index), @"samples": samples,
        @"sampleLimit": @12, @"fieldReason": field ? @"matched-existing-field-rule" : @"no-field-match"});
}

static char kALB5ZeroScopeKey,kALB5ZeroMarkKey;
static NSHashTable *gALB5ZeroRecords;
static NSHashTable<UIView *> *gALBCommentGuidanceViews;
@interface ALB5ZeroBackdropRecord : NSObject
@property(nonatomic,weak) UIView *view;
@property(nonatomic,weak) UIView *scope;
@property(nonatomic,strong) UIColor *color;
@property(nonatomic,strong) UIColor *layerColor;
@property(nonatomic,strong) UIVisualEffect *effect;
@property(nonatomic) BOOL opaque;
@end
@implementation ALB5ZeroBackdropRecord
@end
static void ALB5RestoreZeroView(UIView *view) {
    ALB5ZeroBackdropRecord *record = objc_getAssociatedObject(view, &kALB5ZeroMarkKey);
    if (!record) return;
    // Unmark first so our setters permit the native values to return.
    objc_setAssociatedObject(view, &kALB5ZeroMarkKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    view.backgroundColor = record.color;
    view.layer.backgroundColor = record.layerColor.CGColor;
    view.opaque = record.opaque;
    if ([view isKindOfClass:UIVisualEffectView.class]) ((UIVisualEffectView *)view).effect = record.effect;
    [gALB5ZeroRecords removeObject:record];
}
static void ALB5RestoreZeroScope(UIView *scope) {
    for(ALB5ZeroBackdropRecord *record in gALB5ZeroRecords.allObjects) {
        if(scope && record.scope!=scope)continue;
        UIView *v=record.view;if(!v)continue;
        ALB5RestoreZeroView(v);
    }
}
static BOOL ALB5KeepZeroBackdropClear(UIView *view) {
    ALB5ZeroBackdropRecord *record=objc_getAssociatedObject(view,&kALB5ZeroMarkKey);
    return record && record.scope.window && gAWECommentVisible && AWECommentGlassFeatureEnabled() &&
        [objc_getAssociatedObject(record.scope,&kALB5ZeroScopeKey) boolValue];
}
static void ALB5RememberZeroBackdrop(UIView *view,UIView *scope) {
    if(objc_getAssociatedObject(view,&kALB5ZeroMarkKey))return;
    ALB5ZeroBackdropRecord *record=[ALB5ZeroBackdropRecord new];record.view=view;record.scope=scope;
    record.color=view.backgroundColor;
    record.opaque=view.opaque;
    record.layerColor=view.layer.backgroundColor?[UIColor colorWithCGColor:view.layer.backgroundColor]:nil;
    if([view isKindOfClass:UIVisualEffectView.class])record.effect=((UIVisualEffectView *)view).effect;
    objc_setAssociatedObject(view,&kALB5ZeroMarkKey,record,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(!gALB5ZeroRecords)gALB5ZeroRecords=[NSHashTable weakObjectsHashTable];[gALB5ZeroRecords addObject:record];
}

/* The zero-comment recording identifies a different keyboard accessory from
   MiniEmoticonPanel: AWECommentPublishGuidanceView (440 x 42), whose own view
   AND layer backgrounds are white. Its collection is already transparent.
   Keep a lease on this exact accessory and its full-size collection only;
   never walk into guidance cells, their emoji images, or the keyboard host. */
static void AWECommentClearPublishGuidanceBackdrop(UIView *guidance) {
    if (!guidance) return;
    UIView *scope = gAWECommentPanelSlot;
    BOOL active = guidance.window && scope.window && gAWECommentVisible &&
        AWECommentGlassFeatureEnabled() &&
        [objc_getAssociatedObject(scope, &kALB5ZeroScopeKey) boolValue];
    CGFloat width = CGRectGetWidth(guidance.bounds), height = CGRectGetHeight(guidance.bounds);
    BOOL accessorySize = width >= 100.0 && height >= 7.0 && height <= 100.0;
    if (!active || !accessorySize) {
        ALB5RestoreZeroView(guidance);
        for (UIView *child in guidance.subviews)
            if ([child isKindOfClass:UICollectionView.class]) ALB5RestoreZeroView(child);
        return;
    }
    static char captureKey;
    BOOL capture = ALBCommentCaptureSample(guidance, &captureKey);
    NSMutableArray *samples = capture ? [NSMutableArray array] : nil;
    NSMutableArray<UIView *> *targets = [NSMutableArray arrayWithObject:guidance];
    for (UIView *child in guidance.subviews) {
        if (![child isKindOfClass:UICollectionView.class]) continue;
        if (CGRectContainsRect(CGRectInset(guidance.bounds, -2.0, -2.0), child.frame) &&
            fabs(CGRectGetWidth(child.frame) - width) <= 4.0 &&
            fabs(CGRectGetHeight(child.frame) - height) <= 4.0) [targets addObject:child];
    }
    for (UIView *target in targets) {
        ALB5ZeroBackdropRecord *old = objc_getAssociatedObject(target, &kALB5ZeroMarkKey);
        if (old && old.scope != scope) ALB5RestoreZeroView(target);
        NSDictionary *before = capture ? ALBCommentCaptureView(target) : nil;
        ALB5RememberZeroBackdrop(target, scope);
        if (AWECommentViewHasVisibleBackdrop(target)) {
            target.backgroundColor = UIColor.clearColor;
            target.layer.backgroundColor = UIColor.clearColor.CGColor;
        }
        target.opaque = NO;
        ALBCommentCapturePair(samples, target, before, @"zero-publish-guidance-backing");
    }
    if (capture) ALBReproRecordEvent(@"comment.guidance.clear", @{
        @"scope": ALBCommentCaptureView(scope), @"guidance": ALBCommentCaptureView(guidance),
        @"samples": samples, @"preservedGuidanceCells": @YES});
}

static void AWECommentSyncPublishGuidanceBackdrops(void) {
    for (UIView *guidance in gALBCommentGuidanceViews.allObjects)
        AWECommentClearPublishGuidanceBackdrop(guidance);
}

static void AWECommentClearZeroStateBottomBackdrop(UIView *panel,
                                                    UIView *input,
                                                    UIView *field) {
    if(!panel)return;
    static char captureKey, stateKey;
    BOOL capture = ALBCommentCaptureSample(panel, &captureKey);
    NSDictionary *evidence = nil;
    BOOL empty=AWECommentTreeLooksEmptyState(panel, capture ? &evidence : NULL);
    if (capture) {
        NSNumber *previous = objc_getAssociatedObject(panel, &stateKey);
        if (!previous || previous.boolValue != empty) {
            ALBReproRecordEvent(@"comment.empty-state", @{@"matchedExistingRule": @(empty), @"evidence": evidence,
                @"panel": ALBCommentCaptureView(panel)});
            ALBReproScheduleSnapshot(@"comment.empty-state.changed", 0.4);
            objc_setAssociatedObject(panel, &stateKey, @(empty), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }
    objc_setAssociatedObject(panel,&kALB5ZeroScopeKey,@(empty),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if(!empty) {
        if (capture) ALBReproRecordEvent(@"comment.zero.clear-skipped", @{@"reason": @"empty-state-rule-not-matched", @"evidence": evidence});
        ALB5RestoreZeroScope(panel);return;
    }
    AWECommentSyncPublishGuidanceBackdrops();
    CGFloat panelW = CGRectGetWidth(panel.bounds);
    CGFloat panelH = CGRectGetHeight(panel.bounds);
    if (panelW < 100.0 || panelH < 160.0) {
        if (capture) ALBReproRecordEvent(@"comment.zero.clear-skipped", @{@"reason": @"panel-too-small", @"panel": ALBCommentCaptureView(panel)});
        return;
    }
    CGRect inputRect = input ? [input convertRect:input.bounds toView:panel] : CGRectNull;
    CGRect fieldRect = (input && field) ? [field convertRect:field.bounds toView:panel] : CGRectNull;
    CGFloat zoneTop = CGRectIsNull(inputRect) ? panelH - 170.0 : CGRectGetMinY(inputRect) - 96.0;
    zoneTop = MAX(panelH * 0.62, zoneTop);

    /* Some zero-state footers are siblings of the inner comment panel. Expand
       only through wrappers matching this exact panel, never through a full
       window/player root, and retain the same bottom-band geometry filter. */
    UIView *scanRoot = panel;
    for (NSUInteger depth = 0; depth < 4 && scanRoot.superview; depth++) {
        UIView *parent = scanRoot.superview;
        if ([parent isKindOfClass:UIWindow.class]) break;
        CGRect parentRect = [parent convertRect:parent.bounds toView:panel];
        if (!AWECommentRectNearlyMatchesPanel(parentRect, panel.bounds)) break;
        scanRoot = parent;
    }
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[scanRoot, @0]];
    NSUInteger index = 0;
    NSUInteger cleared = 0;
    NSMutableArray *samples = capture ? [NSMutableArray array] : nil;
    NSMutableDictionary *reasons = capture ? [NSMutableDictionary dictionary] : nil;
    NSMutableArray *candidates = capture ? [NSMutableArray array] : nil;
    while (index < queue.count && index < 420) {
        NSArray *entry = queue[index++];
        UIView *view = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        if (depth < 10) {
            for (UIView *sub in view.subviews) [queue addObject:@[sub, @(depth + 1)]];
        }
        // Include near-miss carriers BEFORE class/geometry rejection, including
        // image-backed strips. This reads geometry only, never image contents.
        if (capture && candidates.count < 16 && view != panel &&
            ![view isKindOfClass:UILabel.class] && ![panel isDescendantOfView:view]) {
            CGRect candidateRect = [view convertRect:view.bounds toView:panel];
            CGFloat cw = CGRectGetWidth(candidateRect), ch = CGRectGetHeight(candidateRect);
            if (cw >= panelW * .50 && ch >= 4.0 && ch <= 240.0 &&
                CGRectGetMaxY(candidateRect) >= panelH - 240.0 && CGRectGetMinY(candidateRect) <= panelH + 16.0) {
                NSString *filter = @"geometry-candidate";
                if (view.hidden || view.alpha < .02 || AWECommentIsOurGlassTree(view)) filter = @"hidden-or-owned-glass";
                else if ((field && (view == field || [view isDescendantOfView:field])) ||
                    [view isKindOfClass:UIControl.class] || [view isKindOfClass:UIImageView.class]) filter = @"preserved-field-or-foreground-class";
                else if (cw < panelW * .86 || ch < 7.0 || ch > 180.0) filter = @"size-outside-rule";
                else if (CGRectGetMaxY(candidateRect) < zoneTop || CGRectGetMinY(candidateRect) > panelH + 10.0) filter = @"outside-bottom-zone";
                else if (!CGRectIsNull(fieldRect) && CGRectContainsRect(CGRectInset(candidateRect, -2.0, -2.0), fieldRect) &&
                    cw < panelW * .96 && ch < 70.0) filter = @"preserved-rounded-field";
                [candidates addObject:@{@"filterReason": filter, @"view": ALBCommentCaptureView(view)}];
            }
        }
        if (view == panel || [panel isDescendantOfView:view] || view.hidden ||
            view.alpha < 0.02 || AWECommentIsOurGlassTree(view)) { ALBCommentCaptureReason(reasons, @"panel-ancestor-hidden-or-owned-glass"); continue; }
        if ((field && (view == field || [view isDescendantOfView:field])) ||
            [view isKindOfClass:UILabel.class] || [view isKindOfClass:UIControl.class] ||
            [view isKindOfClass:UIImageView.class]) { ALBCommentCaptureReason(reasons, @"preserve-field-or-foreground"); continue; }
        CGRect r = [view convertRect:view.bounds toView:panel];
        CGFloat w = CGRectGetWidth(r), h = CGRectGetHeight(r);
        if (w < panelW * 0.86 || h < 7.0 || h > 180.0) { ALBCommentCaptureReason(reasons, @"size-outside-bottom-carrier-rule"); continue; }
        if (CGRectGetMaxY(r) < zoneTop || CGRectGetMinY(r) > panelH + 10.0) { ALBCommentCaptureReason(reasons, @"outside-bottom-zone"); continue; }
        /* Preserve the actual rounded text field/pill.  We only want the
           broad empty-state footer / emoji-row backing carrier. */
        if (!CGRectIsNull(fieldRect) && CGRectContainsRect(CGRectInset(r, -2.0, -2.0), fieldRect) &&
            w < panelW * 0.96 && h < 70.0) { ALBCommentCaptureReason(reasons, @"preserve-rounded-field"); continue; }
        NSDictionary *before = (samples && samples.count < 12) ? ALBCommentCaptureView(view) : nil;
        NSUInteger previousCleared = cleared;
        ALB5RememberZeroBackdrop(view,panel);
        if ([view isKindOfClass:UIVisualEffectView.class]) {
            UIVisualEffectView *effectView = (UIVisualEffectView *)view;
            effectView.effect = nil;
            view.backgroundColor = UIColor.clearColor;
            view.layer.backgroundColor = UIColor.clearColor.CGColor;
            view.opaque = NO;
            cleared++;
        } else if (AWECommentViewHasVisibleBackdrop(view)) {
            /* The zero-state lease owns restoration. Do not also mark this
               view with the permanent panel-cover key. */
            view.backgroundColor = UIColor.clearColor;
            view.layer.backgroundColor = UIColor.clearColor.CGColor;
            view.opaque = NO;
            cleared++;
        }
        ALBCommentCapturePair(samples, view, before, cleared > previousCleared ? @"zero-bottom-carrier-cleared" : @"matched-but-no-visible-background");
    }
    if (capture) ALBReproRecordEvent(@"comment.zero.clear", @{@"evidence": evidence, @"scanRoot": ALBCommentCaptureView(scanRoot),
        @"panel": ALBCommentCaptureView(panel), @"input": ALBCommentCaptureView(input), @"field": ALBCommentCaptureView(field),
        @"zoneTop": @(zoneTop), @"scanned": @(index), @"scanLimit": @420, @"cleared": @(cleared),
        @"sampleLimit": @12, @"samples": samples, @"skippedReasons": reasons,
        @"bottomCandidates": candidates, @"candidateLimit": @16});
    if (cleared) {
        AWECommentProbe(@"COMMENT_ZERO_BOTTOM_CLEAR count=%lu input=%@ zoneTop=%.1f",
                        (unsigned long)cleared,
                        input ? NSStringFromCGRect(inputRect) : @"nil", zoneTop);
    }
}

/*
 The zero-comment composer can create one more translucent carrier as a sibling
 of AWECommentMiniEmoticonPanelView in UITextEffectsWindow.  Normal-comment
 pages usually keep that carrier inside the input container, which is why the
 original cleanup worked there.  Clear only short, nearly same-width siblings
 that overlap the emoji strip; never touch the full keyboard/effects window.
 */
static void AWECommentClearEmoticonSiblingBackdrops(UIView *panel) {
    if (!panel || !panel.window || !gAWECommentVisible) return;
    UIWindow *window = panel.window;
    CGRect panelRect = [panel convertRect:panel.bounds toView:window];
    if (CGRectIsNull(panelRect) || CGRectIsEmpty(panelRect)) return;

    NSMutableArray<NSArray *> *queue = [NSMutableArray array];
    if (panel.superview) [queue addObject:@[panel.superview, @0]];
    NSUInteger index = 0;
    NSUInteger cleared = 0;
    static char captureKey;
    BOOL capture = ALBCommentCaptureSample(panel, &captureKey);
    NSMutableArray *samples = capture ? [NSMutableArray array] : nil;
    NSMutableDictionary *reasons = capture ? [NSMutableDictionary dictionary] : nil;
    NSMutableArray *observed = capture ? [NSMutableArray array] : nil;
    while (index < queue.count && index < 140) {
        NSArray *entry = queue[index++];
        UIView *host = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        ALBCommentCaptureObserved(observed, host);
        if (depth < 2) {
            for (UIView *sub in host.subviews) [queue addObject:@[sub, @(depth + 1)]];
        }
        if (host == panel || [host isDescendantOfView:panel] || [panel isDescendantOfView:host] ||
            host.hidden || host.alpha < 0.02 || [host isKindOfClass:UILabel.class] ||
            [host isKindOfClass:UIControl.class] || [host isKindOfClass:UIImageView.class]) continue;
        CGRect r = [host convertRect:host.bounds toView:window];
        CGFloat h = CGRectGetHeight(r);
        if (h < 7.0 || h > 150.0) { ALBCommentCaptureReason(reasons, @"height-outside-sibling-rule"); continue; }
        BOOL sameBand = CGRectGetWidth(r) >= CGRectGetWidth(panelRect) * 0.88 &&
            fabs(CGRectGetMidX(r) - CGRectGetMidX(panelRect)) <= 20.0;
        CGRect overlap = CGRectIntersection(r, CGRectInset(panelRect, -12.0, -26.0));
        BOOL overlaps = !CGRectIsNull(overlap) && !CGRectIsEmpty(overlap) &&
            CGRectGetHeight(overlap) >= MIN(18.0, h * 0.30);
        if (!sameBand || !overlaps) { ALBCommentCaptureReason(reasons, @"not-same-band-or-no-overlap"); continue; }
        NSDictionary *before = (samples && samples.count < 12) ? ALBCommentCaptureView(host) : nil;
        NSUInteger previousCleared = cleared;
        if ([host isKindOfClass:UIVisualEffectView.class]) {
            ((UIVisualEffectView *)host).effect = nil;
            host.backgroundColor = UIColor.clearColor;
            host.layer.backgroundColor = UIColor.clearColor.CGColor;
            host.opaque = NO;
            cleared++;
        } else if (AWECommentViewHasVisibleBackdrop(host)) {
            AWECommentClearVisibleBackdrop(host);
            cleared++;
        }
        ALBCommentCapturePair(samples, host, before, cleared > previousCleared ? @"emoticon-sibling-cleared" : @"matched-but-no-visible-background");
    }
    if (capture) ALBReproRecordEvent(@"comment.emoji.sibling-clear", @{@"panel": ALBCommentCaptureView(panel),
        @"parent": ALBCommentCaptureView(panel.superview), @"scanned": @(index), @"scanLimit": @140,
        @"cleared": @(cleared), @"samples": samples, @"sampleLimit": @12, @"skippedReasons": reasons,
        @"localBackingViews": observed, @"localViewLimit": @48});
    if (cleared) {
        AWECommentProbe(@"COMMENT_EMOTICON_SIBLING_CLEAR count=%lu panel=%@",
                        (unsigned long)cleared, NSStringFromCGRect(panelRect));
    }
}

static void AWECommentClearEmoticonTopStrip(UIView *panel) {
    /* DYKiller confirms this mini panel lives in UITextEffectsWindow, outside
       the comment controller tree.  Gate it by the active comment/input state
       instead of responder ancestry, then clear only its backdrop carriers. */
    static char captureKey;
    BOOL capture = ALBCommentCaptureSample(panel, &captureKey);
    if (!panel || !panel.window || !AWECommentGlassFeatureEnabled() ||
        !gAWECommentVisible) {
        if (capture) ALBReproRecordEvent(@"comment.emoji.clear-skipped", @{@"panel": ALBCommentCaptureView(panel),
            @"reason": !panel.window ? @"panel-off-window" : (!gAWECommentVisible ? @"comment-not-visible" : @"glass-disabled")});
        return;
    }
    NSMutableArray *samples = capture ? [NSMutableArray array] : nil;
    NSMutableDictionary *reasons = capture ? [NSMutableDictionary dictionary] : nil;
    NSDictionary *beforePanel = capture ? ALBCommentCaptureView(panel) : nil;
    NSMutableArray *observed = capture ? [NSMutableArray array] : nil;
    AWECommentClearVisibleBackdrop(panel);
    ALBCommentCapturePair(samples, panel, beforePanel, @"emoticon-panel-background");
    AWECommentClearEmoticonSiblingBackdrops(panel);
    CGFloat width = CGRectGetWidth(panel.bounds);
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[panel, @0]];
    NSUInteger index = 0;
    NSUInteger cleared = 0;
    while (index < queue.count && index < 180) {
        NSArray *entry = queue[index++];
        UIView *view = entry[0];
        NSUInteger depth = [entry[1] unsignedIntegerValue];
        if (depth < 3) for (UIView *sub in view.subviews) [queue addObject:@[sub, @(depth + 1)]];
        ALBCommentCaptureObserved(observed, view);
        if (view == panel || [view isKindOfClass:UILabel.class] || [view isKindOfClass:UIControl.class] ||
            [view isKindOfClass:UIImageView.class]) continue;
        CGRect r = [view convertRect:view.bounds toView:panel];
        if (CGRectGetWidth(r) < width * 0.92 || CGRectGetHeight(r) < 8.0 ||
            CGRectGetHeight(r) > 100.0 || CGRectGetMinY(r) > 110.0) {
            ALBCommentCaptureReason(reasons, @"outside-emoticon-backing-geometry"); continue;
        }
        NSDictionary *before = (samples && samples.count < 12) ? ALBCommentCaptureView(view) : nil;
        if ([view isKindOfClass:UIVisualEffectView.class]) {
            ((UIVisualEffectView *)view).effect = nil;
            view.backgroundColor = UIColor.clearColor;
            view.layer.backgroundColor = UIColor.clearColor.CGColor;
            view.opaque = NO;
            cleared++;
            ALBCommentCapturePair(samples, view, before, @"emoticon-effect-cleared");
            continue;
        }
        if (AWECommentViewHasVisibleBackdrop(view)) {
            AWECommentClearVisibleBackdrop(view);
            cleared++;
            ALBCommentCapturePair(samples, view, before, @"emoticon-color-cleared");
        } else {
            ALBCommentCapturePair(samples, view, before, @"matched-but-no-visible-background");
        }
    }
    if (capture) ALBReproRecordEvent(@"comment.emoji.strip-clear", @{@"panel": ALBCommentCaptureView(panel),
        @"scanned": @(index), @"scanLimit": @180, @"cleared": @(cleared), @"samples": samples,
        @"sampleLimit": @12, @"skippedReasons": reasons, @"localBackingViews": observed, @"localViewLimit": @48});
    if (cleared) AWECommentProbe(@"COMMENT_EMOTICON_STRIP_CLEAR count=%lu frame=%@",
                                 (unsigned long)cleared, NSStringFromCGRect(panel.frame));
}


static void AWECommentSync(UIViewController *controller, NSString *reason) {
    if (!AWECommentGlassFeatureEnabled() || !controller || !controller.isViewLoaded) return;
    UIViewController *inner = AWECommentChildControllerNamed(
        controller, @"AWECommentPanelContainerSwiftImpl.CommentContainerInnerViewController");
    UIView *panel = inner.isViewLoaded ? inner.view : nil;
    if (!panel || !panel.window) {
        static char missingKey;
        if (ALBCommentCaptureSample(controller, &missingKey)) ALBReproRecordEvent(@"comment.sync-skipped",
            @{@"reason": inner ? @"panel-not-attached" : @"inner-controller-not-matched",
                @"controllerClass": NSStringFromClass(controller.class)});
        return;
    }

    Class inputClass = NSClassFromString(@"AWECommentInputViewSwiftImpl.CommentInputContainerView");
    UIView *input = inputClass ? AWECommentFindSubviewOfClass(controller.view, inputClass, 8) : nil;
    if (!input) input = AWECommentFallbackInputContainer(panel);
    UIView *field = AWECommentInputFieldSlot(input);
    static char captureKey;
    if (ALBCommentCaptureSample(controller, &captureKey)) ALBReproRecordEvent(@"comment.sync-targets",
        @{@"reason": reason ?: @"unknown", @"panel": ALBCommentCaptureView(panel),
            @"input": ALBCommentCaptureView(input), @"field": ALBCommentCaptureView(field),
            @"inputReason": input ? @"matched-native-or-fallback-container" : @"native-and-fallback-unmatched",
            @"glassMode": @(AWECommentGlassMode()), @"commentCount": @"unknown"});

    gAWECommentPanelSlot = panel;
    gAWECommentInputContainer = input;
    UIVisualEffectView *glass = AWECommentEnsurePanelGlass(panel, controller);
    AWECommentClearCovers(panel, panel, 0, 14);
    AWECommentClearInputOuterBackground(input, field);
    AWECommentClearZeroStateBottomBackdrop(panel, input, field);

    /*
     1.0.58: comments no longer rewrite text, attributed strings, icons, or
     controller traits.  Douyin owns all comment foreground colors.  Tint mode
     affects only our glass and follows the app/window appearance.
    */
    NSInteger mode = AWECommentGlassMode();
    UIUserInterfaceStyle appStyle = ALBMaterialEnabled() ? ALBMaterialStyle(panel, NO) : AWECommentSceneStyleForView(panel);
    if (ALBMaterialEnabled() && mode != 2) {
        ALBMaterialApplyEffectView(glass, NO);
    } else if (mode != 0 && glass.overrideUserInterfaceStyle != appStyle) {
        glass.overrideUserInterfaceStyle = appStyle;
    } else if (mode == 0 &&
               glass.overrideUserInterfaceStyle != UIUserInterfaceStyleUnspecified) {
        glass.overrideUserInterfaceStyle = UIUserInterfaceStyleUnspecified;
    }

    NSString *signature = [NSString stringWithFormat:@"%@|%@|%@|%@|%@|%@",
                           reason ?: @"",
                           NSStringFromCGRect(panel.frame),
                           input ? NSStringFromCGRect(input.frame) : @"nil",
                           field ? NSStringFromCGRect([field convertRect:field.bounds toView:input]) : @"nil",
                           glass.effect ? NSStringFromClass(glass.effect.class) : @"nil",
                           [NSString stringWithFormat:@"style=%ld|mode=%ld",
                            (long)AWECommentSceneStyleForView(panel), (long)AWECommentGlassMode()]];
    if (![signature isEqualToString:gAWECommentLastSyncSignature]) {
        gAWECommentLastSyncSignature = [signature copy];
        AWECommentProbe(@"COMMENT_SYNC reason=%@ panel=%@ frame=%@ input=%@ field=%@ glass=%@ style=%ld mode=%ld blocks=%lu",
                        reason ?: @"", NSStringFromClass(panel.class), NSStringFromCGRect(panel.frame),
                        input ? NSStringFromCGRect(input.frame) : @"nil",
                        field ? NSStringFromCGRect([field convertRect:field.bounds toView:input]) : @"nil",
                        glass.effect ? NSStringFromClass(glass.effect.class) : @"nil",
                        (long)AWECommentSceneStyleForView(panel), (long)AWECommentGlassMode(),
                        (unsigned long)gAWECommentGeometryBlockCount);
    }
}

static BOOL AWECommentPlayerController(UIViewController *controller) {
    if (!controller) return NO;
    static NSArray<NSString *> *names = nil;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        names = @[@"AWEAwemePlayVideoViewController",
                  @"AWEDPlayerFeedPlayerViewController",
                  @"AWEDPlayerViewController_Merge"];
    });
    for (NSString *name in names) {
        Class cls = NSClassFromString(name);
        if (cls && [controller isKindOfClass:cls]) return YES;
    }
    return NO;
}

static UIViewController *AWECommentPlayerControllerForView(UIView *view) {
    UIResponder *responder = view;
    NSInteger guard = 0;
    while (responder && guard++ < 90) {
        if ([responder isKindOfClass:UIViewController.class]) {
            UIViewController *cursor = (UIViewController *)responder;
            for (NSInteger depth = 0; cursor && depth < 8; depth++, cursor = cursor.parentViewController) {
                if (AWECommentPlayerController(cursor)) return cursor;
            }
        }
        responder = responder.nextResponder;
    }
    return nil;
}

static BOOL AWECommentIsInsidePlayInteraction(UIView *view) {
    UIResponder *r = view;
    NSInteger guard = 0;
    while (r && guard++ < 90) {
        if ([r isKindOfClass:UIViewController.class]) {
            UIViewController *vc = (UIViewController *)r;
            for (NSInteger depth = 0; vc && depth < 10; depth++, vc = vc.parentViewController) {
                if ([NSStringFromClass(vc.class) containsString:@"AWEPlayInteractionViewController"]) return YES;
            }
        }
        r = r.nextResponder;
    }
    return NO;
}

static BOOL AWECommentColorIsNearBlack(UIColor *color) {
    if (!color) return NO;
    CGFloat r=0,g=0,b=0,a=0,w=0;
    if ([color getRed:&r green:&g blue:&b alpha:&a]) {
        return a >= 0.72 && (0.2126*r + 0.7152*g + 0.0722*b) <= 0.10;
    }
    if ([color getWhite:&w alpha:&a]) return a >= 0.72 && w <= 0.10;
    return NO;
}

static void AWECommentClearTopBlackBarIfNeeded(UIView *view) {
    if (!gAWECommentVisible || !view.window) return;
    CGFloat localWidth = CGRectGetWidth(view.bounds), localHeight = CGRectGetHeight(view.bounds);
    if (localWidth < CGRectGetWidth(view.window.bounds) * 0.90 ||
        localHeight < 38.0 || localHeight > 86.0) return;
    CGRect wr = [view convertRect:view.bounds toView:view.window];
    CGFloat ww = CGRectGetWidth(view.window.bounds);
    CGFloat h = CGRectGetHeight(wr);
    if (CGRectGetWidth(wr) < ww * 0.90 || h < 38.0 || h > 86.0) return;
    if (CGRectGetMinY(wr) < -2.0 || CGRectGetMinY(wr) > 125.0) return;
    if (!AWECommentIsInsidePlayInteraction(view)) return;
    if (!AWECommentColorIsNearBlack(view.backgroundColor) &&
        !(view.layer.backgroundColor &&
          AWECommentColorIsNearBlack([UIColor colorWithCGColor:view.layer.backgroundColor]))) return;

    if (!gAWECommentTopBlackViews) gAWECommentTopBlackViews = [NSHashTable weakObjectsHashTable];
    if (!objc_getAssociatedObject(view, &kAWECommentTopBlackOriginalColorKey)) {
        objc_setAssociatedObject(view, &kAWECommentTopBlackOriginalColorKey,
                                 view.backgroundColor ?: UIColor.blackColor,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentTopBlackOriginalOpaqueKey,
                                 @(view.opaque), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [gAWECommentTopBlackViews addObject:view];
        AWECommentProbe(@"COMMENT_TOP_BLACK clear class=%@ frameWin=%@",
                        NSStringFromClass(view.class), NSStringFromCGRect(wr));
    }
    view.backgroundColor = UIColor.clearColor;
    view.layer.backgroundColor = UIColor.clearColor.CGColor;
    view.opaque = NO;
}

static void AWECommentRestoreTopBlackBars(void) {
    for (UIView *view in gAWECommentTopBlackViews.allObjects) {
        UIColor *color = objc_getAssociatedObject(view, &kAWECommentTopBlackOriginalColorKey);
        NSNumber *opaque = objc_getAssociatedObject(view, &kAWECommentTopBlackOriginalOpaqueKey);
        if (color) {
            view.backgroundColor = color;
            view.layer.backgroundColor = color.CGColor;
        }
        if (opaque) view.opaque = opaque.boolValue;
        objc_setAssociatedObject(view, &kAWECommentTopBlackOriginalColorKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentTopBlackOriginalOpaqueKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [gAWECommentTopBlackViews removeAllObjects];
}

static NSString *AWECommentMediaViewPath(UIView *view) {
    if (!view) return @"nil";
    NSMutableArray<NSString *> *parts = [NSMutableArray array];
    UIView *cursor = view;
    for (NSUInteger i = 0; cursor && i < 6; i++, cursor = cursor.superview) {
        [parts addObject:NSStringFromClass(cursor.class) ?: @"?"];
    }
    return [parts componentsJoinedByString:@"<-"];
}

static BOOL AWECommentMediaFrameIsLarge(UIView *view, UIWindow *window) {
    if (!view || !window || !view.superview || view.hidden || view.alpha < 0.02) return NO;
    CGRect wr = [view.superview convertRect:view.frame toView:window];
    CGFloat ww = CGRectGetWidth(window.bounds), wh = CGRectGetHeight(window.bounds);
    if (CGRectGetWidth(wr) < ww * 0.34 || CGRectGetHeight(wr) < wh * 0.22) return NO;
    CGRect visible = CGRectIntersection(wr, window.bounds);
    if (CGRectIsNull(visible) || CGRectIsEmpty(visible)) return NO;
    return CGRectGetWidth(visible) >= ww * 0.30 && CGRectGetHeight(visible) >= 120.0;
}

static BOOL AWECommentShouldSnapshotMediaView(UIView *view, UIWindow *window) {
    if (!view || !window || !AWECommentMediaFrameIsLarge(view, window) ||
        AWECommentViewIsInsideComment(view)) return NO;
    /* v13: only snapshot the currently-visible video player tree.  The v12
       window-wide scan also captured offscreen photo/live cells after they had
       already been shrunk by the comment transition.  Those bad baselines then
       blocked the native identity restore and even horizontal photo paging. */
    UIViewController *player = AWECommentPlayerControllerForView(view);
    if (!player) return NO;
    NSString *name = NSStringFromClass(view.class);
    return [name containsString:@"PlayerView"] || [name containsString:@"MetalView"] ||
           [name containsString:@"Danmaku"] || [name isEqualToString:@"UIView"] ||
           [name isEqualToString:@"UIImageView"] || [name containsString:@"GradientView"];
}

static void AWECommentClearMediaGeometrySnapshot(void) {
    for (UIView *view in gAWECommentMediaBaselineViews.allObjects) {
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineFrameKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineBoundsKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineCenterKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineTransformKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineKindKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineWindowFrameKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaDismissRestoreKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kALB24MediaParentKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kALB24MediaFeedLeaseKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [gAWECommentMediaBaselineViews removeAllObjects];
    gAWECommentMediaBaselineWindow = nil;
    gAWECommentBaselineHadFullscreen = NO;
}

static BOOL AWECommentWindowRectIsFullscreenLike(CGRect wr, UIWindow *window) {
    if (!window || CGRectIsNull(wr) || CGRectIsEmpty(wr)) return NO;
    CGFloat ww = CGRectGetWidth(window.bounds), wh = CGRectGetHeight(window.bounds);
    if (ww <= 0.0 || wh <= 0.0) return NO;
    BOOL widthOK = CGRectGetWidth(wr) >= ww * 0.94;
    BOOL topOK = CGRectGetMinY(wr) <= 8.0 && CGRectGetMinY(wr) >= -40.0;
    BOOL bottomOK = CGRectGetMaxY(wr) >= wh - 8.0;
    BOOL heightOK = CGRectGetHeight(wr) >= wh - 16.0;
    return widthOK && topOK && bottomOK && heightOK;
}

static void AWECommentStoreMediaBaseline(UIView *view,
                                         UIWindow *window,
                                         NSString *kind,
                                         BOOL dismissRestore) {
    if (!view || !window || !view.superview) return;
    if (!gAWECommentMediaBaselineViews) gAWECommentMediaBaselineViews = [NSHashTable weakObjectsHashTable];

    if (!objc_getAssociatedObject(view, &kAWECommentMediaBaselineFrameKey)) {
        NSMutableDictionary *lease=[NSMutableDictionary dictionary];
        UIView *cell=nil;
        for (UIView *ancestor=view.superview;ancestor && ancestor!=window;ancestor=ancestor.superview) {
            if ([ancestor isKindOfClass:UITableViewCell.class] || [ancestor isKindOfClass:UICollectionViewCell.class]) cell=ancestor;
            if ([ancestor isKindOfClass:UIScrollView.class]) {
                UIScrollView *scroll=(UIScrollView *)ancestor;
                NSHashTable *weak=[NSHashTable weakObjectsHashTable]; [weak addObject:scroll];
                lease[@"scroll"]=weak; lease[@"offset"]=[NSValue valueWithCGPoint:scroll.contentOffset];
                NSIndexPath *index=nil;
                if ([scroll isKindOfClass:UITableView.class] && [cell isKindOfClass:UITableViewCell.class])
                    index=[(UITableView *)scroll indexPathForCell:(UITableViewCell *)cell];
                else if ([scroll isKindOfClass:UICollectionView.class] && [cell isKindOfClass:UICollectionViewCell.class])
                    index=[(UICollectionView *)scroll indexPathForCell:(UICollectionViewCell *)cell];
                if (index && cell) {
                    NSHashTable *weakCell=[NSHashTable weakObjectsHashTable]; [weakCell addObject:cell];
                    lease[@"cell"]=weakCell; lease[@"index"]=index;
                }
                break;
            }
        }
        objc_setAssociatedObject(view,&kALB24MediaFeedLeaseKey,lease,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        NSHashTable *parent=[NSHashTable weakObjectsHashTable];
        [parent addObject:view.superview];
        objc_setAssociatedObject(view,&kALB24MediaParentKey,parent,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineFrameKey,
                                 [NSValue valueWithCGRect:view.frame], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineBoundsKey,
                                 [NSValue valueWithCGRect:view.bounds], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineCenterKey,
                                 [NSValue valueWithCGPoint:view.center], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineTransformKey,
                                 [NSValue valueWithCGAffineTransform:view.transform], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineKindKey,
                                 kind ?: @"player", OBJC_ASSOCIATION_COPY_NONATOMIC);
        CGRect wr = [view.superview convertRect:view.frame toView:window];
        objc_setAssociatedObject(view, &kAWECommentMediaBaselineWindowFrameKey,
                                 [NSValue valueWithCGRect:wr], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [gAWECommentMediaBaselineViews addObject:view];
    }
    if (dismissRestore) {
        objc_setAssociatedObject(view, &kAWECommentMediaDismissRestoreKey,
                                 @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

/* Comments can finish their dismissal animation before Douyin performs the
   final feed relayout.  A frame guard that only runs while the panel is visible
   therefore misses the late 956 -> 873 collapse.  Preserve the exact pre-open
   fullscreen host chain and re-assert it for a short, bounded post-dismiss
   window.  This is intentionally independent of the optional glass feature. */
static void AWECommentRestoreFullscreenAfterDismiss(UIWindow *window, NSString *reason) {
    if (!AWECommentMediaGuardEnabled() || !window ||
        !CGSizeEqualToSize(window.bounds.size,gALB24CommentWindowSize)) return;
    NSUInteger restored = 0, inspected = 0;
    CGFloat wh = CGRectGetHeight(window.bounds);

    for (UIView *view in gAWECommentMediaBaselineViews.allObjects) {
        if (!view || !view.superview || !view.window || view.window != window) continue;
        if (![objc_getAssociatedObject(view, &kAWECommentMediaDismissRestoreKey) boolValue]) continue;
        NSValue *savedFrame = objc_getAssociatedObject(view, &kAWECommentMediaBaselineFrameKey);
        NSValue *savedBounds = objc_getAssociatedObject(view, &kAWECommentMediaBaselineBoundsKey);
        NSValue *savedCenter = objc_getAssociatedObject(view, &kAWECommentMediaBaselineCenterKey);
        NSValue *savedTransform = objc_getAssociatedObject(view, &kAWECommentMediaBaselineTransformKey);
        NSValue *savedWindow = objc_getAssociatedObject(view, &kAWECommentMediaBaselineWindowFrameKey);
        if (!savedFrame || !savedWindow) continue;

        NSHashTable *parent=objc_getAssociatedObject(view,&kALB24MediaParentKey);
        if (parent.anyObject!=view.superview) continue;
        NSDictionary *lease=objc_getAssociatedObject(view,&kALB24MediaFeedLeaseKey);
        NSHashTable *weakScroll=lease[@"scroll"];
        UIScrollView *leased=weakScroll.anyObject;
        if (weakScroll && !leased) continue;
        NSValue *offset=lease[@"offset"];
        if (leased && offset && (fabs(leased.contentOffset.x-offset.CGPointValue.x)>2 ||
                                  fabs(leased.contentOffset.y-offset.CGPointValue.y)>2)) continue;
        NSIndexPath *savedIndex=lease[@"index"];
        if (savedIndex) {
            NSHashTable *weakCell=lease[@"cell"];
            UIView *cell=weakCell.anyObject;
            NSIndexPath *current=nil;
            if ([leased isKindOfClass:UITableView.class] && [cell isKindOfClass:UITableViewCell.class])
                current=[(UITableView *)leased indexPathForCell:(UITableViewCell *)cell];
            else if ([leased isKindOfClass:UICollectionView.class] && [cell isKindOfClass:UICollectionViewCell.class])
                current=[(UICollectionView *)leased indexPathForCell:(UICollectionViewCell *)cell];
            if (![current isEqual:savedIndex]) continue;
        }
        BOOL scrolling=NO;
        for (UIView *a=view.superview;a && a!=window;a=a.superview) {
            if ([a isKindOfClass:UIScrollView.class] && (((UIScrollView *)a).dragging || ((UIScrollView *)a).decelerating)) { scrolling=YES; break; }
        }
        if (scrolling) continue;
        CGRect baselineWin = savedWindow.CGRectValue;
        inspected++;

        CGRect currentWin = [view.superview convertRect:view.frame toView:window];
        CGFloat bottomLoss = CGRectGetMaxY(baselineWin) - CGRectGetMaxY(currentWin);
        CGFloat heightLoss = CGRectGetHeight(baselineWin) - CGRectGetHeight(currentWin);
        CGFloat widthLoss = CGRectGetWidth(baselineWin) - CGRectGetWidth(currentWin);
        BOOL oneTabShort = bottomLoss >= 32.0 && bottomLoss <= 125.0 &&
                           heightLoss >= 24.0 && heightLoss <= 135.0;
        BOOL materiallyShrunk = heightLoss > 24.0 || widthLoss > 18.0;
        BOOL lostBottom = wh > 0.0 && CGRectGetMaxY(currentWin) < wh - 24.0;
        CGAffineTransform baselineT = savedTransform ? savedTransform.CGAffineTransformValue : CGAffineTransformIdentity;
        CGAffineTransform currentT = view.transform;
        BOOL transformChanged = fabs(currentT.a - baselineT.a) > 0.035 ||
                                fabs(currentT.d - baselineT.d) > 0.035 ||
                                fabs(currentT.tx - baselineT.tx) > 8.0 ||
                                fabs(currentT.ty - baselineT.ty) > 12.0;
        // A portrait/letterboxed video can be translated upward without
        // changing size or transform. Compare center as well as scale.
        BOOL shifted=savedCenter && (fabs(view.center.x-savedCenter.CGPointValue.x)>8 ||
            fabs(view.center.y-savedCenter.CGPointValue.y)>8);
        BOOL movedInWindow=fabs(CGRectGetMinY(currentWin)-CGRectGetMinY(baselineWin))>12;
        if (!(oneTabShort || materiallyShrunk || transformChanged || shifted || movedInWindow)) continue;
        (void)lostBottom; // natural letterboxing is not itself a restore trigger

        if (savedTransform && !CGAffineTransformEqualToTransform(view.transform,baselineT)) view.transform = baselineT;
        if (savedBounds && !CGRectEqualToRect(view.bounds,savedBounds.CGRectValue)) view.bounds = savedBounds.CGRectValue;
        if (savedCenter && !CGPointEqualToPoint(view.center,savedCenter.CGPointValue)) view.center = savedCenter.CGPointValue;
        // frame is undefined for a transformed view. Bounds+center+transform
        // above restore the exact pose without an extra competing frame write.
        if ((!savedBounds || !savedCenter) && CGAffineTransformIsIdentity(view.transform) &&
            !CGRectEqualToRect(view.frame,savedFrame.CGRectValue)) view.frame = savedFrame.CGRectValue;
        restored++;

        if (restored <= 10) {
            AWECommentProbe(@"COMMENT_DISMISS_RECOVERY reason=%@ class=%@ currentWin=%@ baselineWin=%@ frame=%@ path=%@",
                            reason ?: @"", NSStringFromClass(view.class),
                            NSStringFromCGRect(currentWin), NSStringFromCGRect(baselineWin),
                            NSStringFromCGRect(view.frame), AWECommentMediaViewPath(view));
        }
    }
    AWECommentProbe(@"COMMENT_DISMISS_RECOVERY_PASS reason=%@ restored=%lu inspected=%lu generation=%lu",
                    reason ?: @"", (unsigned long)restored, (unsigned long)inspected,
                    (unsigned long)gAWECommentMediaSnapshotGeneration);
}

static void AWECommentScheduleDismissRecovery(void) {
    if (!AWECommentMediaGuardEnabled() || !gAWECommentMediaBaselineViews.count) {
        AWECommentClearMediaGeometrySnapshot();
        return;
    }
    NSUInteger generation = gAWECommentMediaSnapshotGeneration;
    NSUInteger presentationGeneration = gAWECommentLifecycleGeneration;
    __weak UIWindow *weakWindow = gAWECommentMediaBaselineWindow;
    NSArray<NSNumber *> *delays = @[@0.0, @0.08, @0.25, @0.55, @0.95, @1.35];
    for (NSNumber *delayValue in delays) {
        NSTimeInterval delay = delayValue.doubleValue;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            if (gAWECommentVisible || gAWECommentOwner ||
                presentationGeneration != gAWECommentLifecycleGeneration ||
                generation != gAWECommentMediaSnapshotGeneration) return;
            UIWindow *window = weakWindow;
            if (!window) return;
            AWECommentRestoreFullscreenAfterDismiss(window,
                [NSString stringWithFormat:@"dismiss+%.2f", delay]);
        });
    }
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.60 * NSEC_PER_SEC)),
                   dispatch_get_main_queue(), ^{
        if (!gAWECommentVisible && !gAWECommentOwner &&
            presentationGeneration == gAWECommentLifecycleGeneration &&
            generation == gAWECommentMediaSnapshotGeneration) {
            AWECommentClearMediaGeometrySnapshot();
            AWECommentProbe(@"COMMENT_DISMISS_RECOVERY_DONE generation=%lu", (unsigned long)generation);
        }
    });
}

static void AWECommentCaptureMediaGeometry(UIWindow *window, NSString *reason) {
    if (!window) return;
    AWECommentClearMediaGeometrySnapshot();
    gAWECommentMediaBaselineWindow = window;
    gALB24CommentWindowSize=window.bounds.size;
    if (!gAWECommentMediaBaselineViews) gAWECommentMediaBaselineViews = [NSHashTable weakObjectsHashTable];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
    NSUInteger idx = 0, captured = 0;
    while (idx < queue.count && idx < 1800 && captured < 48) {
        UIView *view = queue[idx++];
        for (UIView *sub in view.subviews) [queue addObject:sub];
        if (view == window || !AWECommentShouldSnapshotMediaView(view, window)) continue;
        NSString *kind = @"player";
        CGRect wr = [view.superview convertRect:view.frame toView:window];
        BOOL fullscreenLike = AWECommentWindowRectIsFullscreenLike(wr, window);
        /* willAppear can arrive after the native player has already lifted.
           Only an actually fullscreen, unscaled pose is safe to freeze and
           later restore. Letterboxed/already lifted media remain fully under
           native opening AND closing layout; never freeze their late pose. */
        CGAffineTransform pose = view.transform;
        if (!fullscreenLike || fabs(pose.a - 1) > .015 || fabs(pose.d - 1) > .015 ||
            fabs(pose.b) > .015 || fabs(pose.c) > .015 || fabs(pose.ty) > 2) continue;
        AWECommentStoreMediaBaseline(view, window, kind, YES);
        if (fullscreenLike) gAWECommentBaselineHadFullscreen = YES;
        {
            // A letterboxed player's parent may be the view Douyin translates.
            // Snapshot that identified media host too, not only fullscreen media.
            /* Preserve only the media-host ancestors below the cell content.
               Never rewrite AWEFeedViewCell/UITableViewCell geometry itself. */
            UIView *ancestor = view.superview;
            for (NSUInteger depth = 0; ancestor && depth < 5; depth++, ancestor = ancestor.superview) {
                NSString *ancestorName = NSStringFromClass(ancestor.class);
                if ([ancestorName containsString:@"UITableViewCellContentView"] ||
                    [ancestorName containsString:@"AWEFeedViewCell"] ||
                    [ancestor isKindOfClass:UIWindow.class]) break;
                if (!ancestor.superview) break;
                CGRect ar = [ancestor.superview convertRect:ancestor.frame toView:window];
                if (AWECommentWindowRectIsFullscreenLike(ar, window) &&
                    CGAffineTransformIsIdentity(ancestor.transform)) {
                    AWECommentStoreMediaBaseline(ancestor, window, @"dismiss-host", YES);
                }
            }
        }
        if (captured < 12) {
            AWECommentProbe(@"COMMENT_MEDIA_CAPTURE reason=%@ kind=%@ class=%@ frame=%@ win=%@ transform={%.3f %.3f %.2f %.2f} fullscreen=%d path=%@",
                            reason ?: @"", kind, NSStringFromClass(view.class), NSStringFromCGRect(view.frame),
                            NSStringFromCGRect(wr), view.transform.a, view.transform.d,
                            view.transform.tx, view.transform.ty, fullscreenLike, AWECommentMediaViewPath(view));
        }
        captured++;
    }
    gAWECommentMediaSnapshotGeneration++;
    AWECommentProbe(@"COMMENT_MEDIA_CAPTURE_DONE reason=%@ generation=%lu captured=%lu scanned=%lu fullscreen=%d restoreHosts=%lu",
                    reason ?: @"", (unsigned long)gAWECommentMediaSnapshotGeneration,
                    (unsigned long)captured, (unsigned long)idx, gAWECommentBaselineHadFullscreen,
                    (unsigned long)gAWECommentMediaBaselineViews.count);
}

static BOOL AWECommentRectChangeLooksLikePanelShrink(CGRect baseline, CGRect proposed) {
    if (CGRectIsEmpty(baseline) || CGRectIsNull(baseline)) return NO;
    CGFloat bw = MAX(1.0, CGRectGetWidth(baseline));
    CGFloat bh = MAX(1.0, CGRectGetHeight(baseline));
    BOOL shrinks = CGRectGetWidth(proposed) < bw * 0.94 || CGRectGetHeight(proposed) < bh * 0.94;
    BOOL moves = fabs(CGRectGetMinX(proposed) - CGRectGetMinX(baseline)) > 6.0 ||
                 fabs(CGRectGetMinY(proposed) - CGRectGetMinY(baseline)) > 12.0;
    return shrinks || moves;
}

static void AWECommentLogMediaGuard(UIView *view, NSString *property, NSString *from, NSString *to) {
    gAWECommentMediaGuardCount++;
    if (gAWECommentMediaGuardCount <= 40 || gAWECommentMediaGuardCount % 100 == 0) {
        AWECommentProbe(@"COMMENT_MEDIA_GUARD property=%@ kind=%@ class=%@ from=%@ to=%@ path=%@ count=%lu",
                        property ?: @"", objc_getAssociatedObject(view, &kAWECommentMediaBaselineKindKey) ?: @"?",
                        NSStringFromClass(view.class), from ?: @"", to ?: @"",
                        AWECommentMediaViewPath(view), (unsigned long)gAWECommentMediaGuardCount);
    }
}

static void AWECommentScheduleEmoticonTopStripClear(UIView *panel) {
    if (!panel) return;
    static char pendingKey;
    if([objc_getAssociatedObject(panel,&pendingKey) boolValue])return;
    objc_setAssociatedObject(panel,&pendingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIView *weakPanel = panel;
    for (NSNumber *delay in @[@0.0,@0.05,@0.18,@0.45,@0.85,@1.30]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
            (int64_t)(delay.doubleValue*NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            UIView *live = weakPanel;
            if(delay.doubleValue>=1.30)objc_setAssociatedObject(live,&pendingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            if (live.window && gAWECommentVisible) {
                AWECommentClearEmoticonTopStrip(live);
                UIView *commentPanel = gAWECommentPanelSlot;
                UIView *input = gAWECommentInputContainer;
                if (commentPanel.window) {
                    if (!input || !input.window) input = AWECommentFallbackInputContainer(commentPanel);
                    UIView *field = AWECommentInputFieldSlot(input);
                    AWECommentClearInputOuterBackground(input, field);
                    AWECommentClearZeroStateBottomBackdrop(commentPanel, input, field);
                }
            }
        });
    }
}

%hook AWECommentMiniEmoticonPanelView
- (void)didMoveToWindow {
    %orig;
    ALBCommentCaptureLifecycle((UIView *)self, ((UIView *)self).window ? @"emoji.attached" : @"emoji.detached");
    AWECommentScheduleEmoticonTopStripClear((UIView *)self);
}
- (void)didAddSubview:(UIView *)subview {
    %orig(subview);
    static char captureKey;
    if (ALBCommentCaptureSample(self, &captureKey)) ALBCommentCaptureLifecycle((UIView *)self, @"emoji.content-attached");
    AWECommentScheduleEmoticonTopStripClear((UIView *)self);
}
- (void)layoutSubviews {
    %orig;
    AWECommentClearEmoticonTopStrip((UIView *)self);
}
%end

%group AWECommentPublishGuidanceGroup
%hook AWECommentPublishGuidanceView
- (void)didMoveToWindow {
    %orig;
    UIView *guidance = (UIView *)self;
    if (!gALBCommentGuidanceViews) gALBCommentGuidanceViews = [NSHashTable weakObjectsHashTable];
    if (guidance.window) [gALBCommentGuidanceViews addObject:guidance];
    else [gALBCommentGuidanceViews removeObject:guidance];
    AWECommentClearPublishGuidanceBackdrop(guidance);
}
- (void)layoutSubviews {
    %orig;
    AWECommentClearPublishGuidanceBackdrop((UIView *)self);
}
%end
%end

%hook AWECommentContainerViewController
- (void)viewWillAppear:(BOOL)animated {
    UIViewController *controller = (UIViewController *)self;
    // Invalidate old async layout and dismissal completions even when UIKit
    // reuses the same controller instance for the next presentation.
    gAWECommentLifecycleGeneration++;
    NSUInteger generation = gAWECommentLifecycleGeneration;
    ALBCommentCaptureLifecycle(controller.isViewLoaded ? controller.view : nil, @"panel.will-appear");
    UIWindow *window = controller.view.window;
    if (!window) {
        for (UIWindow *candidate in UIApplication.sharedApplication.windows) {
            if (!candidate.hidden && candidate.alpha > 0.01) { window = candidate; break; }
        }
    }
    // A full-screen child can cover comments, setting visible to NO while the
    // same owner retains its original pre-comment pose. Returning from that
    // child (or cancelling dismissal) must not snapshot already-raised media.
    // A completed close releases owner below, even if UIKit later reuses it.
    if (AWECommentMediaGuardEnabled() && gAWECommentOwner != controller) {
        AWECommentCaptureMediaGeometry(window, @"willAppear.pre");
    }
    gAWECommentOwner = controller;
    gAWECommentVisible = YES;
    if (!AWECommentGlassFeatureEnabled() && AWECommentMediaGuardEnabled()) {
        AWECommentProbe(@"COMMENT_FULLSCREEN_GUARD glass=0 captured=%lu",
                        (unsigned long)gAWECommentMediaBaselineViews.count);
    }
    %orig(animated);
    if (AWECommentGlassFeatureEnabled()) {
        AWECommentSync(controller, @"willAppear");
        __weak UIViewController *weakController = controller;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.12 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            UIViewController *live = weakController;
            if (live && gAWECommentOwner == live && generation == gAWECommentLifecycleGeneration &&
                gAWECommentVisible) AWECommentSync(live, @"willAppear+120ms");
        });
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.36 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            UIViewController *live = weakController;
            if (live && gAWECommentOwner == live && generation == gAWECommentLifecycleGeneration &&
                gAWECommentVisible) AWECommentSync(live, @"willAppear+360ms");
        });
    }
}
- (void)viewDidLayoutSubviews {
    %orig;
    if (gAWECommentOwner == (UIViewController *)self && gAWECommentVisible &&
        AWECommentGlassFeatureEnabled()) AWECommentSync((UIViewController *)self, @"layout");
}
- (void)viewWillDisappear:(BOOL)animated {
    /* Release the guard BEFORE the native dismissal writes its return pose.
       Waiting until didDisappear blocked that return on some video types. */
    UIViewController *controller = (UIViewController *)self;
    NSUInteger generation = gAWECommentLifecycleGeneration;
    BOOL ownsPresentation = gAWECommentOwner == controller;
    if (ownsPresentation) {
        gAWECommentVisible = NO;
        objc_setAssociatedObject(controller, &kAWECommentWillDisappearGenerationKey,
                                 @(generation), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    ALBCommentCaptureLifecycle(((UIViewController *)self).viewIfLoaded, @"panel.will-disappear");
    %orig(animated);
    id<UIViewControllerTransitionCoordinator> transition = ((UIViewController *)self).transitionCoordinator;
    if (transition) {
        __weak UIViewController *weakController = controller;
        [transition animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
            UIViewController *live = weakController;
            if (!ownsPresentation || !live || gAWECommentOwner != live ||
                generation != gAWECommentLifecycleGeneration) return;
            if (context.isCancelled) {
                gAWECommentVisible = YES;
                objc_setAssociatedObject(live, &kAWECommentWillDisappearGenerationKey,
                                         nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                if (AWECommentGlassFeatureEnabled()) AWECommentSync(live, @"dismiss-cancelled");
            }
            ALBCommentCaptureLifecycle(live.viewIfLoaded,
                context.isCancelled ? @"panel.dismiss-cancelled" : @"panel.dismiss-completed");
        }];
    }
}
- (void)viewDidDisappear:(BOOL)animated {
    %orig(animated);
    UIViewController *controller = (UIViewController *)self;
    NSNumber *generation = objc_getAssociatedObject(controller, &kAWECommentWillDisappearGenerationKey);
    if (gAWECommentOwner != controller || !generation ||
        generation.unsignedIntegerValue != gAWECommentLifecycleGeneration) return;
    objc_setAssociatedObject(controller, &kAWECommentWillDisappearGenerationKey,
                             nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    gAWECommentVisible = NO;
    // A presented full-screen child can cover comments without closing them.
    // Do not reapply feed geometry behind that child as if comments dismissed.
    if (controller.presentedViewController && !controller.isBeingDismissed) return;
    gAWECommentOwner = nil;
    ALB5RestoreZeroScope(nil);
    gAWECommentPanelSlot = nil;
    gAWECommentInputContainer = nil;
    AWECommentRestoreTopBlackBars();
    ALBCommentCaptureLifecycle(((UIViewController *)self).viewIfLoaded, @"panel.did-disappear");
    /* Keep the pre-open fullscreen snapshot alive briefly. Douyin performs a
       late post-dismiss relayout after this callback on some feed types. */
    AWECommentScheduleDismissRecovery();
    AWECommentProbe(@"COMMENT_VISIBLE 0 guards=%lu fullscreen=%d generation=%lu",
                    (unsigned long)gAWECommentMediaGuardCount, gAWECommentBaselineHadFullscreen,
                    (unsigned long)gAWECommentMediaSnapshotGeneration);
}
%end

%hook AWEListKitMagicCollectionView
- (void)layoutSubviews {
    %orig;
    if (!AWECommentGlassFeatureEnabled() || !AWECommentViewIsInsideComment((UIView *)self)) return;
    UICollectionView *collection = (UICollectionView *)self;
    UIView *superview = collection.superview;
    if (!superview || CGSizeEqualToSize(superview.bounds.size, CGSizeZero)) return;
    if (!CGRectEqualToRect(collection.frame, superview.bounds)) collection.frame = superview.bounds;
    UIEdgeInsets inset = collection.contentInset;
    if (fabs(inset.bottom - 166.0) > 0.5) {
        inset.bottom = 166.0;
        collection.contentInset = inset;
        collection.scrollIndicatorInsets = inset;
    }
}
%end

// Inspect only the interaction controller's shallow top-strip subtree.
// Do not enumerate every UIView's children during Core Animation layout.
%hook AWEPlayInteractionViewController
- (void)viewDidLayoutSubviews {
    %orig;
    if (!gAWECommentVisible || !NSThread.isMainThread ||
        UIApplication.sharedApplication.applicationState != UIApplicationStateActive) return;
    UIView *root = ((UIViewController *)self).viewIfLoaded;
    if (!root.window || !AWECommentGlassFeatureEnabled()) return;
    static char lastScanKey;
    CFTimeInterval now = CACurrentMediaTime();
    NSNumber *last = objc_getAssociatedObject(root, &lastScanKey);
    if (last && now - last.doubleValue < 0.12) return;
    objc_setAssociatedObject(root, &lastScanKey, @(now), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < queue.count && i < 96; i++) {
        UIView *view = queue[i];
        AWECommentClearTopBlackBarIfNeeded(view);
        // The root plus at most two descendant levels; never a full-window scan.
        if (view == root || view.superview == root) {
            for (UIView *child in view.subviews) {
                if (queue.count >= 96) break;
                [queue addObject:child];
            }
        }
    }
}
%end

// TEST18: no responder-chain lookup or player-wide positive-X veto inside
// UIView geometry. UIKit glass/button descendants are not media targets.
// Only a view with a pre-comment snapshot can have its geometry protected.
%hook UIView
- (void)setFrame:(CGRect)frame {
    if (!gAWECommentVisible || !NSThread.isMainThread ||
        UIApplication.sharedApplication.applicationState != UIApplicationStateActive) {
        %orig(frame);
        return;
    }
    NSValue *saved = objc_getAssociatedObject(self, &kAWECommentMediaBaselineFrameKey);
    if (saved && AWECommentMediaGuardEnabled()) {
        CGRect baseline = saved.CGRectValue;
        if (AWECommentRectChangeLooksLikePanelShrink(baseline, frame)) {
            AWECommentLogMediaGuard((UIView *)self, @"frame",
                NSStringFromCGRect(baseline), NSStringFromCGRect(frame));
            %orig(baseline);
            return;
        }
    }
    %orig(frame);
}
- (void)setBounds:(CGRect)bounds {
    if (gAWECommentVisible && [NSThread isMainThread] &&
        UIApplication.sharedApplication.applicationState == UIApplicationStateActive) {
        UIView *view = (UIView *)self;
        NSValue *saved = objc_getAssociatedObject(view, &kAWECommentMediaBaselineBoundsKey);
        if (saved && AWECommentMediaGuardEnabled()) {
            CGRect baseline = saved.CGRectValue;
            CGFloat bw = MAX(1.0, CGRectGetWidth(baseline)), bh = MAX(1.0, CGRectGetHeight(baseline));
            if (CGRectGetWidth(bounds) < bw * 0.94 || CGRectGetHeight(bounds) < bh * 0.94) {
                AWECommentLogMediaGuard(view, @"bounds", NSStringFromCGRect(baseline), NSStringFromCGRect(bounds));
                %orig(baseline);
                return;
            }
        }
    }
    %orig(bounds);
}
- (void)setCenter:(CGPoint)center {
    if (gAWECommentVisible && [NSThread isMainThread] &&
        UIApplication.sharedApplication.applicationState == UIApplicationStateActive) {
        UIView *view = (UIView *)self;
        NSValue *saved = objc_getAssociatedObject(view, &kAWECommentMediaBaselineCenterKey);
        if (saved && AWECommentMediaGuardEnabled()) {
            CGPoint baseline = saved.CGPointValue;
            if (fabs(center.x - baseline.x) > 8.0 || fabs(center.y - baseline.y) > 14.0) {
                AWECommentLogMediaGuard(view, @"center", NSStringFromCGPoint(baseline), NSStringFromCGPoint(center));
                %orig(baseline);
                return;
            }
        }
    }
    %orig(center);
}
- (void)setTransform:(CGAffineTransform)transform {
    if (gAWECommentVisible && [NSThread isMainThread] &&
        UIApplication.sharedApplication.applicationState == UIApplicationStateActive) {
        UIView *view = (UIView *)self;
        NSValue *saved = objc_getAssociatedObject(view, &kAWECommentMediaBaselineTransformKey);
        if (saved && AWECommentMediaGuardEnabled()) {
            CGAffineTransform baseline = saved.CGAffineTransformValue;
            BOOL scale = fabs(transform.a - baseline.a) > 0.04 || fabs(transform.d - baseline.d) > 0.04;
            BOOL move = fabs(transform.tx - baseline.tx) > 8.0 || fabs(transform.ty - baseline.ty) > 12.0;
            if (scale || move) {
                AWECommentLogMediaGuard(view, @"transform",
                    [NSString stringWithFormat:@"{%.3f %.3f %.2f %.2f}", baseline.a, baseline.d, baseline.tx, baseline.ty],
                    [NSString stringWithFormat:@"{%.3f %.3f %.2f %.2f}", transform.a, transform.d, transform.tx, transform.ty]);
                %orig(baseline);
                return;
            }
        }
    }
    %orig(transform);
}
- (void)setBackgroundColor:(UIColor *)color {
    if(ALB5KeepZeroBackdropClear((UIView *)self))color=UIColor.clearColor;
    if (AWECommentGlassFeatureEnabled() &&
        (objc_getAssociatedObject((UIView *)self, &kAWECommentSlotOriginalColorKey) ||
         objc_getAssociatedObject((UIView *)self, &kAWECommentCoverOriginalColorKey) ||
         objc_getAssociatedObject((UIView *)self, &kAWECommentTopBlackOriginalColorKey)) &&
        AWECommentColorIsOpaque(color)) {
        %orig(UIColor.clearColor);
        return;
    }
    %orig(color);
}
%end

%hook UIVisualEffectView
- (void)setEffect:(UIVisualEffect *)effect {
    if (gAWECommentVisible && ALB5KeepZeroBackdropClear((UIView *)self)) effect = nil;
    %orig(effect);
}
%end

%hook CALayer
- (void)setBackgroundColor:(CGColorRef)color {
    if (gAWECommentVisible && NSThread.isMainThread) {
        id owner = ((CALayer *)self).delegate;
        if ([owner isKindOfClass:UIView.class] && ALB5KeepZeroBackdropClear((UIView *)owner))
            color = UIColor.clearColor.CGColor;
    }
    %orig(color);
}
%end

static BOOL AWECommentTransformLooksLikePanelShrink(CGAffineTransform transform) {
    CGFloat sx = hypot(transform.a, transform.c);
    CGFloat sy = hypot(transform.b, transform.d);
    BOOL scaledDown = sx > 0.10 && sy > 0.10 && (sx < 0.90 || sy < 0.90);
    BOOL panelTranslation = fabs(transform.ty) > 72.0 && (sx < 0.98 || sy < 0.98);
    return scaledDown || panelTranslation;
}

static void AWECommentLogPhotoLiveTransformBlock(UIView *view,
                                                  CGAffineTransform transform,
                                                  NSString *kind) {
    gAWECommentGeometryBlockCount++;
    if (gAWECommentGeometryBlockCount <= 40 || gAWECommentGeometryBlockCount % 100 == 0) {
        AWECommentProbe(@"COMMENT_PHOTO_LIVE_BLOCK kind=%@ class=%@ a=%.3f d=%.3f tx=%.2f ty=%.2f visible=%d count=%lu",
                        kind ?: @"", NSStringFromClass(view.class), transform.a, transform.d,
                        transform.tx, transform.ty, gAWECommentVisible,
                        (unsigned long)gAWECommentGeometryBlockCount);
    }
}

%group AWECommentImageGeometryGroup
%hook AWECommentImageContentView
- (void)setTransform:(CGAffineTransform)transform {
    /* v12 proved that capturing a 0.34/0.55 baseline is already too late: the
       later identity restore was then rejected, leaving photos permanently
       shrunken.  Block only the characteristic panel shrink transform and
       ALWAYS allow identity/native restoration and ordinary paging. */
    if (gAWECommentVisible && AWECommentMediaGuardEnabled() &&
        AWECommentTransformLooksLikePanelShrink(transform)) {
        AWECommentLogPhotoLiveTransformBlock((UIView *)self, transform, @"image");
        return;
    }
    %orig(transform);
}
%end
%end

%group AWECommentLivePhotoGeometryGroup
%hook AWECommentLivePhotoContentView
- (void)setTransform:(CGAffineTransform)transform {
    if (gAWECommentVisible && AWECommentMediaGuardEnabled() &&
        AWECommentTransformLooksLikePanelShrink(transform)) {
        AWECommentLogPhotoLiveTransformBlock((UIView *)self, transform, @"live");
        return;
    }
    %orig(transform);
}
%end
%end

%group AWECommentRenderOptimizeGroup
%hook AWECommentABTestSettings
+ (BOOL)enableCommentRenderingOptimize:(id)context {
    if (AWECommentGlassFeatureEnabled()) return NO;
    return %orig(context);
}
%end
%end

%group AWECommentSearchPanelGroup
%hook AWECommentSearchViewController
- (id)obtainPanelColor {
    if (AWECommentGlassFeatureEnabled() && gAWECommentVisible) return UIColor.clearColor;
    return %orig;
}
%end
%end

%ctor {
    @autoreleasepool {
        %init;
        AWECommentProbe(@"SESSION comment-glass-2.0-system-style-radius-dismiss-recovery loaded os=%@",
                        NSProcessInfo.processInfo.operatingSystemVersionString);
        Class imageClass = objc_getClass("BDMultiContentContainer.ImageContentView");
        if (imageClass) {
            %init(AWECommentImageGeometryGroup,
                  AWECommentImageContentView = imageClass);
        }
        Class liveClass = objc_getClass("AWEMultiContentImpl.LivePhotoContentAdapterView");
        if (liveClass) {
            %init(AWECommentLivePhotoGeometryGroup,
                  AWECommentLivePhotoContentView = liveClass);
        }
        Class abClass = NSClassFromString(@"AWECommentABTestSettings");
        if (abClass && [abClass respondsToSelector:@selector(enableCommentRenderingOptimize:)]) {
            %init(AWECommentRenderOptimizeGroup);
        }
        Class searchClass = NSClassFromString(@"AWECommentSearchViewController");
        if (searchClass && [searchClass instancesRespondToSelector:@selector(obtainPanelColor)]) {
            %init(AWECommentSearchPanelGroup);
        }
        if (NSClassFromString(@"AWECommentPublishGuidanceView")) {
            %init(AWECommentPublishGuidanceGroup);
        }
    }
}
