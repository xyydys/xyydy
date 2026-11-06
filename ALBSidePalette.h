#import <MediaPlayer/MediaPlayer.h>
#import <ImageIO/ImageIO.h>
#include "ALBAlbumVideoExporter.h"

static BOOL ALBPaletteIsSlider(NSInteger action) { return action==7 || action==8; }
static UIColor *ALBPaletteTextInk(UIColor *automatic) {
    return ALB5Bool(@"panel.customTextColor",NO)
        ?ALB5Color(@"panel.textColor",UIColor.whiteColor):automatic;
}

/* 3.11: every icon and pill draws the collapsing dock's glass (the right
   column's, ALB25MaterialMotionButtonConfiguration): the glass button's own
   plain glass, never registered, the panel fixed Dark like the dock's video
   glass, ink white (the panel's custom text colour still wins). The panel
   lives in Douyin's window again: no bar window, no edges of its own, no
   pictures of the screen while it is up. Native UIButton owns real touches. */
/* 3.11.19: the bar's own glass (iOS 26), Dark; UIKit turns each glass on
   its own edge (ALBFollowSyncEdge / the bar item faces, ALBBarFace.h). */
static UIButtonConfiguration *ALBPaletteGlassConfiguration(void) {
    UIButtonConfiguration *cfg = ALB25MaterialMotionButtonConfigurationTone(NO);
    cfg.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
    cfg.contentInsets = NSDirectionalEdgeInsetsMake(0, 10, 0, 10);
    cfg.baseBackgroundColor = nil;
    cfg.baseForegroundColor = ALBPaletteTextInk(UIColor.labelColor);
    cfg.titleLineBreakMode = NSLineBreakByClipping;
    cfg.titleTextAttributesTransformer = ^NSDictionary *(NSDictionary *attributes) {
        NSMutableDictionary *result = [attributes mutableCopy];
        result[NSFontAttributeName] = [UIFont systemFontOfSize:12.5 weight:UIFontWeightMedium];
        return result;
    };
    return cfg;
}

/* Clear UIButton still owns its real touch/press effect. UIKit's configuration
   image view can have an inset or non-zero origin; using its old bounds for a
   full-button canvas shifts every fill toward the upper left. Align that one
   canvas to the button at rest, without resetting its native press transform. */
static UIImageView *ALBPaletteAlignedImageCanvas(UIButton *button) {
    UIImageView *host = button.imageView;
    if (!host || !host.superview || CGRectIsEmpty(button.bounds)) return nil;
    if (!button.isTracking && !button.isHighlighted) {
        CGPoint center = [button convertPoint:CGPointMake(CGRectGetMidX(button.bounds), CGRectGetMidY(button.bounds))
                                       toView:host.superview];
        CGRect bounds = (CGRect){CGPointZero, button.bounds.size};
        if (!CGRectEqualToRect(host.bounds, bounds)) host.bounds = bounds;
        if (!CGPointEqualToPoint(host.center, center)) host.center = center;
    }
    host.contentMode = UIViewContentModeScaleToFill;
    host.clipsToBounds = NO;
    return host;
}

@interface ALB5PaletteIcon : UIButton
@property(nonatomic,copy) NSString *roleKey;
@property(nonatomic,strong) UIImage *roleArtwork;
@property(nonatomic,strong) UIColor *automaticInk;
@property(nonatomic,strong) UIImageView *glyphView;
@property(nonatomic,strong) UIView *tintFill;
@property(nonatomic) BOOL originalArtwork;
@property(nonatomic) CGSize canvasSize;
- (void)refreshArtwork;
- (void)layoutArtwork;
@end
/* 3.11.9: the panel draws the right column's glass, a system bar item face
   on each icon and text pill (ALBBarFace.h), each on its own Dark edge and
   turned by UIKit (3.11.10). The two sliders keep the glass button (their
   fill and labels live inside it), on their own edges too. */
@class ALB5PaletteIcon;
static void ALBPaletteFaceIcon(ALB5PaletteIcon *icon);
@implementation ALB5PaletteIcon
- (void)setHighlighted:(BOOL)highlighted { [super setHighlighted:highlighted]; ALBFaceSetPressed(self, highlighted); }
- (void)refreshArtwork {
    if (CGRectIsEmpty(self.bounds) || !self.roleArtwork) return;
    UIColor *ink=AWEApplyConfiguredIconColorMode(self.roleKey,self.automaticInk?:UIColor.labelColor);
    CGFloat point=ALB5Number([self.roleKey stringByAppendingString:@".size"],18,10,34);
    UIImage *glyph=self.roleArtwork;
    if (!self.originalArtwork)
        glyph=[[glyph imageWithConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:point weight:UIImageSymbolWeightRegular]] imageWithTintColor:ink renderingMode:UIImageRenderingModeAlwaysOriginal];
    if (!self.glyphView) {
        self.glyphView=[UIImageView new]; self.glyphView.contentMode=UIViewContentModeScaleAspectFit;
        self.glyphView.userInteractionEnabled=NO;
        self.tintFill=[UIView new]; self.tintFill.userInteractionEnabled=NO;
        self.tintFill.layer.cornerCurve=kCACornerCurveContinuous;
    }
    self.glyphView.image=glyph;
    self.glyphView.alpha=ALB5Number([self.roleKey stringByAppendingString:@".alpha"],1,.1,1);
    self.tintFill.backgroundColor=[ink colorWithAlphaComponent:.12];
    self.tintFill.hidden=!ALB5Bool(@"panel.iconFill",NO);
    [self.glyphView.layer removeAnimationForKey:@"ALB5.breathe"];
    NSInteger breathing=(NSInteger)ALB5Number(@"panel.breathe",0,0,2);
    if (breathing && !UIAccessibilityIsReduceMotionEnabled()) {
        CABasicAnimation *scale=[CABasicAnimation animationWithKeyPath:@"transform.scale"]; scale.fromValue=@.94; scale.toValue=@1;
        CABasicAnimation *lift=[CABasicAnimation animationWithKeyPath:@"transform.translation.y"];
        lift.fromValue=@0; lift.toValue=@(-MIN(2,ALB5Number(@"panel.inset",3,1,6)*.5));
        CAAnimationGroup *group=[CAAnimationGroup animation]; group.animations=breathing==2?@[scale,lift]:@[scale];
        group.duration=ALB5Number(@"panel.breathePeriod",2.6,1,8)/2; group.autoreverses=YES; group.repeatCount=HUGE_VALF;
        group.timingFunction=[CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
        // Only the glyph breathes. Animating the native image host also moved
        // the inset fill upward, making its top/bottom margins unequal at rest.
        [self.glyphView.layer addAnimation:group forKey:@"ALB5.breathe"];
    }
    [self layoutArtwork];
    ALBPaletteFaceIcon(self);
}
- (void)layoutArtwork {
    UIImageView *host=ALBPaletteAlignedImageCanvas(self);
    if (!host || !self.glyphView) return;
    if (self.tintFill.superview!=host) [host addSubview:self.tintFill];
    if (self.glyphView.superview!=host) [host addSubview:self.glyphView];
    CGRect canvas=host.bounds;
    CGFloat inset=MIN(ALB5Number(@"panel.inset",3,1,6),MIN(canvas.size.width,canvas.size.height)*.5);
    CGFloat side=self.originalArtwork?MIN(canvas.size.width,canvas.size.height)-8:ALB5Number([self.roleKey stringByAppendingString:@".size"],18,10,34);
    side=MAX(0,MIN(side,MIN(canvas.size.width,canvas.size.height)-2*inset));
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    self.tintFill.frame=CGRectInset(canvas,inset,inset);
    self.tintFill.layer.cornerRadius=MIN(self.tintFill.bounds.size.width,self.tintFill.bounds.size.height)/2;
    self.glyphView.bounds=CGRectMake(0,0,side,side);
    self.glyphView.center=CGPointMake(CGRectGetMidX(canvas),CGRectGetMidY(canvas));
    [CATransaction commit];
}
- (void)layoutSubviews {
    [super layoutSubviews];
    if (!CGSizeEqualToSize(self.canvasSize,self.bounds.size) && !CGRectIsEmpty(self.bounds)) {
        self.canvasSize=self.bounds.size;
        UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:self.bounds.size];
        UIButtonConfiguration *cfg=[self.configuration copy];
        cfg.image=[[renderer imageWithActions:^(__unused UIGraphicsImageRendererContext *c){}] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
        self.configuration=cfg;
        [super layoutSubviews];
        [self refreshArtwork];
    }
    [self layoutArtwork];
    ALBFaceLayout(self);
}
@end
/* 3.11.10: UIKit deciding, a row's black/white ink of ours is no ink for a
   face: its glyph takes UIKit's own (white on dark glass, black once the
   glass turns light, as the right column's), unless the user picked a
   colour for it or it shows a state in colour (liked, favourited). */
