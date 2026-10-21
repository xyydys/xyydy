#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <math.h>

/*
 Fresh FLEX-based comment glass prototype.

 Geometry comes from the supplied live hierarchy, not from the retired comment
 glass experiments:
   outer panel UICollectionView        440 x 650.08
   top/header region                    40 pt
   AWEListKitMagicCollectionView       440 x 527.08
   bottom CommentInputContainerView     83 pt
   inner native input pill              12,6,416,36

 v1 deliberately does only four things:
   1) keep the media fullscreen while the comment sheet is visible, so glass
      has real moving content behind it;
   2) clear the 83 pt outer input background without touching the 416x36 pill;
   3) put one clear native UIGlassEffect behind the 40 pt header;
   4) put a second clear native UIGlassEffect behind the 527 pt comment body.

 No rounded panel, no manual tint, no adaptive light/dark override, no input
 pill replacement, and no old comment-glass code is carried forward.
*/

static const NSInteger kACGHeaderGlassTag = 26082011;
static const NSInteger kACGBodyGlassTag   = 26082012;
static const NSInteger kACGHostTag        = 26082013;

static char kACGGlassHostKey;
static char kACGHeaderGlassKey;
static char kACGBodyGlassKey;

static __weak UIViewController *gACGVisibleCommentController = nil;
static __weak UIView *gACGLastMagicView = nil;
static __weak UIView *gACGLastPanelRoot = nil;
static __weak UIView *gACGLastInputRoot = nil;

BOOL ACGCommentGlassKeepsFullscreen(void) {
    UIViewController *controller = gACGVisibleCommentController;
    if (!controller) return NO;
    if (controller.isBeingDismissed || controller.isMovingFromParentViewController) {
        return controller.viewIfLoaded.window != nil;
    }
    return YES;
}

static NSString *ACGClassName(id object) {
    return object ? (NSStringFromClass([object class]) ?: @"") : @"";
}

static BOOL ACGIsCommentControllerName(NSString *name) {
    if (name.length == 0) return NO;
    return [name containsString:@"CommentContainerInnerViewController"] ||
           ([name containsString:@"AWECommentPanelContainerSwiftImpl"] &&
            [name containsString:@"Comment"]);
}

static BOOL ACGIsViewInsideVisibleComment(UIView *view) {
    if (!view || !view.window) return NO;
    UIResponder *responder = view;
    NSInteger guard = 0;
    while (responder && guard++ < 80) {
        if ([responder isKindOfClass:UIViewController.class]) {
            UIViewController *controller = (UIViewController *)responder;
            if (controller == gACGVisibleCommentController ||
                ACGIsCommentControllerName(ACGClassName(controller))) {
                return YES;
            }
        }
        responder = responder.nextResponder;
    }
    return NO;
}

static UIVisualEffect *ACGCreateClearLiquidGlass(void) {
    Class glassClass = NSClassFromString(@"UIGlassEffect");
    if (!glassClass) return nil;

    id effect = nil;
    SEL effectWithStyle = NSSelectorFromString(@"effectWithStyle:");
    if ([glassClass respondsToSelector:effectWithStyle]) {
        @try {
            effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(
                glassClass, effectWithStyle, 0);
        } @catch (__unused NSException *exception) {
            effect = nil;
        }
    }

    if (!effect) {
        SEL initWithStyle = NSSelectorFromString(@"initWithStyle:");
        if ([glassClass instancesRespondToSelector:initWithStyle]) {
            @try {
                effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(
                    [glassClass alloc], initWithStyle, 0);
            } @catch (__unused NSException *exception) {
                effect = nil;
            }
        }
    }

    if (!effect && [glassClass instancesRespondToSelector:@selector(init)]) {
        @try {
            effect = ((id (*)(id, SEL))objc_msgSend)(
                [glassClass alloc], @selector(init));
        } @catch (__unused NSException *exception) {
            effect = nil;
        }
    }

    if (!effect || ![effect isKindOfClass:UIVisualEffect.class]) return nil;

    SEL setTintColor = NSSelectorFromString(@"setTintColor:");
    if ([effect respondsToSelector:setTintColor]) {
        ((void (*)(id, SEL, id))objc_msgSend)(effect, setTintColor, nil);
    }

    SEL setInteractive = NSSelectorFromString(@"setInteractive:");
    if ([effect respondsToSelector:setInteractive]) {
        ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, setInteractive, NO);
    }

    return (UIVisualEffect *)effect;
}

