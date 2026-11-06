/* 背景探针 (background probe). Included once from Tweak.x after
   ALB5Settings.h; plain Objective-C, no Logos.

   UI (only while the "probe.enabled" option is on):
   A draggable 44 pt "探" button lives in its own pass-through UIWindow above
   the app (and above LiquidBar's own overlay window, so the bottom-bar area
   can be picked too). The window never becomes key, never drives the status
   bar, forwards orientation questions to the app, and answers hit tests only
   for its own controls; a hit test without an event (another component
   probing "is something covering this point?") always gets nil.
   Tap the button -> pick mode: a transparent full-screen overlay swallows
   the next tap. Every visible view under that point in the app's windows,
   plus every decorative non-view sublayer, is listed front-most first. The
   current entry is outlined (1.5 pt border + translucent fill drawn in the
   probe window, never on the target) and described in a compact panel.

   Rules (NSUserDefaults "AwemeLiquidBar.probe.rules", always active while
   LiquidBar itself is enabled, independent of the button):
   - mode "bg"    clear the view's own background (colour, layer colour,
                  gradient colours, effect, layer contents, a bare image
                  view's image, decorative sublayers covering >= 60 %);
   - mode "view"  hide the whole view (hidden = YES);
   - mode "layer" hide one decorative sublayer of the view.
   A rule names the exact class, its nearest ancestor whose class is not a
   system (UIKit/Core Animation/...) class (the "anchor"), the exact class
   path between them and the size on a 4 pt grid (+-6 pt). Rules are applied
   from per-class hooks installed once on the anchor classes only
   (-layoutSubviews and -didMoveToWindow; plus -setBackgroundColor: on a
   non-system target class of a "bg" rule). An inherited method is added to
   that class and calls the superclass implementation; an own method is
   wrapped. Nothing hooks UIView globally.
   Application only writes values that differ from the wanted value, is never
   re-entrant, records the original values for undo, and stops touching an
   instance that Douyin keeps resetting (> 40 rewrites in 2 s), so it can
   never form a layout loop with Douyin. */

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#include <dlfcn.h>
#include <string.h>
#include <stdlib.h>

static NSString *const kALBProbeRulesKey = @"AwemeLiquidBar.probe.rules";
static NSString *const kALBProbeCenterKey = @"AwemeLiquidBar.probe.buttonCenter";
static NSString *const kALBProbeSectionTitle = @"背景探针";
static NSString *const kALBProbeFileName = @"AwemeLiquidBar_BackgroundProbe.json";
static const CGFloat kALBProbeGrid = 4.0;
static const CGFloat kALBProbeTolerance = 6.0;
static char kALBProbeStateKey;

typedef NS_ENUM(NSInteger, ALBProbeRuleKind) {
    ALBProbeRuleBackground = 0,
    ALBProbeRuleView = 1,
    ALBProbeRuleLayer = 2,
};

@interface ALBProbeWindow : UIWindow
@property(nonatomic, copy) BOOL (^handlesPoint)(CGPoint point);
@end

#pragma mark - Class classification

static const char *ALBProbeOwnImage(void) {
    static const char *image = NULL;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        Dl_info info;
        if (dladdr((const void *)&ALBProbeOwnImage, &info) && info.dli_fname)
            image = strdup(info.dli_fname);
    });
    return image;
}

/* A class compiled into this tweak (ObjC or Swift). */
static BOOL ALBProbeClassIsPlugin(Class cls) {
    const char *own = ALBProbeOwnImage();
    const char *image = cls ? class_getImageName(cls) : NULL;
    return own && image && strcmp(own, image) == 0;
}

/* UIKit / Core Animation / other OS classes: never an anchor, never hooked. */
static BOOL ALBProbeClassIsSystem(Class cls) {
    if (!cls) return YES;
    const char *name = class_getName(cls);
    static const char *prefixes[] = {"UI", "_UI", "CA", "_CA", "NS", "_NS", "WK", "_WK",
        "SwiftUI", "_TtC7SwiftUI", "_TtGC7SwiftUI"};
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); i++)
        if (strncmp(name, prefixes[i], strlen(prefixes[i])) == 0) return YES;
    const char *image = class_getImageName(cls);
    return image && (strncmp(image, "/System/", 8) == 0 || strncmp(image, "/usr/lib/", 9) == 0);
}

static BOOL ALBProbeChainIsPlugin(UIView *view) {
    for (UIView *node = view; node; node = node.superview)
        if (ALBProbeClassIsPlugin([node class])) return YES;
    return NO;
}

#pragma mark - Text helpers

static BOOL ALBProbeColorIsClear(CGColorRef color) {
    return !color || CGColorGetAlpha(color) <= 0.01;
}

static NSString *ALBProbeColorText(CGColorRef color) {
    if (!color) return @"无";
    UIColor *value = [UIColor colorWithCGColor:color];
    CGFloat r = 0, g = 0, b = 0, a = 0;
    if ([value getRed:&r green:&g blue:&b alpha:&a]) {
        return [NSString stringWithFormat:@"RGBA(%.2f, %.2f, %.2f, %.2f) #%02X%02X%02X", r, g, b, a,
            (unsigned)lround(MIN(1.0, MAX(0.0, r)) * 255.0),
            (unsigned)lround(MIN(1.0, MAX(0.0, g)) * 255.0),
            (unsigned)lround(MIN(1.0, MAX(0.0, b)) * 255.0)];
    }
    return [NSString stringWithFormat:@"特殊颜色（不透明度 %.2f）", CGColorGetAlpha(color)];
}

static NSString *ALBProbeViewColorText(UIView *view) {
    UIColor *color = view.backgroundColor;
    if (!color) return @"无";
    return ALBProbeColorText([color resolvedColorWithTraitCollection:view.traitCollection].CGColor);
}

static double ALBProbeNumber(CGFloat value) { return isfinite(value) ? round(value * 10.0) / 10.0 : 0.0; }

static NSString *ALBProbeRectText(CGRect rect) {
    return [NSString stringWithFormat:@"x=%.1f y=%.1f 宽=%.1f 高=%.1f", ALBProbeNumber(rect.origin.x),
        ALBProbeNumber(rect.origin.y), ALBProbeNumber(rect.size.width), ALBProbeNumber(rect.size.height)];
}

static NSDictionary *ALBProbeRectJSON(CGRect rect) {
    return @{@"x": @(ALBProbeNumber(rect.origin.x)), @"y": @(ALBProbeNumber(rect.origin.y)),
             @"w": @(ALBProbeNumber(rect.size.width)), @"h": @(ALBProbeNumber(rect.size.height))};
}

static id ALBProbeColorJSON(CGColorRef color) {
    return color ? ALBProbeColorText(color) : (id)NSNull.null;
}

static CGFloat ALBProbeArea(CGRect rect) {
    return (CGRectIsNull(rect) || CGRectIsEmpty(rect)) ? 0 : rect.size.width * rect.size.height;
}

static NSString *ALBProbeModeName(NSString *mode) {
    if ([mode isEqual:@"view"]) return @"隐藏视图";
    if ([mode isEqual:@"layer"]) return @"隐藏图层";
    return @"隐藏背景";
}

#pragma mark - Rule model

@interface ALBProbeRule : NSObject
@property(nonatomic, copy) NSString *identifier;
@property(nonatomic) ALBProbeRuleKind kind;
@property(nonatomic, copy) NSString *targetName;
@property(nonatomic, copy) NSString *anchorName;
@property(nonatomic, copy) NSArray<NSString *> *pathNames;
@property(nonatomic, copy) NSString *layerName;
@property(nonatomic) CGSize size;
@property(nonatomic) CGSize layerSize;
@property(nonatomic) BOOL selfAnchored;
@property(nonatomic, assign) Class targetClass;
@property(nonatomic, assign) Class anchorClass;
@property(nonatomic, assign) Class layerClass;
@property(nonatomic, copy) NSArray *pathClasses;
@property(nonatomic) BOOL resolved;
+ (instancetype)ruleWithPlist:(NSDictionary *)plist;
@end

@implementation ALBProbeRule
+ (instancetype)ruleWithPlist:(NSDictionary *)plist {
    NSString *identifier = plist[@"id"], *mode = plist[@"mode"];
    NSString *target = plist[@"class"], *anchor = plist[@"anchor"];
    NSArray *path = plist[@"path"];
    if (![identifier isKindOfClass:NSString.class] || ![mode isKindOfClass:NSString.class] ||
        ![target isKindOfClass:NSString.class] || ![anchor isKindOfClass:NSString.class]) return nil;
    if (![path isKindOfClass:NSArray.class]) path = @[];
    for (id name in path) if (![name isKindOfClass:NSString.class]) return nil;
    ALBProbeRule *rule = [self new];
    rule.identifier = identifier;
    rule.kind = [mode isEqual:@"view"] ? ALBProbeRuleView
        : [mode isEqual:@"layer"] ? ALBProbeRuleLayer : ALBProbeRuleBackground;
    rule.targetName = target;
    rule.anchorName = anchor;
    rule.pathNames = path;
    rule.selfAnchored = [plist[@"self"] boolValue];
    rule.size = CGSizeMake([plist[@"w"] doubleValue], [plist[@"h"] doubleValue]);
    if (rule.kind == ALBProbeRuleLayer) {
        NSString *layer = plist[@"layerClass"];
        if (![layer isKindOfClass:NSString.class]) return nil;
        rule.layerName = layer;
        rule.layerSize = CGSizeMake([plist[@"lw"] doubleValue], [plist[@"lh"] doubleValue]);
    }
    Class targetClass = NSClassFromString(target), anchorClass = NSClassFromString(anchor);
    BOOL ok = targetClass && anchorClass && !ALBProbeClassIsSystem(anchorClass) &&
        !ALBProbeClassIsPlugin(anchorClass) && !ALBProbeClassIsPlugin(targetClass) &&
        [anchorClass isSubclassOfClass:UIView.class] && [targetClass isSubclassOfClass:UIView.class] &&
        (!rule.selfAnchored || targetClass == anchorClass);
    NSMutableArray *pathClasses = [NSMutableArray array];
    for (NSString *name in path) {
        Class cls = NSClassFromString(name);
        if (!cls) { ok = NO; break; }
        [pathClasses addObject:cls];
    }
    if (rule.kind == ALBProbeRuleLayer) {
        rule.layerClass = NSClassFromString(rule.layerName);
        if (!rule.layerClass) ok = NO;
    }
    rule.targetClass = targetClass;
    rule.anchorClass = anchorClass;
    rule.pathClasses = pathClasses;
    rule.resolved = ok;
    return rule;
}
@end

/* Per target instance: which rules hold it and what it looked like before. */
@interface ALBProbeTargetState : NSObject
@property(nonatomic, strong) NSMutableDictionary *originals;
@property(nonatomic, strong) NSMutableSet<NSString *> *backgroundRules;
@property(nonatomic, strong) NSMutableSet<NSString *> *hideRules;
@property(nonatomic, strong) NSMutableSet<NSString *> *layerRules;
@property(nonatomic, strong) NSHashTable *hiddenSublayers;
@property(nonatomic) NSUInteger changes;
@property(nonatomic) CFTimeInterval windowStart;
@property(nonatomic) BOOL gaveUp;
@end

@implementation ALBProbeTargetState
- (instancetype)init {
    if ((self = [super init])) {
        _originals = [NSMutableDictionary dictionary];
        _backgroundRules = [NSMutableSet set];
        _hideRules = [NSMutableSet set];
        _layerRules = [NSMutableSet set];
        _hiddenSublayers = [NSHashTable weakObjectsHashTable];
    }
    return self;
}
@end

static NSArray<ALBProbeRule *> *gALBProbeRules;
static NSMapTable *gALBProbeRulesByAnchor;          /* Class (pointer key) -> NSArray<ALBProbeRule *> */
static NSUInteger gALBProbeActiveRuleCount;
static NSMutableDictionary<NSString *, NSHashTable *> *gALBProbeInstances;      /* rule id -> weak views */
static NSMutableDictionary<NSString *, NSHashTable *> *gALBProbeLayerInstances; /* rule id -> weak layers */
static NSMutableSet<NSString *> *gALBProbeLayoutHooks;
static NSMutableSet<NSString *> *gALBProbeSetterHooks;
static NSHashTable *gALBProbePendingAnchors;
static BOOL gALBProbeFlushScheduled;
static BOOL gALBProbeApplying;
static BOOL gALBProbeAllowed;

#pragma mark - Applying and restoring

