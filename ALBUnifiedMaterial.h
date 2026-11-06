#pragma once
#import <UIKit/UIKit.h>
NS_ASSUME_NONNULL_BEGIN
/// 3.0: one fixed material for LiquidBar's own glass that is not bar glass
/// (comments, messages, danmaku, the double-tap panel's tracks, settings):
/// Liquid Glass Regular in Dark. It follows neither the system appearance
/// nor the page.
FOUNDATION_EXPORT UIUserInterfaceStyle ALBMaterialStyle(UIView * _Nullable view, BOOL feedEligible);
FOUNDATION_EXPORT UIVisualEffect * _Nullable ALBMaterialMakeEffect(BOOL interactive);
FOUNDATION_EXPORT UIButtonConfiguration *ALBMaterialButtonConfiguration(void);
FOUNDATION_EXPORT void ALBMaterialApplyButton(UIButton *button, BOOL feedEligible);
FOUNDATION_EXPORT void ALBMaterialApplyEffectView(UIVisualEffectView *view, BOOL feedEligible);
NS_ASSUME_NONNULL_END