static UIView *ACGFindDescendantOfClass(UIView *root, Class cls, NSUInteger limit) {
    if (!root || !cls) return nil;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    NSUInteger index = 0;
    NSUInteger visited = 0;
    while (index < queue.count && visited++ < limit) {
        UIView *view = queue[index++];
        if (view != root && [view isKindOfClass:cls]) return view;
        for (UIView *subview in view.subviews) [queue addObject:subview];
    }
    return nil;
}

static UIView *ACGFindMagicViewInController(UIViewController *controller) {
    Class magicClass = NSClassFromString(@"AWEListKitMagicCollectionView");
    if (!magicClass || !controller.viewIfLoaded) return nil;
    return ACGFindDescendantOfClass(controller.viewIfLoaded, magicClass, 900);
}

static UIView *ACGFindPanelRootFromMagic(UIView *magic) {
    if (!magic || !magic.superview) return nil;

    CGFloat magicWidth = CGRectGetWidth(magic.bounds);
    CGFloat magicHeight = CGRectGetHeight(magic.bounds);
    UIView *best = nil;
    CGFloat bestScore = -CGFLOAT_MAX;

    UIView *cursor = magic.superview;
    for (NSInteger depth = 0; cursor && depth < 16; depth++, cursor = cursor.superview) {
        CGFloat width = CGRectGetWidth(cursor.bounds);
        CGFloat height = CGRectGetHeight(cursor.bounds);
        if (width < magicWidth * 0.95 || height <= magicHeight + 40.0) continue;

        CGRect magicRect = [magic convertRect:magic.bounds toView:cursor];
        CGFloat topGap = CGRectGetMinY(magicRect);
        CGFloat bottomGap = height - CGRectGetMaxY(magicRect);

        CGFloat score = 0.0;
        if ([cursor isKindOfClass:UICollectionView.class]) score += 100.0;
        if (topGap >= 24.0 && topGap <= 60.0) score += 80.0 - fabs(topGap - 40.0);
        if (bottomGap >= 64.0 && bottomGap <= 100.0) score += 100.0 - fabs(bottomGap - 83.0);
        if (fabs(width - magicWidth) <= 2.0) score += 50.0;
        if (height >= magicHeight + 100.0 && height <= magicHeight + 150.0) score += 60.0;

        if (score > bestScore) {
            bestScore = score;
            best = cursor;
        }
        if (score >= 300.0) break;
    }
    return best;
}

static BOOL ACGIsOurGlassTree(UIView *view) {
    UIView *cursor = view;
    NSInteger guard = 0;
    while (cursor && guard++ < 8) {
        if (cursor.tag == kACGHostTag ||
            cursor.tag == kACGHeaderGlassTag ||
            cursor.tag == kACGBodyGlassTag) return YES;
        cursor = cursor.superview;
    }
    return NO;
}

static BOOL ACGClassIsKnownPanelStructure(NSString *name) {
    if (name.length == 0) return NO;
    static NSArray<NSString *> *needles;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        needles = @[
            @"AWEBaseListSectionBackgroundView",
            @"AWETabContainerSectionCell",
            @"AWEListKitTabContentCollectionView",
            @"AWETabContentItemContainerCell",
            @"AWEListKitMagicCollectionView",
            @"AWECommentPanelListSwiftImpl",
            @"AWECommentPanelContainerSwiftImpl"
        ];
    });
    for (NSString *needle in needles) {
        if ([name containsString:needle]) return YES;
    }
    return NO;
}

