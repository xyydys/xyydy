//
//  ALBDockCommon.h —— 三段式收缩底栏（3.11 并入）：公共部分
//
//  液态玻璃、偏好、颜色。偏好放进本插件的命名空间（ALB5Key），设置页就是插件自己的
//  「三段式收缩底栏」那一节（ALB5Settings.h）；改动由设置页统一发 ALB5.OptionsChanged，
//  底栏比对哪几项变了再按原来的逻辑处理（ALBDock.m prefsChanged:）。
//

#pragma once
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include "ALB5Options.h"
#import "ALB25Material.h"

static inline UIColor *ALBDockHex(uint32_t hex, CGFloat a) {
    return [UIColor colorWithRed:((hex >> 16) & 0xFF) / 255.0
                           green:((hex >> 8) & 0xFF) / 255.0
                            blue:(hex & 0xFF) / 255.0
                           alpha:a];
}

#pragma mark - 偏好（键在本插件的命名空间里，默认值同原工程）

#define ALBDockPrefsChangedNotification @"ALBDockPrefsChanged"

static NSString * const kALBDockPrefIdle      = @"dock.idle";          // 静置收缩秒数 2 ~ 30
static NSString * const kALBDockPrefClear     = @"dock.darkTint";      // 3.11.51：三段式底栏只剩“深色色调”（视频页 Clear＋黑色染色 0–0.6）
static NSString * const kALBDockPrefSpeed     = @"dock.speed";         // 底栏动画速度倍率，1.0 = 录屏实测
static NSString * const kALBDockPrefRedDot    = @"dock.redDot";        // 收缩头像右上角红点
// 底栏几何（单位 pt）。高度低于系统胶囊（62）时整条底栏等比缩放：系统底栏、图标、标题、透镜一起缩
static NSString * const kALBDockPrefBarHeight = @"dock.barH";          // 底栏高度 40 ~ 62
static NSString * const kALBDockPrefMidWidth  = @"dock.midW";          // 两个图标的胶囊宽度 44 ~ 110
static NSString * const kALBDockPrefGapA      = @"dock.gapA";          // 四项 ↔ 胶囊 间距 0 ~ 30
static NSString * const kALBDockPrefGapB      = @"dock.gapB";          // 胶囊 ↔ 朋友球 间距 0 ~ 30
static NSString * const kALBDockPrefFuse      = @"dock.fuse";          // 系统底栏放进融合容器（实验）
// 3.11.1：透明的深 / 浅两种染色（和右侧、双击面板、顶栏共用，ALB25Material.m）
static NSString * const kALBDockPrefDarkTint  = @"glass.darkTint";
static NSString * const kALBDockPrefLightTint = @"glass.lightTint";
// 3.11.31：和右侧、双击面板一样的玻璃（登记在底栏自己的边上）
static NSString * const kALBDockPrefFollowEdge = @"dock.followEdge";
// 3.11.33：视频页按背后亮度切深浅（同液态底栏：背后亮＝浅色玻璃＋黑图标）、变浅阈值、选中颜色用液态底栏的强调色
static NSString * const kALBDockPrefAdaptive  = @"dock.adaptiveInk";
static NSString * const kALBDockPrefLightAt   = @"dock.lightAt";
static NSString * const kALBDockPrefAccent    = @"dock.accent";
// 3.11.33：浅色时透明玻璃的白色染色（0＝不加白，只换黑图标；用户 3.11.1 时自己调成过 0）
static NSString * const kALBDockPrefLightWhite = @"dock.lightTint";
// 3.11.45：项目（0 首页 1 消息 2 拍摄 3 我 4 朋友）的顺序、隐藏、单独键是哪一项；中间胶囊可关；↑ 键做什么；单独键显示头像、头像内边距
static NSString * const kALBDockPrefOrder        = @"dock.order";         // 五项的顺序（单独键在右时从左到右）
static NSString * const kALBDockPrefHidden       = @"dock.hidden";        // 不显示的项
static NSString * const kALBDockPrefSingle       = @"dock.singleType";    // 单独键是哪一项（默认朋友）
static NSString * const kALBDockPrefMid          = @"dock.mid";           // 显示中间 ✦|↑ 胶囊
static NSString * const kALBDockPrefArrow        = @"dock.arrow";         // ↑：0 展开菜单 1 消息通知 2 特别关注 3 我的订单 4 观看历史
static NSString * const kALBDockPrefSingleAvatar = @"dock.singleAvatar";  // 单独键显示自己的头像（功能不变）
static NSString * const kALBDockPrefAvatarInset  = @"dock.avatarInset";   // 头像内边距（球和单独键共用，4＝原来的）
static NSString * const kALBDockPrefTabWidth     = @"dock.tabWidth";      // 3.11.46：单个项目的宽（0＝自动）
static NSString * const kALBDockPrefExpandReverse = @"dock.expandReverse"; // 3.11.47：展开＝收缩倒放（关＝3.11.46 的展开曲线）