static ALBProbeTargetState *ALBProbeState(UIView *view, BOOL create) {
    ALBProbeTargetState *state = objc_getAssociatedObject(view, &kALBProbeStateKey);
    if (!state && create) {
        state = [ALBProbeTargetState new];
        objc_setAssociatedObject(view, &kALBProbeStateKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return state;
}

static BOOL ALBProbeStateHasRule(UIView *view, NSString *identifier) {
    ALBProbeTargetState *state = objc_getAssociatedObject(view, &kALBProbeStateKey);
    return state && ([state.backgroundRules containsObject:identifier] ||
        [state.hideRules containsObject:identifier] || [state.layerRules containsObject:identifier]);
}

static NSHashTable *ALBProbeTable(NSMutableDictionary<NSString *, NSHashTable *> *__strong *store,
                                  NSString *identifier) {
    if (!*store) *store = [NSMutableDictionary dictionary];
    NSHashTable *table = (*store)[identifier];
    if (!table) {
        table = [NSHashTable weakObjectsHashTable];
        (*store)[identifier] = table;
    }
    return table;
}

static BOOL ALBProbeIsBareImageView(UIView *view) {
    return [view isKindOfClass:UIImageView.class] && view.subviews.count == 0;
}

/* Layer contents of a text view is its text, not a background. */
static BOOL ALBProbeContentsAreBackground(UIView *view) {
    if ([view isKindOfClass:UILabel.class] || [view isKindOfClass:UITextView.class] ||
        [view isKindOfClass:UITextField.class]) return NO;
    NSString *name = NSStringFromClass([view class]).lowercaseString;
    return ![name containsString:@"label"] && ![name containsString:@"text"];
}

/* A sublayer that is not a subview's layer and only paints colour/image. */
static BOOL ALBProbeLayerIsDecorative(CALayer *layer) {
    if (!layer || [layer.delegate isKindOfClass:UIView.class]) return NO;
    if ([layer isKindOfClass:CAGradientLayer.class] || [layer isKindOfClass:CAShapeLayer.class]) return YES;
    if ([layer class] != CALayer.class) return NO;
    return layer.contents != nil || !ALBProbeColorIsClear(layer.backgroundColor);
}

static BOOL ALBProbeLayerCoversHost(CALayer *sublayer, CALayer *host) {
    CGFloat area = ALBProbeArea(host.bounds);
    return area > 1 && ALBProbeArea(CGRectIntersection(sublayer.frame, host.bounds)) >= area * 0.6;
}

static NSArray<NSString *> *ALBProbeOwnBackgroundParts(UIView *view) {
    NSMutableArray *parts = [NSMutableArray array];
    UIColor *color = view.backgroundColor;
    CALayer *layer = view.layer;
    if (color && !ALBProbeColorIsClear([color resolvedColorWithTraitCollection:view.traitCollection].CGColor))
        [parts addObject:@"背景色"];
    else if (!ALBProbeColorIsClear(layer.backgroundColor)) [parts addObject:@"图层背景"];
    if ([layer isKindOfClass:CAGradientLayer.class] && ((CAGradientLayer *)layer).colors.count)
        [parts addObject:@"渐变"];
    if ([view isKindOfClass:UIVisualEffectView.class] && ((UIVisualEffectView *)view).effect)
        [parts addObject:@"毛玻璃／材质"];
    if ([view isKindOfClass:UIImageView.class] && (((UIImageView *)view).image || layer.contents))
        [parts addObject:@"图片"];
    else if (layer.contents && ALBProbeContentsAreBackground(view)) [parts addObject:@"图层内容"];
    NSUInteger decorative = 0;
    for (CALayer *sublayer in layer.sublayers)
        if (!sublayer.hidden && ALBProbeLayerIsDecorative(sublayer) && ALBProbeLayerCoversHost(sublayer, layer))
            decorative++;
    if (decorative) [parts addObject:[NSString stringWithFormat:@"装饰图层×%lu", (unsigned long)decorative]];
    return parts;
}

static BOOL ALBProbeClearBackground(UIView *view, ALBProbeTargetState *state) {
    BOOL changed = NO;
    NSMutableDictionary *originals = state.originals;
    CALayer *layer = view.layer;
    UIColor *color = view.backgroundColor;
    if (color && !ALBProbeColorIsClear([color resolvedColorWithTraitCollection:view.traitCollection].CGColor)) {
        originals[@"bg"] = color;
        view.backgroundColor = UIColor.clearColor;
        changed = YES;
    }
    CGColorRef layerColor = layer.backgroundColor;
    if (!ALBProbeColorIsClear(layerColor)) {
        originals[@"layerBg"] = (__bridge id)layerColor;
        layer.backgroundColor = UIColor.clearColor.CGColor;
        changed = YES;
    }
    if ([layer isKindOfClass:CAGradientLayer.class] && ((CAGradientLayer *)layer).colors.count) {
        originals[@"colors"] = ((CAGradientLayer *)layer).colors;
        ((CAGradientLayer *)layer).colors = nil;
        changed = YES;
    }
    if ([view isKindOfClass:UIVisualEffectView.class] && ((UIVisualEffectView *)view).effect) {
        originals[@"effect"] = ((UIVisualEffectView *)view).effect;
        ((UIVisualEffectView *)view).effect = nil;
        changed = YES;
    }
    if (!ALBProbeIsBareImageView(view) && layer.contents && ALBProbeContentsAreBackground(view)) {
        originals[@"contents"] = layer.contents;
        layer.contents = nil;
        changed = YES;
    }
    for (CALayer *sublayer in layer.sublayers) {
        if (sublayer.hidden || !ALBProbeLayerIsDecorative(sublayer) || !ALBProbeLayerCoversHost(sublayer, layer)) continue;
        [state.hiddenSublayers addObject:sublayer];
        sublayer.hidden = YES;
        changed = YES;
    }
    return changed;
}

/* Brings one instance to the state its rules ask for; writes only differences. */
static BOOL ALBProbeEnforce(UIView *view, ALBProbeTargetState *state) {
    BOOL changed = NO;
    BOOL wantHidden = state.hideRules.count || (state.backgroundRules.count && ALBProbeIsBareImageView(view));
    if (wantHidden && !view.hidden) {
        if (!state.originals[@"hidden"]) state.originals[@"hidden"] = @NO;
        view.hidden = YES;
        changed = YES;
    }
    if (state.backgroundRules.count && ALBProbeClearBackground(view, state)) changed = YES;
    return changed;
}

static BOOL ALBProbeHideRuleLayers(UIView *host, ALBProbeRule *rule) {
    BOOL changed = NO;
    NSHashTable *table = ALBProbeTable(&gALBProbeLayerInstances, rule.identifier);
    for (CALayer *sublayer in host.layer.sublayers) {
        if ([sublayer class] != rule.layerClass || [sublayer.delegate isKindOfClass:UIView.class]) continue;
        CGSize size = sublayer.bounds.size;
        if (![table containsObject:sublayer] &&
            (fabs(size.width - rule.layerSize.width) > kALBProbeTolerance ||
             fabs(size.height - rule.layerSize.height) > kALBProbeTolerance)) continue;
        [table addObject:sublayer];
        if (!sublayer.hidden) {
            sublayer.hidden = YES;
            changed = YES;
        }
    }
    return changed;
}

static void ALBProbeNoteChange(ALBProbeTargetState *state) {
    CFTimeInterval now = CACurrentMediaTime();
    if (now - state.windowStart > 2.0) {
        state.windowStart = now;
        state.changes = 0;
    }
    /* Douyin keeps undoing this instance: stop instead of fighting it. */
    if (++state.changes > 40) state.gaveUp = YES;
}

static void ALBProbeApplyRule(UIView *view, ALBProbeRule *rule) {
    ALBProbeTargetState *state = ALBProbeState(view, YES);
    [ALBProbeTable(&gALBProbeInstances, rule.identifier) addObject:view];
    if (state.gaveUp) return;
    BOOL changed;
    if (rule.kind == ALBProbeRuleLayer) {
        [state.layerRules addObject:rule.identifier];
        changed = ALBProbeHideRuleLayers(view, rule);
    } else {
        if (rule.kind == ALBProbeRuleView) [state.hideRules addObject:rule.identifier];
        else [state.backgroundRules addObject:rule.identifier];
        changed = ALBProbeEnforce(view, state);
    }
    if (changed) ALBProbeNoteChange(state);
}

static void ALBProbeRestoreView(UIView *view, ALBProbeTargetState *state) {
    NSMutableDictionary *originals = state.originals;
    BOOL wantHidden = state.hideRules.count || (state.backgroundRules.count && ALBProbeIsBareImageView(view));
    if (!wantHidden && originals[@"hidden"]) {
        if (view.hidden) view.hidden = [originals[@"hidden"] boolValue];
        [originals removeObjectForKey:@"hidden"];
    }
    if (state.backgroundRules.count) return;
    CALayer *layer = view.layer;
    UIColor *color = originals[@"bg"];
    if (color) {
        UIColor *now = view.backgroundColor;
        if (!now || ALBProbeColorIsClear(now.CGColor)) view.backgroundColor = color;
        [originals removeObjectForKey:@"bg"];
    }
    id layerColor = originals[@"layerBg"];
    if (layerColor) {
        if (ALBProbeColorIsClear(layer.backgroundColor)) layer.backgroundColor = (__bridge CGColorRef)layerColor;
        [originals removeObjectForKey:@"layerBg"];
    }
    NSArray *colors = originals[@"colors"];
    if (colors) {
        if ([layer isKindOfClass:CAGradientLayer.class] && !((CAGradientLayer *)layer).colors.count)
            ((CAGradientLayer *)layer).colors = colors;
        [originals removeObjectForKey:@"colors"];
    }
    UIVisualEffect *effect = originals[@"effect"];
    if (effect) {
        if ([view isKindOfClass:UIVisualEffectView.class] && !((UIVisualEffectView *)view).effect)
            ((UIVisualEffectView *)view).effect = effect;
        [originals removeObjectForKey:@"effect"];
    }
    id contents = originals[@"contents"];
    if (contents) {
        if (!layer.contents) layer.contents = contents;
        [originals removeObjectForKey:@"contents"];
    }
    for (CALayer *sublayer in state.hiddenSublayers.allObjects) if (sublayer.hidden) sublayer.hidden = NO;
    [state.hiddenSublayers removeAllObjects];
}

/* Undo one rule on every instance it touched (rules that remain keep theirs). */
static void ALBProbeReleaseRule(NSString *identifier) {
    for (UIView *view in gALBProbeInstances[identifier].allObjects) {
        ALBProbeTargetState *state = objc_getAssociatedObject(view, &kALBProbeStateKey);
        if (!state) continue;
        [state.backgroundRules removeObject:identifier];
        [state.hideRules removeObject:identifier];
        [state.layerRules removeObject:identifier];
        ALBProbeRestoreView(view, state);
        if (!state.backgroundRules.count && !state.hideRules.count && !state.layerRules.count) {
            state.gaveUp = NO;
            state.changes = 0;
        }
    }
    for (CALayer *layer in gALBProbeLayerInstances[identifier].allObjects) if (layer.hidden) layer.hidden = NO;
    [gALBProbeInstances removeObjectForKey:identifier];
    [gALBProbeLayerInstances removeObjectForKey:identifier];
}

static void ALBProbeReleaseAllInstances(void) {
    NSMutableSet *identifiers = [NSMutableSet setWithArray:gALBProbeInstances.allKeys ?: @[]];
    [identifiers addObjectsFromArray:gALBProbeLayerInstances.allKeys ?: @[]];
    for (NSString *identifier in identifiers) ALBProbeReleaseRule(identifier);
}

#pragma mark - Matching from the anchor

static BOOL ALBProbeTryTarget(UIView *view, ALBProbeRule *rule) {
    if (!ALBProbeStateHasRule(view, rule.identifier)) {
        CGSize size = view.bounds.size;
        if (fabs(size.width - rule.size.width) > kALBProbeTolerance ||
            fabs(size.height - rule.size.height) > kALBProbeTolerance) return NO;
    }
    ALBProbeApplyRule(view, rule);
    return YES;
}

static BOOL ALBProbeWalk(UIView *view, ALBProbeRule *rule, NSUInteger level) {
    BOOL hit = NO;
    NSArray *path = rule.pathClasses;
    if (level >= path.count) {
        for (UIView *subview in view.subviews)
            if ([subview class] == rule.targetClass && ALBProbeTryTarget(subview, rule)) hit = YES;
        return hit;
    }
    Class wanted = (Class)path[level];
    for (UIView *subview in view.subviews)
        if ([subview class] == wanted && ALBProbeWalk(subview, rule, level + 1)) hit = YES;
    return hit;
}

/* YES when every rule found its target inside this anchor. */
static BOOL ALBProbeApplyAnchor(UIView *anchor, NSArray<ALBProbeRule *> *rules) {
    BOOL complete = YES;
    BOOL outer = !gALBProbeApplying;
    gALBProbeApplying = YES;
    for (ALBProbeRule *rule in rules) {
        if (!rule.resolved) continue;
        BOOL hit = rule.selfAnchored ? ALBProbeTryTarget(anchor, rule) : ALBProbeWalk(anchor, rule, 0);
        if (!hit) complete = NO;
    }
    if (outer) gALBProbeApplying = NO;
    return complete;
}

/* One extra pass after the current layout pass, when nested subviews have
   their final frames. Coalesced; never schedules itself again. */
static void ALBProbeScheduleAnchor(UIView *anchor) {
    if (!gALBProbePendingAnchors) gALBProbePendingAnchors = [NSHashTable weakObjectsHashTable];
    [gALBProbePendingAnchors addObject:anchor];
    if (gALBProbeFlushScheduled) return;
    gALBProbeFlushScheduled = YES;
    dispatch_async(dispatch_get_main_queue(), ^{
        gALBProbeFlushScheduled = NO;
        NSArray *anchors = gALBProbePendingAnchors.allObjects;
        [gALBProbePendingAnchors removeAllObjects];
        if (!gALBProbeAllowed || !gALBProbeActiveRuleCount) return;
        for (UIView *pending in anchors) {
            if (!pending.window) continue;
            NSArray *rules = [gALBProbeRulesByAnchor objectForKey:[pending class]];
            if (rules.count) ALBProbeApplyAnchor(pending, rules);
        }
    });
}

static void ALBProbeAnchorEvent(UIView *anchor, Class hooked, BOOL moved) {
    if (!gALBProbeActiveRuleCount || !gALBProbeAllowed || !anchor) return;
    if ([anchor class] != hooked || !NSThread.isMainThread || !anchor.window) return;
    NSArray *rules = [gALBProbeRulesByAnchor objectForKey:hooked];
    if (!rules.count) return;
    if (gALBProbeApplying) {
        ALBProbeScheduleAnchor(anchor);
        return;
    }
    BOOL complete = ALBProbeApplyAnchor(anchor, rules);
    if (moved || !complete) ALBProbeScheduleAnchor(anchor);
}

static UIColor *ALBProbeFilterBackground(UIView *view, UIColor *color) {
    if (!color || !gALBProbeAllowed || !NSThread.isMainThread) return color;
    ALBProbeTargetState *state = objc_getAssociatedObject(view, &kALBProbeStateKey);
    if (!state || state.gaveUp || !state.backgroundRules.count || ALBProbeColorIsClear(color.CGColor)) return color;
    state.originals[@"bg"] = color;
    return UIColor.clearColor;
}

#pragma mark - Per-class hooks (installed once per class, only for classes named in rules)

static Method ALBProbeOwnMethod(Class cls, SEL selector) {
    unsigned int count = 0;
    Method *methods = class_copyMethodList(cls, &count);
    Method found = NULL;
    for (unsigned int i = 0; i < count; i++) {
        if (method_getName(methods[i]) == selector) {
            found = methods[i];
            break;
        }
    }
    free(methods);
    return found;
}

static BOOL ALBProbeHookVoid(Class cls, SEL selector, void (^after)(UIView *view)) {
    Method own = ALBProbeOwnMethod(cls, selector);
    if (!own) {
        Method inherited = class_getInstanceMethod(cls, selector);
        if (!inherited) return NO;
        Class superclass = class_getSuperclass(cls);
        IMP added = imp_implementationWithBlock(^(UIView *view) {
            IMP superIMP = class_getMethodImplementation(superclass, selector);
            if (superIMP) ((void (*)(id, SEL))superIMP)(view, selector);
            after(view);
        });
        if (class_addMethod(cls, selector, added, method_getTypeEncoding(inherited))) return YES;
        imp_removeBlock(added);
        own = ALBProbeOwnMethod(cls, selector);
        if (!own) return NO;
    }
    IMP previous = method_getImplementation(own);
    method_setImplementation(own, imp_implementationWithBlock(^(UIView *view) {
        ((void (*)(id, SEL))previous)(view, selector);
        after(view);
    }));
    return YES;
}

static BOOL ALBProbeHookBackgroundSetter(Class cls) {
    SEL selector = @selector(setBackgroundColor:);
    Method own = ALBProbeOwnMethod(cls, selector);
    if (!own) {
        Method inherited = class_getInstanceMethod(cls, selector);
        if (!inherited) return NO;
        Class superclass = class_getSuperclass(cls);
        IMP added = imp_implementationWithBlock(^(UIView *view, UIColor *color) {
            IMP superIMP = class_getMethodImplementation(superclass, selector);
            if (superIMP) ((void (*)(id, SEL, UIColor *))superIMP)(view, selector, ALBProbeFilterBackground(view, color));
        });
        if (class_addMethod(cls, selector, added, method_getTypeEncoding(inherited))) return YES;
        imp_removeBlock(added);
        own = ALBProbeOwnMethod(cls, selector);
        if (!own) return NO;
    }
    IMP previous = method_getImplementation(own);
    method_setImplementation(own, imp_implementationWithBlock(^(UIView *view, UIColor *color) {
        ((void (*)(id, SEL, UIColor *))previous)(view, selector, ALBProbeFilterBackground(view, color));
    }));
    return YES;
}

static void ALBProbeInstallHooks(ALBProbeRule *rule) {
    if (!gALBProbeLayoutHooks) gALBProbeLayoutHooks = [NSMutableSet set];
    if (!gALBProbeSetterHooks) gALBProbeSetterHooks = [NSMutableSet set];
    Class anchor = rule.anchorClass;
    NSString *anchorKey = NSStringFromClass(anchor);
    if (![gALBProbeLayoutHooks containsObject:anchorKey]) {
        [gALBProbeLayoutHooks addObject:anchorKey];
        ALBProbeHookVoid(anchor, @selector(layoutSubviews), ^(UIView *view) { ALBProbeAnchorEvent(view, anchor, NO); });
        ALBProbeHookVoid(anchor, @selector(didMoveToWindow), ^(UIView *view) { ALBProbeAnchorEvent(view, anchor, YES); });
    }
    if (rule.kind == ALBProbeRuleBackground && !ALBProbeClassIsSystem(rule.targetClass)) {
        NSString *targetKey = NSStringFromClass(rule.targetClass);
        if (![gALBProbeSetterHooks containsObject:targetKey]) {
            [gALBProbeSetterHooks addObject:targetKey];
            ALBProbeHookBackgroundSetter(rule.targetClass);
        }
    }
}

#pragma mark - Rule storage

static NSArray<NSDictionary *> *ALBProbeStoredRules(void) {
    NSArray *raw = [NSUserDefaults.standardUserDefaults arrayForKey:kALBProbeRulesKey];
    NSMutableArray *rules = [NSMutableArray array];
    for (id item in raw)
        if ([item isKindOfClass:NSDictionary.class] && [item[@"id"] isKindOfClass:NSString.class]) [rules addObject:item];
    return rules;
}

static void ALBProbeStoreRules(NSArray *rules) {
    if (rules.count) [NSUserDefaults.standardUserDefaults setObject:rules forKey:kALBProbeRulesKey];
    else [NSUserDefaults.standardUserDefaults removeObjectForKey:kALBProbeRulesKey];
}

static void ALBProbeCompileRules(void) {
    NSMutableArray *compiled = [NSMutableArray array];
    NSMapTable *byAnchor = [NSMapTable mapTableWithKeyOptions:NSPointerFunctionsOpaqueMemory | NSPointerFunctionsOpaquePersonality
                                                 valueOptions:NSPointerFunctionsStrongMemory];
    NSUInteger active = 0;
    for (NSDictionary *plist in ALBProbeStoredRules()) {
        ALBProbeRule *rule = [ALBProbeRule ruleWithPlist:plist];
        if (!rule) continue;
        [compiled addObject:rule];
        if (!rule.resolved) continue;
        NSMutableArray *list = [byAnchor objectForKey:rule.anchorClass];
        if (!list) {
            list = [NSMutableArray array];
            [byAnchor setObject:list forKey:rule.anchorClass];
        }
        [list addObject:rule];
        ALBProbeInstallHooks(rule);
        active++;
    }
    gALBProbeRules = compiled;
    gALBProbeRulesByAnchor = byAnchor;
    gALBProbeActiveRuleCount = active;
}

/* Front-most first; LiquidBar's probe window and keyboard windows excluded. */
static NSArray<UIWindow *> *ALBProbeAppWindows(void) {
    NSMutableArray<UIWindow *> *windows = [NSMutableArray array];
    for (UIScene *scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class]) continue;
        if (scene.activationState != UISceneActivationStateForegroundActive &&
            scene.activationState != UISceneActivationStateForegroundInactive) continue;
        for (UIWindow *window in ((UIWindowScene *)scene).windows.reverseObjectEnumerator) {
            if ([window isKindOfClass:ALBProbeWindow.class] || window.hidden || window.alpha <= 0.01) continue;
            NSString *name = NSStringFromClass([window class]);
            if ([name containsString:@"Keyboard"] || [name containsString:@"TextEffects"]) continue;
            [windows addObject:window];
        }
    }
    [windows sortWithOptions:NSSortStable usingComparator:^NSComparisonResult(UIWindow *a, UIWindow *b) {
        if (a.windowLevel > b.windowLevel) return NSOrderedAscending;
        if (a.windowLevel < b.windowLevel) return NSOrderedDescending;
        return NSOrderedSame;
    }];
    return windows;
}

