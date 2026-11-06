/* 玻璃深浅 (3.11.19). Included once from Tweak.x after ALBSidePalette.h;
   plain Objective-C, no Logos.

   What is left of 3.11.1-3.11.18 is the way the top chrome and the bottom
   bar already worked, now used by every glass: the right column, the
   double-tap panel and the status toast wear the bottom bar's own glass
   (ALB25MaterialFollowBar) on bar item faces (ALBBarFace.h), each Dark on an
   edge of its own (Tweak.x ALBFollowSyncEdge), and UIKit turns each one
   light or dark by what is behind it. The plug-in reads no picture of the
   screen and has no threshold of its own (3.11.1-3.11.9's tone, 3.11.12-
   3.11.18's trials on UIKit's readings and starting brightness: removed).

   Here: whether the page is a video page (Clear only there, for the top
   chrome's "glass.clear" and the three-part dock), the top chrome's glass
   recipe, and the avatar / top-left menus ("menu.followGlass"). */

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>

FOUNDATION_EXPORT BOOL ALBDockBusy(void);
FOUNDATION_EXPORT BOOL ALBDockSceneIsVideo(void);
extern BOOL ALBDockModeActive(void);

static NSString *gALBToneTopReport, *gALBToneMenuReport;

#pragma mark - Video page or not (3.11.2)

/* Home's channel (or the page) on screen plays no video: 团购 and the other
   life-service / lynx channels, the mall, a grid, the profile. Cached 0.3 s. */
static BOOL ALBToneBusyScrolling(void) {
    UIScrollView *feed = gALB3FeedTable;
    return (feed.window && (feed.isTracking || feed.isDragging || feed.isDecelerating)) || ALBDockBusy();
}
/* Home's 团购 / life-service / lynx channel on screen. Read at most once a
   second, never while the feed scrolls or the dock animates. */
static BOOL ALBToneChannelNonVideo(void) {
    static CFTimeInterval at = -10;
    static BOOL value;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - at < 1.0 || (ALBToneBusyScrolling() && at > 0)) return value;
    at = now;
    UIWindow *window = AWEActiveMainWindow();
    value = window && (ALB33GrouponRouteVisible(window) || ALB25MaterialNonVideoChannel() != nil);
    return value;
}
/* … or the mall, a grid, the profile (the route; the same pacing). */
static BOOL ALBToneNonVideoChannel(void) {
    if (ALBToneChannelNonVideo()) return YES;
    static CFTimeInterval at = -10;
    static BOOL value;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - at < 1.0 || (ALBToneBusyScrolling() && at > 0)) return value;
    at = now;
    UIWindow *window = AWEActiveMainWindow();
    AWEBottomRouteKind route = window ? AWEBottomRouteForWindow(window) : AWEBottomRouteNone;
    value = route == AWEBottomRouteShop || route == AWEBottomRouteGrid || route == AWEBottomRouteProfile;
    return value;
}
/* The dock's own idea of the page (its title from Douyin's bar) in dock
   mode, the tab index otherwise; and not a non-video channel. */
static BOOL ALBToneVideoContext(void) {
    BOOL tab = ALBDockModeActive() ? ALBDockSceneIsVideo() : AWEIndexUsesHomeVisualPolicy(AWECurrentMainTabIndex());
    return tab && !ALBToneNonVideoChannel();
}
/* ALBDockMaterial.m: Home's 团购 / 经验 / 商城 channels are not video pages. */
BOOL ALBDockHostNonVideoChannel(void) { return ALBToneNonVideoChannel(); }


#pragma mark - The top chrome

/* UIKit's own bar glass. On a video page with "glass.clear" every glass of
   it (the group's capsule, a system glass button inside it) is the Clear
   recipe, Dark; on any other page UIKit's own glass and the system's
   light/dark; "glass.clear" off: UIKit's own glass and the chrome's Dark
   (3.11.0). UIKit turns it on its edge either way. */