static void ACGClearPanelStructure(UIView *root, UIView *magic) {
    if (!root || !magic) return;

    root.backgroundColor = UIColor.clearColor;
    root.layer.backgroundColor = UIColor.clearColor.CGColor;
    root.opaque = NO;
    if ([root isKindOfClass:UICollectionView.class]) {
        UICollectionView *collection = (UICollectionView *)root;
        collection.backgroundColor = UIColor.clearColor;
        collection.backgroundView.backgroundColor = UIColor.clearColor;
    }

    CGFloat rootWidth = CGRectGetWidth(root.bounds);
    CGFloat magicHeight = CGRectGetHeight(magic.bounds);
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    NSUInteger index = 0;
    NSUInteger visited = 0;

    while (index < queue.count && visited++ < 900) {
        UIView *view = queue[index++];
        for (UIView *subview in view.subviews) {
            if (!ACGIsOurGlassTree(subview)) [queue addObject:subview];
        }
        if (view == root || ACGIsOurGlassTree(view)) continue;

        NSString *name = ACGClassName(view);
        CGRect rect = [view convertRect:view.bounds toView:root];
        CGFloat width = CGRectGetWidth(rect);
        CGFloat height = CGRectGetHeight(rect);

        BOOL known = ACGClassIsKnownPanelStructure(name);
        BOOL hugePlain = ([view isMemberOfClass:UIView.class] &&
                          width >= rootWidth * 0.96 &&
                          height >= magicHeight * 0.80);
        BOOL collectionStructure = ([view isKindOfClass:UICollectionView.class] &&
                                    width >= rootWidth * 0.96 &&
                                    height >= magicHeight * 0.80);

        if (known || hugePlain || collectionStructure) {
            view.backgroundColor = UIColor.clearColor;
            view.layer.backgroundColor = UIColor.clearColor.CGColor;
            view.opaque = NO;
            if ([view isKindOfClass:UICollectionView.class]) {
                UICollectionView *collection = (UICollectionView *)view;
                collection.backgroundView.backgroundColor = UIColor.clearColor;
            }
        }
    }
}

static UIView *ACGFindInputRoot(UIView *panelRoot) {
    if (!panelRoot || !panelRoot.window) return nil;
    Class inputClass = NSClassFromString(@"AWECommentInputViewSwiftImpl.CommentInputContainerView");
    if (!inputClass) return nil;

    UIView *inside = ACGFindDescendantOfClass(panelRoot, inputClass, 900);
    if (inside) return inside;

    UIWindow *window = panelRoot.window;
    CGRect panelRect = [panelRoot convertRect:panelRoot.bounds toView:window];
    CGFloat panelWidth = CGRectGetWidth(panelRect);
    UIView *best = nil;
    CGFloat bestScore = -CGFLOAT_MAX;

    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
    NSUInteger index = 0;
    NSUInteger visited = 0;
    while (index < queue.count && visited++ < 1400) {
        UIView *view = queue[index++];
        for (UIView *subview in view.subviews) [queue addObject:subview];
        if (![view isKindOfClass:inputClass] || !view.window || view.hidden || view.alpha < 0.01) continue;

        CGRect rect = [view convertRect:view.bounds toView:window];
        CGFloat width = CGRectGetWidth(rect);
        CGFloat height = CGRectGetHeight(rect);
        if (width < panelWidth * 0.85 || height < 55.0 || height > 115.0) continue;

        CGFloat score = 0.0;
        score -= fabs(width - panelWidth);
        score -= fabs(height - 83.0) * 2.0;
        score -= fabs(CGRectGetMaxY(rect) - CGRectGetMaxY(panelRect));
        if (CGRectIntersectsRect(rect, panelRect)) score += 100.0;
        if (score > bestScore) {
            bestScore = score;
            best = view;
        }
    }
    return best;
}

static BOOL ACGLooksLikeNativeInputPill(UIView *view, UIView *inputRoot) {
    if (!view || !inputRoot || view == inputRoot) return NO;
    CGRect rect = [view convertRect:view.bounds toView:inputRoot];
    CGFloat rootWidth = CGRectGetWidth(inputRoot.bounds);
    CGFloat width = CGRectGetWidth(rect);
    CGFloat height = CGRectGetHeight(rect);
    return CGRectGetMinX(rect) >= 6.0 && CGRectGetMinX(rect) <= 24.0 &&
           CGRectGetMinY(rect) >= 0.0 && CGRectGetMinY(rect) <= 18.0 &&
           width >= rootWidth - 42.0 && width <= rootWidth - 12.0 &&
           height >= 30.0 && height <= 44.0;
}