static void ALBProbeSweepView(UIView *view, NSUInteger depth) {
    if (!view || depth > 200) return;
    NSArray *rules = [gALBProbeRulesByAnchor objectForKey:[view class]];
    if (rules.count) ALBProbeApplyAnchor(view, rules);
    for (UIView *subview in view.subviews) ALBProbeSweepView(subview, depth + 1);
}

/* Whole-hierarchy pass: only at start-up, after a rule is added and when
   LiquidBar is switched back on. Layout hooks do everything else. */
static void ALBProbeSweep(void) {
    if (!gALBProbeAllowed || !gALBProbeActiveRuleCount) return;
    for (UIWindow *window in ALBProbeAppWindows()) {
        if (ALBProbeClassIsPlugin([window class])) continue;
        ALBProbeSweepView(window, 0);
    }
}

/* Fills the identity of a rule for `view`; returns an error text or nil. */
static NSString *ALBProbeIdentity(UIView *view, NSMutableDictionary *rule) {
    if (!view) return @"视图已经释放，请重新点选";
    if ([view isKindOfClass:UIWindow.class]) return @"窗口本身不能添加规则";
    if (ALBProbeChainIsPlugin(view)) return @"这是 LiquidBar 插件自己的界面，不能添加规则";
    Class cls = [view class];
    NSMutableArray<NSString *> *path = [NSMutableArray array];
    UIView *anchor = nil;
    for (UIView *parent = view.superview; parent && ![parent isKindOfClass:UIWindow.class]; parent = parent.superview) {
        if (!ALBProbeClassIsSystem([parent class])) {
            anchor = parent;
            break;
        }
        NSString *name = NSStringFromClass([parent class]);
        if (NSClassFromString(name) != [parent class]) return @"路径中的类名无法在运行时还原";
        [path insertObject:name atIndex:0];
    }
    BOOL selfAnchored = NO;
    if (!anchor) {
        if (ALBProbeClassIsSystem(cls)) return @"附近没有可识别的抖音父视图";
        anchor = view;
        selfAnchored = YES;
        [path removeAllObjects];
    }
    NSString *name = NSStringFromClass(cls), *anchorName = NSStringFromClass([anchor class]);
    if (NSClassFromString(name) != cls || NSClassFromString(anchorName) != [anchor class])
        return @"该类名无法在运行时还原";
    rule[@"class"] = name;
    rule[@"anchor"] = anchorName;
    rule[@"path"] = [path copy];
    rule[@"w"] = @(round(view.bounds.size.width / kALBProbeGrid) * kALBProbeGrid);
    rule[@"h"] = @(round(view.bounds.size.height / kALBProbeGrid) * kALBProbeGrid);
    if (selfAnchored) rule[@"self"] = @YES;
    return nil;
}

static BOOL ALBProbeSameRule(NSDictionary *a, NSDictionary *b) {
    for (NSString *key in @[@"mode", @"class", @"anchor", @"path", @"w", @"h", @"self", @"layerClass", @"lw", @"lh"]) {
        id x = a[key], y = b[key];
        if (!x && !y) continue;
        if (![x isEqual:y]) return NO;
    }
    return YES;
}

static NSString *ALBProbeAddRule(NSMutableDictionary *rule, UIView *view, CALayer *layer) {
    NSMutableArray *rules = [ALBProbeStoredRules() mutableCopy];
    for (NSDictionary *existing in rules)
        if (ALBProbeSameRule(existing, rule)) return @"已有相同的规则，无需重复添加";
    NSString *identifier = NSUUID.UUID.UUIDString;
    rule[@"id"] = identifier;
    rule[@"created"] = @((long long)NSDate.date.timeIntervalSince1970);
    [rules addObject:[rule copy]];
    ALBProbeStoreRules(rules);
    ALBProbeCompileRules();
    ALBProbeRule *compiled = nil;
    for (ALBProbeRule *candidate in gALBProbeRules)
        if ([candidate.identifier isEqualToString:identifier]) compiled = candidate;
    NSString *done = [NSString stringWithFormat:@"已%@，规则已保存（共 %lu 条）",
        ALBProbeModeName(rule[@"mode"]), (unsigned long)rules.count];
    if (!compiled.resolved) return [done stringByAppendingString:@"；但类名暂时无法解析，可能不生效"];
    /* Verify that the stored identity finds this very view from its anchor. */
    BOOL verified = NO;
    UIView *anchor = view;
    if (!compiled.selfAnchored)
        for (NSUInteger i = 0; i <= compiled.pathClasses.count && anchor; i++) anchor = anchor.superview;
    if (anchor && [anchor class] == compiled.anchorClass) {
        ALBProbeApplyAnchor(anchor, @[compiled]);
        verified = ALBProbeStateHasRule(view, identifier) &&
            (compiled.kind != ALBProbeRuleLayer || layer.hidden);
    }
    if (!verified) ALBProbeApplyRule(view, compiled);
    ALBProbeSweep();
    return verified ? done : [done stringByAppendingString:@"；定位校验未通过，重启后可能不生效"];
}

static NSDictionary *ALBProbeRemoveRuleWithIdentifier(NSString *identifier) {
    if (![identifier isKindOfClass:NSString.class]) return nil;
    NSMutableArray *rules = [ALBProbeStoredRules() mutableCopy];
    NSDictionary *removed = nil;
    for (NSUInteger i = 0; i < rules.count; i++) {
        if ([rules[i][@"id"] isEqual:identifier]) {
            removed = rules[i];
            [rules removeObjectAtIndex:i];
            break;
        }
    }
    if (!removed) return nil;
    ALBProbeStoreRules(rules);
    ALBProbeReleaseRule(identifier);
    ALBProbeCompileRules();
    return removed;
}

static NSDictionary *ALBProbeRemoveLastRule(void) {
    NSDictionary *last = ALBProbeStoredRules().lastObject;
    return last ? ALBProbeRemoveRuleWithIdentifier(last[@"id"]) : nil;
}

static NSUInteger ALBProbeRemoveAllRules(void) {
    NSArray *rules = ALBProbeStoredRules();
    for (NSDictionary *rule in rules) ALBProbeReleaseRule(rule[@"id"]);
    ALBProbeReleaseAllInstances();
    ALBProbeStoreRules(@[]);
    ALBProbeCompileRules();
    return rules.count;
}

static NSString *ALBProbeRuleTitle(NSDictionary *rule) {
    NSString *mode = rule[@"mode"];
    if ([mode isEqual:@"layer"])
        return [NSString stringWithFormat:@"隐藏图层 · %@（在 %@ 内）", rule[@"layerClass"], rule[@"class"]];
    return [NSString stringWithFormat:@"%@ · %@", ALBProbeModeName(mode), rule[@"class"]];
}

static NSString *ALBProbeRuleDetail(NSDictionary *rule) {
    NSArray *path = [rule[@"path"] isKindOfClass:NSArray.class] ? rule[@"path"] : @[];
    NSString *where = [rule[@"self"] boolValue] ? @"按自身类名定位"
        : [NSString stringWithFormat:@"定位 %@ › %lu 层", rule[@"anchor"], (unsigned long)path.count + 1];
    NSString *identifier = rule[@"id"];
    BOOL resolved = NO;
    for (ALBProbeRule *compiled in gALBProbeRules)
        if ([compiled.identifier isEqual:identifier]) resolved = compiled.resolved;
    NSUInteger hits = gALBProbeInstances[identifier].allObjects.count;
    return [NSString stringWithFormat:@"%@ · %.0f×%.0f · %@", where, [rule[@"w"] doubleValue], [rule[@"h"] doubleValue],
        resolved ? [NSString stringWithFormat:@"当前命中 %lu 个", (unsigned long)hits] : @"类尚未加载，等待生效"];
}

#pragma mark - Candidates

@interface ALBProbeCandidate : NSObject
@property(nonatomic, weak) UIView *view;      /* the view, or the host view of a layer */
@property(nonatomic, weak) CALayer *layer;    /* a decorative sublayer of `view` */
@property(nonatomic) BOOL isLayer;
@property(nonatomic) BOOL plugin;
@property(nonatomic, copy) NSString *name;
@end

@implementation ALBProbeCandidate
@end

/* Post-order, subviews front to back: front-most / deepest first. */
static void ALBProbeCollect(UIView *view, CGPoint point, NSMutableArray<ALBProbeCandidate *> *found,
                            NSUInteger depth, BOOL pluginChain) {
    if (!view || view.hidden || view.alpha <= 0.01 || depth > 160 || found.count >= 400) return;
    BOOL plugin = pluginChain || ALBProbeClassIsPlugin([view class]);
    BOOL inside = CGRectContainsPoint(view.bounds, point);
    if (!inside && view.clipsToBounds) return;
    for (UIView *subview in view.subviews.reverseObjectEnumerator)
        ALBProbeCollect(subview, [view convertPoint:point toView:subview], found, depth + 1, plugin);
    if (!inside) return;
    for (CALayer *sublayer in view.layer.sublayers.reverseObjectEnumerator) {
        if ([sublayer.delegate isKindOfClass:UIView.class] || sublayer.hidden || sublayer.opacity <= 0.01) continue;
        if (!CGRectContainsPoint(sublayer.frame, point)) continue;
        ALBProbeCandidate *candidate = [ALBProbeCandidate new];
        candidate.view = view;
        candidate.layer = sublayer;
        candidate.isLayer = YES;
        candidate.plugin = plugin;
        candidate.name = NSStringFromClass([sublayer class]);
        [found addObject:candidate];
    }
    ALBProbeCandidate *candidate = [ALBProbeCandidate new];
    candidate.view = view;
    candidate.plugin = plugin;
    candidate.name = NSStringFromClass([view class]);
    [found addObject:candidate];
}

static UIWindow *ALBProbeWindowOf(UIView *view) {
    return [view isKindOfClass:UIWindow.class] ? (UIWindow *)view : view.window;
}

static BOOL ALBProbeCandidateAlive(ALBProbeCandidate *candidate) {
    UIView *view = candidate.view;
    if (!view) return NO;
    if (!candidate.isLayer) return YES;
    CALayer *layer = candidate.layer;
    return layer && layer.superlayer == view.layer;
}

static NSArray<NSString *> *ALBProbeChain(UIView *view, NSUInteger levels) {
    NSMutableArray *names = [NSMutableArray array];
    for (UIView *node = view; node && names.count < levels; node = node.superview)
        [names addObject:NSStringFromClass([node class])];
    return names;
}

static NSString *ALBProbeIdentityText(UIView *view) {
    NSMutableDictionary *identity = [NSMutableDictionary dictionary];
    NSString *error = ALBProbeIdentity(view, identity);
    if (error) return [NSString stringWithFormat:@"规则定位：不可用（%@）", error];
    if ([identity[@"self"] boolValue]) return @"规则定位：按自身类名";
    return [NSString stringWithFormat:@"规则定位：%@ › %lu 层", identity[@"anchor"],
        (unsigned long)[identity[@"path"] count] + 1];
}

