/* TEST19: persistent presentation transforms. Read native coordinates by
   inverting the existing presentation map; never restore/reapply on each layout.
   Visibility remains a separate lease and never defines native slot geometry. */
#include <math.h>
typedef struct { double x, y; } ALB6RightPoint;
typedef struct { double a, b, c, d, tx, ty; } ALB6RightAffine;
static inline ALB6RightPoint ALB6RightMapPointOffset(ALB6RightPoint p,
    ALB6RightPoint pivot, double xShift, double shift, double scale) {
    return (ALB6RightPoint){pivot.x + (p.x-pivot.x)*scale + xShift,
        pivot.y + (p.y+shift-pivot.y)*scale};
}
static inline ALB6RightPoint ALB6RightMapPoint(ALB6RightPoint p,
    ALB6RightPoint pivot, double shift, double scale) {
    return ALB6RightMapPointOffset(p,pivot,0,shift,scale);
}
// Convert a desired final-screen edge gap into the upper column translation.
static inline double ALB42RightUpperGapShift(double musicTop, double rowBottom,
                                            double gap, double scale) {
    return musicTop-gap/scale-rowBottom;
}
static inline ALB6RightPoint ALB6RightAffinePoint(ALB6RightPoint p, ALB6RightAffine t) {
    return (ALB6RightPoint){p.x*t.a+p.y*t.c+t.tx, p.x*t.b+p.y*t.d+t.ty};
}
/* Convert the same outer map back through EACH view's native parent.  TEST5
   added a stack-space Y delta directly in unrelated parent coordinate spaces. */
static inline ALB6RightAffine ALB6RightMappedTransformOffset(ALB6RightPoint anchor,
    ALB6RightPoint unitX, ALB6RightPoint unitY, ALB6RightAffine stackToParent,
    ALB6RightPoint layerPosition, ALB6RightPoint pivot, double xShift, double shift, double scale) {
    ALB6RightPoint p=ALB6RightAffinePoint(ALB6RightMapPointOffset(anchor,pivot,xShift,shift,scale),stackToParent);
    ALB6RightPoint x=ALB6RightAffinePoint(ALB6RightMapPointOffset(unitX,pivot,xShift,shift,scale),stackToParent);
    ALB6RightPoint y=ALB6RightAffinePoint(ALB6RightMapPointOffset(unitY,pivot,xShift,shift,scale),stackToParent);
    return (ALB6RightAffine){x.x-p.x,x.y-p.y,y.x-p.x,y.y-p.y,
        p.x-layerPosition.x,p.y-layerPosition.y};
}
static inline ALB6RightAffine ALB6RightMappedTransform(ALB6RightPoint anchor,
    ALB6RightPoint unitX, ALB6RightPoint unitY, ALB6RightAffine stackToParent,
    ALB6RightPoint layerPosition, ALB6RightPoint pivot, double shift, double scale) {
    return ALB6RightMappedTransformOffset(anchor,unitX,unitY,stackToParent,layerPosition,
        pivot,0,shift,scale);
}

/* Keep each remaining role in an existing native ICON slot. In particular an
   avatar row has a different bounds height; using row-bounds centers crowds it
   against Like after removing a middle row. */
static inline double ALB8RightReflowShift(const double *centers, unsigned count,
                                         unsigned hiddenMask, unsigned role) {
    if (!centers || role>=count || (hiddenMask & (1u<<role))) return 0;
    unsigned hiddenBelow=0;
    for (unsigned j=role+1;j<count;j++) if(hiddenMask&(1u<<j)) ++hiddenBelow;
    return centers[role+hiddenBelow]-centers[role];
}

// Share is the visual bottom reference even when hidden. Reconstruct the
// existing Share-visible layout, including its historical hidden-music cap.
static inline double ALB29RightReferenceShift(const double *centers, unsigned count,
    unsigned hiddenMask, unsigned shareRole, double shareRowBottom,
    double musicBottom, int hiddenMusic) {
    double shift=ALB8RightReflowShift(centers,count,hiddenMask & ~(1u<<shareRole),shareRole);
    if (hiddenMusic && shareRowBottom+shift>musicBottom) shift=musicBottom-shareRowBottom;
    return shift;
}
static inline double ALB29RightCountCorrection(double shareCountBottom, double referenceShift,
    double lastCountBottom, double lastShift) {
    return shareCountBottom+referenceShift-lastCountBottom-lastShift;
}
// Only a rendered action can anchor the visible tail. Native cell reuse may
// retain a Favorite/Comment row after the application hides it; that cached
// slot is useful for geometry, but must not leave a gap below the last glyph.
static inline int ALB32RightLastActionIndex(unsigned count, unsigned actionMask,
    unsigned configuredHiddenMask, unsigned nativeInvisibleMask) {
    int last=-1;
    for (unsigned i=0;i<count && i<32;i++)
        if ((actionMask&(1u<<i)) && !((configuredHiddenMask|nativeInvisibleMask)&(1u<<i))) last=(int)i;
    return last;
}

// TEST23: keep the first/last action centers fixed; distribute the two
// interior rows uniformly between them. A row's glyph and count share one
// translation. Apply after native hidden-role reflow, before the existing
// common scale; the share row and its bottom/progress-bar gap do not move.
static inline int ALB23RightEqualActionShifts(const double centers[4], double out[4]) {
    if (!centers || !out) return 0;
    for (unsigned i = 0; i < 4; ++i) {
        if (!isfinite(centers[i]) || (i && centers[i] <= centers[i-1])) return 0;
    }
    double pitch = (centers[3] - centers[0]) / 3.0;
    if (!isfinite(pitch) || pitch <= 0) return 0;
    out[0] = 0; out[3] = 0;
    out[1] = centers[0] + pitch - centers[1];
    out[2] = centers[0] + 2.0 * pitch - centers[2];
    return 1;
}

// Compare native state before applying the user visibility projection.
static inline int ALB10RightAlphaChanged(double nativeBefore,double nativeAfter) {
    return fabs(nativeBefore-nativeAfter)>.01;
}

