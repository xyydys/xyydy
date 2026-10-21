#import "ALBUnifiedMaterial.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>

#import <math.h>
#import "WCLGEdgeEffect.h"

static const NSInteger kAWEMessageGlassTagBase = 26082150;
static NSString * const kAWEMessageEdgeBlurDistanceDefaultsKey =
    @"AwemeLiquidBarMessageEdgeBlurDistanceV1"; /* legacy migration */
static NSString * const kAWEMessageTopEdgeBlurDistanceDefaultsKey =
    @"AwemeLiquidBarMessageTopEdgeBlurDistanceV2";
static NSString * const kAWEMessageBottomEdgeBlurDistanceDefaultsKey =
    @"AwemeLiquidBarMessageBottomEdgeBlurDistanceV2";

static CGFloat AWEMessageButtonInset(void) {
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:@"AwemeLiquidBar.TEST5.message.buttonInset"];
    double value = [stored isKindOfClass:NSNumber.class] ? [stored doubleValue] : 0;
    return isfinite(value) ? MIN(32, MAX(0, value)) : 0;
}

static CGFloat AWEMessageLegacyEdgeBlurDistance(void) {
    id stored = [NSUserDefaults.standardUserDefaults
        objectForKey:kAWEMessageEdgeBlurDistanceDefaultsKey];
    CGFloat value = [stored isKindOfClass:NSNumber.class]
        ? [(NSNumber *)stored doubleValue] : 118.0;
    return MIN(180.0, MAX(60.0, value));
}
static CGFloat AWEMessageTopEdgeBlurDistance(void) {
    id stored = [NSUserDefaults.standardUserDefaults
        objectForKey:kAWEMessageTopEdgeBlurDistanceDefaultsKey];
    CGFloat value = [stored isKindOfClass:NSNumber.class]
        ? [(NSNumber *)stored doubleValue] : AWEMessageLegacyEdgeBlurDistance();
    return MIN(188.0, MAX(40.0, value));
}
static CGFloat AWEMessageBottomEdgeBlurDistance(void) {
    id stored = [NSUserDefaults.standardUserDefaults
        objectForKey:kAWEMessageBottomEdgeBlurDistanceDefaultsKey];
    CGFloat fallback = MIN(188.0, AWEMessageLegacyEdgeBlurDistance() + 8.0);
    CGFloat value = [stored isKindOfClass:NSNumber.class]
        ? [(NSNumber *)stored doubleValue] : fallback;
    return MIN(188.0, MAX(40.0, value));
}
static __weak UIViewController *gAWEMessageActiveController = nil;
static NSHashTable<UIView *> *gAWEMessageClearedViews = nil;
static NSHashTable<UIView *> *gAWEMessageHiddenVisualViews = nil;
static NSString *gAWEMessageLastSignature = nil;
static CFTimeInterval gAWEMessageLastSurfaceProbeTime = 0.0;
static CFTimeInterval gAWEMessageLastMaintenanceTime = 0.0;

static char kAWEMessageSavedBGKey;
static char kAWEMessageSavedOpaqueKey;
static char kAWEMessageSavedHiddenKey;
static char kAWEMessageSavedAlphaKey;
static char kAWEMessageSavedEffectKey;
static char kAWEMessageSavedImageKey;
static char kAWEMessageSavedTextColorKey;
static char kAWEMessageSavedHostFrameKey;
static char kAWEMessageSavedTableFrameKey;
static char kAWEMessageSavedInsetKey;
static char kAWEMessageSavedIndicatorInsetKey;
static char kAWEMessageSavedOffsetKey;
static char kAWEMessageSavedAdjustmentKey;
static char kAWEMessageManagedKey;
static char kAWEMessageAppliedShiftKey;
static char kAWEMessageManagedHostKey;
static char kAWEMessageVisualSuppressedKey;
static char kAWEMessageEdgeSuppressedKey;
static char kAWEMessageInnerListExpandedKey;
static char kAWEMessageTopEdgeBlurKey;
static char kAWEMessageBottomEdgeBlurKey;
static char kAWEMessageEdgeBlurLoggedKey;
static char kAWEMessageAdSlotSuppressedKey;
/* iOS 27 / narrow-screen stability: once the real native right-side controls
   are found, keep their identity even after we inset them. Re-scanning with
   old 440pt absolute x thresholds made Search disappear after its first move
   and made Add impossible to discover on a 390pt device. */
static char kAWEMessageCachedMenuSourceKey;
static char kAWEMessageCachedSearchSourceKey;
static char kAWEMessageNativeImageViewKey;
static char kAWEMessageNativeImageOpacityKey;
static char kAWEMessageRenderedImageKey;
static char kAWEMessageApplyingKey;
static char kAWEMessageCachedAddSourceKey;

#ifdef __cplusplus
extern "C" BOOL ALBForwardOriginalView(UIView *source);
extern "C" BOOL ALBBindOriginalGlassAction(UIButton *button, UIView *source);
extern "C" void ALBUnbindOriginalGlassAction(UIButton *button);
#else
extern BOOL ALBForwardOriginalView(UIView *source);
extern BOOL ALBBindOriginalGlassAction(UIButton *button, UIView *source);
extern void ALBUnbindOriginalGlassAction(UIButton *button);
#endif
static void AWEMessageProbe(NSString *format, ...);

@interface AWEMessageGlassPressForwarder : NSObject
@property (nonatomic, weak) UIControl *sourceControl;
@property (nonatomic, weak) UIView *sourceView;
@property (nonatomic, weak) UIButton *glassPlate;
- (void)pressDown:(UIButton *)sender;
- (void)pressCancel:(UIButton *)sender;
- (void)pressUp:(UIButton *)sender;
@end

@implementation AWEMessageGlassPressForwarder
- (void)pressDown:(UIButton *)sender {
    self.glassPlate.highlighted = YES;
}
- (void)pressCancel:(UIButton *)sender {
    self.glassPlate.highlighted = NO;
}
- (void)pressUp:(UIButton *)sender {
BOOL sent = ALBForwardOriginalView(self.sourceControl ?: self.sourceView);
    AWEMessageProbe(@"MESSAGE_NATIVE_ACTION sent=%d source=%@", sent,
                    NSStringFromClass((self.sourceControl ?: self.sourceView).class));
}
@end

static char kAWEMessageProxyForwarderKey;
static char kAWEMessagePressGestureKey;
static char kAWEMessagePressDriverKey;

@interface AWEMessageGlassTouchDriver : NSObject
@property (nonatomic, weak) UIButton *glassPlate;
- (void)handlePress:(UILongPressGestureRecognizer *)recognizer;
@end

@implementation AWEMessageGlassTouchDriver
- (void)handlePress:(UILongPressGestureRecognizer *)recognizer {
    UIButton *plate = self.glassPlate;
    if (!plate) return;
    UIGestureRecognizerState state = recognizer.state;
    BOOL down = state == UIGestureRecognizerStateBegan ||
                state == UIGestureRecognizerStateChanged;
    plate.highlighted = down;
    if (!down) {
        dispatch_async(dispatch_get_main_queue(), ^{
            plate.highlighted = NO;
        });
    }
}
@end

static NSString *AWEMessageProbePath(void) {
    NSString *documents = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                                NSUserDomainMask,
                                                                YES) firstObject];
    return [documents stringByAppendingPathComponent:@"AwemeLiquidBar_comment_message_probe_2.0.1.log"];
}
static void AWEMessageProbe(NSString *format, ...) {
    if (format.length == 0) return;
    va_list args; va_start(args, format);
    NSString *line = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    if (!line.length) return;
    NSData *data = [[NSString stringWithFormat:@"%@ %@\n", [NSDate date], line]
                    dataUsingEncoding:NSUTF8StringEncoding];
    NSString *path = AWEMessageProbePath();
    @synchronized(NSFileManager.class) {
        NSFileManager *fm = NSFileManager.defaultManager;
        unsigned long long size = [[fm attributesOfItemAtPath:path error:nil] fileSize];
        if (size > 3 * 1024 * 1024) [fm removeItemAtPath:path error:nil];
        if (![fm fileExistsAtPath:path]) { [data writeToFile:path atomically:YES]; return; }
        NSFileHandle *handle = [NSFileHandle fileHandleForWritingAtPath:path];
        if (!handle) return;
        @try { [handle seekToEndOfFile]; [handle writeData:data]; [handle closeFile]; }
        @catch (__unused NSException *exception) { @try { [handle closeFile]; } @catch (__unused NSException *ignored) {} }
    }
}

static BOOL AWEMessageIsExactMessageController(UIViewController *controller) {
    if (!controller) return NO;
    return [NSStringFromClass(controller.class) isEqualToString:@"AWEIMMessageTabViewController"];
}
static BOOL AWEMessageFeatureActive(void) {
    return gAWEMessageActiveController && gAWEMessageActiveController.view.window;
}
static NSArray<UIView *> *AWEMessageSubviews(UIView *root, NSUInteger maxVisited) {
    if (!root) return @[];
    NSMutableArray<UIView *> *out = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    NSUInteger i = 0, visited = 0;
    while (i < queue.count && visited++ < maxVisited) {
        UIView *v = queue[i++];
        if (v != root) [out addObject:v];
        for (UIView *sub in v.subviews) [queue addObject:sub];
    }
    return out;
}
static BOOL AWEMessageIsOurGlass(UIView *view) {
    if (!view) return NO;
    if (view.tag >= kAWEMessageGlassTagBase && view.tag < kAWEMessageGlassTagBase + 40) return YES;
    NSString *identifier = view.accessibilityIdentifier ?: @"";
    return [identifier hasPrefix:@"AwemeLiquidBar.Message"];
}
static BOOL AWEMessageIsInsideOurGlassTree(UIView *view) {
    for (UIView *cursor = view; cursor; cursor = cursor.superview) {
        if (AWEMessageIsOurGlass(cursor)) return YES;
    }
    return NO;
}

static BOOL AWEMessageHasAncestorClass(UIView *view, NSString *className) {
    if (!view || className.length == 0) return NO;
    for (UIView *cursor = view.superview; cursor; cursor = cursor.superview) {
        if ([NSStringFromClass(cursor.class) isEqualToString:className]) return YES;
    }
    return NO;
}
static id AWEMessageSafeValue(id object, NSString *key) {
    @try { return [object valueForKey:key]; }
    @catch (__unused NSException *exception) { return nil; }
}

#pragma mark - State saving / restoration

