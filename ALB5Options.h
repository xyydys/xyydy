#pragma once
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include <math.h>
static NSString *ALB5Key(NSString *name) { return [@"AwemeLiquidBar.TEST5." stringByAppendingString:name]; }
static BOOL ALB5Bool(NSString *name, BOOL fallback) {
    id value = [NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(name)];
    return value ? [value boolValue] : fallback;
}
static CGFloat ALB5Number(NSString *name, CGFloat fallback, CGFloat low, CGFloat high) {
    id value = [NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(name)];
    double number = value ? [value doubleValue] : fallback;
    return isfinite(number) ? MIN(high, MAX(low, number)) : fallback;
}
static UIColor *ALB5Color(NSString *name, UIColor *fallback) {
    NSArray *v = [NSUserDefaults.standardUserDefaults arrayForKey:ALB5Key(name)];
    if (v.count != 4) return fallback;
    for (id n in v) if (![n isKindOfClass:NSNumber.class] || !isfinite([n doubleValue])) return fallback;
    return [UIColor colorWithRed:MIN(1,MAX(0,[v[0] doubleValue])) green:MIN(1,MAX(0,[v[1] doubleValue]))
        blue:MIN(1,MAX(0,[v[2] doubleValue])) alpha:MIN(1,MAX(0,[v[3] doubleValue]))];
}
static UIColor *ALB5TopNativeInk;
static UIColor *ALB5NativeTitleColor(UIView *root) {
    if (!root) return nil;
    if ([root isKindOfClass:UIButton.class]) {
        UIButton *button=(UIButton *)root;NSAttributedString *text=button.currentAttributedTitle;
        UIColor *ink=text.length?[text attribute:NSForegroundColorAttributeName atIndex:0 effectiveRange:NULL]:nil;
        return [ink isKindOfClass:UIColor.class]?ink:button.currentTitleColor;
    }
    NSMutableArray *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i=0;i<queue.count && i<24;i++) {
        UIView *v=queue[i];
        if ([v isKindOfClass:UILabel.class] && ((UILabel *)v).text.length) {
            UILabel *label=(UILabel *)v;NSAttributedString *text=label.attributedText;
            UIColor *ink=text.length?[text attribute:NSForegroundColorAttributeName atIndex:0 effectiveRange:NULL]:nil;
            return [ink isKindOfClass:UIColor.class]?ink:label.textColor;
        }
        [queue addObjectsFromArray:v.subviews];
    }
    return nil;
}
// A controller-managed floating platter may be a sibling of UITabBar. Keep a
// weak reference to the controller's resident root instead of searching the
// whole application window. Selector keys are shared by all translation units.
static void ALB5SetNativeSurfaceRoot(UITabBar *bar, UIView *root) {
    if (!bar) return;
    __weak UIView *weakRoot=root;
    UIView *(^resolver)(void)=^{ return weakRoot; };
    objc_setAssociatedObject(bar,NSSelectorFromString(@"ALB6.NativeSurfaceRoot"),
        root ? [resolver copy] : nil,OBJC_ASSOCIATION_COPY_NONATOMIC);
}
static UIView *ALB5NativeSurfaceRoot(UITabBar *bar) {
    UIView *(^resolver)(void)=objc_getAssociatedObject(bar,
        NSSelectorFromString(@"ALB6.NativeSurfaceRoot"));
    UIView *root=resolver ? resolver() : nil;
    return root ?: bar;
}
// Restore the uploaded 1.0.58 PageFixedGlass v9 Photos-controller contract.
// Its material is created by UIKit; the source code never creates a blur/glass
// effect or assigns a tab appearance to this controller. Clear host backgrounds
// are distinct from forcing UIGlassEffectStyleClear. Keep the native provider,
// selection lens and interaction hierarchy alive on the bottom AND top bars.
static void ALB5ApplySharedBottomSurface(UITabBar *bar) {
    if (!bar) return;
    UIColor *clear=UIColor.clearColor;
    if(![bar.backgroundColor isEqual:clear])bar.backgroundColor=clear;
    if(!bar.layer.backgroundColor || !CGColorEqualToColor(bar.layer.backgroundColor,clear.CGColor))bar.layer.backgroundColor=clear.CGColor;
    if(bar.opaque)bar.opaque=NO;
    if(!bar.translucent)bar.translucent=YES;
    if (NSProcessInfo.processInfo.operatingSystemVersion.majorVersion>=26) {
        // Do not configureWithDefaultBackground here, even on the compact top
        // bar. TEST6 created an explicit material appearance for that bar while
        // claiming to share the untouched Photos controller surface. Foreground
        // setters may copy the current appearance without altering background
        // fields; this helper must not install its own resolved material.
        return;
    }
    const void *key=NSSelectorFromString(@"ALB6.NativeMaterialAppearance");
    if (objc_getAssociatedObject(bar,key)==bar.standardAppearance) return;
    UITabBarAppearance *previous=bar.standardAppearance;
    UITabBarAppearance *a=[UITabBarAppearance new];
    [a configureWithTransparentBackground];
    a.backgroundColor=UIColor.clearColor;
    a.shadowColor=UIColor.clearColor;
    bar.barTintColor=UIColor.clearColor;
    bar.backgroundImage=[UIImage new];
    bar.shadowImage=[UIImage new];
    if (previous) {
        a.stackedLayoutAppearance=[previous.stackedLayoutAppearance copy];
        a.inlineLayoutAppearance=[previous.inlineLayoutAppearance copy];
        a.compactInlineLayoutAppearance=[previous.compactInlineLayoutAppearance copy];
    }
    bar.standardAppearance=a;
    if (@available(iOS 15.0,*)) bar.scrollEdgeAppearance=a;
    objc_setAssociatedObject(bar,key,bar.standardAppearance,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
// TEST6 retires both forced-Clear options. A hidden preference from TEST5 must
// not silently replace the restored UIKit surface after its next layout.
// Retained as a source-compatible no-op for the imported top component.
static void ALB5ApplyPlatterClear(__unused UITabBar *bar, __unused BOOL enabled) {}

static NSArray<NSNumber *> *ALB28PaletteActionOrder(void) {
    NSArray *defaults=@[@0,@1,@9,@2,@3,@10,@11,@12,@4,@5,@6,@7,@8];
    id saved=[NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"panel.actionOrder")];
    NSMutableArray *order=[NSMutableArray arrayWithCapacity:defaults.count];
    for(id value in [saved isKindOfClass:NSArray.class]?saved:@[]) {
        if(![value isKindOfClass:NSNumber.class])continue;
        NSNumber *key=@([value integerValue]);
        if([defaults containsObject:key] && ![order containsObject:key])[order addObject:key];
    }
    for(NSNumber *key in defaults)if(![order containsObject:key])[order addObject:key];
    return order;
}
