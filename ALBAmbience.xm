//
//  ALBAmbience.xm —— 点赞全屏氛围特效（3.11.33 由用户自己的独立测试版 TEST6 并入）
//
//  特效、识别、悬浮球面板的逻辑原样，并入时改了这些：
//    · 偏好放进本插件的命名空间（ALB5Key(@"fx.*")），设置页「点赞特效」一节（ALB5Settings.h 第 8 节）直接改；
//      第一次启动把独立版存过的值（AwemeAmbience_*）抄过来一次
//    · 新增「动画时长」倍率（fx.duration，0.5 ~ 2.5，1.0＝原样）：每次触发时取一次，所有时间（每颗粒子的
//      时长和出场延迟、关键帧、闪烁 / 旋转 / 翻飞的周期、边缘光、发射器、移除时刻）都乘它——
//      整段按同一比例放慢 / 加快，轨迹形状不变，只是走得慢、持续久（或反过来）
//    · 只在插件已授权且总开关开时触发（ALBDockHostPluginActive，Tweak.x）
//    · 悬浮球默认关（设置页已经有全部参数和预览）；开了才建那个窗口。三指双击照旧能唤出
//    · 两个窗口类改名（ALBFX…），和独立版同时装着也不会撞类名；检测到独立版在跑时合并版不触发（免得放两遍），诊断里写明
//    · 兼容模式的触摸钩子只在「按下」那一刻读偏好
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#include "ALB5Options.h"

// Tweak.x：插件已授权且总开关打开
FOUNDATION_EXPORT BOOL ALBDockHostPluginActive(void);

#pragma mark - 设置

#define KEY(k) ALB5Key(@"fx." k)
static NSUserDefaults *AD(void) { return [NSUserDefaults standardUserDefaults]; }
static BOOL ABool(NSString *k, BOOL def) { id v = [AD() objectForKey:k]; return v ? [v boolValue] : def; }
static double ADbl(NSString *k, double def) { id v = [AD() objectForKey:k]; return v ? [v doubleValue] : def; }
static NSInteger AInt(NSString *k, NSInteger def) { id v = [AD() objectForKey:k]; return v ? [v integerValue] : def; }

static BOOL  Enabled(void)     { return ABool(KEY("enabled"), YES); }
static BOOL  TrigButton(void)  { return ABool(KEY("trigButton"), YES); }
static BOOL  TrigDouble(void)  { return ABool(KEY("trigDouble"), YES); }
static BOOL  Haptic(void)      { return ABool(KEY("haptic"), YES); }
static BOOL  EdgeGlowOn(void)  { return ABool(KEY("edgeGlow"), YES); }
static BOOL  BallVisible(void) { return ABool(KEY("ball"), NO); }      // 3.11.33：默认关
// 3.11.52：兼容模式删了（它把任何双击——包括唤出双击面板的——都当点赞放特效；按作品认的引擎一直在用）
static BOOL  CompatMode(void)  { return NO; }
static double Intensity(void)  { return MIN(2.0, MAX(0.4, ADbl(KEY("intensity"), 1.0))); }
static NSInteger CurStyle(void){ return AInt(KEY("style"), 0); }
// 3.11.33：动画时长倍率（1.0＝原样；2.0＝整段放慢到两倍长；0.5＝快一倍）
static double DurationScale(void) { double v = ADbl(KEY("duration"), 1.0); return isfinite(v) ? MIN(2.5, MAX(0.5, v)) : 1.0; }
static NSInteger SprayMode(void){ return AInt(KEY("spray"), 0); } // 0 随机多点 1 两侧对称 2 点赞处

typedef NS_ENUM(NSInteger, AAStyle) {
    AAStyleHeart = 0, AAStyleSakura, AAStyleStar, AAStyleMeteor, AAStyleConfetti,
    AAStyleBubble, AAStyleFirework, AAStyleAI, AAStyleSnow,
    AAStyleFirefly, AAStyleEmoji, AAStyleRose, AAStyleRandom, AAStyleCount
};
static NSArray<NSString *> *StyleNames(void) {
    return @[@"心动", @"樱花", @"星光", @"流星", @"礼花", @"泡泡", @"烟花", @"智能光", @"雪花", @"萤火", @"表情雨", @"玫瑰", @"随机"];
}

static UIColor *RGB(int hex) { return [UIColor colorWithRed:((hex>>16)&0xFF)/255.0 green:((hex>>8)&0xFF)/255.0 blue:(hex&0xFF)/255.0 alpha:1]; }
static UIColor *RGBA(int hex, CGFloat a) { return [RGB(hex) colorWithAlphaComponent:a]; }

#pragma mark - 特效窗口

@interface ALBFXPassWindow : UIWindow @end
@implementation ALBFXPassWindow
- (UIView *)hitTest:(CGPoint)p withEvent:(UIEvent *)e { return nil; }
@end

static ALBFXPassWindow *gFXWindow;
static NSMutableSet<UIWindow *> *gOwnWindows;

static UIWindowScene *ActiveScene(void) {
    for (UIScene *s in UIApplication.sharedApplication.connectedScenes)
        if ([s isKindOfClass:UIWindowScene.class] && s.activationState == UISceneActivationStateForegroundActive) return (UIWindowScene *)s;
    for (UIScene *s in UIApplication.sharedApplication.connectedScenes)
        if ([s isKindOfClass:UIWindowScene.class]) return (UIWindowScene *)s;
    return nil;
}

static CALayer *FXLayer(void) {
    UIWindowScene *scene = ActiveScene();
    if (!scene) return nil;
    if (!gFXWindow || gFXWindow.windowScene != scene) {
        gFXWindow = [[ALBFXPassWindow alloc] initWithWindowScene:scene];
        gFXWindow.windowLevel = UIWindowLevelAlert + 10;
        gFXWindow.userInteractionEnabled = NO;
        gFXWindow.backgroundColor = UIColor.clearColor;
        UIViewController *vc = [UIViewController new];
        vc.view.backgroundColor = UIColor.clearColor;
        vc.view.userInteractionEnabled = NO;
        gFXWindow.rootViewController = vc;
        if (!gOwnWindows) gOwnWindows = [NSMutableSet set];
        [gOwnWindows addObject:gFXWindow];
    }
    gFXWindow.frame = scene.coordinateSpace.bounds;
    gFXWindow.hidden = NO;
    return gFXWindow.rootViewController.view.layer;
}

// 3.11.33：本次触发的时长倍率（FireAt 开头取一次），特效里所有时间都过 TS()
static double gFXTime = 1.0;
static CFTimeInterval TS(CFTimeInterval t) { return t * gFXTime; }

static void RemoveLater(CALayer *l, CFTimeInterval t) {
    // t 是特效时间轴上的时刻，按倍率换成真实时间
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(TS(t) * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ [l removeFromSuperlayer]; });
}
static void After(CFTimeInterval t, dispatch_block_t b) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(t * NSEC_PER_SEC)), dispatch_get_main_queue(), b);
}

static CAEmitterLayer *Emit(CALayer *host, CGPoint pos, CGSize size, NSString *shape, NSString *mode, NSArray *cells, CFTimeInterval emitFor, CFTimeInterval life) {
    CAEmitterLayer *e = [CAEmitterLayer layer];
    e.frame = host.bounds;
    e.emitterPosition = pos;
    e.emitterSize = size;
    e.emitterShape = shape;
    e.emitterMode = mode;
    e.renderMode = kCAEmitterLayerUnordered;
    e.beginTime = CACurrentMediaTime();
    e.speed = (float)(1.0 / gFXTime);     // 3.11.33：发射器整条时间轴按倍率放慢 / 加快
    e.emitterCells = cells;
    [host addSublayer:e];
    After(TS(emitFor), ^{ e.birthRate = 0; });
    RemoveLater(e, emitFor + life + 0.3);
    return e;
}

// 关键：contentsScale 设为屏幕倍率，scale=1 就等于贴图的“点”尺寸
static CAEmitterCell *Cell(UIImage *img, float rate, float life, CGFloat vel, CGFloat scale, CGFloat scaleRange) {
    CAEmitterCell *c = [CAEmitterCell emitterCell];
    c.contents = (__bridge id)img.CGImage;
    c.contentsScale = img.scale;
    c.birthRate = rate * Intensity();
    c.lifetime = life;
    c.lifetimeRange = life * 0.35;
    c.velocity = vel;
    c.velocityRange = vel * 0.45;
    c.scale = scale;
    c.scaleRange = scaleRange;
    return c;
}


#pragma mark - ===== 矢量粒子引擎 =====
// 每个元素是一个独立图层：外层容器负责“轨迹 + 淡入淡出”，内层负责“旋转 / 3D 翻转 / 闪烁 / 变色”。
// 发光一律用 layer.shadow + shadowPath（矢量阴影，开销小），低电量模式自动关掉发光。

static CGFloat Rnd(CGFloat a, CGFloat b) { return a + (b - a) * (arc4random_uniform(100001) / 100000.0); }
static BOOL Coin(CGFloat p) { return Rnd(0, 1) < p; }
static int Count(CGFloat base) { return MAX(1, (int)lround(base * Intensity())); }
static BOOL LowPower(void) { return NSProcessInfo.processInfo.isLowPowerModeEnabled; }
static CFTimeInterval Now(void) { return CACurrentMediaTime(); }
static int PickI(NSArray<NSNumber *> *a) { return a[arc4random_uniform((uint32_t)a.count)].intValue; }

typedef CGPoint (^AAPathFn)(CGFloat t, CGFloat sec);

static NSArray *Sample(AAPathFn f, CGFloat dur, int n) {
    NSMutableArray *a = [NSMutableArray arrayWithCapacity:n + 1];
    for (int i = 0; i <= n; i++) {
        CGFloat t = (CGFloat)i / n;
        [a addObject:[NSValue valueWithCGPoint:f(t, t * dur)]];
    }
    return a;
}

// 线性阻力 + 重力的抛体轨迹（终端速度 = g/k），可叠加左右摇摆
static NSArray *Ballistic(CGPoint p0, CGVector v, CGFloat g, CGFloat k, CGFloat dur, CGFloat swayAmp, CGFloat swayHz, int n) {
    CGFloat ph = Rnd(0, 2 * M_PI);
    return Sample(^CGPoint(CGFloat t, CGFloat s) {
        CGFloat x, y;
        if (k < 0.01) { x = p0.x + v.dx * s; y = p0.y + v.dy * s + 0.5 * g * s * s; }
        else {
            CGFloat e = 1 - exp(-k * s);
            x = p0.x + v.dx / k * e;
            y = p0.y + (v.dy - g / k) / k * e + g / k * s;
        }
        if (swayAmp > 0) x += swayAmp * sin(2 * M_PI * swayHz * s + ph) * MIN(1, s / 0.6);
        return CGPointMake(x, y);
    }, dur, 36);
}

static void RemoveLater(CALayer *l, CFTimeInterval t);
static void After(CFTimeInterval t, dispatch_block_t b);

// 生成一个粒子：content 放在容器原点，容器沿 pts 运动
static CALayer *Spawn(CALayer *host, CALayer *content, CFTimeInterval delay, CFTimeInterval dur,
                      NSArray *pts, NSArray *opVals, NSArray *opTimes, BOOL rotateAuto) {
    CALayer *c = [CALayer layer];
    c.bounds = CGRectZero;
    c.position = [pts.firstObject CGPointValue];
    c.opacity = 0;
    content.position = CGPointZero;
    [c addSublayer:content];
    [host addSublayer:c];
    CAKeyframeAnimation *mv = [CAKeyframeAnimation animationWithKeyPath:@"position"];
    mv.values = pts;
    if (rotateAuto && pts.count >= 2) {
        // 让元素的 +x 轴始终指向运动方向（尾巴在 -x 侧，自然拖在身后）
        NSMutableArray *angs = [NSMutableArray arrayWithCapacity:pts.count];
        CGFloat prev = 0;
        for (NSUInteger i = 0; i < pts.count; i++) {
            CGPoint a = [pts[MIN(i, pts.count - 2)] CGPointValue], b2 = [pts[MIN(i + 1, pts.count - 1)] CGPointValue];
            CGFloat an = atan2(b2.y - a.y, b2.x - a.x);
            if (i > 0) { while (an - prev > M_PI) an -= 2 * M_PI; while (an - prev < -M_PI) an += 2 * M_PI; }
            prev = an;
            [angs addObject:@(an)];
        }
        content.transform = CATransform3DMakeRotation([angs.firstObject doubleValue], 0, 0, 1);
        if (pts.count > 2) {
            CAKeyframeAnimation *ra = [CAKeyframeAnimation animationWithKeyPath:@"transform.rotation.z"];
            ra.values = angs; ra.duration = TS(dur); ra.beginTime = Now() + TS(delay);
            ra.fillMode = kCAFillModeBoth; ra.removedOnCompletion = NO;
            [content addAnimation:ra forKey:@"aa.heading"];
        }
    }
    CAKeyframeAnimation *op = [CAKeyframeAnimation animationWithKeyPath:@"opacity"];
    op.values = opVals ?: @[@0, @1, @1, @0];
    op.keyTimes = opTimes ?: @[@0, @0.12, @0.7, @1];
    CAAnimationGroup *g = [CAAnimationGroup animation];
    g.animations = @[mv, op];
    g.duration = TS(dur);
    g.beginTime = Now() + TS(delay);
    g.fillMode = kCAFillModeBoth;
    g.removedOnCompletion = NO;
    [c addAnimation:g forKey:@"aa.move"];
    RemoveLater(c, delay + dur + 0.15);
    return c;
}

static CALayer *Wrap(CALayer *inner) {
    CALayer *w = [CALayer layer];
    w.bounds = CGRectZero;
    inner.position = CGPointZero;
    [w addSublayer:inner];
    return w;
}

