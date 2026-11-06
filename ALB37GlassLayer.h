// 3.7 — LiquidBar glass at the top of the window: the top chrome only.
//
// 3.11: the right column and the double-tap panel no longer have any glass
// here. They draw the collapsing dock's glass in place, in Douyin's own
// buttons (Tweak.x AWEConfigureAdaptiveSystemGlassIconButton, ALBSidePalette.h):
// the 3.7-3.9 mirrors at the top of the window, 3.8's copies in the bar's
// window, 3.10's E2 glass in the bar's window with its own edges, sampled
// light/dark and redrawn badges, and their probes are gone.
//
// What stays is the top chrome:
//  - "glass.lifted" (iOS 26): the top chrome moves into a pass-through layer
//    at the top of the main window, following Douyin's top bar, and goes back
//    next to the top bar while another page or panel covers it or a page is
//    pushed or popped. Updated right before every Core Animation commit.
//  - "glass.inBarWindow": that layer lives in the bottom bar's own window
//    instead (the bench's E), the chrome registered with the bar's scroll
//    view; the bar window hands it its touches (ALB37HitTest).
//  - "top.e2" (a trial): the chrome stays where it is but is registered with
//    the bar's own scroll view instead of the feed.

@interface ALB37GlassLayerView : UIView
@end
@implementation ALB37GlassLayerView
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    return hit == self ? nil : hit;
}
@end

@interface ALB37ChromeRecord : NSObject
@property (nonatomic, weak) UIView *chrome;
@property (nonatomic, weak) UIView *container;
@property (nonatomic, assign) BOOL covered;
@property (nonatomic, assign) CFTimeInterval checkedAt;
@property (nonatomic, weak) UIScrollView *registeredWith;
@end
@implementation ALB37ChromeRecord
@end

static NSMutableArray<ALB37ChromeRecord *> *gALB37Chromes;
static NSHashTable<UIView *> *gALB37Layers;
static NSHashTable<UIScrollView *> *gALB37EdgeTables;
static CFRunLoopObserverRef gALB37Observer;
static BOOL gALB37Syncing;
// "top.e2" on with the chrome in place: refreshed from the observer too.
static BOOL gALB37TopE2InPlace;
static void ALB37Sync(void);
static void ALB3PlaceTopChromeInPlace(UIView *chrome, UIView *container);
static UIScrollView *ALB36BarScrollView(void);

/* 3.8: the bottom bar's window, when the layer lives there. */
static BOOL ALB37BarMode(void) { return NO; }   // 3.11.51：“放进底栏窗口”设置删了，固定关
static UIWindow *ALB37BarWindow(void) {
    UIWindow *window = gAWEAlbumMorphController.viewIfLoaded.window;
    return [window isKindOfClass:AWEAlbumMorphOverlayWindow.class] ? window : nil;
}
static BOOL ALB37WindowShowing(UIWindow *window) {
    return window && !window.hidden && window.alpha > 0.01;
}
/* Where a chrome lifted from `sourceWindow` goes, nil: nowhere right now. */
static UIWindow *ALB37HostWindow(UIWindow *sourceWindow, BOOL barMode) {
    if (!barMode) return sourceWindow;
    UIWindow *bar = ALB37BarWindow();
    return bar && bar != sourceWindow && bar.windowScene == sourceWindow.windowScene && ALB37WindowShowing(bar) ? bar : nil;
}

static BOOL ALB37Lifted(void) { return ALB25MaterialGlassLifted(); }
/* 3.10.2 "top.e2" (off by default, a trial): the top chrome stays where it
   is (its layout, its menus) but is registered with the bar's own scroll
   view instead of the feed. (3.10.1 moved it into the bar's window: its
   side buttons lost their margins and its menu never opened there.) */
static BOOL ALB310TopE2(void) {
    // 3.11.51：“与底栏同款玻璃”设置删了，固定开（用户一直开着）
    return AWEOperatingSystemMajorVersion() == 26 && NSClassFromString(@"UIGlassEffect") != nil;
}
static BOOL ALB37ChromeLifted(void) { return ALB37Lifted(); }
static BOOL ALB37ChromeBarMode(void) { return ALB37BarMode(); }

