//
//  ALBDock.m —— 三段式收缩底栏（3.11 由用户自己的实验工程并入，底栏逻辑原样，只改了名字、偏好和下面三处接口）
//
//    · 偏好在本插件的命名空间里（ALBDockCommon.h），设置页统一发 ALB5.OptionsChanged，这里比对出变了的键，
//      再按原来的 prefsChanged: 逻辑处理
//    · 竖直位置加上本插件的「底栏垂直位置」（和液态底栏共用，barCenterY）
//    · 头像先用本插件已经读到的本人头像（ALBDockAvatar）
//
//    只有一颗球 = 朋友键那块玻璃，全程大小不变，y 不动
//    收缩：球滑到屏幕中间（到中心变慢不停），路过中栏把它吸进去，主栏朝中心收拢成球、和球同时到达
//    展开：球带着收缩段的速度弹到另一侧，主栏从中心被拉开，中栏在球经过时从球里留下来
//    图标：原地交叉淡化，只缩放不位移
//    上滑换边 = 收缩 + 展开（速度连续）；同一侧上滑不触发；静置 → 收缩；点球 → 展开
//  驱动：CADisplayLink 时间轴逐帧计算；玻璃成形 / 溶解用系统动画；四块玻璃同在 UIGlassContainerEffect
//
#import "ALBDock.h"
#import "ALBDockCommon.h"
#import "ALBDockAvatar.h"
#import "ALBDockBridge.h"

static UIColor *ALBDockDouyinRed(void) { return ALBDockHex(0xFE2C55, 1); }

// 3.11.33：符号图（设的符号这台系统没有时退回 fallback，再不行就问号，不会是 nil）
static UIImage *ALBDockSymbolImage(NSString *name, NSString *fallback, UIImageSymbolConfiguration *cfg) {
    for (NSString *n in @[name ?: @"", fallback ?: @"", @"questionmark"]) {
        if (!n.length) { continue; }
        UIImage *img = cfg ? [UIImage systemImageNamed:n withConfiguration:cfg] : [UIImage systemImageNamed:n];
        if (img) { return img; }
    }
    return nil;
}

#pragma mark - 穿透的玻璃容器

@interface ALBDockPassEffectView : UIVisualEffectView
@end

@implementation ALBDockPassEffectView
- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    return (hit == self || hit == self.contentView) ? nil : hit;
}
@end

#pragma mark - 玻璃块（本身就是 UIVisualEffectView，才能被 GlassContainer 融合）

@interface ALBDockGlassGroup : UIVisualEffectView
@property (nonatomic, assign) CGPoint home;
@property (nonatomic, assign) BOOL glass;
@property (nonatomic, strong) UIView *tint;
@property (nonatomic, copy) NSString *materialKey;   // 3.11.45：现在装的是哪一种玻璃（变了才换 effect）
- (void)applyMaterial;
- (void)applyStyle;
- (void)rematerialize;
@end

// 3.11.45：玻璃的“种类”（透明＋深 / 浅染色、原厂同款、Regular）。深浅档只是 overrideUserInterfaceStyle，种类不变就不换 effect
static NSUInteger gALBDockEffectSwaps, gALBDockRematerials;
// 3.11.51：图标变黑的背后亮度阈值（设置删了；高于它图标变黑，低于它 −0.08 变回白）
static const double kALBDockInkLightAt = 0.85;

// 3.11.44 用户：“三段式底栏……为什么给它改坏了，现在右边两个动不动就变成了深色调”。左边那块是系统底栏自己的胶囊，
// 玻璃是 UIKit 给它的原厂那只（没开透明时不换）；右边两块（中栏 ✦|↑、朋友球）和动画用的主栏是自建的 Regular 玻璃。
// 两者深浅档（overrideUserInterfaceStyle）一直是同一个，但不是同一种玻璃：视频页背后读数一落到“深”，自建的 Regular
// 深色比原厂胶囊的深色重得多，于是只有右边两块明显变深。这里：没开透明时三块自建玻璃直接用系统胶囊原厂那只的副本
// （第一次见到系统胶囊时读出来记着），和左边是同一份玻璃、同一个深浅；开透明时照旧四块都是同一份透明（系统胶囊也换成它）。
static UIVisualEffect *gALBDockNativeGlass;          // 系统胶囊原厂那只（KVC glassEffect 读出来的副本）
static NSString *gALBDockNativeGlassInfo;
static NSUInteger gALBDockNativeGlassUses;

@implementation ALBDockGlassGroup

- (instancetype)init {
    if ((self = [super initWithEffect:nil])) {
        _tint = [[UIView alloc] init];
        _tint.userInteractionEnabled = NO;
        _tint.layer.borderWidth = 0.6;
        _tint.layer.borderColor = [UIColor colorWithWhite:1 alpha:0.12].CGColor;
        [self.contentView addSubview:_tint];
        [self applyMaterial];
    }
    return self;
}

static NSString *ALBDockGroupMaterialKey(void) {
    if (ALBDockGlassUsesClear()) {
        return [NSString stringWithFormat:@"clear|%.3f", ALB25MaterialDarkTint()];   // 3.11.51：固定深色染色
    }
    return gALBDockNativeGlass ? @"native" : @"regular";
}

// 材质跟页面走：视频页深色 + 透明开关；其他页系统 Regular、深浅色跟系统（ALBDockCommon.h）
// 3.11.45 用户：“三段式底栏另外两个不透明是切换深浅色模式导致的”。3.11.44 每次换深浅都在 0.25 秒的动画里把三块的 effect
// 整个换成新的一只，系统深浅正好同时在切的话，这两只新玻璃会停在不透明的那一帧（左边系统胶囊是 UIKit 自己的，不受影响）。
// 现在：深浅只改 overrideUserInterfaceStyle（玻璃自己过渡）；种类变了（透明↔原厂、透明的深↔浅染色）才换 effect，
// 透明之间的换法用动画，别的不带动画、先清空再装；系统深浅一换另外整只重装两次（rematerialize，ALBDockView systemStyleChanged）
- (void)applyStyle {
    UIUserInterfaceStyle st = ALBDockGlassStyle();
    if (self.overrideUserInterfaceStyle != st) { self.overrideUserInterfaceStyle = st; }
}

- (void)installEffectForKey:(NSString *)key {
    UIVisualEffect *native = ALBDockGlassUsesClear() ? nil : gALBDockNativeGlass;
    if (native) { gALBDockNativeGlassUses++; }
    self.effect = native ? [native copy] : ALBDockMakeGlass(YES);
    self.materialKey = key;
}

- (void)applyMaterial {
    self.glass = ALBDockGlassAvailable();
    [self applyStyle];
    NSString *key = ALBDockGroupMaterialKey();
    if (self.effect && [key isEqualToString:self.materialKey]) { return; }
    gALBDockEffectSwaps++;
    BOOL bothClear = [key hasPrefix:@"clear|"] && [self.materialKey hasPrefix:@"clear|"] && self.effect;
    if (bothClear) {
        [UIView animateWithDuration:0.2 delay:0 options:UIViewAnimationOptionBeginFromCurrentState | UIViewAnimationOptionAllowUserInteraction
                         animations:^{ [self installEffectForKey:key]; } completion:nil];
    } else {
        [UIView performWithoutAnimation:^{
            if (self.effect) { self.effect = nil; }
            [self installEffectForKey:key];
        }];
    }
    self.clipsToBounds = !self.glass;
    // 玻璃自己有折射和描边；模糊兜底时补一层暗色和细边
    self.tint.hidden = self.glass;
    self.tint.backgroundColor = [UIColor colorWithWhite:0.08 alpha:0.32];
}

// 整只重装（不带动画）：系统深浅切换后玻璃停在不透明那一帧时用
- (void)rematerialize {
    gALBDockRematerials++;
    [UIView performWithoutAnimation:^{
        [self applyStyle];
        self.effect = nil;
        [self installEffectForKey:ALBDockGroupMaterialKey()];
    }];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat r = CGRectGetHeight(self.bounds) / 2.0;
    self.layer.cornerRadius = r;
    self.layer.cornerCurve = kCACornerCurveContinuous;
    self.tint.frame = self.bounds;
    self.tint.layer.cornerRadius = r;
    self.tint.layer.cornerCurve = kCACornerCurveContinuous;
}

@end

#pragma mark - 图标

@interface ALBDockItem : UIControl
@property (nonatomic, strong) UIImageView *icon;
@property (nonatomic, strong) UILabel *title;
@property (nonatomic, strong) UILabel *badge;
@property (nonatomic, assign) BOOL selectedTab;
@property (nonatomic, copy) NSString *symbol;
// 2.5.0：图标 / 标题规格照抄系统底栏实测（量到之前用兜底值）
@property (nonatomic, assign) BOOL hasNativeMetrics;
@property (nonatomic, assign) CGSize iconSize;
@property (nonatomic, assign) CGFloat iconDY;      // 图标中心相对按钮中心的竖直偏移
@property (nonatomic, assign) CGFloat titleDY;     // 标题中心相对按钮中心的竖直偏移
@property (nonatomic, assign) CGFloat restScale;   // 2.6.0：静止时的缩放（= 底栏缩放比例），按压在它基础上缩
@property (nonatomic, strong) UIImageSymbolConfiguration *cfg;   // 3.11.33：当前的符号配置（换符号时照用）
// 3.11.47 角标规格照抄系统底栏实际画出来的角标（左边、顶边相对图标中心的偏移、高、字体、左右留白），
// 动画里自建的角标和展开结束后接管的系统角标一样大、在同一个位置，交接那一帧不再“跳”
@property (nonatomic, assign) BOOL hasBadgeMetrics;
@property (nonatomic, assign) CGFloat badgeLeftDX, badgeTopDY, badgeH, badgePad;
@property (nonatomic, strong) UIFont *badgeFont;
@property (nonatomic, strong) UIColor *badgeColor;
- (void)applyBadgeLeft:(CGFloat)dx top:(CGFloat)dy height:(CGFloat)h pad:(CGFloat)pad font:(UIFont *)font color:(UIColor *)color;
- (void)applySymbolConfig:(UIImageSymbolConfiguration *)cfg iconSize:(CGSize)size iconDY:(CGFloat)idy
                titleFont:(UIFont *)font titleDY:(CGFloat)tdy;
- (void)reloadSymbol:(NSString *)symbol;
- (void)applyTint;
@end

@implementation ALBDockItem

- (instancetype)initWithSymbol:(NSString *)symbol title:(NSString *)title pointSize:(CGFloat)pt {
    if ((self = [super initWithFrame:CGRectZero])) {
        _symbol = [symbol copy];
        UIImageSymbolConfiguration *cfg = [UIImageSymbolConfiguration configurationWithPointSize:pt weight:UIImageSymbolWeightSemibold];
        _cfg = cfg;
        _icon = [[UIImageView alloc] initWithImage:ALBDockSymbolImage(symbol, nil, cfg)];
        _icon.contentMode = UIViewContentModeCenter;
        _icon.tintColor = ALBDockInk(0.92);
        _icon.userInteractionEnabled = NO;
        [self addSubview:_icon];

        if (title.length) {
            _title = [[UILabel alloc] init];
            _title.text = title;
            _title.font = [UIFont systemFontOfSize:10 weight:UIFontWeightSemibold];
            _title.textColor = ALBDockInk(0.92);
            _title.textAlignment = NSTextAlignmentCenter;
            _title.userInteractionEnabled = NO;
            [self addSubview:_title];
        }

        [self addTarget:self action:@selector(down) forControlEvents:UIControlEventTouchDown | UIControlEventTouchDragEnter];
        [self addTarget:self action:@selector(up) forControlEvents:UIControlEventTouchUpInside | UIControlEventTouchUpOutside | UIControlEventTouchCancel | UIControlEventTouchDragExit];
    }
    return self;
}

- (void)setBadgeText:(NSString *)text {
    if (!self.badge) {
        self.badge = [[UILabel alloc] init];
        self.badge.font = self.badgeFont ?: [UIFont systemFontOfSize:9 weight:UIFontWeightBold];
        self.badge.textColor = [UIColor whiteColor];
        self.badge.textAlignment = NSTextAlignmentCenter;
        self.badge.backgroundColor = self.badgeColor ?: ALBDockDouyinRed();
        self.badge.clipsToBounds = YES;
        self.badge.userInteractionEnabled = NO;
        [self addSubview:self.badge];
    }
    self.badge.text = text;
    [self setNeedsLayout];
}

- (void)applyBadgeLeft:(CGFloat)dx top:(CGFloat)dy height:(CGFloat)h pad:(CGFloat)pad font:(UIFont *)font color:(UIColor *)color {
    if (h < 6 || !font) { return; }
    BOOL same = self.hasBadgeMetrics && fabs(self.badgeLeftDX - dx) < 0.25 && fabs(self.badgeTopDY - dy) < 0.25 &&
                fabs(self.badgeH - h) < 0.25 && fabs(self.badgePad - pad) < 0.25 && [self.badgeFont isEqual:font];
    self.badgeColor = color;
    if (color && self.badge && ![self.badge.backgroundColor isEqual:color]) { self.badge.backgroundColor = color; }
    if (same) { return; }
    self.hasBadgeMetrics = YES;
    self.badgeLeftDX = dx; self.badgeTopDY = dy; self.badgeH = h; self.badgePad = MAX(pad, 4); self.badgeFont = font;
    if (self.badge) { self.badge.font = font; }
    [self setNeedsLayout];
}

- (void)applySymbolConfig:(UIImageSymbolConfiguration *)cfg iconSize:(CGSize)size iconDY:(CGFloat)idy
                titleFont:(UIFont *)font titleDY:(CGFloat)tdy {
    if (cfg) { self.cfg = cfg; self.icon.image = ALBDockSymbolImage(self.symbol, nil, cfg); }
    if (font && self.title) { self.title.font = font; }
    self.iconSize = size;
    self.iconDY = idy;
    self.titleDY = tdy;
    self.hasNativeMetrics = YES;
    [self setNeedsLayout];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGFloat w = CGRectGetWidth(self.bounds), h = CGRectGetHeight(self.bounds);
    if (self.hasNativeMetrics) {
        CGSize s = self.iconSize;
        if (s.width < 1 || s.height < 1) { s = self.icon.image.size; }
        CGFloat idy = self.title ? self.iconDY : 0;
        self.icon.frame = CGRectMake((w - s.width) / 2.0, h / 2.0 + idy - s.height / 2.0, s.width, s.height);
        if (self.title) {
            CGFloat lh = ceil(self.title.font.lineHeight);
            self.title.frame = CGRectMake(0, h / 2.0 + self.titleDY - lh / 2.0, w, lh);
        }
    } else if (self.title) {
        self.icon.frame = CGRectMake(0, h * 0.5 - 17, w, 22);
        self.title.frame = CGRectMake(0, h * 0.5 + 5, w, 12);
    } else {
        self.icon.frame = self.bounds;
    }
    if (self.badge) {
        if (self.hasBadgeMetrics) {
            // 3.11.47：照系统角标（左边、顶边相对图标中心），字多了往右长，同系统
            NSString *text = self.badge.text ?: @"";
            CGFloat tw = text.length ? ceil([text sizeWithAttributes:@{NSFontAttributeName: self.badgeFont}].width) : 0;
            CGFloat bh = self.badgeH, bw = text.length ? MAX(bh, tw + self.badgePad) : bh;
            CGPoint ic = CGPointMake(CGRectGetMidX(self.icon.frame), CGRectGetMidY(self.icon.frame));
            self.badge.frame = CGRectMake(ic.x + self.badgeLeftDX, ic.y + self.badgeTopDY, bw, bh);
            self.badge.layer.cornerRadius = bh / 2.0;
        } else {
            CGSize s = [self.badge.text sizeWithAttributes:@{NSFontAttributeName: self.badge.font}];
            CGFloat bw = MAX(15, s.width + 8);
            self.badge.frame = CGRectMake(w * 0.5 + 3, CGRectGetMinY(self.icon.frame) - 4, bw, 14);
            self.badge.layer.cornerRadius = 7;
        }
    }
}

- (void)setSelectedTab:(BOOL)on {
    _selectedTab = on;
    [self applyTint];
}

// 3.11.33：选中＝液态底栏的强调色（关掉「dock.accent」＝抖音红），未选中＝随玻璃深浅的黑 / 白
- (void)applyTint {
    UIColor *c = self.selectedTab ? ALBDockSelectedTint() : ALBDockInk(0.92);
    self.icon.tintColor = c;
    self.title.textColor = c;
}

// 3.11.33：换成液态底栏设的符号（沿用当前的符号配置，尺寸由下一次量系统底栏重新给）
- (void)reloadSymbol:(NSString *)symbol {
    if (!symbol.length || [symbol isEqualToString:self.symbol]) { return; }
    self.symbol = symbol;
    self.icon.image = ALBDockSymbolImage(symbol, nil, self.cfg);
    if (self.hasNativeMetrics && self.icon.contentMode != UIViewContentModeCenter) { self.iconSize = CGSizeZero; }
    [self setNeedsLayout];
}

- (CGFloat)baseScale { return self.restScale > 0.01 ? self.restScale : 1.0; }

- (void)down {
    CGFloat k = [self baseScale] * kALBDockPressScale;
    [UIView animateWithDuration:kALBDockPressDownDuration delay:0
                        options:UIViewAnimationOptionAllowUserInteraction | UIViewAnimationOptionBeginFromCurrentState
                     animations:^{ self.transform = CGAffineTransformMakeScale(k, k); } completion:nil];
}

- (void)up {
    [UIView animateWithDuration:kALBDockPressUpDuration delay:0 usingSpringWithDamping:kALBDockPressDamping initialSpringVelocity:kALBDockPressVelocity
                        options:UIViewAnimationOptionAllowUserInteraction | UIViewAnimationOptionBeginFromCurrentState
                     animations:^{ self.transform = CGAffineTransformMakeScale([self baseScale], [self baseScale]); } completion:nil];
}

