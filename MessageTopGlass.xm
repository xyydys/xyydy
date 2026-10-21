#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>

static const NSInteger kAWEMessageGlassPlateBaseTag = 26082020;
static char kAWEMessageOriginalScrollFrameKey;
static char kAWEMessageOriginalScrollInsetKey;
static NSString *gAWEMessageLastProbeSignature = nil;

static NSString *AWEMessageDocumentsPath(NSString *name) {
    NSString *documents = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,
                                                                NSUserDomainMask,
                                                                YES) firstObject];
    return [documents stringByAppendingPathComponent:name];
}

static void AWEMessageProbeLog(NSString *format, ...) {
    if (format.length == 0) return;
    va_list args;
    va_start(args, format);
    NSString *line = [[NSString alloc] initWithFormat:format arguments:args];
    va_end(args);
    NSString *record = [NSString stringWithFormat:@"%@ %@\n", [NSDate date], line ?: @""];
    NSData *data = [record dataUsingEncoding:NSUTF8StringEncoding];
    NSString *path = AWEMessageDocumentsPath(@"AwemeLiquidBar_comment_message_probe.log");
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

static BOOL AWEMessageControllerIsRoot(UIViewController *controller) {
    if (!controller) return NO;
    NSString *name = NSStringFromClass(controller.class);
    return [name isEqualToString:@"AWEIMMessageTabViewController"] ||
           [name isEqualToString:@"AWEIMMessageTabRootViewController"];
}

static UIViewController *AWEMessageRootControllerForView(UIView *view) {
    UIResponder *responder = view;
    NSInteger guard = 0;
    while (responder && guard++ < 80) {
        if ([responder isKindOfClass:UIViewController.class]) {
            UIViewController *vc = (UIViewController *)responder;
            UIViewController *cursor = vc;
            for (NSInteger depth = 0; cursor && depth < 8; depth++, cursor = cursor.parentViewController) {
                if (AWEMessageControllerIsRoot(cursor)) return cursor;
            }
        }
        responder = responder.nextResponder;
    }
    return nil;
}

static NSArray<UIView *> *AWEMessageAllSubviews(UIView *root, NSUInteger maxVisited) {
    if (!root) return @[];
    NSMutableArray<UIView *> *result = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    NSUInteger index = 0;
    NSUInteger visited = 0;
    while (index < queue.count && visited++ < maxVisited) {
        UIView *view = queue[index++];
        if (view != root) [result addObject:view];
        for (UIView *subview in view.subviews) [queue addObject:subview];
    }
    return result;
}

static UIView *AWEMessageDirectChildUnderRoot(UIView *view, UIView *root) {
    if (!view || !root) return nil;
    UIView *cursor = view;
    while (cursor.superview && cursor.superview != root) cursor = cursor.superview;
    return cursor.superview == root ? cursor : nil;
}

static UIView *AWEMessageCompactSlotAncestor(UIView *view, UIView *root) {
    if (!view || !root) return nil;
    UIView *best = view;
    UIView *cursor = view;
    NSInteger guard = 0;
    while (cursor.superview && cursor.superview != root && guard++ < 12) {
        UIView *parent = cursor.superview;
        CGRect r = [parent convertRect:parent.bounds toView:root];
        if (CGRectGetWidth(r) <= 82.0 && CGRectGetHeight(r) <= 72.0) {
            best = parent;
            cursor = parent;
        } else {
            break;
        }
    }
    return best;
}

static UIButtonConfiguration *AWEMessageClearGlassConfiguration(void) {
    if (@available(iOS 26.0, *)) {
        Class configurationClass = NSClassFromString(@"UIButtonConfiguration");
        SEL selector = NSSelectorFromString(@"clearGlassButtonConfiguration");
        if (configurationClass && [configurationClass respondsToSelector:selector]) {
            return ((id (*)(id, SEL))objc_msgSend)(configurationClass, selector);
        }
    }
    return nil;
}

static UIButton *AWEMessageEnsurePlate(UIView *navBar, NSInteger index) {
    NSInteger tag = kAWEMessageGlassPlateBaseTag + index;
    UIButton *plate = (UIButton *)[navBar viewWithTag:tag];
    if (plate && ![plate isKindOfClass:UIButton.class]) {
        [plate removeFromSuperview];
        plate = nil;
    }
    if (!plate) {
        plate = [UIButton buttonWithType:UIButtonTypeSystem];
        plate.tag = tag;
        plate.userInteractionEnabled = NO;
        plate.backgroundColor = UIColor.clearColor;
        plate.opaque = NO;
        plate.clipsToBounds = NO;
        UIButtonConfiguration *configuration = AWEMessageClearGlassConfiguration();
        if (configuration) {
            configuration.title = nil;
            configuration.subtitle = nil;
            configuration.image = nil;
            configuration.contentInsets = NSDirectionalEdgeInsetsZero;
            configuration.baseBackgroundColor = nil;
            plate.configuration = configuration;
        }
        [navBar addSubview:plate];
    }
    return plate;
}

static void AWEMessageClearNavigationBackground(UIView *navBar) {
    navBar.backgroundColor = UIColor.clearColor;
    navBar.layer.backgroundColor = UIColor.clearColor.CGColor;
    navBar.opaque = NO;
    CGFloat width = CGRectGetWidth(navBar.bounds);
    CGFloat height = CGRectGetHeight(navBar.bounds);
    for (UIView *view in AWEMessageAllSubviews(navBar, 220)) {
        if (view.tag >= kAWEMessageGlassPlateBaseTag && view.tag < kAWEMessageGlassPlateBaseTag + 10) continue;
        NSString *name = NSStringFromClass(view.class);
        CGRect r = [view convertRect:view.bounds toView:navBar];
        BOOL large = CGRectGetWidth(r) >= width * 0.80 && CGRectGetHeight(r) >= height * 0.55;
        BOOL backgroundNamed = [name containsString:@"BarBackground"] ||
                               [name containsString:@"BackgroundView"] ||
                               [name containsString:@"NavigationBarBackground"];
        if ((large || backgroundNamed) &&
            ![view isKindOfClass:UIControl.class] &&
            ![view isKindOfClass:UILabel.class] &&
            ![view isKindOfClass:UIImageView.class]) {
            view.backgroundColor = UIColor.clearColor;
            view.layer.backgroundColor = UIColor.clearColor.CGColor;
            view.opaque = NO;
            if ([view isKindOfClass:UIVisualEffectView.class]) {
                ((UIVisualEffectView *)view).effect = nil;
            }
        }
    }
}

static UILabel *AWEMessageFindTitleLabel(UIView *navBar) {
    UILabel *best = nil;
    CGFloat bestDistance = CGFLOAT_MAX;
    CGFloat centerX = CGRectGetMidX(navBar.bounds);
    for (UIView *view in AWEMessageAllSubviews(navBar, 220)) {
        if (![view isKindOfClass:UILabel.class]) continue;
        UILabel *label = (UILabel *)view;
        if (![label.text isEqualToString:@"消息"]) continue;
        CGRect r = [label convertRect:label.bounds toView:navBar];
        CGFloat distance = fabs(CGRectGetMidX(r) - centerX);
        if (distance < bestDistance) { bestDistance = distance; best = label; }
    }
    return best;
}

static UIView *AWEMessageBestVisualSourceInBand(UIView *navBar, CGFloat minX, CGFloat maxX) {
    UIView *best = nil;
    CGFloat bestScore = -CGFLOAT_MAX;
    for (UIView *view in AWEMessageAllSubviews(navBar, 260)) {
        if (view.hidden || view.alpha < 0.05) continue;
        if ([view isKindOfClass:UILabel.class]) continue;
        if (view.tag >= kAWEMessageGlassPlateBaseTag && view.tag < kAWEMessageGlassPlateBaseTag + 10) continue;
        CGRect r = [view convertRect:view.bounds toView:navBar];
        CGFloat midX = CGRectGetMidX(r);
        CGFloat w = CGRectGetWidth(r), h = CGRectGetHeight(r);
        if (midX < minX || midX > maxX) continue;
        if (w < 18.0 || w > 72.0 || h < 18.0 || h > 60.0) continue;
        NSString *name = NSStringFromClass(view.class);
        CGFloat score = 0.0;
        if ([view isKindOfClass:UIControl.class]) score += 100.0;
        if ([view isKindOfClass:UIImageView.class]) score += 50.0;
        if ([name containsString:@"Button"] || [name containsString:@"ImageView"]) score += 30.0;
        if (view.accessibilityLabel.length > 0) score += 20.0;
        score -= fabs(w - 36.0);
        score -= fabs(h - 36.0);
        if (score > bestScore) { bestScore = score; best = view; }
    }
    return best;
}

static void AWEMessageHideSecondLeftIcon(UIView *navBar) {
    UIView *source = AWEMessageBestVisualSourceInBand(navBar, 92.0, 190.0);
    if (!source) return;
    UIView *target = AWEMessageCompactSlotAncestor(source, navBar) ?: source;
    // Hide only the compact colored secondary icon slot. Never hide a wide
    // leading/title container just because the icon lives inside it.
    target.hidden = YES;
}

static void AWEMessageLayoutGlassPlate(UIView *navBar,
                                       UIView *source,
                                       NSInteger index,
                                       CGSize size) {
    UIButton *plate = AWEMessageEnsurePlate(navBar, index);
    if (!source || source.hidden || !source.window || !plate.configuration) {
        plate.hidden = YES;
        return;
    }
    CGRect sourceRect = [source convertRect:source.bounds toView:navBar];
    CGPoint center = CGPointMake(CGRectGetMidX(sourceRect), CGRectGetMidY(sourceRect));
    plate.frame = CGRectMake(center.x - size.width * 0.5,
                             center.y - size.height * 0.5,
                             size.width, size.height);
    plate.hidden = NO;
    UIView *slot = AWEMessageCompactSlotAncestor(source, navBar);
    UIView *direct = AWEMessageDirectChildUnderRoot(slot ?: source, navBar);
    if (direct && direct != plate) [navBar insertSubview:plate belowSubview:direct];
}

static void AWEMessageApplyNavigationGlass(UIView *navBar) {
    if (!navBar || !navBar.window || !AWEMessageRootControllerForView(navBar)) return;
    AWEMessageClearNavigationBackground(navBar);
    AWEMessageHideSecondLeftIcon(navBar);

    UIView *menu = AWEMessageBestVisualSourceInBand(navBar, 0.0, 92.0);
    UIView *search = nil;
    UIView *add = nil;
    for (UIView *view in AWEMessageAllSubviews(navBar, 260)) {
        CGRect r = [view convertRect:view.bounds toView:navBar];
        CGFloat midX = CGRectGetMidX(r);
        NSString *label = view.accessibilityLabel ?: @"";
        NSString *name = NSStringFromClass(view.class);
        if (!search && midX >= 315.0 && midX <= 395.0 &&
            ([label containsString:@"搜索"] || [name containsString:@"IESIMButton"])) search = view;
        if (!add && midX > 392.0 && midX <= CGRectGetWidth(navBar.bounds) + 4.0 &&
            ([label containsString:@"添加"] || [label containsString:@"发布"] ||
             [name containsString:@"Button"] || [view isKindOfClass:UIImageView.class])) add = view;
    }
    if (!search) search = AWEMessageBestVisualSourceInBand(navBar, 315.0, 395.0);
    if (!add) add = AWEMessageBestVisualSourceInBand(navBar, 392.0, CGRectGetWidth(navBar.bounds) + 4.0);
    UILabel *title = AWEMessageFindTitleLabel(navBar);

    AWEMessageLayoutGlassPlate(navBar, menu, 0, CGSizeMake(50.0, 50.0));
    AWEMessageLayoutGlassPlate(navBar, search, 1, CGSizeMake(50.0, 50.0));
    AWEMessageLayoutGlassPlate(navBar, add, 2, CGSizeMake(50.0, 50.0));
    AWEMessageLayoutGlassPlate(navBar, title, 3, CGSizeMake(96.0, 46.0));

    NSString *signature = [NSString stringWithFormat:@"%@|%@|%@|%@|%@|%@",
                           NSStringFromCGRect(navBar.frame),
                           menu ? NSStringFromClass(menu.class) : @"nil",
                           search ? NSStringFromClass(search.class) : @"nil",
                           add ? NSStringFromClass(add.class) : @"nil",
                           title ? NSStringFromCGRect([title convertRect:title.bounds toView:navBar]) : @"nil",
                           NSStringFromClass(navBar.class)];
    if (![signature isEqualToString:gAWEMessageLastProbeSignature]) {
        gAWEMessageLastProbeSignature = [signature copy];
        AWEMessageProbeLog(@"MESSAGE_NAV nav=%@ frame=%@ menu=%@ search=%@ add=%@ title=%@",
                           NSStringFromClass(navBar.class), NSStringFromCGRect(navBar.frame),
                           menu ? [NSString stringWithFormat:@"%@ %@", NSStringFromClass(menu.class), NSStringFromCGRect([menu convertRect:menu.bounds toView:navBar])] : @"nil",
                           search ? [NSString stringWithFormat:@"%@ %@", NSStringFromClass(search.class), NSStringFromCGRect([search convertRect:search.bounds toView:navBar])] : @"nil",
                           add ? [NSString stringWithFormat:@"%@ %@", NSStringFromClass(add.class), NSStringFromCGRect([add convertRect:add.bounds toView:navBar])] : @"nil",
                           title ? NSStringFromCGRect([title convertRect:title.bounds toView:navBar]) : @"nil");
    }
}

static UIScrollView *AWEMessageLargestVerticalScrollView(UIView *root) {
    UIScrollView *best = nil;
    CGFloat bestArea = 0.0;
    for (UIView *view in AWEMessageAllSubviews(root, 900)) {
        if (![view isKindOfClass:UIScrollView.class]) continue;
        UIScrollView *scroll = (UIScrollView *)view;
        CGRect r = [scroll convertRect:scroll.bounds toView:root];
        CGFloat w = CGRectGetWidth(r), h = CGRectGetHeight(r);
        if (w < CGRectGetWidth(root.bounds) * 0.82 || h < 300.0) continue;
        if (scroll.alwaysBounceHorizontal && !scroll.alwaysBounceVertical) continue;
        CGFloat area = w * h;
        if (area > bestArea) { bestArea = area; best = scroll; }
    }
    return best;
}

static id AWEMessageSafeValue(id object, NSString *key) {
    if (!object || key.length == 0) return nil;
    @try { return [object valueForKey:key]; }
    @catch (__unused NSException *exception) { return nil; }
}

static void AWEMessageApplyTopFadeGeometry(UIViewController *controller) {
    if (!AWEMessageControllerIsRoot(controller) || !controller.view.window) return;
    UIView *root = controller.view;
    UIScrollView *scroll = AWEMessageLargestVerticalScrollView(root);
    if (!scroll || !scroll.superview) return;

    UIView *nav = nil;
    for (UIView *view in AWEMessageAllSubviews(root, 500)) {
        if ([NSStringFromClass(view.class) isEqualToString:@"AWENavigationBar"]) {
            nav = view; break;
        }
    }
    CGFloat navHeight = nav ? CGRectGetMaxY([nav.superview convertRect:nav.frame toView:root]) : 106.0;
    CGRect frameInRoot = [scroll.superview convertRect:scroll.frame toView:root];

    if (!objc_getAssociatedObject(scroll, &kAWEMessageOriginalScrollFrameKey)) {
        objc_setAssociatedObject(scroll, &kAWEMessageOriginalScrollFrameKey,
                                 [NSValue valueWithCGRect:scroll.frame],
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(scroll, &kAWEMessageOriginalScrollInsetKey,
                                 [NSValue valueWithUIEdgeInsets:scroll.contentInset],
                                 OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    // XHS-style path: only extend a real large scroll view when it demonstrably
    // starts immediately below the navigation bar. If it is already full-screen,
    // do not invent geometry; transparent nav + native topEdgeEffect can work as-is.
    if (frameInRoot.origin.y >= 70.0 && frameInRoot.origin.y <= navHeight + 24.0 &&
        scroll.superview == root) {
        CGFloat delta = frameInRoot.origin.y;
        CGRect newFrame = scroll.frame;
        newFrame.origin.y -= delta;
        newFrame.size.height += delta;
        scroll.frame = newFrame;
        UIEdgeInsets inset = scroll.contentInset;
        if (inset.top < delta - 1.0) inset.top += delta;
        scroll.contentInset = inset;
        UIEdgeInsets indicator = scroll.scrollIndicatorInsets;
        if (indicator.top < delta - 1.0) indicator.top += delta;
        scroll.scrollIndicatorInsets = indicator;
    }
    scroll.clipsToBounds = NO;

    id topEdgeEffect = nil;
    if ([scroll respondsToSelector:NSSelectorFromString(@"topEdgeEffect")]) {
        topEdgeEffect = AWEMessageSafeValue(scroll, @"topEdgeEffect");
    }
    static NSString *lastScrollSignature = nil;
    NSString *signature = [NSString stringWithFormat:@"%@|%@|%@|%@",
                           NSStringFromClass(scroll.class), NSStringFromCGRect(scroll.frame),
                           NSStringFromUIEdgeInsets(scroll.contentInset),
                           topEdgeEffect ? NSStringFromClass([topEdgeEffect class]) : @"nil"];
    if (![signature isEqualToString:lastScrollSignature]) {
        lastScrollSignature = [signature copy];
        AWEMessageProbeLog(@"MESSAGE_SCROLL class=%@ frameRoot=%@ frame=%@ inset=%@ topEdge=%@ navH=%.1f super=%@",
                           NSStringFromClass(scroll.class), NSStringFromCGRect(frameInRoot),
                           NSStringFromCGRect(scroll.frame), NSStringFromUIEdgeInsets(scroll.contentInset),
                           topEdgeEffect ? NSStringFromClass([topEdgeEffect class]) : @"nil",
                           navHeight, NSStringFromClass(scroll.superview.class));
    }
}

%hook AWENavigationBar
- (void)layoutSubviews {
    %orig;
    AWEMessageApplyNavigationGlass((UIView *)self);
}
%end

%hook AWEIMMessageTabViewController
- (void)viewDidLayoutSubviews {
    %orig;
    AWEMessageApplyTopFadeGeometry((UIViewController *)self);
}
- (void)viewDidAppear:(BOOL)animated {
    %orig(animated);
    AWEMessageApplyTopFadeGeometry((UIViewController *)self);
    AWEMessageProbeLog(@"MESSAGE_ROOT_APPEAR vc=%@ frame=%@",
                       NSStringFromClass(((UIViewController *)self).class),
                       NSStringFromCGRect(((UIViewController *)self).view.frame));
}
%end

%hook AWEIMMessageTabRootViewController
- (void)viewDidLayoutSubviews {
    %orig;
    AWEMessageApplyTopFadeGeometry((UIViewController *)self);
}
%end

%ctor {
    @autoreleasepool {
        AWEMessageProbeLog(@"SESSION message-top-glass-v2 loaded os=%@",
                           NSProcessInfo.processInfo.operatingSystemVersionString);
    }
}