static BOOL ALBPaletteInkIsPlain(UIColor *ink) {
    if (!ink) return YES;
    CGFloat white = 0, alpha = 0;
    if (![ink getWhite:&white alpha:&alpha]) return NO;
    return alpha > 0.99 && (white < 0.01 || white > 0.99);
}
/* 3.11.10: whether a panel button is on an edge itself: only a button
   wearing no face (a slider, or every button where bar item faces are not
   available); a face is on its own edge instead (ALBFaceLayout). Marked by
   the same rule wherever the panel touches a button, so no button is left
   on an edge it should not be on (3.11.9: the icons, 12 of 24). */
static void ALBPaletteMarkEdge(UIView *button, BOOL slider) {
    BOOL edge = ALBFollowSystemDecides() && (slider || !ALBBarFacesWanted());
    if (edge == (objc_getAssociatedObject(button, &kALBFollowNoEdgeKey) == nil)) return;
    objc_setAssociatedObject(button, &kALBFollowNoEdgeKey, edge ? nil : @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBPaletteFaceIcon(ALB5PaletteIcon *icon) {
    if (!icon) return;
    ALBPaletteMarkEdge(icon, NO);
    ALBFaceTapOnTouchUp(icon);
    if (ALBBarFacesWanted() && icon.roleArtwork && !CGRectIsEmpty(icon.bounds)) {
        UIColor *ink = AWEApplyConfiguredIconColorMode(icon.roleKey, icon.automaticInk ?: UIColor.labelColor);
        BOOL systemInk = ALBFollowSystemDecides() && AWEStoredSymbolColorMode(icon.roleKey) == AWEConfiguredIconColorModeAutomatic &&
                         !ALB5Bool(@"panel.customTextColor", NO) && ALBPaletteInkIsPlain(icon.automaticInk);
        CGFloat point = ALB5Number([icon.roleKey stringByAppendingString:@".size"], 18, 10, 34);
        CGFloat alpha = ALB5Number([icon.roleKey stringByAppendingString:@".alpha"], 1, .1, 1);
        BOOL fillOn = ALB5Bool(@"panel.iconFill", NO);
        CGFloat inset = ALB5Number(@"panel.inset", 3, 1, 6);
        CGFloat side = MIN(CGRectGetWidth(icon.bounds), CGRectGetHeight(icon.bounds));
        NSString *sig = [NSString stringWithFormat:@"icon|%p|%d|%@|%.1f|%.2f|%d|%.1f|%.1f", icon.roleArtwork, icon.originalArtwork,
                         systemInk ? @"SYSTEM" : (AWEHexStringFromColor(ink) ?: ink.description), point, alpha, fillOn, inset, side];
        UIColor *fill = systemInk ? [UIColor.labelColor colorWithAlphaComponent:.12] : [ink colorWithAlphaComponent:.12];
        NSDictionary *spec = icon.originalArtwork
            ? @{@"image": icon.roleArtwork, @"preserve": @YES, @"point": @(MAX(1.0, side - 8.0)), @"fixedPoint": @YES, @"sig": sig}
            : @{@"image": icon.roleArtwork, @"fg": systemInk ? (id)NSNull.null : (id)ink, @"point": @(point), @"fixedPoint": @YES,
                @"regular": @YES, @"alpha": @(alpha), @"fill": fillOn ? (id)fill : (id)NSNull.null, @"inset": @(inset), @"sig": sig};
        ALBFaceNoteSpec(icon, spec);
    }
    ALBFaceSync(icon);
}

@interface ALBPaletteSlider : UIButton
@property(nonatomic) CGSize canvasSize;
@property(nonatomic, strong) UIView *fillClip;
@property(nonatomic, strong) UIView *fill;
@property(nonatomic, strong) UILabel *nameLabel;
@property(nonatomic, strong) UILabel *valueLabel;
@property(nonatomic, strong) UISelectionFeedbackGenerator *feedback;
@property(nonatomic) CGFloat value;
@property(nonatomic) CGFloat beganValue;
@property(nonatomic) CGFloat beganX;
@property(nonatomic) NSInteger hapticPercent;
@property(nonatomic) BOOL dragging;
@property(nonatomic, copy) void (^valueChanged)(CGFloat);
- (instancetype)initWithName:(NSString *)name color:(UIColor *)color;
- (void)applyValue:(CGFloat)value haptic:(BOOL)haptic;
@end
@implementation ALBPaletteSlider
- (instancetype)initWithName:(NSString *)name color:(UIColor *)color {
    if ((self = [super initWithFrame:CGRectZero])) {
        self.backgroundColor = UIColor.clearColor;
        self.clipsToBounds = NO;
        UIButtonConfiguration *cfg=ALBPaletteGlassConfiguration(); cfg.contentInsets=NSDirectionalEdgeInsetsZero;
        self.configuration=cfg;
        self.fillClip = [UIView new];
        self.fillClip.userInteractionEnabled = NO;
        self.fillClip.clipsToBounds = YES;
        [self addSubview:self.fillClip];
        self.fill = [UIView new];
        UIColor *fillColor=ALB5Color([name isEqualToString:@"音量"]?@"panel.volumeColor":@"panel.brightnessColor",color);
        self.fill.backgroundColor = [fillColor colorWithAlphaComponent:ALB5Number(@"panel.fillAlpha",.26,.05,.65)];
        self.fill.userInteractionEnabled = NO;
        [self.fillClip addSubview:self.fill];
        self.nameLabel = [UILabel new]; self.nameLabel.text = name;
        self.nameLabel.font = [UIFont systemFontOfSize:12.5 weight:UIFontWeightMedium];
        self.nameLabel.textColor = ALBPaletteTextInk(UIColor.labelColor);
        [self addSubview:self.nameLabel];
        self.valueLabel = [UILabel new];
        self.valueLabel.textAlignment = NSTextAlignmentRight;
        self.valueLabel.font = [UIFont monospacedDigitSystemFontOfSize:11.5 weight:UIFontWeightMedium];
        self.valueLabel.textColor = ALBPaletteTextInk(color);
        [self addSubview:self.valueLabel];
        self.feedback = [UISelectionFeedbackGenerator new];
        self.isAccessibilityElement = YES;
        self.accessibilityLabel = name;
        self.accessibilityTraits = UIAccessibilityTraitAdjustable;
        self.accessibilityIdentifier = [@"AwemeLiquidBar.SidePalette.Slider." stringByAppendingString:name];
    }
    return self;
}
- (void)layoutSubviews {
    [super layoutSubviews];
    self.layer.cornerRadius = self.bounds.size.height/2;
    if(!CGSizeEqualToSize(self.canvasSize,self.bounds.size) && !CGRectIsEmpty(self.bounds)) {
        self.canvasSize=self.bounds.size;
        UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:self.bounds.size];
        UIButtonConfiguration *cfg=[self.configuration copy];cfg.image=[renderer imageWithActions:^(__unused UIGraphicsImageRendererContext *c){}];self.configuration=cfg;
        [super layoutSubviews];
    }
    // Put the complete fill and labels inside the native image content. UIKit's
    // press transform now carries the fill together with the visible content.
    UIImageView *host=ALBPaletteAlignedImageCanvas(self);
    UIView *parent=host ?: self;
    for (UIView *v in @[self.fillClip,self.nameLabel,self.valueLabel])
        if (v.superview!=parent) [parent addSubview:v];
    // Every element uses the same local canvas, including non-zero bounds
    // origins. Geometry is symmetric before UIKit deforms that whole canvas.
    CGRect canvas=parent.bounds;
    CGFloat inset=MIN(ALB5Number(@"panel.inset",3,1,6), MIN(canvas.size.width,canvas.size.height)*.5);
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    self.fillClip.frame=CGRectInset(canvas,inset,inset);
    self.fillClip.layer.cornerRadius=self.fillClip.bounds.size.height*.5;
    self.fill.frame=CGRectMake(0,0,self.fillClip.bounds.size.width*self.value,self.fillClip.bounds.size.height);
    self.fill.layer.cornerRadius=MIN(self.fill.bounds.size.width,self.fill.bounds.size.height)*.5;
    self.nameLabel.frame=CGRectMake(CGRectGetMinX(canvas)+10, CGRectGetMinY(canvas), MAX(0,canvas.size.width-68), canvas.size.height);
    self.valueLabel.frame=CGRectMake(CGRectGetMaxX(canvas)-53, CGRectGetMinY(canvas), 43, canvas.size.height);
    [CATransaction commit];
}
- (void)applyValue:(CGFloat)value haptic:(BOOL)haptic {
    self.value = MIN(1, MAX(0, value));
    NSInteger percent = lround(self.value * 100);
    if (haptic && percent != self.hapticPercent) {
        // One request per crossed 1% detent; UIKit/hardware may coalesce fast
        // movement. Never queue delayed vibrations after the finger lifts.
        NSInteger step = percent > self.hapticPercent ? 1 : -1;
        for (NSInteger p=self.hapticPercent; p!=percent; p+=step) [self.feedback selectionChanged];
        [self.feedback prepare];
    }
    self.hapticPercent = percent;
    self.valueLabel.text = [NSString stringWithFormat:@"%ld%%", (long)percent];
    self.accessibilityValue = self.valueLabel.text;
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    self.fill.frame = CGRectMake(0, 0, self.fillClip.bounds.size.width*self.value, self.fillClip.bounds.size.height);
    self.fill.layer.cornerRadius=MIN(self.fill.bounds.size.width,self.fill.bounds.size.height)*.5;
    [CATransaction commit];
}
- (BOOL)beginTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event {
    [super beginTrackingWithTouch:touch withEvent:event];
    self.dragging = YES; self.beganValue = self.value; self.beganX = [touch locationInView:self].x;
    [self.feedback prepare]; return YES;
}
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    return CGRectContainsPoint(CGRectInset(self.bounds, 0, -MAX(0, (44-self.bounds.size.height)/2)), point);
}
- (BOOL)continueTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event {
    [super continueTrackingWithTouch:touch withEvent:event];
    CGFloat x = [touch locationInView:self].x;
    CGFloat next = self.beganValue + (x-self.beganX)/MAX(1,self.bounds.size.width);
    [self applyValue:next haptic:YES];
    if (self.valueChanged) self.valueChanged(self.value);
    return YES;
}
- (void)endTrackingWithTouch:(UITouch *)touch withEvent:(UIEvent *)event { [super endTrackingWithTouch:touch withEvent:event]; self.dragging = NO; }
- (void)cancelTrackingWithEvent:(UIEvent *)event { [super cancelTrackingWithEvent:event]; self.dragging = NO; }
- (void)accessibilityIncrement { [self applyValue:self.value+0.01 haptic:YES]; if(self.valueChanged) self.valueChanged(self.value); }
- (void)accessibilityDecrement { [self applyValue:self.value-0.01 haptic:YES]; if(self.valueChanged) self.valueChanged(self.value); }
@end
/* A text pill's face: its title, its ink, its glass against the icon. */
static void ALBPaletteFacePill(UIView *view, BOOL rightSide) {
    if (![view isKindOfClass:UIButton.class] || [view isKindOfClass:ALBPaletteSlider.class]) return;
    UIButton *pill = (UIButton *)view;
    ALBPaletteMarkEdge(pill, NO);
    ALBFaceTapOnTouchUp(pill);
    NSString *title = pill.configuration.title;
    if (ALBBarFacesWanted() && title.length) {
        UIColor *ink = pill.configuration.baseForegroundColor ?: UIColor.labelColor;
        // 3.11.10: UIKit deciding, UIKit's own ink (the user's text colour still wins).
        BOOL systemInk = ALBFollowSystemDecides() && !ALB5Bool(@"panel.customTextColor", NO);
        NSString *sig = [NSString stringWithFormat:@"pill|%@|%@|%d", title,
                         systemInk ? @"SYSTEM" : (AWEHexStringFromColor(ink) ?: ink.description), rightSide];
        ALBFaceNoteSpec(pill, @{@"title": title, @"fg": systemInk ? (id)NSNull.null : (id)ink, @"align": @(rightSide ? 2 : 1), @"sig": sig});
    }
    ALBFaceSync(pill);
}

