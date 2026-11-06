/* 探针 · 采集模式 (3.11.35). Included once from ALBBackgroundProbe.h, after
   the candidate helpers and before the probe controller; plain Objective-C.

   用户：下一步要给更多地方（长按、聊天页、评论区里的长按页等，参考另一个插件已经盖了玻璃背景的那些
   按钮 / 胶囊）做“真的”玻璃——像右侧音乐、头像那样，把上面实际的图标、文字和玻璃绑在一起一起变形，
   iOS 26 用常规玻璃。这一版先升级探针去把这些地方的实际图标、文字层抓出来：
   · 采集模式开着时只有探针收触摸（抖音收不到，也就没有它自己的点按反馈）；点一下抓一个：
     从点到的最前面那一层往上找“按钮 / 胶囊”（UIControl、类名像按钮 / 格子 / 胶囊、带点击手势、或有圆角
     并有底色 / 毛玻璃的小块），把它里面所有图标（图片视图、带内容的图层、形状图层、动画视图）和文字
     （UILabel、CATextLayer）一起记下；屏幕上用线框框住（胶囊粗框＋编号，图标细白框，文字细黄框），
     框跟着视图走。抓错了可以“扩大 / 缩小”一层或“撤销”
   · “暂停”让触摸回到抖音（切页面），“继续”再抓；“记下页面”记当前页面的控制器链和页面里看起来像跳转
     地址的字符串（给中间那颗双按钮胶囊做快捷跳转用）；“打包本段”把这一段导出成 JSON（剪贴板＋
     Documents/AwemeLiquidBar_Capture_段号.json），然后清空、段号加一
   每一项都记：类名、来自哪个模块（抖音自己的还是别的插件的）、窗口坐标和在胶囊里的坐标、圆角、底色、
   控制器链、点击目标（UIControl 的 target/action、手势类）、规则定位（锚点类＋路径，同背景探针），
   胶囊里 / 旁边的玻璃（UIVisualEffectView 的效果类和所属模块），以及点到的位置上前 12 层。

   3.11.37（看了 3.11.36 抓回来的 7 段）：
   · 点到的是谁：先问 UIKit 自己的命中测试（hitTest，和真的手指一样会穿过 iOS 26 那几层透明的悬浮栏容器），
     从命中的视图里取最前面那层；命中不到才按几何找，并跳过那几层容器。JSON 多一项 hitTest
   · “拆开”：把框住的整块拆成里面每颗按钮 / 每个格子，各记一项（origin = 拆开自 #n）
   · 按钮外面、同一个格子里的文字记成 textsNearby；YYLabel 这类自己画字的标签按文字记（kind 自绘文字）
   · 模块：包里的 AwemeCore 等都记“抖音”；“旁边的玻璃”不再把系统容器、抖音自己的视图算进去
   · 记下页面：多记页面顶部看得见的文字（visibleTitles）；跳转字符串再往参数对象里看一层 */

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

static NSString *const kALBCaptureStageKey = @"AwemeLiquidBar.probe.captureStage";

@interface ALBCapturePart : NSObject
@property(nonatomic, weak) UIView *view;      /* the view, or the host of `layer` */
@property(nonatomic, weak) CALayer *layer;    /* a non-view sublayer (icon / text layer) */
@property(nonatomic, copy) NSString *role;    /* icon / text / glass */
@property(nonatomic, strong) UIView *box;
@end
@implementation ALBCapturePart
@end

@interface ALBCaptureItem : NSObject
@property(nonatomic) NSUInteger number;
@property(nonatomic, copy) NSString *kind;            /* element / page */
@property(nonatomic) CGPoint point;
@property(nonatomic, strong) NSPointerArray *chain;   /* weak: front-most view upwards */
@property(nonatomic) NSUInteger level;                /* the capsule = chain[level] */
@property(nonatomic, strong) NSMutableArray<ALBCapturePart *> *parts;
@property(nonatomic, copy) NSDictionary *snapshot;    /* JSON, made when captured / re-levelled */
@property(nonatomic, copy) NSString *summary;
@property(nonatomic, strong) UIColor *color;
@property(nonatomic, strong) UIView *box;
@property(nonatomic, strong) UILabel *badge;
@property(nonatomic, copy) NSString *hitName;         /* 3.11.37: what UIKit itself would hand the touch to */
@property(nonatomic, copy) NSString *origin;          /* 3.11.37: "点按" / "拆开自 #n" */
@end
@implementation ALBCaptureItem
- (UIView *)capsule {
    return self.level < self.chain.count ? (__bridge UIView *)[self.chain pointerAtIndex:self.level] : nil;
}
@end

static NSMutableArray<ALBCaptureItem *> *gALBCaptureItems;
static NSUInteger gALBCaptureNumber;

#pragma mark - Reading helpers

/* The binary a class comes from: "抖音" for the app itself, else the file name (another plug-in, a framework).
   3.11.37：抖音的代码几乎都在包里的 AwemeCore.framework，3.11.35/36 只认主程序，所以抖音的类全记成了
   “AwemeCore”，“旁边的玻璃”又把“不是抖音”的都当成别的插件 —— 输入框、关闭层都被列成了玻璃。
   现在包里（…/Aweme.app/ 下）的都算抖音；包里单独的 .dylib 是注入进去的插件，照旧记文件名。 */
