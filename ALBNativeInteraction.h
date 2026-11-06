/* Included by Tweak.x after the adaptive glass helpers. UIKit owns tracking. */
static char kALBNativeActionBridgeKey, kALBArtworkLeaseKey, kALBArtworkCacheKey;
/* 3.11.49：原来的动作没送到时的最后一招（BOOL (^)(void)，挂在玻璃按钮上；右下角音乐用它打开音乐页） */
static char kALBNativeFallbackKey;
static NSUInteger gALBNativeGestureAllowed, gALBNativeFallbackRuns, gALBNativeFallbackOK;
/* 3.11.51：最近一次替玻璃按钮转给抖音的是哪个控件（诊断）*/
static NSString *gALBNativeLastForward;
static BOOL ALBArtworkOwnsNativeGesture(UIButton *button, UIGestureRecognizer *gesture);
static NSHashTable<UIButton *> *ALBArtworkButtons(void) {
    static NSHashTable *buttons;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ buttons = [NSHashTable weakObjectsHashTable]; });
    return buttons;
}

/* Shared by the right column and message header. Exclude the ENTIRE owned
   subtree before looking for a native action; an internal UIImageView has no
   accessibility identifier of its own. */
static BOOL ALBIsOwnedView(UIView *view) {
    for (UIView *v = view; v; v = v.superview)
        if ([v.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."]) return YES;
    return NO;
}

BOOL ALBForwardOriginalView(UIView *source) {
    if (!source.window || ALBIsOwnedView(source)) return NO;
    static NSHashTable *dispatching;
    if (!dispatching) dispatching = [NSHashTable weakObjectsHashTable];
    if ([dispatching containsObject:source]) return NO;
    [dispatching addObject:source];
    BOOL sent = NO;
    @try {
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:source];
        for (NSUInteger i=0; i<queue.count && i<64 && !sent; i++) {
            UIView *view = queue[i];
            if (ALBIsOwnedView(view)) continue;
            if ([view isKindOfClass:UIControl.class]) {
                UIControl *control = (UIControl *)view;
                if (control.enabled) {
                    UIControlEvents events = control.allControlEvents;
                    UIControlEvents event = (events & UIControlEventTouchUpInside)
                        ? UIControlEventTouchUpInside : (events & UIControlEventPrimaryActionTriggered);
                    if (event) {
                        [control sendActionsForControlEvents:event]; sent = YES;
                        gALBNativeLastForward = NSStringFromClass(control.class);
                    }
                }
            }
            [queue addObjectsFromArray:view.subviews];
        }
        if (!sent) {
            sent = [source accessibilityActivate];
            gALBNativeLastForward = [NSString stringWithFormat:@"%@（无障碍点按%@）", NSStringFromClass(source.class), sent ? @"" : @"，没接"];
        }
    } @catch (NSException *exception) {
        AWEInteractionDiagnosticWrite(@"NATIVE_CONTROL_EXCEPTION class=%@ exception=%@",
            NSStringFromClass(source.class), exception.name);
    } @finally { [dispatching removeObject:source]; }
    return sent;
}


/* The four feed buttons are UIView, not UIControl. Keep the glass UIButton as
   the actual hit view, and deliver the SAME physical responder sequence to
   its native parent. Native gestures remain on that ancestry and may cancel
   the sequence normally. No fabricated touches or guessed business selectors. */
/* Artwork controls also keep visibility leases while configuration/layout
   writes alpha=1. This is local to our controls, never a global UIView hook. */
/* 3.11: the glass button itself is the visible glass in Douyin's tree (no
   copy elsewhere to hand the touches to): UIKit's own press, stretch and
   distortion are its own. */
/* The avatar's menu (this button's own) is about to grow out of it / has
   finished going away (3.11: ALBRGMenuBegins / ALBRGMenuEnds below). */
static void (*gALBArtworkMenuBegins)(UIButton *button);
static void (*gALBArtworkMenuEnds)(UIButton *button);
@interface ALBArtworkGlassButton : UIButton
@end
/* 3.11.4: a glass with an edge of its own keeps it right under itself
   (ALBFollowSyncEdge; nothing happens unless "glass.followBar" put it there). */
#define ALB_FOLLOW_EDGE_GEOMETRY \
- (void)setFrame:(CGRect)frame { [super setFrame:frame]; if (objc_getAssociatedObject(self, &kALBFollowModeKey)) ALBFollowSyncEdge(self); } \
- (void)setCenter:(CGPoint)center { [super setCenter:center]; if (objc_getAssociatedObject(self, &kALBFollowModeKey)) ALBFollowSyncEdge(self); } \
- (void)setBounds:(CGRect)bounds { [super setBounds:bounds]; if (objc_getAssociatedObject(self, &kALBFollowModeKey)) ALBFollowSyncEdge(self); } \
- (void)didMoveToSuperview { [super didMoveToSuperview]; if (objc_getAssociatedObject(self, &kALBFollowModeKey) || objc_getAssociatedObject(self, &kALBFollowProxyKey)) ALBFollowSyncEdge(self); } \
- (void)layoutSubviews { [super layoutSubviews]; ALBFaceLayout(self); } \
- (void)setHighlighted:(BOOL)highlighted { [super setHighlighted:highlighted]; ALBFaceSetPressed(self, highlighted); } \
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event { return ALBFaceFilterHit(self, [super hitTest:point withEvent:event]); }
@implementation ALBArtworkGlassButton
ALB_FOLLOW_EDGE_GEOMETRY
- (void)setAlpha:(CGFloat)alpha { [super setAlpha:ALB7RightFilterAlpha(self,alpha)]; }
/* 3.11.49 用户：“右下角音乐图标现在的点击也不生效”。音乐的玻璃按钮压在抖音音乐视图里面，抖音的点按手势在它外层；按钮是
   UIControl，系统默认不让外层的单击手势认它的点击，于是音乐页打不开。绑定好的那个原生点按手势放行。 */
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    if (gesture.view != self && ALBArtworkOwnsNativeGesture(self, gesture)) { gALBNativeGestureAllowed++; return YES; }
    return [super gestureRecognizerShouldBegin:gesture];
}
- (UIContextMenuConfiguration *)contextMenuInteraction:(UIContextMenuInteraction *)interaction
                        configurationForMenuAtLocation:(CGPoint)location {
    if (gALBArtworkMenuBegins && self.menu) gALBArtworkMenuBegins(self);
    return [super contextMenuInteraction:interaction configurationForMenuAtLocation:location];
}
- (void)contextMenuInteraction:(UIContextMenuInteraction *)interaction
       willEndForConfiguration:(UIContextMenuConfiguration *)configuration
                      animator:(id<UIContextMenuInteractionAnimating>)animator {
    [super contextMenuInteraction:interaction willEndForConfiguration:configuration animator:animator];
    __weak UIButton *weakSelf = self;
    void (^done)(void) = ^{ UIButton *button = weakSelf; if (button && gALBArtworkMenuEnds) gALBArtworkMenuEnds(button); };
    if (animator) [animator addCompletion:done]; else done();
}
@end

