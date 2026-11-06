/* 3.11.46 背后亮度（和三段式底栏同一种读数：CABackdropLayer 的 tracksLuma，不截图）。
   插件自己的提示条、悬浮面板头部用它决定字是黑是白：玻璃本身固定纯透（Clear），不跟着翻深浅（用户：“我们自己插件的
   那个弹窗提示……它老是切来切去的，直接就用纯透按钮，保证内部的字体能够自适应背景切换黑白就行”）。
   onDark(YES)＝背后亮、该用黑字；带 0.62 / 0.54 的回差。 */

@interface ALBLumaProbe : UIView
@property (nonatomic, copy) void (^onDark)(BOOL darkInk);
@property (nonatomic, assign) NSInteger state;   // -1 未定 0 白字 1 黑字
@property (nonatomic, assign) double lastLuma;
@end

@implementation ALBLumaProbe
+ (Class)layerClass {
    Class backdrop = NSClassFromString(@"CABackdropLayer");
    return backdrop ?: [CALayer class];
}
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        _state = -1;
        _lastLuma = -1;
        self.userInteractionEnabled = NO;
        self.isAccessibilityElement = NO;
        self.accessibilityElementsHidden = YES;
        self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        CALayer *layer = self.layer;
        @try {
            if ([layer respondsToSelector:NSSelectorFromString(@"setTracksLuma:")]) [layer setValue:@YES forKey:@"tracksLuma"];
            if ([layer respondsToSelector:NSSelectorFromString(@"setLumaUpdateRate:")]) [layer setValue:@0.15 forKey:@"lumaUpdateRate"];
            if ([layer respondsToSelector:NSSelectorFromString(@"setScale:")]) [layer setValue:@0.25 forKey:@"scale"];
        } @catch (__unused NSException *e) {}
        layer.opacity = 0.02f;
    }
    return self;
}
- (void)alb_apply:(double)luma {
    if (!isfinite(luma)) return;
    self.lastLuma = luma;
    NSInteger want = self.state;
    if (want == -1) want = luma > 0.58 ? 1 : 0;
    else if (want == 0 && luma > 0.62) want = 1;
    else if (want == 1 && luma < 0.54) want = 0;
    if (want == self.state) return;
    self.state = want;
    if (self.onDark) self.onDark(want == 1);
}
- (void)backdropLayer:(CALayer *)layer didChangeLuma:(double)luma {
    if (![NSThread isMainThread]) {
        __weak typeof(self) weakSelf = self;
        dispatch_async(dispatch_get_main_queue(), ^{ [weakSelf alb_apply:luma]; });
        return;
    }
    [self alb_apply:luma];
}
@end