static inline NSDictionary<NSString *, id> *ALBDockPrefDefaults(void) {
    static NSDictionary *defaults;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        defaults = @{
            kALBDockPrefIdle: @5.0,
            kALBDockPrefClear: @0.28,
            kALBDockPrefSpeed: @1.0,
            kALBDockPrefRedDot: @NO,
            kALBDockPrefBarHeight: @62.0,
            kALBDockPrefMidWidth: @74.0,
            kALBDockPrefGapA: @8.0,
            kALBDockPrefGapB: @12.0,
            kALBDockPrefDarkTint: @0.28,
            kALBDockPrefLightTint: @0.12,
            kALBDockPrefAccent: @YES,
            kALBDockPrefOrder: @[@0, @2, @1, @3, @4],
            kALBDockPrefHidden: @[],
            kALBDockPrefSingle: @4,
            kALBDockPrefMid: @YES,
            kALBDockPrefArrow: @0,
            kALBDockPrefSingleAvatar: @NO,
            kALBDockPrefAvatarInset: @4.0,
            kALBDockPrefTabWidth: @0.0,
        };
    });
    return defaults;
}
static inline id ALBDockPrefValue(NSString *k) {
    id value = [NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(k)];
    return value ?: ALBDockPrefDefaults()[k];
}
static inline BOOL ALBDockPrefBool(NSString *k) { return [ALBDockPrefValue(k) boolValue]; }
static inline double ALBDockPrefDouble(NSString *k) {
    double v = [ALBDockPrefValue(k) doubleValue];
    return isfinite(v) ? v : [ALBDockPrefDefaults()[k] doubleValue];
}

#pragma mark - 3.11.45 项目：顺序、隐藏、单独键

enum { ALBDockTypeHome = 0, ALBDockTypeInbox = 1, ALBDockTypePlus = 2, ALBDockTypeMe = 3, ALBDockTypeFriends = 4, ALBDockTypeCount = 5 };

// 五项都在、不重复的顺序（存的值缺了的补在后面）
static inline NSArray<NSNumber *> *ALBDockTypeOrder(void) {
    id saved = ALBDockPrefValue(kALBDockPrefOrder);
    NSMutableArray<NSNumber *> *order = [NSMutableArray arrayWithCapacity:ALBDockTypeCount];
    for (id v in [saved isKindOfClass:[NSArray class]] ? saved : @[]) {
        if (![v isKindOfClass:[NSNumber class]]) { continue; }
        NSInteger t = [v integerValue];
        if (t >= 0 && t < ALBDockTypeCount && ![order containsObject:@(t)]) { [order addObject:@(t)]; }
    }
    for (NSNumber *t in @[@0, @2, @1, @3, @4]) { if (![order containsObject:t]) { [order addObject:t]; } }
    return order;
}
static inline NSInteger ALBDockSingleType(void) {
    NSInteger t = [ALBDockPrefValue(kALBDockPrefSingle) integerValue];
    return (t >= 0 && t < ALBDockTypeCount) ? t : ALBDockTypeFriends;
}
static inline BOOL ALBDockTypeHidden(NSInteger type) {
    id hidden = ALBDockPrefValue(kALBDockPrefHidden);
    return [hidden isKindOfClass:[NSArray class]] && [hidden containsObject:@(type)];
}
// 主栏里的项（单独键在右时从左到右）：顺序里去掉单独键、去掉隐藏的；全藏了留第一项
static inline NSArray<NSNumber *> *ALBDockMainTypes(void) {
    NSInteger single = ALBDockSingleType();
    NSMutableArray<NSNumber *> *main = [NSMutableArray array];
    NSNumber *first = nil;
    for (NSNumber *t in ALBDockTypeOrder()) {
        if (t.integerValue == single) { continue; }
        if (!first) { first = t; }
        if (!ALBDockTypeHidden(t.integerValue)) { [main addObject:t]; }
    }
    if (!main.count && first) { [main addObject:first]; }
    return main;
}
static inline BOOL ALBDockMidShown(void) { return ALBDockPrefBool(kALBDockPrefMid); }