@interface ALBRightGlassButton : UIButton
@property(nonatomic, weak) UIView *nativeTouchSource;
@property(nonatomic, weak) UIView *activeNativeSource;
@property(nonatomic, copy) NSSet<UITouch *> *activeTouches;
- (void)cancelNativeSequence;
@end
@implementation ALBRightGlassButton
ALB_FOLLOW_EDGE_GEOMETRY
- (void)setAlpha:(CGFloat)alpha { [super setAlpha:ALB7RightFilterAlpha(self,alpha)]; }
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    self.activeNativeSource = self.nativeTouchSource;
    self.activeTouches = touches;
    [super touchesBegan:touches withEvent:event];
    [self.activeNativeSource touchesBegan:touches withEvent:event];
    UIView *source = self.activeNativeSource;
    BOOL nativeEnd = [source methodForSelector:@selector(touchesEnded:withEvent:)] !=
        [UIView instanceMethodForSelector:@selector(touchesEnded:withEvent:)];
    AWEInteractionDiagnosticWrite(@"RIGHT_PHYSICAL_BEGIN source=%@ handler=%d gestures=%lu glassHit=%d",
        NSStringFromClass(source.class), nativeEnd, (unsigned long)source.gestureRecognizers.count,
        touches.anyObject.view == self || [touches.anyObject.view isDescendantOfView:self]);
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self.activeNativeSource touchesMoved:touches withEvent:event];
    [super touchesMoved:touches withEvent:event];
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UIView *source = self.activeNativeSource;
    self.activeNativeSource = nil;
    self.activeTouches = nil;
    [super touchesEnded:touches withEvent:event];
    if (source.window == self.window && source.window) [source touchesEnded:touches withEvent:event];
    AWEInteractionDiagnosticWrite(@"RIGHT_PHYSICAL_END source=%@", NSStringFromClass(source.class));
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    UIView *source = self.activeNativeSource;
    self.activeNativeSource = nil;
    self.activeTouches = nil;
    [source touchesCancelled:touches withEvent:event];
    [super touchesCancelled:touches withEvent:event];
}
- (void)cancelNativeSequence {
    UIView *source = self.activeNativeSource;
    NSSet *touches = self.activeTouches;
    self.activeNativeSource = nil;
    self.activeTouches = nil;
    if (source && touches.count) [source touchesCancelled:touches withEvent:nil];
    [self cancelTrackingWithEvent:nil];
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (!self.window && self.activeNativeSource) [self cancelNativeSequence];
}
@end