static void KF(CALayer *l, NSString *kp, NSArray *vals, NSArray *times, CFTimeInterval dur, CFTimeInterval delay, BOOL autorev, float repeat) {
    CAKeyframeAnimation *a = [CAKeyframeAnimation animationWithKeyPath:kp];
    a.values = vals;
    if (times) a.keyTimes = times;
    a.duration = TS(dur);
    a.autoreverses = autorev;
    a.repeatCount = repeat;
    a.beginTime = Now() + TS(delay);
    a.fillMode = kCAFillModeBoth;
    a.removedOnCompletion = NO;
    [l addAnimation:a forKey:[NSString stringWithFormat:@"aa.%@.%u", kp, arc4random()]];
}
// 参考写法：autoreverses + 无限重复 + 每颗周期随机抖动
static void Twinkle(CALayer *l, CGFloat lo, CGFloat hi, CFTimeInterval period, CFTimeInterval delay) {
    CABasicAnimation *a = [CABasicAnimation animationWithKeyPath:@"opacity"];
    a.fromValue = @(lo); a.toValue = @(hi);
    a.duration = TS(period + Rnd(0, 0.018 + period * 0.25));
    a.autoreverses = YES; a.repeatCount = HUGE_VALF;
    a.timeOffset = Rnd(0, a.duration);
    a.beginTime = Now() + TS(delay);
    a.fillMode = kCAFillModeBoth; a.removedOnCompletion = NO;
    [l addAnimation:a forKey:[NSString stringWithFormat:@"aa.tw.%u", arc4random()]];
}
static void Spin(CALayer *l, CGFloat turnsPerSec, CFTimeInterval dur) {
    CABasicAnimation *a = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
    CGFloat start = Rnd(0, 2 * M_PI);
    a.fromValue = @(start); a.toValue = @(start + turnsPerSec * 2 * M_PI * dur);
    a.duration = TS(dur); a.fillMode = kCAFillModeBoth; a.removedOnCompletion = NO;
    a.beginTime = Now();
    [l addAnimation:a forKey:[NSString stringWithFormat:@"aa.spin.%u", arc4random()]];
}
// 纸片 / 花瓣 3D 翻飞
static void Flip(CALayer *l, BOOL axisX, CFTimeInterval period) {
    CABasicAnimation *a = [CABasicAnimation animationWithKeyPath:axisX ? @"transform.rotation.x" : @"transform.rotation.y"];
    CGFloat s = Rnd(0, 2 * M_PI);
    a.fromValue = @(s); a.toValue = @(s + (Coin(0.5) ? 2 : -2) * M_PI);
    a.duration = TS(period); a.repeatCount = HUGE_VALF; a.removedOnCompletion = NO;
    [l addAnimation:a forKey:[NSString stringWithFormat:@"aa.flip.%u", arc4random()]];
}
// 彩虹色相轮转（参考：5 个色相 keyframe）
static void HueCycle(CAShapeLayer *l, NSString *kp, CGFloat sat, CGFloat alpha, CFTimeInterval period) {
    NSArray *hues = @[@0.58, @0.72, @0.86, @0.05, @0.42, @0.58];
    CGFloat off = Rnd(0, 1);
    NSMutableArray *cols = [NSMutableArray array];
    for (NSNumber *h in hues) [cols addObject:(id)[UIColor colorWithHue:fmod(h.doubleValue + off, 1) saturation:sat brightness:1 alpha:alpha].CGColor];
    CAKeyframeAnimation *a = [CAKeyframeAnimation animationWithKeyPath:kp];
    a.values = cols; a.duration = TS(period); a.repeatCount = HUGE_VALF; a.removedOnCompletion = NO;
    a.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
    [l addAnimation:a forKey:[NSString stringWithFormat:@"aa.hue.%u", arc4random()]];
    if ([kp isEqualToString:@"fillColor"]) {
        CAKeyframeAnimation *s = [a copy]; s.keyPath = @"shadowColor";
        [l addAnimation:s forKey:[NSString stringWithFormat:@"aa.hues.%u", arc4random()]];
    }
}

#pragma mark 形状路径（全部以 (0,0) 为中心）

static CGPathRef P(UIBezierPath *b) { return b.CGPath; }

static UIBezierPath *HeartP(CGFloat s) {
    CGFloat x = -s/2, y = -s/2 - s*0.04, w = s, h = s;
    UIBezierPath *p = [UIBezierPath bezierPath];
    [p moveToPoint:CGPointMake(x + .5*w, y + .93*h)];
    [p addCurveToPoint:CGPointMake(x + .02*w, y + .34*h) controlPoint1:CGPointMake(x + .22*w, y + .73*h) controlPoint2:CGPointMake(x + .02*w, y + .56*h)];
    [p addCurveToPoint:CGPointMake(x + .5*w, y + .17*h) controlPoint1:CGPointMake(x + .02*w, y + .04*h) controlPoint2:CGPointMake(x + .40*w, y + .0*h)];
    [p addCurveToPoint:CGPointMake(x + .98*w, y + .34*h) controlPoint1:CGPointMake(x + .60*w, y + .0*h) controlPoint2:CGPointMake(x + .98*w, y + .04*h)];
    [p addCurveToPoint:CGPointMake(x + .5*w, y + .93*h) controlPoint1:CGPointMake(x + .98*w, y + .56*h) controlPoint2:CGPointMake(x + .78*w, y + .73*h)];
    [p closePath];
    return p;
}
// 参考：四角星，内径 = 外径 × 0.342
static UIBezierPath *Star4P(CGFloat r) {
    CGFloat ri = r * 0.342 * 0.7071;
    UIBezierPath *p = [UIBezierPath bezierPath];
    [p moveToPoint:CGPointMake(0, -r)];
    [p addLineToPoint:CGPointMake(ri, -ri)]; [p addLineToPoint:CGPointMake(r, 0)];
    [p addLineToPoint:CGPointMake(ri, ri)];  [p addLineToPoint:CGPointMake(0, r)];
    [p addLineToPoint:CGPointMake(-ri, ri)]; [p addLineToPoint:CGPointMake(-r, 0)];
    [p addLineToPoint:CGPointMake(-ri, -ri)];
    [p closePath];
    return p;
}
static UIBezierPath *Star5P(CGFloat R) {
    UIBezierPath *p = [UIBezierPath bezierPath];
    for (int i = 0; i < 10; i++) {
        CGFloat a = -M_PI_2 + i * M_PI / 5, rr = (i % 2) ? R * 0.382 : R;
        CGPoint pt = CGPointMake(cos(a) * rr, sin(a) * rr);
        if (!i) [p moveToPoint:pt]; else [p addLineToPoint:pt];
    }
    [p closePath];
    return p;
}
static UIBezierPath *PetalP(CGFloat h) { // 樱花瓣：带缺口
    CGFloat w = h * 0.72, x = -w/2, y = -h/2;
    UIBezierPath *p = [UIBezierPath bezierPath];
    [p moveToPoint:CGPointMake(x + w/2, y + h)];
    [p addCurveToPoint:CGPointMake(x + w*0.3, y + h*0.03) controlPoint1:CGPointMake(x - w*0.2, y + h*0.62) controlPoint2:CGPointMake(x - w*0.02, y + h*0.1)];
    [p addLineToPoint:CGPointMake(x + w/2, y + h*0.17)];
    [p addLineToPoint:CGPointMake(x + w*0.7, y + h*0.03)];
    [p addCurveToPoint:CGPointMake(x + w/2, y + h) controlPoint1:CGPointMake(x + w*1.02, y + h*0.1) controlPoint2:CGPointMake(x + w*1.2, y + h*0.62)];
    [p closePath];
    return p;
}
static UIBezierPath *RosePetalP(CGFloat s) {
    CGFloat w = s, h = s * 0.92, x = -w/2, y = -h/2;
    UIBezierPath *p = [UIBezierPath bezierPath];
    [p moveToPoint:CGPointMake(x + w*0.5, y + h)];
    [p addCurveToPoint:CGPointMake(x + w*0.42, y + h*0.04) controlPoint1:CGPointMake(x - w*0.28, y + h*0.72) controlPoint2:CGPointMake(x + w*0.0, y + h*0.02)];
    [p addQuadCurveToPoint:CGPointMake(x + w*0.6, y + h*0.06) controlPoint:CGPointMake(x + w*0.52, y + h*0.12)];
    [p addCurveToPoint:CGPointMake(x + w*0.5, y + h) controlPoint1:CGPointMake(x + w*1.05, y + h*0.05) controlPoint2:CGPointMake(x + w*1.25, y + h*0.72)];
    [p closePath];
    return p;
}
static UIBezierPath *BlossomP(CGFloat r) {
    UIBezierPath *all = [UIBezierPath bezierPath];
    for (int i = 0; i < 5; i++) {
        UIBezierPath *p = PetalP(r);
        [p applyTransform:CGAffineTransformMakeTranslation(0, -r * 0.5)];
        [p applyTransform:CGAffineTransformMakeRotation(i * 2 * M_PI / 5 + M_PI)];
        [all appendPath:p];
    }
    return all;
}
static UIBezierPath *CircleP(CGFloat r) { return [UIBezierPath bezierPathWithOvalInRect:CGRectMake(-r, -r, 2*r, 2*r)]; }
static UIBezierPath *SnowP(CGFloat r, BOOL fancy) {
    UIBezierPath *p = [UIBezierPath bezierPath];
    for (int i = 0; i < 6; i++) {
        CGFloat a = i * M_PI / 3;
        [p moveToPoint:CGPointZero]; [p addLineToPoint:CGPointMake(cos(a)*r, sin(a)*r)];
        int nb = fancy ? 2 : 1;
        for (int j = 0; j < nb; j++) {
            CGFloat t = j ? 0.78 : 0.5, bl = r * (j ? 0.2 : 0.3);
            CGPoint m = CGPointMake(cos(a)*r*t, sin(a)*r*t);
            [p moveToPoint:m]; [p addLineToPoint:CGPointMake(m.x + cos(a+0.85)*bl, m.y + sin(a+0.85)*bl)];
            [p moveToPoint:m]; [p addLineToPoint:CGPointMake(m.x + cos(a-0.85)*bl, m.y + sin(a-0.85)*bl)];
        }
    }
    return p;
}


#pragma mark 预渲染精灵（性能关键）
// 所有带发光 / 渐变的元素都在第一次用到时用矢量路径画成位图并缓存，之后只是贴图 + 变换，
// GPU 每帧不再做实时阴影模糊和遮罩离屏渲染，这是 TEST4 掉帧的主要原因。

#define kSprS 32.0      // 精灵基准尺寸（形状最长边）
#define kSprP 0.55      // 四周留给光晕的边距（相对基准尺寸）