#pragma mark - 和本插件的接口（Tweak.x 实现）

// 3.11.45：✦ ＝ 打开双击面板（在 source 的位置）；↑ 快捷跳转（kind 1 消息通知 2 特别关注 3 我的订单 4 观看历史）
FOUNDATION_EXPORT BOOL ALBDockHostOpenPalette(UIView * _Nullable source);
FOUNDATION_EXPORT BOOL ALBDockHostQuickJump(NSInteger kind);
// ↑：打开快捷跳转面板（和 ✦ 的悬浮面板同一种；已开着就关）
FOUNDATION_EXPORT BOOL ALBDockHostOpenQuickJumpPanel(UIView * _Nullable source);
FOUNDATION_EXPORT NSString * _Nonnull ALBDockHostQuickJumpTitle(NSInteger kind);
FOUNDATION_EXPORT NSString * _Nonnull ALBDockHostQuickJumpSymbol(NSInteger kind);
FOUNDATION_EXPORT BOOL ALBDockHostQuickJumpReady(NSInteger kind);

// 「底栏垂直位置」（和液态底栏共用，正值向下）
FOUNDATION_EXPORT CGFloat ALBDockHostVerticalOffset(void);
// 插件已授权且总开关打开：只有这时才把抖音原生底栏变透明（否则原生底栏原样保留）
FOUNDATION_EXPORT BOOL ALBDockHostPluginActive(void);
// 插件总开关、清屏、插件设置页、弹出页面（半屏页、相册、操作表）盖住原生底栏等综合判断：此刻底栏可以显示
FOUNDATION_EXPORT BOOL ALBDockHostAllowsDock(UIView * _Nullable nativeBar);
// 插件自己已经读到的本人头像（没有就 nil）
FOUNDATION_EXPORT UIImage * _Nullable ALBDockHostOwnAvatar(void);
// 3.11.33：液态底栏「图标」页给某一项设的系统符号（没设或设的符号不存在 → fallback），同 AwemeAlbumMorphBar.swift 用的那一套
FOUNDATION_EXPORT NSString * _Nonnull ALBDockHostConfiguredSymbol(NSString * _Nonnull key, NSString * _Nonnull fallback);
// 3.11.33：液态底栏选中项的颜色（设置里的强调色）
FOUNDATION_EXPORT UIColor * _Nullable ALBDockHostAccentColor(void);
// 图标页改了图标 / 强调色（Tweak.x AWERefreshAllConfiguredIcons 发）
#define ALBDockIconsChangedNotification @"ALBDock.IconsChanged"

#pragma mark - 按页面区分材质（实现在 ALBDockMaterial.m）
//
//  视频页（首页 / 朋友）：深色 + 「透明玻璃」开关（开 = Clear + 黑 28% 染色）
//  其他页：系统 Regular 玻璃、不染色、深浅色取系统（UIWindowScene 的 trait——抖音把窗口钉成浅色，
//  沿视图树继承拿不到真值），系统「液态玻璃：透明 / 着色」设置由系统材质自己处理
//

#define ALBDockMaterialChangedNotification @"ALBDockMaterialChanged"
// 3.11.45：系统深浅切换了（不管在不在视频页都发；底栏把三块自建玻璃整只重装，免得停在不透明那一帧）
#define ALBDockSystemStyleChangedNotification @"ALBDockSystemStyleChanged"

