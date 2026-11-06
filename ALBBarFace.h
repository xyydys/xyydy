/* 3.11.7: the right column's (3.11.9: and the double-tap panel's) glass is
   a real UIKit bar item, the same kind as the top chrome's side
   buttons (ALB3BarGlassButton, ALB3Chrome.swift: a one-item navigation bar
   scaled so its item's glass is exactly the view), which the user found
   perfect: its glass, its press, its ink turning dark on light glass, its
   menu growing out of it. 3.11.4-3.11.6 put the bar's glass object into the
   glass UIButton instead and it did not look or behave like the top.

   Each right-column glass UIButton keeps its place and its jobs (geometry,
   Douyin's gestures through its ancestry, the physical touches handed to
   Douyin's own button, the native action bridge) but draws nothing of its
   own: a bar item ("face") fills it, on top, and everything else in the
   button is hidden. The face is registered with an edge of its own (an empty
   scroll view right under it, ALBFollowSyncEdge), so UIKit turns each one
   light or dark by what is behind it, as the top chrome's.

   Touches (3.11.11): the face's control takes them (UIKit's own press) and
   hands every phase to the glass button under it (its own touch methods:
   Douyin's button, the action bridge, the avatar's menu, the panel's
   actions); see ALBFaceTouchAdopt. */


static char kALBFaceKey, kALBFaceSpecKey, kALBFaceHiddenKey, kALBFaceAppliedKey;

static BOOL ALBBarFacesWanted(void) {
    return ALB25MaterialFollowBar() && ALB3ChromeAvailable() && NSClassFromString(@"ALB3BarGlassButton") != nil;
}
static ALB3BarGlassButton *ALBFaceFor(UIView *button) {
    return button ? objc_getAssociatedObject(button, &kALBFaceKey) : nil;
}
static BOOL ALBFaceActive(UIView *button) {
    ALB3BarGlassButton *face = ALBFaceFor(button);
    return face && face.superview == button;
}
static UIImage *ALBFaceBlankImage(void) {
    static UIImage *image;
    if (!image) {
        UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat preferredFormat];
        format.opaque = NO;
        image = [[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(1, 1) format:format]
            imageWithActions:^(__unused UIGraphicsImageRendererContext *context) {}];
    }
    return image;
}

/* What the face shows, recorded where the button is dressed
   (AWEConfigureAdaptiveSystemGlassIconButton / …PlateButton). */
