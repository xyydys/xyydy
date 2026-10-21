#pragma once
#import <UIKit/UIKit.h>
#import <objc/message.h>

#pragma mark - System Liquid Glass bridge

// Apple public UIKit enum values (iOS 26+):
// UIGlassEffectStyleRegular = 0
// UIGlassEffectStyleClear   = 1
// Runtime dispatch keeps this small reference tweak buildable even with an older Theos SDK.
typedef NS_ENUM(NSInteger, AGLGlassStyle) {
    AGLGlassStyleRegular = 0,
    AGLGlassStyleClear = 1,
};

static BOOL AGLSystemGlassAvailable(void) {
    Class cls = NSClassFromString(@"UIGlassEffect");
    return cls && [cls respondsToSelector:NSSelectorFromString(@"effectWithStyle:")];
}

static UIVisualEffect *AGLMakeSystemGlass(AGLGlassStyle style, UIColor *tintColor, BOOL interactive) {
    Class cls = NSClassFromString(@"UIGlassEffect");
    SEL make = NSSelectorFromString(@"effectWithStyle:");
    if (cls && [cls respondsToSelector:make]) {
        id effect = ((id (*)(id, SEL, NSInteger))objc_msgSend)(cls, make, style);
        SEL setTint = NSSelectorFromString(@"setTintColor:");
        SEL setInteractive = NSSelectorFromString(@"setInteractive:");
        if ([effect respondsToSelector:setTint]) {
            ((void (*)(id, SEL, id))objc_msgSend)(effect, setTint, tintColor);
        }
        if ([effect respondsToSelector:setInteractive]) {
            ((void (*)(id, SEL, BOOL))objc_msgSend)(effect, setInteractive, interactive);
        }
        return effect;
    }

    // The package can load on older systems without crashing, but the fallback is clearly
    // identified as NOT a Liquid Glass test in the status line.
    return [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterial];
}


