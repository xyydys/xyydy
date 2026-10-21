/* One low-resolution compositor capture feeds every visible row. The palette
   samples at up to 8 Hz, updates each row independently, and uses luminance hysteresis
   instead of the previous two half-second confirmations. */
static UIImage *AWECreateLowResolutionBackdropSnapshot(void);
static CGFloat AWEIOS27AverageLumaInPatch(UIImage *, CGPoint, CGFloat);
static CGFloat AWEIOS27MedianLuma(CGFloat *, NSUInteger);

// TEST18: return the last snapshot during layout. Capture at most once on a
// later main-queue turn. Keep `queued` set while drawing, because drawing may
// cause UIKit to lay out controls which read this same cache again.
static UIImage *ALB8ContrastSnapshot(UIWindow *window) {
    static __weak UIWindow *capturedWindow;
    static UIImage *snapshot;
    static CFTimeInterval requestedAt;
    static BOOL queued;
    if (!NSThread.isMainThread || !window || gAWESettingsVisible ||
        UIApplication.sharedApplication.applicationState != UIApplicationStateActive) return nil;
    if (capturedWindow != window) {
        capturedWindow = window;
        snapshot = nil;
        requestedAt = 0;
    }
    CFTimeInterval now = CACurrentMediaTime();
    if (!queued && now - requestedAt >= 0.12) {
        queued = YES;
        requestedAt = now;
        __weak UIWindow *weakWindow = window;
        dispatch_async(dispatch_get_main_queue(), ^{
            @try {
                UIWindow *liveWindow = weakWindow;
                if (!liveWindow || liveWindow != capturedWindow ||
                    liveWindow != AWEActiveMainWindow() || gAWESettingsVisible ||
                    UIApplication.sharedApplication.applicationState != UIApplicationStateActive) return;
                UIImage *next = AWECreateLowResolutionBackdropSnapshot();
                if (next && liveWindow == capturedWindow) {
                    snapshot = next;
                    // Apply the newly available sample on this turn. Waiting
                    // for a second timer tick made each row visibly lag.
                    [NSNotificationCenter.defaultCenter postNotificationName:@"ALB28ContrastSnapshotReady" object:liveWindow];
                }
            } @finally {
                queued = NO;
            }
        });
    }
    return snapshot;
}
static BOOL ALB8ContrastSurfaceVisible(UIView *surface,UIWindow *window) {
    if (!surface || surface.window!=window) return NO;
    CGRect visible=[surface convertRect:surface.bounds toView:window];
    for(UIView *v=surface;v;v=v.superview) {
        if(v.hidden || v.alpha<.01 || v.layer.opacity<.01)return NO;
        if(v.clipsToBounds)visible=CGRectIntersection(visible,[v convertRect:v.bounds toView:window]);
        if(CGRectIsNull(visible) || CGRectIsEmpty(visible))return NO;
    }
    return CGRectIntersectsRect(visible,window.bounds);
}
static BOOL ALB8ContrastLightInk(UIView *surface,UIWindow *window,UIImage *snapshot,
                               BOOL inside,BOOL hasSample,BOOL previous) {
    CGFloat values[8]; NSUInteger count=0;
    CGRect frame=[surface convertRect:surface.bounds toView:window];
    for (NSUInteger row=0;row<2;row++) for (NSUInteger column=0;column<4;column++) {
        CGFloat x=CGRectGetMinX(frame)+CGRectGetWidth(frame)*(.16+.22*column);
        // Keep samples clear of glyph/text centers. Surface fill is allowed:
        // readability is determined against the displayed local material.
        CGFloat y=inside ? CGRectGetMinY(frame)+CGRectGetHeight(frame)*(row?.86:.14)
            : (row?CGRectGetMaxY(frame)+10:CGRectGetMinY(frame)-10);
        CGPoint point=CGPointMake(x,MIN(MAX(window.safeAreaInsets.top+4,y),
            window.bounds.size.height-window.safeAreaInsets.bottom-4));
        CGPoint screen=[window convertPoint:point toWindow:nil];
        CGFloat value=snapshot?AWEIOS27AverageLumaInPatch(snapshot,screen,8):-1;
        if(value>=0)values[count++]=value;
    }
    if(count<3)return previous;
    CGFloat luma=AWEIOS27MedianLuma(values,count);
    if(!hasSample)return luma<.5;
    if(previous && luma>.54)return NO;
    if(!previous && luma<.46)return YES;
    return previous;
}
@interface ALBClearContrast : NSObject
@property(nonatomic,weak) UIView *surface;
@property(nonatomic,strong) NSArray<UIView *> *surfaces;
@property(nonatomic,strong) NSMutableArray *rowInk;
@property(nonatomic,strong) NSTimer *timer;
@property(nonatomic,copy) void (^apply)(BOOL lightInk);
@property(nonatomic,copy) void (^applyRow)(NSUInteger row,BOOL lightInk);
@property(nonatomic,copy) void (^poll)(void);
@property(nonatomic) BOOL hasSample,lightInk,samplesInside;
- (void)refresh;
- (void)snapshotReady:(NSNotification *)notification;
- (void)stop;
@end
@implementation ALBClearContrast
- (void)dealloc { [_timer invalidate]; [NSNotificationCenter.defaultCenter removeObserver:self]; }
- (void)stop {
    [self.timer invalidate]; self.timer=nil;
    [NSNotificationCenter.defaultCenter removeObserver:self name:@"ALB28ContrastSnapshotReady" object:nil];
}
- (void)snapshotReady:(NSNotification *)notification {
    (void)notification;
    [self refresh];
}
- (void)refresh {
    if(gAWESettingsVisible || UIApplication.sharedApplication.applicationState!=UIApplicationStateActive)return;
    UIWindow *window=AWEActiveMainWindow();
    UIView *surface=self.surface;
    if(self.surfaces.count) {
        surface=nil;
        for(UIView *candidate in self.surfaces)
            if(ALB8ContrastSurfaceVisible(candidate,window)) { surface=candidate; break; }
    }
    if(!window || !ALB8ContrastSurfaceVisible(surface,window))return;
    if(self.poll)self.poll();
    UIImage *snapshot=ALB8ContrastSnapshot(window);
    BOOL fallback=gAWETransparentBackdropContrastValid?gAWETransparentBackdropForegroundLight:
        (gAWEHomeAdaptiveForegroundValid?gAWEHomeAdaptiveForegroundIsLight:AWESystemInterfaceStyle()==UIUserInterfaceStyleDark);
    if(self.surfaces.count) {
        for(NSUInteger row=0;row<self.surfaces.count;row++) {
            UIView *view=self.surfaces[row];
            if(!ALB8ContrastSurfaceVisible(view,window))continue;
            id state=self.rowInk[row]; BOOL sampled=[state isKindOfClass:NSNumber.class];
            BOOL previous=sampled?[state boolValue]:fallback;
            BOOL next=ALB8ContrastLightInk(view,window,snapshot,YES,sampled,previous);
            self.rowInk[row]=@(next);
            if((!sampled || next!=previous) && self.applyRow)self.applyRow(row,next);
        }
    } else {
        BOOL next=ALB8ContrastLightInk(surface,window,snapshot,self.samplesInside,self.hasSample,self.hasSample?self.lightInk:fallback);
        BOOL changed=!self.hasSample || next!=self.lightInk;
        self.hasSample=YES; self.lightInk=next;
        if(changed && self.apply)self.apply(next);
    }
}
@end
static void ALB8StartContrastTimer(ALBClearContrast *observer,NSTimeInterval interval) {
    __weak ALBClearContrast *weakObserver=observer;
    observer.timer=[NSTimer timerWithTimeInterval:interval repeats:YES block:^(__unused NSTimer *timer){ [weakObserver refresh]; }];
    [NSRunLoop.mainRunLoop addTimer:observer.timer forMode:NSRunLoopCommonModes];
    [NSNotificationCenter.defaultCenter addObserver:observer selector:@selector(snapshotReady:)
        name:@"ALB28ContrastSnapshotReady" object:nil];
    [observer refresh];
}
static ALBClearContrast *ALBObserveClearContrast(UIView *surface,BOOL inside,void (^apply)(BOOL)) {
    ALBClearContrast *observer=[ALBClearContrast new];
    observer.surface=surface; observer.samplesInside=inside; observer.apply=apply;
    ALB8StartContrastTimer(observer,.14);
    return observer;
}
static ALBClearContrast *ALBObserveClearContrastRows(NSArray<UIView *> *rows,void (^apply)(NSUInteger,BOOL)) {
    ALBClearContrast *observer=[ALBClearContrast new];
    observer.surfaces=rows; observer.applyRow=apply;
    observer.rowInk=[NSMutableArray arrayWithCapacity:rows.count];
    for(__unused UIView *row in rows)[observer.rowInk addObject:NSNull.null];
    ALB8StartContrastTimer(observer,.14);
    return observer;
}