static BOOL ALBToneMenuShowing(UIWindow *window) {
    for (UIView *subview in window.subviews) {
        NSString *name = NSStringFromClass(subview.class);
        if ([name hasPrefix:@"_UI"] && [name containsString:@"ContextMenu"]) return YES;
        for (UIView *child in subview.subviews) {
            NSString *inner = NSStringFromClass(child.class);
            if ([inner hasPrefix:@"_UI"] && [inner containsString:@"ContextMenu"]) return YES;
        }
    }
    return NO;
}
static UIView *ALBToneTopChrome(void) {
    UIView *container = gAWEHomeFeedTopContainer;
    UIView *chrome = container ? (UIView *)ALB3TopChromeForContainer(container) : nil;
    return chrome.window ? chrome : nil;
}
static void ALBToneApplyTop(UIView *chrome) {
    if (!chrome || ALBToneMenuShowing(chrome.window)) return;
    BOOL option = ALB25MaterialLiftedClear();
    BOOL clear = option && ALBToneVideoContext();
    UIUserInterfaceStyle want = clear ? UIUserInterfaceStyleDark
        : (option ? chrome.window.windowScene.traitCollection.userInterfaceStyle : UIUserInterfaceStyleDark);
    if (want != UIUserInterfaceStyleLight) want = UIUserInterfaceStyleDark;
    // Light/dark first: UIKit re-resolves a bar item's glass on a trait
    // change, so the recipe is written after it.
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:chrome];
    for (NSUInteger i = 0; i < queue.count && i < 400; i++) {
        UIView *view = queue[i];
        [queue addObjectsFromArray:view.subviews];
        UIUserInterfaceStyle style = view.overrideUserInterfaceStyle;
        // Only the views the chrome pinned to Dark itself (the chrome, its buttons).
        if ((style == UIUserInterfaceStyleDark || style == UIUserInterfaceStyleLight) && style != want &&
            [NSStringFromClass(view.class) hasPrefix:@"ALB3"])
            view.overrideUserInterfaceStyle = want;
    }
    gALBToneTopReport = ALB25MaterialApplyTone(chrome, clear, NO, 0, 400);
}
static void ALBToneTouchTop(UIView *chrome) {
    static CFTimeInterval last;
    CFTimeInterval now = CACurrentMediaTime();
    if (!chrome || now - last < 0.1) return;
    last = now;
    ALBToneApplyTop(chrome);
}

static BOOL ALBToneViewShowing(UIView *view, UIWindow *window) {
    if (!view || view.window != window) return NO;
    for (UIView *cursor = view; cursor && cursor != window; cursor = cursor.superview)
        if (cursor.hidden || cursor.alpha < 0.05) return NO;
    return YES;
}
static void ALBToneTick(void) {
    if (!AWEPluginFeaturesEnabled() || AWEOperatingSystemMajorVersion() < 26 ||
        UIApplication.sharedApplication.applicationState != UIApplicationStateActive) return;
    // The top chrome's glass every pass (cheap): the recipe on a video page,
    // UIKit's own elsewhere or with "glass.clear" off.
    ALBToneApplyTop(ALBToneTopChrome());
}
static void ALBToneStart(void) {
    static NSTimer *timer;
    if (timer) return;
    timer = [NSTimer timerWithTimeInterval:0.25 repeats:YES block:^(__unused NSTimer *t) { ALBToneTick(); }];
    timer.tolerance = 0.05;
    [NSRunLoop.mainRunLoop addTimer:timer forMode:NSRunLoopCommonModes];
}

#pragma mark - Menus ("menu.followGlass")

/* The avatar's menu and the top-left menu get their button's glass before
   their first frame (a run-loop pass right before Core Animation commits,
   for 0.6 s after the menu appears), never more transparent than 35 %. */
