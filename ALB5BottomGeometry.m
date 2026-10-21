#import "ALB5Options.h"
#import <QuartzCore/QuartzCore.h>
#include <string.h>
#import "ALBNativeState.h"
extern void AWEAlbumMorphProbeLog(const char *message);

static UIImage *ALB5OwnAvatar;
static NSString *ALB8OwnIdentity;
static NSString *ALB8AvatarCachePath(void) {
    if(!ALB8OwnIdentity.length)return nil;
    // Account-specific cache. A different signed-in user must never inherit
    // the previous account's avatar, including at launch before resolution.
    uint64_t hash=1469598103934665603ULL;
    NSData *data=[ALB8OwnIdentity dataUsingEncoding:NSUTF8StringEncoding];
    const unsigned char *bytes=data.bytes;for(NSUInteger i=0;i<data.length;i++){hash^=bytes[i];hash*=1099511628211ULL;}
    NSString *folder=[NSSearchPathForDirectoriesInDomains(NSCachesDirectory,NSUserDomainMask,YES) firstObject];
    return [folder stringByAppendingPathComponent:[NSString stringWithFormat:@"AwemeLiquidBar/own-avatar-%016llx.jpg",(unsigned long long)hash]];
}
UIImage *ALB8OwnAvatar(void) { return ALB5OwnAvatar; }
void ALB8SetOwnIdentity(NSString *uid) {
    if(!NSThread.isMainThread){dispatch_async(dispatch_get_main_queue(),^{ALB8SetOwnIdentity(uid);});return;}
    if((!uid.length && !ALB8OwnIdentity.length) || [uid isEqualToString:ALB8OwnIdentity])return;
    ALB8OwnIdentity=[uid copy];ALB5OwnAvatar=nil;
    NSString *path=ALB8AvatarCachePath();
    if(path)ALB5OwnAvatar=[UIImage imageWithContentsOfFile:path];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"ALB8OwnProfileDidChange" object:nil];
}
void ALB5SetOwnAvatar(UIImage *image) {
    if(!image || image==ALB5OwnAvatar)return;
    if(!NSThread.isMainThread){dispatch_async(dispatch_get_main_queue(),^{ALB5SetOwnAvatar(image);});return;}
    ALB5OwnAvatar=image;
    [[NSNotificationCenter defaultCenter] postNotificationName:@"ALB8OwnProfileDidChange" object:nil];
    NSString *path=ALB8AvatarCachePath(); // Capture the account before dispatch.
    if(path)dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{
        [[NSFileManager defaultManager] createDirectoryAtPath:path.stringByDeletingLastPathComponent withIntermediateDirectories:YES attributes:nil error:nil];
        [UIImageJPEGRepresentation(image,.88) writeToFile:path options:NSDataWritingAtomic error:nil];
    });
}
BOOL ALB8ExpandOnLeaveFeed(void) { return ALB5Bool(@"bottom.expandOnLeaveFeed",YES); }