static NSString *ALBProbeAppliedText(ALBProbeCandidate *candidate) {
    UIView *view = candidate.view;
    ALBProbeTargetState *state = view ? objc_getAssociatedObject(view, &kALBProbeStateKey) : nil;
    if (!state) return nil;
    NSMutableArray *parts = [NSMutableArray array];
    if (candidate.isLayer) {
        CALayer *layer = candidate.layer;
        for (NSString *identifier in state.layerRules)
            if (layer && [gALBProbeLayerInstances[identifier] containsObject:layer]) {
                [parts addObject:@"隐藏图层"];
                break;
            }
    } else {
        if (state.backgroundRules.count) [parts addObject:@"隐藏背景"];
        if (state.hideRules.count) [parts addObject:@"隐藏视图"];
    }
    if (!parts.count) return nil;
    return [NSString stringWithFormat:@"已生效：%@%@", [parts componentsJoinedByString:@"、"],
        state.gaveUp ? @"（抖音反复改回，已暂停）" : @""];
}

static NSString *ALBProbeDescribe(ALBProbeCandidate *candidate) {
    if (!candidate) return @"";
    if (!ALBProbeCandidateAlive(candidate))
        return [NSString stringWithFormat:@"%@\n（已释放或已移走，请重新点选）", candidate.name];
    UIView *view = candidate.view;
    CALayer *layer = candidate.layer;
    NSMutableArray<NSString *> *lines = [NSMutableArray array];
    if (candidate.isLayer) {
        [lines addObject:[NSString stringWithFormat:@"图层：%@", candidate.name]];
        [lines addObject:[NSString stringWithFormat:@"窗口坐标：%@", ALBProbeRectText([view convertRect:layer.frame toView:nil])]];
        [lines addObject:[NSString stringWithFormat:@"尺寸：%.1f × %.1f · 不透明度 %.2f",
            layer.bounds.size.width, layer.bounds.size.height, layer.opacity]];
        [lines addObject:[NSString stringWithFormat:@"图层背景：%@", ALBProbeColorText(layer.backgroundColor)]];
        if ([layer isKindOfClass:CAGradientLayer.class]) {
            NSArray *colors = ((CAGradientLayer *)layer).colors;
            [lines addObject:[NSString stringWithFormat:@"渐变：%lu 个颜色%@", (unsigned long)colors.count,
                colors.count ? [@"，第一个 " stringByAppendingString:ALBProbeColorText((__bridge CGColorRef)colors.firstObject)] : @""]];
        }
        [lines addObject:[NSString stringWithFormat:@"有内容：%@ · 子图层 %lu",
            layer.contents ? @"是" : @"否", (unsigned long)layer.sublayers.count]];
        [lines addObject:[NSString stringWithFormat:@"所在视图：%@", [ALBProbeChain(view, 3) componentsJoinedByString:@" ‹ "]]];
    } else {
        CALayer *own = view.layer;
        NSUInteger extra = 0;
        for (CALayer *sublayer in own.sublayers) if (![sublayer.delegate isKindOfClass:UIView.class]) extra++;
        NSArray *background = ALBProbeOwnBackgroundParts(view);
        NSArray *chain = ALBProbeChain(view.superview, 3);
        [lines addObject:[NSString stringWithFormat:@"类名：%@", candidate.name]];
        [lines addObject:[NSString stringWithFormat:@"窗口坐标：%@", ALBProbeRectText([view convertRect:view.bounds toView:nil])]];
        [lines addObject:[NSString stringWithFormat:@"尺寸：%.1f × %.1f · 透明度 %.2f",
            view.bounds.size.width, view.bounds.size.height, view.alpha]];
        [lines addObject:[NSString stringWithFormat:@"背景色：%@", ALBProbeViewColorText(view)]];
        [lines addObject:[NSString stringWithFormat:@"图层背景：%@", ALBProbeColorText(own.backgroundColor)]];
        [lines addObject:[NSString stringWithFormat:@"图层：%@ · 有内容 %@ · 子图层 %lu（非视图 %lu）",
            NSStringFromClass([own class]), own.contents ? @"是" : @"否",
            (unsigned long)own.sublayers.count, (unsigned long)extra]];
        [lines addObject:[NSString stringWithFormat:@"自身背景：%@",
            background.count ? [background componentsJoinedByString:@"、"] : @"无（可以试试下一层）"]];
        [lines addObject:[NSString stringWithFormat:@"父视图：%@",
            chain.count ? [chain componentsJoinedByString:@" ‹ "] : @"无"]];
    }
    [lines addObject:ALBProbeIdentityText(view)];
    NSString *applied = ALBProbeAppliedText(candidate);
    if (applied) [lines addObject:applied];
    if (candidate.plugin) [lines addObject:@"提示：这是 LiquidBar 插件自身的界面"];
    return [lines componentsJoinedByString:@"\n"];
}

static NSDictionary *ALBProbeCandidateJSON(ALBProbeCandidate *candidate, NSUInteger index) {
    NSMutableDictionary *item = [NSMutableDictionary dictionary];
    item[@"index"] = @(index);
    item[@"kind"] = candidate.isLayer ? @"layer" : @"view";
    item[@"class"] = candidate.name ?: @"";
    item[@"plugin"] = @(candidate.plugin);
    if (!ALBProbeCandidateAlive(candidate)) {
        item[@"released"] = @YES;
        return item;
    }
    UIView *view = candidate.view;
    CALayer *layer = candidate.isLayer ? candidate.layer : view.layer;
    UIWindow *window = ALBProbeWindowOf(view);
    item[@"window"] = window ? NSStringFromClass([window class]) : @"";
    item[@"frameInWindow"] = ALBProbeRectJSON(candidate.isLayer ? [view convertRect:layer.frame toView:nil]
                                                                : [view convertRect:view.bounds toView:nil]);
    CGSize size = candidate.isLayer ? layer.bounds.size : view.bounds.size;
    item[@"size"] = @{@"w": @(ALBProbeNumber(size.width)), @"h": @(ALBProbeNumber(size.height))};
    item[@"layerClass"] = NSStringFromClass([layer class]);
    item[@"layerBackgroundColor"] = ALBProbeColorJSON(layer.backgroundColor);
    item[@"layerHasContents"] = @(layer.contents != nil);
    item[@"sublayerCount"] = @(layer.sublayers.count);
    item[@"opacity"] = @(ALBProbeNumber(layer.opacity));
    item[@"cornerRadius"] = @(ALBProbeNumber(layer.cornerRadius));
    if ([layer isKindOfClass:CAGradientLayer.class]) {
        NSMutableArray *colors = [NSMutableArray array];
        for (id color in ((CAGradientLayer *)layer).colors) [colors addObject:ALBProbeColorText((__bridge CGColorRef)color)];
        item[@"gradientColors"] = colors;
    }
    if (candidate.isLayer) {
        item[@"hostView"] = NSStringFromClass([view class]);
        item[@"superviewChain"] = ALBProbeChain(view, 3);
    } else {
        UIColor *color = view.backgroundColor;
        item[@"backgroundColor"] = color ? ALBProbeViewColorText(view) : (id)NSNull.null;
        item[@"alpha"] = @(ALBProbeNumber(view.alpha));
        item[@"hidden"] = @(view.hidden);
        item[@"userInteractionEnabled"] = @(view.userInteractionEnabled);
        item[@"clipsToBounds"] = @(view.clipsToBounds);
        item[@"ownBackground"] = ALBProbeOwnBackgroundParts(view);
        NSMutableArray *decorative = [NSMutableArray array];
        for (CALayer *sublayer in layer.sublayers) {
            if ([sublayer.delegate isKindOfClass:UIView.class]) continue;
            [decorative addObject:@{@"class": NSStringFromClass([sublayer class]),
                                    @"frame": ALBProbeRectJSON(sublayer.frame),
                                    @"hidden": @(sublayer.hidden),
                                    @"backgroundColor": ALBProbeColorJSON(sublayer.backgroundColor),
                                    @"hasContents": @(sublayer.contents != nil)}];
        }
        item[@"nonViewSublayers"] = decorative;
        item[@"superviewChain"] = ALBProbeChain(view.superview, 3);
        // 3.11.34：它属于哪个控制器（沿响应链往上，最多 3 个），用来认出是哪块面板
        NSMutableArray<NSString *> *controllers = [NSMutableArray array];
        for (UIResponder *r = view; r && controllers.count < 3 && ![r isKindOfClass:UIWindow.class]; r = r.nextResponder) {
            if ([r isKindOfClass:UIViewController.class]) [controllers addObject:NSStringFromClass([r class])];
        }
        item[@"controllers"] = controllers;
    }
    NSMutableDictionary *identity = [NSMutableDictionary dictionary];
    NSString *error = ALBProbeIdentity(view, identity);
    item[@"ruleIdentity"] = error ? @{@"error": error} : identity;
    NSString *applied = ALBProbeAppliedText(candidate);
    if (applied) item[@"applied"] = applied;
    return item;
}

#include "ALBProbeCapture.h"

#pragma mark - Probe window and UI

@implementation ALBProbeWindow
- (BOOL)canBecomeKeyWindow { return NO; }
- (BOOL)_canBecomeKeyWindow { return NO; }
- (BOOL)_canAffectStatusBarAppearance { return NO; }
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    if (!event || self.hidden || !self.handlesPoint || !self.handlesPoint(point)) return NO;
    return [super pointInside:point withEvent:event];
}
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    if (!event) return nil;
    UIView *hit = [super hitTest:point withEvent:event];
    return (hit == self || hit == self.rootViewController.view) ? nil : hit;
}
@end

@interface ALBProbeRootController : UIViewController
@property(nonatomic, copy) void (^layoutHandler)(void);
@end

@implementation ALBProbeRootController
- (void)loadView {
    UIView *canvas = [UIView new];
    canvas.backgroundColor = UIColor.clearColor;
    self.view = canvas;
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    if (self.layoutHandler) self.layoutHandler();
}
/* Orientation and status bar stay the app's decision. */
- (UIViewController *)albHostController {
    UIViewController *host = AWEActiveMainWindow().rootViewController;
    while (host.presentedViewController && !host.presentedViewController.isBeingDismissed)
        host = host.presentedViewController;
    return host == self ? nil : host;
}
- (UIInterfaceOrientationMask)supportedInterfaceOrientations {
    UIViewController *host = [self albHostController];
    return host ? host.supportedInterfaceOrientations : UIInterfaceOrientationMaskPortrait;
}
- (BOOL)prefersStatusBarHidden {
    UIViewController *host = [self albHostController];
    return host ? host.prefersStatusBarHidden : NO;
}
- (UIStatusBarStyle)preferredStatusBarStyle {
    UIViewController *host = [self albHostController];
    return host ? host.preferredStatusBarStyle : UIStatusBarStyleDefault;
}
@end

@interface ALBProbeController : NSObject <UITableViewDataSource, UITableViewDelegate>
@property(nonatomic, strong) ALBProbeWindow *window;
@property(nonatomic, strong) ALBProbeRootController *root;
@property(nonatomic, strong) UIButton *button;
@property(nonatomic, strong) UIView *overlay;
@property(nonatomic, strong) UILabel *hint;
@property(nonatomic, strong) UIView *highlight;
@property(nonatomic, strong) UIView *panel;
@property(nonatomic, strong) UIVisualEffectView *panelBlur;
@property(nonatomic, strong) UIView *header;
@property(nonatomic, strong) UIView *grabber;
@property(nonatomic, strong) UILabel *titleLabel;
@property(nonatomic, strong) UILabel *infoLabel;
@property(nonatomic, strong) UIButton *prevButton;
@property(nonatomic, strong) UIButton *nextButton;
@property(nonatomic, copy) NSArray<UIButton *> *inspectRowA;
@property(nonatomic, copy) NSArray<UIButton *> *inspectRowB;
@property(nonatomic, copy) NSArray<UIButton *> *rulesRow;
@property(nonatomic, copy) NSArray<UIButton *> *clearButtons;
@property(nonatomic, strong) UITableView *rulesTable;
@property(nonatomic, strong) UILabel *emptyLabel;
@property(nonatomic, strong) UILabel *toastLabel;
@property(nonatomic, strong) CADisplayLink *link;
@property(nonatomic, copy) NSArray<ALBProbeCandidate *> *candidates;
@property(nonatomic, copy) NSArray<NSDictionary *> *ruleSnapshot;
@property(nonatomic) NSUInteger index;
@property(nonatomic) BOOL picking;
@property(nonatomic) BOOL panelVisible;
@property(nonatomic) BOOL showingRules;
@property(nonatomic) BOOL buttonPlaced;
@property(nonatomic) BOOL buttonDragging;
@property(nonatomic) BOOL panelDragged;
@property(nonatomic) CGFloat panelY;
@property(nonatomic) CGPoint dragOrigin;
@property(nonatomic) CGFloat panelDragOrigin;
@property(nonatomic) CGPoint pickPoint;
@property(nonatomic) CFTimeInterval clearArmedUntil;
@property(nonatomic) NSUInteger toastGeneration;
/* 3.11.35 采集模式（ALBProbeCapture.h） */
@property(nonatomic) BOOL capturing;
@property(nonatomic) BOOL capturePaused;
@property(nonatomic) BOOL captureBarBottom;
@property(nonatomic, strong) UIView *captureCanvas;
@property(nonatomic, strong) UIView *captureBar;
@property(nonatomic, strong) UIVisualEffectView *captureBlur;
@property(nonatomic, strong) UILabel *captureTitle;
@property(nonatomic, strong) UILabel *captureInfo;
@property(nonatomic, strong) UIButton *capturePauseButton;
@property(nonatomic, copy) NSArray<UIButton *> *captureRowA;
@property(nonatomic, copy) NSArray<UIButton *> *captureRowB;
@end

static ALBProbeController *gALBProbeController;

@implementation ALBProbeController

#pragma mark Window lifecycle

- (BOOL)attach {
    UIWindowScene *scene = AWEActiveMainWindow().windowScene;
    if (!scene) return NO;
    if (!self.window) [self buildInScene:scene];
    else if (self.window.windowScene != scene) self.window.windowScene = scene;
    CGRect bounds = scene.coordinateSpace.bounds;
    if (!CGRectEqualToRect(self.window.frame, bounds)) self.window.frame = bounds;
    if (self.window.hidden) self.window.hidden = NO;
    [self layoutAll];
    return YES;
}

- (void)detach {
    self.capturing = NO;
    for (ALBCaptureItem *item in gALBCaptureItems) {
        item.box = nil;
        item.badge = nil;
        for (ALBCapturePart *part in item.parts) part.box = nil;
    }
    [self stopLink];
    self.picking = NO;
    self.panelVisible = NO;
    self.candidates = nil;
    self.window.hidden = YES;
    self.window.rootViewController = nil;
    self.window = nil;
    self.root = nil;
}