static const CGFloat kALBToneMenuMinTint = 0.35;
static BOOL ALBToneMenuFollow(void) { return NO; }   // 3.11.49：展开菜单不再跟随按钮的玻璃（用系统自己的样子）
static UIView *ALBToneMenuIn(UIView *view, NSUInteger depth) {
    if (!view) return nil;
    NSString *name = NSStringFromClass(view.class);
    if ([name hasPrefix:@"_UI"] && [name containsString:@"ContextMenu"]) return view;
    if (depth >= 3) return nil;
    for (UIView *subview in view.subviews) {
        UIView *found = ALBToneMenuIn(subview, depth + 1);
        if (found) return found;
    }
    return nil;
}
/* Where the menu's own content is (the first view clearly smaller than the screen). */
static CGRect ALBToneMenuContentRect(UIView *menu) {
    UIWindow *window = menu.window;
    CGFloat screen = CGRectGetWidth(window.bounds) * CGRectGetHeight(window.bounds);
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:menu];
    for (NSUInteger i = 0; i < queue.count && i < 200; i++) {
        UIView *view = queue[i];
        CGRect rect = [view convertRect:view.bounds toView:nil];
        if (view != menu && !view.hidden && CGRectGetWidth(rect) >= 120.0 &&
            CGRectGetWidth(rect) * CGRectGetHeight(rect) < screen * 0.6) return rect;
        [queue addObjectsFromArray:view.subviews];
    }
    return CGRectNull;
}
static void ALBToneStyleMenus(UIWindow *window) {
    if (!window || !ALBToneMenuFollow()) return;
    UIButton *avatar = nil;
    for (UIButton *button in ALBRGButtons().allObjects) if (button.window && ALBRGMenuActive(button)) { avatar = button; break; }
    // 3.11.4: the avatar on its own edge ("glass.followBar"): UIKit's menu,
    // which follows the button by itself (as the top-left one follows the top).
    if (avatar && ALB25MaterialFollowBar()) return;
    UIView *chrome = avatar ? nil : ALBToneTopChrome();
    if (chrome && !ALBToneViewShowing(chrome, window)) chrome = nil;
    if (!avatar && !chrome) return;
    for (UIView *subview in [window.subviews copy]) {
        UIView *menu = ALBToneMenuIn(subview, 0);
        if (!menu) continue;
        BOOL clear, light;
        if (avatar) {
            clear = ALB25MaterialMotionClear();
            light = avatar.overrideUserInterfaceStyle == UIUserInterfaceStyleLight;
        } else {
            CGRect content = ALBToneMenuContentRect(menu);
            if (CGRectIsNull(content) || CGRectGetMidY(content) > CGRectGetHeight(window.bounds) * 0.45 ||
                CGRectGetMidX(content) > CGRectGetWidth(window.bounds) * 0.6) continue;
            clear = ALB25MaterialLiftedClear() && ALBToneVideoContext();
            light = NO;
        }
        if (!clear) continue;
        gALBToneMenuReport = [NSString stringWithFormat:@"%@：%@%@", avatar ? @"头像菜单" : @"左上角菜单",
                              light ? @"浅色 " : @"深色 ", ALB25MaterialApplyTone(menu, YES, light, kALBToneMenuMinTint, 600)];
    }
}
/* A view came into a window: for 0.6 s, right before every Core Animation
   commit, a menu that appeared gets its glass (so its first frame already
   has it; 3.11.1 waited 0-0.6 s and the menu showed UIKit's glass first). */
static __weak UIWindow *gALBToneMenuWindow;
static CFTimeInterval gALBToneMenuUntil;
static CFRunLoopObserverRef gALBToneMenuObserver;
static void ALBToneNoteWindowSubview(UIWindow *window, UIView *subview) {
    if (!subview || !ALBToneMenuFollow() || !AWEPluginFeaturesEnabled()) return;
    gALBToneMenuWindow = window;
    gALBToneMenuUntil = CACurrentMediaTime() + 0.6;
    if (gALBToneMenuObserver) return;
    gALBToneMenuObserver = CFRunLoopObserverCreateWithHandler(kCFAllocatorDefault, kCFRunLoopBeforeWaiting, true, 1999000,
        ^(CFRunLoopObserverRef observer, __unused CFRunLoopActivity activity) {
            if (CACurrentMediaTime() > gALBToneMenuUntil || !gALBToneMenuWindow) {
                CFRunLoopRemoveObserver(CFRunLoopGetMain(), observer, kCFRunLoopCommonModes);
                CFRelease(observer);
                gALBToneMenuObserver = NULL;
                return;
            }
            ALBToneStyleMenus(gALBToneMenuWindow);
        });
    CFRunLoopAddObserver(CFRunLoopGetMain(), gALBToneMenuObserver, kCFRunLoopCommonModes);
}


#pragma mark - 诊断

