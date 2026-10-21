#import "ALBTopNativeGroup.h"
#import "ALBNativeBarSurface.h"
#import "../ALBUnifiedMaterial.h"
#import "ALBCompactGeometry.h"
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIGestureRecognizerSubclass.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <math.h>
#import "../ALBReproCaptureSignal.h"
#include "../ALB5Options.h"
#include "ALB23LensPolicy.h"

// TEST30 capture-only metadata. Read actual UIKit state without creating a
// preview, probing hitTest, changing effects or recording titles/images.
static id ALB30TopCaptureColor(UIColor *color, UITraitCollection *traits) {
    if (!color) return NSNull.null;
    CGFloat r=0,g=0,b=0,a=0;
    UIColor *resolved=[color resolvedColorWithTraitCollection:traits];
    return [resolved getRed:&r green:&g blue:&b alpha:&a] ? @[@(r),@(g),@(b),@(a)] : @"nonRGB";
}
static NSDictionary *ALB30TopCaptureView(UIView *view) {
    if (!view) return @{};
    CALayer *shown=(CALayer *)view.layer.presentationLayer;
    NSMutableDictionary *value=[@{@"object":[NSString stringWithFormat:@"%p",view],
        @"class":NSStringFromClass(view.class), @"bounds":NSStringFromCGRect(view.bounds),
        @"center":NSStringFromCGPoint(view.center), @"frame":NSStringFromCGRect(view.frame),
        @"transform":NSStringFromCGAffineTransform(view.transform), @"alpha":@(view.alpha),
        @"hidden":@(view.hidden), @"layerOpacity":@(view.layer.opacity),
        @"presentationOpacity":shown ? @(shown.opacity) : NSNull.null,
        @"userInteractionEnabled":@(view.userInteractionEnabled),
        @"backgroundRGBA":ALB30TopCaptureColor(view.backgroundColor,view.traitCollection),
        @"tintRGBA":ALB30TopCaptureColor(view.tintColor,view.traitCollection),
        @"style":@(view.traitCollection.userInterfaceStyle),
        @"overrideStyle":@(view.overrideUserInterfaceStyle),
        @"contentScale":@(view.contentScaleFactor), @"layerScale":@(view.layer.contentsScale),
        @"animationCount":@(view.layer.animationKeys.count)} mutableCopy];
    if ([view isKindOfClass:UIVisualEffectView.class]) {
        UIVisualEffect *effect=((UIVisualEffectView *)view).effect;
        NSString *description=effect.description ?: @"";
        value[@"effectClass"]=effect ? NSStringFromClass(effect.class) : @"nil";
        value[@"effectDescription"]=[description substringToIndex:MIN(description.length,240)];
    }
    if ([view isKindOfClass:UIImageView.class]) {
        UIImage *image=((UIImageView *)view).image;
        value[@"imageSize"]=NSStringFromCGSize(image.size);
        value[@"imageScale"]=@(image.scale);
        value[@"imagePixels"]=NSStringFromCGSize(CGSizeMake(image.size.width*image.scale,image.size.height*image.scale));
        value[@"imageRenderingMode"]=@(image.renderingMode);
        value[@"contentMode"]=@(view.contentMode);
    }
    return value;
}

void ALB5ApplyBottomBarSurface(UITabBar *bar) { ALB5ApplySharedBottomSurface(bar); ALBMaterialApplyBar(bar, NO); }

void ALB7LayoutTopControlInBounds(UIView *control, CGRect bounds) {
    if (!control || CGRectIsEmpty(bounds)) return;
    CGRect naturalBounds = CGRectMake(0, 0, CGRectGetWidth(bounds), CGRectGetHeight(bounds));
    CGPoint naturalCenter = CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));
    // A UIButton can be translated/stretched by native glass tracking. Writing
    // frame in that state changes its untransformed bounds and bakes the press
    // offset into the next resting pose. Never touch that native transform.
    if (!CGRectEqualToRect(control.bounds, naturalBounds)) control.bounds = naturalBounds;
    if (!CGPointEqualToPoint(control.center, naturalCenter)) control.center = naturalCenter;
}