- (UIButton *)panelButton:(NSString *)title action:(SEL)action tint:(UIColor *)tint {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    [button setTitleColor:tint ?: UIColor.whiteColor forState:UIControlStateNormal];
    button.titleLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
    button.titleLabel.adjustsFontSizeToFitWidth = YES;
    button.titleLabel.minimumScaleFactor = 0.7;
    button.backgroundColor = [UIColor colorWithWhite:1 alpha:0.13];
    button.layer.cornerRadius = 10;
    button.layer.cornerCurve = kCACornerCurveContinuous;
    [button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
    [self.panel addSubview:button];
    return button;
}

- (UILabel *)capsuleLabel {
    UILabel *label = [UILabel new];
    label.textColor = UIColor.whiteColor;
    label.font = [UIFont systemFontOfSize:13 weight:UIFontWeightMedium];
    label.textAlignment = NSTextAlignmentCenter;
    label.numberOfLines = 0;
    label.backgroundColor = [UIColor colorWithWhite:0.05 alpha:0.86];
    label.layer.cornerRadius = 14;
    label.layer.cornerCurve = kCACornerCurveContinuous;
    label.clipsToBounds = YES;
    label.userInteractionEnabled = NO;
    label.hidden = YES;
    return label;
}

- (void)buildInScene:(UIWindowScene *)scene {
    ALBProbeWindow *window = [[ALBProbeWindow alloc] initWithWindowScene:scene];
    window.windowLevel = UIWindowLevelAlert + 20;
    window.backgroundColor = UIColor.clearColor;
    window.opaque = NO;
    __weak ALBProbeController *weakSelf = self;
    window.handlesPoint = ^BOOL(CGPoint point) { return [weakSelf handlesPoint:point]; };
    ALBProbeRootController *root = [ALBProbeRootController new];
    root.layoutHandler = ^{ [weakSelf layoutAll]; };
    window.rootViewController = root;
    self.window = window;
    self.root = root;
    UIView *canvas = root.view;

    UIView *highlight = [UIView new];
    highlight.userInteractionEnabled = NO;
    highlight.layer.borderWidth = 1.5;
    highlight.hidden = YES;
    [canvas addSubview:highlight];
    self.highlight = highlight;

    UIView *overlay = [UIView new];
    overlay.backgroundColor = [UIColor colorWithRed:1 green:0.45 blue:0 alpha:0.06];
    overlay.hidden = YES;
    [overlay addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(overlayTapped:)]];
    [canvas addSubview:overlay];
    self.overlay = overlay;

    // 3.11.35：采集到的线框画在这一层（不接触摸）
    UIView *captureCanvas = [UIView new];
    captureCanvas.userInteractionEnabled = NO;
    captureCanvas.backgroundColor = UIColor.clearColor;
    [canvas addSubview:captureCanvas];
    self.captureCanvas = captureCanvas;

    UIView *panel = [UIView new];
    panel.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
    panel.hidden = YES;
    panel.layer.shadowColor = UIColor.blackColor.CGColor;
    panel.layer.shadowOpacity = 0.35;
    panel.layer.shadowRadius = 16;
    panel.layer.shadowOffset = CGSizeMake(0, 6);
    [canvas addSubview:panel];
    self.panel = panel;
    UIVisualEffectView *blur = [[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemChromeMaterialDark]];
    blur.layer.cornerRadius = 18;
    blur.layer.cornerCurve = kCACornerCurveContinuous;
    blur.clipsToBounds = YES;
    blur.contentView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.28];
    [panel addSubview:blur];
    self.panelBlur = blur;

    UIView *header = [UIView new];
    [header addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(panelPanned:)]];
    [panel addSubview:header];
    self.header = header;
    UIView *grabber = [UIView new];
    grabber.backgroundColor = [UIColor colorWithWhite:1 alpha:0.35];
    grabber.layer.cornerRadius = 2;
    grabber.userInteractionEnabled = NO;
    [header addSubview:grabber];
    self.grabber = grabber;
    UILabel *title = [UILabel new];
    title.textColor = UIColor.whiteColor;
    title.font = [UIFont systemFontOfSize:13 weight:UIFontWeightBold];
    title.lineBreakMode = NSLineBreakByTruncatingMiddle;
    [header addSubview:title];
    self.titleLabel = title;

    UILabel *info = [UILabel new];
    info.textColor = [UIColor colorWithWhite:1 alpha:0.92];
    info.font = [UIFont monospacedSystemFontOfSize:11 weight:UIFontWeightRegular];
    info.numberOfLines = 0;
    info.lineBreakMode = NSLineBreakByCharWrapping;
    [panel addSubview:info];
    self.infoLabel = info;

    UIColor *warn = UIColor.systemOrangeColor, *danger = UIColor.systemRedColor;
    self.prevButton = [self panelButton:@"上一层" action:@selector(previousTapped) tint:nil];
    self.nextButton = [self panelButton:@"下一层" action:@selector(nextTapped) tint:nil];
    self.inspectRowA = @[self.prevButton, self.nextButton,
        [self panelButton:@"隐藏背景" action:@selector(hideBackgroundTapped) tint:warn],
        [self panelButton:@"隐藏视图" action:@selector(hideViewTapped) tint:warn]];
    UIButton *clearA = [self panelButton:@"全部恢复" action:@selector(clearTapped) tint:danger];
    self.inspectRowB = @[[self panelButton:@"复制信息" action:@selector(copyTapped) tint:nil],
        [self panelButton:@"撤销上次" action:@selector(undoTapped) tint:nil], clearA,
        [self panelButton:@"规则" action:@selector(rulesTapped) tint:nil],
        [self panelButton:@"退出" action:@selector(exitTapped) tint:nil]];
    UIButton *clearB = [self panelButton:@"全部恢复" action:@selector(clearTapped) tint:danger];
    self.rulesRow = @[[self panelButton:@"返回" action:@selector(backTapped) tint:nil],
        [self panelButton:@"撤销上次" action:@selector(undoTapped) tint:nil], clearB,
        [self panelButton:@"采集" action:@selector(switchToCaptureTapped) tint:UIColor.systemTealColor],
        [self panelButton:@"退出" action:@selector(exitTapped) tint:nil]];
    self.clearButtons = @[clearA, clearB];

    UITableView *table = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStylePlain];
    table.backgroundColor = UIColor.clearColor;
    table.separatorColor = [UIColor colorWithWhite:1 alpha:0.15];
    table.rowHeight = 54;
    table.dataSource = self;
    table.delegate = self;
    table.hidden = YES;
    [table addGestureRecognizer:[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(rulesLongPressed:)]];
    [panel addSubview:table];
    self.rulesTable = table;
    UILabel *empty = [UILabel new];
    empty.text = @"还没有规则";
    empty.textColor = [UIColor colorWithWhite:1 alpha:0.6];
    empty.font = [UIFont systemFontOfSize:13];
    empty.textAlignment = NSTextAlignmentCenter;
    empty.hidden = YES;
    [panel addSubview:empty];
    self.emptyLabel = empty;

    [self buildCaptureBarIn:canvas];

    UILabel *hint = [self capsuleLabel];
    hint.text = @"点选模式：轻点要检查的位置（再点按钮取消）";
    [canvas addSubview:hint];
    self.hint = hint;

    UILabel *toast = [self capsuleLabel];
    [canvas addSubview:toast];
    self.toastLabel = toast;

    UIButton *button = [UIButton buttonWithType:UIButtonTypeCustom];
    button.frame = CGRectMake(0, 0, 44, 44);
    button.layer.cornerRadius = 22;
    button.layer.shadowColor = UIColor.blackColor.CGColor;
    button.layer.shadowOpacity = 0.3;
    button.layer.shadowRadius = 6;
    button.layer.shadowOffset = CGSizeMake(0, 2);
    [button setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    button.accessibilityLabel = @"背景探针";
    [button addTarget:self action:@selector(buttonTapped) forControlEvents:UIControlEventTouchUpInside];
    [button addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(buttonPanned:)]];
    UILongPressGestureRecognizer *longPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(buttonLongPressed:)];
    longPress.minimumPressDuration = 0.5;
    [button addGestureRecognizer:longPress];
    [canvas addSubview:button];
    self.button = button;
    [self styleButton];
}

- (BOOL)handlesPoint:(CGPoint)point {
    if (!self.root) return NO;
    // 3.11.35：采集中整块屏幕都归探针（抖音收不到触摸）；暂停时只有按钮和工具条
    if (self.capturing) {
        if (!self.capturePaused) return YES;
        if (!self.captureBar.hidden && CGRectContainsPoint(self.captureBar.frame, point)) return YES;
        return !self.button.hidden && CGRectContainsPoint(CGRectInset(self.button.frame, -6, -6), point);
    }
    if (self.picking) return YES;
    if (!self.button.hidden && CGRectContainsPoint(CGRectInset(self.button.frame, -6, -6), point)) return YES;
    return self.panelVisible && CGRectContainsPoint(self.panel.frame, point);
}

#pragma mark Layout

- (CGPoint)clampedCenter:(CGPoint)center {
    UIView *canvas = self.root.view;
    CGRect bounds = canvas.bounds;
    UIEdgeInsets safe = canvas.safeAreaInsets;
    CGFloat radius = 22, margin = 6;
    CGFloat minX = safe.left + radius + margin, maxX = bounds.size.width - safe.right - radius - margin;
    /* Never over the status bar / Dynamic Island, never over the home indicator. */
    CGFloat minY = MAX(safe.top, 20) + radius + margin, maxY = bounds.size.height - MAX(safe.bottom, 8) - radius - margin;
    if (maxX < minX) maxX = minX;
    if (maxY < minY) maxY = minY;
    if (!isfinite(center.x) || !isfinite(center.y)) center = CGPointMake(maxX, (minY + maxY) * 0.5);
    return CGPointMake(MIN(maxX, MAX(minX, center.x)), MIN(maxY, MAX(minY, center.y)));
}

- (void)layoutAll {
    UIView *canvas = self.root.view;
    if (!canvas) return;
    CGRect bounds = canvas.bounds;
    if (bounds.size.width < 1 || bounds.size.height < 1) return;
    self.overlay.frame = bounds;
    if (!self.buttonPlaced) {
        NSString *stored = [NSUserDefaults.standardUserDefaults stringForKey:kALBProbeCenterKey];
        CGPoint center = stored.length ? CGPointFromString(stored)
            : CGPointMake(bounds.size.width, bounds.size.height * 0.42);
        self.button.center = [self clampedCenter:center];
        self.buttonPlaced = YES;
    } else if (!self.buttonDragging) {
        CGPoint center = [self clampedCenter:self.button.center];
        if (!CGPointEqualToPoint(center, self.button.center)) self.button.center = center;
    }
    self.captureCanvas.frame = bounds;
    [self layoutCaptureBar];
    UIEdgeInsets safe = canvas.safeAreaInsets;
    CGSize size = [self.hint sizeThatFits:CGSizeMake(bounds.size.width - 60, 60)];
    CGFloat width = MIN(bounds.size.width - 32, size.width + 28);
    self.hint.frame = CGRectMake((bounds.size.width - width) * 0.5, MAX(safe.top, 20) + 8, width, size.height + 14);
    if (self.panelVisible) [self placePanel];
}

- (CGFloat)layoutRow:(NSArray<UIButton *> *)buttons y:(CGFloat)y width:(CGFloat)width {
    CGFloat pad = 12, gap = 6, height = 34;
    CGFloat itemWidth = (width - pad * 2 - gap * (buttons.count - 1)) / buttons.count;
    for (NSUInteger i = 0; i < buttons.count; i++)
        buttons[i].frame = CGRectMake(pad + i * (itemWidth + gap), y, itemWidth, height);
    return y + height;
}

- (CGFloat)layoutPanelContent:(CGFloat)width {
    CGFloat pad = 12;
    self.header.frame = CGRectMake(0, 0, width, 36);
    self.grabber.frame = CGRectMake((width - 36) * 0.5, 6, 36, 4);
    self.titleLabel.frame = CGRectMake(pad, 14, width - pad * 2, 18);
    CGFloat y = 38;
    if (!self.showingRules) {
        CGSize size = [self.infoLabel sizeThatFits:CGSizeMake(width - pad * 2, CGFLOAT_MAX)];
        CGFloat height = MIN(ceil(size.height), 236);
        self.infoLabel.frame = CGRectMake(pad, y, width - pad * 2, height);
        y += height + 10;
        y = [self layoutRow:self.inspectRowA y:y width:width] + 6;
        y = [self layoutRow:self.inspectRowB y:y width:width] + 12;
    } else {
        CGFloat height = MIN(MAX((CGFloat)self.ruleSnapshot.count, 1) * 54, 270);
        self.rulesTable.frame = CGRectMake(0, y, width, height);
        self.emptyLabel.frame = self.rulesTable.frame;
        y += height + 8;
        y = [self layoutRow:self.rulesRow y:y width:width] + 12;
    }
    return y;
}

- (void)placePanel {
    UIView *canvas = self.root.view;
    if (!canvas) return;
    CGRect bounds = canvas.bounds;
    UIEdgeInsets safe = canvas.safeAreaInsets;
    CGFloat width = MIN(bounds.size.width - 16, 520);
    CGFloat height = [self layoutPanelContent:width];
    CGFloat minY = MAX(safe.top, 20) + 6;
    CGFloat maxY = MAX(minY, bounds.size.height - MAX(safe.bottom, 8) - 6 - height);
    CGFloat y;
    if (self.panelDragged) {
        y = MIN(maxY, MAX(minY, self.panelY));
    } else {
        /* Bottom sheet by default; move to the top when it would cover the outline. */
        CGRect avoid = self.showingRules ? CGRectNull : [self highlightRect];
        CGFloat bottom = ALBProbeArea(CGRectIntersection(CGRectMake(0, maxY, bounds.size.width, height), avoid));
        CGFloat top = ALBProbeArea(CGRectIntersection(CGRectMake(0, minY, bounds.size.width, height), avoid));
        y = top + 1 < bottom ? minY : maxY;
    }
    self.panel.frame = CGRectMake((bounds.size.width - width) * 0.5, y, width, height);
    self.panelBlur.frame = self.panel.bounds;
    self.panel.layer.shadowPath = [UIBezierPath bezierPathWithRoundedRect:self.panel.bounds cornerRadius:18].CGPath;
}

- (void)styleButton {
    if (self.capturing) {
        // 采集：按钮写的是点了会做什么
        BOOL on = !self.capturePaused;
        self.button.backgroundColor = on ? [UIColor.systemTealColor colorWithAlphaComponent:0.95] : [UIColor colorWithWhite:0.08 alpha:0.74];
        self.button.layer.borderWidth = on ? 2 : 1;
        self.button.layer.borderColor = (on ? UIColor.whiteColor : [UIColor colorWithWhite:1 alpha:0.55]).CGColor;
        [self.button setTitle:on ? @"暂停" : @"继续" forState:UIControlStateNormal];
        self.button.titleLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightBold];
        return;
    }
    BOOL active = self.picking;
    self.button.backgroundColor = active ? [UIColor.systemOrangeColor colorWithAlphaComponent:0.95]
                                         : [UIColor colorWithWhite:0.08 alpha:0.74];
    self.button.layer.borderWidth = active ? 2 : 1;
    self.button.layer.borderColor = (active ? UIColor.whiteColor : [UIColor colorWithWhite:1 alpha:0.55]).CGColor;
    [self.button setTitle:active ? @"取消" : @"探" forState:UIControlStateNormal];
    self.button.titleLabel.font = [UIFont systemFontOfSize:active ? 13 : 18 weight:UIFontWeightBold];
}