/* Inverse of the production slot map. Used for coordinate reads, not writes. */
static inline ALB6RightPoint ALB19RightUnmapPointOffset(ALB6RightPoint p,
    ALB6RightPoint pivot, double xShift, double shift, double scale) {
    return (ALB6RightPoint){pivot.x+(p.x-xShift-pivot.x)/scale,
        pivot.y+(p.y-pivot.y)/scale-shift};
}
static inline ALB6RightPoint ALB19RightUnmapPoint(ALB6RightPoint p,
    ALB6RightPoint pivot, double shift, double scale) {
    return ALB19RightUnmapPointOffset(p,pivot,0,shift,scale);
}
static inline int ALB19RightAffineChanged(ALB6RightAffine a, ALB6RightAffine b) {
    return !isfinite(a.a) || !isfinite(a.b) || !isfinite(a.c) ||
        !isfinite(a.d) || !isfinite(a.tx) || !isfinite(a.ty) ||
        fabs(a.a-b.a)>1e-8 || fabs(a.b-b.b)>1e-8 ||
        fabs(a.c-b.c)>1e-8 || fabs(a.d-b.d)>1e-8 ||
        fabs(a.tx-b.tx)>1e-5 || fabs(a.ty-b.ty)>1e-5;
}

#ifndef ALB6_RIGHT_LAYOUT_GEOMETRY_ONLY
static char kALB5RightEditsKey, kALB6RightGeometryDepthKey, kALB6RightApplyingKey;
static char kALB7RightVisibilityKey, kALB7RightVisibilityListKey;
static char kALB8RightRoleCacheKey, kALB8RightTransformKey;
static BOOL gALB8PaletteRightHidden;
static NSHashTable<UIView *> *ALB8RightStacks(void) {
    static NSHashTable<UIView *> *stacks;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ stacks=[NSHashTable weakObjectsHashTable]; });
    return stacks;
}
static NSUInteger gALB6RightMutationDepth;
static BOOL ALB6RightLayoutIsMutating(void) { return gALB6RightMutationDepth>0; }
@interface ALB6RightEdit : NSObject
@property(nonatomic,weak) UIView *view;
@property(nonatomic,weak) UIView *stack;
@property(nonatomic) CGAffineTransform originalTransform;
@property(nonatomic) CGAffineTransform appliedTransform;
@property(nonatomic) CGAffineTransform projection;
@property(nonatomic) CGRect nativeFrame;
@property(nonatomic) CGPoint pivot;
@property(nonatomic) CGFloat xShift,shift,scale;
@property(nonatomic) BOOL projectionActive;
@end
@implementation ALB6RightEdit
@end

/* Role roots and free-standing decoration roots are non-overlapping (checked
   below). Therefore at most one rail projection affects any descendant.
   UIKit still performs the actual conversion, retaining its native parent,
   bounds-origin and layer-coordinate behavior; only OUR map is inverted. */
static ALB6RightEdit *ALB19RightProjectionForView(UIView *view, UIView *stack) {
    NSUInteger depth=0;
    for (UIView *v=view; v && v!=stack && depth++<128; v=v.superview) {
        ALB6RightEdit *r=objc_getAssociatedObject(v,&kALB8RightTransformKey);
        if (r && r.projectionActive && r.stack==stack && r.view==v &&
            isfinite(r.scale) && r.scale>1e-8) return r;
    }
    return nil;
}
static CGPoint ALB19RightConvertPoint(UIView *view,CGPoint point,UIView *target,UIView *stack) {
    if (!view || !target || !stack) return [view convertPoint:point toView:target];
    if (view==target) return point;
    CGPoint p=[view convertPoint:point toView:stack];
    ALB6RightEdit *source=ALB19RightProjectionForView(view,stack);
    if (source) {
        ALB6RightPoint q=ALB19RightUnmapPointOffset((ALB6RightPoint){p.x,p.y},
            (ALB6RightPoint){source.pivot.x,source.pivot.y},source.xShift,source.shift,source.scale);
        p=CGPointMake(q.x,q.y);
    }
    ALB6RightEdit *destination=ALB19RightProjectionForView(target,stack);
    if (destination) {
        ALB6RightPoint q=ALB6RightMapPointOffset((ALB6RightPoint){p.x,p.y},
            (ALB6RightPoint){destination.pivot.x,destination.pivot.y},destination.xShift,destination.shift,destination.scale);
        p=CGPointMake(q.x,q.y);
    }
    return [stack convertPoint:p toView:target];
}
static CGRect ALB19RightConvertRect(UIView *view,CGRect rect,UIView *target,UIView *stack) {
    CGPoint a=ALB19RightConvertPoint(view,rect.origin,target,stack);
    CGPoint b=ALB19RightConvertPoint(view,CGPointMake(CGRectGetMaxX(rect),CGRectGetMinY(rect)),target,stack);
    CGPoint c=ALB19RightConvertPoint(view,CGPointMake(CGRectGetMinX(rect),CGRectGetMaxY(rect)),target,stack);
    CGPoint d=ALB19RightConvertPoint(view,CGPointMake(CGRectGetMaxX(rect),CGRectGetMaxY(rect)),target,stack);
    CGFloat x=MIN(MIN(a.x,b.x),MIN(c.x,d.x)), y=MIN(MIN(a.y,b.y),MIN(c.y,d.y));
    return CGRectMake(x,y,MAX(MAX(a.x,b.x),MAX(c.x,d.x))-x,
        MAX(MAX(a.y,b.y),MAX(c.y,d.y))-y);
}
static BOOL ALB19RightTransformsDiffer(CGAffineTransform a,CGAffineTransform b) {
    return ALB19RightAffineChanged((ALB6RightAffine){a.a,a.b,a.c,a.d,a.tx,a.ty},
        (ALB6RightAffine){b.a,b.b,b.c,b.d,b.tx,b.ty});
}
static BOOL ALB19RightFiniteTransform(CGAffineTransform t) {
    return isfinite(t.a)&&isfinite(t.b)&&isfinite(t.c)&&isfinite(t.d)&&isfinite(t.tx)&&isfinite(t.ty);
}
static BOOL ALB19RightSamePoint(CGPoint a,CGPoint b) {
    return fabs(a.x-b.x)<1e-6 && fabs(a.y-b.y)<1e-6;
}
static BOOL ALB19RightSameRect(CGRect a,CGRect b) {
    return ALB19RightSamePoint(a.origin,b.origin) &&
        fabs(a.size.width-b.size.width)<1e-6 && fabs(a.size.height-b.size.height)<1e-6;
}
static void ALB19RightWriteTransform(UIView *view,CGAffineTransform transform) {
    if (!view || !ALB19RightFiniteTransform(transform) ||
        !ALB19RightTransformsDiffer(view.layer.affineTransform,transform)) return;
    ++gALB6RightMutationDepth;
    @try { view.transform=transform; }
    @finally { --gALB6RightMutationDepth; }
}