static void AWEMessageRememberSurfaceView(UIView *view) {
    if (!view || AWEMessageIsInsideOurGlassTree(view)) return;
    if (!gAWEMessageClearedViews) gAWEMessageClearedViews = [NSHashTable weakObjectsHashTable];
    if (!objc_getAssociatedObject(view, &kAWEMessageSavedOpaqueKey)) {
        objc_setAssociatedObject(view, &kAWEMessageSavedBGKey,
                                 view.backgroundColor ?: [NSNull null],
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWEMessageSavedOpaqueKey,
                                 @(view.opaque), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWEMessageSavedHiddenKey,
                                 @(view.hidden), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWEMessageSavedAlphaKey,
                                 @(view.alpha), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if ([view isKindOfClass:UIVisualEffectView.class]) {
            UIVisualEffect *effect = ((UIVisualEffectView *)view).effect;
            objc_setAssociatedObject(view, &kAWEMessageSavedEffectKey,
                                     effect ?: [NSNull null],
                                     OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        [gAWEMessageClearedViews addObject:view];
    }
}
static void AWEMessageClearSurfaceView(UIView *view, BOOL hideImageBackdrop) {
    if (!view || AWEMessageIsInsideOurGlassTree(view)) return;
    AWEMessageRememberSurfaceView(view);
    view.backgroundColor = UIColor.clearColor;
    view.layer.backgroundColor = UIColor.clearColor.CGColor;
    view.opaque = NO;
    if ([view isKindOfClass:UIVisualEffectView.class]) ((UIVisualEffectView *)view).effect = nil;
    if (hideImageBackdrop && [view isKindOfClass:UIImageView.class]) view.hidden = YES;
}
static void AWEMessageRestoreSurfaceViews(void) {
    for (UIView *view in gAWEMessageClearedViews.allObjects) {
        id bg = objc_getAssociatedObject(view, &kAWEMessageSavedBGKey);
        NSNumber *opaque = objc_getAssociatedObject(view, &kAWEMessageSavedOpaqueKey);
        NSNumber *hidden = objc_getAssociatedObject(view, &kAWEMessageSavedHiddenKey);
        NSNumber *alpha = objc_getAssociatedObject(view, &kAWEMessageSavedAlphaKey);
        id effect = objc_getAssociatedObject(view, &kAWEMessageSavedEffectKey);
        UIColor *color = (bg && bg != [NSNull null]) ? bg : nil;
        view.backgroundColor = color;
        view.layer.backgroundColor = color.CGColor;
        if (opaque) view.opaque = opaque.boolValue;
        if (hidden) view.hidden = hidden.boolValue;
        if (alpha) view.alpha = alpha.doubleValue;
        if ([view isKindOfClass:UIVisualEffectView.class] && effect) {
            ((UIVisualEffectView *)view).effect = effect == [NSNull null] ? nil : effect;
        }
        objc_setAssociatedObject(view, &kAWEMessageSavedBGKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWEMessageSavedOpaqueKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWEMessageSavedHiddenKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWEMessageSavedAlphaKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(view, &kAWEMessageSavedEffectKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [gAWEMessageClearedViews removeAllObjects];
}

static void AWEMessageRememberAndHideVisual(UIView *view) {
    if (!view || AWEMessageIsInsideOurGlassTree(view)) return;
    if ([objc_getAssociatedObject(view, &kAWEMessageVisualSuppressedKey) boolValue]) return;
    objc_setAssociatedObject(view, &kAWEMessageVisualSuppressedKey, @YES,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (!gAWEMessageHiddenVisualViews) gAWEMessageHiddenVisualViews = [NSHashTable weakObjectsHashTable];
    if ([view isKindOfClass:UIImageView.class]) {
        UIImageView *iv = (UIImageView *)view;
        if (!objc_getAssociatedObject(iv, &kAWEMessageSavedImageKey)) {
            objc_setAssociatedObject(iv, &kAWEMessageSavedImageKey,
                                     iv.image ?: [NSNull null], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [gAWEMessageHiddenVisualViews addObject:iv];
        }
        iv.image = nil;
    } else if ([view isKindOfClass:UILabel.class]) {
        UILabel *label = (UILabel *)view;
        if (!objc_getAssociatedObject(label, &kAWEMessageSavedTextColorKey)) {
            objc_setAssociatedObject(label, &kAWEMessageSavedTextColorKey,
                                     label.textColor ?: [NSNull null], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            [gAWEMessageHiddenVisualViews addObject:label];
        }
        label.textColor = UIColor.clearColor;
    }
}
static void AWEMessageHideSourceArtwork(UIView *source) {
    if (!source) return;
    if ([source isKindOfClass:UIImageView.class] || [source isKindOfClass:UILabel.class]) {
        AWEMessageRememberAndHideVisual(source);
    }
    for (UIView *sub in AWEMessageSubviews(source, 80)) {
        if ([sub isKindOfClass:UIImageView.class] || [sub isKindOfClass:UILabel.class]) {
            AWEMessageRememberAndHideVisual(sub);
        }
    }
}
static void AWEMessageRestoreVisuals(void) {
    for (UIView *view in gAWEMessageHiddenVisualViews.allObjects) {
        if ([view isKindOfClass:UIImageView.class]) {
            id image = objc_getAssociatedObject(view, &kAWEMessageSavedImageKey);
            ((UIImageView *)view).image = image == [NSNull null] ? nil : image;
            objc_setAssociatedObject(view, &kAWEMessageSavedImageKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_setAssociatedObject(view, &kAWEMessageVisualSuppressedKey, nil,
                                     OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        } else if ([view isKindOfClass:UILabel.class]) {
            id color = objc_getAssociatedObject(view, &kAWEMessageSavedTextColorKey);
            ((UILabel *)view).textColor = color == [NSNull null] ? nil : color;
            objc_setAssociatedObject(view, &kAWEMessageSavedTextColorKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_setAssociatedObject(view, &kAWEMessageVisualSuppressedKey, nil,
                                     OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }
    [gAWEMessageHiddenVisualViews removeAllObjects];
}

#pragma mark - Navigation sources / plates

static UIView *AWEMessageFindNav(UIView *root) {
    for (UIView *v in AWEMessageSubviews(root, 700)) {
        if ([NSStringFromClass(v.class) isEqualToString:@"AWENavigationBar"]) return v;
    }
    return nil;
}
static UIScrollView *AWEMessageFindTable(UIView *root) {
    UIScrollView *fallback = nil; CGFloat bestArea = 0.0;
    for (UIView *v in AWEMessageSubviews(root, 1200)) {
        if (![v isKindOfClass:UIScrollView.class]) continue;
        NSString *name = NSStringFromClass(v.class);
        if ([name isEqualToString:@"AWEIMChatTabTableView"]) return (UIScrollView *)v;
        CGRect r = [v convertRect:v.bounds toView:root];
        CGFloat area = CGRectGetWidth(r) * CGRectGetHeight(r);
        if (CGRectGetWidth(r) >= CGRectGetWidth(root.bounds) * 0.90 &&
            CGRectGetHeight(r) >= 500.0 && area > bestArea) {
            fallback = (UIScrollView *)v; bestArea = area;
        }
    }
    return fallback;
}
static UIView *AWEMessageContentHost(UIScrollView *scroll, UIView *root, CGFloat navBottom) {
    UIView *cursor = scroll, *best = nil;
    NSInteger guard = 0;
    while (cursor && cursor != root && guard++ < 30) {
        UIView *parent = cursor.superview;
        if (!parent) break;
        CGRect r = [parent convertRect:parent.bounds toView:root];
        CGFloat y = CGRectGetMinY(r);
        if (y >= 48.0 && y <= navBottom + 24.0 &&
            CGRectGetWidth(r) >= CGRectGetWidth(root.bounds) * 0.90 &&
            CGRectGetHeight(r) >= 500.0) {
            best = parent;
        }
        cursor = parent;
    }
    return best;
}
static UIView *AWEMessageCompactAncestor(UIView *view, UIView *navBar) {
    if (!view || !navBar) return nil;
    UIView *best = view, *cursor = view;
    NSInteger guard = 0;
    while (cursor.superview && cursor.superview != navBar && guard++ < 12) {
        UIView *parent = cursor.superview;
        CGRect r = [parent convertRect:parent.bounds toView:navBar];
        if (CGRectGetWidth(r) <= 68.0 && CGRectGetHeight(r) <= 62.0) {
            best = parent; cursor = parent;
        } else break;
    }
    return best;
}

static UIView *AWEMessagePressGestureHost(UIView *source, UIView *navBar);

/* Release-fix: move the actual native hit owner, not just DUXBaseImageView.
   The menu artwork is nested inside a gesture host; moving only the artwork
   made the icon look correct while taps stayed at the old x coordinate. */
static UIView *AWEMessageMoveNativeTopControl(UIView *source,
                                              UIView *navBar,
                                              CGFloat targetCenterX) {
    if (!source || !navBar) return nil;
    UIView *owner = AWEMessagePressGestureHost(source, navBar);
    if (!owner) owner = AWEMessageCompactAncestor(source, navBar) ?: source;
    if (!owner.superview || owner == navBar) return owner;
    CGRect ownerRect = [owner convertRect:owner.bounds toView:navBar];
    if (CGRectGetWidth(ownerRect) > 72.0 || CGRectGetHeight(ownerRect) > 70.0) {
        return owner;
    }
    CGPoint currentInNav = [owner.superview convertPoint:owner.center toView:navBar];
    CGPoint desiredInNav = CGPointMake(targetCenterX, currentInNav.y);
    CGPoint desired = [navBar convertPoint:desiredInNav toView:owner.superview];
    if (isfinite(desired.x) && isfinite(desired.y) &&
        hypot(owner.center.x - desired.x, owner.center.y - desired.y) > 0.25) {
        owner.center = desired;
    }
    return owner;
}
static UIView *AWEMessageMenuSource(UIView *navBar) {
    UIView *cached = objc_getAssociatedObject(navBar, &kAWEMessageCachedMenuSourceKey);
    if (cached.window && [cached isDescendantOfView:navBar] &&
        !AWEMessageIsInsideOurGlassTree(cached)) return cached;
    for (UIView *v in AWEMessageSubviews(navBar, 300)) {
        if (AWEMessageIsInsideOurGlassTree(v) || v.hidden) continue;
        CGRect r = [v convertRect:v.bounds toView:navBar];
        if (CGRectGetMidX(r) < 8 || CGRectGetMidX(r) > 62 ||
            CGRectGetMidY(r) < 58 || CGRectGetMidY(r) > 112) continue;
        if ([v isKindOfClass:UIImageView.class] && ((UIImageView *)v).image) {
            objc_setAssociatedObject(navBar, &kAWEMessageCachedMenuSourceKey, v, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            return v;
        }
    }
    return nil;
}
static UIView *AWEMessageSearchSource(UIView *navBar) {
    if (!navBar) return nil;
    UIView *cached = objc_getAssociatedObject(
        navBar, &kAWEMessageCachedSearchSourceKey);
    if (cached && cached.window && !AWEMessageIsInsideOurGlassTree(cached) && [cached isDescendantOfView:navBar] &&
        !cached.hidden && cached.alpha > 0.01) {
        return cached;
    }

    CGFloat width = MAX(1.0, CGRectGetWidth(navBar.bounds));
    UIView *best = nil;
    CGFloat bestScore = -CGFLOAT_MAX;
    for (UIView *v in AWEMessageSubviews(navBar, 360)) {
        if (AWEMessageIsInsideOurGlassTree(v) || v.hidden || v.alpha <= 0.01) continue;
        NSString *name = NSStringFromClass(v.class);
        NSString *label = v.accessibilityLabel ?: @"";
        BOOL semantic =
            [name isEqualToString:@"IESIMButton"] ||
            [label containsString:@"搜索"];
        if (!semantic) continue;
        CGRect r = [v convertRect:v.bounds toView:navBar];
        CGFloat midX = CGRectGetMidX(r), midY = CGRectGetMidY(r);
        if (midX < width * 0.56 || midX > width - 18.0 ||
            midY < 58.0 || midY > 112.0) continue;
        /* Search is the left member of the two right-side controls. */
        CGFloat score = midX;
        if ([label containsString:@"搜索"]) score += 1000.0;
        if ([name isEqualToString:@"IESIMButton"]) score += 500.0;
        if (score > bestScore) { bestScore = score; best = v; }
    }
    if (best) {
        objc_setAssociatedObject(navBar, &kAWEMessageCachedSearchSourceKey,
                                 best, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return best;
}

static UIView *AWEMessageAddSource(UIView *navBar) {
    if (!navBar) return nil;
    UIView *cached = objc_getAssociatedObject(
        navBar, &kAWEMessageCachedAddSourceKey);
    if (cached && cached.window && !AWEMessageIsInsideOurGlassTree(cached) && [cached isDescendantOfView:navBar] &&
        !cached.hidden && cached.alpha > 0.01) {
        return cached;
    }

    CGFloat width = MAX(1.0, CGRectGetWidth(navBar.bounds));
    UIView *best = nil;
    CGFloat bestScore = -CGFLOAT_MAX;
    for (UIView *v in AWEMessageSubviews(navBar, 360)) {
        if (AWEMessageIsInsideOurGlassTree(v) || v.hidden || v.alpha <= 0.01) continue;
        NSString *name = NSStringFromClass(v.class);
        NSString *label = v.accessibilityLabel ?: @"";
        BOOL semantic =
            [name isEqualToString:@"AWEButton"] ||
            [label containsString:@"添加"] ||
            [label containsString:@"更多"] ||
            [label containsString:@"发布"];
        if (!semantic) continue;
        CGRect r = [v convertRect:v.bounds toView:navBar];
        CGFloat midX = CGRectGetMidX(r), midY = CGRectGetMidY(r);
        if (midX < width * 0.72 || midX > width + 8.0 ||
            midY < 58.0 || midY > 112.0) continue;
        /* Add is the right-most semantic native control. */
        CGFloat score = midX;
        if ([label containsString:@"添加"] || [label containsString:@"更多"])
            score += 1000.0;
        if ([name isEqualToString:@"AWEButton"]) score += 500.0;
        if (score > bestScore) { bestScore = score; best = v; }
    }
    if (best) {
        objc_setAssociatedObject(navBar, &kAWEMessageCachedAddSourceKey,
                                 best, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return best;
}
static UILabel *AWEMessageTitleSource(UIView *navBar) {
    for (UIView *v in AWEMessageSubviews(navBar, 240)) {
        if ([v isKindOfClass:UILabel.class] && [((UILabel *)v).text isEqualToString:@"消息"]) return (UILabel *)v;
    }
    return nil;
}
static UIButtonConfiguration *AWEMessageRegularGlassConfiguration(void);
static UIImage *AWEMessageSymbol(NSString *name, CGFloat size, UIImageSymbolWeight weight);

static UIControl *AWEMessageControlAncestor(UIView *source, UIView *navBar) {
    if (!source || !navBar) return nil;
    NSUInteger depth = 0;
    for (UIView *cursor = source; cursor && cursor != navBar && depth++ < 14; cursor = cursor.superview) {
        if ([cursor isKindOfClass:UIControl.class]) return (UIControl *)cursor;
    }
    return nil;
}

static UIView *AWEMessagePressGestureHost(UIView *source, UIView *navBar) {
    if (!source || !navBar) return nil;
    NSUInteger depth = 0;
    UIView *fallback = nil;
    for (UIView *cursor = source; cursor && cursor != navBar && depth++ < 12; cursor = cursor.superview) {
        if ([cursor isKindOfClass:UIControl.class]) return cursor;
        for (UIGestureRecognizer *gr in cursor.gestureRecognizers) {
            if ([gr isKindOfClass:UITapGestureRecognizer.class] && gr.enabled) return cursor;
        }
        CGRect r = [cursor convertRect:cursor.bounds toView:navBar];
        if (!fallback && CGRectGetWidth(r) <= 70.0 && CGRectGetHeight(r) <= 70.0 &&
            cursor.userInteractionEnabled) {
            fallback = cursor;
        }
    }
    return fallback;
}

static void AWEMessageInstallPressDriver(UIView *source, UIView *navBar, UIButton *plate) {
    UIView *host = AWEMessagePressGestureHost(source, navBar);
    if (!host || !plate) return;
    UILongPressGestureRecognizer *press =
        objc_getAssociatedObject(host, &kAWEMessagePressGestureKey);
    AWEMessageGlassTouchDriver *driver =
        objc_getAssociatedObject(host, &kAWEMessagePressDriverKey);
    if (!driver) {
        driver = [AWEMessageGlassTouchDriver new];
        objc_setAssociatedObject(host, &kAWEMessagePressDriverKey, driver,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    driver.glassPlate = plate;
    if (!press) {
        press = [[UILongPressGestureRecognizer alloc] initWithTarget:driver
                                                              action:@selector(handlePress:)];
        press.minimumPressDuration = 0.0;
        press.allowableMovement = 18.0;
        press.cancelsTouchesInView = NO;
        press.delaysTouchesBegan = NO;
        press.delaysTouchesEnded = NO;
        [host addGestureRecognizer:press];
        objc_setAssociatedObject(host, &kAWEMessagePressGestureKey, press,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

static void AWEMessageOpenGlassClipChain(UIView *navBar, UIView *root) {
    if (!navBar) return;
    NSUInteger depth = 0;
    for (UIView *cursor = navBar; cursor && depth++ < 8; cursor = cursor.superview) {
        cursor.clipsToBounds = NO;
        cursor.layer.masksToBounds = NO;
        if (cursor == root) break;
    }
}

static void AWEMessageClearCompactSourceChrome(UIView *source, UIView *navBar) {
    if (!source || !navBar) return;
    UIView *compact = AWEMessageCompactAncestor(source, navBar) ?: source;
    NSArray<UIView *> *nodes = [@[compact] arrayByAddingObjectsFromArray:AWEMessageSubviews(compact, 90)];
    CGFloat cw = MAX(1.0, CGRectGetWidth(compact.bounds));
    CGFloat ch = MAX(1.0, CGRectGetHeight(compact.bounds));
    for (UIView *v in nodes) {
        if (AWEMessageIsInsideOurGlassTree(v)) continue;
        if ([v isKindOfClass:UIImageView.class] || [v isKindOfClass:UILabel.class]) continue;
        CGRect r = [v convertRect:v.bounds toView:compact];
        BOOL fills = CGRectGetWidth(r) >= cw * 0.72 && CGRectGetHeight(r) >= ch * 0.72;
        NSString *name = NSStringFromClass(v.class) ?: @"";
        BOOL chrome = fills || [name containsString:@"Background"] ||
                      [name containsString:@"Backdrop"] ||
                      [name containsString:@"Blur"] ||
                      [name containsString:@"Effect"];
        if (!chrome) continue;
        v.backgroundColor = UIColor.clearColor;
        v.layer.backgroundColor = UIColor.clearColor.CGColor;
        v.opaque = NO;
        if ([v isKindOfClass:UIVisualEffectView.class]) {
            ((UIVisualEffectView *)v).effect = nil;
        }
    }
}

static UIUserInterfaceStyle AWEMessageResolvedAdaptiveInterfaceStyle(void) {
UIWindow *window = gAWEMessageActiveController.viewIfLoaded.window;
    UIUserInterfaceStyle style = window.windowScene.traitCollection.userInterfaceStyle;
    if (style == UIUserInterfaceStyleUnspecified) style = UIScreen.mainScreen.traitCollection.userInterfaceStyle;
    return style;
}

static void AWEMessageApplyAdaptiveStyleToGlassButton(UIButton *button) {
    ALBMaterialApplyButton(button, NO);
}

static void AWEMessageReleaseProxy(UIButton *proxy) {
    if (!proxy) return;
    ALBUnbindOriginalGlassAction(proxy);
    UIImageView *image = objc_getAssociatedObject(proxy, &kAWEMessageNativeImageViewKey);
    NSNumber *opacity = objc_getAssociatedObject(proxy, &kAWEMessageNativeImageOpacityKey);
    if (image && opacity) image.layer.opacity = opacity.floatValue;
    objc_setAssociatedObject(proxy, &kAWEMessageNativeImageViewKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(proxy, &kAWEMessageNativeImageOpacityKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [proxy cancelTrackingWithEvent:nil];
    proxy.highlighted = NO;
    proxy.hidden = YES;
}

static UIImageView *AWEMessageNativeImage(UIView *source) {
    if (!source || AWEMessageIsInsideOurGlassTree(source)) return nil;
    if ([source isKindOfClass:UIImageView.class] && ((UIImageView *)source).image) return (UIImageView *)source;
    if ([source isKindOfClass:UIButton.class] && ((UIButton *)source).imageView.image) return ((UIButton *)source).imageView;
    for (UIView *view in AWEMessageSubviews(source, 64)) {
        if (AWEMessageIsInsideOurGlassTree(view)) continue;
        if ([view isKindOfClass:UIImageView.class] && ((UIImageView *)view).image &&
            view.bounds.size.width >= 12 && view.bounds.size.width <= 52) return (UIImageView *)view;
    }
    return nil;
}

static UIButton *AWEMessageEnsurePressProxy(UIView *navBar, NSInteger index,
                                             UIView *visualSource, UIButton *unusedPlate) {
    (void)unusedPlate;
    NSInteger tag = kAWEMessageGlassTagBase + 20 + index;
    UIButton *proxy = (UIButton *)[navBar viewWithTag:tag];
    UIImageView *imageView = AWEMessageNativeImage(visualSource);
    if (!navBar || index < 0 || index > 2 || !imageView.image ||
        AWEMessageIsInsideOurGlassTree(visualSource)) {
        AWEMessageReleaseProxy(proxy);
        return nil;
    }
    UIView *actionSource = AWEMessageControlAncestor(visualSource, navBar)
        ?: AWEMessagePressGestureHost(visualSource, navBar) ?: visualSource;
    if (actionSource == proxy || AWEMessageIsInsideOurGlassTree(actionSource)) return nil;
    if (!proxy) {
        proxy = [UIButton buttonWithType:UIButtonTypeSystem];
        proxy.tag = tag;
        proxy.backgroundColor = UIColor.clearColor;
        proxy.opaque = NO;
        proxy.clipsToBounds = NO;
        proxy.accessibilityIdentifier = @"AwemeLiquidBar.MessageNativeGlassProxy.TEST2";
        [navBar addSubview:proxy];
    }
    // An inset can extend beyond the native gesture owner's old hit bounds.
    // Keep this actual control on the full nav surface and lease only its
    // one-tap recognizer; ALBUnbindOriginalGlassAction returns it on teardown.
    objc_setAssociatedObject(proxy, NSSelectorFromString(@"ALB28.KeepProxyInParent"), @YES,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (proxy.superview != navBar && !proxy.tracking) {
        ALBUnbindOriginalGlassAction(proxy);
        [navBar addSubview:proxy];
    }
    UIImageView *old = objc_getAssociatedObject(proxy, &kAWEMessageNativeImageViewKey);
    if (old != imageView) {
        AWEMessageReleaseProxy(proxy);
        objc_setAssociatedObject(proxy, &kAWEMessageNativeImageViewKey, imageView, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(proxy, &kAWEMessageNativeImageOpacityKey, @(imageView.layer.opacity), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    // Keep the original bitmap AND its displayed point size, not the asset's
    // pixel dimensions. UIKit then owns the image's press/stretch animation.
    UIImage *originalImage = imageView.image;
    CGSize targetSize = imageView.bounds.size;
    targetSize.width = MIN(32, MAX(12, targetSize.width));
    targetSize.height = MIN(32, MAX(12, targetSize.height));
    NSDictionary *cache = objc_getAssociatedObject(proxy, &kAWEMessageRenderedImageKey);
    UIImage *image = cache[@"rendered"];
    if (cache[@"source"] != originalImage || !CGSizeEqualToSize([cache[@"size"] CGSizeValue], targetSize)) {
        CGFloat scale = MIN(targetSize.width/MAX(1,originalImage.size.width), targetSize.height/MAX(1,originalImage.size.height));
        CGSize size = CGSizeMake(originalImage.size.width*scale,originalImage.size.height*scale);
        UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:targetSize];
        image = [[renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
            [originalImage drawInRect:CGRectMake((targetSize.width-size.width)/2,(targetSize.height-size.height)/2,size.width,size.height)];
        }] imageWithRenderingMode:originalImage.renderingMode];
        objc_setAssociatedObject(proxy, &kAWEMessageRenderedImageKey,
            @{@"source":originalImage,@"size":[NSValue valueWithCGSize:targetSize],@"rendered":image}, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    UIButtonConfiguration *cfg = proxy.configuration;
    if (!cfg || cfg.image != image) {
        cfg = AWEMessageRegularGlassConfiguration();
        if (!cfg) { AWEMessageReleaseProxy(proxy); return nil; }
        cfg.contentInsets = NSDirectionalEdgeInsetsZero;
        cfg.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
        cfg.image = image;
        cfg.title = nil;
        cfg.baseForegroundColor = UIColor.labelColor;
        proxy.configuration = cfg;
    }
    proxy.tintColor = UIColor.labelColor;
    proxy.accessibilityLabel = visualSource.accessibilityLabel ?: imageView.accessibilityLabel;
    AWEMessageApplyAdaptiveStyleToGlassButton(proxy);
    CGRect sourceRect = [imageView convertRect:imageView.bounds toView:navBar];
    CGRect desired = CGRectMake(CGRectGetMidX(sourceRect)-21, CGRectGetMidY(sourceRect)-21, 42, 42);
    /* Move the real proxy button, so its glass, glyph and hit rectangle all
       share one frame. The forwarding target remains the native control;
       moving that private ancestor instead would invalidate its own layout. */
    CGFloat inset = AWEMessageButtonInset();
    CGFloat middle = CGRectGetMidX(navBar.bounds);
    if (index == 0) {
        inset = MIN(inset, MAX(0, middle - 68 - CGRectGetMidX(sourceRect)));
    } else {
        // Keep the right pair together; on small screens stop before either
        // button reaches the title rather than overlapping two hit regions.
        UIImageView *searchImage = AWEMessageNativeImage(AWEMessageSearchSource(navBar));
        CGRect nearest = searchImage ? [searchImage convertRect:searchImage.bounds toView:navBar] : sourceRect;
        inset = MIN(inset, MAX(0, CGRectGetMidX(nearest) - middle - 68));
    }
    desired.origin.x += index == 0 ? inset : -inset;
    desired.origin.x = MIN(MAX(0, CGRectGetWidth(navBar.bounds) - desired.size.width), MAX(0, desired.origin.x));
    if (!proxy.tracking) {
        CGRect local = [navBar convertRect:desired toView:proxy.superview];
        if (!CGRectEqualToRect(proxy.frame, local)) proxy.frame = local;
    }
    proxy.layer.zPosition = 0;
    proxy.hidden = NO;
    proxy.userInteractionEnabled = YES;
    ALBBindOriginalGlassAction(proxy, actionSource);
    UIView *anchor = imageView;
    while (anchor.superview && anchor.superview != proxy.superview) anchor = anchor.superview;
    if (anchor.superview == proxy.superview && anchor != proxy &&
        [proxy.superview.subviews indexOfObject:proxy] != [proxy.superview.subviews indexOfObject:anchor] + 1)
        [proxy.superview insertSubview:proxy aboveSubview:anchor];
    imageView.layer.opacity = 0;
    return proxy;
}
static void AWEMessageHideAdIcon(UIView *navBar) {
    for (UIView *v in AWEMessageSubviews(navBar, 320)) {
        if (AWEMessageIsInsideOurGlassTree(v) || v.hidden) continue;
        CGRect r = [v convertRect:v.bounds toView:navBar];
        CGFloat mx = CGRectGetMidX(r), my = CGRectGetMidY(r);
        if (mx < 52.0 || mx > 112.0 || my < 68.0 || my > 104.0) continue;
        if (CGRectGetWidth(r) < 20.0 || CGRectGetWidth(r) > 48.0 ||
            CGRectGetHeight(r) < 20.0 || CGRectGetHeight(r) > 48.0) continue;
        NSString *name = NSStringFromClass(v.class);
        if (![v isKindOfClass:UIImageView.class] && ![name containsString:@"ImageView"] &&
            ![name containsString:@"Button"]) continue;
        UIView *slot = AWEMessageCompactAncestor(v, navBar) ?: v;
        AWEMessageRememberSurfaceView(slot);
        slot.hidden = YES;
        AWEMessageProbe(@"MESSAGE_AD_HIDE class=%@ frame=%@", NSStringFromClass(slot.class),
                        NSStringFromCGRect([slot convertRect:slot.bounds toView:navBar]));
        break;
    }
}

static void AWEMessageSuppressAdSlotIfNeeded(UIView *slot) {
    if (!slot || !slot.window || !AWEMessageFeatureActive()) return;
    UIViewController *controller = gAWEMessageActiveController;
    UIView *navBar = (controller && controller.isViewLoaded) ? AWEMessageFindNav(controller.view) : nil;
    if (!navBar || ![slot isDescendantOfView:navBar]) return;
    CGRect r = [slot convertRect:slot.bounds toView:navBar];
    CGFloat mx = CGRectGetMidX(r), my = CGRectGetMidY(r);
    if (mx < 48.0 || mx > 118.0 || my < 62.0 || my > 110.0) return;
    if (CGRectGetWidth(r) < 18.0 || CGRectGetWidth(r) > 52.0 ||
        CGRectGetHeight(r) < 18.0 || CGRectGetHeight(r) > 52.0) return;
    if (![objc_getAssociatedObject(slot, &kAWEMessageAdSlotSuppressedKey) boolValue]) {
        objc_setAssociatedObject(slot, &kAWEMessageAdSlotSuppressedKey, @YES,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        AWEMessageRememberSurfaceView(slot);
        AWEMessageProbe(@"MESSAGE_AD_SLOT_SUPPRESS class=%@ frame=%@",
                        NSStringFromClass(slot.class), NSStringFromCGRect(r));
    }
    slot.hidden = YES;
}

static UIButtonConfiguration *AWEMessageRegularGlassConfiguration(void) {
    return ALBMaterialButtonConfiguration();
}
static UIImage *AWEMessageSymbol(NSString *name, CGFloat size, UIImageSymbolWeight weight) {
    if (@available(iOS 13.0, *)) {
        UIImageSymbolConfiguration *cfg = [UIImageSymbolConfiguration configurationWithPointSize:size weight:weight];
        return [[UIImage systemImageNamed:name withConfiguration:cfg]
                imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
    }
    return nil;
}
static CGRect AWEMessagePlateFrame(UIView *navBar, NSInteger index) {
    CGFloat width = CGRectGetWidth(navBar.bounds);
    switch (index) {
        case 0: return CGRectMake(10.0, 65.0, 42.0, 42.0);
        case 1: return CGRectMake(MAX(0.0,width-100.0), 65.0, 42.0, 42.0);
        case 2: return CGRectMake(MAX(0.0,width-52.0), 65.0, 42.0, 42.0);
        default: return CGRectMake(round((width-78.0)*0.5), 65.0, 78.0, 42.0);
    }
}
static UIButton *AWEMessageEnsurePlate(UIView *navBar, NSInteger index) {
    NSInteger tag = kAWEMessageGlassTagBase + index;
    UIButton *button = (UIButton *)[navBar viewWithTag:tag];
    BOOL created = NO;
    if (!button || ![button isKindOfClass:UIButton.class]) {
        [button removeFromSuperview];
        button = [UIButton buttonWithType:UIButtonTypeSystem];
        button.tag = tag;
        button.userInteractionEnabled = NO;
        button.backgroundColor = UIColor.clearColor;
        button.opaque = NO;
        [navBar addSubview:button];
        created = YES;
    }
    if (created || !button.configuration) {
        UIButtonConfiguration *cfg = AWEMessageRegularGlassConfiguration();
        if (!cfg) { button.hidden = YES; return button; }
        cfg.contentInsets = NSDirectionalEdgeInsetsZero;
        cfg.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
        cfg.baseBackgroundColor = nil;
        cfg.baseForegroundColor = nil;
        cfg.subtitle = nil;
        cfg.title = nil;
        cfg.image = nil;
        button.configuration = cfg;
        button.tintColor = UIColor.clearColor;
        button.overrideUserInterfaceStyle = UIUserInterfaceStyleUnspecified;
    }
    AWEMessageApplyAdaptiveStyleToGlassButton(button);
    CGRect desired = AWEMessagePlateFrame(navBar,index);
    if (!CGRectEqualToRect(button.frame, desired)) button.frame = desired;
    button.hidden = NO;
    button.layer.zPosition = -1.0;
    return button;
}

static CGRect AWEMessagePlateFrameForSource(UIView *source, UIView *navBar, BOOL titlePlate) {
    if (!source || !navBar || !source.superview) return CGRectZero;
    CGRect r = [source convertRect:source.bounds toView:navBar];
    if (CGRectIsEmpty(r) || CGRectIsNull(r)) return CGRectZero;
    /* v16 deliberately keeps Douyin's source controls at their native frames.
       v15 moved nested IESIMButton/AWEButton instances directly and broke the
       whole header. Only the passive glass plate changes size. */
    CGFloat width = titlePlate ? 78.0 : 42.0;
    CGFloat height = 42.0;
    return CGRectMake(CGRectGetMidX(r) - width * 0.5,
                      CGRectGetMidY(r) - height * 0.5, width, height);
}

static void AWEMessagePositionBackgroundPlate(UIButton *plate, UIView *source,
                                               UIView *navBar, BOOL titlePlate) {
    if (!plate || !navBar) return;
    CGRect frame = AWEMessagePlateFrameForSource(source, navBar, titlePlate);
    if (CGRectIsEmpty(frame)) { plate.hidden = YES; return; }
    plate.frame = frame;
    plate.layer.cornerRadius = CGRectGetHeight(frame) * 0.5;
    if (@available(iOS 13.0, *)) plate.layer.cornerCurve = kCACornerCurveContinuous;
    plate.hidden = NO;
    plate.layer.zPosition = 0.0;
    UIView *anchor = source;
    while (anchor.superview && anchor.superview != navBar) anchor = anchor.superview;
    if (anchor.superview == navBar && anchor != plate) {
        [navBar insertSubview:plate belowSubview:anchor];
    } else {
        [navBar sendSubviewToBack:plate];
    }
}

static NSString *AWEMessageColorDescription(UIColor *color) {
    if (!color) return @"nil";
    UIColor *resolved = [color resolvedColorWithTraitCollection:UIScreen.mainScreen.traitCollection];
    return resolved.description ?: @"?";
}

static void AWEMessageProbeTopBackgroundCandidates(UIView *root, UIView *navBar) {
    if (!root || !navBar) return;
    CGFloat width = CGRectGetWidth(root.bounds);
    NSUInteger emitted = 0;
    for (UIView *v in AWEMessageSubviews(root, 900)) {
        if (v == root || AWEMessageIsInsideOurGlassTree(v) || v.hidden || v.alpha < 0.01) continue;
        if ([v isKindOfClass:UILabel.class] || [v isKindOfClass:UIControl.class]) continue;
        CGRect r = [v convertRect:v.bounds toView:root];
        if (CGRectGetMaxY(r) < 0.0 || CGRectGetMinY(r) > 145.0) continue;
        if (CGRectGetWidth(r) < width * 0.72 || CGRectGetHeight(r) < 18.0) continue;
        NSString *name = NSStringFromClass(v.class);
        BOOL effect = [v isKindOfClass:UIVisualEffectView.class];
        UIColor *bg = v.backgroundColor;
        if (!effect && (!bg || CGColorGetAlpha(bg.CGColor) < 0.03)) continue;
        AWEMessageProbe(@"MESSAGE_TOP_CAND class=%@ frame=%@ bg=%@ effect=%d opaque=%d z=%.1f",
                        name, NSStringFromCGRect(r), AWEMessageColorDescription(bg),
                        effect, v.opaque, v.layer.zPosition);
        if (++emitted >= 24) break;
    }
}

static NSString *AWEMessageViewPath(UIView *view, UIView *stop) {
    if (!view) return @"nil";
    NSMutableArray<NSString *> *parts = [NSMutableArray array];
    for (UIView *cursor = view; cursor && parts.count < 7; cursor = cursor.superview) {
        [parts addObject:NSStringFromClass(cursor.class) ?: @"?"];
        if (cursor == stop) break;
    }
    return [parts componentsJoinedByString:@"<-"];
}

static void AWEMessageProbeSurfaceGeometry(UIView *root, UIView *nav, UIScrollView *table,
                                           UIView *host, CGFloat shift, NSString *reason) {
    if (!root || !nav || !table) return;
    CFTimeInterval now = CACurrentMediaTime();
    if ([reason isEqualToString:@"layout"] && now - gAWEMessageLastSurfaceProbeTime < 0.25) return;
    gAWEMessageLastSurfaceProbeTime = now;
    CGRect navRoot = nav.superview ? [nav.superview convertRect:nav.frame toView:root] : nav.frame;
    CGRect tableRoot = table.superview ? [table.superview convertRect:table.frame toView:root] : table.frame;
    CGRect hostRoot = (host && host.superview) ? [host.superview convertRect:host.frame toView:root] : CGRectZero;
    NSString *rootBG = AWEMessageColorDescription(root.backgroundColor);
    NSString *tableBG = AWEMessageColorDescription(table.backgroundColor);
    NSString *windowBG = AWEMessageColorDescription(root.window.backgroundColor);
    AWEMessageProbe(@"MESSAGE_SURFACE reason=%@ navRoot=%@ tableRoot=%@ hostRoot=%@ shift=%.1f inset=%@ offset=%@ rootBG=%@ tableBG=%@ windowBG=%@ hostPath=%@",
                    reason ?: @"", NSStringFromCGRect(navRoot), NSStringFromCGRect(tableRoot),
                    host ? NSStringFromCGRect(hostRoot) : @"nil", shift,
                    NSStringFromUIEdgeInsets(table.contentInset), NSStringFromCGPoint(table.contentOffset),
                    rootBG, tableBG, windowBG, AWEMessageViewPath(host, root));
}

static void AWEMessageClearTopBackgroundCarriers(UIView *root, UIView *navBar) {
    if (!root || !navBar) return;
    /* v15 logs consistently identified the roaming veil as a translucent,
       almost full-width 440x440 UIImageView under AFDCloseFriendsPlaceHolderView.
       Do not clear its parent/root; only hide that exact visual carrier. */
    NSUInteger cleared = 0;
    CGFloat rootWidth = CGRectGetWidth(root.bounds);
    for (UIView *v in AWEMessageSubviews(root, 1000)) {
        if (v == root || v == navBar || AWEMessageIsInsideOurGlassTree(v) || v.hidden) continue;
        if ([objc_getAssociatedObject(v, &kAWEMessageVisualSuppressedKey) boolValue]) continue;
        if (![v isKindOfClass:UIImageView.class]) continue;
        if (!AWEMessageHasAncestorClass(v, @"AFDCloseFriendsPlaceHolderView")) continue;
        CGRect r = [v convertRect:v.bounds toView:root];
        CGFloat w = CGRectGetWidth(r), h = CGRectGetHeight(r);
        UIColor *bg = v.backgroundColor;
        CGFloat alpha = (bg && bg.CGColor) ? CGColorGetAlpha(bg.CGColor) : 0.0;
        BOOL geometry = w >= rootWidth * 0.92 && h >= 320.0 && h <= 560.0;
        BOOL translucent = alpha > 0.01 && alpha < 0.22;
        if (!geometry || !translucent) continue;
        AWEMessageRememberAndHideVisual(v);
        if (cleared++ < 4) {
            AWEMessageProbe(@"MESSAGE_V17_VEIL_HIDE class=%@ frame=%@ alpha=%.3f path=%@",
                            NSStringFromClass(v.class), NSStringFromCGRect(r), alpha,
                            AWEMessageViewPath(v, root));
        }
    }
}

static void AWEMessageClearHeaderHairline(UIView *root, UIView *navBar) {
    if (!root || !navBar) return;
    CGFloat width = CGRectGetWidth(root.bounds);
    for (UIView *v in AWEMessageSubviews(root, 800)) {
        if (v == root || v == navBar || AWEMessageIsInsideOurGlassTree(v) ||
            v.hidden || !v.superview) continue;
        if ([v isKindOfClass:UIControl.class] || [v isKindOfClass:UILabel.class] ||
            [v isKindOfClass:UIImageView.class]) continue;
        CGRect r = [v.superview convertRect:v.frame toView:root];
        if (CGRectGetWidth(r) >= width * 0.88 &&
            CGRectGetHeight(r) > 0.0 && CGRectGetHeight(r) <= 1.5 &&
            CGRectGetMinY(r) >= 100.0 && CGRectGetMaxY(r) <= 112.0) {
            AWEMessageClearSurfaceView(v, NO);
            v.alpha = 0.0;
            AWEMessageProbe(@"MESSAGE_V17_HAIRLINE_HIDE class=%@ frame=%@",
                            NSStringFromClass(v.class), NSStringFromCGRect(r));
        }
    }
}

static BOOL AWEMessageIsScrollEdgeEffectView(UIView *view) {
    if (!view) return NO;
    NSString *name = NSStringFromClass(view.class) ?: @"";
    return [name containsString:@"ScrollEdgeEffectView"];
}

static void AWEMessageSuppressRuntimeEdgeView(UIView *view) {
    if (!view || !AWEMessageFeatureActive()) return;
    UIWindow *messageWindow = gAWEMessageActiveController.view.window;
    if (!messageWindow || view.window != messageWindow || !view.superview) return;
    CGRect wr = [view.superview convertRect:view.frame toView:messageWindow];
    CGFloat ww = CGRectGetWidth(messageWindow.bounds);
    CGFloat wh = CGRectGetHeight(messageWindow.bounds);
    BOOL wide = CGRectGetWidth(wr) >= ww * 0.88;
    BOOL edgeHeight = CGRectGetHeight(wr) >= 28.0 && CGRectGetHeight(wr) <= 220.0;
    BOOL nearEdge = CGRectGetMinY(wr) <= 170.0 || CGRectGetMaxY(wr) >= wh - 190.0;
    if (!wide || !edgeHeight || !nearEdge) return;
    if (![objc_getAssociatedObject(view, &kAWEMessageEdgeSuppressedKey) boolValue]) {
        objc_setAssociatedObject(view, &kAWEMessageEdgeSuppressedKey, @YES,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        AWEMessageProbe(@"MESSAGE_V19_EDGE_EFFECT_HIDE class=%@ frame=%@",
                        NSStringFromClass(view.class), NSStringFromCGRect(wr));
    }
    view.hidden = YES;
    view.alpha = 0.0;
    view.userInteractionEnabled = NO;
    view.backgroundColor = UIColor.clearColor;
    view.layer.backgroundColor = UIColor.clearColor.CGColor;
}

static void AWEMessageDisableScrollEdgeEffects(UIView *root) {
    if (!root) return;
    UIView *scope = root.window ?: root;
    NSUInteger hidden = 0;
    for (UIView *v in AWEMessageSubviews(scope, 2600)) {
        if (!AWEMessageIsScrollEdgeEffectView(v)) continue;
        if ([objc_getAssociatedObject(v, &kAWEMessageEdgeSuppressedKey) boolValue]) continue;
        CGRect r = v.superview ? [v.superview convertRect:v.frame toView:scope] : CGRectZero;
        AWEMessageSuppressRuntimeEdgeView(v);
        if (v.hidden && hidden++ < 6) {
            AWEMessageProbe(@"MESSAGE_V19_EDGE_SCAN_HIT class=%@ frame=%@",
                            NSStringFromClass(v.class), NSStringFromCGRect(r));
        }
    }
}

#pragma mark - Transparent nav + full surface

static void AWEMessageClearNavigationBackground(UIView *navBar) {
    if (!navBar) return;
    AWEMessageClearSurfaceView(navBar, NO);
    CGFloat w = CGRectGetWidth(navBar.bounds), h = CGRectGetHeight(navBar.bounds);
    for (UIView *v in AWEMessageSubviews(navBar, 420)) {
        if (AWEMessageIsInsideOurGlassTree(v)) continue;
        CGRect r = [v convertRect:v.bounds toView:navBar];
        NSString *name = NSStringFromClass(v.class);
        BOOL full = CGRectGetWidth(r) >= w*0.86 && CGRectGetHeight(r) >= h*0.45;
        BOOL row = CGRectGetMinY(r) >= 48.0 && CGRectGetWidth(r) >= w*0.90 && CGRectGetHeight(r) >= 36.0;
        BOOL named = [name containsString:@"Background"] || [name containsString:@"Backdrop"] ||
                     [name containsString:@"BarBackground"];
        if ((full || row || named) && ![v isKindOfClass:UIControl.class] && ![v isKindOfClass:UILabel.class]) {
            AWEMessageClearSurfaceView(v, [v isKindOfClass:UIImageView.class]);
        }
    }
}


static void AWEMessageExtendInnerContentV20(UIView *root, UIScrollView *outer,
                                             CGFloat effectiveShift) {
    if (!root || !outer || effectiveShift < 40.0) return;
    CGFloat rootW = CGRectGetWidth(root.bounds), rootH = CGRectGetHeight(root.bounds);
    NSUInteger changed = 0;
    for (UIView *v in AWEMessageSubviews(root, 1800)) {
        if (![v isKindOfClass:UIScrollView.class] || !v.superview) continue;
        if (v == outer) continue;
        NSString *name = NSStringFromClass(v.class) ?: @"";
        if (![name containsString:@"AWEIM"]) continue;
        CGRect rr = [v convertRect:v.bounds toView:root];
        CGFloat gap = rootH - CGRectGetMaxY(rr);
        BOOL fullWidth = CGRectGetWidth(rr) >= rootW * 0.88;
        BOOL substantial = CGRectGetHeight(rr) >= 360.0;
        BOOL sameMissingBand = gap >= effectiveShift - 18.0 &&
                               gap <= effectiveShift + 18.0;
        if (!fullWidth || !substantial || !sameMissingBand) continue;

        CGPoint bottomInParent =
            [v.superview convertPoint:CGPointMake(0.0, rootH) fromView:root];
        CGFloat localGap = bottomInParent.y - CGRectGetMaxY(v.frame);
        if (localGap < 40.0 || localGap > 140.0) continue;

        CGRect before = v.frame;
        CGRect desired = before;
        desired.size.height += localGap;
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        v.frame = desired;
        v.clipsToBounds = NO;
        v.layer.masksToBounds = NO;
        UIScrollView *inner = (UIScrollView *)v;
        UIEdgeInsets inset = inner.contentInset;
        UIEdgeInsets indicator = inner.verticalScrollIndicatorInsets;
        inset.bottom = 0.0;
        indicator.bottom = 0.0;
        inner.contentInset = inset;
        inner.verticalScrollIndicatorInsets = indicator;
        [CATransaction commit];

        /* The 850pt inner list can itself sit inside one or more 850pt
           anonymous wrappers. Extend only wrappers sharing the exact missing
           106pt bottom band, stopping at the already-full outer list. */
        for (UIView *cursor = v.superview;
             cursor && cursor != root && cursor.superview;
             cursor = cursor.superview) {
            CGRect cr = [cursor convertRect:cursor.bounds toView:root];
            CGFloat cg = rootH - CGRectGetMaxY(cr);
            if (CGRectGetWidth(cr) < rootW * 0.86) continue;
            if (cg >= effectiveShift - 18.0 && cg <= effectiveShift + 18.0) {
                CGPoint p = [cursor.superview convertPoint:CGPointMake(0.0, rootH)
                                                   fromView:root];
                CGFloat lg = p.y - CGRectGetMaxY(cursor.frame);
                if (lg >= 40.0 && lg <= 140.0) {
                    CGRect cf = cursor.frame;
                    cf.size.height += lg;
                    cursor.frame = cf;
                    cursor.clipsToBounds = NO;
                    cursor.layer.masksToBounds = NO;
                }
            }
        }

        if (![objc_getAssociatedObject(v, &kAWEMessageInnerListExpandedKey) boolValue]) {
            objc_setAssociatedObject(v, &kAWEMessageInnerListExpandedKey, @YES,
                                     OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            AWEMessageProbe(@"MESSAGE_V20_INNER_EXTEND class=%@ before=%@ after=%@ gap=%.1f",
                            name, NSStringFromCGRect(before),
                            NSStringFromCGRect(v.frame), localGap);
        }
        changed++;
    }
    if (changed > 0) {
        outer.clipsToBounds = NO;
        outer.layer.masksToBounds = NO;
    }
}


static WCLGEdgeEffectView *AWEMessageEnsureEdgeBlur(UIView *root, BOOL top) {
    if (!root) return nil;
    const void *key = top ? &kAWEMessageTopEdgeBlurKey : &kAWEMessageBottomEdgeBlurKey;
    WCLGEdgeEffectView *effect = objc_getAssociatedObject(root, key);
    if (!effect) {
        effect = [[WCLGEdgeEffectView alloc] initWithFrame:CGRectZero];
        effect.userInteractionEnabled = NO;
        effect.accessibilityIdentifier = top
            ? @"AwemeLiquidBar.Message.EdgeBlur.Top.1.0.58"
            : @"AwemeLiquidBar.Message.EdgeBlur.Bottom.1.0.58";
        effect.tag = kAWEMessageGlassTagBase + (top ? 32 : 33);
        objc_setAssociatedObject(root, key, effect, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [root addSubview:effect];
    } else if (effect.superview != root) {
        [effect removeFromSuperview];
        [root addSubview:effect];
    }
    return effect;
}

static void AWEMessageUpdateEdgeBlurRelease(UIView *root, UIView *nav,
                                         UIScrollView *table) {
    if (!root || !root.window) return;
    CGFloat width = CGRectGetWidth(root.bounds), height = CGRectGetHeight(root.bounds);
    if (width < 10.0 || height < 100.0) return;

    /*
     1.0.58 release: blur-only edge treatment.  No CALayer mask is applied to
     the message scroll view, so scrolling/layout cannot be affected by an
     opacity fade.  One user-facing distance parameter controls the visual
     reach while the proven WCG variable-blur implementation stays passive.
    */
    const CGFloat topDistance = AWEMessageTopEdgeBlurDistance();
    const CGFloat bottomDistance = AWEMessageBottomEdgeBlurDistance();
    const CGFloat topHeight = topDistance;
    const CGFloat bottomHeight = bottomDistance;
    const CGFloat topAlpha = 0.14;
    const CGFloat bottomAlpha = 0.10;
    const CGFloat blurRadius = 1.0;
    UIColor *contentColor = table.backgroundColor ?: root.backgroundColor;
    if (!contentColor) {
        if (@available(iOS 13.0, *)) contentColor = UIColor.systemBackgroundColor;
        else contentColor = UIColor.whiteColor;
    }

    WCLGEdgeEffectView *top = AWEMessageEnsureEdgeBlur(root, YES);
    WCLGEdgeEffectView *bottom = AWEMessageEnsureEdgeBlur(root, NO);
    top.frame = CGRectMake(0.0, 0.0, width, MIN(topHeight, height));
    bottom.frame = CGRectMake(0.0, MAX(0.0, height - bottomHeight),
                              width, MIN(bottomHeight, height));
    [top updateWithContentColor:contentColor blur:YES alpha:topAlpha
                           rect:top.bounds edge:WCLGEdgeEffectEdgeTop
                      edgeInset:0.0 transitionLength:MAX(44.0, topDistance * 0.78)
                     blurRadius:blurRadius];
    [bottom updateWithContentColor:contentColor blur:YES alpha:bottomAlpha
                              rect:bottom.bounds edge:WCLGEdgeEffectEdgeBottom
                         edgeInset:0.0 transitionLength:MAX(48.0, bottomDistance * 0.92)
                        blurRadius:blurRadius];

    UIView *topNav = nav;
    while (topNav && topNav.superview && topNav.superview != root) {
        topNav = topNav.superview;
    }
    if (topNav && topNav.superview == root) {
        [root insertSubview:top belowSubview:topNav];
        [root insertSubview:bottom belowSubview:topNav];
    } else {
        [root bringSubviewToFront:top];
        [root bringSubviewToFront:bottom];
    }

    NSString *signature = [NSString stringWithFormat:@"%.0f|%.0f|%.2f|%.2f|%.1f",
                           topHeight, bottomHeight, topAlpha, bottomAlpha, blurRadius];
    NSString *oldSignature = objc_getAssociatedObject(
        root, &kAWEMessageEdgeBlurLoggedKey);
    if (![oldSignature isEqualToString:signature]) {
        AWEMessageProbe(@"MESSAGE_1_0_57_EDGE_BLUR topDistance=%.0f bottomDistance=%.0f top=%.0f/%.2f bottom=%.0f/%.2f blur=%.1f variable=%d",
                        topDistance, bottomDistance, topHeight, topAlpha,
                        bottomHeight, bottomAlpha, blurRadius,
                        WCLGEdgeVariableBlurIsAvailable());
        objc_setAssociatedObject(root, &kAWEMessageEdgeBlurLoggedKey,
                                 signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
    }
}

/* TEST8: reinforce existing rounded groups in the message list only. The
   native header, avatars, unread badges and ordinary row backgrounds keep
   their existing rendering. A semantic fill follows the actual screen mode. */
static char kALB8MessageSectionKey, kALB8MessageSectionLabelKey;
static NSHashTable<UIView *> *ALB8MessageSections;
static UIUserInterfaceStyle ALB8MessageLastSectionStyle=(UIUserInterfaceStyle)-1;
static BOOL ALB8MessageIsRoundedGroup(UIView *view, UIScrollView *table) {
    if(!view || view==table || view.hidden || view.alpha<.01 ||
       AWEMessageIsInsideOurGlassTree(view) || [view isKindOfClass:UIImageView.class] ||
       [view isKindOfClass:UIControl.class] || [view isKindOfClass:UILabel.class] ||
       [view isKindOfClass:UIScrollView.class])return NO;
    CGFloat width=CGRectGetWidth(view.bounds),height=CGRectGetHeight(view.bounds);
    if(width<CGRectGetWidth(table.bounds)*.58 || height<28 || height>360 ||
       view.layer.cornerRadius<9)return NO;
    NSString *name=NSStringFromClass(view.class).lowercaseString;
    BOOL carrier=[name containsString:@"background"] || [name containsString:@"section"] ||
                 [name containsString:@"group"] || [name containsString:@"card"];
    // A plain UIView is accepted only when it already has a visible rounded fill.
    CGFloat alpha=view.backgroundColor?CGColorGetAlpha(view.backgroundColor.CGColor):0;
    return carrier || ([view isMemberOfClass:UIView.class] && alpha>.015 && alpha<.95);
}
static UIColor *ALB8MessageSectionColor(void) {
    static UIColor *fill;static dispatch_once_t once;
    dispatch_once(&once, ^{ fill=[UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *traits) {
        return traits.userInterfaceStyle==UIUserInterfaceStyleDark
            ? [UIColor colorWithWhite:.16 alpha:.48] : [UIColor colorWithWhite:.97 alpha:.56];
    }]; });
    return fill;
}
static void ALB8MessageApplySection(UIView *view, UIUserInterfaceStyle style) {
    if(!objc_getAssociatedObject(view,&kALB8MessageSectionKey)) {
        objc_setAssociatedObject(view,&kALB8MessageSectionKey,@[view.backgroundColor ?: (id)NSNull.null,@(view.overrideUserInterfaceStyle)],OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if(!ALB8MessageSections)ALB8MessageSections=[NSHashTable weakObjectsHashTable];
        [ALB8MessageSections addObject:view];
    }
    view.overrideUserInterfaceStyle=style;
    UIColor *fill=ALB8MessageSectionColor();view.backgroundColor=fill;view.opaque=NO;
    // Neutral text can safely follow the reinforced backing. Preserve colored
    // status, link and unread text rather than repainting the entire subtree.
    for(UIView *child in AWEMessageSubviews(view,72)) {
        if(![child isKindOfClass:UILabel.class] || AWEMessageIsInsideOurGlassTree(child))continue;
        UILabel *label=(UILabel *)child;
        UIColor *old=objc_getAssociatedObject(label,&kALB8MessageSectionLabelKey) ?: label.textColor;
        UIColor *resolved=[old resolvedColorWithTraitCollection:label.traitCollection];
        CGFloat r=0,g=0,b=0,a=0,w=0;
        BOOL neutral=[resolved getRed:&r green:&g blue:&b alpha:&a] && MAX(r,MAX(g,b))-MIN(r,MIN(g,b))<.06;
        if(!neutral && [resolved getWhite:&w alpha:&a])neutral=YES;
        if(!neutral || a<.1 || label.attributedText.length)continue;
        if(!objc_getAssociatedObject(label,&kALB8MessageSectionLabelKey))objc_setAssociatedObject(label,&kALB8MessageSectionLabelKey,old,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        label.textColor=a<.75?UIColor.secondaryLabelColor:UIColor.labelColor;
    }
}
static void ALB8MessageRefreshSections(UIScrollView *table, BOOL discover) {
    UIUserInterfaceStyle style=AWEMessageResolvedAdaptiveInterfaceStyle();
    BOOL recolor=style!=ALB8MessageLastSectionStyle;ALB8MessageLastSectionStyle=style;
    if(discover)for(UIView *view in AWEMessageSubviews(table,640)) {
        if(ALB8MessageIsRoundedGroup(view,table))ALB8MessageApplySection(view,style);
    }
    for(UIView *view in ALB8MessageSections.allObjects) {
        if(!view.window || ![view isDescendantOfView:table])continue;
        if(discover || recolor)ALB8MessageApplySection(view,style);
        else { UIColor *fill=ALB8MessageSectionColor();if(![view.backgroundColor isEqual:fill])view.backgroundColor=fill; }
    }
}
static void ALB8MessageRestoreSections(void) {
    for(UIView *view in ALB8MessageSections.allObjects) {
        NSArray *old=objc_getAssociatedObject(view,&kALB8MessageSectionKey);
        if(old.count==2) {view.backgroundColor=old[0]==NSNull.null?nil:old[0];view.overrideUserInterfaceStyle=(UIUserInterfaceStyle)[old[1] integerValue];}
        objc_setAssociatedObject(view,&kALB8MessageSectionKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        for(UIView *child in AWEMessageSubviews(view,72))if([child isKindOfClass:UILabel.class]) {
            UIColor *ink=objc_getAssociatedObject(child,&kALB8MessageSectionLabelKey);
            if(ink) {((UILabel *)child).textColor=ink;objc_setAssociatedObject(child,&kALB8MessageSectionLabelKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);}
        }
    }
    [ALB8MessageSections removeAllObjects];
}

static void AWEMessageApplyFullSurfaceBody(UIViewController *controller) {
    if (!AWEMessageIsExactMessageController(controller) ||
        !controller.isViewLoaded || !controller.view.window) return;

    UIView *root = controller.view;
    UIView *nav = AWEMessageFindNav(root);
    UIScrollView *table = AWEMessageFindTable(root);
    if (!nav || !table) return;

    CGRect navRoot = [nav.superview convertRect:nav.frame toView:root];
    CGFloat navBottom = CGRectGetMaxY(navRoot);
    UIView *host = AWEMessageContentHost(table, root, navBottom);
    UIView *managedHost = objc_getAssociatedObject(table, &kAWEMessageManagedHostKey);
    if (managedHost && managedHost.window == root.window &&
        [table isDescendantOfView:managedHost]) {
        host = managedHost;
    }

    BOOL already = [objc_getAssociatedObject(table, &kAWEMessageManagedKey) boolValue];
    CFTimeInterval maintenanceNow = CACurrentMediaTime();
    BOOL doMaintenance = !already ||
                         maintenanceNow - gAWEMessageLastMaintenanceTime >= 1.25;
    if (doMaintenance) gAWEMessageLastMaintenanceTime = maintenanceNow;
    ALB8MessageRefreshSections(table, doMaintenance);

    /* Keep the message root itself intact. Expensive hierarchy maintenance is
       deliberately rate-limited. v17 did several 800-1400-node walks from every
       viewDidLayoutSubviews and produced >12k log lines in ~18 seconds. */
    if (doMaintenance) {
        AWEMessageClearNavigationBackground(nav);
        AWEMessageHideAdIcon(nav);
    }

    UIView *menu = AWEMessageMenuSource(nav);
    UIView *search = AWEMessageSearchSource(nav);
    UIView *add = AWEMessageAddSource(nav);
    UILabel *title = AWEMessageTitleSource(nav);

    /* Native owners stay at their own frames. Only the actual action proxies
       move inward, retaining native-action forwarding at the displayed spot. */
    UIButton *menuPlate = AWEMessageEnsurePlate(nav, 0);
    UIButton *searchPlate = AWEMessageEnsurePlate(nav, 1);
    UIButton *addPlate = AWEMessageEnsurePlate(nav, 2);
    UIButton *titlePlate = AWEMessageEnsurePlate(nav, 3);

    AWEMessageOpenGlassClipChain(nav, root);
    NSArray *sources = @[menu ?: (id)NSNull.null, search ?: (id)NSNull.null, add ?: (id)NSNull.null];
    NSArray *plates = @[menuPlate, searchPlate, addPlate];
    for (NSInteger i=0; i<3; i++) {
        UIView *source = sources[i] == NSNull.null ? nil : sources[i];
        UIButton *plate = plates[i];
        if (!source) {
            plate.hidden = YES;
            AWEMessageReleaseProxy((UIButton *)[nav viewWithTag:kAWEMessageGlassTagBase + 20 + i]);
            continue;
        }
        UIView *oldHost = AWEMessagePressGestureHost(source, nav);
        UIGestureRecognizer *old = objc_getAssociatedObject(oldHost, &kAWEMessagePressGestureKey);
        if (old) { [oldHost removeGestureRecognizer:old]; objc_setAssociatedObject(oldHost, &kAWEMessagePressGestureKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
        UIButton *proxy = AWEMessageEnsurePressProxy(nav, i, source, plate);
        plate.hidden = proxy != nil;
        if (!proxy) AWEMessagePositionBackgroundPlate(plate, source, nav, NO);
    }
    AWEMessagePositionBackgroundPlate(titlePlate, title, nav, YES);
    if (doMaintenance) {
        AWEMessageClearCompactSourceChrome(menu, nav);
        AWEMessageClearCompactSourceChrome(search, nav);
        AWEMessageClearCompactSourceChrome(add, nav);
    }

    NSNumber *storedShift = objc_getAssociatedObject(table, &kAWEMessageAppliedShiftKey);
    CGFloat effectiveShift = storedShift ? storedShift.doubleValue : 0.0;

    if (!already) {
        objc_setAssociatedObject(table, &kAWEMessageSavedInsetKey,
                                 [NSValue valueWithUIEdgeInsets:table.contentInset],
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(table, &kAWEMessageSavedIndicatorInsetKey,
                                 [NSValue valueWithUIEdgeInsets:table.verticalScrollIndicatorInsets],
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(table, &kAWEMessageSavedOffsetKey,
                                 [NSValue valueWithCGPoint:table.contentOffset],
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(table, &kAWEMessageSavedAdjustmentKey,
                                 @(table.contentInsetAdjustmentBehavior),
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);

        if (host && host != root && host.superview) {
            CGRect hostRoot = [host.superview convertRect:host.frame toView:root];
            CGFloat candidate = CGRectGetMinY(hostRoot);
            if (candidate >= 40.0 && candidate <= navBottom + 32.0) {
                effectiveShift = candidate;
                objc_setAssociatedObject(host, &kAWEMessageSavedHostFrameKey,
                                         [NSValue valueWithCGRect:host.frame],
                                         OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                objc_setAssociatedObject(table, &kAWEMessageManagedHostKey, host,
                                         OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
        }
        if (effectiveShift <= 1.0 && table.superview) {
            CGRect tableRoot = [table.superview convertRect:table.frame toView:root];
            CGFloat candidate = CGRectGetMinY(tableRoot);
            if (candidate >= 40.0 && candidate <= navBottom + 32.0) {
                effectiveShift = candidate;
                objc_setAssociatedObject(table, &kAWEMessageSavedTableFrameKey,
                                         [NSValue valueWithCGRect:table.frame],
                                         OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
        }
        objc_setAssociatedObject(table, &kAWEMessageAppliedShiftKey, @(effectiveShift),
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(table, &kAWEMessageManagedKey, @YES,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    if (effectiveShift > 1.0) {
        NSValue *savedHostFrame = host ? objc_getAssociatedObject(host, &kAWEMessageSavedHostFrameKey) : nil;
        if (host && savedHostFrame) {
            CGRect base = savedHostFrame.CGRectValue;
            CGRect desired = base;
            desired.origin.y -= effectiveShift;
            desired.size.height += effectiveShift;
            if (!CGRectEqualToRect(host.frame, desired)) host.frame = desired;
        } else if (table.superview) {
            /* Fallback when the table itself is the first movable content node.
               Always derive from the saved native frame so repeated layouts cannot
               accumulate another 106pt shift. */
            NSValue *savedTableFrame = objc_getAssociatedObject(table, &kAWEMessageSavedTableFrameKey);
            if (savedTableFrame) {
                CGRect desired = savedTableFrame.CGRectValue;
                desired.origin.y -= effectiveShift;
                desired.size.height += effectiveShift;
                if (!CGRectEqualToRect(table.frame, desired)) table.frame = desired;
            }
        }

        UIEdgeInsets baseInset = [objc_getAssociatedObject(table, &kAWEMessageSavedInsetKey) UIEdgeInsetsValue];
        UIEdgeInsets baseIndicator = [objc_getAssociatedObject(table, &kAWEMessageSavedIndicatorInsetKey) UIEdgeInsetsValue];
        UIEdgeInsets inset = baseInset;
        inset.top += effectiveShift;
        inset.bottom = 0.0;
        UIEdgeInsets indicator = baseIndicator;
        indicator.top += effectiveShift;
        indicator.bottom = 0.0;
        table.contentInset = inset;
        table.verticalScrollIndicatorInsets = indicator;
        table.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
        table.bounces = YES;
        table.alwaysBounceVertical = YES;
        if (!already && !table.dragging && !table.tracking && !table.decelerating) {
            CGPoint offset = [objc_getAssociatedObject(table, &kAWEMessageSavedOffsetKey) CGPointValue];
            offset.y -= effectiveShift;
            [table setContentOffset:offset animated:NO];
        }
    }

    /* Preserve the list's native dynamic background. It now extends under the
       header and gives the clear glass a real light/dark backing instead of the
       black window root. */
    if (!table.backgroundColor && root.backgroundColor) table.backgroundColor = root.backgroundColor;
    /* v20 fixes the remaining bottom blank band: FLEX/logs showed the outer
       AWEIMChatTabTableView at 440x956 while an inner
       AWEIMTabSkeletonCollectionView stayed at 440x850. */
    AWEMessageExtendInnerContentV20(root, table, effectiveShift);
    AWEMessageUpdateEdgeBlurRelease(root, nav, table);
    /* FLEX identified the grey top/bottom veil as UIKit.ScrollEdgeEffectView.
       These are expensive whole-tree operations, so only do them at first apply
       and on a slow maintenance cadence. Already-suppressed objects are marked
       and never processed/logged again. */
    if (doMaintenance) {
        AWEMessageDisableScrollEdgeEffects(root);
        AWEMessageClearTopBackgroundCarriers(root, nav);
        AWEMessageClearHeaderHairline(root, nav);
        AWEMessageProbeSurfaceGeometry(root, nav, table, host, effectiveShift,
                                       already ? @"maintenance" : @"first-apply");
    }

    NSString *sig = [NSString stringWithFormat:@"%@|%@|%@|%.1f|%@|%@|%@",
                     NSStringFromCGRect(nav.frame), NSStringFromCGRect(table.frame),
                     host ? NSStringFromCGRect(host.frame) : @"nil", effectiveShift,
                     menu ? NSStringFromClass(menu.class) : @"nil",
                     search ? NSStringFromClass(search.class) : @"nil",
                     add ? NSStringFromClass(add.class) : @"nil"];
    if (![sig isEqualToString:gAWEMessageLastSignature]) {
        gAWEMessageLastSignature = [sig copy];
        AWEMessageProbe(@"MESSAGE_V20 nav=%@ table=%@ host=%@ shift=%.1f inset=%@ menu=%@ search=%@ add=%@",
                        NSStringFromCGRect(nav.frame), NSStringFromCGRect(table.frame),
                        host ? NSStringFromCGRect(host.frame) : @"nil", effectiveShift,
                        NSStringFromUIEdgeInsets(table.contentInset),
                        menu ? NSStringFromClass(menu.class) : @"nil",
                        search ? NSStringFromClass(search.class) : @"nil",
                        add ? NSStringFromClass(add.class) : @"nil");
        AWEMessageProbe(@"MESSAGE_V9_NATIVE_POS native-menu-tap-preserved=1 menuDriver=%d search=%d add=%d frames=%@/%@/%@/%@",
                        objc_getAssociatedObject(AWEMessagePressGestureHost(menu, nav), &kAWEMessagePressGestureKey) != nil,
                        objc_getAssociatedObject(AWEMessagePressGestureHost(search, nav), &kAWEMessagePressGestureKey) != nil,
                        objc_getAssociatedObject(AWEMessagePressGestureHost(add, nav), &kAWEMessagePressGestureKey) != nil,
                        NSStringFromCGRect(menuPlate.frame),
                        NSStringFromCGRect(titlePlate.frame),
                        NSStringFromCGRect(searchPlate.frame),
                        NSStringFromCGRect(addPlate.frame));
    }
}


static void AWEMessageApplyFullSurface(UIViewController *controller) {
    if (!controller || [objc_getAssociatedObject(controller, &kAWEMessageApplyingKey) boolValue]) return;
    objc_setAssociatedObject(controller, &kAWEMessageApplyingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    @try { AWEMessageApplyFullSurfaceBody(controller); }
    @finally { objc_setAssociatedObject(controller, &kAWEMessageApplyingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); }
}

static __attribute__((unused)) void AWEMessageRestoreFullSurface(UIViewController *controller) {
    if (!controller || !controller.isViewLoaded) return;
    UIView *root = controller.view;
    UIScrollView *table = AWEMessageFindTable(root);
    if (table && [objc_getAssociatedObject(table, &kAWEMessageManagedKey) boolValue]) {
        UIView *nav = AWEMessageFindNav(root);
        CGFloat navBottom = nav ? CGRectGetMaxY([nav.superview convertRect:nav.frame toView:root]) : 110.0;
        UIView *host = AWEMessageContentHost(table, root, navBottom);
        NSValue *hostFrame = host ? objc_getAssociatedObject(host, &kAWEMessageSavedHostFrameKey) : nil;
        if (host && hostFrame) {
            host.frame = hostFrame.CGRectValue;
            objc_setAssociatedObject(host, &kAWEMessageSavedHostFrameKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        NSValue *tableFrame = objc_getAssociatedObject(table, &kAWEMessageSavedTableFrameKey);
        if (tableFrame) {
            table.frame = tableFrame.CGRectValue;
            objc_setAssociatedObject(table, &kAWEMessageSavedTableFrameKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        NSValue *inset = objc_getAssociatedObject(table, &kAWEMessageSavedInsetKey);
        NSValue *indicator = objc_getAssociatedObject(table, &kAWEMessageSavedIndicatorInsetKey);
        NSValue *offset = objc_getAssociatedObject(table, &kAWEMessageSavedOffsetKey);
        NSNumber *adjust = objc_getAssociatedObject(table, &kAWEMessageSavedAdjustmentKey);
        if (inset) table.contentInset = inset.UIEdgeInsetsValue;
        if (indicator) table.verticalScrollIndicatorInsets = indicator.UIEdgeInsetsValue;
        if (adjust) table.contentInsetAdjustmentBehavior = (UIScrollViewContentInsetAdjustmentBehavior)adjust.integerValue;
        if (offset && !table.dragging && !table.tracking && !table.decelerating) [table setContentOffset:offset.CGPointValue animated:NO];
        objc_setAssociatedObject(table, &kAWEMessageManagedKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(table, &kAWEMessageAppliedShiftKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(table, &kAWEMessageManagedHostKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    for (UIView *v in AWEMessageSubviews(root, 900)) {
        if (AWEMessageIsOurGlass(v)) {
            if ([v isKindOfClass:UIButton.class]) AWEMessageReleaseProxy((UIButton *)v);
            [v removeFromSuperview];
        }
    }
    ALB8MessageRestoreSections();
    AWEMessageRestoreVisuals();
    AWEMessageRestoreSurfaceViews();
    gAWEMessageLastSignature = nil;
    gAWEMessageLastSurfaceProbeTime = 0.0;
    gAWEMessageLastMaintenanceTime = 0.0;
    AWEMessageProbe(@"MESSAGE_RESTORE controller=%@", NSStringFromClass(controller.class));
}

%hook AWEIMMessageTabViewController
- (void)traitCollectionDidChange:(UITraitCollection *)previous {
    %orig(previous);
    if (gAWEMessageActiveController == (UIViewController *)self)
        dispatch_async(dispatch_get_main_queue(), ^{ AWEMessageApplyFullSurface((UIViewController *)self); });
}
- (void)viewWillAppear:(BOOL)animated {
    %orig(animated);
    gAWEMessageActiveController = (UIViewController *)self;
}
- (void)viewDidLayoutSubviews {
    %orig;
    if (gAWEMessageActiveController == (UIViewController *)self) {
        AWEMessageApplyFullSurface((UIViewController *)self);
    }
}
- (void)viewDidAppear:(BOOL)animated {
    %orig(animated);
    gAWEMessageActiveController = (UIViewController *)self;
    AWEMessageApplyFullSurface((UIViewController *)self);
    __weak UIViewController *weakSelf = (UIViewController *)self;
    const double delays[] = {0.12, 0.45, 0.90};
    for (NSUInteger i = 0; i < 3; i++) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delays[i] * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            UIViewController *live = weakSelf;
            if (live && gAWEMessageActiveController == live) {
                AWEMessageApplyFullSurface(live);
            }
        });
    }
}
- (void)viewWillDisappear:(BOOL)animated {
    /* Keep the managed message surface warm.  v12 restored a saved host frame
       while Douyin was simultaneously relaying out the persistent tab
       controller; the next visit then discovered a different host and the
       translucent placeholder appeared at a different edge.  The message tab
       is persistent, so leaving the idempotent full-surface geometry in place
       is both cheaper and stable across tab switches/pushes. */
    for (UIView *view in AWEMessageSubviews(((UIViewController *)self).view, 500)) {
        if (AWEMessageIsOurGlass(view) && [view isKindOfClass:UIButton.class]) {
            [(UIButton *)view cancelTrackingWithEvent:nil];
            ((UIButton *)view).highlighted = NO;
        }
    }
    if (gAWEMessageActiveController == (UIViewController *)self) gAWEMessageActiveController = nil;
    AWEMessageProbe(@"MESSAGE_KEEP_WARM controller=%@", NSStringFromClass(object_getClass((id)self)));
    %orig(animated);
}
%end

%hook AWEIMSlotIntrinsicView
- (void)didMoveToWindow {
    %orig;
    AWEMessageSuppressAdSlotIfNeeded((UIView *)self);
}
- (void)layoutSubviews {
    %orig;
    AWEMessageSuppressAdSlotIfNeeded((UIView *)self);
}
- (void)setHidden:(BOOL)hidden {
    if (AWEMessageFeatureActive() &&
        [objc_getAssociatedObject((UIView *)self, &kAWEMessageAdSlotSuppressedKey) boolValue]) {
        %orig(YES);
        return;
    }
    if (AWEMessageFeatureActive() && ((UIView *)self).window) {
        AWEMessageSuppressAdSlotIfNeeded((UIView *)self);
        if ([objc_getAssociatedObject((UIView *)self, &kAWEMessageAdSlotSuppressedKey) boolValue]) {
            %orig(YES);
            return;
        }
    }
    %orig(hidden);
}
%end

%hook _TtC5UIKit20ScrollEdgeEffectView
- (void)didMoveToWindow {
    %orig;
    AWEMessageSuppressRuntimeEdgeView((UIView *)self);
}
- (void)layoutSubviews {
    %orig;
    AWEMessageSuppressRuntimeEdgeView((UIView *)self);
}
%end

%ctor {
    @autoreleasepool {
        %init;
        [NSNotificationCenter.defaultCenter
            addObserverForName:@"AwemeLiquidBar.iOS27AdaptiveGlassStyleDidChange"
                        object:nil
                         queue:NSOperationQueue.mainQueue
                    usingBlock:^(__unused NSNotification *note) {
            UIViewController *live = gAWEMessageActiveController;
            if (live && live.isViewLoaded && live.view.window) {
                AWEMessageApplyFullSurface(live);
            }
        }];
        [NSNotificationCenter.defaultCenter
            addObserverForName:@"ALB28.MessageButtonInsetChanged" object:nil queue:NSOperationQueue.mainQueue
            usingBlock:^(__unused NSNotification *note) {
                UIViewController *live = gAWEMessageActiveController;
                if (live.viewIfLoaded.window) AWEMessageApplyFullSurface(live);
            }];
        for (NSNotificationName name in @[UIApplicationDidBecomeActiveNotification,
                                            UIAccessibilityReduceTransparencyStatusDidChangeNotification]) {
            [NSNotificationCenter.defaultCenter addObserverForName:name object:nil queue:NSOperationQueue.mainQueue
                usingBlock:^(__unused NSNotification *note) {
                    UIViewController *live = gAWEMessageActiveController;
                    if (live.viewIfLoaded.window) AWEMessageApplyFullSurface(live);
                }];
        }
        AWEMessageProbe(@"SESSION message-top-glass-2.0-native-menu-blur-v8 loaded os=%@",
                        NSProcessInfo.processInfo.operatingSystemVersionString);
    }
}
