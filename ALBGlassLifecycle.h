/* Included once, after ALBGlassTone.h, by Tweak.x.

   A short, explicitly started, READ-ONLY lifecycle recorder.
   The recorder never
   changes UIKit's glass, registrations, traits, visibility or layout, and
   never captures the screen. All UI references held between samples are
   weak. Object numbers are associated monotonically increasing numbers,
   not addresses (a reused address must not look like the same object).

   Deliberately no luminance/delegate hook: the previously observed edge
   callbacks did not establish what controls the actual glass. This records
   object/registration/visibility changes only. Class-name glass candidates
   and backdrop layers are evidence of their own lifetime, not evidence of
   brightness, rendering activity or the reason a glass changes tone.
 */

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>

static const NSUInteger kALBGlassLifecycleCapacity = 240;
static const CFTimeInterval kALBGlassLifecycleDuration = 90.0;
static char kALBGlassLifecycleIdentityKey;
static NSUInteger gALBGlassLifecycleNextIdentity;

@interface ALBGlassLifecycleWatch : NSObject
@property(nonatomic, weak) UIView *view;
@property(nonatomic, copy) NSString *label;
@property(nonatomic) NSUInteger identity;
@property(nonatomic, copy) NSString *lastState;
@property(nonatomic, copy) NSString *lastParts;
@property(nonatomic) BOOL wasVisible;
@property(nonatomic) CFTimeInterval warmUntil;
@property(nonatomic) CFTimeInterval lastWarm;
@end
@implementation ALBGlassLifecycleWatch
@end

static NSTimer *gALBGlassLifecycleTimer;
static NSMutableArray<ALBGlassLifecycleWatch *> *gALBGlassLifecycleWatches;
static NSMutableArray<NSString *> *gALBGlassLifecycleLines;
static NSUInteger gALBGlassLifecycleRingNext, gALBGlassLifecycleLineCount;
static CFTimeInterval gALBGlassLifecycleStarted, gALBGlassLifecycleEnded;
static NSDate *gALBGlassLifecycleStartDate, *gALBGlassLifecycleEndDate;
static BOOL gALBGlassLifecycleRunning, gALBGlassLifecyclePaused, gALBGlassLifecycleSampling;
static NSString *gALBGlassLifecycleContext, *gALBGlassLifecycleEndReason;
static NSMutableDictionary<NSNumber *, NSString *> *gALBGlassLifecycleLegend;
static NSMutableArray<NSNumber *> *gALBGlassLifecycleLegendOrder;