@interface ALBSidePalette : UIView <UIGestureRecognizerDelegate>
@property(nonatomic, weak) AWEPlayInteractionViewController *sourceController;
// 3.11.45: a touch that is not on one of the panel's glass buttons closes it; a swipe closes it and goes on to the feed.
@property(nonatomic, strong) ALBOutsidePress *outsidePress;   // 3.11.46：按下就关（ALBOutsidePress.h）
@property(nonatomic, strong) UITapGestureRecognizer *gapTap;
@property(nonatomic, weak) UIView *sourceStack;
@property(nonatomic, strong) id aweme;
@property(nonatomic, copy) NSString *identity;
@property(nonatomic, strong) UIScrollView *group;
@property(nonatomic, strong) NSArray<UIButton *> *icons;
@property(nonatomic, strong) NSArray<UIView *> *pills;
@property(nonatomic, strong) UIButton *dismissButton;
@property(nonatomic, strong) MPVolumeView *volumeView;
@property(nonatomic, weak) UISlider *systemVolumeSlider;
@property(nonatomic, strong) ALBPaletteSlider *volume;
@property(nonatomic, strong) ALBPaletteSlider *brightness;
@property(nonatomic) CGPoint anchorFraction;
@property(nonatomic) BOOL volumeObserved;
@property(nonatomic) BOOL dismissing;
@property(nonatomic) BOOL rootFeedSession;
@property(nonatomic) NSUInteger sourceGeneration;
@property(nonatomic,copy) NSString *materialSignature;
// The feed's window (the panel's own window; 3.10.2-3.10.12 it could be the bar's).
@property(nonatomic, weak) UIWindow *feedWindow;
- (void)beginRowContrast;
- (void)refreshPaletteMaterial;
- (void)refreshCurrentSource;
- (void)setSourceActionsEnabled:(BOOL)enabled;
- (void)retargetController:(AWEPlayInteractionViewController *)controller stack:(UIView *)stack;
- (void)feedItemDidCommit:(NSNotification *)notification;
- (void)dismiss;
- (void)actionTapped:(UIButton *)sender;
- (void)refreshLevels;
@end
static __weak ALBSidePalette *gALBSidePalette;
static char kALBVolumeObservationContext;
static NSUInteger gALBPaletteOutsideCloses, gALBPaletteSwipeCloses, gALBPaletteZoneOpens, gALBPaletteZoneComments;

/* The panel's ink and light/dark (3.11.10, the right column's rule): every
   icon, pill and slider glass Dark on its own Dark edge, and UIKit turns
   each one and hands its ink to what is inside (a slider's name: the label
   colour of where it is drawn). The user's own text colour still wins. */
static void ALBPaletteApplyInk(ALBSidePalette *palette) {
    if (!palette.window || !palette.pills.count) return;
    BOOL custom = ALB5Bool(@"panel.customTextColor", NO);
    BOOL follow = ALB25MaterialFollowBar();
    UIColor *ink = ALBPaletteTextInk(UIColor.whiteColor);
    for (NSUInteger row = 0; row < palette.pills.count && row < palette.icons.count; row++) {
        ALB5PaletteIcon *icon = (ALB5PaletteIcon *)palette.icons[row];
        NSInteger action = icon.tag;
        if (action != 1 && action != 7 && action != 8 &&
            ![icon.automaticInk isEqual:UIColor.systemPinkColor] &&
            ![icon.automaticInk isEqual:UIColor.systemYellowColor] && ![icon.automaticInk isEqual:ink]) {
            icon.automaticInk = ink; [icon refreshArtwork];
        }
        UIView *pill = palette.pills[row];
        if ([pill isKindOfClass:ALBPaletteSlider.class]) {
            ALBPaletteSlider *slider = (ALBPaletteSlider *)pill;
            UIColor *nameInk = follow ? ALBPaletteTextInk(UIColor.labelColor) : ink;
            if (![slider.nameLabel.textColor isEqual:nameInk]) slider.nameLabel.textColor = nameInk;
            if (custom && ![slider.valueLabel.textColor isEqual:ink]) slider.valueLabel.textColor = ink;
        } else if ([pill isKindOfClass:UIButton.class]) {
            UIButton *button = (UIButton *)pill;
            if (![button.configuration.baseForegroundColor isEqual:ink]) {
                UIButtonConfiguration *cfg = [button.configuration copy];
                cfg.baseForegroundColor = ink; button.configuration = cfg;
            }
        }
        if (!follow) continue;
        for (UIView *view in @[icon, pill]) {
            if (![view isKindOfClass:UIButton.class]) continue;
            UIButton *glass = (UIButton *)view;
            if (view == pill) ALBPaletteFacePill(pill, palette.anchorFraction.x > 0.5);
            if (ALBFaceActive(glass)) ALBFaceSyncStyle(glass);
            else if (glass.overrideUserInterfaceStyle != UIUserInterfaceStyleDark) glass.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
        }
    }
}