static NSString *ALBCaptureModule(Class cls) {
    const char *image = cls ? class_getImageName(cls) : NULL;
    if (!image) return @"?";
    NSString *path = [NSString stringWithUTF8String:image];
    static NSString *main, *appFolder;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        main = NSBundle.mainBundle.executablePath;
        NSString *app = NSBundle.mainBundle.bundlePath.lastPathComponent;
        if (app.length) appFolder = [NSString stringWithFormat:@"/%@/", app];
    });
    if (main && [path isEqualToString:main]) return @"抖音";
    if (appFolder && [path containsString:appFolder] && ![path.pathExtension isEqualToString:@"dylib"]) return @"抖音";
    if ([path hasPrefix:@"/System/"] || [path hasPrefix:@"/usr/lib/"]) return @"系统";
    return path.lastPathComponent ?: @"?";
}

static NSArray<NSString *> *ALBCaptureControllers(UIResponder *start, NSUInteger limit) {
    NSMutableArray<NSString *> *names = [NSMutableArray array];
    for (UIResponder *r = start; r && names.count < limit && ![r isKindOfClass:UIWindow.class]; r = r.nextResponder)
        if ([r isKindOfClass:UIViewController.class]) [names addObject:NSStringFromClass([r class])];
    return names;
}

static NSString *ALBCaptureColor(UIColor *color, UIView *traitSource) {
    if (!color) return nil;
    UIColor *resolved = traitSource ? [color resolvedColorWithTraitCollection:traitSource.traitCollection] : color;
    return ALBProbeColorText(resolved.CGColor);
}

/* "symbol(system: heart.fill)" in a UIImage description -> "heart.fill". */
static NSString *ALBCaptureSymbolName(UIImage *image) {
    if (!image.isSymbolImage) return nil;
    NSString *text = image.description;
    NSRange start = [text rangeOfString:@"system: "];
    if (start.location == NSNotFound) start = [text rangeOfString:@"named("];
    if (start.location == NSNotFound) return @"(符号)";
    NSUInteger from = NSMaxRange(start);
    NSRange end = [text rangeOfString:@")" options:0 range:NSMakeRange(from, text.length - from)];
    return end.location == NSNotFound ? @"(符号)" : [text substringWithRange:NSMakeRange(from, end.location - from)];
}

static NSDictionary *ALBCaptureImageJSON(UIImage *image, UIView *traitSource) {
    if (!image) return @{};
    NSMutableDictionary *d = [NSMutableDictionary dictionary];
    d[@"size"] = @{@"w": @(ALBProbeNumber(image.size.width)), @"h": @(ALBProbeNumber(image.size.height))};
    d[@"scale"] = @(image.scale);
    d[@"rendering"] = image.renderingMode == UIImageRenderingModeAlwaysTemplate ? @"模板"
        : (image.renderingMode == UIImageRenderingModeAlwaysOriginal ? @"原色" : @"自动");
    NSString *symbol = ALBCaptureSymbolName(image);
    if (symbol) d[@"symbol"] = symbol;
    if (image.images.count) d[@"frames"] = @(image.images.count);
    (void)traitSource;
    return d;
}

static NSArray<NSString *> *ALBCaptureActions(UIView *view) {
    NSMutableArray<NSString *> *out = [NSMutableArray array];
    if ([view isKindOfClass:UIControl.class]) {
        UIControl *control = (UIControl *)view;
        for (id target in control.allTargets) {
            NSArray *actions = [control actionsForTarget:target forControlEvent:UIControlEventTouchUpInside];
            for (NSString *action in actions) [out addObject:[NSString stringWithFormat:@"%@ %@", NSStringFromClass([target class]), action]];
            if (!actions.count) [out addObject:[NSString stringWithFormat:@"%@（其他事件）", NSStringFromClass([target class])]];
        }
    }
    for (UIGestureRecognizer *g in view.gestureRecognizers) {
        if (!g.enabled) continue;
        [out addObject:[NSString stringWithFormat:@"手势 %@", NSStringFromClass(g.class)]];
    }
    return out;
}

static NSMutableDictionary *ALBCaptureViewJSON(UIView *view, UIView *capsule) {
    NSMutableDictionary *d = [NSMutableDictionary dictionary];
    if (!view) return d;
    d[@"class"] = NSStringFromClass([view class]);
    d[@"module"] = ALBCaptureModule([view class]);
    d[@"frameInWindow"] = ALBProbeRectJSON([view convertRect:view.bounds toView:nil]);
    if (capsule && capsule != view) d[@"frameInCapsule"] = ALBProbeRectJSON([view convertRect:view.bounds toView:capsule]);
    d[@"alpha"] = @(ALBProbeNumber(view.alpha));
    if (view.hidden) d[@"hidden"] = @YES;
    d[@"cornerRadius"] = @(ALBProbeNumber(view.layer.cornerRadius));
    if (view.clipsToBounds) d[@"clips"] = @YES;
    NSString *bg = view.backgroundColor ? ALBProbeViewColorText(view) : nil;
    if (bg) d[@"background"] = bg;
    if (view.layer.backgroundColor && !ALBProbeColorIsClear(view.layer.backgroundColor))
        d[@"layerBackground"] = ALBProbeColorText(view.layer.backgroundColor);
    d[@"layerClass"] = NSStringFromClass([view.layer class]);
    if (!view.userInteractionEnabled) d[@"touches"] = @NO;
    if (view.accessibilityLabel.length) d[@"accessibilityLabel"] = view.accessibilityLabel;
    if (view.accessibilityIdentifier.length) d[@"accessibilityIdentifier"] = view.accessibilityIdentifier;
    if (view.tintColor && [view isKindOfClass:UIImageView.class]) d[@"tint"] = ALBCaptureColor(view.tintColor, view);
    NSArray *actions = ALBCaptureActions(view);
    if (actions.count) d[@"taps"] = actions;
    if ([view isKindOfClass:UIControl.class]) {
        UIControl *control = (UIControl *)view;
        d[@"control"] = @{@"enabled": @(control.enabled), @"selected": @(control.selected), @"highlighted": @(control.highlighted)};
    }
    if ([view isKindOfClass:UIVisualEffectView.class]) {
        UIVisualEffect *effect = ((UIVisualEffectView *)view).effect;
        d[@"effect"] = effect ? NSStringFromClass(effect.class) : @"无";
    }
    d[@"superviewChain"] = ALBProbeChain(view.superview, 4);
    d[@"controllers"] = ALBCaptureControllers(view, 3);
    NSMutableDictionary *identity = [NSMutableDictionary dictionary];
    NSString *error = ALBProbeIdentity(view, identity);
    d[@"ruleIdentity"] = error ? @{@"error": error} : identity;
    return d;
}