@interface ALB8RightRoleRecord : NSObject
@property(nonatomic,weak) UIView *anchor;
@property(nonatomic,weak) UIView *root;
@property(nonatomic) CGPoint iconPoint;
@property(nonatomic) BOOL hasIconPoint;
@property(nonatomic,weak) UIView *icon;
@end
@implementation ALB8RightRoleRecord
@end

@interface ALB7RightVisibilityLease : NSObject
@property(nonatomic,weak) UIView *view;
@property(nonatomic,weak) UIView *stack;
@property(nonatomic,copy) NSString *role;
@property(nonatomic) CGFloat nativeAlpha;
@property(nonatomic) BOOL nativeInteractive;
@end
@implementation ALB7RightVisibilityLease
@end

static void ALB5ApplyRightLayout(UIView *stack);
static BOOL ALB6RightContains(UIView *root, UIView *view) {
    return root && view && (root==view || [view isDescendantOfView:root]);
}
static BOOL ALB8RightRoleHidden(NSString *role) {
    return ALB5Bool([@"right.hide." stringByAppendingString:role],NO) ||
        (gALB8PaletteRightHidden && ![role isEqualToString:@"music"]);
}
static BOOL ALB7RightVisibilityActive(ALB7RightVisibilityLease *lease) {
    return lease && AWEPluginFeaturesEnabled() &&
        ALB6RightContains(lease.stack,lease.view) &&
        ALB8RightRoleHidden(lease.role);
}
static CGFloat ALB7RightNativeAlpha(UIView *view) {
    ALB7RightVisibilityLease *lease=objc_getAssociatedObject(view,&kALB7RightVisibilityKey);
    return ALB7RightVisibilityActive(lease)?lease.nativeAlpha:view.alpha;
}
/* Called by the exact native button / element setters, BEFORE UIKit gets a
   show write. A delayed refresh cannot prevent a visible intervening frame. */
