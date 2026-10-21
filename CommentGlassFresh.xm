#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>

// Fresh comment-glass implementation built from auth44 baseline only.
// No code from the failed c1/c2/flex experiments is reused here.

static const NSInteger kAWEFreshCommentGlassTag = 26082001;
static BOOL gAWEFreshCommentVisible = NO;
extern "C" BOOL AWEFreshCommentGlassVisible(void) { return gAWEFreshCommentVisible; }
static NSString *gAWEFreshCommentLastLogSignature = nil;

static NSString *AWEFreshDocumentsPath(NSString *name) {
    NSString *documents = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                                NSUserDomainMask,
                                                                YES) firstObject];
    return [documents stringByAppendingPathComponent:name];
}

static void AWEFreshProbeLog(NSString *format, ...) {
    if (format.length == 0) return;
    va_list args;
    va_start(args, format);
    NSString *line = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    NSString *record = [NSString stringWithFormat:@"%@ %@\n", [NSDate date], line ?: @""];
    NSData *data = [record dataUsingEncoding:NSUTF8StringEncoding];
    NSString *path = AWEFreshDocumentsPath(@"AwemeLiquidBar_comment_message_probe.log");
    if (![[NSFileManager defaultManager] fileExistsAtPath:path]) {
        [data writeToFile:path atomically:YES];
        return;
    }
    NSFileHandle *handle = [NSFileHandle fileHandleForWritingAtPath:path];
    if (!handle) return;
    @try {
        [handle seekToEndOfFile];
        [handle writeData:data];
        [handle closeFile];
    } @catch (__unused NSException *exception) {
    }
}

static BOOL AWEFreshIsCommentInnerController(UIViewController *controller) {
    if (!controller) return NO;
    NSString *name = NSStringFromClass(controller.class);
    return [name containsString:@"CommentContainerInnerViewController"] ||
           ([name containsString:@"AWECommentPanelContainerSwiftImpl"] &&
            [name containsString:@"Comment"]);
}

static UIViewController *AWEFreshControllerForView(UIView *view) {
    UIResponder *responder = view;
    NSInteger guard = 0;
    while (responder && guard++ < 80) {
        if ([responder isKindOfClass:UIViewController.class]) {
            return (UIViewController *)responder;
        }
        responder = responder.nextResponder;
    }
    return nil;
}

static BOOL AWEFreshViewInsideComment(UIView *view) {
    UIViewController *controller = AWEFreshControllerForView(view);
    for (NSInteger depth = 0; controller && depth < 10; depth++, controller = controller.parentViewController) {
        NSString *name = NSStringFromClass(controller.class);
        if ([name.lowercaseString containsString:@"comment"]) return YES;
    }
    return NO;
}

static UIVisualEffect *AWEFreshCreateClearGlass(void) {
    if (@available(iOS 26.0, *)) {
        Class glassClass = NSClassFromString(@"UIGlassEffect");
        SEL selector = NSSelectorFromString(@"effectWithStyle:");
        if (glassClass && [glassClass respondsToSelector:selector]) {
            // UIGlassEffectStyleClear = 1 in the iOS 26 SDK. Use the runtime
            // call so the tweak remains loadable on older deployment targets.
            id effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(glassClass, selector, 1);
            SEL setInteractive = NSSelectorFromString(@"setInteractive:");
            if ([effect respondsToSelector:setInteractive]) {
                ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, setInteractive, YES);
            }
            SEL setTintColor = NSSelectorFromString(@"setTintColor:");
            if ([effect respondsToSelector:setTintColor]) {
                ((void (*)(id, SEL, id))objc_msgSend)(effect, setTintColor, nil);
            }
            if ([effect isKindOfClass:UIVisualEffect.class]) return effect;
        }
    }
    return nil;
}

static BOOL AWEFreshIsOurGlassTree(UIView *view) {
    UIView *cursor = view;
    NSInteger depth = 0;
    while (cursor && depth++ < 6) {
        if (cursor.tag == kAWEFreshCommentGlassTag) return YES;
        cursor = cursor.superview;
    }
    return NO;
}

