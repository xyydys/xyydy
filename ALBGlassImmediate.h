#ifndef ALB_GLASS_IMMEDIATE_H
#define ALB_GLASS_IMMEDIATE_H

/* Included once, after ALBGlassLifecycle.h, by Tweak.x.

   3.11.23 sped up every face-tone animation of the right column and the
   panel; the user saw no change (the dark came before the animations: the
   readings, ALBGlassSeed.h). 3.11.24: it acts only while ALBGlassSeed.h
   hands a new glass its start (gALBSeedApplying): the five face-tone
   animations UIKit adds right then finish at once, so that start is the
   glass's first frame. Every other animation (presses, the system's own
   light/dark changes later on) is left exactly as UIKit made it.

   3.11.27: with 3.11.26 the pocket gets the right level within 0.1 s of
   coming back, and what the user still sees is the glass turning from its
   dark start to that level: these five tone animations, 0.5 s each (3.11.23
   recorded them on the right column and the panel), added when the glass
   follows the pocket, a moment after the hand-over. So they also finish at
   once inside the owner's window after it came back (ALBGlassSeed.h: the
   pocket's comeback, the right column shown again, the panel opened, a
   pocket's first reading, another video): 1.5 s. Inside the same window
   the pocket's own level change (opacity, filters.* on the backdrop layers
   inside one of our edge pockets) finishes at once too. Geometry, groups
   and everything outside those windows are left as UIKit made them.
 */

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <string.h>
#import <math.h>

typedef void (*ALBGlassImmediateAddAnimationIMP)(CALayer *, SEL, CAAnimation *, NSString *);
static ALBGlassImmediateAddAnimationIMP gALBGlassImmediateOriginal;
static BOOL gALBGlassImmediateAttempted, gALBGlassImmediateInstalled;
static NSString *gALBGlassImmediateStatus = @"未安装";
static CFTimeInterval gALBGlassImmediateInstalledAt;
static NSUInteger gALBGlassImmediateRightMatched, gALBGlassImmediateRightShortened;
static NSUInteger gALBGlassImmediatePanelMatched, gALBGlassImmediatePanelShortened;
static NSUInteger gALBGlassImmediateComplexSkipped, gALBGlassImmediateGroupsSkipped;
static NSUInteger gALBGlassImmediateExceptions;
static NSMutableArray<NSString *> *gALBGlassImmediateRecent;
/* Non-zero while ALBGlassSeed.h hands a new glass its start (main thread). */
static NSUInteger gALBSeedApplying;
/* 3.11.27: the window after one of our glasses came back (ALBGlassSeed.h):
   any snap window open at all (cheap), this owner's open, and the owner of
   a layer inside one of our edge pockets. */
static BOOL ALBGlassSeedSnapActive(void);
static BOOL ALBGlassSeedSnapping(UIView *owner);
static UIView *ALBGlassSeedPocketLayerOwner(CALayer *layer, NSUInteger *kind);
static NSUInteger gALBGlassImmediateWindowGlass, gALBGlassImmediatePocketMatched, gALBGlassImmediatePocketSnapped;
static NSCountedSet<NSString *> *gALBGlassImmediateSeenPaths;

static BOOL ALBGlassImmediateTonePath(NSString *path) {
    return [path isEqualToString:@"filters.glassBackground.inputFaceColorMatrixBlack"] ||
        [path isEqualToString:@"filters.glassBackground.inputFaceColorMatrixWhite"] ||
        [path isEqualToString:@"filters.glassBackground.inputFaceColorMatrixSaturation"] ||
        [path isEqualToString:@"filters.glassBackground.inputFaceColorMatrixFillColor"] ||
        [path isEqualToString:@"filters.glassBackground.inputFaceOpacity"];
}

/* Getter-only verification of the actual filter on this actual layer.
   A matching animation key/path alone is not sufficient evidence. */