static NSMutableDictionary<NSString *, UIImage *> *gSprCache;
static NSString *CK(UIColor *c) {
    CGFloat r = 0, g = 0, b = 0, a = 0; [c getRed:&r green:&g blue:&b alpha:&a];
    return [NSString stringWithFormat:@"%02x%02x%02x%02x", (int)(r*255), (int)(g*255), (int)(b*255), (int)(a*255)];
}
static UIImage *SprImg(NSString *key, void (^draw)(CGContextRef ctx, CGFloat S)) {
    if (!gSprCache) gSprCache = [NSMutableDictionary dictionary];
    UIImage *hit = gSprCache[key];
    if (hit) return hit;
    CGFloat C = kSprS * (1 + 2 * kSprP);
    UIGraphicsImageRendererFormat *f = [UIGraphicsImageRendererFormat preferredFormat]; f.opaque = NO;
    UIImage *img = [[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(C, C) format:f] imageWithActions:^(UIGraphicsImageRendererContext *rc) {
        CGContextTranslateCTM(rc.CGContext, C / 2, C / 2);
        draw(rc.CGContext, kSprS);
    }];
    gSprCache[key] = img;
    return img;
}
// size = 形状本体的最长边（点）
static CALayer *SprL(UIImage *img, CGFloat size) {
    CALayer *l = [CALayer layer];
    l.contents = (__bridge id)img.CGImage;
    l.contentsScale = img.scale;
    CGFloat c = size * (1 + 2 * kSprP);
    l.bounds = CGRectMake(0, 0, c, c);
    return l;
}
typedef UIBezierPath *(*AAShapeFn)(CGFloat);
static UIBezierPath *Fit(AAShapeFn fn, CGFloat S) {
    UIBezierPath *p = fn(100);
    CGRect bb = p.bounds;
    CGFloat s = S / MAX(bb.size.width, bb.size.height);
    CGAffineTransform t = CGAffineTransformMakeScale(s, s);
    t = CGAffineTransformTranslate(t, -CGRectGetMidX(bb), -CGRectGetMidY(bb));
    [p applyTransform:t];
    return p;
}
static void GradFill(CGContextRef ctx, UIBezierPath *p, NSArray<UIColor *> *cols, CGPoint a, CGPoint b) {
    CGContextSaveGState(ctx);
    [p addClip];
    NSMutableArray *cg = [NSMutableArray array];
    for (UIColor *c in cols) [cg addObject:(id)c.CGColor];
    CGColorSpaceRef sp = CGColorSpaceCreateDeviceRGB();
    CGGradientRef g = CGGradientCreateWithColors(sp, (__bridge CFArrayRef)cg, NULL);
    CGContextDrawLinearGradient(ctx, g, a, b, kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
    CGGradientRelease(g); CGColorSpaceRelease(sp);
    CGContextRestoreGState(ctx);
}
static void RadFill(CGContextRef ctx, CGPoint c, CGFloat r, NSArray<UIColor *> *cols, NSArray<NSNumber *> *locs) {
    NSMutableArray *cg = [NSMutableArray array];
    for (UIColor *x in cols) [cg addObject:(id)x.CGColor];
    CGFloat L[8]; for (NSUInteger i = 0; i < locs.count && i < 8; i++) L[i] = locs[i].doubleValue;
    CGColorSpaceRef sp = CGColorSpaceCreateDeviceRGB();
    CGGradientRef g = CGGradientCreateWithColors(sp, (__bridge CFArrayRef)cg, locs ? L : NULL);
    CGContextDrawRadialGradient(ctx, g, c, 0, c, r, 0);
    CGGradientRelease(g); CGColorSpaceRelease(sp);
}
static void GlowFill(CGContextRef ctx, UIBezierPath *p, UIColor *fill, UIColor *glow, CGFloat blur) {
    if (glow && blur > 0) {
        CGContextSaveGState(ctx);
        CGContextSetShadowWithColor(ctx, CGSizeZero, blur, glow.CGColor);
        [fill setFill]; [p fill]; [p fill];
        CGContextRestoreGState(ctx);
    }
    [fill setFill]; [p fill];
}

// 通用：矢量形状 + 烘焙光晕
static CALayer *GS(NSString *kind, AAShapeFn fn, CGFloat size, UIColor *fill, UIColor *glow, CGFloat glowRatio) {
    NSString *k = [NSString stringWithFormat:@"gs-%@-%@-%@-%.2f", kind, CK(fill), glow ? CK(glow) : @"-", glowRatio];
    UIImage *img = SprImg(k, ^(CGContextRef ctx, CGFloat S) { GlowFill(ctx, Fit(fn, S), fill, glow, S * glowRatio); });
    return SprL(img, size);
}
// 彩虹变色的形状：预渲染 6 个色相，用 contents 关键帧交叉淡变（零实时渲染开销）
static CALayer *HueGS(NSString *kind, AAShapeFn fn, CGFloat size, CGFloat glowRatio, CFTimeInterval period) {
    NSArray *hues = @[@0.58, @0.72, @0.86, @0.02, @0.12, @0.45];
    int off = arc4random_uniform(6);
    NSMutableArray *imgs = [NSMutableArray array];
    for (int i = 0; i <= 6; i++) {
        CGFloat hh = [hues[(i + off) % 6] doubleValue];
        UIColor *c = [UIColor colorWithHue:hh saturation:0.55 brightness:1 alpha:1];
        NSString *k = [NSString stringWithFormat:@"hue-%@-%.2f-%.2f", kind, hh, glowRatio];
        UIImage *img = SprImg(k, ^(CGContextRef ctx, CGFloat S) { GlowFill(ctx, Fit(fn, S), c, c, S * glowRatio); });
        [imgs addObject:(id)img.CGImage];
    }
    CALayer *l = SprL(gSprCache[[NSString stringWithFormat:@"hue-%@-%.2f-%.2f", kind, [hues[off] doubleValue], glowRatio]], size);
    l.contents = imgs.firstObject;
    CAKeyframeAnimation *a = [CAKeyframeAnimation animationWithKeyPath:@"contents"];
    a.values = imgs; a.duration = TS(period); a.repeatCount = HUGE_VALF; a.removedOnCompletion = NO;
    [l addAnimation:a forKey:@"aa.huec"];
    return l;
}
// 发光点：白色内核 → 颜色 → 柔和光晕；r = 内核半径
static CALayer *DotL(CGFloat r, UIColor *c) {
    UIImage *img = SprImg([NSString stringWithFormat:@"dot-%@", CK(c)], ^(CGContextRef ctx, CGFloat S) {
        CGFloat C = S * (1 + 2 * kSprP) / 2;
        RadFill(ctx, CGPointZero, C, @[UIColor.whiteColor, c, [c colorWithAlphaComponent:0.35], [c colorWithAlphaComponent:0.08], [c colorWithAlphaComponent:0]], @[@0, @0.1, @0.24, @0.55, @1]);
    });
    // 内核约占画布半径的 10%
    CGFloat canvas = r / 0.1 * 2;
    return SprL(img, canvas / (1 + 2 * kSprP));
}
// 立体糖果爱心
static CALayer *HeartL(CGFloat size, int top, int bottom, BOOL glow) {
    UIImage *img = SprImg([NSString stringWithFormat:@"heart-%x-%x-%d", top, bottom, glow], ^(CGContextRef ctx, CGFloat S) {
        UIBezierPath *hp = Fit(HeartP, S);
        CGRect r = hp.bounds;
        if (glow) {
            CGContextSaveGState(ctx);
            CGContextSetShadowWithColor(ctx, CGSizeZero, S * 0.32, RGBA(bottom, 0.85).CGColor);
            [RGB(bottom) setFill]; [hp fill];
            CGContextRestoreGState(ctx);
        }
        GradFill(ctx, hp, @[RGB(top), RGB(bottom)], CGPointMake(0, r.origin.y), CGPointMake(0, CGRectGetMaxY(r)));
        CGContextSaveGState(ctx); [hp addClip];
        RadFill(ctx, CGPointMake(0, CGRectGetMaxY(r) + S * 0.1), S * 0.55, @[[UIColor colorWithWhite:0 alpha:0.18], [UIColor colorWithWhite:0 alpha:0]], nil);
        CGContextSaveGState(ctx);
        CGContextTranslateCTM(ctx, -S * 0.2, -S * 0.2);
        CGContextRotateCTM(ctx, -0.65);
        UIBezierPath *hl = [UIBezierPath bezierPathWithOvalInRect:CGRectMake(-S * 0.14, -S * 0.08, S * 0.28, S * 0.16)];
        GradFill(ctx, hl, @[[UIColor colorWithWhite:1 alpha:0.85], [UIColor colorWithWhite:1 alpha:0]], CGPointMake(0, -S * 0.08), CGPointMake(0, S * 0.08));
        CGContextRestoreGState(ctx);
        [[UIColor colorWithWhite:1 alpha:0.9] setFill];
        [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(S * 0.16, -S * 0.3, S * 0.07, S * 0.07)] fill];
        CGContextRestoreGState(ctx);
    });
    return SprL(img, size);
}
static CALayer *OutlineHeartL(CGFloat size, int col) {
    UIImage *img = SprImg([NSString stringWithFormat:@"oheart-%x", col], ^(CGContextRef ctx, CGFloat S) {
        UIBezierPath *hp = Fit(HeartP, S * 0.9);
        hp.lineWidth = S * 0.1;
        CGContextSetShadowWithColor(ctx, CGSizeZero, S * 0.2, RGB(col).CGColor);
        [RGB(col) setStroke]; [hp stroke];
    });
    return SprL(img, size);
}
static CALayer *PetalL(CGFloat size, int a, int b) {
    UIImage *img = SprImg([NSString stringWithFormat:@"petal-%x-%x", a, b], ^(CGContextRef ctx, CGFloat S) {
        UIBezierPath *p = Fit(PetalP, S);
        CGRect r = p.bounds;
        GradFill(ctx, p, @[RGB(a), RGB(b)], CGPointMake(0, r.origin.y), CGPointMake(0, CGRectGetMaxY(r)));
        CGContextSaveGState(ctx); [p addClip];
        UIBezierPath *v = [UIBezierPath bezierPath];
        [v moveToPoint:CGPointMake(0, CGRectGetMaxY(r) - S * 0.05)]; [v addLineToPoint:CGPointMake(0, r.origin.y + S * 0.35)];
        v.lineWidth = S * 0.035; [[UIColor colorWithWhite:1 alpha:0.35] setStroke]; [v stroke];
        CGContextRestoreGState(ctx);
    });
    return SprL(img, size);
}
static CALayer *RosePetalL(CGFloat size, int a, int b) {
    UIImage *img = SprImg([NSString stringWithFormat:@"rose-%x-%x", a, b], ^(CGContextRef ctx, CGFloat S) {
        UIBezierPath *p = Fit(RosePetalP, S);
        CGRect r = p.bounds;
        GradFill(ctx, p, @[RGB(a), RGB(b)], CGPointMake(r.origin.x + r.size.width * 0.2, r.origin.y), CGPointMake(CGRectGetMaxX(r) - r.size.width * 0.2, CGRectGetMaxY(r)));
        CGContextSaveGState(ctx); [p addClip];
        RadFill(ctx, CGPointMake(-S * 0.12, -S * 0.18), S * 0.38, @[[UIColor colorWithWhite:1 alpha:0.32], [UIColor colorWithWhite:1 alpha:0]], nil);
        CGContextRestoreGState(ctx);
    });
    return SprL(img, size);
}
static CALayer *BlossomL(CGFloat size, int a, int b) {
    UIImage *img = SprImg([NSString stringWithFormat:@"blossom-%x-%x", a, b], ^(CGContextRef ctx, CGFloat S) {
        UIBezierPath *p = Fit(BlossomP, S);
        CGContextSaveGState(ctx); [p addClip];
        RadFill(ctx, CGPointZero, S * 0.5, @[RGB(b), RGB(a), RGB(a)], @[@0, @0.45, @1]);
        CGContextRestoreGState(ctx);
        RadFill(ctx, CGPointZero, S * 0.13, @[RGB(0xE04F7C), RGBA(0xE04F7C, 0)], nil);
        for (int i = 0; i < 7; i++) {
            CGFloat an = i * 2 * M_PI / 7, d = S * 0.12;
            [RGB(0xFFE082) setFill];
            [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(cos(an) * d - S * 0.025, sin(an) * d - S * 0.025, S * 0.05, S * 0.05)] fill];
        }
    });
    return SprL(img, size);
}
static UIBezierPath *SnowFancy(CGFloat r) { return SnowP(r, YES); }
static UIBezierPath *SnowPlain(CGFloat r) { return SnowP(r, NO); }
static CALayer *SnowL(CGFloat size, BOOL fancy) {
    UIImage *img = SprImg([NSString stringWithFormat:@"snow-%d", fancy], ^(CGContextRef ctx, CGFloat S) {
        UIBezierPath *p = Fit(fancy ? SnowFancy : SnowPlain, S);
        p.lineWidth = S * 0.07; p.lineCapStyle = kCGLineCapRound;
        CGContextSetShadowWithColor(ctx, CGSizeZero, S * 0.12, RGB(0x9FD8FF).CGColor);
        [UIColor.whiteColor setStroke]; [p stroke];
        RadFill(ctx, CGPointZero, S * 0.1, @[UIColor.whiteColor, RGBA(0xFFFFFF, 0)], nil);
    });
    return SprL(img, size);
}
static CALayer *BubbleL(CGFloat size) {
    int v = arc4random_uniform(3);
    UIImage *img = SprImg([NSString stringWithFormat:@"bubble-%d", v], ^(CGContextRef ctx, CGFloat S) {
        CGFloat R = S / 2;
        RadFill(ctx, CGPointZero, R, @[RGBA(0xFFFFFF, 0.02), RGBA(0xBFE9FF, 0.06), RGBA(0xFFFFFF, 0.3)], @[@0, @0.7, @1]);
        int hues[] = {0xFF8FD8, 0xFFD27A, 0x8CFFB5, 0x7AD7FF, 0xB59CFF, 0xFF8FD8};
        for (int i = 0; i < 5; i++) {
            UIBezierPath *arc = [UIBezierPath bezierPathWithArcCenter:CGPointZero radius:R - S * 0.035 startAngle:(i + v) * 2 * M_PI / 5 endAngle:(i + v + 1) * 2 * M_PI / 5 clockwise:YES];
            arc.lineWidth = S * 0.06;
            [RGBA(hues[i], 0.55) setStroke]; [arc stroke];
        }
        UIBezierPath *rim = [UIBezierPath bezierPathWithArcCenter:CGPointZero radius:R - S * 0.01 startAngle:0 endAngle:2 * M_PI clockwise:YES];
        rim.lineWidth = S * 0.025; [RGBA(0xFFFFFF, 0.75) setStroke]; [rim stroke];
        UIBezierPath *hl = [UIBezierPath bezierPathWithArcCenter:CGPointZero radius:R * 0.7 startAngle:M_PI * 1.08 endAngle:M_PI * 1.42 clockwise:YES];
        hl.lineWidth = S * 0.08; hl.lineCapStyle = kCGLineCapRound;
        [RGBA(0xFFFFFF, 0.9) setStroke]; [hl stroke];
        [RGBA(0xFFFFFF, 0.85) setFill];
        [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(R * 0.38, R * 0.36, S * 0.08, S * 0.08)] fill];
    });
    return SprL(img, size);
}
// 无发光的普通矢量形状（礼花纸片用，CPU 只画一次，开销极低）
static CAShapeLayer *PlainL(UIBezierPath *path, UIColor *fill) {
    CAShapeLayer *s = [CAShapeLayer layer];
    s.bounds = CGRectZero; s.path = path.CGPath; s.fillColor = fill.CGColor;
    return s;
}
static CAShapeLayer *PlainStroke(UIBezierPath *path, UIColor *stroke, CGFloat lw) {
    CAShapeLayer *s = [CAShapeLayer layer];
    s.bounds = CGRectZero; s.path = path.CGPath;
    s.fillColor = UIColor.clearColor.CGColor; s.strokeColor = stroke.CGColor; s.lineWidth = lw; s.lineCap = kCALineCapRound;
    return s;
}
// 流星 / 带尾迹的火花（容器开 rotateAuto，+x 为飞行方向）
static CALayer *MeteorBody(CGFloat head, CGFloat tailLen, UIColor *col, int dustN) {
    CALayer *root = [CALayer layer]; root.bounds = CGRectZero;
    CGFloat tw = MAX(1.0, head * 0.55);
    CAGradientLayer *tail = [CAGradientLayer layer];
    tail.frame = CGRectMake(-tailLen, -tw / 2, tailLen, tw);
    tail.startPoint = CGPointMake(0, 0.5); tail.endPoint = CGPointMake(1, 0.5);
    tail.colors = @[(id)[col colorWithAlphaComponent:0].CGColor, (id)[col colorWithAlphaComponent:0.35].CGColor, (id)[col colorWithAlphaComponent:0.9].CGColor];
    tail.locations = @[@0, @0.7, @1];
    tail.cornerRadius = tw / 2;
    [root addSublayer:tail];
    Twinkle(tail, 0.2, 0.95, 0.16, 0);
    CALayer *h = DotL(head / 2, col);
    [root addSublayer:h];
    Twinkle(h, 0.45, 1.0, 0.22, 0);
    for (int i = 0; i < dustN; i++) {
        CAShapeLayer *d = PlainL(CircleP(Rnd(0.4, 0.9)), [col colorWithAlphaComponent:0.8]);
        d.position = CGPointMake(-Rnd(14, tailLen * 1.1), Rnd(-6, 6));
        [root addSublayer:d];
        Twinkle(d, 0.15, 0.85, 0.32, i * 0.017);
    }
    return root;
}
static UIImage *EmojiImg(NSString *e) {
    NSString *k = [@"emoji-" stringByAppendingString:e];
    if (gSprCache[k]) return gSprCache[k];
    UIGraphicsImageRendererFormat *f = [UIGraphicsImageRendererFormat preferredFormat]; f.opaque = NO;
    UIImage *img = [[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(48, 48) format:f] imageWithActions:^(UIGraphicsImageRendererContext *c) {
        NSDictionary *at = @{NSFontAttributeName: [UIFont systemFontOfSize:38]};
        CGSize s = [e sizeWithAttributes:at];
        [e drawAtPoint:CGPointMake((48 - s.width) / 2, (48 - s.height) / 2) withAttributes:at];
    }];
    if (!gSprCache) gSprCache = [NSMutableDictionary dictionary];
    gSprCache[k] = img;
    return img;
}
static CALayer *EmojiL(NSString *e, CGFloat size) {
    UIImage *img = EmojiImg(e);
    CALayer *l = [CALayer layer];
    l.contents = (__bridge id)img.CGImage;
    l.contentsScale = img.scale;
    l.bounds = CGRectMake(0, 0, size * 1.25, size * 1.25);
    return l;
}
static CATransform3D Persp(CGFloat d) { CATransform3D t = CATransform3DIdentity; t.m34 = -1.0 / d; return t; }

#pragma mark 喷射源（随机多点 / 两侧对称 / 点赞处）

static CGPoint gSrcP[6];
static CGFloat gSrcA[6];
static int gSrcN = 1;
static BOOL gSrcEdge;

static CGFloat SprayAngle(CALayer *h, CGPoint o, CGFloat spread) {
    CGRect b = h.bounds;
    if (o.x > b.size.width * 0.72) return M_PI + Rnd(-0.15, 0.85) * spread;
    if (o.x < b.size.width * 0.28) return -Rnd(-0.15, 0.85) * spread;
    return -M_PI_2 + Rnd(-1, 1) * M_PI * spread;
}
static CGVector Vel(CGFloat ang, CGFloat sp) { return CGVectorMake(cos(ang) * sp, sin(ang) * sp); }

static void SetupSources(CALayer *h, CGPoint o) {
    CGFloat W = h.bounds.size.width, H = h.bounds.size.height;
    NSInteger mode = SprayMode();
    if (mode == 2) { gSrcEdge = NO; gSrcN = 1; gSrcP[0] = o; return; }
    gSrcEdge = YES;
    if (mode == 1) { // 左右两侧下半部分、屏幕外，对称向内上方喷
        CGFloat y = Rnd(0.56, 0.82) * H, off = Rnd(30, 55), a = Rnd(0.5, 0.85);
        gSrcN = 2;
        gSrcP[0] = CGPointMake(-off, y);     gSrcA[0] = -a;
        gSrcP[1] = CGPointMake(W + off, y);  gSrcA[1] = M_PI + a;
        return;
    }
    // 随机多点：2~4 个屏幕外的点，各自瞄准屏幕内一个随机位置
    gSrcN = 2 + arc4random_uniform(3);
    for (int i = 0; i < gSrcN; i++) {
        CGFloat off = Rnd(30, 60);
        int edge = arc4random_uniform(5); // 0/1 左, 2/3 右, 4 底
        CGPoint p;
        if (edge < 2) p = CGPointMake(-off, Rnd(0.3, 0.95) * H);
        else if (edge < 4) p = CGPointMake(W + off, Rnd(0.3, 0.95) * H);
        else p = CGPointMake(Rnd(0.1, 0.9) * W, H + off);
        CGPoint t = CGPointMake(Rnd(0.2, 0.8) * W, Rnd(0.2, 0.65) * H);
        gSrcP[i] = p; gSrcA[i] = atan2(t.y - p.y, t.x - p.x);
    }
}
static CGPoint SrcP(int i) { return gSrcP[i % gSrcN]; }
static CGFloat SrcA(CALayer *h, int i, CGFloat spread) {
    if (!gSrcEdge) return SprayAngle(h, gSrcP[0], spread);
    return gSrcA[i % gSrcN] + Rnd(-0.5, 0.5) * spread * 0.9;
}
// 屏幕外喷入需要更大的初速度
static CGFloat SrcV(CGFloat v) { return gSrcEdge ? v * 1.5 : v; }
static int SprayCount(CGFloat base) { return Count(base * (gSrcEdge ? 1.0 + 0.25 * (gSrcN - 1) : 1.0)); }

#pragma mark 屏幕边缘无缝氛围光（智能光 / 各风格的边缘光共用）