#pragma mark Highlight

- (ALBProbeCandidate *)current {
    return self.index < self.candidates.count ? self.candidates[self.index] : nil;
}

- (CGRect)highlightRect {
    ALBProbeCandidate *candidate = [self current];
    if (!candidate || !ALBProbeCandidateAlive(candidate) || !self.window) return CGRectNull;
    UIView *view = candidate.view;
    UIWindow *window = ALBProbeWindowOf(view);
    if (!window) return CGRectNull;
    CGRect rect = candidate.isLayer ? [view convertRect:candidate.layer.frame toView:nil]
                                    : [view convertRect:view.bounds toView:nil];
    rect = [self.window convertRect:rect fromWindow:window];
    rect = [self.root.view convertRect:rect fromView:nil];
    if (!isfinite(rect.origin.x) || !isfinite(rect.origin.y) || !isfinite(rect.size.width) ||
        !isfinite(rect.size.height)) return CGRectNull;
    return rect;
}

- (void)updateHighlight {
    if (!self.panelVisible || self.showingRules) {
        self.highlight.hidden = YES;
        return;
    }
    CGRect rect = [self highlightRect];
    if (CGRectIsNull(rect)) {
        self.highlight.hidden = YES;
        return;
    }
    if (!CGRectEqualToRect(self.highlight.frame, rect)) self.highlight.frame = rect;
    self.highlight.hidden = NO;
}

- (void)tick:(CADisplayLink *)link {
    [self updateHighlight];
    if (self.capturing) [self updateCaptureBoxes];
}

- (void)startLink {
    if (self.link) return;
    self.link = [CADisplayLink displayLinkWithTarget:self selector:@selector(tick:)];
    self.link.preferredFramesPerSecond = 20;
    [self.link addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}

- (void)stopLink {
    if (self.capturing) return;      // 采集的线框要一直跟着视图走
    [self.link invalidate];
    self.link = nil;
}

#pragma mark Button, pick mode

- (void)setPickMode:(BOOL)picking {
    self.picking = picking;
    self.overlay.hidden = !picking;
    self.hint.hidden = !picking;
    [self styleButton];
    if (picking) {
        self.panel.hidden = YES;
        self.panelVisible = NO;
        self.highlight.hidden = YES;
        [self stopLink];
        [self layoutAll];
    }
}

- (void)buttonTapped {
    if (self.capturing) {
        [self applyCapturePaused:!self.capturePaused];
        return;
    }
    if (self.picking) {
        [self setPickMode:NO];
        if (self.candidates.count) [self showPanel];
        return;
    }
    [[[UISelectionFeedbackGenerator alloc] init] selectionChanged];
    [self setPickMode:YES];
}

- (void)buttonPanned:(UIPanGestureRecognizer *)pan {
    UIView *canvas = self.root.view;
    if (pan.state == UIGestureRecognizerStateBegan) {
        self.dragOrigin = self.button.center;
        self.buttonDragging = YES;
    }
    CGPoint translation = [pan translationInView:canvas];
    self.button.center = [self clampedCenter:CGPointMake(self.dragOrigin.x + translation.x,
                                                         self.dragOrigin.y + translation.y)];
    if (pan.state == UIGestureRecognizerStateEnded || pan.state == UIGestureRecognizerStateCancelled ||
        pan.state == UIGestureRecognizerStateFailed) {
        self.buttonDragging = NO;
        CGPoint center = self.button.center;
        CGPoint target = [self clampedCenter:CGPointMake(center.x < canvas.bounds.size.width * 0.5 ? 0
                                                         : canvas.bounds.size.width, center.y)];
        [UIView animateWithDuration:0.25 delay:0 usingSpringWithDamping:0.82 initialSpringVelocity:0
                            options:UIViewAnimationOptionBeginFromCurrentState animations:^{
            self.button.center = target;
        } completion:nil];
        [NSUserDefaults.standardUserDefaults setObject:NSStringFromCGPoint(target) forKey:kALBProbeCenterKey];
    }
}

- (void)buttonLongPressed:(UILongPressGestureRecognizer *)press {
    if (press.state != UIGestureRecognizerStateBegan) return;
    if (self.capturing) {
        [self toast:@"采集中：点按钮暂停 / 继续；回到背景探针点工具条的“切回探针”"];
        return;
    }
    [[[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium] impactOccurred];
    [self setPickMode:NO];
    self.showingRules = YES;
    [self showPanel];
}

- (void)overlayTapped:(UITapGestureRecognizer *)tap {
    if (tap.state != UIGestureRecognizerStateEnded) return;
    if (self.capturing) [self captureAt:[tap locationInView:self.root.view]];
    else [self pickAt:[tap locationInView:self.root.view]];
}

- (void)pickAt:(CGPoint)point {
    [self setPickMode:NO];
    CGPoint inWindow = [self.root.view convertPoint:point toView:nil];
    self.pickPoint = inWindow;
    NSMutableArray<ALBProbeCandidate *> *found = [NSMutableArray array];
    for (UIWindow *window in ALBProbeAppWindows()) {
        CGPoint local = [window convertPoint:inWindow fromWindow:self.window];
        if (!CGRectContainsPoint(window.bounds, local)) continue;
        ALBProbeCollect(window, local, found, 0, NO);
    }
    if (!found.count) {
        [self toast:@"这里没有找到视图"];
        if (self.candidates.count) [self showPanel];
        return;
    }
    self.candidates = found;
    NSUInteger first = [found indexOfObjectPassingTest:^BOOL(ALBProbeCandidate *candidate, __unused NSUInteger i, __unused BOOL *stop) {
        return !candidate.plugin;
    }];
    self.index = first == NSNotFound ? 0 : first;
    self.showingRules = NO;
    self.panelDragged = NO;
    [[[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight] impactOccurred];
    [self showPanel];
}

#pragma mark Panel

- (void)showPanel {
    self.panelVisible = YES;
    self.panel.hidden = NO;
    [self refreshPanel];
}

- (void)hidePanel {
    self.panelVisible = NO;
    self.panel.hidden = YES;
    self.highlight.hidden = YES;
    [self stopLink];
}

- (void)refreshPanel {
    if (!self.panelVisible) return;
    BOOL rules = self.showingRules;
    self.infoLabel.hidden = rules;
    for (UIButton *button in self.inspectRowA) button.hidden = rules;
    for (UIButton *button in self.inspectRowB) button.hidden = rules;
    for (UIButton *button in self.rulesRow) button.hidden = !rules;
    self.rulesTable.hidden = !rules;
    if (rules) {
        self.ruleSnapshot = ALBProbeStoredRules();
        self.titleLabel.text = [NSString stringWithFormat:@"已保存规则（%lu 条）· 左滑或长按删除",
            (unsigned long)self.ruleSnapshot.count];
        self.emptyLabel.hidden = self.ruleSnapshot.count > 0;
        [self.rulesTable reloadData];
        [self stopLink];
        self.highlight.hidden = YES;
    } else {
        self.emptyLabel.hidden = YES;
        ALBProbeCandidate *candidate = [self current];
        self.titleLabel.text = [NSString stringWithFormat:@"背景探针 · 第 %lu/%lu 层%@",
            (unsigned long)self.index + 1, (unsigned long)self.candidates.count,
            candidate.isLayer ? @" · 图层" : @""];
        self.infoLabel.text = ALBProbeDescribe(candidate);
        NSArray<UIColor *> *palette = @[UIColor.systemPinkColor, UIColor.systemOrangeColor, UIColor.systemGreenColor,
            UIColor.systemBlueColor, UIColor.systemPurpleColor, UIColor.systemTealColor, UIColor.systemYellowColor];
        UIColor *color = palette[self.index % palette.count];
        self.highlight.layer.borderColor = color.CGColor;
        self.highlight.backgroundColor = [color colorWithAlphaComponent:0.22];
        self.prevButton.alpha = self.index > 0 ? 1 : 0.4;
        self.nextButton.alpha = self.index + 1 < self.candidates.count ? 1 : 0.4;
        [self startLink];
        [self updateHighlight];
    }
    [self placePanel];
}

- (void)panelPanned:(UIPanGestureRecognizer *)pan {
    if (pan.state == UIGestureRecognizerStateBegan) self.panelDragOrigin = self.panel.frame.origin.y;
    self.panelDragged = YES;
    self.panelY = self.panelDragOrigin + [pan translationInView:self.root.view].y;
    [self placePanel];
}

#pragma mark Actions

- (void)previousTapped {
    if (self.index == 0) return;
    self.index--;
    [self refreshPanel];
}

- (void)nextTapped {
    if (self.index + 1 >= self.candidates.count) return;
    self.index++;
    [self refreshPanel];
}

- (void)addRuleWithMode:(NSString *)mode {
    ALBProbeCandidate *candidate = [self current];
    if (!candidate || !ALBProbeCandidateAlive(candidate)) {
        [self toast:@"这一层已经释放，请重新点选"];
        return;
    }
    UIView *view = candidate.view;
    CALayer *layer = candidate.layer;
    NSMutableDictionary *rule = [NSMutableDictionary dictionary];
    NSString *error = ALBProbeIdentity(view, rule);
    if (error) {
        [self toast:error];
        return;
    }
    if (candidate.isLayer) {
        NSString *layerName = NSStringFromClass([layer class]);
        if (NSClassFromString(layerName) != [layer class]) {
            [self toast:@"该图层类名无法在运行时还原"];
            return;
        }
        mode = @"layer";
        rule[@"layerClass"] = layerName;
        rule[@"lw"] = @(round(layer.bounds.size.width / kALBProbeGrid) * kALBProbeGrid);
        rule[@"lh"] = @(round(layer.bounds.size.height / kALBProbeGrid) * kALBProbeGrid);
    } else if ([mode isEqual:@"bg"] && !ALBProbeOwnBackgroundParts(view).count) {
        ALBProbeTargetState *state = ALBProbeState(view, NO);
        [self toast:state.backgroundRules.count ? @"这一层的背景已经隐藏了"
                                                : @"这一层自身没有背景，可点“下一层”继续找"];
        return;
    }
    rule[@"mode"] = mode;
    [[[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium] impactOccurred];
    [self toast:ALBProbeAddRule(rule, view, layer)];
    [self refreshPanel];
}

- (void)hideBackgroundTapped { [self addRuleWithMode:@"bg"]; }
- (void)hideViewTapped { [self addRuleWithMode:@"view"]; }

- (void)copyTapped {
    NSMutableArray *items = [NSMutableArray array];
    [self.candidates enumerateObjectsUsingBlock:^(ALBProbeCandidate *candidate, NSUInteger i, __unused BOOL *stop) {
        [items addObject:ALBProbeCandidateJSON(candidate, i)];
    }];
    NSDictionary *payload = @{
        @"tool": @"AwemeLiquidBar 背景探针",
        @"time": [[NSISO8601DateFormatter new] stringFromDate:NSDate.date],
        @"system": UIDevice.currentDevice.systemVersion ?: @"",
        @"app": NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"] ?: @"",
        @"point": @{@"x": @(ALBProbeNumber(self.pickPoint.x)), @"y": @(ALBProbeNumber(self.pickPoint.y))},
        @"selectedIndex": @(self.index),
        @"candidates": items,
        @"rules": ALBProbeStoredRules()};
    NSData *data = nil;
    if ([NSJSONSerialization isValidJSONObject:payload])
        data = [NSJSONSerialization dataWithJSONObject:payload
                                               options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:nil];
    if (!data) {
        [self toast:@"生成信息失败"];
        return;
    }
    UIPasteboard.generalPasteboard.string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    NSString *folder = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    BOOL saved = folder && [data writeToFile:[folder stringByAppendingPathComponent:kALBProbeFileName] atomically:YES];
    [self toast:saved ? [NSString stringWithFormat:@"已复制 %lu 层信息，并保存到 Documents/%@", (unsigned long)items.count, kALBProbeFileName]
                      : @"已复制到剪贴板（保存文件失败）"];
}

- (void)undoTapped {
    NSDictionary *removed = ALBProbeRemoveLastRule();
    [self toast:removed ? [@"已撤销：" stringByAppendingString:ALBProbeRuleTitle(removed)] : @"没有可撤销的规则"];
    [self refreshPanel];
}

- (void)setClearTitle:(NSString *)title {
    for (UIButton *button in self.clearButtons) [button setTitle:title forState:UIControlStateNormal];
}

- (void)clearTapped {
    if (!ALBProbeStoredRules().count) {
        [self toast:@"当前没有规则"];
        return;
    }
    CFTimeInterval now = CACurrentMediaTime();
    if (now > self.clearArmedUntil) {
        self.clearArmedUntil = now + 3;
        [self setClearTitle:@"再点确认"];
        [self toast:@"再点一次“全部恢复”确认：删除全部规则并恢复原样"];
        __weak ALBProbeController *weakSelf = self;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            ALBProbeController *strongSelf = weakSelf;
            if (strongSelf && CACurrentMediaTime() > strongSelf.clearArmedUntil) [strongSelf setClearTitle:@"全部恢复"];
        });
        return;
    }
    self.clearArmedUntil = 0;
    [self setClearTitle:@"全部恢复"];
    NSUInteger count = ALBProbeRemoveAllRules();
    [self toast:[NSString stringWithFormat:@"已全部恢复，删除了 %lu 条规则", (unsigned long)count]];
    [self refreshPanel];
}

- (void)rulesTapped {
    self.showingRules = YES;
    [self refreshPanel];
}

- (void)backTapped {
    if (!self.candidates.count) {
        [self hidePanel];
        return;
    }
    self.showingRules = NO;
    [self refreshPanel];
}

- (void)exitTapped {
    [self hidePanel];
    self.candidates = nil;
    self.showingRules = NO;
}

- (void)deleteRule:(NSString *)identifier {
    NSDictionary *removed = ALBProbeRemoveRuleWithIdentifier(identifier);
    [self toast:removed ? [@"已删除并恢复：" stringByAppendingString:ALBProbeRuleTitle(removed)] : @"规则已不存在"];
    [self refreshPanel];
}

- (void)rulesLongPressed:(UILongPressGestureRecognizer *)press {
    if (press.state != UIGestureRecognizerStateBegan) return;
    NSIndexPath *path = [self.rulesTable indexPathForRowAtPoint:[press locationInView:self.rulesTable]];
    if (!path || (NSUInteger)path.row >= self.ruleSnapshot.count) return;
    [[[UINotificationFeedbackGenerator alloc] init] notificationOccurred:UINotificationFeedbackTypeWarning];
    [self deleteRule:self.ruleSnapshot[path.row][@"id"]];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.ruleSnapshot.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@"ALBProbeRule"];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@"ALBProbeRule"];
        cell.backgroundColor = UIColor.clearColor;
        cell.selectionStyle = UITableViewCellSelectionStyleNone;
        cell.textLabel.textColor = UIColor.whiteColor;
        cell.textLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
        cell.textLabel.lineBreakMode = NSLineBreakByTruncatingMiddle;
        cell.detailTextLabel.textColor = [UIColor colorWithWhite:1 alpha:0.62];
        cell.detailTextLabel.font = [UIFont systemFontOfSize:11];
        cell.detailTextLabel.numberOfLines = 2;
    }
    NSDictionary *rule = (NSUInteger)indexPath.row < self.ruleSnapshot.count ? self.ruleSnapshot[indexPath.row] : @{};
    cell.textLabel.text = [NSString stringWithFormat:@"%ld. %@", (long)indexPath.row + 1, ALBProbeRuleTitle(rule)];
    cell.detailTextLabel.text = ALBProbeRuleDetail(rule);
    return cell;
}