static BOOL AWEFreshShouldClearCommentBackground(UIView *view, UIView *host) {
    if (!view || view == host || AWEFreshIsOurGlassTree(view)) return NO;
    if ([view isKindOfClass:UIVisualEffectView.class] ||
        [view isKindOfClass:UILabel.class] ||
        [view isKindOfClass:UIButton.class] ||
        [view isKindOfClass:UIImageView.class] ||
        [view isKindOfClass:UITextField.class] ||
        [view isKindOfClass:UITextView.class]) return NO;

    NSString *name = NSStringFromClass(view.class);
    NSArray<NSString *> *known = @[
        @"AWEBaseListSectionBackgroundView",
        @"AWETabContainerSectionCell",
        @"AWETabContentItemContainerCell",
        @"AWEListKitTabContentCollectionView",
        @"AWEListKitMagicCollectionView"
    ];
    for (NSString *needle in known) {
        if ([name containsString:needle]) return YES;
    }

    CGRect converted = [view convertRect:view.bounds toView:host];
    CGFloat hostWidth = CGRectGetWidth(host.bounds);
    BOOL largePlain = [view class] == UIView.class &&
                      CGRectGetWidth(converted) >= hostWidth * 0.88 &&
                      CGRectGetHeight(converted) >= 70.0;
    if (!largePlain) return NO;
    UIColor *color = view.backgroundColor;
    return color && CGColorGetAlpha(color.CGColor) > 0.15;
}

static void AWEFreshClearCommentBackgrounds(UIView *host) {
    if (!host) return;
    host.backgroundColor = UIColor.clearColor;
    host.layer.backgroundColor = UIColor.clearColor.CGColor;
    host.opaque = NO;

    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:host];
    NSUInteger index = 0;
    NSUInteger visited = 0;
    while (index < queue.count && visited++ < 650) {
        UIView *view = queue[index++];
        for (UIView *subview in [view.subviews copy]) {
            if (AWEFreshIsOurGlassTree(subview)) continue;
            if (AWEFreshShouldClearCommentBackground(subview, host)) {
                subview.backgroundColor = UIColor.clearColor;
                subview.layer.backgroundColor = UIColor.clearColor.CGColor;
                subview.opaque = NO;
                if ([subview isKindOfClass:UICollectionView.class]) {
                    UICollectionView *collection = (UICollectionView *)subview;
                    collection.backgroundColor = UIColor.clearColor;
                    collection.backgroundView.backgroundColor = UIColor.clearColor;
                }
            }
            [queue addObject:subview];
        }
    }
}

static void AWEFreshInstallPanelGlass(UIViewController *controller, NSString *reason) {
    if (!AWEFreshIsCommentInnerController(controller)) return;
    UIView *host = controller.view;
    if (!host || !host.window || CGRectIsEmpty(host.bounds)) return;

    AWEFreshClearCommentBackgrounds(host);

    UIVisualEffectView *glass = (UIVisualEffectView *)[host viewWithTag:kAWEFreshCommentGlassTag];
    if (glass && ![glass isKindOfClass:UIVisualEffectView.class]) {
        [glass removeFromSuperview];
        glass = nil;
    }
    if (!glass) {
        UIVisualEffect *effect = AWEFreshCreateClearGlass();
        if (!effect) {
            AWEFreshProbeLog(@"COMMENT_GLASS_CREATE_FAIL host=%@ frame=%@",
                             NSStringFromClass(host.class), NSStringFromCGRect(host.frame));
            return;
        }
        glass = [[UIVisualEffectView alloc] initWithEffect:nil];
        glass.tag = kAWEFreshCommentGlassTag;
        glass.userInteractionEnabled = NO;
        glass.backgroundColor = UIColor.clearColor;
        glass.opaque = NO;
        glass.alpha = 1.0;
        glass.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        [host insertSubview:glass atIndex:0];
        glass.effect = effect;
    }
    glass.frame = host.bounds;
    glass.layer.cornerRadius = 0.0;
    glass.layer.borderWidth = 0.0;
    glass.layer.borderColor = UIColor.clearColor.CGColor;
    if (host.subviews.firstObject != glass) [host sendSubviewToBack:glass];

    NSString *signature = [NSString stringWithFormat:@"%@|%@|%@|%@",
                           NSStringFromClass(controller.class),
                           NSStringFromCGRect(host.frame),
                           NSStringFromClass(glass.effect.class),
                           reason ?: @""];
    if (![signature isEqualToString:gAWEFreshCommentLastLogSignature]) {
        gAWEFreshCommentLastLogSignature = [signature copy];
        AWEFreshProbeLog(@"COMMENT_PANEL hostVC=%@ host=%@ frame=%@ bounds=%@ glass=%@ reason=%@",
                         NSStringFromClass(controller.class), NSStringFromClass(host.class),
                         NSStringFromCGRect(host.frame), NSStringFromCGRect(host.bounds),
                         NSStringFromClass(glass.effect.class), reason ?: @"");
    }
}