static CGFloat ScreenCorner(void) {
    CGFloat r = 0;
    @try { r = [[UIScreen.mainScreen valueForKey:@"_displayCornerRadius"] doubleValue]; } @catch (__unused id e) {}
    return r > 0 ? r : 44;
}
// 遮罩：边缘不透明、向内平滑衰减到全透明（几十圈细环叠出来，没有任何“条带”边界）
static UIImage *EdgeMask(CGSize size, CGFloat thick) {
    static NSMutableDictionary *cache; if (!cache) cache = [NSMutableDictionary dictionary];
    NSString *k = [NSString stringWithFormat:@"%.0fx%.0f-%.0f", size.width, size.height, thick];
    if (cache[k]) return cache[k];
    CGFloat corner = ScreenCorner();
    UIGraphicsImageRendererFormat *f = [UIGraphicsImageRendererFormat preferredFormat]; f.opaque = NO; f.scale = 1;
    UIImage *img = [[[UIGraphicsImageRenderer alloc] initWithSize:size format:f] imageWithActions:^(UIGraphicsImageRendererContext *c) {
        int N = 64;
        CGFloat step = thick / N;
        for (int i = 0; i < N; i++) {
            CGFloat in0 = i * step, in1 = (i + 1) * step;
            CGFloat t = (CGFloat)i / N;
            CGFloat a = pow(1 - t, 2.4);
            UIBezierPath *ring = [UIBezierPath bezierPathWithRoundedRect:CGRectInset(CGRectMake(0, 0, size.width, size.height), in0, in0) cornerRadius:MAX(0, corner - in0)];
            [ring appendPath:[UIBezierPath bezierPathWithRoundedRect:CGRectInset(CGRectMake(0, 0, size.width, size.height), in1, in1) cornerRadius:MAX(0, corner - in1)]];
            ring.usesEvenOddFillRule = YES;
            [[UIColor colorWithWhite:1 alpha:a] setFill];
            [ring fill];
        }
    }];
    cache[k] = img;
    return img;
}
static CGPoint Perimeter(CGSize s, CGFloat u) {
    u = u - floor(u);
    CGFloat L = 2 * (s.width + s.height), d = u * L;
    if (d < s.width) return CGPointMake(d, 0);
    d -= s.width; if (d < s.height) return CGPointMake(s.width, d);
    d -= s.height; if (d < s.width) return CGPointMake(s.width - d, s.height);
    d -= s.width; return CGPointMake(0, s.height - d);
}
static void EdgeAura(CALayer *host, NSArray<NSNumber *> *pal, CFTimeInterval dur, CGFloat strength, CGFloat thick, BOOL force) {
    if (!EdgeGlowOn() && !force) return;
    CGRect b = host.bounds;
    CALayer *root = [CALayer layer];
    root.frame = b;
    // 遮罩：四条边各一层系统连续渐变（GPU 插值，无任何分层/接缝），角落自然叠加
    CALayer *mask = [CALayer layer];
    mask.frame = b;
    NSArray *fall = @[(id)[UIColor colorWithWhite:1 alpha:1].CGColor, (id)[UIColor colorWithWhite:1 alpha:0.62].CGColor,
                      (id)[UIColor colorWithWhite:1 alpha:0.28].CGColor, (id)[UIColor colorWithWhite:1 alpha:0.08].CGColor,
                      (id)[UIColor colorWithWhite:1 alpha:0].CGColor];
    NSArray *locs = @[@0, @0.22, @0.48, @0.75, @1];
    CGFloat W = b.size.width, H = b.size.height, tb = thick * 1.15;
    CGRect frs[4] = { CGRectMake(0, 0, thick, H), CGRectMake(W - thick, 0, thick, H), CGRectMake(0, 0, W, thick * 0.8), CGRectMake(0, H - tb, W, tb) };
    CGPoint sp[4] = { {0, 0.5}, {1, 0.5}, {0.5, 0}, {0.5, 1} }, ep[4] = { {1, 0.5}, {0, 0.5}, {0.5, 1}, {0.5, 0} };
    for (int i = 0; i < 4; i++) {
        CAGradientLayer *e = [CAGradientLayer layer];
        e.frame = frs[i]; e.colors = fall; e.locations = locs;
        e.startPoint = sp[i]; e.endPoint = ep[i];
        [mask addSublayer:e];
        // 厚度呼吸：光向内涌进来再退回去（连续缩放，不会出纹）
        CGFloat ax = (i < 2) ? 1 : 0;
        CGPoint anchor = (i == 0) ? CGPointMake(0, 0.5) : (i == 1) ? CGPointMake(1, 0.5) : (i == 2) ? CGPointMake(0.5, 0) : CGPointMake(0.5, 1);
        CGRect fr = e.frame; e.anchorPoint = anchor; e.frame = fr;
        NSString *kp = ax ? @"transform.scale.x" : @"transform.scale.y";
        KF(e, kp, @[@0.55, @1.08, @0.95, @0.85], @[@0, @0.35, @0.7, @1], dur, 0, NO, 1);
    }
    root.mask = mask;
    // 底色：锥形渐变铺满，慢慢旋转（经过遮罩后是一圈无缝的柔光）
    CGFloat d = hypot(b.size.width, b.size.height) * 1.2;
    CAGradientLayer *con = [CAGradientLayer layer];
    con.type = kCAGradientLayerConic;
    con.bounds = CGRectMake(0, 0, d, d);
    con.position = CGPointMake(b.size.width / 2, b.size.height / 2);
    con.startPoint = CGPointMake(0.5, 0.5); con.endPoint = CGPointMake(0.5, 0);
    NSMutableArray *cc = [NSMutableArray array];
    for (NSNumber *n in pal) [cc addObject:(id)RGB(n.intValue).CGColor];
    [cc addObject:cc.firstObject];
    con.colors = cc;
    con.opacity = 0.85;
    [root addSublayer:con];
    CABasicAnimation *rot = [CABasicAnimation animationWithKeyPath:@"transform.rotation.z"];
    rot.fromValue = @(Rnd(0, M_PI)); rot.toValue = @([rot.fromValue doubleValue] + M_PI * 0.9);
    rot.duration = TS(dur); rot.removedOnCompletion = NO; rot.fillMode = kCAFillModeForwards;
    [con addAnimation:rot forKey:@"r"];
    // 亮斑：沿着屏幕四周游走的径向渐变光团，让亮度有起伏、颜色在流动
    int blobs = 5;
    for (int i = 0; i < blobs; i++) {
        CGFloat R = Rnd(150, 240);
        CAGradientLayer *bl = [CAGradientLayer layer];
        bl.type = kCAGradientLayerRadial;
        bl.bounds = CGRectMake(0, 0, R * 2, R * 2);
        bl.startPoint = CGPointMake(0.5, 0.5); bl.endPoint = CGPointMake(1, 1);
        int c0 = pal[i % pal.count].intValue, c1 = pal[(i + 2) % pal.count].intValue;
        bl.colors = @[(id)RGB(c0).CGColor, (id)RGBA(c0, 0).CGColor];
        CGFloat u0 = (CGFloat)i / blobs + Rnd(-0.03, 0.03), dir = (i % 2 ? 1 : -1) * Rnd(0.08, 0.18);
        bl.position = Perimeter(b.size, u0);
        [root addSublayer:bl];
        KF(bl, @"position", Sample(^CGPoint(CGFloat t, CGFloat s) { return Perimeter(b.size, u0 + dir * t); }, dur, 24), nil, dur, 0, NO, 1);
        KF(bl, @"colors", @[@[(id)RGB(c0).CGColor, (id)RGBA(c0, 0).CGColor], @[(id)RGB(c1).CGColor, (id)RGBA(c1, 0).CGColor], @[(id)RGB(c0).CGColor, (id)RGBA(c0, 0).CGColor]], nil, dur, 0, NO, 1);
    }
    root.opacity = 0;
    [host addSublayer:root];
    KF(root, @"opacity", @[@0, @(strength), @(strength * 0.85), @0], @[@0, @0.16, @0.6, @1], dur, 0, NO, 1);
    RemoveLater(root, dur + 0.2);
}


#pragma mark 通用散布元素

static void StarSprinkle(CALayer *h, int n, NSArray<NSNumber *> *cols, BOOL rainbow, CGFloat rMin, CGFloat rMax, CFTimeInterval window) {
    CGRect b = h.bounds;
    for (int i = 0; i < n; i++) {
        CGFloat r = Rnd(rMin, rMax);
        CALayer *s;
        if (rainbow) s = HueGS(@"s4", Star4P, r * 2, 0.35, Rnd(1.2, 2.4));
        else {
            UIColor *c = cols ? RGB(PickI(cols)) : [UIColor colorWithRed:0.86 green:0.92 blue:1 alpha:1];
            s = GS(@"s4", Star4P, r * 2, c, c, 0.35);
        }
        Twinkle(s, 0.35, 1.0, Rnd(0.18, 0.5), 0);
        CALayer *w = Wrap(s);
        Spin(w, Rnd(0.08, 0.3) * (Coin(0.5) ? 1 : -1), 3);
        CGPoint p0 = CGPointMake(Rnd(16, b.size.width - 16), Rnd(b.size.height * 0.08, b.size.height * 0.9));
        CGFloat ax = Rnd(6, 14), ay = Rnd(6, 14), fx = Rnd(0.3, 0.6), fy = Rnd(0.25, 0.55), rise = Rnd(10, 40);
        CFTimeInterval dur = Rnd(1.0, 1.8);
        NSArray *pts = Sample(^CGPoint(CGFloat t, CGFloat sec) {
            return CGPointMake(p0.x + ax * sin(2 * M_PI * fx * sec), p0.y + ay * sin(2 * M_PI * fy * sec + 1) - rise * t);
        }, dur, 16);
        KF(s, @"transform.scale", @[@0.1, @1.25, @1.0, @0.2], @[@0, @0.2, @0.65, @1], dur, 0, NO, 1);
        Spawn(h, w, Rnd(0, window), dur, pts, @[@0, @1, @1, @0], @[@0, @0.15, @0.7, @1], NO);
    }
}
static void Dust(CALayer *h, int n, NSArray<NSNumber *> *cols, CFTimeInterval window) {
    CGRect b = h.bounds;
    for (int i = 0; i < n; i++) {
        CALayer *d = DotL(Rnd(0.5, 1.1), RGB(PickI(cols)));
        Twinkle(d, 0.1, 1, Rnd(0.12, 0.3), 0);
        CGPoint p0 = CGPointMake(Rnd(0, b.size.width), Rnd(0, b.size.height));
        CFTimeInterval dur = Rnd(0.9, 1.6);
        Spawn(h, d, Rnd(0, window), dur, @[[NSValue valueWithCGPoint:p0], [NSValue valueWithCGPoint:CGPointMake(p0.x + Rnd(-20, 20), p0.y - Rnd(15, 60))]], nil, nil, NO);
    }
}

#pragma mark - ===== 各风格 =====

static NSArray *HeartPal(void) { return @[@[@0xFF9AB4, @0xF0144A], @[@0xFFB8CA, @0xFF4D79], @[@0xFF7A9C, @0xD6003F], @[@0xFFD3DE, @0xFF7FA0], @[@0xFFB199, @0xFF4D6D]]; }

static void FX_Heart(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0xFF4D79, @0xFF8FB0, @0xFF6A88, @0xFFB3C6], 1.7, 0.95, 140, NO);
    int n = SprayCount(24);
    for (int i = 0; i < n; i++) {
        NSArray *c = HeartPal()[arc4random_uniform(5)];
        CGFloat s = Coin(0.25) ? Rnd(13, 19) : Rnd(5, 11);
        CALayer *hrt = HeartL(s, [c[0] intValue], [c[1] intValue], YES);
        CALayer *w = Wrap(hrt);
        w.transform = CATransform3DMakeRotation(Rnd(-0.5, 0.5), 0, 0, 1);
        KF(hrt, @"transform.scale", @[@0.3, @1.15, @1, @0.7], @[@0, @0.12, @0.5, @1], 1.6, 0, NO, 1);
        CFTimeInterval dur = Rnd(1.2, 2.0);
        NSArray *pts = Ballistic(SrcP(i), Vel(SrcA(h, i, 0.75), SrcV(Rnd(260, 720))), -Rnd(40, 160), 2.2, dur, Rnd(4, 14), Rnd(0.6, 1.4), 0);
        Spawn(h, w, Rnd(0, 0.15), dur, pts, @[@0, @1, @1, @0], @[@0, @0.06, @0.6, @1], NO);
    }
    int m = Count(30);
    for (int i = 0; i < m; i++) {
        BOOL far = Coin(0.55);
        CGFloat s = far ? Rnd(3.5, 6.5) : Rnd(7, 13);
        CALayer *hrt;
        if (Coin(0.18)) hrt = OutlineHeartL(s, 0xFFC2D1);
        else { NSArray *c = HeartPal()[arc4random_uniform(5)]; hrt = HeartL(s, [c[0] intValue], [c[1] intValue], !far); }
        if (far) hrt.opacity = 0.65;
        Twinkle(hrt, far ? 0.4 : 0.6, 1, Rnd(0.25, 0.6), 0);
        CALayer *w = Wrap(hrt);
        w.transform = CATransform3DMakeRotation(Rnd(-0.4, 0.4), 0, 0, 1);
        KF(hrt, @"transform.scale", @[@0, @1.2, @1, @0.6], @[@0, @0.18, @0.4, @1], 1.6, 0, NO, 1);
        CGPoint p0 = CGPointMake(Rnd(10, b.size.width - 10), Rnd(b.size.height * 0.12, b.size.height * 0.95));
        CFTimeInterval dur = Rnd(1.1, 1.8);
        NSArray *pts = Ballistic(p0, CGVectorMake(Rnd(-10, 10), -Rnd(far ? 20 : 40, far ? 50 : 110)), -Rnd(20, 60), 0.6, dur, Rnd(3, 10), Rnd(0.5, 1.1), 0);
        Spawn(h, w, Rnd(0, 0.55), dur, pts, @[@0, @1, @1, @0], @[@0, @0.12, @0.65, @1], NO);
    }
    StarSprinkle(h, Count(12), @[@0xFFF1C9, @0xFFFFFF, @0xFFD1DC], NO, 3, 8, 0.7);
    Dust(h, Count(26), @[@0xFFFFFF, @0xFFD1DC, @0xFFE7A8], 0.8);
}

static void FX_Sakura(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0xFFB7C9, @0xFFD6E2, @0xFF9DB8, @0xFFE8EF], 2.6, 0.6, 120, NO);
    NSArray *pal = @[@[@0xFFF1F5, @0xFFB3C7], @[@0xFFE0E9, @0xFF9AB5], @[@0xFFFFFF, @0xFFCAD8], @[@0xFFD3E0, @0xF77FA3]];
    int n = Count(40);
    for (int i = 0; i < n; i++) {
        BOOL far = Coin(0.45);
        CGFloat s = far ? Rnd(6, 9) : Rnd(10, 16);
        NSArray *c = pal[arc4random_uniform(4)];
        CALayer *petal = Coin(0.15) ? BlossomL(s * 1.2, [c[0] intValue], [c[1] intValue]) : PetalL(s, [c[0] intValue], [c[1] intValue]);
        if (far) petal.opacity = 0.7;
        CALayer *flip = Wrap(petal);
        Flip(flip, Coin(0.5), Rnd(0.9, 2.0));
        CALayer *w = Wrap(flip);
        w.sublayerTransform = Persp(260);
        Spin(w, Rnd(0.1, 0.45) * (Coin(0.5) ? 1 : -1), 4.5);
        CGPoint p0 = CGPointMake(Rnd(-0.05, 1.3) * b.size.width, -Rnd(15, 140));
        CFTimeInterval dur = Rnd(2.6, 4.2) * (far ? 1.2 : 1);
        NSArray *pts = Ballistic(p0, CGVectorMake(-Rnd(40, 110), Rnd(60, 140)), Rnd(50, 110), 0.9, dur, Rnd(18, 46), Rnd(0.25, 0.6), 0);
        Spawn(h, w, Rnd(0, 0.9), dur, pts, @[@0, @1, @1, @0], @[@0, @0.1, @0.8, @1], NO);
    }
    int k = SprayCount(16);
    for (int i = 0; i < k; i++) {
        NSArray *c = pal[arc4random_uniform(4)];
        CALayer *pt = PetalL(Rnd(8, 13), [c[0] intValue], [c[1] intValue]);
        CALayer *flip = Wrap(pt); Flip(flip, YES, Rnd(0.5, 1.0));
        CALayer *w = Wrap(flip);
        w.sublayerTransform = Persp(260);
        Spin(w, Rnd(0.3, 0.9), 2.4);
        CFTimeInterval dur = Rnd(1.8, 2.6);
        Spawn(h, w, Rnd(0, 0.12), dur, Ballistic(SrcP(i), Vel(SrcA(h, i, 0.7), SrcV(Rnd(200, 480))), 140, 1.8, dur, 12, 0.6, 0), nil, nil, NO);
    }
    Dust(h, Count(16), @[@0xFFFFFF, @0xFFE3EC], 1.0);
}

