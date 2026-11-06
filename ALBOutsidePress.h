/* 3.11.46 “碰到面板外面就关”（双击面板、悬浮面板共用）。
   用户（3.11.45）：“双击面板打开时，点任意地方不能关闭，只能上滑关闭”。3.11.45 在窗口上挂的是一个普通点按手势
   （UITapGestureRecognizer）：抖音视频上的控件（UIControl 会拒绝祖先视图上的单击手势）和它自己的点按手势经常先认，
   我们这个就被挡掉了；上下滑那一个是拖动手势，不受影响，所以“只能上滑关闭”。
   现在换成按下就开始的长按手势（minimumPressDuration 0）：
   · 按下的那一刻就关面板（不等抬手，谁也挡不住：不会被别的手势阻止，canBePreventedByGestureRecognizer 恒 NO）；
   · 抖音的点按 / 双击手势要等它失败——它是“成功”结束的，所以这一下不会暂停视频、不会点赞；
   · 拖动、滚动和它同时识别，所以上下滑照样翻到下一条；
   · 它的目标是它自己（不指向面板），面板没了它也安全；面板关掉时触摸还没结束就等这一下结束再摘掉，
     不然抖音的点按会在它被摘掉的那一刻接着认出来（视频被暂停）。
   insideControl 返回 YES 的触摸不归它（面板自己的按钮、胶囊、卡片）。 */

@interface ALBOutsidePress : UILongPressGestureRecognizer <UIGestureRecognizerDelegate>
@property (nonatomic, copy) BOOL (^insideControl)(UITouch *touch);
@property (nonatomic, copy) void (^onClose)(void);
@property (nonatomic, copy) void (^onEnded)(BOOL swipe, NSString *hitClass);
@property (nonatomic, assign) BOOL fired, retiring;
@property (nonatomic, assign) CGPoint startPoint;
@property (nonatomic, copy) NSString *hitClass;
+ (instancetype)installOnWindow:(UIWindow *)window;
- (void)retire;
@end

static NSUInteger gALBOutsidePressCloses, gALBOutsidePressSwipes;
static NSString *gALBOutsidePressLastHit;

static BOOL ALBOutsideIsTapLike(UIGestureRecognizer *other) {
    if ([other isKindOfClass:UITapGestureRecognizer.class]) return YES;
    NSString *name = NSStringFromClass(other.class);
    return [name containsString:@"Tap"] || [name containsString:@"Click"];
}

@implementation ALBOutsidePress
+ (instancetype)installOnWindow:(UIWindow *)window {
    if (!window) return nil;
    ALBOutsidePress *press = [[ALBOutsidePress alloc] initWithTarget:nil action:nil];
    [press addTarget:press action:@selector(alb_handle:)];
    press.minimumPressDuration = 0;
    press.allowableMovement = CGFLOAT_MAX;
    press.cancelsTouchesInView = NO;
    press.delaysTouchesBegan = NO;
    press.delaysTouchesEnded = NO;
    press.delegate = press;
    [window addGestureRecognizer:press];
    return press;
}
- (BOOL)canBePreventedByGestureRecognizer:(UIGestureRecognizer *)preventing { return NO; }
- (BOOL)canPreventGestureRecognizer:(UIGestureRecognizer *)prevented { return ALBOutsideIsTapLike(prevented); }
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldReceiveTouch:(UITouch *)touch {
    if (self.retiring && self.state == UIGestureRecognizerStatePossible) return NO;
    if (self.insideControl && self.insideControl(touch)) return NO;
    return YES;
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)other {
    return !ALBOutsideIsTapLike(other);
}
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gesture shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer *)other {
    return ALBOutsideIsTapLike(other) && other.view.window == self.view.window;
}
- (void)alb_handle:(ALBOutsidePress *)press {
    UIGestureRecognizerState state = press.state;
    if (state == UIGestureRecognizerStateBegan) {
        self.startPoint = [press locationInView:press.view];
        UIView *hit = [press.view hitTest:self.startPoint withEvent:nil];
        self.hitClass = hit ? NSStringFromClass(hit.class) : @"-";
        if (!self.fired && !self.retiring) {
            self.fired = YES;
            gALBOutsidePressCloses++;
            gALBOutsidePressLastHit = self.hitClass;
            void (^close)(void) = self.onClose;
            if (close) close();
        }
        return;
    }
    if (state == UIGestureRecognizerStateEnded || state == UIGestureRecognizerStateCancelled || state == UIGestureRecognizerStateFailed) {
        CGPoint p = [press locationInView:press.view];
        BOOL swipe = hypot(p.x - self.startPoint.x, p.y - self.startPoint.y) > 12;
        if (swipe) gALBOutsidePressSwipes++;
        if (self.onEnded) self.onEnded(swipe, self.hitClass);
        if (self.fired || self.retiring) [self alb_remove];
    }
}
- (void)alb_remove {
    self.onClose = nil;
    self.onEnded = nil;
    self.insideControl = nil;
    __weak UIView *view = self.view;
    ALBOutsidePress *me = self;
    dispatch_async(dispatch_get_main_queue(), ^{ [view removeGestureRecognizer:me]; });
}
/* 面板关了：触摸还在进行就等它结束再摘（见文件头），否则马上摘 */
- (void)retire {
    self.retiring = YES;
    self.onClose = nil;
    UIGestureRecognizerState state = self.state;
    if (state == UIGestureRecognizerStateBegan || state == UIGestureRecognizerStateChanged) return;
    [self alb_remove];
}
@end