/* 3.11.4-3.11.19: every glass on the bar's glass, on its own edge. */
static NSString *ALBFollowReport(void) {
    if (!ALB25MaterialFollowBar()) return @"关（iOS 26 以下）";
    NSUInteger right = 0, rightEdges = 0;
    for (UIButton *button in ALBRGButtons().allObjects) {
        if (!button.window) continue;
        right++;
        if (objc_getAssociatedObject(button, &kALBFollowModeKey)) rightEdges++;
    }
    ALBSidePalette *palette = gALBSidePalette;
    NSString *panel = @"未打开";
    if (palette.window) {
        NSUInteger count = 0, faceCount = 0, faceEdgeCount = 0, taps = 0, sliders = 0, sliderEdges = 0, disabled = 0;
        NSString *panelSample = nil;
        NSMutableArray<UIView *> *glasses = [NSMutableArray arrayWithArray:palette.icons ?: @[]];
        [glasses addObjectsFromArray:palette.pills ?: @[]];
        for (UIView *glass in glasses) {
            if ([glass isKindOfClass:ALBPaletteSlider.class]) {
                sliders++;
                if (objc_getAssociatedObject(glass, &kALBFollowModeKey)) sliderEdges++;
                continue;
            }
            count++;
            if (objc_getAssociatedObject(glass, &kALBFaceTapKey)) taps++;
            if ([glass isKindOfClass:UIControl.class] && !((UIControl *)glass).enabled) disabled++;
            if (![glass isKindOfClass:UIButton.class] || !ALBFaceActive(glass)) continue;
            faceCount++;
            ALB3BarGlassButton *face = ALBFaceFor(glass);
            if (objc_getAssociatedObject(face, &kALBFollowModeKey)) faceEdgeCount++;
            if (!panelSample && [palette.pills containsObject:glass])
                panelSample = [NSString stringWithFormat:@"%@ 登记=%@ 控件=%@", face.diagnosticsText, ALB35RegistrationText(face),
                               ALBFaceTouchSubclassOf(ALBFaceControl(face)) ? @"已接管" : @"未接管"];
        }
        panel = [NSString stringWithFormat:@"系统栏按钮%lu/%lu（登记%lu） 按抬起发动作%lu/%lu 不可点%lu 滑条登记%lu/%lu 胶囊示例：%@",
                 (unsigned long)faceCount, (unsigned long)count, (unsigned long)faceEdgeCount, (unsigned long)taps, (unsigned long)count,
                 (unsigned long)disabled, (unsigned long)sliderEdges, (unsigned long)sliders, panelSample ?: @"-"];
    }
    NSUInteger faces = 0, faceEdges = 0;
    NSString *sample = nil;
    for (UIButton *button in ALBRGButtons().allObjects) {
        if (!button.window || !ALBFaceActive(button)) continue;
        faces++;
        ALB3BarGlassButton *face = ALBFaceFor(button);
        if (objc_getAssociatedObject(face, &kALBFollowModeKey)) faceEdges++;
        if (!sample && [button isKindOfClass:ALBRightGlassButton.class])
            sample = [NSString stringWithFormat:@"%@ 登记=%@ 风格=%ld/%ld", face.diagnosticsText, ALB35RegistrationText(face),
                      (long)face.overrideUserInterfaceStyle, (long)face.traitCollection.userInterfaceStyle];
    }
    return [NSString stringWithFormat:@"开（底栏的玻璃、每颗自己的边，深浅由系统切） 系统栏按钮%@ 右侧%lu颗（登记%lu） 示例：%@ ｜按压：接管%lu个控件 按下%lu 抬起%lu 取消%lu（其中面板 按下%lu 抬起%lu 取消%lu） 面板补发动作%lu ｜按钮里玻璃=%@ 右侧按钮自己登记%lu/%lu ｜双击面板：%@",
        ALBBarFacesWanted() ? @"开" : @"不可用", (unsigned long)faces, (unsigned long)faceEdges, sample ?: @"-",
        (unsigned long)gALBFaceTouchAdopted, (unsigned long)gALBFaceTouchBegan, (unsigned long)gALBFaceTouchEnded,
        (unsigned long)gALBFaceTouchCancelled, (unsigned long)gALBFacePanelBegan, (unsigned long)gALBFacePanelEnded,
        (unsigned long)gALBFacePanelCancelled, (unsigned long)gALBFaceTapSent, ALB25MaterialFollowGlassKey(),
        (unsigned long)rightEdges, (unsigned long)right, panel];
}
static NSString *ALBToneReport(void) {
    UIView *chrome = ALBToneTopChrome();
    UIView *root = AWEActiveMainWindow().rootViewController.viewIfLoaded;
    NSString *topPlace = !chrome ? @"不在" : (root && [chrome isDescendantOfView:root] ? @"原地" : @"最上层");
    return [NSString stringWithFormat:
        @"视频页=%@\n"
        "  顶栏：%@ 透明=%@ %@\n"
        "  菜单跟随按钮玻璃=%@ %@\n"
        "  顶栏同款：%@",
        ALBToneVideoContext() ? @"是" : @"否",
        topPlace, ALB25MaterialLiftedClear() ? @"开" : @"关", gALBToneTopReport ?: @"",
        ALBToneMenuFollow() ? @"开" : @"关", gALBToneMenuReport ?: @"",
        ALBFollowReport()];
}