/* One pass-through layer per window, above the root view: above everything
   of Douyin's page tree, below what UIKit presents over it (sheets,
   alerts) and below the plug-in's own window-level views. */
static char kALB37LayerKey;
static ALB37GlassLayerView *ALB37LayerForWindow(UIWindow *window) {
    if (!window) return nil;
    ALB37GlassLayerView *layer = objc_getAssociatedObject(window, &kALB37LayerKey);
    if (!layer) {
        layer = [[ALB37GlassLayerView alloc] initWithFrame:window.bounds];
        layer.backgroundColor = UIColor.clearColor;
        layer.opaque = NO;
        layer.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        layer.accessibilityIdentifier = @"AwemeLiquidBar.GlassLayer";
        objc_setAssociatedObject(window, &kALB37LayerKey, layer, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!gALB37Layers) gALB37Layers = [NSHashTable weakObjectsHashTable];
        [gALB37Layers addObject:layer];
    }
    UIView *root = window.rootViewController.viewIfLoaded;
    if ([window isKindOfClass:AWEAlbumMorphOverlayWindow.class]) {
        // 3.8.1: the bar's window, above the bar's controller view (and so
        // above the bar's own scroll view), exactly where the bench's E was.
        // Touches are routed by the window's own hit test, not by order.
        UIView *controllerView = gAWEAlbumMorphController.viewIfLoaded;
        if (layer.superview != window) {
            [window addSubview:layer];
        } else if (controllerView.superview == window) {
            NSArray<UIView *> *siblings = window.subviews;
            if ([siblings indexOfObjectIdenticalTo:layer] < [siblings indexOfObjectIdenticalTo:controllerView])
                [window insertSubview:layer aboveSubview:controllerView];
        }
    } else if (root.superview == window) {
        if (layer.superview != window) {
            [window insertSubview:layer aboveSubview:root];
        } else {
            NSArray<UIView *> *siblings = window.subviews;
            if ([siblings indexOfObjectIdenticalTo:layer] < [siblings indexOfObjectIdenticalTo:root])
                [window insertSubview:layer aboveSubview:root];
        }
    } else if (layer.superview != window) {
        [window addSubview:layer];
    }
    if (!CGRectEqualToRect(layer.frame, window.bounds)) layer.frame = window.bounds;
    return layer;
}

static CGFloat ALB37EffectiveAlpha(UIView *view) {
    UIWindow *window = view.window;
    if (!window) return 0;
    CGFloat alpha = 1;
    for (UIView *cursor = view; cursor && cursor != window; cursor = cursor.superview) {
        if (cursor.hidden) return 0;
        alpha *= cursor.alpha;
        if (alpha < 0.01) return 0;
    }
    return alpha;
}

/* The view's place in `target`: its own bounds, its centre mapped from its
   superview, and a transform made of the superview's uniform scale and
   `local` (the top bar's transform). */
static BOOL ALB37Place(UIView *source, UIView *target, CGAffineTransform local, CGRect *bounds, CGPoint *center,
                       CGAffineTransform *transform) {
    UIView *parent = source.superview;
    CGRect own = source.bounds;
    if (!parent || CGRectIsEmpty(own) || !target) return NO;
    CGPoint anchor = CGPointMake(source.center.x + local.tx, source.center.y + local.ty);
    CGPoint mapped = [parent convertPoint:anchor toView:target];
    CGRect unit = [parent convertRect:CGRectMake(0, 0, 100, 100) toView:target];
    if (!isfinite(mapped.x) || !isfinite(mapped.y) || CGRectIsNull(unit)) return NO;
    CGFloat factor = CGRectGetWidth(unit) / 100.0;
    CGFloat scale = isfinite(factor) && factor > 0.01 ? factor : 1.0;
    *bounds = CGRectMake(0, 0, CGRectGetWidth(own), CGRectGetHeight(own));
    *center = mapped;
    *transform = CGAffineTransformMake(local.a * scale, local.b * scale, local.c * scale, local.d * scale, 0, 0);
    return YES;
}

static UIViewController *ALB37OwnerController(UIView *view) {
    for (UIResponder *responder = view; responder; responder = responder.nextResponder)
        if ([responder isKindOfClass:UIViewController.class]) return (UIViewController *)responder;
    return nil;
}

/* The page a controller belongs to: the controller right under a
   navigation controller (or the topmost one). */