static UIView *AWEFreshFindInputPill(UIView *container) {
    if (!container) return nil;
    CGFloat width = CGRectGetWidth(container.bounds);
    UIView *best = nil;
    CGFloat bestScore = -CGFLOAT_MAX;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:container];
    NSUInteger index = 0;
    NSUInteger visited = 0;
    while (index < queue.count && visited++ < 250) {
        UIView *view = queue[index++];
        for (UIView *subview in view.subviews) [queue addObject:subview];
        if (view == container || AWEFreshIsOurGlassTree(view)) continue;
        CGRect r = [view convertRect:view.bounds toView:container];
        CGFloat w = CGRectGetWidth(r), h = CGRectGetHeight(r);
        if (w < width * 0.82 || w > width * 0.99) continue;
        if (h < 28.0 || h > 48.0) continue;
        if (CGRectGetMinX(r) < 5.0 || CGRectGetMinX(r) > 28.0) continue;
        if (CGRectGetMinY(r) < 0.0 || CGRectGetMinY(r) > 20.0) continue;
        CGFloat score = 0.0;
        score -= fabs(w - (width - 24.0));
        score -= fabs(h - 36.0) * 3.0;
        if ([view class] == UIView.class) score += 25.0;
        if (score > bestScore) { bestScore = score; best = view; }
    }
    return best;
}

static void AWEFreshClearInputContainer(UIView *container) {
    if (!container || !container.window) return;
    UIView *pill = AWEFreshFindInputPill(container);
    container.backgroundColor = UIColor.clearColor;
    container.layer.backgroundColor = UIColor.clearColor.CGColor;
    container.opaque = NO;
    container.clipsToBounds = NO;

    for (UIView *subview in [container.subviews copy]) {
        if (pill && (subview == pill || [pill isDescendantOfView:subview] ||
            [subview isDescendantOfView:pill])) continue;
        CGRect r = [subview convertRect:subview.bounds toView:container];
        BOOL fullWidth = CGRectGetWidth(r) >= CGRectGetWidth(container.bounds) * 0.88;
        BOOL backgroundLike = fullWidth && CGRectGetHeight(r) >= 48.0;
        if (backgroundLike && ![subview isKindOfClass:UIControl.class] &&
            ![subview isKindOfClass:UILabel.class] &&
            ![subview isKindOfClass:UIImageView.class]) {
            subview.backgroundColor = UIColor.clearColor;
            subview.layer.backgroundColor = UIColor.clearColor.CGColor;
            subview.opaque = NO;
        }
    }

    static NSString *lastInputSignature = nil;
    NSString *signature = [NSString stringWithFormat:@"%@|%@|%@",
                           NSStringFromClass(container.class),
                           NSStringFromCGRect(container.frame),
                           pill ? NSStringFromCGRect([pill convertRect:pill.bounds toView:container]) : @"nil"];
    if (![signature isEqualToString:lastInputSignature]) {
        lastInputSignature = [signature copy];
        AWEFreshProbeLog(@"COMMENT_INPUT container=%@ frame=%@ pill=%@ pillFrame=%@",
                         NSStringFromClass(container.class), NSStringFromCGRect(container.frame),
                         pill ? NSStringFromClass(pill.class) : @"nil",
                         pill ? NSStringFromCGRect([pill convertRect:pill.bounds toView:container]) : @"nil");
    }
}