#pragma mark - Finding the capsule and its contents

static BOOL ALBCaptureNameLooksLikeButton(NSString *name) {
    static NSRegularExpression *pattern;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        pattern = [NSRegularExpression regularExpressionWithPattern:@"(Button|Btn|Cell|Item|Capsule|Pill|Chip|Tab|Entrance|Bubble|Action|Option)"
                                                            options:NSRegularExpressionCaseInsensitive error:nil];
    });
    return name.length && [pattern firstMatchInString:name options:0 range:NSMakeRange(0, name.length)] != nil;
}

static BOOL ALBCaptureHasTap(UIView *view) {
    for (UIGestureRecognizer *g in view.gestureRecognizers)
        if (g.enabled && ([g isKindOfClass:UITapGestureRecognizer.class] || [g isKindOfClass:UILongPressGestureRecognizer.class])) return YES;
    return NO;
}

/* 2 = clearly a button / capsule, 1 = maybe (rounded with a fill or an effect, or tappable), 0 = no. */
static NSInteger ALBCaptureCapsuleScore(UIView *view) {
    if ([view isKindOfClass:UIWindow.class]) return 0;
    if ([view isKindOfClass:UIControl.class] || [view isKindOfClass:UITableViewCell.class] ||
        [view isKindOfClass:UICollectionViewCell.class]) return 2;
    if (!ALBProbeClassIsSystem([view class]) && ALBCaptureNameLooksLikeButton(NSStringFromClass([view class]))) return 2;
    if (ALBCaptureHasTap(view)) return 1;
    BOOL filled = (view.backgroundColor && CGColorGetAlpha(view.backgroundColor.CGColor) > 0.02) ||
                  (view.layer.backgroundColor && CGColorGetAlpha(view.layer.backgroundColor) > 0.02) ||
                  [view isKindOfClass:UIVisualEffectView.class];
    if (view.layer.cornerRadius >= 6 && filled) return 1;
    return 0;
}

/* From the front-most view under the finger up to (not including) anything as big as 60 % of the window. */
static NSPointerArray *ALBCaptureChainFrom(UIView *front) {
    NSPointerArray *chain = [NSPointerArray weakObjectsPointerArray];
    UIWindow *window = front.window;
    CGFloat limit = window ? CGRectGetWidth(window.bounds) * CGRectGetHeight(window.bounds) * 0.6 : CGFLOAT_MAX;
    for (UIView *node = front; node && ![node isKindOfClass:UIWindow.class] && chain.count < 14; node = node.superview) {
        if (chain.count && node.bounds.size.width * node.bounds.size.height >= limit) break;
        [chain addPointer:(__bridge void *)node];
    }
    return chain;
}

static NSUInteger ALBCaptureDefaultLevel(NSPointerArray *chain) {
    NSUInteger maybe = NSNotFound;
    for (NSUInteger i = 0; i < chain.count; i++) {
        UIView *view = (__bridge UIView *)[chain pointerAtIndex:i];
        if (!view) continue;
        // A cell's content view stands for the cell.
        NSInteger score = ALBCaptureCapsuleScore(view);
        if (score == 2) return i;
        if (score == 1 && maybe == NSNotFound) maybe = i;
    }
    if (maybe != NSNotFound) return maybe;
    UIView *front = chain.count ? (__bridge UIView *)[chain pointerAtIndex:0] : nil;
    BOOL leaf = [front isKindOfClass:UIImageView.class] || [front isKindOfClass:UILabel.class];
    return leaf && chain.count > 1 ? 1 : 0;
}

static BOOL ALBCaptureVisible(UIView *view) {
    return view && !view.hidden && view.alpha > 0.01 && view.bounds.size.width >= 2 && view.bounds.size.height >= 2;
}

/* 3.11.37：自己画字的标签（群聊输入框上面那排粉丝群入口用的 YYLabel 之类）不是 UILabel，3.11.35 把它们当成
   “自绘内容”图标，文字没读出来。类名里带 Label、能回 attributedText / text 的，按文字记。 */
static NSString *ALBCaptureCustomLabelText(UIView *view) {
    if (!view || [view isKindOfClass:UILabel.class] || [view isKindOfClass:UITextView.class] ||
        [view isKindOfClass:UITextField.class] || [view isKindOfClass:UIImageView.class]) return nil;
    if ([NSStringFromClass([view class]) rangeOfString:@"Label" options:NSCaseInsensitiveSearch].location == NSNotFound) return nil;
    id value = nil;
    @try {
        if ([view respondsToSelector:@selector(attributedText)]) value = [view valueForKey:@"attributedText"];
        if (!([value isKindOfClass:NSAttributedString.class] && ((NSAttributedString *)value).length) &&
            [view respondsToSelector:@selector(text)])
            value = [view valueForKey:@"text"];
    } @catch (__unused NSException *exception) {
        value = nil;
    }
    NSString *text = [value isKindOfClass:NSAttributedString.class] ? ((NSAttributedString *)value).string
        : ([value isKindOfClass:NSString.class] ? value : nil);
    return text.length ? text : nil;
}