- (UISwipeActionsConfiguration *)tableView:(UITableView *)tableView
    trailingSwipeActionsConfigurationForRowAtIndexPath:(NSIndexPath *)indexPath {
    if ((NSUInteger)indexPath.row >= self.ruleSnapshot.count) return nil;
    NSString *identifier = self.ruleSnapshot[indexPath.row][@"id"];
    __weak ALBProbeController *weakSelf = self;
    UIContextualAction *remove = [UIContextualAction contextualActionWithStyle:UIContextualActionStyleDestructive title:@"删除"
        handler:^(__unused UIContextualAction *action, __unused UIView *sourceView, void (^completion)(BOOL)) {
        completion(YES);
        dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf deleteRule:identifier]; });
    }];
    return [UISwipeActionsConfiguration configurationWithActions:@[remove]];
}

- (void)toast:(NSString *)text {
    if (!text.length) return;
    UIView *canvas = self.root.view;
    if (!canvas || self.window.hidden) {
        AWEShowDownloadToast(text);
        return;
    }
    UILabel *label = self.toastLabel;
    label.text = text;
    CGRect bounds = canvas.bounds;
    UIEdgeInsets safe = canvas.safeAreaInsets;
    CGSize size = [label sizeThatFits:CGSizeMake(bounds.size.width - 72, 200)];
    CGFloat width = MIN(bounds.size.width - 32, size.width + 28);
    BOOL panelOnTop = self.panelVisible && CGRectGetMidY(self.panel.frame) < bounds.size.height * 0.5;
    CGFloat y = panelOnTop ? CGRectGetMaxY(self.panel.frame) + 10 : MAX(safe.top, 20) + 52;
    if (self.capturing && !self.captureBar.hidden)
        y = self.captureBarBottom ? CGRectGetMinY(self.captureBar.frame) - size.height - 26 : CGRectGetMaxY(self.captureBar.frame) + 10;
    label.frame = CGRectMake((bounds.size.width - width) * 0.5, y, width, size.height + 16);
    [canvas bringSubviewToFront:label];
    [canvas bringSubviewToFront:self.button];
    label.hidden = NO;
    label.alpha = 0;
    NSUInteger generation = ++self.toastGeneration;
    [UIView animateWithDuration:0.18 animations:^{ label.alpha = 1; }];
    __weak ALBProbeController *weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2.6 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        ALBProbeController *strongSelf = weakSelf;
        if (!strongSelf || strongSelf.toastGeneration != generation) return;
        [UIView animateWithDuration:0.25 animations:^{ label.alpha = 0; } completion:^(__unused BOOL finished) {
            if (strongSelf.toastGeneration == generation) label.hidden = YES;
        }];
    });
}

#pragma mark Capture (3.11.35)