static UIView *ALBFindVisibleActionStack(UIView *root) {
    if (!root || !root.window) return nil;
    UIWindow *window=root.window;
    UIView *best=nil; CGFloat score=-CGFLOAT_MAX;
    NSMutableArray *queue=[NSMutableArray arrayWithObject:root];
    for (NSUInteger i=0;i<queue.count && i<900;i++) {
        UIView *v=queue[i];
        if (v.hidden || ALB7RightNativeAlpha(v)<.01 || [v.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."]) continue;
        if (AWESameStyleIsRightActionStackV23(v)) {
            CGRect rect=[v convertRect:v.bounds toView:window];
            CGRect visible=CGRectIntersection(rect,window.bounds);
            if (!CGRectIsNull(visible) && !CGRectIsEmpty(visible)) {
                CGFloat fraction=(visible.size.width*visible.size.height)/MAX(1,rect.size.width*rect.size.height);
                CGFloat rank=fraction*10000-fabs(CGRectGetMidY(rect)-CGRectGetMidY(window.bounds));
                if (rank>score) { score=rank; best=v; }
            }
            continue;
        }
        [queue addObjectsFromArray:v.subviews];
    }
    return best;
}

// Never let the global current-controller fallback join a recycled cell's
// stack to another cell's model. These paths stop at the stack's own ancestry.
static id ALB8PaletteModelForStack(UIView *stack) {
    if(!stack || !stack.window)return nil;
    SEL selector=NSSelectorFromString(@"awelistkit_cellContext");
    for(UIView *view=stack;view && view!=stack.window;view=view.superview) {
        if([view respondsToSelector:selector]) {
            @try {
                id context=((id(*)(id,SEL))objc_msgSend)(view,selector);
                id model=AWEFirstSafeValue(context,@[@"model",@"awemeModel",@"itemModel"]);
                if(model)return model;
            } @catch(__unused NSException *exception) {}
        }
        id model=AWEFirstSafeValue(view,@[@"awemeModel",@"itemModel"]);
        if(model)return model;
    }
    for(UIResponder *r=stack;r;r=r.nextResponder) {
        if(![r isKindOfClass:UIViewController.class])continue;
        id model=AWEFirstSafeValue(r,@[@"awemeModel",@"currentAwemeModel",@"currentAweme",@"itemModel"]);
        if(model)return model;
    }
    return nil;
}
static BOOL ALB8PaletteControllerOwnsStack(AWEPlayInteractionViewController *controller,UIView *stack,id model) {
    UIView *view=controller.viewIfLoaded;
    if(!view.window || view.window!=stack.window || ![stack isDescendantOfView:view])return NO;
    id controllerModel=AWEFirstSafeValue(controller,@[@"awemeModel",@"currentAwemeModel",@"currentAweme",@"itemModel"]);
    return !controllerModel || [AWEAwemeIdentity(controllerModel) isEqualToString:AWEAwemeIdentity(model)];
}
static BOOL ALB8PaletteLikeState(UIView *stack,BOOL *known) {
    if(known)*known=NO;
    NSArray<UIView *> *buttons=AWESortedVisibleHomeActionButtons(stack);
    for(UIView *button in buttons) {
        if(AWEHomeActionKindForButton(button,buttons)!=AWEHomeActionKindLike)continue;
        if(known)*known=YES;
        UIImageView *source=objc_getAssociatedObject(button,&kAWEHomeActionOriginalImageViewKey);
        return AWEHomeActionIsSelected(button,AWEHomeActionKindLike,source);
    }
    return NO;
}

// Discover only the current stack's actual follow entry. The existing exact
// native dispatcher reads its registered target; no guessed follow selector,
// model mutation or network request is introduced here.
static UIView *ALBPaletteFollowEntry(UIView *stack) {
    if(!stack.window)return nil;
    NSMutableArray<UIView *> *queue=[NSMutableArray arrayWithObject:stack];
    UIView *best=nil; NSInteger bestScore=0;
    for(NSUInteger i=0;i<queue.count && i<180;i++) {
        UIView *view=queue[i];
        if(ALBIsOwnedView(view) || view.hidden || ALB7RightNativeAlpha(view)<.01)continue;
        NSString *name=NSStringFromClass(view.class).lowercaseString;
        NSString *label=view.accessibilityLabel?:@"";
        if([view isKindOfClass:UIButton.class] && !label.length)
            label=[(UIButton *)view titleForState:UIControlStateNormal]?:@"";
        BOOL followed=[label containsString:@"已关注"] || [label containsString:@"取消关注"] ||
            [label containsString:@"互相关注"] || [name containsString:@"unfollow"] ||
            [name containsString:@"followed"] || [name containsString:@"following"];
        if([view isKindOfClass:UIControl.class] &&
           (!((UIControl *)view).enabled || ((UIControl *)view).selected))followed=YES;
        BOOL exact=[label isEqualToString:@"关注"] || [label isEqualToString:@"关注作者"] ||
            [label isEqualToString:@"+关注"] || [label isEqualToString:@"加关注"];
        BOOL named=[name containsString:@"follow"] &&
            ([name containsString:@"button"] || [name containsString:@"view"]);
        if(!followed && (exact || named)) {
            NSMutableArray *labels=[NSMutableArray arrayWithArray:view.subviews];
            for(NSUInteger j=0;j<labels.count && j<24;j++) {
                UIView *child=labels[j];
                if(ALBIsOwnedView(child))continue;
                NSString *text=[child isKindOfClass:UILabel.class]?((UILabel *)child).text:child.accessibilityLabel;
                if([text containsString:@"已关注"] || [text containsString:@"取消关注"] ||
                   [text containsString:@"互相关注"]){followed=YES;break;}
                [labels addObjectsFromArray:child.subviews];
            }
        }
        if(followed)continue;
        if(!followed && (exact || named)) {
            CGRect rect=[view convertRect:view.bounds toView:stack.window];
            if(CGRectIntersectsRect(rect,stack.window.bounds) && rect.size.width>8 &&
               rect.size.width<=120 && rect.size.height>8 && rect.size.height<=80) {
                NSInteger score=(exact?100:0)+([view isKindOfClass:UIControl.class]?20:0)+
                    (view.gestureRecognizers.count?10:0);
                if(score>bestScore){best=view;bestScore=score;}
            }
        }
        [queue addObjectsFromArray:view.subviews];
    }
    return best;
}

/* 3.11.18 (ALBGlassTone.h): the new panel's bar items start from the preset
   brightness instead of dark. */

/* 3.11.46：双击面板每一项的动作（双击面板、三段式底栏 ✦ 的悬浮面板共用） */
static void ALBPaletteRun(NSInteger index, UIView *stack, id model, AWEPlayInteractionViewController *controller, NSString *source) {
    switch (index) {
        case 0: {
            UIView *host=AWEFindViewByExactRuntimeClass(stack,@"AWEPlayInteractionUserProfilePictureView",0);
            if (!host) host=AWEFindViewByExactRuntimeClass(stack,@"AWEPlayInteractionUserProfileContainerView",0);
            if (!ALBForwardOriginalView(host)) AWEShowDownloadToast(@"未找到作者主页入口");
            break;
        }
        case 1:
            if (AWEAlbumImagesForAweme(model).count) AWEDownloadCurrentImageForAweme(model);
            else ALBMSSave(model, ALBMSWhatMain, source);
            break;
        case 2:
            if (AWEAlbumImagesForAweme(model).count) ALBExportAlbumVideo(model);
            else {
                id share=AWEFirstSafeValue(model,@[@"shareInfo",@"shareModel"]);
                NSString *url=AWEFirstRemoteURLString(AWEFirstSafeValue(share,@[@"shareURL",@"shareUrl",@"url"]),0);
                if (url.length) { UIPasteboard.generalPasteboard.string=url; AWEShowDownloadToast(@"作品链接已复制"); }
                else AWEShowDownloadToast(@"当前作品没有可用的分享链接");
            }
            break;
        case 3: {
            UIView *owner=nil;
            ALBFXArmLike(model);   // 点赞特效：这一条马上要变成已赞（面板点的，没碰点赞键）
            if(!AWEForwardHomeActionKindFromStack(stack,AWEHomeActionKindLike,&owner)) {
                ALB43RecordPanelActionMiss(stack,AWEHomeActionKindLike,@"like-before-controller-fallback");
                if([controller respondsToSelector:@selector(performLikeAction)])[controller performLikeAction];
                else AWEShowDownloadToast(@"未识别到当前作品的点赞动作");
            }
            break;
        }
        case 4: AWEDownloadAudioForAweme(model); break;
        case 5: {
            UIView *owner=nil;
            if(!AWEForwardHomeActionKindFromStack(stack,AWEHomeActionKindComment,&owner)) {
                ALB43RecordPanelActionMiss(stack,AWEHomeActionKindComment,@"comment-before-controller-fallback");
                if([controller respondsToSelector:@selector(performCommentAction)])[controller performCommentAction];
                else AWEShowDownloadToast(@"未识别到当前作品的评论动作");
            }
            break;
        }
        case 6: AWECopyCompleteTextForAweme(model); break;
        case 9:
            if(AWEAlbumImagesForAweme(model).count)AWEDownloadAllImagesForAweme(model);
            else AWEShowDownloadToast(@"当前作品不是图集");
            break;
        case 10: {
            UIView *owner=nil;
            if(!AWEForwardHomeActionKindFromStack(stack,AWEHomeActionKindFavorite,&owner)) {
                ALB43RecordPanelActionMiss(stack,AWEHomeActionKindFavorite,@"favorite");
                AWEShowDownloadToast(@"未识别到当前作品的收藏入口");
            }
            break;
        }
        case 11: {
            UIView *owner=nil;
            if(!AWEForwardHomeActionKindFromStack(stack,AWEHomeActionKindShare,&owner)) {
                ALB43RecordPanelActionMiss(stack,AWEHomeActionKindShare,@"share");
                AWEShowDownloadToast(@"未识别到当前作品的转发入口");
            }
            break;
        }
        case 12: {
            UIView *entry=ALBPaletteFollowEntry(stack);
            if(!entry || !AWEForwardExactNativeTapFromView(entry,@"PALETTE_FOLLOW"))
                AWEShowDownloadToast(@"当前作者已关注，或未找到可用的关注入口");
            break;
        }
        case 13:
            if (gAWEStableCleanScreenEnabled) AWEStableExitCleanScreen();
            else if (stack.window) AWEStableEnterCleanScreenFromStack(stack);
            break;
        case 14: ALBMSSave(model, ALBMSWhatCurrentLive, source); break;
        case 15: ALBMSSave(model, ALBMSWhatAllLive, source); break;
        case 16: ALBMSSave(model, ALBMSWhatAllTypes, source); break;
    }
}

@implementation ALBSidePalette
/* 3.11.17: a touch on an icon's or a pill's bar item goes to the item's own
   control, and that control hands it on to the panel's button
   (ALBBarFace.h ALBFaceAdoptHit; 3.11.16: the panel's taps did nothing). */
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = ALBFaceAdoptHit([super hitTest:point withEvent:event]);
    /* 3.11.45 用户：“把双击面板的关闭方式改为只要没有碰到对应的玻璃按钮胶囊就可以触发关闭，以及双击面板展开时如果上下滑动也可以
       触发关闭并直接滑到下一个视频”。不在按钮 / 胶囊 / 滑条上的触摸不归面板：交给下面的视频（3.11.46：窗口上的
       ALBOutsidePress 按下就关面板、抖音的点按等它所以不暂停；滑动照常交给视频流，直接到下一条）。面板列表能滚时列表留着。 */
    if (UIAccessibilityIsVoiceOverRunning()) return hit;
    if (!hit || hit == self || hit == self.dismissButton) return nil;
    if (hit == self.group && !self.group.scrollEnabled) return nil;
    return hit;
}
/* 碰到面板自己的按钮 / 胶囊 / 滑条：不归关面板的手势 */
- (BOOL)alb_touchOnPanelControl:(UITouch *)touch {
    UIView *view = touch.view;
    if (!view || ![view isDescendantOfView:self]) return NO;
    if (self.group.scrollEnabled && [view isDescendantOfView:self.group]) return YES;   // 列表能滚：在列表上不关
    for (UIView *v = view; v && v != self; v = v.superview)
        if ([v isKindOfClass:UIControl.class] && v != self.dismissButton) return YES;
    return NO;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    if (self.dismissing) return NO;
    return ![self alb_touchOnPanelControl:touch];
}
- (void)alb_gapTapped:(UITapGestureRecognizer *)gesture {
    if (gesture.state != UIGestureRecognizerStateEnded) return;
    gALBPaletteOutsideCloses++;
    [self dismiss];
}
/* 3.11.46：窗口上挂一个按下就关的手势（ALBOutsidePress）：点哪里都关、不暂停视频；上下滑关掉以后照样翻到下一条 */
- (void)alb_installOutsideGestures {
    UIWindow *window = self.feedWindow ?: self.window;
    if (!window || self.outsidePress.view == window) return;
    [self alb_removeOutsideGestures];
    ALBOutsidePress *press = [ALBOutsidePress installOnWindow:window];
    __weak typeof(self) weakSelf = self;
    press.insideControl = ^BOOL(UITouch *touch) {
        ALBSidePalette *palette = weakSelf;
        return !palette || palette.dismissing || [palette alb_touchOnPanelControl:touch];
    };
    press.onClose = ^{ [weakSelf dismiss]; };
    press.onEnded = ^(BOOL swipe, __unused NSString *hitClass) {
        if (swipe) gALBPaletteSwipeCloses++; else gALBPaletteOutsideCloses++;
    };
    self.outsidePress = press;
    if (!self.gapTap) {
        UITapGestureRecognizer *gap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(alb_gapTapped:)];
        gap.cancelsTouchesInView = NO;
        gap.delegate = self;
        [self.group addGestureRecognizer:gap];
        self.gapTap = gap;
    }
}
- (void)alb_removeOutsideGestures {
    [self.outsidePress retire];
    self.outsidePress = nil;
}
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.backgroundColor = UIColor.clearColor;
        self.accessibilityIdentifier = @"AwemeLiquidBar.SidePalette";
        self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        self.dismissButton = [UIButton buttonWithType:UIButtonTypeCustom];
        self.dismissButton.backgroundColor=UIColor.clearColor;
        self.dismissButton.accessibilityLabel = @"关闭快捷面板";
        [self.dismissButton addTarget:self action:@selector(dismiss) forControlEvents:UIControlEventTouchUpInside];
        [self addSubview:self.dismissButton];
        self.group = [UIScrollView new];
        self.group.showsVerticalScrollIndicator = NO;
        self.group.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
        self.group.delaysContentTouches = NO;
        self.group.clipsToBounds = NO; // Native press distortion may extend beyond a row.
        [self addSubview:self.group];
        self.volumeView = [[MPVolumeView alloc] initWithFrame:CGRectMake(-10000, 0, 160, 44)];
        self.volumeView.showsRouteButton = NO;
        self.volumeView.userInteractionEnabled = NO;
        self.volumeView.accessibilityElementsHidden = YES;
        [self addSubview:self.volumeView];
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(refreshLevels) name:UIScreenBrightnessDidChangeNotification object:nil];
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(refreshLevels) name:AVAudioSessionRouteChangeNotification object:nil];
        [AVAudioSession.sharedInstance addObserver:self forKeyPath:@"outputVolume" options:NSKeyValueObservingOptionNew context:&kALBVolumeObservationContext];
        self.volumeObserved = YES;
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(feedItemDidCommit:) name:@"ALB8FeedItemDidCommit" object:nil];
        [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(refreshPaletteMaterial) name:@"AwemeLiquidBar.iOS27AdaptiveGlassStyleDidChange" object:nil];
    }
    return self;
}
- (void)dealloc {
    [self alb_removeOutsideGestures];
    [NSNotificationCenter.defaultCenter removeObserver:self];
    if (self.volumeObserved) [AVAudioSession.sharedInstance removeObserver:self forKeyPath:@"outputVolume" context:&kALBVolumeObservationContext];
}
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
    if (context != &kALBVolumeObservationContext) { [super observeValueForKeyPath:keyPath ofObject:object change:change context:context]; return; }
    dispatch_async(dispatch_get_main_queue(), ^{ [self refreshLevels]; });
}
- (void)refreshLevels {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{ [self refreshLevels]; }); return; }
    if (!self.volume.dragging) [self.volume applyValue:AVAudioSession.sharedInstance.outputVolume haptic:NO];
    if (!self.brightness.dragging) [self.brightness applyValue:UIScreen.mainScreen.brightness haptic:NO];
    [self.volumeView layoutIfNeeded];
    NSMutableArray *queue = [NSMutableArray arrayWithArray:self.volumeView.subviews];
    for (NSUInteger i=0; i<queue.count && i<32; i++) {
        UIView *child=queue[i];
        if ([child isKindOfClass:UISlider.class]) { self.systemVolumeSlider=(UISlider *)child; break; }
        [queue addObjectsFromArray:child.subviews];
    }
    self.volume.enabled = self.systemVolumeSlider != nil && self.systemVolumeSlider.enabled;
    self.volume.alpha = self.volume.enabled ? 1 : 0.45;
    self.volume.accessibilityHint = self.volume.enabled ? @"每次调整百分之一" : @"当前音频输出不支持软件调节音量";
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (!self.window) [self alb_removeOutsideGestures];
    else if (!self.dismissing) [self alb_installOutsideGestures];
    if (!self.window && gALBSidePalette==self) {
        gALBSidePalette=nil; ALB8SetPaletteRightHidden(NO);
    }
    if (self.window && !self.dismissing) {
        [self refreshPaletteMaterial];
        dispatch_async(dispatch_get_main_queue(), ^{ [self refreshLevels]; });
        [self beginRowContrast];
    }
}
- (void)traitCollectionDidChange:(UITraitCollection *)previous {
    [super traitCollectionDidChange:previous];
    [self refreshPaletteMaterial];
}
- (void)refreshPaletteMaterial {
    // 3.11: the bar's glass on every icon and pill, the panel Dark (UIKit
    // turns each glass on its own edge).
    NSString *signature=[NSString stringWithFormat:@"motion|%d|%.3f|%lu|f%d%@",ALB25MaterialMotionClear(),
        ALB25MaterialDarkTint(),(unsigned long)self.icons.count,ALB25MaterialFollowBar(),
        ALB25MaterialFollowBar()?ALB25MaterialFollowGlassKey():@""];
    if(self.overrideUserInterfaceStyle!=UIUserInterfaceStyleDark)self.overrideUserInterfaceStyle=UIUserInterfaceStyleDark;
    if([signature isEqualToString:self.materialSignature])return;
    self.materialSignature=signature;
    // 3.11.5: the edges follow (system or plug-in deciding) on the next layout.
    [self setNeedsLayout];
    NSMutableArray<UIButton *> *buttons=[NSMutableArray arrayWithArray:self.icons?:@[]];
    for(UIView *pill in self.pills)if([pill isKindOfClass:UIButton.class])[buttons addObject:(UIButton *)pill];
    for(UIButton *button in buttons) {
        UIButtonConfiguration *old=button.configuration;
        UIButtonConfiguration *cfg=ALBPaletteGlassConfiguration();
        cfg.contentInsets=old.contentInsets;
        cfg.image=old.image; cfg.title=old.title; cfg.baseForegroundColor=old.baseForegroundColor;
        button.configuration=cfg;
    }
}
- (void)beginRowContrast {
    if (!self.window || !self.pills.count) return;
    ALBPaletteApplyInk(self);
}
- (void)setSourceActionsEnabled:(BOOL)enabled {
    for(UIButton *button in self.icons)if(!ALBPaletteIsSlider(button.tag))button.enabled=enabled;
    for(UIView *pill in self.pills)
        if([pill isKindOfClass:UIControl.class] && ![pill isKindOfClass:ALBPaletteSlider.class])
            ((UIControl *)pill).enabled=enabled;
}
- (void)retargetController:(AWEPlayInteractionViewController *)controller stack:(UIView *)stack {
    if(self.dismissing || !self.window)return;
    stack=stack?:ALBFindVisibleActionStack(controller.viewIfLoaded);
    id model=ALB8PaletteModelForStack(stack);
    if(stack.window!=(self.feedWindow?:self.window) || !model) {
        self.sourceController=nil; self.sourceStack=nil; self.aweme=nil; self.identity=nil;
        [self setSourceActionsEnabled:NO]; return;
    }
    if(!ALB8PaletteControllerOwnsStack(controller,stack,model))controller=nil;
    NSString *identity=AWEAwemeIdentity(model);
    BOOL changed=![self.identity isEqualToString:identity] || self.sourceStack!=stack;
    self.sourceController=controller; self.sourceStack=stack;
    self.aweme=model; self.identity=identity;
    if(changed) {
        [self buildRows]; [self setNeedsLayout]; [self layoutIfNeeded];
        [self beginRowContrast];
        AWEInteractionDiagnosticWrite(@"SIDE_PALETTE_RETARGET model=%@ controllerBound=%d",identity,controller!=nil);
    }
    [self setSourceActionsEnabled:YES];
}
- (void)refreshCurrentSource {
    if(!self.window || self.dismissing)return;
    if(!AWEPluginFeaturesEnabled() || (self.rootFeedSession && (AWECurrentRouteSuppressesBar() ||
        !AWEIndexUsesHomeVisualPolicy(AWECurrentMainTabIndex())))) { [self dismiss]; return; }
    UIView *stack=ALBFindVisibleActionStack((self.feedWindow?:self.window).rootViewController.view);
    id model=ALB8PaletteModelForStack(stack);
    AWEPlayInteractionViewController *controller=nil;
    for(UIResponder *r=stack;r;r=r.nextResponder) {
        if([r isKindOfClass:NSClassFromString(@"AWEPlayInteractionViewController")]) {
            controller=(AWEPlayInteractionViewController *)r; break;
        }
    }
    if(!controller && ALB8PaletteControllerOwnsStack(self.sourceController,stack,model))
        controller=self.sourceController;
    [self retargetController:controller stack:stack];
}
- (void)feedItemDidCommit:(NSNotification *)notification {
    (void)notification;
    [self dismiss];
}
- (void)buildRows {
    for (UIView *view in self.icons) [view removeFromSuperview];
    for (UIView *view in self.pills) [view removeFromSuperview];
    self.icons=nil; self.pills=nil; self.volume=nil; self.brightness=nil;
    BOOL album = AWEAlbumImagesForAweme(self.aweme).count > 0;
    BOOL likeKnown = NO;
    BOOL liked = ALB8PaletteLikeState(self.sourceStack, &likeKnown);
    id author = AWEFirstSafeValue(self.aweme, @[@"author", @"authorModel", @"user"]);
    NSString *nickname = AWEStringFromValue(AWEFirstSafeValue(author, @[@"nickname", @"nickName", @"name"]));
    /* 3.11.45：点赞作品 / 收藏作品；视频走最高画质（实况作品是保存实况）；图集多了保存当前实况、保存所有实况、
       保存所有类型图片（有实况或视频时才出现）；一键清屏 */
    BOOL mainLive = !album && self.aweme && ALBMSMainIsLive(self.aweme);
    NSUInteger albumCount = album ? AWEAlbumImagesForAweme(self.aweme).count : 0;
    ALBMSKind currentKind = album ? ALBMSAlbumCurrentKind(self.aweme) : ALBMSKindVideo;
    NSUInteger lives = album ? ALBMSAlbumLiveCount(self.aweme) : 0;
    BOOL mixed = lives > 0;
    if (album && !mixed) for (id image in AWEAlbumImagesForAweme(self.aweme)) if (ALBMSAlbumEntry(image).kind == ALBMSKindVideo) { mixed = YES; break; }
    NSArray *titles = @[nickname.length ? nickname : @"作者主页", album ? @"保存当前图片" : (mainLive ? @"保存实况" : @"保存当前视频"),
        album ? @"保存图集为视频" : @"复制作品链接", (likeKnown && liked) ? @"取消点赞" : @"点赞作品", @"保存音频", @"打开评论区", @"复制完整文案", @"音量", @"亮度", @"保存所有图片", @"收藏作品", @"转发作品", @"关注作者",
        gAWEStableCleanScreenEnabled ? @"退出清屏" : @"一键清屏", currentKind == ALBMSKindVideo ? @"保存当前视频" : @"保存当前实况", @"保存所有实况", @"保存所有类型图片"];
    NSArray *symbols = @[@"person.crop.circle", @"square.and.arrow.down", album ? @"film" : @"link", (likeKnown && liked) ? @"heart.fill" : @"heart", @"music.note", @"text.bubble", @"doc.on.doc", @"speaker.wave.2.fill", @"sun.max.fill", @"photo.on.rectangle.angled", @"star", @"arrowshape.turn.up.right", @"person.badge.plus",
        gAWEStableCleanScreenEnabled ? @"eye" : @"eye.slash", currentKind == ALBMSKindVideo ? @"video" : @"livephoto", @"livephoto", @"square.stack.3d.down.right"];
    BOOL favoriteKnown=NO, favorited=NO;
    NSArray<UIView *> *nativeActions=ALB33AllHomeActionButtons(self.sourceStack);
    for(UIView *action in nativeActions)if(ALB33PanelActionKind(action,nativeActions)==AWEHomeActionKindFavorite) {
        UIImageView *image=objc_getAssociatedObject(action,&kAWEHomeActionOriginalImageViewKey);
        favoriteKnown=YES; favorited=AWEHomeActionIsSelected(action,AWEHomeActionKindFavorite,image); break;
    }
    if(favoriteKnown && favorited) {
        NSMutableArray *updated=[titles mutableCopy]; updated[10]=@"取消收藏"; titles=updated;
        updated=[symbols mutableCopy]; updated[10]=@"star.fill"; symbols=updated;
    }
    NSMutableArray *icons=[NSMutableArray array], *pills=[NSMutableArray array];
    for (NSNumber *actionID in ALB28PaletteActionOrder()) {
        NSUInteger i=actionID.unsignedIntegerValue;
        if(!ALB5Bool([NSString stringWithFormat:@"panel.action.%lu",(unsigned long)i],YES))continue;
        if(i==9 && !(album && albumCount>1))continue;
        if(i==14 && !(album && (currentKind==ALBMSKindLive || currentKind==ALBMSKindVideo)))continue;
        if(i==15 && !(album && albumCount>1 && lives>0))continue;
        if(i==16 && !(album && albumCount>1 && mixed))continue;
        if(i>=titles.count)continue;
        ALB5PaletteIcon *icon = [ALB5PaletteIcon buttonWithType:UIButtonTypeSystem];
        icon.roleKey=[NSString stringWithFormat:@"palette.%lu",(unsigned long)i];
        UIButtonConfiguration *cfg=ALBPaletteGlassConfiguration();
        cfg.contentInsets=NSDirectionalEdgeInsetsZero;
        cfg.image=[UIImage systemImageNamed:AWEConfiguredSymbolName(icon.roleKey,symbols[i]) withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:18 weight:UIImageSymbolWeightRegular]];
        if (i==0) {
            UIView *host = AWEFindViewByExactRuntimeClass(self.sourceStack, @"AWEPlayInteractionUserProfilePictureView", 0);
            UIImage *avatar = AWEBestProfileAvatarImageView(host).image;
            if (avatar) {
                UIGraphicsImageRenderer *r=[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(34,34)];
                cfg.image=[[r imageWithActions:^(UIGraphicsImageRendererContext *c) {
                    [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(0,0,34,34)] addClip]; [avatar drawInRect:CGRectMake(0,0,34,34)];
                }] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
            }
        }
        UIColor *accent=i==8 ? UIColor.systemYellowColor : (i==1 || i==7 ? UIColor.systemBlueColor : nil);
        if (i==3 && likeKnown && liked) accent=UIColor.systemPinkColor;
        if (i==10 && favoriteKnown && favorited) accent=UIColor.systemYellowColor;
        if (accent) cfg.baseForegroundColor=accent;
        icon.roleArtwork=cfg.image; icon.originalArtwork=i==0 && cfg.image.renderingMode==UIImageRenderingModeAlwaysOriginal;
        icon.automaticInk=accent?:UIColor.labelColor;
        icon.configuration=cfg; icon.tag=i; icon.accessibilityLabel=titles[i];
        icon.accessibilityIdentifier=[NSString stringWithFormat:@"AwemeLiquidBar.SidePalette.Icon.%lu",(unsigned long)i];
        [icon addTarget:self action:@selector(actionTapped:) forControlEvents:UIControlEventTouchUpInside];
        [self.group addSubview:icon]; [icons addObject:icon];
        if (!ALBPaletteIsSlider((NSInteger)i)) {
            UIButton *pill=[UIButton buttonWithType:UIButtonTypeSystem];
            UIButtonConfiguration *text=ALBPaletteGlassConfiguration(); text.title=titles[i]; pill.configuration=text;
            pill.titleLabel.numberOfLines=1; pill.titleLabel.adjustsFontSizeToFitWidth=YES; pill.titleLabel.minimumScaleFactor=0.8;
            pill.tag=i; [pill addTarget:self action:@selector(actionTapped:) forControlEvents:UIControlEventTouchUpInside];
            pill.accessibilityIdentifier=[NSString stringWithFormat:@"AwemeLiquidBar.SidePalette.Action.%lu",(unsigned long)i];
            [self.group addSubview:pill]; [pills addObject:pill];
        } else {
            ALBPaletteSlider *slider=[[ALBPaletteSlider alloc] initWithName:titles[i] color:accent];
            [self.group addSubview:slider]; [pills addObject:slider];
            if(i==7) self.volume=slider; else self.brightness=slider;
            icon.userInteractionEnabled=NO; icon.accessibilityElementsHidden=YES;
        }
    }
    self.icons=icons; self.pills=pills;
    self.materialSignature=nil; [self refreshPaletteMaterial];
    __weak ALBSidePalette *weakSelf=self;
    self.volume.valueChanged=^(CGFloat value) {
        ALBSidePalette *live=weakSelf;
        UISlider *slider=live.systemVolumeSlider;
        if (!slider) {
            [live.volumeView layoutIfNeeded];
            for (UIView *child in live.volumeView.subviews) if ([child isKindOfClass:UISlider.class]) { slider=(UISlider *)child; live.systemVolumeSlider=slider; break; }
        }
        if (slider) { [slider setValue:value animated:NO]; [slider sendActionsForControlEvents:UIControlEventValueChanged]; }
    };
    self.brightness.valueChanged=^(CGFloat value) { UIScreen.mainScreen.brightness=value; };
    [self refreshLevels];
}
- (void)layoutSubviews {
    [super layoutSubviews];
    // 2.4: the palette is a UIView, not a UIButton (2.3 sent it -isTracking and
    // crashed). Its buttons are applied in -refreshPaletteMaterial.
    CGFloat width=self.bounds.size.width, height=self.bounds.size.height;
    CGFloat top=MIN(MAX(self.safeAreaInsets.top+54, 100),height*0.25);
    CGFloat bottom=MIN(MAX(self.safeAreaInsets.bottom+64, 90),height*0.20);
    CGFloat scale=MIN(1.05, MAX(0.82,width/393.0));
    CGFloat side=38*scale, gap=5*scale, row=side+gap, pillHeight=27*scale;
    CGFloat total=MAX(0,row*self.icons.count-gap), groupWidth=(38+9+146)*scale;
    CGFloat visibleHeight=MIN(total, MAX(1,height-top-bottom));
    BOOL right=self.anchorFraction.x>0.5;
    CGFloat x=right ? width-12-groupWidth : 12;
    CGFloat y=MIN(MAX(top,self.anchorFraction.y*height-visibleHeight*0.5), MAX(top,height-bottom-visibleHeight));
    self.group.frame=CGRectMake(x,y,groupWidth,visibleHeight);
    self.group.contentSize=CGSizeMake(groupWidth,total);
    self.group.alwaysBounceVertical=total>visibleHeight;
    self.group.scrollEnabled=total>visibleHeight;
    self.dismissButton.frame=self.bounds;
    self.group.clipsToBounds=total>visibleHeight;
    for (NSUInteger i=0; i<self.icons.count; i++) {
        CGFloat pillWidth=146*scale;
        if(![self.pills[i] isKindOfClass:ALBPaletteSlider.class]) {
            UIButton *button=(UIButton *)self.pills[i];
            CGFloat text=[button.configuration.title sizeWithAttributes:@{NSFontAttributeName:[UIFont systemFontOfSize:12.5 weight:UIFontWeightMedium]}].width;
            pillWidth=MIN(146*scale, MAX(64*scale,text+22*scale));
        }
        self.icons[i].frame=CGRectMake(right ? groupWidth-side : 0, i*row, side, side);
        CGFloat pillX=right ? groupWidth-side-9*scale-pillWidth : side+9*scale;
        self.pills[i].frame=CGRectMake(pillX, i*row+(side-pillHeight)/2, pillWidth, pillHeight);
    }
    // 3.11.9: every icon and text pill wears a bar item face, each on its
    // own Dark edge (ALBFaceLayout), turned by UIKit (3.11.10). A button under
    // a face is never on an edge itself (3.11.9 put the icons on one at their
    // first layout, before their face, and nothing took it away); the two
    // sliders keep their glass button, on an edge of their own
    // (ALBPaletteMarkEdge).
    for (UIButton *icon in self.icons) {
        ALBPaletteMarkEdge(icon, NO);
        ALBFaceLayout(icon);
        ALBFollowSyncEdge(icon);
    }
    for (UIView *pill in self.pills) {
        BOOL slider = [pill isKindOfClass:ALBPaletteSlider.class];
        if ([pill isKindOfClass:UIButton.class] && !slider && !ALBFaceFor(pill))
            ALBPaletteFacePill(pill, right);
        ALBPaletteMarkEdge(pill, slider);
        if ([pill isKindOfClass:UIButton.class]) ALBFaceLayout((UIButton *)pill);
        ALBFollowSyncEdge(pill);
    }
}
- (void)dismiss {
    if (self.dismissing) return;
    ALBGlassLifecycleMark(@"面板关闭：动画前");
    self.dismissing=YES; ++self.sourceGeneration;
    // 3.11.28: its glass fades out as UIKit draws it (no instant finish).
    ALBGlassSeedPanelClosing(self);
    if (gALBSidePalette==self) { gALBSidePalette=nil; ALB8SetPaletteRightHidden(NO); }
    [self alb_removeOutsideGestures];
    self.userInteractionEnabled=NO;
    // 3.11.28: down to just above UIKit's "hidden" line (alpha < 0.01), then
    // removed: no visibility change while it fades, so its glass keeps its
    // pockets and look to the end.
    CGFloat fadeTo=0.012;   // 3.11.51：新玻璃直接到位固定开
    [UIView animateWithDuration:UIAccessibilityIsReduceMotionEnabled() ? 0 : 0.16 animations:^{ self.alpha=fadeTo; }
        completion:^(__unused BOOL finished) { [self removeFromSuperview]; ALBGlassLifecycleMark(@"面板关闭：移出窗口后"); }];
}
- (void)actionTapped:(UIButton *)sender {
    if (!AWEPluginFeaturesEnabled()) { [self dismiss]; return; }
    NSInteger index=sender.tag;
    if (ALBPaletteIsSlider(index) || index<0 || index>16) return;
    if(self.dismissing)return;
    UIView *stack=self.sourceStack;
    id model=ALB8PaletteModelForStack(stack);
    if(stack.window!=(self.feedWindow?:self.window) || !model || ![AWEAwemeIdentity(model) isEqualToString:self.identity]) {
        [self setSourceActionsEnabled:NO];
        AWEShowDownloadToast(@"正在切换作品，请稍后再试"); return;
    }
    AWEPlayInteractionViewController *controller=ALB8PaletteControllerOwnsStack(self.sourceController,stack,model)
        ?self.sourceController:nil;
    [self dismiss];
    ALBPaletteRun(index, stack, model, controller, @"双击面板");
}
@end

