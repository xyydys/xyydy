/* Menu touches cannot also be interpreted as the native avatar/profile tap. */
@interface ALBAvatarMenuGestureGate : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic, weak) id<UIGestureRecognizerDelegate> original;
@property(nonatomic, strong) NSHashTable<UIButton *> *menuButtons;
@end
@implementation ALBAvatarMenuGestureGate
- (BOOL)respondsToSelector:(SEL)selector {
    return [super respondsToSelector:selector] || [self.original respondsToSelector:selector];
}
- (id)forwardingTargetForSelector:(SEL)selector {
    return [self.original respondsToSelector:selector] ? self.original : [super forwardingTargetForSelector:selector];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    for (UIButton *button in self.menuButtons.allObjects)
        if (button.window && !button.hidden && button.userInteractionEnabled && button.menu &&
            (touch.view == button || [touch.view isDescendantOfView:button] ||
             ([button.superview.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.CleanScreen.MenuRegion"] &&
              touch.view == button.superview))) return NO;
    return [self.original respondsToSelector:_cmd]
        ? [self.original gestureRecognizer:gesture shouldReceiveTouch:touch] : YES;
}
@end
static void ALBProtectAvatarMenuTouch(UIButton *button, UIView *stack) {
    static char key;
    if (![button isDescendantOfView:stack]) return;
    // The clean-screen region owns an explicit tap recognizer. Protect it from
    // feed-level taps, without putting our own recognizer behind this gate.
    BOOL independentRegion=[button.superview.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.CleanScreen.MenuRegion"];
    UIView *firstAncestor=independentRegion?button.superview.superview:button.superview;
    for (UIView *v = firstAncestor; v && v != button.window; v = v.superview) {
        for (UIGestureRecognizer *gesture in v.gestureRecognizers) {
            NSString *name = NSStringFromClass(gesture.class).lowercaseString;
            if (![gesture isKindOfClass:UITapGestureRecognizer.class] &&
                ![name containsString:@"tap"] && ![name containsString:@"click"]) continue;
            ALBAvatarMenuGestureGate *gate = objc_getAssociatedObject(gesture, &key);
            if (!gate) {
                gate = [ALBAvatarMenuGestureGate new];
                gate.menuButtons = [NSHashTable weakObjectsHashTable];
                objc_setAssociatedObject(gesture, &key, gate, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            if (gesture.delegate != gate) gate.original = gesture.delegate;
            [gate.menuButtons addObject:button];
            gesture.delegate = gate;
        }
    }
}

// A separate controller-owned region retains only the original avatar target.
// A bare window sibling receives touches but has no owning controller from
// which UIButton can present its menu. The hidden feed avatar is not reused.
static __weak UIWindow *gALB8CleanAnchorWindow;
static CGRect gALB8CleanAnchorFrame;
static void ALB30CleanPlacementLog(NSString *reason, UIView *stack, CGRect frame) {
    if (!ALBReproIsRecording()) return;
    static CFTimeInterval last;
    CFTimeInterval now=CACurrentMediaTime();
    if (now-last<0.5) return;
    last=now;
    ALBReproRecordEvent(@"clean.menu.placement", @{@"reason":reason,
        @"stack":ALB30ReproView(stack), @"frame":NSStringFromCGRect(frame),
        @"rememberedFrame":NSStringFromCGRect(gALB8CleanAnchorFrame),
        @"settingsVisible":@(gAWESettingsVisible), @"cleanEnabled":@(gAWEStableCleanScreenEnabled)});
}
static void ALB8RememberCleanMenuAnchor(UIView *stack,UIButton *button) {
    UIWindow *window=stack.window;
    CGRect frame=button.window==window?[button convertRect:button.bounds toView:window]:CGRectZero;
    if(window && !CGRectIsEmpty(frame) && CGRectIntersectsRect(frame,window.bounds)) {
        gALB8CleanAnchorWindow=window; gALB8CleanAnchorFrame=frame;
    }
    if (ALBReproIsRecording()) ALBReproRecordEvent(@"clean.menu.rememberAnchor", @{
        @"source":ALB30ReproView(button), @"stack":ALB30ReproView(stack),
        @"candidate":NSStringFromCGRect(frame), @"remembered":NSStringFromCGRect(gALB8CleanAnchorFrame)});
}
static void ALBRefreshCleanMenuAppearance(UIView *host, UIButton *button) {
    // Douyin may pin its window to light/dark. Resolve again for each menu
    // activation so this independent menu follows the current system theme.
    UIUserInterfaceStyle style=ALBMaterialEnabled() ? ALBMaterialStyle(button, YES) : AWESystemInterfaceStyle();
    if (button.overrideUserInterfaceStyle!=style) button.overrideUserInterfaceStyle=style;
    UIResponder *responder=host.nextResponder;
    if ([responder isKindOfClass:UIViewController.class]) {
        UIViewController *controller=(UIViewController *)responder;
        if (controller.overrideUserInterfaceStyle!=style) controller.overrideUserInterfaceStyle=style;
    }
}
@interface ALBCleanMenuHost : UIView
@property(nonatomic,weak) UIView *stack;
@property(nonatomic,strong) UIButton *button;
@property(nonatomic,strong) UITapGestureRecognizer *activationTap;
@property(nonatomic) BOOL activationPending;
@property(nonatomic) CGRect hitRegion;
@end
@implementation ALBCleanMenuHost
- (void)activateMenu:(UITapGestureRecognizer *)recognizer {
    if (recognizer.state != UIGestureRecognizerStateEnded || self.activationPending ||
        !gAWEStableCleanScreenEnabled || gAWESettingsVisible || !self.window || !self.button.menu) return;
    self.activationPending=YES;
    __weak ALBCleanMenuHost *weakSelf=self;
    // Finish the real tap first. Do not synthesize touches or send TouchUpInside
    // to a menu-only button, which does not activate UIButton's primary menu.
    dispatch_async(dispatch_get_main_queue(), ^{
        ALBCleanMenuHost *host=weakSelf;
        if (!host) return;
        host.activationPending=NO;
        if (!gAWEStableCleanScreenEnabled || gAWESettingsVisible || !host.window || !host.button.menu) return;
        if (@available(iOS 17.4, *)) {
            // This button has no artwork. UIKit may have hidden its source
            // during the previous menu transition; that cannot disable exit.
            host.button.hidden=NO;
            host.button.alpha=1;
            host.button.layer.opacity=1;
            ALBRefreshCleanMenuAppearance(host,host.button);
            if (ALBReproIsRecording()) ALBReproRecordEvent(@"clean.menu.primaryAction", @{
                @"host":ALB30ReproView(host), @"button":ALB30ReproView(host.button),
                @"controllerOwned":@([host.nextResponder isKindOfClass:UIViewController.class])});
            [host.button performPrimaryAction];
        }
    });
}
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    if(!gAWEStableCleanScreenEnabled || gAWESettingsVisible ||
        !self.stack.window || self.stack.window!=self.window || self.hidden ||
        !CGRectContainsPoint(self.hitRegion,point)) {
        if (ALBReproIsRecording() && CGRectContainsPoint(self.hitRegion,point))
            ALBReproRecordEvent(@"clean.menu.hitRejected.state", @{
                @"host":ALB30ReproView(self), @"stack":ALB30ReproView(self.stack),
                @"cleanEnabled":@(gAWEStableCleanScreenEnabled), @"settingsVisible":@(gAWESettingsVisible)});
        return nil;
    }
    // Consult the existing small weak controller registry only after a hit
    // reaches this tiny region. A comment/share/profile sheet owns its pixels;
    // clean-screen's hidden bottom bar is deliberately not a route predicate.
    for(UIViewController *controller in AWESensitiveVisibleControllers()) {
        UIView *modal=controller.viewIfLoaded;
        if(modal.window!=self.window || !AWEViewActuallyVisibleInWindow(modal))continue;
        if([self.stack isDescendantOfView:modal])continue; // The feed's own page is not an overlay.
        CGPoint local=[self convertPoint:point toView:modal];
        if(CGRectContainsPoint(modal.bounds,local)) {
            if (ALBReproIsRecording()) ALBReproRecordEvent(@"clean.menu.hitRejected.sensitiveController", @{
                @"controller":NSStringFromClass(controller.class), @"view":ALB30ReproView(modal)});
            return nil;
        }
    }
    NSUInteger depth=0;
    for(UIResponder *r=self.stack;r && depth++<32;r=r.nextResponder) {
        if(![r isKindOfClass:UIViewController.class])continue;
        UIViewController *controller=(UIViewController *)r;
        UIViewController *presented=controller.presentedViewController;
        if(presented && !presented.isBeingDismissed && presented.viewIfLoaded.window) {
            if (ALBReproIsRecording()) ALBReproRecordEvent(@"clean.menu.hitRejected.presentedController", @{
                @"controller":NSStringFromClass(presented.class)});
            return nil;
        }
        if([controller isKindOfClass:UINavigationController.class]) {
            UIView *visible=((UINavigationController *)controller).visibleViewController.viewIfLoaded;
            if(visible.window && ![self.stack isDescendantOfView:visible]) {
                if (ALBReproIsRecording()) ALBReproRecordEvent(@"clean.menu.hitRejected.navigation", @{
                    @"visible":ALB30ReproView(visible), @"stack":ALB30ReproView(self.stack)});
                return nil;
            }
        }
    }
    // Native ancestors are intentionally hidden during clean screen. Their
    // alpha/visibility must not disable this independent exit/menu target.
    CGPoint p=[self convertPoint:point toView:self.button];
    UIView *hit=self.activationTap.enabled?self:[self.button hitTest:p withEvent:event];
    // On modern systems the independent region owns the tap and explicitly
    // activates the same native UIButton menu. iOS 15–17.3 retain native button
    // tracking; no unsupported primary-action selector is sent there.
    if (ALBReproIsRecording()) ALBReproRecordEvent(@"clean.menu.hit", @{
        @"result":ALB30ReproView(hit), @"button":ALB30ReproView(self.button),
        @"region":NSStringFromCGRect(self.hitRegion), @"hasMenu":@(self.button.menu!=nil)});
    return hit;
}
@end
@interface ALBCleanMenuController : UIViewController
@end
@implementation ALBCleanMenuController
@end
static ALBCleanMenuHost *gALBCleanMenuHost;
static ALBCleanMenuController *gALBCleanMenuController;
static void ALBRemoveCleanMenuHost(void) {
    if (ALBReproIsRecording()) ALBReproRecordEvent(@"clean.menu.remove", @{
        @"host":ALB30ReproView(gALBCleanMenuHost)});
    [gALBCleanMenuController willMoveToParentViewController:nil];
    [gALBCleanMenuHost removeFromSuperview];
    [gALBCleanMenuController removeFromParentViewController];
    gALBCleanMenuController=nil; gALBCleanMenuHost=nil;
    gALB8CleanAnchorWindow=nil; gALB8CleanAnchorFrame=CGRectZero;
}
static BOOL ALBPlaceCleanMenu(UIButton *sourceButton,UIView *stack,CGRect avatarFrame) {
    UIWindow *window=stack.window;
    UIViewController *owner=window.rootViewController;
    if(!window || !owner || owner.view.window!=window || !sourceButton || !sourceButton.menu || gAWESettingsVisible) {
        ALB30CleanPlacementLog(@"missing-window-button-menu-or-settings",stack,avatarFrame);
        return NO;
    }
    CGRect candidate=[stack convertRect:avatarFrame toView:window];
    CGRect viewport=UIEdgeInsetsInsetRect(window.bounds,window.safeAreaInsets);
    BOOL remembered=gALB8CleanAnchorWindow==window && !CGRectIsEmpty(gALB8CleanAnchorFrame);
    // The CURRENT stack may lose its visible geometry as clean-screen hides
    // native ancestors. Its pre-hide anchor is authoritative. Offscreen reuse
    // callbacks must still not take ownership of the invisible menu.
    BOOL current=stack==gAWEStableCleanScreenStack;
    if(!current && !CGRectContainsPoint(viewport,CGPointMake(CGRectGetMidX(candidate),CGRectGetMidY(candidate)))) {
        ALB30CleanPlacementLog(@"noncurrent-offscreen-stack",stack,candidate);
        return NO;
    }
    CGRect frame=remembered?gALB8CleanAnchorFrame:candidate;
    if(!CGRectContainsPoint(viewport,CGPointMake(CGRectGetMidX(frame),CGRectGetMidY(frame)))) {
        ALB30CleanPlacementLog(@"anchor-outside-viewport",stack,frame);
        return NO;
    }
    if(!gALBCleanMenuHost || gALBCleanMenuHost.window!=window || gALBCleanMenuController.parentViewController!=owner) {
        [gALBCleanMenuController willMoveToParentViewController:nil];
        [gALBCleanMenuHost removeFromSuperview];
        [gALBCleanMenuController removeFromParentViewController];
        gALBCleanMenuController=[ALBCleanMenuController new];
        gALBCleanMenuHost=[[ALBCleanMenuHost alloc] initWithFrame:owner.view.bounds];
        gALBCleanMenuController.view=gALBCleanMenuHost;
        [owner addChildViewController:gALBCleanMenuController];
        gALBCleanMenuHost.autoresizingMask=UIViewAutoresizingFlexibleWidth|UIViewAutoresizingFlexibleHeight;
        gALBCleanMenuHost.backgroundColor=UIColor.clearColor;
        gALBCleanMenuHost.opaque=NO;
        gALBCleanMenuHost.accessibilityIdentifier=@"AwemeLiquidBar.CleanScreen.MenuRegion";
        UIButton *button=[UIButton buttonWithType:UIButtonTypeCustom];
        button.backgroundColor=UIColor.clearColor; button.showsMenuAsPrimaryAction=YES;
        button.opaque=NO;
        button.accessibilityLabel=@"清屏菜单";
        button.accessibilityHint=@"点按头像原位置，打开菜单或退出清屏";
        gALBCleanMenuHost.button=button;
        UITapGestureRecognizer *activation=[[UITapGestureRecognizer alloc] initWithTarget:gALBCleanMenuHost action:@selector(activateMenu:)];
        activation.cancelsTouchesInView=YES;
        if (@available(iOS 17.4, *)) activation.enabled=YES; else activation.enabled=NO;
        gALBCleanMenuHost.activationTap=activation;
        [gALBCleanMenuHost addGestureRecognizer:activation];
        [gALBCleanMenuHost addSubview:button]; [owner.view addSubview:gALBCleanMenuHost];
        [gALBCleanMenuController didMoveToParentViewController:owner];
        if (ALBReproIsRecording()) ALBReproRecordEvent(@"clean.menu.create", @{
            @"host":ALB30ReproView(gALBCleanMenuHost), @"window":ALB30ReproView(window)});
    }
    gALBCleanMenuHost.stack=stack; gAWEStableCleanScreenStack=stack;
    UIButton *button=gALBCleanMenuHost.button;
    if(!button.tracking && button.menu!=sourceButton.menu)button.menu=sourceButton.menu;
    button.enabled=YES;
    button.showsMenuAsPrimaryAction=YES;
    ALBRefreshCleanMenuAppearance(gALBCleanMenuHost,button);
    CGRect region=[gALBCleanMenuHost convertRect:CGRectIntersection(viewport,CGRectInset(frame,-8,-8)) fromView:window];
    gALBCleanMenuHost.hitRegion=region;
    button.transform=CGAffineTransformIdentity;
    button.frame=region; button.hidden=NO; button.alpha=1; button.userInteractionEnabled=YES;
    gALBCleanMenuHost.hidden=NO;
    gALBCleanMenuHost.alpha=1;
    gALBCleanMenuHost.layer.opacity=1;
    gALBCleanMenuHost.userInteractionEnabled=YES;
    // Preserve the controller responder chain and cover only the original
    // avatar region. Every other point still falls through to feed content.
    [owner.view bringSubviewToFront:gALBCleanMenuHost];
    ALBProtectAvatarMenuTouch(button,owner.view);
    ALB30CleanPlacementLog(@"placed",stack,region);
    return YES;
}
