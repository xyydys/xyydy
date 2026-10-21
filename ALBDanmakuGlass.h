/* Scoped to Douyin's existing danmaku containers. The native entry keeps its
   own action; no publish selector, text-field or business model is replaced. */
@interface ALB28DanmakuGlassRecord : NSObject
@property(nonatomic,weak) UIView *source;
@property(nonatomic,strong) UIButton *button;
@property(nonatomic,strong) UIButtonConfiguration *originalConfiguration;
@property(nonatomic,strong) UIButtonConfiguration *appliedConfiguration;
@property(nonatomic,strong) UIColor *originalBackground;
@property(nonatomic,strong) UIColor *originalLayerBackground;
@property(nonatomic,strong) NSMapTable<UIView *,NSNumber *> *childOpacities;
@property(nonatomic) BOOL usesNativeButton;
@property(nonatomic) NSUInteger materialGeneration;
@end
@implementation ALB28DanmakuGlassRecord
@end
static char kALB28DanmakuRecordKey,kALB28DanmakuUpdatingKey,kALB28DanmakuScanKey,kALB28DanmakuOwnerKey;
static void ALB28RefreshDanmakuGlass(UIView *root);

static NSHashTable<UIView *> *ALB28DanmakuRoots(void) {
    static NSHashTable *roots;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        roots=[NSHashTable weakObjectsHashTable];
        [NSNotificationCenter.defaultCenter addObserverForName:@"ALB5.OptionsChanged" object:nil
            queue:NSOperationQueue.mainQueue usingBlock:^(__unused NSNotification *note) {
                for(UIView *root in roots.allObjects)ALB28RefreshDanmakuGlass(root);
            }];
    });
    return roots;
}
static void ALB28RestoreDanmakuRecord(ALB28DanmakuGlassRecord *record) {
    if(!record)return;
    UIView *source=record.source;
    if(record.usesNativeButton) {
        if(record.button.configuration==record.appliedConfiguration)
            record.button.configuration=record.originalConfiguration;
    } else {
        ALBUnbindOriginalGlassAction(record.button);
        [record.button removeFromSuperview];
    }
    source.backgroundColor=record.originalBackground;
    source.layer.backgroundColor=record.originalLayerBackground.CGColor;
    if(source)objc_setAssociatedObject(source,&kALB28DanmakuOwnerKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    for(UIView *child in record.childOpacities.keyEnumerator)
        child.layer.opacity=[[record.childOpacities objectForKey:child] floatValue];
}
static NSString *ALB28DanmakuEntryEvidence(UIView *view) {
    NSMutableArray *pieces=[NSMutableArray array];
    for(UIView *node=view;node && pieces.count<9;node=node.superview) {
        [pieces addObject:NSStringFromClass(node.class)?:@""];
        [pieces addObject:node.accessibilityLabel?:@""];
        [pieces addObject:node.accessibilityIdentifier?:@""];
    }
    if([view isKindOfClass:UIButton.class])
        [pieces addObject:[(UIButton *)view titleForState:UIControlStateNormal]?:@""];
    return [[pieces componentsJoinedByString:@" "] lowercaseString];
}
static UIView *ALB28FindDanmakuEntry(UIView *root) {
    NSMutableArray<UIView *> *queue=[NSMutableArray arrayWithObject:root];
    UIView *best=nil; NSInteger rank=0;
    for(NSUInteger i=0;i<queue.count && i<160;i++) {
        UIView *view=queue[i];
        if(ALBIsOwnedView(view) || view.hidden || view.alpha<.01)continue;
        NSHashTable *owners=objc_getAssociatedObject(view,&kALB28DanmakuOwnerKey);
        if(owners.anyObject && owners.anyObject!=root)continue;
        CGSize size=view.bounds.size;
        if(size.width>=24 && size.width<=66 && size.height>=24 && size.height<=66 &&
           MAX(size.width,size.height)<=MIN(size.width,size.height)*1.35 &&
           ![view isKindOfClass:UIImageView.class] && ![view isKindOfClass:UILabel.class]) {
            NSString *evidence=ALB28DanmakuEntryEvidence(view);
            BOOL explicitEntry=[evidence containsString:@"发弹幕"] || [evidence containsString:@"发送弹幕"] ||
                [evidence containsString:@"发一条弹幕"];
            BOOL danmaku=[evidence containsString:@"danmaku"] || [evidence containsString:@"danmu"];
            BOOL entry=[evidence containsString:@"send"] || [evidence containsString:@"publish"] ||
                [evidence containsString:@"input"] || [evidence containsString:@"entry"];
            if(explicitEntry || (danmaku && entry)) {
                BOOL control=[view isKindOfClass:UIControl.class] && ((UIControl *)view).enabled &&
                    (((UIControl *)view).allControlEvents &
                        (UIControlEventTouchUpInside|UIControlEventPrimaryActionTriggered));
                BOOL gesture=AWEPrimaryHomeActionTapGesture(view)!=nil;
                if(control || gesture) {
                    NSInteger score=(explicitEntry?100:0)+(control?30:0)+(gesture?20:0);
                    if(score>rank){best=view;rank=score;}
                }
            }
        }
        [queue addObjectsFromArray:view.subviews];
    }
    return best;
}
static UIImage *ALB28DanmakuEntryImage(UIView *source) {
    if([source isKindOfClass:UIButton.class]) {
        UIButton *button=(UIButton *)source;
        UIImage *image=button.configuration.image?:[button imageForState:UIControlStateNormal];
        if(image)return image;
    }
    NSMutableArray *queue=[NSMutableArray arrayWithArray:source.subviews];
    for(NSUInteger i=0;i<queue.count && i<48;i++) {
        UIView *view=queue[i];
        if(ALBIsOwnedView(view))continue;
        if([view isKindOfClass:UIImageView.class] && ((UIImageView *)view).image)
            return ((UIImageView *)view).image;
        [queue addObjectsFromArray:view.subviews];
    }
    return [UIImage systemImageNamed:@"text.bubble"];
}
static void ALB28RefreshDanmakuGlass(UIView *root) {
    if(!root || [objc_getAssociatedObject(root,&kALB28DanmakuUpdatingKey) boolValue])return;
    [ALB28DanmakuRoots() addObject:root];
    objc_setAssociatedObject(root,&kALB28DanmakuUpdatingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    @try {
        ALB28DanmakuGlassRecord *record=objc_getAssociatedObject(root,&kALB28DanmakuRecordKey);
        BOOL enabled=AWEPluginFeaturesEnabled() && ALB5Bool(@"danmaku.glass",NO) && root.window;
        if(!enabled || (record && (!record.source || ![record.source isDescendantOfView:root]))) {
            ALB28RestoreDanmakuRecord(record);
            objc_setAssociatedObject(root,&kALB28DanmakuRecordKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            record=nil;
            if(!enabled)return;
        }
        if(!record) {
            CFTimeInterval now=CACurrentMediaTime();
            if(now-[objc_getAssociatedObject(root,&kALB28DanmakuScanKey) doubleValue]<.3)return;
            objc_setAssociatedObject(root,&kALB28DanmakuScanKey,@(now),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            UIView *source=ALB28FindDanmakuEntry(root);
            if(!source)return; // Unknown variants retain their original button.
            record=[ALB28DanmakuGlassRecord new]; record.source=source;
            NSHashTable *owners=[NSHashTable weakObjectsHashTable];[owners addObject:root];
            objc_setAssociatedObject(source,&kALB28DanmakuOwnerKey,owners,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            record.originalBackground=source.backgroundColor;
            record.originalLayerBackground=source.layer.backgroundColor?[UIColor colorWithCGColor:source.layer.backgroundColor]:nil;
            record.childOpacities=[NSMapTable weakToStrongObjectsMapTable];
            record.usesNativeButton=[source isKindOfClass:UIButton.class];
            record.button=record.usesNativeButton?(UIButton *)source:[ALBRightGlassButton buttonWithType:UIButtonTypeCustom];
            if(!record.usesNativeButton) {
                record.button.accessibilityIdentifier=@"AwemeLiquidBar.Danmaku.SendGlass";
                record.button.accessibilityLabel=source.accessibilityLabel.length?source.accessibilityLabel:@"发弹幕";
                [source addSubview:record.button];
                if(!ALBBindOriginalGlassAction(record.button,source)) {
                    ALB28RestoreDanmakuRecord(record);
                    AWEInteractionDiagnosticWrite(@"DANMAKU_GLASS original-retained reason=action-bind-failed source=%@",
                        NSStringFromClass(source.class));
                    return;
                }
            }
            objc_setAssociatedObject(root,&kALB28DanmakuRecordKey,record,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            AWEInteractionDiagnosticWrite(@"DANMAKU_GLASS bound=%@ nativeButton=%d size=%@",
                NSStringFromClass(source.class),record.usesNativeButton,NSStringFromCGSize(source.bounds.size));
        }
        UIView *source=record.source;
        UIButton *button=record.button;
        if(button.tracking)return;
        BOOL nativeUpdated = record.usesNativeButton && button.configuration != record.appliedConfiguration;
        if(!record.appliedConfiguration || nativeUpdated || record.materialGeneration != ALBMaterialGeneration()) {
            if(nativeUpdated)record.originalConfiguration=[button.configuration copy];
            UIButtonConfiguration *configuration=AWECreateAdaptiveGlassIconConfiguration(NO);
            configuration.cornerStyle=UIButtonConfigurationCornerStyleCapsule;
            configuration.contentInsets=NSDirectionalEdgeInsetsZero;
            configuration.baseBackgroundColor=nil;
            configuration.image=ALB28DanmakuEntryImage(source);
            configuration.preferredSymbolConfigurationForImage=[UIImageSymbolConfiguration
                configurationWithPointSize:MIN(23,MIN(source.bounds.size.width,source.bounds.size.height)*.55)
                weight:UIImageSymbolWeightRegular];
            configuration.baseForegroundColor=source.tintColor?:UIColor.labelColor;
            button.configuration=configuration;
            record.appliedConfiguration=button.configuration;
            record.materialGeneration=ALBMaterialGeneration();
        }
        if (ALBMaterialEnabled()) button.overrideUserInterfaceStyle = ALBMaterialStyle(button, YES);
        if(![source.backgroundColor isEqual:UIColor.clearColor])source.backgroundColor=UIColor.clearColor;
        if(!source.layer.backgroundColor || CGColorGetAlpha(source.layer.backgroundColor)>0)
            source.layer.backgroundColor=UIColor.clearColor.CGColor;
        if(!record.usesNativeButton) {
            for(UIView *child in source.subviews) {
                if(child==button || [button isDescendantOfView:child])continue;
                if(![record.childOpacities objectForKey:child])
                    [record.childOpacities setObject:@(child.layer.opacity) forKey:child];
                if(child.layer.opacity!=0)child.layer.opacity=0;
            }
            CGRect frame=[source convertRect:source.bounds toView:button.superview];
            ALBSetRestingButtonGeometry(button,frame);
        }
    } @finally {
        objc_setAssociatedObject(root,&kALB28DanmakuUpdatingKey,@NO,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}