/* Save clipping per native ancestor, shared by any artwork/buttons using it.
   Only clipping changes: sibling order, follow badge and native masks are
   restored when the last owner releases the ancestor. */
@interface ALBClipRecord : NSObject
@property(nonatomic, weak) UIView *view;
@property(nonatomic) BOOL clips;
@property(nonatomic) BOOL masks;
@property(nonatomic, strong) CALayer *mask;
@property(nonatomic, strong) NSHashTable *owners;
@end
@implementation ALBClipRecord
@end
static char kALBClipRecordKey, kALBButtonClipRecordsKey;
static void ALBReleaseClipRecord(ALBClipRecord *record, UIButton *button) {
    [record.owners removeObject:button];
    if (record.owners.allObjects.count) return;
    UIView *view = record.view;
    view.clipsToBounds = record.clips;
    view.layer.masksToBounds = record.masks;
    view.layer.mask = record.mask;
    objc_setAssociatedObject(view, &kALBClipRecordKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBReleaseClipping(UIButton *button) {
    for (ALBClipRecord *record in objc_getAssociatedObject(button, &kALBButtonClipRecordsKey))
        ALBReleaseClipRecord(record, button);
    objc_setAssociatedObject(button, &kALBButtonClipRecordsKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBOpenClipping(UIButton *button, UIView *stop) {
    NSMutableArray *records = [NSMutableArray array];
    for (UIView *v = button.superview; v && v != stop && v != v.window; v = v.superview) {
        ALBClipRecord *record = objc_getAssociatedObject(v, &kALBClipRecordKey);
        if (!record && !v.clipsToBounds && !v.layer.masksToBounds && !v.layer.mask) continue;
        if (!record) {
            record = [ALBClipRecord new];
            record.view = v; record.clips = v.clipsToBounds;
            record.masks = v.layer.masksToBounds; record.mask = v.layer.mask;
            record.owners = [NSHashTable weakObjectsHashTable];
            objc_setAssociatedObject(v, &kALBClipRecordKey, record, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        [record.owners addObject:button]; [records addObject:record];
        v.clipsToBounds = NO; v.layer.masksToBounds = NO; v.layer.mask = nil;
    }
    for (ALBClipRecord *old in objc_getAssociatedObject(button, &kALBButtonClipRecordsKey))
        if (![records containsObject:old]) ALBReleaseClipRecord(old, button);
    objc_setAssociatedObject(button, &kALBButtonClipRecordsKey, records, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    button.clipsToBounds = NO; button.layer.masksToBounds = NO;
}

@interface ALBNativeActionBridge : NSObject <UIGestureRecognizerDelegate>
@property(nonatomic, weak) UIView *source;
@property(nonatomic, weak) UIButton *button;
@property(nonatomic, strong) UIGestureRecognizer *gesture;
@property(nonatomic, weak) UIView *originalGestureView;
@property(nonatomic, weak) id<UIGestureRecognizerDelegate> originalDelegate;
@property(nonatomic) BOOL originalCancels;
@property(nonatomic) BOOL delivered;
@property(nonatomic) BOOL rightColumn;
@property(nonatomic) BOOL receivingOwnTouch;
@property(nonatomic) NSUInteger sequence;
@property(nonatomic) NSUInteger activatedSequence;
- (void)unbind;
- (void)activate:(UIButton *)button;
@end
@implementation ALBNativeActionBridge {
    BOOL _asking;
}
// 3.2.1: a re-entrant ask answers for this bridge alone (backstop against a loop).
- (BOOL)respondsToSelector:(SEL)selector {
    if ([super respondsToSelector:selector]) return YES;
    if (_asking) return NO;
    _asking = YES;
    BOOL answer = [self.originalDelegate respondsToSelector:selector];
    _asking = NO;
    return answer;
}
- (id)forwardingTargetForSelector:(SEL)selector {
    if (_asking) return [super forwardingTargetForSelector:selector];
    _asking = YES;
    BOOL answer = [self.originalDelegate respondsToSelector:selector];
    _asking = NO;
    return answer ? self.originalDelegate : [super forwardingTargetForSelector:selector];
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    UIButton *button = self.button;
    BOOL ourTouch = touch.view == button || [touch.view isDescendantOfView:button];
    self.receivingOwnTouch = ourTouch;
    if (ourTouch && button.window && !button.hidden && button.enabled) {
        self.delivered = NO;
        self.sequence++;
        // Only this button's original single-tap action gets this exception.
        // Douyin delegates commonly reject UIControl descendants by class.
        return YES;
    }
    id<UIGestureRecognizerDelegate> original = self.originalDelegate;
    return [original respondsToSelector:_cmd] ? [original gestureRecognizer:gesture shouldReceiveTouch:touch] : YES;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture
        shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other {
    if (other.view == self.button || [other.view isDescendantOfView:self.button]) return YES;
    id<UIGestureRecognizerDelegate> original = self.originalDelegate;
    return [original respondsToSelector:_cmd]
        ? [original gestureRecognizer:gesture shouldRecognizeSimultaneouslyWithGestureRecognizer:other] : NO;
}
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    if ((self.rightColumn || [self.button isKindOfClass:ALBArtworkGlassButton.class]) && self.receivingOwnTouch) return YES;
    if (self.receivingOwnTouch && [objc_getAssociatedObject(self.button,
        NSSelectorFromString(@"ALB28.KeepProxyInParent")) boolValue]) return YES;
    id<UIGestureRecognizerDelegate> original = self.originalDelegate;
    return [original respondsToSelector:_cmd] ? [original gestureRecognizerShouldBegin:gesture] : YES;
}
- (void)touchDown:(UIButton *)button { self.delivered = NO; self.sequence++; }
- (void)nativeAction:(UIGestureRecognizer *)gesture {
    if (gesture.state != UIGestureRecognizerStateEnded) return;
    self.delivered = YES;
    AWEInteractionDiagnosticWrite(@"NATIVE_GLASS_PHYSICAL_ACTION source=%@ gesture=%@",
        NSStringFromClass(self.source.class), NSStringFromClass(gesture.class));
}
- (void)activate:(UIButton *)button {
    // The native UIView already got the real responder/gesture sequence.
    // Sending an extra fallback here could toggle Like/Favorite twice.
    if (self.rightColumn) return;
    /* 3.11.51：音乐玻璃的点按是表面那层转交过来、抬手时补发的（ALBFaceTapOnTouchUp），没有 TouchDown，序号和“已送到”不会
       重置；上一次送到后这里就一直当“已送到”跳过。同一个序号第二次进来＝新的一次点按。 */
    if (self.sequence == self.activatedSequence) { self.sequence++; self.delivered = NO; }
    self.activatedSequence = self.sequence;
    NSUInteger sequence = self.sequence;
    __weak ALBNativeActionBridge *weakSelf = self;
    // UIButton and recognizer targets have unspecified order. Wait for UIKit
    // to finish this physical event, then deliver at most ONE business action.
    dispatch_async(dispatch_get_main_queue(), ^{
        ALBNativeActionBridge *bridge = weakSelf;
        if (!bridge || bridge.sequence != sequence || bridge.delivered ||
            !button.window || button.hidden) return;
        BOOL sent = ALBForwardOriginalView(bridge.source);
        BOOL (^fallback)(void) = objc_getAssociatedObject(button, &kALBNativeFallbackKey);
        if (!sent && fallback) {
            gALBNativeFallbackRuns++;
            sent = fallback();
            if (sent) gALBNativeFallbackOK++;
        }
        bridge.delivered = sent;
        AWEInteractionDiagnosticWrite(@"NATIVE_GLASS_CONTROL_FALLBACK sent=%d source=%@",
            sent, NSStringFromClass(bridge.source.class));
        if (!sent) AWEShowDownloadToast(@"此按钮动作未匹配，请导出诊断日志");
    });
}
- (void)unbind {
    self.sequence++;
    UIGestureRecognizer *gesture = self.gesture;
    [gesture removeTarget:self action:@selector(nativeAction:)];
    if (gesture.delegate == self) gesture.delegate = self.originalDelegate;
    if (gesture) gesture.cancelsTouchesInView = self.originalCancels;
    if (gesture.view == self.button && self.originalGestureView)
        [self.originalGestureView addGestureRecognizer:gesture];
    self.gesture = nil;
    self.originalGestureView = nil;
    self.originalDelegate = nil;
}
- (void)dealloc { [self unbind]; }
@end

static BOOL ALBArtworkOwnsNativeGesture(UIButton *button, UIGestureRecognizer *gesture) {
    ALBNativeActionBridge *bridge = button ? objc_getAssociatedObject(button, &kALBNativeActionBridgeKey) : nil;
    return bridge && gesture && bridge.gesture == gesture;
}

void ALBUnbindOriginalGlassAction(UIButton *button) {
    ALBNativeActionBridge *bridge = objc_getAssociatedObject(button, &kALBNativeActionBridgeKey);
    [bridge unbind];
    bridge.source = nil;
    if ([button isKindOfClass:ALBRightGlassButton.class]) {
        [(ALBRightGlassButton *)button cancelNativeSequence];
        ((ALBRightGlassButton *)button).nativeTouchSource = nil;
        ALBReleaseClipping(button);
    }
}

BOOL ALBBindOriginalGlassAction(UIButton *button, UIView *source) {
    if (!button || !source.window || ALBIsOwnedView(source)) return NO;
    ALBNativeActionBridge *bridge = objc_getAssociatedObject(button, &kALBNativeActionBridgeKey);
    if (!bridge) {
        bridge = [ALBNativeActionBridge new];
        bridge.button = button;
        [button addTarget:bridge action:@selector(touchDown:) forControlEvents:UIControlEventTouchDown];
        [button addTarget:bridge action:@selector(activate:) forControlEvents:UIControlEventTouchUpInside];
        objc_setAssociatedObject(button, &kALBNativeActionBridgeKey, bridge, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    bridge.rightColumn = [button isKindOfClass:ALBRightGlassButton.class];
    if (bridge.source != source) {
        if (bridge.rightColumn) [(ALBRightGlassButton *)button cancelNativeSequence];
        [bridge unbind]; bridge.source = source;
    }
    if (bridge.rightColumn && !button.tracking) {
        ((ALBRightGlassButton *)button).nativeTouchSource = source;
        // Stay under the original UIView's real recognizer/responder ancestry.
        // 3.11: the right column's glass sits at the bottom of Douyin's own
        // button (the dock's place for its glass): Douyin's count, badges and
        // animations draw over the glass; touches still reach the glass first
        // (AWEFeedVideoButton's hit test). Other glasses (the danmaku send
        // glass) keep 3.10's place on top of their source.
        BOOL underNative = [button.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.HomeAction.NativeButton"];
        if (button.superview != source) {
            CGRect frame = [button.superview convertRect:button.frame toView:source];
            if (underNative) [source insertSubview:button atIndex:0]; else [source addSubview:button];
            button.frame = frame;
        } else if (underNative && !ALBFollowAtBottom(button)) {
            [source insertSubview:button atIndex:0];
        }
        source.userInteractionEnabled = YES;
    }
    button.userInteractionEnabled = YES;
    button.exclusiveTouch = NO;
    // 3.2.1: still bound when a gate above holds this bridge in its chain.
    if (bridge.gesture && bridge.gesture.enabled && (bridge.gesture.delegate == bridge ||
        ALB32DelegateChainContains(bridge.gesture.delegate, bridge))) return YES;
    [bridge unbind];
    UIGestureRecognizer *gesture = AWEPrimaryHomeActionTapGesture(source);
    if ([gesture isKindOfClass:UITapGestureRecognizer.class] &&
        ((UITapGestureRecognizer *)gesture).numberOfTapsRequired != 1) gesture = nil;
    UIView *host = gesture.view;
    if (!host) return YES; // original UIControl path
    // 3.2.1: a gate above may still hold this bridge; never loop back to it.
    if (ALB32DelegateChainContains(gesture.delegate, bridge)) return YES;
    bridge.originalGestureView = host;
    bridge.originalDelegate = gesture.delegate;
    bridge.originalCancels = gesture.cancelsTouchesInView;
    bridge.gesture = gesture;
    // Preserve the original recognizer.view whenever it can contain the glass.
    // Only the new glass moves; source controls keep their frames and parents.
    if (![button isDescendantOfView:host]) {
        BOOL keepProxyInParent = [objc_getAssociatedObject(button,
            NSSelectorFromString(@"ALB28.KeepProxyInParent")) boolValue];
        if (!keepProxyInParent && !bridge.rightColumn && !host.hidden && host.alpha > 0.01 && ![host isKindOfClass:UIImageView.class] &&
            host.layer.opacity > 0.01 && button.superview) {
            CGRect frame = [button.superview convertRect:button.frame toView:host];
            [host addSubview:button];
            button.frame = frame;
        } else {
            [button addGestureRecognizer:gesture];
        }
    }
    gesture.cancelsTouchesInView = bridge.rightColumn ? bridge.originalCancels : NO;
    gesture.delegate = bridge;
    [gesture addTarget:bridge action:@selector(nativeAction:)];
    return YES;
}

static void ALBBindNativeAction(UIButton *button, UIView *source, UIView *stack, AWEHomeActionKind kind) {
    (void)kind;
    ALBBindOriginalGlassAction(button, source);
    ALBOpenClipping(button, stack);
}

static void ALBSetRestingButtonGeometry(UIButton *button,CGRect restingFrame) {
    /* A released native glass button may still be animating its transform.
       Assigning frame in that state solves a new (smaller) bounds from the
       transformed rectangle. The smaller bounds outlive the release spring.
       Bounds/center describe the resting geometry without changing tracking
       or any UIKit-owned press/deformation transform. */
    if (!button || button.tracking || !isfinite(restingFrame.size.width) ||
        !isfinite(restingFrame.size.height) || restingFrame.size.width<=0 ||
        restingFrame.size.height<=0) return;
    CGRect bounds=CGRectMake(0,0,restingFrame.size.width,restingFrame.size.height);
    CGPoint center=CGPointMake(CGRectGetMidX(restingFrame),CGRectGetMidY(restingFrame));
    ALB19RightSetBounds(button,bounds);
    ALB19RightSetCenter(button,center);
}

static void ALBLayoutRightNativeButton(AWEHomeActionSlotView *slot) {
    UIButton *button = slot.systemGlassButton;
    UIView *host = button.superview;
    if (!button || !host || button.tracking) return;
    CGRect frame = host == slot ? slot.bounds :
        ALB19RightConvertRect(slot,slot.bounds,host,slot.superview);
    ALBSetRestingButtonGeometry(button,frame);
}

/* Keep decorative artwork in its original local stacking plane. Follow and
   same-style controls retain their native sibling order and hit priority. */
/* 3.11.4: right above `below`, the glass's own edge between them aside
   (ALBFollowSyncEdge keeps that edge right under the glass). */
static BOOL ALBFollowRightAbove(UIView *button, UIView *below) {
    NSArray<UIView *> *siblings = button.superview.subviews;
    NSUInteger at = [siblings indexOfObjectIdenticalTo:button], base = [siblings indexOfObjectIdenticalTo:below];
    if (at == NSNotFound || base == NSNotFound) return NO;
    if (at == base + 1) return YES;
    UIView *proxy = objc_getAssociatedObject(button, &kALBFollowProxyKey);
    return proxy && at == base + 2 && siblings[base + 1] == proxy;
}
static void ALBPlaceArtworkButton(UIButton *button, UIImageView *source, UIView *coordinates, CGRect frame) {
    UIView *parent = source.superview;
    if (!button || !parent || !coordinates || ALBIsOwnedView(source)) return;
    CGRect local = ALB19RightConvertRect(coordinates,frame,parent,coordinates);
    if (button.superview != parent) [parent insertSubview:button aboveSubview:source];
    else if (!ALBFollowRightAbove(button, source))
        [parent insertSubview:button aboveSubview:source];
    button.layer.zPosition = source.layer.zPosition;
    ALBOpenClipping(button, coordinates);
    ALBSetRestingButtonGeometry(button,local);
}

@interface ALBArtworkLease : NSObject
@property(nonatomic, weak) UIImageView *source;
@property(nonatomic) float originalOpacity;
- (void)restore;
@end
@implementation ALBArtworkLease
- (void)restore { if (self.source) self.source.layer.opacity = self.originalOpacity; self.source = nil; }
- (void)dealloc { [self restore]; }
@end

static void ALBReleaseArtwork(UIButton *button) {
    ALBReleaseClipping(button);
    ALBArtworkLease *lease = objc_getAssociatedObject(button, &kALBArtworkLeaseKey);
    [lease restore];
    objc_setAssociatedObject(button, &kALBArtworkLeaseKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

static void ALBRestoreAllNativeArtwork(void) {
    for (UIButton *button in ALBArtworkButtons().allObjects) { ALBReleaseArtwork(button); ALBUnbindOriginalGlassAction(button); button.hidden = YES; }
    // 3.11.4: with the plug-in off every right-column glass leaves its edge
    // (ALBFollowSyncEdge removes the registration and the empty scroll view).
    for (UIButton *button in ALBRGButtons().allObjects) { ALBFaceSync(button); ALBFollowSyncEdge(button); }
}

/* 3.11: where UIKit really draws the artwork on its glass button (the layer
   showing that very image) and, when it is off the middle by more than a
   third of a point, the button's content insets push it back (at most four
   corrections per picture, measured once after a layout, never per frame).
   The picture is as big as the glass with the artwork in its middle, in
   whole points, and the button sits under the column's own scale (not a
   copy in another window), so glass and picture are concentric by
   construction; the measurement only catches UIKit placing the image
   against an edge (3.10.6). */
static char kALBArtworkShiftKey, kALBArtworkShiftTriesKey, kALBArtworkShiftImageKey, kALBArtworkShiftPendingKey;
static CALayer *ALBArtworkLayerShowing(CALayer *layer, CGImageRef image, NSUInteger depth) {
    if (!layer || !image || depth > 10) return nil;
    if ((__bridge void *)layer.contents == (void *)image) return layer;
    for (CALayer *sublayer in layer.sublayers) {
        CALayer *found = ALBArtworkLayerShowing(sublayer, image, depth + 1);
        if (found) return found;
    }
    return nil;
}
static void ALBArtworkApplyShift(UIButton *button) {
    UIButtonConfiguration *configuration = button.configuration;
    if (!configuration.image || button.isTracking || button.isHighlighted || ALBRGMenuActive(button)) return;
    CGPoint shift = [objc_getAssociatedObject(button, &kALBArtworkShiftKey) CGPointValue];
    NSDirectionalEdgeInsets insets = NSDirectionalEdgeInsetsMake(shift.y, shift.x, -shift.y, -shift.x);
    NSDirectionalEdgeInsets now = configuration.contentInsets;
    if (fabs(now.top - insets.top) < 0.01 && fabs(now.leading - insets.leading) < 0.01 &&
        fabs(now.bottom - insets.bottom) < 0.01 && fabs(now.trailing - insets.trailing) < 0.01) return;
    UIButtonConfiguration *next = [configuration copy];
    next.contentInsets = insets;
    button.configuration = next;
}
static void ALBArtworkMeasure(UIButton *button) {
    UIImage *image = button.configuration.image;
    if (!image || image.isSymbolImage || !button.window || CGRectIsEmpty(button.bounds) ||
        button.isTracking || button.isHighlighted || ALBRGMenuActive(button)) return;
    if (objc_getAssociatedObject(button, &kALBArtworkShiftImageKey) != image) {
        objc_setAssociatedObject(button, &kALBArtworkShiftImageKey, image, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        objc_setAssociatedObject(button, &kALBArtworkShiftTriesKey, @0, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSInteger tries = [objc_getAssociatedObject(button, &kALBArtworkShiftTriesKey) integerValue];
    if (tries >= 4) return;
    [button layoutIfNeeded];
    CALayer *drawn = ALBArtworkLayerShowing(button.layer, image.CGImage, 0);
    if (!drawn) return;
    CGRect rect = [drawn convertRect:drawn.bounds toLayer:button.layer];
    CGPoint offset = CGPointMake(CGRectGetMidX(button.bounds) - CGRectGetMidX(rect),
                                 CGRectGetMidY(button.bounds) - CGRectGetMidY(rect));
    if (fabs(offset.x) < 0.34 && fabs(offset.y) < 0.34) return;
    CGPoint shift = [objc_getAssociatedObject(button, &kALBArtworkShiftKey) CGPointValue];
    shift.x += offset.x; shift.y += offset.y;
    objc_setAssociatedObject(button, &kALBArtworkShiftKey, [NSValue valueWithCGPoint:shift], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(button, &kALBArtworkShiftTriesKey, @(tries + 1), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ALBArtworkApplyShift(button);
}
static void ALBArtworkScheduleMeasure(UIButton *button) {
    if (!button || [objc_getAssociatedObject(button, &kALBArtworkShiftPendingKey) boolValue]) return;
    objc_setAssociatedObject(button, &kALBArtworkShiftPendingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIButton *weakButton = button;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.05 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        UIButton *live = weakButton;
        if (!live) return;
        objc_setAssociatedObject(live, &kALBArtworkShiftPendingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        ALBArtworkMeasure(live);
    });
}

static void ALBInstallNativeArtwork(UIButton *button, UIImageView *source, CGFloat side, CGFloat canvas, NSString *role) {
    if (!button || !source.image) { ALBReleaseArtwork(button); return; }
    ALBArtworkLease *lease = objc_getAssociatedObject(button, &kALBArtworkLeaseKey);
    if (lease.source != source) {
        [lease restore];
        lease = [ALBArtworkLease new];
        lease.source = source;
        lease.originalOpacity = source.layer.opacity;
        objc_setAssociatedObject(button, &kALBArtworkLeaseKey, lease, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    canvas = MAX(side, canvas);
    NSDictionary *cache = objc_getAssociatedObject(button, &kALBArtworkCacheKey);
    /* 3.11: a new picture (the cell now shows another video's author or
       cover) means no menu of this button is up any more, even if UIKit
       never reported its end: drop the menu flag so the picture is not held. */
    if ((cache && cache[@"source"] != source.image) && objc_getAssociatedObject(button, &kALBRGMenuActiveKey)) {
        objc_setAssociatedObject(button, &kALBRGMenuActiveKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        ALBRGToneStyle(button);
    }
    UIImage *image = cache[@"image"];
    if (cache[@"source"] != source.image || [cache[@"side"] doubleValue] != side ||
        [cache[@"canvas"] doubleValue] != canvas) {
        UIImage *original = source.image;
        UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(canvas, canvas)];
        CGFloat inset = (canvas - side) * 0.5;
        image = [[renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
            CGRect circle = CGRectMake(inset, inset, side, side);
            [[UIBezierPath bezierPathWithOvalInRect:circle] addClip];
            CGFloat scale = MAX(side / MAX(1, original.size.width), side / MAX(1, original.size.height));
            CGSize size = CGSizeMake(original.size.width * scale, original.size.height * scale);
            [original drawInRect:CGRectMake(inset + (side-size.width)*0.5, inset + (side-size.height)*0.5, size.width, size.height)];
        }] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
        objc_setAssociatedObject(button, &kALBArtworkCacheKey,
            @{@"source":source.image, @"side":@(side), @"canvas":@(canvas), @"image":image}, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSString *identity = [NSString stringWithFormat:@"%@.%p", role, (__bridge void *)image];
    AWEConfigureAdaptiveSystemGlassIconButton(button, image, identity, canvas, nil, YES, AWEHomeTintedGlassEnabled(), NO);
    // The UIImage is configuration-owned, so UIKit's lens also deforms it.
    ALBArtworkApplyShift(button);
    ALBArtworkScheduleMeasure(button);
    source.layer.opacity = 0;
    [ALBArtworkButtons() addObject:button];
}

static UIImageView *ALBMusicArtwork(UIView *root) {
    if (!root) return nil;
    NSMutableArray *queue = [NSMutableArray arrayWithObject:root];
    UIImageView *best = nil;
    CGFloat score = CGFLOAT_MAX;
    for (NSUInteger i=0; i<queue.count && i<80; i++) {
        UIView *view = queue[i];
        if ([view.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."]) continue;
        if ([view isKindOfClass:UIImageView.class] && ((UIImageView *)view).image) {
            CGFloat w=view.bounds.size.width, h=view.bounds.size.height;
            CGFloat delta=fabs(w-40)+fabs(h-40);
            if (w>=20 && w<=64 && h>=20 && h<=64 && delta<score) { best=(UIImageView *)view; score=delta; }
        }
        [queue addObjectsFromArray:view.subviews];
    }
    return best;
}

/* 3.11: the avatar's menu grows out of the avatar's own glass button (it is
   the visible avatar glass now, in Douyin's tree). The button's glass is
   turned by UIKit on its own edge, but the menu is no edge element and would
   take the button's Dark: one small picture of Douyin's window when the menu
   opens decides (3.10.12's rule, half brightness). Nothing re-dresses it
   during the menu (ALBRGToneStyle waits); once the menu has gone it goes back
   to Dark. */
static UIImage *AWECreateLowResolutionBackdropSnapshot(void);
static CGFloat AWEIOS27AverageLumaInPatch(UIImage *image, CGPoint point, CGFloat patchPoints);
static CGFloat AWEIOS27MedianLuma(CGFloat *values, NSUInteger count);
static void ALBRGMenuBegins(UIButton *button) {
    if (!button.window) return;
    objc_setAssociatedObject(button, &kALBRGMenuActiveKey, @(CACurrentMediaTime()), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    UIUserInterfaceStyle style = UIUserInterfaceStyleDark;
    UIImage *snapshot = AWECreateLowResolutionBackdropSnapshot();
    const CGFloat threshold = 0.5;
    if (snapshot) {
        CGRect frame = [button.window convertRect:[button convertRect:button.bounds toView:nil] toWindow:nil];
        CGFloat values[8]; NSUInteger count = 0;
        for (NSUInteger i = 0; i < 8; i++) {
            CGFloat angle = M_PI * 2 * i / 8.0, radius = CGRectGetWidth(frame) * 0.72;
            CGPoint point = CGPointMake(CGRectGetMidX(frame) + cos(angle) * radius, CGRectGetMidY(frame) + sin(angle) * radius);
            CGFloat value = AWEIOS27AverageLumaInPatch(snapshot, point, 8);
            if (value >= 0) values[count++] = value;
        }
        if (count >= 3 && AWEIOS27MedianLuma(values, count) > threshold) style = UIUserInterfaceStyleLight;
    }
    if (button.overrideUserInterfaceStyle != style) {
        button.overrideUserInterfaceStyle = style;
        [button setNeedsUpdateConfiguration];
        [button layoutIfNeeded];
    }
}
static void ALBRGMenuEnds(UIButton *button) {
    // The flag itself, not ALBRGMenuActive: a menu left open past 20 s still
    // ends here and goes back to Dark.
    if (!objc_getAssociatedObject(button, &kALBRGMenuActiveKey)) return;
    objc_setAssociatedObject(button, &kALBRGMenuActiveKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (button.overrideUserInterfaceStyle == UIUserInterfaceStyleDark) return;
    [UIView animateWithDuration:0.3 delay:0 options:UIViewAnimationOptionAllowUserInteraction | UIViewAnimationOptionBeginFromCurrentState
                     animations:^{ ALBRGToneStyle(button); [button layoutIfNeeded]; } completion:nil];
}
__attribute__((constructor)) static void ALBRGInstallMenuHooks(void) {
    gALBArtworkMenuBegins = ALBRGMenuBegins;
    gALBArtworkMenuEnds = ALBRGMenuEnds;
}