// One ancestor owns the resting scale. Native controls retain their own press
// transforms underneath it; no independent side shrink or compounded scale.
@interface ALB8TopModuleView : UIView
@end
@implementation ALB8TopModuleView
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    return hit == self ? nil : hit;
}
@end
UIView *ALB8TopModuleHost(UIView *container) {
    if (!container) return nil;
    SEL key = NSSelectorFromString(@"ALB8.TopSharedModule");
    UIView *host = objc_getAssociatedObject(container, key);
    if (!host) {
        host = [ALB8TopModuleView new];
        host.backgroundColor = UIColor.clearColor;
        host.opaque = NO;
        host.clipsToBounds = NO;
        host.layer.zPosition = 1400;
        host.accessibilityIdentifier = @"AwemeLiquidBar.TopSharedModule";
        objc_setAssociatedObject(container, key, host, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        [container addSubview:host];
    }
    if (!CGRectEqualToRect(host.bounds, container.bounds)) host.bounds = container.bounds;
    CGPoint center = CGPointMake(CGRectGetMidX(container.bounds), CGRectGetMidY(container.bounds));
    if (!CGPointEqualToPoint(host.center, center)) host.center = center;
    return host;
}
UIView *ALB8TopOriginalContainer(UIView *view) {
    return [view isKindOfClass:ALB8TopModuleView.class] ? view.superview : view;
}
void ALB7ApplyTopModuleGeometry(UIView *container, UIView *group, UIView *left,
    UIView *right, CGPoint leftNatural, CGPoint rightNatural, CGFloat scale) {
    if (!container || CGRectIsEmpty(container.bounds)) return;
    if (!isfinite(scale) || scale <= 0) scale = 1;
    UIView *host = ALB8TopModuleHost(container);
    CGFloat centerX = CGRectGetMidX(container.bounds), centerY = CGRectGetMidY(container.bounds);
    UIView *views[] = {group, left, right};
    CGFloat naturalX[] = {centerX, leftNatural.x, rightNatural.x};
    for (NSUInteger index = 0; index < 3; index++) {
        UIView *view = views[index];
        if (!view) continue;
        if (view.superview != host) [host addSubview:view];
        // These are plugin wrappers, not UIKit's interactive glass controls.
        // Clear TEST7's independent wrapper transform once, then keep identity.
        if (!CGAffineTransformIsIdentity(view.transform)) view.transform = CGAffineTransformIdentity;
        CGRect sideBounds = CGRectMake(0, 0, 44, 44);
        if (index != 0 && !CGRectEqualToRect(view.bounds, sideBounds)) view.bounds = sideBounds;
        CGPoint center = CGPointMake(naturalX[index], centerY);
        if (!CGPointEqualToPoint(view.center, center)) view.center = center;
    }
    CGAffineTransform transform = CGAffineTransformMakeScale(scale, scale);
    if (!CGAffineTransformEqualToTransform(host.transform, transform)) host.transform = transform;
}

UIColor *ALB8NativeTopTitleColor(UIView *root, NSString *title) {
    if (!root) return nil;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    UIColor *fallback = nil;
    for (NSUInteger index = 0; index < queue.count && index < 96; index++) {
        UIView *view = queue[index];
        if ([view.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."]) continue;
        NSString *text = nil; NSAttributedString *attributed = nil; UIColor *color = nil;
        if ([view isKindOfClass:UILabel.class]) {
            UILabel *label = (UILabel *)view;
            text = label.text; attributed = label.attributedText; color = label.textColor;
        } else if ([view isKindOfClass:UIButton.class]) {
            UIButton *button = (UIButton *)view;
            text = button.currentTitle; attributed = button.currentAttributedTitle;
            color = button.currentTitleColor;
        }
        if (text.length || attributed.length) {
            UIColor *ink = attributed.length ? [attributed attribute:NSForegroundColorAttributeName atIndex:0 effectiveRange:NULL] : nil;
            if ([ink isKindOfClass:UIColor.class]) color = ink;
            if (color && [text ?: attributed.string isEqualToString:title]) return color;
            if (!fallback && color) fallback = color;
        }
        [queue addObjectsFromArray:view.subviews];
    }
    return fallback;
}

static BOOL ALB6RectNear(CGRect a, CGRect b) {
    // Ignore subpixel provider noise, but retain actual attachment/size changes.
    return fabs(a.origin.x-b.origin.x) < 0.05 && fabs(a.origin.y-b.origin.y) < 0.05 &&
        fabs(a.size.width-b.size.width) < 0.05 && fabs(a.size.height-b.size.height) < 0.05;
}

static BOOL XHSQ920TrackingInView(UIView *view, NSUInteger depth) {
    if (!view || depth > 10) return NO;
    if ([view isKindOfClass:UIControl.class] && ((UIControl *)view).tracking) return YES;
    for (UIGestureRecognizer *gesture in view.gestureRecognizers)
        if (gesture.state == UIGestureRecognizerStateBegan || gesture.state == UIGestureRecognizerStateChanged) return YES;
    for (UIView *child in view.subviews)
        if (XHSQ920TrackingInView(child, depth + 1)) return YES;
    return NO;
}

// Observe only the end of a native touch sequence. This recognizer never
// recognizes, prevents or cancels a gesture; UITabBar keeps all interaction.
@interface ALB5TopTouchObserver : UIGestureRecognizer
@property(nonatomic, strong) NSMutableSet<UITouch *> *observedTouches;
@property(nonatomic, copy) void (^didFinishTracking)(void);
@property(nonatomic, copy) void (^didBeginTracking)(void);
@property(nonatomic, copy) void (^didEndActionTap)(NSUInteger sequence);
@property(nonatomic) NSUInteger actionSequence;
@property(nonatomic) BOOL actionSequenceLive;
@property(nonatomic) NSTimeInterval touchStartedAt;
@property(nonatomic) CGFloat maximumTravel;
@property(nonatomic, copy) void (^didEndSingleSlotSwipe)(CGPoint translation);
@property(nonatomic) CGPoint touchOrigin;
@property(nonatomic) CGPoint touchPosition;
@property(nonatomic) BOOL singleTouchSequence;
@end
@implementation ALB5TopTouchObserver
- (instancetype)init {
    if ((self = [super initWithTarget:nil action:NULL])) {
        _observedTouches = [NSMutableSet set];
        self.cancelsTouchesInView = NO;
        self.delaysTouchesBegan = NO;
        self.delaysTouchesEnded = NO;
    }
    return self;
}
- (BOOL)canPreventGestureRecognizer:(UIGestureRecognizer *)other { return NO; }
- (BOOL)canBePreventedByGestureRecognizer:(UIGestureRecognizer *)other { return NO; }
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if (!self.observedTouches.count) {
        UITouch *touch = touches.anyObject;
        self.touchOrigin = self.touchPosition = [touch locationInView:self.view];
        self.singleTouchSequence = touches.count == 1;
        self.actionSequence++;
        if (!self.actionSequence) self.actionSequence = 1;
        self.actionSequenceLive = YES;
        self.touchStartedAt = touch.timestamp;
        self.maximumTravel = 0;
        if (self.didBeginTracking) self.didBeginTracking();
    } else self.singleTouchSequence = NO;
    [self.observedTouches unionSet:touches];
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if (self.singleTouchSequence) {
        self.touchPosition = [touches.anyObject locationInView:self.view];
        self.maximumTravel = MAX(self.maximumTravel,
            hypot(self.touchPosition.x - self.touchOrigin.x, self.touchPosition.y - self.touchOrigin.y));
    }
}
- (void)finishTouches:(NSSet<UITouch *> *)touches cancelled:(BOOL)cancelled {
    if (self.singleTouchSequence) self.touchPosition = [touches.anyObject locationInView:self.view];
    [self.observedTouches minusSet:touches];
    if (self.observedTouches.count) return;
    CGPoint delta = CGPointMake(self.touchPosition.x - self.touchOrigin.x,
                                self.touchPosition.y - self.touchOrigin.y);
    BOOL single = self.singleTouchSequence;
    self.maximumTravel = MAX(self.maximumTravel, hypot(delta.x, delta.y));
    NSUInteger sequence = self.actionSequence;
    NSTimeInterval duration = touches.anyObject.timestamp - self.touchStartedAt;
    BOOL tap = !cancelled && single && self.maximumTravel <= 12.0 && duration >= 0 && duration <= 0.65 &&
        CGRectContainsPoint(self.view.bounds, self.touchPosition);
    self.singleTouchSequence = NO;
    self.state = UIGestureRecognizerStateFailed;
    if (!cancelled && single && self.didEndSingleSlotSwipe) self.didEndSingleSlotSwipe(delta);
    if (tap && self.didEndActionTap) self.didEndActionTap(sequence);
    if (self.didFinishTracking) self.didFinishTracking();
    // The selection delegate and this observer can arrive in either order.
    // Retain this physical sequence through the current event dispatch only.
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        ALB5TopTouchObserver *live = weakSelf;
        if (live.actionSequence == sequence && !live.observedTouches.count)
            live.actionSequenceLive = NO;
    });
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self finishTouches:touches cancelled:NO]; }
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event { [self finishTouches:touches cancelled:YES]; }
- (void)reset {
    BOOL pending = self.observedTouches.count > 0;
    [super reset];
    [self.observedTouches removeAllObjects];
    if (pending && self.didFinishTracking) self.didFinishTracking();
}
@end

// Match the successful Photos-style bottom's controller/content relationship.
// This content is transparent and inert. Host feed gestures never target it.
@interface ALB8TopProxyController : UICollectionViewController
@end
@implementation ALB8TopProxyController
- (instancetype)init {
    NSCollectionLayoutSize *size = [NSCollectionLayoutSize sizeWithWidthDimension:
        [NSCollectionLayoutDimension fractionalWidthDimension:1]
        heightDimension:[NSCollectionLayoutDimension absoluteDimension:96]];
    NSCollectionLayoutItem *item = [NSCollectionLayoutItem itemWithLayoutSize:size];
    NSCollectionLayoutGroup *group = [NSCollectionLayoutGroup verticalGroupWithLayoutSize:size subitem:item count:1];
    NSCollectionLayoutSection *section = [NSCollectionLayoutSection sectionWithGroup:group];
    return [super initWithCollectionViewLayout:[[UICollectionViewCompositionalLayout alloc] initWithSection:section]];
}
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.clearColor;
    self.view.opaque = NO;
    self.collectionView.backgroundColor = UIColor.clearColor;
    self.collectionView.opaque = NO;
    self.collectionView.alwaysBounceVertical = YES;
    self.collectionView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    self.collectionView.showsVerticalScrollIndicator = NO;
    self.collectionView.userInteractionEnabled = NO;
    self.collectionView.accessibilityIdentifier = @"AwemeLiquidBar.TopNative.ProxyCollection";
    [self.collectionView registerClass:UICollectionViewCell.class forCellWithReuseIdentifier:@"transparent"];
}
- (NSInteger)collectionView:(UICollectionView *)view numberOfItemsInSection:(NSInteger)section { return 80; }
- (UICollectionViewCell *)collectionView:(UICollectionView *)view cellForItemAtIndexPath:(NSIndexPath *)path {
    UICollectionViewCell *cell = [view dequeueReusableCellWithReuseIdentifier:@"transparent" forIndexPath:path];
    cell.backgroundColor = cell.contentView.backgroundColor = UIColor.clearColor;
    return cell;
}
@end

@interface ALB8TopTabController : UITabBarController
@property(nonatomic, copy) void (^didFinishLayout)(void);
@end
@implementation ALB8TopTabController
- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.clearColor;
    self.view.opaque = NO;
    self.view.clipsToBounds = NO;
    self.tabBar.backgroundColor = UIColor.clearColor;
    self.tabBar.opaque = NO;
    self.tabBar.translucent = YES;
    self.tabBar.clipsToBounds = NO;
    self.tabBar.itemPositioning = UITabBarItemPositioningFill;
    self.tabBar.itemWidth = self.tabBar.itemSpacing = 0;
    // The entire top module is scaled by its common parent. Native independent
    // scroll minimization would add a second, unsynchronized contraction.
    SEL minimize = NSSelectorFromString(@"setTabBarMinimizeBehavior:");
    if ([self respondsToSelector:minimize]) ((void(*)(id,SEL,NSInteger))objc_msgSend)(self,minimize,1);
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    if (self.didFinishLayout) self.didFinishLayout();
}
@end

BOOL ALB8IsTopChromeController(UIViewController *controller) {
    return [controller isKindOfClass:ALB8TopTabController.class] ||
        [controller isKindOfClass:ALB8TopProxyController.class];
}