static void ACGClearOuterInputBackgroundOnly(UIView *inputRoot) {
    if (!inputRoot) return;

    inputRoot.backgroundColor = UIColor.clearColor;
    inputRoot.layer.backgroundColor = UIColor.clearColor.CGColor;
    inputRoot.opaque = NO;
    inputRoot.clipsToBounds = NO;

    CGFloat rootWidth = CGRectGetWidth(inputRoot.bounds);
    CGFloat rootHeight = CGRectGetHeight(inputRoot.bounds);
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:inputRoot];
    NSUInteger index = 0;
    NSUInteger visited = 0;

    while (index < queue.count && visited++ < 260) {
        UIView *view = queue[index++];
        for (UIView *subview in view.subviews) [queue addObject:subview];
        if (view == inputRoot) continue;
        if (ACGLooksLikeNativeInputPill(view, inputRoot)) continue;

        CGRect rect = [view convertRect:view.bounds toView:inputRoot];
        CGFloat width = CGRectGetWidth(rect);
        CGFloat height = CGRectGetHeight(rect);
        NSString *name = ACGClassName(view);

        BOOL fullBar = width >= rootWidth * 0.90 && height >= rootHeight * 0.68;
        BOOL namedBackground = ([name containsString:@"Background"] ||
                                [name containsString:@"BlurView"] ||
                                [name containsString:@"SkinView"]) &&
                               width >= rootWidth * 0.85 && height >= rootHeight * 0.55;
        if (fullBar || namedBackground) {
            view.backgroundColor = UIColor.clearColor;
            view.layer.backgroundColor = UIColor.clearColor.CGColor;
            view.opaque = NO;
        }
    }
}