static void FX_Star(CALayer *h, CGPoint o) {
    EdgeAura(h, @[@0xFFD27A, @0xFFB3E6, @0x9FC8FF, @0xC9A8FF], 1.8, 0.7, 130, NO);
    int n = SprayCount(32);
    for (int i = 0; i < n; i++) {
        CGFloat r = Coin(0.2) ? Rnd(8, 12) : Rnd(3, 7);
        CALayer *s = HueGS(@"s4", Star4P, r * 2, 0.35, Rnd(0.9, 1.8));
        Twinkle(s, 0.4, 1, Rnd(0.15, 0.35), 0);
        CALayer *w = Wrap(s);
        Spin(w, Rnd(0.2, 0.8) * (Coin(0.5) ? 1 : -1), 2.5);
        KF(s, @"transform.scale", @[@0.3, @1.2, @1, @0.3], @[@0, @0.1, @0.6, @1], 2.0, 0, NO, 1);
        CFTimeInterval dur = Rnd(1.5, 2.3);
        Spawn(h, w, Rnd(0, 0.12), dur, Ballistic(SrcP(i), Vel(SrcA(h, i, 0.85), SrcV(Rnd(250, 760))), 60, 2.6, dur, Rnd(4, 12), 0.8, 0), nil, @[@0, @0.05, @0.7, @1], NO);
    }
    StarSprinkle(h, Count(20), nil, YES, 3, 10, 0.6);
    Dust(h, Count(30), @[@0xFFFFFF, @0xFFF0C2, @0xD7E6FF], 0.8);
}

static void FX_Meteor(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0x7FA8FF, @0xB9C9FF, @0x8FE3FF, @0xC6B3FF], 2.0, 0.55, 120, NO);
    NSArray *cols = @[@0xFFFFFF, @0xDDE8FF, @0xFFE9C7, @0xCFF4FF, @0xF2D9FF];
    int n = Count(13);
    for (int i = 0; i < n; i++) {
        CGFloat head = Rnd(2.4, 5.2);
        CGFloat ang = M_PI * 0.72 + Rnd(-0.06, 0.06);
        CGPoint p0 = Coin(0.6) ? CGPointMake(Rnd(0.2, 1.25) * b.size.width, -Rnd(20, 120))
                               : CGPointMake(b.size.width + Rnd(20, 80), Rnd(0, 0.55) * b.size.height);
        CGFloat len = Rnd(0.7, 1.2) * hypot(b.size.width, b.size.height) * 0.7;
        CGPoint p1 = CGPointMake(p0.x + cos(ang) * len, p0.y + sin(ang) * len);
        CFTimeInterval dur = len / (Rnd(600, 1050) * (head / 4.0 + 0.5));
        CALayer *body = MeteorBody(head, Rnd(46, 120) * (head / 3.5), RGB(PickI(cols)), arc4random_uniform(5));
        Spawn(h, body, i * 0.03 + Rnd(0, 1.0), dur, @[[NSValue valueWithCGPoint:p0], [NSValue valueWithCGPoint:p1]], @[@0, @1, @1, @0], @[@0, @0.08, @0.8, @1], YES);
    }
    int k = SprayCount(7);
    for (int i = 0; i < k; i++) {
        CGPoint p = SrcP(i);
        CGFloat ang = SrcA(h, i, 0.6), len = SrcV(Rnd(160, 320));
        CGPoint p1 = CGPointMake(p.x + cos(ang) * len, p.y + sin(ang) * len);
        Spawn(h, MeteorBody(Rnd(2, 3.4), Rnd(30, 60), RGB(0xFFFFFF), 2), Rnd(0, 0.15), Rnd(0.4, 0.65),
              @[[NSValue valueWithCGPoint:p], [NSValue valueWithCGPoint:p1]], @[@0, @1, @0.8, @0], @[@0, @0.1, @0.7, @1], YES);
    }
    StarSprinkle(h, Count(18), nil, NO, 2.5, 6, 1.2);
    Dust(h, Count(20), @[@0xFFFFFF, @0xDDE8FF], 1.2);
}

static void FX_Confetti(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0xFF5C8A, @0xFFC93C, @0x3FD1E3, @0xA77BFF, @0x6BD67A], 2.0, 0.55, 120, NO);
    NSArray *cols = @[@0xFF4D79, @0xFFC93C, @0x3FD1E3, @0x6BD67A, @0xA77BFF, @0xFF8A4C, @0xFFFFFF, @0x4D8DFF];
    void (^piece)(CGPoint, CGVector, CFTimeInterval, CFTimeInterval) = ^(CGPoint p0, CGVector v, CFTimeInterval delay, CFTimeInterval dur) {
        int shape = arc4random_uniform(10);
        UIColor *c = RGB(PickI(cols));
        CALayer *cont;
        if (shape < 5) { CGFloat w = Rnd(4, 7), hh = w * Rnd(1.6, 2.4); cont = PlainL([UIBezierPath bezierPathWithRoundedRect:CGRectMake(-w/2, -hh/2, w, hh) cornerRadius:1], c); }
        else if (shape < 7) cont = PlainL(CircleP(Rnd(2.2, 3.6)), c);
        else if (shape < 8) cont = PlainL(Star5P(Rnd(3.5, 5.5)), c);
        else {
            UIBezierPath *p = [UIBezierPath bezierPath];
            CGFloat L = Rnd(10, 16);
            [p moveToPoint:CGPointMake(0, -L/2)];
            [p addCurveToPoint:CGPointMake(0, L/2) controlPoint1:CGPointMake(L * 0.45, -L * 0.15) controlPoint2:CGPointMake(-L * 0.45, L * 0.15)];
            cont = PlainStroke(p, c, Rnd(1.6, 2.4));
        }
        CALayer *flip = Wrap(cont);
        Flip(flip, Coin(0.6), Rnd(0.35, 0.8));
        CALayer *w = Wrap(flip);
        w.sublayerTransform = Persp(200);
        Spin(w, Rnd(0.3, 1.4) * (Coin(0.5) ? 1 : -1), dur);
        Spawn(h, w, delay, dur, Ballistic(p0, v, 900, 2.6, dur, Rnd(8, 22), Rnd(0.6, 1.3), 0), @[@1, @1, @1, @0], @[@0, @0.1, @0.82, @1], NO);
    };
    if (gSrcEdge) {
        int n = SprayCount(60);
        for (int i = 0; i < n; i++)
            piece(SrcP(i), Vel(SrcA(h, i, 0.6), Rnd(1100, 2000)), Rnd(0, 0.18), Rnd(2.4, 3.4));
    } else {
        int n = Count(44);
        for (int side = 0; side < 2; side++) {
            CGPoint p0 = CGPointMake(side ? b.size.width + 6 : -6, b.size.height + 6);
            for (int i = 0; i < n; i++) {
                CGFloat ang = side ? (-M_PI + Rnd(0.75, 1.3)) : -Rnd(0.75, 1.3);
                piece(p0, Vel(ang, Rnd(1100, 2000)), Rnd(0, 0.18), Rnd(2.4, 3.4));
            }
        }
        for (int i = 0; i < Count(12); i++) piece(o, Vel(SprayAngle(h, o, 0.9), Rnd(300, 700)), 0, Rnd(1.8, 2.4));
    }
    for (int i = 0; i < Count(22); i++)
        piece(CGPointMake(Rnd(0, b.size.width), -Rnd(10, 60)), CGVectorMake(Rnd(-30, 30), Rnd(20, 80)), Rnd(0.2, 1.0), Rnd(2.4, 3.2));
}

static void FX_Bubble(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0x6FD3FF, @0xB59CFF, @0x9CFFE3, @0xFFB3E6], 2.4, 0.55, 120, NO);
    int n = Count(28);
    for (int i = 0; i < n; i++) {
        CGFloat r = Coin(0.2) ? Rnd(14, 24) : Rnd(4, 11);
        CALayer *bb = BubbleL(r * 2);
        CFTimeInterval dur = Rnd(2.0, 3.4);
        KF(bb, @"transform.scale", @[@0.6, @1, @1, @1.45], @[@0, @0.2, @0.9, @1], dur, 0, NO, 1);
        CALayer *w = Wrap(bb);
        KF(w, @"transform", @[[NSValue valueWithCATransform3D:CATransform3DMakeScale(1.06, 0.94, 1)], [NSValue valueWithCATransform3D:CATransform3DMakeScale(0.95, 1.05, 1)]], nil, Rnd(0.35, 0.6), 0, YES, HUGE_VALF);
        CGPoint p0 = CGPointMake(Rnd(0, b.size.width), b.size.height + r + Rnd(0, 40));
        CGFloat top = Rnd(0.08, 0.6) * b.size.height, ax = Rnd(8, 22), fx = Rnd(0.4, 0.9), ph = Rnd(0, 6.28);
        NSArray *pts = Sample(^CGPoint(CGFloat t, CGFloat s) {
            CGFloat e = 1 - pow(1 - t, 1.6);
            return CGPointMake(p0.x + ax * sin(2 * M_PI * fx * s + ph), p0.y + (top - p0.y) * e);
        }, dur, 30);
        Spawn(h, w, Rnd(0, 0.9), dur, pts, @[@0, @1, @1, @0], @[@0, @0.08, @0.9, @1], NO);
    }
    int k = SprayCount(14);
    for (int i = 0; i < k; i++) {
        CALayer *bb = BubbleL(Rnd(6, 16));
        CFTimeInterval dur = Rnd(1.6, 2.2);
        Spawn(h, bb, Rnd(0, 0.12), dur, Ballistic(SrcP(i), Vel(SrcA(h, i, 0.8), SrcV(Rnd(150, 380))), -90, 1.6, dur, 10, 0.9, 0), nil, nil, NO);
    }
    Dust(h, Count(14), @[@0xFFFFFF, @0xCFF4FF], 1.0);
}

static void FX_Firework(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0xFF7043, @0xFFC14D, @0xB388FF, @0x40C4FF], 2.4, 0.5, 120, NO);
    NSArray *cols = @[@0xFF4D79, @0xFFD54F, @0x64FFDA, @0xB388FF, @0x40C4FF, @0xFF8A4C, @0xFFFFFF];
    void (^burst)(CGPoint, CFTimeInterval, CGFloat) = ^(CGPoint c, CFTimeInterval delay, CGFloat scale) {
        UIColor *main = RGB(PickI(cols)), *alt = RGB(PickI(cols));
        CAGradientLayer *flash = [CAGradientLayer layer];
        flash.type = kCAGradientLayerRadial;
        flash.bounds = CGRectMake(0, 0, 200 * scale, 200 * scale);
        flash.startPoint = CGPointMake(0.5, 0.5); flash.endPoint = CGPointMake(1, 1);
        flash.colors = @[(id)[main colorWithAlphaComponent:0.5].CGColor, (id)[main colorWithAlphaComponent:0].CGColor];
        KF(flash, @"transform.scale", @[@0.3, @1.15], nil, 0.45, delay, NO, 1);
        Spawn(h, flash, delay, 0.5, @[[NSValue valueWithCGPoint:c], [NSValue valueWithCGPoint:c]], @[@0, @1, @0], @[@0, @0.12, @1], NO);
        CALayer *big = GS(@"s4", Star4P, Rnd(20, 30) * scale, UIColor.whiteColor, main, 0.4);
        KF(big, @"transform.scale", @[@0, @1.3, @0.8, @0], @[@0, @0.18, @0.5, @1], 0.7, delay, NO, 1);
        CALayer *bw = Wrap(big); Spin(bw, 0.35, 1.2);
        Spawn(h, bw, delay, 0.7, @[[NSValue valueWithCGPoint:c], [NSValue valueWithCGPoint:c]], @[@0, @1, @1, @0], @[@0, @0.1, @0.6, @1], NO);
        int n = Count(24 * scale);
        BOOL ring = Coin(0.4);
        for (int i = 0; i < n; i++) {
            CGFloat ang = ring ? (2 * M_PI * i / n + Rnd(-0.06, 0.06)) : Rnd(0, 2 * M_PI);
            CGFloat sp = (ring ? Rnd(230, 260) : Rnd(80, 280)) * scale;
            UIColor *c1 = Coin(0.7) ? main : alt;
            CALayer *el;
            int kind = arc4random_uniform(10);
            if (kind < 4) { CALayer *s4 = GS(@"s4", Star4P, Rnd(6, 13), Coin(0.5) ? UIColor.whiteColor : c1, c1, 0.4); el = Wrap(s4); Spin(el, Rnd(0.3, 1.0) * (Coin(0.5) ? 1 : -1), 1.6); }
            else if (kind < 6) { CALayer *s5 = GS(@"s5", Star5P, Rnd(6, 10), c1, c1, 0.35); el = Wrap(s5); Spin(el, Rnd(0.3, 1.2) * (Coin(0.5) ? 1 : -1), 1.6); }
            else el = DotL(Rnd(1.0, 2.0), c1);
            Twinkle(el, 0.25, 1, Rnd(0.07, 0.16), delay + Rnd(0.25, 0.45));
            KF(el, @"transform.scale", @[@0.3, @1.15, @1, @0.2], @[@0, @0.12, @0.6, @1], 1.4, delay, NO, 1);
            CFTimeInterval dur = Rnd(1.1, 1.6);
            Spawn(h, el, delay, dur, Ballistic(c, Vel(ang, sp), 45, 2.6, dur, 0, 0, 0), @[@0, @1, @1, @0], @[@0, @0.05, @0.7, @1], NO);
        }
    };
    if (!gSrcEdge) burst(o, 0, 0.8);
    int shells = MAX(3, Count(5));
    for (int k = 0; k < shells; k++) {
        CGPoint c = CGPointMake(Rnd(0.14, 0.86) * b.size.width, Rnd(0.12, 0.72) * b.size.height);
        burst(c, (gSrcEdge ? 0 : 0.12) + k * Rnd(0.14, 0.26), Rnd(0.8, 1.15));
    }
    Dust(h, Count(22), @[@0xFFE9A8, @0xFFFFFF, @0xFFD1DC], 1.2);
}

