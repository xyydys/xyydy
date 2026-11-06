//
//  ALBSheetGlass.xm —— 长按面板的背景换成液态玻璃（3.11.34）
//
//  用户：多功能插件的长按面板（取代了抖音原来长按弹出的面板，里面有抖音原来的分享等功能，加上插件
//  自己的解析下载）只改背景成液态玻璃；每个模块那块灰色半透明不用动（或者改小圆角也行，现在有点大）；
//  整块背景的圆角像评论区一样能自己调。
//
//  背景探针（3.11.33）：面板是抖音自己的底部弹出组件，在抖音主窗口里——
//    UITransitionView ┬ DUXAccessibilityDismissView（全屏，点空白处关闭）
//                     └ UIView（外框：圆角 20、裁边、从 y=382 起）
//                         └ UIView（574 高）┬ UIView（透明）
//                                          └ DUXVisualEffectView（整块背景：模糊＋73% #1C1C1C，面板内容在它的 contentView 里）
//  做法（同评论区，CommentGlass.xm）：
//    · 认面板：抖音的毛玻璃视图出现时（didMoveToWindow，0 / 0.1 / 0.35 秒各看一次，内容可能晚一点才装进来）——
//      整宽、贴着屏幕底边、往上两三层有一个带圆角并裁边的外框、外框旁边有那层“点空白处关闭”——是底部弹出面板；
//      面板里（控制器和视图）有类名带 LongPress 的才算长按面板（「sheet.allSheets」开着时其他底部弹出面板也算），
//      带 Comment 的不碰（评论区另有一套）
//    · 背景：抖音那层毛玻璃的效果去掉（记下原来的，关掉时还原；抖音再设也先替它记着、不让它画），
//      在它下面垫一块评论区同款玻璃（ALBMaterialMakeEffect：透明＋28% 黑，深色），面板内容原样在上面
//    · 圆角：外框和玻璃都用「sheet.radius」（只圆上面两个角，同评论区）
//    · 模块圆角（「sheet.moduleRadiusOn」，默认关）：面板里那些灰色半透明的块（宽 ≥70、高 ≥40、自己有半透明底色、
//      本来就有圆角、不是整宽背景）改成「sheet.moduleRadius」；关掉还原。面板在屏幕上时每 0.2 秒补一次（新出现的格子）
//      3.11.35：抖音长按面板的模块底其实是 CAShapeLayer 画的路径（视图自己圆角 0），按路径改：看哪几个角是圆的，
//      换成同样几个角、新半径的圆角路径（成组的上 / 下两格各圆两个角，中间的格子不动）；记下原路径，关掉还原
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#include "ALB5Options.h"
#import "ALBUnifiedMaterial.h"

// Tweak.x：插件已授权且总开关打开
FOUNDATION_EXPORT BOOL ALBDockHostPluginActive(void);

@interface DUXVisualEffectView : UIVisualEffectView
@end

static const NSInteger kALBSheetGlassTag = 26113401;

static BOOL ALBSheetSystemOK(void) {
    return NSProcessInfo.processInfo.operatingSystemVersion.majorVersion >= 26 && NSClassFromString(@"UIGlassEffect") != nil;
}
static BOOL ALBSheetEnabled(void) { return ALBSheetSystemOK() && ALB5Bool(@"sheet.glass", YES) && ALBDockHostPluginActive(); }
static BOOL ALBSheetAllSheets(void) { return ALB5Bool(@"sheet.allSheets", NO); }
static CGFloat ALBSheetRadius(void) { return ALB5Number(@"sheet.radius", 20, 8, 44); }
static BOOL ALBSheetModulesOn(void) { return ALB5Bool(@"sheet.moduleRadiusOn", NO); }
static CGFloat ALBSheetModuleRadius(void) { return ALB5Number(@"sheet.moduleRadius", 12, 4, 24); }

#pragma mark - 每块面板的状态

@interface ALBSheetState : NSObject
@property (nonatomic, weak) UIVisualEffectView *dux;       // 抖音那层毛玻璃（面板整块背景）
@property (nonatomic, weak) UIView *outer;                 // 外框（带圆角、裁边）
@property (nonatomic, strong) UIVisualEffectView *glass;   // 垫在下面的玻璃
@property (nonatomic, strong) UIVisualEffect *originalEffect;
@property (nonatomic, assign) BOOL active;                 // 现在是玻璃
@property (nonatomic, assign) BOOL matched;                // 认成长按面板（或「其他面板也用」）
@property (nonatomic, assign) BOOL checked;                // 已经认过（内容装好以后）
@property (nonatomic, assign) CGFloat frameRadius;         // 外框原来的圆角
@property (nonatomic, assign) CACornerMask frameCorners;
@property (nonatomic, strong) NSTimer *moduleTimer;
@property (nonatomic, assign) NSUInteger modulesChanged;
@end
@implementation ALBSheetState
@end