static UIVisualEffectView *ACGEnsureGlass(UIView *host,
                                         const void *key,
                                         NSInteger tag) {
    UIVisualEffectView *glass = objc_getAssociatedObject(host, key);
    if (glass && ![glass isKindOfClass:UIVisualEffectView.class]) {
        [glass removeFromSuperview];
        glass = nil;
    }
    if (!glass) {
        UIVisualEffect *effect = ACGCreateClearLiquidGlass();
        if (!effect) return nil;

        glass = [[UIVisualEffectView alloc] initWithEffect:nil];
        glass.tag = tag;
        glass.userInteractionEnabled = NO;
        glass.backgroundColor = UIColor.clearColor;
        glass.opaque = NO;
        glass.alpha = 1.0;
        glass.layer.cornerRadius = 0.0;
        glass.layer.masksToBounds = NO;
        [host addSubview:glass];
        glass.effect = effect;
        objc_setAssociatedObject(host, key, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return glass;
}

static UIView *ACGEnsureGlassHost(UIView *panelRoot) {
    UIView *host = objc_getAssociatedObject(panelRoot, &kACGGlassHostKey);
    if (!host || host.superview != panelRoot) {
        host = [[UIView alloc] initWithFrame:CGRectZero];
        host.tag = kACGHostTag;
        host.userInteractionEnabled = NO;
        host.backgroundColor = UIColor.clearColor;
        host.opaque = NO;
        [panelRoot insertSubview:host atIndex:0];
        objc_setAssociatedObject(panelRoot, &kACGGlassHostKey,
                                 host, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return host;
}

static void ACGInstallTwoRegionGlass(UIView *panelRoot, UIView *magic) {
    if (!panelRoot || !magic) return;

    UIView *host = ACGEnsureGlassHost(panelRoot);
    if (!host) return;

    CGPoint contentOffset = CGPointZero;
    if ([panelRoot isKindOfClass:UIScrollView.class]) {
        contentOffset = ((UIScrollView *)panelRoot).contentOffset;
    }
    host.frame = CGRectMake(contentOffset.x, contentOffset.y,
                            CGRectGetWidth(panelRoot.bounds),
                            CGRectGetHeight(panelRoot.bounds));

    CGRect magicRectInRoot = [magic convertRect:magic.bounds toView:panelRoot];
    magicRectInRoot.origin.x -= contentOffset.x;
    magicRectInRoot.origin.y -= contentOffset.y;

    CGFloat hostWidth = CGRectGetWidth(host.bounds);
    CGFloat hostHeight = CGRectGetHeight(host.bounds);
    CGFloat headerHeight = MAX(0.0, MIN(CGRectGetMinY(magicRectInRoot), hostHeight));
    CGRect headerFrame = CGRectMake(0.0, 0.0, hostWidth, headerHeight);
    CGRect bodyFrame = CGRectIntersection(host.bounds, magicRectInRoot);

    UIVisualEffectView *headerGlass =
        ACGEnsureGlass(host, &kACGHeaderGlassKey, kACGHeaderGlassTag);
    UIVisualEffectView *bodyGlass =
        ACGEnsureGlass(host, &kACGBodyGlassKey, kACGBodyGlassTag);
    if (!headerGlass || !bodyGlass) return;

    headerGlass.hidden = CGRectGetHeight(headerFrame) < 8.0;
    bodyGlass.hidden = CGRectGetHeight(bodyFrame) < 40.0;
    headerGlass.frame = CGRectIntegral(headerFrame);
    bodyGlass.frame = CGRectIntegral(bodyFrame);

    if (host.superview == panelRoot) [panelRoot sendSubviewToBack:host];
}

static void ACGSyncFromMagicView(UIView *magic) {
    if (!magic || !magic.window || !ACGIsViewInsideVisibleComment(magic)) return;

    UIView *panelRoot = ACGFindPanelRootFromMagic(magic);
    if (!panelRoot) return;

    gACGLastMagicView = magic;
    gACGLastPanelRoot = panelRoot;

    ACGClearPanelStructure(panelRoot, magic);
    UIView *inputRoot = ACGFindInputRoot(panelRoot);
    if (inputRoot) {
        gACGLastInputRoot = inputRoot;
        ACGClearOuterInputBackgroundOnly(inputRoot);
    }
    ACGInstallTwoRegionGlass(panelRoot, magic);
}

static void ACGScheduleSync(UIView *magic) {
    if (!magic) return;
    __weak UIView *weakMagic = magic;
    NSArray<NSNumber *> *delays = @[@0.0, @0.08, @0.25, @0.60];
    for (NSNumber *delay in delays) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                       (int64_t)(delay.doubleValue * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            UIView *strongMagic = weakMagic;
            if (strongMagic.window) ACGSyncFromMagicView(strongMagic);
        });
    }
}

%hook AWEListKitMagicCollectionView

- (void)didMoveToWindow {
    %orig;
    UIView *view = (UIView *)self;
    if (view.window && ACGIsViewInsideVisibleComment(view)) {
        ACGScheduleSync(view);
    }
}

- (void)layoutSubviews {
    %orig;
    UIView *view = (UIView *)self;
    if (view.window && ACGIsViewInsideVisibleComment(view)) {
        ACGSyncFromMagicView(view);
    }
}

- (void)setBackgroundColor:(UIColor *)color {
    UIView *view = (UIView *)self;
    if (view.window && ACGIsViewInsideVisibleComment(view)) {
        %orig(UIColor.clearColor);
        view.opaque = NO;
    } else {
        %orig(color);
    }
}

%end

%hook AWEBaseListSectionBackgroundView

- (void)setBackgroundColor:(UIColor *)color {
    UIView *view = (UIView *)self;
    if (view.window && ACGIsViewInsideVisibleComment(view)) {
        %orig(UIColor.clearColor);
        view.opaque = NO;
    } else {
        %orig(color);
    }
}

- (void)didMoveToWindow {
    %orig;
    UIView *view = (UIView *)self;
    if (view.window && ACGIsViewInsideVisibleComment(view)) {
        view.backgroundColor = UIColor.clearColor;
        view.layer.backgroundColor = UIColor.clearColor.CGColor;
        view.opaque = NO;
    }
}

%end

%hook AWEListKitTabContentCollectionView

- (void)setBackgroundColor:(UIColor *)color {
    UIView *view = (UIView *)self;
    if (view.window && ACGIsViewInsideVisibleComment(view)) {
        %orig(UIColor.clearColor);
        view.opaque = NO;
    } else {
        %orig(color);
    }
}

%end

%group ACGCommentControllerGroup
%hook ACGCommentContainerInnerViewController

- (void)viewWillAppear:(BOOL)animated {
    gACGVisibleCommentController = (UIViewController *)self;
    %orig(animated);
}

- (void)viewDidLayoutSubviews {
    %orig;
    UIView *magic = ACGFindMagicViewInController((UIViewController *)self);
    if (magic) ACGSyncFromMagicView(magic);
}

- (void)viewDidAppear:(BOOL)animated {
    %orig(animated);
    gACGVisibleCommentController = (UIViewController *)self;
    UIView *magic = ACGFindMagicViewInController((UIViewController *)self);
    if (magic) ACGScheduleSync(magic);
}

- (void)viewDidDisappear:(BOOL)animated {
    %orig(animated);
    if (gACGVisibleCommentController == (UIViewController *)self) {
        gACGVisibleCommentController = nil;
        gACGLastMagicView = nil;
        gACGLastPanelRoot = nil;
        gACGLastInputRoot = nil;
    }
}

%end
%end

%ctor {
    @autoreleasepool {
        %init;
        Class commentClass = objc_getClass(
            "AWECommentPanelContainerSwiftImpl.CommentContainerInnerViewController");
        if (commentClass) {
            %init(ACGCommentControllerGroup,
                  ACGCommentContainerInnerViewController = commentClass);
        }
        NSLog(@"[AwemeLiquidBar.CommentGlassFlex] fresh v1 loaded");
    }
}