@interface ALBTopNativeGroup () <UITabBarControllerDelegate>
@property(nonatomic, strong) UITabBar *nativeBar;
@property(nonatomic, strong) ALB8TopTabController *nativeController;
@property(nonatomic, copy) NSArray *nativeTabs;
@property(nonatomic, strong) id auxiliaryActionTab;
@property(nonatomic, weak) UIView *auxiliaryActionRoot;
@property(nonatomic, weak) UIView *parkedPrimaryPlatter;
@property(nonatomic, strong) UIScrollView *rail;
@property(nonatomic, strong) ALB5TopTouchObserver *touchObserver;
@property(nonatomic, copy) NSArray<NSString *> *renderedTitles;
@property(nonatomic) NSInteger visibleStart;
@property(nonatomic) NSInteger renderedStart;
@property(nonatomic) NSInteger pendingWindowDirection;
@property(nonatomic) NSUInteger externalSelectionRevision;
@property(nonatomic) BOOL appearanceDirty;
@property(nonatomic) BOOL glyphsDirty;
@property(nonatomic) BOOL layoutInProgress;
@property(nonatomic) BOOL trackingLayoutPending;
@property(nonatomic) CGFloat nativeScale;
@property(nonatomic) CGFloat glyphDisplayScale;
@property(nonatomic) CGSize fittedSize;
@property(nonatomic, weak) UIView *fittedPlatter;
@property(nonatomic) CGRect fittedPlatterRect;
@property(nonatomic) NSUInteger providerFitPasses;
@property(nonatomic) BOOL needsProviderFit;
@property(nonatomic) NSUInteger lastActionSequence;
@property(nonatomic, strong) NSMapTable<UIView *, NSNumber *> *idleLensOpacities;
@property(nonatomic) CFTimeInterval nextIdleLensScan;
- (void)dispatchNativeActionForSequence:(NSUInteger)sequence;
- (void)updateIdleActionLensVisibility;
- (void)restoreIdleActionLenses;
- (UIView *)nativePlatter;
- (BOOL)nativePlatterGeometryChanged;
- (void)scheduleTrackingCompletion;
- (NSInteger)effectiveVisibleCount;
- (void)requestGlobalSelection:(NSInteger)index;
- (BOOL)handleNativeItemSelection:(UITabBarItem *)item;
- (void)attachNativeController;
@end

@implementation ALBTopNativeGroup
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) [self commonInit];
    return self;
}
- (instancetype)initWithCoder:(NSCoder *)coder {
    if ((self = [super initWithCoder:coder])) [self commonInit];
    return self;
}
- (void)commonInit {
    _titles = @[];
    _renderedTitles = @[];
    _renderedStart = -1;
    _visibleItemCount = 3;
    _selectedIndex = -1;
    _keepsSingleSelectionCapsule = YES;
    _automaticContrastEnabled = NO;
    _fallbackForegroundColor = UIColor.labelColor;
    _selectionAccentColor = [UIColor colorWithRed:1 green:0.16 blue:0.29 alpha:1];
    _titleFont = [UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];
    _nativeScale = 1.0;
    self.backgroundColor = UIColor.clearColor;
    self.opaque = NO;
    self.clipsToBounds = NO;

    _rail = [UIScrollView new];
    _rail.backgroundColor = UIColor.clearColor;
    _rail.clipsToBounds = NO;
    // The complete native platter always fits this view. A long tab bar behind
    // a scrolling clip is what cut its left round end off in TEST5.
    _rail.scrollEnabled = NO;
    _rail.bounces = NO;
    _rail.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    _rail.delaysContentTouches = NO;
    _rail.canCancelContentTouches = NO;
    _rail.showsHorizontalScrollIndicator = NO;
    _rail.showsVerticalScrollIndicator = NO;
    [self addSubview:_rail];

    _nativeController = [ALB8TopTabController new];
    _nativeController.delegate = self;
    [_nativeController loadViewIfNeeded];
    _nativeBar = _nativeController.tabBar;
    _nativeBar.accessibilityIdentifier = @"AwemeLiquidBar.TopNative.NativeTabBar.q920";
    ALB5SetNativeSurfaceRoot(_nativeBar, _nativeController.view);
    [_rail addSubview:_nativeController.view];
    __weak typeof(self) weakSelf = self;
    _nativeController.didFinishLayout = ^{
        ALBTopNativeGroup *group = weakSelf;
        if (group && !group.layoutInProgress && !group.isSelectionTracking &&
            group.providerFitPasses < 2 && [group nativePlatterGeometryChanged]) {
            group.needsProviderFit = YES;
            [group setNeedsLayout];
        }
    };
    _touchObserver = [ALB5TopTouchObserver new];
    _touchObserver.didFinishTracking = ^{
        ALBTopNativeGroup *group=weakSelf;
        [group scheduleTrackingCompletion];
        if (group.onTrackingFinished) group.onTrackingFinished();
    };
    _touchObserver.didBeginTracking = ^{
        // Observe the physical sequence before search delegates decide whether
        // to accept it. No visibility, geometry or native recognizer mutation.
        ALBTopNativeGroup *group=weakSelf;
        if (group.onTrackingBegan) group.onTrackingBegan();
    };
    _touchObserver.didEndActionTap = ^(NSUInteger sequence) {
        ALBTopNativeGroup *group = weakSelf;
        if (group.actionOnly) [group dispatchNativeActionForSequence:sequence];
    };
    _touchObserver.didEndSingleSlotSwipe = ^(CGPoint delta) {
        ALBTopNativeGroup *group = weakSelf;
        // A one-slot tab bar has no neighbouring native item to scrub to.
        // Observe a completed horizontal swipe without cancelling UIKit touch.
        if (!group.actionOnly && group.effectiveVisibleCount == 1 && fabs(delta.x) >= 24 &&
            fabs(delta.x) > fabs(delta.y) * 1.5) {
            NSInteger step = delta.x < 0 ? 1 : -1;
            NSInteger before = group.selectedIndex;
            dispatch_async(dispatch_get_main_queue(), ^{
                ALBTopNativeGroup *live = weakSelf;
                if (live && live.selectedIndex == before && !live.isSelectionTracking)
                    [live requestGlobalSelection:before + step];
            });
        }
    };
    [_nativeController.view addGestureRecognizer:_touchObserver];
    _appearanceDirty = YES;
    _glyphsDirty = YES;
    [self applyAppearance];
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(accentChanged:)
        name:@"XHSLiquidBar.SelectionAccentDidChange" object:nil];
}
- (void)dealloc {
    [NSNotificationCenter.defaultCenter removeObserver:self];
    [_nativeController willMoveToParentViewController:nil];
    [_nativeController.view removeFromSuperview];
    [_nativeController removeFromParentViewController];
}
- (void)attachNativeController {
    UIViewController *parent = nil;
    if (self.window) {
        for (UIResponder *cursor = self.nextResponder; cursor; cursor = cursor.nextResponder) {
            if ([cursor isKindOfClass:UIViewController.class] && cursor != self.nativeController) {
                parent = (UIViewController *)cursor; break;
            }
        }
    }
    if (self.nativeController.parentViewController == parent) return;
    [self.nativeController willMoveToParentViewController:nil];
    [self.nativeController removeFromParentViewController];
    if (parent) {
        [parent addChildViewController:self.nativeController];
        [self.nativeController didMoveToParentViewController:parent];
    }
}
- (void)didMoveToWindow {
    [super didMoveToWindow];
    [self attachNativeController];
    self.fittedSize = CGSizeZero;
    self.glyphsDirty = YES;
    [self setNeedsLayout];
}
- (void)setOverrideUserInterfaceStyle:(UIUserInterfaceStyle)style {
    if (self.overrideUserInterfaceStyle==style) return;
    [super setOverrideUserInterfaceStyle:style];
    self.appearanceDirty=YES;
    [self setNeedsLayout];
}
- (void)traitCollectionDidChange:(UITraitCollection *)previous {
    [super traitCollectionDidChange:previous];
    if (!previous || [self.traitCollection hasDifferentColorAppearanceComparedToTraitCollection:previous]) {
        self.appearanceDirty = YES;
        self.glyphsDirty = YES;
        self.fittedSize = CGSizeZero;
        [self setNeedsLayout];
    }
    if (!previous || self.traitCollection.displayScale != previous.displayScale) {
        self.glyphsDirty = YES;
        [self setNeedsLayout];
    }
}
- (BOOL)isSelectionTracking {
    return self.touchObserver.observedTouches.count > 0 || XHSQ920TrackingInView(self.nativeController.view, 0);
}
- (void)setSelectionAccentColor:(UIColor *)color {
    UIColor *next = color ?: [UIColor colorWithRed:1 green:0.16 blue:0.29 alpha:1];
    if ([_selectionAccentColor isEqual:next]) return;
    _selectionAccentColor = next;
    self.glyphsDirty = YES;
    self.appearanceDirty = YES;
    [self setNeedsLayout];
}
- (void)accentChanged:(NSNotification *)notification {
    if (![notification.object isKindOfClass:UIColor.class]) return;
    UIColor *color = notification.object;
    if (NSThread.isMainThread) { self.selectionAccentColor = color; return; }
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{ weakSelf.selectionAccentColor = color; });
}
- (void)setFallbackForegroundColor:(UIColor *)color {
    UIColor *next = color ?: UIColor.labelColor;
    if ([_fallbackForegroundColor isEqual:next]) return;
    _fallbackForegroundColor = next;
    self.glyphsDirty = YES;
    self.appearanceDirty = YES;
    [self setNeedsLayout];
}
- (void)setTitleFont:(UIFont *)font {
    UIFont *next = font ?: [UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];
    if ([_titleFont isEqual:next]) return;
    _titleFont = next;
    self.glyphsDirty = YES;
    [self setNeedsLayout];
}
- (void)setActionImage:(UIImage *)image {
    if (_actionImage==image || [_actionImage isEqual:image]) return;
    _actionImage=image;
    self.glyphsDirty=YES;
    [self setNeedsLayout];
}
- (void)setActionOnly:(BOOL)actionOnly {
    if (_actionOnly==actionOnly) return;
    _actionOnly=actionOnly;
    self.glyphsDirty=YES;
    [self setNeedsLayout];
}
// Use the same image-as-item pipeline as title glyphs, with compensation only
// for this module's stable native canvas scale, never a native press transform.
- (UIImage *)actionGlyphWithDisplayScale:(CGFloat)displayScale {
    UIImage *source=self.actionImage;
    if (!source) return nil;
    CGFloat factor=1/MAX(.35,self.nativeScale);
    CGSize size=CGSizeMake(MAX(1,source.size.width*factor),MAX(1,source.size.height*factor));
    UIGraphicsImageRendererFormat *format=[UIGraphicsImageRendererFormat defaultFormat];
    format.opaque=NO; format.scale=MAX(1,displayScale);
    UIGraphicsImageRenderer *renderer=[[UIGraphicsImageRenderer alloc] initWithSize:size format:format];
    return [[renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        [source drawInRect:(CGRect){CGPointZero,size}];
    }] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}
