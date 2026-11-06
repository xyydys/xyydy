#import "ALBDockBridge.h"
#import <objc/message.h>

static __weak UIView *gBridgeBar;
static BOOL gBridgePending;

// 依赖的抖音私有符号集中在这里，抖音改名时只改这里
static NSString *const kALBDockTabClickSel  = @"tabBarButtonDidTouchUpInside:gestureRecognizer:";
static NSString *const kALBDockPlusClickSel = @"plusTabBarButtonDidClick:";
static NSString *const kALBDockBadgeContainerClass = @"AWENormalModeTabBarBadgeContainerView";
static NSString *const kALBDockBadgeClass = @"DUXBadge";
static const long long kALBDockPlusType = 2;
// 按钮 type → 抖音 tabID（homepage_home 等）的固定映射表，抖音启动时注册
static NSString *const kALBDockTabMapClass  = @"AWENormalModeTabBarItemConfigsMap";
static NSString *const kALBDockTabMapShared = @"sharedMap";
static NSString *const kALBDockTabMapLookup = @"getTabIdWithTabBarItemType:";

static id ALBDockKV(id obj, NSString *key) {
    if (!obj || key.length == 0) { return nil; }
    @try { return [obj valueForKey:key]; } @catch (__unused NSException *e) { return nil; }
}

void ALBDockBridgeSetNativeBar(UIView *bar) { if (bar) { gBridgeBar = bar; } }
UIView *ALBDockBridgeNativeBar(void) { return gBridgeBar; }

static NSArray *ALBDockButtons(void) {
    id buttons = ALBDockKV(gBridgeBar, @"tabBarButtons");
    return [buttons isKindOfClass:[NSArray class]] ? buttons : nil;
}

static UITabBarController *ALBDockController(void) {
    for (UIResponder *r = gBridgeBar.nextResponder; r; r = r.nextResponder) {
        if ([r isKindOfClass:[UITabBarController class]]) { return (UITabBarController *)r; }
    }
    return nil;
}

// 2.5.1：tabID 取不到不缓存（映射表可能还没就绪）
static NSString *ALBDockButtonTabID(id button) {
    static NSMutableDictionary<NSNumber *, NSString *> *cache;
    if (!cache) { cache = [NSMutableDictionary dictionary]; }
    id type = ALBDockKV(button, @"type");
    if (![type isKindOfClass:[NSNumber class]]) { return nil; }
    NSString *tabID = cache[type];
    if (tabID) { return tabID; }
    Class mapClass = NSClassFromString(kALBDockTabMapClass);
    SEL shared = NSSelectorFromString(kALBDockTabMapShared), lookup = NSSelectorFromString(kALBDockTabMapLookup);
    if (![mapClass respondsToSelector:shared]) { return nil; }
    id map = ((id (*)(id, SEL))objc_msgSend)(mapClass, shared);
    if (![map respondsToSelector:lookup]) { return nil; }
    id value = ((id (*)(id, SEL, long long))objc_msgSend)(map, lookup, [type longLongValue]);
    if (![value isKindOfClass:[NSString class]] || [value length] == 0) { return nil; }
    cache[type] = value;
    return value;
}

static NSString *ALBDockTitleForTabID(NSString *tabID) {
    if (!tabID) { return nil; }
    static NSDictionary<NSString *, NSString *> *titles;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        titles = @{ @"homepage_home": @"首页", @"homepage_familiar": @"朋友", @"homepage_mall": @"商城",
                    @"homepage_notification": @"消息", @"homepage_profile": @"我", @"homepage_publish": @"拍摄" };
    });
    return titles[tabID];
}

// 标题优先按 tabID 认（首页键会被抖音换成刷新动画 / 运营图、别的插件也可能改字，按显示文字认不可靠）
static NSString *ALBDockButtonTitle(id button) {
    NSString *byID = ALBDockTitleForTabID(ALBDockButtonTabID(button));
    if (byID) { return byID; }
    id inner = ALBDockKV(button, @"innerView");
    NSString *t = ALBDockKV(ALBDockKV(inner, @"label"), @"text");
    if ([t isKindOfClass:[NSString class]] && t.length) { return t; }
    t = ALBDockKV(inner, @"currentTitleText");
    if ([t isKindOfClass:[NSString class]] && t.length) { return t; }
    t = [button isKindOfClass:[UIView class]] ? [(UIView *)button accessibilityLabel] : nil;
    return t.length ? t : nil;
}

