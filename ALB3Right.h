/* Bar-glass helpers of the top chrome. Included by Tweak.x after
   ALBNativeInteraction.h. (3.2: the right column and the double-tap panel
   are 2.5.3's again; their optional registered glass is in ALB25Material.m.)
 */

static ALB3BarGlassButton *ALB3MakeBarGlassButton(NSString *identifier) {
    if (!ALB3ChromeAvailable()) return nil;
    Class buttonClass = NSClassFromString(@"ALB3BarGlassButton");
    if (!buttonClass) return nil;
    ALB3BarGlassButton *button = [[buttonClass alloc] initWithFrame:CGRectZero];
    // The AwemeLiquidBar. prefix keeps every native search and forward away
    // from our own views (ALBIsOwnedView).
    button.accessibilityIdentifier = identifier;
    return button;
}

/* A touch on our glass is the glass's alone. Douyin's own tap recognizers
   on the ancestors (the profile tap, the cell's pause / double-tap like)
   would otherwise also see it, so each of them is given a delegate gate that
   refuses touches inside a registered glass view and defers everything else
   to the original delegate. The clean-screen menu button is registered too:
   a UIButton counts only while it has a menu. */
@interface ALB3GlassTouchGate : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic, weak) id<UIGestureRecognizerDelegate> original;
@property(nonatomic, strong) NSHashTable<UIView *> *views;
@end

@implementation ALB3GlassTouchGate {
    BOOL _asking;
}
/* A re-entrant ask answers for this gate alone (backstop against a loop). */
- (BOOL)respondsToSelector:(SEL)selector {
    if ([super respondsToSelector:selector]) return YES;
    if (_asking) return NO;
    _asking = YES;
    BOOL answer = [self.original respondsToSelector:selector];
    _asking = NO;
    return answer;
}
- (id)forwardingTargetForSelector:(SEL)selector {
    if (_asking) return [super forwardingTargetForSelector:selector];
    _asking = YES;
    BOOL answer = [self.original respondsToSelector:selector];
    _asking = NO;
    return answer ? self.original : [super forwardingTargetForSelector:selector];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    for (UIView *view in self.views.allObjects) {
        if (!view.window || view.hidden || !view.userInteractionEnabled || view.alpha <= 0.01) continue;
        if ([view isKindOfClass:UIButton.class] && !((UIButton *)view).menu) continue;
        if (touch.view == view || [touch.view isDescendantOfView:view]) return NO;
        if ([view.superview.accessibilityIdentifier
                isEqualToString:@"AwemeLiquidBar.CleanScreen.MenuRegion"] &&
            touch.view == view.superview) return NO;
    }
    id<UIGestureRecognizerDelegate> original = self.original;
    return [original respondsToSelector:_cmd]
        ? [original gestureRecognizer:gesture shouldReceiveTouch:touch] : YES;
}
/* UIKit caches which delegate methods exist when the delegate is set. The
   original delegate is weak and may go away first, so every method answers
   itself with UIKit's default instead of relying on forwarding. */
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    id<UIGestureRecognizerDelegate> original = self.original;
    return [original respondsToSelector:_cmd] ? [original gestureRecognizerShouldBegin:gesture] : YES;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture
        shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other {
    id<UIGestureRecognizerDelegate> original = self.original;
    return [original respondsToSelector:_cmd]
        ? [original gestureRecognizer:gesture shouldRecognizeSimultaneouslyWithGestureRecognizer:other] : NO;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture
        shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer *)other {
    id<UIGestureRecognizerDelegate> original = self.original;
    return [original respondsToSelector:_cmd]
        ? [original gestureRecognizer:gesture shouldRequireFailureOfGestureRecognizer:other] : NO;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture
        shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    id<UIGestureRecognizerDelegate> original = self.original;
    return [original respondsToSelector:_cmd]
        ? [original gestureRecognizer:gesture shouldBeRequiredToFailByGestureRecognizer:other] : NO;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceivePress:(UIPress *)press {
    id<UIGestureRecognizerDelegate> original = self.original;
    return [original respondsToSelector:_cmd]
        ? [original gestureRecognizer:gesture shouldReceivePress:press] : YES;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveEvent:(UIEvent *)event {
    id<UIGestureRecognizerDelegate> original = self.original;
    return [original respondsToSelector:_cmd]
        ? [original gestureRecognizer:gesture shouldReceiveEvent:event] : YES;
}
@end

static void ALB3ProtectGlassTouch(UIView *view, UIView *stack) {
    static char gateKey;
    if (!view || !stack || ![view isDescendantOfView:stack]) return;
    // The clean-screen region owns an explicit tap recognizer; protect it
    // from feed-level taps without putting that recognizer behind the gate.
    BOOL independentRegion = [view.superview.accessibilityIdentifier
        isEqualToString:@"AwemeLiquidBar.CleanScreen.MenuRegion"];
    UIView *firstAncestor = independentRegion ? view.superview.superview : view.superview;
    for (UIView *ancestor = firstAncestor; ancestor && ancestor != view.window;
         ancestor = ancestor.superview) {
        for (UIGestureRecognizer *gesture in ancestor.gestureRecognizers) {
            NSString *name = NSStringFromClass(gesture.class).lowercaseString;
            if (![gesture isKindOfClass:UITapGestureRecognizer.class] &&
                ![name containsString:@"tap"] && ![name containsString:@"click"]) continue;
            ALB3GlassTouchGate *gate = objc_getAssociatedObject(gesture, &gateKey);
            if (!gate) {
                gate = [ALB3GlassTouchGate new];
                gate.views = [NSHashTable weakObjectsHashTable];
                objc_setAssociatedObject(gesture, &gateKey, gate,
                                         OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            }
            // 3.2.1: never wrap a chain that already holds this gate.
            if (gesture.delegate != gate && !ALB32DelegateChainContains(gesture.delegate, gate)) {
                gate.original = gesture.delegate;
                gesture.delegate = gate;
            }
            if (![gate.views containsObject:view]) [gate.views addObject:view];
        }
    }
}
