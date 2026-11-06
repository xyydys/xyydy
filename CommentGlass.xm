#import "ALBUnifiedMaterial.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <math.h>
#import "ALBReproCaptureSignal.h"

// Fresh implementation from the auth44 baseline.
// Structure: glass in the panel slot; video geometry protection
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
/* 3.11.44: tint strength of the comment panel's glass (0 = the system's own glass, no tint). */
static NSString * const kAWECommentGlassTintAlphaDefaultsKey = @"AwemeLiquidBarCommentGlassTintAlphaV1";
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
    /* 3.11.47 用户：“纯透明模式肯定是几乎没有可读性，直接合并到色调模式里面”。存的 0（纯透明）一律按色调走：
       色调浓度 0＝系统 Regular 玻璃；浓度不是 0＝Clear＋染色（原来纯透明的那种透，只是带了色调） */
    if (mode <= 0) mode = 1;
    return MIN(2, mode); /* 1 tint (Regular, or Clear+tint), 2 frosted material */
}

/* 3.11.44 用户：“如果能调节这个色调的透明度的话更好”。浅色模式染白、深色模式染黑，0＝不染（系统玻璃原样） */
static CGFloat AWECommentGlassTintAlpha(void) {
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:kAWECommentGlassTintAlphaDefaultsKey];
    CGFloat value = [stored isKindOfClass:NSNumber.class] ? [(NSNumber *)stored doubleValue] : 0.0;
    return MIN(0.8, MAX(0.0, value));
}

static CGFloat AWECommentGlassCornerRadius(void) {
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:kAWECommentGlassCornerRadiusDefaultsKey];
    CGFloat value = [stored isKindOfClass:NSNumber.class] ? [(NSNumber *)stored doubleValue] : 22.0;
    return MIN(40.0, MAX(16.0, value));
}

/* Fullscreen media preservation is a geometry feature, not a glass feature.
   When the user's Fullscreen Playback switch is on, opening/closing comments
   must not shrink a previously-fullscreen player even if comment glass is off. */
/* 3.11.45 用户：“我之前做过的打开评论区时作品不会收缩到顶部这个开关现在已经完全失效了，之前也不能针对实况作品识别到这个
   动作，这次加入诊断整个重做，确保不会收缩上去，以及不要出现万一收缩上去又下不来的”。原来这一套（评论打开前记下媒体视图的
   位置、之后挡住缩小）整个停用，换成这种做法——视频不是事后挡，是根本不让它缩：
   · 视频容器（AWEDPlayerViewController_Merge 的视图）：全局 setFrame: 里只认“下一个响应者是它”的那一个视图，主窗口里
     原点挪开或者变得比父视图小一律写成 原点 0、至少父视图那么大（全屏播放撑得更高的照旧）；willDisplay / 布局后各补钉一次；
     抖音的 videoDidShrink（展开评论区时把视频缩成小窗的入口）不执行。
   · 图文和实况：RichContentContainerViewController 的 updateShrinkState:…（图文版的 videoDidShrink）要缩的时候不执行，
     要还原的照常执行；另外老的两条（图片 / 实况内容视图要缩的形变挡掉、还原照放）也接到这个开关上。
   · 评论展开时 HUD 顶上那条纯黑遮罩藏起来。
   钉的规则只会“放大回去”，不会把视频缩小，所以不会出现缩上去下不来。开关：AwemeLiquidBarCommentFreezeV1（默认开）。 */