/* 3.11.37：iOS 26 的标签栏 / 导航控制器在最前面盖了几层透明的“悬浮栏”容器（_UIFloatingBarContainerView、
   FloatingBarHostingView、_UITabBarContainerWrapperView …），触摸会穿过它们，但按几何找“最前面那层”会先找到它们。
   3.11.36 的第 1 段（评论长按面板）三次都框成了整块面板、第 2 段群聊输入区三十多次全框成了
   _UITabBarContainerWrapperView，就是这个原因。这些容器只在 UIKit 自己的命中测试找不到目标时才用来兜底跳过。 */
static BOOL ALBCapturePassThrough(UIView *view) {
    NSString *name = NSStringFromClass([view class]);
    for (NSString *part in @[@"FloatingBar", @"_UITabBarContainer", @"UITransitionView", @"UIDropShadowView",
                             @"UILayoutContainerView", @"UIViewControllerWrapperView", @"UINavigationTransitionView"])
        if ([name containsString:part]) return YES;
    return NO;
}

/* Icons and texts inside the capsule (views and non-view sublayers), plus glass. */
static void ALBCaptureContents(UIView *capsule, NSMutableArray<ALBCapturePart *> *parts,
                               NSMutableArray *icons, NSMutableArray *texts, NSMutableArray *glass) {
    if (!capsule) return;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:capsule];
    for (NSUInteger i = 0; i < queue.count && i < 300; i++) {
        UIView *view = queue[i];
        if (!ALBCaptureVisible(view)) continue;
        [queue addObjectsFromArray:view.subviews];
        if (ALBProbeClassIsPlugin([view class])) continue;
        NSString *name = NSStringFromClass([view class]);
        NSString *role = nil;
        NSMutableDictionary *info = nil;
        if ([view isKindOfClass:UILabel.class] && ((UILabel *)view).text.length) {
            UILabel *label = (UILabel *)view;
            role = @"text";
            info = ALBCaptureViewJSON(view, capsule);
            NSString *text = label.text.length > 80 ? [[label.text substringToIndex:80] stringByAppendingString:@"…"] : label.text;
            info[@"text"] = text;
            info[@"font"] = [NSString stringWithFormat:@"%@ %.1f", label.font.fontName ?: @"?", label.font.pointSize];
            info[@"textColor"] = ALBCaptureColor(label.textColor, label) ?: @"-";
            info[@"lines"] = @(label.numberOfLines);
        } else if (ALBCaptureCustomLabelText(view)) {
            NSString *text = ALBCaptureCustomLabelText(view);
            role = @"text";
            info = ALBCaptureViewJSON(view, capsule);
            info[@"kind"] = @"自绘文字";
            info[@"text"] = text.length > 80 ? [[text substringToIndex:80] stringByAppendingString:@"…"] : text;
        } else if ([view isKindOfClass:UIImageView.class] && ((UIImageView *)view).image) {
            UIImageView *imageView = (UIImageView *)view;
            role = @"icon";
            info = ALBCaptureViewJSON(view, capsule);
            info[@"kind"] = @"图片视图";
            info[@"image"] = ALBCaptureImageJSON(imageView.image, imageView);
            info[@"contentMode"] = @(imageView.contentMode);
            if (imageView.highlightedImage) info[@"highlightedImage"] = ALBCaptureImageJSON(imageView.highlightedImage, imageView);
        } else if ([name rangeOfString:@"(LOT|Lottie|Animation|Animat|PAG|SVGA)" options:NSRegularExpressionSearch].location != NSNotFound) {
            role = @"icon";
            info = ALBCaptureViewJSON(view, capsule);
            info[@"kind"] = @"动画视图";
        } else if ([view isKindOfClass:UIVisualEffectView.class] && view != capsule) {
            role = @"glass";
            info = ALBCaptureViewJSON(view, capsule);
            info[@"relation"] = @"胶囊里面";
        } else if (view != capsule && view.layer.contents && view.bounds.size.width <= 120 && view.bounds.size.height <= 120 &&
                   ![view isKindOfClass:UIImageView.class]) {
            role = @"icon";
            info = ALBCaptureViewJSON(view, capsule);
            info[@"kind"] = @"自绘内容";
        }
        if (role && info) {
            ALBCapturePart *part = [ALBCapturePart new];
            part.view = view;
            part.role = role;
            [parts addObject:part];
            [([role isEqualToString:@"text"] ? texts : ([role isEqualToString:@"glass"] ? glass : icons)) addObject:info];
        }
        // Non-view sublayers: text layers, icon layers with contents, small shapes.
        for (CALayer *layer in view.layer.sublayers) {
            if ([layer.delegate isKindOfClass:UIView.class] || layer.hidden || layer.opacity <= 0.01) continue;
            CGSize size = layer.bounds.size;
            NSString *kind = nil;
            if ([layer isKindOfClass:CATextLayer.class]) kind = @"文字图层";
            else if (layer.contents && size.width >= 4 && size.height >= 4 && size.width <= 120 && size.height <= 120) kind = @"带内容的图层";
            else if ([layer isKindOfClass:CAShapeLayer.class]) {
                CGRect box = CGPathGetPathBoundingBox(((CAShapeLayer *)layer).path);
                if (!CGRectIsNull(box) && box.size.width >= 4 && box.size.width <= 60 && box.size.height <= 60) kind = @"形状图层";
            }
            if (!kind) continue;
            NSMutableDictionary *li = [NSMutableDictionary dictionary];
            li[@"class"] = NSStringFromClass([layer class]);
            li[@"kind"] = kind;
            li[@"host"] = name;
            li[@"frameInWindow"] = ALBProbeRectJSON([view convertRect:layer.frame toView:nil]);
            li[@"frameInCapsule"] = ALBProbeRectJSON([view convertRect:layer.frame toView:capsule]);
            if ([layer isKindOfClass:CATextLayer.class]) {
                id string = ((CATextLayer *)layer).string;
                NSString *text = [string isKindOfClass:NSAttributedString.class] ? ((NSAttributedString *)string).string
                    : ([string isKindOfClass:NSString.class] ? string : @"");
                li[@"text"] = text.length > 80 ? [text substringToIndex:80] : text;
            }
            if ([layer isKindOfClass:CAShapeLayer.class]) {
                CGRect box = CGPathGetPathBoundingBox(((CAShapeLayer *)layer).path);
                li[@"pathBox"] = ALBProbeRectJSON(box);
                li[@"fill"] = ALBProbeColorText(((CAShapeLayer *)layer).fillColor);
            }
            ALBCapturePart *part = [ALBCapturePart new];
            part.view = view;
            part.layer = layer;
            part.role = [kind isEqualToString:@"文字图层"] ? @"text" : @"icon";
            [parts addObject:part];
            [([part.role isEqualToString:@"text"] ? texts : icons) addObject:li];
        }
        if (parts.count >= 30) break;
    }
}

