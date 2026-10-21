#pragma once
#import <UIKit/UIKit.h>
NS_ASSUME_NONNULL_BEGIN
// Return the actual native tab surface effect, never the selection lens.
FOUNDATION_EXPORT UIVisualEffect * _Nullable ALB15NativeGlassEffect(UIView * _Nullable root);
FOUNDATION_EXPORT NSString *ALB15GlassSignature(UIVisualEffect *effect, UITraitCollection *traits);
FOUNDATION_EXPORT UIVisualEffect * _Nullable ALB15CopyInteractiveGlass(UIVisualEffect *effect, BOOL interactive);
FOUNDATION_EXPORT BOOL ALB15MergeUpdate(UITabBar *bar, UIView *host, UIView *left, UIView *right,
                                      CGRect leftRect, CGRect rightRect, BOOL compact, BOOL animated);
FOUNDATION_EXPORT void ALB15MergeReset(UITabBar *bar);
FOUNDATION_EXPORT UIView * _Nullable ALB15MergeHit(UITabBar *bar, CGPoint point, UIEvent * _Nullable event);
FOUNDATION_EXPORT CGRect ALB15MergeHitRect(UITabBar *bar);
FOUNDATION_EXPORT BOOL ALB15MergeVisible(UITabBar *bar);
FOUNDATION_EXPORT NSString *ALB15MergeStatus(UITabBar *bar);
FOUNDATION_EXPORT NSString *ALB15TopGlassStatus(UIView *source);
FOUNDATION_EXPORT void ALB15TopMaterialDidLayout(UIView *source);
FOUNDATION_EXPORT BOOL ALB15TopButtonManaged(UIButton *button);
FOUNDATION_EXPORT void ALB15UpdateTopButtons(UIView *source, UIView *leftSlot, UIButton *leftButton,
                                           UIView *rightSlot, UIButton *rightButton);
FOUNDATION_EXPORT void ALB15ExpandCompactBar(UITabBar *bar);
FOUNDATION_EXPORT UIImage * _Nullable ALB8OwnAvatar(void);
NS_ASSUME_NONNULL_END