static void AWEFreshScanInputContainers(UIView *root) {
    if (!root) return;
    Class inputClass = NSClassFromString(@"AWECommentInputViewSwiftImpl.CommentInputContainerView");
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    NSUInteger index = 0;
    NSUInteger visited = 0;
    while (index < queue.count && visited++ < 700) {
        UIView *view = queue[index++];
        if (inputClass && [view isKindOfClass:inputClass]) {
            AWEFreshClearInputContainer(view);
            continue;
        }
        for (UIView *subview in view.subviews) [queue addObject:subview];
    }
}

%hook AWEBaseListViewController
- (void)viewDidLayoutSubviews {
    %orig;
    if (AWEFreshIsCommentInnerController((UIViewController *)self)) {
        AWEFreshInstallPanelGlass((UIViewController *)self, @"base-list-layout");
        AWEFreshScanInputContainers(((UIViewController *)self).view);
    }
}
- (void)viewDidAppear:(BOOL)animated {
    %orig(animated);
    if (AWEFreshIsCommentInnerController((UIViewController *)self)) {
        AWEFreshInstallPanelGlass((UIViewController *)self, @"base-list-appear");
        AWEFreshScanInputContainers(((UIViewController *)self).view);
    }
}
%end

%hook AWECommentContainerViewController
- (void)viewDidAppear:(BOOL)animated {
    %orig(animated);
    gAWEFreshCommentVisible = YES;
    AWEFreshProbeLog(@"COMMENT_VISIBLE 1 vc=%@ frame=%@",
                     NSStringFromClass(((UIViewController *)self).class),
                     NSStringFromCGRect(((UIViewController *)self).view.frame));
    AWEFreshScanInputContainers(((UIViewController *)self).view);
}
- (void)viewDidLayoutSubviews {
    %orig;
    gAWEFreshCommentVisible = YES;
    AWEFreshScanInputContainers(((UIViewController *)self).view);
}
- (void)viewDidDisappear:(BOOL)animated {
    %orig(animated);
    gAWEFreshCommentVisible = NO;
    AWEFreshProbeLog(@"COMMENT_VISIBLE 0 vc=%@", NSStringFromClass(((UIViewController *)self).class));
}
%end

%hook AWEBaseListSectionBackgroundView
- (void)setBackgroundColor:(UIColor *)color {
    if (AWEFreshViewInsideComment((UIView *)self)) {
        %orig(UIColor.clearColor);
        ((UIView *)self).opaque = NO;
        return;
    }
    %orig(color);
}
%end

%hook AWEListKitTabContentCollectionView
- (void)setBackgroundColor:(UIColor *)color {
    if (AWEFreshViewInsideComment((UIView *)self)) {
        %orig(UIColor.clearColor);
        ((UIView *)self).opaque = NO;
        return;
    }
    %orig(color);
}
%end

%hook AWETabContainerSectionCell
- (void)setBackgroundColor:(UIColor *)color {
    if (AWEFreshViewInsideComment((UIView *)self)) {
        %orig(UIColor.clearColor);
        ((UIView *)self).opaque = NO;
        return;
    }
    %orig(color);
}
%end

%hook AWETabContentItemContainerCell
- (void)setBackgroundColor:(UIColor *)color {
    if (AWEFreshViewInsideComment((UIView *)self)) {
        %orig(UIColor.clearColor);
        ((UIView *)self).opaque = NO;
        return;
    }
    %orig(color);
}
%end

%hook AWEListKitMagicCollectionView
- (void)layoutSubviews {
    %orig;
    UIView *view = (UIView *)self;
    if (!AWEFreshViewInsideComment(view)) return;
    view.backgroundColor = UIColor.clearColor;
    view.layer.backgroundColor = UIColor.clearColor.CGColor;
    view.opaque = NO;
}
%end

%hook BDMultiContentContainer_ImageContentView
- (void)setTransform:(CGAffineTransform)transform {
    if (gAWEFreshCommentVisible) return;
    %orig(transform);
}
%end

%ctor {
    @autoreleasepool {
        AWEFreshProbeLog(@"SESSION fresh-comment-glass-v2 loaded os=%@",
                         NSProcessInfo.processInfo.operatingSystemVersionString);
    }
}
