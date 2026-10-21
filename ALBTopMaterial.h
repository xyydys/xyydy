#pragma once
#import "ALBUnifiedMaterial.h"
// The original UIButton retains menu, target/actions and native interaction.
@interface ALB42TopMaterialButton : UIButton
@end
@implementation ALB42TopMaterialButton
- (void)layoutSubviews { [super layoutSubviews]; ALBMaterialApplyButton(self, YES); }
- (void)traitCollectionDidChange:(UITraitCollection *)previous { [super traitCollectionDidChange:previous]; ALBMaterialApplyButton(self, YES); }
@end