@end

#pragma mark - 「吸附」动效：曲线
//
//  设计：三个组件全部挂在「球」上 —— 球走到哪，谁就被吸进去 / 从球里放出来。
//
//  球（朋友键 ↔ 头像）—— 2.2.0 改成「一条连续的速度曲线」，不再是收缩段 + 展开段两段拼接
//    （两段拼接在中心处速度先降到底、再被弹簧猛拉起来，速度曲线是个尖角，眼睛看就是一顿）
//    速度 v(u) = (u + 0.12)·(1 − u)² × (1 − 0.68·exp(−((u − uc)/0.10)²))
//      前一项：起步就有 ~760pt/s（手一松立刻动）、软着陆；后一项：在球经过屏幕中心的时刻 uc 柔和地降到 ~34%
//    换边 0.56s（98% 位移在 0.47s 内完成）；静置收缩 0.36s（停在中心）；从中心展开 0.42s
//    图标：原地交叉淡化，只缩放不位移；展开时按球的位置在最后 30% 行程换回朋友
//  中栏（✦|↑）：进度由球的位置驱动，被路过的球吸进去 / 球经过时从球里「滴」回原位
//  主栏（四 tab）：进度与球完全同步 —— 外侧边缘与球镜像地从两头向中心合拢 / 张开（行程都是 175pt），
//                 tab 被边缘从外往内依次吃掉、展开时从内往外出现；y 全程不变
//
//  驱动：整段动画开始时一次性算好（每 1/60s 一帧），交给 UIView 关键帧动画，由系统渲染进程独立播放，
//  主线程忙（抖音翻页加载视频）也不会卡住它；玻璃只改位置 / 尺寸，由 UIGlassContainerEffect 自己融合。
//

static CGFloat ALBDockClamp01(CGFloat v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static CGFloat ALBDockSmooth(CGFloat v) { v = ALBDockClamp01(v); return v * v * (3 - 2 * v); }
static CGFloat ALBDockLerp(CGFloat a, CGFloat b, CGFloat k) { return a + (b - a) * k; }

static const double kSwapT     = 0.56;   // 换边整段
static const double kCollapseT = 0.36;   // 静置收缩
static const double kExpandT   = 0.42;   // 从中心展开

#define ALBDOCK_PROF_N 600
static double gProfSwap[ALBDOCK_PROF_N + 1];    // 带中心放慢
static double gProfPlain[ALBDOCK_PROF_N + 1];   // 不带
static double gSwapHalfU = 0.33;            // 换边曲线经过中心（P = 0.5）的归一化时刻

static void ALBDockIntegrate(const double *v, double *P) {
    P[0] = 0;
    for (int i = 1; i <= ALBDOCK_PROF_N; i++) { P[i] = P[i - 1] + (v[i] + v[i - 1]) * 0.5; }
    double tot = P[ALBDOCK_PROF_N];
    for (int i = 0; i <= ALBDOCK_PROF_N; i++) { P[i] /= tot; }
}

static double ALBDockProfileAt(const double *P, double u) {
    if (u <= 0) { return 0; }
    if (u >= 1) { return 1; }
    double f = u * ALBDOCK_PROF_N; int i = (int)f; double r = f - i;
    return P[i] + (P[i + 1] - P[i]) * r;
}

static double ALBDockProfileInverse(const double *P, double target) {
    for (int i = 1; i <= ALBDOCK_PROF_N; i++) {
        if (P[i] >= target) {
            double r = (target - P[i - 1]) / MAX(1e-9, P[i] - P[i - 1]);
            return (i - 1 + r) / (double)ALBDOCK_PROF_N;
        }
    }
    return 1;
}

static void ALBDockBuildProfiles(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        const double a = 0.12, k = 0.68, sig = 0.10;
        static double B[ALBDOCK_PROF_N + 1], v[ALBDOCK_PROF_N + 1];
        for (int i = 0; i <= ALBDOCK_PROF_N; i++) {
            double u = i / (double)ALBDOCK_PROF_N;
            B[i] = (u + a) * (1 - u) * (1 - u);
        }
        ALBDockIntegrate(B, gProfPlain);
        // 放慢的中心要正好落在「球经过屏幕中心」那一刻：迭代把 uc 对准 P = 0.5
        double uc = ALBDockProfileInverse(gProfPlain, 0.5);
        for (int it = 0; it < 40; it++) {
            for (int i = 0; i <= ALBDOCK_PROF_N; i++) {
                double u = i / (double)ALBDOCK_PROF_N, d = (u - uc) / sig;
                v[i] = B[i] * (1 - k * exp(-d * d));
            }
            ALBDockIntegrate(v, gProfSwap);
            uc += (0.5 - ALBDockProfileAt(gProfSwap, uc)) * 0.6;
        }
        gSwapHalfU = ALBDockProfileInverse(gProfSwap, 0.5);
    });
}

// 某一侧布局的几何（纯计算，不碰视图）
typedef struct {
    CGRect main, mid, single;
    CGFloat tabX[ALBDockTypeCount];     // 各 tab 中心绝对 x（按项目下标：首页 消息 拍摄 我 朋友；不在主栏的不用）
    CGFloat rank[ALBDockTypeCount];     // 离屏幕中心由远到近的名次（0 = 最外侧，只排主栏里的）
} ALBDockGeo;

#pragma mark - 系统 UITabBar（四项 + 原生透镜）

// 不让 UITabBar 按安全区改布局：它挂在覆盖层里，位置由我们按实测胶囊手动对齐
@interface ALBDockNativeTabBar : UITabBar
@end
static UIView *ALBDockFindPlatter(UIView *bar);

@implementation ALBDockNativeTabBar
- (UIEdgeInsets)safeAreaInsets { return UIEdgeInsetsZero; }
// 2.6.0：UITabBar 比胶囊宽（两侧各约 21pt 透明边），它压在中栏上面，间距调小后会挡住 ✦ / ↑ 的点击 —— 只认胶囊范围
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *platter = ALBDockFindPlatter(self);
    if (!platter) { return [super pointInside:point withEvent:event]; }
    CGRect r = [platter convertRect:platter.bounds toView:self];
    return CGRectContainsPoint(CGRectInset(r, 0, -6), point);
}
@end

// 抖音自己底栏的标题：和抖音底栏对话（选中页、角标、转发点击）只认这一套
// 3.11.45：五项都是“项目”（下标＝ALBDockType：首页 消息 拍摄 我 朋友），哪一项做单独键、哪几项进主栏、主栏顺序都可以设
static NSArray<NSString *> *ALBDockTabTitles(void) { return @[@"首页", @"消息", @"拍摄", @"我", @"朋友"]; }
// 3.11.33：显示出来的标题 / 符号＝液态底栏里同一功能那一项（AwemeAlbumMorphBar.swift tabForItemKey：
// 首页 / 消息 / 发布 / 我 / 朋友，符号是「图标」页给 newbar.* 设的，没设就是液态底栏的默认符号）
static NSArray<NSString *> *ALBDockDisplayTitles(void) { return @[@"首页", @"消息", @"发布", @"我", @"朋友"]; }
static NSArray<NSString *> *ALBDockTabFallbacks(void) {
    return @[@"photo.on.rectangle.angled", @"message", @"plus", @"person.crop.circle", @"hands.and.sparkles"];
}
static NSArray<NSString *> *ALBDockTabSymbols(void) {
    NSArray<NSString *> *keys = @[@"newbar.home.normal", @"newbar.messages.normal", @"newbar.publish.normal",
                                  @"newbar.profile.normal", @"newbar.friends.normal"];
    NSArray<NSString *> *fallbacks = ALBDockTabFallbacks();
    NSMutableArray<NSString *> *symbols = [NSMutableArray arrayWithCapacity:ALBDockTypeCount];
    for (NSUInteger i = 0; i < ALBDockTypeCount; i++) {
        NSString *s = ALBDockHostConfiguredSymbol(keys[i], fallbacks[i]);
        [symbols addObject:s.length ? s : fallbacks[i]];
    }
    return symbols;
}

#pragma mark - 3.11.33 背后亮度探针
//
//  液态底栏是系统自己的标签栏：系统量它背后多亮，亮了玻璃变浅、图标变黑。三段式底栏这几块玻璃系统不管
//  （3.11.31 实测登记到边上也没有口袋），所以照系统的办法自己量：底栏最底下垫一层不加滤镜、几乎透明的
//  背景层（CABackdropLayer，系统按 0.25 秒一次报它背后画面的亮度——和右侧口袋、双击面板探针是同一种读数，
//  不截图、不在主线程画任何东西）。读数交给底栏（ALBDockView -backdropLuma:）按「变浅阈值」定深浅。
//
@interface ALBDockLumaProbe : UIView
@property (nonatomic, copy) void (^onLuma)(double luma);
@end

@implementation ALBDockLumaProbe
+ (Class)layerClass {
    Class backdrop = NSClassFromString(@"CABackdropLayer");
    return backdrop ?: [CALayer class];
}
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.userInteractionEnabled = NO;
        self.isAccessibilityElement = NO;
        self.accessibilityElementsHidden = YES;
        CALayer *layer = self.layer;
        @try {
            if ([layer respondsToSelector:NSSelectorFromString(@"setTracksLuma:")]) { [layer setValue:@YES forKey:@"tracksLuma"]; }
            if ([layer respondsToSelector:NSSelectorFromString(@"setLumaUpdateRate:")]) { [layer setValue:@0.2 forKey:@"lumaUpdateRate"]; }   // 3.11.49：0.15 → 0.2（读数是系统渲染进程给的，开销很小；变色本身改成渐变）
            if ([layer respondsToSelector:NSSelectorFromString(@"setScale:")]) { [layer setValue:@0.25 forKey:@"scale"]; }
        } @catch (__unused NSException *e) {}
        layer.opacity = 0.02f;
    }
    return self;
}
- (void)backdropLayer:(CALayer *)layer didChangeLuma:(double)luma {
    if (!isfinite(luma)) { return; }
    if (![NSThread isMainThread]) {
        __weak typeof(self) weakSelf = self;
        dispatch_async(dispatch_get_main_queue(), ^{ void (^cb)(double) = weakSelf.onLuma; if (cb) { cb(luma); } });
        return;
    }
    if (self.onLuma) { self.onLuma(luma); }
}
@end

static UIView *ALBDockFindSubview(UIView *root, BOOL (^match)(UIView *v)) {
    for (UIView *sub in root.subviews) {
        if (match(sub)) { return sub; }
        UIView *f = ALBDockFindSubview(sub, match);
        if (f) { return f; }
    }
    return nil;
}

#pragma mark - 底栏

@interface ALBDockView () <UITabBarDelegate>
@property (nonatomic, assign, readwrite) ALBDockSide side;
@property (nonatomic, assign, readwrite) BOOL collapsed;
@property (nonatomic, assign, readwrite) BOOL transitioning;

@property (nonatomic, strong) UIView *stack;
@property (nonatomic, strong) UIVisualEffectView *container;
@property (nonatomic, strong) ALBDockGlassGroup *mainGroup;
@property (nonatomic, strong) ALBDockGlassGroup *midGroup;
@property (nonatomic, strong) ALBDockGlassGroup *singleGroup;
@property (nonatomic, strong) UIView *selPill;
@property (nonatomic, strong) UIView *divider;
@property (nonatomic, strong) NSArray<ALBDockItem *> *tabs;     // 首页 消息 拍摄 我 朋友（下标＝项目，身份固定；不在主栏的藏起来）
@property (nonatomic, strong) NSArray<ALBDockItem *> *midItems;
// 3.11.45：主栏里的项（单独键在右时从左到右）、单独键是哪一项、中间胶囊开不开、↑ 是面板还是直接跳、单独键显示头像
@property (nonatomic, copy) NSArray<NSNumber *> *mainTypes;
@property (nonatomic, assign) NSInteger singleType;
@property (nonatomic, assign) BOOL midOn;
@property (nonatomic, assign) NSInteger arrowKind;
@property (nonatomic, assign) BOOL avatarSingle;
@property (nonatomic, assign) CGFloat avatarInset;
@property (nonatomic, assign) NSUInteger styleGen;
@property (nonatomic, strong) ALBDockItem *singleItem;
@property (nonatomic, assign) NSInteger selectedIndex;
@property (nonatomic, strong) UIImageView *avatarView;
@property (nonatomic, strong) UIView *redDot;
@property (nonatomic, strong) NSTimer *idleTimer;

// 系统底栏（iOS 26+）：静止时显示它（原生可按压、可拖动的选中透镜 + 原生角标），
// 收缩 / 展开动画期间换成可融合的自建玻璃，动画结束再换回来
@property (nonatomic, strong) ALBDockNativeTabBar *nativeBar;
@property (nonatomic, assign) NSInteger nativeOrderSide;   // 当前 items 是按哪一侧排的（-1 = 未建）
@property (nonatomic, assign) CGRect nativeTarget;         // 上次对齐的目标胶囊（变了才重排）
@property (nonatomic, assign) CGFloat nativeInset;         // UITabBar 左右到胶囊的内缩（实测）
@property (nonatomic, assign) CGFloat nativePlatterH;      // 胶囊原始高（实测，通常 62）
@property (nonatomic, strong) NSMutableDictionary<NSNumber *, NSArray<NSNumber *> *> *measuredTabX; // side → 各 tab 实测中心 x
@property (nonatomic, copy) NSString *nativeSelectedTitle;
// 2.5.0：系统底栏实测规格（自建图标照抄，交接前后尺寸一致）
@property (nonatomic, assign) BOOL nativeMetricsReady;
@property (nonatomic, assign) CGSize nativeLensSize;       // 选中透镜大小（自建选中块照抄）
@property (nonatomic, assign) BOOL materialPending;        // 动画期间页面变了，结束后再换材质
@property (nonatomic, assign) BOOL nativeShown;            // 2.6.0：系统底栏显示 / 挪出屏幕（不再用 alpha）
// 当前布局的几何（placeGroupsForSide 记下，动画以它为准）
@property (nonatomic, assign) CGRect mainHome;
@property (nonatomic, assign) CGRect midHome;
@property (nonatomic, assign) CGFloat tabW;
@property (nonatomic, strong) NSArray<NSNumber *> *tabHomeX;    // 各 tab 中心的绝对 x（按 tabs 下标）
@property (nonatomic, strong) NSArray<NSNumber *> *tabOuterRank; // 离屏幕中心由远到近的名次（0 = 最外侧）

// 动画参数（开始时算好）
@property (nonatomic, assign) CGFloat x0;
@property (nonatomic, assign) CGFloat ex0;
@property (nonatomic, assign) CGFloat ex1;
@property (nonatomic, assign) CGFloat startV;
@property (nonatomic, assign) double planT;        // 整段时长
@property (nonatomic, assign) double planTc;       // 球到屏幕中心的时刻（之前是收缩视觉，之后是展开视觉）
@property (nonatomic, assign) CGFloat planXs;      // 球起点
@property (nonatomic, assign) CGFloat planXe;      // 球终点
@property (nonatomic, assign) BOOL planDip;        // 用带中心放慢的曲线
@property (nonatomic, assign) CGFloat xc;
@property (nonatomic, assign) double collapseT;
@property (nonatomic, assign) CGFloat centerV;     // 收缩段到中心时的速度（pt/s，带方向）
@property (nonatomic, assign) double v0n;          // 展开段归一化初速度
@property (nonatomic, assign) double cachedSpeed;   // 动画期间不读 NSUserDefaults
@property (nonatomic, assign) BOOL cachedRedDot;
// 3.11.47 红点绑到真的未读：收进球里的项（主栏几项＋单独键）只要有一个有角标（数字或小红点）才亮
@property (nonatomic, assign) BOOL hasUnread;
@property (nonatomic, copy) NSString *unreadSource;
@property (nonatomic, assign) BOOL badgeMeasurePending;
@property (nonatomic, assign) BOOL cachedReverse;
@property (nonatomic, assign) BOOL planRev;

@property (nonatomic, copy) NSDictionary<NSString *, id> *prefSnapshot;   // 3.11：上一次的偏好（比对用）
@property (nonatomic, assign) BOOL platterRecheck;                         // 3.11.1：换深浅后再核对一次系统胶囊
// 3.11.33：液态底栏的符号（动画期间换了就等结束再重建系统底栏的项）、背后亮度
@property (nonatomic, assign) BOOL itemsPending;
@property (nonatomic, strong) ALBDockLumaProbe *lumaProbe;
@property (nonatomic, assign) NSInteger lumaState;        // -1 还没定 / 0 深 / 1 浅
@property (nonatomic, assign) double lumaLast;
@property (nonatomic, assign) CFTimeInterval lumaAt;
@property (nonatomic, assign) NSUInteger lumaReads;
@property (nonatomic, assign) NSUInteger lumaFlips;
@property (nonatomic, assign) NSUInteger lumaCheckGen;
@end

@implementation ALBDockView

- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.backgroundColor = [UIColor clearColor];
        self.overrideUserInterfaceStyle = ALBDockGlassStyle();
        _idleDelay = ALBDockPrefDouble(kALBDockPrefIdle);
        _side = ALBDockSideRight;

        UIVisualEffect *ce = ALBDockMakeGlassContainer(4.0);
        if (ce) {
            _container = [[ALBDockPassEffectView alloc] initWithEffect:ce];
            _container.frame = self.bounds;
            _container.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
            [self addSubview:_container];
            _stack = _container.contentView;
        } else {
            _stack = self;
        }

        _mainGroup = [[ALBDockGlassGroup alloc] init];
        _midGroup = [[ALBDockGlassGroup alloc] init];
        _singleGroup = [[ALBDockGlassGroup alloc] init];
        for (UIView *g in @[_mainGroup, _midGroup, _singleGroup]) { [_stack addSubview:g]; }

        _selPill = [[UIView alloc] init];
        _selPill.userInteractionEnabled = NO;
        _selPill.backgroundColor = ALBDockDynamic([UIColor colorWithWhite:1 alpha:0.14], [UIColor colorWithWhite:0 alpha:0.07]);
        _selPill.layer.cornerCurve = kCACornerCurveContinuous;
        [_mainGroup.contentView addSubview:_selPill];

        // 3.11.33：符号、标题用液态底栏同一功能那一项的（ALBDockTabSymbols / ALBDockDisplayTitles）
        // 3.11.45：五项（含朋友）都建出来，按设置决定哪几项进主栏、哪一项做单独键（loadTypes）
        [self loadTypes];
        NSArray<NSString *> *symbols = ALBDockTabSymbols();
        NSArray<NSString *> *titles = ALBDockDisplayTitles();
        NSMutableArray *tabs = [NSMutableArray array];
        for (NSUInteger i = 0; i < ALBDockTypeCount; i++) {
            ALBDockItem *it = [[ALBDockItem alloc] initWithSymbol:symbols[i] title:titles[i] pointSize:16];
            [it addTarget:self action:@selector(tabTapped:) forControlEvents:UIControlEventTouchUpInside];
            [_mainGroup.contentView addSubview:it];
            [tabs addObject:it];
        }
        _tabs = tabs;
        [_tabs[1] setBadgeText:@"99+"];
        _selectedIndex = 0;
        _tabs[0].selectedTab = YES;

        // 3.11.45：✦ 打开双击面板；↑ 按设置是菜单（整块胶囊展开）或者直接跳到某一页
        ALBDockItem *spark = [[ALBDockItem alloc] initWithSymbol:@"sparkles" title:nil pointSize:17];
        ALBDockItem *upload = [[ALBDockItem alloc] initWithSymbol:@"arrow.up.circle.fill" title:nil pointSize:21];
        _midItems = @[spark, upload];
        [spark addTarget:self action:@selector(sparkTapped) forControlEvents:UIControlEventTouchUpInside];
        [upload addTarget:self action:@selector(arrowTapped) forControlEvents:UIControlEventTouchUpInside];
        for (ALBDockItem *it in _midItems) { [_midGroup.contentView addSubview:it]; }
        [self applyArrowMode];
        _divider = [[UIView alloc] init];
        _divider.userInteractionEnabled = NO;
        _divider.backgroundColor = ALBDockInk(0.22);
        [_midGroup.contentView addSubview:_divider];

        _avatarView = [[UIImageView alloc] init];
        _avatarView.userInteractionEnabled = NO;
        _avatarView.clipsToBounds = YES;
        _avatarView.alpha = 0.0;
        _avatarView.tintColor = ALBDockInk(0.9);
        [_singleGroup.contentView addSubview:_avatarView];

        _singleItem = [[ALBDockItem alloc] initWithSymbol:symbols[_singleType] title:nil pointSize:15];
        [_singleItem addTarget:self action:@selector(singleTapped) forControlEvents:UIControlEventTouchUpInside];
        [_singleGroup.contentView addSubview:_singleItem];
        // 收缩后朋友图标是透明的（不参与点击），点球本身展开
        UITapGestureRecognizer *ballTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(ballTapped)];
        ballTap.cancelsTouchesInView = NO;
        [_singleGroup addGestureRecognizer:ballTap];

        _redDot = [[UIView alloc] init];
        _redDot.userInteractionEnabled = NO;
        _redDot.backgroundColor = ALBDockDouyinRed();
        _redDot.alpha = 0.0;
        [_singleGroup.contentView addSubview:_redDot];

        _nativeOrderSide = -1;
        _measuredTabX = [NSMutableDictionary dictionary];
        if (ALBDockGlassAvailable()) {
            _nativeBar = [[ALBDockNativeTabBar alloc] initWithFrame:CGRectMake(0, 0, 360, 83)];
            _nativeBar.delegate = self;
            _nativeBar.overrideUserInterfaceStyle = ALBDockGlassStyle();
            _nativeBar.tintColor = ALBDockSelectedTint();      // 3.11.33：同液态底栏的强调色（可关）
            _nativeBar.unselectedItemTintColor = ALBDockInk(0.92);
            // 2.6.0：放进同一个玻璃融合容器（三块自建玻璃之上），系统胶囊按压拉伸靠近中栏时才有机会融合。
            // 显示 / 隐藏改成挪出屏幕（setNativeShown:），容器里不切 alpha / hidden
            [_stack addSubview:_nativeBar];   // 3.11.51：按压融合固定开
            _nativeShown = YES;
            [self setNativeShown:NO];
        }

        // 3.11.33：背后亮度探针垫在最底下（三块玻璃、系统底栏都在它上面，它只看底栏背后的画面）
        _lumaState = -1;
        _lumaLast = NAN;
        if (NSClassFromString(@"CABackdropLayer")) {
            _lumaProbe = [[ALBDockLumaProbe alloc] initWithFrame:CGRectZero];
            __weak typeof(self) weakSelf = self;
            _lumaProbe.onLuma = ^(double luma) { [weakSelf backdropLuma:luma]; };
            [self insertSubview:_lumaProbe atIndex:0];
        }

        [self reloadAvatar];
        [self applyInk];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(iconsChanged)
                                                     name:ALBDockIconsChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(syncNative)
                                                     name:ALBDockBridgeChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(reloadAvatar)
                                                     name:ALBDockAvatarChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(prefsChanged:)
                                                     name:ALBDockPrefsChangedNotification object:nil];
        // 3.11：设置页（ALB5Settings.h）改完统一发 ALB5.OptionsChanged，比对出变了的键再逐个处理
        _prefSnapshot = [self currentPrefs];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(optionsChanged)
                                                     name:@"ALB5.OptionsChanged" object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(verticalOffsetChanged)
                                                     name:@"ALBDock.VerticalOffsetChanged" object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(materialChanged)
                                                     name:ALBDockMaterialChangedNotification object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(systemStyleChanged)
                                                     name:ALBDockSystemStyleChangedNotification object:nil];
    }
    return self;
}

#pragma mark 3.11.45 项目：顺序、隐藏、单独键、中间胶囊、↑、头像

- (void)loadTypes {
    self.mainTypes = ALBDockMainTypes();
    self.singleType = ALBDockSingleType();
    self.midOn = ALBDockMidShown();
    self.arrowKind = MIN(MAX([ALBDockPrefValue(kALBDockPrefArrow) integerValue], 0), 4);
    self.avatarSingle = ALBDockPrefBool(kALBDockPrefSingleAvatar);
    self.avatarInset = round(MIN(MAX(ALBDockPrefDouble(kALBDockPrefAvatarInset), 0), 14));
}

// 顺序 / 隐藏 / 单独键 / 中间胶囊变了：主栏重排、系统底栏的项重建、单独键换图标
- (void)typesChanged {
    NSArray<NSNumber *> *wasMain = self.mainTypes;
    NSInteger wasSingle = self.singleType;
    [self loadTypes];
    [self applyArrowMode];
    if (self.singleType != wasSingle) {
        [self.singleItem reloadSymbol:ALBDockTabSymbols()[self.singleType]];
        self.singleItem.selectedTab = NO;
    }
    if (![wasMain isEqualToArray:self.mainTypes] || self.singleType != wasSingle) {
        self.nativeOrderSide = -1;
        self.nativeSelectedTitle = nil;
        if (self.selectedIndex >= 0 && ![self.mainTypes containsObject:@(self.selectedIndex)]) {
            self.tabs[self.selectedIndex].selectedTab = NO;
            self.selectedIndex = -1;
        }
    }
    self.nativeTarget = CGRectZero;
    [self.measuredTabX removeAllObjects];
    if (self.transitioning) { self.itemsPending = YES; return; }
    if (self.nativeBar) { [self rebuildNativeItemsForSide:self.side]; }
    [self setNeedsLayout];
    [self layoutIfNeeded];
    [self syncNative];
    if (self.collapsed) { [self expandToSide:self.side]; } else { [self noteInteraction]; }
}

// 第 type 项的功能（拍摄走抖音的拍摄回调，其他切页）
- (void)performType:(NSInteger)type {
    if (type < 0 || type >= ALBDockTypeCount) { return; }
    if (type == ALBDockTypePlus) { ALBDockBridgeTapPlus(); return; }
    ALBDockBridgeTapTitle(ALBDockTabTitles()[type]);
}

// ↑：0＝打开快捷跳转面板（和 ✦ 的面板同一种），其他＝直接跳到那一页
- (void)applyArrowMode {
    ALBDockItem *arrow = self.midItems.lastObject;
    if (!arrow) { return; }
    BOOL panel = self.arrowKind == 0;
    NSString *symbol = panel ? @"arrow.up.circle.fill" : ALBDockHostQuickJumpSymbol(self.arrowKind);
    if (![arrow.symbol isEqualToString:symbol]) { [arrow reloadSymbol:symbol]; }
    arrow.accessibilityLabel = panel ? @"快捷跳转" : ALBDockHostQuickJumpTitle(self.arrowKind);
    self.midItems.firstObject.accessibilityLabel = @"悬浮面板";
}

- (void)sparkTapped {
    if (self.transitioning) { return; }
    [self noteInteraction];
    ALBDockHostOpenPalette(self.midItems.firstObject);
}

- (void)arrowTapped {
    if (self.transitioning) { return; }
    [self noteInteraction];
    if (self.arrowKind != 0) { ALBDockHostQuickJump(self.arrowKind); return; }
    ALBDockHostOpenQuickJumpPanel(self.midItems.lastObject);
}

// 3.11.45：系统深浅一换，三块自建玻璃整只重装两次（0.05 秒、0.45 秒后；动画中就等结束）
- (void)systemStyleChanged {
    NSUInteger gen = ++self.styleGen;
    ALBDockMaterialLog(@"系统深浅切换：0.05 / 0.45 秒后三块玻璃整只重装");
    __weak typeof(self) weakSelf = self;
    for (NSNumber *delay in @[@0.05, @0.45]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay.doubleValue * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            ALBDockView *ss = weakSelf;
            if (!ss || gen != ss.styleGen) { return; }
            if (ss.transitioning) { ss.materialPending = YES; return; }
            for (ALBDockGlassGroup *g in @[ss.mainGroup, ss.midGroup, ss.singleGroup]) { [g rematerialize]; }
            [ss applyInk];
            ss.platterRecheck = YES;
            [ss applyNativePlatterGlass];
        });
    }
}

// 3.11.45 用户：“三段式底栏的图标颜色切换比较迟钝，不能及时刷新”。系统底栏的未选中色原来是动态色，UITabBar 要等它自己
// 下一次重排才按新深浅重新解析；现在每次换深浅直接写按现在深浅解析好的颜色
// 3.11.49 用户：“它的变化是突然的，你能不能增加一个逐渐变化的过程？”。图标、标题、分隔线、头像、系统底栏的按钮在换颜色前
// 各挂一个 0.35 秒的淡入淡出（CATransition，只作用在这些小视图上，不碰玻璃），换完颜色就是渐变过去的
static void ALBDockFadeLayer(CALayer *layer) {
    if (!layer) { return; }
    CATransition *fade = [CATransition animation];
    fade.type = kCATransitionFade;
    fade.duration = 0.35;
    fade.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseInEaseOut];
    [layer addAnimation:fade forKey:@"alb.inkFade"];
}
- (void)applyInkFaded {
    if (self.window && !UIAccessibilityIsReduceMotionEnabled()) {
        for (ALBDockItem *t in [[self.tabs arrayByAddingObjectsFromArray:self.midItems] arrayByAddingObject:self.singleItem]) {
            ALBDockFadeLayer(t.icon.layer);
            ALBDockFadeLayer(t.title.layer);
        }
        ALBDockFadeLayer(self.divider.layer);
        ALBDockFadeLayer(self.avatarView.layer);
        Class tabButton = NSClassFromString(@"_UITabButton");
        UIView *platter = self.nativeBar ? ALBDockFindPlatter(self.nativeBar) : nil;
        if (platter && tabButton) {
            NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:platter];
            for (NSUInteger i = 0; i < queue.count && i < 200; i++) {
                for (UIView *sub in queue[i].subviews) {
                    if ([sub isKindOfClass:tabButton]) { ALBDockFadeLayer(sub.layer); continue; }
                    [queue addObject:sub];
                }
            }
        }
    }
    [self applyInk];
}

- (void)applyInk {
    // 3.11.51：玻璃固定深色，图标、文字按背后亮度切黑白（ALBDockInkStyle）；自建各项自己挂上这个深浅，动态色才按它解析
    UIUserInterfaceStyle inkStyle = ALBDockInkStyle();
    UITraitCollection *tc = [UITraitCollection traitCollectionWithUserInterfaceStyle:inkStyle];
    UIColor *ink = [ALBDockInk(0.92) resolvedColorWithTraitCollection:tc];
    if (self.nativeBar && ![self.nativeBar.unselectedItemTintColor isEqual:ink]) { self.nativeBar.unselectedItemTintColor = ink; }
    for (ALBDockItem *t in [[self.tabs arrayByAddingObjectsFromArray:self.midItems] arrayByAddingObject:self.singleItem]) {
        if (t.overrideUserInterfaceStyle != inkStyle) { t.overrideUserInterfaceStyle = inkStyle; }
        [t applyTint];
    }
    self.divider.backgroundColor = [ALBDockInk(0.22) resolvedColorWithTraitCollection:tc];
    self.avatarView.tintColor = [ALBDockInk(0.9) resolvedColorWithTraitCollection:tc];
}

- (NSDictionary<NSString *, id> *)currentPrefs {
    NSMutableDictionary *values = [NSMutableDictionary dictionary];
    for (NSString *k in ALBDockPrefDefaults()) { values[k] = ALBDockPrefValue(k); }
    return values;
}

- (void)optionsChanged {
    NSDictionary<NSString *, id> *now = [self currentPrefs];
    NSDictionary<NSString *, id> *was = self.prefSnapshot;
    self.prefSnapshot = now;
    BOOL geometry = NO, types = NO;
    for (NSString *k in now) {
        if ([now[k] isEqual:was[k]]) { continue; }
        // 几何四项一起处理一次就够了
        if ([@[kALBDockPrefBarHeight, kALBDockPrefMidWidth, kALBDockPrefGapA, kALBDockPrefGapB, kALBDockPrefTabWidth] containsObject:k]) {
            if (geometry) { continue; }
            geometry = YES;
        }
        // 3.11.45：项目几项也一起处理一次
        if ([@[kALBDockPrefOrder, kALBDockPrefHidden, kALBDockPrefSingle, kALBDockPrefMid] containsObject:k]) {
            if (types) { continue; }
            types = YES;
        }
        [[NSNotificationCenter defaultCenter] postNotificationName:ALBDockPrefsChangedNotification object:k];
    }
}

// 「底栏垂直位置」变了：静止态整套重排（动画中的话，动画结束的 applySteadyState 自然用上新位置）
- (void)verticalOffsetChanged {
    // 滑块每动一下都会发：只标记，下一帧合并成一次布局
    self.nativeTarget = CGRectZero;
    if (!self.transitioning) { [self setNeedsLayout]; }
}

- (void)prefsChanged:(NSNotification *)n {
    NSString *k = n.object;
    if ([k isEqualToString:kALBDockPrefIdle]) {
        self.idleDelay = ALBDockPrefDouble(kALBDockPrefIdle);
        [self noteInteraction];
    } else if ([k isEqualToString:kALBDockPrefClear] || [k isEqualToString:kALBDockPrefDarkTint] ||
               [k isEqualToString:kALBDockPrefLightTint]) {
        [self materialChanged];
    } else if ([k isEqualToString:kALBDockPrefAccent]) {
        [self applySelectedTint];
    } else if ([k isEqualToString:kALBDockPrefRedDot]) {
        self.cachedRedDot = ALBDockPrefBool(kALBDockPrefRedDot) && self.hasUnread;
        self.redDot.alpha = (self.collapsed && self.cachedRedDot) ? 1.0 : 0.0;

    } else if ([@[kALBDockPrefOrder, kALBDockPrefHidden, kALBDockPrefSingle, kALBDockPrefMid] containsObject:k]) {
        [self typesChanged];
    } else if ([k isEqualToString:kALBDockPrefArrow]) {
        [self loadTypes];
        [self applyArrowMode];
    } else if ([k isEqualToString:kALBDockPrefSingleAvatar] || [k isEqualToString:kALBDockPrefAvatarInset]) {
        [self loadTypes];
        if (!self.transitioning) { [self setNeedsLayout]; [self layoutIfNeeded]; }
        if (!self.collapsed) { [self noteInteraction]; }
    } else if ([@[kALBDockPrefBarHeight, kALBDockPrefMidWidth, kALBDockPrefGapA, kALBDockPrefGapB, kALBDockPrefTabWidth] containsObject:k]) {
        // 几何设置：强制系统底栏重新对齐、重新量图标位置，静止态整套重排
        self.nativeTarget = CGRectZero;
        [self.measuredTabX removeAllObjects];
        if (!self.transitioning) { [self setNeedsLayout]; [self layoutIfNeeded]; }
        // 调尺寸时保持展开预览：已收缩就展开回来，展开着就重新计时（不然 5 秒后收缩，只剩球在变）
        if (self.collapsed) { if (!self.transitioning) { [self expandToSide:self.side]; } }
        else { [self noteInteraction]; }
    }
}

