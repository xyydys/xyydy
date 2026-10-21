#import "ALB15NativeGlass.h"
#import <objc/message.h>
#import <objc/runtime.h>

UIVisualEffect *ALB15NativeGlassEffect(UIView *root) {
    if (!root) return nil;
    Class glassClass=NSClassFromString(@"UIGlassEffect");
    if (!glassClass) return nil;
    NSMutableArray<UIView *> *queue=[NSMutableArray arrayWithObject:root];
    SEL getter=NSSelectorFromString(@"glassEffect");
    // Never copy selected capsule/lens materials as the outer material.
    for(NSUInteger i=0;i<queue.count && i<100;i++) {
        UIView *v=queue[i]; NSString *name=NSStringFromClass(v.class);
        if([name containsString:@"Lens"] || [v.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar.NativeMerge."])continue;
        id effect=nil;
        @try {
            NSMethodSignature *signature=[v methodSignatureForSelector:getter];
            if([v respondsToSelector:getter] && signature.numberOfArguments==2 && signature.methodReturnType[0]=='@')
                effect=((id(*)(id,SEL))objc_msgSend)(v,getter);
            if(!effect && [v isKindOfClass:UIVisualEffectView.class])effect=((UIVisualEffectView *)v).effect;
        } @catch(__unused NSException *e) { effect=nil; }
        if([effect isKindOfClass:glassClass]) return effect;
        [queue addObjectsFromArray:v.subviews];
    }
    return nil;
}
UIVisualEffect *ALB15CopyInteractiveGlass(UIVisualEffect *effect,BOOL interactive) {
    if(!effect || ![effect conformsToProtocol:@protocol(NSCopying)])return nil;
    UIVisualEffect *copy=[effect copy]; SEL set=NSSelectorFromString(@"setInteractive:");
    if(![copy respondsToSelector:set])return nil;
    ((void(*)(id,SEL,BOOL))objc_msgSend)(copy,set,interactive);
    return copy;
}
static const void *ALB15MergeKey(void){return NSSelectorFromString(@"ALB15.SystemMerge.Instance");}
static id ALB15Merge(UITabBar *bar,BOOL create) {
    if(!NSClassFromString(@"UIGlassContainerEffect"))return nil;
    id value=objc_getAssociatedObject(bar,ALB15MergeKey());
    if(!value && create) {
        Class cls=NSClassFromString(@"ALB15SystemMerge");
        if(cls)value=[[cls alloc]init];
        if(value)objc_setAssociatedObject(bar,ALB15MergeKey(),value,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return value;
}
BOOL ALB15MergeUpdate(UITabBar *bar,UIView *host,UIView *left,UIView *right,
                     CGRect lr,CGRect rr,BOOL compact,BOOL animated) {
    id merge=ALB15Merge(bar,YES);
    SEL selector=NSSelectorFromString(@"updateWithBar:host:left:right:leftRect:rightRect:compact:animated:");
    if(![merge respondsToSelector:selector])return NO;
    return ((BOOL(*)(id,SEL,id,id,id,id,CGRect,CGRect,BOOL,BOOL))objc_msgSend)(merge,selector,bar,host,left,right,lr,rr,compact,animated);
}
void ALB15MergeReset(UITabBar *bar) {
    id merge=ALB15Merge(bar,NO);if(merge)((void(*)(id,SEL))objc_msgSend)(merge,NSSelectorFromString(@"reset"));
}
UIView *ALB15MergeHit(UITabBar *bar,CGPoint point,UIEvent *event) {
    id merge=ALB15Merge(bar,NO);SEL s=NSSelectorFromString(@"hitAt:event:");
    return merge?((id(*)(id,SEL,CGPoint,id))objc_msgSend)(merge,s,point,event):nil;
}
CGRect ALB15MergeHitRect(UITabBar *bar) {
    id merge=ALB15Merge(bar,NO);SEL s=NSSelectorFromString(@"hitRect");
    return merge?((CGRect(*)(id,SEL))objc_msgSend)(merge,s):CGRectNull;
}
BOOL ALB15MergeVisible(UITabBar *bar) {
    id merge=ALB15Merge(bar,NO);
    return merge?((BOOL(*)(id,SEL))objc_msgSend)(merge,NSSelectorFromString(@"active")):NO;
}
NSString *ALB15MergeStatus(UITabBar *bar) {
    id merge=ALB15Merge(bar,NO);
    return merge?((id(*)(id,SEL))objc_msgSend)(merge,NSSelectorFromString(@"status")):@"等待原生融合对象";
}
BOOL ALB15TopButtonManaged(UIButton *button) {
    return [objc_getAssociatedObject(button,NSSelectorFromString(@"ALB15.TopEffectManaged")) boolValue];
}
void ALB15UpdateTopButtons(UIView *source,UIView *leftSlot,UIButton *leftButton,UIView *rightSlot,UIButton *rightButton) {
    if(!source || !leftSlot || !rightSlot || !leftButton || !rightButton || !NSClassFromString(@"UIGlassContainerEffect"))return;
    const void *key=NSSelectorFromString(@"ALB15.TopMaterialCoordinator");
    id coordinator=objc_getAssociatedObject(source,key);
    if(!coordinator) {
        Class cls=NSClassFromString(@"ALB15TopMaterial");if(cls)coordinator=[[cls alloc]init];
        if(coordinator)objc_setAssociatedObject(source,key,coordinator,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    SEL s=NSSelectorFromString(@"updateWithSource:leftSlot:leftButton:rightSlot:rightButton:");
    if([coordinator respondsToSelector:s])((void(*)(id,SEL,id,id,id,id,id))objc_msgSend)(coordinator,s,source,leftSlot,leftButton,rightSlot,rightButton);
}

void ALB15TopMaterialDidLayout(UIView *source) {
    id coordinator=objc_getAssociatedObject(source,NSSelectorFromString(@"ALB15.TopMaterialCoordinator"));
    SEL refresh=NSSelectorFromString(@"refresh");
    if([coordinator respondsToSelector:refresh])((void(*)(id,SEL))objc_msgSend)(coordinator,refresh);
}

NSString *ALB15TopGlassStatus(UIView *source) {
    id coordinator=objc_getAssociatedObject(source,NSSelectorFromString(@"ALB15.TopMaterialCoordinator"));
    SEL status=NSSelectorFromString(@"status");
    return [coordinator respondsToSelector:status]?((id(*)(id,SEL))objc_msgSend)(coordinator,status):@"等待中央原生材质";
}

NSString *ALB15GlassSignature(UIVisualEffect *effect,UITraitCollection *traits) {
    id style=nil;UIColor *tint=nil;
    @try {
        if([effect respondsToSelector:NSSelectorFromString(@"style")])style=[effect valueForKey:@"style"];
        if([effect respondsToSelector:NSSelectorFromString(@"tintColor")])tint=[effect valueForKey:@"tintColor"];
    } @catch(__unused NSException *e) {}
    NSString *color=@"none";
    if([tint isKindOfClass:UIColor.class]) {
        CGFloat r=0,g=0,b=0,a=0;
        UIColor *resolved=[tint resolvedColorWithTraitCollection:traits];
        if([resolved getRed:&r green:&g blue:&b alpha:&a])color=[NSString stringWithFormat:@"%.6f/%.6f/%.6f/%.6f",r,g,b,a];
        else color=resolved.description;
    }
    return [NSString stringWithFormat:@"%@|%@|%@|%ld|%ld|%d",NSStringFromClass(effect.class),style?:@"native",color,(long)traits.userInterfaceStyle,(long)traits.accessibilityContrast,UIAccessibilityIsReduceTransparencyEnabled()];
}
