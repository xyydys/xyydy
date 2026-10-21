/* Included by Tweak.x after the adaptive glass helpers. UIKit owns tracking. */
static char kALBNativeActionBridgeKey, kALBArtworkLeaseKey, kALBArtworkCacheKey;
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
                    if (event) { [control sendActionsForControlEvents:event]; sent = YES; }
                }
            }
            [queue addObjectsFromArray:view.subviews];
        }
        if (!sent) sent = [source accessibilityActivate];
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
@interface ALBArtworkGlassButton : UIButton
@end
@implementation ALBArtworkGlassButton
- (void)setAlpha:(CGFloat)alpha { [super setAlpha:ALB7RightFilterAlpha(self,alpha)]; }
@end

@interface ALBRightGlassButton : UIButton
@property(nonatomic, weak) UIView *nativeTouchSource;
@property(nonatomic, weak) UIView *activeNativeSource;
@property(nonatomic, copy) NSSet<UITouch *> *activeTouches;
- (void)cancelNativeSequence;
@end
@implementation ALBRightGlassButton
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
- (void)unbind;
- (void)activate:(UIButton *)button;
@end
@implementation ALBNativeActionBridge
- (BOOL)respondsToSelector:(SEL)selector {
    return [super respondsToSelector:selector] || [self.originalDelegate respondsToSelector:selector];
}
- (id)forwardingTargetForSelector:(SEL)selector {
    return [self.originalDelegate respondsToSelector:selector] ? self.originalDelegate : [super forwardingTargetForSelector:selector];
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
    if (self.rightColumn && self.receivingOwnTouch) return YES;
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
    NSUInteger sequence = self.sequence;
    __weak ALBNativeActionBridge *weakSelf = self;
    // UIButton and recognizer targets have unspecified order. Wait for UIKit
    // to finish this physical event, then deliver at most ONE business action.
    dispatch_async(dispatch_get_main_queue(), ^{
        ALBNativeActionBridge *bridge = weakSelf;
        if (!bridge || bridge.sequence != sequence || bridge.delivered ||
            !button.window || button.hidden) return;
        BOOL sent = ALBForwardOriginalView(bridge.source);
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
        if (button.superview != source) {
            CGRect frame = [button.superview convertRect:button.frame toView:source];
            [source addSubview:button]; button.frame = frame;
        }
        source.userInteractionEnabled = YES;
    }
    button.userInteractionEnabled = YES;
    button.exclusiveTouch = NO;
    if (bridge.gesture && bridge.gesture.enabled && bridge.gesture.delegate == bridge) return YES;
    [bridge unbind];
    UIGestureRecognizer *gesture = AWEPrimaryHomeActionTapGesture(source);
    if ([gesture isKindOfClass:UITapGestureRecognizer.class] &&
        ((UITapGestureRecognizer *)gesture).numberOfTapsRequired != 1) gesture = nil;
    UIView *host = gesture.view;
    if (!host) return YES; // original UIControl path
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
static void ALBPlaceArtworkButton(UIButton *button, UIImageView *source, UIView *coordinates, CGRect frame) {
    UIView *parent = source.superview;
    if (!button || !parent || !coordinates || ALBIsOwnedView(source)) return;
    CGRect local = ALB19RightConvertRect(coordinates,frame,parent,coordinates);
    if (button.superview != parent) [parent insertSubview:button aboveSubview:source];
    else if ([parent.subviews indexOfObject:button] != [parent.subviews indexOfObject:source] + 1)
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
}

static void ALBInstallNativeArtwork(UIButton *button, UIImageView *source, CGFloat side, NSString *role) {
    if (!button || !source.image) { ALBReleaseArtwork(button); return; }
    ALBArtworkLease *lease = objc_getAssociatedObject(button, &kALBArtworkLeaseKey);
    if (lease.source != source) {
        [lease restore];
        lease = [ALBArtworkLease new];
        lease.source = source;
        lease.originalOpacity = source.layer.opacity;
        objc_setAssociatedObject(button, &kALBArtworkLeaseKey, lease, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSDictionary *cache = objc_getAssociatedObject(button, &kALBArtworkCacheKey);
    UIImage *image = cache[@"image"];
    if (cache[@"source"] != source.image || [cache[@"side"] doubleValue] != side) {
        UIImage *original = source.image;
        UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(side, side)];
        image = [[renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
            [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(0, 0, side, side)] addClip];
            CGFloat scale = MAX(side / MAX(1, original.size.width), side / MAX(1, original.size.height));
            CGSize size = CGSizeMake(original.size.width * scale, original.size.height * scale);
            [original drawInRect:CGRectMake((side-size.width)*0.5, (side-size.height)*0.5, size.width, size.height)];
        }] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
        objc_setAssociatedObject(button, &kALBArtworkCacheKey,
            @{@"source":source.image, @"side":@(side), @"image":image}, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSString *identity = [NSString stringWithFormat:@"%@.%p", role, (__bridge void *)image];
    AWEConfigureAdaptiveSystemGlassIconButton(button, image, identity, side, nil, YES, AWEHomeTintedGlassEnabled(), NO);
    // The UIImage is configuration-owned, so UIKit's lens also deforms it.
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