/* Glass or covers next to the capsule (siblings overlapping it, ancestors up to 3) — another plug-in's background glass. */
static void ALBCaptureNearbyGlass(UIView *capsule, NSMutableArray *glass) {
    if (!capsule.superview) return;
    CGRect mine = [capsule convertRect:capsule.bounds toView:nil];
    for (UIView *sibling in capsule.superview.subviews) {
        if (sibling == capsule || !ALBCaptureVisible(sibling)) continue;
        CGRect theirs = [sibling convertRect:sibling.bounds toView:nil];
        CGRect overlap = CGRectIntersection(mine, theirs);
        if (CGRectIsNull(overlap) || overlap.size.width * overlap.size.height < mine.size.width * mine.size.height * 0.5) continue;
        // 3.11.37：系统自己的容器（UITransitionView 之类）不算；只算毛玻璃 / 玻璃、类名带 Glass 的、别的插件的。
        NSString *module = ALBCaptureModule([sibling class]);
        BOOL glassy = [sibling isKindOfClass:UIVisualEffectView.class] ||
                      (![module isEqualToString:@"抖音"] && ![module isEqualToString:@"系统"]) ||
                      [NSStringFromClass([sibling class]) rangeOfString:@"Glass" options:NSCaseInsensitiveSearch].location != NSNotFound;
        if (!glassy) continue;
        NSMutableDictionary *info = ALBCaptureViewJSON(sibling, capsule);
        info[@"relation"] = [capsule.superview.subviews indexOfObjectIdenticalTo:sibling] < [capsule.superview.subviews indexOfObjectIdenticalTo:capsule]
            ? @"同级·在下面" : @"同级·在上面";
        [glass addObject:info];
    }
    UIView *node = capsule.superview;
    for (NSUInteger i = 0; node && i < 3 && ![node isKindOfClass:UIWindow.class]; i++, node = node.superview) {
        if (![node isKindOfClass:UIVisualEffectView.class]) continue;
        NSMutableDictionary *info = ALBCaptureViewJSON(node, capsule);
        info[@"relation"] = [NSString stringWithFormat:@"往上第 %lu 层", (unsigned long)i + 1];
        [glass addObject:info];
    }
}

/* 3.11.37：属于这颗按钮、却在它外面的文字（聊天加号面板：72×72 的按钮在格子上半截，名字在下面，
   3.11.36 只框到按钮，名字没记）。范围：往上 3 层内最近的格子；没有格子时只看父视图，而且父视图不能比按钮大太多。 */
static NSArray *ALBCaptureNearbyTexts(UIView *capsule) {
    NSMutableArray *out = [NSMutableArray array];
    if (!capsule.superview) return out;
    UIView *scope = nil, *node = capsule.superview;
    for (NSUInteger i = 0; node && i < 3 && ![node isKindOfClass:UIWindow.class]; i++, node = node.superview)
        if ([node isKindOfClass:UICollectionViewCell.class] || [node isKindOfClass:UITableViewCell.class]) { scope = node; break; }
    if (!scope) {
        CGFloat mine = capsule.bounds.size.width * capsule.bounds.size.height;
        CGFloat theirs = capsule.superview.bounds.size.width * capsule.superview.bounds.size.height;
        if (mine > 0 && theirs <= mine * 3) scope = capsule.superview;
    }
    if (!scope) return out;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:scope];
    for (NSUInteger i = 0; i < queue.count && i < 120 && out.count < 8; i++) {
        UIView *view = queue[i];
        if (!ALBCaptureVisible(view) || view == capsule) continue;   // the capsule's own texts are already listed
        [queue addObjectsFromArray:view.subviews];
        NSString *text = [view isKindOfClass:UILabel.class] ? ((UILabel *)view).text : ALBCaptureCustomLabelText(view);
        if (!text.length) continue;
        NSMutableDictionary *info = ALBCaptureViewJSON(view, capsule);
        info[@"text"] = text.length > 80 ? [[text substringToIndex:80] stringByAppendingString:@"…"] : text;
        if ([view isKindOfClass:UILabel.class]) {
            UILabel *label = (UILabel *)view;
            info[@"font"] = [NSString stringWithFormat:@"%@ %.1f", label.font.fontName ?: @"?", label.font.pointSize];
            info[@"textColor"] = ALBCaptureColor(label.textColor, label) ?: @"-";
        } else {
            info[@"kind"] = @"自绘文字";
        }
        [out addObject:info];
    }
    return out;
}