- (void)setForceClear:(BOOL)value { if (_forceClear == value) return; _forceClear = value; self.appearanceDirty = YES; [self setNeedsLayout]; }
- (void)setTitleForegroundColors:(NSArray<UIColor *> *)colors {
    NSArray *next = [colors copy] ?: @[];
    if ([_titleForegroundColors isEqualToArray:next]) return;
    _titleForegroundColors = next;
    self.glyphsDirty = YES;
    [self setNeedsLayout];
}
- (void)applyAppearance {
    if (!self.appearanceDirty || self.isSelectionTracking) return;
    ALB5ApplyBottomBarSurface(self.nativeBar);
    // Do not assign standardAppearance/scrollEdgeAppearance on iOS26+. Even
    // copying the standalone bar's default freezes a different surface recipe.
    // Original-color title images below own ink only, including unselected tabs.
    self.nativeBar.tintColor = self.selectionAccentColor;
    self.nativeBar.unselectedItemTintColor = self.fallbackForegroundColor;
    self.nativeController.overrideUserInterfaceStyle = self.overrideUserInterfaceStyle;
    self.nativeController.view.overrideUserInterfaceStyle = self.overrideUserInterfaceStyle;
    // 2.2: the bottom bar also sets the style on the UITabBar itself. Without
    // it the top group's platter glass kept the window's (light) traits on
    // iOS 27.0 even when the group/controller/view were pinned Dark.
    if (self.nativeBar.overrideUserInterfaceStyle != self.overrideUserInterfaceStyle)
        self.nativeBar.overrideUserInterfaceStyle = self.overrideUserInterfaceStyle;
    self.appearanceDirty = NO;
}
- (void)setTitles:(NSArray<NSString *> *)titles {
    NSArray<NSString *> *next = [titles copy] ?: @[];
    if ([_titles isEqualToArray:next]) return;
    _titles = next;
    if (_selectedIndex >= (NSInteger)_titles.count) _selectedIndex = -1;
    [self setNeedsLayout];
    [self invalidateIntrinsicContentSize];
}
// A side action is a genuine UISearchTab auxiliary control, not a selected
// regular tab. This removes the *source* of the selection capsule, at rest and
// while pressed, without hiding a lens, portal, icon or touch target.
- (BOOL)rebuildAuxiliaryActionIfNeeded {
    if (!self.actionOnly) return NO;
    if (self.auxiliaryActionTab) {
        self.renderedTitles=self.titles.count ? @[self.titles.firstObject] : @[@"操作"];
        return YES;
    }
    Class regularClass=NSClassFromString(@"UITab"), searchClass=NSClassFromString(@"UISearchTab");
    SEL regularInit=NSSelectorFromString(@"initWithTitle:image:identifier:viewControllerProvider:");
    SEL searchInit=NSSelectorFromString(@"initWithViewControllerProvider:");
    SEL setTabs=NSSelectorFromString(@"setTabs:animated:");
    if (![regularClass instancesRespondToSelector:regularInit] ||
        ![searchClass instancesRespondToSelector:searchInit] ||
        ![self.nativeController respondsToSelector:setTabs]) return NO;
    ALB8TopProxyController *content=[ALB8TopProxyController new];
    ALB8TopProxyController *searchContent=[ALB8TopProxyController new];
    UIViewController *(^mainProvider)(id)=^UIViewController *(id tab){ return content; };
    UIViewController *(^searchProvider)(id)=^UIViewController *(id tab){ return searchContent; };
    id regular=((id(*)(id,SEL,id,id,id,id))objc_msgSend)([regularClass alloc],regularInit,
        @"",nil,@"top.action.parking",mainProvider);
    id search=((id(*)(id,SEL,id))objc_msgSend)([searchClass alloc],searchInit,searchProvider);
    if (!regular || !search) return NO;
    SEL autoSearch=NSSelectorFromString(@"setAutomaticallyActivatesSearch:");
    if ([search respondsToSelector:autoSearch]) ((void(*)(id,SEL,BOOL))objc_msgSend)(search,autoSearch,NO);
    UIImage *glyph=[self actionGlyphWithDisplayScale:MAX(1,self.traitCollection.displayScale)];
    ((void(*)(id,SEL,id))objc_msgSend)(search,@selector(setImage:),glyph);
    self.auxiliaryActionTab=search;
    self.nativeTabs=@[regular,search];
    ((void(*)(id,SEL,id,BOOL))objc_msgSend)(self.nativeController,setTabs,self.nativeTabs,NO);
    ((void(*)(id,SEL,id))objc_msgSend)(self.nativeController,NSSelectorFromString(@"setSelectedTab:"),regular);
    self.renderedTitles=self.titles.count ? @[self.titles.firstObject] : @[@"操作"];
    self.renderedStart=self.visibleStart=0;
    self.fittedSize=CGSizeZero; self.needsProviderFit=YES; self.providerFitPasses=0;
    self.glyphsDirty=YES;
    return YES;
}
- (UIView *)actionAuxiliaryPlatter {
    UIView *root=self.nativeController.view;
    UIView *aux=nil;
    NSMutableArray<UIView *> *queue=[NSMutableArray arrayWithObject:root];
    for (NSUInteger i=0;i<queue.count && i<160;i++) {
        UIView *v=queue[i];
        if ([NSStringFromClass(v.class) containsString:@"TabBarAuxiliaryView"] && !CGRectIsEmpty(v.bounds)) { aux=v; break; }
        if (![v isKindOfClass:UIScrollView.class]) [queue addObjectsFromArray:v.subviews];
    }
    if (!aux) return nil;
    self.auxiliaryActionRoot=aux;
    // Only the unused PRIMARY parking platter is hidden. It contains no
    // business action. The actual auxiliary remains wholly native and intact.
    queue=[NSMutableArray arrayWithObject:root];
    UIView *actual=nil;
    for (NSUInteger i=0;i<queue.count && i<192;i++) {
        UIView *v=queue[i];
        if ([NSStringFromClass(v.class) containsString:@"TabBarPlatterView"]) {
            if ([v isDescendantOfView:aux]) actual=v;
            else if (![aux isDescendantOfView:v]) {
                if (!v.hidden) v.hidden=YES;
                self.parkedPrimaryPlatter=v;
            }
        }
        if (![v isKindOfClass:UIScrollView.class]) [queue addObjectsFromArray:v.subviews];
    }
    return actual ?: aux;
}
- (void)fitAuxiliaryActionToSize:(CGSize)size {
    if (CGSizeEqualToSize(self.fittedSize,size) && self.fittedPlatter && !self.needsProviderFit) return;
    if (self.providerFitPasses>=2 && CGSizeEqualToSize(self.fittedSize,size)) return;
    self.providerFitPasses++; self.needsProviderFit=NO;
    UIView *canvas=self.nativeController.view;
    canvas.transform=CGAffineTransformIdentity;
    canvas.bounds=CGRectMake(0,0,360,160);
    [canvas setNeedsLayout]; [canvas layoutIfNeeded];
    UIView *platter=[self actionAuxiliaryPlatter];
    if (!platter) {
        self.needsProviderFit=YES;
        return; // never replace missing native glass with a selected tab
    }
    CGRect rect=[platter convertRect:platter.bounds toView:canvas];
    if (rect.size.height<=0 || rect.size.width<=0) return;
    CGFloat scale=MIN(size.width/rect.size.width,size.height/rect.size.height);
    self.nativeScale=scale;
    canvas.transform=CGAffineTransformMakeScale(scale,scale);
    canvas.center=CGPointMake(size.width/2-(CGRectGetMidX(rect)-CGRectGetMidX(canvas.bounds))*scale,
        size.height/2-(CGRectGetMidY(rect)-CGRectGetMidY(canvas.bounds))*scale);
    self.fittedSize=size; self.fittedPlatter=platter; self.fittedPlatterRect=rect;
    self.glyphsDirty=YES;
}