static UIViewController *ALB37Page(UIViewController *controller) {
    UIViewController *page = controller;
    while (page.parentViewController && ![page.parentViewController isKindOfClass:UINavigationController.class])
        page = page.parentViewController;
    return page;
}

/* A push or pop (also the interactive back swipe) of the navigation
   controller the view's page is in: those pages are inside the root view,
   under the layer, and positions and covering are only final after it.
   Presentations need nothing: what UIKit presents is above the layer. */
static BOOL ALB37InTransition(UIView *view) {
    UIViewController *navigation = ALB37Page(ALB37OwnerController(view)).parentViewController;
    return [navigation isKindOfClass:UINavigationController.class] && navigation.transitionCoordinator != nil;
}

static UIView *ALB37RootView(UIView *view) {
    UIView *root = view.window.rootViewController.viewIfLoaded;
    return root && root.window == view.window && [view isDescendantOfView:root] ? root : nil;
}

/* 3.10.7: UIKit's menu (the top-left one): its full-screen layer that only
   catches the dismissing tap never covers anything; only the menu itself
   does, where it is. */
static UIView *ALB311MenuContainer(UIView *view) {
    for (UIView *cursor = view; cursor; cursor = cursor.superview) {
        NSString *name = NSStringFromClass(cursor.class);
        if ([name hasPrefix:@"_UI"] && [name containsString:@"ContextMenu"]) return cursor;
    }
    return nil;
}
static UIView *ALB311MenuContentAt(UIView *hit, UIView *container, UIWindow *window) {
    CGFloat screenArea = CGRectGetWidth(window.bounds) * CGRectGetHeight(window.bounds);
    for (UIView *cursor = hit; cursor && cursor != container; cursor = cursor.superview) {
        CGRect rect = [cursor convertRect:cursor.bounds toView:nil];
        if (CGRectGetWidth(rect) * CGRectGetHeight(rect) < screenArea * 0.8) return cursor;
    }
    return nil;
}
/* 3.8, the layer in the bar's window: whatever of the main window is above
   the root view (a presented page or sheet) covers too. */
static UIView *ALB37HitAbove(UIView *source, UIView *root, CGPoint pointInRoot, BOOL barMode, BOOL *outsideRoot) {
    (void)source;
    *outsideRoot = NO;
    if (!barMode) return [root hitTest:pointInRoot withEvent:nil];
    UIWindow *window = root.window;
    UIViewController *presented = window.rootViewController.presentedViewController;
    if (presented && ![NSStringFromClass(presented.class) containsString:@"Menu"]) { *outsideRoot = YES; return nil; }
    CGPoint inWindow = [root convertPoint:pointInRoot toView:window];
    UIView *hit = [window hitTest:inWindow withEvent:nil];
    UIView *menu = ALB311MenuContainer(hit);
    if (menu) {
        UIView *content = ALB311MenuContentAt(hit, menu, window);
        if (!content) return nil;
        *outsideRoot = YES;
        return content;
    }
    if (hit && hit != window && hit != root && ![hit isDescendantOfView:root]) *outsideRoot = YES;
    return hit == window ? nil : hit;
}

/* Top chrome: covered when the topmost view at the top bar belongs to
   another page (a pushed page, a drawer). Every feed channel under the top
   bar (推荐, 关注, 精选, 团购 …) is the same page. */
static BOOL ALB37ChromeCovered(UIView *container, UIView *chrome, BOOL barMode) {
    UIView *root = ALB37RootView(container);
    if (!root) return YES;
    CGPoint point = [container convertPoint:CGPointMake(CGRectGetMidX(container.bounds), CGRectGetMidY(container.bounds)) toView:root];
    if (!CGRectContainsPoint(root.bounds, point)) return NO;
    // Back in place the chrome is in the root view: look under it (hidden for
    // this one call, before anything is drawn).
    BOOL inRoot = chrome && !chrome.hidden && [chrome isDescendantOfView:root];
    if (inRoot) chrome.hidden = YES;
    BOOL outside = NO;
    UIView *hit = ALB37HitAbove(container, root, point, barMode, &outside);
    if (inRoot) chrome.hidden = NO;
    if (outside) return YES;
    if (!hit || [container isDescendantOfView:hit] || [hit isDescendantOfView:container]) return NO;
    UIViewController *mine = ALB37Page(ALB37OwnerController(container));
    UIViewController *theirs = ALB37Page(ALB37OwnerController(hit));
    if (!mine || !theirs || mine == theirs) return NO;
    // A page whose view contains the top bar (the home container) is ours too.
    UIView *theirView = theirs.viewIfLoaded;
    return !(theirView && [container isDescendantOfView:theirView]);
}