// 页面 / 系统深浅色 / 透明开关变了：换材质。动画期间不切 effect（会卡），等结束再换
- (void)materialChanged {
    [self syncEdge];
    if (self.transitioning) { self.materialPending = YES; return; }
    self.materialPending = NO;
    UIUserInterfaceStyle st = ALBDockGlassStyle();
    if (self.overrideUserInterfaceStyle != st) {
        ALBDockMaterialLog([NSString stringWithFormat:@"四块玻璃→%@%@", st == UIUserInterfaceStyleDark ? @"深" : @"浅",
                            ALBDockGlassUsesClear() ? @"（透明）" : (gALBDockNativeGlass ? @"（原厂同款）" : @"（Regular）")]);
        self.overrideUserInterfaceStyle = st;
    }
    if (self.nativeBar && self.nativeBar.overrideUserInterfaceStyle != st) { self.nativeBar.overrideUserInterfaceStyle = st; }
    // 3.11.45：深浅在动画里过渡（只改 override）；玻璃种类变了才换 effect（applyMaterial 自己决定带不带动画）
    // 3.11.49：过渡 0.2 → 0.4 秒，图标颜色同时淡过去
    [UIView animateWithDuration:0.4 delay:0 options:UIViewAnimationOptionBeginFromCurrentState | UIViewAnimationOptionAllowUserInteraction
                     animations:^{
        for (ALBDockGlassGroup *g in @[self.mainGroup, self.midGroup, self.singleGroup]) { [g applyStyle]; }
    } completion:nil];
    for (ALBDockGlassGroup *g in @[self.mainGroup, self.midGroup, self.singleGroup]) { [g applyMaterial]; }
    [self applyInkFaded];
    [self applyNativePlatterGlass];
    // 3.11.1：系统底栏刚换了深浅，UIKit 可能把胶囊的玻璃换回原厂的：0.2 秒后核对一次，被换掉了就再写
    __weak typeof(self) weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        ALBDockView *ss = weakSelf;
        if (!ss || ss.transitioning) { return; }
        ss.platterRecheck = YES;
        [ss applyNativePlatterGlass];
    });
}

#pragma mark 3.11.33 液态底栏的图标、颜色，背后亮度

// 选中颜色（强调色 / 抖音红）写到自建五项和系统底栏
- (void)applySelectedTint {
    for (ALBDockItem *t in [self.tabs arrayByAddingObject:self.singleItem]) { [t applyTint]; }
    UIColor *sel = ALBDockSelectedTint();
    if (self.nativeBar && ![self.nativeBar.tintColor isEqual:sel]) { self.nativeBar.tintColor = sel; }
}

// 图标页改了图标 / 强调色：符号变了才换（这个通知每次改设置都会发）
- (void)iconsChanged {
    [self applySelectedTint];
    NSArray<NSString *> *symbols = ALBDockTabSymbols();
    NSString *single = symbols[self.singleType];
    BOOL changed = ![single isEqualToString:self.singleItem.symbol];
    for (NSUInteger i = 0; i < ALBDockTypeCount && i < self.tabs.count; i++) {
        if (![symbols[i] isEqualToString:self.tabs[i].symbol]) { changed = YES; }
    }
    if (!changed) { return; }
    for (NSUInteger i = 0; i < ALBDockTypeCount && i < self.tabs.count; i++) { [self.tabs[i] reloadSymbol:symbols[i]]; }
    [self.singleItem reloadSymbol:single];
    if (self.transitioning) { self.itemsPending = YES; return; }
    [self reloadNativeItems];
}

// 系统底栏的四项按新符号重建，并重新对齐、重新量图标规格（新符号宽窄不同）
- (void)reloadNativeItems {
    self.itemsPending = NO;
    if (!self.nativeBar || self.nativeOrderSide < 0) { return; }
    [self rebuildNativeItemsForSide:(ALBDockSide)self.nativeOrderSide];
    self.nativeTarget = CGRectZero;
    [self.measuredTabX removeAllObjects];
    [self setNeedsLayout];
}

// 探针垫在底栏那一条（整宽、底栏高），收缩成球时也是这条
- (void)layoutLumaProbe {
    if (!self.lumaProbe) { return; }
    CGFloat H = [self barH];
    CGRect want = CGRectMake(0, [self barCenterY] - H / 2.0, CGRectGetWidth(self.bounds), H);
    if (!CGRectEqualToRect(self.lumaProbe.frame, want)) { self.lumaProbe.frame = want; }
    if (self.lumaProbe.superview == self && self.subviews.firstObject != self.lumaProbe) { [self sendSubviewToBack:self.lumaProbe]; }
}

// 背后亮度 → 深 / 浅（只在视频页；非视频页照旧跟系统深浅）。高于阈值变浅，低于阈值 −0.08 变深；
// 中间不动。回到视频页（或刚打开）的第一次读数直接定；之后读数换边，0.15 秒后最新读数还在那一边才换（防抖）。
// 交给 ALBDockMaterialNoteBackdrop：浅＝浅色玻璃（透明＝白色染色）＋黑图标，和 3.11.1 原来的浅色透明同一条路
- (void)backdropLuma:(double)luma {
    self.lumaLast = luma;
    self.lumaAt = CACurrentMediaTime();
    self.lumaReads++;
    if (!ALBDockSceneIsVideo()) {
        self.lumaState = -1;
        return;
    }
    if (self.lumaState < 0) {
        NSInteger first = [self lumaSideFor:luma current:-1];
        self.lumaState = first;
        if ((first == 1) != ALBDockSceneIsLight()) {
            ALBDockMaterialLog([NSString stringWithFormat:@"回到视频页第一次读数 %.3f", luma]);
        }
        ALBDockMaterialNoteBackdrop(first == 1);
        return;
    }
    // 以底栏此刻真正的深浅为准（离开视频页时材质那边会把「浅」忘掉）
    NSInteger cur = ALBDockSceneIsLight() ? 1 : 0;
    self.lumaState = cur;
    if ([self lumaSideFor:luma current:cur] == cur) { return; }
    NSUInteger gen = ++self.lumaCheckGen;
    __weak typeof(self) weakSelf = self;
    // 3.11.49：防抖 0.15 → 0.2 秒
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        ALBDockView *ss = weakSelf;
        if (!ss || gen != ss.lumaCheckGen || ss.lumaState < 0 || !isfinite(ss.lumaLast)) { return; }
        if (!ALBDockSceneIsVideo()) { return; }
        NSInteger was = ALBDockSceneIsLight() ? 1 : 0;
        NSInteger now = [ss lumaSideFor:ss.lumaLast current:was];
        if (now == was) { return; }
        ss.lumaState = now;
        ss.lumaFlips++;
        ALBDockMaterialLog([NSString stringWithFormat:@"读数 %.3f 越过阈值（0.2 秒后仍在那边）", ss.lumaLast]);
        ALBDockMaterialNoteBackdrop(now == 1);
    });
}

// current：此刻的深浅（-1＝还没定，取近的那边）
- (NSInteger)lumaSideFor:(double)luma current:(NSInteger)current {
    double at = kALBDockInkLightAt;   // 3.11.51：阈值设置删了，固定
    if (luma > at) { return 1; }
    if (luma < at - 0.08) { return 0; }
    if (current >= 0) { return current; }
    return luma >= at - 0.04 ? 1 : 0;
}

#pragma mark 穿透

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
    UIView *hit = [super hitTest:point withEvent:event];
    return hit == self ? nil : hit;
}

- (BOOL)containsDockPoint:(CGPoint)p {
    if (self.hidden || self.alpha < 0.01) { return NO; }
    for (UIView *v in @[self.mainGroup, self.midGroup, self.singleGroup]) {
        if (!v.hidden && CGRectContainsPoint(CGRectInset(v.frame, -8, -12), p)) { return YES; }
    }
    if (self.nativeBar && self.nativeShown &&
        CGRectContainsPoint(CGRectInset(self.nativeBar.frame, 0, -6), p)) { return YES; }
    return NO;
}

#pragma mark 几何（2.6.0：高度 / 中栏宽 / 两个间距都来自设置，默认 62 / 74 / 8 / 12，边距 20）
//  有系统底栏时：高度上限 = 系统胶囊实测高（通常 62）；设得更低 = 整条底栏等比缩放（itemK < 1），
//  系统底栏整体 transform 缩放，自建图标按同一比例缩放，两者始终一样大
//  没有系统底栏（iOS 26 以下）时高度直接用设置值

- (CGFloat)scale { return MIN(MAX(CGRectGetWidth(self.bounds) / 440.0, 0.82), 1.15); }
- (CGFloat)platterH { return self.nativePlatterH > 10 ? self.nativePlatterH : 62.0; }
- (CGFloat)barH {
    CGFloat want = ALBDockPrefDouble(kALBDockPrefBarHeight);
    if (want < 1) { want = 62; }
    if (self.nativeBar) {
        // 滑块最大值（62）= 系统原高（实测可能是 62.33 / 64 之类），保证默认时 itemK 精确为 1、系统底栏不被重采样
        if (want >= 61.5) { return [self platterH]; }
        return round(MIN(MAX(want, 40.0), [self platterH]));
    }
    return round(MIN(MAX(want, 40.0), 80.0));
}
// 系统底栏与自建图标的缩放比例（= 底栏高 / 系统胶囊原高）
- (CGFloat)itemK { return self.nativeBar ? [self barH] / [self platterH] : 1.0; }
- (CGFloat)R { return [self barH] / 2.0; }

- (CGFloat)barCenterY {
    CGFloat safe = self.safeAreaInsets.bottom;
    if (safe <= 0 && self.window) { safe = self.window.safeAreaInsets.bottom; }
    CGFloat h = CGRectGetHeight(self.bounds);
    // 3.11：加上本插件的「底栏垂直位置」（和液态底栏同一个滑块，正值向下）
    CGFloat offset = ALBDockHostVerticalOffset();
    if (!isfinite(offset)) { offset = 0; }
    return (safe > 0 ? h - safe - [self barH] / 2.0 + 8.0 : h - 12.0 - [self barH] / 2.0) + offset;
}

- (CGPoint)ballPoint { return CGPointMake(CGRectGetMidX(self.bounds), [self barCenterY]); }

// 3.11.46 用户：“去掉按钮之后……它不会莫名其妙变长才对，就是原本它多长现在就多长……根本就用不着自适应把它拓展到最大。
// 而且……去掉两个按钮的胶囊之后，它们两个就不居中了。按道理来说，不管中间设置的间距是多少，都应该以左右边界为界限居中”。
// 格子宽固定：设置里的“单个项目宽度”（0＝自动＝原来默认排法 [四项][胶囊][单独键] 排满时的四分之一），不管中栏开不开、
// 藏了几项都是这个宽；主栏＝格子宽×项数，整组 [主栏][间距][中栏][间距][单独键] 在左右边距之间居中（单独键在左时镜像）。
// 默认全显示时整组正好排满，和原来一模一样。不显示中栏时主栏 ↔ 单独键用“主栏 ↔ 胶囊”的间距。
- (void)framesForSide:(ALBDockSide)side main:(CGRect *)mainF mid:(CGRect *)midF single:(CGRect *)singleF {
    CGFloat s = [self scale], W = CGRectGetWidth(self.bounds), H = [self barH];
    CGFloat margin = round(20 * s);
    // 2.6.0：中栏宽、两个间距按设置（pt，所见即所得，不随高度缩放）
    CGFloat gapA = round(MIN(MAX(ALBDockPrefDouble(kALBDockPrefGapA), 0), 30));
    CGFloat gapB = round(MIN(MAX(ALBDockPrefDouble(kALBDockPrefGapB), 0), 30));
    CGFloat midW = round(MIN(MAX(ALBDockPrefDouble(kALBDockPrefMidWidth), 44), 110)), singleW = H;
    BOOL mid = self.midOn;
    CGFloat inset = 4.0;
    CGFloat autoFull = MAX(120, W - margin * 2 - midW - singleW - gapA - gapB);
    CGFloat cell = (autoFull - inset * 2) / 4.0;
    CGFloat custom = ALBDockPrefDouble(kALBDockPrefTabWidth);
    if (custom >= 30) cell = MIN(custom, 140);
    NSUInteger n = MAX((NSUInteger)1, self.mainTypes.count);
    CGFloat usedMidW = mid ? midW : 0, usedGapB = mid ? gapB : 0;
    CGFloat others = gapA + usedMidW + usedGapB + singleW;
    CGFloat room = W - margin * 2 - others;          // 主栏最多这么宽（格子设得太大时收回来）
    CGFloat mainW = MIN(round(cell * n + inset * 2), MAX(60, room));
    CGFloat total = mainW + others;
    CGFloat x0 = round((W - total) / 2.0);
    CGFloat y = [self barCenterY] - H / 2.0;
    if (side == ALBDockSideRight) {
        *mainF = CGRectMake(x0, y, mainW, H);
        *midF = CGRectMake(CGRectGetMaxX(*mainF) + gapA, y, usedMidW, H);
        *singleF = CGRectMake(CGRectGetMaxX(*midF) + usedGapB, y, singleW, H);
    } else {
        *singleF = CGRectMake(x0, y, singleW, H);
        *midF = CGRectMake(CGRectGetMaxX(*singleF) + usedGapB, y, usedMidW, H);
        *mainF = CGRectMake(CGRectGetMaxX(*midF) + gapA, y, mainW, H);
    }
}

// 主栏 tab 顺序：单独键在右 → 设置里的顺序（默认 首页 拍摄 消息 我）；在左 → 倒过来（我 消息 拍摄 首页）
- (NSArray<NSNumber *> *)orderForSide:(ALBDockSide)side {
    NSArray<NSNumber *> *m = self.mainTypes.count ? self.mainTypes : @[@0];
    return side == ALBDockSideRight ? m : m.reverseObjectEnumerator.allObjects;
}

- (ALBDockGeo)geoForSide:(ALBDockSide)side {
    ALBDockGeo g;
    CGRect mainF, midF, singleF;
    [self framesForSide:side main:&mainF mid:&midF single:&singleF];
    g.main = mainF; g.mid = midF; g.single = singleF;
    NSArray<NSNumber *> *order = [self orderForSide:side];
    NSUInteger n = order.count;
    CGFloat inset = 4.0, cell = (mainF.size.width - inset * 2) / (CGFloat)MAX((NSUInteger)1, n);
    for (int i = 0; i < ALBDockTypeCount; i++) { g.tabX[i] = CGRectGetMidX(mainF); g.rank[i] = 0; }
    for (NSUInteger slot = 0; slot < n; slot++) {
        g.tabX[order[slot].unsignedIntegerValue] = CGRectGetMinX(mainF) + inset + cell * (slot + 0.5);
    }
    NSArray<NSNumber *> *measured = self.measuredTabX[@(side)];
    if (measured.count == ALBDockTypeCount) {
        for (NSNumber *t in order) { g.tabX[t.integerValue] = measured[t.integerValue].doubleValue; }
    }
    CGFloat cx = CGRectGetMidX(self.bounds);
    for (NSNumber *ti in order) {
        int i = ti.intValue, r = 0;
        for (NSNumber *tk in order) { if (fabs(g.tabX[tk.intValue] - cx) > fabs(g.tabX[i] - cx) + 0.01) { r++; } }
        g.rank[i] = r;
    }
    return g;
}

// 把三块放到 side 布局的最终状态，并记下几何
- (void)placeGroupsForSide:(ALBDockSide)side {
    self.mainGroup.hidden = NO;
    self.midGroup.hidden = NO;
    CGRect mainF, midF, singleF;
    [self framesForSide:side main:&mainF mid:&midF single:&singleF];
    self.mainHome = mainF;
    self.midHome = midF;

    self.singleGroup.transform = CGAffineTransformIdentity;
    self.singleGroup.bounds = CGRectMake(0, 0, singleF.size.width, singleF.size.height);
    self.singleGroup.home = CGPointMake(CGRectGetMidX(singleF), CGRectGetMidY(singleF));
    self.singleGroup.center = self.singleGroup.home;
    self.mainGroup.home = CGPointMake(CGRectGetMidX(mainF), CGRectGetMidY(mainF));
    self.midGroup.home = CGPointMake(CGRectGetMidX(midF), CGRectGetMidY(midF));

    NSArray<NSNumber *> *order = [self orderForSide:side];
    NSUInteger n = order.count;
    CGFloat inset = 4.0, cell = (mainF.size.width - inset * 2) / (CGFloat)MAX((NSUInteger)1, n);
    self.tabW = cell;
    NSMutableArray *hx = [NSMutableArray array];
    for (NSUInteger i = 0; i < ALBDockTypeCount; i++) { [hx addObject:@(CGRectGetMidX(mainF))]; }
    for (NSUInteger slot = 0; slot < n; slot++) {
        hx[order[slot].unsignedIntegerValue] = @(CGRectGetMinX(mainF) + inset + cell * (slot + 0.5));
    }
    self.tabHomeX = hx;
    CGFloat cx = CGRectGetMidX(self.bounds);
    NSArray *sorted = [order sortedArrayUsingComparator:^NSComparisonResult(NSNumber *a, NSNumber *b) {
        CGFloat da = fabs([hx[a.integerValue] doubleValue] - cx), db = fabs([hx[b.integerValue] doubleValue] - cx);
        return da > db ? NSOrderedAscending : (da < db ? NSOrderedDescending : NSOrderedSame);
    }];
    NSMutableArray *rank = [NSMutableArray array];
    for (NSUInteger i = 0; i < ALBDockTypeCount; i++) { [rank addObject:@0]; }
    [sorted enumerateObjectsUsingBlock:^(NSNumber *idx, NSUInteger r, BOOL *stop) { rank[idx.integerValue] = @(r); }];
    self.tabOuterRank = rank;

    [self setMainX0:CGRectGetMinX(mainF) x1:CGRectGetMaxX(mainF)];
    for (NSUInteger i = 0; i < ALBDockTypeCount; i++) {
        BOOL on = [order containsObject:@(i)];
        if (self.tabs[i].hidden == on) { self.tabs[i].hidden = !on; }
        if (on) { [self placeTab:i absX:[hx[i] doubleValue] alpha:1 scale:1]; }
    }
    if (self.midOn) {
        [self setMidX0:CGRectGetMinX(midF) x1:CGRectGetMaxX(midF) iconAlpha:1 iconScale:1];
    } else {
        [self parkMid];
    }

    CGFloat d = singleF.size.width, ik = [self itemK];
    self.singleItem.transform = CGAffineTransformIdentity;
    self.singleItem.bounds = CGRectMake(0, 0, d / ik, d / ik);
    self.singleItem.center = CGPointMake(d / 2, d / 2);
    self.singleItem.transform = [self itemScale:1];
    self.singleItem.restScale = ik;
    // 3.11.45：头像内边距可调（球和“单独键显示头像”共用，4＝原来的）
    CGFloat ad = MAX(8, d - 2 * self.avatarInset);
    self.avatarView.transform = CGAffineTransformIdentity;
    self.avatarView.bounds = CGRectMake(0, 0, ad, ad);
    self.avatarView.center = CGPointMake(d / 2, d / 2);
    self.avatarView.layer.cornerRadius = ad / 2.0;
    CGFloat ds = 9.0;
    self.redDot.frame = CGRectMake(d - ds - 4, 4, ds, ds);
    self.redDot.layer.cornerRadius = ds / 2.0;
}