static BOOL ALBDockIsPlus(id button) { return [ALBDockKV(button, @"type") longLongValue] == kALBDockPlusType; }

// 按标题找；找不到按位置兜底（非拍摄按钮依次为 首页 朋友 消息 我）
static id ALBDockButtonForTitle(NSString *title) {
    NSArray *buttons = ALBDockButtons();
    NSMutableArray *pages = [NSMutableArray array];
    for (id b in buttons) {
        if (![b isKindOfClass:[UIView class]] || !((UIView *)b).superview || ALBDockIsPlus(b)) { continue; }
        [pages addObject:b];
        NSString *t = ALBDockButtonTitle(b);
        if (t && [t isEqualToString:title]) { return b; }
    }
    NSUInteger idx = [@[@"首页", @"朋友", @"消息", @"我"] indexOfObject:title];
    return (idx != NSNotFound && idx < pages.count) ? pages[idx] : nil;
}

BOOL ALBDockBridgeTapTitle(NSString *title) {
    UITabBarController *c = ALBDockController();
    id button = ALBDockButtonForTitle(title);
    SEL sel = NSSelectorFromString(kALBDockTabClickSel);
    if (!c || !button || ![c respondsToSelector:sel]) { return NO; }
    ((void (*)(id, SEL, id, id))objc_msgSend)(c, sel, button, ALBDockKV(button, @"singleTapGes"));
    ALBDockBridgeScheduleChanged();
    return YES;
}

BOOL ALBDockBridgeTapPlus(void) {
    UITabBarController *c = ALBDockController();
    id plus = nil;
    for (id b in ALBDockButtons()) { if (ALBDockIsPlus(b)) { plus = b; break; } }
    SEL sel = NSSelectorFromString(kALBDockPlusClickSel);
    if (!c || !plus || ![c respondsToSelector:sel]) { return NO; }
    ((void (*)(id, SEL, id))objc_msgSend)(c, sel, plus);
    return YES;
}

NSString *ALBDockBridgeSelectedTitle(void) {
    UITabBarController *c = ALBDockController();
    if (!c) { return nil; }
    NSInteger sel = (NSInteger)c.selectedIndex;
    NSArray *buttons = ALBDockButtons();
    NSMutableArray *pages = [NSMutableArray array];
    for (id b in buttons) {
        if (![b isKindOfClass:[UIView class]] || ALBDockIsPlus(b)) { continue; }
        [pages addObject:b];
        id vi = ALBDockKV(b, @"validIndex");
        if (vi && [vi integerValue] == sel) {
            NSString *t = ALBDockButtonTitle(b);
            if (t) { return t; }
        }
    }
    return nil;
}

// 读角标只扫子视图，不碰 badge getter（可能懒建对象）
NSString *ALBDockBridgeBadgeForTitle(NSString *title) {
    id button = ALBDockButtonForTitle(title);
    if (![button isKindOfClass:[UIView class]]) { return nil; }
    UIView *badge = nil;
    for (UIView *container in ((UIView *)button).subviews) {
        if (![NSStringFromClass(container.class) isEqualToString:kALBDockBadgeContainerClass] ||
            container.isHidden || container.alpha < 0.01) { continue; }
        for (UIView *cand in container.subviews) {
            if ([NSStringFromClass(cand.class) isEqualToString:kALBDockBadgeClass] &&
                !cand.isHidden && cand.alpha >= 0.01 && !CGRectIsEmpty(cand.bounds)) { badge = cand; break; }
        }
        if (badge) { break; }
    }
    if (!badge) { return nil; }
    NSString *text = ALBDockKV(badge, @"badgeText");
    if ([text isKindOfClass:[NSString class]] && text.length) { return text; }
    unsigned long long n = [ALBDockKV(badge, @"badgeNumber") unsignedLongLongValue];
    return n > 0 ? @(n).stringValue : @"";
}

void ALBDockBridgeScheduleChanged(void) {
    if (gBridgePending) { return; }
    gBridgePending = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        gBridgePending = NO;
        [[NSNotificationCenter defaultCenter] postNotificationName:ALBDockBridgeChangedNotification object:nil];
    });
}
