#pragma once
#import <UIKit/UIKit.h>
#include "ALBMaterialPolicy.h"
#include <stdint.h>
#import "ALBReproCaptureSignal.h"

// Import these as C functions. @_silgen_name only renames Swift symbols and
// cannot establish the C ABI for CGRect, Objective-C references, or BOOL.
NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT void ALB5UpdateBottomGeometry(UITabBar *bar, UIView *host,
                                               BOOL compact, BOOL animated)
    NS_SWIFT_NAME(ALB5UpdateBottomGeometry(_:_:_:_:));
FOUNDATION_EXPORT void ALB6RestoreBottomGeometry(UITabBar *bar)
    NS_SWIFT_NAME(ALB6RestoreBottomGeometry(_:));
FOUNDATION_EXPORT BOOL ALB8BeginBottomLayout(UITabBar *bar)
    NS_SWIFT_NAME(ALB8BeginBottomLayout(_:));
FOUNDATION_EXPORT void ALB8EndBottomLayout(UITabBar *bar)
    NS_SWIFT_NAME(ALB8EndBottomLayout(_:));
FOUNDATION_EXPORT BOOL ALB8ExpandOnLeaveFeed(void)
    NS_SWIFT_NAME(ALB8ExpandOnLeaveFeed());
FOUNDATION_EXPORT void ALB8SetBottomVisibleFraction(UITabBar *bar, CGFloat fraction)
    NS_SWIFT_NAME(ALB8SetBottomVisibleFraction(_:_:));
FOUNDATION_EXPORT CGRect ALB5CompactHitRect(UITabBar *bar)
    NS_SWIFT_NAME(ALB5CompactHitRect(_:));
FOUNDATION_EXPORT BOOL ALB7BottomHasSurfacePair(UITabBar *bar)
    NS_SWIFT_NAME(ALB7BottomHasSurfacePair(_:));
FOUNDATION_EXPORT NSString *ALB6BottomGeometryStatus(UITabBar *bar)
    NS_SWIFT_NAME(ALB6BottomGeometryStatus(_:));
FOUNDATION_EXPORT int32_t AWEAlbumMorphRequestNativeMinimized(UIViewController *controller,
                                                            int32_t minimized,
                                                            int32_t animated)
    NS_SWIFT_NAME(AWEAlbumMorphRequestNativeMinimized(_:_:_:));
FOUNDATION_EXPORT int32_t ALB15NativeMinimizedState(UIViewController *controller)
    NS_SWIFT_NAME(ALB15NativeMinimizedState(_:));
FOUNDATION_EXPORT int32_t ALB15ObserveCompactGeometry(UITabBar *bar)
    NS_SWIFT_NAME(ALB15ObserveCompactGeometry(_:));
FOUNDATION_EXPORT NSInteger ALB15CompactStyle(void) NS_SWIFT_NAME(ALB15CompactStyle());
FOUNDATION_EXPORT CGFloat ALB15BottomScale(void) NS_SWIFT_NAME(ALB15BottomScale());
FOUNDATION_EXPORT UIImage * _Nullable ALB8OwnAvatar(void) NS_SWIFT_NAME(ALB8OwnAvatar());
FOUNDATION_EXPORT void ALB5CompactStateChanged(BOOL compact)
    NS_SWIFT_NAME(ALB5CompactStateChanged(_:));
FOUNDATION_EXPORT void AWEAlbumMorphProbeLog(const char *message)
    NS_SWIFT_NAME(AWEAlbumMorphProbeLog(_:));
FOUNDATION_EXPORT int32_t AWEAlbumMorphHideTitlesEnabled(void)
    NS_SWIFT_NAME(AWEAlbumMorphHideTitlesEnabled());

// Returns the retained UIImage as an opaque pointer; Swift consumes exactly
// one retain through Unmanaged<UIImage>.takeRetainedValue().
FOUNDATION_EXPORT void * _Nullable AWEAlbumMorphCreateConfiguredImage(
    const char *keyCString, const char *fallbackCString, double pointSize)
    NS_SWIFT_NAME(AWEAlbumMorphCreateConfiguredImage(_:_:_:));

FOUNDATION_EXPORT void ALB49TraceNativeScale(UITabBar *bar, const char *phase)
    NS_SWIFT_NAME(ALB49TraceNativeScale(_:_:));
FOUNDATION_EXPORT void ALB50ApplyBottomClearFromController(UITabBar *bar)
    NS_SWIFT_NAME(ALB50ApplyBottomClearFromController(_:));
NS_ASSUME_NONNULL_END