// 中间胶囊不显示：挪出屏幕（不切 hidden / effect）
- (void)parkMid {
    [self parkOffscreen:self.midGroup];
    for (ALBDockItem *it in self.midItems) { it.alpha = 0; }
    self.divider.alpha = 0;
    [self fallbackAlpha:self.midGroup value:0];
}

// k = 1 正常；k → 0 时宽高一起向自身中心缩到球的 40%（藏在球 / 头像底下）。
// 2.5.1：2.5.0 是缩成 1pt 的点，极小的玻璃块放进融合容器疑似让整组玻璃失去折射，改成不小于 40% 的正常尺寸；
// 静止时藏起来的玻璃改为挪到屏幕外（parkOffscreen:），不和球叠层
- (CGFloat)minGlass { return round([self barH] * 0.4); }

// 自建图标统一按 itemK 缩放：图标规格照抄系统底栏原尺寸（未缩放的 pt），视觉大小 = 原尺寸 × itemK，和系统底栏一致
- (CGAffineTransform)itemScale:(CGFloat)s {
    CGFloat k = [self itemK] * s;
    return CGAffineTransformMakeScale(k, k);
}

- (void)setMainX0:(CGFloat)a x1:(CGFloat)b { [self setMainX0:a x1:b k:1]; }

- (void)setMainX0:(CGFloat)a x1:(CGFloat)b k:(CGFloat)k {
    CGFloat H = [self barH], m = [self minGlass];
    CGFloat w = ALBDockLerp(m, MAX(m, b - a), k), h = ALBDockLerp(m, H, k);
    self.mainGroup.transform = CGAffineTransformIdentity;
    self.mainGroup.bounds = CGRectMake(0, 0, w, h);
    self.mainGroup.center = CGPointMake((a + b) / 2.0, [self barCenterY]);   // y 永远固定
}

- (void)placeTab:(NSUInteger)i absX:(CGFloat)x alpha:(CGFloat)a scale:(CGFloat)s {
    ALBDockItem *it = self.tabs[i];
    CGFloat H = [self barH], mh = CGRectGetHeight(self.mainGroup.bounds), ik = [self itemK];
    CGFloat minX = self.mainGroup.center.x - CGRectGetWidth(self.mainGroup.bounds) / 2.0;
    it.transform = CGAffineTransformIdentity;
    it.bounds = CGRectMake(0, 0, self.tabW / ik, H / ik);
    it.center = CGPointMake(x - minX, mh / 2.0);
    it.alpha = a;
    it.transform = [self itemScale:s];
    it.restScale = ik;
    // 3.11.45：选中的那一项不在主栏（单独键、隐藏了、或者不是这几页）时不显示选中块
    if (self.selectedIndex < 0 || ![self.mainTypes containsObject:@(self.selectedIndex)]) { self.selPill.alpha = 0; }
    if ((NSInteger)i == self.selectedIndex) {
        // 选中块照抄系统透镜的实测大小（× 缩放比例；量到之前按格子算）
        CGSize lens = self.nativeLensSize;
        CGFloat w = lens.width > 1 ? lens.width * ik : self.tabW - 2;
        CGFloat h = lens.height > 1 ? MIN(lens.height * ik, H) : H - 8;
        w = MIN(w, CGRectGetWidth(self.mainGroup.bounds) - 8);
        self.selPill.bounds = CGRectMake(0, 0, MAX(0, w), h);
        self.selPill.center = CGPointMake(x - minX, mh / 2.0);
        self.selPill.layer.cornerRadius = h / 2.0;
        self.selPill.alpha = a;
    }
}

- (void)setMidX0:(CGFloat)a x1:(CGFloat)b iconAlpha:(CGFloat)ia iconScale:(CGFloat)s {
    [self setMidX0:a x1:b iconAlpha:ia iconScale:s k:1];
}

- (void)setMidX0:(CGFloat)a x1:(CGFloat)b iconAlpha:(CGFloat)ia iconScale:(CGFloat)s k:(CGFloat)k {
    CGFloat H = [self barH], hw = CGRectGetWidth(self.midHome) / 2.0, m = [self minGlass];
    CGFloat w = ALBDockLerp(m, MAX(m, b - a), k), h = ALBDockLerp(m, H, k);
    self.midGroup.transform = CGAffineTransformIdentity;
    self.midGroup.bounds = CGRectMake(0, 0, w, h);
    self.midGroup.center = CGPointMake((a + b) / 2.0, [self barCenterY]);
    NSArray *cx = @[@(w * 0.25), @(w * 0.75)];
    CGFloat ik = [self itemK];
    for (NSUInteger i = 0; i < 2; i++) {
        ALBDockItem *it = self.midItems[i];
        it.transform = CGAffineTransformIdentity;
        it.bounds = CGRectMake(0, 0, hw / ik, H / ik);
        it.center = CGPointMake([cx[i] doubleValue], h / 2.0);
        it.alpha = ia;
        it.transform = [self itemScale:s];
        it.restScale = ik;
    }
    self.divider.frame = CGRectMake(w / 2.0 - 0.5, h * 0.32, 1.0, h * 0.36);
    self.divider.alpha = ia;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    [self syncEdge];
    [self layoutLumaProbe];
    if (self.transitioning) { return; }
    [self applySteadyState];
}

// 3.11.31：四块玻璃登记在底栏自己的边上（ALBDockHostSyncEdge，关掉「dock.followEdge」就撤）
- (void)syncEdge {
    NSMutableArray<UIView *> *glass = [NSMutableArray array];
    for (UIView *v in @[self.mainGroup ?: (id)[NSNull null], self.midGroup ?: (id)[NSNull null],
                        self.singleGroup ?: (id)[NSNull null], self.nativeBar ?: (id)[NSNull null]]) {
        if ([v isKindOfClass:[UIView class]]) { [glass addObject:v]; }
    }
    ALBDockHostSyncEdge(self, glass);
}

// 静止态：展开时显示系统底栏、自建主栏挪到屏幕外；收缩时主栏 / 中栏都挪到屏幕外 —— 球永远只有自己一层玻璃
// 系统底栏进出融合容器（只在静止时换父视图；容器内容视图和底栏坐标系相同，位置不变）
- (void)ensureNativeParent {
    if (!self.nativeBar || self.transitioning) { return; }
    UIView *parent = self.container ? self.stack : self;   // 3.11.51：按压融合固定开
    if (self.nativeBar.superview == parent) { return; }
    CGPoint c = self.nativeBar.center;
    [parent addSubview:self.nativeBar];
    self.nativeBar.center = c;
}

- (void)applySteadyState {
    [self ensureNativeParent];
    [self placeGroupsForSide:self.side];
    // 3.11.45：单独键显示头像（功能不变）：展开时也是头像，收缩前后不换图标
    BOOL face = self.collapsed || self.avatarSingle;
    self.avatarView.alpha = face ? 1 : 0;
    self.singleItem.alpha = face ? 0 : 1;
    if (self.collapsed) {
        self.singleGroup.center = [self ballPoint];
        [self parkUnderBall];
        [self setNativeShown:NO];
        return;
    }
    if (self.nativeBar) {
        [self layoutNativeForSide:self.side];
        [self parkMainUnderX:self.singleGroup.center.x];
        [self setNativeShown:YES];
        [self syncNative];
    }
}

// 藏起来的玻璃块：正常大小（球那么大）挪到屏幕底边以下。只改位置，不切 effect / hidden（切了会卡）
// 2.4.0 是叠在球下面 → 球多一层玻璃、色调变深；2.5.0 是缩成 1pt 的点 → 疑似整组玻璃没了折射；2.5.1 改成挪出屏幕
- (void)parkOffscreen:(ALBDockGlassGroup *)g {
    CGFloat H = [self barH];
    g.transform = CGAffineTransformIdentity;
    g.bounds = CGRectMake(0, 0, H, H);
    g.center = CGPointMake(CGRectGetMidX(self.bounds), CGRectGetHeight(self.bounds) + H * 2.0);
}

- (void)parkMainUnderX:(CGFloat)x {
    [self parkOffscreen:self.mainGroup];
    CGFloat minX = self.mainGroup.center.x - CGRectGetWidth(self.mainGroup.bounds) / 2.0;
    for (NSUInteger i = 0; i < ALBDockTypeCount; i++) { [self placeTab:i absX:minX alpha:0 scale:0.72]; }
    [self fallbackAlpha:self.mainGroup value:0];
}

#pragma mark 系统底栏

- (void)rebuildNativeItemsForSide:(ALBDockSide)side {
    // 3.11.33：标题、符号＝液态底栏同一功能那一项（和液态底栏一样不带尺寸配置，交给系统排）
    NSArray<NSString *> *titles = ALBDockDisplayTitles();
    NSArray<NSString *> *symbols = ALBDockTabSymbols();
    NSArray<NSString *> *fallbacks = ALBDockTabFallbacks();
    NSMutableArray *items = [NSMutableArray array];
    for (NSNumber *n in [self orderForSide:side]) {     // 3.11.45：主栏里的项（tag＝项目）
        NSInteger i = n.integerValue;
        // 2.5.0：不带任何尺寸配置，图标大小 / 字重 / 标题字号全部交给系统按标准排
        UIImage *img = ALBDockSymbolImage(symbols[i], fallbacks[i], nil);
        UITabBarItem *item = [[UITabBarItem alloc] initWithTitle:titles[i] image:img tag:i];
        [items addObject:item];
    }
    [self.nativeBar setItems:items animated:NO];
    self.nativeOrderSide = side;
    self.nativeSelectedTitle = nil;   // 强制下一次 sync 重写选中项
    [self syncNative];
}

static UIView *ALBDockFindPlatter(UIView *bar) {
    static Class platterClass;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ platterClass = NSClassFromString(@"UIKit._UITabBarPlatterView"); });
    return ALBDockFindSubview(bar, ^BOOL(UIView *v) {
        if (platterClass) { return [v isKindOfClass:platterClass]; }
        return [NSStringFromClass(v.class) hasSuffix:@"TabBarPlatterView"];
    });
}

// 把系统底栏的胶囊对齐到 side 布局的主栏位置。
// 2.6.0：底栏高度 = 设置值（上限是系统胶囊原高）；低于原高时系统底栏整体按 itemK 缩放（同「胶囊高度」），
// 图标、标题、透镜随之缩放，自建图标按同一比例缩放
// 只在目标变化时写，避免反复重排冲掉系统的选中状态和长按拖动手势
- (void)layoutNativeForSide:(ALBDockSide)side {
    if (!self.nativeBar) { return; }
    if (self.nativeOrderSide != side) { [self rebuildNativeItemsForSide:side]; }
    ALBDockGeo g = [self geoForSide:side];
    CGRect target = g.main;
    if (CGRectEqualToRect(target, self.nativeTarget) && self.measuredTabX[@(side)].count == ALBDockTypeCount) { return; }
    self.nativeTarget = target;

    if (self.nativeInset <= 0) { self.nativeInset = 21; }
    if (self.nativePlatterH <= 0) { self.nativePlatterH = 62; }
    CGFloat platterH0 = self.nativePlatterH;
    UITabBar *bar = self.nativeBar;
    bar.transform = CGAffineTransformIdentity;
    UIView *platter = nil;
    CGRect pf = CGRectZero;
    for (int pass = 0; pass < 3; pass++) {
        CGFloat want = CGRectGetWidth(target) / [self itemK];
        bar.bounds = CGRectMake(0, 0, want + 2 * self.nativeInset, MAX(83, self.nativePlatterH + 21));
        [bar setNeedsLayout];
        [bar layoutIfNeeded];
        platter = ALBDockFindPlatter(bar);
        if (!platter) { break; }
        pf = [platter convertRect:platter.bounds toView:bar];
        CGFloat inset = (CGRectGetWidth(bar.bounds) - CGRectGetWidth(pf)) / 2.0;
        BOOL changed = fabs(inset - self.nativeInset) > 0.5 || fabs(CGRectGetHeight(pf) - self.nativePlatterH) > 0.5;
        self.nativeInset = inset;
        if (CGRectGetHeight(pf) > 10) { self.nativePlatterH = CGRectGetHeight(pf); }
        if (!changed) { break; }
    }
    if (CGRectIsEmpty(pf)) {
        pf = CGRectMake(self.nativeInset, 0, CGRectGetWidth(bar.bounds) - 2 * self.nativeInset, self.nativePlatterH);
    }
    CGFloat k = [self itemK];
    CGPoint bc = CGPointMake(CGRectGetMidX(bar.bounds), CGRectGetMidY(bar.bounds));
    CGPoint pc = CGPointMake(CGRectGetMidX(pf), CGRectGetMidY(pf));
    bar.center = CGPointMake(CGRectGetMidX(target) - k * (pc.x - bc.x), CGRectGetMidY(target) - k * (pc.y - bc.y));
    [self applyNativeTransform];
    [self applyNativePlatterGlass];
    [self measureNativeTabsForSide:side platter:platter];
    // 胶囊实测高和预设的 62 不同：整套几何按实测高重排一次
    if (fabs(self.nativePlatterH - platterH0) > 0.5) {
        self.nativeTarget = CGRectZero;
        [self setNeedsLayout];
    }
}

// 2.6.0：系统底栏在融合容器里，显示 / 隐藏只改 transform（缩放 + 挪出屏幕），不切 alpha / hidden
- (void)setNativeShown:(BOOL)shown {
    _nativeShown = shown;
    [self applyNativeTransform];
}

- (void)applyNativeTransform {
    UITabBar *bar = self.nativeBar;
    if (!bar) { return; }
    CGFloat k = [self itemK];
    CGAffineTransform t = CGAffineTransformMakeScale(k, k);
    if (!_nativeShown) { t = CGAffineTransformConcat(t, CGAffineTransformMakeTranslation(0, 3000)); }
    if (!CGAffineTransformEqualToTransform(bar.transform, t)) { bar.transform = t; }
}

static UIImageView *ALBDockFindSymbolIcon(UIView *root) {
    return (UIImageView *)ALBDockFindSubview(root, ^BOOL(UIView *v) {
        if (![v isKindOfClass:[UIImageView class]] || v.hidden || v.alpha < 0.01) { return NO; }
        if ([NSStringFromClass(v.superview.class) containsString:@"Badge"]) { return NO; }
        UIImage *img = ((UIImageView *)v).image;
        return img != nil && img.isSymbolImage;
    });
}

static UILabel *ALBDockFindTitleLabel(UIView *root) {
    NSArray<NSString *> *titles = ALBDockDisplayTitles();
    return (UILabel *)ALBDockFindSubview(root, ^BOOL(UIView *v) {
        return [v isKindOfClass:[UILabel class]] && ((UILabel *)v).text.length && [titles containsObject:((UILabel *)v).text];
    });
}

// 照抄系统底栏实际画出来的规格 —— 图标的符号配置 / 尺寸 / 位置、标题字体 / 位置、透镜大小。
// 2.6.0：逐个按钮量、按标题对应到自建的那一项（2.5.x 是拿第一颗的规格套到四颗上：消息的图标比首页宽，
// 被套进首页那颗的框里画小了，展开后换回系统底栏就「放大」一下）
static UIImageSymbolConfiguration *ALBDockIconConfig(UIImageView *iconView) {
    UIImageSymbolConfiguration *cfg = iconView.image.symbolConfiguration;
    UIImageSymbolConfiguration *pref = iconView.preferredSymbolConfiguration;
    if (pref) { cfg = cfg ? (UIImageSymbolConfiguration *)[cfg configurationByApplyingConfiguration:pref] : pref; }
    return cfg;
}