static void ALBFaceNoteIcon(UIButton *button, UIImage *image, CGFloat pointSize, UIColor *foreground, BOOL preserve, NSString *signature) {
    if (!button) return;
    // The dressing's own signature names the picture: the face draws it again
    // only when that changes (a new UIImage object on every pass would make
    // it re-render and re-measure its bar every pass).
    if ([objc_getAssociatedObject(button, &kALBFaceSpecKey)[@"sig"] isEqualToString:signature]) return;
    objc_setAssociatedObject(button, &kALBFaceSpecKey,
        @{@"image": image ?: (id)NSNull.null, @"point": @(pointSize), @"fg": foreground ?: (id)NSNull.null,
          @"preserve": @(preserve), @"sig": signature ?: @""},
        OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBFaceNotePlate(UIButton *button) {
    if (button) objc_setAssociatedObject(button, &kALBFaceSpecKey, @{@"plate": @YES, @"sig": @"plate"}, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
/* 3.11.9 (the double-tap panel): any spec, its signature the caller's. Keys:
   image / fg / preserve / point / fixedPoint (point as given, not the
   bitmap's) / regular / alpha / fill / inset, or title / align. */
static void ALBFaceNoteSpec(UIButton *button, NSDictionary *spec) {
    if (!button || !spec) return;
    if ([objc_getAssociatedObject(button, &kALBFaceSpecKey)[@"sig"] isEqualToString:spec[@"sig"]]) return;
    objc_setAssociatedObject(button, &kALBFaceSpecKey, spec, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBFaceApplySpec(ALB3BarGlassButton *face, NSDictionary *spec) {
    NSString *sig = spec[@"sig"];
    if (sig.length && [objc_getAssociatedObject(face, &kALBFaceAppliedKey) isEqualToString:sig]) return;
    objc_setAssociatedObject(face, &kALBFaceAppliedKey, sig, OBJC_ASSOCIATION_COPY_NONATOMIC);
    UIColor *tint = [spec[@"fg"] isKindOfClass:UIColor.class] ? spec[@"fg"] : nil;
    // A text capsule (a panel row's pill): the bar item's own title glass.
    NSString *title = [spec[@"title"] isKindOfClass:NSString.class] ? spec[@"title"] : nil;
    if (title) {
        if (face.contentAlignment != [spec[@"align"] integerValue]) face.contentAlignment = [spec[@"align"] integerValue];
        [face setTitle:title tint:tint];
        return;
    }
    if (face.regularWeight != [spec[@"regular"] boolValue]) face.regularWeight = [spec[@"regular"] boolValue];
    CGFloat alpha = spec[@"alpha"] ? [spec[@"alpha"] doubleValue] : 1.0;
    if (fabs(face.iconAlpha - alpha) > 0.001) face.iconAlpha = alpha;
    UIColor *fill = [spec[@"fill"] isKindOfClass:UIColor.class] ? spec[@"fill"] : nil;
    if (face.fillColor != fill && ![face.fillColor isEqual:fill]) face.fillColor = fill;
    if (spec[@"inset"] && fabs(face.fillInset - [spec[@"inset"] doubleValue]) > 0.01) face.fillInset = [spec[@"inset"] doubleValue];
    UIImage *image = [spec[@"image"] isKindOfClass:UIImage.class] ? spec[@"image"] : nil;
    if ([spec[@"plate"] boolValue] || !image) {
        if (face.pointSize != 1) face.pointSize = 1;
        [face setOriginalImage:ALBFaceBlankImage()];
        return;
    }
    BOOL preserve = [spec[@"preserve"] boolValue];
    UIColor *foreground = tint;
    // A bitmap is drawn at its own size, as the glass button drew it; a
    // symbol at the column's point size (the panel: always its own size).
    CGFloat point = image.isSymbolImage || [spec[@"fixedPoint"] boolValue] ? [spec[@"point"] doubleValue] : MAX(image.size.width, image.size.height);
    if (!isfinite(point) || point < 1) point = 24;
    if (fabs(face.pointSize - point) > 0.01) face.pointSize = point;
    // A full-colour picture (artwork on its canvas, a liked state in colour)
    // as it is; a glyph as a template: UIKit's ink, or the user's colour.
    if (preserve) [face setOriginalImage:image];
    else [face setImage:image artwork:NO tint:foreground];
}

static BOOL ALBFaceTracking(UIView *button) { (void)button; return NO; }

/* The item's control (UIKit's _UIButtonBarButton). */
static UIControl *ALBFaceControl(ALB3BarGlassButton *face) {
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:face];
    for (NSUInteger i = 0; i < queue.count && i < 80; i++) {
        UIView *view = queue[i];
        if (view != face && [view isKindOfClass:UIControl.class] && !CGRectIsEmpty(view.bounds)) return (UIControl *)view;
        [queue addObjectsFromArray:view.subviews];
    }
    return nil;
}
/* 3.11.11: the item's own press (UIKit's stretch and squash of the glass
   toward the finger). That press is UIKit's, on the item's control and the
   platters around it, so the touch lands in the control: the face takes
   touches, and its control (one instance, given a subclass of its own class
   at run time, as key-value observing does) hands every touch phase, as it
   gets it, to the glass button's own touch methods (Douyin's button, the
   action bridge, the avatar's menu, the panel's actions). Recognizers on the
   button and above it still see the touch (they are its ancestors), and the
   control never holds them back.

   3.11.19, the double-tap panel ("修复双击面板无法按压生效"): its buttons are
   plain UIButtons whose action is their own TouchUpInside, which a UIControl
   only sends for a touch it tracked itself; a handed-on touch is not one for
   certain. A panel button (ALBFaceTapOnTouchUp) therefore gets its action
   from the touch itself: the touch ended (not cancelled) on the button and
   the button sent no TouchUpInside of its own for it → the plug-in sends it,
   once, right after UIKit is done with that touch. The right column's
   buttons hand their touches to Douyin as before. */
static char kALBFaceTouchClassKey, kALBFaceTouchLiveKey, kALBFaceTapKey;
static NSUInteger gALBFaceTouchBegan, gALBFaceTouchEnded, gALBFaceTouchCancelled, gALBFaceTouchAdopted, gALBFaceTapSent;
// The panel's share of them (诊断).
static NSUInteger gALBFacePanelBegan, gALBFacePanelEnded, gALBFacePanelCancelled;
/* A panel button's taps: when its touch began, when it last sent its own
   TouchUpInside (a target of its own, recording only). */
@interface ALBFaceTapState : NSObject
@property(nonatomic) CFTimeInterval began, sent;
- (void)ownerSent;
@end
@implementation ALBFaceTapState
- (void)ownerSent { self.sent = CACurrentMediaTime(); }
@end
static void ALBFaceTapOnTouchUp(UIButton *button) {
    if (!button || objc_getAssociatedObject(button, &kALBFaceTapKey)) return;
    ALBFaceTapState *state = [ALBFaceTapState new];
    objc_setAssociatedObject(button, &kALBFaceTapKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [button addTarget:state action:@selector(ownerSent) forControlEvents:UIControlEventTouchUpInside];
}
static Class ALBFaceTouchSubclassOf(id object) {
    for (Class cls = object_getClass(object); cls; cls = class_getSuperclass(cls))
        if (objc_getAssociatedObject(cls, &kALBFaceTouchClassKey)) return cls;
    return Nil;
}
static Class ALBFaceTouchBase(id object) {
    Class sub = ALBFaceTouchSubclassOf(object);
    return sub ? class_getSuperclass(sub) : Nil;
}
static UIButton *ALBFaceTouchOwner(UIView *control) {
    static Class faceClass;
    if (!faceClass) faceClass = NSClassFromString(@"ALB3BarGlassButton");
    if (!faceClass) return nil;
    for (UIView *view = control.superview; view; view = view.superview) {
        if (![view isKindOfClass:faceClass]) continue;
        UIView *owner = view.superview;
        return [owner isKindOfClass:UIButton.class] && (UIView *)ALBFaceFor(owner) == view ? (UIButton *)owner : nil;
    }
    return nil;
}
static void ALBFaceTouchSuper(id self, SEL cmd, NSSet *touches, UIEvent *event) {
    Class base = ALBFaceTouchBase(self);
    if (!base) return;
    struct objc_super target = { .receiver = self, .super_class = base };
    ((void (*)(struct objc_super *, SEL, NSSet *, UIEvent *))objc_msgSendSuper)(&target, cmd, touches, event);
}
/* The button a touch began with (weak: it may go while the touch is down). */
@interface ALBFaceTouchOwnerBox : NSObject
@property(nonatomic, weak) UIButton *owner;
@end
@implementation ALBFaceTouchOwnerBox
@end
static void ALBFaceTouchesBegan(id self, SEL cmd, NSSet *touches, UIEvent *event) {
    ALBFaceTouchSuper(self, cmd, touches, event);
    UIButton *owner = ALBFaceTouchOwner(self);
    if (!owner) return;
    gALBFaceTouchBegan++;
    ALBFaceTapState *tap = objc_getAssociatedObject(owner, &kALBFaceTapKey);
    if (tap) gALBFacePanelBegan++;
    tap.began = CACurrentMediaTime();
    ALBFaceTouchOwnerBox *box = [ALBFaceTouchOwnerBox new];
    box.owner = owner;
    objc_setAssociatedObject(self, &kALBFaceTouchLiveKey, box, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [owner touchesBegan:touches withEvent:event];
}
static UIButton *ALBFaceTouchLiveOwner(id self, BOOL finish) {
    ALBFaceTouchOwnerBox *box = objc_getAssociatedObject(self, &kALBFaceTouchLiveKey);
    if (!box) return nil;
    if (finish) objc_setAssociatedObject(self, &kALBFaceTouchLiveKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // The owner it began with, only while it still wears this face.
    UIButton *owner = box.owner;
    return owner && ALBFaceTouchOwner(self) == owner ? owner : nil;
}
static void ALBFaceTouchesMoved(id self, SEL cmd, NSSet *touches, UIEvent *event) {
    ALBFaceTouchSuper(self, cmd, touches, event);
    [ALBFaceTouchLiveOwner(self, NO) touchesMoved:touches withEvent:event];
}
static void ALBFaceTouchesEnded(id self, SEL cmd, NSSet *touches, UIEvent *event) {
    ALBFaceTouchSuper(self, cmd, touches, event);
    UIButton *owner = ALBFaceTouchLiveOwner(self, YES);
    if (!owner) return;
    gALBFaceTouchEnded++;
    [owner touchesEnded:touches withEvent:event];
    // A panel button: the tap ended on it (UIKit's own slop for a button,
    // about 16 pt around it) and it sent nothing itself → its action, once,
    // after UIKit has finished with this touch.
    ALBFaceTapState *tap = objc_getAssociatedObject(owner, &kALBFaceTapKey);
    UITouch *touch = touches.anyObject;
    if (tap) gALBFacePanelEnded++;
    if (!tap || tap.began <= 0 || !touch) return;
    CFTimeInterval began = tap.began;
    tap.began = 0;
    if (!CGRectContainsPoint(CGRectInset(owner.bounds, -16.0, -16.0), [touch locationInView:owner])) return;
    __weak UIButton *weakOwner = owner;
    dispatch_async(dispatch_get_main_queue(), ^{
        UIButton *live = weakOwner;
        if (!live.window || !live.enabled || !live.userInteractionEnabled || tap.sent >= began) return;
        gALBFaceTapSent++;
        [live sendActionsForControlEvents:UIControlEventTouchUpInside];
    });
}
static void ALBFaceTouchesCancelled(id self, SEL cmd, NSSet *touches, UIEvent *event) {
    ALBFaceTouchSuper(self, cmd, touches, event);
    UIButton *owner = ALBFaceTouchLiveOwner(self, YES);
    if (!owner) return;
    gALBFaceTouchCancelled++;
    ALBFaceTapState *tap = objc_getAssociatedObject(owner, &kALBFaceTapKey);
    if (tap) gALBFacePanelCancelled++;
    tap.began = 0;
    [owner touchesCancelled:touches withEvent:event];
}
/* Recognizers of the button and above it (Douyin's, the bridge's, the
   menu's): never held back by this control; its own: as UIKit says. */
static BOOL ALBFaceTouchShouldBegin(id self, SEL cmd, UIGestureRecognizer *gesture) {
    UIView *view = gesture.view;
    if (view && view != self && ![view isDescendantOfView:(UIView *)self]) return YES;
    Class base = ALBFaceTouchBase(self);
    if (!base) return YES;
    struct objc_super target = { .receiver = self, .super_class = base };
    return ((BOOL (*)(struct objc_super *, SEL, UIGestureRecognizer *))objc_msgSendSuper)(&target, cmd, gesture);
}
static Class ALBFaceTouchClass(id self, SEL cmd) {
    (void)cmd;
    return ALBFaceTouchBase(self) ?: object_getClass(self);
}
static void ALBFaceTouchAdopt(UIControl *control) {
    if (!control || ALBFaceTouchSubclassOf(control)) return;
    Class base = object_getClass(control);
    NSString *baseName = NSStringFromClass(base);
    if (!baseName.length || [baseName hasPrefix:@"NSKVONotifying_"]) return;
    NSString *name = [@"ALBFaceTouch_" stringByAppendingString:baseName];
    Class sub = NSClassFromString(name);
    if (!sub) {
        sub = objc_allocateClassPair(base, name.UTF8String, 0);
        if (!sub) return;
        class_addMethod(sub, @selector(touchesBegan:withEvent:), (IMP)ALBFaceTouchesBegan, "v@:@@");
        class_addMethod(sub, @selector(touchesMoved:withEvent:), (IMP)ALBFaceTouchesMoved, "v@:@@");
        class_addMethod(sub, @selector(touchesEnded:withEvent:), (IMP)ALBFaceTouchesEnded, "v@:@@");
        class_addMethod(sub, @selector(touchesCancelled:withEvent:), (IMP)ALBFaceTouchesCancelled, "v@:@@");
        class_addMethod(sub, @selector(gestureRecognizerShouldBegin:), (IMP)ALBFaceTouchShouldBegin, "B@:@");
        class_addMethod(sub, @selector(class), (IMP)ALBFaceTouchClass, "#@:");
        objc_registerClassPair(sub);
        objc_setAssociatedObject(sub, &kALBFaceTouchClassKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    } else if (!objc_getAssociatedObject(sub, &kALBFaceTouchClassKey)) {
        return;     // a class of that name that is not ours
    }
    object_setClass(control, sub);
    gALBFaceTouchAdopted++;
}

/* 3.11.11: the item presses itself while its control has the touch; the
   face itself never moves. */
static void ALBFaceSetPressed(UIButton *button, BOOL pressed) {
    (void)pressed;
    ALB3BarGlassButton *face = ALBFaceFor(button);
    if (!face || face.superview != button) return;
    if (!CGAffineTransformIsIdentity(face.transform)) face.transform = CGAffineTransformIdentity;
}

/* The button's own drawing hidden while it wears a face (UIKit may show its
   parts again on a configuration update; the next layout hides them again),
   given back without one. */
static void ALBFaceHideButtonParts(UIButton *button, BOOL hide) {
    NSHashTable<UIView *> *hidden = objc_getAssociatedObject(button, &kALBFaceHiddenKey);
    if (!hide) {
        for (UIView *view in hidden.allObjects) if (view.superview == button) view.hidden = NO;
        [hidden removeAllObjects];
        return;
    }
    if (!hidden) {
        hidden = [NSHashTable weakObjectsHashTable];
        objc_setAssociatedObject(button, &kALBFaceHiddenKey, hidden, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    ALB3BarGlassButton *face = ALBFaceFor(button);
    UIView *edge = face ? objc_getAssociatedObject(face, &kALBFollowProxyKey) : nil;
    for (UIView *view in button.subviews) {
        if (view == face || view == edge || view.hidden) continue;
        view.hidden = YES;
        [hidden addObject:view];
    }
}
/* Light/dark of the face: Dark (like the liquid bar's page) on its own Dark
   edge, and UIKit turns it, as the top chrome's. */
static void ALBFaceSyncStyle(UIButton *button) {
    ALB3BarGlassButton *face = ALBFaceFor(button);
    if (face && face.overrideUserInterfaceStyle != UIUserInterfaceStyleDark)
        face.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
}
/* 3.11.8: no menu on the face (the button keeps its menu and its actions).
   3.11.11: the face takes touches, its item has an empty action (a live
   button, which presses right) and its control hands every touch to the
   button (ALBFaceTouchAdopt). */
static void ALBFaceSyncActions(UIButton *button, ALB3BarGlassButton *face) {
    (void)button;
    if (face.menu) face.menu = nil;
    if (!face.primaryHandler) face.primaryHandler = ^{};
    ALBFaceTouchAdopt(ALBFaceControl(face));
    if (!face.userInteractionEnabled) face.userInteractionEnabled = YES;
}
/* Every layout of a faced button: the face fills it on top, the rest hidden,
   its edge under it, its menu the button's. */
static void ALBFaceLayout(UIButton *button) {
    ALB3BarGlassButton *face = ALBFaceFor(button);
    if (!face) return;
    if (face.superview != button) { ALBFaceHideButtonParts(button, NO); return; }
    if (button.subviews.lastObject != face) [button bringSubviewToFront:face];
    if (!CGRectEqualToRect(face.frame, button.bounds)) face.frame = button.bounds;
    ALBFaceHideButtonParts(button, YES);
    ALBFaceSyncActions(button, face);
    // 3.11.10: every face on its own Dark edge (a panel button under a face
    // is never on an edge itself, ALBPaletteMarkEdge).
    ALBFollowSyncEdge(face);
}
/* Puts on / takes off the face as the options say; the button's dressing
   calls it. */
static void ALBFaceSync(UIButton *button) {
    if (!button || !NSThread.isMainThread) return;
    ALB3BarGlassButton *face = ALBFaceFor(button);
    BOOL want = ALBBarFacesWanted() && button.superview && AWEPluginFeaturesEnabled();
    if (!want) {
        if (!face) return;
        [face removeFromSuperview];
        ALBFollowSyncEdge(face);            // no superview: its edge and registration go
        ALBFaceHideButtonParts(button, NO);
        face.transform = CGAffineTransformIdentity;
        objc_setAssociatedObject(button, &kALBFaceKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [button setNeedsLayout];
        return;
    }
    if (!face) {
        face = [[NSClassFromString(@"ALB3BarGlassButton") alloc] initWithFrame:button.bounds];
        face.accessibilityIdentifier = @"AwemeLiquidBar.BarFace";
        face.accessibilityTitle = button.accessibilityLabel;
        objc_setAssociatedObject(button, &kALBFaceKey, face, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (face.superview != button) [button addSubview:face];
    // (No interaction flag copied: the button itself gates its touches, and a
    // new button only becomes interactive when its action is bound.)
    ALBFaceApplySpec(face, objc_getAssociatedObject(button, &kALBFaceSpecKey));
    ALBFaceSyncStyle(button);
    ALBFaceLayout(button);
}
/* 3.11.17: where a touch lands in a face (the right column, the panel),
   right before UIKit hands it the touch: the item's control (not one of its
   inner views) is the one touched, and it hands the touch on to its glass
   button (ALBFaceTouchAdopt), whenever UIKit built that control. 3.11.11-
   3.11.16 handed that over only at a layout of the button after the face
   had built its control; a control UIKit built (or rebuilt) after that took
   the touch and kept it (user, 3.11.16: the panel's taps did nothing, the
   right column's now and then). */
static UIView *ALBFaceAdoptHit(UIView *hit) {
    if (!hit) return hit;
    static Class faceClass;
    if (!faceClass) faceClass = NSClassFromString(@"ALB3BarGlassButton");
    if (!faceClass) return hit;
    ALB3BarGlassButton *face = nil;
    for (UIView *view = hit; view; view = view.superview) {
        if ([view isKindOfClass:faceClass]) { face = (ALB3BarGlassButton *)view; break; }
    }
    if (!face || ![face.superview isKindOfClass:UIButton.class] || (UIView *)ALBFaceFor(face.superview) != face) return hit;
    UIControl *control = ALBFaceControl(face);
    if (!control || !(hit == control || [hit isDescendantOfView:control])) return hit;
    ALBFaceTouchAdopt(control);
    return control;
}
/* 3.11.8: the button takes its touches as before (the face takes none);
   3.11.17: a touch in its face, the face's control (ALBFaceAdoptHit). */
static UIView *ALBFaceFilterHit(UIButton *button, UIView *hit) { (void)button; return ALBFaceAdoptHit(hit); }
