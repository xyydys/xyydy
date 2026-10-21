#import "ALBUnifiedMaterial.h"
#import "ALB5Options.h"
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>

static BOOL (^ALBFeedResolver)(void);
static UIUserInterfaceStyle (^ALBAppResolver)(void);
static NSUInteger ALBGeneration = 1;
static NSHashTable<UIView *> *ALBSurfaces;
static __weak UITabBar *ALBBottomSource;
static char ALBRoleKey, ALBEffectKey, ALBOriginalEffectKey, ALBInstalledEffectKey;
static char ALBSignatureKey, ALBOriginalStyleKey, ALBFallbackKey, ALBLeasesKey, ALBAuditKey;
static char ALBApplyingKey;
static char ALBOriginalButtonBackgroundKey;
static NSDictionary *ALBBottomRecipe;

static NSString *ALBPref(NSString *key) { return ALB5Key([@"glass." stringByAppendingString:key]); }
NSInteger ALBMaterialMode(void) {
    NSNumber *mode = [NSUserDefaults.standardUserDefaults objectForKey:ALBPref(@"mode")];
    return ALBMaterialResolveMode((int)NSProcessInfo.processInfo.operatingSystemVersion.majorVersion,
        mode.intValue, mode != nil, ALB5Bool(@"bottom.clearMode", NO));
}
BOOL ALBMaterialEnabled(void) { return ALB5Bool(@"glass.enabled", YES); }
NSUInteger ALBMaterialGeneration(void) { return ALBGeneration; }

void ALBMaterialInstallResolvers(BOOL (^feed)(void), UIUserInterfaceStyle (^app)(void)) {
    ALBFeedResolver = [feed copy]; ALBAppResolver = [app copy];
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        ALBSurfaces = [NSHashTable weakObjectsHashTable];
        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        for (NSString *name in @[@"ALB5.OptionsChanged", UIApplicationDidBecomeActiveNotification,
                                  UIAccessibilityReduceTransparencyStatusDidChangeNotification])
            [center addObserverForName:name object:nil queue:NSOperationQueue.mainQueue
                usingBlock:^(__unused NSNotification *note) { ALBMaterialRefresh(); }];
    });
}

UIUserInterfaceStyle ALBMaterialStyle(UIView *view, BOOL feedEligible) {
    UIScreen *screen = view.window.screen ?: UIScreen.mainScreen;
    UIUserInterfaceStyle systemStyle = screen.traitCollection.userInterfaceStyle;
    if (systemStyle == UIUserInterfaceStyleUnspecified) systemStyle = UIUserInterfaceStyleLight;
    UIUserInterfaceStyle appStyle = ALBAppResolver ? ALBAppResolver() : view.window.traitCollection.userInterfaceStyle;
    NSInteger preference = [NSUserDefaults.standardUserDefaults integerForKey:ALBPref(@"appearance")];
    return (UIUserInterfaceStyle)ALBMaterialResolveStyle((int)preference, (int)systemStyle,
        (int)appStyle, feedEligible && ALBFeedResolver && ALBFeedResolver(), ALB5Bool(@"glass.feedDark", YES));
}

static UIColor *ALBMaterialTint(void) {
    NSInteger choice = [NSUserDefaults.standardUserDefaults integerForKey:ALBPref(@"tint")];
    // Clear is the tested untinted recipe even when a Regular tint is stored.
    if (ALBMaterialMode() != ALBMaterialRegular || choice == 0) return nil;
    CGFloat alpha = ALB5Number(@"glass.tintStrength", .12, 0, .6);
    UIColor *color = choice == 1 ? UIColor.labelColor : choice == 2 ? UIColor.whiteColor :
                     choice == 3 ? UIColor.blackColor : UIColor.systemBlueColor;
    return [color colorWithAlphaComponent:alpha];
}

UIVisualEffect *ALBMaterialMakeEffect(BOOL interactive) {
    if (@available(iOS 26.0, *)) {
        Class cls = NSClassFromString(@"UIGlassEffect");
        SEL maker = NSSelectorFromString(@"effectWithStyle:");
        if ([cls respondsToSelector:maker]) {
            NSInteger style = ALBMaterialMode() == ALBMaterialRegular ? 0 : 1;
            UIVisualEffect *effect = ((id (*)(id,SEL,NSInteger))objc_msgSend)(cls, maker, style);
            SEL tint = NSSelectorFromString(@"setTintColor:"), touch = NSSelectorFromString(@"setInteractive:");
            if ([effect respondsToSelector:tint]) ((void (*)(id,SEL,id))objc_msgSend)(effect, tint, ALBMaterialTint());
            if ([effect respondsToSelector:touch]) ((void (*)(id,SEL,BOOL))objc_msgSend)(effect, touch, interactive);
            return effect;
        }
    }
    return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
}