- (UIButton *)captureButton:(NSString *)title action:(SEL)action tint:(UIColor *)tint {
    UIButton *button = [UIButton buttonWithType:UIButtonTypeSystem];
    [button setTitle:title forState:UIControlStateNormal];
    [button setTitleColor:tint ?: UIColor.whiteColor forState:UIControlStateNormal];
    button.titleLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
    button.titleLabel.adjustsFontSizeToFitWidth = YES;
    button.titleLabel.minimumScaleFactor = 0.7;
    button.backgroundColor = [UIColor colorWithWhite:1 alpha:0.13];
    button.layer.cornerRadius = 10;
    button.layer.cornerCurve = kCACornerCurveContinuous;
    [button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
    [self.captureBar addSubview:button];
    return button;
}

- (void)buildCaptureBarIn:(UIView *)canvas {
    UIView *bar = [UIView new];
    bar.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
    bar.hidden = YES;
    bar.layer.shadowColor = UIColor.blackColor.CGColor;
    bar.layer.shadowOpacity = 0.35;
    bar.layer.shadowRadius = 12;
    bar.layer.shadowOffset = CGSizeMake(0, 4);
    [canvas addSubview:bar];
    self.captureBar = bar;
    UIVisualEffectView *blur = [[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemChromeMaterialDark]];
    blur.layer.cornerRadius = 16;
    blur.layer.cornerCurve = kCACornerCurveContinuous;
    blur.clipsToBounds = YES;
    blur.contentView.backgroundColor = [UIColor colorWithWhite:0 alpha:0.3];
    blur.userInteractionEnabled = NO;
    [bar addSubview:blur];
    self.captureBlur = blur;
    UILabel *title = [UILabel new];
    title.textColor = UIColor.whiteColor;
    title.font = [UIFont systemFontOfSize:13 weight:UIFontWeightBold];
    [bar addSubview:title];
    self.captureTitle = title;
    UILabel *info = [UILabel new];
    info.textColor = [UIColor colorWithWhite:1 alpha:0.85];
    info.font = [UIFont systemFontOfSize:11.5 weight:UIFontWeightRegular];
    info.numberOfLines = 2;
    info.lineBreakMode = NSLineBreakByTruncatingMiddle;
    [bar addSubview:info];
    self.captureInfo = info;
    self.capturePauseButton = [self captureButton:@"暂停" action:@selector(capturePauseTapped) tint:UIColor.systemTealColor];
    self.captureRowA = @[self.capturePauseButton,
        [self captureButton:@"撤销" action:@selector(captureUndoTapped) tint:nil],
        [self captureButton:@"扩大一层" action:@selector(captureGrowTapped) tint:nil],
        [self captureButton:@"缩小一层" action:@selector(captureShrinkTapped) tint:nil],
        [self captureButton:@"拆开" action:@selector(captureSplitTapped) tint:UIColor.systemGreenColor]];
    self.captureRowB = @[[self captureButton:@"记下页面" action:@selector(capturePageTapped) tint:UIColor.systemYellowColor],
        [self captureButton:@"打包本段" action:@selector(capturePackTapped) tint:UIColor.systemOrangeColor],
        [self captureButton:@"上 / 下" action:@selector(captureMoveTapped) tint:nil],
        [self captureButton:@"切回探针" action:@selector(captureExitTapped) tint:nil]];
}

- (void)layoutCaptureBar {
    UIView *canvas = self.root.view;
    if (!canvas || !self.captureBar) return;
    self.captureBar.hidden = !self.capturing;
    if (!self.capturing) return;
    CGRect bounds = canvas.bounds;
    UIEdgeInsets safe = canvas.safeAreaInsets;
    CGFloat width = MIN(bounds.size.width - 16, 520), pad = 12, gap = 6, rowH = 32;
    CGFloat y = 10;
    self.captureTitle.frame = CGRectMake(pad, y, width - pad * 2, 18);
    y += 20;
    self.captureInfo.frame = CGRectMake(pad, y, width - pad * 2, 30);
    y += 34;
    for (NSArray<UIButton *> *row in @[self.captureRowA, self.captureRowB]) {
        CGFloat itemW = (width - pad * 2 - gap * (row.count - 1)) / row.count;
        for (NSUInteger i = 0; i < row.count; i++) row[i].frame = CGRectMake(pad + i * (itemW + gap), y, itemW, rowH);
        y += rowH + gap;
    }
    CGFloat height = y + 4;
    CGFloat top = self.captureBarBottom ? bounds.size.height - MAX(safe.bottom, 8) - 6 - height : MAX(safe.top, 20) + 4;
    self.captureBar.frame = CGRectMake((bounds.size.width - width) * 0.5, top, width, height);
    self.captureBlur.frame = self.captureBar.bounds;
    self.captureBar.layer.shadowPath = [UIBezierPath bezierPathWithRoundedRect:self.captureBar.bounds cornerRadius:16].CGPath;
    [canvas bringSubviewToFront:self.captureBar];
    [canvas bringSubviewToFront:self.toastLabel];
    [canvas bringSubviewToFront:self.button];
}

- (void)refreshCaptureBar {
    if (!self.capturing) return;
    NSInteger stage = MAX((NSInteger)1, [NSUserDefaults.standardUserDefaults integerForKey:kALBCaptureStageKey]);
    NSUInteger elements = 0, pages = 0;
    for (ALBCaptureItem *item in gALBCaptureItems) ([item.kind isEqualToString:@"page"] ? pages++ : elements++);
    self.captureTitle.text = [NSString stringWithFormat:@"采集 · 第 %ld 段 · 元素 %lu · 页面 %lu%@", (long)stage,
        (unsigned long)elements, (unsigned long)pages, self.capturePaused ? @" ·（暂停中，可正常操作抖音）" : @""];
    ALBCaptureItem *last = gALBCaptureItems.lastObject;
    self.captureInfo.text = last ? [NSString stringWithFormat:@"#%lu %@", (unsigned long)last.number, last.summary]
        : @"点按钮 / 胶囊抓一个（胶囊粗框，图标白框，文字黄框）；框得不对用“扩大 / 缩小一层”或“撤销”";
    [self.capturePauseButton setTitle:self.capturePaused ? @"继续" : @"暂停" forState:UIControlStateNormal];
    [self styleButton];
}

- (void)startCapturePaused:(BOOL)paused {
    if (!gALBCaptureItems) gALBCaptureItems = [NSMutableArray array];
    [self setPickMode:NO];
    [self hidePanel];
    self.capturing = YES;
    [self startLink];
    [self applyCapturePaused:paused];
    [self layoutAll];
}

- (void)endCapture {
    self.capturing = NO;
    self.capturePaused = NO;
    self.overlay.hidden = YES;
    self.overlay.backgroundColor = [UIColor colorWithRed:1 green:0.45 blue:0 alpha:0.06];
    for (ALBCaptureItem *item in gALBCaptureItems) {
        [item.box removeFromSuperview];
        item.box = nil;
        item.badge = nil;
        for (ALBCapturePart *part in item.parts) { [part.box removeFromSuperview]; part.box = nil; }
    }
    [self stopLink];
    [self styleButton];
    [self layoutAll];
}

// 3.11.36：3.11.35 这里叫 setCapturePaused:，正好盖住了属性自己的设置方法，里面再 self.capturePaused = 就无限递归、
// 栈溢出闪退（设置了“采集”后每次启动都会进来）。改名，属性用自动生成的设置方法
- (void)applyCapturePaused:(BOOL)paused {
    self.capturePaused = paused;
    self.overlay.backgroundColor = [UIColor colorWithRed:0 green:0.75 blue:0.8 alpha:0.05];
    self.overlay.hidden = paused || !self.capturing;
    [self refreshCaptureBar];
    [self layoutAll];
}

- (void)capturePauseTapped { [self applyCapturePaused:!self.capturePaused]; }

// 3.11.37：先问 UIKit 自己的命中测试。iOS 26 的标签栏 / 导航控制器在最前面盖着几层透明的悬浮栏容器，
// 真的手指会穿过去，按几何找“最前面那层”却会先找到它们（3.11.36 第 1、2 段大半都框错在这里）。
// 命中测试从最前面的抖音窗口问起：拿到视图就用；窗口把触摸留给自己就不再往后问；插件自己的窗口不问。
- (void)captureAt:(CGPoint)point {
    CGPoint inWindow = [self.root.view convertPoint:point toView:nil];
    NSMutableArray<ALBProbeCandidate *> *found = [NSMutableArray array];
    UIView *hit = nil;
    BOOL hitAsked = NO;
    for (UIWindow *window in ALBProbeAppWindows()) {
        CGPoint local = [window convertPoint:inWindow fromWindow:self.window];
        if (!CGRectContainsPoint(window.bounds, local)) continue;
        if (!hitAsked && !ALBProbeClassIsPlugin([window class]) && window.userInteractionEnabled) {
            UIView *answer = [window hitTest:local withEvent:nil];
            if (answer) {
                hitAsked = YES;
                if (answer != window && !ALBProbeChainIsPlugin(answer)) hit = answer;
            }
        }
        ALBProbeCollect(window, local, found, 0, NO);
    }
    UIWindow *appWindow = AWEActiveMainWindow();
    CGFloat big = appWindow ? CGRectGetWidth(appWindow.bounds) * CGRectGetHeight(appWindow.bounds) * 0.6 : CGFLOAT_MAX;
    UIView *hitFront = nil, *front = nil, *fallback = nil;
    NSMutableArray<NSString *> *atPoint = [NSMutableArray array];
    for (ALBProbeCandidate *candidate in found) {
        if (candidate.plugin || !candidate.view || [candidate.view isKindOfClass:UIWindow.class]) continue;
        if (atPoint.count < 12) {
            CGSize size = candidate.isLayer ? candidate.layer.bounds.size : candidate.view.bounds.size;
            [atPoint addObject:[NSString stringWithFormat:@"%@%@ %.0f×%.0f", candidate.name, candidate.isLayer ? @"（图层）" : @"",
                size.width, size.height]];
        }
        if (candidate.isLayer) continue;
        UIView *view = candidate.view;
        // The front-most (deepest) view inside what UIKit hit: shrinking can still go down to the icon / text.
        if (hit && !hitFront && (view == hit || [view isDescendantOfView:hit])) hitFront = view;
        if (ALBCapturePassThrough(view)) continue;
        if (!fallback) fallback = view;
        if (!front && view.bounds.size.width * view.bounds.size.height < big) front = view;
    }
    // A button whose touch area is bigger than itself (back arrows): UIKit hits it, the geometry does not.
    UIView *chosen = hitFront ?: (hit ?: (front ?: fallback));
    if (!chosen) {
        [self toast:@"这里没有抖音的视图"];
        return;
    }
    NSString *hitName = hit ? [NSString stringWithFormat:@"%@ %.0f×%.0f", NSStringFromClass([hit class]),
        hit.bounds.size.width, hit.bounds.size.height] : @"（命中测试没拿到，按几何找）";
    [self addCaptureItemFrom:chosen point:inWindow atPoint:atPoint hit:hitName origin:@"点按" level:NSNotFound];
    [[[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight] impactOccurred];
    [self refreshCaptureBar];
    [self updateCaptureBoxes];
}

- (ALBCaptureItem *)addCaptureItemFrom:(UIView *)front point:(CGPoint)inWindow atPoint:(NSArray<NSString *> *)atPoint
                                   hit:(NSString *)hitName origin:(NSString *)origin level:(NSUInteger)level {
    ALBCaptureItem *item = [ALBCaptureItem new];
    item.number = ++gALBCaptureNumber;
    item.kind = @"element";
    item.point = inWindow;
    item.chain = ALBCaptureChainFrom(front);
    item.level = level == NSNotFound ? ALBCaptureDefaultLevel(item.chain) : MIN(level, item.chain.count ? item.chain.count - 1 : 0);
    item.hitName = hitName;
    item.origin = origin;
    NSArray<UIColor *> *palette = @[UIColor.systemPinkColor, UIColor.systemOrangeColor, UIColor.systemGreenColor,
        UIColor.systemBlueColor, UIColor.systemPurpleColor, UIColor.systemTealColor, UIColor.systemRedColor];
    item.color = palette[item.number % palette.count];
    ALBCaptureRead(item, atPoint);
    [gALBCaptureItems addObject:item];
    return item;
}

// 3.11.37 “拆开”：最后一项框住的整块 → 里面每颗按钮 / 每个格子各记一项（原来那一整块保留，方便对照位置）。
- (void)captureSplitTapped {
    ALBCaptureItem *item = [self lastElement];
    UIView *capsule = item.capsule;
    if (!capsule) { [self toast:@"先框住一整块（点一下，再“扩大一层”到整块），再拆开"]; return; }
    NSArray<UIView *> *pieces = ALBCaptureSplit(capsule);
    if (!pieces.count) { [self toast:@"这一块里面没找到按钮 / 格子，可以“扩大一层”再拆"]; return; }
    NSString *origin = [NSString stringWithFormat:@"拆开自 #%lu", (unsigned long)item.number];
    for (UIView *piece in pieces) {
        CGRect frame = [piece convertRect:piece.bounds toView:nil];
        [self addCaptureItemFrom:piece point:CGPointMake(CGRectGetMidX(frame), CGRectGetMidY(frame)) atPoint:nil
                             hit:nil origin:origin level:0];
    }
    [[[UINotificationFeedbackGenerator alloc] init] notificationOccurred:UINotificationFeedbackTypeSuccess];
    [self toast:[NSString stringWithFormat:@"#%lu 拆成了 %lu 项%@", (unsigned long)item.number, (unsigned long)pieces.count,
        pieces.count >= 40 ? @"（一次最多 40 项，剩下的往下滑再框一次）" : @""]];
    [self refreshCaptureBar];
    [self updateCaptureBoxes];
}

- (ALBCaptureItem *)lastElement {
    for (ALBCaptureItem *item in gALBCaptureItems.reverseObjectEnumerator)
        if ([item.kind isEqualToString:@"element"]) return item;
    return nil;
}

- (void)removeBoxesOf:(ALBCaptureItem *)item {
    [item.box removeFromSuperview];
    item.box = nil;
    item.badge = nil;
    for (ALBCapturePart *part in item.parts) { [part.box removeFromSuperview]; part.box = nil; }
}

- (void)captureUndoTapped {
    ALBCaptureItem *last = gALBCaptureItems.lastObject;
    if (!last) { [self toast:@"没有可撤销的"]; return; }
    [self removeBoxesOf:last];
    [gALBCaptureItems removeLastObject];
    [self toast:[NSString stringWithFormat:@"已撤销 #%lu", (unsigned long)last.number]];
    [self refreshCaptureBar];
}

- (void)captureRelevel:(NSInteger)delta {
    ALBCaptureItem *item = [self lastElement];
    if (!item) { [self toast:@"先抓一个按钮或胶囊"]; return; }
    NSInteger level = (NSInteger)item.level;
    do { level += delta; } while (level >= 0 && level < (NSInteger)item.chain.count && ![item.chain pointerAtIndex:level]);
    if (level < 0 || level >= (NSInteger)item.chain.count) {
        [self toast:delta > 0 ? @"已经是最大的一层了（再大就是整块页面）" : @"已经是点到的最里面一层了"];
        return;
    }
    [self removeBoxesOf:item];
    item.level = (NSUInteger)level;
    ALBCaptureRead(item, nil);
    [self refreshCaptureBar];
    [self updateCaptureBoxes];
}

- (void)captureGrowTapped { [self captureRelevel:1]; }
- (void)captureShrinkTapped { [self captureRelevel:-1]; }

- (void)capturePageTapped {
    UIWindow *window = AWEActiveMainWindow();
    if (!window) { [self toast:@"找不到抖音的窗口"]; return; }
    ALBCaptureItem *item = [ALBCaptureItem new];
    item.number = ++gALBCaptureNumber;
    item.kind = @"page";
    NSMutableDictionary *snapshot = [ALBCapturePageJSON(window) mutableCopy];
    snapshot[@"n"] = @(item.number);
    snapshot[@"time"] = [[NSISO8601DateFormatter new] stringFromDate:NSDate.date];
    NSMutableArray *windows = [NSMutableArray array];
    for (UIWindow *w in ALBProbeAppWindows())
        [windows addObject:[NSString stringWithFormat:@"%@ 层级 %.0f%@", NSStringFromClass([w class]), w.windowLevel,
            ALBProbeClassIsPlugin([w class]) ? @"（插件）" : @""]];
    snapshot[@"windows"] = windows;
    item.snapshot = snapshot;
    NSArray *routes = snapshot[@"routeStrings"];
    NSArray *titles = snapshot[@"visibleTitles"];
    item.summary = [NSString stringWithFormat:@"页面 %@%@ · 像跳转地址的字符串 %lu 条", snapshot[@"top"] ?: @"?",
        titles.count ? [NSString stringWithFormat:@"「%@」", titles.firstObject] : @"", (unsigned long)routes.count];
    [gALBCaptureItems addObject:item];
    [[[UINotificationFeedbackGenerator alloc] init] notificationOccurred:UINotificationFeedbackTypeSuccess];
    [self toast:[NSString stringWithFormat:@"已记下页面：%@", snapshot[@"top"] ?: @"?"]];
    [self refreshCaptureBar];
}

- (void)capturePackTapped {
    if (!gALBCaptureItems.count) { [self toast:@"这一段还没有抓到东西"]; return; }
    NSInteger stage = MAX((NSInteger)1, [NSUserDefaults.standardUserDefaults integerForKey:kALBCaptureStageKey]);
    NSMutableArray *items = [NSMutableArray array];
    for (ALBCaptureItem *item in gALBCaptureItems) if (item.snapshot) [items addObject:item.snapshot];
    NSDictionary *payload = @{
        @"tool": @"AwemeLiquidBar 采集",
        @"stage": @(stage),
        @"time": [[NSISO8601DateFormatter new] stringFromDate:NSDate.date],
        @"system": UIDevice.currentDevice.systemVersion ?: @"",
        @"app": NSBundle.mainBundle.infoDictionary[@"CFBundleShortVersionString"] ?: @"",
        @"items": items};
    NSData *data = nil;
    if ([NSJSONSerialization isValidJSONObject:payload])
        data = [NSJSONSerialization dataWithJSONObject:payload options:NSJSONWritingPrettyPrinted | NSJSONWritingSortedKeys error:nil];
    if (!data) { [self toast:@"打包失败（数据里有无法导出的内容）"]; return; }
    UIPasteboard.generalPasteboard.string = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    NSString *name = [NSString stringWithFormat:@"AwemeLiquidBar_Capture_%ld.json", (long)stage];
    NSString *folder = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    BOOL saved = folder && [data writeToFile:[folder stringByAppendingPathComponent:name] atomically:YES];
    for (ALBCaptureItem *item in gALBCaptureItems) [self removeBoxesOf:item];
    [gALBCaptureItems removeAllObjects];
    [NSUserDefaults.standardUserDefaults setInteger:stage + 1 forKey:kALBCaptureStageKey];
    [[[UINotificationFeedbackGenerator alloc] init] notificationOccurred:UINotificationFeedbackTypeSuccess];
    [self toast:[NSString stringWithFormat:@"第 %ld 段已打包（%lu 项）：已复制%@，接着抓的是第 %ld 段", (long)stage,
        (unsigned long)items.count, saved ? [NSString stringWithFormat:@"并保存到 Documents/%@", name] : @"（保存文件失败）", (long)stage + 1]];
    [self refreshCaptureBar];
}

- (void)captureMoveTapped {
    self.captureBarBottom = !self.captureBarBottom;
    [self layoutAll];
}

- (void)captureExitTapped {
    [NSUserDefaults.standardUserDefaults setInteger:0 forKey:ALB5Key(@"probe.mode")];
    [self endCapture];
    [self toast:gALBCaptureItems.count ? @"已切回背景探针；没打包的采集还留着，回到采集会接着显示" : @"已切回背景探针"];
}

- (void)switchToCaptureTapped {
    [NSUserDefaults.standardUserDefaults setInteger:1 forKey:ALB5Key(@"probe.mode")];
    self.showingRules = NO;
    [self startCapturePaused:NO];
    [self toast:@"采集模式：点按钮 / 胶囊抓一个，抖音收不到这些点按"];
}

/* Settings "probe.mode": 1 = capture. Called whenever the options change. */
- (void)syncCaptureMode {
    BOOL want = (NSInteger)lround(ALB5Number(@"probe.mode", 0, 0, 1)) == 1;
    if (want && !self.capturing) [self startCapturePaused:YES];
    else if (!want && self.capturing) [self endCapture];
}

- (UIView *)captureBoxWithColor:(UIColor *)color width:(CGFloat)width fill:(CGFloat)fill {
    UIView *box = [UIView new];
    box.userInteractionEnabled = NO;
    box.layer.borderWidth = width;
    box.layer.borderColor = color.CGColor;
    box.backgroundColor = [color colorWithAlphaComponent:fill];
    [self.captureCanvas addSubview:box];
    return box;
}

- (CGRect)canvasRectForView:(UIView *)view layer:(CALayer *)layer {
    UIWindow *window = view.window;
    if (!window || !self.window || view.hidden) return CGRectNull;
    CGRect rect = layer ? [view convertRect:layer.frame toView:nil] : [view convertRect:view.bounds toView:nil];
    rect = [self.window convertRect:rect fromWindow:window];
    rect = [self.root.view convertRect:rect fromView:nil];
    if (!isfinite(rect.origin.x) || !isfinite(rect.origin.y) || !isfinite(rect.size.width) || !isfinite(rect.size.height)) return CGRectNull;
    return rect;
}

- (void)updateCaptureBoxes {
    if (!self.capturing || !self.captureCanvas) return;
    for (ALBCaptureItem *item in gALBCaptureItems) {
        if (![item.kind isEqualToString:@"element"]) continue;
        UIView *capsule = [item capsule];
        if (!item.box) {
            item.box = [self captureBoxWithColor:item.color width:2 fill:0.10];
            UILabel *badge = [UILabel new];
            badge.text = [NSString stringWithFormat:@"%lu", (unsigned long)item.number];
            badge.font = [UIFont systemFontOfSize:11 weight:UIFontWeightBold];
            badge.textColor = UIColor.whiteColor;
            badge.textAlignment = NSTextAlignmentCenter;
            badge.backgroundColor = item.color;
            badge.layer.cornerRadius = 8;
            badge.clipsToBounds = YES;
            [item.box addSubview:badge];
            item.badge = badge;
        }
        CGRect rect = [self canvasRectForView:capsule layer:nil];
        item.box.hidden = CGRectIsNull(rect);
        if (!item.box.hidden && !CGRectEqualToRect(item.box.frame, rect)) {
            item.box.frame = rect;
            CGFloat w = MAX(16, item.badge.text.length * 8 + 8);
            item.badge.frame = CGRectMake(-1, -17, w, 16);
        }
        for (ALBCapturePart *part in item.parts) {
            if (!part.box) {
                UIColor *color = [part.role isEqualToString:@"text"] ? UIColor.systemYellowColor
                    : ([part.role isEqualToString:@"glass"] ? UIColor.cyanColor : UIColor.whiteColor);
                part.box = [self captureBoxWithColor:color width:1 fill:0.0];
            }
            CGRect partRect = [self canvasRectForView:part.view layer:part.layer];
            part.box.hidden = CGRectIsNull(partRect) || (part.layer && !part.layer.superlayer);
            if (!part.box.hidden && !CGRectEqualToRect(part.box.frame, partRect)) part.box.frame = partRect;
        }
    }
}

@end

#pragma mark - Lifecycle

static void ALBProbeUpdateFloatingUI(void) {
    BOOL wanted = gALBProbeAllowed && ALB5Bool(@"probe.enabled", NO);
    if (wanted) {
        if (!gALBProbeController) gALBProbeController = [ALBProbeController new];
        if ([gALBProbeController attach]) [gALBProbeController syncCaptureMode];
    } else if (gALBProbeController) {
        [gALBProbeController detach];
        gALBProbeController = nil;
    }
}

static void ALBProbeRetryUnresolved(BOOL force) {
    /* A rule whose classes were not loaded yet (lazy framework / first launch). */
    static CFTimeInterval last;
    CFTimeInterval now = CACurrentMediaTime();
    if (!force && now - last < 5.0) return;
    last = now;
    for (ALBProbeRule *rule in gALBProbeRules) {
        if (rule.resolved) continue;
        ALBProbeCompileRules();
        ALBProbeSweep();
        return;
    }
}

static void ALBProbeRefresh(BOOL activation) {
    BOOL allowed = AWEPluginFeaturesEnabled();
    if (allowed != gALBProbeAllowed) {
        gALBProbeAllowed = allowed;
        /* LiquidBar switched off: give every view back; on again: re-apply. */
        if (allowed) ALBProbeSweep();
        else ALBProbeReleaseAllInstances();
    }
    if (activation && allowed) ALBProbeRetryUnresolved(NO);
    ALBProbeUpdateFloatingUI();
}

/* Called once from the %ctor main-queue block. */
static void ALBProbeStart(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        gALBProbeAllowed = AWEPluginFeaturesEnabled();
        ALBProbeCompileRules();
        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        NSOperationQueue *main = NSOperationQueue.mainQueue;
        for (NSString *name in @[@"ALB5.OptionsChanged", AWEAuthorizationNotificationName()])
            [center addObserverForName:name object:nil queue:main usingBlock:^(__unused NSNotification *note) {
                ALBProbeRefresh(NO);
            }];
        for (NSString *name in @[UIApplicationDidBecomeActiveNotification, UISceneDidActivateNotification,
                                 UIWindowDidBecomeKeyNotification])
            [center addObserverForName:name object:nil queue:main usingBlock:^(__unused NSNotification *note) {
                ALBProbeRefresh(YES);
            }];
        if (gALBProbeRules.count) {
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.8 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                ALBProbeRetryUnresolved(YES);
                ALBProbeSweep();
            });
        }
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            ALBProbeUpdateFloatingUI();
        });
    });
}

/* Settings entry: opens the "背景探针" section of ALB5OptionSections(). */
static UIButton *ALBProbeSettingsEntryButton(UIViewController *host, UIColor *accent) {
    UIButton *button = AWECreateGlassTextButton(@"背景探针", accent, YES);
    __weak UIViewController *weakHost = host;
    [button addAction:[UIAction actionWithHandler:^(__unused UIAction *action) {
        UIViewController *owner = weakHost;
        NSUInteger index = [ALB5OptionSections() indexOfObjectPassingTest:^BOOL(NSDictionary *section, __unused NSUInteger i, __unused BOOL *stop) {
            return [section[@"title"] isEqual:kALBProbeSectionTitle];
        }];
        if (!owner || index == NSNotFound) return;
        ALB5OptionsController *options = [[ALB5OptionsController alloc]
            initWithSections:[NSIndexSet indexSetWithIndex:index] title:kALBProbeSectionTitle];
        if (owner.navigationController) [owner.navigationController pushViewController:options animated:YES];
        else [owner presentViewController:[[UINavigationController alloc] initWithRootViewController:options]
                                 animated:YES completion:nil];
    }] forControlEvents:UIControlEventTouchUpInside];
    return button;
}