// 智能光：范围收窄，贴近屏幕边缘
static void FX_AI(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    NSArray *pal = @[@0xBC82F3, @0xF5B9EA, @0x8D9FFF, @0xFF6778, @0xFFBA71, @0xC686FF];
    EdgeAura(h, pal, 2.6, 1.0, 105, YES);
    EdgeAura(h, @[@0xFFFFFF, @0xF5B9EA, @0xC686FF, @0xFFBA71], 2.2, 0.4, 32, YES);
    int n = Count(26);
    for (int i = 0; i < n; i++) {
        CGPoint p0 = Perimeter(b.size, Rnd(0, 1));
        CGPoint c = CGPointMake(b.size.width / 2, b.size.height / 2);
        CGFloat dist = Rnd(20, 70), ang = atan2(c.y - p0.y, c.x - p0.x) + Rnd(-0.4, 0.4);
        CGPoint p1 = CGPointMake(p0.x + cos(ang) * dist, p0.y + sin(ang) * dist);
        CALayer *el;
        if (Coin(0.35)) { CALayer *s = HueGS(@"s4", Star4P, Rnd(6, 13), 0.35, Rnd(1.2, 2.2)); el = Wrap(s); Spin(el, Rnd(0.2, 0.5), 2.5); }
        else el = DotL(Rnd(0.8, 1.8), RGB(PickI(pal)));
        Twinkle(el, 0.3, 1, Rnd(0.2, 0.5), 0);
        Spawn(h, el, Rnd(0, 0.8), Rnd(1.4, 2.2), @[[NSValue valueWithCGPoint:p0], [NSValue valueWithCGPoint:p1]], nil, nil, NO);
    }
    int k = SprayCount(14);
    for (int i = 0; i < k; i++) {
        CALayer *d = DotL(Rnd(1, 2), RGB(PickI(pal)));
        CFTimeInterval dur = Rnd(1.2, 1.8);
        Spawn(h, d, Rnd(0, 0.15), dur, Ballistic(SrcP(i), Vel(SrcA(h, i, 0.8), SrcV(Rnd(120, 360))), -80, 2, dur, 8, 1, 0), nil, nil, NO);
    }
}

static void FX_Snow(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0x9FD8FF, @0xDDEFFF, @0xB8C8FF, @0xE8F7FF], 2.8, 0.6, 130, NO);
    int n = Count(50);
    for (int i = 0; i < n; i++) {
        CGFloat depth = Rnd(0, 1);
        CALayer *el;
        if (depth > 0.55) { el = Wrap(SnowL(8 + depth * 12, Coin(0.5))); Spin(el, Rnd(0.05, 0.25) * (Coin(0.5) ? 1 : -1), 5); }
        else el = DotL(0.8 + depth * 1.8, [UIColor colorWithWhite:1 alpha:1]);
        el.opacity = 0.45 + depth * 0.55;
        CGPoint p0 = CGPointMake(Rnd(-0.1, 1.15) * b.size.width, -Rnd(10, 160));
        CFTimeInterval dur = Rnd(3.0, 4.6) - depth * 0.8;
        NSArray *pts = Ballistic(p0, CGVectorMake(-Rnd(5, 30), 40 + depth * 80), 40 + depth * 40, 0.8, dur, Rnd(10, 26), Rnd(0.25, 0.55), 0);
        Spawn(h, el, Rnd(0, 1.0), dur, pts, @[@0, @1, @1, @0], @[@0, @0.1, @0.8, @1], NO);
    }
    int k = SprayCount(14);
    for (int i = 0; i < k; i++) {
        CALayer *w = Wrap(SnowL(Rnd(7, 12), NO)); Spin(w, 0.4, 2);
        CFTimeInterval dur = Rnd(1.8, 2.4);
        Spawn(h, w, Rnd(0, 0.12), dur, Ballistic(SrcP(i), Vel(SrcA(h, i, 0.8), SrcV(Rnd(150, 380))), 60, 1.8, dur, 8, 0.6, 0), nil, nil, NO);
    }
}

static void FX_Firefly(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0xC6FF6B, @0xFFE66B, @0x9BFFB0, @0xEAFF8A], 2.8, 0.4, 110, NO);
    NSArray *cols = @[@0xD4FF6B, @0xFFE66B, @0xA8FF8A, @0xF6FFB0];
    int n = Count(32);
    for (int i = 0; i < n; i++) {
        CALayer *f = DotL(Rnd(1.0, 2.4), RGB(PickI(cols)));
        Twinkle(f, 0.15, 1, Rnd(0.35, 0.9), 0);
        KF(f, @"transform.scale", @[@0.7, @1.3], nil, Rnd(0.4, 0.9), 0, YES, HUGE_VALF);
        CGPoint c = CGPointMake(Rnd(0.05, 0.95) * b.size.width, Rnd(0.15, 0.95) * b.size.height);
        CGFloat rx = Rnd(10, 40), ry = Rnd(8, 30), w1 = Rnd(0.3, 0.8), w2 = Rnd(0.25, 0.7), ph = Rnd(0, 6.28), drift = Rnd(-30, -5);
        CFTimeInterval dur = Rnd(2.4, 3.4);
        NSArray *pts = Sample(^CGPoint(CGFloat t, CGFloat s) {
            return CGPointMake(c.x + rx * cos(2 * M_PI * w1 * s + ph), c.y + ry * sin(2 * M_PI * w2 * s + ph * 1.3) + drift * s);
        }, dur, 40);
        Spawn(h, f, Rnd(0, 0.8), dur, pts, @[@0, @1, @1, @0], @[@0, @0.2, @0.7, @1], NO);
    }
    int k = SprayCount(14);
    for (int i = 0; i < k; i++) {
        CALayer *f = DotL(Rnd(1.0, 2.0), RGB(PickI(cols)));
        Twinkle(f, 0.2, 1, Rnd(0.3, 0.6), 0);
        CFTimeInterval dur = Rnd(2.0, 2.8);
        Spawn(h, f, Rnd(0, 0.3), dur, Ballistic(SrcP(i), Vel(SrcA(h, i, 0.9), SrcV(Rnd(80, 220))), -30, 1.2, dur, 20, 0.5, 0), nil, nil, NO);
    }
}

static void FX_EmojiRain(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0xFF7AB6, @0xFFC24D, @0xFF8A4C, @0xFFB3D1], 2.0, 0.55, 120, NO);
    NSArray *es = @[@"😍", @"🥰", @"💖", @"🔥", @"👍", @"💯", @"😘", @"🤩", @"💕", @"✨", @"🎉", @"❤️"];
    for (int i = 0; i < Count(28); i++) {
        CGFloat s = Coin(0.3) ? Rnd(22, 30) : Rnd(12, 20);
        CALayer *w = Wrap(EmojiL(es[arc4random_uniform((uint32_t)es.count)], s)); Spin(w, Rnd(-0.3, 0.3), 3);
        CFTimeInterval dur = Rnd(1.8, 2.8);
        CGPoint p0 = CGPointMake(Rnd(0, b.size.width), -Rnd(20, 120));
        Spawn(h, w, Rnd(0, 0.8), dur, Ballistic(p0, CGVectorMake(Rnd(-20, 20), Rnd(80, 200)), 700, 1.6, dur, Rnd(0, 10), 0.6, 0), @[@0, @1, @1, @0], @[@0, @0.08, @0.85, @1], NO);
    }
    int k = SprayCount(16);
    for (int i = 0; i < k; i++) {
        CALayer *e = EmojiL(es[arc4random_uniform((uint32_t)es.count)], Rnd(12, 22));
        CALayer *w = Wrap(e); Spin(w, Rnd(-0.8, 0.8), 2);
        KF(e, @"transform.scale", @[@0.3, @1.15, @1], @[@0, @0.15, @1], 1.6, 0, NO, 1);
        CFTimeInterval dur = Rnd(1.6, 2.2);
        Spawn(h, w, Rnd(0, 0.12), dur, Ballistic(SrcP(i), Vel(SrcA(h, i, 0.8), SrcV(Rnd(350, 750))), 900, 1.4, dur, 0, 0, 0), @[@0, @1, @1, @0], @[@0, @0.05, @0.75, @1], NO);
    }
}

static void FX_Rose(CALayer *h, CGPoint o) {
    CGRect b = h.bounds;
    EdgeAura(h, @[@0xD3123A, @0xFF3D6E, @0x9E0B2E, @0xFF7A93], 2.4, 0.8, 130, NO);
    NSArray *pal = @[@[@0xFF4D6D, @0xA8001E], @[@0xE8174A, @0x7A0018], @[@0xFF6F8E, @0xC2002F], @[@0xFF8FA3, @0xD3123A]];
    CALayer *(^petal)(CGFloat) = ^CALayer *(CGFloat s) {
        NSArray *c = pal[arc4random_uniform(4)];
        CALayer *flip = Wrap(RosePetalL(s, [c[0] intValue], [c[1] intValue])); Flip(flip, Coin(0.5), Rnd(0.8, 1.8));
        CALayer *w = Wrap(flip);
        w.sublayerTransform = Persp(240);
        Spin(w, Rnd(0.1, 0.5) * (Coin(0.5) ? 1 : -1), 4);
        return w;
    };
    for (int i = 0; i < Count(34); i++) {
        CGFloat s = Coin(0.4) ? Rnd(7, 10) : Rnd(11, 17);
        CFTimeInterval dur = Rnd(2.6, 3.8);
        CGPoint p0 = CGPointMake(Rnd(-0.1, 1.1) * b.size.width, -Rnd(15, 140));
        Spawn(h, petal(s), Rnd(0, 0.9), dur, Ballistic(p0, CGVectorMake(Rnd(-40, 40), Rnd(80, 150)), 80, 0.9, dur, Rnd(14, 36), Rnd(0.3, 0.6), 0), @[@0, @1, @1, @0], @[@0, @0.1, @0.82, @1], NO);
    }
    int k = SprayCount(14);
    for (int i = 0; i < k; i++) {
        CFTimeInterval dur = Rnd(1.8, 2.5);
        Spawn(h, petal(Rnd(8, 13)), Rnd(0, 0.12), dur, Ballistic(SrcP(i), Vel(SrcA(h, i, 0.8), SrcV(Rnd(220, 520))), 200, 1.8, dur, 10, 0.6, 0), nil, nil, NO);
    }
    for (int i = 0; i < Count(12); i++) {
        CGFloat s = Rnd(5, 10);
        CALayer *hr = HeartL(s, 0xFF6F8E, 0xC2002F, YES);
        Twinkle(hr, 0.5, 1, Rnd(0.3, 0.6), 0);
        CGPoint p0 = CGPointMake(Rnd(0.1, 0.9) * b.size.width, Rnd(0.3, 0.95) * b.size.height);
        CFTimeInterval dur = Rnd(1.4, 2.0);
        Spawn(h, hr, Rnd(0, 0.6), dur, Ballistic(p0, CGVectorMake(0, -Rnd(30, 80)), -30, 0.6, dur, 8, 0.8, 0), nil, nil, NO);
    }
    Dust(h, Count(18), @[@0xFFD6DE, @0xFFFFFF], 0.9);
}

#pragma mark - 触发调度

static CFTimeInterval gLastFire;
static NSString *gDiag = @"尚未触发";
static NSString *gHookInfo = @"未安装";
static NSUInteger gFXFires, gFXSkippedStandalone, gFXSkippedInactive;

// 3.11.33：独立版也装着（它的面板类在）→ 合并版不放，免得一次点赞放两遍
static BOOL StandalonePresent(void) {
    static BOOL seen;
    if (!seen) seen = NSClassFromString(@"AAPanelWindow") != nil || NSClassFromString(@"AAPassWindow") != nil;
    return seen;
}

static void FireAt(CGPoint origin, NSInteger style, NSString *source) {
    if (!ALBDockHostPluginActive()) { gFXSkippedInactive++; return; }
    if (StandalonePresent()) { gFXSkippedStandalone++; gDiag = @"检测到独立版点赞特效在运行，合并版不放（卸载独立版后生效）"; return; }
    if (Now() - gLastFire < 0.35) return;
    gLastFire = Now();
    gFXTime = DurationScale();
    gFXFires++;
    CALayer *h = FXLayer();
    if (!h) return;
    if (style == AAStyleRandom || style < 0 || style >= AAStyleCount) style = arc4random_uniform(AAStyleRandom);
    gDiag = [NSString stringWithFormat:@"最近触发：%@（%@，时长×%.2f）", source,
             style >= 0 && style < (NSInteger)StyleNames().count ? StyleNames()[style] : @"?", gFXTime];
    if (Haptic()) {
        UIImpactFeedbackGenerator *f = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
        [f impactOccurred];
    }
    SetupSources(h, origin);
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    switch (style) {
        case AAStyleSakura:   FX_Sakura(h, origin); break;
        case AAStyleStar:     FX_Star(h, origin); break;
        case AAStyleMeteor:   FX_Meteor(h, origin); break;
        case AAStyleConfetti: FX_Confetti(h, origin); break;
        case AAStyleBubble:   FX_Bubble(h, origin); break;
        case AAStyleFirework: FX_Firework(h, origin); break;
        case AAStyleAI:       FX_AI(h, origin); break;
        case AAStyleSnow:     FX_Snow(h, origin); break;
        case AAStyleFirefly:  FX_Firefly(h, origin); break;
        case AAStyleEmoji:    FX_EmojiRain(h, origin); break;
        case AAStyleRose:     FX_Rose(h, origin); break;
        default:              FX_Heart(h, origin); break;
    }
    [CATransaction commit];
}
static CGPoint DefaultOrigin(void) {
    CGRect b = ActiveScene().coordinateSpace.bounds;
    return CGPointMake(b.size.width - 34, b.size.height * 0.52);
}
#pragma mark - 悬浮球 + 设置面板

@interface ALBFXPanelWindow : UIWindow
@property (nonatomic, strong) UIView *ball;
@property (nonatomic, strong) UIVisualEffectView *panel;
@property (nonatomic, strong) UILabel *diag;
@property (nonatomic, strong) NSMutableArray<UISwitch *> *switches;
- (void)ballTap;
- (void)setupUI;
- (void)closePanel;
@end

static ALBFXPanelWindow *gPanelWindow;

static UIVisualEffectView *GlassView(CGFloat radius) {
    UIVisualEffect *eff = nil;
    Class gc = NSClassFromString(@"UIGlassEffect");
    if (gc && [gc respondsToSelector:@selector(effectWithStyle:)]) {
        eff = ((id (*)(id, SEL, NSInteger))objc_msgSend)(gc, @selector(effectWithStyle:), 0);
        @try { [eff setValue:@YES forKey:@"interactive"]; } @catch (__unused id e) {}
    }
    if (!eff) eff = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemMaterialDark];
    UIVisualEffectView *v = [[UIVisualEffectView alloc] initWithEffect:eff];
    v.layer.cornerRadius = radius;
    v.layer.cornerCurve = kCACornerCurveContinuous;
    v.clipsToBounds = YES;
    return v;
}

@implementation ALBFXPanelWindow {
    NSMutableArray<UIButton *> *_styleButtons;
    UIView *_dim;
}

- (UIView *)hitTest:(CGPoint)p withEvent:(UIEvent *)e {
    UIView *v = [super hitTest:p withEvent:e];
    if (!self.panel.hidden) return v;
    if (self.ball && !self.ball.hidden && CGRectContainsPoint(self.ball.frame, p)) return v;
    return nil;
}