static BOOL ALBGlassImmediateHasFilter(CALayer *layer) {
    @try {
        NSUInteger examined = 0;
        for (id filter in layer.filters) {
            if (examined++ >= 16) break;
            for (NSString *key in @[@"name", @"type"]) {
                id value = nil;
                @try { value = [filter valueForKey:key]; }
                @catch (__unused NSException *exception) { value = nil; }
                if ([value isKindOfClass:NSString.class] && [value isEqualToString:@"glassBackground"])
                    return YES;
            }
        }
    } @catch (__unused NSException *exception) { return NO; }
    return NO;
}

/* 1 right column, 2 palette, 0 outside the allowed owners. Find a UIView
   through at most 48 layer ancestors, then inspect at most 48 view ancestors.
   Finish the view walk even after finding an owner, to exclude top/bottom
   bars; a chain beyond the bound is rejected rather than guessed. */
static NSUInteger ALBGlassImmediateOwner(CALayer *layer, NSString **ownerType, UIView **ownerView) {
    UIView *start = nil;
    CALayer *cursor = layer;
    for (NSUInteger i = 0; cursor && i < 48; i++, cursor = cursor.superlayer) {
        id delegate = cursor.delegate;
        if ([delegate isKindOfClass:UIView.class]) { start = delegate; break; }
    }
    if (!start) return 0;

    Class faceClass = NSClassFromString(@"ALB3BarGlassButton");
    Class rightClass = NSClassFromString(@"ALBRightGlassButton");
    Class artworkClass = NSClassFromString(@"ALBArtworkGlassButton");
    Class sliderClass = NSClassFromString(@"ALBPaletteSlider");
    Class topClass = NSClassFromString(@"ALB3TopChromeView");
    UIView *owner = nil, *view = start;
    NSUInteger kind = 0;
    for (NSUInteger i = 0; view && i < 48; i++, view = view.superview) {
        if ((topClass && [view isKindOfClass:topClass]) || [view isKindOfClass:UITabBar.class]) return 0;
        if (sliderClass && [view isKindOfClass:sliderClass]) {
            if (!owner) { owner = view; kind = 2; }
        }
        if (!faceClass || ![view isKindOfClass:faceClass]) continue;
        UIView *button = view.superview;
        if (![button isKindOfClass:UIButton.class] || (UIView *)ALBFaceFor(button) != view) continue;
        NSUInteger candidate = 0;
        if ((rightClass && [button isKindOfClass:rightClass]) ||
            (artworkClass && [button isKindOfClass:artworkClass])) candidate = 1;
        else if ([button.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar.SidePalette."]) candidate = 2;
        if (!owner && candidate) { owner = button; kind = candidate; }
    }
    if (view || !owner) return 0;
    if (ownerType) *ownerType = NSStringFromClass(owner.class);
    if (ownerView) *ownerView = owner;
    return kind;
}

static void ALBGlassImmediateNote(NSString *path, CFTimeInterval duration, float originalSpeed,
                                 float fastSpeed, NSString *ownerType, NSUInteger kind,
                                 NSString *skipReason) {
    if (!gALBGlassImmediateRecent) gALBGlassImmediateRecent = [NSMutableArray array];
    if (gALBGlassImmediateRecent.count >= 16) [gALBGlassImmediateRecent removeObjectAtIndex:0];
    NSString *result = skipReason ?: [NSString stringWithFormat:@"加速至%.0f倍速，有效时长≈%.9fs",
        fastSpeed, duration / fastSpeed];
    [gALBGlassImmediateRecent addObject:[NSString stringWithFormat:@"+%.3fs %@ %@ 原时长=%.6fs 原速=%.3f %@ %@",
        MAX(0, CACurrentMediaTime() - gALBGlassImmediateInstalledAt), kind == 1 ? @"右侧" : @"面板",
        path, duration, originalSpeed, ownerType ?: @"?", result]];
}

/* Why a property animation is left as it is (nil: it can finish at once). */
static NSString *ALBGlassImmediateSkipReason(CAAnimation *animation) {
    CAPropertyAnimation *property = (CAPropertyAnimation *)animation;
    CFTimeInterval duration = animation.duration;
    float speed = animation.speed;
    if (property.additive || property.cumulative) return @"叠加/累积动画原样";
    if (animation.repeatCount != 0 || animation.repeatDuration != 0 || animation.autoreverses) return @"重复/往返动画原样";
    if (!isfinite(duration) || duration <= 0 || !isfinite(speed) || speed <= 0 || speed >= 100000000.0f)
        return @"时长/速度不适合加速，原样";
    if (animation.beginTime != 0 || animation.timeOffset != 0) return @"自定义开始/偏移时序原样";
    return nil;
}

/* The same animation, finishing at once at its own end value (spring
   response, from/to/by, keyframes, fill and delegate kept). */
static CAAnimation *ALBGlassImmediateFast(CAAnimation *animation) {
    CAAnimation *fast = [animation copy];
    fast.speed = (float)MIN(100000000.0, (double)animation.speed * 1000000.0);
    fast.beginTime = 0;
    fast.timeOffset = 0;
    return fast;
}

static void ALBGlassImmediateSeen(NSString *path, NSString *where) {
    if (!gALBGlassImmediateSeenPaths) gALBGlassImmediateSeenPaths = [NSCountedSet set];
    NSString *entry = [NSString stringWithFormat:@"%@ %@", where, path ?: @"<组/其他>"];
    if (gALBGlassImmediateSeenPaths.count < 32 || [gALBGlassImmediateSeenPaths containsObject:entry])
        [gALBGlassImmediateSeenPaths addObject:entry];
}

static void ALBGlassImmediateAddAnimation(CALayer *layer, SEL selector,
                                         CAAnimation *animation, NSString *key) {
    CAAnimation *forwarded = animation;
    /* UIKit/layer animations submitted off-main are always forwarded untouched.
       No diagnostics or owner traversal happens on that path. */
    if (NSThread.isMainThread) {
        @try {
            BOOL propertyAnimation = [animation isKindOfClass:CABasicAnimation.class] ||
                [animation isKindOfClass:CAKeyframeAnimation.class];
            NSString *path = propertyAnimation ? ((CAPropertyAnimation *)animation).keyPath : nil;
            BOOL groupAnimation = [animation isKindOfClass:CAAnimationGroup.class];
            BOOL window = gALBSeedApplying == 0 && ALBGlassSeedSnapActive();
            // 3.11.27: the pocket's own level, inside one of our edge pockets.
            if (window && propertyAnimation && !ALBGlassImmediateTonePath(path) &&
                ([path isEqualToString:@"opacity"] || [path hasPrefix:@"filters."]) && AWEPluginFeaturesEnabled()) {
                NSUInteger pocketKind = 0;
                UIView *pocketOwner = ALBGlassSeedPocketLayerOwner(layer, &pocketKind);
                if (pocketOwner && ALBGlassSeedSnapping(pocketOwner)) {
                    gALBGlassImmediatePocketMatched++;
                    NSString *skipReason = ALBGlassImmediateSkipReason(animation);
                    ALBGlassImmediateSeen(path, skipReason ? @"口袋(原样)" : @"口袋");
                    if (!skipReason) {
                        forwarded = ALBGlassImmediateFast(animation);
                        gALBGlassImmediatePocketSnapped++;
                        ALBGlassImmediateNote(path, animation.duration, animation.speed, forwarded.speed,
                            NSStringFromClass(pocketOwner.class), pocketKind, nil);
                    }
                }
            }
            if ((ALBGlassImmediateTonePath(path) || groupAnimation) && (gALBSeedApplying > 0 || window) &&
                AWEPluginFeaturesEnabled() &&
                ALBGlassImmediateHasFilter(layer)) {
                NSString *ownerType = nil;
                UIView *ownerView = nil;
                NSUInteger kind = ALBGlassImmediateOwner(layer, &ownerType, &ownerView);
                // Outside the hand-over itself: only the owner's own window.
                if (kind && gALBSeedApplying == 0) {
                    if (!ALBGlassSeedSnapping(ownerView)) kind = 0;
                    else gALBGlassImmediateWindowGlass++;
                }
                if (kind) ALBGlassImmediateSeen(path, kind == 1 ? @"右侧玻璃" : @"面板玻璃");
                if (kind) {
                    CFTimeInterval originalDuration = animation.duration;
                    float originalSpeed = animation.speed;
                    if (groupAnimation) {
                        /* A group can contain timing dependencies or spring
                           subanimations. Do not unwrap or alter any child. */
                        gALBGlassImmediateGroupsSkipped++;
                        ALBGlassImmediateNote(@"<动画组，未展开>", originalDuration, originalSpeed,
                            0, ownerType, kind, @"动画组原样；不代表命中五项色调路径");
                    } else {
                        if (kind == 1) gALBGlassImmediateRightMatched++;
                        else gALBGlassImmediatePanelMatched++;
                        NSString *skipReason = ALBGlassImmediateSkipReason(animation);
                        if (skipReason) {
                            gALBGlassImmediateComplexSkipped++;
                            ALBGlassImmediateNote(path, originalDuration, originalSpeed,
                                0, ownerType, kind, skipReason);
                        } else {
                            /* Keep the entire native physical/timing interval,
                               including CASpringAnimation's spring response.
                               Merely clipping a spring's duration can stop at
                               its starting value. copy also preserves native
                               from/to/by, keyframes, easing, delegate and fill. */
                            CAAnimation *fast = ALBGlassImmediateFast(animation);
                            ALBGlassImmediateNote(path, originalDuration, originalSpeed,
                                fast.speed, ownerType, kind, nil);
                            if (kind == 1) gALBGlassImmediateRightShortened++;
                            else gALBGlassImmediatePanelShortened++;
                            forwarded = fast;
                        }
                    }
                }
            }
        } @catch (__unused NSException *exception) {
            gALBGlassImmediateExceptions++;
            forwarded = animation;
        }
    }
    /* One and only one call to the implementation that was effective on
       CABackdropLayer before installation, regardless of every filter above. */
    gALBGlassImmediateOriginal(layer, selector, forwarded, key);
}

static const char *ALBGlassImmediateUnqualifiedType(const char *type) {
    while (type && *type && strchr("rnNoORV", *type)) type++;
    return type;
}

static BOOL ALBGlassImmediateMethodSignature(Method method) {
    if (!method || method_getNumberOfArguments(method) != 4) return NO;
    char result[32] = {0};
    method_getReturnType(method, result, sizeof(result));
    if (*ALBGlassImmediateUnqualifiedType(result) != 'v') return NO;
    const char expected[] = {'@', ':', '@', '@'};
    for (NSUInteger i = 0; i < 4; i++) {
        char argument[64] = {0};
        method_getArgumentType(method, (unsigned int)i, argument, sizeof(argument));
        if (*ALBGlassImmediateUnqualifiedType(argument) != expected[i]) return NO;
    }
    return YES;
}

static void ALBGlassImmediateInstall(void) {
    /* The caller installs at its main-thread startup entry. A background
       call does not mutate either runtime hooks or main-thread diagnostics. */
    if (!NSThread.isMainThread || gALBGlassImmediateAttempted) return;
    gALBGlassImmediateAttempted = YES;
    if (@available(iOS 26.0, *)) {
        Class backdropClass = NSClassFromString(@"CABackdropLayer");
        if (!backdropClass) { gALBGlassImmediateStatus = @"安装失败：CABackdropLayer 不存在"; return; }
        SEL selector = @selector(addAnimation:forKey:);
        Method method = class_getInstanceMethod(backdropClass, selector);
        if (!ALBGlassImmediateMethodSignature(method)) {
            gALBGlassImmediateStatus = @"安装失败：addAnimation:forKey: 签名不符";
            return;
        }
        IMP original = method_getImplementation(method);
        if (!original || original == (IMP)ALBGlassImmediateAddAnimation) {
            gALBGlassImmediateStatus = @"安装失败：原始实现不可用";
            return;
        }
        gALBGlassImmediateOriginal = (ALBGlassImmediateAddAnimationIMP)original;
        if (!class_addMethod(backdropClass, selector, (IMP)ALBGlassImmediateAddAnimation,
                             method_getTypeEncoding(method))) {
            /* An own implementation exists: replace only that class's own
               method, retaining its actual IMP. Never swizzle CALayer. */
            Method own = class_getInstanceMethod(backdropClass, selector);
            Method inherited = class_getInstanceMethod(class_getSuperclass(backdropClass), selector);
            if (own == inherited) {
                gALBGlassImmediateStatus = @"安装失败：未找到 CABackdropLayer 自身方法，保留父类";
                return;
            }
            if (!ALBGlassImmediateMethodSignature(own)) {
                gALBGlassImmediateStatus = @"安装失败：实际方法签名不符";
                return;
            }
            gALBGlassImmediateOriginal = (ALBGlassImmediateAddAnimationIMP)
                method_setImplementation(own, (IMP)ALBGlassImmediateAddAnimation);
        }
        gALBGlassImmediateInstalled = YES;
        gALBGlassImmediateInstalledAt = CACurrentMediaTime();
        gALBGlassImmediateStatus = @"已安装（仅 CABackdropLayer）";
    } else {
        gALBGlassImmediateStatus = @"未安装：需要 iOS 26 或更高版本";
    }
}

static NSString *ALBGlassImmediateReport(void) {
    if (!NSThread.isMainThread) return @"原生色调动画诊断需在主线程读取";
    NSMutableString *text = [NSMutableString stringWithFormat:
        @"回来后的深浅过渡（3.11.27：交值那一刻和回来后 1.5 秒内瞬时完成）：状态=%@；插件=%@\n"
        @"玻璃五项色调：右侧匹配=%lu 瞬时=%lu；面板匹配=%lu 瞬时=%lu（其中在回来窗口里%lu）；口袋档位：匹配=%lu 瞬时=%lu；跳过复杂动画=%lu；保留动画组=%lu；异常原样=%lu",
        gALBGlassImmediateStatus, AWEPluginFeaturesEnabled() ? @"开" : @"关",
        (unsigned long)gALBGlassImmediateRightMatched, (unsigned long)gALBGlassImmediateRightShortened,
        (unsigned long)gALBGlassImmediatePanelMatched, (unsigned long)gALBGlassImmediatePanelShortened,
        (unsigned long)gALBGlassImmediateWindowGlass,
        (unsigned long)gALBGlassImmediatePocketMatched, (unsigned long)gALBGlassImmediatePocketSnapped,
        (unsigned long)gALBGlassImmediateComplexSkipped, (unsigned long)gALBGlassImmediateGroupsSkipped,
        (unsigned long)gALBGlassImmediateExceptions];
    if (!gALBGlassImmediateInstalled) [text appendString:@"\n挂钩未安装，未改变动画。"];
    if (gALBGlassImmediateRightMatched + gALBGlassImmediatePanelMatched + gALBGlassImmediatePocketMatched == 0)
        [text appendString:@"\n回来窗口里没有见到这些动画。"];
    if (gALBGlassImmediateSeenPaths.count) {
        NSMutableArray<NSString *> *parts = [NSMutableArray array];
        for (NSString *entry in [gALBGlassImmediateSeenPaths.allObjects sortedArrayUsingSelector:@selector(compare:)])
            [parts addObject:[NSString stringWithFormat:@"%@×%lu", entry, (unsigned long)[gALBGlassImmediateSeenPaths countForObject:entry]]];
        [text appendFormat:@"\n窗口里见到的动画：%@", [parts componentsJoinedByString:@"；"]];
    }
    if (gALBGlassImmediateRecent.count)
        [text appendFormat:@"\n最近最多16条：\n%@", [gALBGlassImmediateRecent componentsJoinedByString:@"\n"]];
    return text;
}

#endif