- (void)rebuildItemsIfNeeded {
    if (self.isSelectionTracking) return;
    if (self.actionOnly) {
        [self rebuildAuxiliaryActionIfNeeded];
        return;
    }
    NSInteger count = MIN(self.effectiveVisibleCount, (NSInteger)self.titles.count);
    self.visibleStart = ALB6TopWindowStart((NSInteger)self.titles.count, count,
        self.visibleStart, self.selectedIndex, self.pendingWindowDirection);
    self.pendingWindowDirection = 0;
    NSArray<NSString *> *next = [self.titles subarrayWithRange:NSMakeRange(self.visibleStart, count)];
    if (self.renderedStart == self.visibleStart && [self.renderedTitles isEqualToArray:next]) return;
    // Only the visible slots are present in the native bar. Its two rounded
    // glass ends remain inside the module, even with seven host categories and
    // a two-item user preference. Tags retain the original business route.
    if (self.nativeBar.items.count != next.count) {
        NSMutableArray<UIViewController *> *controllers = [NSMutableArray array];
        NSMutableArray *tabs = [NSMutableArray array];
        Class tabClass = NSClassFromString(@"UITab");
        SEL initializer = NSSelectorFromString(@"initWithTitle:image:identifier:viewControllerProvider:");
        SEL setter = NSSelectorFromString(@"setTabs:animated:");
        BOOL modern = tabClass && [tabClass instancesRespondToSelector:initializer] &&
            [self.nativeController respondsToSelector:setter];
        for (NSUInteger local = 0; local < next.count; local++) {
            ALB8TopProxyController *proxy = [ALB8TopProxyController new];
            [controllers addObject:proxy];
            if (modern) {
                NSString *identifier = [NSString stringWithFormat:@"top.slot.%lu", (unsigned long)local];
                UIViewController *(^provider)(id) = ^UIViewController *(id tab) { return proxy; };
                NSInteger route = self.visibleStart + (NSInteger)local;
                UIColor *ink = route < (NSInteger)self.titleForegroundColors.count ? self.titleForegroundColors[route] : self.fallbackForegroundColor;
                UIImage *initialGlyph = self.actionOnly ? [self actionGlyphWithDisplayScale:MAX(1,self.traitCollection.displayScale)] : [self titleGlyphForString:next[local] color:ink displayScale:MAX(1, self.traitCollection.displayScale)];
                id tab = ((id(*)(id,SEL,id,id,id,id))objc_msgSend)([tabClass alloc], initializer, @"", initialGlyph, identifier, provider);
                if (tab) [tabs addObject:tab];
            } else proxy.tabBarItem = [[UITabBarItem alloc] initWithTitle:nil image:nil tag:local];
        }
        if (modern && tabs.count == next.count) {
            self.nativeTabs = tabs;
            ((void(*)(id,SEL,id,BOOL))objc_msgSend)(self.nativeController,setter,tabs,NO);
        } else {
            self.nativeTabs = @[];
            [self.nativeController setViewControllers:controllers animated:NO];
        }
        [self.nativeController.view setNeedsLayout];
        [self.nativeController.view layoutIfNeeded];
        self.fittedSize = CGSizeZero;
    }
    for (NSUInteger local = 0; local < self.nativeBar.items.count && local < next.count; local++) {
        UITabBarItem *item = self.nativeBar.items[local];
        item.tag = self.visibleStart + (NSInteger)local;
        item.accessibilityLabel = next[local];
        item.accessibilityIdentifier = [NSString stringWithFormat:@"AwemeLiquidBar.TopNative.Item.%ld", (long)item.tag];
    }
    self.renderedTitles = next;
    self.renderedStart = self.visibleStart;
    self.glyphsDirty = YES;
}
- (UIImage *)titleGlyphForString:(NSString *)title color:(UIColor *)color displayScale:(CGFloat)displayScale {
    CGFloat scale = MAX(0.35, self.nativeScale);
    UIFont *font = [self.titleFont fontWithSize:self.titleFont.pointSize / scale];
    NSMutableParagraphStyle *paragraph = [NSMutableParagraphStyle new];
    paragraph.lineBreakMode = NSLineBreakByTruncatingTail;
    NSDictionary *attributes = @{NSFontAttributeName:font, NSForegroundColorAttributeName:[color resolvedColorWithTraitCollection:self.traitCollection],
        NSParagraphStyleAttributeName:paragraph};
    CGSize measured = [title sizeWithAttributes:attributes];
    // Add a device pixel on either side to preserve antialiasing at the edge.
    CGFloat padding = 1.0 / displayScale;
    // Bound unusually long location names to the existing item footprint. Draw
    // one truncated line at the same font size rather than letting UIKit shrink
    // an arbitrarily wide image. Accessibility retains the full native title.
    CGFloat visibleSlotWidth = CGRectGetWidth(self.bounds) / self.effectiveVisibleCount;
    UIView *platter = [self nativePlatter];
    if (platter && self.nativeBar.items.count) {
        CGRect platterRect = [platter convertRect:platter.bounds toView:self];
        CGFloat actualSlotWidth = CGRectGetWidth(platterRect) / self.nativeBar.items.count;
        if (actualSlotWidth > 0 && isfinite(actualSlotWidth))
            visibleSlotWidth = MIN(visibleSlotWidth, actualSlotWidth);
    }
    CGFloat availableWidth = MAX(self.titleFont.pointSize * 2, visibleSlotWidth - 8.0) / scale;
    CGFloat textWidth = MIN(measured.width, availableWidth);
    CGSize size = CGSizeMake(MAX(1, ceil(textWidth * displayScale) / displayScale + padding * 2),
                             MAX(1, ceil(measured.height * displayScale) / displayScale + padding * 2));
    UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat defaultFormat];
    format.scale = displayScale;
    format.opaque = NO;
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc] initWithSize:size format:format];
    UIImage *image = [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        if (measured.width <= availableWidth) {
            [title drawAtPoint:CGPointMake(padding, padding) withAttributes:attributes];
        } else {
            [title drawInRect:CGRectMake(padding, padding, size.width - padding * 2,
                size.height - padding * 2) withAttributes:attributes];
        }
    }];
    return [image imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}