- (void)setupUI {
    UIView *root = self.rootViewController.view;
    _dim = [[UIView alloc] initWithFrame:root.bounds];
    _dim.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    _dim.backgroundColor = [UIColor colorWithWhite:0 alpha:0.25];
    _dim.hidden = YES;
    [_dim addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(closePanel)]];
    [root addSubview:_dim];

    UIVisualEffectView *ball = GlassView(24);
    ball.frame = CGRectMake(0, 0, 48, 48);
    UIImageView *icon = [[UIImageView alloc] initWithImage:[[UIImage systemImageNamed:@"heart.fill" withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:19 weight:UIImageSymbolWeightSemibold]] imageWithTintColor:RGB(0xFF5C7A) renderingMode:UIImageRenderingModeAlwaysOriginal]];
    icon.contentMode = UIViewContentModeCenter;
    icon.frame = ball.bounds;
    [ball.contentView addSubview:icon];
    CGRect sb = root.bounds;
    ball.center = CGPointMake(ADbl(KEY("ballX"), sb.size.width - 30), ADbl(KEY("ballY"), sb.size.height * 0.3));
    [root addSubview:ball];
    self.ball = ball;
    [ball addGestureRecognizer:[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(ballTap)]];
    [ball addGestureRecognizer:[[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(ballPan:)]];
    [ball addGestureRecognizer:[[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(ballLong:)]];
    ball.hidden = !BallVisible();
    [self buildPanel];
}

- (void)ballLong:(UILongPressGestureRecognizer *)g {
    if (g.state == UIGestureRecognizerStateBegan) FireAt(DefaultOrigin(), CurStyle(), @"长按预览");
}

- (void)ballPan:(UIPanGestureRecognizer *)g {
    UIView *root = self.rootViewController.view;
    CGPoint t = [g translationInView:root];
    self.ball.center = CGPointMake(self.ball.center.x + t.x, self.ball.center.y + t.y);
    [g setTranslation:CGPointZero inView:root];
    if (g.state == UIGestureRecognizerStateEnded || g.state == UIGestureRecognizerStateCancelled) {
        CGRect b = root.bounds;
        UIEdgeInsets s = root.safeAreaInsets;
        CGFloat x = self.ball.center.x < b.size.width / 2 ? 30 : b.size.width - 30;
        CGFloat y = MIN(MAX(self.ball.center.y, s.top + 30), b.size.height - s.bottom - 30);
        [UIView animateWithDuration:0.45 delay:0 usingSpringWithDamping:0.7 initialSpringVelocity:0.5 options:0 animations:^{
            self.ball.center = CGPointMake(x, y);
        } completion:nil];
        [AD() setDouble:x forKey:KEY("ballX")];
        [AD() setDouble:y forKey:KEY("ballY")];
    }
}

- (UILabel *)label:(NSString *)t size:(CGFloat)s weight:(UIFontWeight)w {
    UILabel *l = [UILabel new];
    l.text = t; l.font = [UIFont systemFontOfSize:s weight:w];
    l.textColor = UIColor.whiteColor;
    return l;
}

- (UIView *)switchRow:(NSString *)title key:(NSString *)key def:(BOOL)def {
    UIStackView *row = [UIStackView new];
    row.alignment = UIStackViewAlignmentCenter;
    UISwitch *sw = [UISwitch new];
    sw.onTintColor = RGB(0xFF5C7A);
    sw.on = ABool(key, def);
    sw.accessibilityIdentifier = key;
    sw.transform = CGAffineTransformMakeScale(0.85, 0.85);
    [sw addTarget:self action:@selector(switchChanged:) forControlEvents:UIControlEventValueChanged];
    if (!self.switches) self.switches = [NSMutableArray array];
    [self.switches addObject:sw];
    [row addArrangedSubview:[self label:title size:14 weight:UIFontWeightMedium]];
    [row addArrangedSubview:[UIView new]];
    [row addArrangedSubview:sw];
    return row;
}

- (void)switchChanged:(UISwitch *)sw {
    [AD() setBool:sw.on forKey:sw.accessibilityIdentifier];
    if ([sw.accessibilityIdentifier isEqualToString:KEY("ball")] && !sw.on) {
        [self closePanel];
        self.ball.hidden = YES;
    }
}

- (void)buildPanel {
    UIView *root = self.rootViewController.view;
    UIVisualEffectView *p = GlassView(28);
    CGFloat W = MIN(340, root.bounds.size.width - 28);
    p.hidden = YES;
    [root addSubview:p];
    self.panel = p;

    UIStackView *st = [UIStackView new];
    st.axis = UILayoutConstraintAxisVertical; st.spacing = 9;
    st.translatesAutoresizingMaskIntoConstraints = NO;
    [p.contentView addSubview:st];
    [NSLayoutConstraint activateConstraints:@[
        [st.leadingAnchor constraintEqualToAnchor:p.contentView.leadingAnchor constant:18],
        [st.trailingAnchor constraintEqualToAnchor:p.contentView.trailingAnchor constant:-18],
        [st.topAnchor constraintEqualToAnchor:p.contentView.topAnchor constant:16],
        [st.bottomAnchor constraintEqualToAnchor:p.contentView.bottomAnchor constant:-14],
    ]];

    UIStackView *head = [UIStackView new];
    head.alignment = UIStackViewAlignmentCenter;
    UIButton *close = [UIButton buttonWithType:UIButtonTypeSystem];
    [close setImage:[UIImage systemImageNamed:@"xmark.circle.fill"] forState:UIControlStateNormal];
    close.tintColor = [UIColor colorWithWhite:1 alpha:0.6];
    [close addTarget:self action:@selector(closePanel) forControlEvents:UIControlEventTouchUpInside];
    [head addArrangedSubview:[self label:@"氛围特效" size:18 weight:UIFontWeightBold]];
    [head addArrangedSubview:[UIView new]];
    [head addArrangedSubview:close];
    [st addArrangedSubview:head];
    [st addArrangedSubview:[self switchRow:@"总开关" key:KEY("enabled") def:YES]];

    _styleButtons = [NSMutableArray array];
    UIStackView *grid = [UIStackView new];
    grid.axis = UILayoutConstraintAxisVertical; grid.spacing = 7;
    NSArray *names = StyleNames();
    for (int r = 0; r < 4; r++) {
        UIStackView *row = [UIStackView new];
        row.distribution = UIStackViewDistributionFillEqually; row.spacing = 7;
        for (int c = 0; c < 4; c++) {
            NSInteger idx = r * 4 + c;
            if (idx >= (NSInteger)names.count) { [row addArrangedSubview:[UIView new]]; continue; }
            UIButton *b = [UIButton buttonWithType:UIButtonTypeCustom];
            b.tag = idx;
            [b setTitle:names[idx] forState:UIControlStateNormal];
            b.titleLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
            b.titleLabel.adjustsFontSizeToFitWidth = YES;
            b.layer.cornerRadius = 15; b.layer.cornerCurve = kCACornerCurveContinuous;
            [b.heightAnchor constraintEqualToConstant:32].active = YES;
            [b addTarget:self action:@selector(styleTap:) forControlEvents:UIControlEventTouchUpInside];
            [row addArrangedSubview:b];
            [_styleButtons addObject:b];
        }
        [grid addArrangedSubview:row];
    }
    [st addArrangedSubview:grid];
    [self refreshStyles];

    UIStackView *irow = [UIStackView new];
    irow.spacing = 10; irow.alignment = UIStackViewAlignmentCenter;
    [irow addArrangedSubview:[self label:@"粒子密度" size:14 weight:UIFontWeightMedium]];
    UISlider *sl = [UISlider new];
    sl.minimumValue = 0.4; sl.maximumValue = 2.0; sl.value = Intensity();
    sl.minimumTrackTintColor = RGB(0xFF5C7A);
    [sl addTarget:self action:@selector(sliderChanged:) forControlEvents:UIControlEventValueChanged];
    [irow addArrangedSubview:sl];
    [st addArrangedSubview:irow];

    // 3.11.33：动画时长（和设置页的「动画时长」是同一个值）
    UIStackView *drow = [UIStackView new];
    drow.spacing = 10; drow.alignment = UIStackViewAlignmentCenter;
    [drow addArrangedSubview:[self label:@"动画时长" size:14 weight:UIFontWeightMedium]];
    UISlider *dsl = [UISlider new];
    dsl.minimumValue = 0.5; dsl.maximumValue = 2.5; dsl.value = DurationScale();
    dsl.minimumTrackTintColor = RGB(0xFF5C7A);
    [dsl addTarget:self action:@selector(durationChanged:) forControlEvents:UIControlEventValueChanged];
    [dsl addTarget:self action:@selector(preview) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside];
    [drow addArrangedSubview:dsl];
    [st addArrangedSubview:drow];

    UIStackView *srow = [UIStackView new];
    srow.spacing = 10; srow.alignment = UIStackViewAlignmentCenter;
    [srow addArrangedSubview:[self label:@"喷射位置" size:14 weight:UIFontWeightMedium]];
    UISegmentedControl *seg = [[UISegmentedControl alloc] initWithItems:@[@"随机多点", @"两侧对称", @"点赞处"]];
    seg.selectedSegmentIndex = SprayMode();
    seg.selectedSegmentTintColor = RGB(0xFF5C7A);
    [seg setTitleTextAttributes:@{NSForegroundColorAttributeName: [UIColor colorWithWhite:1 alpha:0.85], NSFontAttributeName: [UIFont systemFontOfSize:12 weight:UIFontWeightSemibold]} forState:UIControlStateNormal];
    [seg setTitleTextAttributes:@{NSForegroundColorAttributeName: UIColor.whiteColor, NSFontAttributeName: [UIFont systemFontOfSize:12 weight:UIFontWeightBold]} forState:UIControlStateSelected];
    [seg addTarget:self action:@selector(sprayChanged:) forControlEvents:UIControlEventValueChanged];
    [srow addArrangedSubview:seg];
    [st addArrangedSubview:srow];

    [st addArrangedSubview:[self switchRow:@"点赞按钮触发" key:KEY("trigButton") def:YES]];
    [st addArrangedSubview:[self switchRow:@"双击点赞触发" key:KEY("trigDouble") def:YES]];
    [st addArrangedSubview:[self switchRow:@"边缘光晕" key:KEY("edgeGlow") def:YES]];
    [st addArrangedSubview:[self switchRow:@"震动反馈" key:KEY("haptic") def:YES]];
    [st addArrangedSubview:[self switchRow:@"显示悬浮球" key:KEY("ball") def:NO]];

    UIButton *prev = [UIButton buttonWithType:UIButtonTypeCustom];
    [prev setTitle:@"预览当前特效" forState:UIControlStateNormal];
    prev.titleLabel.font = [UIFont systemFontOfSize:15 weight:UIFontWeightBold];
    prev.backgroundColor = RGB(0xFF5C7A);
    prev.layer.cornerRadius = 20; prev.layer.cornerCurve = kCACornerCurveContinuous;
    [prev.heightAnchor constraintEqualToConstant:40].active = YES;
    [prev addTarget:self action:@selector(preview) forControlEvents:UIControlEventTouchUpInside];
    [st addArrangedSubview:prev];

    UILabel *d = [self label:@"" size:11 weight:UIFontWeightRegular];
    d.textColor = [UIColor colorWithWhite:1 alpha:0.6];
    d.textAlignment = NSTextAlignmentCenter; d.numberOfLines = 0;
    [st addArrangedSubview:d];
    self.diag = d;

    CGSize fit = [p systemLayoutSizeFittingSize:CGSizeMake(W, UILayoutFittingCompressedSize.height) withHorizontalFittingPriority:UILayoutPriorityRequired verticalFittingPriority:UILayoutPriorityFittingSizeLevel];
    p.frame = CGRectMake(0, 0, W, fit.height + 30);
}

- (void)refreshStyles {
    NSInteger cur = CurStyle();
    for (UIButton *b in _styleButtons) {
        BOOL on = b.tag == cur;
        b.backgroundColor = on ? RGB(0xFF5C7A) : [UIColor colorWithWhite:1 alpha:0.12];
        [b setTitleColor:on ? UIColor.whiteColor : [UIColor colorWithWhite:1 alpha:0.85] forState:UIControlStateNormal];
    }
}
- (void)styleTap:(UIButton *)b {
    [AD() setInteger:b.tag forKey:KEY("style")];
    [self refreshStyles];
    FireAt(DefaultOrigin(), b.tag, @"面板预览");
}
- (void)sprayChanged:(UISegmentedControl *)s { [AD() setInteger:s.selectedSegmentIndex forKey:KEY("spray")]; FireAt(DefaultOrigin(), CurStyle(), @"面板预览"); }
- (void)sliderChanged:(UISlider *)s { [AD() setDouble:s.value forKey:KEY("intensity")]; }
- (void)durationChanged:(UISlider *)s { [AD() setDouble:s.value forKey:KEY("duration")]; }
- (void)preview { FireAt(DefaultOrigin(), CurStyle(), @"面板预览"); }

- (void)ballTap {
    UIView *root = self.rootViewController.view;
    for (UISwitch *sw in self.switches) {
        NSString *k = sw.accessibilityIdentifier;
        BOOL def = ![k isEqualToString:KEY("compat")] && ![k isEqualToString:KEY("ball")];
        sw.on = ABool(k, def);
    }
    self.diag.text = [NSString stringWithFormat:@"识别引擎：%@\n%@\n长按悬浮球预览 · 隐藏后三指双击唤回", gHookInfo, gDiag];
    CGRect b = root.bounds;
    self.panel.center = CGPointMake(b.size.width / 2, b.size.height / 2);
    self.panel.hidden = NO; _dim.hidden = NO;
    self.panel.alpha = 0; _dim.alpha = 0;
    self.panel.transform = CGAffineTransformMakeScale(0.85, 0.85);
    [UIView animateWithDuration:0.42 delay:0 usingSpringWithDamping:0.78 initialSpringVelocity:0.4 options:0 animations:^{
        self.panel.alpha = 1; self->_dim.alpha = 1; self.panel.transform = CGAffineTransformIdentity;
    } completion:nil];
}
- (void)closePanel {
    [UIView animateWithDuration:0.22 animations:^{
        self.panel.alpha = 0; self->_dim.alpha = 0; self.panel.transform = CGAffineTransformMakeScale(0.9, 0.9);
    } completion:^(BOOL f) { self.panel.hidden = YES; self->_dim.hidden = YES; self.panel.transform = CGAffineTransformIdentity; }];
}
@end

static void SetupPanel(void) {
    UIWindowScene *scene = ActiveScene();
    if (!scene) return;
    if (gPanelWindow && gPanelWindow.windowScene == scene) return;
    gPanelWindow = [[ALBFXPanelWindow alloc] initWithWindowScene:scene];
    gPanelWindow.frame = scene.coordinateSpace.bounds;
    gPanelWindow.windowLevel = UIWindowLevelAlert + 20;
    gPanelWindow.backgroundColor = UIColor.clearColor;
    UIViewController *vc = [UIViewController new];
    vc.view.backgroundColor = UIColor.clearColor;
    gPanelWindow.rootViewController = vc;
    gPanelWindow.hidden = NO;
    [gPanelWindow setupUI];
    if (!gOwnWindows) gOwnWindows = [NSMutableSet set];
    [gOwnWindows addObject:gPanelWindow];
}

static void ShowBall(void) {
    if (!ALBDockHostPluginActive() || StandalonePresent()) return;   // 独立版在跑时让它自己的球出来
    [AD() setBool:YES forKey:KEY("ball")];
    SetupPanel();
    UIView *ball = gPanelWindow.ball;
    ball.hidden = NO;
    ball.transform = CGAffineTransformMakeScale(0.3, 0.3);
    [UIView animateWithDuration:0.5 delay:0 usingSpringWithDamping:0.6 initialSpringVelocity:0.5 options:0 animations:^{
        ball.transform = CGAffineTransformIdentity;
    } completion:nil];
}

#pragma mark - 触发识别
// 主引擎：直接监听抖音视频模型的“已点赞”状态从 0 变成 1。
// 只有在用户刚刚触摸过屏幕（1.5 秒内）时才算，避免刷到已赞视频时误触发。
// 取消点赞是 1→0，天然不会触发；双击点赞同样会把状态改成 1，所以也能识别。

static CFTimeInterval gLastTouchTime;
static CGPoint gLastTouchPt;
static BOOL gLastTouchOnLike;
static CGPoint gLikeCenter;
static CGPoint LikeOrigin(void) { return (gLikeCenter.x > 0) ? gLikeCenter : DefaultOrigin(); }
static CFTimeInterval gLastModelFire;

static BOOL OldDigged(id model) {
    @try { return [[model valueForKey:@"userDigged"] boolValue]; } @catch (__unused id e) { return NO; }
}
/* 3.11.48 用户：“点赞特效识别不到……我有双击点赞、三击点赞、双击面板点赞、新面板点赞，这么多点赞”。3.11.47 只认“按在点赞键上 / 真的
   双击”，面板里的点赞（没碰点赞键）一律被拦下，双击面板模式下的连击也认不出。现在按“哪一条作品变成已赞”来认：
   · 插件自己的点赞（双击面板、悬浮面板的“点赞作品”）点之前先登记这一条（ALBFXArmLike），2.5 秒内这一条变成已赞就放；
   · 用户 1.5 秒内碰过屏幕，而且碰的是视频上（往上找得到播放互动层 / 信息流 / 作品详情）、碰的那一条就是变成已赞的那一条
     （ALBFXAwemeIDForView，按作品 ID 对）——点赞键、双击、三击、连击都算；滑到下一条时碰的是上一条，进收藏 / 喜欢页碰的是
     列表格子，都对不上，不放；
   · 一次触摸最多放一次，0.8 秒内来的第二个“已赞”一律不算（列表批量刷新）。 */
extern "C" NSString *ALBFXAwemeIDForView(UIView *view);
extern "C" NSString *ALBFXAwemeIDForModel(id model);
static CFTimeInterval gLastDoubleTime, gLastConsumedGesture, gArmedUntil;
static CGPoint gLastDoublePt;
static __weak UIView *gLastTouchView;
static NSString *gArmedID;
static NSUInteger gFXModelIgnored, gFXModelBursts, gFXArmed;
static NSString *gFXIgnoredLast;
static BOOL InVideoArea(UIView *v);
extern "C" void ALBFXArmLike(id model) {
    gArmedUntil = CACurrentMediaTime() + 2.5;
    gArmedID = model ? ALBFXAwemeIDForModel(model) : nil;
    gFXArmed++;
}
static void OnDiggChanged(id model, BOOL old, BOOL now) {
    if (!Enabled() || old || !now) return;
    CFTimeInterval t = CACurrentMediaTime();
    if (t - gLastModelFire < 0.8) { gFXModelBursts++; return; }
    NSString *mid = ALBFXAwemeIDForModel(model);
    NSString *via = nil;
    CGPoint at = LikeOrigin();
    if (gArmedUntil > t && (!gArmedID.length || !mid.length || [gArmedID isEqualToString:mid])) {
        gArmedUntil = 0;
        via = @"面板点赞";
    } else if (t - gLastTouchTime < 1.5) {
        UIView *touched = gLastTouchView;
        if (!touched || !InVideoArea(touched)) {
            gFXModelIgnored++;
            gFXIgnoredLast = @"刚碰过屏幕，但碰的不是视频（列表、切页之类）";
            return;
        }
        NSString *tid = ALBFXAwemeIDForView(touched);
        if (tid.length && mid.length && ![tid isEqualToString:mid]) {
            gFXModelIgnored++;
            gFXIgnoredLast = @"碰的是另一条视频（滑动切换）";
            return;
        }
        BOOL doubled = gLastDoubleTime > 0 && t - gLastDoubleTime < 1.5;
        if (!tid.length && !gLastTouchOnLike && !doubled) {
            gFXModelIgnored++;
            gFXIgnoredLast = @"认不出碰的是哪一条，也不是点赞键 / 双击";
            return;
        }
        if (gLastTouchTime == gLastConsumedGesture) { gFXModelBursts++; return; }
        if (gLastTouchOnLike ? !TrigButton() : !TrigDouble()) return;
        gLastConsumedGesture = gLastTouchTime;
        via = gLastTouchOnLike ? @"点赞按钮" : @"双击 / 连击视频";
        if (!gLastTouchOnLike) at = doubled ? gLastDoublePt : gLastTouchPt;
    } else {
        gFXModelIgnored++;
        gFXIgnoredLast = @"最近没碰屏幕（数据刷新）";
        return;
    }
    gLastModelFire = t;
    NSString *label = [via stringByAppendingString:@"（模型）"];
    dispatch_async(dispatch_get_main_queue(), ^{ FireAt(at, CurStyle(), label); });
}

static IMP gOrigSetDigg;
static void hk_setDigg_int(id self, SEL _cmd, long long v) {
    BOOL old = OldDigged(self);
    ((void (*)(id, SEL, long long))gOrigSetDigg)(self, _cmd, v);
    OnDiggChanged(self, old, v != 0);
}
static void hk_setDigg_bool(id self, SEL _cmd, BOOL v) {
    BOOL old = OldDigged(self);
    ((void (*)(id, SEL, BOOL))gOrigSetDigg)(self, _cmd, v);
    OnDiggChanged(self, old, v);
}
static void hk_setDigg_obj(id self, SEL _cmd, id v) {
    BOOL old = OldDigged(self);
    ((void (*)(id, SEL, id))gOrigSetDigg)(self, _cmd, v);
    OnDiggChanged(self, old, [v respondsToSelector:@selector(boolValue)] && [v boolValue]);
}

static void InstallModelHook(void) {
    NSArray *classes = @[@"AWEAwemeModel", @"AWEAwemeStatisticsModel"];
    for (NSString *cn in classes) {
        Class cls = NSClassFromString(cn);
        if (!cls) continue;
        Method m = class_getInstanceMethod(cls, @selector(setUserDigged:));
        if (!m) continue;
        char t[32] = {0};
        method_getArgumentType(m, 2, t, sizeof(t));
        IMP rep;
        switch (t[0]) {
            case 'B': case 'c': case 'C': rep = (IMP)hk_setDigg_bool; break;
            case '@': rep = (IMP)hk_setDigg_obj; break;
            default: rep = (IMP)hk_setDigg_int; break;
        }
        gOrigSetDigg = method_setImplementation(m, rep);
        gHookInfo = [NSString stringWithFormat:@"%@ ✅", cn];
        return;
    }
    gHookInfo = @"模型钩子未找到 ❌";
}

static BOOL ClassChainContains(UIView *v, NSArray<NSString *> *keys, int depth);
// 在视频上：往上的视图和控制器里有播放互动层 / 信息流格子 / 播放器 / 作品详情
static BOOL InVideoArea(UIView *v) {
    static NSArray<NSString *> *keys;
    if (!keys) keys = @[@"PlayInteraction", @"FeedCell", @"FeedTable", @"DPlayer", @"PlayVideo", @"RichContent", @"AwemeDetail", @"TTPlayer", @"ElementStack"];
    NSUInteger steps = 0;
    for (UIResponder *r = v; r && steps < 80; r = r.nextResponder, steps++) {
        NSString *cn = NSStringFromClass(r.class);
        for (NSString *k in keys) if ([cn rangeOfString:k].location != NSNotFound) return YES;
    }
    return NO;
}
// 兼容引擎：点击后等界面刷新，再看按钮是不是变成“已赞”
static int LikeButtonHit(UIView *v, UIView **outBtn) {
    UIView *start = v;
    for (int i = 0; v && i < 6; i++, v = v.superview) {
        NSString *cn = NSStringFromClass(v.class);
        NSString *ax = v.accessibilityLabel ?: @"";
        BOOL digg = [cn rangeOfString:@"Digg" options:NSCaseInsensitiveSearch].location != NSNotFound;
        // 3.11.47：“喜欢”页签之类的不在视频上，下面 InVideoArea 挡掉
        BOOL axLike = ([ax containsString:@"赞"] || [ax containsString:@"喜欢"]) && ![ax containsString:@"评论"];
        if (digg || axLike) {
            if (!InVideoArea(start)) return 0;
            if (outBtn) *outBtn = v;
            return 1;
        }
    }
    return 0;
}
static BOOL ViewLooksLiked(UIView *v) {
    for (int i = 0; v && i < 3; i++, v = v.superview) {
        NSString *ax = v.accessibilityLabel ?: @"";
        if ([ax containsString:@"已点赞"] || [ax containsString:@"已喜欢"] || [ax containsString:@"取消点赞"]) return YES;
        if ([ax containsString:@"未点赞"]) return NO;
        if ([v isKindOfClass:UIControl.class] && ((UIControl *)v).selected) return YES;
    }
    return NO;
}

static BOOL ClassChainContains(UIView *v, NSArray<NSString *> *keys, int depth) {
    for (int i = 0; v && i < depth; i++, v = v.superview) {
        NSString *cn = NSStringFromClass(v.class);
        for (NSString *k in keys) if ([cn rangeOfString:k options:NSCaseInsensitiveSearch].location != NSNotFound) return YES;
    }
    return NO;
}

static CFTimeInterval gLastBeganTime, gLast3Time;
static CGPoint gLastBeganPt;

static void HandleEvent(UIEvent *event) {
    if (event.type != UIEventTypeTouches) return;
    NSSet<UITouch *> *all = event.allTouches;

    if (all.count == 3) {
        if (!ALBDockHostPluginActive()) return;
        BOOL allBegan = YES;
        for (UITouch *t in all) if (t.phase != UITouchPhaseBegan) { allBegan = NO; break; }
        if (allBegan) {
            CFTimeInterval now = CACurrentMediaTime();
            if (now - gLast3Time < 0.45) { ShowBall(); gLast3Time = 0; } else gLast3Time = now;
        }
        return;
    }
    if (all.count != 1) return;
    UITouch *t = all.anyObject;
    if (t.phase != UITouchPhaseBegan || !Enabled()) return;     // 3.11.33：只在按下那一刻读偏好
    if (t.window && [gOwnWindows containsObject:t.window]) return;
    UIView *v = t.view;
    CGPoint p = [t locationInView:nil];

    // 记录“按下”，抖音的双击手势会把抬起取消掉，所以用按下判断
    if (t.phase == UITouchPhaseBegan) {
        UIView *btn = nil;
        gLastTouchTime = CACurrentMediaTime();
        gLastTouchPt = p;
        gLastTouchView = v;
        gLastTouchOnLike = LikeButtonHit(v, &btn) == 1;
        if (gLastTouchOnLike && btn) gLikeCenter = [btn convertPoint:CGPointMake(CGRectGetMidX(btn.bounds), CGRectGetMidY(btn.bounds)) toView:nil];
        // 双击 / 连击位置（落在不是按钮的地方；是不是视频、是哪一条，等作品变成已赞时再认）
        if (!gLastTouchOnLike && ![v isKindOfClass:UIControl.class]) {
            static CFTimeInterval prevBegan;
            static CGPoint prevPt;
            if (t.timestamp - prevBegan < 0.35 && hypot(p.x - prevPt.x, p.y - prevPt.y) < 60) {
                gLastDoubleTime = CACurrentMediaTime();
                gLastDoublePt = p;
                prevBegan = 0;
            } else {
                prevBegan = t.timestamp;
                prevPt = p;
            }
        }

    }
}

%hook UIApplication
- (void)sendEvent:(UIEvent *)event {
    @try { HandleEvent(event); } @catch (__unused id e) {}
    %orig;
}
%end

#pragma mark - 3.11.33 和本插件的接口

// 独立版存过的值抄一次（只抄合并版还没有的键）
static void MigrateStandaloneKeys(void) {
    NSUserDefaults *d = AD();
    if ([d boolForKey:KEY("migrated")]) return;
    NSDictionary<NSString *, NSString *> *map = @{
        @"enabled": KEY("enabled"), @"trigButton": KEY("trigButton"), @"trigDouble": KEY("trigDouble"),
        @"haptic": KEY("haptic"), @"edgeGlow": KEY("edgeGlow"), @"ball": KEY("ball"), @"compat": KEY("compat"),
        @"intensity": KEY("intensity"), @"style3": KEY("style"), @"spray": KEY("spray"),
        @"ballX": KEY("ballX"), @"ballY": KEY("ballY") };
    for (NSString *old in map) {
        id v = [d objectForKey:[@"AwemeAmbience_" stringByAppendingString:old]];
        if (v && ![d objectForKey:map[old]]) [d setObject:v forKey:map[old]];
    }
    [d setBool:YES forKey:KEY("migrated")];
}

// 悬浮球跟着设置：开了才建窗口，关了收起面板、藏球
static void ApplyBallPreference(void) {
    if (BallVisible() && ALBDockHostPluginActive() && !StandalonePresent()) {
        SetupPanel();
        if (gPanelWindow.ball.hidden) gPanelWindow.ball.hidden = NO;
    } else if (gPanelWindow) {
        if (!gPanelWindow.panel.hidden) [gPanelWindow closePanel];
        gPanelWindow.ball.hidden = YES;
    }
}

// 设置页改了样式 / 喷射位置 / 时长 / 密度：放一次给你看（特效窗口在设置页上面）
static NSString *gFXOptionSnapshot;
static NSString *OptionSnapshot(void) {
    return [NSString stringWithFormat:@"%ld|%ld|%.3f|%.3f", (long)CurStyle(), (long)SprayMode(), DurationScale(), Intensity()];
}
static void OptionsChanged(void) {
    NSString *now = OptionSnapshot();
    BOOL changed = gFXOptionSnapshot && ![now isEqualToString:gFXOptionSnapshot];
    gFXOptionSnapshot = now;
    ApplyBallPreference();
    if (changed && Enabled()) { gLastFire = 0; FireAt(DefaultOrigin(), CurStyle(), @"设置页预览"); }
}

// 设置页「预览当前特效」（ALB5Settings.h ALB5OptionAction）
extern "C" void ALBFXPreview(void) {
    gLastFire = 0;
    FireAt(DefaultOrigin(), CurStyle(), @"设置页预览");
}

// 诊断（ALB5Settings.h ALB5AdaptationText）
extern "C" NSString *ALBFXReport(void) {
    NSArray *spray = @[@"随机多点", @"两侧对称", @"点赞处"];
    NSInteger st = CurStyle(), sp = SprayMode();
    return [NSString stringWithFormat:@"%@ 样式=%@ 时长×%.2f 密度×%.2f 喷射=%@ 按钮=%@ 双击=%@ 边缘光=%@ 震动=%@ 悬浮球=%@；识别引擎：%@；%@；放过 %lu 次（插件未启用时跳过 %lu、独立版在时跳过 %lu）；独立版：%@",
        Enabled() ? @"开" : @"关",
        st >= 0 && st < (NSInteger)StyleNames().count ? StyleNames()[st] : @"?",
        DurationScale(), Intensity(), sp >= 0 && sp < 3 ? spray[sp] : @"?",
        TrigButton() ? @"开" : @"关", TrigDouble() ? @"开" : @"关", EdgeGlowOn() ? @"开" : @"关",
        Haptic() ? @"开" : @"关", BallVisible() ? @"开" : @"关",
        gHookInfo, [NSString stringWithFormat:@"%@；按作品认：面板点赞登记 %lu 次；拦下 %lu 次（最近：%@）、同一下多出来的拦下 %lu 次",
            gDiag, (unsigned long)gFXArmed, (unsigned long)gFXModelIgnored, gFXIgnoredLast ?: @"-", (unsigned long)gFXModelBursts],
        (unsigned long)gFXFires, (unsigned long)gFXSkippedInactive, (unsigned long)gFXSkippedStandalone,
        StandalonePresent() ? @"在运行（请卸载，合并版才会放）" : @"没有"];
}

%ctor {
    @autoreleasepool {
        %init;
        MigrateStandaloneKeys();
        gFXOptionSnapshot = OptionSnapshot();
        InstallModelHook();
        [[NSNotificationCenter defaultCenter] addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) {
            if ([gHookInfo hasPrefix:@"模型钩子未找到"]) InstallModelHook();
            After(1.0, ^{ ApplyBallPreference(); });
        }];
        [[NSNotificationCenter defaultCenter] addObserverForName:@"ALB5.OptionsChanged" object:nil queue:NSOperationQueue.mainQueue usingBlock:^(NSNotification *n) {
            OptionsChanged();
        }];
    }
}
