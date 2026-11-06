//
//  ALBDockMaterial.m —— 按页面区分材质（2.5.0）
//
//  · 视频页（首页 / 朋友）：沿用原来的深色 + 透明开关
//  · 其他页：系统 Regular 玻璃，深浅色跟系统
//
//  系统深浅色只能从 UIWindowScene 拿：抖音把 window.overrideUserInterfaceStyle 钉成浅色，
//  它自己的深色模式是换肤画的，沿视图树继承到的永远是浅色（实测）。
//  场景上挂 trait 监听即时生效；底栏 0.35s 的同步定时器也会顺手再读一次兜底。
//

#import "ALBDockCommon.h"

static BOOL gALBDockVideo = YES;                                    // 抖音启动在首页
static UIUserInterfaceStyle gALBDockSystem = UIUserInterfaceStyleDark;
static __weak UIWindowScene *gALBDockObservedScene;
static BOOL gALBDockPostPending = NO;

static BOOL gALBDockLight = NO;                                     // 3.11.1：视频页背后够亮
BOOL ALBDockSceneIsVideo(void) { return gALBDockVideo; }
BOOL ALBDockSceneIsLight(void) { return gALBDockVideo && gALBDockLight; }
UIUserInterfaceStyle ALBDockSystemStyle(void) { return gALBDockSystem; }

// 3.11.44：深浅变化的记录（最近 12 条）
static NSMutableArray<NSString *> *gALBDockMaterialEvents;
void ALBDockMaterialLog(NSString *event) {
    if (!event.length) { return; }
    if (!gALBDockMaterialEvents) { gALBDockMaterialEvents = [NSMutableArray array]; }
    [gALBDockMaterialEvents addObject:[NSString stringWithFormat:@"%.2f %@", CACurrentMediaTime(), event]];
    while (gALBDockMaterialEvents.count > 12) { [gALBDockMaterialEvents removeObjectAtIndex:0]; }
}
NSString *ALBDockMaterialLogReport(void) {
    if (!gALBDockMaterialEvents.count) { return @"还没有变过"; }
    CFTimeInterval now = CACurrentMediaTime();
    NSMutableArray<NSString *> *lines = [NSMutableArray array];
    for (NSString *event in gALBDockMaterialEvents) {
        NSRange space = [event rangeOfString:@" "];
        double at = space.location == NSNotFound ? now : [[event substringToIndex:space.location] doubleValue];
        [lines addObject:[NSString stringWithFormat:@"%.1f 秒前 %@", now - at,
                          space.location == NSNotFound ? event : [event substringFromIndex:space.location + 1]]];
    }
    return [lines componentsJoinedByString:@"；"];
}

// 合并到下一轮主队列再通知，一轮最多一次
static void ALBDockMaterialPost(void) {
    if (gALBDockPostPending) { return; }
    gALBDockPostPending = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        gALBDockPostPending = NO;
        [[NSNotificationCenter defaultCenter] postNotificationName:ALBDockMaterialChangedNotification object:nil];
    });
}

static void ALBDockSetSystemStyle(UIUserInterfaceStyle s) {
    if (s == UIUserInterfaceStyleUnspecified || s == gALBDockSystem) { return; }
    gALBDockSystem = s;
    ALBDockMaterialLog([NSString stringWithFormat:@"系统深浅→%@", s == UIUserInterfaceStyleDark ? @"深" : @"浅"]);
    if (!gALBDockVideo) { ALBDockMaterialPost(); }      // 视频页固定深色，系统切深浅色不影响
    // 3.11.45：不管在不在视频页，自建玻璃都要整只重装一次（系统换深浅时玻璃会停在不透明那一帧）
    dispatch_async(dispatch_get_main_queue(), ^{
        [[NSNotificationCenter defaultCenter] postNotificationName:ALBDockSystemStyleChangedNotification object:nil];
    });
}

static void ALBDockObserveScene(UIWindowScene *scene) {
    if (!scene || scene == gALBDockObservedScene) { return; }
    gALBDockObservedScene = scene;
    // iOS 17+ 的 registerForTraitChanges:withHandler:，运行时调用（SDK 低也能编）
    SEL reg = NSSelectorFromString(@"registerForTraitChanges:withHandler:");
    Class trait = NSClassFromString(@"UITraitUserInterfaceStyle");
    if (!trait || ![scene respondsToSelector:reg]) { return; }
    void (^handler)(id, UITraitCollection *) = ^(id env, UITraitCollection *previous) {
        if ([env respondsToSelector:@selector(traitCollection)]) {
            ALBDockSetSystemStyle([(id<UITraitEnvironment>)env traitCollection].userInterfaceStyle);
        }
    };
    ((id (*)(id, SEL, id, id))objc_msgSend)(scene, reg, @[trait], handler);
}

// 3.11.1：Tweak.x 的取样（每 0.5 秒最多一次，静止时）告诉这里视频页底栏背后是不是够亮；变了才通知
void ALBDockMaterialNoteBackdrop(BOOL light) {
    if (light == gALBDockLight) { return; }
    gALBDockLight = light;
    ALBDockMaterialLog([NSString stringWithFormat:@"背后亮度→%@%@", light ? @"浅" : @"深", gALBDockVideo ? @"" : @"（不在视频页，不用）"]);
    if (gALBDockVideo) { ALBDockMaterialPost(); }
}

void ALBDockMaterialUpdate(UIWindow *window, NSString *title) {
    UIWindowScene *scene = window.windowScene;
    if (scene) {
        ALBDockObserveScene(scene);
        ALBDockSetSystemStyle(scene.traitCollection.userInterfaceStyle);
    }
    if (title.length) {
        // 2.5.1：只有明确是 消息 / 我 / 商城 才算非视频页；认不出来一律按视频页（深色 + 透明），宁可不换也不能错换
        // 3.11.2：首页里的团购 / 经验 / 商城频道也不算视频页（用系统玻璃，透明在大片白底上太亮）
        BOOL video = !([title hasPrefix:@"消息"] || [title hasPrefix:@"我"] || [title hasPrefix:@"商城"]) &&
                     !ALBDockHostNonVideoChannel();
        if (video != gALBDockVideo) {
            gALBDockVideo = video;
            ALBDockMaterialLog([NSString stringWithFormat:@"页面→%@（%@）", video ? @"视频页" : @"非视频页", title]);
            // 3.11.1：离开视频页就忘掉「背后亮」，回来时先按深色，等下一次取样（不会把上次的浅色带回来）
            if (!video) { gALBDockLight = NO; }
            ALBDockMaterialPost();
        }
    }
}