- (void)updateTitleGlyphsIfNeeded {
    if (self.actionOnly && self.auxiliaryActionTab) {
        CGFloat scale=MAX(1,self.traitCollection.displayScale);
        if (!self.glyphsDirty && self.glyphDisplayScale==scale) return;
        UIImage *glyph=[self actionGlyphWithDisplayScale:scale];
        ((void(*)(id,SEL,id))objc_msgSend)(self.auxiliaryActionTab,@selector(setImage:),glyph);
        SEL selected=NSSelectorFromString(@"setSelectedImage:");
        if ([self.auxiliaryActionTab respondsToSelector:selected])
            ((void(*)(id,SEL,id))objc_msgSend)(self.auxiliaryActionTab,selected,glyph);
        self.glyphDisplayScale=scale; self.glyphsDirty=NO;
        return;
    }

    if (!self.glyphsDirty || self.isSelectionTracking) return;
    CGFloat displayScale = self.window.screen.scale;
    if (displayScale <= 0) displayScale = self.traitCollection.displayScale;
    if (displayScale <= 0) displayScale = UIScreen.mainScreen.scale;
    displayScale = MAX(1, displayScale);
    for (NSUInteger index = 0; index < self.nativeBar.items.count && index < self.renderedTitles.count; index++) {
        UITabBarItem *item = self.nativeBar.items[index];
        NSInteger route = self.renderedStart + (NSInteger)index;
        // Providers may create their UITabBarItems lazily during the first
        // controller layout. Do not leave a late-created item with tag zero.
        item.tag = route;
        item.accessibilityLabel = self.renderedTitles[index];
        item.accessibilityIdentifier = [NSString stringWithFormat:@"AwemeLiquidBar.TopNative.Item.%ld", (long)route];
        UIColor *ink = route >= 0 && route < (NSInteger)self.titleForegroundColors.count ? self.titleForegroundColors[route] :
            (route == self.selectedIndex ? self.selectionAccentColor : self.fallbackForegroundColor);
        UIImage *glyph = self.actionOnly ? [self actionGlyphWithDisplayScale:displayScale] : [self titleGlyphForString:self.renderedTitles[index] color:ink displayScale:displayScale];
        if (index < self.nativeTabs.count) {
            id tab = self.nativeTabs[index];
            ((void(*)(id,SEL,id))objc_msgSend)(tab, @selector(setImage:), glyph);
            SEL selectedImage = NSSelectorFromString(@"setSelectedImage:");
            if ([tab respondsToSelector:selectedImage]) ((void(*)(id,SEL,id))objc_msgSend)(tab, selectedImage, glyph);
        }
        item.image = glyph;
        item.selectedImage = glyph;
        item.imageInsets = UIEdgeInsetsZero;
    }
    self.glyphDisplayScale = displayScale;
    self.glyphsDirty = NO;
}
- (NSInteger)effectiveVisibleCount { return MAX(1, MIN(self.visibleItemCount, (NSInteger)self.titles.count)); }
- (BOOL)hidesSelection {
    // An action carrier still has an active native proxy. Never clear its bar's
    // selectedItem while the controller continues to own a selected tab.
    return !self.actionOnly && [self effectiveVisibleCount] == 1 && !self.keepsSingleSelectionCapsule;
}
- (void)setVisibleItemCount:(NSInteger)count {
    NSInteger next = MAX(1, MIN(4, count));
    if (_visibleItemCount == next) return;
    _visibleItemCount = next;
    [self setNeedsLayout];
    [self invalidateIntrinsicContentSize];
}
- (void)setKeepsSingleSelectionCapsule:(BOOL)keep {
    if (_keepsSingleSelectionCapsule == keep) return;
    _keepsSingleSelectionCapsule = keep;
    self.nextIdleLensScan = 0;
    if (keep) [self restoreIdleActionLenses];
    [self setNeedsLayout];
}
- (void)setUsesNativeTitleFallback:(BOOL)fallback { _usesNativeTitleFallback = fallback; }
- (void)setSelectedIndex:(NSInteger)index { [self setSelectedIndex:index animated:NO]; }
- (void)setSelectedIndex:(NSInteger)index animated:(BOOL)animated {
    if (index == NSNotFound) index = -1;
    if (index < -1 || index >= (NSInteger)self.titles.count) return;
    self.externalSelectionRevision++;
    BOOL changed = _selectedIndex != index;
    _selectedIndex = index;
    if (changed) self.glyphsDirty = YES;
    NSInteger local = index - self.renderedStart;
    UITabBarItem *wanted = ![self hidesSelection] && local >= 0 && local < (NSInteger)self.nativeBar.items.count
        ? self.nativeBar.items[local] : nil;
    if (changed || (!self.actionOnly && self.nativeBar.selectedItem != wanted)) [self setNeedsLayout];
    // The host calls this while doing its own layout. Defer layout to UIKit
    // instead of synchronously refitting the native platter on every callback.
    if (animated && changed && !self.isSelectionTracking) [self setNeedsLayout];
}
- (void)scheduleTrackingCompletion {
    if (self.trackingLayoutPending) return;
    self.trackingLayoutPending = YES;
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
        ALBTopNativeGroup *group = weakSelf;
        if (!group) return;
        group.trackingLayoutPending = NO;
        [group setNeedsLayout];
        if (!group.isSelectionTracking) {
            [group layoutIfNeeded];
        }
    });
}
- (UIView *)nativePlatter {
    if (self.actionOnly) return [self actionAuxiliaryPlatter];
    UIView *cached = self.fittedPlatter;
    if (cached && !CGRectIsEmpty(cached.bounds) && [cached isDescendantOfView:self.nativeController.view]) return cached;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:self.nativeController.view];
    for (NSUInteger i = 0; i < queue.count && i < 100; i++) {
        UIView *view = queue[i];
        if ([NSStringFromClass(view.class) containsString:@"TabBarPlatterView"] && !CGRectIsEmpty(view.bounds)) return view;
        [queue addObjectsFromArray:view.subviews];
    }
    return nil;
}
- (BOOL)nativePlatterGeometryChanged {
    UIView *platter = [self nativePlatter];
    CGRect rect = platter ? [platter convertRect:platter.bounds toView:self.nativeController.view] : CGRectZero;
    return platter != self.fittedPlatter || !ALB6RectNear(rect, self.fittedPlatterRect);
}
- (void)useCompactFallbackSize:(CGSize)size {
    UIView *canvas = self.nativeController.view;
    canvas.transform = CGAffineTransformIdentity;
    canvas.bounds = CGRectMake(0, 0, size.width, MAX(83, size.height));
    [canvas setNeedsLayout]; [canvas layoutIfNeeded];
    CGRect bar = [self.nativeBar convertRect:self.nativeBar.bounds toView:canvas];
    canvas.center = CGPointMake(size.width / 2 - (CGRectGetMidX(bar)-CGRectGetMidX(canvas.bounds)),
        size.height / 2 - (CGRectGetMidY(bar)-CGRectGetMidY(canvas.bounds)));
    self.nativeScale = 1;
    self.glyphsDirty = YES;
    self.fittedSize = size;
    self.fittedPlatter = nil;
    self.fittedPlatterRect = CGRectZero;
}
- (void)fitNativeBarToSize:(CGSize)size {
    if (self.actionOnly) { [self fitAuxiliaryActionToSize:size]; return; }
    UIView *canvas = self.nativeController.view;
    BOOL newSize = !CGSizeEqualToSize(self.fittedSize, size);
    if (newSize) { self.providerFitPasses = 0; self.needsProviderFit = YES; }
    if (!self.needsProviderFit || self.providerFitPasses >= 2) return;
    // One initial fit and at most one late-provider refit for this external
    // size/trait/item generation. Native child layout is not an endless new
    // fitting input. Press/release keeps UIKit's own child transforms intact.
    self.needsProviderFit = NO;
    self.providerFitPasses++;
    self.glyphsDirty = YES;
    canvas.transform = CGAffineTransformIdentity;
    canvas.bounds = CGRectMake(0, 0, MAX(160, size.width + 40), 160);
    canvas.center = CGPointMake(size.width / 2, size.height / 2);
    [canvas setNeedsLayout]; [canvas layoutIfNeeded];
    UIView *platter = [self nativePlatter];
    if (!platter) { [self useCompactFallbackSize:size]; return; }
    for (NSUInteger pass = 0; pass < 3; pass++) {
        CGRect rect = [platter convertRect:platter.bounds toView:canvas];
        XHSQ100Fit fit = XHSQ100FitNativePlatter(size.width, size.height,
            CGRectGetWidth(canvas.bounds), rect.size.width, rect.size.height);
        if (!fit.valid || fabs(fit.width-CGRectGetWidth(canvas.bounds)) < 0.1) break;
        canvas.bounds = CGRectMake(0, 0, fit.width, 160);
        [canvas setNeedsLayout]; [canvas layoutIfNeeded];
        platter = [self nativePlatter] ?: platter;
    }
    CGRect rect = [platter convertRect:platter.bounds toView:canvas];
    XHSQ100Fit fit = XHSQ100FitNativePlatter(size.width, size.height,
        CGRectGetWidth(canvas.bounds), rect.size.width, rect.size.height);
    if (!fit.valid) { [self useCompactFallbackSize:size]; return; }
    self.nativeScale = fit.scale;
    canvas.transform = CGAffineTransformMakeScale(fit.scale, fit.scale);
    canvas.center = CGPointMake(size.width/2 -
        (CGRectGetMidX(rect)-CGRectGetMidX(canvas.bounds))*fit.scale,
        size.height/2 - (CGRectGetMidY(rect)-CGRectGetMidY(canvas.bounds))*fit.scale);
    self.fittedSize = size;
    self.fittedPlatter = platter;
    self.fittedPlatterRect = rect;
}
- (void)layoutSubviews {
    if (self.layoutInProgress) return;
    self.layoutInProgress = YES;
    @try {
    [super layoutSubviews];
    if (!self.window || CGRectIsEmpty(self.bounds) || self.isSelectionTracking) return;
    [self rebuildItemsIfNeeded];
    self.rail.frame = self.bounds;
    CGSize nativeSize = self.bounds.size;
    if (!CGSizeEqualToSize(self.rail.contentSize, nativeSize)) self.rail.contentSize = nativeSize;
    if (!CGPointEqualToPoint(self.rail.contentOffset, CGPointZero)) self.rail.contentOffset = CGPointZero;
    self.rail.scrollEnabled = NO;
    self.rail.clipsToBounds = NO;
    self.rail.bounces = NO;
    [self applyAppearance];
    [self fitNativeBarToSize:nativeSize];
    [self updateTitleGlyphsIfNeeded];
    NSInteger localSelection = _selectedIndex - self.renderedStart;
    BOOL select = ![self hidesSelection] && localSelection >= 0 && localSelection < (NSInteger)self.nativeBar.items.count;
    UITabBarItem *wanted = select ? self.nativeBar.items[localSelection] : nil;
    if (select) {
        if (localSelection < (NSInteger)self.nativeTabs.count) {
            id tab = self.nativeTabs[localSelection];
            SEL get = NSSelectorFromString(@"selectedTab"), set = NSSelectorFromString(@"setSelectedTab:");
            if (((id(*)(id,SEL))objc_msgSend)(self.nativeController,get) != tab)
                ((void(*)(id,SEL,id))objc_msgSend)(self.nativeController,set,tab);
        } else if (localSelection < (NSInteger)self.nativeController.viewControllers.count &&
                   self.nativeController.selectedIndex != (NSUInteger)localSelection)
            self.nativeController.selectedIndex = localSelection;
    }
    // Only override item selection for the explicit single-capsule preference;
    // controller selection still owns the active proxy and business route.
    if (!self.actionOnly && self.nativeBar.selectedItem != wanted) self.nativeBar.selectedItem = wanted;
    [self.nativeBar layoutIfNeeded];
    // UIKit can install its provider after items/appearance/window attachment.
    // One bounded recheck, then later ordinary provider layout invalidates cache.
    [self fitNativeBarToSize:nativeSize];
    [self updateTitleGlyphsIfNeeded];
    [self.nativeBar layoutIfNeeded];
    ALB5ApplyPlatterClear(self.nativeBar, self.forceClear);
    [self updateIdleActionLensVisibility];
    } @finally {
        self.layoutInProgress = NO;
    }
    ALBMaterialApplyBar(self.nativeBar, NO);
}
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    if (self.hidden || self.alpha < 0.01 || !self.userInteractionEnabled ||
        ![self pointInside:point withEvent:event]) return nil;
    return [super hitTest:point withEvent:event];
}
- (BOOL)tabBarController:(UITabBarController *)controller shouldSelectViewController:(UIViewController *)viewController {
    if (self.actionOnly) {
        if (controller.viewControllers.count>1 && viewController==controller.viewControllers.lastObject)
            [self dispatchNativeActionForSequence:self.touchObserver.actionSequenceLive ? self.touchObserver.actionSequence : 0];
        return NO;
    }
    return [self handleNativeItemSelection:viewController.tabBarItem];
}
- (BOOL)tabBarController:(UITabBarController *)controller shouldSelectTab:(id)tab {
    if (self.actionOnly) {
        if (tab==self.auxiliaryActionTab)
            [self dispatchNativeActionForSequence:self.touchObserver.actionSequenceLive ? self.touchObserver.actionSequence : 0];
        return NO; // search action, never a selected regular-tab capsule
    }
    NSUInteger index = [self.nativeTabs indexOfObjectIdenticalTo:tab];
    return index < self.nativeBar.items.count ? [self handleNativeItemSelection:self.nativeBar.items[index]] : NO;
}
// TEST23: action delivery is separate from native selection state. UIKit's
// delegate and the non-cancelling end observer claim the same physical tap.
- (void)dispatchNativeActionForSequence:(NSUInteger)sequence {
    if (ALBReproIsRecording()) ALBReproRecordEvent(@"top.action.dispatch",@{
        @"group":ALB30TopCaptureView(self), @"sequence":@(sequence),
        @"previousSequence":@(self.lastActionSequence), @"actionOnly":@(self.actionOnly),
        @"enabled":@(self.enabled), @"windowAttached":@(self.window!=nil),
        @"hasSelectionHandler":@(self.onSelection!=nil)});
    if (!self.actionOnly || !self.enabled || self.hidden || !self.window) return;
    if (sequence && self.lastActionSequence == sequence) return;
    if (sequence) self.lastActionSequence = sequence;
    if (self.onSelection) self.onSelection(0);
}
- (NSArray<NSString *> *)suppressedBackdropClassNames {
    NSMutableArray<NSString *> *names = [NSMutableArray array];
    for (UIView *view in self.idleLensOpacities.keyEnumerator)
        if (view) [names addObject:NSStringFromClass(view.class)];
    return names;
}
- (void)restoreIdleActionLenses {
    for (UIView *view in self.idleLensOpacities.keyEnumerator) {
        NSNumber *saved = [self.idleLensOpacities objectForKey:view];
        if (view && saved && view.layer.opacity != saved.floatValue)
            view.layer.opacity = saved.floatValue;
    }
    [self.idleLensOpacities removeAllObjects];
    self.nextIdleLensScan = 0;
}
- (void)updateIdleActionLensVisibility {
    // Auxiliary actions have no selected regular-tab capsule to suppress.
    // Never touch the native lens during tracking or release.
}
- (BOOL)handleNativeItemSelection:(UITabBarItem *)item {
    UITabBar *tabBar = self.nativeBar;
    NSUInteger renderedIndex = [tabBar.items indexOfObjectIdenticalTo:item];
    if (renderedIndex == NSNotFound || renderedIndex >= self.renderedTitles.count) return NO;
    // Titles are presentation only: GPS updates may rename the city while the
    // original item is tracking. Its stable slot still identifies the route.
    NSInteger index = item.tag;
    if (index != self.renderedStart + (NSInteger)renderedIndex || index < 0 || index >= (NSInteger)self.titles.count) {
        [self scheduleTrackingCompletion];
        return NO;
    }
    if (self.actionOnly) {
        // Actual native UITabButton has tracked the physical touch. The wrapper
        // dispatches business/menu actions only now; it never emulates tracking.
        NSUInteger sequence = self.touchObserver.actionSequenceLive ? self.touchObserver.actionSequence : 0;
        [self dispatchNativeActionForSequence:sequence];
        [self scheduleTrackingCompletion];
        return NO;
    }
    NSInteger previous = _selectedIndex;
    NSUInteger revision = self.externalSelectionRevision;
    BOOL accepted = !self.onSelection || self.onSelection(index);
    if (revision == self.externalSelectionRevision) _selectedIndex = accepted ? index : previous;
    if (accepted && _selectedIndex == index) {
        self.pendingWindowDirection = index > previous ? 1 : (index < previous ? -1 : 0);
        // Re-tapping the edge exposes the next native slot without invoking a
        // synthetic host swipe. Single-slot navigation uses the passive swipe.
        if (!self.pendingWindowDirection && self.effectiveVisibleCount > 1) {
            if (renderedIndex + 1 == self.nativeBar.items.count && index + 1 < (NSInteger)self.titles.count) self.pendingWindowDirection = 1;
            else if (renderedIndex == 0 && index > 0) self.pendingWindowDirection = -1;
        }
    }
    // UIKit keeps the successful native selection animation. A rejection or
    // synchronous host correction waits until the native touch has completed.
    [self setNeedsLayout];
    [self scheduleTrackingCompletion];
    if (accepted) [self sendActionsForControlEvents:UIControlEventValueChanged];
    return accepted;
}
- (void)requestGlobalSelection:(NSInteger)index {
    if (index < 0 || index >= (NSInteger)self.titles.count || index == _selectedIndex) return;
    NSInteger previous = _selectedIndex;
    NSUInteger revision = self.externalSelectionRevision;
    BOOL accepted = !self.onSelection || self.onSelection(index);
    if (revision == self.externalSelectionRevision) _selectedIndex = accepted ? index : previous;
    self.pendingWindowDirection = 0;
    [self scheduleTrackingCompletion];
    if (accepted) [self sendActionsForControlEvents:UIControlEventValueChanged];
}
- (void)accessibilityIncrement { [self requestGlobalSelection:self.selectedIndex + 1]; }
- (void)accessibilityDecrement { [self requestGlobalSelection:self.selectedIndex - 1]; }
- (NSDictionary<NSString *,id> *)diagnosticSummary {
    UIView *platter = [self nativePlatter];
    CGRect platterRect = platter ? [platter convertRect:platter.bounds toView:self] : CGRectZero;
    NSMutableArray<NSString *> *classes = [NSMutableArray array];
    NSMutableArray<NSDictionary *> *glyphs = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:self.nativeController.view];
    for (NSUInteger index = 0; index < queue.count && index < 100; index++) {
        UIView *view = queue[index];
        NSString *name = NSStringFromClass(view.class);
        if (![classes containsObject:name]) [classes addObject:name];
        if ([view isKindOfClass:UIImageView.class] && ((UIImageView *)view).image && glyphs.count < 16) {
            UIImageView *imageView = (UIImageView *)view;
            [glyphs addObject:@{@"class":name,
                @"frameInGroup":NSStringFromCGRect([view convertRect:view.bounds toView:self]),
                @"imageSize":NSStringFromCGSize(imageView.image.size),
                @"imageScale":@(imageView.image.scale), @"contentMode":@(imageView.contentMode),
                @"hidden":@(view.hidden), @"alpha":@(view.alpha)}];
        }
        [queue addObjectsFromArray:view.subviews];
    }
    NSUInteger selected = self.nativeBar.selectedItem ?
        [self.nativeBar.items indexOfObjectIdenticalTo:self.nativeBar.selectedItem] : NSNotFound;
    NSMutableArray<NSString *> *diagnosticTitles = [self.renderedTitles mutableCopy];
    if (diagnosticTitles.count > 2 && ![@[@"同城", @"附近"] containsObject:diagnosticTitles[2]])
        diagnosticTitles[2] = @"<dynamic-city>";
    NSMutableArray<NSDictionary *> *moduleGeometry = [NSMutableArray array];
    UIView *moduleContainer = self.superview.superview;
    for (UIView *wrapper in moduleContainer.subviews) {
        NSString *identifier = wrapper.accessibilityIdentifier ?: @"";
        if (![identifier hasPrefix:@"AwemeLiquidBar.HomeTopSlot."] &&
            ![identifier isEqualToString:@"AwemeLiquidBar.TopLiquidCategoryGroup"]) continue;
        NSMutableArray<NSDictionary *> *controls = [NSMutableArray array];
        for (UIView *control in wrapper.subviews) {
            if (![control isKindOfClass:UIButton.class] || control.hidden) continue;
            [controls addObject:@{@"class":NSStringFromClass(control.class),
                @"bounds":NSStringFromCGRect(control.bounds),
                @"center":NSStringFromCGPoint(control.center),
                @"frameInContainer":NSStringFromCGRect([control convertRect:control.bounds toView:moduleContainer]),
                @"nativePressTransform":NSStringFromCGAffineTransform(control.transform)}];
        }
        [moduleGeometry addObject:@{@"id":identifier,
            @"frameInContainer":NSStringFromCGRect([wrapper convertRect:wrapper.bounds toView:moduleContainer]),
            @"bounds":NSStringFromCGRect(wrapper.bounds),
            @"center":NSStringFromCGPoint(wrapper.center),
            @"moduleTransform":NSStringFromCGAffineTransform(wrapper.transform),
            @"controls":controls}];
    }
    return @{@"implementation":@"test24-native-auxiliary-actions",
        @"actionOnly":@(self.actionOnly),
        @"keepsIdleActionCapsule":@(self.keepsSingleSelectionCapsule),
        @"suppressedIdleLensCount":@(self.idleLensOpacities.count),
        @"suppressionPolicy":@"UISearchTab auxiliary; no selected regular-tab lens",
        @"suppressedBackdropClasses":[self suppressedBackdropClassNames],
        @"lastActionSequence":@(self.lastActionSequence),
        @"topModuleGeometry":moduleGeometry,
        @"systemVersion":UIDevice.currentDevice.systemVersion ?: @"unknown",
        @"nativeBarClass":NSStringFromClass(self.nativeBar.class),
        @"nativeControllerClass":NSStringFromClass(self.nativeController.class),
        @"nativeControllerAttached":@(self.nativeController.parentViewController != nil),
        @"providerStatus":platter ? @"floating-platter-observed" : @"floating-provider-unconfirmed",
        @"materialHelper":@"ALB5ApplyBottomBarSurface (shared with bottom bar)",
        @"platterClass":platter ? NSStringFromClass(platter.class) : @"missing",
        @"platterInGroup":NSStringFromCGRect(platterRect),
        @"groupFrame":NSStringFromCGRect(self.frame), @"groupBounds":NSStringFromCGRect(self.bounds),
        @"nativeBounds":NSStringFromCGRect(self.nativeBar.bounds),
        @"nativeFrame":NSStringFromCGRect(self.nativeBar.frame),
        @"nativeScale":@(self.nativeScale), @"titlePointSizeTarget":@(self.titleFont.pointSize),
        @"glyphDisplayScale":@(self.glyphDisplayScale),
        @"itemCount":@(self.nativeBar.items.count), @"hostItemCount":@(self.titles.count),
        @"windowStart":@(self.renderedStart), @"railOffset":NSStringFromCGPoint(self.rail.contentOffset),
        @"titles":diagnosticTitles,
        @"selectedIndex":@(_selectedIndex), @"nativeSelectedIndex":@(selected == NSNotFound ? -1 : self.nativeBar.items[selected].tag),
        @"tracking":@(self.isSelectionTracking), @"externalSelectionRevision":@(self.externalSelectionRevision),
        @"inWindow":@(self.window != nil), @"hidden":@(self.hidden),
        @"userInterfaceStyle":@(self.traitCollection.userInterfaceStyle),
        @"reduceTransparency":@(UIAccessibilityIsReduceTransparencyEnabled()),
        @"reduceMotion":@(UIAccessibilityIsReduceMotionEnabled()),
        @"viewClasses":classes, @"imageViews":glyphs};
}
- (CGSize)intrinsicContentSize {
    NSInteger count = [self effectiveVisibleCount];
    return CGSizeMake(count == 1 ? 96 : MAX(148, count * 74), 44);
}
+ (CGRect)frameInBounds:(CGRect)bounds visibleCount:(NSInteger)count leftReserve:(CGFloat)left rightReserve:(CGFloat)right {
    count = MAX(1, MIN(4, count));
    CGFloat available = MAX(0, CGRectGetWidth(bounds) - 2 * MAX(0, MAX(left, right)));
    if (available <= 0 || CGRectIsEmpty(bounds)) return CGRectZero;
    CGFloat width = MIN(available, MAX(count == 1 ? 92 : 148, count * (count == 1 ? 96 : 74)));
    return CGRectMake(CGRectGetMidX(bounds) - width / 2, CGRectGetMidY(bounds) - 22, width, 44);
}
@end