/* 3.11.45：拆成“在窗口里某一点打开”（三段式底栏的 ✦ 也用它，ALBDockHostOpenPalette） */
static BOOL ALBPresentSidePaletteAt(AWEPlayInteractionViewController *controller, CGPoint location);
static BOOL ALBPresentSidePalette(AWEPlayInteractionViewController *controller, id sender) {
    UIWindow *window=controller.view.window;
    if (!window) return NO;
    CGPoint location; UIView *coordinateView=nil;
    if (!AWEGetDoubleTapLocation(sender, controller.view, &location, &coordinateView) || !coordinateView) return NO;
    location=[coordinateView convertPoint:location toView:window];
    return ALBPresentSidePaletteAt(controller, location);
}
static BOOL ALBPresentSidePaletteAt(AWEPlayInteractionViewController *controller, CGPoint location) {
    UIWindow *window=controller.view.window;
    if (!window) return NO;
    if (gALBSidePalette.window==window) { [gALBSidePalette dismiss]; return YES; }
    [gALBSidePalette dismiss];
    ALBGlassLifecycleMark(@"面板打开：创建前");
    ALBSidePalette *palette=[[ALBSidePalette alloc] initWithFrame:window.bounds];
    palette.sourceController=controller;
    palette.feedWindow=window;
    palette.rootFeedSession=AWEIndexUsesHomeVisualPolicy(AWECurrentMainTabIndex()) && !AWECurrentRouteSuppressesBar();
    palette.sourceStack=ALBFindVisibleActionStack(controller.view);
    palette.aweme=ALB8PaletteModelForStack(palette.sourceStack);
    if(!ALB8PaletteControllerOwnsStack(controller,palette.sourceStack,palette.aweme))palette.sourceController=nil;
    palette.identity=AWEAwemeIdentity(palette.aweme);
    palette.anchorFraction=CGPointMake(location.x/MAX(1,window.bounds.size.width), location.y/MAX(1,window.bounds.size.height));
    [palette buildRows];
    if(!palette.icons.count) {
        AWEShowDownloadToast(@"快捷面板尚未启用任何功能");
        return YES;
    }
    [palette setSourceActionsEnabled:palette.sourceStack!=nil && palette.aweme!=nil];
    // 3.11.24: the right column's settled glass readings, before it is hidden
    // (the panel's glasses start from them, ALBGlassSeed.h).
    NSArray<NSArray<NSNumber *> *> *rightSamples=ALBGlassSeedRightSamples(window);
    [window addSubview:palette];
    gALBSidePalette=palette;
    ALB8SetPaletteRightHidden(YES);
    [palette setNeedsLayout]; [palette layoutIfNeeded];
    ALBPaletteApplyInk(palette);
    ALBGlassLifecycleMark(@"面板打开：首次布局后");
    // 3.11.28: from just above UIKit's "hidden" line (alpha < 0.01) rather
    // than 0: the glass just built keeps its pockets through the fade-in
    // instead of losing them and building again (ALBGlassSeed.h).
    palette.group.alpha=0.012;   // 3.11.51：新玻璃直接到位固定开
    [UIView animateWithDuration:UIAccessibilityIsReduceMotionEnabled() ? 0 : 0.18 animations:^{ palette.group.alpha=1; }];
    ALBGlassSeedPanelOpened(palette, rightSamples);
    UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification,palette.icons.firstObject);
    AWEInteractionDiagnosticWrite(@"SIDE_PALETTE_OPEN model=%@ point=%@ side=%@",palette.identity,
        NSStringFromCGPoint(location),palette.anchorFraction.x>0.5 ? @"right" : @"left");
    return YES;
}

static void ALB8PaletteControllerAppeared(AWEPlayInteractionViewController *controller) {
    if(gALBSidePalette && gALBSidePalette.sourceController!=controller) [gALBSidePalette dismiss];
}
static void ALB8PaletteControllerDisappearing(AWEPlayInteractionViewController *controller) {
    if(gALBSidePalette.sourceController==controller) [gALBSidePalette dismiss];
}