FOUNDATION_EXPORT BOOL ALBDockSceneIsVideo(void);
// 3.11.1：视频页背后够亮（本插件按「浅色阈值」取样判断，Tweak.x ALBTone*）→ 浅色玻璃
FOUNDATION_EXPORT BOOL ALBDockSceneIsLight(void);
FOUNDATION_EXPORT void ALBDockMaterialNoteBackdrop(BOOL light);
// 3.11.44：底栏深浅每次变化的记录（诊断“三段式底栏”里列最近几次：什么时候、变成什么、为什么）
FOUNDATION_EXPORT void ALBDockMaterialLog(NSString * _Nonnull event);
FOUNDATION_EXPORT NSString * _Nonnull ALBDockMaterialLogReport(void);
// 3.11.2：首页里的团购 / 经验 / 商城这类频道（不是视频）：底栏用系统玻璃，不用透明（Tweak.x ALBGlassTone.h）
FOUNDATION_EXPORT BOOL ALBDockHostNonVideoChannel(void);
FOUNDATION_EXPORT UIUserInterfaceStyle ALBDockSystemStyle(void);
// window：取它所在场景的系统深浅色；title：抖音当前选中页（首页 / 朋友 / 消息 / 我），nil = 不变
FOUNDATION_EXPORT void ALBDockMaterialUpdate(UIWindow * _Nullable window, NSString * _Nullable title);

// 3.11.31「dock.followEdge」（3.11.32 起默认关：实测底栏这种玻璃登记后系统没给它口袋、不随背景切，反而失去透明；Tweak.x ALB37GlassLayer.h）：底栏和右侧、双击面板同一种玻璃——
// 常规玻璃（不用透明染色）、深浅取顶栏那条边的（视频页深色，其他页跟页面），四块玻璃登记在底栏自己的边上，
// 由系统按背后亮度切。关掉＝下面原来的按页面深色 / 透明。
FOUNDATION_EXPORT BOOL ALBDockHostFollowEdge(void);
FOUNDATION_EXPORT UIUserInterfaceStyle ALBDockHostEdgeStyle(void);
FOUNDATION_EXPORT void ALBDockHostSyncEdge(UIView * _Nullable dock, NSArray<UIView *> * _Nullable glass);

static inline BOOL ALBDockGlassUsesClear(void) {
    return !ALBDockHostFollowEdge() && ALBDockSceneIsVideo() && ALB25MaterialMotionClear();
}
// 3.11.51 用户：“玻璃锁定深色”“随亮度切换，如果是根据图标的话，那它还有点用”。视频页玻璃固定深色；
// 背后亮度只决定图标、文字的黑白（ALBDockInkStyle）。其他页照旧跟系统深浅。
static inline UIUserInterfaceStyle ALBDockGlassStyle(void) {
    if (ALBDockHostFollowEdge()) { return ALBDockHostEdgeStyle(); }
    if (!ALBDockSceneIsVideo()) { return ALBDockSystemStyle(); }
    return UIUserInterfaceStyleDark;
}
static inline UIUserInterfaceStyle ALBDockInkStyle(void) {
    if (!ALBDockHostFollowEdge() && ALBDockSceneIsVideo()) {
        return ALBDockSceneIsLight() ? UIUserInterfaceStyleLight : UIUserInterfaceStyleDark;
    }
    return ALBDockGlassStyle();
}

// 玻璃上的内容色：深色下白、浅色下黑（动态色，跟着视图的 overrideUserInterfaceStyle 自动换）
static inline UIColor *ALBDockInk(CGFloat a) {
    return [UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *tc) {
        return tc.userInterfaceStyle == UIUserInterfaceStyleLight ? [UIColor colorWithWhite:0 alpha:a]
                                                                  : [UIColor colorWithWhite:1 alpha:a];
    }];
}
// 3.11.33：选中项的颜色＝液态底栏的强调色（「dock.accent」，默认开）；关掉或读不到＝抖音红
static inline UIColor *ALBDockSelectedTint(void) {
    UIColor *accent = ALBDockPrefBool(kALBDockPrefAccent) ? ALBDockHostAccentColor() : nil;
    return accent ?: ALBDockHex(0xFE2C55, 1);
}
static inline UIColor *ALBDockDynamic(UIColor *dark, UIColor *light) {
    return [UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *tc) {
        return tc.userInterfaceStyle == UIUserInterfaceStyleLight ? light : dark;
    }];
}

