#import "ALBUnifiedMaterial.h"
#import "ALB25Material.h"
#import <objc/runtime.h>
#import <objc/message.h>

// 3.0 — the material of every LiquidBar glass that is not bar glass.
// Bar glass (the top chrome, the right rail, the double-tap panel, the
// bottom UITabBar) is UIKit's own and only fixed to Dark by its owner.

UIUserInterfaceStyle ALBMaterialStyle(UIView *view, BOOL feedEligible) {
    (void)view; (void)feedEligible;
    return UIUserInterfaceStyleDark;
}

// Runtime dispatch: UIGlassEffect exists from iOS 26 (style 0 = Regular).
UIVisualEffect *ALBMaterialMakeEffect(BOOL interactive) {
    // 3.9: the Clear recipe for every non-bar glass while lifted.
    if (ALB25MaterialGlassLifted()) return ALB25MaterialLiftedEffect(interactive);
    Class glassClass = NSClassFromString(@"UIGlassEffect");
    SEL maker = NSSelectorFromString(@"effectWithStyle:");
    if (glassClass && [glassClass respondsToSelector:maker]) {
        UIVisualEffect *effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(glassClass, maker, 0);
        SEL setInteractive = NSSelectorFromString(@"setInteractive:");
        if ([effect respondsToSelector:setInteractive])
            ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, setInteractive, interactive);
        if (effect) return effect;
    }
    return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemThinMaterialDark];
}

UIButtonConfiguration *ALBMaterialButtonConfiguration(void) {
    SEL maker = NSSelectorFromString(@"glassButtonConfiguration");
    Class configurationClass = UIButtonConfiguration.class;
    if (![configurationClass respondsToSelector:maker]) return [UIButtonConfiguration plainButtonConfiguration];
    UIButtonConfiguration *configuration = ((id (*)(id, SEL))objc_msgSend)(configurationClass, maker);
    // 3.5: the glass object the right column draws ("right.glassSource"; the
    // bottom bar's own by default), not the button's own Regular glass.
    if (ALB25MaterialGlassLifted()) ALB25MaterialInstallLiftedGlass(configuration);
    else ALB25MaterialInstallRegisteredGlass(configuration);
    return configuration;
}

void ALBMaterialApplyButton(UIButton *button, BOOL feedEligible) {
    (void)feedEligible;
    if (button && button.overrideUserInterfaceStyle != UIUserInterfaceStyleDark)
        button.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
}

void ALBMaterialApplyEffectView(UIVisualEffectView *view, BOOL feedEligible) {
    (void)feedEligible;
    if (![view isKindOfClass:UIVisualEffectView.class]) return;
    if (view.overrideUserInterfaceStyle != UIUserInterfaceStyleDark)
        view.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
}