static CGFloat ALB7RightFilterAlpha(UIView *view,CGFloat alpha) {
    ALB7RightVisibilityLease *lease=objc_getAssociatedObject(view,&kALB7RightVisibilityKey);
    if (!ALB7RightVisibilityActive(lease)) return alpha;
    if (!ALB6RightLayoutIsMutating()) lease.nativeAlpha=alpha;
    return 0;
}
static void ALB7RightPruneVisibility(UIView *stack) {
    NSMutableArray *kept=[NSMutableArray array];
    for (ALB7RightVisibilityLease *lease in objc_getAssociatedObject(stack,&kALB7RightVisibilityListKey)) {
        if (lease.stack!=stack) continue;
        if (ALB7RightVisibilityActive(lease)) { [kept addObject:lease]; continue; }
        UIView *view=lease.view;
        if (objc_getAssociatedObject(view,&kALB7RightVisibilityKey)!=lease) continue;
        objc_setAssociatedObject(view,&kALB7RightVisibilityKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        ++gALB6RightMutationDepth;
        if (view.alpha==0) view.alpha=lease.nativeAlpha;
        view.userInteractionEnabled=lease.nativeInteractive;
        --gALB6RightMutationDepth;
    }
    objc_setAssociatedObject(stack,&kALB7RightVisibilityListKey,kept,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALB8RightReleaseVisibilityLease(ALB7RightVisibilityLease *lease) {
    UIView *view=lease.view;
    if (!view || objc_getAssociatedObject(view,&kALB7RightVisibilityKey)!=lease) return;
    objc_setAssociatedObject(view,&kALB7RightVisibilityKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ++gALB6RightMutationDepth;
    if (view.alpha==0) view.alpha=lease.nativeAlpha;
    view.userInteractionEnabled=lease.nativeInteractive;
    --gALB6RightMutationDepth;
}
static void ALB7RightSuppress(UIView *view,UIView *stack,NSString *role) {
    if (!view || !ALB6RightContains(stack,view)) return;
    ALB7RightVisibilityLease *lease=objc_getAssociatedObject(view,&kALB7RightVisibilityKey);
    // A recycled native button can represent another role without detaching.
    // The old Comment lease must not keep the new Like button invisible.
    if (lease && (lease.stack!=stack || ![lease.role isEqualToString:role])) {
        ALB8RightReleaseVisibilityLease(lease); lease=nil;
    }
    if (!ALB8RightRoleHidden(role)) return;
    if (!lease) {
        lease=[ALB7RightVisibilityLease new]; lease.view=view;
        lease.nativeAlpha=view.alpha; lease.nativeInteractive=view.userInteractionEnabled;
        objc_setAssociatedObject(view,&kALB7RightVisibilityKey,lease,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    NSMutableArray *leases=objc_getAssociatedObject(stack,&kALB7RightVisibilityListKey);
    if (!leases) {
        leases=[NSMutableArray array];
        objc_setAssociatedObject(stack,&kALB7RightVisibilityListKey,leases,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (![leases containsObject:lease]) [leases addObject:lease];
    lease.stack=stack; lease.role=role;
    ++gALB6RightMutationDepth;
    if (view.alpha!=0) view.alpha=0;
    view.userInteractionEnabled=NO;
    --gALB6RightMutationDepth;
}
static void ALB5RestoreRightLayout(UIView *stack) {
    if (!stack) return;
    NSArray<ALB6RightEdit *> *edits=objc_getAssociatedObject(stack,&kALB5RightEditsKey);
    if (!edits.count) return;
    objc_setAssociatedObject(stack,&kALB5RightEditsKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [CATransaction begin]; [CATransaction setDisableActions:YES];
    @try {
        for (ALB6RightEdit *record in edits) {
            UIView *v=record.view;
            if (!v || objc_getAssociatedObject(v,&kALB8RightTransformKey)!=record) continue;
            record.projectionActive=NO;
            objc_setAssociatedObject(v,&kALB8RightTransformKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            ALB19RightWriteTransform(v,record.originalTransform);
        }
    } @finally { [CATransaction commit]; }
}
static UILabel *ALB32RightTailLabel(UIView *row,UIView *stack) {
    if (!row || !stack) return nil;
    UILabel *result=nil;
    CGFloat bottom=-CGFLOAT_MAX;
    NSMutableArray<UIView *> *queue=[NSMutableArray arrayWithObject:row];
    for (NSUInteger i=0;i<queue.count && i<60;i++) {
        UIView *view=queue[i];
        if (ALBIsOwnedView(view) || view.hidden || ALB7RightNativeAlpha(view)<=.001) continue;
        if ([view isKindOfClass:UILabel.class]) {
            UILabel *label=(UILabel *)view;
            NSString *text=label.attributedText.string ?: label.text;
            // Zero-count native actions use a name (e.g. Share) in the same
            // label. Its visual baseline is still the reference, not the
            // larger element wrapper's bottom padding.
            if (text.length && CGRectGetWidth(label.bounds)>0 && CGRectGetHeight(label.bounds)>0) {
                CGFloat candidate=CGRectGetMaxY(ALB19RightConvertRect(view,view.bounds,stack,stack));
                if (candidate>bottom) { bottom=candidate; result=label; }
            }
        }
        [queue addObjectsFromArray:view.subviews];
    }
    return result;
}
static CGFloat ALB29RightCountBottom(UIView *row,UIView *stack) {
    UIView *reference=ALB32RightTailLabel(row,stack) ?: row;
    return CGRectGetMaxY(ALB19RightConvertRect(reference,reference.bounds,stack,stack));
}
static CGAffineTransform ALB6RightViewTransform(UIView *,UIView *,CGPoint,CGFloat,CGFloat,CGFloat);
static CGAffineTransform ALB10RightReadTransform(UIView *view,CGAffineTransform actual) {
    ALB6RightEdit *record=objc_getAssociatedObject(view,&kALB8RightTransformKey);
    return record && !ALB6RightLayoutIsMutating()?record.originalTransform:actual;
}
static CGAffineTransform ALB9RightFilterTransform(UIView *view,CGAffineTransform native) {
    if(ALB6RightLayoutIsMutating())return native;
    ALB6RightEdit *r=objc_getAssociatedObject(view,&kALB8RightTransformKey);
    if(!r)return native;
    // Compose the requested native matrix with the saved presentation map.
    // Do NOT temporarily write a native transform here: a native spring may
    // be active and would capture that transient write as its animation start.
    r.originalTransform=native;
    r.appliedTransform=CGAffineTransformConcat(native,r.projection);
    return r.appliedTransform;
}
static CGRect ALB9RightNativeFrame(UIView *view,CGRect actual) {
    ALB6RightEdit *r=objc_getAssociatedObject(view,&kALB8RightTransformKey);
    return r && !ALB6RightLayoutIsMutating()?r.nativeFrame:actual;
}
static CGFloat ALB9RightReadAlpha(UIView *view,CGFloat actual) {
    ALB7RightVisibilityLease *lease=objc_getAssociatedObject(view,&kALB7RightVisibilityKey);
    return !ALB6RightLayoutIsMutating() && ALB7RightVisibilityActive(lease)?lease.nativeAlpha:actual;
}
static void ALB9RightWriteGeometry(UIView *view,void (^write)(void)) {
    ALB6RightEdit *r=objc_getAssociatedObject(view,&kALB8RightTransformKey);
    if(!r || ALB6RightLayoutIsMutating()){write();return;}
    /* Enter only for an actual native geometry change, never for a layout
       callback or an identical frame/bounds/center write. */
    [CATransaction begin];[CATransaction setDisableActions:YES];++gALB6RightMutationDepth;
    @try {
        r.projectionActive=NO;
        ALB19RightWriteTransform(view,r.originalTransform);
        write();
        r.originalTransform=view.layer.affineTransform;
        r.nativeFrame=view.frame;
        r.appliedTransform=ALB6RightViewTransform(view,r.stack,r.pivot,r.xShift,r.shift,r.scale);
        CGFloat determinant=r.originalTransform.a*r.originalTransform.d-r.originalTransform.b*r.originalTransform.c;
        if(fabs(determinant)>1e-8) r.projection=CGAffineTransformConcat(CGAffineTransformInvert(r.originalTransform),r.appliedTransform);
        else r.appliedTransform=CGAffineTransformConcat(r.originalTransform,r.projection);
        ALB19RightWriteTransform(view,r.appliedTransform);
        r.projectionActive=YES;
    } @finally {--gALB6RightMutationDepth;[CATransaction commit];}
}
static BOOL ALB19RightSkipNativeFrame(UIView *view,CGRect frame) {
    ALB6RightEdit *r=objc_getAssociatedObject(view,&kALB8RightTransformKey);
    return r && !ALB6RightLayoutIsMutating() && ALB19RightSameRect(r.nativeFrame,frame);
}
static BOOL ALB19RightSkipNativeBounds(UIView *view,CGRect bounds) {
    return !ALB6RightLayoutIsMutating() && objc_getAssociatedObject(view,&kALB8RightTransformKey) &&
        ALB19RightSameRect(view.bounds,bounds);
}
static BOOL ALB19RightSkipNativeCenter(UIView *view,CGPoint center) {
    return !ALB6RightLayoutIsMutating() && objc_getAssociatedObject(view,&kALB8RightTransformKey) &&
        ALB19RightSamePoint(view.center,center);
}
static void ALB19RightSetBounds(UIView *view,CGRect bounds) {
    if (!view || ALB19RightSameRect(view.bounds,bounds)) return;
    ALB9RightWriteGeometry(view,^{ view.bounds=bounds; });
}
static void ALB19RightSetCenter(UIView *view,CGPoint center) {
    if (!view || ALB19RightSamePoint(view.center,center)) return;
    ALB9RightWriteGeometry(view,^{ view.center=center; });
}
static void ALB10RightLocalLayout(UIView *view,void (^layout)(void)) {
    /* Bounds-local button layout does not need an outer-transform reset.
       Native geometry setters are handled individually above. */
    (void)view;
    layout();
}
static void ALB6BeginRightGeometry(UIView *stack) {
    if (!stack) return;
    NSUInteger depth=[objc_getAssociatedObject(stack,&kALB6RightGeometryDepthKey) unsignedIntegerValue];
    objc_setAssociatedObject(stack,&kALB6RightGeometryDepthKey,@(depth+1),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    // Nesting only. Do not write transforms or wrap native layout in a transaction.
}
static void ALB6EndRightGeometry(UIView *stack) {
    if (!stack) return;
    NSUInteger depth=[objc_getAssociatedObject(stack,&kALB6RightGeometryDepthKey) unsignedIntegerValue];
    if (!depth) return;
    objc_setAssociatedObject(stack,&kALB6RightGeometryDepthKey,depth>1?@(depth-1):nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (depth==1 && ![objc_getAssociatedObject(stack,&kALB6RightApplyingKey) boolValue])
        ALB5ApplyRightLayout(stack);
}
static BOOL ALB6RightNativeVisible(UIView *view,UIView *stack) {
    if (!view || !ALB6RightContains(stack,view)) return NO;
    for (UIView *v=view;v && v!=stack;v=v.superview) {
        if (v.hidden || ALB7RightNativeAlpha(v)<=.001) return NO;
    }
    return YES;
}
/* Expand only through ancestors that contain no other role.  A shared wrapper
   can never receive both the avatar transform and the action-column transform. */
static UIView *ALB6RightRoot(UIView *anchor,UIView *stack,NSArray<UIView *> *anchors) {
    if (!ALB6RightContains(stack,anchor) || ALBIsOwnedView(anchor)) return nil;
    UIView *best=anchor;
    Class elementClass=NSClassFromString(@"AWEBaseElementView");
    for (UIView *v=anchor;v && v!=stack;v=v.superview) {
        if (ALBIsOwnedView(v)) break;
        BOOL conflict=NO;
        for (UIView *other in anchors)
            if (other!=anchor && ALB6RightContains(v,other)) { conflict=YES; break; }
        if (conflict) break;
        // Stop at the first native element, including its subclasses. During
        // initial binding other roles may be absent; climbing to an unbound
        // common container would accidentally treat the whole rail as avatar.
        if ((elementClass && [v isKindOfClass:elementClass]) ||
            [NSStringFromClass(v.class) containsString:@"AWEBaseElementView"]) return v;
        if (v!=anchor && (CGRectGetWidth(v.bounds)>140 || CGRectGetHeight(v.bounds)>160)) break;
        best=v;
    }
    return best;
}
static UIView *ALB6RightMusicAnchor(UIView *stack,UIButton *glass) {
    // Prefer a recognized native music subtree. Never call the old "lowest
    // 52..92-point element" heuristic after other elements have been hidden.
    NSMutableArray<UIView *> *queue=[NSMutableArray arrayWithArray:stack.subviews];
    UIView *marker=nil;
    for (NSUInteger i=0;i<queue.count && i<180;i++) {
        UIView *v=queue[i];
        if (ALBIsOwnedView(v) || !ALB6RightNativeVisible(v,stack)) continue;
        if (AWESameStyleStableAnchorClass(v)) return v;
        if (!marker && AWEViewHasSameStyleMarker(v)) marker=v;
        [queue addObjectsFromArray:v.subviews];
    }
    if (marker) return marker;
    // A lease records the actual native artwork; no cached parent survives
    // cell reuse. A class-less artwork is accepted only with a native source.
    ALBArtworkLease *lease=objc_getAssociatedObject(glass,&kALBArtworkLeaseKey);
    ALBNativeActionBridge *bridge=objc_getAssociatedObject(glass,&kALBNativeActionBridgeKey);
    if (ALB6RightNativeVisible(bridge.source,stack) &&
        ALB6RightContains(bridge.source,lease.source)) return bridge.source;
    return nil;
}
static CGPoint ALB6RightCenter(UIView *view,UIView *stack) {
    return ALB19RightConvertPoint(view,CGPointMake(CGRectGetMidX(view.bounds),CGRectGetMidY(view.bounds)),stack,stack);
}
static CGPoint ALB6RightCountCenter(UIView *share,UIView *stack) {
    return ALB6RightCenter(ALB32RightTailLabel(share,stack) ?: share,stack);
}
static CGAffineTransform ALB6RightViewTransform(UIView *v,UIView *stack,
                                               CGPoint pivot,CGFloat xShift,CGFloat shift,CGFloat scale) {
    UIView *parent=v.superview;
    CGPoint local=CGPointMake(v.bounds.origin.x+v.bounds.size.width*v.layer.anchorPoint.x,
                             v.bounds.origin.y+v.bounds.size.height*v.layer.anchorPoint.y);
    CGPoint p=ALB19RightConvertPoint(v,local,stack,stack);
    CGPoint x=ALB19RightConvertPoint(v,CGPointMake(local.x+1,local.y),stack,stack);
    CGPoint y=ALB19RightConvertPoint(v,CGPointMake(local.x,local.y+1),stack,stack);
    CGPoint o=ALB19RightConvertPoint(stack,CGPointZero,parent,stack);
    CGPoint ex=ALB19RightConvertPoint(stack,CGPointMake(1,0),parent,stack);
    CGPoint ey=ALB19RightConvertPoint(stack,CGPointMake(0,1),parent,stack);
    ALB6RightAffine inverse={ex.x-o.x,ex.y-o.y,ey.x-o.x,ey.y-o.y,o.x,o.y};
    ALB6RightAffine t=ALB6RightMappedTransformOffset((ALB6RightPoint){p.x,p.y},
        (ALB6RightPoint){x.x,x.y},(ALB6RightPoint){y.x,y.y},inverse,
        (ALB6RightPoint){v.layer.position.x,v.layer.position.y},
        (ALB6RightPoint){pivot.x,pivot.y},xShift,shift,scale);
    return CGAffineTransformMake(t.a,t.b,t.c,t.d,t.tx,t.ty);
}
static void ALB5ApplyRightLayout(UIView *stack) {
    if (!NSThread.isMainThread || UIApplication.sharedApplication.applicationState!=UIApplicationStateActive) return;
    if (!stack || [objc_getAssociatedObject(stack,&kALB6RightGeometryDepthKey) unsignedIntegerValue] ||
        [objc_getAssociatedObject(stack,&kALB6RightApplyingKey) boolValue]) return;
    objc_setAssociatedObject(stack,&kALB6RightApplyingKey,@YES,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [CATransaction begin];[CATransaction setDisableActions:YES];
    @try {
        ALB7RightPruneVisibility(stack);
        [ALB8RightStacks() addObject:stack];
        if (!AWEPluginFeaturesEnabled() || !stack.window ||
            gAWEStableCleanScreenEnabled || !AWESameStyleIsRightActionStackV23(stack)) {
            ALB5RestoreRightLayout(stack); return;
        }
        NSArray<NSString *> *roles=@[@"avatar",@"like",@"comment",@"favorite",@"share",@"music"];
        CGFloat columnScale=ALB5Number(@"right.scale",1,.5,1.25);
        CGFloat musicGap=ALB5Number(@"right.musicGap",12,0,80);
        BOOL equalActionSpacing=ALB5Bool(@"right.equalActionSpacing",YES);
        BOOL requested=YES; // Axis alignment and fixed music anchor also apply at 100%.
        for (NSString *role in roles) requested|=ALB8RightRoleHidden(role);
        if (!requested) { ALB5RestoreRightLayout(stack); return; }

        NSMutableDictionary<NSString *,ALB8RightRoleRecord *> *cache=objc_getAssociatedObject(stack,&kALB8RightRoleCacheKey);
        if (!cache) {
            cache=[NSMutableDictionary dictionary];
            objc_setAssociatedObject(stack,&kALB8RightRoleCacheKey,cache,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        NSMutableDictionary<NSString *,UIView *> *anchors=[NSMutableDictionary dictionary];
        NSMutableDictionary<NSString *,UIView *> *icons=[NSMutableDictionary dictionary];
        for (NSString *role in roles) {
            ALB8RightRoleRecord *record=cache[role];
            // A temporarily alpha-hidden native row still owns its native slot.
            // Discard on detach/reuse, never on a plugin visibility write.
            if (ALB6RightContains(stack,record.anchor) && !ALBIsOwnedView(record.anchor)) {
                anchors[role]=record.anchor;
                if (ALB6RightContains(record.anchor,record.icon)) icons[role]=record.icon;
            } else [cache removeObjectForKey:role];
        }
        NSMutableDictionary<NSString *,NSArray<UIView *> *> *decorations=[NSMutableDictionary dictionary];
        NSDictionary *slots=objc_getAssociatedObject(stack,&kAWEHomeActionStackSlotsKey);
        for (AWEHomeActionSlotView *slot in slots.allValues) {
            NSInteger kind=slot.actionKind;
            if (kind<1 || kind>4 || !ALB6RightContains(stack,slot.originalView)) continue;
            NSString *role=roles[kind];
            for (NSString *oldRole in anchors.allKeys) {
                if (![oldRole isEqualToString:role] && anchors[oldRole]==slot.originalView) {
                    [anchors removeObjectForKey:oldRole]; [icons removeObjectForKey:oldRole];
                    [cache removeObjectForKey:oldRole];
                }
            }
            anchors[role]=slot.originalView;
            if (ALB6RightContains(slot.originalView,slot.sourceImageView)) icons[role]=slot.sourceImageView;
            NSMutableArray *parts=[NSMutableArray arrayWithObject:slot];
            if (slot.systemGlassButton) [parts addObject:slot.systemGlassButton];
            decorations[role]=parts;
        }
        AWEHomeProfileGlassView *profile=objc_getAssociatedObject(stack,&kAWEProfileGlassSlotKey);
        UIView *picture=profile.profileHost;
        if (!ALB6RightContains(stack,picture)) picture=AWEFindViewByExactRuntimeClass(stack,@"AWEPlayInteractionUserProfilePictureView",0);
        if (!picture) picture=AWEFindViewByExactRuntimeClass(stack,@"AWEPlayInteractionUserProfileContainerView",0);
        if (ALB6RightContains(stack,picture)) {
            anchors[@"avatar"]=picture;
            if (ALB6RightContains(picture,profile.avatarImageView)) icons[@"avatar"]=profile.avatarImageView;
        }
        if (profile) {
            NSMutableArray *parts=[NSMutableArray arrayWithObject:profile];
            // The visible menuButton is often a native sibling, not a child
            // of profile. TEST5 transformed only the empty profile plate.
            if (profile.menuButton) [parts addObject:profile.menuButton];
            decorations[@"avatar"]=parts;
        }
        UIButton *musicGlass=objc_getAssociatedObject(stack,&kAWESameStyleStackPlateKey);
        UIView *musicAnchor=ALB6RightMusicAnchor(stack,musicGlass);
        if (musicAnchor) {
            BOOL conflict=NO;
            for (NSString *otherRole in anchors) {
                if ([otherRole isEqualToString:@"music"]) continue;
                UIView *v=anchors[otherRole];
                if (ALB6RightContains(musicAnchor,v) || ALB6RightContains(v,musicAnchor)) { conflict=YES; break; }
            }
            if (!conflict) {
                anchors[@"music"]=musicAnchor;
                ALBArtworkLease *art=objc_getAssociatedObject(musicGlass,&kALBArtworkLeaseKey);
                UIImageView *nativeArtwork=art.source ?: ALBMusicArtwork(musicAnchor);
                if (ALB6RightContains(musicAnchor,nativeArtwork)) icons[@"music"]=nativeArtwork;
            }
        }
        if (musicGlass) decorations[@"music"]=@[musicGlass];
        NSMutableDictionary<NSString *,UIView *> *roots=[NSMutableDictionary dictionary];
        NSMutableSet<UIView *> *claimed=[NSMutableSet set];
        for (NSString *role in anchors) {
            UIView *root=ALB6RightRoot(anchors[role],stack,anchors.allValues);
            if (root) {
                roots[role]=root;
                ALB8RightRoleRecord *record=cache[role] ?: [ALB8RightRoleRecord new];
                record.anchor=anchors[role];record.root=root;
                if(icons[role])record.icon=icons[role];
                { // Re-read native icon geometry after cell reuse without touching layers.
                    UIView *icon=record.icon?:record.anchor;
                    record.iconPoint=ALB19RightConvertPoint(icon,CGPointMake(CGRectGetMidX(icon.bounds),CGRectGetMidY(icon.bounds)),root,stack);
                    record.hasIconPoint=YES;
                }
                cache[role]=record;
            }
            // Suppression must not depend on successful geometry discovery.
            [claimed addObject:anchors[role]];
            if (root) [claimed addObject:root];
            ALB7RightSuppress(anchors[role],stack,role);
            ALB7RightSuppress(root,stack,role);
            for (UIView *part in decorations[role]) {
                if (ALB6RightContains(stack,part)) [claimed addObject:part];
                ALB7RightSuppress(part,stack,role);
            }
        }
        // Release obsolete wrapper leases even if the old role is still set
        // hidden. Native cell reuse may retain that wrapper for a new element.
        NSMutableArray *liveLeases=[NSMutableArray array];
        for (ALB7RightVisibilityLease *lease in objc_getAssociatedObject(stack,&kALB7RightVisibilityListKey)) {
            if (!ALB6RightContains(stack,lease.view) || ![claimed containsObject:lease.view]) ALB8RightReleaseVisibilityLease(lease);
            else if (objc_getAssociatedObject(lease.view,&kALB7RightVisibilityKey)==lease) [liveLeases addObject:lease];
        }
        objc_setAssociatedObject(stack,&kALB7RightVisibilityListKey,liveLeases,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        // Refuse ambiguous hierarchies. Equality-only checks did not prevent
        // scaling a shared avatar/action ancestor twice.
        NSArray<UIView *> *allRoots=roots.allValues;
        for (NSUInteger i=0;i<allRoots.count;i++) for (NSUInteger j=i+1;j<allRoots.count;j++)
            if (ALB6RightContains(allRoots[i],allRoots[j]) || ALB6RightContains(allRoots[j],allRoots[i])) {
                ALB5RestoreRightLayout(stack); return;
            }
        // Capture the image centers in unmodified native coordinates. Keeping
        // role order stable avoids an animated/native parent crossing swapping
        // the meaning of the slots. Cache retains attached hidden roles.
        NSMutableArray<NSString *> *order=[NSMutableArray array];
        NSMutableDictionary<NSString *,NSNumber *> *nativeCenters=[NSMutableDictionary dictionary];
        NSMutableDictionary<NSString *,NSNumber *> *nativeXCenters=[NSMutableDictionary dictionary];
        for (NSString *role in roles) if(roots[role]) {
            [order addObject:role];
            ALB8RightRoleRecord *record=cache[role];
            CGPoint center=record.hasIconPoint?ALB19RightConvertPoint(roots[role],record.iconPoint,stack,stack):ALB6RightCenter(anchors[role],stack);
            nativeCenters[role]=@(center.y);
            nativeXCenters[role]=@(center.x);
        }
        double positions[6]={0}; unsigned hiddenMask=0;
        for (NSUInteger i=0;i<order.count;i++) {
            positions[i]=[nativeCenters[order[i]] doubleValue];
            // Palette hiding does not reflow the rail. This preserves native
            // geometry while swiping and gives an identical close position.
            if(ALB5Bool([@"right.hide." stringByAppendingString:order[i]],NO)) hiddenMask|=(1u<<i);
        }
        NSMutableDictionary<NSString *,NSNumber *> *shifts=[NSMutableDictionary dictionary];
        for (NSUInteger i=0;i<order.count;i++)
            shifts[order[i]]=@(ALB8RightReflowShift(positions,(unsigned)order.count,hiddenMask,(unsigned)i));
        if((hiddenMask & (1u<<(order.count?order.count-1:0))) && roots[@"music"]) {
            NSString *last=nil;
            for(NSString *role in order) if(!ALB8RightRoleHidden(role))last=role;
            UIView *lastRoot=last?roots[last]:nil;
            if(lastRoot && ![last isEqualToString:@"music"]) {
                CGFloat musicBottom=CGRectGetMaxY(ALB19RightConvertRect(roots[@"music"],roots[@"music"].bounds,stack,stack));
                CGFloat rowBottom=CGRectGetMaxY(ALB19RightConvertRect(lastRoot,lastRoot.bounds,stack,stack))+[shifts[last] doubleValue];
                CGFloat correction=MIN(0,musicBottom-rowBottom);
                for(NSString *role in order) if(!ALB8RightRoleHidden(role)) shifts[role]=@([shifts[role] doubleValue]+correction);
            }
        }
        // Four complete, visible action rows only. Keep existing first/last
        // positions and all label offsets; no movement of the progress bar,
        // avatar, music, or share anchor. Reads use the native-coordinate map.
        if (equalActionSpacing) {
            NSArray<NSString *> *actions=@[@"like",@"comment",@"favorite",@"share"];
            double centers[4]={0}, equalShifts[4]={0};
            BOOL complete=YES;
            for (NSUInteger i=0;i<4;i++) {
                NSString *role=actions[i];
                ALB8RightRoleRecord *record=cache[role];
                if (!roots[role] || !nativeCenters[role] || !record.hasIconPoint ||
                    !icons[role] || ALB8RightRoleHidden(role)) { complete=NO; break; }
                centers[i]=[nativeCenters[role] doubleValue]+[shifts[role] doubleValue];
            }
            if (complete && ALB23RightEqualActionShifts(centers,equalShifts)) {
                // Endpoints have zero correction by construction.
                for (NSUInteger i=1;i<3;i++) {
                    NSString *role=actions[i];
                    shifts[role]=@([shifts[role] doubleValue]+equalShifts[i]);
                }
            }
        }
        UIView *share=roots[@"share"];
        CGPoint pivot=share?ALB6RightCountCenter(share,stack):CGPointMake(CGRectGetMidX(stack.bounds),CGRectGetMaxY(stack.bounds));
        pivot.y += [shifts[@"share"] doubleValue];
        // Fix only the hidden-Share tail. Anchor the remaining COUNT, not its
        // icon or wrapper bottom, to precisely the Share-visible count baseline.
        // Keep the same scale pivot as that reference layout; changing the
        // pivot when Share disappears was adding another scaled inter-row gap.
        if (share && ALB5Bool(@"right.hide.share",NO) && !gALB8PaletteRightHidden) {
            NSUInteger shareIndex=[order indexOfObject:@"share"];
            UIView *music=roots[@"music"];
            BOOL hiddenMusic=music && ALB5Bool(@"right.hide.music",NO);
            CGFloat shareBottom=CGRectGetMaxY(ALB19RightConvertRect(share,share.bounds,stack,stack));
            CGFloat musicBottom=music ? CGRectGetMaxY(ALB19RightConvertRect(music,music.bounds,stack,stack)) : 0;
            CGFloat referenceShift=ALB29RightReferenceShift(positions,(unsigned)order.count,
                hiddenMask,(unsigned)shareIndex,shareBottom,musicBottom,hiddenMusic);
            unsigned actionMask=0,nativeInvisibleMask=0;
            for (NSUInteger i=0;i<order.count;i++) {
                NSString *role=order[i];
                if ([@[@"like",@"comment",@"favorite"] containsObject:role]) actionMask|=(1u<<i);
                if (!ALB6RightNativeVisible(anchors[role] ?: roots[role],stack)) nativeInvisibleMask|=(1u<<i);
            }
            int lastIndex=ALB32RightLastActionIndex((unsigned)order.count,actionMask,hiddenMask,nativeInvisibleMask);
            NSString *last=lastIndex>=0 ? order[(NSUInteger)lastIndex] : nil;
            if (last) {
                CGFloat correction=ALB29RightCountCorrection(ALB29RightCountBottom(share,stack),referenceShift,
                    ALB29RightCountBottom(roots[last],stack),[shifts[last] doubleValue]);
                for (NSString *role in order)
                    if (![role isEqualToString:@"music"] && !ALB8RightRoleHidden(role))
                        shifts[role]=@([shifts[role] doubleValue]+correction);
                pivot=ALB6RightCountCenter(share,stack);
                pivot.y+=referenceShift;
            }
        }
        // The music center is the fixed scale origin. Shrinking moves ONLY
        // the upper rail toward it; hidden-Share reflow cannot raise music.
        NSString *axisRole=nil;
        for (NSString *candidate in @[@"share",@"favorite",@"comment",@"like",@"avatar"])
            if (roots[candidate] && nativeXCenters[candidate] && !ALB8RightRoleHidden(candidate) &&
                ALB6RightNativeVisible(anchors[candidate],stack)) {
                axisRole=candidate; break;
            }
        CGFloat axisX=axisRole ? [nativeXCenters[axisRole] doubleValue] : pivot.x;
        BOOL visibleMusic=roots[@"music"] && !ALB8RightRoleHidden(@"music");
        if (visibleMusic) {
            pivot=CGPointMake(axisX,[nativeCenters[@"music"] doubleValue]);
            shifts[@"music"]=@0;
            // Actual empty space: bottom of the last visible row (including
            // its count label) to the upper edge of the music glass. A hidden
            // Share automatically selects Favorite/Comment/Like instead.
            if (axisRole) {
                UIButton *lastGlass=nil;
                for (UIView *part in decorations[axisRole])
                    if ([part isKindOfClass:UIButton.class]) { lastGlass=(UIButton *)part; break; }
                CGFloat rowBottom=ALB29RightCountBottom(roots[axisRole],stack)+[shifts[axisRole] doubleValue];
                if (lastGlass) rowBottom=MAX(rowBottom,
                    CGRectGetMaxY(ALB19RightConvertRect(lastGlass,lastGlass.bounds,stack,stack))+[shifts[axisRole] doubleValue]);
                UIView *musicSurface=musicGlass ?: anchors[@"music"];
                CGFloat musicTop=CGRectGetMinY(ALB19RightConvertRect(musicSurface,musicSurface.bounds,stack,stack));
                CGFloat delta=ALB42RightUpperGapShift(musicTop,rowBottom,musicGap,columnScale);
                for (NSString *role in order)
                    if (![role isEqualToString:@"music"] && !ALB8RightRoleHidden(role))
                        shifts[role]=@([shifts[role] doubleValue]+delta);
            }
        }

        NSArray<ALB6RightEdit *> *previous=objc_getAssociatedObject(stack,&kALB5RightEditsKey);
        NSMutableArray<ALB6RightEdit *> *edits=[NSMutableArray array];
        NSMutableArray<UIView *> *covered=[NSMutableArray arrayWithArray:allRoots];
        for (NSString *role in order) {
            UIView *root=roots[role]; BOOL hidden=ALB8RightRoleHidden(role);
            CGFloat shift=[shifts[role] doubleValue];
            CGFloat scale=columnScale;
            CGFloat xShift=(axisX-[nativeXCenters[role] doubleValue])*columnScale;
            // One column pivot makes right.scale a true whole-rail scale,
            // including the music item and the Share-to-music spacing.
            CGPoint rolePivot=pivot;
            NSMutableArray<UIView *> *parts=[NSMutableArray arrayWithObject:root];
            for (UIView *part in decorations[role]) {
                if (!part.superview || !ALB6RightContains(stack,part)) continue;
                BOOL inherited=NO;
                for (UIView *owner in covered) if (ALB6RightContains(owner,part)) { inherited=YES; break; }
                if (!inherited) { [parts addObject:part]; [covered addObject:part]; }
            }
            for (UIView *v in parts) {
                if (hidden || (fabs(scale-1)<.0001 && fabs(xShift)<.0001 && fabs(shift)<.0001)) continue;
                ALB6RightEdit *record=objc_getAssociatedObject(v,&kALB8RightTransformKey);
                if (record && record.stack!=stack) continue;
                CGAffineTransform native=record?record.originalTransform:v.layer.affineTransform;
                CGFloat determinant=native.a*native.d-native.b*native.c;
                if (!isfinite(determinant) || fabs(determinant)<=1e-8) continue;
                // Recover the native input using the OLD projection before
                // publishing the new pivot/scale. No temporary layer writes.
                CGRect nativeFrame=ALB19RightConvertRect(v,v.bounds,v.superview,stack);
                CGAffineTransform desired=ALB6RightViewTransform(v,stack,rolePivot,xShift,shift,scale);
                if (!ALB19RightFiniteTransform(desired)) continue;
                if (!record) record=[ALB6RightEdit new];
                record.view=v;record.stack=stack;record.originalTransform=native;
                record.nativeFrame=nativeFrame;record.pivot=rolePivot;
                record.xShift=xShift;record.shift=shift;record.scale=scale;record.appliedTransform=desired;
                record.projection=CGAffineTransformConcat(CGAffineTransformInvert(native),desired);
                record.projectionActive=YES;
                [edits addObject:record];
                objc_setAssociatedObject(v,&kALB8RightTransformKey,record,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
                ALB19RightWriteTransform(v,desired);
            }
        }
        // Restore only removed/rebound roles, or roles whose setting is back
        // to identity. Stable roles keep their current presentation matrix.
        for (ALB6RightEdit *old in previous) {
            if ([edits containsObject:old]) continue;
            UIView *v=old.view;
            if (!v || objc_getAssociatedObject(v,&kALB8RightTransformKey)!=old) continue;
            old.projectionActive=NO;
            objc_setAssociatedObject(v,&kALB8RightTransformKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            ALB19RightWriteTransform(v,old.originalTransform);
        }
        objc_setAssociatedObject(stack,&kALB5RightEditsKey,edits,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    } @finally {
        [CATransaction commit];
        objc_setAssociatedObject(stack,&kALB6RightApplyingKey,nil,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}
static void ALB8SetPaletteRightHidden(BOOL hidden) {
    if (gALB8PaletteRightHidden==hidden) return;
    gALB8PaletteRightHidden=hidden;
    for (UIView *stack in ALB8RightStacks().allObjects) ALB5ApplyRightLayout(stack);
    UIView *current=AWEFindCurrentRightActionStackAcrossWindows();
    if (current) ALB5ApplyRightLayout(current);
}
#endif