- (void)measureNativeMetricsInPlatter:(UIView *)platter buttons:(NSArray<UIView *> *)buttons lensCopies:(NSArray<UIView *> *)lensCopies {
    CGFloat pcy = CGRectGetMidY(platter.bounds);
    NSArray<NSString *> *titles = ALBDockDisplayTitles();   // 3.11.33：系统底栏上显示的是液态底栏的标题
    UIImageSymbolConfiguration *shared = nil;
    NSMutableIndexSet *done = [NSMutableIndexSet indexSet];
    __block UIImageView *firstIcon = nil;
    __block CGFloat firstIconDY = 0, firstTitleDY = 0;
    __block UIFont *firstFont = nil;
    // 先量正常那份，没量到的再量透镜里那份（选中项的底图可能被系统藏起来）
    for (NSArray<UIView *> *list in @[buttons, lensCopies ?: @[]]) {
        for (UIView *b in list) {
            UIImageView *iconView = ALBDockFindSymbolIcon(b);
            UILabel *label = ALBDockFindTitleLabel(b);
            NSUInteger idx = label ? [titles indexOfObject:label.text] : NSNotFound;
            if (!iconView || idx == NSNotFound || [done containsIndex:idx]) { continue; }
            UIImageSymbolConfiguration *cfg = ALBDockIconConfig(iconView);
            CGPoint ic = [iconView convertPoint:CGPointMake(CGRectGetMidX(iconView.bounds), CGRectGetMidY(iconView.bounds)) toView:platter];
            CGPoint lc = [label convertPoint:CGPointMake(CGRectGetMidX(label.bounds), CGRectGetMidY(label.bounds)) toView:platter];
            ALBDockItem *t = self.tabs[idx];
            t.icon.contentMode = iconView.contentMode;
            [t applySymbolConfig:cfg iconSize:iconView.bounds.size iconDY:ic.y - pcy titleFont:label.font titleDY:lc.y - pcy];
            [done addIndex:idx];
            if (idx == 0 || !shared) { shared = cfg; }      // 朋友球、✦ ↑ 用首页那颗的符号配置
            if (!firstIcon) { firstIcon = iconView; firstIconDY = ic.y - pcy; firstTitleDY = lc.y - pcy; firstFont = label.font; }
        }
    }
    // 两份都没量到的项：用量到的第一项的规格（图标尺寸按自己的符号算，不套别人的框）
    if (firstIcon) {
        for (NSUInteger i = 0; i < self.tabs.count; i++) {
            if ([done containsIndex:i]) { continue; }
            ALBDockItem *t = self.tabs[i];
            t.icon.contentMode = UIViewContentModeCenter;
            [t applySymbolConfig:shared iconSize:CGSizeZero iconDY:firstIconDY titleFont:firstFont titleDY:firstTitleDY];
        }
    }
    if (shared) {
        for (ALBDockItem *t in [self.midItems arrayByAddingObject:self.singleItem]) {
            [t applySymbolConfig:shared iconSize:CGSizeZero iconDY:0 titleFont:nil titleDY:0];
        }
        self.nativeMetricsReady = YES;
    }
    UIView *lens = ALBDockFindSubview(platter, ^BOOL(UIView *v) {
        return [NSStringFromClass(v.class) containsString:@"TabSelection"] && !v.hidden && CGRectGetWidth(v.bounds) > 10;
    });
    // 只认像透镜的尺寸（比整条胶囊窄得多、不比胶囊高），量到容器之类就不用
    CGSize ls = lens.bounds.size;
    if (lens && ls.width < CGRectGetWidth(platter.bounds) * 0.6 && ls.height <= CGRectGetHeight(platter.bounds) + 0.5) {
        self.nativeLensSize = ls;
    }
}

// 量系统底栏每颗按钮的中心（每个 tab 系统画两遍，跳过透镜里那份 SelectedContentView）
- (void)measureNativeTabsForSide:(ALBDockSide)side platter:(UIView *)platter {
    if (!platter) { return; }
    Class tabButton = NSClassFromString(@"_UITabButton");
    if (!tabButton) { return; }
    NSMutableArray<UIView *> *buttons = [NSMutableArray array];
    NSMutableArray<UIView *> *lensCopies = [NSMutableArray array];
    __block void (^collect)(UIView *, BOOL);
    void (^__block __weak weakCollect)(UIView *, BOOL);
    collect = ^(UIView *root, BOOL inSelected) {
        for (UIView *sub in root.subviews) {
            BOOL sel = inSelected || [NSStringFromClass(sub.class) containsString:@"SelectedContentView"];
            if ([sub isKindOfClass:tabButton]) { [(sel ? lensCopies : buttons) addObject:sub]; continue; }
            weakCollect(sub, sel);
        }
    };
    weakCollect = collect;
    collect(platter, NO);
    if (buttons.count) { [self measureNativeMetricsInPlatter:platter buttons:buttons lensCopies:lensCopies]; }
    [self measureNativeBadges];
    NSArray<NSNumber *> *order = [self orderForSide:side];
    if (buttons.count != order.count) { return; }
    [buttons sortUsingComparator:^NSComparisonResult(UIView *a, UIView *b) {
        CGFloat ax = [a convertPoint:CGPointMake(CGRectGetMidX(a.bounds), 0) toView:self].x;
        CGFloat bx = [b convertPoint:CGPointMake(CGRectGetMidX(b.bounds), 0) toView:self].x;
        return ax < bx ? NSOrderedAscending : NSOrderedDescending;
    }];
    NSMutableArray *xs = [NSMutableArray array];
    for (NSUInteger i = 0; i < ALBDockTypeCount; i++) { [xs addObject:@0]; }
    for (NSUInteger slot = 0; slot < order.count; slot++) {
        UIView *b = buttons[slot];
        CGFloat x = [b convertPoint:CGPointMake(CGRectGetMidX(b.bounds), CGRectGetMidY(b.bounds)) toView:self].x;
        xs[order[slot].unsignedIntegerValue] = @(x);
    }
    self.measuredTabX[@(side)] = xs;
}

// 「透明玻璃」开关也作用到系统胶囊（KVC glassEffect）；关掉时还原原厂那只
// 2.5.0：只在视频页（首页 / 朋友）换成透明；其他页还原原厂玻璃 —— 原厂那只本来就跟系统深浅色和「透明 / 着色」设置
- (void)applyNativePlatterGlass {
    UIView *platter = self.nativeBar ? ALBDockFindPlatter(self.nativeBar) : nil;
    if (!platter) { return; }
    static char kOrigKey, kAppliedKey, kInstalledKey;
    BOOL clear = ALBDockGlassUsesClear();
    // 3.11.44：系统胶囊原厂那只玻璃记下来，三块自建玻璃没开透明时就用它（和左边同一份）
    if (!gALBDockNativeGlass) {
        id factory = objc_getAssociatedObject(platter, &kOrigKey);
        if (factory == [NSNull null]) { factory = nil; }
        if (!factory && !objc_getAssociatedObject(platter, &kAppliedKey)) {
            @try { factory = [platter valueForKey:@"glassEffect"]; } @catch (__unused NSException *e) { factory = nil; }
        }
        if ([factory isKindOfClass:[UIVisualEffect class]]) {
            gALBDockNativeGlass = [(UIVisualEffect *)factory copy];
            NSString *style = @"?", *interactive = @"?";
            @try { style = [[factory valueForKey:@"style"] description] ?: @"?"; } @catch (__unused NSException *e) {}
            @try { interactive = [[factory valueForKey:@"interactive"] boolValue] ? @"是" : @"否"; } @catch (__unused NSException *e) {}
            gALBDockNativeGlassInfo = [NSString stringWithFormat:@"%@ style=%@ 可按压=%@", NSStringFromClass([factory class]), style, interactive];
            ALBDockMaterialLog(@"读到系统胶囊原厂玻璃，自建三块换成同一份");
            if (!clear && !self.transitioning) {
                [UIView performWithoutAnimation:^{
                    for (ALBDockGlassGroup *g in @[self.mainGroup, self.midGroup, self.singleGroup]) { [g applyMaterial]; }
                }];
            }
        } else if (!gALBDockNativeGlassInfo) {
            gALBDockNativeGlassInfo = factory ? [NSString stringWithFormat:@"读到的不是玻璃（%@），自建的照旧用 Regular", NSStringFromClass([factory class])]
                                              : @"读不到（glassEffect 不在），自建的照旧用 Regular";
        }
    }
    // 3.11.1：透明分深 / 浅两种染色，染色值也算在里面（设置里能调）
    NSString *want = clear ? [NSString stringWithFormat:@"clear|%.3f", ALB25MaterialDarkTint()] : @"native";
    NSString *applied = objc_getAssociatedObject(platter, &kAppliedKey);
    BOOL recheck = self.platterRecheck;
    self.platterRecheck = NO;
    if ([applied isEqualToString:want] || (!applied && !clear)) {
        if (!recheck || !clear) { return; }
        id current = nil;
        @try { current = [platter valueForKey:@"glassEffect"]; } @catch (__unused NSException *e) {}
        if (current == objc_getAssociatedObject(platter, &kInstalledKey)) { return; }
    }
    @try {
        if (!objc_getAssociatedObject(platter, &kOrigKey)) {
            id orig = [platter valueForKey:@"glassEffect"];
            objc_setAssociatedObject(platter, &kOrigKey, orig ?: [NSNull null], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        if (clear) {
            [platter setValue:ALBDockMakeGlass(YES) forKey:@"glassEffect"];
            objc_setAssociatedObject(platter, &kInstalledKey, [platter valueForKey:@"glassEffect"], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        } else if (applied) {
            id orig = objc_getAssociatedObject(platter, &kOrigKey);
            [platter setValue:(orig == [NSNull null] ? nil : orig) forKey:@"glassEffect"];
        }
    } @catch (__unused NSException *e) {}
    objc_setAssociatedObject(platter, &kAppliedKey, want, OBJC_ASSOCIATION_COPY_NONATOMIC);
}

// 3.11.44：透明时系统胶囊装的是插件的透明玻璃（和右边两块同一份）；UIKit 重建胶囊（换深浅、换选中页、重新布局）时会把它换回
// 原厂那只，而装过的记号还在，之前要等下一次换深浅才再装，这段时间左边是原厂玻璃、右边两块是透明＋深色染色，看着就是右边变深。
// 读回来的玻璃是副本（不能比指针），比染色：插件装的深色档带黑色染色、浅色档带白色染色，原厂那只没有染色
static BOOL ALBDockSameTint(UIColor *a, UIColor *b) {
    CGFloat ar = 0, ag = 0, ab = 0, aa = 0, br = 0, bg = 0, bb = 0, ba = 0;
    if (a && ![a getRed:&ar green:&ag blue:&ab alpha:&aa]) { CGFloat w = 0; [a getWhite:&w alpha:&aa]; ar = ag = ab = w; }
    if (b && ![b getRed:&br green:&bg blue:&bb alpha:&ba]) { CGFloat w = 0; [b getWhite:&w alpha:&ba]; br = bg = bb = w; }
    if (aa < 0.005 && ba < 0.005) { return YES; }
    return fabs(aa - ba) < 0.02 && fabs(ar - br) < 0.02 && fabs(ag - bg) < 0.02 && fabs(ab - bb) < 0.02;
}
static NSUInteger gALBDockPlatterReverted;
static NSInteger gALBDockPlatterVerify;     // 0 还没核对过 1 能核对 -1 读回来的玻璃不带染色，核对不了（不再核对，免得每次都重装）
- (void)verifyNativePlatterGlass {
    if (gALBDockPlatterVerify < 0 || !ALBDockGlassUsesClear() || self.transitioning || !self.nativeShown) { return; }
    UIView *platter = self.nativeBar ? ALBDockFindPlatter(self.nativeBar) : nil;
    if (!platter) { return; }
    id current = nil;
    @try { current = [platter valueForKey:@"glassEffect"]; } @catch (__unused NSException *e) { return; }
    if (![current respondsToSelector:@selector(tintColor)]) { return; }
    UIColor *have = [current valueForKey:@"tintColor"];
    UIVisualEffect *want = ALBDockMakeGlass(YES);
    UIColor *wantTint = [want respondsToSelector:@selector(tintColor)] ? [(id)want valueForKey:@"tintColor"] : nil;
    if (ALBDockSameTint(have, wantTint)) { return; }
    gALBDockPlatterReverted++;
    ALBDockMaterialLog([NSString stringWithFormat:@"系统胶囊的玻璃被换回原厂（第 %lu 次），重新装上", (unsigned long)gALBDockPlatterReverted]);
    self.platterRecheck = YES;
    [self applyNativePlatterGlass];
    id after = nil;
    @try { after = [platter valueForKey:@"glassEffect"]; } @catch (__unused NSException *e) { after = nil; }
    UIColor *afterTint = [after respondsToSelector:@selector(tintColor)] ? [after valueForKey:@"tintColor"] : nil;
    if (!ALBDockSameTint(afterTint, wantTint)) {
        gALBDockPlatterVerify = -1;
        ALBDockMaterialLog(@"装上以后读回来的玻璃不带染色：没法核对，停止核对");
        return;
    }
    gALBDockPlatterVerify = 1;
}

// 选中页 / 角标从抖音原生底栏镜像过来；值没变不写
// 3.11.45：五项按项目认（单独键也可能是首页 / 消息 / 我），选中的不在主栏时系统底栏不选任何一项
- (void)syncNative {
    NSString *sel = ALBDockBridgeSelectedTitle();
    ALBDockMaterialUpdate(self.window, sel);     // 2.5.0：切到 / 离开首页、朋友页时换材质（变了才通知）
    NSArray<NSString *> *titles = ALBDockTabTitles();
    NSInteger idx = sel ? (NSInteger)[titles indexOfObject:sel] : NSNotFound;

    // 自建图标（动画期间用）
    if (sel && idx != NSNotFound) {
        NSInteger want = [self.mainTypes containsObject:@(idx)] ? idx : -1;
        if (want != self.selectedIndex) {
            if (self.selectedIndex >= 0 && self.selectedIndex < (NSInteger)self.tabs.count) { self.tabs[self.selectedIndex].selectedTab = NO; }
            self.selectedIndex = want;
            if (want >= 0) { self.tabs[want].selectedTab = YES; }
        }
        BOOL singleSel = idx == self.singleType;
        if (self.singleItem.selectedTab != singleSel) { self.singleItem.selectedTab = singleSel; }
    }

    if (self.nativeBar && sel && ![sel isEqualToString:self.nativeSelectedTitle]) {
        UITabBarItem *target = nil;
        for (UITabBarItem *it in self.nativeBar.items) {
            if (idx != NSNotFound && it.tag == idx) { target = it; break; }
        }
        if (self.nativeBar.selectedItem != target) { self.nativeBar.selectedItem = target; }
        self.nativeSelectedTitle = sel;
    }

    // 角标（主栏的写到系统底栏和自建那一项；单独键写到单独键）
    BOOL badgeChanged = NO, anyBadge = NO;
    NSMutableArray<NSString *> *unread = [NSMutableArray array];
    for (NSUInteger i = 0; i < ALBDockTypeCount; i++) {
        if (i == ALBDockTypePlus) { continue; }
        NSString *badge = ALBDockBridgeBadgeForTitle(titles[i]);
        for (UITabBarItem *it in self.nativeBar.items) {
            if (it.tag != (NSInteger)i) { continue; }
            NSString *cur = it.badgeValue;
            if (cur != badge && ![cur isEqualToString:badge]) { it.badgeValue = badge; badgeChanged = YES; }
        }
        ALBDockItem *t = self.tabs[i];
        if (badge.length) { [t setBadgeText:badge]; t.badge.hidden = NO; anyBadge = YES; }
        else if (t.badge) { t.badge.hidden = YES; }
        if ((NSInteger)i == self.singleType) {
            ALBDockItem *one = self.singleItem;
            if (badge.length) { [one setBadgeText:badge]; one.badge.hidden = NO; }
            else if (one.badge) { one.badge.hidden = YES; }
        }
        // 3.11.47：收进球里的项（主栏里的、单独键）有角标（数字或只是个点）才算有未读
        if (badge && ([self.mainTypes containsObject:@(i)] || (NSInteger)i == self.singleType)) {
            [unread addObject:badge.length ? [NSString stringWithFormat:@"%@ %@", titles[i], badge] : [NSString stringWithFormat:@"%@ 小红点", titles[i]]];
        }
    }
    self.hasUnread = unread.count > 0;
    self.unreadSource = unread.count ? [unread componentsJoinedByString:@"、"] : nil;
    BOOL dotWanted = ALBDockPrefBool(kALBDockPrefRedDot) && self.hasUnread;
    if (!self.transitioning) {
        self.cachedRedDot = dotWanted;
        CGFloat want = (self.collapsed && dotWanted) ? 1.0 : 0.0;
        if (fabs(self.redDot.alpha - want) > 0.01) {
            [UIView animateWithDuration:0.2 animations:^{ self.redDot.alpha = want; }];
        }
    }
    // 3.11.47：角标变了（或者还没量过）：等系统底栏把角标画出来，再照它的规格改自建的角标
    if ((badgeChanged || (anyBadge && !self.tabs.firstObject.hasBadgeMetrics)) && !self.badgeMeasurePending && self.nativeBar) {
        self.badgeMeasurePending = YES;
        __weak typeof(self) weakSelf = self;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.25 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            ALBDockView *ss = weakSelf;
            if (!ss) { return; }
            ss.badgeMeasurePending = NO;
            if (ss.transitioning) { return; }
            [ss.nativeBar layoutIfNeeded];
            [ss measureNativeBadges];
        });
    }
}

// 3.11.47 用户：“未读的这个消息角标它乱跳……之前修过，但是现在还是跳”。录屏：收缩 / 展开动画里画的是自建的角标（9pt、高 14），
// 展开结束系统底栏接管时换成系统画的角标（更大、位置也不一样），交接那一帧“跳”一下。现在量系统底栏实际画出来的角标
// （左边、顶边相对图标中心的偏移、高、字体、左右留白、底色），自建角标照抄；角标是图标那一项的子视图，跟着运动路径走
static NSUInteger gALBDockBadgeMeasures;
static NSString *gALBDockBadgeSpec;
- (void)measureNativeBadges {
    if (!self.nativeBar) { return; }
    UIView *platter = ALBDockFindPlatter(self.nativeBar);
    Class tabButton = NSClassFromString(@"_UITabButton");
    if (!platter || !tabButton) { return; }
    NSArray<NSString *> *titles = ALBDockDisplayTitles();
    NSMutableArray<UIView *> *buttons = [NSMutableArray array];
    NSMutableArray<UIView *> *lensCopies = [NSMutableArray array];
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[platter, @NO]];
    for (NSUInteger q = 0; q < queue.count && q < 400; q++) {
        UIView *root = queue[q][0];
        BOOL inSel = [queue[q][1] boolValue];
        for (UIView *sub in root.subviews) {
            BOOL sel = inSel || [NSStringFromClass(sub.class) containsString:@"SelectedContentView"];
            if ([sub isKindOfClass:tabButton]) { [(sel ? lensCopies : buttons) addObject:sub]; continue; }
            [queue addObject:@[sub, @(sel)]];
        }
    }
    NSMutableIndexSet *done = [NSMutableIndexSet indexSet];
    BOOL got = NO;
    CGFloat sdx = 0, sdy = 0, sh = 0, spad = 0;
    UIFont *sfont = nil;
    UIColor *scolor = nil;
    for (NSArray<UIView *> *list in @[buttons, lensCopies]) {
        for (UIView *b in list) {
            UIView *badge = ALBDockFindSubview(b, ^BOOL(UIView *v) {
                return [NSStringFromClass(v.class) containsString:@"Badge"] && !v.hidden && v.alpha > 0.01 &&
                       CGRectGetWidth(v.bounds) > 4 && CGRectGetHeight(v.bounds) > 4;
            });
            if (!badge) { continue; }
            UIImageView *icon = ALBDockFindSymbolIcon(b);
            UILabel *label = ALBDockFindTitleLabel(b);
            NSUInteger idx = label ? [titles indexOfObject:label.text] : NSNotFound;
            if (!icon || idx == NSNotFound || idx >= self.tabs.count || [done containsIndex:idx]) { continue; }
            CGRect br = [badge convertRect:badge.bounds toView:platter];
            CGPoint ic = [icon convertPoint:CGPointMake(CGRectGetMidX(icon.bounds), CGRectGetMidY(icon.bounds)) toView:platter];
            UILabel *text = [badge isKindOfClass:[UILabel class]] ? (UILabel *)badge
                : (UILabel *)ALBDockFindSubview(badge, ^BOOL(UIView *v) { return [v isKindOfClass:[UILabel class]]; });
            UIFont *font = text.font ?: [UIFont systemFontOfSize:MAX(9, round(CGRectGetHeight(br) * 0.62)) weight:UIFontWeightMedium];
            CGFloat tw = text.text.length ? ceil([text.text sizeWithAttributes:@{NSFontAttributeName: font}].width) : 0;
            CGFloat pad = tw > 0 ? CGRectGetWidth(br) - tw : 8;
            UIColor *color = badge.backgroundColor;
            if (!color || CGColorGetAlpha(color.CGColor) < 0.5) { color = nil; }
            [self.tabs[idx] applyBadgeLeft:CGRectGetMinX(br) - ic.x top:CGRectGetMinY(br) - ic.y
                                    height:CGRectGetHeight(br) pad:pad font:font color:color];
            [done addIndex:idx];
            if (!got) {
                got = YES;
                sdx = CGRectGetMinX(br) - ic.x; sdy = CGRectGetMinY(br) - ic.y; sh = CGRectGetHeight(br); spad = pad;
                sfont = font; scolor = color;
            }
        }
    }
    if (!got) { return; }
    for (NSUInteger i = 0; i < self.tabs.count; i++) {
        if (![done containsIndex:i]) { [self.tabs[i] applyBadgeLeft:sdx top:sdy height:sh pad:spad font:sfont color:scolor]; }
    }
    [self.singleItem applyBadgeLeft:sdx top:sdy height:sh pad:spad font:sfont color:scolor];
    gALBDockBadgeMeasures++;
    gALBDockBadgeSpec = [NSString stringWithFormat:@"左 %.1f 顶 %.1f 高 %.1f 留白 %.1f 字号 %.1f%@",
                         sdx, sdy, sh, spad, sfont.pointSize, scolor ? @"（底色照系统）" : @""];
}

