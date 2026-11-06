#import "ALB25Material.h"
#import "ALB5Options.h"
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <string.h>
#import <sys/utsname.h>

// 2.5 MATERIAL3 — the single owner of every plug-in Liquid Glass surface.
//
// Bars   : the bottom AlbumMorph UITabBarController and the top category
//          group are native UIKit floating tab bars. Their glass is the
//          platter's glass: KVC "glassEffect" (iOS 26.6 probe and
//          this project's v10 on iOS 27). UIKit realizes it as the platter's
//          private _background (_UIViewGlass). Every UIView answers that key,
//          so only a view that already HAS glass is an owner.
// Buttons: 2.4.1's probe showed every button drawing the system button's own
//          private material (UIBackgroundConfiguration _material, a
//          _UIViewGlass with tint nil), so tint and dimming reached the bars
//          only. Source 1 (default) puts the bottom bar's real _background
//          glass (same Light/Dark) or the same recipe's glass into _material.
// Views  : UIVisualEffectView effect (the same UIGlassEffect recipe).
// Luma   : nothing. 2.4.1-2.4.4 clamped luminance (hooks) and later changed
//          the glass objects' own adaptivity; probe 4 showed the hooks never
//          reached this glass, and 2.4.4's change left the bar icons stuck.
//          Both are gone: iOS 26 adapts the glass and the icons natively.
// Pages  : 2.4.3 — Clear only while a video feed is on screen (Home/Friends);
//          every other page draws System Regular (bars back to UIKit's own
//          glass with its shadow and depth). Tweak.x notes the target page.

static BOOL (^ALBFeedResolver)(void);
static UIUserInterfaceStyle (^ALBAppResolver)(void);
static NSUInteger ALBGeneration = 1;
static NSHashTable<UIView *> *ALBSurfaces;
static __weak UITabBar *ALBBottomSource;
static char ALBRoleKey, ALBEffectSignatureKey, ALBInstalledEffectKey, ALBOriginalEffectKey;
static char ALBOriginalStyleKey, ALBAuditKey, ALBApplyingKey, ALBWrittenStyleKey;
static char ALBUnstableReadKey, ALBReplaceWindowKey, ALBButtonPathKey;
static char ALBOwnerOriginalKey, ALBOwnerInstalledKey, ALBOwnerSignatureKey, ALBOwnerUnstableKey;
static char ALBPlatterCacheKey, ALBViewGlassKey, ALBOriginalCornerKey;
static char ALBInstalledMaterialKey, ALB25MaterialDriftKey, ALBCarriedKey, ALBCarriedOriginalKey;
static char ALBMenuSessionKey, ALBForceApplyKey;
// The bottom bar's realized glass (main platter _background) per Light/Dark,
// see ALBCaptureBarGlass: {glass, key, recipe, clear}. One slot per style, so
// leaving the feed (Dark) for a page (Light) never invalidates the other.
static NSMutableDictionary<NSNumber *, NSDictionary *> *ALBBarGlassByStyle;
static UIUserInterfaceStyle ALBBarGlassStyle;
static NSString *ALBBarGlassState = @"unseen";
static __weak id ALBBarGlassSource;
static NSMutableDictionary<NSString *, NSNumber *> *ALBGlassOwnerClasses;
static NSString *ALBFactoryEffectClass[2];
// 3.2 (one switch, on by default since 3.3): the feed's buttons (the right
// column and the double-tap panel) draw the top chrome's kind of glass
// instead: UIKit Regular glass without tint, fixed Dark, each glass
// registered as the feed's edge element by Tweak.x ("登记不画"). A captured
// realized glass (ALB25MaterialCaptureRegisteredGlass) is used when a caller
// supplies one. Nothing else about the buttons changes.
static UIView *ALBSurfaceRoot(UITabBar *bar);
static NSArray<UIView *> *ALBCollectPlatters(UITabBar *bar, UIView *root);
static id ALBRegisteredGlass;
static NSString *ALBRegisteredGlassKey;
static BOOL ALBRegisteredFeed(BOOL feed) { return feed && ALB25MaterialRegisteredGlassEnabled(); }

#pragma mark - Preferences

static int ALBOSMajor(void) { return (int)NSProcessInfo.processInfo.operatingSystemVersion.majorVersion; }
// 3.3: the old unified material settings page is gone (the registered Dark
// glass replaced it). Every former setting is its default; values stored by
// 2.x/3.2 are never read again.
static NSInteger ALBPrefInteger(NSString *key, NSInteger fallback) { (void)key; return fallback; }
static BOOL ALBFixedBool(NSString *key, BOOL fallback) { (void)key; return fallback; }
static CGFloat ALBFixedNumber(NSString *key, CGFloat fallback, CGFloat low, CGFloat high) {
    (void)key; (void)low; (void)high; return fallback;
}
NSInteger ALB25MaterialMode(void) { return ALB25MaterialRegular; }
BOOL ALB25MaterialEnabled(void) { return YES; }
// The page the bottom bar is showing: a video feed or not. Noted by Tweak.x
// with the explicit target tab (AWEApplyAlbumMorphResolvedAppearance); the
// feed resolver only seeds it before the first note.
static BOOL ALBVideoContextValue = YES, ALBVideoContextKnown, ALBVideoContextNoted;
static BOOL ALBVideoContext(void) {
    if (!ALBVideoContextKnown && !ALBVideoContextNoted && NSThread.isMainThread && ALBFeedResolver) {
        ALBVideoContextValue = ALBFeedResolver();
        ALBVideoContextKnown = YES;
    }
    return ALBVideoContextValue;
}
// What the glass draws right now: the chosen mode, or Regular off the video feed.
static NSInteger ALBEffectiveMode(void) {
    return ALB25MaterialEffectiveMode((int)ALB25MaterialMode(), ALBFixedBool(@"glass.clearVideoOnly", YES), ALBVideoContext());
}
BOOL ALB25MaterialUsesClearStyle(void) {
    return ALB25MaterialEnabled() && ALB25MaterialModeIsClear((int)ALBEffectiveMode());
}
// Full-width, on-screen view controllers of the key window (the page the
// user sees and its containers).
static UIWindow *ALBKeyWindow(void) {
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class]) continue;
        for (UIWindow *window in ((UIWindowScene *)scene).windows) if (window.isKeyWindow) return window;
    }
    return nil;
}
static NSArray<NSString *> *ALBVisiblePageNames(void) {
    UIWindow *window = ALBKeyWindow();
    UIViewController *root = window.rootViewController;
    if (!root) return @[];
    NSMutableArray<NSString *> *names = [NSMutableArray array];
    NSMutableArray<UIViewController *> *queue = [NSMutableArray arrayWithObject:root];
    CGFloat width = CGRectGetWidth(window.bounds);
    for (NSUInteger i = 0; i < queue.count && i < 180; i++) {
        UIViewController *controller = queue[i];
        for (UIViewController *child in controller.childViewControllers) if (queue.count < 180) [queue addObject:child];
        if (controller.presentedViewController && queue.count < 180) [queue addObject:controller.presentedViewController];
        UIView *view = controller.viewIfLoaded;
        if (!view.window || view.hidden || view.alpha < .01) continue;
        CGRect visible = CGRectIntersection([view convertRect:view.bounds toView:window], window.bounds);
        if (CGRectIsNull(visible) || CGRectGetWidth(visible) < width * .78 || CGRectGetHeight(visible) < 200) continue;
        [names addObject:NSStringFromClass(controller.class)];
    }
    return names;
}
// Douyin's Home also hosts channels that play no video (2.4.3: 团购 kept
// Clear). Names seen on screen decide; the probe lists them for new ones.
static NSString *ALBNonVideoChannelName(void) {
    for (NSString *name in ALBVisiblePageNames()) {
        NSString *lower = name.lowercaseString;
        for (NSString *token in @[@"groupbuy", @"groupon", @"tuangou", @"lifeservice", @"localservice", @"locallife", @"lynx"])
            if ([lower containsString:token]) return name;
    }
    return nil;
}
static NSString *ALBLastNonVideoChannel;
void ALB25MaterialNoteVideoContext(BOOL video) {
    if (!NSThread.isMainThread) return;
    if (video) {
        ALBLastNonVideoChannel = ALBNonVideoChannelName();
        if (ALBLastNonVideoChannel) video = NO;
    }
    BOOL changed = ALBVideoContextKnown && ALBVideoContextValue != video;
    ALBVideoContextValue = video; ALBVideoContextKnown = YES; ALBVideoContextNoted = YES;
    if (!changed) return;
    static BOOL scheduled;
    if (scheduled) return;
    scheduled = YES;
    // Off the caller's appearance pass: every surface re-applies once.
    dispatch_async(dispatch_get_main_queue(), ^{ scheduled = NO; ALB25MaterialRefresh(); });
}
// iOS 26.6 exposes the same platter glass entry as iOS 27.
BOOL ALB25MaterialBarsWantClear(void) { return ALB25MaterialUsesClearStyle() && ALBOSMajor() >= 26; }
static NSInteger ALBAppearancePreference(void) { return ALBPrefInteger(@"appearance", ALBAppearanceDefault); }
// 2.5: one button path, no switch. The bar material (source 1) with the
// system configuration as its automatic fallback. Stored 2.4.x choices are
// ignored; the factory/view-glass branches remain only as fallbacks.
static NSInteger ALBButtonSource(void) { return ALBButtonSourceResolve(-1, false); }
NSUInteger ALB25MaterialGeneration(void) { return ALBGeneration; }
static NSString *ALBModeName(NSInteger mode) {
    switch (mode) {
        case ALB25MaterialClear: return @"clear-tint";
        case ALB25MaterialNagram: return @"clear-nagram";
        default: return @"regular";
    }
}
static NSString *ALBSourceName(NSInteger source) {
    return source == ALBButtonSourceBarMaterial ? @"bar-material" :
           source == ALBButtonSourceViewGlass ? @"view-glass" : @"factory";
}

#pragma mark - Appearance

void ALB25MaterialInstallResolvers(BOOL (^feed)(void), UIUserInterfaceStyle (^app)(void)) {
    ALBFeedResolver = [feed copy]; ALBAppResolver = [app copy];
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        ALBSurfaces = [NSHashTable weakObjectsHashTable];
        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        // A refresh re-applies every surface; signatures make unchanged ones no-ops.
        for (NSString *name in @[@"ALB5.OptionsChanged", UIApplicationDidBecomeActiveNotification,
                                  UIAccessibilityReduceTransparencyStatusDidChangeNotification])
            [center addObserverForName:name object:nil queue:NSOperationQueue.mainQueue
                usingBlock:^(__unused NSNotification *note) { ALB25MaterialRefresh(); }];
    });
}

UIUserInterfaceStyle ALB25MaterialStyle(UIView *view, BOOL feedEligible) {
    if (ALBRegisteredFeed(feedEligible)) return UIUserInterfaceStyleDark;
    UIScreen *screen = view.window.screen ?: UIScreen.mainScreen;
    UIUserInterfaceStyle systemStyle = screen.traitCollection.userInterfaceStyle;
    if (systemStyle == UIUserInterfaceStyleUnspecified) systemStyle = UIUserInterfaceStyleLight;
    UIUserInterfaceStyle appStyle = ALBAppResolver ? ALBAppResolver() : view.window.traitCollection.userInterfaceStyle;
    return (UIUserInterfaceStyle)ALB25MaterialResolveStyle((int)ALBAppearancePreference(), (int)systemStyle,
        (int)appStyle, feedEligible && ALBFeedResolver && ALBFeedResolver(), ALBFixedBool(@"glass.feedDark", YES));
}
#pragma mark - Effect recipe

// Clear: black tint in Dark only. Nagram: never a tint. Regular:
// the optional user tint; "follow appearance" uses the iOS 27 lab's mapping
// (Light = white, Dark = black).
static UIColor *ALB25MaterialTintForStyle(UIUserInterfaceStyle style) {
    if (!ALB25MaterialEnabled()) return nil;
    NSInteger mode = ALBEffectiveMode();
    if (mode == ALB25MaterialClear) {
        double alpha = ALB25MaterialClearTintAlpha((int)mode, style == UIUserInterfaceStyleDark ? 2 : 1,
                                                 ALBFixedNumber(@"glass.clearDarkTint", .30, 0, .6));
        return alpha > .001 ? [UIColor colorWithWhite:0 alpha:alpha] : nil;
    }
    if (mode != ALB25MaterialRegular) return nil;
    NSInteger choice = ALBPrefInteger(@"tint", 0);
    if (choice <= 0 || choice > 4) return nil;
    CGFloat alpha = ALBFixedNumber(@"glass.tintStrength", .12, 0, .6);
    if (alpha <= .001) return nil;
    BOOL dark = style == UIUserInterfaceStyleDark;
    UIColor *color = choice == 1 ? (dark ? UIColor.blackColor : UIColor.whiteColor) :
                     choice == 2 ? UIColor.whiteColor : choice == 3 ? UIColor.blackColor :
                     [UIColor colorWithRed:.12 green:.55 blue:1 alpha:1];
    return [color colorWithAlphaComponent:alpha];
}

// Runtime dispatch, identical to the verified AwemeGlassLab bridge.
static UIVisualEffect *ALBMakeGlass(BOOL clear, UIColor *tint, BOOL interactive) {
    Class cls = NSClassFromString(@"UIGlassEffect");
    SEL maker = NSSelectorFromString(@"effectWithStyle:");
    if (cls && [cls respondsToSelector:maker]) {
        UIVisualEffect *effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(cls, maker, clear ? 1 : 0);
        SEL setTint = NSSelectorFromString(@"setTintColor:"), setInteractive = NSSelectorFromString(@"setInteractive:");
        if ([effect respondsToSelector:setTint]) ((void (*)(id, SEL, id))objc_msgSend)(effect, setTint, tint);
        if ([effect respondsToSelector:setInteractive]) ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, setInteractive, interactive);
        return effect;
    }
    return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
}
static UIVisualEffect *ALBEffectForStyle(UIUserInterfaceStyle style, BOOL interactive) {
    return ALBMakeGlass(ALB25MaterialUsesClearStyle(), ALB25MaterialTintForStyle(style), interactive);
}
UIVisualEffect *ALB25MaterialMakeEffect(BOOL interactive) {
    return ALBEffectForStyle(UITraitCollection.currentTraitCollection.userInterfaceStyle, interactive);
}
static BOOL ALBEffectInteractive(id effect, BOOL fallback) {
    if (!effect) return fallback;
    @try { id value = [effect valueForKey:@"interactive"]; if ([value respondsToSelector:@selector(boolValue)]) return [value boolValue]; }
    @catch (__unused NSException *exception) {}
    return fallback;
}

#pragma mark - Surfaces