static NSString * const kAWECommentFreezeDefaultsKey = @"AwemeLiquidBarCommentFreezeV1";
static BOOL AWECommentFreezeOn(void) {
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:kAWECommentFreezeDefaultsKey];
    return [stored isKindOfClass:NSNumber.class] ? [(NSNumber *)stored boolValue] : YES;
}
static BOOL AWECommentMediaGuardEnabled(void) { return NO; }   // 3.11.45：老的记位置挡缩小停用（见上）
static Class gAWEFreezeMergeClass;
static NSUInteger gAWEFreezeSetFramePins, gAWEFreezeMoveWrites, gAWEFreezeWillDisplayPins, gAWEFreezeLayoutPins;
static NSUInteger gAWEFreezeShrinkSuppressed, gAWEFreezeRichBlocked, gAWEFreezeRichPassed, gAWEFreezeTransformBlocks;
static NSUInteger gAWEFreezeCoverHidden;
static NSString *gAWEFreezeRichClass, *gAWEFreezeScan, *gAWEFreezeLastPin;
/* 视频容器该在的位置：只放大不缩小（CGRectNull＝不用动） */
static CGRect AWECommentFreezeTarget(UIView *view, CGRect frame) {
    UIWindow *window = view.window;
    if (!window || window.windowLevel != UIWindowLevelNormal) return CGRectNull;
    UIView *parent = view.superview;
    CGFloat w = CGRectGetWidth(parent.bounds), h = CGRectGetHeight(parent.bounds);
    if (w <= 1.0 || h <= 1.0) return CGRectNull;
    BOOL moved = fabs(frame.origin.x) > 0.5 || fabs(frame.origin.y) > 0.5;
    BOOL smaller = frame.size.width < w - 0.5 || frame.size.height < h - 0.5;
    if (!moved && !smaller) return CGRectNull;
    return CGRectMake(0, 0, MAX(frame.size.width, w), MAX(frame.size.height, h));
}
static void AWECommentFreezeScanWindow(UIWindow *window);
static BOOL AWECommentFreezePinController(UIViewController *merge) {
    if (!AWECommentFreezeOn()) return NO;
    UIView *view = merge.viewIfLoaded;
    if (!view) return NO;
    CGRect target = AWECommentFreezeTarget(view, view.frame);
    if (CGRectIsNull(target)) return NO;
    view.frame = target;
    return YES;
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


static UIColor *AWECommentGlassTintForStyle(UIUserInterfaceStyle style) {
    CGFloat alpha = AWECommentGlassTintAlpha();
    if (alpha < 0.005) return nil;
    return [UIColor colorWithWhite:(style == UIUserInterfaceStyleDark ? 0.0 : 1.0) alpha:alpha];
}

/* 3.11.44 用户：“整个评论区的面板恢复……浅色是浅色玻璃，深色是深色玻璃”。之前好些版本这里走的是右侧那份统一材质（ALBMaterialMakeEffect：
   透明＋28% 黑，再由 ALBMaterialApplyEffectView 把深浅定死成深色），于是系统浅色时评论区也是深色、黑字看不清。恢复成最早的做法
   （1.0.58）：纯透明＝系统 Clear 玻璃、色调＝系统 Regular 玻璃、磨砂＝系统材质模糊，全部不染色，深浅跟系统
   （overrideUserInterfaceStyle＝窗口场景的深浅）；另加“色调浓度”（浅色染白、深色染黑）。 */
/* 3.11.45 用户：“评论区玻璃色调的调节功能不生效。参考三段式底栏的透明玻璃做法吧”。Regular 玻璃自己按背后亮度适配，
   淡的白 / 黑染色在它上面几乎看不出来；三段式底栏的透明是 Clear＋染色（Clear 只认染色）。所以：浓度不是 0 时，“色调”也用
   Clear＋染色（浅色染白、深色染黑）；浓度 0＝系统 Regular 原样。“纯透明”一直是 Clear（＋浓度）。 */
static NSUInteger gAWECommentTintClearUses;
static UIVisualEffect *AWECommentMakeGlassEffectForMode(NSInteger mode, UIUserInterfaceStyle style) {
    if (!AWECommentGlassFeatureEnabled()) return nil;
    if (mode == 2) return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
    if (@available(iOS 26.0, *)) {
        UIColor *tint = AWECommentGlassTintForStyle(style);
        BOOL clear = mode == 0 || tint != nil;
        if (mode == 1 && tint) gAWECommentTintClearUses++;
        UIGlassEffect *effect = [UIGlassEffect effectWithStyle:(clear ? UIGlassEffectStyleClear : UIGlassEffectStyleRegular)];
        effect.interactive = YES;
        effect.tintColor = tint;
        return effect;
    }
    return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
}

/* 3.11.47 用户：“这个玻璃有问题啊，它怎么跟底部是分开的呢？整个评论区从上到下应该是一整块玻璃，然后底部的那个输入长条我让它
   悬浮就行，除此之外不要有任何分界线之类的，然后那个悬浮条的材质换成系统的高斯模糊……输入框弹起时让除了输入框之外的区域也变成
   系统的高斯模糊，只不过两者稍微区分一下……不需要圆角了，圆角没办法弄，太丑了。也就是说，评论区不包含分享面板的区域，只有
   评论区的背景是液态玻璃”。上面 3.11.46 那套（输入区自己一块玻璃、面板玻璃截到输入区顶边、键盘弹起时上圆角）整个换掉：
   · 评论面板一整块液态玻璃从上到下，不截、不藏（AWECommentSyncPanelTrim 只负责把以前截过的还原成整块）。
   · 常驻态：输入区自己没有任何底（抖音的不透明底照旧清掉），只有输入框那颗胶囊是一块系统模糊（systemMaterial），浮在玻璃上；
     顶边那条细线照旧藏。
   · 键盘弹起 / 回复态（键盘通知、输入区变高、顶到面板上面、或者被抬离面板底边）：输入区整块垫一层系统模糊（systemThinMaterial，
     直角，不要圆角），输入框胶囊还是 systemMaterial——比背后那层厚一点，两者有区分。
   · 键盘弹起时输入区不要圆角。 */
static char kAWECommentBarGlassKey, kAWECommentFieldGlassKey, kAWECommentFieldOriginalColorKey;
static char kAWECommentBarExpandedKey, kAWECommentHairlineHiddenKey, kAWECommentBlurStyleKey;
static NSUInteger gAWECommentBarGlassInstalls, gAWECommentFieldGlassInstalls, gAWECommentHairlinesHidden, gAWECommentPanelTrims;
static NSUInteger gAWECommentBarBlurShows, gAWECommentKeyboardSyncs, gAWECommentInputLayoutSyncs, gAWECommentFieldMisses;
static NSString *gAWECommentInputState, *gAWECommentHairlineLast, *gAWECommentKeyboardState;
static BOOL gAWECommentKeyboardUp;
static void AWECommentSyncPanelTrim(void);
@interface AWECommentBarGlassView : UIVisualEffectView
@end
@implementation AWECommentBarGlassView
@end

static BOOL AWECommentIsOurGlassTree(UIView *view) {
    UIView *cursor = view;
    for (NSInteger depth = 0; cursor && depth < 8; depth++, cursor = cursor.superview) {
        if (cursor.tag == kAWECommentGlassTag) return YES;
        if ([cursor isKindOfClass:AWECommentBarGlassView.class]) return YES;
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
        if (AWECommentIsOurGlassTree(sub) || [sub isKindOfClass:AWECommentBarGlassView.class]) continue;
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
    // 3.11.46：不再描边（玻璃自己有边缘高光，再加一圈描边会和抖音的细线、输入区玻璃的边叠在一起）
    glass.layer.borderWidth = 0;

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
    UIUserInterfaceStyle style = AWECommentSceneStyleForView(slot);      // 3.11.44：三种材质都跟系统深浅
    if (glass.overrideUserInterfaceStyle != style) glass.overrideUserInterfaceStyle = style;
    AWECommentApplyRoundedGlassChrome(glass, slot);
    /* Clip Douyin's same-frame panel wrappers to the identical top-corner path.
       This removes the rectangular translucent carrier that otherwise leaks
       through beside the two rounded glass corners. */
    AWECommentClipPanelChainToRoundedTop(slot, controller,
                                         AWECommentGlassCornerRadius());
    // 材质、深浅、色调浓度任何一样变了：效果重做（染色是做进效果里的）
    CGFloat tintAlpha = AWECommentGlassTintAlpha();
    NSString *applied = [NSString stringWithFormat:@"%ld|%ld|%.3f", (long)mode, (long)style, tintAlpha];
    NSString *appliedBefore = objc_getAssociatedObject(glass, &kAWECommentGlassModeAppliedKey);
    if (![appliedBefore isKindOfClass:NSString.class] || ![appliedBefore isEqualToString:applied]) {
        glass.effect = nil;
        objc_setAssociatedObject(glass, &kAWECommentGlassModeAppliedKey, applied,
                                 OBJC_ASSOCIATION_COPY_NONATOMIC);
    }
    // 磨砂（系统模糊）没有染色这一项：浓度铺在效果视图的内容层上
    UIColor *overlay = mode == 2 ? AWECommentGlassTintForStyle(style) : nil;
    UIColor *overlayNow = glass.contentView.backgroundColor;
    if (!((overlay == nil && (overlayNow == nil || CGColorGetAlpha(overlayNow.CGColor) < 0.001)) || [overlay isEqual:overlayNow]))
        glass.contentView.backgroundColor = overlay ?: UIColor.clearColor;
    if (!glass.effect) {
        UIVisualEffect *effect = AWECommentMakeGlassEffectForMode(mode, style);
        if (effect) {
            // Mount the effect view first, then
            // materialize the glass effect after geometry is valid.
            dispatch_async(dispatch_get_main_queue(), ^{
                NSString *still = objc_getAssociatedObject(glass, &kAWECommentGlassModeAppliedKey);
                if (glass.superview && !glass.effect && [still isEqual:applied]) {
                    glass.effect = effect;
                }
            });
        }
    }
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

/* 3.11.47：顺着能打字的那个视图往上找输入框胶囊（键盘弹起时输入框会变高 / 右边多出表情键，按尺寸找可能落空，
   3.11.46 那张图里键盘弹起时的输入框还是不透明白底就是没找到它） */
static UIView *AWECommentInputFieldByText(UIView *container) {
    if (!container) return nil;
    CGFloat width = CGRectGetWidth(container.bounds);
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:container];
    UIView *text = nil;
    for (NSUInteger i = 0; i < queue.count && i < 240; i++) {
        UIView *v = queue[i];
        NSString *name = NSStringFromClass(v.class);
        if (v != container && !v.hidden && ([v isKindOfClass:UITextView.class] || [v isKindOfClass:UITextField.class] ||
            [name rangeOfString:@"TextView"].location != NSNotFound)) { text = v; break; }
        [queue addObjectsFromArray:v.subviews];
    }
    if (!text) return nil;
    for (UIView *up = text; up && up != container; up = up.superview) {
        CGRect r = [up convertRect:up.bounds toView:container];
        if (r.size.width < width * 0.45 || r.size.height < 28.0 || r.size.height > 150.0) continue;
        UIColor *bg = up.backgroundColor;
        BOOL drawn = (bg && CGColorGetAlpha(bg.CGColor) > 0.02) || objc_getAssociatedObject(up, &kAWECommentFieldOriginalColorKey);
        if (up.layer.cornerRadius >= 8.0 || drawn) return up;
    }
    return nil;
}

static UIView *AWECommentInputFieldSlot(UIView *container) {
    if (!container) return nil;
    UIView *cached = objc_getAssociatedObject(container, &kAWECommentInputFieldKey);
    if (cached && cached.superview && !cached.hidden && [cached isDescendantOfView:container]) {
        CGFloat ch = CGRectGetHeight(cached.bounds);
        if (ch >= 28.0 && ch <= 150.0) return cached;
    }
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
    if (!best) best = AWECommentInputFieldByText(container);
    if (!best) gAWECommentFieldMisses++;
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
    if (container.backgroundColor && CGColorGetAlpha(container.backgroundColor.CGColor) > 0.025 &&
        !objc_getAssociatedObject(container, &kAWECommentCoverOriginalColorKey))
        objc_setAssociatedObject(container, &kAWECommentCoverOriginalColorKey, container.backgroundColor, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
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
        /* The input container is the semantic unit: remove its
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
    /* This mini panel lives in UITextEffectsWindow, outside
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


/* 3.11.47：系统模糊（材质变了才换；深浅跟窗口场景） */
static void AWECommentApplyBlur(UIVisualEffectView *view, UIBlurEffectStyle blurStyle, UIView *styleView) {
    if (!view) return;
    UIUserInterfaceStyle style = AWECommentSceneStyleForView(styleView ?: view);
    if (view.overrideUserInterfaceStyle != style) view.overrideUserInterfaceStyle = style;
    NSNumber *applied = objc_getAssociatedObject(view, &kAWECommentBlurStyleKey);
    if (view.effect && applied && applied.integerValue == (NSInteger)blurStyle) return;
    view.effect = [UIBlurEffect effectWithStyle:blurStyle];
    objc_setAssociatedObject(view, &kAWECommentBlurStyleKey, @(blurStyle), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

/* 抖音画在输入区顶边的细线：藏起来（记着，功能关了还原不了也无所谓——下次打开评论区是新的） */
static void AWECommentHideInputHairlines(UIView *panel, UIView *input) {
    if (!input) return;
    CGFloat width = CGRectGetWidth(input.bounds);
    CGFloat scale = input.window.screen.scale ?: 3.0;
    NSMutableArray<UIView *> *candidates = [NSMutableArray array];
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[input, @0]];
    for (NSUInteger i = 0; i < queue.count && i < 160; i++) {
        UIView *v = queue[i][0];
        NSUInteger depth = [queue[i][1] unsignedIntegerValue];
        if (v != input) [candidates addObject:v];
        if (depth < 3 && ![v isKindOfClass:UIVisualEffectView.class]) for (UIView *sub in v.subviews) [queue addObject:@[sub, @(depth + 1)]];
    }
    // 面板里、贴着输入区顶边的那条（输入区的兄弟）
    for (UIView *sibling in input.superview.subviews) if (sibling != input) [candidates addObject:sibling];
    CGRect inputRect = [input convertRect:input.bounds toView:input.superview];
    for (UIView *v in candidates) {
        if (v.hidden || [v isKindOfClass:UILabel.class] || [v isKindOfClass:UIControl.class] || [v isKindOfClass:UIVisualEffectView.class]) continue;
        CGRect r = [v convertRect:v.bounds toView:input.superview];
        BOOL thin = r.size.height > 0 && r.size.height <= MAX(1.5, 2.0 / scale);
        BOOL wide = r.size.width >= width * 0.6;
        BOOL nearTop = fabs(CGRectGetMinY(r) - CGRectGetMinY(inputRect)) <= 6.0 || fabs(CGRectGetMaxY(r) - CGRectGetMinY(inputRect)) <= 3.0;
        UIColor *bg = v.backgroundColor;
        BOOL drawn = (bg && CGColorGetAlpha(bg.CGColor) > 0.02) || v.layer.backgroundColor;
        if (!thin || !wide || !nearTop || !drawn) continue;
        v.hidden = YES;
        objc_setAssociatedObject(v, &kAWECommentHairlineHiddenKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        gAWECommentHairlinesHidden++;
        gAWECommentHairlineLast = [NSString stringWithFormat:@"%@ %@（父 %@）", NSStringFromClass(v.class), NSStringFromCGRect(r), NSStringFromClass(v.superview.class)];
    }
}

static void AWECommentSyncInputGlass(UIView *panel, UIView *input, UIView *field) {
    if (!input || !input.window || input.hidden || input.alpha < 0.01) return;
    CGRect inPanel = panel ? [input convertRect:input.bounds toView:panel] : CGRectNull;
    CGFloat top = panel ? CGRectGetMinY(inPanel) : 0;
    CGFloat lift = panel ? CGRectGetHeight(panel.bounds) - CGRectGetMaxY(inPanel) : 0;
    BOOL expanded = gAWECommentKeyboardUp || CGRectGetHeight(input.bounds) > 120.0 || top < -0.5 || lift > 40.0;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    AWECommentBarGlassView *bar = objc_getAssociatedObject(input, &kAWECommentBarGlassKey);
    if (bar && bar.superview != input) { [bar removeFromSuperview]; bar = nil; }
    if (expanded) {
        if (!bar) {
            bar = [[AWECommentBarGlassView alloc] initWithEffect:nil];
            bar.userInteractionEnabled = NO;
            bar.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            bar.accessibilityIdentifier = @"AwemeLiquidBar.CommentInputBlur";
            objc_setAssociatedObject(input, &kAWECommentBarGlassKey, bar, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            gAWECommentBarGlassInstalls++;
        }
        if (input.subviews.firstObject != bar) [input insertSubview:bar atIndex:0];
        if (!CGRectEqualToRect(bar.frame, input.bounds)) bar.frame = input.bounds;
        if (@available(iOS 26.0, *)) {
            bar.cornerConfiguration = [UICornerConfiguration configurationWithUniformRadius:[UICornerRadius fixedRadius:0]];
        }
        bar.layer.cornerRadius = 0;
        if (bar.hidden) { bar.hidden = NO; gAWECommentBarBlurShows++; }
        AWECommentApplyBlur(bar, UIBlurEffectStyleSystemThinMaterial, panel ?: input);
    } else if (bar && !bar.hidden) {
        bar.hidden = YES;
    }
    if (bar) objc_setAssociatedObject(bar, &kAWECommentBarExpandedKey, @(expanded), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (field) {
        UIVisualEffectView *glass = objc_getAssociatedObject(field, &kAWECommentFieldGlassKey);
        if (glass && glass.superview != field) { [glass removeFromSuperview]; glass = nil; }
        UIColor *color = field.backgroundColor;
        BOOL taken = objc_getAssociatedObject(field, &kAWECommentFieldOriginalColorKey) != nil;
        if (!taken && color && CGColorGetAlpha(color.CGColor) > 0.02) {
            objc_setAssociatedObject(field, &kAWECommentFieldOriginalColorKey, color, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            taken = YES;
        }
        if (!taken && field.layer.cornerRadius >= 8.0) taken = YES;   // 本来就没底色的胶囊：也垫一块模糊
        if (taken) {
            if (field.backgroundColor && CGColorGetAlpha(field.backgroundColor.CGColor) > 0.02) field.backgroundColor = UIColor.clearColor;
            if (field.layer.backgroundColor && CGColorGetAlpha(field.layer.backgroundColor) > 0.02) field.layer.backgroundColor = UIColor.clearColor.CGColor;
            if (!glass) {
                glass = [[AWECommentBarGlassView alloc] initWithEffect:nil];
                glass.userInteractionEnabled = NO;
                glass.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
                glass.accessibilityIdentifier = @"AwemeLiquidBar.CommentFieldBlur";
                objc_setAssociatedObject(field, &kAWECommentFieldGlassKey, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                gAWECommentFieldGlassInstalls++;
            }
            if (field.subviews.firstObject != glass) [field insertSubview:glass atIndex:0];
            if (!CGRectEqualToRect(glass.frame, field.bounds)) glass.frame = field.bounds;
            CGFloat fieldRadius = MIN(field.layer.cornerRadius, CGRectGetHeight(field.bounds) / 2.0);
            if (@available(iOS 26.0, *)) {
                glass.cornerConfiguration = fieldRadius > 0
                    ? [UICornerConfiguration configurationWithUniformRadius:[UICornerRadius fixedRadius:fieldRadius]]
                    : [UICornerConfiguration capsuleConfiguration];
            } else {
                glass.layer.cornerRadius = fieldRadius > 0 ? fieldRadius : CGRectGetHeight(field.bounds) / 2.0;
                glass.clipsToBounds = YES;
            }
            AWECommentApplyBlur((UIVisualEffectView *)glass, UIBlurEffectStyleSystemMaterial, panel ?: input);
        }
    }
    AWECommentHideInputHairlines(panel, input);
    [CATransaction commit];
    NSString *state = [NSString stringWithFormat:@"%@（输入区 %@，顶边在面板里 y=%.0f，离面板底 %.0f，键盘=%@；输入框 %@）",
        expanded ? @"键盘弹起 / 回复态：输入区系统模糊（薄）＋输入框系统模糊" : @"常驻态：只有输入框一块系统模糊",
        NSStringFromCGRect([input convertRect:input.bounds toView:nil]), top, lift, gAWECommentKeyboardUp ? @"起" : @"落",
        field ? [NSString stringWithFormat:@"%@ %@", NSStringFromClass(field.class), NSStringFromCGRect([field convertRect:field.bounds toView:input])] : @"没找到"];
    gAWECommentInputState = state;
    if (expanded) gAWECommentKeyboardState = state;
}

/* 3.11.47：评论面板一整块玻璃，不再截到输入区顶边；以前截过 / 藏过的还原成整块 */
static void AWECommentSyncPanelTrim(void) {
    UIView *panel = gAWECommentPanelSlot;
    UIVisualEffectView *panelGlass = panel ? objc_getAssociatedObject(panel, &kAWECommentSlotGlassKey) : nil;
    if (!panelGlass || panelGlass.superview != panel) return;
    if (panelGlass.hidden) panelGlass.hidden = NO;
    if (!CGRectEqualToRect(panelGlass.frame, panel.bounds)) {
        panelGlass.frame = panel.bounds;
        gAWECommentPanelTrims++;
    }
}

/* 3.11.47：输入区自己布局时（键盘起落、回复态切换，抖音会在这里重设输入框底色）同步一次，只动我们自己的视图和底色 */
static void AWECommentInputLayoutSync(UIView *input) {
    static BOOL busy;
    if (busy || !gAWECommentVisible || !AWECommentGlassFeatureEnabled() || !input.window) return;
    busy = YES;
    gAWECommentInputLayoutSyncs++;
    UIView *panel = gAWECommentPanelSlot;
    AWECommentSyncInputGlass(panel.window == input.window ? panel : nil, input, AWECommentInputFieldSlot(input));
    busy = NO;
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
    // 3.11.47：键盘弹起时输入区可能被挪到控制器视图外面（窗口里跟着键盘的那一层）：在窗口里再找一次
    if (!input && inputClass && panel.window) input = AWECommentFindSubviewOfClass(panel.window, inputClass, 16);
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
    // 3.11.47：面板一整块玻璃；输入框胶囊一块系统模糊，键盘弹起时输入区再垫一层薄的系统模糊（见 AWECommentSyncInputGlass 上面）
    AWECommentSyncInputGlass(panel, input, field);
    AWECommentSyncPanelTrim();

    /*
     1.0.58: comments no longer rewrite text, attributed strings, icons, or
     controller traits.  Douyin owns all comment foreground colors.  Tint mode
     affects only our glass and follows the app/window appearance.
    */
    /* 3.11.44: the glass's light/dark is the system's (set in
       AWECommentEnsurePanelGlass); no forced dark material any more. */

    NSString *signature = [NSString stringWithFormat:@"%@|%@|%@|%@|%@|%@",
                           reason ?: @"",
                           NSStringFromCGRect(panel.frame),
                           input ? NSStringFromCGRect(input.frame) : @"nil",
                           field ? NSStringFromCGRect([field convertRect:field.bounds toView:input]) : @"nil",
                           glass.effect ? NSStringFromClass(glass.effect.class) : @"nil",
                           [NSString stringWithFormat:@"style=%ld|mode=%ld|tint=%.2f",
                            (long)AWECommentSceneStyleForView(panel), (long)AWECommentGlassMode(), AWECommentGlassTintAlpha()]];
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
        gAWEFreezeCoverHidden++;
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
    {
        __weak UIWindow *weakScanWindow = window;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.6 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            UIWindow *scanWindow = weakScanWindow;
            if (gAWECommentVisible && scanWindow) AWECommentFreezeScanWindow(scanWindow);
        });
    }
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
    if (!root.window || !(AWECommentGlassFeatureEnabled() || AWECommentFreezeOn())) return;
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
    // 3.11.45：视频容器（下一个响应者是 AWEDPlayerViewController_Merge）只放大不缩小（AWECommentFreezeTarget）
    Class mergeClass = gAWEFreezeMergeClass;
    if (mergeClass) {
        UIResponder *owner = ((UIView *)self).nextResponder;
        if (owner && [owner isKindOfClass:mergeClass] && AWECommentFreezeOn()) {
            CGRect target = AWECommentFreezeTarget((UIView *)self, frame);
            if (!CGRectIsNull(target)) {
                gAWEFreezeSetFramePins++;
                if (fabs(frame.origin.x) > 0.5 || fabs(frame.origin.y) > 0.5) gAWEFreezeMoveWrites++;
                if (gAWEFreezeSetFramePins <= 3 || gAWEFreezeSetFramePins % 50 == 0)
                    gAWEFreezeLastPin = [NSString stringWithFormat:@"%@ → %@（评论%@）", NSStringFromCGRect(frame), NSStringFromCGRect(target),
                                         gAWECommentVisible ? @"开着" : @"没开"];
                %orig(target);
                return;
            }
        }
    }
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
    if (gAWECommentVisible && AWECommentFreezeOn() &&
        AWECommentTransformLooksLikePanelShrink(transform)) {
        gAWEFreezeTransformBlocks++;
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
    if (gAWECommentVisible && AWECommentFreezeOn() &&
        AWECommentTransformLooksLikePanelShrink(transform)) {
        gAWEFreezeTransformBlocks++;
        AWECommentLogPhotoLiveTransformBlock((UIView *)self, transform, @"live");
        return;
    }
    %orig(transform);
}
%end
%end

/* 3.11.45：视频容器 */
%group AWECommentFreezeMergeGroup
%hook AWECommentFreezeMerge
- (void)willDisplay {
    %orig;
    if (AWECommentFreezePinController((UIViewController *)self)) gAWEFreezeWillDisplayPins++;
}
- (void)viewDidLayoutSubviews {
    %orig;
    if (AWECommentFreezePinController((UIViewController *)self)) gAWEFreezeLayoutPins++;
}
- (void)videoDidShrink {
    if (!AWECommentFreezeOn()) { %orig; return; }
    gAWEFreezeShrinkSuppressed++;
    UIView *gradient = nil;
    @try { gradient = [(id)self valueForKey:@"gradientBackgroundView"]; } @catch (__unused NSException *e) { gradient = nil; }
    if ([gradient isKindOfClass:UIView.class] && gradient.alpha < 1.0) gradient.alpha = 1.0;
}
%end
%end

/* 3.11.45：图文 / 实况（图文版的 videoDidShrink）。要缩的时候不执行（不调原方法，参数类型就无所谓），还原照常 */
%group AWECommentFreezeRichGroup
%hook AWECommentFreezeRich
- (void)updateShrinkState:(BOOL)shrink insets:(UIEdgeInsets)insets animated:(BOOL)animated {
    if (shrink && AWECommentFreezeOn()) { gAWEFreezeRichBlocked++; return; }
    if (!shrink) gAWEFreezeRichPassed++;
    %orig;
}
- (void)updateShrinkState:(BOOL)shrink insets:(UIEdgeInsets)insets animated:(BOOL)animated animationDuration:(double)duration {
    if (shrink && AWECommentFreezeOn()) { gAWEFreezeRichBlocked++; return; }
    if (!shrink) gAWEFreezeRichPassed++;
    %orig;
}
%end
%end

/* 3.11.45 诊断：评论打开 0.6 秒后，主窗口里像媒体的大视图（类名、位置、形变、归属控制器）——没挡住的话看是谁缩了 */
static void AWECommentFreezeScanWindow(UIWindow *window) {
    if (!window) return;
    CGRect screen = window.bounds;
    CGFloat minArea = screen.size.width * screen.size.height * 0.12;
    UIView *panel = gAWECommentOwner.viewIfLoaded;
    NSMutableArray<NSString *> *found = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
    for (NSUInteger i = 0; i < queue.count && i < 3000 && found.count < 14; i++) {
        UIView *v = queue[i];
        if (v.hidden || v.alpha < 0.01 || (panel && v == panel)) continue;
        [queue addObjectsFromArray:v.subviews];
        NSString *name = NSStringFromClass(v.class);
        BOOL media = (gAWEFreezeMergeClass && [v.nextResponder isKindOfClass:gAWEFreezeMergeClass]) ||
            [name rangeOfString:@"Video"].location != NSNotFound || [name rangeOfString:@"Player"].location != NSNotFound ||
            [name rangeOfString:@"Image"].location != NSNotFound || [name rangeOfString:@"Live"].location != NSNotFound ||
            [name rangeOfString:@"Photo"].location != NSNotFound || [name rangeOfString:@"RichContent"].location != NSNotFound ||
            [name rangeOfString:@"MultiContent"].location != NSNotFound;
        if (!media) continue;
        CGRect r = [v convertRect:v.bounds toView:window];
        if (r.size.width * r.size.height < minArea || !CGRectIntersectsRect(r, screen)) continue;
        CGAffineTransform t = v.transform;
        NSString *owner = [v.nextResponder isKindOfClass:UIViewController.class] ? NSStringFromClass(v.nextResponder.class) : nil;
        [found addObject:[NSString stringWithFormat:@"%@ %@%@%@", name, NSStringFromCGRect(r),
            CGAffineTransformIsIdentity(t) ? @"" : [NSString stringWithFormat:@" 形变 a=%.2f d=%.2f ty=%.0f", t.a, t.d, t.ty],
            owner ? [@" 属于 " stringByAppendingString:owner] : @""]];
    }
    gAWEFreezeScan = found.count ? [found componentsJoinedByString:@"；"] : @"没找到像媒体的大视图";
}

extern "C" NSString *AWECommentFreezeReport(void) {
    return [NSString stringWithFormat:@"开关=%@；视频容器类=%@；图文容器类=%@；setFrame 钉回 %lu 次（其中挪开的 %lu 次，最近 %@）、willDisplay 补钉 %lu 次、布局后补钉 %lu 次；"
        "videoDidShrink 不执行 %lu 次；图文/实况要缩被挡 %lu 次、还原放行 %lu 次；图片/实况内容视图缩小形变挡掉 %lu 次；"
        "3.11.47 输入区（面板整块玻璃、输入框系统模糊、键盘弹起输入区薄模糊）：输入区模糊装了 %lu 块（显示 %lu 次）、输入框模糊 %lu 块（没找到输入框 %lu 次）、面板玻璃还原成整块 %lu 次、藏细线 %lu 条（最近 %@）；键盘通知补同步 %lu 次、输入区布局补同步 %lu 次；最近一次输入区 %@；最近一次键盘弹起时 %@；"
        "色调浓度用 Clear 的次数 %lu；HUD 顶上黑遮罩藏 %lu 条；最近一次评论打开 0.6 秒后：%@",
        AWECommentFreezeOn() ? @"开" : @"关", gAWEFreezeMergeClass ? NSStringFromClass(gAWEFreezeMergeClass) : @"没找到",
        gAWEFreezeRichClass ?: @"没找到", (unsigned long)gAWEFreezeSetFramePins, (unsigned long)gAWEFreezeMoveWrites, gAWEFreezeLastPin ?: @"-",
        (unsigned long)gAWEFreezeWillDisplayPins, (unsigned long)gAWEFreezeLayoutPins, (unsigned long)gAWEFreezeShrinkSuppressed,
        (unsigned long)gAWEFreezeRichBlocked, (unsigned long)gAWEFreezeRichPassed, (unsigned long)gAWEFreezeTransformBlocks,
        (unsigned long)gAWECommentBarGlassInstalls, (unsigned long)gAWECommentBarBlurShows, (unsigned long)gAWECommentFieldGlassInstalls,
        (unsigned long)gAWECommentFieldMisses, (unsigned long)gAWECommentPanelTrims,
        (unsigned long)gAWECommentHairlinesHidden, gAWECommentHairlineLast ?: @"-", (unsigned long)gAWECommentKeyboardSyncs,
        (unsigned long)gAWECommentInputLayoutSyncs, gAWECommentInputState ?: @"-", gAWECommentKeyboardState ?: @"还没弹过键盘",
        (unsigned long)gAWECommentTintClearUses, (unsigned long)gAWEFreezeCoverHidden,
        gAWEFreezeScan ?: @"还没打开过评论区"];
}

/* 3.11.47：输入区自己的布局 */
%group AWECommentInputLayoutGroup
%hook AWECommentInputContainerHook
- (void)layoutSubviews {
    %orig;
    AWECommentInputLayoutSync((UIView *)self);
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
        gAWEFreezeMergeClass = objc_getClass("AWEDPlayerViewController_Merge");
        if (gAWEFreezeMergeClass) {
            %init(AWECommentFreezeMergeGroup, AWECommentFreezeMerge = gAWEFreezeMergeClass);
        }
        Class richClass = Nil;
        for (NSString *name in @[@"RichContentContainerViewController", @"AWEKnowledgeRichContentImpl.RichContentContainerViewController",
                                 @"AWERichContentImpl.RichContentContainerViewController"]) {
            Class candidate = objc_getClass(name.UTF8String);
            if (candidate && [candidate instancesRespondToSelector:@selector(updateShrinkState:insets:animated:)]) {
                richClass = candidate;
                gAWEFreezeRichClass = name;
                break;
            }
        }
        if (!richClass) {
            // 名字对不上：在类名带 RichContent 的类里找自己声明了 updateShrinkState:insets:animated: 的（诊断里写出来）
            unsigned int count = 0;
            Class *classes = objc_copyClassList(&count);
            for (unsigned int i = 0; i < count && !richClass; i++) {
                const char *cname = class_getName(classes[i]);
                if (!cname || !strstr(cname, "RichContent")) continue;
                unsigned int mcount = 0;
                Method *methods = class_copyMethodList(classes[i], &mcount);
                for (unsigned int m = 0; m < mcount; m++) {
                    if (method_getName(methods[m]) == @selector(updateShrinkState:insets:animated:)) {
                        richClass = classes[i];
                        gAWEFreezeRichClass = [@(cname) stringByAppendingString:@"（按方法找到的）"];
                        break;
                    }
                }
                free(methods);
            }
            free(classes);
        }
        if (richClass) {
            %init(AWECommentFreezeRichGroup, AWECommentFreezeRich = richClass);
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
        Class inputContainerClass = objc_getClass("AWECommentInputViewSwiftImpl.CommentInputContainerView");
        if (inputContainerClass) {
            %init(AWECommentInputLayoutGroup, AWECommentInputContainerHook = inputContainerClass);
        }
        // 3.11.47：键盘起落：记下来（输入区按“键盘弹起”处理），起落后各补同步两次
        for (NSString *name in @[UIKeyboardWillShowNotification, UIKeyboardWillHideNotification, UIKeyboardDidShowNotification, UIKeyboardDidHideNotification]) {
            [NSNotificationCenter.defaultCenter addObserverForName:name object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *note) {
                BOOL up = [note.name isEqualToString:UIKeyboardWillShowNotification] || [note.name isEqualToString:UIKeyboardDidShowNotification];
                if ([note.name isEqualToString:UIKeyboardWillShowNotification] || [note.name isEqualToString:UIKeyboardDidShowNotification]) {
                    CGRect end = [note.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
                    up = CGRectGetHeight(end) > 80.0;
                }
                gAWECommentKeyboardUp = up;
                if (!gAWECommentVisible || !AWECommentGlassFeatureEnabled()) return;
                UIViewController *owner = gAWECommentOwner;
                if (!owner) return;
                gAWECommentKeyboardSyncs++;
                AWECommentSync(owner, up ? @"keyboard-up" : @"keyboard-down");
                __weak UIViewController *weakOwner = owner;
                dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.3 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                    UIViewController *live = weakOwner;
                    if (live && gAWECommentVisible && gAWECommentOwner == live) AWECommentSync(live, @"keyboard+300ms");
                });
            }];
        }
    }
}
