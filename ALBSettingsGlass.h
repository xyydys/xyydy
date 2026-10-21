/* UIKit owns the floating glass panel and the existing settings navigation. */
#import "ALBReproCaptureSignal.h"
static char kALBSettingsBackdropKey, kALBSettingsFloatingKey, kALBSettingsScrollVisibilityKey;
static char kALBSettingsSectionFrostKey;
static char kALB30SettingsCaptureStateKey;

// TEST30 capture-only helpers: do not query private material properties or
// copy settings titles/content. Full scene trees belong to explicit captures.
static id ALB30SettingsCaptureColor(UIColor *color, UITraitCollection *traits) {
    if (!color) return NSNull.null;
    CGFloat r=0,g=0,b=0,a=0;
    UIColor *resolved=[color resolvedColorWithTraitCollection:traits];
    return [resolved getRed:&r green:&g blue:&b alpha:&a] ? @[@(r),@(g),@(b),@(a)] : @"nonRGB";
}
static NSDictionary *ALB30SettingsCaptureView(UIView *view) {
    if (!view) return @{};
    NSMutableDictionary *state=[@{@"object":[NSString stringWithFormat:@"%p",view],
        @"class":NSStringFromClass(view.class), @"frame":NSStringFromCGRect(view.frame),
        @"bounds":NSStringFromCGRect(view.bounds), @"center":NSStringFromCGPoint(view.center),
        @"transform":NSStringFromCGAffineTransform(view.transform), @"alpha":@(view.alpha),
        @"hidden":@(view.hidden), @"opaque":@(view.opaque), @"layerOpacity":@(view.layer.opacity),
        @"clipsToBounds":@(view.clipsToBounds), @"cornerRadius":@(view.layer.cornerRadius),
        @"backgroundRGBA":ALB30SettingsCaptureColor(view.backgroundColor,view.traitCollection),
        @"tintRGBA":ALB30SettingsCaptureColor(view.tintColor,view.traitCollection),
        @"layerBackgroundRGBA":view.layer.backgroundColor ?
            ALB30SettingsCaptureColor([UIColor colorWithCGColor:view.layer.backgroundColor],view.traitCollection) : NSNull.null,
        @"style":@(view.traitCollection.userInterfaceStyle),
        @"overrideStyle":@(view.overrideUserInterfaceStyle)} mutableCopy];
    if ([view isKindOfClass:UIVisualEffectView.class]) {
        UIVisualEffect *effect=((UIVisualEffectView *)view).effect;
        NSString *description=effect.description ?: @"";
        state[@"effectClass"]=effect ? NSStringFromClass(effect.class) : @"nil";
        state[@"effectDescription"]=[description substringToIndex:MIN(description.length,240)];
    }
    if (@available(iOS 26.0,*)) {
        state[@"effectiveCornerRadii"]=@[
            @([view effectiveRadiusForCorner:UIRectCornerTopLeft]),
            @([view effectiveRadiusForCorner:UIRectCornerTopRight]),
            @([view effectiveRadiusForCorner:UIRectCornerBottomLeft]),
            @([view effectiveRadiusForCorner:UIRectCornerBottomRight])];
    }
    return state;
}
static void ALB30SettingsRecord(NSString *phase, UIViewController *controller) {
    if (!ALBReproIsRecording() || !controller) return;
    UINavigationController *navigation=[controller isKindOfClass:UINavigationController.class] ?
        (UINavigationController *)controller : controller.navigationController;
    UIViewController *owner=navigation ?: controller;
    UIView *root=owner.viewIfLoaded;
    NSMutableArray *containers=[NSMutableArray array];
    UIView *ancestor=root;
    for (NSUInteger depth=0;ancestor && depth<8;depth++,ancestor=ancestor.superview) {
        NSMutableDictionary *node=[ALB30SettingsCaptureView(ancestor) mutableCopy];
        NSMutableArray *backings=[NSMutableArray array];
        NSUInteger inspected=0;
        for (UIView *child in ancestor.subviews) {
            if (inspected++>=16) break;
            if ([child isKindOfClass:UIVisualEffectView.class] ||
                [NSStringFromClass(child.class).lowercaseString containsString:@"background"])
                [backings addObject:ALB30SettingsCaptureView(child)];
        }
        node[@"backings"]=backings;
        [containers addObject:node];
    }
    NSMutableDictionary *payload=[@{@"controller":NSStringFromClass(controller.class),
        @"owner":NSStringFromClass(owner.class), @"containers":containers,
        @"presentationStyle":@(owner.modalPresentationStyle),
        @"beingPresented":@(owner.isBeingPresented), @"beingDismissed":@(owner.isBeingDismissed),
        @"style":@(owner.traitCollection.userInterfaceStyle),
        @"overrideStyle":@(owner.overrideUserInterfaceStyle),
        @"customFloating":@([objc_getAssociatedObject(navigation,&kALBSettingsFloatingKey) boolValue] ||
            [root.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.Settings.Floating"]),
        @"pluginBackdrop":ALB30SettingsCaptureView(objc_getAssociatedObject(root,&kALBSettingsBackdropKey))} mutableCopy];
    if (@available(iOS 15.0,*)) {
        UISheetPresentationController *sheet=owner.sheetPresentationController;
        if (sheet) {
            payload[@"sheetClass"]=NSStringFromClass(sheet.class);
            payload[@"selectedDetent"]=sheet.selectedDetentIdentifier ?: @"nil";
            payload[@"detentCount"]=@(sheet.detents.count);
            payload[@"prefersGrabberVisible"]=@(sheet.prefersGrabberVisible);
            payload[@"prefersScrollingExpands"]=@(sheet.prefersScrollingExpandsWhenScrolledToEdge);
            payload[@"preferredCornerRadius"]=@(sheet.preferredCornerRadius);
            payload[@"largestUndimmedDetent"]=sheet.largestUndimmedDetentIdentifier ?: @"nil";
            payload[@"presentationContainer"]=ALB30SettingsCaptureView(sheet.containerView);
            payload[@"presentedView"]=ALB30SettingsCaptureView(sheet.presentedView);
        }
    }
    ALBReproRecordEvent(phase,payload);
}
// TEST7: on a 393 pt phone, the 369 pt logical panel renders 302.58 pt wide,
// placing its right edge at 347.79 pt beside the native right action rail.
static const CGFloat kALBSettingsFloatingScale = 0.82;

/* This observer belongs only to our settings scroll. Hidden CALayers keep the
   stack's layout and accessibility order intact; changing UIView.hidden would
   collapse arranged rows and start another layout during a scroll. */
@interface ALBSettingsScrollVisibility : NSObject <UIScrollViewDelegate>
@property(nonatomic, weak) UIScrollView *scroll;
@property(nonatomic, weak) id<UIScrollViewDelegate> originalDelegate;
@property(nonatomic, copy) NSArray<UIView *> *sections;
- (void)updateVisibility;
@end
@implementation ALBSettingsScrollVisibility
- (void)updateVisibility {
    UIScrollView *scroll = self.scroll;
    if (!scroll.window || CGRectIsEmpty(scroll.bounds)) return;
    // One extra row on either edge keeps fast scrolling free of reveal flashes.
    CGRect viewport = CGRectInset(scroll.bounds, 0, -88.0);
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    for (UIView *section in self.sections) {
        if (!section.superview || CGRectIsEmpty(section.bounds)) continue;
        CGRect rect = [section convertRect:section.bounds toView:scroll];
        BOOL hidden = !CGRectIntersectsRect(viewport, rect);
        if (section.layer.hidden != hidden) section.layer.hidden = hidden;
    }
    [CATransaction commit];
}
- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
    [self updateVisibility];
    if ([self.originalDelegate respondsToSelector:_cmd])
        [self.originalDelegate scrollViewDidScroll:scrollView];
}
- (BOOL)respondsToSelector:(SEL)selector {
    return [super respondsToSelector:selector] || [self.originalDelegate respondsToSelector:selector];
}
- (id)forwardingTargetForSelector:(SEL)selector {
    return [self.originalDelegate respondsToSelector:selector] ? self.originalDelegate : [super forwardingTargetForSelector:selector];
}
@end

static void ALBSettingsObserveScroll(UIScrollView *scroll, NSArray<UIView *> *sections) {
    if (!scroll) return;
    ALBSettingsScrollVisibility *observer = objc_getAssociatedObject(scroll, &kALBSettingsScrollVisibilityKey);
    if (!observer) {
        observer = [ALBSettingsScrollVisibility new]; observer.scroll = scroll;
        objc_setAssociatedObject(scroll, &kALBSettingsScrollVisibilityKey, observer, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    observer.sections = sections;
    if (scroll.delegate != observer) { observer.originalDelegate = scroll.delegate; scroll.delegate = observer; }
    [observer updateVisibility];
}
static void ALBSettingsUpdateScrollVisibility(UIScrollView *scroll) {
    [(ALBSettingsScrollVisibility *)objc_getAssociatedObject(scroll, &kALBSettingsScrollVisibilityKey) updateVisibility];
}

static UIColor *ALBSettingsSectionColor(void) {
    return [UIColor colorWithDynamicProvider:^UIColor *(UITraitCollection *traits) {
        return traits.userInterfaceStyle == UIUserInterfaceStyleDark
            ? [UIColor colorWithWhite:1 alpha:0.05] : [UIColor colorWithWhite:1 alpha:0.10];
    }];
}

/* Keep the outer clear glass, but give text a genuinely frosted local backing.
   A cached translucent material avoids piling effects up on every appearance pass. */
static void ALBSettingsInstallSectionFrost(UIView *section) {
    if (!section) return;
    UIVisualEffectView *frost = objc_getAssociatedObject(section, &kALBSettingsSectionFrostKey);
    if (!frost) {
        frost = [[UIVisualEffectView alloc] initWithEffect:
            [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial]];
        frost.userInteractionEnabled = NO;
        frost.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        frost.accessibilityIdentifier = @"AwemeLiquidBar.Settings.SectionFrost";
        [section insertSubview:frost atIndex:0];
        objc_setAssociatedObject(section, &kALBSettingsSectionFrostKey, frost, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (ALBReproIsRecording()) ALBReproRecordEvent(@"settings.sectionFrost.created",@{
            @"section":ALB30SettingsCaptureView(section), @"frost":ALB30SettingsCaptureView(frost)});
    }
    section.backgroundColor = UIColor.clearColor;
    section.opaque = NO;
    if (!CGRectEqualToRect(frost.frame, section.bounds)) frost.frame = section.bounds;
    if (section.subviews.firstObject != frost) [section sendSubviewToBack:frost];
}

static void ALBSettingsInstallCellFrost(UITableViewCell *cell) {
    if (!cell) return;
    cell.backgroundColor = UIColor.clearColor;
    cell.opaque = NO;
    if (![cell.backgroundView isKindOfClass:UIVisualEffectView.class]) {
        UIVisualEffectView *frost = [[UIVisualEffectView alloc] initWithEffect:
            [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial]];
        frost.userInteractionEnabled = NO;
        cell.backgroundView = frost;
        if (ALBReproIsRecording()) ALBReproRecordEvent(@"settings.cellFrost.created",@{
            @"cell":ALB30SettingsCaptureView(cell), @"frost":ALB30SettingsCaptureView(frost)});
    }
}

static void ALBInstallSettingsGlass(UIViewController *controller) {
    UINavigationController *navigation = controller.navigationController;
    UIView *surface = navigation.view ?: controller.view;
    BOOL floating = [objc_getAssociatedObject(navigation, &kALBSettingsFloatingKey) boolValue];
    UIVisualEffectView *glass = objc_getAssociatedObject(surface, &kALBSettingsBackdropKey);
    if (floating) {
        // The outer panel is the sole material surface. A second glass here
        // would sample the first glass instead of the playing video.
        [glass removeFromSuperview];
        objc_setAssociatedObject(surface, &kALBSettingsBackdropKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    } else {
        if (!glass) {
            glass = [[UIVisualEffectView alloc] initWithEffect:ALBClearGlassEffect(NO)];
            glass.accessibilityIdentifier = @"AwemeLiquidBar.Settings.Glass";
            glass.userInteractionEnabled = NO;
            glass.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            // The glass must render its rim using the sheet's actual shape;
            // clipping a square effect in an ancestor only cuts the rim off.
            if (@available(iOS 26.0,*)) {
                glass.cornerConfiguration = [UICornerConfiguration configurationWithRadius:
                    [UICornerRadius containerConcentricRadius]];
            }
            glass.backgroundColor = UIColor.clearColor;
            glass.contentView.backgroundColor = UIColor.clearColor;
            glass.opaque = NO;
            [surface insertSubview:glass atIndex:0];
            objc_setAssociatedObject(surface, &kALBSettingsBackdropKey, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        glass.frame = surface.bounds;
        ALBRefreshClearGlass(glass);
        [surface sendSubviewToBack:glass];
    }
    surface.backgroundColor = UIColor.clearColor;
    surface.opaque = NO;
    controller.view.backgroundColor = UIColor.clearColor;
    controller.view.opaque = NO;
    controller.edgesForExtendedLayout = UIRectEdgeAll;
    // Douyin often pins its window to Light. The actual screen preference is
    // independent; floating content inherits the outer panel's system traits.
    UIUserInterfaceStyle style = floating ? UIUserInterfaceStyleUnspecified : (ALBMaterialEnabled() ? ALBMaterialStyle(surface, NO) : AWESystemInterfaceStyle());
    if (controller.overrideUserInterfaceStyle != style) controller.overrideUserInterfaceStyle = style;
    if (!floating && navigation.overrideUserInterfaceStyle != style) navigation.overrideUserInterfaceStyle = style;
    UINavigationBar *bar = navigation.navigationBar;
    if (!objc_getAssociatedObject(bar, &kALBSettingsBackdropKey)) {
        UINavigationBarAppearance *appearance = [UINavigationBarAppearance new];
        [appearance configureWithTransparentBackground];
        appearance.backgroundColor = UIColor.clearColor;
        appearance.shadowColor = UIColor.clearColor;
        bar.standardAppearance = appearance;
        bar.scrollEdgeAppearance = appearance;
        bar.compactAppearance = appearance;
        if (@available(iOS 15.0, *)) bar.compactScrollEdgeAppearance = appearance;
        bar.backgroundColor = UIColor.clearColor;
        bar.opaque = NO;
        bar.translucent = YES;
        bar.tintColor = UIColor.labelColor;
        objc_setAssociatedObject(bar, &kALBSettingsBackdropKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (ALBReproIsRecording()) {
        NSString *state=[NSString stringWithFormat:@"%p/%@/%ld/%d",glass,
            NSStringFromCGRect(surface.bounds),(long)surface.traitCollection.userInterfaceStyle,floating];
        if (![state isEqualToString:objc_getAssociatedObject(surface,&kALB30SettingsCaptureStateKey)]) {
            objc_setAssociatedObject(surface,&kALB30SettingsCaptureStateKey,state,OBJC_ASSOCIATION_COPY_NONATOMIC);
            ALB30SettingsRecord(@"settings.glass.install",controller);
            ALBReproScheduleSnapshot(@"settings.glass.installed.material",0.30);
        }
    }
}

@interface ALBSettingsFloatingController : UIViewController
@property(nonatomic, strong) UINavigationController *settingsNavigation;
@property(nonatomic, strong) UIVisualEffectView *glassPanel;
- (void)refreshSystemAppearance;
- (instancetype)initWithNavigation:(UINavigationController *)navigation;
@end

@implementation ALBSettingsFloatingController
- (instancetype)initWithNavigation:(UINavigationController *)navigation {
    if ((self = [super init])) {
        _settingsNavigation = navigation;
        objc_setAssociatedObject(navigation, &kALBSettingsFloatingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        self.modalPresentationStyle = UIModalPresentationOverFullScreen;
        self.modalTransitionStyle = UIModalTransitionStyleCrossDissolve;
    }
    return self;
}
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.clearColor;
    self.view.opaque = NO;
    self.view.accessibilityViewIsModal = YES;
    self.view.accessibilityIdentifier = @"AwemeLiquidBar.Settings.Floating";

    UIButton *dismiss = [UIButton buttonWithType:UIButtonTypeCustom];
    dismiss.frame = self.view.bounds;
    dismiss.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    dismiss.backgroundColor = UIColor.clearColor;
    dismiss.accessibilityLabel = @"关闭设置";
    [dismiss addTarget:self action:@selector(closePanel) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:dismiss];

    UIVisualEffectView *panel = [[UIVisualEffectView alloc] initWithEffect:ALBClearGlassEffect(NO)];
    self.glassPanel = panel;
    panel.translatesAutoresizingMaskIntoConstraints = YES;
    panel.layer.cornerRadius = 36;
    panel.layer.cornerCurve = kCACornerCurveContinuous;
    panel.clipsToBounds = YES;
    panel.backgroundColor = UIColor.clearColor;
    panel.contentView.backgroundColor = UIColor.clearColor;
    panel.opaque = NO;
    panel.accessibilityIdentifier = @"AwemeLiquidBar.Settings.FloatingGlass";
    [self.view addSubview:panel];
    // Scale the whole panel, including navigation, typography and controls.
    // TEST6's 2/3 scale was too small; 0.82 reaches the right action rail on
    // the supplied phone layout without spreading the content to full width.
    // Keep the logical child size unscaled so text does not reflow into a tiny
    // viewport. UIKit converts hit tests through this single outer transform.
    panel.transform = CGAffineTransformMakeScale(kALBSettingsFloatingScale, kALBSettingsFloatingScale);

    UINavigationController *navigation = self.settingsNavigation;
    [self addChildViewController:navigation];
    navigation.view.translatesAutoresizingMaskIntoConstraints = NO;
    navigation.view.backgroundColor = UIColor.clearColor;
    navigation.view.opaque = NO;
    [panel.contentView addSubview:navigation.view];
    [NSLayoutConstraint activateConstraints:@[
        [navigation.view.leadingAnchor constraintEqualToAnchor:panel.contentView.leadingAnchor],
        [navigation.view.trailingAnchor constraintEqualToAnchor:panel.contentView.trailingAnchor],
        [navigation.view.topAnchor constraintEqualToAnchor:panel.contentView.topAnchor],
        [navigation.view.bottomAnchor constraintEqualToAnchor:panel.contentView.bottomAnchor]
    ]];
    [navigation didMoveToParentViewController:self];
    ALBInstallSettingsGlass(navigation.topViewController);
    [self refreshSystemAppearance];
    // Follow traits once per appearance change. Never screenshot the whole
    // live settings scroll or toggle the entire navigation tree on a timer.
    navigation.overrideUserInterfaceStyle = ALBMaterialEnabled() ? ALBMaterialStyle(navigation.view, NO) : AWESystemInterfaceStyle();
    ALB30SettingsRecord(@"settings.custom.viewDidLoad",self);
}
- (void)refreshSystemAppearance {
    UIUserInterfaceStyle style = ALBMaterialEnabled() ? ALBMaterialStyle(self.view, NO) : AWESystemInterfaceStyle();
    if (self.overrideUserInterfaceStyle != style) self.overrideUserInterfaceStyle = style;
    if (self.glassPanel.overrideUserInterfaceStyle != style) self.glassPanel.overrideUserInterfaceStyle = style;
    ALBRefreshClearGlass(self.glassPanel);
    if(self.settingsNavigation.overrideUserInterfaceStyle!=style)self.settingsNavigation.overrideUserInterfaceStyle=style;
    ALB30SettingsRecord(@"settings.custom.appearance",self);
}
- (void)traitCollectionDidChange:(UITraitCollection *)previous {
    [super traitCollectionDidChange:previous];
    if (!previous || [self.traitCollection hasDifferentColorAppearanceComparedToTraitCollection:previous])
        [self refreshSystemAppearance];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    CGRect safe = self.view.safeAreaLayoutGuide.layoutFrame;
    if (CGRectIsEmpty(safe)) return;
    const CGFloat scale = kALBSettingsFloatingScale;
    CGFloat top = CGRectGetMinY(safe) + 12.0;
    CGFloat bottom = CGRectGetMaxY(safe) - 12.0;
    CGRect keyboard = self.view.keyboardLayoutGuide.layoutFrame;
    if (!CGRectIsEmpty(keyboard) && CGRectGetMinY(keyboard) > top && CGRectGetMinY(keyboard) < bottom)
        bottom = CGRectGetMinY(keyboard) - 12.0;
    CGFloat width = MIN(640.0, MAX(1.0, CGRectGetWidth(safe) - 24.0));
    CGFloat availableHeight = MAX(1.0, bottom - top);
    CGFloat height = MIN(MAX(1.0, CGRectGetHeight(safe) - 24.0), availableHeight / scale);
    CGRect bounds = CGRectMake(0, 0, width, height);
    CGPoint center = CGPointMake(CGRectGetMidX(safe), MIN(MAX(CGRectGetMidY(safe), top+height*scale/2), top+availableHeight-height*scale/2));
    // Do not rewrite navigation constraints/effects on each child scroll layout.
    if (!CGRectEqualToRect(self.glassPanel.bounds, bounds)) self.glassPanel.bounds = bounds;
    if (!CGPointEqualToPoint(self.glassPanel.center, center)) self.glassPanel.center = center;
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    gAWESettingsVisible = YES;
    ALB30SettingsRecord(@"settings.custom.willAppear",self);
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    ALB30SettingsRecord(@"settings.custom.didAppear",self);
    ALBReproScheduleSnapshot(@"settings.custom.appeared.material",0.30);
}
- (void)viewWillDisappear:(BOOL)animated {
    ALB30SettingsRecord(@"settings.custom.willDisappear",self);
    [super viewWillDisappear:animated];
}
- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    ALB30SettingsRecord(@"settings.custom.didDisappear",self);
    if (self.isBeingDismissed || !self.presentingViewController) {
        gAWESettingsVisible = NO;
        ALB5ScheduleIdleCollapse();
    }
}
- (void)closePanel {
    ALB30SettingsRecord(@"settings.custom.closeRequested",self);
    [self dismissViewControllerAnimated:YES completion:nil];
}
- (BOOL)accessibilityPerformEscape { [self closePanel]; return YES; }
@end

// This is a real system sheet (the 1.0.56 presentation), not a scaled panel
// with a custom pan gesture. UIKit owns detents, keyboard, corners and motion.
@interface ALBSettingsNativeNavigationController : UINavigationController <UISheetPresentationControllerDelegate>
@property(nonatomic) CFTimeInterval albReproLastLayoutAt;
@property(nonatomic,copy) NSString *albReproLayoutState;
@property(nonatomic,copy) NSString *albSettledMaterialState;
- (void)refreshSettledSheetGlass;
@end
@implementation ALBSettingsNativeNavigationController
- (void)refreshSettledSheetGlass {
    if (@available(iOS 15.0,*)) {
        if (!self.viewIfLoaded.window || self.isBeingDismissed ||
            self.modalPresentationStyle!=UIModalPresentationPageSheet) return;
        UISheetPresentationController *sheet=self.sheetPresentationController;
        NSString *detent=sheet.selectedDetentIdentifier;
        if (!detent || CGRectIsEmpty(self.view.bounds)) return;
        NSString *state=[NSString stringWithFormat:@"%@/%@",detent,NSStringFromCGRect(self.view.bounds)];
        if ([self.albSettledMaterialState isEqualToString:state]) return;
        // Initial construction happens at full-window size. Re-resolve Clear
        // once when the native medium sheet has settled, never during a drag
        // or on every layout. The already-correct large material is retained.
        if (![detent isEqualToString:UISheetPresentationControllerDetentIdentifierMedium]) {
            self.albSettledMaterialState=state;
            return;
        }
        UIVisualEffectView *glass=objc_getAssociatedObject(self.view,&kALBSettingsBackdropKey);
        if (!glass) return;
        self.albSettledMaterialState=state;
        [UIView performWithoutAnimation:^{
            glass.frame=self.view.bounds;
            glass.effect=ALBClearGlassEffect(NO);
        }];
        ALB30SettingsRecord(@"settings.native.medium.materialRefresh",self);
    }
}
- (void)sheetPresentationControllerDidChangeSelectedDetentIdentifier:(UISheetPresentationController *)sheet API_AVAILABLE(ios(15.0)) {
    if (sheet!=self.sheetPresentationController) return;
    __weak typeof(self) weakSelf=self;
    void (^refresh)(void)=^{
        ALBSettingsNativeNavigationController *owner=weakSelf;
        if (!owner.viewIfLoaded.window) return;
        [owner.viewIfLoaded layoutIfNeeded];
        [owner refreshSettledSheetGlass];
        ALBReproScheduleSnapshot(@"settings.native.detentChanged.material",0.30);
    };
    id<UIViewControllerTransitionCoordinator> coordinator=self.transitionCoordinator;
    if (coordinator && [coordinator animateAlongsideTransition:nil completion:^(id<UIViewControllerTransitionCoordinatorContext> context) {
        (void)context;
        refresh();
    }]) return;
    // UIKit does not always expose a coordinator for a detent change. Use one
    // deferred layout pass; no polling timer and no per-frame effect rebuild.
    dispatch_async(dispatch_get_main_queue(),refresh);
}
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    ALB30SettingsRecord(@"settings.native.willAppear",self);
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self refreshSettledSheetGlass];
    ALB30SettingsRecord(@"settings.native.didAppear",self);
    ALBReproScheduleSnapshot(@"settings.native.appeared.material",0.30);
}
- (void)viewWillDisappear:(BOOL)animated {
    ALB30SettingsRecord(@"settings.native.willDisappear",self);
    [super viewWillDisappear:animated];
}
- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    ALB30SettingsRecord(@"settings.native.didDisappear",self);
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    BOOL captureLayout=NO;
    if (ALBReproIsRecording() && CACurrentMediaTime()-self.albReproLastLayoutAt>=0.20) {
        NSString *detent=@"nil";
        if (@available(iOS 15.0,*)) detent=self.sheetPresentationController.selectedDetentIdentifier ?: @"nil";
        NSString *state=[NSString stringWithFormat:@"%@/%@/%@/%ld",NSStringFromCGRect(self.view.frame),
            NSStringFromCGRect(self.view.bounds),detent,(long)self.traitCollection.userInterfaceStyle];
        if (![self.albReproLayoutState isEqualToString:state]) {
            self.albReproLastLayoutAt=CACurrentMediaTime();
            self.albReproLayoutState=state;
            captureLayout=YES;
            ALB30SettingsRecord(@"settings.native.layout.beforeWrapperClear",self);
        }
    }
    if (self.modalPresentationStyle!=UIModalPresentationPageSheet ||
        !self.presentationController.containerView) {
        if (captureLayout) ALB30SettingsRecord(@"settings.native.layout.noSheetContainer",self);
        return;
    }
    UIView *container=self.presentationController.containerView;
    UIView *child=self.view;
    // Clear only the settings sheet's own wrappers, below the transition
    // container. Never walk the presenting feed or its dimming view. Keep
    // native clipping/corners, and the navigation bar's top-edge treatment.
    for (NSUInteger depth=0;child.superview && child.superview!=container && depth<6;depth++) {
        UIView *wrapper=child.superview;
        wrapper.backgroundColor=UIColor.clearColor;
        wrapper.opaque=NO;
        for (UIView *backing in wrapper.subviews) {
            if (backing==child || [backing isKindOfClass:UIControl.class]) continue;
            CGRect rect=[backing convertRect:backing.bounds toView:wrapper];
            BOOL full=rect.size.width>=wrapper.bounds.size.width*.85 &&
                      rect.size.height>=wrapper.bounds.size.height*.85;
            NSString *name=NSStringFromClass(backing.class).lowercaseString;
            if (!full || (![backing isKindOfClass:UIVisualEffectView.class] &&
                ![name containsString:@"background"])) continue;
            backing.backgroundColor=UIColor.clearColor;
            backing.layer.backgroundColor=UIColor.clearColor.CGColor;
            backing.opaque=NO;
            if ([backing isKindOfClass:UIVisualEffectView.class] && ((UIVisualEffectView *)backing).effect)
                ((UIVisualEffectView *)backing).effect=nil;
        }
        child=wrapper;
    }
    if (captureLayout) {
        ALB30SettingsRecord(@"settings.native.layout.afterWrapperClear",self);
        ALBReproScheduleSnapshot(@"settings.native.detentOrGeometry.material",0.30);
    }
}
@end

static UIViewController *ALBSettingsPresentation(UINavigationController *navigation) {
    if (!ALB5Bool(@"settings.halfSheet", YES)) {
        ALB30SettingsRecord(@"settings.presentation.customSelected",navigation);
        return [[ALBSettingsFloatingController alloc] initWithNavigation:navigation];
    }
    navigation.modalPresentationStyle=UIModalPresentationPageSheet;
    navigation.overrideUserInterfaceStyle=ALBMaterialEnabled() ? ALBMaterialStyle(navigation.view, NO) : AWESystemInterfaceStyle();
    if (@available(iOS 15.0,*)) {
        UISheetPresentationController *sheet=navigation.sheetPresentationController;
        sheet.detents=@[UISheetPresentationControllerDetent.mediumDetent,
                        UISheetPresentationControllerDetent.largeDetent];
        sheet.selectedDetentIdentifier=UISheetPresentationControllerDetentIdentifierMedium;
        sheet.prefersGrabberVisible=YES;
        sheet.prefersScrollingExpandsWhenScrolledToEdge=NO;
        if ([navigation isKindOfClass:ALBSettingsNativeNavigationController.class])
            sheet.delegate=(ALBSettingsNativeNavigationController *)navigation;
    }
    ALBInstallSettingsGlass(navigation.topViewController);
    ALB30SettingsRecord(@"settings.presentation.nativeSelected",navigation);
    return navigation;
}