static void ALBRegister(UIView *view, NSString *role) {
    if (!ALBSurfaces) ALBSurfaces = [NSHashTable weakObjectsHashTable];
    [ALBSurfaces addObject:view];
    objc_setAssociatedObject(view, &ALBRoleKey, role, OBJC_ASSOCIATION_COPY_NONATOMIC);
}
// Enabled: write the resolved style and remember both it and the original.
// Disabled: restore only if the override is still the one written here, once;
// any other owner's later value is left alone (no per-layout tug of war).
static void ALBApplyStyle(UIView *view, BOOL feed) {
    NSNumber *written = objc_getAssociatedObject(view, &ALBWrittenStyleKey);
    if (!ALB25MaterialEnabled() && !ALBRegisteredFeed(feed)) {
        if (!written) return;
        if (view.overrideUserInterfaceStyle == (UIUserInterfaceStyle)written.integerValue)
            view.overrideUserInterfaceStyle = (UIUserInterfaceStyle)[objc_getAssociatedObject(view, &ALBOriginalStyleKey) integerValue];
        objc_setAssociatedObject(view, &ALBWrittenStyleKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    if (!written && !objc_getAssociatedObject(view, &ALBOriginalStyleKey))
        objc_setAssociatedObject(view, &ALBOriginalStyleKey, @(view.overrideUserInterfaceStyle), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    UIUserInterfaceStyle style = ALB25MaterialStyle(view, feed);
    if (view.overrideUserInterfaceStyle != style) view.overrideUserInterfaceStyle = style;
    if (!written || written.integerValue != style)
        objc_setAssociatedObject(view, &ALBWrittenStyleKey, @(style), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
// The recipe itself, not a refresh counter: an app activation or an options
// broadcast that changes nothing must not rebuild a visible glass.
static NSString *ALBRecipe(UIUserInterfaceStyle style) {
    return [NSString stringWithFormat:@"%ld/%ld/%d/%d/%@", (long)ALBEffectiveMode(), (long)style,
        ALB25MaterialEnabled(), UIAccessibilityIsReduceTransparencyEnabled(), ALB25MaterialTintForStyle(style) ?: @"-"];
}
static UITraitCollection *ALBTraitsForStyle(UIView *view, UIUserInterfaceStyle style) {
    UITraitCollection *override = [UITraitCollection traitCollectionWithUserInterfaceStyle:style];
    return [UITraitCollection traitCollectionWithTraitsFromCollections:@[view.traitCollection, override]];
}
static void ALBSetAudit(UIView *view, NSString *path, NSUInteger count, NSDictionary *extra) {
    NSMutableDictionary *audit = [@{@"path":path ?: @"?", @"changed":@(count),
        @"mode":ALBModeName(ALBEffectiveMode()), @"style":@(view.overrideUserInterfaceStyle)} mutableCopy];
    if (extra) [audit addEntriesFromDictionary:extra];
    objc_setAssociatedObject(view, &ALBAuditKey, audit, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
// Getter stability: a copying getter would make identity checks loop.
static BOOL ALBReadIsStable(id (^read)(void)) { id first = read(), second = read(); return first == second; }
// At most three re-assert passes a second per view, so a surface that UIKit
// (or a caller) keeps rebuilding can never turn into a layout loop.
static BOOL ALBAllowReplacementPass(UIView *view) {
    CFTimeInterval now = CACurrentMediaTime();
    NSArray *window = objc_getAssociatedObject(view, &ALBReplaceWindowKey);
    double start = window.count == 2 ? [window[0] doubleValue] : 0;
    NSInteger count = window.count == 2 ? [window[1] integerValue] : 0;
    if (now - start > 1.0) { start = now; count = 0; }
    if (count >= 3) return NO;
    objc_setAssociatedObject(view, &ALBReplaceWindowKey, @[@(start), @(count + 1)], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return YES;
}
static id ALBPerform(id object, NSString *selectorName) {
    SEL selector = NSSelectorFromString(selectorName);
    Method method = object ? class_getInstanceMethod(object_getClass(object), selector) : NULL;
    if (!method || method_getNumberOfArguments(method) != 2) return nil;
    char type[8] = {0};
    method_getReturnType(method, type, sizeof(type));
    // Only an object return may be retained by ARC.
    if (type[0] != '@') return nil;
    return ((id (*)(id, SEL))objc_msgSend)(object, selector);
}

#pragma mark - View-level glass, the same path as the platters

static id ALBViewGlass(UIView *view) {
    @try { return ALBPerform(view, @"_glassEffect"); } @catch (__unused NSException *exception) { return nil; }
}
static BOOL ALBSetViewGlass(UIView *view, UIVisualEffect *effect) {
    SEL selector = NSSelectorFromString(@"_setGlassEffect:");
    if (![view respondsToSelector:selector]) return NO;
    // A private setter on a UIButton: an assertion here must fall back, not
    // crash on every launch with the preference saved.
    @try { ((void (*)(id, SEL, id))objc_msgSend)(view, selector, effect); }
    @catch (__unused NSException *exception) { return NO; }
    return YES;
}
static void ALBSetCapsuleCorners(UIView *view, BOOL capsule) {
    if (@available(iOS 26.0, *)) {
        if (capsule) {
            if (!objc_getAssociatedObject(view, &ALBOriginalCornerKey))
                objc_setAssociatedObject(view, &ALBOriginalCornerKey, view.cornerConfiguration ?: (id)NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            view.cornerConfiguration = [UICornerConfiguration capsuleConfiguration];
        } else {
            id original = objc_getAssociatedObject(view, &ALBOriginalCornerKey);
            if ([original isKindOfClass:UICornerConfiguration.class]) view.cornerConfiguration = original;
            objc_setAssociatedObject(view, &ALBOriginalCornerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
    }
}
// Remove view glass this file added (button source changed or disabled).
static void ALBClearViewGlass(UIView *view) {
    if (![objc_getAssociatedObject(view, &ALBViewGlassKey) boolValue]) return;
    ALBSetViewGlass(view, nil);
    ALBSetCapsuleCorners(view, NO);
    objc_setAssociatedObject(view, &ALBViewGlassKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

#pragma mark - Bar glass, shared with the buttons

// The glass value without object addresses: equal keys draw the same glass.
static NSString *ALB25MaterialKey(id material) {
    if (!material) return nil;
    static NSRegularExpression *addresses;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ addresses = [NSRegularExpression regularExpressionWithPattern:@"0x[0-9a-fA-F]+" options:0 error:nil]; });
    NSString *text = [material description] ?: @"";
    return [addresses stringByReplacingMatchesInString:text options:0 range:NSMakeRange(0, text.length) withTemplate:@"0x"];
}
// What the glass looks like: provider configuration and tint. UIKit may
// re-derive an equal material (new id/flags); only these two matter here.
static NSString *ALB25MaterialEssence(id material) {
    NSString *key = ALB25MaterialKey(material);
    if (!key) return nil;
    static NSRegularExpression *configuration, *tint;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        configuration = [NSRegularExpression regularExpressionWithPattern:@"Configuration\\((\\w+)\\)" options:0 error:nil];
        tint = [NSRegularExpression regularExpressionWithPattern:@"tint: (.*?), backdropGroupName" options:0 error:nil];
    });
    NSTextCheckingResult *c = [configuration firstMatchInString:key options:0 range:NSMakeRange(0, key.length)];
    NSTextCheckingResult *t = [tint firstMatchInString:key options:0 range:NSMakeRange(0, key.length)];
    if (!c || !t) return key;
    return [NSString stringWithFormat:@"%@|%@", [key substringWithRange:[c rangeAtIndex:1]], [key substringWithRange:[t rangeAtIndex:1]]];
}
// 3.6: a _UIViewGlassGroup (the bars' container, 3.5 诊断) is not a glass
// one can draw: 3.5 could copy the top chrome's group into the buttons.
static BOOL ALBIsViewGlass(id material) {
    if (!material) return NO;
    NSString *name = NSStringFromClass([material class]);
    return [name containsString:@"ViewGlass"] && ![name containsString:@"Group"];
}
// UIView's private realized material. KVC in a guard: the 2.4.1 probe read it
// this way on platters (a _UIViewGlass) and on buttons (nil).
static id ALBRealizedGlass(UIView *view) {
    id value = nil;
    @try { value = [view valueForKey:@"_background"]; } @catch (__unused NSException *exception) { value = nil; }
    return ALBIsViewGlass(value) ? value : nil;
}
static id ALBGlassCopy(id glass) {
    if (!glass || ![glass respondsToSelector:@selector(copyWithZone:)]) return glass;
    @try { return [glass copy] ?: glass; } @catch (__unused NSException *exception) { return glass; }
}
// The recipe's glass, as UIGlassEffect hands it to UIKit (tint included).
static id ALBRecipeGlass(UIUserInterfaceStyle style, BOOL clear) {
    UIVisualEffect *effect = ALBMakeGlass(clear, ALB25MaterialTintForStyle(style), YES);
    id glass = nil;
    @try { glass = [effect valueForKey:@"glass"]; } @catch (__unused NSException *exception) { glass = nil; }
    return ALBIsViewGlass(glass) ? glass : nil;
}
// The captured native bar glass for this Light/Dark, only under the same
// recipe (mode, Light/Dark, tint) it was captured with.
static NSDictionary *ALBBarGlassEntry(UIUserInterfaceStyle style, BOOL clear) {
    NSDictionary *entry = ALBBarGlassByStyle[@(style)];
    if (!entry || !ALB25MaterialEnabled() || [entry[@"clear"] boolValue] != clear) return nil;
    return [entry[@"recipe"] isEqualToString:ALBRecipe(style)] ? entry : nil;
}
static NSString *ALBBarGlassKeyFor(UIUserInterfaceStyle style) { return ALBBarGlassByStyle[@(style)][@"key"]; }
// What the buttons draw:
//  - the bar keeps UIKit's own glass (System mode, no tint): a copy of that
//    native glass in the same Light/Dark, so the buttons follow whatever
//    UIKit gives the bar;
//  - the plug-in writes the bar (Clear, tinted Clear, Regular tint): the
//    same recipe's _UIViewGlass, i.e. exactly what is written into the bar.
static id ALBButtonGlass(UIUserInterfaceStyle style, BOOL clear, NSString **origin) {
    NSDictionary *entry = ALBBarGlassEntry(style, clear);
    if (entry) { if (origin) *origin = @"bar"; return ALBGlassCopy(entry[@"glass"]); }
    id glass = ALBRecipeGlass(style, clear);
    if (origin) *origin = glass ? @"recipe" : @"none";
    return glass;
}

// Re-apply the on-screen bar-material buttons once the native bar glass they
// copy changed. Their signatures carry the glass, so only those rebuild; no
// generation bump (callers' own caches stay valid).
static void ALBRefreshBarMaterialButtons(void) {
    static BOOL scheduled;
    if (scheduled) return;
    scheduled = YES;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.12 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        scheduled = NO;
        for (UIView *view in ALBSurfaces.allObjects) {
            NSString *role = objc_getAssociatedObject(view, &ALBRoleKey);
            if (![role hasPrefix:@"button"] || !view.window) continue;
            ALB25MaterialApplyButton((UIButton *)view, [role hasSuffix:@"feed"]);
        }
    });
}
static void ALB25MaterialApplyBarsAgainSoon(void) {
    static NSUInteger attempts;
    static CFTimeInterval windowStart;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - windowStart > 2.0) { windowStart = now; attempts = 0; }
    if (attempts >= 6) return;
    attempts++;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.08 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        for (UIView *view in ALBSurfaces.allObjects) {
            NSString *role = objc_getAssociatedObject(view, &ALBRoleKey);
            if ([role hasPrefix:@"bar"] && view.window) ALB25MaterialApplyBar((UITabBar *)view, [role hasSuffix:@"bottom"]);
        }
    });
}
// Called at the end of each bottom-bar pass while the bar keeps UIKit's own
// glass. A pass that just restored an owner is skipped (UIKit realizes it on
// its next layout); the native glass is never a Clear configuration.
static void ALBCaptureBarGlass(NSArray<UIView *> *platters, UIUserInterfaceStyle style, BOOL changedThisPass) {
    if (changedThisPass) { ALBBarGlassState = @"pending-restore"; ALB25MaterialApplyBarsAgainSoon(); return; }
    id glass = nil; UIView *source = nil; CGFloat area = 0;
    for (UIView *platter in platters) {
        if (platter.hidden || platter.alpha < .01 || !platter.window) continue;
        CGFloat size = CGRectGetWidth(platter.bounds) * CGRectGetHeight(platter.bounds);
        if (size <= area) continue;
        id realized = ALBRealizedGlass(platter);
        if (realized) { glass = realized; source = platter; area = size; }
    }
    if (!glass) { ALBBarGlassState = platters.count ? @"no-realized-glass" : @"no-platter"; return; }
    NSString *recipe = ALBRecipe(style);
    NSDictionary *previous = ALBBarGlassByStyle[@(style)];
    // The same realized object under the same recipe, read less than a
    // second ago: nothing to re-read (UIKit may still mutate it in place, so
    // the shortcut expires).
    CFTimeInterval now = CACurrentMediaTime();
    static CFTimeInterval lastRead;
    if (glass == ALBBarGlassSource && ALBBarGlassStyle == style && [previous[@"recipe"] isEqualToString:recipe] &&
        now - lastRead < 1.0) return;
    lastRead = now;
    NSString *essence = ALB25MaterialEssence(glass);
    // A Clear configuration read back is the plug-in's own only while it
    // writes Clear; 3.3+ never does (fixed Regular), so a native bar glass
    // that is Clear (the system's 透明 setting) is read like any other.
    if ([essence hasPrefix:@"clear|"] && ALB25MaterialUsesClearStyle()) {
        ALBBarGlassState = @"pending-native";
        ALB25MaterialApplyBarsAgainSoon();
        return;
    }
    ALBBarGlassState = @"native";
    ALBBarGlassSource = glass;
    ALBBarGlassStyle = style;
    if ([essence isEqualToString:previous[@"key"]] && [recipe isEqualToString:previous[@"recipe"]] &&
        ![previous[@"clear"] boolValue]) return;
    if (!ALBBarGlassByStyle) ALBBarGlassByStyle = [NSMutableDictionary dictionary];
    ALBBarGlassByStyle[@(style)] = @{@"glass":ALBGlassCopy(glass), @"key":essence ?: @"", @"recipe":recipe, @"clear":@NO,
                                     @"platter":NSStringFromClass(source.class)};
    if (ALBButtonSource() == ALBButtonSourceBarMaterial) ALBRefreshBarMaterialButtons();
}

#pragma mark - Buttons

static UIButtonConfiguration *ALBFactoryButtonConfiguration(BOOL clear) {
    SEL maker = NSSelectorFromString(clear ? @"clearGlassButtonConfiguration" : @"glassButtonConfiguration");
    Class cls = UIButtonConfiguration.class;
    return [cls respondsToSelector:maker] ? ((id (*)(id, SEL))objc_msgSend)(cls, maker) :
        [UIButtonConfiguration plainButtonConfiguration];
}
// Does this configuration draw glass of its own (private material or effect)?
static BOOL ALBConfigurationHasGlass(UIButtonConfiguration *configuration) {
    UIBackgroundConfiguration *background = configuration.background;
    return background.visualEffect != nil || ALBPerform(background, @"_material") != nil;
}
// The base for a source. View glass uses a plain configuration: the glass is
// on the button view, exactly like the bar's platter. The bar material keeps
// the system glass configuration (press, capsule, foreground) and replaces
// only its material.
static UIButtonConfiguration *ALBBaseButtonConfiguration(BOOL clear, NSInteger source) {
    if (source == ALBButtonSourceViewGlass) return [UIButtonConfiguration plainButtonConfiguration];
    return ALBFactoryButtonConfiguration(clear);
}
// The glass button background's private material, the same slot the system
// configuration fills with its own glass. Checked on the copy first: the
// configuration is only changed when the material really landed, so a
// failure leaves the untouched system configuration (never glassless).
static BOOL ALBInstallMaterial(UIButtonConfiguration *configuration, id glass) {
    UIBackgroundConfiguration *background = [configuration.background copy];
    SEL setMaterial = NSSelectorFromString(@"_setMaterial:");
    if (!glass || ![background respondsToSelector:setMaterial]) return NO;
    // Never an effect and a material together (a factory that carries one).
    if (background.visualEffect) background.visualEffect = nil;
    @try { ((void (*)(id, SEL, id))objc_msgSend)(background, setMaterial, glass); }
    @catch (__unused NSException *exception) { return NO; }
    id landed = ALBPerform(background, @"_material");
    if (!landed || (landed != glass && ![ALB25MaterialEssence(landed) isEqualToString:ALB25MaterialEssence(glass)])) return NO;
    configuration.background = background;
    configuration.baseBackgroundColor = nil;
    return YES;
}
static NSString *ALBInstallButtonEffect(UIButtonConfiguration *configuration, BOOL clear, UIUserInterfaceStyle style,
                                        NSInteger source) {
    if (source == ALBButtonSourceViewGlass) {
        UIBackgroundConfiguration *background = [configuration.background copy];
        background.visualEffect = nil; background.backgroundColor = UIColor.clearColor;
        configuration.background = background; configuration.baseBackgroundColor = nil;
        return @"view-glass";
    }
    if (source == ALBButtonSourceBarMaterial) {
        NSString *origin = nil;
        id glass = ALBButtonGlass(style, clear, &origin);
        // Not installable: the untouched system configuration (never glassless).
        if (!glass || !ALBInstallMaterial(configuration, glass)) return @"bar-material-unavailable-factory";
        return [@"bar-material-" stringByAppendingString:origin];
    }
    UIVisualEffect *factory = configuration.background.visualEffect;
    BOOL factoryGlass = [NSStringFromClass(factory.class) containsString:@"GlassEffect"];
    ALBFactoryEffectClass[clear ? 1 : 0] = factory ? NSStringFromClass(factory.class) : @"private-material";
    if (!factoryGlass) return @"factory-private-glass";
    // A factory that carries a UIGlassEffect (another OS release): the shared
    // recipe replaces it, the private material is cleared (never both).
    UIBackgroundConfiguration *background = [configuration.background copy];
    SEL setMaterial = NSSelectorFromString(@"_setMaterial:");
    if ([background respondsToSelector:setMaterial]) ((void (*)(id, SEL, id))objc_msgSend)(background, setMaterial, nil);
    background.visualEffect = ALBEffectForStyle(style, YES);
    background.backgroundColor = UIColor.clearColor;
    configuration.background = background;
    configuration.baseBackgroundColor = nil;
    return @"factory-glass-replaced";
}
// 3.5 "right.glassSource": which glass object the registered buttons draw.
//  0 (default) the bottom bar's own realized Dark platter glass, copied as
//    2.5.3 copied it: the bars' glass follows the system's transparency
//    setting (透明/着色), UIKit's Regular recipe does not, and 3.2-3.4 drew
//    the recipe, the visible difference to the bar;
//  1 the top chrome's own realized bar-item glass;
//  2 UIKit's Regular recipe (3.2-3.4).
// A source not read yet falls back to the next one down the list.
NSInteger ALB25MaterialRegisteredGlassSource(void) {
    return (NSInteger)lround(ALB5Number(@"right.glassSource", 0, 0, 2));
}
static id ALBRegisteredSourceGlass(NSString **key, NSString **origin) {
    NSInteger source = ALB25MaterialRegisteredGlassSource();
    NSDictionary *bottom = source == 0 ? ALBBarGlassByStyle[@(UIUserInterfaceStyleDark)] : nil;
    if (bottom[@"glass"]) {
        if (key) *key = [@"b:" stringByAppendingString:bottom[@"key"] ?: @""];
        if (origin) *origin = @"bottom";
        return bottom[@"glass"];
    }
    if (source <= 1 && ALBRegisteredGlass) {
        if (key) *key = [@"t:" stringByAppendingString:ALBRegisteredGlassKey ?: @""];
        if (origin) *origin = @"top";
        return ALBRegisteredGlass;
    }
    if (key) *key = @"recipe";
    if (origin) *origin = @"recipe";
    return nil;
}
// The chosen glass, or (none read yet / Regular chosen) plain Regular glass
// without the user's tint, as the top chrome's items draw it.
static NSString *ALBInstallRegisteredEffect(UIButtonConfiguration *configuration) {
    NSString *origin = nil;
    id glass = ALBGlassCopy(ALBRegisteredSourceGlass(NULL, &origin));
    if (!glass) {
        UIVisualEffect *effect = ALBMakeGlass(NO, nil, YES);
        @try { glass = [effect valueForKey:@"glass"]; } @catch (__unused NSException *exception) { glass = nil; }
        if (!ALBIsViewGlass(glass)) glass = nil;
        origin = @"recipe";
    }
    if (!glass || !ALBInstallMaterial(configuration, glass)) return @"bar-material-registered-unavailable-factory";
    return [@"bar-material-registered-" stringByAppendingString:origin];
}
BOOL ALB25MaterialRegisteredGlassEnabled(void) { return ALB5Bool(@"right.registeredGlass", YES); }
// 3.7 "glass.lifted" (on by default): the feed's glass is a plain
// UIGlassEffect (Regular, interactive, no tint) on the view itself, never
// registered; Tweak.x puts
// it at the top of the window (ALB37GlassLayer.h).
static char ALBKeepsTreeGlassKey, ALBGlasslessKey;
// 3.9: the lifted recipe: UIGlassEffect Clear (style 1),
// interactive, black 28 % tint ("glass.clear", on by default; Clear ignores
// overrideUserInterfaceStyle and only a tint darkens it), or Regular without
// tint (darkened by the Dark override).
/* 玻璃透明度（0–1）：0＝系统自己的玻璃；大于 0＝Clear 玻璃＋黑色染色 0.45×(1−值)，越大越透。顶栏（top.glassAlpha）和三段式底栏
   （dock.glassAlpha，视频页）各一个；右侧按钮、双击面板在 iOS 26 用的是底栏自己的玻璃，不受它们影响；展开菜单不跟。
   没存过时按 3.11.48 的统一滑块（glass.alpha）或更早的开关（glass.clear / glass.motionClear）和黑色染色（glass.darkTint）换算一次存下。 */
static CGFloat ALB25AlphaFor(NSString *name, BOOL legacyOn) {
    NSString *key = ALB5Key(name);
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:key];
    if ([stored isKindOfClass:NSNumber.class]) return MIN(MAX([stored doubleValue], 0.0), 1.0);
    CGFloat value = 0;
    id shared = [NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"glass.alpha")];
    if ([shared isKindOfClass:NSNumber.class]) {
        value = MIN(MAX([shared doubleValue], 0.0), 1.0);
    } else if (legacyOn) {
        CGFloat tint = ALB5Number(@"glass.darkTint", 0.28, 0.0, 0.6);
        value = MIN(MAX(1.0 - tint / 0.45, 0.05), 1.0);
    }
    value = round(value * 100.0) / 100.0;
    [NSUserDefaults.standardUserDefaults setDouble:value forKey:key];
    return value;
}
/* 3.11.51 用户：“顶栏的玻璃已经是栏按钮了，它也不需要去调整浓度”“整个三段式底栏玻璃就一个深色色调需要自定义，其余的都走默认”。
   顶栏不再有透明度（0＝系统自己的栏按钮玻璃）；三段式底栏只剩“深色色调”（dock.darkTint，0–0.6）：视频页固定 Clear＋这个黑色染色。
   没存过时按 3.11.49 的玻璃透明度（dock.glassAlpha）换算一次存下：0.45×(1−值)，透明度为 0（系统玻璃）时取原来的 0.28。 */
static CGFloat ALB25TintForAlpha(CGFloat t) { return t > 0.005 ? 0.45 * (1.0 - t) : 0.28; }
CGFloat ALB25MaterialTopAlpha(void) { return 0; }
static CGFloat ALB25MaterialDockTint(void) {
    NSString *key = ALB5Key(@"dock.darkTint");
    id stored = [NSUserDefaults.standardUserDefaults objectForKey:key];
    if ([stored isKindOfClass:NSNumber.class]) return MIN(MAX([stored doubleValue], 0.0), 0.6);
    CGFloat tint = ALB25TintForAlpha(ALB25AlphaFor(@"dock.glassAlpha", ALB5Bool(@"glass.motionClear", NO)));
    tint = round(MIN(MAX(tint, 0.0), 0.6) * 100.0) / 100.0;
    [NSUserDefaults.standardUserDefaults setDouble:tint forKey:key];
    return tint;
}
// 诊断里还按“透明度”显示的地方：由深色色调反推
CGFloat ALB25MaterialDockAlpha(void) { return MIN(MAX(1.0 - ALB25MaterialDockTint() / 0.45, 0.05), 1.0); }
CGFloat ALB25MaterialGlassAlpha(void) { return ALB25MaterialDockAlpha(); }
CGFloat ALB25MaterialTopDarkTint(void) { return ALB25TintForAlpha(ALB25MaterialTopAlpha()); }
BOOL ALB25MaterialLiftedClear(void) { return ALB25MaterialTopAlpha() > 0.005; }
UIVisualEffect *ALB25MaterialLiftedEffect(BOOL interactive) {
    BOOL clear = ALB25MaterialLiftedClear();
    return ALBMakeGlass(clear, clear ? [UIColor colorWithWhite:0 alpha:ALB25MaterialTopDarkTint()] : nil, interactive);
}
// 3.11: the right column's and the double-tap panel's glass, drawn the way
// the collapsing dock draws its video glass: one plain UIGlassEffect on the
// button itself, never registered with a scroll view, light/dark from the
// button's own override (Dark). Regular is the system glass button's own
// glass, untouched; "glass.motionClear" is Clear with a 28 % black tint (the
// dock's "透明玻璃"), put into the same glass button configuration.
BOOL ALB25MaterialMotionClear(void) { return YES; }   // 3.11.51：三段式底栏视频页固定 Clear＋深色色调
// 3.11.1: the Clear recipe's black tint ("glass.darkTint", 28 % by default,
// the dock's original; the top chrome's "glass.clear" and the dock's Clear).
// The white tint of 3.11.1's light tone is fixed at 12 % (3.11.19: the
// plug-in no longer turns a glass light itself; UIKit does, on its edge).
CGFloat ALB25MaterialDarkTint(void) { return ALB25MaterialDockTint(); }
CGFloat ALB25MaterialLightTint(void) { return 0.12; }
UIColor *ALB25MaterialToneTint(BOOL light) {
    return light ? [UIColor colorWithWhite:1 alpha:ALB25MaterialLightTint()]
                 : [UIColor colorWithWhite:0 alpha:ALB25MaterialDarkTint()];
}
UIVisualEffect *ALB25MaterialMotionEffectTone(BOOL interactive, BOOL light) {
    BOOL clear = ALB25MaterialMotionClear();
    return ALBMakeGlass(clear, clear ? ALB25MaterialToneTint(light) : nil, interactive);
}
UIVisualEffect *ALB25MaterialMotionEffect(BOOL interactive) { return ALB25MaterialMotionEffectTone(interactive, NO); }
/* 3.11.4-3.11.19: every glass (the right column, the double-tap panel, the
   status toast) is the bottom bar's own glass on iOS 26 (no option since
   3.11.19: the way the top chrome and the bottom bar work). */
BOOL ALB25MaterialFollowBar(void) {
    return ALBOSMajor() >= 26 && NSClassFromString(@"UIGlassEffect");
}
/* 3.11.4: the glass "glass.followBar" draws: always the bottom bar's own
   realized glass (Dark, else Light read), whatever "right.glassSource" says
   (3.11.4's first build went through that option, which the user had at 2 =
   UIKit's Regular recipe: smoothness 0, not the bar's 6, and not following
   the system's 透明／着色); not read yet: the top chrome's, then the recipe. */
static id ALBFollowSourceGlass(BOOL light, NSString **key) {
    // The plug-in turning a glass light: the bar's light reading first.
    NSArray<NSNumber *> *order = light ? @[@(UIUserInterfaceStyleLight), @(UIUserInterfaceStyleDark)]
                                       : @[@(UIUserInterfaceStyleDark), @(UIUserInterfaceStyleLight)];
    for (NSNumber *style in order) {
        NSDictionary *entry = ALBBarGlassByStyle[style];
        if (entry[@"glass"]) { if (key) *key = [@"b:" stringByAppendingString:entry[@"key"] ?: @""]; return entry[@"glass"]; }
    }
    if (ALBRegisteredGlass) { if (key) *key = [@"t:" stringByAppendingString:ALBRegisteredGlassKey ?: @""]; return ALBRegisteredGlass; }
    if (key) *key = @"recipe";
    return nil;
}
/* 3.11.9: the same glass as a visual effect (the status toast). */
UIVisualEffect *ALB25MaterialFollowEffect(void) {
    Class G = NSClassFromString(@"UIGlassEffect");
    SEL make = NSSelectorFromString(@"effectWithGlass:");
    id glass = ALBGlassCopy(ALBFollowSourceGlass(NO, NULL));
    if (G && glass && [G respondsToSelector:make]) {
        id effect = nil;
        @try { effect = ((id (*)(id, SEL, id))objc_msgSend)(G, make, glass); } @catch (__unused NSException *exception) { effect = nil; }
        if ([effect isKindOfClass:UIVisualEffect.class]) return effect;
    }
    return ALBMakeGlass(NO, nil, NO);
}
NSString *ALB25MaterialFollowGlassKey(void) {
    NSString *key = nil;
    ALBFollowSourceGlass(NO, &key);
    return key ?: @"recipe";
}
UIButtonConfiguration *ALB25MaterialMotionButtonConfigurationTone(BOOL light) {
    UIButtonConfiguration *configuration = ALBFactoryButtonConfiguration(NO);
    // 3.11.4: the bar's own glass (3.10's E / E2 face: ALBInstallRegisteredEffect,
    // "right.glassSource" 0 = the bottom bar's realized platter glass; not read
    // yet: the top chrome's, then UIKit's Regular). Its light/dark comes from
    // UIKit through the glass's edge, never from a tone.
    if (ALB25MaterialFollowBar()) {
        id glass = ALBGlassCopy(ALBFollowSourceGlass(light, NULL));
        if (!glass) {
            UIVisualEffect *effect = ALBMakeGlass(NO, nil, YES);
            @try { glass = [effect valueForKey:@"glass"]; } @catch (__unused NSException *exception) { glass = nil; }
            if (!ALBIsViewGlass(glass)) glass = nil;
        }
        if (glass) ALBInstallMaterial(configuration, glass);
        return configuration;
    }
    if (!ALB25MaterialMotionClear()) return configuration;
    id glass = nil;
    @try { glass = [ALB25MaterialMotionEffectTone(YES, light) valueForKey:@"glass"]; } @catch (__unused NSException *exception) { glass = nil; }
    // Not installable: the factory Clear glass (never glassless).
    if (!ALBIsViewGlass(glass) || !ALBInstallMaterial(configuration, glass)) return ALBFactoryButtonConfiguration(YES);
    return configuration;
}
UIButtonConfiguration *ALB25MaterialMotionButtonConfiguration(void) { return ALB25MaterialMotionButtonConfigurationTone(NO); }

// 3.11.1: UIKit glass that the plug-in does not build itself (the top
// chrome's bar items and category group, a context menu's platters) gets
// the Clear recipe in the given tone through its owners' KVC "glassEffect"
// (the platter path above); `clear` NO puts UIKit's own glass back. Only a
// view that already has glass is an owner; the same recipe is never written
// twice, a re-asserted one at most three times a second.
static char ALBToneOwnerKey;
static BOOL ALBReadGlassEffect(UIView *view, id *value);
static void ALBForgetGlassOwner(UIView *owner);
static BOOL ALBNameIsLens(NSString *name);
NSString *ALB25MaterialNonVideoChannel(void) { return ALBNonVideoChannelName(); }
static char ALBToneButtonOriginalKey, ALBToneButtonEssenceKey;
NSString *ALB25MaterialApplyTone(UIView *root, BOOL clear, BOOL light, CGFloat minTint, NSUInteger budget) {
    if (!root || !NSThread.isMainThread) return @"无";
    // 只用于顶栏：顶栏自己的透明度
    UIColor *tint = light ? [UIColor colorWithWhite:1 alpha:ALB25MaterialLightTint()]
                          : [UIColor colorWithWhite:0 alpha:ALB25MaterialTopDarkTint()];
    CGFloat white = 0, alpha = 0;
    [tint getWhite:&white alpha:&alpha];
    if (alpha < minTint) tint = [UIColor colorWithWhite:white alpha:minTint];
    [tint getWhite:&white alpha:&alpha];
    NSString *signature = [NSString stringWithFormat:@"tone|%d|%.3f", light, alpha];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    NSUInteger owners = 0, written = 0, restored = 0, buttons = 0;
    NSMutableOrderedSet<NSString *> *classes = [NSMutableOrderedSet orderedSet];
    for (NSUInteger i = 0; i < queue.count && i < budget; i++) {
        UIView *view = queue[i];
        if (view.hidden) continue;
        // A tab bar's selection lens / selected capsule keeps UIKit's own
        // look, its whole subtree (as the platter path, ALBIsLens).
        NSString *name = NSStringFromClass(view.class);
        if (ALBNameIsLens(name) || [name containsString:@"Highlight"] || [name containsString:@"Indicator"]) continue;
        // A system glass button (its configuration's private material, as
        // the right column's): the same recipe through its configuration,
        // and nothing inside it (UIKit's own glass view there is the
        // configuration's, never written a second way).
        BOOL glassButton = [view isKindOfClass:UIButton.class] && ((UIButton *)view).configuration &&
            ALBConfigurationHasGlass(((UIButton *)view).configuration);
        if (!glassButton) [queue addObjectsFromArray:view.subviews];
        if (glassButton) {
            UIButton *button = (UIButton *)view;
            if (button.isTracking || button.isHighlighted) continue;
            buttons++;
            NSString *had = objc_getAssociatedObject(button, &ALBToneOwnerKey);
            id material = ALBPerform(button.configuration.background, @"_material");
            if (!clear) {
                UIButtonConfiguration *original = objc_getAssociatedObject(button, &ALBToneButtonOriginalKey);
                if (had && original) {
                    UIButtonConfiguration *back = [button.configuration copy];
                    back.background = original.background;
                    button.configuration = back;
                    restored++;
                }
                objc_setAssociatedObject(button, &ALBToneOwnerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                objc_setAssociatedObject(button, &ALBToneButtonOriginalKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                continue;
            }
            // Ours already: the same object, or UIKit re-resolved it into an
            // equal one (its essence), which is then remembered as ours.
            if ([had isEqualToString:signature] && material) {
                if (material == objc_getAssociatedObject(button, &ALBOwnerInstalledKey)) continue;
                NSString *essence = ALB25MaterialEssence(material);
                if (essence && [essence isEqualToString:objc_getAssociatedObject(button, &ALBToneButtonEssenceKey)]) {
                    objc_setAssociatedObject(button, &ALBOwnerInstalledKey, material, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                    continue;
                }
            }
            if (had && !ALBAllowReplacementPass(button)) continue;
            if (!had) objc_setAssociatedObject(button, &ALBToneButtonOriginalKey, [button.configuration copy], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            id glass = nil;
            @try { glass = [ALBMakeGlass(YES, tint, YES) valueForKey:@"glass"]; } @catch (__unused NSException *exception) { glass = nil; }
            UIButtonConfiguration *next = [button.configuration copy];
            if (ALBIsViewGlass(glass) && ALBInstallMaterial(next, glass)) {
                button.configuration = next;
                id landed = ALBPerform(button.configuration.background, @"_material");
                objc_setAssociatedObject(button, &ALBOwnerInstalledKey, landed, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                objc_setAssociatedObject(button, &ALBToneButtonEssenceKey, landed ? ALB25MaterialEssence(landed) : nil, OBJC_ASSOCIATION_COPY_NONATOMIC);
                objc_setAssociatedObject(button, &ALBToneOwnerKey, signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
                written++;
                [classes addObject:NSStringFromClass(button.class)];
            }
            continue;
        }
        id current = nil;
        BOOL effectView = [view isKindOfClass:UIVisualEffectView.class] &&
            [NSStringFromClass([((UIVisualEffectView *)view).effect class]) containsString:@"GlassEffect"];
        if (effectView) current = ((UIVisualEffectView *)view).effect;
        else if (!ALBReadGlassEffect(view, &current) || !current) continue;
        owners++;
        [classes addObject:name];
        id installed = objc_getAssociatedObject(view, &ALBOwnerInstalledKey);
        NSString *had = objc_getAssociatedObject(view, &ALBToneOwnerKey);
        // A getter that hands out a new object on every read is "ours" once
        // written (as ALBApplyGlassOwner): never rewritten on every pass.
        BOOL unstable = [objc_getAssociatedObject(view, &ALBOwnerUnstableKey) boolValue];
        BOOL ours = installed && (current == installed || unstable);
        if (!clear) {
            if (had) {
                id original = objc_getAssociatedObject(view, &ALBOwnerOriginalKey);
                @try {
                    if (effectView) ((UIVisualEffectView *)view).effect = original;
                    else [view setValue:original forKey:@"glassEffect"];
                } @catch (__unused NSException *exception) {}
                objc_setAssociatedObject(view, &ALBToneOwnerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                ALBForgetGlassOwner(view);
                restored++;
            }
            continue;
        }
        if (ours && [had isEqualToString:signature]) continue;
        if (installed && [had isEqualToString:signature] && !ALBAllowReplacementPass(view)) continue;
        if (!had && !objc_getAssociatedObject(view, &ALBOwnerOriginalKey))
            objc_setAssociatedObject(view, &ALBOwnerOriginalKey, current, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        UIVisualEffect *effect = ALBMakeGlass(YES, tint, ALBEffectInteractive(current, YES));
        @try {
            if (effectView) ((UIVisualEffectView *)view).effect = effect;
            else [view setValue:effect forKey:@"glassEffect"];
            id (^read)(void) = ^id{
                id value = nil;
                @try { value = effectView ? ((UIVisualEffectView *)view).effect : [view valueForKey:@"glassEffect"]; }
                @catch (__unused NSException *exception) {}
                return value;
            };
            id readback = read();
            objc_setAssociatedObject(view, &ALBOwnerUnstableKey, ALBReadIsStable(read) ? nil : @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_setAssociatedObject(view, &ALBOwnerInstalledKey, readback ?: effect, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            objc_setAssociatedObject(view, &ALBToneOwnerKey, signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
            written++;
        } @catch (__unused NSException *exception) {}
    }
    return [NSString stringWithFormat:@"玻璃%lu处 按钮玻璃%lu颗 换%lu 还原%lu%@%@", (unsigned long)owners, (unsigned long)buttons,
            (unsigned long)written, (unsigned long)restored, classes.count ? @" " : @"",
            [[classes.array subarrayWithRange:NSMakeRange(0, MIN((NSUInteger)4, classes.count))] componentsJoinedByString:@","]];
}
BOOL ALB25MaterialInstallLiftedGlass(UIButtonConfiguration *configuration) {
    if (!configuration) return NO;
    id glass = nil;
    @try { glass = [ALB25MaterialLiftedEffect(YES) valueForKey:@"glass"]; } @catch (__unused NSException *exception) { glass = nil; }
    return ALBIsViewGlass(glass) && ALBInstallMaterial(configuration, glass);
}
BOOL ALB25MaterialGlassLifted(void) {
    // iOS 26 only for now: on iOS 27 the bottom bar's host is inside the
    // root view, below the layer.
    // 3.11.51：“玻璃放到最上层”设置删了，固定关（顶栏原地）
    return NO;
}
static void ALBRefreshFeedButtons(void) {
    static BOOL scheduled;
    if (scheduled) return;
    scheduled = YES;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.12 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        scheduled = NO;
        for (UIView *view in ALBSurfaces.allObjects) {
            if (![objc_getAssociatedObject(view, &ALBRoleKey) isEqual:@"button.feed"] || !view.window) continue;
            ALB25MaterialApplyButton((UIButton *)view, YES);
        }
    });
}
// The realized glass inside `root`: a round glass first when `preferRound`
// (a side button's, the right column's kind), else the largest.
static id ALBFindRealizedGlass(UIView *root, BOOL preferRound, UIView **owner) {
    id glass = nil, round = nil; UIView *glassOwner = nil, *roundOwner = nil;
    CGFloat area = 0, roundArea = 0;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger index = 0; index < queue.count && index < 300; index++) {
        UIView *view = queue[index];
        [queue addObjectsFromArray:view.subviews];
        if (view.hidden || view.alpha < .01) continue;
        id realized = ALBRealizedGlass(view);
        if (!realized) continue;
        CGFloat width = CGRectGetWidth(view.bounds), height = CGRectGetHeight(view.bounds), size = width * height;
        if (height >= 24 && fabs(width - height) <= 6 && size > roundArea) { round = realized; roundOwner = view; roundArea = size; }
        if (size > area) { glass = realized; glassOwner = view; area = size; }
    }
    if (preferRound && round) { glass = round; glassOwner = roundOwner; }
    if (owner) *owner = glassOwner;
    return glass;
}
void ALB25MaterialCaptureRegisteredGlass(UIView *source) {
    // 3.11.6: also for "glass.followBar" (under the three-part dock there is
    // no liquid bar to read: the top chrome's UIKit glass is the one there).
    if (!source.window || !NSThread.isMainThread || (!ALB25MaterialRegisteredGlassEnabled() && !ALB25MaterialFollowBar())) return;
    // 3.11.1: the top chrome draws the toned Clear recipe ("glass.clear"),
    // not UIKit's glass: nothing of it is copied to the message page.
    if (ALB25MaterialLiftedClear()) return;
    // 3.5: read only while a source may use it ("顶栏", or "底栏" not read yet;
    // 3.11.5: or "glass.followBar", whose fallback it is).
    if (ALB25MaterialRegisteredGlassSource() >= 2 && !ALB25MaterialFollowBar()) return;
    // Read at most every 2 s once seen: a description that is not stable
    // between reads must not rebuild the buttons on every top refresh.
    static CFTimeInterval lastRead;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - lastRead < (ALBRegisteredGlass ? 2.0 : 1.0)) return;
    lastRead = now;
    // A side button's glass before the group's capsule.
    id glass = ALBFindRealizedGlass(source, YES, NULL);
    if (!glass) return;
    NSString *key = ALB25MaterialEssence(glass) ?: @"";
    if ([key isEqualToString:ALBRegisteredGlassKey]) return;
    ALBRegisteredGlass = ALBGlassCopy(glass);
    ALBRegisteredGlassKey = key;
    ALBRefreshFeedButtons();
}
void ALB25MaterialCaptureBarGlass(UITabBar *bar) {
    if (![bar isKindOfClass:UITabBar.class] || !bar.window || !NSThread.isMainThread) return;
    if (ALBOSMajor() < 26 || ALBButtonSource() != ALBButtonSourceBarMaterial) return;
    UIUserInterfaceStyle style = bar.traitCollection.userInterfaceStyle;
    if (style != UIUserInterfaceStyleLight && style != UIUserInterfaceStyleDark) return;
    // As in 2.5.3: only UIKit's own bar glass is copied (System Regular, no
    // tint); under any other recipe the buttons draw the recipe's glass.
    if (!ALB25MaterialBarKeepsNative(ALB25MaterialEnabled(), (int)ALBEffectiveMode(),
                                     ALB25MaterialTintForStyle(style) != nil)) return;
    ALBCaptureBarGlass(ALBCollectPlatters(bar, ALBSurfaceRoot(bar)), style, NO);
}
// Callers that never pass through ALB25MaterialApplyButton (danmaku entry) keep
// a glass configuration even when the view-glass source is selected.
UIButtonConfiguration *ALB25MaterialButtonConfiguration(void) {
    BOOL clear = ALB25MaterialUsesClearStyle();
    NSInteger source = ALBButtonSource() == ALBButtonSourceViewGlass ? ALBButtonSourceFactory : ALBButtonSource();
    UIButtonConfiguration *configuration = ALBFactoryButtonConfiguration(clear);
    if (ALB25MaterialEnabled())
        ALBInstallButtonEffect(configuration, clear, UITraitCollection.currentTraitCollection.userInterfaceStyle, source);
    return configuration;
}
// Every public foreground/layout field. The background (and with it the
// factory's glass kind) comes from the new base.
static void ALBCopyButtonForeground(UIButtonConfiguration *from, UIButtonConfiguration *to) {
    if (!from || !to) return;
    to.cornerStyle = from.cornerStyle;
    to.buttonSize = from.buttonSize;
    to.macIdiomStyle = from.macIdiomStyle;
    to.baseForegroundColor = from.baseForegroundColor;
    to.image = from.image;
    to.imageColorTransformer = from.imageColorTransformer;
    to.preferredSymbolConfigurationForImage = from.preferredSymbolConfigurationForImage;
    to.showsActivityIndicator = from.showsActivityIndicator;
    to.activityIndicatorColorTransformer = from.activityIndicatorColorTransformer;
    to.contentInsets = from.contentInsets;
    to.imagePlacement = from.imagePlacement;
    to.imagePadding = from.imagePadding;
    to.titlePadding = from.titlePadding;
    to.titleAlignment = from.titleAlignment;
    to.automaticallyUpdateForSelection = from.automaticallyUpdateForSelection;
    to.titleTextAttributesTransformer = from.titleTextAttributesTransformer;
    to.subtitleTextAttributesTransformer = from.subtitleTextAttributesTransformer;
    if (from.attributedTitle) to.attributedTitle = from.attributedTitle; else to.title = from.title;
    if (from.attributedSubtitle) to.attributedSubtitle = from.attributedSubtitle; else to.subtitle = from.subtitle;
    // Later additions are copied by selector so no SDK availability applies.
    for (NSString *name in @[@"indicatorColorTransformer", @"symbolContentTransition"]) {
        SEL get = NSSelectorFromString(name);
        SEL set = NSSelectorFromString([NSString stringWithFormat:@"set%@%@:",
            [[name substringToIndex:1] uppercaseString], [name substringFromIndex:1]]);
        if ([from respondsToSelector:get] && [to respondsToSelector:set])
            ((void (*)(id, SEL, id))objc_msgSend)(to, set, ((id (*)(id, SEL))objc_msgSend)(from, get));
    }
    for (NSString *name in @[@"indicator", @"titleLineBreakMode", @"subtitleLineBreakMode"]) {
        SEL get = NSSelectorFromString(name);
        SEL set = NSSelectorFromString([NSString stringWithFormat:@"set%@%@:",
            [[name substringToIndex:1] uppercaseString], [name substringFromIndex:1]]);
        if ([from respondsToSelector:get] && [to respondsToSelector:set])
            ((void (*)(id, SEL, NSInteger))objc_msgSend)(to, set, ((NSInteger (*)(id, SEL))objc_msgSend)(from, get));
    }
}
// Something replaced what this file installed (a caller set a fresh glass
// configuration, or UIKit dropped the view glass).
static BOOL ALBButtonFellBack(UIButton *button) {
    return [objc_getAssociatedObject(button, &ALBButtonPathKey) hasSuffix:@"-unavailable-factory"];
}
static BOOL ALBButtonDrifted(UIButton *button, UIButtonConfiguration *current, NSInteger source) {
    // A source that already fell back to the system configuration is final.
    if ([objc_getAssociatedObject(button, &ALBUnstableReadKey) boolValue] || ALBButtonFellBack(button)) return NO;
    switch (source) {
        case ALBButtonSourceViewGlass: return ALBConfigurationHasGlass(current);
        case ALBButtonSourceBarMaterial: {
            NSArray *installed = objc_getAssociatedObject(button, &ALBInstalledMaterialKey);
            if (installed.count != 2) return NO;
            id material = ALBPerform(current.background, @"_material");
            // Same object: unchanged (fast path, every layout pass).
            if (material == installed[0]) return NO;
            if (![ALB25MaterialEssence(material) isEqualToString:installed[1]]) return YES;
            // An equal material from a caller's rebuild: remember this object
            // so later passes take the fast path again.
            if (material) objc_setAssociatedObject(button, &ALBInstalledMaterialKey, @[material, installed[1]], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            return NO;
        }
        default: {
            UIVisualEffect *installed = objc_getAssociatedObject(button, &ALBInstalledEffectKey);
            return installed && current.background.visualEffect != installed;
        }
    }
}
static UIButtonConfiguration *ALBFallbackConfiguration(UIButton *button, UIButtonConfiguration *current, BOOL clear,
                                                       UIUserInterfaceStyle style) {
    __block UIButtonConfiguration *fallback = nil;
    [ALBTraitsForStyle(button, style) performAsCurrentTraitCollection:^{
        fallback = ALBFactoryButtonConfiguration(clear);
        ALBCopyButtonForeground(current, fallback);
        ALBInstallButtonEffect(fallback, clear, style, ALBButtonSourceFactory);
    }];
    fallback.baseBackgroundColor = nil;
    return fallback;
}

#pragma mark - Native carriers (2.5)

// Probe 4 + the 0.3.4 lab: glass the system draws for a BAR element (tab bar
// platter, bar button item) follows the system Clear/Tinted setting and the
// bar recipe; a UIButton's own glass is a different path that never matches,
// whatever value is copied into it. A carried button is drawn by such a
// native bar element behind it and keeps only the touch, menu and actions.
BOOL ALB25MaterialButtonIsCarried(UIButton *button) {
    return [objc_getAssociatedObject(button, &ALBCarriedKey) boolValue];
}
static BOOL ALBConfigurationDrawsAnything(UIButtonConfiguration *configuration) {
    UIColor *fill = configuration.background.backgroundColor;
    return configuration.image || configuration.title.length || configuration.attributedTitle.length ||
        configuration.subtitle.length || ALBConfigurationHasGlass(configuration) ||
        (fill && CGColorGetAlpha(fill.CGColor) > 0.001);
}
// Empty the button (keeping its insets) and remember what the host app set,
// so a fallback can give it back. Re-runs when the host sets new content.
static void ALBApplyCarriedButton(UIButton *button) {
    UIButtonConfiguration *current = button.configuration;
    if (!ALBConfigurationDrawsAnything(current) &&
        [objc_getAssociatedObject(button, &ALBButtonPathKey) isEqual:@"native-carrier"]) return;
    // Keep the real host configuration (it has glass); a host "repair" that
    // only drops an image into the empty configuration never replaces it.
    if (ALBConfigurationDrawsAnything(current) &&
        (ALBConfigurationHasGlass(current) || !objc_getAssociatedObject(button, &ALBCarriedOriginalKey)))
        objc_setAssociatedObject(button, &ALBCarriedOriginalKey, [current copy], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    UIButtonConfiguration *empty = [UIButtonConfiguration plainButtonConfiguration];
    empty.contentInsets = current.contentInsets;
    // Host update handlers compare the foreground colour; keep it so they
    // see nothing to "fix" (the image stays out).
    empty.baseForegroundColor = current.baseForegroundColor;
    empty.background = [UIBackgroundConfiguration clearConfiguration];
    ALBClearViewGlass(button);
    button.configuration = empty;
    objc_setAssociatedObject(button, &ALBButtonPathKey, @"native-carrier", OBJC_ASSOCIATION_COPY_NONATOMIC);
    objc_setAssociatedObject(button, &ALBEffectSignatureKey, @"native-carrier", OBJC_ASSOCIATION_COPY_NONATOMIC);
    objc_setAssociatedObject(button, &ALBInstalledMaterialKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(button, &ALBInstalledEffectKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ALBSetAudit(button, @"native-carrier", 1, @{@"source":@"native-carrier"});
}
void ALB25MaterialSetButtonCarried(UIButton *button, BOOL carried) {
    if (![button isKindOfClass:UIButton.class] || !NSThread.isMainThread) return;
    if (ALB25MaterialButtonIsCarried(button) == carried) return;
    objc_setAssociatedObject(button, &ALBCarriedKey, carried ? @YES : nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (!carried) {
        // The carrier is gone or not built yet: the button's own glass again,
        // with the content the host app gave it.
        UIButtonConfiguration *saved = objc_getAssociatedObject(button, &ALBCarriedOriginalKey);
        if (saved) button.configuration = saved;
        objc_setAssociatedObject(button, &ALBCarriedOriginalKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(button, &ALBEffectSignatureKey, nil, OBJC_ASSOCIATION_COPY_NONATOMIC);
        objc_setAssociatedObject(button, &ALBButtonPathKey, nil, OBJC_ASSOCIATION_COPY_NONATOMIC);
    }
    [button setNeedsLayout];
}

#pragma mark - Menu sessions (2.5.1)

// UIKit morphs a glass button into its menu and back (and hides the button
// meanwhile). A carried button is empty, so 2.5 gave that morph nothing to
// start from: the menu grew out of an empty capsule above a carrier that
// never moved. For one session the button draws its own glass again; the
// host hides the carrier and carries the button again once the menu is gone.
BOOL ALB25MaterialButtonInMenuSession(UIButton *button) {
    return button && [objc_getAssociatedObject(button, &ALBMenuSessionKey) boolValue];
}
void ALB25MaterialApplyButtonNow(UIButton *button, BOOL feed) {
    if (![button isKindOfClass:UIButton.class] || !NSThread.isMainThread) return;
    objc_setAssociatedObject(button, &ALBForceApplyKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ALB25MaterialApplyButton(button, feed);
    objc_setAssociatedObject(button, &ALBForceApplyKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
NSDictionary *ALB25MaterialBeginButtonMenuSession(UIButton *button) {
    if (![button isKindOfClass:UIButton.class] || !NSThread.isMainThread) return @{@"ownGlass":@NO, @"reason":@"not-main-or-button"};
    BOOL wasCarried = ALB25MaterialButtonIsCarried(button);
    BOOL hadOriginal = objc_getAssociatedObject(button, &ALBCarriedOriginalKey) != nil;
    objc_setAssociatedObject(button, &ALBMenuSessionKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // The host's own configuration back (it has glass), then the current
    // material once, now: UIKit reads the button right after this returns.
    ALB25MaterialSetButtonCarried(button, NO);
    ALB25MaterialApplyButtonNow(button, YES);
    UIButtonConfiguration *configuration = button.configuration;
    BOOL glass = configuration && ALBConfigurationHasGlass(configuration);
    if (glass) objc_setAssociatedObject(button, &ALBMenuSessionKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [button layoutIfNeeded];
    return @{@"ownGlass":@(glass), @"wasCarried":@(wasCarried), @"hadOriginal":@(hadOriginal),
             @"image":@(configuration.image != nil),
             @"path":objc_getAssociatedObject(button, &ALBButtonPathKey) ?: @"-"};
}
void ALB25MaterialEndButtonMenuSession(UIButton *button) {
    if (![button isKindOfClass:UIButton.class]) return;
    objc_setAssociatedObject(button, &ALBMenuSessionKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [button setNeedsLayout];
}
static NSString *ALBShort(id object, NSUInteger limit);

void ALB25MaterialApplyButton(UIButton *button, BOOL feed) {
    if (![button isKindOfClass:UIButton.class] || !NSThread.isMainThread) return;
    if ([objc_getAssociatedObject(button, &ALBApplyingKey) boolValue]) return;
    UIButtonConfiguration *current = button.configuration;
    // Invisible hit buttons carry no configuration and no glass.
    if (!current) return;
    ALBRegister(button, feed ? @"button.feed" : @"button.page");
    // A menu session applies once on purpose, before UIKit takes the button.
    BOOL forced = [objc_getAssociatedObject(button, &ALBForceApplyKey) boolValue];
    // Let UIKit finish the real press/menu source transition first. The next
    // owned layout/appearance pass applies any pending material generation.
    if (!forced && (button.isTracking || button.isHighlighted)) return;
    // 2.5.1: frozen while UIKit morphs this button into its menu and back.
    if (!forced && ALB25MaterialButtonInMenuSession(button)) return;
    objc_setAssociatedObject(button, &ALBApplyingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (ALB25MaterialButtonIsCarried(button)) {
        ALBApplyCarriedButton(button);
        objc_setAssociatedObject(button, &ALBApplyingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        return;
    }
    ALBApplyStyle(button, feed);
    // (3.11: the right column's E2 glass and its glassless in-place buttons are gone.)
    BOOL lifted = feed && ALB25MaterialGlassLifted() &&
                  ![objc_getAssociatedObject(button, &ALBKeepsTreeGlassKey) boolValue];
    BOOL registered = ALBRegisteredFeed(feed) && !lifted;
    BOOL enabled = ALB25MaterialEnabled() || registered || lifted;
    UIUserInterfaceStyle style = ALB25MaterialStyle(button, feed);
    // Opt-out returns to 2.3's native backgrounds: Clear on feed controls.
    BOOL clear = (registered || lifted) ? NO : enabled ? ALB25MaterialUsesClearStyle() : feed;
    NSInteger source = lifted ? ALBButtonSourceViewGlass : registered ? ALBButtonSourceBarMaterial :
                       enabled ? ALBButtonSource() : ALBButtonSourceFactory;
    // The bar's glass is part of the recipe: a new bar glass rebuilds.
    NSDictionary *barEntry = !registered && source == ALBButtonSourceBarMaterial ? ALBBarGlassEntry(style, clear) : nil;
    NSString *bar = barEntry ? [@"b:" stringByAppendingString:barEntry[@"key"]] : @"r";
    // Disabled: one rebuild on the on->off change, never per page/appearance.
    NSString *registeredKey = nil;
    if (registered) ALBRegisteredSourceGlass(&registeredKey, NULL);
    BOOL glassless = lifted && [objc_getAssociatedObject(button, &ALBGlasslessKey) boolValue];
    NSString *signature = lifted ? [NSString stringWithFormat:@"lifted/%@/%d", glassless ? @"none" : @"view-glass", ALB25MaterialLiftedClear()] :
                          registered ? [NSString stringWithFormat:@"registered/%@", registeredKey ?: @"recipe"] :
                          enabled ? [NSString stringWithFormat:@"%@/k%d/s%ld/%@", ALBRecipe(style), clear, (long)source, bar] :
                                    [NSString stringWithFormat:@"off/k%d", clear];
    BOOL stale = ![objc_getAssociatedObject(button, &ALBEffectSignatureKey) isEqual:signature];
    // UIKit dropped the view glass after it was read back: that path does not
    // hold on this button, take the system configuration once.
    BOOL lostViewGlass = !stale && enabled && source == ALBButtonSourceViewGlass && !ALBButtonFellBack(button) &&
        [objc_getAssociatedObject(button, &ALBViewGlassKey) boolValue] && !ALBViewGlass(button);
    BOOL driftDetected = !stale && !lostViewGlass && enabled && ALBButtonDrifted(button, current, source);
    BOOL drifted = driftDetected && ALBAllowReplacementPass(button);
    // UIKit keeps putting its own material back on this button: stop after
    // five re-asserts within 2.5 s (a loop, not a caller's occasional
    // rebuild) and keep the system configuration.
    NSArray *driftWindow = objc_getAssociatedObject(button, &ALB25MaterialDriftKey);
    CFTimeInterval driftStart = driftWindow.count == 2 ? [driftWindow[0] doubleValue] : 0, now = CACurrentMediaTime();
    NSInteger materialDrifts = driftWindow.count == 2 ? [driftWindow[1] integerValue] : 0;
    if (stale || now - driftStart > 2.5) { driftStart = now; materialDrifts = 0; }
    if (drifted && source == ALBButtonSourceBarMaterial) materialDrifts++;
    objc_setAssociatedObject(button, &ALB25MaterialDriftKey, materialDrifts ? @[@(driftStart), @(materialDrifts)] : nil,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    BOOL overridden = drifted && source == ALBButtonSourceBarMaterial && materialDrifts > 4;
    // Throttled: never show a caller's glass configuration and view glass together.
    if (driftDetected && !drifted && source == ALBButtonSourceViewGlass) ALBClearViewGlass(button);
    if (lostViewGlass) {
        ALBClearViewGlass(button);
        button.configuration = ALBFallbackConfiguration(button, current, clear, style);
        objc_setAssociatedObject(button, &ALBButtonPathKey, @"view-glass-unavailable-factory", OBJC_ASSOCIATION_COPY_NONATOMIC);
        objc_setAssociatedObject(button, &ALBInstalledEffectKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (stale || drifted) {
        __block UIButtonConfiguration *next = nil;
        __block NSString *path = @"opt-out-native-factory";
        __block UIVisualEffect *viewGlass = nil;
        UITraitCollection *traits = ALBTraitsForStyle(button, style);
        [traits performAsCurrentTraitCollection:^{
            next = ALBBaseButtonConfiguration(clear, source);
            ALBCopyButtonForeground(current, next);
            // A plain base would otherwise fall back to the tint (blue) foreground.
            if (source == ALBButtonSourceViewGlass && !next.baseForegroundColor) next.baseForegroundColor = UIColor.labelColor;
            if (registered) path = ALBInstallRegisteredEffect(next);
            else if (enabled) path = ALBInstallButtonEffect(next, clear, style, source);
            if (enabled && source == ALBButtonSourceViewGlass)
                viewGlass = lifted ? (glassless ? nil : ALB25MaterialLiftedEffect(YES)) : ALBEffectForStyle(style, YES);
        }];
        next.baseBackgroundColor = nil;
        NSString *intended = [path hasPrefix:@"bar-material-"] ? ALB25MaterialEssence(ALBPerform(next.background, @"_material")) : nil;
        button.configuration = next;
        if (viewGlass && ALBSetViewGlass(button, viewGlass) && ALBViewGlass(button)) {
            ALBSetCapsuleCorners(button, YES);
            objc_setAssociatedObject(button, &ALBViewGlassKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        } else {
            ALBClearViewGlass(button);
            if (viewGlass) {
                // This OS has no usable view glass: never leave a glassless
                // button, fall back to the system configuration.
                button.configuration = ALBFallbackConfiguration(button, current, clear, style);
                path = @"view-glass-unavailable-factory";
            }
        }
        // The bar material must really be on the button the setter returned;
        // otherwise the untouched system configuration (never glassless).
        // The setter may also have swapped it for UIKit's own glass.
        id landed = ALBPerform(button.configuration.background, @"_material");
        BOOL rejected = intended && ![ALB25MaterialEssence(landed) isEqualToString:intended];
        if (enabled && [path hasPrefix:@"bar-material-"] && ![path hasSuffix:@"-unavailable-factory"] &&
            (!landed || rejected || overridden)) {
            button.configuration = ALBFallbackConfiguration(button, current, clear, style);
            path = overridden ? @"bar-material-overridden-unavailable-factory" :
                   rejected ? @"bar-material-rejected-unavailable-factory" : @"bar-material-unavailable-factory";
            landed = nil;
        }
        BOOL barMaterial = enabled && [path hasPrefix:@"bar-material-"] && ![path hasSuffix:@"-unavailable-factory"];
        objc_setAssociatedObject(button, &ALBInstalledMaterialKey, barMaterial && landed ? @[landed, ALB25MaterialEssence(landed)] : nil,
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        UIVisualEffect *readback = button.configuration.background.visualEffect;
        BOOL stable = barMaterial ?
            ALBReadIsStable(^id{ return ALBPerform(button.configuration.background, @"_material"); }) :
            ALBReadIsStable(^id{ return button.configuration.background.visualEffect; });
        objc_setAssociatedObject(button, &ALBUnstableReadKey, stable ? nil : @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(button, &ALBInstalledEffectKey, enabled && readback ? readback : nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(button, &ALBEffectSignatureKey, signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
        objc_setAssociatedObject(button, &ALBButtonPathKey, path, OBJC_ASSOCIATION_COPY_NONATOMIC);
        if (stale) objc_setAssociatedObject(button, &ALBReplaceWindowKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (stale || drifted || lostViewGlass || !objc_getAssociatedObject(button, &ALBAuditKey))
        ALBSetAudit(button, objc_getAssociatedObject(button, &ALBButtonPathKey) ?: @"pending", stale || drifted,
                    @{@"kind":clear ? @"clear" : @"regular", @"source":ALBSourceName(source), @"driftPass":@(drifted),
                      @"materialDrifts":@(materialDrifts)});
    objc_setAssociatedObject(button, &ALBApplyingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

#pragma mark - Effect views

void ALB25MaterialApplyEffectView(UIVisualEffectView *view, BOOL feed) {
    if (![view isKindOfClass:UIVisualEffectView.class] || !NSThread.isMainThread) return;
    ALBRegister(view, feed ? @"effect.feed" : @"effect.page"); ALBApplyStyle(view, feed);
    UIUserInterfaceStyle style = ALB25MaterialStyle(view, feed);
    NSString *signature = ALB25MaterialEnabled() ? ALBRecipe(style) : @"off";
    UIVisualEffect *installed = objc_getAssociatedObject(view, &ALBInstalledEffectKey);
    BOOL unstable = [objc_getAssociatedObject(view, &ALBUnstableReadKey) boolValue];
    BOOL same = [objc_getAssociatedObject(view, &ALBEffectSignatureKey) isEqual:signature];
    if (same && (!ALB25MaterialEnabled() || unstable || view.effect == installed)) return;
    if (!objc_getAssociatedObject(view, &ALBOriginalEffectKey))
        objc_setAssociatedObject(view, &ALBOriginalEffectKey, view.effect ?: NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    id original = objc_getAssociatedObject(view, &ALBOriginalEffectKey);
    if (ALB25MaterialEnabled()) {
        // Keep the caller's interactive glass (download HUD, comment panel).
        BOOL interactive = ALBEffectInteractive(original == NSNull.null ? nil : original, NO);
        __block UIVisualEffect *effect = nil;
        [ALBTraitsForStyle(view, style) performAsCurrentTraitCollection:^{ effect = ALBEffectForStyle(style, interactive); }];
        view.effect = effect;
    } else if (installed && (unstable || view.effect == installed)) view.effect = original == NSNull.null ? nil : original;
    BOOL stable = ALBReadIsStable(^id{ return view.effect; });
    objc_setAssociatedObject(view, &ALBUnstableReadKey, stable ? nil : @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(view, &ALBInstalledEffectKey, ALB25MaterialEnabled() ? view.effect : nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(view, &ALBEffectSignatureKey, signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
    ALBSetAudit(view, @"effect-view", 1, nil);
}

#pragma mark - Bars

// Lens / selection subtrees keep their own glass and interaction.
static BOOL ALBNameIsLens(NSString *name) {
    for (NSString *token in @[@"Lens", @"SelectedContent", @"Selection", @"DestOut", @"Portal"])
        if ([name containsString:token]) return YES;
    return NO;
}
static BOOL ALBIsLens(UIView *view, UIView *stop) {
    for (UIView *v = view; v && v != stop; v = v.superview)
        if (ALBNameIsLens(NSStringFromClass(v.class))) return YES;
    return NO;
}
static UIView *ALBSurfaceRoot(UITabBar *bar) {
    UIView *root = ALB5NativeSurfaceRoot(bar);
    return root.window ? root : bar;
}
// Platter-named views (main capsule, search/auxiliary capsule, their content
// views) under the controller's surface root, skipping lens subtrees and the
// proxy controllers' scroll views. Cached; re-scanned when an entry left the
// root, or once a second while the bar keeps laying out.
static NSArray<UIView *> *ALBCollectPlatters(UITabBar *bar, UIView *root) {
    NSArray *cache = objc_getAssociatedObject(bar, &ALBPlatterCacheKey);
    NSPointerArray *cached = cache.count == 3 ? cache[0] : nil;
    // The root is held non-retained: it owns the bar, so a strong reference
    // from the bar would keep the whole controller hierarchy alive.
    if (cached && [cache[1] nonretainedObjectValue] == root && CACurrentMediaTime() - [cache[2] doubleValue] < 1.0) {
        NSArray *alive = cached.allObjects; BOOL valid = alive.count == cached.count;
        for (UIView *view in alive) if (![view isDescendantOfView:root]) { valid = NO; break; }
        if (valid) return alive;
    }
    NSMutableArray<UIView *> *found = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    if (![bar isDescendantOfView:root]) [queue addObject:bar];
    for (NSUInteger i = 0; i < queue.count && i < 320; i++) {
        UIView *view = queue[i];
        if (i > 0 && [view isKindOfClass:UIScrollView.class]) continue;
        NSString *name = NSStringFromClass(view.class);
        // Breadth-first: a lens is never descended, so only its own name matters.
        if (ALBNameIsLens(name)) continue;
        if ([name containsString:@"Platter"] || [name containsString:@"SearchPill"]) [found addObject:view];
        [queue addObjectsFromArray:view.subviews];
    }
    NSPointerArray *weak = [NSPointerArray weakObjectsPointerArray];
    for (UIView *view in found) [weak addPointer:(__bridge void *)view];
    objc_setAssociatedObject(bar, &ALBPlatterCacheKey, @[weak, [NSValue valueWithNonretainedObject:root],
                             @(CACurrentMediaTime())], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return found;
}
// KVC "glassEffect" (as in v10): it resolves UIView's private
// -_glassEffect accessor that respondsToSelector:@selector(glassEffect) misses.
// A class that throws once is never asked again.
static BOOL ALBReadGlassEffect(UIView *view, id *value) {
    if (!ALBGlassOwnerClasses) ALBGlassOwnerClasses = [NSMutableDictionary dictionary];
    NSString *name = NSStringFromClass(view.class);
    NSNumber *known = ALBGlassOwnerClasses[name];
    if (known && !known.boolValue) return NO;
    id current = nil;
    @try { current = [view valueForKey:@"glassEffect"]; }
    @catch (__unused NSException *exception) { ALBGlassOwnerClasses[name] = @NO; return NO; }
    if (current && ![NSStringFromClass([current class]) containsString:@"GlassEffect"]) { ALBGlassOwnerClasses[name] = @NO; return NO; }
    ALBGlassOwnerClasses[name] = @YES;
    if (value) *value = current;
    return YES;
}
static void ALBForgetGlassOwner(UIView *owner) {
    const void *keys[] = {&ALBOwnerInstalledKey, &ALBOwnerSignatureKey, &ALBOwnerOriginalKey, &ALBOwnerUnstableKey};
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        objc_setAssociatedObject(owner, keys[i], nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static NSString *ALBRestoreGlassOwner(UIView *owner) {
    id original = objc_getAssociatedObject(owner, &ALBOwnerOriginalKey);
    if (original) {
        @try { [owner setValue:original forKey:@"glassEffect"]; }
        @catch (__unused NSException *exception) {}
    }
    ALBForgetGlassOwner(owner);
    [owner setNeedsLayout];
    return @"restored-native";
}
static NSString *ALBApplyGlassOwner(UIView *owner, id current, BOOL keepNative, UIUserInterfaceStyle style, NSString *signature) {
    id installed = objc_getAssociatedObject(owner, &ALBOwnerInstalledKey);
    BOOL unstable = [objc_getAssociatedObject(owner, &ALBOwnerUnstableKey) boolValue];
    BOOL ours = installed && (current == installed || unstable);
    if (keepNative) return ours ? ALBRestoreGlassOwner(owner) : @"native";
    // Same object and recipe: never rewrite on a layout pass (that would
    // interrupt UIKit's own presentation transitions).
    BOOL sameRecipe = [objc_getAssociatedObject(owner, &ALBOwnerSignatureKey) isEqual:signature];
    if (ours && sameRecipe) return @"platter-glassEffect";
    // UIKit put its own glass back with the recipe unchanged: re-assert, but at
    // most three times a second so a platter that rebuilds on layout can't loop.
    if (installed && sameRecipe && !ALBAllowReplacementPass(owner)) return @"platter-glassEffect-throttled";
    if (!objc_getAssociatedObject(owner, &ALBOwnerOriginalKey))
        objc_setAssociatedObject(owner, &ALBOwnerOriginalKey, current, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // UIKit's platter glass is interactive; it carries press/touch/highlight.
    BOOL interactive = ALBEffectInteractive(current, YES);
    __block UIVisualEffect *effect = nil;
    [ALBTraitsForStyle(owner, style) performAsCurrentTraitCollection:^{ effect = ALBEffectForStyle(style, interactive); }];
    @try {
        [owner setValue:effect forKey:@"glassEffect"];
        id readback = [owner valueForKey:@"glassEffect"];
        BOOL stable = ALBReadIsStable(^id{ id value = nil; @try { value = [owner valueForKey:@"glassEffect"]; } @catch (__unused NSException *e) {} return value; });
        objc_setAssociatedObject(owner, &ALBOwnerUnstableKey, stable ? nil : @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(owner, &ALBOwnerInstalledKey, readback ?: effect, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(owner, &ALBOwnerSignatureKey, signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
    } @catch (__unused NSException *exception) {
        ALBGlassOwnerClasses[NSStringFromClass(owner.class)] = @NO;
        return @"platter-glassEffect-write-failed";
    }
    [owner setNeedsLayout];
    return @"platter-glassEffect";
}

void ALB25MaterialApplyBar(UITabBar *bar, BOOL bottom) {
    if (![bar isKindOfClass:UITabBar.class] || !NSThread.isMainThread || !bar.window) return;
    if ([objc_getAssociatedObject(bar, &ALBApplyingKey) boolValue]) return;
    objc_setAssociatedObject(bar, &ALBApplyingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    ALBRegister(bar, bottom ? @"bar.bottom" : @"bar.top");
    if (bottom) ALBBottomSource = bar;
    UIView *root = ALBSurfaceRoot(bar);
    // A controller-managed bar (bottom AlbumMorph, top group) gets its
    // Light/Dark from its controller with the explicit target tab; writing a
    // second, resolver-based value here would flip it during transitions.
    // Only a standalone bar is styled here, and its tint follows its override.
    BOOL ownedStyle = ALB5NativeSurfaceRoot(bar) != bar;
    if (!ownedStyle) ALBApplyStyle(bar, YES);
    UIUserInterfaceStyle override = bar.overrideUserInterfaceStyle;
    UIUserInterfaceStyle style = override == UIUserInterfaceStyleLight || override == UIUserInterfaceStyleDark ?
        override : ALB25MaterialStyle(bar, YES);
    BOOL enabled = ALB25MaterialEnabled();
    BOOL clear = ALB25MaterialUsesClearStyle();
    UIColor *tint = ALB25MaterialTintForStyle(style);
    BOOL keepNative = ALB25MaterialBarKeepsNative(enabled, (int)ALBEffectiveMode(), tint != nil);
    // Only what changes the written effect: a Home <-> tab Light/Dark switch
    // with the same effect must not rewrite the platter mid-transition.
    NSString *signature = [NSString stringWithFormat:@"%d/%d/%@", enabled, clear, tint ?: @"-"];
    NSArray<UIView *> *platters = ALBCollectPlatters(bar, root);
    NSUInteger owners = 0, written = 0, changedOwners = 0;
    NSMutableArray *paths = [NSMutableArray array];
    for (UIView *platter in platters) {
        if (!ownedStyle) ALBApplyStyle(platter, YES);
        id current = nil;
        if (!ALBReadGlassEffect(platter, &current)) continue;
        if (!current) {
            // No glass right now (every UIView answers the key; UIKit also
            // empties a platter while minimizing or morphing). Never add one,
            // and forget anything recorded so it is not re-asserted.
            if (objc_getAssociatedObject(platter, &ALBOwnerInstalledKey)) ALBForgetGlassOwner(platter);
            continue;
        }
        owners++;
        id before = objc_getAssociatedObject(platter, &ALBOwnerSignatureKey);
        NSString *path = ALBApplyGlassOwner(platter, current, keepNative, style, signature);
        BOOL ours = [path hasPrefix:@"platter-glassEffect"] && ![path hasSuffix:@"-write-failed"];
        if ([path isEqualToString:@"platter-glassEffect"]) written++;
        if ([path isEqualToString:@"restored-native"] ||
            (ours && ![before isEqual:objc_getAssociatedObject(platter, &ALBOwnerSignatureKey)])) changedOwners++;
        if (![objc_getAssociatedObject(platter, &ALBAuditKey)[@"path"] isEqual:path]) ALBSetAudit(platter, path, 1, nil);
        [paths addObject:[NSString stringWithFormat:@"%@:%@", NSStringFromClass(platter.class), path]];
    }
    // Only UIKit's own bar glass is copied; a written bar's glass is the recipe.
    if (bottom && enabled && keepNative) ALBCaptureBarGlass(platters, style, changedOwners > 0);
    else if (bottom) ALBBarGlassState = enabled ? @"written-recipe" : @"disabled";
    NSString *barPath = !enabled ? @"disabled" : keepNative ? @"native-bar" :
        written ? @"platter-glassEffect" : owners ? @"platter-glassEffect-pending" :
        platters.count ? @"no-glass-owner" : @"no-platter-found";
    NSDictionary *previous = objc_getAssociatedObject(bar, &ALBAuditKey);
    if (![previous[@"path"] isEqual:barPath] || ![previous[@"owners"] isEqual:paths] ||
        [previous[@"platters"] unsignedIntegerValue] != platters.count)
        ALBSetAudit(bar, barPath, written, @{@"root":NSStringFromClass(root.class),
            @"platters":@(platters.count), @"glassOwners":@(owners), @"owners":paths});
    [CATransaction commit];
    objc_setAssociatedObject(bar, &ALBApplyingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

// Every input of the recipe. Callers' own configuration caches key on the
// generation, so it only advances when one of these really changed.
static NSString *ALBGlobalRecipeKey(void) {
    // The effective mode carries the video/page switch.
    return [NSString stringWithFormat:@"%d/%d/%ld/%ld/%ld/%ld/%.3f/%.3f/%ld/%d/%d/%ld",
        ALB25MaterialRegisteredGlassEnabled(), ALB25MaterialEnabled(), (long)ALB25MaterialMode(), (long)ALBEffectiveMode(), (long)ALBPrefInteger(@"tint", 0), (long)ALBAppearancePreference(),
        ALBFixedNumber(@"glass.tintStrength", .12, 0, .6), ALBFixedNumber(@"glass.clearDarkTint", .30, 0, .6),
        (long)ALBButtonSource(), ALBFixedBool(@"glass.feedDark", YES),
        UIAccessibilityIsReduceTransparencyEnabled(), (long)UIScreen.mainScreen.traitCollection.userInterfaceStyle];
}
void ALB25MaterialRefresh(void) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{ ALB25MaterialRefresh(); }); return; }
    static NSString *lastKey;
    NSString *key = ALBGlobalRecipeKey();
    if (![key isEqualToString:lastKey]) { lastKey = key; ALBGeneration++; }
    if (ALBBottomSource) ALB25MaterialApplyBar(ALBBottomSource, YES);
    for (UIView *view in ALBSurfaces.allObjects) {
        NSString *role = objc_getAssociatedObject(view, &ALBRoleKey);
        if ([role hasPrefix:@"button"]) ALB25MaterialApplyButton((UIButton *)view, [role hasSuffix:@"feed"]);
        else if ([role hasPrefix:@"effect"]) ALB25MaterialApplyEffectView((UIVisualEffectView *)view, [role hasSuffix:@"feed"]);
        else if ([role hasPrefix:@"bar"]) ALB25MaterialApplyBar((UITabBar *)view, [role hasSuffix:@"bottom"]);
        [view setNeedsLayout];
    }
}

#pragma mark - Status

static NSString *ALBShort(id object, NSUInteger limit) {
    NSString *text = [object description] ?: @"nil";
    if (text.length <= limit) return text;
    NSRange safe = [text rangeOfComposedCharacterSequencesForRange:NSMakeRange(0, limit)];
    return [[text substringToIndex:MIN(text.length, NSMaxRange(safe))] stringByAppendingString:@"…"];
}
// 2.5.1 top-left menu probe: what a button holds right now. Read-only.
NSDictionary *ALB25MaterialButtonProbeState(UIButton *button) {
    if (![button isKindOfClass:UIButton.class]) return @{};
    UIButtonConfiguration *configuration = button.configuration;
    id material = configuration ? ALBPerform(configuration.background, @"_material") : nil;
    return @{@"carried":@(ALB25MaterialButtonIsCarried(button)), @"menuSession":@(ALB25MaterialButtonInMenuSession(button)),
             @"configuration":@(configuration != nil), @"ownGlass":@(configuration && ALBConfigurationHasGlass(configuration)),
             @"image":@(configuration.image != nil),
             @"material":material ? ALBShort(ALB25MaterialEssence(material), 160) : @"nil",
             @"path":objc_getAssociatedObject(button, &ALBButtonPathKey) ?: @"-",
             @"hasCarriedOriginal":@(objc_getAssociatedObject(button, &ALBCarriedOriginalKey) != nil)};
}
NSDictionary *ALB25MaterialAudit(UIView *view) {
    return view ? objc_getAssociatedObject(view, &ALBAuditKey) : nil;
}
NSString *ALB25MaterialRegisteredGlassKey(void) {
    if (ALB25MaterialGlassLifted()) return [NSString stringWithFormat:@"lifted/%d", ALB25MaterialLiftedClear()];
    NSString *key = nil;
    ALBRegisteredSourceGlass(&key, NULL);
    return [NSString stringWithFormat:@"%ld/%@", (long)ALB25MaterialRegisteredGlassSource(), key ?: @"recipe"];
}
// 3.10.10: what InstallRegisteredEffect puts in, whatever "glass.lifted" is
// (the E2 glass always takes the registered glass; the key above says
// "lifted" while that option is on and never changed for it).
NSString *ALB25MaterialRegisteredSourceKey(void) {
    NSString *key = nil;
    ALBRegisteredSourceGlass(&key, NULL);
    return [NSString stringWithFormat:@"%ld/%@", (long)ALB25MaterialRegisteredGlassSource(), key ?: @"recipe"];
}
BOOL ALB25MaterialInstallRegisteredEffect(UIButtonConfiguration *configuration) {
    if (!configuration || !NSThread.isMainThread) return NO;
    return ![ALBInstallRegisteredEffect(configuration) hasSuffix:@"-unavailable-factory"];
}
BOOL ALB25MaterialInstallRegisteredGlass(UIButtonConfiguration *configuration) {
    if (!configuration || !NSThread.isMainThread) return NO;
    id glass = ALBGlassCopy(ALBRegisteredSourceGlass(NULL, NULL));
    return glass ? ALBInstallMaterial(configuration, glass) : NO;
}
// 3.5 诊断: what one glass is made of. A configured button: its private
// material; otherwise the realized glass on or inside `root`.
NSString *ALB25MaterialGlassReport(UIView *root, BOOL preferRound) {
    if (![root isKindOfClass:UIView.class]) return @"无";
    UIView *owner = nil; id glass = nil; NSString *slot = @"bg";
    if ([root isKindOfClass:UIButton.class]) {
        glass = ALBPerform(((UIButton *)root).configuration.background, @"_material");
        if (glass) { owner = root; slot = @"material"; }
    }
    if (!glass) glass = ALBFindRealizedGlass(root, preferRound, &owner);
    if (!glass || !owner) return [NSString stringWithFormat:@"%@ 无玻璃", NSStringFromClass(root.class)];
    return [NSString stringWithFormat:@"%@ %.0f×%.0f 风格=%ld/%ld %@=%@", NSStringFromClass(owner.class),
            CGRectGetWidth(owner.bounds), CGRectGetHeight(owner.bounds), (long)owner.overrideUserInterfaceStyle,
            (long)owner.traitCollection.userInterfaceStyle, slot, ALBShort(ALB25MaterialKey(glass), 520)];
}
NSString *ALB25MaterialBottomGlassReport(UITabBar *bar) {
    if (![bar isKindOfClass:UITabBar.class]) return @"无";
    return ALB25MaterialGlassReport(ALBSurfaceRoot(bar), NO);
}
// 3.6 诊断: everything that decides how one glass renders.
// The bottom bar's main capsule (largest platter with a realized glass).
UIView *ALB25MaterialBottomPlatter(UITabBar *bar) {
    if (![bar isKindOfClass:UITabBar.class]) return nil;
    UIView *best = nil; CGFloat area = 0;
    for (UIView *platter in ALBCollectPlatters(bar, ALBSurfaceRoot(bar))) {
        if (platter.hidden || platter.alpha < .01 || !platter.window || !ALBRealizedGlass(platter)) continue;
        CGFloat size = CGRectGetWidth(platter.bounds) * CGRectGetHeight(platter.bounds);
        if (size > area) { best = platter; area = size; }
    }
    return best;
}
UIView *ALB25MaterialGlassOwner(UIView *root, BOOL preferRound) {
    UIView *owner = nil;
    if (!root) return nil;
    ALBFindRealizedGlass(root, preferRound, &owner);
    return owner;
}
static id ALBRawBackground(UIView *view) {
    @try { return [view valueForKey:@"_background"]; } @catch (__unused NSException *exception) { return nil; }
}
static NSString *ALBLayerValue(CALayer *layer, NSString *key) {
    id value = nil;
    @try { value = [layer valueForKey:key]; } @catch (__unused NSException *exception) { return nil; }
    if (!value) return nil;
    if ([value isKindOfClass:NSArray.class]) {
        NSMutableArray *names = [NSMutableArray array];
        for (id item in (NSArray *)value) {
            id name = nil;
            @try { name = [item valueForKey:@"name"]; } @catch (__unused NSException *exception) { name = nil; }
            [names addObject:name ? [name description] : NSStringFromClass([item class])];
        }
        return [NSString stringWithFormat:@"[%@]", [names componentsJoinedByString:@","]];
    }
    return ALBShort(value, 60);
}
// Backdrop (sampling) layers under the glass view and their settings.
static NSString *ALBBackdropSummary(UIView *view) {
    NSMutableArray<NSString *> *rows = [NSMutableArray array];
    NSMutableArray<NSString *> *classes = [NSMutableArray array];
    NSMutableArray<CALayer *> *queue = [NSMutableArray arrayWithObject:view.layer];
    for (NSUInteger i = 0; i < queue.count && i < 120; i++) {
        CALayer *layer = queue[i];
        if (layer.sublayers.count) [queue addObjectsFromArray:layer.sublayers];
        NSString *name = NSStringFromClass(layer.class);
        if (i > 0 && classes.count < 24 && ![classes containsObject:name]) [classes addObject:name];
        if (![name containsString:@"Backdrop"] || rows.count >= 5) continue;
        NSMutableArray<NSString *> *values = [NSMutableArray arrayWithObject:name];
        for (NSString *key in @[@"groupName", @"groupNamespace", @"scale", @"captureOnly", @"enabled", @"marginWidth",
                                @"windowServerAware", @"reducesCaptureBitDepth", @"ignoresScreenClip", @"allowsInPlaceFiltering", @"filters"]) {
            NSString *value = ALBLayerValue(layer, key);
            if (value) [values addObject:[NSString stringWithFormat:@"%@=%@", key, value]];
        }
        [values addObject:[NSString stringWithFormat:@"不透明度=%.2f 隐藏=%d", layer.opacity, layer.hidden]];
        [rows addObject:[values componentsJoinedByString:@" "]];
    }
    return [NSString stringWithFormat:@"%@；层=%@", rows.count ? [rows componentsJoinedByString:@" ｜ "] : @"无取样层",
            [classes componentsJoinedByString:@","]];
}
// Every ancestor up to the window that changes how what is behind reaches
// the glass (alpha, masks, filters, rasterizing, glass groups, fills).
static NSString *ALBAncestorSummary(UIView *view) {
    NSMutableArray<NSString *> *rows = [NSMutableArray array];
    NSUInteger depth = 0;
    for (UIView *cursor = view.superview; cursor && depth < 60; cursor = cursor.superview, depth++) {
        CALayer *layer = cursor.layer;
        NSMutableArray<NSString *> *flags = [NSMutableArray array];
        if (cursor.alpha < .999) [flags addObject:[NSString stringWithFormat:@"透明度%.2f", cursor.alpha]];
        if (layer.mask) [flags addObject:@"遮罩"];
        if (layer.filters.count) [flags addObject:[@"滤镜" stringByAppendingString:ALBLayerValue(layer, @"filters") ?: @""]];
        if (layer.backgroundFilters.count) [flags addObject:@"背景滤镜"];
        if (layer.compositingFilter) [flags addObject:@"合成滤镜"];
        if (layer.shouldRasterize) [flags addObject:@"栅格化"];
        if (layer.allowsGroupOpacity && layer.opacity < .999) [flags addObject:@"组透明"];
        if (!CATransform3DIsIdentity(layer.transform)) [flags addObject:@"变换"];
        if (cursor.overrideUserInterfaceStyle != UIUserInterfaceStyleUnspecified)
            [flags addObject:[NSString stringWithFormat:@"风格%ld", (long)cursor.overrideUserInterfaceStyle]];
        CGFloat fillAlpha = cursor.backgroundColor ? CGColorGetAlpha(cursor.backgroundColor.CGColor) : 0;
        if (fillAlpha > .01) [flags addObject:[NSString stringWithFormat:@"底色%.2f", fillAlpha]];
        id background = ALBRawBackground(cursor);
        if ([NSStringFromClass([background class]) containsString:@"Group"]) [flags addObject:@"玻璃组"];
        else if (ALBIsViewGlass(background)) [flags addObject:@"玻璃"];
        if ([NSStringFromClass(cursor.class) containsString:@"Backdrop"] || [cursor isKindOfClass:UIVisualEffectView.class])
            [flags addObject:@"效果视图"];
        if (!flags.count || rows.count >= 16) continue;
        [rows addObject:[NSString stringWithFormat:@"%lu:%@(%@)", (unsigned long)(depth + 1), NSStringFromClass(cursor.class),
                         [flags componentsJoinedByString:@"+"]]];
    }
    return [NSString stringWithFormat:@"共%lu层 %@", (unsigned long)depth,
            rows.count ? [rows componentsJoinedByString:@" "] : @"（都没有）"];
}
NSString *ALB25MaterialGlassDeepReport(UIView *view) {
    if (![view isKindOfClass:UIView.class]) return @"无";
    id glass = nil; NSString *slot = @"bg";
    if ([view isKindOfClass:UIButton.class]) {
        glass = ALBPerform(((UIButton *)view).configuration.background, @"_material");
        slot = @"material";
    }
    if (!glass) { glass = ALBRealizedGlass(view); slot = @"bg"; }
    UIWindow *window = view.window;
    NSMutableString *text = [NSMutableString stringWithFormat:@"%@ %.0f×%.0f 风格=%ld/%ld 窗口=%@ 层级%.0f",
        NSStringFromClass(view.class), CGRectGetWidth(view.bounds), CGRectGetHeight(view.bounds),
        (long)view.overrideUserInterfaceStyle, (long)view.traitCollection.userInterfaceStyle,
        window ? NSStringFromClass(window.class) : @"无", window.windowLevel];
    [text appendFormat:@"\n  玻璃[%@]=%@", slot, glass ? ALBShort(ALB25MaterialKey(glass), 1500) : @"无"];
    [text appendFormat:@"\n  取样=%@", ALBBackdropSummary(view)];
    [text appendFormat:@"\n  祖先=%@", ALBAncestorSummary(view)];
    return text;
}
// The chosen source, what the buttons really get, and what was read.
NSString *ALB25MaterialRegisteredGlassStatus(void) {
    NSString *origin = nil;
    ALBRegisteredSourceGlass(NULL, &origin);
    NSArray *names = @[@"底栏", @"顶栏", @"常规"];
    NSDictionary *dark = ALBBarGlassByStyle[@(UIUserInterfaceStyleDark)], *light = ALBBarGlassByStyle[@(UIUserInterfaceStyleLight)];
    return [NSString stringWithFormat:@"玻璃成分=%@ 实际=%@ 登记=%@ 底栏读取=%@(深%@/浅%@) 顶栏读取=%@ 降低透明度=%@ 增强对比度=%@",
            names[(NSUInteger)ALB25MaterialRegisteredGlassSource()], origin ?: @"?",
            ALB25MaterialRegisteredGlassEnabled() ? @"开" : @"关", ALBBarGlassState,
            dark ? @"有" : @"无", light ? @"有" : @"无", ALBRegisteredGlass ? @"有" : @"无",
            UIAccessibilityIsReduceTransparencyEnabled() ? @"开" : @"关",
            UIAccessibilityDarkerSystemColorsEnabled() ? @"开" : @"关"];
}
// The glass classes' private names, for the next step (read-only).
NSString *ALB25MaterialGlassRuntimeNames(void) {
    NSMutableArray<NSString *> *names = [NSMutableArray array];
    for (NSString *className in @[@"UIGlassEffect", @"_UIViewGlass"]) {
        Class cls = NSClassFromString(className);
        if (!cls) { [names addObject:[className stringByAppendingString:@":无"]]; continue; }
        NSMutableArray<NSString *> *found = [NSMutableArray array];
        for (int meta = 0; meta < 2; meta++) {
            unsigned int count = 0;
            Method *methods = class_copyMethodList(meta ? object_getClass(cls) : cls, &count);
            for (unsigned int i = 0; i < count && found.count < 40; i++) {
                NSString *name = NSStringFromSelector(method_getName(methods[i]));
                NSString *lower = name.lowercaseString;
                if ([lower containsString:@"glass"] || [lower containsString:@"variant"] || [lower containsString:@"style"] ||
                    [lower containsString:@"group"] || [lower containsString:@"clear"] || [lower containsString:@"init"])
                    [found addObject:meta ? [@"+" stringByAppendingString:name] : name];
            }
            free(methods);
        }
        [names addObject:[NSString stringWithFormat:@"%@:%@", className, [found componentsJoinedByString:@","]]];
    }
    return [names componentsJoinedByString:@" | "];
}
NSString *ALB25MaterialViewGlassSummary(UIView *view) {
    if (![view isKindOfClass:UIView.class]) return nil;
    id effect = ALBViewGlass(view), realized = ALBRealizedGlass(view);
    if (!effect && !realized) return nil;
    NSMutableString *text = [NSMutableString string];
    if (effect) [text appendFormat:@"effect:%@", NSStringFromClass([effect class])];
    if (realized) [text appendFormat:@"%@bg:%@", text.length ? @" " : @"", ALBShort(ALB25MaterialEssence(realized), 120)];
    return text;
}
NSDictionary *ALB25MaterialSnapshot(void) {
    NSMutableArray *rows = [NSMutableArray array];
    for (UIView *view in ALBSurfaces.allObjects) {
        if (!view.window) continue;
        NSMutableDictionary *row = [@{@"role":objc_getAssociatedObject(view, &ALBRoleKey) ?: @"?",
            @"class":NSStringFromClass(view.class), @"style":@(view.overrideUserInterfaceStyle),
            @"resolvedStyle":@(view.traitCollection.userInterfaceStyle), @"alpha":@(view.alpha)} mutableCopy];
        NSDictionary *audit = objc_getAssociatedObject(view, &ALBAuditKey); if (audit) row[@"coverage"] = audit;
        if ([view isKindOfClass:UIVisualEffectView.class]) row[@"effect"] = ALBShort(((UIVisualEffectView *)view).effect, 200);
        if ([view isKindOfClass:UIButton.class]) {
            UIButton *button = (UIButton *)view;
            UIBackgroundConfiguration *background = button.configuration.background;
            id material = ALBPerform(background, @"_material");
            row[@"effect"] = background.visualEffect ? ALBShort(background.visualEffect, 200) :
                material ? @"private-material" : @"none";
            NSString *barKey = ALBBarGlassKeyFor(button.overrideUserInterfaceStyle);
            if (material && barKey) row[@"materialIsBarGlass"] = @([ALB25MaterialEssence(material) isEqualToString:ALB25MaterialEssence(barKey)]);
            row[@"viewGlass"] = ALBViewGlass(button) ? @"yes" : @"no";
            row[@"menu"] = @(button.menu != nil);
            row[@"menuAsPrimaryAction"] = @(button.showsMenuAsPrimaryAction);
            row[@"targetCount"] = @(button.allTargets.count);
            row[@"interaction"] = @(button.userInteractionEnabled);
        }
        if ([view isKindOfClass:UITabBar.class]) {
            NSMutableArray *carriers = [NSMutableArray array];
            for (UIView *platter in ALBCollectPlatters((UITabBar *)view, ALBSurfaceRoot((UITabBar *)view))) {
                NSMutableDictionary *entry = [@{@"class":NSStringFromClass(platter.class),
                    @"override":@(platter.overrideUserInterfaceStyle),
                    @"style":@(platter.traitCollection.userInterfaceStyle), @"hidden":@(platter.hidden)} mutableCopy];
                NSDictionary *a = objc_getAssociatedObject(platter, &ALBAuditKey); if (a) entry[@"coverage"] = a;
                id glass = nil; if (ALBReadGlassEffect(platter, &glass)) entry[@"glassEffect"] = glass ? ALBShort(glass, 200) : @"nil";
                [carriers addObject:entry];
            }
            row[@"carriers"] = carriers;
        }
        [rows addObject:row];
    }
    return @{@"version":@"2.5-MATERIAL3", @"os":NSProcessInfo.processInfo.operatingSystemVersionString,
        @"enabled":@(ALB25MaterialEnabled()), @"mode":ALBModeName(ALB25MaterialMode()),
        @"clearStyle":@(ALB25MaterialUsesClearStyle()), @"barsWantClear":@(ALB25MaterialBarsWantClear()),
        @"clearDarkTint":@(ALBFixedNumber(@"glass.clearDarkTint", .30, 0, .6)),
        @"buttonSource":ALBSourceName(ALBButtonSource()),
        @"barGlass":@{@"state":ALBBarGlassState ?: @"?", @"lastStyle":@(ALBBarGlassStyle),
                      @"lightRecipe":ALBBarGlassByStyle[@(UIUserInterfaceStyleLight)][@"recipe"] ?: @"-",
                      @"darkRecipe":ALBBarGlassByStyle[@(UIUserInterfaceStyleDark)][@"recipe"] ?: @"-"},
        @"clearVideoOnly":@(ALBFixedBool(@"glass.clearVideoOnly", YES)), @"videoContext":@(ALBVideoContext()),
        @"effectiveMode":ALBModeName(ALBEffectiveMode()),
        @"nonVideoChannel":ALBLastNonVideoChannel ?: @"-",
        @"factoryEffect":@{@"glass":ALBFactoryEffectClass[0] ?: @"unseen", @"clearGlass":ALBFactoryEffectClass[1] ?: @"unseen"},
        @"glassOwnerClasses":[ALBGlassOwnerClasses copy] ?: @{},
        @"appearance":@(ALBAppearancePreference()),
        @"feedDark":@(ALBFixedBool(@"glass.feedDark",YES)), @"systemStyle":@(UIScreen.mainScreen.traitCollection.userInterfaceStyle),
        @"reduceTransparency":@(UIAccessibilityIsReduceTransparencyEnabled()),
        @"generation":@(ALBGeneration), @"surfaces":rows};
}

#pragma mark - Structure probe (user-triggered, read-only)

// An empty token list matches every selector.
static NSArray<NSString *> *ALBMatchingSelectors(Class cls, Class stop, NSArray<NSString *> *tokens, NSUInteger limit) {
    NSMutableOrderedSet *result = [NSMutableOrderedSet orderedSet];
    for (Class c = cls; c && c != stop && result.count < limit; c = class_getSuperclass(c)) {
        unsigned int count = 0; Method *methods = class_copyMethodList(c, &count);
        for (unsigned int i = 0; i < count && result.count < limit; i++) {
            NSString *name = NSStringFromSelector(method_getName(methods[i]));
            BOOL match = tokens.count == 0;
            for (NSString *token in tokens)
                if ([name rangeOfString:token options:NSCaseInsensitiveSearch].location != NSNotFound) { match = YES; break; }
            if (!match) continue;
            const char *types = method_getTypeEncoding(methods[i]);
            [result addObject:[NSString stringWithFormat:@"%@.%@ %s", NSStringFromClass(c), name, types ?: ""]];
        }
        free(methods);
    }
    return result.array;
}
static id ALBProbeKey(id object, NSString *key, NSUInteger limit) {
    @try { id value = [object valueForKey:key]; return value ? [NSString stringWithFormat:@"%@ %@", NSStringFromClass([value class]), ALBShort(value, limit)] : @"nil"; }
    @catch (NSException *exception) { return [@"throws:" stringByAppendingString:exception.name ?: @"?"]; }
}
// Where a shadow could come from: the layer itself, a sublayer, or the parent.
static NSDictionary *ALBProbeLayer(CALayer *layer) {
    if (!layer) return @{};
    NSMutableArray *sublayers = [NSMutableArray array];
    for (CALayer *child in layer.sublayers) {
        if (sublayers.count >= 12) break;
        NSMutableString *entry = [NSStringFromClass(child.class) mutableCopy];
        if (child.name.length) [entry appendFormat:@" name=%@", child.name];
        if (child.shadowOpacity > 0) [entry appendFormat:@" shadow=%.2f/r%.1f/%@", child.shadowOpacity, child.shadowRadius, child.shadowPath ? @"path" : @"alpha"];
        if (child.filters.count) [entry appendFormat:@" filters=%lu", (unsigned long)child.filters.count];
        if (child.compositingFilter) [entry appendFormat:@" compositing=%@", child.compositingFilter];
        if (child.hidden) [entry appendString:@" hidden"];
        [sublayers addObject:entry];
    }
    return @{@"class":NSStringFromClass(layer.class), @"masksToBounds":@(layer.masksToBounds),
        @"cornerRadius":@(layer.cornerRadius), @"shadowOpacity":@(layer.shadowOpacity), @"shadowRadius":@(layer.shadowRadius),
        @"shadowOffset":NSStringFromCGSize(layer.shadowOffset), @"shadowPath":@(layer.shadowPath != NULL),
        @"filters":@(layer.filters.count), @"mask":@(layer.mask != nil), @"sublayers":sublayers};
}
static NSDictionary *ALBProbeView(UIView *view, UIView *root) {
    NSMutableArray *chain = [NSMutableArray array];
    for (Class c = view.class; c && c != UIResponder.class && chain.count < 8; c = class_getSuperclass(c)) [chain addObject:NSStringFromClass(c)];
    id glass = ALBViewGlass(view);
    NSMutableArray *subviews = [NSMutableArray array];
    for (UIView *child in view.subviews) [subviews addObject:NSStringFromClass(child.class)];
    return @{@"classChain":chain, @"frameInRoot":NSStringFromCGRect([view convertRect:view.bounds toView:root]),
        @"hidden":@(view.hidden), @"override":@(view.overrideUserInterfaceStyle), @"style":@(view.traitCollection.userInterfaceStyle),
        @"lens":@(ALBIsLens(view, root)),
        @"glassEffect":glass ? ALBShort(glass, 300) : @"nil",
        @"glass":glass ? ALBProbeKey(glass, @"glass", 4000) : @"nil",
        @"_background":ALBProbeKey(view, @"_background", 4000),
        @"_backgroundIsBarGlass":ALBBarGlassKeyFor(view.traitCollection.userInterfaceStyle) && ALBRealizedGlass(view) ?
            @([ALB25MaterialEssence(ALBRealizedGlass(view)) isEqualToString:ALB25MaterialEssence(ALBBarGlassKeyFor(view.traitCollection.userInterfaceStyle))]) : @"-",
        @"layer":ALBProbeLayer(view.layer), @"superview":view.superview ? NSStringFromClass(view.superview.class) : @"nil",
        @"superLayer":ALBProbeLayer(view.superview.layer),
        @"traits":ALBShort(view.traitCollection, 4000), @"subviews":subviews};
}
static NSDictionary *ALBProbeBar(UITabBar *bar) {
    UIView *root = ALBSurfaceRoot(bar);
    NSMutableArray *platters = [NSMutableArray array];
    // Fresh scan, independent of the material cache.
    objc_setAssociatedObject(bar, &ALBPlatterCacheKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    for (UIView *platter in ALBCollectPlatters(bar, root))
        if (platters.count < 8 && ALBViewGlass(platter)) [platters addObject:ALBProbeView(platter, root)];
    return @{@"bar":NSStringFromClass(bar.class), @"root":NSStringFromClass(root.class),
        @"barOverride":@(bar.overrideUserInterfaceStyle), @"rootOverride":@(root.overrideUserInterfaceStyle),
        @"audit":objc_getAssociatedObject(bar, &ALBAuditKey) ?: @{}, @"glassPlatters":platters};
}
// Read-only typed getter call (object, integer, BOOL, double, float).
static id ALBProbeScalar(id object, NSString *name) {
    SEL selector = NSSelectorFromString(name);
    Method method = object ? class_getInstanceMethod(object_getClass(object), selector) : NULL;
    if (!method || method_getNumberOfArguments(method) != 2) return @"-";
    char type[16] = {0};
    method_getReturnType(method, type, sizeof(type));
    @try {
        switch (type[0]) {
            case 'q': case 'l': return @(((long long (*)(id, SEL))objc_msgSend)(object, selector));
            case 'Q': case 'L': return @(((unsigned long long (*)(id, SEL))objc_msgSend)(object, selector));
            case 'i': return @(((int (*)(id, SEL))objc_msgSend)(object, selector));
            case 'I': return @(((unsigned int (*)(id, SEL))objc_msgSend)(object, selector));
            case 'B': return @(((BOOL (*)(id, SEL))objc_msgSend)(object, selector));
            case 'd': return @(((double (*)(id, SEL))objc_msgSend)(object, selector));
            case 'f': return @(((float (*)(id, SEL))objc_msgSend)(object, selector));
            case '@': { id value = ((id (*)(id, SEL))objc_msgSend)(object, selector);
                        return value ? [NSString stringWithFormat:@"%@ %@", NSStringFromClass([value class]), ALBShort(value, 600)] : @"nil"; }
            default: return [NSString stringWithFormat:@"type:%s", type];
        }
    } @catch (NSException *exception) { return [@"throws:" stringByAppendingString:exception.name ?: @"?"]; }
}
// Every readable knob of a _UIViewGlass (listed by probe 3).
static NSDictionary *ALBProbeGlassProps(id glass) {
    if (!glass) return @{};
    NSMutableDictionary *props = [NSMutableDictionary dictionary];
    for (NSString *name in @[@"variant", @"size", @"smoothness", @"identifier", @"flexible", @"allowsGrouping",
                             @"excludingShadow", @"excludingPlatter", @"excludingForeground", @"excludingControlLensing",
                             @"excludingControlDisplacement", @"contentLensing", @"highlightsDisplayAngle",
                             @"boostWhitePoint", @"subvariant", @"controlTintColor", @"tintColor", @"backdropGroupName",
                             @"_flexVariant", @"_identityGlass"])
        props[name] = ALBProbeScalar(glass, name);
    return props;
}
// What UIKit draws for a glass in a given trait environment and size.
static id ALBProbeResolved(id glass, UITraitCollection *traits, CGSize size) {
    SEL selector = NSSelectorFromString(@"_resolvedMaterialUsingTraitCollection:size:");
    Method method = glass ? class_getInstanceMethod(object_getClass(glass), selector) : NULL;
    const char *types = method ? method_getTypeEncoding(method) : NULL;
    if (!types || strcmp(types, "@40@0:8@16{CGSize=dd}24") != 0 || !traits) return @"unavailable";
    @try {
        id resolved = ((id (*)(id, SEL, id, CGSize))objc_msgSend)(glass, selector, traits, size);
        return resolved ? [NSString stringWithFormat:@"%@ %@", NSStringFromClass([resolved class]), ALBShort(resolved, 3000)] : @"nil";
    } @catch (NSException *exception) { return [@"throws:" stringByAppendingString:exception.name ?: @"?"]; }
}
// The views that actually draw a background material under a button.
static NSArray *ALBProbeMaterialHosts(UIView *root) {
    NSMutableArray *hosts = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < queue.count && i < 40 && hosts.count < 6; i++) {
        UIView *view = queue[i];
        [queue addObjectsFromArray:view.subviews];
        id has = ALBProbeScalar(view, @"_hasBackgroundMaterial");
        id glass = ALBProbeScalar(view, @"_glassEffect");
        BOOL hasGlass = [glass isKindOfClass:NSString.class] && ![glass isEqual:@"nil"] && ![glass isEqual:@"-"];
        if (![has isEqual:@YES] && !hasGlass) continue;
        [hosts addObject:@{@"class":NSStringFromClass(view.class), @"size":NSStringFromCGSize(view.bounds.size),
            @"hasBackgroundMaterial":has, @"isResolvedBackgroundGlass":ALBProbeScalar(view, @"_isResolvedBackgroundGlass"),
            @"_background":ALBProbeScalar(view, @"_background"),
            @"_resolvedBackgroundMaterial":ALBProbeScalar(view, @"_resolvedBackgroundMaterial"),
            @"_shouldAdaptToMaterials":ALBProbeScalar(view, @"_shouldAdaptToMaterials"),
            @"traits":ALBShort(view.traitCollection, 1200)}];
    }
    return hosts;
}
// Probe 3 showed the buttons and the bars drawing the SAME glass value (Clear,
// tint nil) yet looking different, so the difference is in what UIKit resolves
// for each view: size and/or trait environment. Resolve both ways round.
static NSDictionary *ALBProbeCompare(void) {
    UITabBar *bar = ALBBottomSource;
    UIView *platter = nil; CGFloat area = 0;
    if (bar.window)
        for (UIView *candidate in ALBCollectPlatters(bar, ALBSurfaceRoot(bar))) {
            CGFloat size = CGRectGetWidth(candidate.bounds) * CGRectGetHeight(candidate.bounds);
            if (!candidate.hidden && size > area && ALBRealizedGlass(candidate)) { platter = candidate; area = size; }
        }
    UIButton *button = nil;
    for (UIView *view in ALBSurfaces.allObjects)
        if ([view isKindOfClass:UIButton.class] && view.window && !view.hidden &&
            CGRectIntersectsRect([view convertRect:view.bounds toView:nil], view.window.bounds) &&
            ALBPerform(((UIButton *)view).configuration.background, @"_material")) { button = (UIButton *)view; break; }
    if (!platter || !button) return @{@"available":@NO, @"platter":@(platter != nil), @"button":@(button != nil)};
    id barGlass = ALBRealizedGlass(platter);
    id buttonGlass = ALBPerform(button.configuration.background, @"_material");
    CGSize barSize = platter.bounds.size, buttonSize = button.bounds.size;
    UITraitCollection *barTraits = platter.traitCollection, *buttonTraits = button.traitCollection;
    return @{@"available":@YES,
        @"platterClass":NSStringFromClass(platter.class), @"buttonClass":NSStringFromClass(button.class),
        @"barSize":NSStringFromCGSize(barSize), @"buttonSize":NSStringFromCGSize(buttonSize),
        @"barGlassProps":ALBProbeGlassProps(barGlass), @"buttonGlassProps":ALBProbeGlassProps(buttonGlass),
        @"resolved":@{
            @"barGlass_barTraits_barSize":ALBProbeResolved(barGlass, barTraits, barSize),
            @"barGlass_buttonTraits_barSize":ALBProbeResolved(barGlass, buttonTraits, barSize),
            @"barGlass_barTraits_buttonSize":ALBProbeResolved(barGlass, barTraits, buttonSize),
            @"buttonGlass_buttonTraits_buttonSize":ALBProbeResolved(buttonGlass, buttonTraits, buttonSize),
            @"buttonGlass_barTraits_buttonSize":ALBProbeResolved(buttonGlass, barTraits, buttonSize),
            @"buttonGlass_buttonTraits_barSize":ALBProbeResolved(buttonGlass, buttonTraits, barSize)},
        @"platterResolvedBackgroundMaterial":ALBProbeScalar(platter, @"_resolvedBackgroundMaterial"),
        @"viewGlassIvars":({ NSMutableArray *ivars = [NSMutableArray array]; unsigned int count = 0;
            Ivar *list = class_copyIvarList(object_getClass(barGlass), &count);
            for (unsigned int i = 0; i < count && i < 60; i++)
                [ivars addObject:[NSString stringWithFormat:@"%s %s", ivar_getName(list[i]) ?: "?", ivar_getTypeEncoding(list[i]) ?: ""]];
            free(list); ivars; }),
        @"platterShouldAdapt":ALBProbeScalar(platter, @"_shouldAdaptToMaterials"),
        @"buttonMaterialHosts":ALBProbeMaterialHosts(button),
        @"buttonAllowGlassAdaptivity":ALBProbeScalar(button.configuration, @"_allowGlassAdaptivity"),
        @"buttonConfiguration":ALBShort(button.configuration, 800),
        @"barTraits":ALBShort(barTraits, 1500), @"buttonTraits":ALBShort(buttonTraits, 1500)};
}
static NSDictionary *ALBProbeFactory(BOOL clear) {
    UIButtonConfiguration *configuration = ALBFactoryButtonConfiguration(clear);
    UIBackgroundConfiguration *background = configuration.background;
    id material = ALBPerform(background, @"_material");
    return @{@"configuration":ALBShort(configuration, 600), @"background":ALBShort(background, 600),
        @"visualEffect":background.visualEffect ? ALBShort(background.visualEffect, 300) : @"nil",
        @"_material":material ? [NSString stringWithFormat:@"%@ %@", NSStringFromClass([material class]), ALBShort(material, 4000)] : @"nil",
        @"_allowGlassAdaptivity":ALBProbeKey(configuration, @"_allowGlassAdaptivity", 40)};
}
NSDictionary *ALB25MaterialProbeReport(void) {
    NSMutableDictionary *report = [ALB25MaterialSnapshot() mutableCopy];
    struct utsname system; uname(&system);
    report[@"device"] = @(system.machine);
    report[@"app"] = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"?";
    NSMutableDictionary *bars = [NSMutableDictionary dictionary];
    NSMutableArray *buttons = [NSMutableArray array];
    NSUInteger topIndex = 0;
    for (UIView *view in ALBSurfaces.allObjects) {
        if (!view.window) continue;
        if ([view isKindOfClass:UITabBar.class]) {
            NSString *role = objc_getAssociatedObject(view, &ALBRoleKey) ?: @"bar";
            NSString *key = [role hasSuffix:@"bottom"] ? @"bottom" : [NSString stringWithFormat:@"top%lu", (unsigned long)topIndex++];
            bars[key] = ALBProbeBar((UITabBar *)view);
        } else if ([view isKindOfClass:UIButton.class] && buttons.count < 6 && !view.hidden &&
                   CGRectIntersectsRect([view convertRect:view.bounds toView:nil], view.window.bounds)) {
            // On-screen buttons only (2.4.1 listed cells thousands of points away).
            UIButton *button = (UIButton *)view;
            NSMutableDictionary *row = [ALBProbeView(button, button.window) mutableCopy];
            id material = ALBPerform(button.configuration.background, @"_material");
            row[@"_material"] = material ? ALBShort(material, 4000) : @"nil";
            NSString *barKey = ALBBarGlassKeyFor(button.overrideUserInterfaceStyle);
            row[@"_materialIsBarGlass"] = material && barKey ? @([ALB25MaterialEssence(material) isEqualToString:ALB25MaterialEssence(barKey)]) : @"-";
            row[@"audit"] = objc_getAssociatedObject(button, &ALBAuditKey) ?: @{};
            row[@"visualEffect"] = button.configuration.background.visualEffect ?
                ALBProbeKey(button.configuration.background.visualEffect, @"glass", 4000) : @"nil";
            [buttons addObject:row];
        }
    }
    report[@"bars"] = bars;
    report[@"buttons"] = buttons;
    report[@"buttonFactory"] = @{@"glass":ALBProbeFactory(NO), @"clearGlass":ALBProbeFactory(YES)};
    NSMutableDictionary *effects = [NSMutableDictionary dictionary];
    for (NSNumber *clear in @[@NO, @YES]) {
        UIVisualEffect *effect = ALBMakeGlass(clear.boolValue, nil, YES);
        effects[clear.boolValue ? @"clear" : @"regular"] = @{@"glass":ALBProbeKey(effect, @"glass", 4000)};
    }
    // Does UIGlassEffect's tint reach its _UIViewGlass (the button material path)?
    UIVisualEffect *tinted = ALBMakeGlass(YES, [UIColor colorWithWhite:0 alpha:.3], YES);
    effects[@"clearTintedBlack30"] = @{@"glass":ALBProbeKey(tinted, @"glass", 4000)};
    report[@"glassEffect"] = effects;
    report[@"compare"] = ALBProbeCompare();
    report[@"visiblePages"] = ALBVisiblePageNames();
    report[@"barGlassValue"] = @{@"light":ALBShort(ALBBarGlassByStyle[@(UIUserInterfaceStyleLight)][@"glass"], 4000),
                                 @"dark":ALBShort(ALBBarGlassByStyle[@(UIUserInterfaceStyleDark)][@"glass"], 4000)};
    report[@"screenTraits"] = ALBShort(UIScreen.mainScreen.traitCollection, 4000);
    // Knobs for the next round (size/frost/shadow), read-only listings.
    Class viewGlass = [ALBPerform(ALBFactoryButtonConfiguration(YES).background, @"_material") class] ?: NSClassFromString(@"_UIViewGlass");
    Class platter = nil;
    for (UIView *view in ALBSurfaces.allObjects)
        if ([view isKindOfClass:UITabBar.class] && view.window)
            for (UIView *candidate in ALBCollectPlatters((UITabBar *)view, ALBSurfaceRoot((UITabBar *)view)))
                if (!platter && [NSStringFromClass(candidate.class) containsString:@"PlatterView"]) platter = candidate.class;
    report[@"selectors"] = @{
        @"viewGlass":viewGlass ? ALBMatchingSelectors(viewGlass, NSObject.class, @[], 160) : @[],
        @"glassEffect":NSClassFromString(@"UIGlassEffect") ? ALBMatchingSelectors(NSClassFromString(@"UIGlassEffect"), UIVisualEffect.class, @[], 80) : @[],
        @"backgroundConfiguration":ALBMatchingSelectors(UIBackgroundConfiguration.class, NSObject.class, @[@"material", @"glass", @"effect", @"shadow"], 60),
        @"platter":platter ? ALBMatchingSelectors(platter, UIView.class, @[@"glass", @"shadow", @"background", @"material", @"luma", @"tint", @"layout"], 80) : @[],
        @"view":ALBMatchingSelectors(UIView.class, UIResponder.class, @[@"_background", @"glass", @"material", @"luma"], 80),
        @"buttonConfiguration":ALBMatchingSelectors(UIButtonConfiguration.class, NSObject.class, @[@"glass", @"adapt", @"material", @"_"], 60),
        @"systemBackgroundView":NSClassFromString(@"_UISystemBackgroundView") ?
            ALBMatchingSelectors(NSClassFromString(@"_UISystemBackgroundView"), UIView.class, @[@"glass", @"material", @"adapt", @"shadow", @"luma", @"effect"], 60) : @[]};
    return report;
}