#pragma mark - iOS 26 液态玻璃（运行时取类，SDK 不需要有）
//
//    默认 UIGlassEffectStyleRegular（跟随 overrideUserInterfaceStyle 深色）
//    透明 UIGlassEffectStyleClear + tintColor 染色（Clear 只认染色不认 override）
//  另外用 UIGlassContainerEffect 把底栏三块 + 圆键装进同一个容器，收缩时它们靠近就像水银一样融合。
//

static inline BOOL ALBDockGlassAvailable(void) { return NSClassFromString(@"UIGlassEffect") != nil; }

// clear：YES = Clear + 染色（深色：黑「glass.darkTint」默认 28%；浅色：白「glass.lightTint」），NO = Regular
static inline UIVisualEffect *ALBDockMakeGlassTone(BOOL interactive, BOOL clear, BOOL light) {
    Class G = NSClassFromString(@"UIGlassEffect");
    SEL make = NSSelectorFromString(@"effectWithStyle:");
    if (G && [G respondsToSelector:make]) {
        id e = ((id (*)(id, SEL, NSInteger))objc_msgSend)(G, make, clear ? 1 : 0);
        SEL setInter = NSSelectorFromString(@"setInteractive:");
        if ([e respondsToSelector:setInter]) { ((void (*)(id, SEL, BOOL))objc_msgSend)(e, setInter, interactive); }
        if (clear) {
            SEL setTint = NSSelectorFromString(@"setTintColor:");
            // 3.11.33：浅色用底栏自己的白色染色（「dock.lightTint」，默认 0），深色照旧黑色染色
            UIColor *tint = light ? [UIColor colorWithWhite:1 alpha:MIN(MAX(ALBDockPrefDouble(kALBDockPrefLightWhite), 0.0), 0.4)]
                                  : ALB25MaterialToneTint(NO);
            if ([e respondsToSelector:setTint]) {
                ((void (*)(id, SEL, id))objc_msgSend)(e, setTint, tint);
            }
        }
        if ([e isKindOfClass:[UIVisualEffect class]]) { return e; }
    }
    return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterial];   // 跟随 override 深浅
}

static inline UIVisualEffect *ALBDockMakeGlassStyle(BOOL interactive, BOOL clear) { return ALBDockMakeGlassTone(interactive, clear, NO); }

// 底栏 / 胶囊：按页面（视频页透明开关、按背后亮度深 / 浅，其他页 Regular）
static inline UIVisualEffect *ALBDockMakeGlass(BOOL interactive) {
    return ALBDockMakeGlassTone(interactive, ALBDockGlassUsesClear(), NO);   // 3.11.51：玻璃固定深色染色
}

static inline UIVisualEffect *ALBDockMakeGlassContainer(CGFloat spacing) {
    Class C = NSClassFromString(@"UIGlassContainerEffect");
    if (!C) { return nil; }
    id e = [[C alloc] init];
    SEL setSpacing = NSSelectorFromString(@"setSpacing:");
    if ([e respondsToSelector:setSpacing]) { ((void (*)(id, SEL, CGFloat))objc_msgSend)(e, setSpacing, spacing); }
    return [e isKindOfClass:[UIVisualEffect class]] ? e : nil;
}

#pragma mark - 圆键按压（原工程常量）

static const NSTimeInterval kALBDockPressDownDuration = 0.10;
static const NSTimeInterval kALBDockPressUpDuration   = 0.34;
static const CGFloat        kALBDockPressDamping      = 0.55;
static const CGFloat        kALBDockPressVelocity     = 0.50;
static const CGFloat        kALBDockPressScale        = 0.92;