#pragma mark UITabBarDelegate —— 真实点击

- (void)tabBar:(UITabBar *)tabBar didSelectItem:(UITabBarItem *)item {
    [self noteInteraction];
    NSInteger i = item.tag;
    if (i == ALBDockTypePlus) {
        // 拍摄不是页面：走抖音的拍摄回调，透镜弹回当前页
        ALBDockBridgeTapPlus();
        self.nativeSelectedTitle = nil;
        dispatch_async(dispatch_get_main_queue(), ^{ [self syncNative]; });
        return;
    }
    if (i >= 0 && i < ALBDockTypeCount) {
        self.nativeSelectedTitle = ALBDockTabTitles()[i];
        ALBDockBridgeTapTitle(ALBDockTabTitles()[i]);
    }
}

- (void)safeAreaInsetsDidChange {
    [super safeAreaInsetsDidChange];
    [self setNeedsLayout];
}

- (void)didMoveToWindow {
    [super didMoveToWindow];
    [self syncEdge];
    if (self.window) { [self setNeedsLayout]; [self armIdle]; }
}

#pragma mark 点击

- (void)tabTapped:(ALBDockItem *)sender {
    NSInteger idx = [self.tabs indexOfObject:sender];
    if (idx == NSNotFound || self.transitioning) { return; }
    [self noteInteraction];
    if (idx == ALBDockTypePlus) { ALBDockBridgeTapPlus(); return; }
    ALBDockBridgeTapTitle(ALBDockTabTitles()[idx]);
    if (idx == self.selectedIndex) { return; }
    if (self.selectedIndex >= 0) { self.tabs[self.selectedIndex].selectedTab = NO; }
    self.selectedIndex = idx;
    sender.selectedTab = YES;
    CGFloat H = [self barH];
    [UIView animateWithDuration:0.42 delay:0 usingSpringWithDamping:0.78 initialSpringVelocity:0.3
                        options:UIViewAnimationOptionAllowUserInteraction | UIViewAnimationOptionBeginFromCurrentState
                     animations:^{
        self.selPill.center = CGPointMake(sender.center.x, H / 2.0);
    } completion:nil];
}

- (void)anyTapped { [self noteInteraction]; }

// 3.11.45：单独键显示头像时图标是透明的（不吃点击），点击落在球上：展开着就执行单独键的功能
- (void)ballTapped {
    if (self.transitioning) { return; }
    if (self.collapsed) { [self expandToSide:self.side]; return; }
    if (self.singleItem.alpha < 0.01) { [self singleTapped]; }
}

- (void)singleTapped {
    if (self.collapsed) { [self expandToSide:self.side]; return; }
    if (self.transitioning) { return; }
    [self noteInteraction];
    [self performType:self.singleType];   // 默认朋友球 → 抖音的朋友页；3.11.45 可设成任一项
}

- (void)reloadAvatar {
    UIImage *img = [ALBDockAvatar image];
    if (img) {
        self.avatarView.image = img;
        self.avatarView.contentMode = UIViewContentModeScaleAspectFill;
    } else {
        UIImageSymbolConfiguration *cfg = [UIImageSymbolConfiguration configurationWithPointSize:22 weight:UIImageSymbolWeightRegular];
        self.avatarView.image = [UIImage systemImageNamed:@"person.crop.circle.fill" withConfiguration:cfg];
        self.avatarView.contentMode = UIViewContentModeCenter;
    }
}

#pragma mark 闲置计时

- (void)noteInteraction {
    if (self.collapsed || self.transitioning) { return; }
    [self armIdle];
}

- (void)armIdle {
    [self.idleTimer invalidate];
    self.idleTimer = nil;
    if (self.idleDelay <= 0 || self.collapsed || !self.window) { return; }
    __weak typeof(self) ws = self;
    self.idleTimer = [NSTimer timerWithTimeInterval:self.idleDelay repeats:NO block:^(NSTimer *t) {
        __strong typeof(ws) ss = ws;
        if (!ss || ss.collapsed || ss.transitioning) { return; }
        [ss collapseWithCompletion:nil];
    }];
    [[NSRunLoop mainRunLoop] addTimer:self.idleTimer forMode:NSRunLoopCommonModes];
}

- (void)cancelIdle {
    [self.idleTimer invalidate];
    self.idleTimer = nil;
}

#pragma mark 触发（逻辑同原版）

// 上滑：换了一侧才触发（收缩段 + 展开段，速度连续）；同一侧不动；已收缩时直接朝那一侧展开
- (void)swipeUpFromLeft:(BOOL)left {
    if (self.transitioning || self.hidden || self.alpha < 0.01) { return; }
    ALBDockSide target = left ? ALBDockSideLeft : ALBDockSideRight;
    if (self.collapsed) { [self expandToSide:target]; return; }
    if (target == self.side) { [self noteInteraction]; return; }
    [self runCollapse:YES expand:YES target:target completion:nil];
}

- (void)collapseWithCompletion:(void (^)(void))done {
    if (self.collapsed || self.transitioning) { return; }
    [self runCollapse:YES expand:NO target:self.side completion:done];
}

- (void)expandToSide:(ALBDockSide)side {
    if (!self.collapsed || self.transitioning) { return; }
    [self runCollapse:NO expand:YES target:side completion:nil];
}

#pragma mark - 动画（一次算好，交给 Core Animation 播放）

- (void)cachePrefs {
    double v = ALBDockPrefDouble(kALBDockPrefSpeed);
    self.cachedSpeed = v > 0.1 ? v : 1.0;
    self.cachedRedDot = ALBDockPrefBool(kALBDockPrefRedDot) && self.hasUnread;
    self.cachedReverse = YES;   // 3.11.51：展开＝收缩倒放固定开
}

// 模糊兜底（iOS 26 以下）没有融合，才用整块透明度；玻璃从不改 alpha / effect
- (void)fallbackAlpha:(ALBDockGlassGroup *)g value:(CGFloat)a {
    if (!g.glass) { g.alpha = a; }
}

- (void)runCollapse:(BOOL)doC expand:(BOOL)doE target:(ALBDockSide)target completion:(void (^)(void))done {
    ALBDockBuildProfiles();
    [self cachePrefs];
    self.transitioning = YES;
    [self cancelIdle];

    ALBDockGeo g0 = [self geoForSide:self.side];    // 已带系统底栏实测的图标位置
    ALBDockGeo g1 = [self geoForSide:target];
    self.xc = [self ballPoint].x;
    CGFloat ballNow = self.singleGroup.center.x;

    self.planRev = NO;
    if (doC && doE) {            // 换边：一条连续曲线，侧边 → 中心 → 另一侧
        self.planT = kSwapT; self.planDip = YES;
        self.planXs = ballNow; self.planXe = CGRectGetMidX(g1.single);
        self.planTc = gSwapHalfU * kSwapT;
    } else if (doC) {            // 静置收缩：侧边 → 中心，停住
        self.planT = kCollapseT; self.planDip = NO;
        self.planXs = ballNow; self.planXe = self.xc;
        self.planTc = kCollapseT;
    } else {                     // 从中心展开
        // 3.11.47 用户：“展开动画好像没有完全对应收缩动画去倒放”。开着「展开＝收缩倒放」：时长、球的速度曲线、
        // 每一块的形状 / 位置 / 透明度全按收缩那一段倒着放（applyExpandAt 里直接用收缩的算法、时间倒过来）
        self.planRev = self.cachedReverse;
        self.planT = self.planRev ? kCollapseT : kExpandT; self.planDip = NO;
        self.planXs = self.xc; self.planXe = CGRectGetMidX(g1.single);
        self.planTc = 0;
    }
    self.x0 = self.planXs;
    self.ex1 = CGRectGetMidX(g1.single);

    // 交接（同一帧内完成，不做动画）：系统底栏 → 自建可融合玻璃，图标落在系统图标的实测位置上
    [UIView performWithoutAnimation:^{
        if (doC) { [self applyCollapseAt:0 ballX:ballNow geo:g0]; }
        else { [self applyExpandAt:0 ballX:self.xc geo:g1]; }
        [self.mainGroup layoutIfNeeded];
        [self.midGroup layoutIfNeeded];
        if (self.nativeBar) { [self setNativeShown:NO]; }
    }];
    // 系统底栏藏起来之后，再按目标那一侧重排 / 对齐 / 量图标位置（这份开销放在动画开始前）
    if (self.nativeBar && doE) {
        [self layoutNativeForSide:target];
        g1 = [self geoForSide:target];
    }
    double T = self.planT;
    double T1 = self.planTc;
    NSInteger N = MAX(4, (NSInteger)ceil(T * 60.0));
    NSTimeInterval dur = T / self.cachedSpeed;

    __weak typeof(self) ws = self;
    [UIView animateKeyframesWithDuration:dur delay:0
                                 options:UIViewKeyframeAnimationOptionCalculationModeLinear |
                                         UIViewKeyframeAnimationOptionAllowUserInteraction
                              animations:^{
        for (NSInteger k = 1; k <= N; k++) {
            double t = T * (double)k / (double)N;
            [UIView addKeyframeWithRelativeStartTime:(double)(k - 1) / (double)N relativeDuration:1.0 / (double)N animations:^{
                __strong typeof(ws) ss = ws;
                if (!ss) { return; }
                CGFloat x = [ss ballXAt:t];
                if (t < T1 - 1e-9) { [ss applyCollapseAt:t ballX:x geo:g0]; }
                else { [ss applyExpandAt:t - T1 ballX:x geo:g1]; }
                [ss.mainGroup layoutIfNeeded];
                [ss.midGroup layoutIfNeeded];
            }];
        }
    } completion:^(BOOL finished) {
        __strong typeof(ws) ss = ws;
        if (!ss) { return; }
        if (doE) {
            ss.side = target;
            ss.collapsed = NO;
            ss.transitioning = NO;
            [ss applySteadyState];     // 自建主栏藏回球底下，系统底栏（原生透镜）在同一帧接管
            ss.singleItem.alpha = ss.avatarSingle ? 0 : 1;
            ss.singleItem.transform = [ss itemScale:1];
            ss.avatarView.alpha = ss.avatarSingle ? 1 : 0;
            ss.avatarView.transform = CGAffineTransformIdentity;
            ss.redDot.alpha = 0;
            [ss fallbackAlpha:ss.mainGroup value:1];
            [ss fallbackAlpha:ss.midGroup value:ss.midOn ? 1 : 0];
            if (ss.nativeBar) { [ss parkMainUnderX:ss.singleGroup.center.x]; }
            [ss armIdle];   // 0x3037A0：展开后重新挂闲置计时器
        } else {
            ss.collapsed = YES;
            ss.transitioning = NO;
            [ss applySteadyState];
            ss.singleItem.alpha = 0;
            ss.singleItem.transform = [ss itemScale:1];
            ss.avatarView.alpha = 1;
            ss.avatarView.transform = CGAffineTransformIdentity;
            ss.redDot.alpha = ss.cachedRedDot ? 1 : 0;
        }
        if (ss.materialPending) {                           // 动画期间切了页面 / 系统深浅：现在补换材质
            [ss materialChanged];
            for (ALBDockGlassGroup *g in @[ss.mainGroup, ss.midGroup, ss.singleGroup]) { [g rematerialize]; }
        }
        if (ss.itemsPending) { [ss reloadNativeItems]; }    // 3.11.33：动画期间改了图标 / 3.11.45 项目：现在补建系统底栏的项
        if (done) { done(); }
    }];
}

#pragma mark 收缩段（侧边 → 中间）

- (CGFloat)ballXAt:(double)t {
    double u = self.planT > 0 ? t / self.planT : 1;
    // 倒放：收缩的曲线时间倒过来（起步慢、到边上快，正好是收缩“起步快、进中心慢”倒着看）
    double P = self.planRev ? 1.0 - ALBDockProfileAt(gProfPlain, 1.0 - u)
                            : ALBDockProfileAt(self.planDip ? gProfSwap : gProfPlain, u);
    return ALBDockLerp(self.planXs, self.planXe, P);
}

- (void)applyCollapseAt:(double)t ballX:(CGFloat)x geo:(ALBDockGeo)g {
    CGFloat R = [self R];
    double tc = MAX(0.05, self.planTc);
    self.singleGroup.center = CGPointMake(x, [self barCenterY]);

    // 球图标：原地换，不位移（时间按收缩段长度缩放）；3.11.45 单独键本来就是头像：不换
    CGFloat kf = ALBDockSmooth(t / (0.5 * tc));
    CGFloat ka = ALBDockSmooth((t - 0.3 * tc) / (0.6 * tc));
    if (self.avatarSingle) {
        self.singleItem.alpha = 0;
        self.avatarView.alpha = 1;
        self.avatarView.transform = CGAffineTransformIdentity;
    } else {
        self.singleItem.alpha = 1 - kf;
        self.singleItem.transform = [self itemScale:1 - 0.30 * kf];
        self.avatarView.alpha = ka;
        self.avatarView.transform = CGAffineTransformMakeScale(0.84 + 0.16 * ka, 0.84 + 0.16 * ka);
    }
    self.redDot.alpha = self.cachedRedDot ? ka : 0;

    // 中栏：被路过的球吸进去（缩到球的位置和大小，由玻璃容器融合）；3.11.45 不显示中栏时不动它
    if (self.midOn) { [self collapseMidAt:x geo:g R:R]; }

    // 主栏（2.3.0）：进度与球完全同步 —— 球从一侧来，主栏外侧边缘从另一侧以同样的曲线、同样的行程（都是 175pt）
    // 镜像地向中心合拢，两头同时到达，像幕布从两边合上；y 不动
    // tab 被合拢过来的边缘依次「吃掉」：最外侧先淡出，最内侧的「我」最后，避免出现大块空玻璃
    CGRect M = g.main;
    CGFloat pc = ALBDockClamp01((x - self.x0) / ((self.xc - self.x0) ?: 1));
    // 最后 14% 行程主栏连宽带高缩进球心（到达时缩到 40%，藏在头像底下，结束后挪出屏幕）
    CGFloat km = 1 - ALBDockSmooth((pc - 0.86) / 0.14);
    [self setMainX0:ALBDockLerp(CGRectGetMinX(M), self.xc - R, pc) x1:ALBDockLerp(CGRectGetMaxX(M), self.xc + R, pc) k:km];
    for (NSNumber *n in self.mainTypes) {
        NSUInteger i = n.unsignedIntegerValue;
        [self placeTab:i absX:ALBDockLerp(g.tabX[i], self.xc, pc * 0.9)
                 alpha:1 - ALBDockSmooth((pc - (0.06 + 0.15 * g.rank[i])) / 0.30)
                 scale:1 - 0.25 * pc];
    }
    [self fallbackAlpha:self.mainGroup value:1 - ALBDockSmooth((pc - 0.4) / 0.6)];
}

