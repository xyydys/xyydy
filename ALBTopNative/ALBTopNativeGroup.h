#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/// Stable layout for our control wrappers. Bounds/center remain independent
/// from the native UIButton's press or menu-morph transform.
FOUNDATION_EXPORT void ALB7LayoutTopControlInBounds(UIView * _Nullable control, CGRect bounds);
/// Natural side centers are supplied in container coordinates. All three
/// wrappers share one vertical baseline and shrink about that same center.
FOUNDATION_EXPORT void ALB7ApplyTopModuleGeometry(UIView *container,
    UIView * _Nullable group, UIView * _Nullable left, UIView * _Nullable right,
    CGPoint leftNatural, CGPoint rightNatural, CGFloat scale);

FOUNDATION_EXPORT UIView * _Nullable ALB8TopModuleHost(UIView * _Nullable container);
FOUNDATION_EXPORT UIView * _Nullable ALB8TopOriginalContainer(UIView * _Nullable view);
FOUNDATION_EXPORT UIColor * _Nullable ALB8NativeTopTitleColor(UIView * _Nullable root, NSString *title);
FOUNDATION_EXPORT BOOL ALB8IsTopChromeController(UIViewController *controller);

/// One controller-owned interactive UITabBar with transparent proxy content.
/// Native item selection owns the lens and gestures; no segmented overlay.
/// Main thread only, including diagnosticSummary.
@interface ALBTopNativeGroup : UIControl
/// Display labels for stable route slots; renaming a label never changes its route.
// Action slots use the identical controller/material path, replacing only glyphs.
@property(nonatomic, strong, nullable) UIImage *actionImage;
@property(nonatomic) BOOL actionOnly;
@property(nonatomic) BOOL forceClear;
/// The controller-owned native bar (read-only; 2.2 iOS 27 Clear lookup).
@property(nonatomic, readonly, strong, nullable) UITabBar *nativeBar;
@property(nonatomic, copy) NSArray<NSString *> *titles;
@property(nonatomic) NSInteger visibleItemCount; // 1...4; native slots form a movable window over all routes
@property(nonatomic, readonly) BOOL isSelectionTracking;
@property(nonatomic) BOOL usesNativeTitleFallback; // Host compatibility; all items remain native.
@property(nonatomic) NSInteger selectedIndex; // -1 means no selection
@property(nonatomic) BOOL keepsSingleSelectionCapsule;
@property(nonatomic) BOOL automaticContrastEnabled; // Host compatibility; UIKit owns the material.
@property(nonatomic, copy) NSArray<UIColor *> *titleForegroundColors; // Original ink per host route, selected or not.
@property(nonatomic, strong) UIColor *fallbackForegroundColor;
@property(nonatomic, strong) UIColor *selectionAccentColor; // Same selected-item tint as the bottom bar.
@property(nonatomic, strong) UIFont *titleFont; // Final visible glyph target: ALB's 14pt semibold.

/// NO rejects the switch. YES leaves confirmation/rollback to the host.
@property(nonatomic, copy, nullable) BOOL (^onSelection)(NSInteger index);
/// Fired at the first physical touch, before any menu/search delivery. Hosts
/// may snapshot an action-delivery generation for exactly-once fallback.
@property(nonatomic, copy, nullable) void (^onTrackingBegan)(void);
/// Fired after the real native touch ends or cancels; never a synthetic tap.
@property(nonatomic, copy, nullable) void (^onTrackingFinished)(void);
/// Read-only, bounded layout snapshot. Does not take screenshots or run timers.
/// A missing floating platter is explicitly reported as unconfirmed.
@property(nonatomic, readonly, copy) NSDictionary<NSString *, id> *diagnosticSummary;

/// Page-to-bar synchronization; never dispatches onSelection/valueChanged.
/// Corrections arriving during tracking are applied after the touch completes.
- (void)setSelectedIndex:(NSInteger)index animated:(BOOL)animated;

/// ALB footprint: 44pt high, 74pt/item (96pt for one), symmetric side reserves.
+ (CGRect)frameInBounds:(CGRect)bounds visibleCount:(NSInteger)count
           leftReserve:(CGFloat)left rightReserve:(CGFloat)right;
@end

NS_ASSUME_NONNULL_END
