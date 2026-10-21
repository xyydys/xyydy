#pragma once
#import <UIKit/UIKit.h>
#include "ALBMaterialPolicy.h"
NS_ASSUME_NONNULL_BEGIN
FOUNDATION_EXPORT NSInteger ALBMaterialMode(void);
FOUNDATION_EXPORT BOOL ALBMaterialEnabled(void);
FOUNDATION_EXPORT NSUInteger ALBMaterialGeneration(void);
FOUNDATION_EXPORT UIUserInterfaceStyle ALBMaterialStyle(UIView * _Nullable view, BOOL feedEligible);
FOUNDATION_EXPORT void ALBMaterialInstallResolvers(BOOL (^feed)(void), UIUserInterfaceStyle (^app)(void));
FOUNDATION_EXPORT UIVisualEffect * _Nullable ALBMaterialMakeEffect(BOOL interactive);
FOUNDATION_EXPORT UIButtonConfiguration *ALBMaterialButtonConfiguration(void);
FOUNDATION_EXPORT void ALBMaterialApplyButton(UIButton *button, BOOL feedEligible);
FOUNDATION_EXPORT void ALBMaterialApplyEffectView(UIVisualEffectView *view, BOOL feedEligible);
FOUNDATION_EXPORT void ALBMaterialApplyBar(UITabBar *bar, BOOL bottom);
FOUNDATION_EXPORT void ALBMaterialRefresh(void);
FOUNDATION_EXPORT NSDictionary *ALBMaterialSnapshot(void);
@interface ALBMaterialSettingsController : UITableViewController
@end
NS_ASSUME_NONNULL_END