static void ALB37ChromeInPlace(ALB37ChromeRecord *record) {
    UIView *chrome = record.chrome;
    if (record.registeredWith) { ALB3RegisterEdge(chrome, nil, 1); record.registeredWith = nil; }
    ALB3PlaceTopChromeInPlace(chrome, record.container);
}

static void ALB37SyncChrome(ALB37ChromeRecord *record, CFTimeInterval now) {
    UIView *chrome = record.chrome, *container = record.container;
    UIWindow *window = container.window;
    BOOL barMode = ALB37ChromeBarMode();
    UIWindow *host = window ? ALB37HostWindow(window, barMode) : nil;
    // Not under the root view, the page is being pushed or popped, or
    // nowhere to go: in place.
    if (!host || !ALB37RootView(container) || ALB37InTransition(container)) { ALB37ChromeInPlace(record); return; }
    ALB37GlassLayerView *layer = ALB37LayerForWindow(host);
    CGFloat alpha = ALB37EffectiveAlpha(container);
    // Douyin only hiding its top bar (clean screen, comments): hidden where it is.
    if (!ALB3TopContainerShowing(container) || alpha < 0.01) {
        if (chrome.superview == layer) { if (!chrome.hidden) chrome.hidden = YES; }
        else ALB37ChromeInPlace(record);
        return;
    }
    CGRect bounds; CGPoint center; CGAffineTransform transform;
    if (!ALB37Place(container, layer, container.transform, &bounds, &center, &transform)) { ALB37ChromeInPlace(record); return; }
    if (now - record.checkedAt > 0.033) {
        record.checkedAt = now;
        record.covered = ALB37ChromeCovered(container, chrome, barMode);
    }
    if (record.covered) { ALB37ChromeInPlace(record); return; }
    if (chrome.superview != layer) [layer addSubview:chrome];
    // E: in the bar's window, registered with the bar's own scroll view.
    UIScrollView *barScroll = barMode ? ALB36BarScrollView() : nil;
    if (barScroll != record.registeredWith) {
        ALB3RegisterEdge(chrome, barScroll, 1);
        record.registeredWith = barScroll;
    }
    if (chrome.hidden) chrome.hidden = NO;
    if (fabs(chrome.alpha - alpha) > 0.001) chrome.alpha = alpha;
    if (!CGRectEqualToRect(chrome.bounds, bounds)) chrome.bounds = bounds;
    if (!CGPointEqualToPoint(chrome.center, center)) chrome.center = center;
    if (!CGAffineTransformEqualToTransform(chrome.transform, transform)) chrome.transform = transform;
    // 3.11.1: UIKit may rebuild a bar item's glass; its tone's recipe goes back on.
    ALBToneTouchTop(chrome);
}

/* 3.11.7: "top.e2" without the liquid bar (the three-part dock has no bar
   page to register with; 3.11.6 fell back to the feed, which the user saw go
   wrong): an empty, untouchable scroll view as big as the window, whose top
   edge the chrome is registered with. 3.11.8: 3.11.7 put it right under the
   chrome, inside Douyin's top container, and the user saw the top still
   unlike the liquid bar's. Now it sits where the liquid bar's page sits
   relative to the chrome: above all of Douyin's content, at the bottom of
   the dock's own layer at the top of the main window (ALBDockOverlayView);
   right under the chrome only when that layer is not there. Gone in any
   other case. */
static char kALB311TopEdgeKey;
FOUNDATION_EXPORT BOOL ALBDockSceneIsVideo(void);
static UIUserInterfaceStyle ALB33PageAppearanceStyle(UIWindow *window, NSInteger index);
/* 3.11.10: the light/dark the liquid bar's page has, which the top chrome
   registered with it starts from and leans to (AWEApplyAlbumMorphResolved-
   Appearance: Dark on a video feed, the page's own look elsewhere). 3.11.6-
   3.11.9's edges in the three-part dock took Douyin's Light main window and
   the top turned the light, whiter glass (user report: material wrong). */