/* 3.11.37 “拆开”：把框住的一整块（面板、一排）拆成里面的每颗按钮 / 每个格子，各记一项，不用一行一行点。
   取最外层的按钮 / 格子；格子里还套着列表（一整排横着滑的头像、表情页签）时往里拆。 */
static BOOL ALBCaptureHasInnerList(UIView *view, NSUInteger depth) {
    if (depth > 4) return NO;
    for (UIView *sub in view.subviews) {
        if ([sub isKindOfClass:UIScrollView.class] && ALBCaptureVisible(sub)) return YES;
        if (ALBCaptureHasInnerList(sub, depth + 1)) return YES;
    }
    return NO;
}

static NSArray<UIView *> *ALBCaptureSplit(UIView *capsule) {
    NSMutableArray<UIView *> *pieces = [NSMutableArray array];
    UIWindow *window = capsule.window;
    if (!window) return pieces;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:capsule.subviews];
    for (NSUInteger i = 0; i < queue.count && i < 900 && pieces.count < 40; i++) {
        UIView *view = queue[i];
        if (!ALBCaptureVisible(view) || ALBProbeClassIsPlugin([view class])) continue;
        CGRect frame = [view convertRect:view.bounds toView:nil];
        if (!CGRectIntersectsRect(frame, window.bounds)) continue;           // scrolled away
        BOOL piece = ALBCaptureCapsuleScore(view) == 2 && frame.size.width >= 16 && frame.size.height >= 16 &&
                     !ALBCaptureHasInnerList(view, 0);
        if (piece) [pieces addObject:view];
        else [queue addObjectsFromArray:view.subviews];
    }
    [pieces sortUsingComparator:^NSComparisonResult(UIView *a, UIView *b) {
        CGRect ra = [a convertRect:a.bounds toView:nil], rb = [b convertRect:b.bounds toView:nil];
        if (fabs(CGRectGetMinY(ra) - CGRectGetMinY(rb)) > 4) return CGRectGetMinY(ra) < CGRectGetMinY(rb) ? NSOrderedAscending : NSOrderedDescending;
        if (CGRectGetMinX(ra) == CGRectGetMinX(rb)) return NSOrderedSame;
        return CGRectGetMinX(ra) < CGRectGetMinX(rb) ? NSOrderedAscending : NSOrderedDescending;
    }];
    return pieces;
}

/* (Re)reads an element item from its capsule level. */
static void ALBCaptureRead(ALBCaptureItem *item, NSArray<NSString *> *atPoint) {
    UIView *capsule = [item capsule];
    NSMutableArray<ALBCapturePart *> *parts = [NSMutableArray array];
    NSMutableArray *icons = [NSMutableArray array], *texts = [NSMutableArray array], *glass = [NSMutableArray array];
    ALBCaptureContents(capsule, parts, icons, texts, glass);
    ALBCaptureNearbyGlass(capsule, glass);
    for (ALBCapturePart *old in item.parts) [old.box removeFromSuperview];
    item.parts = parts;
    NSMutableArray<NSString *> *chain = [NSMutableArray array];
    for (NSUInteger i = 0; i < item.chain.count; i++) {
        UIView *view = (__bridge UIView *)[item.chain pointerAtIndex:i];
        [chain addObject:view ? [NSString stringWithFormat:@"%@%@ %.0f×%.0f", i == item.level ? @"▶" : @"",
            NSStringFromClass([view class]), view.bounds.size.width, view.bounds.size.height] : @"(已释放)"];
    }
    NSMutableDictionary *snapshot = [NSMutableDictionary dictionary];
    snapshot[@"n"] = @(item.number);
    snapshot[@"kind"] = @"element";
    snapshot[@"time"] = [[NSISO8601DateFormatter new] stringFromDate:NSDate.date];
    snapshot[@"point"] = @{@"x": @(ALBProbeNumber(item.point.x)), @"y": @(ALBProbeNumber(item.point.y))};
    snapshot[@"capsule"] = capsule ? ALBCaptureViewJSON(capsule, nil) : @{@"error": @"已释放"};
    snapshot[@"capsuleLevel"] = @(item.level);
    snapshot[@"chainUpwards"] = chain;
    snapshot[@"icons"] = icons;
    snapshot[@"texts"] = texts;
    snapshot[@"glass"] = glass;
    NSArray *nearby = capsule ? ALBCaptureNearbyTexts(capsule) : @[];
    if (nearby.count) snapshot[@"textsNearby"] = nearby;
    if (item.hitName.length) snapshot[@"hitTest"] = item.hitName;
    if (item.origin.length) snapshot[@"origin"] = item.origin;
    if (atPoint) snapshot[@"atPoint"] = atPoint;
    else if (item.snapshot[@"atPoint"]) snapshot[@"atPoint"] = item.snapshot[@"atPoint"];
    snapshot[@"page"] = ALBCaptureControllers(capsule, 4);
    item.snapshot = snapshot;
    NSString *firstText = texts.count ? texts.firstObject[@"text"] : (nearby.count ? nearby.firstObject[@"text"] : nil);
    item.summary = [NSString stringWithFormat:@"%@%@ · 图标 %lu · 文字 %lu%@%@%@",
        capsule ? NSStringFromClass([capsule class]) : @"(已释放)",
        capsule ? [NSString stringWithFormat:@" %.0f×%.0f", capsule.bounds.size.width, capsule.bounds.size.height] : @"",
        (unsigned long)icons.count, (unsigned long)texts.count,
        nearby.count ? [NSString stringWithFormat:@" · 旁边文字 %lu", (unsigned long)nearby.count] : @"",
        glass.count ? [NSString stringWithFormat:@" · 玻璃 %lu", (unsigned long)glass.count] : @"",
        firstText.length ? [NSString stringWithFormat:@"「%@」", firstText] : @""];
}