static char kALBSheetStateKey, kALBSheetModuleRadiusKey, kALBSheetShapeOriginalKey, kALBSheetShapeOursKey, kALBSheetShapeRadiusKey;
static NSHashTable<UIVisualEffectView *> *gALBSheetViews;
static NSUInteger gALBSheetSeen, gALBSheetApplied, gALBSheetModuleViews;
static NSMutableArray<NSString *> *gALBSheetLog;
static NSMutableOrderedSet<NSString *> *gALBSheetModuleClasses;
static NSUInteger gALBSheetShapeModules;

static ALBSheetState *ALBSheetStateOf(UIView *dux) { return objc_getAssociatedObject(dux, &kALBSheetStateKey); }

static void ALBSheetNote(NSString *line) {
    if (!gALBSheetLog) gALBSheetLog = [NSMutableArray array];
    [gALBSheetLog addObject:line];
    while (gALBSheetLog.count > 6) [gALBSheetLog removeObjectAtIndex:0];
}

#pragma mark - 认面板

// 外框：往上三层内第一个带圆角并裁边的；它的父视图里要有那层“点空白处关闭”
static UIView *ALBSheetFrameFor(UIView *dux) {
    UIView *cursor = dux.superview;
    for (NSInteger depth = 0; cursor && depth < 3; depth++, cursor = cursor.superview) {
        if (cursor.layer.cornerRadius > 0.5 && (cursor.clipsToBounds || cursor.layer.masksToBounds)) {
            for (UIView *sibling in cursor.superview.subviews) {
                if ([NSStringFromClass(sibling.class) isEqualToString:@"DUXAccessibilityDismissView"]) return cursor;
            }
            return nil;
        }
    }
    return nil;
}

// 整宽、贴着屏幕底边
static BOOL ALBSheetLooksLikeBottomSheet(UIView *dux) {
    UIWindow *window = dux.window;
    if (!window || dux.hidden) return NO;
    CGRect r = [dux convertRect:dux.bounds toView:window];
    CGFloat W = CGRectGetWidth(window.bounds), H = CGRectGetHeight(window.bounds);
    return CGRectGetWidth(r) >= W - 2 && CGRectGetMaxY(r) >= H - 2 && CGRectGetHeight(r) >= 120 && CGRectGetMinY(r) > 20;
}

// 面板里的控制器、视图类名（控制器链往上找，内容往下找，最多 400 个视图）
static void ALBSheetCollectNames(UIView *dux, NSMutableOrderedSet<NSString *> *controllers, NSMutableSet<NSString *> *views) {
    for (UIResponder *r = dux.nextResponder; r && ![r isKindOfClass:UIWindow.class]; r = r.nextResponder) {
        if ([r isKindOfClass:UIViewController.class]) [controllers addObject:NSStringFromClass(r.class)];
    }
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:dux];
    for (NSUInteger i = 0; i < queue.count && i < 400; i++) {
        UIView *v = queue[i];
        if (v.tag == kALBSheetGlassTag) continue;
        [views addObject:NSStringFromClass(v.class)];
        if ([v.nextResponder isKindOfClass:UIViewController.class]) [controllers addObject:NSStringFromClass(v.nextResponder.class)];
        [queue addObjectsFromArray:v.subviews];
    }
}

static BOOL ALBSheetNamesContain(id<NSFastEnumeration> names, NSString *needle) {
    for (NSString *n in names) if ([n rangeOfString:needle options:NSCaseInsensitiveSearch].location != NSNotFound) return YES;
    return NO;
}

#pragma mark - 模块圆角

static BOOL ALBSheetColorIsVeil(CGColorRef color) {
    if (!color) return NO;
    CGFloat a = CGColorGetAlpha(color);
    return a >= 0.02 && a <= 0.9;
}