static void ALBRegister(UIView *view, NSString *role) {
    if (!ALBSurfaces) ALBSurfaces = [NSHashTable weakObjectsHashTable];
    [ALBSurfaces addObject:view];
    objc_setAssociatedObject(view, &ALBRoleKey, role, OBJC_ASSOCIATION_COPY_NONATOMIC);
    if (!objc_getAssociatedObject(view, &ALBOriginalStyleKey))
        objc_setAssociatedObject(view, &ALBOriginalStyleKey, @(view.overrideUserInterfaceStyle), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBApplyStyle(UIView *view, BOOL feed) {
    UIUserInterfaceStyle style = ALBMaterialEnabled() ? ALBMaterialStyle(view, feed) :
        [objc_getAssociatedObject(view, &ALBOriginalStyleKey) integerValue];
    if (view.overrideUserInterfaceStyle != style) view.overrideUserInterfaceStyle = style;
}
static NSString *ALBSignature(UIView *view, BOOL feed) {
    return [NSString stringWithFormat:@"%lu/%ld/%ld/%d/%d", (unsigned long)ALBGeneration,
        (long)ALBMaterialMode(), (long)ALBMaterialStyle(view, feed), ALBMaterialEnabled(),
        UIAccessibilityIsReduceTransparencyEnabled()];
}
static UITraitCollection *ALBMaterialTraits(UIView *view, BOOL feed) {
    UITraitCollection *style = [UITraitCollection traitCollectionWithUserInterfaceStyle:ALBMaterialStyle(view, feed)];
    return [UITraitCollection traitCollectionWithTraitsFromCollections:@[view.traitCollection, style]];
}
static UIVisualEffect *ALBEffectForSurface(UIView *view, BOOL feed, BOOL interactive) {
    __block UIVisualEffect *effect = nil;
    // Material constructors may resolve traits before their first render.
    // Resolve inside the same explicit style used by the destination view.
    [ALBMaterialTraits(view, feed) performAsCurrentTraitCollection:^{ effect = ALBMaterialMakeEffect(interactive); }];
    return effect;
}

UIButtonConfiguration *ALBMaterialButtonConfiguration(void) {
    BOOL clear = ALBMaterialEnabled() && ALBMaterialMode() != ALBMaterialRegular;
    SEL maker = NSSelectorFromString(clear ? @"clearGlassButtonConfiguration" : @"glassButtonConfiguration");
    Class cls = UIButtonConfiguration.class;
    UIButtonConfiguration *configuration = [cls respondsToSelector:maker] ?
        ((id (*)(id,SEL))objc_msgSend)(cls, maker) : [UIButtonConfiguration plainButtonConfiguration];
    if (ALBMaterialEnabled()) {
        // Public background API. Keep the native glass configuration, the
        // UIButton, its menu, target/actions and configuration update handler.
        UIBackgroundConfiguration *background = [configuration.background copy];
        background.visualEffect = ALBMaterialMakeEffect(YES);
        background.backgroundColor = UIColor.clearColor;
        configuration.background = background;
        configuration.baseBackgroundColor = nil;
    }
    return configuration;
}

void ALBMaterialApplyEffectView(UIVisualEffectView *view, BOOL feed) {
    if (!view || !NSThread.isMainThread) return;
    ALBRegister(view, feed ? @"effect.feed" : @"effect.page"); ALBApplyStyle(view, feed);
    NSString *signature = ALBSignature(view, feed);
    if ([objc_getAssociatedObject(view, &ALBSignatureKey) isEqual:signature] &&
        (!ALBMaterialEnabled() || view.effect == objc_getAssociatedObject(view, &ALBInstalledEffectKey))) return;
    if (!objc_getAssociatedObject(view, &ALBOriginalEffectKey))
        objc_setAssociatedObject(view, &ALBOriginalEffectKey, view.effect ?: NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    id original = objc_getAssociatedObject(view, &ALBOriginalEffectKey);
    if (ALBMaterialEnabled()) view.effect = ALBEffectForSurface(view, feed, NO);
    else if (view.effect == objc_getAssociatedObject(view, &ALBInstalledEffectKey)) view.effect = original == NSNull.null ? nil : original;
    objc_setAssociatedObject(view, &ALBInstalledEffectKey, ALBMaterialEnabled() ? view.effect : nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(view, &ALBSignatureKey, signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
}

// A lease only covers a renderer's appearance. Foreground view layers and
// native lenses/menu sources are excluded; inert backdrop owners are checked.
@interface ALBMaterialLayerLease : NSObject
@property(nonatomic, weak) CALayer *layer;
@property(nonatomic) float opacity;
@property(nonatomic, copy) NSDictionary *values;
@end
@implementation ALBMaterialLayerLease @end

static BOOL ALBIsLens(UIView *view) {
    for (UIView *v = view; v; v = v.superview) {
        NSString *name = NSStringFromClass(v.class);
        for (NSString *token in @[@"Lens", @"SelectedContent", @"Selection", @"DestOut", @"Portal"])
            if ([name containsString:token]) return YES;
    }
    return NO;
}
static BOOL ALBPureBackdropView(UIView *view) {
    if (!view || view.userInteractionEnabled || ALBIsLens(view)) return NO;
    NSString *name = NSStringFromClass(view.class);
    BOOL backdrop = [name containsString:@"VisualEffectBackdropView"] ||
        [name containsString:@"UIBackdropView"] || [name containsString:@"GlassBackgroundView"];
    if (!backdrop) return NO;
    NSMutableArray *queue = [NSMutableArray arrayWithObject:view];
    for (NSUInteger i = 0; i < queue.count && i < 64; i++) {
        UIView *v = queue[i];
        if ([v isKindOfClass:UIControl.class] || [v isKindOfClass:UILabel.class] ||
            [v isKindOfClass:UIImageView.class] || ALBIsLens(v)) return NO;
        [queue addObjectsFromArray:v.subviews];
    }
    return queue.count <= 64;
}
static NSArray<CALayer *> *ALBRenderLayers(UIView *view) {
    NSMutableArray *result = [NSMutableArray array], *queue = [NSMutableArray arrayWithArray:view.layer.sublayers ?: @[]];
    for (NSUInteger i = 0; i < queue.count && i < 100; i++) {
        CALayer *layer = queue[i];
        // A view-backed layer is excluded unless its owner is a known,
        // noninteractive backdrop containing no foreground or control.
        if ([layer.delegate isKindOfClass:UIView.class] && !ALBPureBackdropView(layer.delegate)) continue;
        NSString *name = NSStringFromClass(layer.class).lowercaseString;
        NSString *label = (layer.name ?: @"").lowercaseString;
        if ([label containsString:@"lens"] || [label containsString:@"selection"] ||
            [label containsString:@"content"] || [label containsString:@"portal"]) continue;
        if ([name containsString:@"multilayer"] || [name containsString:@"backdrop"] || [name containsString:@"glass"] ||
            ([layer.delegate isKindOfClass:UIView.class] && ALBPureBackdropView(layer.delegate))) {
            [result addObject:layer];
        }
        [queue addObjectsFromArray:layer.sublayers ?: @[]];
    }
    return result;
}
static NSDictionary *ALBLayerValues(CALayer *layer) {
    NSMutableDictionary *values = [NSMutableDictionary dictionary];
    for (NSString *key in @[@"filters", @"backgroundFilters", @"compositingFilter"]) {
        @try { id value = [layer valueForKey:key]; if (value) values[key] = value; }
        @catch (__unused NSException *exception) {}
    }
    if (layer.backgroundColor) values[@"@fill"] = [UIColor colorWithCGColor:layer.backgroundColor];
    return values;
}
static void ALBRestoreLayers(UIView *owner) {
    NSArray<ALBMaterialLayerLease *> *leases = objc_getAssociatedObject(owner, &ALBLeasesKey);
    for (ALBMaterialLayerLease *lease in leases) {
        if (!lease.layer) continue;
        if (lease.layer.opacity != lease.opacity) lease.layer.opacity = lease.opacity;
        for (NSString *key in lease.values) {
            if ([key isEqualToString:@"@fill"]) {
                UIColor *color = lease.values[key]; lease.layer.backgroundColor = color == (id)NSNull.null ? nil : color.CGColor; continue;
            }
            @try { id value = lease.values[key]; [lease.layer setValue:value == NSNull.null ? nil : value forKey:key]; }
            @catch (__unused NSException *exception) {}
        }
    }
    objc_setAssociatedObject(owner, &ALBLeasesKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBSetAudit(UIView *view, NSString *path, NSUInteger count) {
    NSMutableDictionary *audit = [@{@"path":path, @"changed":@(count),
        @"mode":@(ALBMaterialMode()), @"style":@(view.overrideUserInterfaceStyle)} mutableCopy];
    UIVisualEffectView *renderer = objc_getAssociatedObject(view, &ALBFallbackKey);
    UIVisualEffect *effect = renderer.effect ?: objc_getAssociatedObject(view, &ALBInstalledEffectKey);
    if (effect) { audit[@"effectClass"] = NSStringFromClass(effect.class); audit[@"effect"] = effect.description ?: @"nil"; }
    objc_setAssociatedObject(view, &ALBAuditKey, audit, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

// Native26: copy only the verified bottom renderer recipe onto equivalent
// render-only layers. Nothing is moved into or out of a button/contentView.
static NSUInteger ALBSyncBottomRecipe(UIView *owner) {
    if (!ALBBottomRecipe.count) return 0;
    NSArray<CALayer *> *layers = ALBRenderLayers(owner);
    NSMutableArray *leases = [NSMutableArray array]; NSUInteger changed = 0;
    for (CALayer *layer in layers) {
        NSDictionary *recipe = ALBBottomRecipe[NSStringFromClass(layer.class)];
        if (!recipe.count) continue;
        ALBMaterialLayerLease *lease = [ALBMaterialLayerLease new]; lease.layer = layer;
        lease.opacity = layer.opacity;
        NSMutableDictionary *saved = [NSMutableDictionary dictionary];
        for (NSString *key in recipe) {
            if ([key isEqualToString:@"@fill"]) {
                saved[key] = layer.backgroundColor ? [UIColor colorWithCGColor:layer.backgroundColor] : NSNull.null; continue;
            }
            @try { saved[key] = [layer valueForKey:key] ?: NSNull.null; }
            @catch (__unused NSException *exception) {}
        }
        lease.values = saved;
        [leases addObject:lease];
        for (NSString *key in recipe) {
            if ([key isEqualToString:@"@fill"]) { layer.backgroundColor = ((UIColor *)recipe[key]).CGColor; changed++; continue; }
            @try { [layer setValue:recipe[key] forKey:key]; changed++; }
            @catch (__unused NSException *exception) {}
        }
    }
    objc_setAssociatedObject(owner, &ALBLeasesKey, leases, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return changed;
}

void ALBMaterialApplyButton(UIButton *button, BOOL feed) {
    if (!button || !NSThread.isMainThread) return;
    if ([objc_getAssociatedObject(button, &ALBApplyingKey) boolValue]) return;
    ALBRegister(button, feed ? @"button.feed" : @"button.page");
    // Let UIKit finish the real press/menu source transition first. The next
    // owned layout/appearance pass applies any pending material generation.
    if (button.isTracking || button.isHighlighted) return;
    objc_setAssociatedObject(button, &ALBApplyingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ALBApplyStyle(button, feed);
    NSString *signature = ALBSignature(button, feed);
    if (button.configuration && !objc_getAssociatedObject(button, &ALBOriginalButtonBackgroundKey)) {
        // Restore a native background on opt-out; foreground is always read
        // from the current configuration so imported artwork is not reverted.
        SEL maker = NSSelectorFromString(feed ? @"clearGlassButtonConfiguration" : @"glassButtonConfiguration");
        UIButtonConfiguration *native = [UIButtonConfiguration.class respondsToSelector:maker] ?
            ((id (*)(id,SEL))objc_msgSend)(UIButtonConfiguration.class,maker) : [UIButtonConfiguration plainButtonConfiguration];
        objc_setAssociatedObject(button, &ALBOriginalButtonBackgroundKey, native.background, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    UIVisualEffect *current = button.configuration.background.visualEffect;
    if ((! [objc_getAssociatedObject(button, &ALBSignatureKey) isEqual:signature] ||
         (ALBMaterialEnabled() && current != objc_getAssociatedObject(button, &ALBInstalledEffectKey))) && button.configuration) {
        UIButtonConfiguration *configuration = [button.configuration copy];
        // Replace the background of this same configuration. Do not recreate
        // the control or replace its native UIMenu/primaryAction/hit-test path.
        __block UIButtonConfiguration *material = nil;
        [ALBMaterialTraits(button, feed) performAsCurrentTraitCollection:^{ material = ALBMaterialButtonConfiguration(); }];
        configuration.background = ALBMaterialEnabled() ? material.background : objc_getAssociatedObject(button, &ALBOriginalButtonBackgroundKey);
        configuration.baseBackgroundColor = nil;
        button.configuration = configuration;
        objc_setAssociatedObject(button, &ALBInstalledEffectKey, button.configuration.background.visualEffect, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(button, &ALBSignatureKey, signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
    }
    if (ALBMaterialMode() == ALBMaterialNative26 && ALBMaterialEnabled()) {
        ALBRestoreLayers(button);
        NSUInteger count = ALBSyncBottomRecipe(button);
        ALBSetAudit(button, count ? @"bottom-native-recipe" : @"native-clear-button-fallback", count);
    } else {
        ALBRestoreLayers(button); ALBSetAudit(button, @"native-button-background", 1);
    }
    objc_setAssociatedObject(button, &ALBApplyingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void ALBRestoreCarrier(UIView *carrier) {
    ALBRestoreLayers(carrier);
    UIVisualEffectView *fallback = objc_getAssociatedObject(carrier, &ALBFallbackKey);
    [fallback removeFromSuperview];
    objc_setAssociatedObject(carrier, &ALBFallbackKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    id original = objc_getAssociatedObject(carrier, &ALBOriginalEffectKey);
    SEL setter = NSSelectorFromString(@"setGlassEffect:");
    if (original && [carrier respondsToSelector:setter]) {
        ((void (*)(id,SEL,id))objc_msgSend)(carrier, setter, original == NSNull.null ? nil : original);
        objc_setAssociatedObject(carrier, &ALBSignatureKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}
static NSUInteger ALBApplyCarrier(UIView *carrier, BOOL native26) {
    if (ALBIsLens(carrier)) return 0;
    ALBRestoreLayers(carrier);
    UIVisualEffectView *fallback = objc_getAssociatedObject(carrier, &ALBFallbackKey);
    if (!ALBMaterialEnabled()) { ALBRestoreCarrier(carrier); return 0; }
    if (native26) {
        NSUInteger count = ALBSyncBottomRecipe(carrier);
        if (count) { [fallback removeFromSuperview]; objc_setAssociatedObject(carrier, &ALBFallbackKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); ALBSetAudit(carrier, @"bottom-native-recipe", count); return count; }
    }
    // Prefer a live public glass effect view to a separate renderer. A carrier
    // that already contains one must never receive a second glass background.
    NSMutableArray<UIView *> *effectQueue = [NSMutableArray arrayWithArray:carrier.subviews];
    NSUInteger effects = 0;
    for (NSUInteger i = 0; i < effectQueue.count && i < 80; i++) {
        UIView *view = effectQueue[i];
        if (ALBIsLens(view) || view == fallback) continue;
        [effectQueue addObjectsFromArray:view.subviews];
        if (![view isKindOfClass:UIVisualEffectView.class]) continue;
        UIVisualEffect *effect = ((UIVisualEffectView *)view).effect;
        NSString *name = NSStringFromClass(effect.class);
        if ([name containsString:@"GlassEffect"] && ![name containsString:@"Container"]) {
            ALBMaterialApplyEffectView((UIVisualEffectView *)view, YES); effects++;
        }
    }
    if (effects) { [fallback removeFromSuperview]; objc_setAssociatedObject(carrier, &ALBFallbackKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC); ALBSetAudit(carrier, @"native-effect-view", effects); return effects; }
    SEL getter = NSSelectorFromString(@"glassEffect"), setter = NSSelectorFromString(@"setGlassEffect:");
    if ([carrier respondsToSelector:getter] && [carrier respondsToSelector:setter]) {
        Method get = class_getInstanceMethod(carrier.class, getter), set = class_getInstanceMethod(carrier.class, setter);
        char result[16] = {0}, argument[16] = {0};
        if (get && set && method_getNumberOfArguments(get) == 2 && method_getNumberOfArguments(set) == 3) {
            method_getReturnType(get, result, sizeof(result)); method_getArgumentType(set, 2, argument, sizeof(argument));
        }
        if (result[0] == '@' && argument[0] == '@') {
            NSString *signature = ALBSignature(carrier, YES);
            if (![objc_getAssociatedObject(carrier, &ALBSignatureKey) isEqual:signature]) {
                id original = ((id (*)(id,SEL))objc_msgSend)(carrier, getter);
                if (!objc_getAssociatedObject(carrier, &ALBOriginalEffectKey))
                    objc_setAssociatedObject(carrier, &ALBOriginalEffectKey, original ?: NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                ((void (*)(id,SEL,id))objc_msgSend)(carrier, setter, ALBEffectForSurface(carrier, YES, YES));
                objc_setAssociatedObject(carrier, &ALBInstalledEffectKey, ((id (*)(id,SEL))objc_msgSend)(carrier,getter), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                objc_setAssociatedObject(carrier, &ALBSignatureKey, signature, OBJC_ASSOCIATION_COPY_NONATOMIC);
            }
            ALBSetAudit(carrier, @"native-glassEffect", 1); return 1;
        }
    }
    NSArray<CALayer *> *layers = ALBRenderLayers(carrier);
    if (!layers.count) { ALBSetAudit(carrier, @"unresolved-native-renderer", 0); return 0; }
    NSMutableArray *leases = [NSMutableArray array];
    for (CALayer *layer in layers) {
        ALBMaterialLayerLease *lease = [ALBMaterialLayerLease new]; lease.layer = layer;
        lease.opacity = layer.opacity; lease.values = @{}; [leases addObject:lease];
        if (layer.opacity != 0) layer.opacity = 0;
    }
    objc_setAssociatedObject(carrier, &ALBLeasesKey, leases, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (!fallback) {
        fallback = [[UIVisualEffectView alloc] initWithEffect:nil];
        fallback.userInteractionEnabled = NO; fallback.contentView.userInteractionEnabled = NO;
        fallback.accessibilityIdentifier = @"AwemeLiquidBar.SharedMaterial.Renderer";
        [carrier insertSubview:fallback atIndex:0];
        objc_setAssociatedObject(carrier, &ALBFallbackKey, fallback, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    // Only the carrier's own model bounds. No window-scaled CGRect feeds back
    // into layout, and native collapse/press transforms remain on the carrier.
    fallback.frame = carrier.bounds; fallback.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    fallback.layer.cornerRadius = CGRectGetHeight(carrier.bounds) / 2;
    fallback.layer.cornerCurve = kCACornerCurveContinuous;
    if (@available(iOS 26.0, *))
        fallback.cornerConfiguration = [UICornerConfiguration capsuleConfiguration];
    fallback.clipsToBounds = NO;
    ALBMaterialApplyEffectView(fallback, YES);
    ALBSetAudit(carrier, native26 ? @"native26-clear-effect-fallback" : @"public-UIGlassEffect-renderer", layers.count);
    return layers.count;
}

void ALBMaterialApplyBar(UITabBar *bar, BOOL bottom) {
    if (!bar || !NSThread.isMainThread) return;
    if ([objc_getAssociatedObject(bar, &ALBApplyingKey) boolValue]) return;
    objc_setAssociatedObject(bar, &ALBApplyingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    CATransaction.begin(); CATransaction.setDisableActions(YES);
    ALBRegister(bar, bottom ? @"bar.bottom" : @"bar.top"); ALBApplyStyle(bar, YES);
    if (bottom) ALBBottomSource = bar;
    BOOL native26 = ALBMaterialEnabled() && ALBMaterialMode() == ALBMaterialNative26;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:bar];
    NSUInteger applied = 0; CGFloat recipeWidth = 0;
    NSMutableDictionary *recipe = [NSMutableDictionary dictionary];
    for (NSUInteger i = 0; i < queue.count && i < 240; i++) {
        UIView *view = queue[i]; if (ALBIsLens(view)) continue;
        if ([view.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.SharedMaterial.Renderer"]) continue;
        NSString *name = NSStringFromClass(view.class);
        if ([name containsString:@"PlatterView"] || [name containsString:@"SearchPill"]) {
            ALBApplyStyle(view, YES);
            if (native26 && bottom) {
                ALBRestoreCarrier(view);
                NSMutableDictionary *candidate = [NSMutableDictionary dictionary];
                for (CALayer *layer in ALBRenderLayers(view)) {
                    NSDictionary *values = ALBLayerValues(layer);
                    if (values.count) candidate[NSStringFromClass(layer.class)] = values;
                }
                // Use the primary, widest bottom platter as the reference.
                // An auxiliary search pill must not overwrite its recipe.
                if (candidate.count && CGRectGetWidth(view.bounds) >= recipeWidth) {
                    recipe = candidate; recipeWidth = CGRectGetWidth(view.bounds);
                }
                // Restore any previously replaced effect-view recipe too.
                NSMutableArray *restoreQueue = [NSMutableArray arrayWithArray:view.subviews];
                for (NSUInteger k = 0; k < restoreQueue.count && k < 80; k++) {
                    UIView *v = restoreQueue[k]; if (ALBIsLens(v)) continue;
                    [restoreQueue addObjectsFromArray:v.subviews];
                    if (![v isKindOfClass:UIVisualEffectView.class]) continue;
                    id original = objc_getAssociatedObject(v, &ALBOriginalEffectKey);
                    if (original) { ((UIVisualEffectView *)v).effect = original == NSNull.null ? nil : original;
                        objc_setAssociatedObject(v, &ALBSignatureKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                        [ALBSurfaces removeObject:v]; }
                }
                ALBSetAudit(view, @"preserved-bottom-native26", recipe.count); continue;
            }
            NSUInteger count = ALBApplyCarrier(view, native26); applied += count;
            if (!count) [queue addObjectsFromArray:view.subviews];
        } else if ([view isKindOfClass:UIVisualEffectView.class]) {
            UIVisualEffect *effect = ((UIVisualEffectView *)view).effect;
            NSString *type = NSStringFromClass(effect.class);
            if ([type containsString:@"GlassEffect"] && ![type containsString:@"Container"]) {
                if (native26 && bottom) {
                    id original = objc_getAssociatedObject(view, &ALBOriginalEffectKey);
                    if (original) ((UIVisualEffectView *)view).effect = original == NSNull.null ? nil : original;
                    [ALBSurfaces removeObject:view]; continue;
                }
                ALBMaterialApplyEffectView((UIVisualEffectView *)view, YES); applied++;
            } else [queue addObjectsFromArray:view.subviews];
        } else [queue addObjectsFromArray:view.subviews];
    }
    if (native26 && bottom && recipe.count) ALBBottomRecipe = [recipe copy];
    if (!native26) ALBBottomRecipe = nil;
    ALBSetAudit(bar, native26 && bottom ? @"preserved-bottom-native26" : applied ? @"shared-material" : @"unresolved-native-renderer", applied);
    CATransaction.commit();
    objc_setAssociatedObject(bar, &ALBApplyingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

void ALBMaterialRefresh(void) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{ ALBMaterialRefresh(); }); return; }
    ALBGeneration++;
    if (ALBBottomSource) ALBMaterialApplyBar(ALBBottomSource, YES);
    for (UIView *view in ALBSurfaces.allObjects) {
        NSString *role = objc_getAssociatedObject(view, &ALBRoleKey);
        if ([role hasPrefix:@"button"]) ALBMaterialApplyButton((UIButton *)view, [role hasSuffix:@"feed"]);
        else if ([role hasPrefix:@"effect"]) ALBMaterialApplyEffectView((UIVisualEffectView *)view, [role hasSuffix:@"feed"]);
        else if ([role hasPrefix:@"bar"]) ALBMaterialApplyBar((UITabBar *)view, [role hasSuffix:@"bottom"]);
        [view setNeedsLayout];
    }
}

NSDictionary *ALBMaterialSnapshot(void) {
    NSMutableArray *rows = [NSMutableArray array];
    for (UIView *view in ALBSurfaces.allObjects) {
        if (!view.window) continue;
        NSMutableDictionary *row = [@{@"role":objc_getAssociatedObject(view, &ALBRoleKey) ?: @"?",
            @"class":NSStringFromClass(view.class), @"style":@(view.overrideUserInterfaceStyle),
            @"alpha":@(view.alpha)} mutableCopy];
        NSDictionary *audit = objc_getAssociatedObject(view, &ALBAuditKey); if (audit) row[@"coverage"] = audit;
        if ([view isKindOfClass:UIVisualEffectView.class]) row[@"effect"] = ((UIVisualEffectView *)view).effect.description ?: @"nil";
        if ([view isKindOfClass:UIButton.class]) {
            UIButton *button = (UIButton *)view;
            row[@"effect"] = button.configuration.background.visualEffect.description ?: @"native-provider";
            row[@"menu"] = @(button.menu != nil);
            row[@"menuAsPrimaryAction"] = @(button.showsMenuAsPrimaryAction);
            row[@"targetCount"] = @(button.allTargets.count);
            row[@"interaction"] = @(button.userInteractionEnabled);
        }
        if ([view isKindOfClass:UITabBar.class]) {
            NSMutableArray *carriers = [NSMutableArray array], *q = [NSMutableArray arrayWithObject:view];
            for (NSUInteger i = 0; i < q.count && i < 240; i++) {
                UIView *v = q[i]; [q addObjectsFromArray:v.subviews];
                NSDictionary *a = objc_getAssociatedObject(v, &ALBAuditKey);
                if (a && v != view) [carriers addObject:@{@"class":NSStringFromClass(v.class), @"coverage":a}];
            }
            row[@"carriers"] = carriers;
        }
        [rows addObject:row];
    }
    return @{@"enabled":@(ALBMaterialEnabled()), @"mode":@(ALBMaterialMode()),
        @"appearance":@([NSUserDefaults.standardUserDefaults integerForKey:ALBPref(@"appearance")]),
        @"feedDark":@(ALB5Bool(@"glass.feedDark",YES)), @"systemStyle":@(UIScreen.mainScreen.traitCollection.userInterfaceStyle),
        @"tint":ALBMaterialTint().description ?: @"nil", @"reduceTransparency":@(UIAccessibilityIsReduceTransparencyEnabled()),
        @"generation":@(ALBGeneration), @"surfaces":rows};
}

@implementation ALBMaterialSettingsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad { [super viewDidLoad]; self.title = @"统一液态玻璃"; }
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 6; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 0 || section == 3) return 1;
    if (section == 1) return NSProcessInfo.processInfo.operatingSystemVersion.majorVersion == 26 ? 3 : 2;
    if (section == 2) return 4;
    if (section == 4) return 6;
    return 1;
}
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return @[@"总开关", @"所有玻璃的材质", @"深浅色来源", @"视频页保护", @"Regular 色调", @"当前状态"][section];
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    if (section == 1) return @"Regular 在系统最透明时仍有扩散。Clear 使用无色调的原生 Clear；系统透明度/色调偏好和降低透明度仍可能使其更不透明。iOS 26 原生模式保留底栏，并同步可读取的材质配方。";
    if (section == 2) return @"跟随系统直接读取屏幕的深浅色，不受抖音窗口固定浅色影响。首页和朋友视频页保护开启时优先保持深色。";
    if (section == 3) return @"仅保护首页与朋友视频流；商城、图文网格、消息、个人主页及弹出的设置/评论面板使用所选来源。";
    if (section == 4) return @"色调强度仅设置玻璃 tintColor，不伪造透明度。Clear 始终 tintColor=nil。系统玻璃滑块仍由 iOS 设置控制。";
    return nil;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.numberOfLines = 0; cell.detailTextLabel.numberOfLines = 0;
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    if (path.section == 0 || path.section == 3) {
        UISwitch *toggle = [UISwitch new]; toggle.tag = path.section;
        toggle.on = path.section == 0 ? ALBMaterialEnabled() : ALB5Bool(@"glass.feedDark", YES);
        [toggle addTarget:self action:@selector(toggle:) forControlEvents:UIControlEventValueChanged];
        cell.accessoryView = toggle; cell.textLabel.text = path.section == 0 ? @"统一材质、色调与深浅色" : @"首页 / 朋友视频页始终深色";
    } else if (path.section == 1) {
        cell.textLabel.text = @[@"Regular · 系统常规玻璃", @"Clear · 无色调透明玻璃", @"iOS 26 · 保留底栏高透明材质"][path.row];
        if (ALBMaterialMode() == path.row) cell.accessoryType = UITableViewCellAccessoryCheckmark;
    } else if (path.section == 2) {
        cell.textLabel.text = @[@"跟随系统", @"跟随抖音", @"始终浅色", @"始终深色"][path.row];
        if ([defaults integerForKey:ALBPref(@"appearance")] == path.row) cell.accessoryType = UITableViewCellAccessoryCheckmark;
    } else if (path.section == 4 && path.row < 5) {
        cell.textLabel.text = @[@"无色调", @"随深浅色调节", @"白色", @"黑色", @"蓝色"][path.row];
        if ([defaults integerForKey:ALBPref(@"tint")] == path.row) cell.accessoryType = UITableViewCellAccessoryCheckmark;
    } else if (path.section == 4) {
        UISlider *slider = [UISlider new]; slider.minimumValue = 0; slider.maximumValue = .6;
        slider.value = ALB5Number(@"glass.tintStrength", .12, 0, .6);
        [slider addTarget:self action:@selector(strength:) forControlEvents:UIControlEventValueChanged];
        cell.textLabel.text = @"色调强度"; cell.accessoryView = slider; slider.frame = CGRectMake(0,0,160,32);
    } else {
        cell.textLabel.text = @"查看 / 复制材质状态"; cell.accessoryType = UITableViewCellAccessoryDisclosureIndicator;
        cell.detailTextLabel.text = @"同时保存进现有复现记录，包含实际覆盖路径及未适配的渲染器。";
    }
    if (path.section == 4 && ALBMaterialMode() != ALBMaterialRegular) {
        cell.userInteractionEnabled = NO; cell.textLabel.textColor = UIColor.secondaryLabelColor;
        if ([cell.accessoryView isKindOfClass:UIControl.class]) ((UIControl *)cell.accessoryView).enabled = NO;
    }
    return cell;
}
- (void)changed { [NSUserDefaults.standardUserDefaults synchronize]; [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil]; [self.tableView reloadData]; }
- (void)toggle:(UISwitch *)sender { [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:ALBPref(sender.tag == 0 ? @"enabled" : @"feedDark")]; [self changed]; }
- (void)strength:(UISlider *)sender { [NSUserDefaults.standardUserDefaults setDouble:sender.value forKey:ALBPref(@"tintStrength")]; [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil]; }
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    NSUserDefaults *defaults = NSUserDefaults.standardUserDefaults;
    if (path.section == 1) {
        [defaults setInteger:path.row forKey:ALBPref(@"mode")];
        [defaults setBool:path.row == ALBMaterialClear forKey:ALB5Key(@"bottom.clearMode")];
    } else if (path.section == 2) [defaults setInteger:path.row forKey:ALBPref(@"appearance")];
    else if (path.section == 4 && path.row < 5) [defaults setInteger:path.row forKey:ALBPref(@"tint")];
    else if (path.section == 5) {
        NSData *data = [NSJSONSerialization dataWithJSONObject:ALBMaterialSnapshot() options:NSJSONWritingPrettyPrinted error:nil];
        NSString *value = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"实际材质状态" message:value preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"复制" style:UIAlertActionStyleDefault handler:^(__unused UIAlertAction *a) { UIPasteboard.generalPasteboard.string = value; }]];
        [alert addAction:[UIAlertAction actionWithTitle:@"关闭" style:UIAlertActionStyleCancel handler:nil]];
        [self presentViewController:alert animated:YES completion:nil]; return;
    } else return;
    [self changed];
}
@end