static NSUInteger ALBGlassLifecycleID(id object) {
    if (!object) return 0;
    NSNumber *number = objc_getAssociatedObject(object, &kALBGlassLifecycleIdentityKey);
    if (!number) {
        number = @(++gALBGlassLifecycleNextIdentity);
        objc_setAssociatedObject(object, &kALBGlassLifecycleIdentityKey, number, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (gALBGlassLifecycleRunning && gALBGlassLifecycleLegend.count < 256 && !gALBGlassLifecycleLegend[number]) {
        NSString *name = NSStringFromClass([object class]);
        gALBGlassLifecycleLegend[number] = name.length > 64 ? [name substringFromIndex:name.length - 64] : name;
        [gALBGlassLifecycleLegendOrder addObject:number];
    }
    return number.unsignedIntegerValue;
}
static NSString *ALBGlassLifecycleObject(id object) {
    return [NSString stringWithFormat:@"#%lu", (unsigned long)ALBGlassLifecycleID(object)];
}
static void ALBGlassLifecycleAppend(NSString *text) {
    if (!gALBGlassLifecycleLines || !text.length) return;
    NSString *line = [NSString stringWithFormat:@"+%05.2fs %@", MAX(0, CACurrentMediaTime() - gALBGlassLifecycleStarted), text];
    if (gALBGlassLifecycleLines.count < kALBGlassLifecycleCapacity) {
        [gALBGlassLifecycleLines addObject:line];
    } else {
        gALBGlassLifecycleLines[gALBGlassLifecycleRingNext] = line;
        gALBGlassLifecycleRingNext = (gALBGlassLifecycleRingNext + 1) % kALBGlassLifecycleCapacity;
    }
    gALBGlassLifecycleLineCount++;
}

/* Geometry only: a covered or alpha-zero view can still be in the window.
   Never call hitTest, layoutIfNeeded or a glass/material getter here. */
static BOOL ALBGlassLifecycleOnScreen(UIView *view) {
    UIWindow *window = view.window;
    if (!view || !window || CGRectIsEmpty(view.bounds)) return NO;
    CGRect rect = [view convertRect:view.bounds toView:window];
    if (CGRectIsNull(rect) || CGRectIsInfinite(rect) || CGRectIsEmpty(rect)) return NO;
    return CGRectIntersectsRect(window.bounds, rect);
}
static NSString *ALBGlassLifecycleVisibility(UIView *view, BOOL *visible) {
    if (visible) *visible = NO;
    if (!view) return @"无";
    CGFloat alpha = 1, opacity = 1;
    UIView *hiddenBy = nil;
    CALayer *layerHiddenBy = nil;
    UIView *cursor = view;
    NSUInteger depth = 0;
    for (; cursor && depth < 64; cursor = cursor.superview, depth++) {
        alpha *= MAX(0, MIN(1, cursor.alpha));
        opacity *= MAX(0, MIN(1, cursor.layer.opacity));
        if (!hiddenBy && cursor.hidden) hiddenBy = cursor;
        if (!layerHiddenBy && cursor.layer.hidden) layerHiddenBy = cursor.layer;
    }
    BOOL screen = ALBGlassLifecycleOnScreen(view);
    // UIView alpha getters can be virtualized by this project's native
    // visibility leases. Record both, and use model layer opacity for the
    // diagnostic's visibility transition, without reading presentation or
    // claiming that a model layer is actually being rendered.
    if (visible) *visible = screen && !hiddenBy && !layerHiddenBy && opacity > 0.01;
    return [NSString stringWithFormat:@"窗%lu/父%lu/屏%@/隐%lu/层隐%lu/α%.2f/o累%.2f/式%ld:%ld%@",
        (unsigned long)ALBGlassLifecycleID(view.window), (unsigned long)ALBGlassLifecycleID(view.superview),
        screen ? @"内" : @"外", (unsigned long)ALBGlassLifecycleID(hiddenBy),
        (unsigned long)ALBGlassLifecycleID(layerHiddenBy), alpha, opacity,
        (long)view.overrideUserInterfaceStyle, (long)view.traitCollection.userInterfaceStyle,
        cursor ? @"/祖先截断" : @""];
}

/* Every traversal has both a visit cap and a queue cap. In particular,
   appending a very large subviews/sublayers array cannot escape the cap. */
static void ALBGlassLifecycleQueue(NSMutableArray *queue, NSArray *children, NSUInteger cap) {
    for (id child in children) {
        if (queue.count >= cap) break;
        [queue addObject:child];
    }
}
static NSDictionary<NSString *, NSString *> *ALBGlassLifecycleGlassViews(UIView *root) {
    NSMutableArray<NSString *> *parts = [NSMutableArray array], *ids = [NSMutableArray array];
    if (root) {
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
        for (NSUInteger i = 0; i < queue.count && i < 96; i++) {
            UIView *view = queue[i];
            NSString *name = NSStringFromClass(view.class);
            if ([name containsString:@"PlatterGlass"] || [name containsString:@"GlassView"]) {
                [ids addObject:[NSString stringWithFormat:@"%lu", (unsigned long)ALBGlassLifecycleID(view)]];
                [parts addObject:[NSString stringWithFormat:@"%@(h%da%.2f)", ALBGlassLifecycleObject(view), view.hidden, view.alpha]];
                if (parts.count >= 3) break;
            }
            ALBGlassLifecycleQueue(queue, view.subviews, 96);
        }
    }
    return @{@"ids": [ids componentsJoinedByString:@","], @"text": [parts componentsJoinedByString:@","]};
}
static NSDictionary<NSString *, NSString *> *ALBGlassLifecycleBackdrops(UIView *root, NSUInteger cap) {
    NSMutableArray<NSString *> *parts = [NSMutableArray array], *ids = [NSMutableArray array];
    if (root) {
        NSMutableArray<CALayer *> *queue = [NSMutableArray arrayWithObject:root.layer];
        for (NSUInteger i = 0; i < queue.count && i < cap; i++) {
            CALayer *layer = queue[i];
            NSString *name = NSStringFromClass(layer.class);
            if ([name containsString:@"Backdrop"]) {
                [ids addObject:[NSString stringWithFormat:@"%lu", (unsigned long)ALBGlassLifecycleID(layer)]];
                [parts addObject:[NSString stringWithFormat:@"%@(h%do%.2f)", ALBGlassLifecycleObject(layer), layer.hidden, layer.opacity]];
                if (parts.count >= 3) break;
            }
            ALBGlassLifecycleQueue(queue, layer.sublayers, cap);
        }
    }
    return @{@"ids": [ids componentsJoinedByString:@","], @"text": [parts componentsJoinedByString:@","]};
}
static NSString *ALBGlassLifecycleRegistration(UIView *surface, UIView *proxy) {
    NSUInteger depth = 0;
    for (UIView *cursor = surface; cursor && depth < 6; cursor = cursor.superview, depth++) {
        NSUInteger seen = 0;
        for (id<UIInteraction> interaction in cursor.interactions) {
            if (seen++ >= 8) break;
            NSObject *object = (NSObject *)interaction;
            if (![NSStringFromClass(object.class) containsString:@"ScrollEdgeElementContainer"]) continue;
            // The same established getter keys used by ALB35RegistrationText;
            // no speculative luminance/material KVC and no setter.
            id scroll = nil, edge = nil;
            @try {
                if ([object respondsToSelector:NSSelectorFromString(@"scrollView")]) scroll = [object valueForKey:@"scrollView"];
                if ([object respondsToSelector:NSSelectorFromString(@"edge")]) edge = [object valueForKey:@"edge"];
            } @catch (__unused NSException *exception) {}
            return [NSString stringWithFormat:@"I#%lu/S%@/e%@/d%lu/E%@",
                (unsigned long)ALBGlassLifecycleID(object), ALBGlassLifecycleObject(scroll),
                [edge isKindOfClass:NSNumber.class] ? edge : @"?", (unsigned long)depth,
                proxy ? (scroll == proxy ? @"同" : @"异") : @"无"];
        }
    }
    return @"未检出";
}

static NSDictionary *ALBGlassLifecycleSnapshot(ALBGlassLifecycleWatch *watch) {
    UIView *owner = watch.view;
    if (!owner) {
        NSString *text = watch.identity
            ? [NSString stringWithFormat:@"对象#%lu 已销毁（弱引用已清空）", (unsigned long)watch.identity]
            : @"尚未出现";
        return @{@"text": text, @"parts": @"", @"visible": @NO};
    }
    UIView *face = [owner isKindOfClass:UIButton.class] ? (UIView *)ALBFaceFor(owner) : nil;
    UIView *surface = face ?: owner;
    UIView *proxy = objc_getAssociatedObject(surface, &kALBFollowProxyKey);
    BOOL ownerVisible = NO, surfaceVisible = NO;
    NSString *ownerVisibility = ALBGlassLifecycleVisibility(owner, &ownerVisible);
    NSString *surfaceVisibility = face ? ALBGlassLifecycleVisibility(face, &surfaceVisible) : ownerVisibility;
    if (!face) surfaceVisible = ownerVisible;
    NSDictionary *glass = ALBGlassLifecycleGlassViews(surface);
    NSDictionary *layers = ALBGlassLifecycleBackdrops(surface, 96);
    NSDictionary *edgeLayers = ALBGlassLifecycleBackdrops(proxy, 64);
    NSString *parts = [NSString stringWithFormat:@"%lu/%lu/%@/%@/%@",
        (unsigned long)ALBGlassLifecycleID(face), (unsigned long)ALBGlassLifecycleID(proxy),
        glass[@"ids"], layers[@"ids"], edgeLayers[@"ids"]];
    NSString *text = [NSString stringWithFormat:
        @"O%@{%@} F%@{%@} E%@{%@} M%ld R{%@} G[%@] B[%@] EB[%@]",
        ALBGlassLifecycleObject(owner), ownerVisibility, ALBGlassLifecycleObject(face), face ? surfaceVisibility : @"",
        ALBGlassLifecycleObject(proxy), proxy ? ALBGlassLifecycleVisibility(proxy, NULL) : @"",
        (long)[objc_getAssociatedObject(surface, &kALBFollowModeKey) integerValue], ALBGlassLifecycleRegistration(surface, proxy),
        glass[@"text"], layers[@"text"], edgeLayers[@"text"]];
    return @{@"text": text, @"parts": parts, @"visible": @(ownerVisible && surfaceVisible)};
}
static void ALBGlassLifecycleSampleWatch(ALBGlassLifecycleWatch *watch, CFTimeInterval now, NSString *forcedReason) {
    NSDictionary *snapshot = ALBGlassLifecycleSnapshot(watch);
    NSString *state = snapshot[@"text"], *parts = snapshot[@"parts"];
    BOOL visible = [snapshot[@"visible"] boolValue];
    BOOL appeared = !watch.lastState;
    BOOL restored = visible && !watch.wasVisible;
    BOOL partsChanged = watch.lastParts && ![watch.lastParts isEqualToString:parts];
    if ((appeared && watch.view) || restored || (partsChanged && watch.view)) watch.warmUntil = now + 1.5;
    BOOL changed = ![watch.lastState isEqualToString:state];
    BOOL warm = watch.view && now <= watch.warmUntil && now - watch.lastWarm >= 0.5;
    if (forcedReason || changed || warm) {
        NSString *reason = forcedReason ?: (appeared ? @"首次观察" : (restored ? @"可见性恢复" : (changed ? @"状态变化" : @"短时复查")));
        NSString *detail = changed ? state : [NSString stringWithFormat:@"对象#%lu 状态同前", (unsigned long)watch.identity];
        ALBGlassLifecycleAppend([NSString stringWithFormat:@"%@ [%@] %@", watch.label, reason, detail]);
        watch.lastWarm = now;
    }
    watch.lastState = state;
    watch.lastParts = parts;
    watch.wasVisible = visible;
}
static void ALBGlassLifecycleAdopt(ALBGlassLifecycleWatch *watch, UIView *candidate, CFTimeInterval now) {
    if (!candidate || watch.view == candidate) return;
    if (watch.identity) ALBGlassLifecycleSampleWatch(watch, now, @"原对象结束跟踪");
    watch.view = candidate;
    watch.identity = ALBGlassLifecycleID(candidate);
    watch.lastState = nil;
    watch.lastParts = nil;
    watch.wasVisible = NO;
    watch.warmUntil = now + 1.5;
    watch.lastWarm = -10;
}

static NSString *ALBGlassLifecycleScene(void) {
    // The existing sensitive-controller registry, not a scan of the app's
    // controller/view tree. Attachment is reported as attachment, not as a
    // claim that the controller covers a glass.
    NSMutableArray<NSString *> *sensitive = [NSMutableArray array];
    NSUInteger visited = 0;
    for (UIViewController *controller in AWESensitiveVisibleControllers()) {
        if (visited++ >= 32) break;
        if (!controller.viewIfLoaded.window) continue;
        [sensitive addObject:ALBGlassLifecycleObject(controller)];
        if (sensitive.count >= 4) break;
    }
    [sensitive sortUsingSelector:@selector(compare:)];
    ALBSidePalette *palette = gALBSidePalette;
    return [NSString stringWithFormat:@"场景 设置=%d 面板#%lu/窗#%lu 右隐藏=%d UIKit菜单(主窗浅层)=%d 最近评估路由=%ld 敏感控制器(挂窗)=[%@]",
        gAWESettingsVisible, (unsigned long)ALBGlassLifecycleID(palette), (unsigned long)ALBGlassLifecycleID(palette.window),
        gALB8PaletteRightHidden, ALBToneMenuShowing(AWEActiveMainWindow()), (long)gALB30LastEvaluatedRoute,
        [sensitive componentsJoinedByString:@","]];
}
static void ALBGlassLifecycleCapture(CFTimeInterval now) {
    if (gALBGlassLifecycleSampling || !gALBGlassLifecycleRunning) return;
    gALBGlassLifecycleSampling = YES;
    @try {
        NSString *scene = ALBGlassLifecycleScene();
        if (![gALBGlassLifecycleContext isEqualToString:scene]) {
            gALBGlassLifecycleContext = scene;
            ALBGlassLifecycleAppend(scene);
        }
        ALBGlassLifecycleWatch *action = gALBGlassLifecycleWatches[0], *artwork = gALBGlassLifecycleWatches[1];
        UIView *actionCandidate = ALBGlassLifecycleOnScreen(action.view) ? action.view : nil;
        UIView *artworkCandidate = ALBGlassLifecycleOnScreen(artwork.view) ? artwork.view : nil;
        // Keep the selected on-screen objects even while their ancestor's
        // alpha is zero. Re-select only after they leave screen geometry.
        NSUInteger visited = 0;
        for (UIButton *button in ALBRGButtons()) {
            if (visited++ >= 192 || (actionCandidate && artworkCandidate)) break;
            if (!ALBGlassLifecycleOnScreen(button)) continue;
            if (!actionCandidate && [button isKindOfClass:ALBRightGlassButton.class]) actionCandidate = button;
            else if (!artworkCandidate && [button isKindOfClass:ALBArtworkGlassButton.class]) artworkCandidate = button;
        }
        ALBGlassLifecycleAdopt(action, actionCandidate, now);
        ALBGlassLifecycleAdopt(artwork, artworkCandidate, now);
        ALBSidePalette *palette = gALBSidePalette;
        UIView *pill = nil;
        NSUInteger pills = 0;
        for (UIView *view in palette.pills) {
            if (pills++ >= 32) break;
            if (![view isKindOfClass:ALBPaletteSlider.class]) { pill = view; break; }
        }
        ALBGlassLifecycleAdopt(gALBGlassLifecycleWatches[2], palette.icons.firstObject, now);
        ALBGlassLifecycleAdopt(gALBGlassLifecycleWatches[3], pill, now);
        ALBGlassLifecycleAdopt(gALBGlassLifecycleWatches[4], palette.volume, now);
        ALBGlassLifecycleAdopt(gALBGlassLifecycleWatches[5], ALBToneTopChrome(), now);
        for (ALBGlassLifecycleWatch *watch in gALBGlassLifecycleWatches) ALBGlassLifecycleSampleWatch(watch, now, nil);
    } @finally {
        gALBGlassLifecycleSampling = NO;
    }
}

static void ALBGlassLifecycleFinish(NSString *reason) {
    if (!gALBGlassLifecycleRunning) return;
    gALBGlassLifecycleRunning = NO;
    [gALBGlassLifecycleTimer invalidate];
    gALBGlassLifecycleTimer = nil;
    gALBGlassLifecycleEnded = CACurrentMediaTime();
    gALBGlassLifecycleEndDate = NSDate.date;
    gALBGlassLifecycleEndReason = reason;
    ALBGlassLifecycleAppend([NSString stringWithFormat:@"记录结束：%@；不再采样。", reason]);
    // The log contains only strings; stopped recordings retain no watches.
    gALBGlassLifecycleWatches = nil;
}
static void ALBGlassLifecycleTick(void) {
    if (!gALBGlassLifecycleRunning) return;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - gALBGlassLifecycleStarted >= kALBGlassLifecycleDuration) {
        ALBGlassLifecycleFinish(@"90秒到期（按实际经过时间）");
        return;
    }
    if (UIApplication.sharedApplication.applicationState != UIApplicationStateActive) {
        if (!gALBGlassLifecyclePaused) ALBGlassLifecycleAppend(@"应用非活跃，暂停采样；90秒期限继续计时。");
        gALBGlassLifecyclePaused = YES;
        return;
    }
    if (gALBGlassLifecyclePaused) ALBGlassLifecycleAppend(@"应用恢复活跃，继续采样。");
    gALBGlassLifecyclePaused = NO;
    ALBGlassLifecycleCapture(now);
}
static void ALBGlassLifecycleStart(void) {
    if (!NSThread.isMainThread) {
        dispatch_async(dispatch_get_main_queue(), ^{ ALBGlassLifecycleStart(); });
        return;
    }
    [gALBGlassLifecycleTimer invalidate];
    gALBGlassLifecycleTimer = nil;
    gALBGlassLifecycleLines = [NSMutableArray arrayWithCapacity:kALBGlassLifecycleCapacity];
    gALBGlassLifecycleLegend = [NSMutableDictionary dictionaryWithCapacity:256];
    gALBGlassLifecycleLegendOrder = [NSMutableArray arrayWithCapacity:256];
    gALBGlassLifecycleRingNext = 0;
    gALBGlassLifecycleLineCount = 0;
    gALBGlassLifecycleStarted = CACurrentMediaTime();
    gALBGlassLifecycleEnded = 0;
    gALBGlassLifecycleStartDate = NSDate.date;
    gALBGlassLifecycleEndDate = nil;
    gALBGlassLifecycleEndReason = nil;
    gALBGlassLifecycleContext = nil;
    gALBGlassLifecyclePaused = NO;
    gALBGlassLifecycleSampling = NO;
    gALBGlassLifecycleRunning = YES;
    gALBGlassLifecycleWatches = [NSMutableArray arrayWithCapacity:6];
    for (NSString *label in @[@"右侧动作", @"右侧头像/唱片", @"面板首图标", @"面板首文字", @"面板音量", @"顶栏参照"]) {
        ALBGlassLifecycleWatch *watch = [ALBGlassLifecycleWatch new];
        watch.label = label;
        watch.lastWarm = -10;
        [gALBGlassLifecycleWatches addObject:watch];
    }
    ALBGlassLifecycleAppend(@"记录开始：最多90秒，活跃时每0.25秒观察；保留最近240条。");
    gALBGlassLifecycleTimer = [NSTimer timerWithTimeInterval:0.25 repeats:YES block:^(__unused NSTimer *timer) {
        ALBGlassLifecycleTick();
    }];
    gALBGlassLifecycleTimer.tolerance = 0.04;
    [NSRunLoop.mainRunLoop addTimer:gALBGlassLifecycleTimer forMode:NSRunLoopCommonModes];
    ALBGlassLifecycleTick();
}
static void ALBGlassLifecycleStop(void) {
    if (!NSThread.isMainThread) {
        dispatch_async(dispatch_get_main_queue(), ^{ ALBGlassLifecycleStop(); });
        return;
    }
    ALBGlassLifecycleFinish(@"用户手动结束");
}
static void ALBGlassLifecycleMark(NSString *event) {
    // Called by exact panel/visibility lifecycle sites. No recording means
    // an immediate return; recording never forces a layout or a commit.
    if (!gALBGlassLifecycleRunning || !NSThread.isMainThread) return;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - gALBGlassLifecycleStarted >= kALBGlassLifecycleDuration) {
        ALBGlassLifecycleFinish(@"90秒到期（按实际经过时间）");
        return;
    }
    NSString *limited = event.length > 160 ? [event substringToIndex:160] : event;
    ALBGlassLifecycleAppend([NSString stringWithFormat:@"事件：%@", limited ?: @"未命名"]);
    if (UIApplication.sharedApplication.applicationState == UIApplicationStateActive) ALBGlassLifecycleCapture(now);
}
static NSString *ALBGlassLifecycleReport(void) {
    if (!gALBGlassLifecycleStartDate) return @"玻璃生命周期：尚未开始记录（设置页的诊断菜单可启动90秒记录）。面板打开按原有行为隐藏右侧，关闭后恢复；记录器只读。";
    if (NSThread.isMainThread && gALBGlassLifecycleRunning &&
        CACurrentMediaTime() - gALBGlassLifecycleStarted >= kALBGlassLifecycleDuration)
        ALBGlassLifecycleFinish(@"90秒到期（按实际经过时间）");
    CFTimeInterval elapsed = (gALBGlassLifecycleRunning ? CACurrentMediaTime() : gALBGlassLifecycleEnded) - gALBGlassLifecycleStarted;
    NSMutableString *report = [NSMutableString stringWithFormat:
        @"玻璃生命周期（只读）\n开始：%@\n状态：%@，经过%.1f秒；结束：%@\n"
        "记录范围：屏内最多两颗右侧代表（动作、头像/唱片），面板首图标、首文字、音量滑条，顶栏参照；离屏后弱跟踪最后对象，出现下一对象时换样。\n"
        "限制：未读取实际亮度、系统最终玻璃深浅或渲染活动；记录不能单独证明读数暂停、起步深色或压暗是根因。玻璃候选按类名查找（96视图上限），Backdrop按层类名查找（本体96层、边64层上限），空列表不等于不存在。屏内只判断与窗口矩形相交，不判断被遮挡或祖先裁剪。α为祖先视图alpha getter乘积，getter可能被宿主/插件虚拟化；o累为祖先模型层opacity乘积，用于辅助判定隐藏/恢复。隐/层隐0表示未见hidden视图/层，式为override:trait；未读presentation，这些模型值不代表最终画面透明度或渲染活动。\n"
        "事件行只标记调用时间；其后的样本来自当时的当前对象。旧面板关闭完成回调可能晚于新面板创建，须按对象编号判断，不能把相邻行当作同一面板。最近评估路由是现有缓存原值（可为-1），不代表实时路由；菜单只检测主窗浅层UIKit菜单。\n"
        "对象#编号在本进程单调分配，不用内存地址判定重建；记录器只读，不修改玻璃、登记、布局、alpha、顶栏或底栏，没有截图。面板打开按原有行为隐藏右侧，关闭后恢复；此显隐行为不代表玻璃首帧问题已修复。\n"
        "缩写：O本体/F栏按钮face/E空滚动边/M自身登记模式/R登记/G玻璃候选/B本体Backdrop/EB边Backdrop；R中I交互、S滚动视图、e边值、d祖先层数、E是否同proxy；h隐藏/a视图alpha/o层opacity。窗/父/隐后的数也是对象编号，0为无。\n"
        "共写入%lu条，保留%lu条%@。变化时写完整状态；新出现/恢复/内部变化后1.5秒内每0.5秒复查，无变化只写“同前”。\n",
        gALBGlassLifecycleStartDate, gALBGlassLifecycleRunning ? @"正在记录" : (gALBGlassLifecycleEndReason ?: @"已结束"),
        MAX(0, elapsed), gALBGlassLifecycleEndDate ?: (id)@"尚未结束",
        (unsigned long)gALBGlassLifecycleLineCount, (unsigned long)gALBGlassLifecycleLines.count,
        gALBGlassLifecycleLineCount > kALBGlassLifecycleCapacity ? @"（较早记录已被环形覆盖）" : @""];
    NSUInteger count = gALBGlassLifecycleLines.count;
    NSUInteger first = count == kALBGlassLifecycleCapacity ? gALBGlassLifecycleRingNext : 0;
    for (NSUInteger i = 0; i < count; i++) [report appendFormat:@"%@\n", gALBGlassLifecycleLines[(first + i) % count]];
    [report appendString:@"类名图例（最多256项；超出仍保留稳定编号）：\n"];
    for (NSNumber *number in gALBGlassLifecycleLegendOrder) [report appendFormat:@"#%@=%@\n", number, gALBGlassLifecycleLegend[number]];
    return report;
}
