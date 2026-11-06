//
//  ALBDockHooks.x —— 三段式收缩底栏接入抖音（3.11）
//
//  只在设置里选了「三段式收缩底栏」并重启抖音后才生效（%init(ALBDockGroup) 只在那时调用）：
//  · 本插件自己的液态底栏不再安装（Tweak.x AWEBottomBarDisplayEnabled 在这一模式下为 NO，
//    走它原有的「释放底栏」路径），抖音原生底栏 AWENormalModeTabBar 留在层级里（判断页面、角标、
//    转发点击都靠它），但完全透明、不吃点击
//  · 主窗口最上面放一层穿透的覆盖层，里面是 ALBDockView（三段式收缩底栏）
//  · 全屏手势观察器：只观察不拦截。左半屏上滑 → 单独键到左；右半屏上滑 → 单独键到右
//  · 底栏 5 个按钮（首页 消息 拍摄 我 + 朋友球）走抖音自己的点击回调，真实切页；四项用系统 UITabBar 的透镜
//  原工程的中间动效胶囊、流星雨、悬浮设置按钮、探测、右侧玻璃和头像测试菜单都没有带过来
//  （右侧玻璃的画法并进了本插件自己的右侧按钮，见 Tweak.x AWEConfigureAdaptiveSystemGlassIconButton）。
//

#import <UIKit/UIKit.h>
#import <UIKit/UIGestureRecognizerSubclass.h>
#import "ALBDock.h"
#import "ALBDockAvatar.h"
#import "ALBDockCommon.h"
#import "ALBDockBridge.h"

// Tweak.x：设置里选了三段式底栏（启动时读一次，换样式要重启抖音）
extern BOOL ALBDockModeActive(void);

@interface AWENormalModeTabBar : UIView
@end

@interface AWENormalModeTabBarGeneralButton : UIView
@end

#pragma mark - 只观察、永不识别的手势

@interface ALBDockTouchWatcher : UIGestureRecognizer <UIGestureRecognizerDelegate>
@property (nonatomic, assign) CGPoint startPoint;
@property (nonatomic, assign) NSTimeInterval startTime;
@property (nonatomic, assign) BOOL tracking;
@end

#pragma mark - 覆盖层

@interface ALBDockOverlayView : UIView
@end

@implementation ALBDockOverlayView
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    return hit == self ? nil : hit;
}
@end

static ALBDockOverlayView *gALBDockOverlay;
static ALBDockView *gALBDock;
static ALBDockTouchWatcher *gALBDockWatcher;
static NSUInteger gALBDockTick;
static __weak UIView *gALBDockNativeBar;
static NSTimer *gALBDockSyncTimer;
// 抖音自己最后要的原生底栏透明度（插件未启用时原样还给它），和这边是否把它变成了透明、不吃点击
static CGFloat gALBDockNativeAlpha = 1.0;
static BOOL gALBDockWritingAlpha;
static BOOL gALBDockBlankedNative;

// 3.11：插件已授权且总开关打开 → 原生底栏透明、不吃点击；否则（总开关关、授权失效）原样还给抖音，
// 不会出现「三段式底栏隐藏了、原生底栏也是透明的」两边都没有底栏的情况
static void ALBDockApplyNativeBlank(UIView *bar) {
    if (!bar) { return; }
    BOOL active = ALBDockHostPluginActive();
    CGFloat want = active ? 0.0 : gALBDockNativeAlpha;
    if (fabs(bar.alpha - want) > 0.001) {
        gALBDockWritingAlpha = YES;
        bar.alpha = want;
        gALBDockWritingAlpha = NO;
    }
    if (active) {
        bar.userInteractionEnabled = NO;
        gALBDockBlankedNative = YES;
    } else if (gALBDockBlankedNative) {
        bar.userInteractionEnabled = YES;
        gALBDockBlankedNative = NO;
    }
}

@implementation ALBDockTouchWatcher

- (instancetype)init {
    if ((self = [super initWithTarget:nil action:NULL])) {
        self.cancelsTouchesInView = NO;
        self.delaysTouchesBegan = NO;
        self.delaysTouchesEnded = NO;
        self.delegate = self;
    }
    return self;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesBegan:touches withEvent:event];
    UITouch *t = touches.anyObject;
    if (!t || event.allTouches.count > 1) { self.tracking = NO; return; }
    self.startPoint = [t locationInView:self.view];
    self.startTime = t.timestamp;
    // 落在底栏上就不算屏幕上滑
    self.tracking = !(gALBDockOverlay && [t.view isDescendantOfView:gALBDockOverlay]);
    [gALBDock noteInteraction];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesEnded:touches withEvent:event];
    UITouch *t = touches.anyObject;
    if (self.tracking && t) {
        CGPoint p = [t locationInView:self.view];
        CGFloat dx = p.x - self.startPoint.x, dy = p.y - self.startPoint.y;
        NSTimeInterval dt = t.timestamp - self.startTime;
        BOOL swipeUp = dy < -70.0 && fabs(dy) > fabs(dx) * 1.3 && dt < 1.2;
        if (swipeUp) {
            BOOL left = self.startPoint.x < CGRectGetWidth(self.view.bounds) / 2.0;
            [gALBDock swipeUpFromLeft:left];
        }
    }
    self.tracking = NO;
    self.state = UIGestureRecognizerStateFailed;
}

- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesCancelled:touches withEvent:event];
    self.tracking = NO;
    self.state = UIGestureRecognizerStateFailed;
}

- (void)reset {
    [super reset];
    self.tracking = NO;
}

- (BOOL)canPreventGestureRecognizer:(UIGestureRecognizer *)g { return NO; }
- (BOOL)canBePreventedByGestureRecognizer:(UIGestureRecognizer *)g { return NO; }
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)a shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)b { return YES; }

@end

#pragma mark - 挂载

static UIWindow *ALBDockMainWindow(void) {
    UIApplication *app = [UIApplication sharedApplication];
    id<UIApplicationDelegate> del = app.delegate;
    if ([del respondsToSelector:@selector(window)]) {
        UIWindow *w = [del window];
        if (w && !w.hidden) { return w; }
    }
    for (UIScene *scene in app.connectedScenes) {
        if (![scene isKindOfClass:[UIWindowScene class]]) { continue; }
        for (UIWindow *w in ((UIWindowScene *)scene).windows) {
            if (w.isKeyWindow) { return w; }
        }
    }
    return nil;
}

static BOOL ALBDockViewVisible(UIView *v) {
    if (!v || !v.window) { return NO; }
    for (UIView *p = v; p; p = p.superview) {
        if (p.hidden) { return NO; }
        if (p != v && p.alpha < 0.01) { return NO; }
    }
    CGRect r = [v convertRect:v.bounds toView:nil];
    return CGRectIntersectsRect(CGRectInset(r, 2, 2), v.window.bounds);
}

// 原生底栏不在屏幕上（进了详情页 / 评论 / 直播等）时，三段式底栏一起隐藏；插件总开关关掉、清屏时也隐藏
static void ALBDockSyncVisibility(void) {
    if (!gALBDock) { return; }
    UIView *bar = gALBDockNativeBar;
    BOOL barVisible = bar ? ALBDockViewVisible(bar) : YES;
    ALBDockApplyNativeBlank(bar);
    BOOL show = barVisible && ALBDockHostAllowsDock(bar);
    // 兜底再读一次当前页面和系统深浅色（主路径是底栏同步和场景 trait 监听），变了才会通知
    if (barVisible) { ALBDockMaterialUpdate(gALBDockOverlay.window, ALBDockBridgeSelectedTitle()); }
    else { ALBDockMaterialUpdate(gALBDockOverlay.window, nil); }
    // 3.11.44：系统胶囊的玻璃被 UIKit 换回原厂的（透明时右边两块就比它深）：再装上
    if (barVisible && !gALBDock.transitioning) { [gALBDock verifyNativePlatterGlass]; }
    // 抓头像要遍历整棵视图树：只在没有头像、底栏没在动画时做，拿到一次就不再扫（避免周期性卡顿）
    if (++gALBDockTick % 8 == 0 && !gALBDock.transitioning && ![ALBDockAvatar image]) {
        [ALBDockAvatar scanWindow:gALBDockOverlay.window nativeBarVisible:(bar != nil && barVisible)];
    }
    CGFloat target = show ? 1.0 : 0.0;
    if (fabs(gALBDock.alpha - target) > 0.01) {
        [UIView animateWithDuration:0.2 animations:^{ gALBDock.alpha = target; }];
    }
}

static void ALBDockAttach(void) {
    UIWindow *w = ALBDockMainWindow();
    if (!w) { return; }
    if (!gALBDockOverlay) {
        gALBDockOverlay = [[ALBDockOverlayView alloc] initWithFrame:w.bounds];
        gALBDockOverlay.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        gALBDockOverlay.backgroundColor = [UIColor clearColor];
        gALBDockOverlay.accessibilityIdentifier = @"AwemeLiquidBar.Dock.Overlay";

        gALBDock = [[ALBDockView alloc] initWithFrame:gALBDockOverlay.bounds];
        gALBDock.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [gALBDockOverlay addSubview:gALBDock];

        [ALBDockAvatar refreshFromAccount];

        gALBDockWatcher = [[ALBDockTouchWatcher alloc] init];

        gALBDockSyncTimer = [NSTimer timerWithTimeInterval:0.35 repeats:YES block:^(NSTimer *t) { ALBDockSyncVisibility(); }];
        [[NSRunLoop mainRunLoop] addTimer:gALBDockSyncTimer forMode:NSRunLoopCommonModes];
    }
    if (gALBDockOverlay.superview != w) {
        [gALBDockOverlay removeFromSuperview];
        gALBDockOverlay.frame = w.bounds;
        [w addSubview:gALBDockOverlay];
    }
    if (gALBDockWatcher.view != w) {
        [gALBDockWatcher.view removeGestureRecognizer:gALBDockWatcher];
        [w addGestureRecognizer:gALBDockWatcher];
    }
    [w bringSubviewToFront:gALBDockOverlay];
}