#pragma mark - Pages (quick jumps)

static BOOL ALBCaptureKeyLooksLikeRoute(NSString *name) {
    NSString *lower = name.lowercaseString;
    for (NSString *word in @[@"schema", @"scheme", @"url", @"route", @"router", @"enter", @"page", @"source", @"scene", @"path", @"link"])
        if ([lower containsString:word]) return YES;
    return NO;
}

/* Strings in the controller's own (non-system) classes that look like a route: any NSString / NSURL ivar
   holding "://", or one whose name says schema / url / route / page …; dictionaries holding "://".
   3.11.37：3.11.36 只看控制器自己，第 6、7 段 5 个页面一条 “://” 都没找到（像 BDXViewController 这种网页 / Lynx 容器，
   地址放在它持有的参数对象里）。现在再往里看一层：控制器用 strong / copy 属性持有的、抖音自己的对象
   （不是视图、不是控制器、不是系统类），同样找字符串。只读强引用属性背后的变量，不调 getter（不触发懒加载）。 */
static void ALBCaptureRouteScan(id object, NSString *via, NSUInteger depth, NSMutableArray<NSString *> *found, NSHashTable *seen) {
    if (!object || found.count >= 30 || [seen containsObject:object]) return;
    [seen addObject:object];
    // [object class], not object_getClass: a KVO subclass (NSKVONotifying_…) would read as a system class.
    for (Class cls = [object class]; cls && !ALBProbeClassIsSystem(cls) && found.count < 30; cls = class_getSuperclass(cls)) {
        unsigned int count = 0;
        Ivar *ivars = class_copyIvarList(cls, &count);
        for (unsigned int i = 0; i < count && found.count < 30; i++) {
            const char *type = ivar_getTypeEncoding(ivars[i]);
            const char *name = ivar_getName(ivars[i]);
            if (!type || !name) continue;
            BOOL string = strcmp(type, "@\"NSString\"") == 0 || strcmp(type, "@\"NSMutableString\"") == 0;
            BOOL url = strcmp(type, "@\"NSURL\"") == 0;
            BOOL dict = strcmp(type, "@\"NSDictionary\"") == 0 || strcmp(type, "@\"NSMutableDictionary\"") == 0;
            if (!string && !url && !dict) continue;
            id value = object_getIvar(object, ivars[i]);
            if (!value) continue;
            NSString *text = nil;
            @try {
                text = url ? ((NSURL *)value).absoluteString : (dict ? [value description] : value);
            } @catch (__unused NSException *exception) {
                text = nil;
            }
            if (![text isKindOfClass:NSString.class] || !text.length) continue;
            NSString *ivarName = [NSString stringWithUTF8String:name];
            BOOL routeLike = [text containsString:@"://"] || (!dict && ALBCaptureKeyLooksLikeRoute(ivarName));
            if (!routeLike) continue;
            if (text.length > 300) text = [[text substringToIndex:300] stringByAppendingString:@"…"];
            [found addObject:[NSString stringWithFormat:@"%@%@.%@ = %@", via ?: @"", NSStringFromClass(cls), ivarName, text]];
        }
        free(ivars);
        if (depth >= 1) continue;
        unsigned int propertyCount = 0;
        objc_property_t *properties = class_copyPropertyList(cls, &propertyCount);
        for (unsigned int p = 0; p < propertyCount && found.count < 30; p++) {
            char *strongMark = property_copyAttributeValue(properties[p], "&");
            char *copyMark = property_copyAttributeValue(properties[p], "C");
            char *typeMark = property_copyAttributeValue(properties[p], "T");
            char *ivarMark = property_copyAttributeValue(properties[p], "V");
            BOOL owned = strongMark != NULL || copyMark != NULL;
            NSString *typeName = nil;
            if (typeMark && strlen(typeMark) > 3 && typeMark[0] == '@' && typeMark[1] == '"' && typeMark[2] != '<') {
                typeName = [[NSString alloc] initWithBytes:typeMark + 2 length:strlen(typeMark) - 3 encoding:NSUTF8StringEncoding];
                NSRange angle = [typeName rangeOfString:@"<"];
                if (angle.location != NSNotFound) typeName = [typeName substringToIndex:angle.location];
            }
            Class propertyClass = typeName.length ? NSClassFromString(typeName) : Nil;
            Ivar backing = ivarMark ? class_getInstanceVariable(cls, ivarMark) : NULL;
            free(strongMark);
            free(copyMark);
            free(typeMark);
            free(ivarMark);
            if (!owned || !propertyClass || !backing || ALBProbeClassIsSystem(propertyClass) ||
                [propertyClass isSubclassOfClass:UIView.class] || [propertyClass isSubclassOfClass:UIViewController.class]) continue;
            id value = object_getIvar(object, backing);
            if (!value || ![value isKindOfClass:propertyClass]) continue;
            NSString *step = [NSString stringWithFormat:@"%@%@.%s → ", via ?: @"", NSStringFromClass(cls), property_getName(properties[p])];
            ALBCaptureRouteScan(value, step, depth + 1, found, seen);
        }
        free(properties);
    }
}