// 3.11.35：抖音长按面板的模块底是 CAShapeLayer 画的圆角路径（背景探针：AWELongPressPanelSettingCell 里
// 一层 440 宽的视图带两个 CAShapeLayer，自己的圆角是 0）。从路径的四个角往里走，看走多远才进到路径里面，
// 判断哪几个角是圆的（圆角 r 时约 0.29r）——成组的格子只有上面或下面两个角圆，中间的格子不圆，照原样保留
static CGFloat ALBSheetCornerDepth(CGPathRef path, CGPoint corner, CGFloat dx, CGFloat dy) {
    for (CGFloat d = 0.5; d <= 16; d += 0.5)
        if (CGPathContainsPoint(path, NULL, CGPointMake(corner.x + d * dx, corner.y + d * dy), NO)) return d;
    return 16;
}

// 模块底路径换成新圆角：够大（≥70×30）、不是整宽、至少一个角是圆的；按原路径判断哪几个角圆，nil＝不是模块底。
// 一定要拿抖音的原路径来判断（我们换上的小圆角路径再判断会被当成不圆）
static UIBezierPath *ALBSheetModulePath(CGPathRef source, CGFloat maxW, CGFloat radius) {
    if (!source) return nil;
    CGRect box = CGPathGetPathBoundingBox(source);
    if (CGRectIsNull(box) || box.size.width < 70 || box.size.height < 30 || box.size.width > maxW) return nil;
    CGFloat minX = CGRectGetMinX(box), maxX = CGRectGetMaxX(box), minY = CGRectGetMinY(box), maxY = CGRectGetMaxY(box);
    CGFloat depth[4] = {
        ALBSheetCornerDepth(source, CGPointMake(minX, minY), 1, 1), ALBSheetCornerDepth(source, CGPointMake(maxX, minY), -1, 1),
        ALBSheetCornerDepth(source, CGPointMake(minX, maxY), 1, -1), ALBSheetCornerDepth(source, CGPointMake(maxX, maxY), -1, -1) };
    UIRectCorner masks[4] = { UIRectCornerTopLeft, UIRectCornerTopRight, UIRectCornerBottomLeft, UIRectCornerBottomRight };
    UIRectCorner rounded = 0;
    for (int i = 0; i < 4; i++) if (depth[i] >= 1.5) rounded |= masks[i];
    if (!rounded) return nil;
    CGFloat r = MIN(radius, MIN(box.size.width, box.size.height) / 2.0);
    return [UIBezierPath bezierPathWithRoundedRect:box byRoundingCorners:rounded cornerRadii:CGSizeMake(r, r)];
}