/* 3.11.1 诊断: the 团购 channel's lists and fills (read-only). */
static NSString *ALB47GrouponReport(void);
static NSString *ALBGrouponReport(void) {
    UIView *container = gALB33GrouponContainer;
    UIWindow *window = container.window;
    if (!container || !window) return [NSString stringWithFormat:@"团购：不在屏幕上；%@", ALB47GrouponReport()];
    CGFloat ww = CGRectGetWidth(window.bounds), wh = CGRectGetHeight(window.bounds);
    NSMutableArray<NSString *> *lines = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:container];
    for (NSUInteger i = 0; i < queue.count && i < 900 && lines.count < 16; i++) {
        UIView *view = queue[i];
        [queue addObjectsFromArray:view.subviews];
        if (view.hidden || view.alpha < 0.01 || ALBIsOwnedView(view)) continue;
        CGRect rect = [view convertRect:view.bounds toView:window];
        if (CGRectGetWidth(rect) < ww * 0.85) continue;
        if ([view isKindOfClass:UIScrollView.class]) {
            UIScrollView *scroll = (UIScrollView *)view;
            if (CGRectGetHeight(rect) < wh * 0.25 || !ALB34ScrollIsVerticalList(scroll)) continue;
            [lines addObject:[NSString stringWithFormat:@"  列表 %@ y%.0f-%.0f 偏移%.0f 内容高%.0f 下内边距%.0f 已延长=%@ 裁剪=%d",
                NSStringFromClass(view.class), CGRectGetMinY(rect), CGRectGetMaxY(rect), scroll.contentOffset.y,
                scroll.contentSize.height, scroll.adjustedContentInset.bottom,
                objc_getAssociatedObject(scroll, &kALB34GrouponFrameKey) ? @"是" : @"否", view.clipsToBounds]];
            continue;
        }
        UIColor *background = view.backgroundColor;
        CGFloat alpha = background ? CGColorGetAlpha(background.CGColor) : 0;
        BOOL cleared = objc_getAssociatedObject(view, &kALB34GrouponClearedKey) != nil;
        if ((alpha < 0.05 && !cleared) || CGRectGetHeight(rect) > 200.0) continue;
        BOOL bottomBand = CGRectGetMaxY(rect) >= wh - 4.0 && CGRectGetMinY(rect) >= wh - 160.0;
        BOOL upperBand = CGRectGetMinY(rect) >= 60.0 && CGRectGetMaxY(rect) <= wh * 0.45;
        if (!bottomBand && !upperBand) continue;
        CGFloat white = 0;
        [background getWhite:&white alpha:NULL];
        [lines addObject:[NSString stringWithFormat:@"  %@ %@ y%.0f-%.0f 底色亮%.2f 不透明%.2f 子视图%lu%@", bottomBand ? @"底部底色" : @"上部底色",
            NSStringFromClass(view.class), CGRectGetMinY(rect), CGRectGetMaxY(rect), white, alpha,
            (unsigned long)view.subviews.count, cleared ? @" 已清" : @""]];
    }
    return [NSString stringWithFormat:@"团购（%@）：%@\n%@", AWEBottomRouteName(AWEBottomRouteForWindow(window)), ALB47GrouponReport(),
            lines.count ? [lines componentsJoinedByString:@"\n"] : @"  没有找到列表或底色"];
}