// TEST17: observation and native hit routing only. UIKit is the sole owner of
// platter geometry, interpolation, corner continuity and press deformation.
// In particular, never wrap native layout in a disabled-actions transaction.
@interface ALB15BottomState : NSObject
@property(nonatomic,weak) UITabBar *bar;
@property(nonatomic,weak) UIView *host;
@property(nonatomic,weak) UIView *leftPlatter;
@property(nonatomic,weak) UIView *auxiliary;
@property(nonatomic) BOOL compact;
@property(nonatomic) CGFloat visibleFraction;
@property(nonatomic) CFTimeInterval scannedAt;
@property(nonatomic) CGRect barBounds;
@property(nonatomic) CGFloat expandedLeftWidth;
@property(nonatomic) NSUInteger itemCount;
@property(nonatomic) BOOL hitTestInProgress;
@end
@implementation ALB15BottomState
@end
static char ALB15BottomStateKey;
static ALB15BottomState *ALB15State(UITabBar *bar) {
    if (!bar) return nil;
    ALB15BottomState *s=objc_getAssociatedObject(bar,&ALB15BottomStateKey);
    if (!s) {
        s=[ALB15BottomState new]; s.bar=bar; s.visibleFraction=1;
        objc_setAssociatedObject(bar,&ALB15BottomStateKey,s,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return s;
}
// TEST42 FIX8: the centered-avatar compact scheme is removed; UIKit's native
// minimize is the only compact style.
NSInteger ALB15CompactStyle(void) { return 1; }
CGFloat ALB15BottomScale(void) { return ALB5Number(@"bottom.scale",1,.55,1.15); }
static BOOL ALB15Attached(UIView *v,UIView *host) {
    return v && host && v.window==host.window && [v isDescendantOfView:host];
}
static void ALB15DiscoverNativeChrome(ALB15BottomState *s) {
    UIView *host=s.host; UITabBar *bar=s.bar;
    if (!host.window || bar.window!=host.window) return;
    // A bounded, event/layout-driven read. No recurring display link, feed scan,
    // image overlay, copied glass, native transform, mask or alpha mutation.
    BOOL valid=ALB15Attached(s.leftPlatter,host) && !s.leftPlatter.hidden &&
        ALB15Attached(s.auxiliary,host) && !s.auxiliary.hidden;
    if (s.itemCount!=bar.items.count) {
        s.itemCount=bar.items.count; s.expandedLeftWidth=0; valid=NO;
    }
    CFTimeInterval now=CACurrentMediaTime();
    if (valid && CGRectEqualToRect(s.barBounds,bar.bounds)) return;
    if (now-s.scannedAt<.15) return;
    s.scannedAt=now; s.barBounds=bar.bounds;
    CGRect band=[bar convertRect:bar.bounds toView:host];
    NSMutableArray<UIView *> *q=[NSMutableArray arrayWithArray:host.subviews];
    UIView *left=nil,*aux=nil;
    for (NSUInteger i=0;i<q.count && i<192;i++) {
        UIView *v=q[i];
        if ([v isKindOfClass:UIScrollView.class]) continue;
        NSString *n=NSStringFromClass(v.class);
        CGRect r=[v convertRect:v.bounds toView:host];
        BOOL nearby=CGRectIntersectsRect(CGRectInset(band,-64,-64),r);
        if (nearby && !v.hidden && v.alpha>.01) {
            if ([n containsString:@"TabBarAuxiliaryView"]) aux=v;
            if ([n containsString:@"TabBarPlatterView"]) {
                BOOL inAux=NO;
                for (UIView *a=v.superview;a && a!=host;a=a.superview)
                    if ([NSStringFromClass(a.class) containsString:@"TabBarAuxiliaryView"]) { inAux=YES; break; }
                if (!inAux && (!left || CGRectGetMinX(r)<CGRectGetMinX([left convertRect:left.bounds toView:host]))) left=v;
            }
        }
        [q addObjectsFromArray:v.subviews];
    }
    s.leftPlatter=left; s.auxiliary=aux;
}
// Compatibility exports retained for hooks from the baseline. These do NOT
// suppress UIKit transactions or restore/apply private surface poses anymore.
void ALB6RestoreBottomGeometry(UITabBar *bar) { (void)bar; }
BOOL ALB8BeginBottomLayout(UITabBar *bar) { (void)bar; return NO; }
void ALB8EndBottomLayout(UITabBar *bar) { (void)bar; }
void ALB8SetBottomVisibleFraction(UITabBar *bar,CGFloat fraction) {
    ALB15State(bar).visibleFraction=isfinite(fraction)?MIN(1,MAX(0,fraction)):0;
}
void ALB5UpdateBottomGeometry(UITabBar *bar,UIView *host,BOOL compact,BOOL animated) {
    (void)animated;
    if (!bar || !host) return;
    ALB15BottomState *s=ALB15State(bar);
    if (s.host!=host) { s.leftPlatter=nil; s.auxiliary=nil; s.scannedAt=0; s.expandedLeftWidth=0; }
    s.host=host; s.compact=compact;
    if (ALB5NativeSurfaceRoot(bar)!=host) ALB5SetNativeSurfaceRoot(bar,host);
    ALB15DiscoverNativeChrome(s);
}
void ALB7ReapplyBottomGeometry(UITabBar *bar) { (void)bar; }
BOOL ALB7BottomHasSurfacePair(UITabBar *bar) {
    ALB15BottomState *s=ALB15State(bar);
    return ALB15Attached(s.leftPlatter,s.host) && ALB15Attached(s.auxiliary,s.host);
}
CGRect ALB5CompactHitRect(UITabBar *bar) {
    ALB15BottomState *s=ALB15State(bar);
    if (s.visibleFraction<.82) return CGRectNull;
    if (!s.compact) return CGRectNull;
    CGRect r=CGRectNull;
    for (UIView *v in @[s.leftPlatter ?: (id)NSNull.null,s.auxiliary ?: (id)NSNull.null]) {
        if (![v isKindOfClass:UIView.class] || !ALB15Attached(v,s.host) || v.hidden || v.alpha<=.01) continue;
        CGRect piece=[v convertRect:v.bounds toView:bar];
        r=CGRectIsNull(r)?piece:CGRectUnion(r,piece);
    }
    return r;
}
static BOOL ALB15HitBelongsToChrome(UIView *hit,UIView *root) {
    for (UIView *a=hit;a && a!=root;a=a.superview) {
        NSString *name=NSStringFromClass(a.class);
        if ([a isKindOfClass:UITabBar.class] || [name containsString:@"TabBarPlatterView"] ||
            [name containsString:@"TabBarAuxiliaryView"] || [name containsString:@"TabButton"]) return YES;
        if ([a isKindOfClass:UIScrollView.class]) return NO;
    }
    return NO;
}
UIView *ALB15NativeBottomHitView(UITabBar *bar,CGPoint point,UIEvent *event) {
    ALB15BottomState *s=ALB15State(bar);
    UIView *host=s.host ?: ALB5NativeSurfaceRoot(bar);
    if (!host.window || s.visibleFraction<.82 || host.hidden || host.alpha<=.01 || !host.userInteractionEnabled) return nil;
    // UIKit can put the auxiliary/search platter beside, not inside, UITabBar.
    // Test the real native subtrees and return their true hit, never the host.
    CGPoint inHost=[bar convertPoint:point toView:host];
    CGRect band=CGRectInset([bar convertRect:bar.bounds toView:host],-64,-36);
    if (!CGRectContainsPoint(band,inHost) || s.hitTestInProgress) return nil;
    s.hitTestInProgress=YES;
    @try {
    for (UIView *branch in host.subviews.reverseObjectEnumerator) {
        if ([branch isKindOfClass:UIScrollView.class]) continue;
        UIView *hit=[branch hitTest:[host convertPoint:inHost toView:branch] withEvent:event];
        if (hit && ALB15HitBelongsToChrome(hit,host)) return hit;
    }
    return nil;
    } @finally {
        s.hitTestInProgress=NO;
    }
}
UIView *ALB8CompactNativeHitView(UITabBar *bar,CGPoint point,UIEvent *event) {
    return ALB15NativeBottomHitView(bar,point,event);
}
// Read-only fallback if a future UIKit build has no BOOL state getter. Geometry
// is evidence, not authority: -1 means unconfirmed, not a failed animation.
int32_t ALB15ObserveCompactGeometry(UITabBar *bar) {
    ALB15BottomState *s=ALB15State(bar);
    ALB15DiscoverNativeChrome(s);
    UIView *left=s.leftPlatter;
    if (!ALB15Attached(left,s.host) || left.hidden || left.alpha<=.01) return -1;
    CGRect r=[left convertRect:left.bounds toView:s.host];
    int result=ALB15ClassifyNativeGeometry(r.size.width,r.size.height,s.expandedLeftWidth);
    if (result==0) s.expandedLeftWidth=MAX(s.expandedLeftWidth,r.size.width);
    return result;
}
NSString *ALB6BottomGeometryStatus(UITabBar *bar) {
    ALB15BottomState *s=ALB15State(bar);
    if (!s.host.window) return @"TEST28 · 等待原生底栏挂载";
    CGRect left=s.leftPlatter?[s.leftPlatter convertRect:s.leftPlatter.bounds toView:s.host]:CGRectNull;
    CGRect aux=s.auxiliary?[s.auxiliary convertRect:s.auxiliary.bounds toView:s.host]:CGRectNull;
    return [NSString stringWithFormat:@"TEST28 resident=UIKit observedLeft=%d style=%ld\nleft=%@\nauxiliary=%@\n左球=系统滚动；居中=等高横缩与系统玻璃容器融合",
        ALB15ObserveCompactGeometry(bar),(long)ALB15CompactStyle(),NSStringFromCGRect(left),NSStringFromCGRect(aux)];
}

// Only called while the main selected tab is the user's own profile. Never
// borrow the playing video's author avatar. Missing evidence keeps search.
void ALB5CaptureOwnAvatar(UIWindow *window) {
    if(!window)return;
    static CFTimeInterval last; if(CACurrentMediaTime()-last<1)return;last=CACurrentMediaTime();
    NSMutableArray *q=[NSMutableArray arrayWithObject:window];UIImage *best=nil;CGFloat score=-CGFLOAT_MAX;
    for(NSUInteger i=0;i<q.count && i<900;i++) {
        UIView *v=q[i];if(v.hidden || v.alpha<.01 || [v.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."])continue;
        [q addObjectsFromArray:v.subviews];if(![v isKindOfClass:UIImageView.class])continue;
        UIImage *image=((UIImageView *)v).image;
        if(!image || image.size.width<64 || image.size.height<64)continue;
        CGRect r=[v convertRect:v.bounds toView:window];
        if(r.size.width<36 || r.size.width>160 || r.size.height<36 || r.size.height>160 || CGRectGetMidY(r)>window.bounds.size.height*.5)continue;
        BOOL avatar=NO;
        for(UIView *a=v;a && a!=window;a=a.superview) {
            NSString *name=NSStringFromClass(a.class).lowercaseString;
            if([name containsString:@"avatar"] || [name containsString:@"profileheader"]) { avatar=YES;break; }
        }
        CGFloat rank=r.size.width-CGRectGetMinY(r)*.2;
        if(avatar && rank>score) {score=rank;best=image;}
    }
    if(best)ALB5SetOwnAvatar(best);
}