// 3.11.1：Tweak.x 的背后亮度取样用（只读）
CGRect ALBDockBackdropRect(void) { return gALBDock ? [gALBDock backdropRectInWindow] : CGRectNull; }
BOOL ALBDockBusy(void) { return gALBDock.transitioning; }
// 3.11.2：三段式底栏此刻在屏幕上（Tweak.x 的“内容延伸到底栏后面”那些处理据此在这个模式下也运行）
BOOL ALBDockShowing(void) { return gALBDock.window != nil && !gALBDock.hidden && gALBDock.alpha > 0.01; }

// 3.11：本插件的诊断（ALB5Settings.h）
NSString *ALBDockDiagnostics(void) {
    if (!ALBDockModeActive()) { return @"三段式底栏：未启用"; }
    if (!gALBDock) { return @"三段式底栏：已选，还没挂上主窗口"; }
    return [NSString stringWithFormat:@"三段式底栏：%@ 透明度%.2f\n%@", gALBDock.window ? @"在主窗口" : @"不在窗口",
            gALBDock.alpha, [gALBDock debugReport]];
}

#pragma mark - Hooks

%group ALBDockGroup

%hook UIWindow

- (void)didAddSubview:(UIView *)subview {
    %orig;
    if (gALBDockOverlay && subview != gALBDockOverlay && gALBDockOverlay.superview == self) {
        [self bringSubviewToFront:gALBDockOverlay];
        // 弹出页面（半屏页、相册、操作表）刚加进窗口：马上判断一次要不要让开，不等 0.35 秒的定时器
        dispatch_async(dispatch_get_main_queue(), ^{ ALBDockSyncVisibility(); });
    }
}

- (void)becomeKeyWindow {
    %orig;
    dispatch_async(dispatch_get_main_queue(), ^{ ALBDockAttach(); });
}

%end

// 抖音原生底栏：保留在层级里（判断页面用），但完全透明、不吃点击
%hook AWENormalModeTabBar

- (void)didMoveToWindow {
    %orig;
    gALBDockNativeBar = (UIView *)self;
    ALBDockBridgeSetNativeBar((UIView *)self);
    ALBDockApplyNativeBlank((UIView *)self);
    ALBDockBridgeScheduleChanged();
}

- (void)setAlpha:(CGFloat)alpha {
    if (!gALBDockWritingAlpha) { gALBDockNativeAlpha = alpha; }
    %orig(ALBDockHostPluginActive() ? 0.0 : alpha);
}

- (void)layoutSubviews {
    %orig;
    gALBDockNativeBar = (UIView *)self;
    ALBDockBridgeSetNativeBar((UIView *)self);
    ALBDockApplyNativeBlank((UIView *)self);
    ALBDockBridgeScheduleChanged();   // 选中页可能变了：合并到下一轮再同步给三段式底栏
}

%end

// 角标变化不一定伴随底栏布局（收到推送时就不会），在抖音写角标的入口上补一次同步
%hook AWENormalModeTabBarGeneralButton

- (id)p_showBadgeWithStyle:(unsigned long long)style count:(long long)count text:(id)text config:(id)config {
    id r = %orig;
    ALBDockBridgeScheduleChanged();
    return r;
}

- (void)hideBadge {
    %orig;
    ALBDockBridgeScheduleChanged();
}

%end

%end

%ctor {
    if (!ALBDockModeActive()) { return; }
    %init(ALBDockGroup);
    // 本插件读到 / 换了本人头像（设置页个人卡片那张）：球上没有更高优先级的头像时跟着换
    [[NSNotificationCenter defaultCenter] addObserverForName:@"AwemeLiquidBar.OwnProfileChanged"
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification *n) {
        [[NSNotificationCenter defaultCenter] postNotificationName:ALBDockAvatarChangedNotification object:nil];
    }];
    [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationDidBecomeActiveNotification
                                                      object:nil
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification *n) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.4 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            ALBDockAttach();
            [ALBDockAvatar refreshFromAccount];
        });
    }];
}