static void ALBSheetRememberShape(CAShapeLayer *shape, CGPathRef original, UIBezierPath *ours, CGFloat radius) {
    objc_setAssociatedObject(shape, &kALBSheetShapeOriginalKey, [UIBezierPath bezierPathWithCGPath:original], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(shape, &kALBSheetShapeOursKey, ours, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(shape, &kALBSheetShapeRadiusKey, @(radius), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

// 一层 CAShapeLayer（定时补 / 设置改了时）：是模块底就换成新圆角的路径；off＝还原
static BOOL ALBSheetApplyShape(CAShapeLayer *shape, CGFloat maxW, CGFloat radius, BOOL on) {
    CGPathRef current = shape.path;
    UIBezierPath *ours = objc_getAssociatedObject(shape, &kALBSheetShapeOursKey);
    UIBezierPath *original = objc_getAssociatedObject(shape, &kALBSheetShapeOriginalKey);
    // 比内容不比指针（CAShapeLayer 可能存的是拷贝）
    BOOL showingOurs = ours && current && CGPathEqualToPath(current, ours.CGPath);
    if (!on) {
        if (showingOurs && original) {
            [CATransaction begin];
            [CATransaction setDisableActions:YES];
            shape.path = original.CGPath;
            [CATransaction commit];
        }
        objc_setAssociatedObject(shape, &kALBSheetShapeOursKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(shape, &kALBSheetShapeOriginalKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(shape, &kALBSheetShapeRadiusKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return NO;
    }
    if (!current || shape.hidden || shape.opacity < 0.01) return NO;
    if (showingOurs && fabs([objc_getAssociatedObject(shape, &kALBSheetShapeRadiusKey) doubleValue] - radius) < 0.01) return YES;
    // 抖音换过路径（重新布局）：现在这条就是新的原样；只是圆角设置变了：从记着的原样重做
    CGPathRef source = showingOurs && original ? original.CGPath : current;
    UIBezierPath *path = ALBSheetModulePath(source, maxW, radius);
    if (!path) return NO;
    ALBSheetRememberShape(shape, source, path, radius);
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    shape.path = path.CGPath;
    [CATransaction commit];
    return YES;
}

// 3.11.36：用户：“模块圆角改了以后，离屏再回来就又得变一次”——格子重新出现时抖音重设路径，定时器下一拍才改，
// 中间那一下是原来的圆角。现在在抖音设路径的那一刻就换（CAShapeLayer setPath: 钩子，只在「改模块圆角」开着时做事；
// 只看挂在视图上、往上 7 层内有类名带 LongPressPanel 的那些，别处的形状图层一个判断就放过）
static BOOL gALBSheetShapeHook;
static CGFloat gALBSheetShapeRadius = 12;
static NSUInteger gALBSheetShapeHookHits;
static void ALBSheetSyncShapeHook(void) {
    gALBSheetShapeRadius = ALBSheetModuleRadius();
    gALBSheetShapeHook = ALBSheetEnabled() && ALBSheetModulesOn();
}
static BOOL ALBSheetShapeInPanel(CAShapeLayer *shape, CGFloat *maxW) {
    id host = shape.superlayer.delegate;
    if (![host isKindOfClass:UIView.class]) return NO;
    UIView *view = host;
    for (int i = 0; view && i < 7; i++, view = view.superview) {
        const char *name = object_getClassName(view);
        if (name && strstr(name, "LongPressPanel")) {
            UIView *root = ((UIView *)host).window;
            *maxW = (root ? CGRectGetWidth(root.bounds) : CGRectGetWidth(UIScreen.mainScreen.bounds)) - 8;
            return YES;
        }
    }
    return NO;
}

// 3.11.38：用户：3.11.36 以后“模块圆角离屏会失效，进来后又会再次变回设置的圆角”还在。setPath 钩子只认已经挂在面板里的
// 图层；格子重新出现时抖音很可能是先给新建的、还没挂上去的图层设路径，再挂进格子——设路径那一刻认不出是面板里的，
// 就漏掉了，等定时器（0.5 秒）才改，中间看到的是原来的圆角。
// 现在：认不出、而且图层还没挂上去（或所在视图还不在窗口里）、路径大小像模块底的，先记下；本次 CATransaction 提交前
// （布局都做完、画面还没送去显示）再看一次，那时已经挂进格子了就当场换，画面上不会出现原来的圆角。
// 提交前回调用 CATransaction +addCommitHandler:forPhase:（系统私有，WebKit 也这样用；没有或加不上时退回下一轮主线程）
@interface CATransaction (ALBSheetCommit)
+ (BOOL)addCommitHandler:(void (^)(void))block forPhase:(int)phase;
@end
static NSHashTable<CAShapeLayer *> *gALBSheetPendingShapes;
static BOOL gALBSheetPendingScheduled;
static NSUInteger gALBSheetPendingSeen, gALBSheetLateHits, gALBSheetCommitFallback;
static void ALBSheetFlushPendingShapes(void) {
    gALBSheetPendingScheduled = NO;
    NSArray<CAShapeLayer *> *layers = gALBSheetPendingShapes.allObjects;
    [gALBSheetPendingShapes removeAllObjects];
    if (!gALBSheetShapeHook) return;
    for (CAShapeLayer *shape in layers) {
        CGFloat maxW = 0;
        if (!ALBSheetShapeInPanel(shape, &maxW)) continue;
        CGPathRef before = CGPathRetain(shape.path);
        if (ALBSheetApplyShape(shape, maxW, gALBSheetShapeRadius, YES) && before && !CGPathEqualToPath(before, shape.path))
            gALBSheetLateHits++;
        CGPathRelease(before);
    }
}
static void ALBSheetQueuePendingShape(CAShapeLayer *shape) {
    if (!gALBSheetPendingShapes) gALBSheetPendingShapes = [NSHashTable weakObjectsHashTable];
    [gALBSheetPendingShapes addObject:shape];
    gALBSheetPendingSeen++;
    if (gALBSheetPendingScheduled) return;
    gALBSheetPendingScheduled = YES;
    BOOL added = NO;
    if ([CATransaction respondsToSelector:@selector(addCommitHandler:forPhase:)])
        added = [CATransaction addCommitHandler:^{ ALBSheetFlushPendingShapes(); } forPhase:1 /* 提交前 */];
    if (!added) {
        gALBSheetCommitFallback++;
        dispatch_async(dispatch_get_main_queue(), ^{ ALBSheetFlushPendingShapes(); });
    }
}
// 设路径时认不出来的：图层还没挂上，或者挂着的视图还不在窗口里（格子正在搭），大小又像模块底
static BOOL ALBSheetShapeMaybeLater(CAShapeLayer *shape, CGPathRef path) {
    if (shape.superlayer) {
        id host = shape.superlayer.delegate;
        if ([host isKindOfClass:UIView.class] && ((UIView *)host).window) return NO;   // 在窗口里、又不在面板里：不是
    }
    CGRect box = CGPathGetPathBoundingBox(path);
    CGFloat maxW = CGRectGetWidth(UIScreen.mainScreen.bounds) - 8;
    return !CGRectIsNull(box) && box.size.width >= 70 && box.size.height >= 30 && box.size.width <= maxW;
}

static void ALBSheetApplyModules(ALBSheetState *state) {
    UIVisualEffectView *dux = state.dux;
    if (!dux) return;
    BOOL on = state.active && ALBSheetModulesOn();
    CGFloat radius = ALBSheetModuleRadius();
    CGFloat maxW = CGRectGetWidth(dux.bounds) - 8;
    NSUInteger changed = 0, shapes = 0;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:dux.contentView.subviews];
    for (NSUInteger i = 0; i < queue.count && i < 600; i++) {
        UIView *v = queue[i];
        [queue addObjectsFromArray:v.subviews];
        // 3.11.35：这个视图自己画的模块底（不是视图的那些 CAShapeLayer）
        for (CALayer *sublayer in v.layer.sublayers) {
            if (![sublayer isKindOfClass:CAShapeLayer.class] || [sublayer.delegate isKindOfClass:UIView.class]) continue;
            if (ALBSheetApplyShape((CAShapeLayer *)sublayer, maxW, radius, on)) {
                changed++;
                shapes++;
                if (!gALBSheetModuleClasses) gALBSheetModuleClasses = [NSMutableOrderedSet orderedSet];
                CGRect box = CGPathGetPathBoundingBox(((CAShapeLayer *)sublayer).path);
                if (gALBSheetModuleClasses.count < 8) [gALBSheetModuleClasses addObject:[NSString stringWithFormat:@"%@ 的路径 %.0f×%.0f", NSStringFromClass(v.class), box.size.width, box.size.height]];
            }
        }
        NSNumber *saved = objc_getAssociatedObject(v, &kALBSheetModuleRadiusKey);
        if (!on) {
            if (saved) {
                v.layer.cornerRadius = saved.doubleValue;
                objc_setAssociatedObject(v, &kALBSheetModuleRadiusKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            continue;
        }
        if (v.hidden || v.alpha < 0.01 || [v isKindOfClass:UILabel.class] || [v isKindOfClass:UIImageView.class]) continue;
        CGSize s = v.bounds.size;
        if (s.width < 70 || s.height < 40 || s.width > maxW) continue;
        CGFloat current = v.layer.cornerRadius;
        if (!saved && current < 6) continue;
        BOOL veil = ALBSheetColorIsVeil(v.backgroundColor.CGColor) || ALBSheetColorIsVeil(v.layer.backgroundColor) ||
                    [v isKindOfClass:UIVisualEffectView.class];
        if (!saved && !veil) continue;
        if (!saved) objc_setAssociatedObject(v, &kALBSheetModuleRadiusKey, @(current), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (fabs(current - radius) > 0.01) {
            v.layer.cornerRadius = radius;
            if (@available(iOS 13.0, *)) v.layer.cornerCurve = kCACornerCurveContinuous;
        }
        changed++;
        if (!gALBSheetModuleClasses) gALBSheetModuleClasses = [NSMutableOrderedSet orderedSet];
        if (gALBSheetModuleClasses.count < 8) [gALBSheetModuleClasses addObject:[NSString stringWithFormat:@"%@ %.0f×%.0f", NSStringFromClass(v.class), s.width, s.height]];
    }
    state.modulesChanged = changed;
    if (changed > gALBSheetModuleViews) gALBSheetModuleViews = changed;
    if (shapes > gALBSheetShapeModules) gALBSheetShapeModules = shapes;
}

static void ALBSheetUpdateModuleTimer(ALBSheetState *state) {
    BOOL want = state.active && ALBSheetModulesOn() && state.dux.window;
    if (!want) { [state.moduleTimer invalidate]; state.moduleTimer = nil; return; }
    if (state.moduleTimer) return;
    __weak ALBSheetState *weakState = state;
    state.moduleTimer = [NSTimer scheduledTimerWithTimeInterval:0.5 repeats:YES block:^(NSTimer *timer) {
        ALBSheetState *s = weakState;
        if (!s || !s.dux.window || !s.active || !ALBSheetModulesOn()) { [timer invalidate]; if (s) s.moduleTimer = nil; return; }
        ALBSheetApplyModules(s);
    }];
}

#pragma mark - 换上 / 还原

static void ALBSheetLayoutGlass(ALBSheetState *state) {
    UIVisualEffectView *dux = state.dux, *glass = state.glass;
    if (!dux || !glass) return;
    CGFloat radius = ALBSheetRadius();
    if (glass.superview != dux.superview) [dux.superview insertSubview:glass belowSubview:dux];
    else if ([dux.superview.subviews indexOfObjectIdenticalTo:glass] > [dux.superview.subviews indexOfObjectIdenticalTo:dux])
        [dux.superview insertSubview:glass belowSubview:dux];
    if (!CGRectEqualToRect(glass.frame, dux.frame)) glass.frame = dux.frame;
    glass.layer.cornerRadius = radius;
    glass.layer.maskedCorners = kCALayerMinXMinYCorner | kCALayerMaxXMinYCorner;
    UIView *outer = state.outer;
    if (outer && fabs(outer.layer.cornerRadius - radius) > 0.01) {
        outer.layer.cornerRadius = radius;
        outer.layer.cornerCurve = kCACornerCurveContinuous;
    }
}

static void ALBSheetApply(ALBSheetState *state) {
    UIVisualEffectView *dux = state.dux;
    if (!dux || !dux.superview) return;
    if (!state.active) {
        state.originalEffect = dux.effect;
        UIView *outer = state.outer;
        state.frameRadius = outer.layer.cornerRadius;
        state.frameCorners = outer.layer.maskedCorners;
        state.active = YES;
        dux.effect = nil;          // 钩子看到 active：不记 nil，照样去掉
        gALBSheetApplied++;
    }
    if (!state.glass) {
        UIVisualEffectView *glass = [[UIVisualEffectView alloc] initWithEffect:nil];
        glass.tag = kALBSheetGlassTag;
        glass.userInteractionEnabled = NO;
        glass.backgroundColor = UIColor.clearColor;
        glass.clipsToBounds = YES;
        glass.layer.masksToBounds = YES;
        glass.layer.cornerCurve = kCACornerCurveContinuous;
        glass.layer.borderWidth = 0.65;
        glass.layer.borderColor = [UIColor colorWithWhite:1 alpha:0.20].CGColor;
        glass.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        ALBMaterialApplyEffectView(glass, NO);      // 深色，同评论区
        state.glass = glass;
    }
    ALBSheetLayoutGlass(state);
    if (!state.glass.effect) {
        // 同评论区：先装上、位置定了再给效果
        __weak ALBSheetState *weakState = state;
        dispatch_async(dispatch_get_main_queue(), ^{
            ALBSheetState *s = weakState;
            if (s.active && s.glass.superview && !s.glass.effect) s.glass.effect = ALBMaterialMakeEffect(NO);
        });
    }
    ALBSheetApplyModules(state);
    ALBSheetUpdateModuleTimer(state);
}

static void ALBSheetRestore(ALBSheetState *state) {
    if (!state.active) return;
    state.active = NO;
    ALBSheetApplyModules(state);      // active 已关：模块圆角还原
    [state.moduleTimer invalidate];
    state.moduleTimer = nil;
    [state.glass removeFromSuperview];
    state.glass = nil;
    UIView *outer = state.outer;
    if (outer) {
        outer.layer.cornerRadius = state.frameRadius;
        outer.layer.maskedCorners = state.frameCorners;
    }
    UIVisualEffectView *dux = state.dux;
    if (dux) dux.effect = state.originalEffect;
}

// 认一次（内容装好才认得出来）并按设置换上 / 还原
static void ALBSheetEvaluate(UIVisualEffectView *dux) {
    if (!dux.window || !ALBSheetSystemOK()) return;
    ALBSheetState *state = ALBSheetStateOf(dux);
    if (!state) {
        if (!ALBSheetLooksLikeBottomSheet(dux)) return;
        UIView *outer = ALBSheetFrameFor(dux);
        if (!outer) return;
        state = [ALBSheetState new];
        state.dux = dux;
        state.outer = outer;
        objc_setAssociatedObject(dux, &kALBSheetStateKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!gALBSheetViews) gALBSheetViews = [NSHashTable weakObjectsHashTable];
        [gALBSheetViews addObject:dux];
        gALBSheetSeen++;
    }
    NSMutableOrderedSet<NSString *> *controllers = [NSMutableOrderedSet orderedSet];
    NSMutableSet<NSString *> *views = [NSMutableSet set];
    ALBSheetCollectNames(dux, controllers, views);
    BOOL longPress = ALBSheetNamesContain(controllers, @"LongPress") || ALBSheetNamesContain(views, @"LongPress");
    BOOL comment = ALBSheetNamesContain(controllers, @"Comment");
    // 3.11.43：分享面板、评论的分享面板由 ALBMoreGlass.h 做（玻璃挂在面板自己的底里，跟着拖动走），这里不碰
    // （3.11.42 在这里给它们垫玻璃：玻璃挂在毛玻璃视图旁边，面板一往下拖，玻璃停在原地）
    BOOL other = ALBSheetNamesContain(controllers, @"SharePanel") && !longPress;
    BOOL matched = !comment && !other && (longPress || ALBSheetAllSheets());
    if (!state.checked || matched != state.matched) {
        NSArray *names = controllers.array;
        if (names.count > 5) names = [names subarrayWithRange:NSMakeRange(0, 5)];
        ALBSheetNote([NSString stringWithFormat:@"控制器[%@] 长按=%@%@ → %@", [names componentsJoinedByString:@" "],
                      longPress ? @"是" : @"否", comment ? @" 评论区" : @"", matched ? @"套用" : @"不碰"]);
    }
    state.checked = YES;
    state.matched = matched;
    if (matched && ALBSheetEnabled()) ALBSheetApply(state);
    else ALBSheetRestore(state);
}

static void ALBSheetSchedule(UIVisualEffectView *dux) {
    __weak UIVisualEffectView *weakDux = dux;
    for (NSNumber *delay in @[@0, @0.1, @0.35]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay.doubleValue * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            UIVisualEffectView *d = weakDux;
            if (d.window) ALBSheetEvaluate(d);
        });
    }
}

#pragma mark - 钩子

%group ALBSheetGroup
%hook DUXVisualEffectView
- (void)didMoveToWindow {
    %orig;
    UIVisualEffectView *me = (UIVisualEffectView *)self;
    if (me.window) {
        if (ALBSheetSystemOK() && (ALBSheetStateOf(me) || ALB5Bool(@"sheet.glass", YES))) ALBSheetSchedule(me);
    } else {
        ALBSheetState *state = ALBSheetStateOf(me);
        [state.moduleTimer invalidate];
        state.moduleTimer = nil;
    }
}
- (void)layoutSubviews {
    %orig;
    ALBSheetState *state = ALBSheetStateOf((UIView *)self);
    if (state.active) {
        ALBSheetLayoutGlass(state);
        if (ALBSheetModulesOn()) ALBSheetApplyModules(state);
    }
}
- (void)setEffect:(UIVisualEffect *)effect {
    ALBSheetState *state = ALBSheetStateOf((UIView *)self);
    if (state.active) {
        // 抖音想换的效果先替它记着（还原时用），现在不画
        if (effect) state.originalEffect = effect;
        %orig(nil);
        return;
    }
    %orig(effect);
}
%end
%end

%group ALBSheetShapeGroup
%hook CAShapeLayer
- (void)setPath:(CGPathRef)path {
    if (gALBSheetShapeHook && path && [NSThread isMainThread]) {
        CGFloat maxW = 0;
        if (ALBSheetShapeInPanel((CAShapeLayer *)self, &maxW)) {
            UIBezierPath *ours = objc_getAssociatedObject(self, &kALBSheetShapeOursKey);
            if (!(ours && CGPathEqualToPath(path, ours.CGPath))) {
                UIBezierPath *fixed = ALBSheetModulePath(path, maxW, gALBSheetShapeRadius);
                if (fixed) {
                    ALBSheetRememberShape((CAShapeLayer *)self, path, fixed, gALBSheetShapeRadius);
                    gALBSheetShapeHookHits++;
                    %orig(fixed.CGPath);
                    return;
                }
            }
        } else if (ALBSheetShapeMaybeLater((CAShapeLayer *)self, path)) {
            %orig(path);
            ALBSheetQueuePendingShape((CAShapeLayer *)self);   // 3.11.38：提交前再看一次
            return;
        }
    }
    %orig(path);
}
%end
%end

#pragma mark - 和本插件的接口

/* 视图在哪种面板里（ALBMoreGlass.h 用：多功能插件的长按面板里装着的抖音分享面板不碰）：0 不在认过的面板里，2 套了玻璃的长按面板 */
extern "C" NSInteger ALBSheetKindForView(UIView *view) {
    if (!view) return 0;
    for (UIVisualEffectView *dux in gALBSheetViews.allObjects) {
        ALBSheetState *s = ALBSheetStateOf(dux);
        if (!s.checked || !s.matched || !dux.window || !dux.superview) continue;
        if ([view isDescendantOfView:dux.superview]) return 2;
    }
    return 0;
}

// 诊断（ALB5Settings.h ALB5AdaptationText）
extern "C" NSString *ALBSheetGlassReport(void) {
    NSUInteger live = 0, active = 0, modules = 0;
    for (UIVisualEffectView *dux in gALBSheetViews.allObjects) {
        ALBSheetState *s = ALBSheetStateOf(dux);
        if (!s || !dux.window) continue;
        live++;
        if (s.active) { active++; modules += s.modulesChanged; }
    }
    return [NSString stringWithFormat:@"%@ 圆角 %.0f 模块圆角=%@ %.0f 其他面板=%@；见过底部弹出面板 %lu 次、套用 %lu 次；现在屏幕上 %lu 块、玻璃 %lu 块、改了模块 %lu 个（最多一次 %lu 个，其中路径画的 %lu 个：%@；设路径时当场换 %lu 次；设路径时还没挂上 %lu 次→提交前补上 %lu 次，提交前回调加不上 %lu 次）；最近：%@",
        !ALBSheetSystemOK() ? @"系统低于 iOS 26（不做）" : (ALB5Bool(@"sheet.glass", YES) ? @"开" : @"关"),
        ALBSheetRadius(), ALBSheetModulesOn() ? @"开" : @"关", ALBSheetModuleRadius(), ALBSheetAllSheets() ? @"开" : @"关",
        (unsigned long)gALBSheetSeen, (unsigned long)gALBSheetApplied, (unsigned long)live, (unsigned long)active, (unsigned long)modules,
        (unsigned long)gALBSheetModuleViews, (unsigned long)gALBSheetShapeModules,
        gALBSheetModuleClasses.count ? [gALBSheetModuleClasses.array componentsJoinedByString:@"、"] : @"-",
        (unsigned long)gALBSheetShapeHookHits,
        (unsigned long)gALBSheetPendingSeen, (unsigned long)gALBSheetLateHits, (unsigned long)gALBSheetCommitFallback,
        gALBSheetLog.count ? [gALBSheetLog componentsJoinedByString:@"；"] : @"还没见过（打开一次长按面板再导出）"];
}

// 设置改了：屏幕上的面板按新设置重来（关掉＝还原）
static void ALBSheetOptionsChanged(void) {
    ALBSheetSyncShapeHook();
    for (UIVisualEffectView *dux in gALBSheetViews.allObjects) {
        ALBSheetState *s = ALBSheetStateOf(dux);
        if (!s || !dux.window) continue;
        if (s.matched && ALBSheetEnabled() && s.active) {
            s.glass.effect = nil;          // 玻璃按当前设置重做（评论区的「顶栏用透明玻璃」也管这块）
            ALBSheetApply(s);
        } else {
            ALBSheetEvaluate(dux);
        }
    }
}

%ctor {
    @autoreleasepool {
        if (objc_getClass("DUXVisualEffectView")) {
            %init(ALBSheetGroup);
        }
        if (ALBSheetSystemOK()) {
            %init(ALBSheetShapeGroup);
            dispatch_async(dispatch_get_main_queue(), ^{ ALBSheetSyncShapeHook(); });
        }
        [[NSNotificationCenter defaultCenter] addObserverForName:@"ALB5.OptionsChanged" object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) {
            ALBSheetOptionsChanged();
        }];
    }
}