static NSArray<NSString *> *ALBCaptureRouteStrings(UIViewController *controller) {
    NSMutableArray<NSString *> *found = [NSMutableArray array];
    NSHashTable *seen = [NSHashTable hashTableWithOptions:NSPointerFunctionsStrongMemory | NSPointerFunctionsObjectPointerPersonality];
    ALBCaptureRouteScan(controller, nil, 0, found, seen);
    return found;
}

/* 3.11.37：抖音的页面几乎都是自己画的导航栏，title 是空的，3.11.36 记下的页面只有类名，认不出是哪一页。
   现在记页面顶部（窗口 y < 170）看得见的文字，字号大的、靠中间的在前，最多 4 条。 */
static NSArray<NSString *> *ALBCaptureVisibleTitles(UIViewController *top) {
    UIView *root = top.viewIfLoaded;
    UIWindow *window = root.window;
    if (!root || !window) return @[];
    NSMutableArray<UILabel *> *labels = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < queue.count && i < 700; i++) {
        UIView *view = queue[i];
        if (!ALBCaptureVisible(view)) continue;
        [queue addObjectsFromArray:view.subviews];
        if (![view isKindOfClass:UILabel.class]) continue;
        UILabel *label = (UILabel *)view;
        if (label.text.length < 1 || label.text.length > 30) continue;
        CGRect frame = [label convertRect:label.bounds toView:nil];
        if (CGRectGetMinY(frame) > 170 || !CGRectIntersectsRect(frame, window.bounds)) continue;
        [labels addObject:label];
    }
    CGFloat middle = CGRectGetMidX(window.bounds);
    [labels sortUsingComparator:^NSComparisonResult(UILabel *a, UILabel *b) {
        if (fabs(a.font.pointSize - b.font.pointSize) > 0.5) return a.font.pointSize > b.font.pointSize ? NSOrderedAscending : NSOrderedDescending;
        CGFloat da = fabs(CGRectGetMidX([a convertRect:a.bounds toView:nil]) - middle);
        CGFloat db = fabs(CGRectGetMidX([b convertRect:b.bounds toView:nil]) - middle);
        if (da == db) return NSOrderedSame;
        return da < db ? NSOrderedAscending : NSOrderedDescending;
    }];
    NSMutableArray<NSString *> *out = [NSMutableArray array];
    for (UILabel *label in labels) {
        if (out.count >= 4) break;
        [out addObject:[NSString stringWithFormat:@"%@（字号 %.0f）", label.text, label.font.pointSize]];
    }
    return out;
}

static NSDictionary *ALBCaptureControllerJSON(UIViewController *controller) {
    NSMutableDictionary *d = [NSMutableDictionary dictionary];
    d[@"class"] = NSStringFromClass([controller class]);
    d[@"module"] = ALBCaptureModule([controller class]);
    if (controller.title.length) d[@"title"] = controller.title;
    if (controller.navigationItem.title.length) d[@"navTitle"] = controller.navigationItem.title;
    if ([controller isKindOfClass:UINavigationController.class]) {
        NSMutableArray *stack = [NSMutableArray array];
        for (UIViewController *child in ((UINavigationController *)controller).viewControllers) [stack addObject:NSStringFromClass([child class])];
        d[@"stack"] = stack;
    }
    if ([controller isKindOfClass:UITabBarController.class]) d[@"selectedIndex"] = @(((UITabBarController *)controller).selectedIndex);
    if (controller.childViewControllers.count) {
        NSMutableArray *children = [NSMutableArray array];
        for (UIViewController *child in controller.childViewControllers)
            if (children.count < 12) [children addObject:NSStringFromClass([child class])];
        d[@"children"] = children;
    }
    if (controller.presentingViewController) d[@"presented"] = @YES;
    return d;
}

/* The controller chain the user is looking at: root → selected tab / top of navigation → presented …
   plus route-looking strings of the top-most one and its visible children. */
static NSDictionary *ALBCapturePageJSON(UIWindow *window) {
    NSMutableArray *chain = [NSMutableArray array];
    UIViewController *controller = window.rootViewController, *top = nil;
    for (NSUInteger guard = 0; controller && guard < 16; guard++) {
        [chain addObject:ALBCaptureControllerJSON(controller)];
        top = controller;
        UIViewController *next = nil;
        if (controller.presentedViewController && !controller.presentedViewController.isBeingDismissed) next = controller.presentedViewController;
        else if ([controller isKindOfClass:UITabBarController.class]) next = ((UITabBarController *)controller).selectedViewController;
        else if ([controller isKindOfClass:UINavigationController.class]) next = ((UINavigationController *)controller).topViewController;
        controller = next;
    }
    NSMutableDictionary *page = [NSMutableDictionary dictionary];
    page[@"kind"] = @"page";
    page[@"window"] = window ? NSStringFromClass([window class]) : @"";
    page[@"controllers"] = chain;
    if (top) {
        page[@"top"] = NSStringFromClass([top class]);
        NSMutableArray *routes = [NSMutableArray arrayWithArray:ALBCaptureRouteStrings(top)];
        for (UIViewController *child in top.childViewControllers) {
            if (routes.count >= 30) break;
            if (child.viewIfLoaded.window) [routes addObjectsFromArray:ALBCaptureRouteStrings(child)];
        }
        page[@"routeStrings"] = routes;
        page[@"visibleTitles"] = ALBCaptureVisibleTitles(top);
    }
    return page;
}