- (void)collapseMidAt:(CGFloat)x geo:(ALBDockGeo)g R:(CGFloat)R {
    CGRect mh = g.mid;
    CGFloat dir = self.xc >= self.x0 ? 1 : -1;
    CGFloat u = (x - self.x0) * dir;
    CGFloat ea = (CGRectGetMinX(mh) - self.x0) * dir, eb = (CGRectGetMaxX(mh) - self.x0) * dir;
    CGFloat a = MIN(ea, eb), b = MAX(ea, eb);
    CGFloat q = ALBDockClamp01((u - (a - R)) / (((a + b) / 2.0) - (a - R)));
    CGFloat e = ALBDockSmooth(q);
    // 完全吸进球之后（球心过了中栏中心），在球里面继续缩到 40%（被球 / 头像盖住）
    CGFloat kd = 1 - ALBDockSmooth((u - (a + b) / 2.0) / (1.2 * R));
    [self setMidX0:ALBDockLerp(CGRectGetMinX(mh), x - R, e) x1:ALBDockLerp(CGRectGetMaxX(mh), x + R, e)
         iconAlpha:1 - ALBDockSmooth(q / 0.5) iconScale:1 - 0.25 * e k:kd];
    [self fallbackAlpha:self.midGroup value:1 - q];
}

// 收缩态：中栏 / 主栏挪到屏幕外（不隐藏、不切 effect），球只剩自己一层玻璃
- (void)parkUnderBall {
    [self parkMainUnderX:0];
    [self parkMid];
}

#pragma mark 展开段（中间 → 侧边）

- (void)applyExpandAt:(double)t ballX:(CGFloat)x geo:(ALBDockGeo)g {
    if (self.planRev) {
        // 3.11.47：收缩段倒着放 —— 起点当成目标那一侧，时间从收缩结束往回走
        double seg = MAX(0.05, self.planT - self.planTc);
        CGFloat savedX0 = self.x0;
        double savedTc = self.planTc;
        self.x0 = self.ex1;
        self.planTc = seg;
        [self applyCollapseAt:MAX(0.0, seg - t) ballX:x geo:g];
        self.x0 = savedX0;
        self.planTc = savedTc;
        return;
    }
    CGFloat R = [self R];
    self.singleGroup.center = CGPointMake(x, [self barCenterY]);

    // 球图标：按球的位置，在最后 30% 行程换回朋友（原地换）；3.11.45 单独键显示头像：不换
    CGFloat pe = ALBDockClamp01((x - self.xc) / ((self.ex1 - self.xc) ?: 1));
    CGFloat ka = ALBDockSmooth((pe - 0.55) / 0.25);
    self.redDot.alpha = self.cachedRedDot ? 1 - ka : 0;
    if (self.avatarSingle) {
        self.avatarView.alpha = 1;
        self.avatarView.transform = CGAffineTransformIdentity;
        self.singleItem.alpha = 0;
    } else {
        self.avatarView.alpha = 1 - ka;
        self.avatarView.transform = CGAffineTransformMakeScale(1 - 0.16 * ka, 1 - 0.16 * ka);
        CGFloat kf = ALBDockSmooth((pe - 0.68) / 0.27);
        self.singleItem.alpha = kf;
        self.singleItem.transform = [self itemScale:0.70 + 0.30 * kf];
    }

    // 主栏（2.3.0）：与球同进度镜像张开 —— 球往一侧走，主栏外侧边缘往另一侧以同样的曲线展开
    // tab 从最内侧（我）开始依次出现
    CGRect M = g.main;
    CGFloat m = pe;
    // 从球心 40% 大小长出来（前 14% 行程长到全高），和收缩段对称
    CGFloat km = ALBDockSmooth(m / 0.14);
    [self setMainX0:ALBDockLerp(self.xc - R, CGRectGetMinX(M), m) x1:ALBDockLerp(self.xc + R, CGRectGetMaxX(M), m) k:km];
    CGFloat last = (CGFloat)MAX((NSInteger)self.mainTypes.count - 1, 0);
    for (NSNumber *n in self.mainTypes) {
        NSUInteger i = n.unsignedIntegerValue;
        CGFloat inner = last - g.rank[i];   // 0 = 最内侧
        [self placeTab:i absX:ALBDockLerp(self.xc, g.tabX[i], m)
                 alpha:ALBDockSmooth((m - (0.18 + 0.15 * inner)) / 0.30)
                 scale:0.75 + 0.25 * m];
    }
    [self fallbackAlpha:self.mainGroup value:ALBDockSmooth(m / 0.6)];

    if (!self.midOn) { return; }
    CGRect mh = g.mid;
    CGFloat dir = self.ex1 >= self.xc ? 1 : -1;   // 中栏：位置驱动
    CGFloat u = (x - self.xc) * dir;
    CGFloat en = (CGRectGetMinX(mh) - self.xc) * dir, ef = (CGRectGetMaxX(mh) - self.xc) * dir;
    CGFloat n = MIN(en, ef), f = MAX(en, ef);
    CGFloat q = ALBDockClamp01((u - (n + R * 0.2)) / ((f + R) - (n + R * 0.2)));
    CGFloat e = ALBDockSmooth(q);
    // 2.5.0：留下来之前，先在球里从点长回球那么大（被球盖住看不见）
    CGFloat kg = ALBDockSmooth((u - (n + R * 0.2 - 1.2 * R)) / (1.2 * R));
    [self setMidX0:ALBDockLerp(x - R, CGRectGetMinX(mh), e) x1:ALBDockLerp(x + R, CGRectGetMaxX(mh), e)
         iconAlpha:ALBDockSmooth((q - 0.4) / 0.6) iconScale:0.75 + 0.25 * e k:kg];
    [self fallbackAlpha:self.midGroup value:e];
}

#pragma mark - 探测（2.6.0，只读）

static void ALBDockDumpTree(UIView *v, UIView *ref, NSUInteger depth, NSMutableString *o, NSUInteger *lines) {
    if (!v || depth > 9 || *lines > 160) { return; }
    (*lines)++;
    NSMutableString *line = [NSMutableString string];
    for (NSUInteger i = 0; i < depth; i++) { [line appendString:@"  "]; }
    CGRect r = [v convertRect:v.bounds toView:ref];
    [line appendFormat:@"%@ (%.1f,%.1f,%.1f×%.1f)", NSStringFromClass(v.class), r.origin.x, r.origin.y, r.size.width, r.size.height];
    if (v.hidden) { [line appendString:@" 隐藏"]; }
    if (v.alpha < 0.99) { [line appendFormat:@" alpha=%.2f", v.alpha]; }
    if ([v isKindOfClass:[UIVisualEffectView class]]) {
        UIVisualEffect *e = ((UIVisualEffectView *)v).effect;
        [line appendFormat:@" effect=%@", e ? NSStringFromClass(e.class) : @"nil"];
    }
    // 3.11.44：深浅（自己设的 / 实际继承到的；0 没设 1 浅 2 深）
    if ([v isKindOfClass:[UIVisualEffectView class]] || [v respondsToSelector:NSSelectorFromString(@"glassEffect")] || depth == 0) {
        [line appendFormat:@" 深浅=%ld/%ld", (long)v.overrideUserInterfaceStyle, (long)v.traitCollection.userInterfaceStyle];
    }
    if ([v respondsToSelector:NSSelectorFromString(@"glassEffect")]) {
        id g = nil;
        @try { g = [v valueForKey:@"glassEffect"]; } @catch (__unused NSException *e) {}
        [line appendFormat:@" glassEffect=%@", g ? NSStringFromClass([g class]) : @"nil"];
    }
    if (v.interactions.count) {
        NSMutableArray *names = [NSMutableArray array];
        for (id<UIInteraction> it in v.interactions) { [names addObject:NSStringFromClass([it class])]; }
        [line appendFormat:@" interactions=%@", [names componentsJoinedByString:@","]];
    }
    if ([v isKindOfClass:[UILabel class]] && ((UILabel *)v).text.length) {
        UILabel *l = (UILabel *)v;
        [line appendFormat:@" \"%@\" %.1fpt", l.text, l.font.pointSize];
    }
    if ([v isKindOfClass:[UIImageView class]] && ((UIImageView *)v).image.isSymbolImage) {
        [line appendFormat:@" 符号 %@", ((UIImageView *)v).image.symbolConfiguration.description ?: @""];
    }
    [o appendFormat:@"%@\n", line];
    for (UIView *sub in v.subviews) { ALBDockDumpTree(sub, ref, depth + 1, o, lines); }
}

- (NSString *)debugReport {
    NSMutableString *o = [NSMutableString string];
    CGRect mainF, midF, singleF;
    [self framesForSide:self.side main:&mainF mid:&midF single:&singleF];
    [o appendFormat:@"底栏：高 %.0f（系统胶囊原高 %.1f，缩放 %.3f）中栏宽 %.0f 间距 %.0f / %.0f 主栏宽 %.0f %@ %@\n",
     [self barH], [self platterH], [self itemK], midF.size.width, CGRectGetMinX(midF) - CGRectGetMaxX(mainF),
     CGRectGetMinX(singleF) - CGRectGetMaxX(midF), mainF.size.width,
     self.side == ALBDockSideRight ? @"单独键在右" : @"单独键在左", self.collapsed ? @"收缩" : @"展开"];
    // 3.11.45
    NSArray<NSString *> *names5 = ALBDockDisplayTitles();
    NSMutableArray<NSString *> *mainNames = [NSMutableArray array];
    for (NSNumber *t in self.mainTypes) { [mainNames addObject:names5[t.integerValue]]; }
    NSMutableArray<NSString *> *orderNames = [NSMutableArray array];
    for (NSNumber *t in ALBDockTypeOrder()) {
        [orderNames addObject:[NSString stringWithFormat:@"%@%@", names5[t.integerValue], ALBDockTypeHidden(t.integerValue) ? @"(藏)" : @""]];
    }
    [o appendFormat:@"项目：顺序 %@；主栏 %@；单独键=%@%@；中间胶囊=%@；↑=%@；头像内边距 %.0f；单个项目宽度 %@\n"
                     "玻璃换种类 %lu 次、整只重装 %lu 次\n",
     [orderNames componentsJoinedByString:@" "], [mainNames componentsJoinedByString:@" "], names5[self.singleType],
     self.avatarSingle ? @"（显示头像）" : @"", self.midOn ? @"显示" : @"不显示",
     self.arrowKind == 0 ? @"快捷跳转面板" : ALBDockHostQuickJumpTitle(self.arrowKind), self.avatarInset,
     ALBDockPrefDouble(kALBDockPrefTabWidth) >= 30 ? [NSString stringWithFormat:@"%.0f", ALBDockPrefDouble(kALBDockPrefTabWidth)] : @"自动",
     (unsigned long)gALBDockEffectSwaps, (unsigned long)gALBDockRematerials];
    [o appendFormat:@"角标照系统：量了 %lu 次（%@）；红点：开关=%@ 有未读=%@%@ 现在=%@；展开＝收缩倒放=%@\n",
     (unsigned long)gALBDockBadgeMeasures, gALBDockBadgeSpec ?: @"还没量到：系统底栏上还没出现过角标",
     ALBDockPrefBool(kALBDockPrefRedDot) ? @"开" : @"关", self.hasUnread ? @"是" : @"否",
     self.unreadSource.length ? [NSString stringWithFormat:@"（%@）", self.unreadSource] : @"",
     self.redDot.alpha > 0.5 ? @"亮" : @"不亮", @"开（固定）"];
    [o appendFormat:@"材质：%@%@ 透明=%@ 系统深浅=%@ 透镜实测=%@\n", ALBDockSceneIsVideo() ? @"视频页" : @"非视频页",
     ALBDockSceneIsLight() ? @"（背后亮→图标变黑，玻璃仍深色）" : @"",
     ALBDockGlassUsesClear() ? @"是" : @"否", ALBDockSystemStyle() == UIUserInterfaceStyleDark ? @"深" : @"浅",
     NSStringFromCGSize(self.nativeLensSize)];
    // 3.11.33
    NSMutableArray<NSString *> *syms = [NSMutableArray array];
    NSArray<NSString *> *names = ALBDockDisplayTitles();
    for (NSUInteger i = 0; i < self.tabs.count && i < names.count; i++) {
        [syms addObject:[NSString stringWithFormat:@"%@=%@", names[i], self.tabs[i].symbol ?: @"-"]];
    }
    [syms addObject:[NSString stringWithFormat:@"单独键=%@", self.singleItem.symbol ?: @"-"]];
    CFTimeInterval age = self.lumaAt > 0 ? CACurrentMediaTime() - self.lumaAt : -1;
    [o appendFormat:@"图标（液态底栏的符号）：%@；选中色=%@\n背后亮度：%@ %@（%@，读 %lu 次，换过 %lu 次）阈值 %.2f / %.2f 探针=%@\n",
     [syms componentsJoinedByString:@" "],
     ALBDockPrefBool(kALBDockPrefAccent) ? (ALBDockHostAccentColor() ? @"液态底栏强调色" : @"读不到强调色→抖音红") : @"抖音红",
     @"开（只切图标）",
     isfinite(self.lumaLast) ? [NSString stringWithFormat:@"%.3f", self.lumaLast] : @"还没有读数",
     age < 0 ? @"-" : [NSString stringWithFormat:@"%.1f 秒前", age],
     (unsigned long)self.lumaReads, (unsigned long)self.lumaFlips,
     kALBDockInkLightAt, kALBDockInkLightAt - 0.08,
     self.lumaProbe ? (self.lumaProbe.window ? @"在" : @"不在窗口里") : @"无（系统没有 CABackdropLayer）"];
    // 3.11.44：左边系统胶囊和右边两块是不是同一份玻璃、同一个深浅
    UIView *platter = self.nativeBar ? ALBDockFindPlatter(self.nativeBar) : nil;
    id platterGlass = nil;
    if (platter) { @try { platterGlass = [platter valueForKey:@"glassEffect"]; } @catch (__unused NSException *e) {} }
    NSString *(^styleName)(UIUserInterfaceStyle) = ^NSString *(UIUserInterfaceStyle st) {
        return st == UIUserInterfaceStyleDark ? @"深" : (st == UIUserInterfaceStyleLight ? @"浅" : @"没设");
    };
    [o appendFormat:@"玻璃同源（3.11.44）：系统胶囊原厂玻璃=%@；自建三块用原厂同款 %lu 次；透明时胶囊被换回原厂再装 %lu 次（核对%@）；现在 胶囊=%@ 中栏=%@ 朋友球=%@\n"
                     "深浅：应为%@ 系统底栏设的=%@ 胶囊实际=%@ 中栏设的/实际=%@/%@ 朋友球设的/实际=%@/%@\n深浅变化（最近的在后）：%@\n",
     gALBDockNativeGlassInfo ?: @"还没见到系统胶囊", (unsigned long)gALBDockNativeGlassUses, (unsigned long)gALBDockPlatterReverted,
     gALBDockPlatterVerify < 0 ? @"停了：读回的玻璃不带染色" : (gALBDockPlatterVerify > 0 ? @"能用" : @"还没换过"),
     platterGlass ? NSStringFromClass([platterGlass class]) : @"-",
     self.midGroup.effect ? NSStringFromClass(self.midGroup.effect.class) : @"nil",
     self.singleGroup.effect ? NSStringFromClass(self.singleGroup.effect.class) : @"nil",
     styleName(ALBDockGlassStyle()), styleName(self.nativeBar.overrideUserInterfaceStyle),
     platter ? styleName(platter.traitCollection.userInterfaceStyle) : @"-",
     styleName(self.midGroup.overrideUserInterfaceStyle), styleName(self.midGroup.traitCollection.userInterfaceStyle),
     styleName(self.singleGroup.overrideUserInterfaceStyle), styleName(self.singleGroup.traitCollection.userInterfaceStyle),
     ALBDockMaterialLogReport()];
    [o appendFormat:@"融合容器：%@ spacing=4 系统底栏在容器里=%@\n",
     self.container ? NSStringFromClass(self.container.effect.class) : @"无",
     (self.nativeBar && self.nativeBar.superview == self.stack && self.container) ? @"是" : @"否"];
    if (self.nativeBar) {
        [o appendString:@"系统底栏结构（坐标相对 UITabBar）：\n"];
        NSUInteger lines = 0;
        ALBDockDumpTree(self.nativeBar, self.nativeBar, 0, o, &lines);
    }
    return o;
}

- (CGRect)backdropRectInWindow {
    if (!self.window || self.hidden || self.alpha < 0.01) { return CGRectNull; }
    CGRect r = CGRectNull;
    for (UIView *v in @[self.mainGroup, self.midGroup, self.singleGroup]) {
        if (!v || v.hidden || v.alpha < 0.01 || CGRectIsEmpty(v.bounds)) { continue; }
        CGRect wr = [v convertRect:v.bounds toView:nil];
        if (!CGRectIntersectsRect(wr, self.window.bounds)) { continue; }    // 3.11.45：挪到屏幕外的不算
        r = CGRectUnion(r, wr);
    }
    if (self.nativeBar && self.nativeShown) {
        UIView *platter = ALBDockFindPlatter(self.nativeBar);
        if (platter.window) { r = CGRectUnion(r, [platter convertRect:platter.bounds toView:nil]); }
    }
    return r;
}

- (void)dealloc {
    [self.idleTimer invalidate];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
