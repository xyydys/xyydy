/* Observe Douyin's own pan; never replace its delegate, touch delivery, or offset. */
#include "ALBScrollIntent.h"
#include "ALB7BottomMath.h"
static char kALBRootPanObserverKey;
static NSHashTable<UIScrollView *> *gALBObservedRoots;
static BOOL ALB5IsFeedRootPage(void) {
    NSInteger page = AWECurrentMainTabIndex();
    return page == 0 || (page == 1 && !AWESecondTabIsShop());
}
/* Both modes consume the same root-pan direction. Only native-left geometry
   and animation are delegated to UIKit; app lists are never rebound. */
static NSUInteger gALB5IdleGeneration;
static BOOL gALB6IdleScheduled;
static BOOL gALB5TimerCollapsed;
static __weak UITableView *gALB6VisibleFeedTable;
static NSString *gALB6CollapseLastEvent = @"尚未读取作品位置";
static NSString *ALB6CollapseDiagnostics(void) {
    return [NSString stringWithFormat:@"%@；当前%@；列表=%@；AWEFeedTableView类=%@；定时=%@；所有根页面共用手势；两种模式共用方向手势",
        gALB6CollapseLastEvent ?: @"等待作品", gAWEAlbumMorphCompact ? @"收缩" : @"展开",
        gALB6VisibleFeedTable ? NSStringFromClass(gALB6VisibleFeedTable.class) : @"未发现",
        NSClassFromString(@"AWEFeedTableView") ? @"存在" : @"不存在", gALB6IdleScheduled ? @"等待" : @"未计时"];
}
static BOOL ALB6FeedMayCommit(void) {
    return ALB5IsFeedRootPage() && gAWEAlbumMorphController && gAWEAlbumMorphVisible &&
        !gAWESettingsVisible && !AWECurrentRouteSuppressesBar();
}
static void ALB5ScheduleIdleCollapse(void) {
    NSUInteger generation = ++gALB5IdleGeneration;
    gALB6IdleScheduled = NO;
    if (ALB15CompactStyle()!=0 || !ALB5Bool(@"collapse.timer", NO) ||
        gAWEAlbumMorphCompact || !ALB6FeedMayCommit()) return;
    gALB6IdleScheduled = YES;
    NSInteger page = AWECurrentMainTabIndex();
    NSTimeInterval delay = ALB5Number(@"collapse.delay", 8, 1, 120);
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(delay*NSEC_PER_SEC)),dispatch_get_main_queue(),^{
        if (generation != gALB5IdleGeneration) return;
        gALB6IdleScheduled = NO;
        if (ALB15CompactStyle()!=0 || !ALB5Bool(@"collapse.timer",NO) ||
            gAWEAlbumMorphCompact || page != AWECurrentMainTabIndex() || !ALB6FeedMayCommit()) return;
        gALB5TimerCollapsed = YES;
        gALB6CollapseLastEvent = @"定时到达 → 收缩";
        [gAWEAlbumMorphController setCompactMode:YES animated:YES];
    });
}
static BOOL ALB6CompactForFeedDirection(BOOL next, BOOL timerMode, BOOL currentlyCompact,
                                      BOOL nextExpands, BOOL timerCollapsed) {
    // "Next expands" applies only to an idle-timer ball. The same next-item
    // commit must not undo a collapse that its root pan just requested.
    return ALB8FeedCompactPolicy(next,timerMode,currentlyCompact,timerCollapsed && nextExpands);
}
static void ALB6CommitFeedDirection(BOOL next, NSString *source, UIView *currentCell) {
    if (!ALB6FeedMayCommit()) return;
    BOOL timerMode = ALB15CompactStyle()==0 && ALB5Bool(@"collapse.timer", NO);
    BOOL timerCollapsed = timerMode && gALB5TimerCollapsed && gAWEAlbumMorphCompact;
    BOOL compact = ALB6CompactForFeedDirection(next, timerMode, gAWEAlbumMorphCompact,
                                             ALB5Bool(@"collapse.nextExpands", YES), timerCollapsed);
    gALB5TimerCollapsed = timerCollapsed && compact;
    gALB6CollapseLastEvent = [NSString stringWithFormat:@"%@：%@ → %@", source,
        next ? @"下一条" : @"上一条", compact ? @"收缩" : @"展开"];
    if (next || ALB5Bool(@"collapse.expandOnDown", YES))
        [gAWEAlbumMorphController setCompactMode:compact animated:YES];
    AWEAlbumMorphProbeWrite(@"FEED_ITEM_COMMIT %@", gALB6CollapseLastEvent);
    NSMutableDictionary *info=[@{@"next":@(next),@"source":source?:@"feed"} mutableCopy];
    if(currentCell)info[@"cell"]=currentCell;
    [[NSNotificationCenter defaultCenter] postNotificationName:@"ALB8FeedItemDidCommit" object:nil userInfo:info];
    ALB5ScheduleIdleCollapse();
}
static char kALB5FeedIndexKey,kALB5FeedIdentityKey;
static NSString *ALB5FeedIdentity(id model) {
    for(NSString *key in @[@"awemeId",@"awemeID",@"itemId",@"itemID"]) {
        @try { id value=[model valueForKey:key];if([value isKindOfClass:NSString.class] && [value length])return value;
            if([value isKindOfClass:NSNumber.class])return [value stringValue]; } @catch(__unused NSException *e){}
    }
    return nil;
}
static id ALB6FeedSafeValue(id object, NSString *key) {
    if (!object) return nil;
    @try { return [object valueForKey:key]; } @catch (__unused NSException *exception) { return nil; }
}
static BOOL ALB6FeedViewIsVisible(UIView *view) {
    UIWindow *window = view.window;
    if (!window || !ALB5IsFeedRootPage()) return NO;
    UIWindow *sourceWindow = gAWEAlbumMorphSuppressedOriginalBar.window ?: AWEActiveMainWindow();
    if (sourceWindow && window != sourceWindow) return NO;
    for (UIView *cursor = view; cursor && cursor != window; cursor = cursor.superview)
        if (cursor.hidden || cursor.alpha < .01) return NO;
    CGRect frame = [view convertRect:view.bounds toView:window];
    CGRect intersection = CGRectIntersection(frame, window.bounds);
    return !CGRectIsNull(intersection) && intersection.size.width > window.bounds.size.width * .65 &&
        intersection.size.height > window.bounds.size.height * .35;
}
@interface ALB6FeedTableState : NSObject
@property(nonatomic) CGPoint lastOffset;
@property(nonatomic) CGSize lastSize;
@property(nonatomic) CFTimeInterval lastMotion;
@property(nonatomic) BOOL sampled;
@property(nonatomic) BOOL pending;
@property(nonatomic) NSInteger page,lastUserDirection;
@property(nonatomic,copy) NSIndexPath *committedPath;
@property(nonatomic,copy) NSString *committedIdentity;
@end
@implementation ALB6FeedTableState @end
static char kALB6FeedTableStateKey;
static void ALB6ObserveFeedTable(UIScrollView *scroll);
static NSString *ALB6IdentityForFeedCell(UITableViewCell *cell) {
    if (!cell) return nil;
    // Inspect only the selected cell and its known model/controller slots.
    // This is never a window tree scan, and runs only after a row settles.
    for (NSString *key in @[@"aweme",@"awemeModel",@"model"]) {
        NSString *identity = ALB5FeedIdentity(ALB6FeedSafeValue(cell, key));
        if (identity) return identity;
    }
    for (NSString *key in @[@"viewController",@"cellViewController",@"feedCellViewController",@"playVideoViewController"]) {
        id controller = ALB6FeedSafeValue(cell, key);
        for (NSString *modelKey in @[@"aweme",@"awemeModel",@"model"]) {
            NSString *identity = ALB5FeedIdentity(ALB6FeedSafeValue(controller, modelKey));
            if (identity) return identity;
        }
    }
    return nil;
}
static void ALB6ReadSettledFeedTable(UITableView *table, ALB6FeedTableState *state) {
    if (!ALB6FeedViewIsVisible(table) || gAWESettingsVisible) return;
    // UITableView's bounds are in content coordinates. A horizontal photo
    // pager inside this row can never change this outer index path.
    CGPoint center = CGPointMake(CGRectGetMidX(table.bounds), CGRectGetMidY(table.bounds));
    NSIndexPath *path = [table indexPathForRowAtPoint:center];
    if (!path) return;
    UITableViewCell *cell = [table cellForRowAtIndexPath:path];
    if (!cell) return;
    NSString *identity = ALB6IdentityForFeedCell(cell);
    NSIndexPath *oldPath = state.committedPath;
    NSString *oldIdentity = state.committedIdentity;
    state.committedPath = path;
    state.committedIdentity = identity;
    gALB6VisibleFeedTable = table;
    if (!oldPath) {
        gALB6CollapseLastEvent = [NSString stringWithFormat:@"已读取作品位置 %ld/%ld",(long)path.section,(long)path.row];
        ALB5ScheduleIdleCollapse();
        return;
    }
    BOOL known=identity.length>0 && oldIdentity.length>0;
    int direction=ALB8CommittedWorkDirection(oldPath.section,oldPath.row,path.section,path.row,
        known,known && [identity isEqualToString:oldIdentity],state.lastUserDirection);
    if(direction)ALB6CommitFeedDirection(direction>0,[oldPath isEqual:path]?@"复用作品位置":@"作品列表",cell);
    state.lastUserDirection=0;
}
static void ALB6ScheduleFeedSettle(UITableView *table, ALB6FeedTableState *state) {
    if (state.pending) return;
    state.pending = YES;
    __weak UITableView *weakTable = table;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(.09*NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        UITableView *live = weakTable;
        state.pending = NO;
        if (!live || !ALB6FeedViewIsVisible(live) || gAWESettingsVisible || AWECurrentRouteSuppressesBar()) return;
        if (live.isDragging || live.isDecelerating || CACurrentMediaTime()-state.lastMotion < .075) {
            ALB6ScheduleFeedSettle(live, state);
            return;
        }
        ALB6ReadSettledFeedTable(live, state);
    });
}
static void ALB6ObserveFeedTable(UIScrollView *scroll) {
    if (![scroll isKindOfClass:UITableView.class] || !ALB5IsFeedRootPage() || gAWESettingsVisible) return;
    UITableView *table = (UITableView *)scroll;
    if (!ALB6FeedViewIsVisible(table) || AWECurrentRouteSuppressesBar()) return;
    ALB6FeedTableState *state = objc_getAssociatedObject(table, &kALB6FeedTableStateKey);
    if (!state) {
        state = [ALB6FeedTableState new]; state.page = AWECurrentMainTabIndex();
        objc_setAssociatedObject(table, &kALB6FeedTableStateKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    if (state.page != AWECurrentMainTabIndex()) {
        state.page = AWECurrentMainTabIndex(); state.committedPath = nil; state.committedIdentity = nil; state.sampled = NO;
    }
    CGPoint offset = table.contentOffset;
    BOOL moved = !state.sampled || fabs(offset.y-state.lastOffset.y) > .25;
    BOOL resized = !CGSizeEqualToSize(state.lastSize, table.bounds.size);
    if(state.sampled && moved && (table.isDragging || table.isDecelerating) && fabs(offset.y-state.lastOffset.y)>1)
        state.lastUserDirection=offset.y>state.lastOffset.y?1:-1;
    state.lastOffset = offset; state.lastSize = table.bounds.size; state.sampled = YES;
    if (moved) state.lastMotion = CACurrentMediaTime();
    if (!state.committedPath && !table.isDragging && !table.isDecelerating) ALB6ReadSettledFeedTable(table, state);
    if (moved || resized || !state.committedPath) ALB6ScheduleFeedSettle(table, state);
}
static void ALB6ObserveFeedCell(UIView *cell) {
    if (!ALB5IsFeedRootPage() || gAWESettingsVisible) return;
    // Known feed-cell hooks also cover builds that rename the outer table.
    // Stop at the actual ancestor table; never scan the window or its gestures.
    UIView *ancestor = cell;
    for (NSUInteger depth = 0; ancestor && depth < 14; depth++, ancestor = ancestor.superview) {
        if ([ancestor isKindOfClass:UITableView.class]) {
            ALB6ObserveFeedTable((UIScrollView *)ancestor);
            return;
        }
    }
}
static void ALB5FeedIndexWillChange(UIViewController *host, NSInteger index) {
    if (!ALB5IsFeedRootPage() || objc_getAssociatedObject(host,&kALB5FeedIndexKey)) return;
    id old = ALB6FeedSafeValue(host, @"currentIndex");
    if ([old isKindOfClass:NSNumber.class] && [old integerValue]>=0 && [old integerValue]!=index)
        objc_setAssociatedObject(host,&kALB5FeedIndexKey,old,OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALB5FeedIndexDidChange(UIViewController *host, NSInteger index, id model) {
    if (index<0 || !ALB6FeedViewIsVisible(host.viewIfLoaded)) return;
    NSString *identity = ALB5FeedIdentity(model);
    NSString *oldIdentity = objc_getAssociatedObject(host,&kALB5FeedIdentityKey);
    NSNumber *previous = objc_getAssociatedObject(host,&kALB5FeedIndexKey);
    objc_setAssociatedObject(host,&kALB5FeedIndexKey,@(index),OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (identity) objc_setAssociatedObject(host,&kALB5FeedIdentityKey,identity,OBJC_ASSOCIATION_COPY_NONATOMIC);
    if (gALB6VisibleFeedTable && ALB6FeedViewIsVisible(gALB6VisibleFeedTable)) {
        ALB6ObserveFeedTable(gALB6VisibleFeedTable);
        return; // The live outer row is authoritative; no double commits.
    }
    if (identity && [identity isEqualToString:oldIdentity]) return;
    if (previous && previous.integerValue!=index)
        ALB6CommitFeedDirection(index>previous.integerValue, @"作品回调",host.viewIfLoaded);
    else if (!previous) ALB5ScheduleIdleCollapse();
}

static BOOL ALBIsRootScroll(UIScrollView *scroll) {
    if (![scroll isKindOfClass:UIScrollView.class] || !scroll.window) return NO;
    if ([scroll.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."]) return NO;
    NSString *name=NSStringFromClass(scroll.class).lowercaseString;
    if ([name containsString:@"comment"] || [name containsString:@"keyboard"] ||
        [name containsString:@"input"] || [name containsString:@"horizontal"]) return NO;
    // Accept root-sized vertical content instead of a three-class whitelist.
    // Small inner photo carousels / comment lists must never own the tab bar.
    CGSize size=scroll.bounds.size, screen=scroll.window.bounds.size;
    if (size.width<screen.width*.70 || size.height<screen.height*.40) return NO;
    BOOL vertical=scroll.alwaysBounceVertical || scroll.contentSize.height>size.height+8 ||
        [name containsString:@"feedtable"] || [name containsString:@"profilestick"];
    if (!vertical) return NO;
    for (UIView *v=scroll.superview; v && v!=scroll.window; v=v.superview) {
        if ([v.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."]) return NO;
        NSString *ancestor=NSStringFromClass(v.class).lowercaseString;
        if ([ancestor containsString:@"comment"] || [ancestor containsString:@"sheet"] ||
            [ancestor containsString:@"keyboard"]) return NO;
        if ([v isKindOfClass:UIScrollView.class] && v.bounds.size.height>size.height*.9 &&
            ((UIScrollView *)v).contentSize.height>v.bounds.size.height+8) return NO;
    }
    return YES;
}
static BOOL ALBRootMatchesSelection(UIScrollView *scroll) {
    UIWindow *source=gAWEAlbumMorphSuppressedOriginalBar.window ?: AWEActiveMainWindow();
    if (!ALBIsRootScroll(scroll) || (source && scroll.window!=source) ||
        AWEAnyVisibleCommentPanelForRightRecovery()) return NO;
    for (UIView *v=scroll; v && v!=scroll.window; v=v.superview)
        if (v.hidden || v.alpha<.01) return NO;
    CGRect rect=[scroll convertRect:scroll.bounds toView:scroll.window];
    CGRect visible=CGRectIntersection(rect,scroll.window.bounds);
    return !CGRectIsNull(visible) && visible.size.height>scroll.window.bounds.size.height*.35 &&
        visible.size.width>scroll.window.bounds.size.width*.65;
}

@interface AWEAlbumMorphCollapseGestureDelegate : NSObject <UIGestureRecognizerDelegate> {
    ALBPanState _panState;
}
@property(nonatomic, weak) UIScrollView *source;
- (void)alb_handleCollapsePan:(UIPanGestureRecognizer *)pan;
@end
static void ALBInstallRootPanObserver(UIScrollView *scroll);
@implementation AWEAlbumMorphCollapseGestureDelegate
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture
    shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other { return YES; }
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    if (!AWEAlbumMorphCollapseEnabled() || !gAWEAlbumMorphVisible ||
        gAWESettingsVisible || AWECurrentRouteSuppressesBar()) return NO;
    for (UIView *v = touch.view; v && v != gesture.view; v = v.superview) {
        if ([v.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."] ||
            ([v isKindOfClass:UIControl.class] && v.bounds.size.height < 160)) return NO;
        if (ALBIsRootScroll((UIScrollView *)v)) {
            ALBInstallRootPanObserver((UIScrollView *)v);
            // This same sequence already reaches the real pan. No competing
            // recognizer / second commit from the fallback window pan.
            return NO;
        }
    }
    CGPoint point = [touch locationInView:gesture.view];
    return point.y > MAX(90, gesture.view.safeAreaInsets.top + 44) &&
        point.y < gesture.view.bounds.size.height - MAX(88, gesture.view.safeAreaInsets.bottom + 58);
}
- (void)alb_handleCollapsePan:(UIPanGestureRecognizer *)pan {
    if (!AWEAlbumMorphCollapseEnabled() || !gAWEAlbumMorphVisible || gAWESettingsVisible ||
        !gAWEAlbumMorphController || AWECurrentRouteSuppressesBar()) {
        if (pan.state == UIGestureRecognizerStateBegan)
            AWEAlbumMorphProbeWrite(@"ROOT_PAN_BLOCKED enabled=%d visible=%d settings=%d route=%d",
                AWEAlbumMorphCollapseEnabled(), gAWEAlbumMorphVisible, gAWESettingsVisible, AWECurrentRouteSuppressesBar());
        return;
    }
    if (self.source && !ALBRootMatchesSelection(self.source)) {
        if (pan.state == UIGestureRecognizerStateBegan)
            AWEAlbumMorphProbeWrite(@"ROOT_PAN_SOURCE_REJECTED source=%@ selected=%ld sameWindow=%d",
                NSStringFromClass(self.source.class), (long)gAWEAlbumMorphController.currentOriginalIndex,
                self.source.window == gAWEAlbumMorphController.view.window);
        return;
    }
    if (pan.state == UIGestureRecognizerStateBegan) {
        ALBPanBegin(&_panState, 0, 0);
        if (self.source) [gAWEAlbumMorphController bindSourceScrollView:self.source];
        [gAWEAlbumMorphController prepareMinimizeForGesture];
        AWEAlbumMorphProbeWrite(@"ROOT_PAN_BEGIN source=%@ sameWindow=%d parent=%@",
            self.source ? NSStringFromClass(self.source.class) : @"window-fallback",
            pan.view.window == gAWEAlbumMorphController.view.window,
            NSStringFromClass(gAWEAlbumMorphController.parentViewController.class));
    }
    if (pan.state != UIGestureRecognizerStateBegan && pan.state != UIGestureRecognizerStateChanged &&
        pan.state != UIGestureRecognizerStateEnded) return;
    CGPoint delta = [pan translationInView:pan.view], velocity = [pan velocityInView:pan.view];
    int direction = ALBPanUpdate(&_panState, delta.x, delta.y, velocity.x, velocity.y, 4);
    if (!direction) return;
    // Readback is authoritative; an intent must not declare a native state.
    if (direction > 0 || ALB5Bool(@"collapse.expandOnDown", YES)) {
        // An upward pan from expanded chrome owns this collapse. Preserve an
        // existing timer ball's owner so its next-item preference still works.
        if (!gAWEAlbumMorphCompact || direction<0) gALB5TimerCollapsed=NO;
        [gAWEAlbumMorphController setCompactMode:direction>0 animated:YES];
    }
    AWEAlbumMorphProbeWrite(@"ROOT_PAN_COMMIT compact=%d target=%d source=%@ dy=%.1f vy=%.1f",
        gAWEAlbumMorphCompact, direction>0, self.source ? NSStringFromClass(self.source.class) : @"window-fallback",
        delta.y, velocity.y);
}
@end

static void ALBInstallRootPanObserver(UIScrollView *scroll) {
    if (!AWEAlbumMorphCollapseEnabled() || !gAWEAlbumMorphController || !ALBIsRootScroll(scroll)) return;
    UIPanGestureRecognizer *pan = scroll.panGestureRecognizer;
    if (!objc_getAssociatedObject(pan, &kALBRootPanObserverKey)) {
        AWEAlbumMorphCollapseGestureDelegate *observer = [AWEAlbumMorphCollapseGestureDelegate new];
        observer.source = scroll;
        [pan addTarget:observer action:@selector(alb_handleCollapsePan:)];
        objc_setAssociatedObject(pan, &kALBRootPanObserverKey, observer, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!gALBObservedRoots) gALBObservedRoots = [NSHashTable weakObjectsHashTable];
        [gALBObservedRoots addObject:scroll];
        AWEAlbumMorphProbeWrite(@"ROOT_PAN_INSTALLED source=%@", NSStringFromClass(scroll.class));
    }
    if (ALBRootMatchesSelection(scroll)) [gAWEAlbumMorphController bindSourceScrollView:scroll];
}
static void ALBRemoveRootPanObservers(void) {
    for (UIScrollView *scroll in gALBObservedRoots.allObjects) {
        id observer = objc_getAssociatedObject(scroll.panGestureRecognizer, &kALBRootPanObserverKey);
        if (observer) [scroll.panGestureRecognizer removeTarget:observer action:@selector(alb_handleCollapsePan:)];
        objc_setAssociatedObject(scroll.panGestureRecognizer, &kALBRootPanObserverKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    [gALBObservedRoots removeAllObjects];
}
static void AWEInstallAlbumMorphCollapseGesture(UIWindow *window) {
    if (ALB5IsFeedRootPage() && ALB15CompactStyle()==0) {
        // Resume timed mode after a settings/route transition has restored
        // visibility. Ordinary layout passes must NOT reset an armed timer.
        if (!ALB5Bool(@"collapse.timer", NO) || gAWESettingsVisible) {
            ++gALB5IdleGeneration;
            gALB6IdleScheduled = NO;
        } else if (!gALB6IdleScheduled && !gAWEAlbumMorphCompact && ALB6FeedMayCommit()) {
            ALB5ScheduleIdleCollapse();
        }
    } else if (gALB6IdleScheduled) { ++gALB5IdleGeneration; gALB6IdleScheduled=NO; }
    if (!window || !AWEAlbumMorphCollapseEnabled() || gAWESettingsVisible) {
        ALBRemoveRootPanObservers();
        [gAWEAlbumMorphCollapseGesture.view removeGestureRecognizer:gAWEAlbumMorphCollapseGesture];
        gAWEAlbumMorphCollapseGesture = nil;
        gAWEAlbumMorphCollapseGestureWindow = nil;
        gAWEAlbumMorphCollapseGestureDelegate = nil;
        return;
    }
    // Cached root pages need binding even when didMoveToWindow doesn't fire.
    static CFTimeInterval lastScan;
    if (CACurrentMediaTime() - lastScan > 0.3) {
        lastScan = CACurrentMediaTime();
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:window];
        for (NSUInteger i = 0; i < queue.count && i < 1000; i++) {
            UIView *v = queue[i];
            if ([v.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."]) continue;
            if (ALBIsRootScroll((UIScrollView *)v)) ALBInstallRootPanObserver((UIScrollView *)v);
            [queue addObjectsFromArray:v.subviews];
        }
    }
    if (gAWEAlbumMorphCollapseGesture.view == window) return;
    [gAWEAlbumMorphCollapseGesture.view removeGestureRecognizer:gAWEAlbumMorphCollapseGesture];
    AWEAlbumMorphCollapseGestureDelegate *observer = [AWEAlbumMorphCollapseGestureDelegate new];
    UIPanGestureRecognizer *pan = [[UIPanGestureRecognizer alloc] initWithTarget:observer action:@selector(alb_handleCollapsePan:)];
    pan.cancelsTouchesInView = NO;
    pan.delaysTouchesBegan = NO;
    pan.delaysTouchesEnded = NO;
    pan.maximumNumberOfTouches = 1;
    pan.delegate = observer;
    [window addGestureRecognizer:pan];
    gAWEAlbumMorphCollapseGesture = pan;
    gAWEAlbumMorphCollapseGestureWindow = window;
    gAWEAlbumMorphCollapseGestureDelegate = observer;
}