static UIUserInterfaceStyle ALB311TopEdgeStyle(UIWindow *window) {
    if (ALBDockSceneIsVideo()) return UIUserInterfaceStyleDark;
    UIUserInterfaceStyle style = ALB33PageAppearanceStyle(window, 0);
    return style == UIUserInterfaceStyleLight ? UIUserInterfaceStyleLight : UIUserInterfaceStyleDark;
}
static UIScrollView *ALB311TopOwnEdge(UIView *chrome, BOOL want) {
    UIScrollView *proxy = objc_getAssociatedObject(chrome, &kALB311TopEdgeKey);
    UIView *parent = chrome.superview;
    UIWindow *window = chrome.window;
    if (!want || !parent || !window) {
        if (proxy.superview) [proxy removeFromSuperview];
        return nil;
    }
    if (!proxy) {
        proxy = ALBFollowMakeProxy();
        proxy.accessibilityIdentifier = @"AwemeLiquidBar.TopEdge";
        objc_setAssociatedObject(chrome, &kALB311TopEdgeKey, proxy, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    UIUserInterfaceStyle base = ALB311TopEdgeStyle(window);
    if (proxy.overrideUserInterfaceStyle != base) proxy.overrideUserInterfaceStyle = base;
    UIView *overlay = nil;
    Class overlayClass = NSClassFromString(@"ALBDockOverlayView");
    for (UIView *subview in window.subviews.reverseObjectEnumerator)
        if (overlayClass && [subview isKindOfClass:overlayClass] && !subview.hidden) { overlay = subview; break; }
    if (overlay) {
        if (proxy.superview != overlay || overlay.subviews.firstObject != proxy) [overlay insertSubview:proxy atIndex:0];
        if (!CGRectEqualToRect(proxy.frame, overlay.bounds)) proxy.frame = overlay.bounds;
        return proxy;
    }
    NSArray<UIView *> *siblings = parent.subviews;
    NSUInteger at = [siblings indexOfObjectIdenticalTo:chrome];
    if (proxy.superview != parent || at == 0 || at == NSNotFound || siblings[at - 1] != proxy)
        [parent insertSubview:proxy belowSubview:chrome];
    CGRect frame = [window convertRect:window.bounds toView:parent];
    if (!CGRectEqualToRect(proxy.frame, frame)) proxy.frame = frame;
    return proxy;
}

/* 3.11.31 "dock.followEdge" (default on; the user: the three-part dock is
   always deep-toned, and would rather it be the glass the right column and
   the panel are, without choosing a tint per page). The dock's glass the
   top chrome's way: Regular glass (no Clear tint), the light/dark of the
   top chrome's own edge (ALB311TopEdgeStyle: Dark on a video page, the
   page's own look elsewhere), and its four glass pieces (the three groups
   and the system tab bar) registered at the bottom of an edge of their own:
   an empty window-sized scroll view in the dock's layer, right under the
   dock (the top chrome's sits at the bottom of the same layer). UIKit then
   turns them light or dark by what is behind them, as it does the top
   chrome. Off: the dock's own per-page Dark / Clear (3.11.2). */
static char kALBDockEdgeKey, kALBDockEdgeRegisteredKey;
static NSUInteger gALBDockEdgeRegistered;
BOOL ALBDockHostFollowEdge(void) {
    // 3.11.32: off by default (3.11.31 on the phone: "registered 4/4, pocket
    // never appeared": UIKit gives this kind of glass, UIGlassEffect views,
    // no pocket; the dock only lost its Clear).
    // 3.11.51：“登记到底栏边缘”设置删了，固定关
    return NO;
}
UIUserInterfaceStyle ALBDockHostEdgeStyle(void) {
    UIWindow *window = AWEActiveMainWindow();
    return window ? ALB311TopEdgeStyle(window) : UIUserInterfaceStyleDark;
}
void ALBDockHostSyncEdge(UIView *dock, NSArray<UIView *> *glass) {
    if (!dock || !NSThread.isMainThread) return;
    UIView *overlay = dock.superview;
    UIScrollView *edge = objc_getAssociatedObject(dock, &kALBDockEdgeKey);
    BOOL want = ALBDockHostFollowEdge() && overlay && dock.window;
    if (!want) {
        for (UIView *view in glass) {
            if (![view isKindOfClass:UIView.class] || !objc_getAssociatedObject(view, &kALBDockEdgeRegisteredKey)) continue;
            ALB3RegisterEdge(view, nil, 4);
            objc_setAssociatedObject(view, &kALBDockEdgeRegisteredKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if (edge.superview) [edge removeFromSuperview];
        gALBDockEdgeRegistered = 0;
        return;
    }
    if (!edge) {
        edge = ALBFollowMakeProxy();
        edge.accessibilityIdentifier = @"AwemeLiquidBar.DockEdge";
        objc_setAssociatedObject(dock, &kALBDockEdgeKey, edge, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    UIUserInterfaceStyle style = ALBDockHostEdgeStyle();
    if (edge.overrideUserInterfaceStyle != style) edge.overrideUserInterfaceStyle = style;
    // Under the dock in its layer (not taking index 0 from the top chrome's edge).
    NSArray<UIView *> *siblings = overlay.subviews;
    NSUInteger edgeAt = [siblings indexOfObjectIdenticalTo:edge], dockAt = [siblings indexOfObjectIdenticalTo:dock];
    if (edge.superview != overlay || edgeAt == NSNotFound || dockAt == NSNotFound || edgeAt > dockAt)
        [overlay insertSubview:edge belowSubview:dock];
    if (!CGRectEqualToRect(edge.frame, overlay.bounds)) edge.frame = overlay.bounds;
    NSUInteger registered = 0;
    for (UIView *view in glass) {
        if (![view isKindOfClass:UIView.class]) continue;
        if (objc_getAssociatedObject(view, &kALBDockEdgeRegisteredKey) != edge) {
            ALB3RegisterEdge(view, edge, 4);
            objc_setAssociatedObject(view, &kALBDockEdgeRegisteredKey, edge, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        registered++;
    }
    gALBDockEdgeRegistered = registered;
}

/* The top chrome's registration as the top bar's refresh sees it: the
   feed's (not lifted), the bar's while the layer holds it, else none. */
static void ALB37RefreshChromeRegistration(UIView *chrome, UIView *container) {
    if (!ALB37ChromeLifted()) {
        // 3.10.8: the top bar never gets an edge of its own (on its own it
        // turned white over anything a little bright): the bar's shared one
        // ("top.e2"), else the feed's. 3.11.7: "top.e2" with no liquid bar
        // (the three-part dock): a window-sized edge of its own under it.
        UIScrollView *barScroll = ALB310TopE2() && ALB37BarWindow() ? ALB36BarScrollView() : nil;
        UIScrollView *own = ALB311TopOwnEdge(chrome, ALB310TopE2() && !barScroll && ALBDockModeActive());
        ALB3RegisterEdge(chrome, barScroll ?: own ?: ALB3CurrentFeedTable(container), 1);
        return;
    }
    ALB311TopOwnEdge(chrome, NO);
    for (ALB37ChromeRecord *record in gALB37Chromes)
        if (record.chrome == chrome && record.registeredWith) return;
    ALB3RegisterEdge(chrome, nil, 1);
}

/* 3.8: the bar's window hands touches on the lifted top chrome to it, and
   to what UIKit adds to the window for it (the top-left menu, popovers). */
static UIView *ALB37HitTest(UIWindow *window, CGPoint point, UIEvent *event) {
    ALB37GlassLayerView *layer = objc_getAssociatedObject(window, &kALB37LayerKey);
    if (!layer || layer.superview != window || layer.hidden) return nil;
    UIView *controllerView = gAWEAlbumMorphController.viewIfLoaded;
    for (UIView *subview in window.subviews.reverseObjectEnumerator) {
        if (subview == layer || subview == controllerView || subview.hidden) continue;
        NSString *name = NSStringFromClass(subview.class);
        if (![name hasPrefix:@"_UI"] && ![name hasPrefix:@"UITransition"] && ![name containsString:@"ContextMenu"] &&
            ![name containsString:@"Dimming"]) continue;
        UIView *hit = [subview hitTest:[window convertPoint:point toView:subview] withEvent:event];
        if (hit) return hit;
    }
    return [layer hitTest:[window convertPoint:point toView:layer] withEvent:event];
}

/* Unregistered, the feed's top and right edge effects are hidden by hand,
   as registering hid them ("登记不画"); what they were is put back when
   the option goes off. */
static char kALB37EdgeOriginalKey;
static void ALB37HideFeedEdges(UIScrollView *table) {
    if (!table) return;
    NSMutableDictionary *original = objc_getAssociatedObject(table, &kALB37EdgeOriginalKey);
    for (NSString *name in @[@"topEdgeEffect", @"rightEdgeEffect"]) {
        if (![table respondsToSelector:NSSelectorFromString(name)]) continue;
        @try {
            id effect = [table valueForKey:name];
            if (![effect respondsToSelector:NSSelectorFromString(@"setHidden:")]) continue;
            BOOL hidden = [[effect valueForKey:@"hidden"] boolValue];
            if (hidden) continue;
            if (!original) {
                original = [NSMutableDictionary dictionary];
                objc_setAssociatedObject(table, &kALB37EdgeOriginalKey, original, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                if (!gALB37EdgeTables) gALB37EdgeTables = [NSHashTable weakObjectsHashTable];
                [gALB37EdgeTables addObject:table];
            }
            if (!original[name]) original[name] = @NO;
            [effect setValue:@YES forKey:@"hidden"];
        } @catch (__unused NSException *exception) {}
    }
}
static void ALB37RestoreFeedEdges(void) {
    for (UIScrollView *table in gALB37EdgeTables.allObjects) {
        NSDictionary *original = objc_getAssociatedObject(table, &kALB37EdgeOriginalKey);
        for (NSString *name in original) {
            @try {
                id effect = [table valueForKey:name];
                if ([effect respondsToSelector:NSSelectorFromString(@"setHidden:")]) [effect setValue:original[name] forKey:@"hidden"];
            } @catch (__unused NSException *exception) {}
        }
        objc_setAssociatedObject(table, &kALB37EdgeOriginalKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [gALB37EdgeTables removeAllObjects];
}

static void ALB37SyncBody(void);
static void ALB37Sync(void) {
    if (!NSThread.isMainThread || gALB37Syncing) return;
    gALB37Syncing = YES;
    @try { ALB37SyncBody(); } @finally { gALB37Syncing = NO; }
}
static void ALB37SyncBody(void) {
    BOOL topLifted = ALB37ChromeLifted() || ALB310TopE2();
    if (!topLifted) return;
    CFTimeInterval now = CACurrentMediaTime();
    static CFTimeInterval edgesCheckedAt, topCheckedAt;
    // 3.10.2: the in-place top chrome on the bar's scroll view (the bar's
    // selected page can change under it).
    if (ALB310TopE2() && !ALB37ChromeLifted() && now - topCheckedAt > 0.5) {
        topCheckedAt = now;
        UIView *container = gAWEHomeFeedTopContainer;
        UIView *chrome = container ? ALB3TopChromeForContainer(container) : nil;
        if (chrome) ALB37RefreshChromeRegistration(chrome, container);
    }
    // Unregistered from the feed, its top edge effect is hidden by hand.
    if (now - edgesCheckedAt > 0.5) { edgesCheckedAt = now; ALB37HideFeedEdges(gALB3FeedTable); }
    for (ALB37ChromeRecord *record in [gALB37Chromes copy]) {
        if (!record.chrome || !record.container) { [gALB37Chromes removeObjectIdenticalTo:record]; continue; }
        ALB37SyncChrome(record, now);
    }
}

/* Right before Core Animation commits (after UIKit's own before-commit
   work): what the chrome follows has its final position for this frame. */
static void ALB37EnsureObserver(void) {
    if (gALB37Observer) return;
    gALB37Observer = CFRunLoopObserverCreateWithHandler(kCFAllocatorDefault,
        kCFRunLoopBeforeWaiting | kCFRunLoopExit, true, 1999500,
        ^(__unused CFRunLoopObserverRef observer, __unused CFRunLoopActivity activity) {
            if (gALB37Chromes.count || gALB37TopE2InPlace) ALB37Sync();
        });
    CFRunLoopAddObserver(CFRunLoopGetMain(), gALB37Observer, kCFRunLoopCommonModes);
}

/* The top chrome: called by ALB3MirrorTopChrome while lifted. */
static void ALB37TrackChrome(UIView *chrome, UIView *container) {
    if (!chrome || !container) return;
    if (!gALB37Chromes) gALB37Chromes = [NSMutableArray array];
    ALB37ChromeRecord *record = nil;
    for (ALB37ChromeRecord *candidate in gALB37Chromes) if (candidate.chrome == chrome) { record = candidate; break; }
    if (!record) {
        record = [ALB37ChromeRecord new];
        record.chrome = chrome;
        [gALB37Chromes addObject:record];
    }
    record.container = container;
    ALB37EnsureObserver();
    ALB37SyncChrome(record, CACurrentMediaTime());
}
static BOOL ALB37TracksChrome(UIView *chrome) {
    for (ALB37ChromeRecord *record in gALB37Chromes) if (record.chrome == chrome) return YES;
    return NO;
}
static void ALB37ForgetChrome(UIView *chrome) {
    for (ALB37ChromeRecord *record in [gALB37Chromes copy])
        if (record.chrome == chrome || !record.chrome) [gALB37Chromes removeObjectIdenticalTo:record];
}

/* Off: the top chrome goes back next to Douyin's top bar, the layers go,
   the feed's edge effects are what they were. */
static void ALB37ReleaseAll(void) {
    for (ALB37ChromeRecord *record in [gALB37Chromes copy]) {
        [gALB37Chromes removeObjectIdenticalTo:record];
        if (record.chrome && record.container) ALB37ChromeInPlace(record);
    }
    ALB37RestoreFeedEdges();
    for (UIView *layer in gALB37Layers.allObjects) {
        UIWindow *window = layer.window;
        [layer removeFromSuperview];
        if (window) objc_setAssociatedObject(window, &kALB37LayerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [gALB37Layers removeAllObjects];
}

/* At launch (initial value) and on every options change. */
static void ALB37NoteOptions(void) {
    static int wasTop = -1, wasTopBar = -1, wasTopE2 = -1;
    BOOL top = ALB37ChromeLifted(), topBar = ALB37ChromeBarMode();
    // Off, or moved between the two windows: everything back first (the
    // next top bar pass lifts it again into the right window).
    BOOL topMoved = top && wasTopBar >= 0 && wasTopBar != (int)topBar;
    BOOL releasedAll = (wasTop == 1 && !top) || topMoved;
    if (releasedAll) ALB37ReleaseAll();
    // Switched on, moved, or still on after everything came back: the top
    // chrome lifts now, not on the top bar's next layout.
    if (top && (wasTop == 0 || topMoved) && gAWEHomeFeedTopContainer)
        ALB3MirrorTopChrome(gAWEHomeFeedTopContainer);
    BOOL topE2 = ALB310TopE2();
    if (wasTopE2 >= 0 && wasTopE2 != (int)topE2 && !top && gAWEHomeFeedTopContainer) {
        UIView *chrome = ALB3TopChromeForContainer(gAWEHomeFeedTopContainer);
        if (chrome) ALB37RefreshChromeRegistration(chrome, gAWEHomeFeedTopContainer);
        if (!topE2) ALB37RestoreFeedEdges();
    }
    gALB37TopE2InPlace = topE2 && !top;
    if (gALB37TopE2InPlace) ALB37EnsureObserver();
    wasTopE2 = topE2 ? 1 : 0;
    wasTop = top ? 1 : 0;
    wasTopBar = topBar ? 1 : 0;
}

/* 诊断 */
static NSString *ALB37Status(void) {
    NSUInteger chromes = 0;
    for (ALB37ChromeRecord *record in gALB37Chromes) if ([record.chrome.superview isKindOfClass:ALB37GlassLayerView.class]) chromes++;
    return [NSString stringWithFormat:@"顶栏E2=%@ 顶栏放到最上层=%@ 位置=%@ 顶栏在最上层=%lu/%lu",
            ALB310TopE2() ? @"开" : @"关", ALB37Lifted() ? @"开" : @"关",
            ALB37BarMode() ? (ALB37BarWindow() ? @"底栏窗口" : @"底栏窗口(不在)") : @"主窗口",
            (unsigned long)chromes, (unsigned long)gALB37Chromes.count];
}
