/* 分享面板、评论长按（分享评论给）面板、图文页评论框的玻璃。

   · 聊天页（顶栏排法、表情栏、表情 / 加号面板卡片）全部删掉，聊天页回到抖音原样。
   · 深浅：全部跟系统（窗口场景的 userInterfaceStyle，同 ALBSceneStyle），不再按面板原来的底色、字色各猜各的
     （3.11.43 的整块底按底色、胶囊按底色往上找、输入层按自己的底色，于是一块面板里深浅不一，评论长按面板深色模式下是浅色玻璃）。
     系统深浅一切换，场景的特征变化回调里把这里装过的全部重做一遍（registerForTraitChanges）。
   · 玻璃：Regular、不染色（tint nil）、overrideUserInterfaceStyle＝系统深浅；整块底挂在面板自己的视图里最底下
     （跟着面板走），上圆角跟外壳同心（最少 20）。
   · 会按压的（功能图标一圈、关闭键、发送键、评论面板每一行、图文页评论框）：系统玻璃按钮、原来的内容画进按钮的图里，按下去一起
     变形（ALBPillApply，ALBChatFace.h；关闭键直接换成系统玻璃按钮配置）。
   弹幕设置不做。Included by Tweak.x right after ALBChatFace.h; hooks installed at run time (ALBMoreGlassInstall). */

FOUNDATION_EXPORT NSInteger ALBSheetKindForView(UIView *view);
static void ALBMoreObserveScene(UIView *view);

static NSMutableOrderedSet<NSString *> *gALBMoreHooked, *gALBMoreMissing;
static NSUInteger gALBMoreSectionHidden, gALBMoreCardsCleared, gALBMoreDividers, gALBMoreOverlays, gALBMoreEditableSkips;
static NSUInteger gALBMorePanelGlass, gALBMoreCloseButtons, gALBMoreStyleFlips;
static NSString *gALBMoreStyleLast;
static NSHashTable<UIView *> *gALBMoreSections, *gALBMoreOverlayViews, *gALBMorePanelSlots;
static NSHashTable<UIViewController *> *gALBMoreShareContents, *gALBMoreCommentSheets;

#pragma mark - 运行时挂钩（原方法跑完再同步）

typedef void (^ALBMoreAfter)(id object);
enum { ALBMoreHookVoid = 0, ALBMoreHookObject = 1, ALBMoreHookBool = 2, ALBMoreHookTwoObjects = 3 };

static BOOL ALBMoreHook(Class cls, NSString *selectorName, int kind, ALBMoreAfter after) {
    SEL sel = NSSelectorFromString(selectorName);
    Method method = cls ? class_getInstanceMethod(cls, sel) : NULL;
    if (!method || !after) return NO;
    IMP original = method_getImplementation(method);
    IMP replacement = NULL;
    if (kind == ALBMoreHookVoid) {
        replacement = imp_implementationWithBlock(^(id me) { ((void (*)(id, SEL))original)(me, sel); after(me); });
    } else if (kind == ALBMoreHookObject) {
        replacement = imp_implementationWithBlock(^(id me, id arg) { ((void (*)(id, SEL, id))original)(me, sel, arg); after(me); });
    } else if (kind == ALBMoreHookBool) {
        replacement = imp_implementationWithBlock(^(id me, BOOL arg) { ((void (*)(id, SEL, BOOL))original)(me, sel, arg); after(me); });
    } else if (kind == ALBMoreHookTwoObjects) {
        replacement = imp_implementationWithBlock(^(id me, id a, id b) { ((void (*)(id, SEL, id, id))original)(me, sel, a, b); after(me); });
    } else {
        return NO;
    }
    // 自己没写这个方法（继承来的）：加一份只在这个类上；自己写了：换掉
    if (!class_addMethod(cls, sel, replacement, method_getTypeEncoding(method))) method_setImplementation(method, replacement);
    return YES;
}
static Class ALBMoreClass(NSArray<NSString *> *names) {
    for (NSString *name in names) {
        Class cls = objc_getClass(name.UTF8String);
        if (cls) {
            if (!gALBMoreHooked) gALBMoreHooked = [NSMutableOrderedSet orderedSet];
            [gALBMoreHooked addObject:[name componentsSeparatedByString:@"."].lastObject];
            return cls;
        }
    }
    if (!gALBMoreMissing) gALBMoreMissing = [NSMutableOrderedSet orderedSet];
    [gALBMoreMissing addObject:[names.firstObject componentsSeparatedByString:@"."].lastObject ?: @"?"];
    return Nil;
}
/* 布局、进窗口都同步一次 */
static void ALBMoreHookView(Class cls, void (*sync)(UIView *)) {
    if (!cls || !sync) return;
    ALBMoreAfter after = ^(id object) { sync((UIView *)object); };
    ALBMoreHook(cls, @"layoutSubviews", ALBMoreHookVoid, after);
    ALBMoreHook(cls, @"didMoveToWindow", ALBMoreHookVoid, after);
}

#pragma mark - 在哪个页面

/* 往上（响应链）所有控制器的类名，空格连起来；记在视图上（同一个窗口里不再算） */
static char kALBMoreContextKey;
static NSString *ALBMoreControllers(UIView *view) {
    UIWindow *window = view.window;
    if (!window) return @"";
    NSArray *cached = objc_getAssociatedObject(view, &kALBMoreContextKey);
    if (cached.count == 2 && [cached[0] pointerValue] == (__bridge void *)window) return cached[1];
    NSMutableString *names = [NSMutableString string];
    NSUInteger steps = 0;
    for (UIResponder *r = view.nextResponder; r && r != window && steps < 120; r = r.nextResponder, steps++)
        if ([r isKindOfClass:UIViewController.class]) [names appendFormat:@"%@ ", NSStringFromClass(r.class)];
    NSString *result = names.copy;
    objc_setAssociatedObject(view, &kALBMoreContextKey, @[[NSValue valueWithPointer:(__bridge void *)window], result], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return result;
}
static BOOL ALBMoreInCommentPanel(UIView *view) {
    return [ALBMoreControllers(view) rangeOfString:@"CommentLongPressPanel"].location != NSNotFound;
}
static BOOL ALBMoreInPanels(UIView *view) {
    NSString *names = ALBMoreControllers(view);
    if ([names rangeOfString:@"CommentLongPressPanel"].location != NSNotFound) return YES;
    // 多功能插件的长按面板里也装着抖音的分享面板：那块 3.11.34 起已经做好（用户：比这里的做法好），里面不动
    if ([names rangeOfString:@"LongPress"].location != NSNotFound || ALBSheetKindForView(view) == 2) return NO;
    return [names rangeOfString:@"SharePanel"].location != NSNotFound;
}
static BOOL ALBMoreGlassReady(void) { return AWEPluginFeaturesEnabled() && NSThread.isMainThread; }

static id ALBMoreGetter(id object, NSString *name) {
    SEL sel = NSSelectorFromString(name);
    if (!object || ![object respondsToSelector:sel]) return nil;
    return ((id (*)(id, SEL))objc_msgSend)(object, sel);
}

#pragma mark - 不按压的整块玻璃（系统 Regular）

/* 玻璃先有大小再给效果（0 尺寸时写入效果，系统不建材质层，之后只改 frame 补不回来） */
static UIVisualEffectView *ALBMoreMakeGlass(void) {
    UIVisualEffectView *glass = [[UIVisualEffectView alloc] initWithEffect:nil];
    glass.userInteractionEnabled = NO;
    glass.accessibilityIdentifier = @"AwemeLiquidBar.PanelGlass";
    return glass;
}
static char kALBMoreGlassCornerKey;
/* top / bottom：<0 跟外面同心（最少 |值|，containerConcentricRadiusWithMinimum），0 直角，>0 固定 */
static void ALBMoreGlassPlace(UIVisualEffectView *glass, UIView *host, CGRect frame, UIUserInterfaceStyle style, CGFloat top, CGFloat bottom) {
    if (!glass || !host) return;
    BOOL wasEmpty = glass.bounds.size.width < 8 || glass.bounds.size.height < 8;
    if (glass.superview != host || host.subviews.firstObject != glass) [host insertSubview:glass atIndex:0];
    if (!CGRectEqualToRect(glass.frame, frame)) glass.frame = frame;
    BOOL usable = frame.size.width >= 8 && frame.size.height >= 8;
    if (glass.overrideUserInterfaceStyle != style) glass.overrideUserInterfaceStyle = style;
    if (@available(iOS 26.0, *)) {
        NSString *cornerKey = [NSString stringWithFormat:@"%.1f/%.1f", top, bottom];
        if (![objc_getAssociatedObject(glass, &kALBMoreGlassCornerKey) isEqualToString:cornerKey]) {
            objc_setAssociatedObject(glass, &kALBMoreGlassCornerKey, cornerKey, OBJC_ASSOCIATION_COPY_NONATOMIC);
            UICornerRadius *topRadius = top < 0 ? [UICornerRadius containerConcentricRadiusWithMinimum:-top] : [UICornerRadius fixedRadius:top];
            UICornerRadius *bottomRadius = bottom < 0 ? [UICornerRadius containerConcentricRadiusWithMinimum:-bottom] : [UICornerRadius fixedRadius:bottom];
            glass.cornerConfiguration = [UICornerConfiguration configurationWithUniformTopRadius:topRadius uniformBottomRadius:bottomRadius];
        }
        if (wasEmpty && usable && glass.effect) glass.effect = nil;
        if (usable && !glass.effect) {
            UIGlassEffect *effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleRegular];
            [UIView performWithoutAnimation:^{ glass.effect = effect; }];
        }
    }
}

#pragma mark - 面板的整块底（分享面板、评论的分享面板、选人后的输入白垫）

/* 底色槽（不透明的那张底）记原色、透明明，最底下挂一块玻璃（跟着它走：面板往下拖时玻璃一起走）。
   3.11.44：深浅跟系统（ALBMoreStyle），不再看这张底原来的颜色 */
static char kALBMorePanelGlassKey;
/* 3.11.45 用户：“评论区的长按转发，我发现抖音他自己没有适配深色模式，所以如果深色模式下给他用深色玻璃的话，字体就看不到了，
   所以那一块只能锁浅色”。评论长按（分享评论给）面板里的一律浅色；分享面板照旧跟系统 */
static NSUInteger gALBMoreCommentLightLocks;
static UIUserInterfaceStyle ALBMoreStyle(UIView *view) {
    if (ALBMoreInCommentPanel(view)) {
        if (ALBSceneStyle(view) == UIUserInterfaceStyleDark) gALBMoreCommentLightLocks++;
        return UIUserInterfaceStyleLight;
    }
    return ALBSceneStyle(view);
}
static BOOL ALBMorePanelApply(UIView *slot, CGFloat topRadius) {
    if (!slot || !slot.window) return NO;
    UIColor *color = slot.backgroundColor;
    BOOL taken = ALBBackingTaken(slot);
    if (!taken && !(color && CGColorGetAlpha(color.CGColor) >= 0.99)) return NO;
    UIVisualEffectView *glass = objc_getAssociatedObject(slot, &kALBMorePanelGlassKey);
    ALBBackingClear(slot);
    if (!glass) {
        glass = ALBMoreMakeGlass();
        glass.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
        objc_setAssociatedObject(slot, &kALBMorePanelGlassKey, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!gALBMorePanelSlots) gALBMorePanelSlots = [NSHashTable weakObjectsHashTable];
        [gALBMorePanelSlots addObject:slot];
        gALBMorePanelGlass++;
    }
    ALBMoreGlassPlace(glass, slot, slot.bounds, ALBMoreStyle(slot), topRadius, 0);
    return YES;
}
static void ALBMorePanelRestore(UIView *slot) {
    if (!slot) return;
    UIView *glass = objc_getAssociatedObject(slot, &kALBMorePanelGlassKey);
    [glass removeFromSuperview];
    objc_setAssociatedObject(slot, &kALBMorePanelGlassKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    ALBBackingRestore(slot);
}

/* 外壳：控制器视图下面直接那层抖音的毛玻璃（DUXVisualEffectView），只为读它的圆角 */
static UIView *ALBMoreShell(UIViewController *controller) {
    Class cls = objc_getClass("DUXVisualEffectView");
    if (!cls || !controller.isViewLoaded) return nil;
    for (UIView *sub in controller.view.subviews) if ([sub isKindOfClass:cls]) return sub;
    return nil;
}

#pragma mark - 评论的分享面板：每行一块胶囊（卡片行）

@interface ALBMoreCard : NSObject
@property (nonatomic, weak) CALayer *layer;
@property (nonatomic, assign) CGColorRef fill;
@property (nonatomic, assign) BOOL saved;
@property (nonatomic, strong) NSHashTable<CALayer *> *dividers;
@end
@implementation ALBMoreCard
- (void)dealloc { if (_fill) CGColorRelease(_fill); }
@end
static char kALBMoreCardKey;

static CGRect ALBMoreLayerRect(CALayer *layer) {
    if ([layer isKindOfClass:CAShapeLayer.class]) {
        CGPathRef path = ((CAShapeLayer *)layer).path;
        if (path && !CGPathIsEmpty(path)) return [layer convertRect:CGPathGetBoundingBox(path) toLayer:layer.superlayer];
    }
    return layer.frame;
}
static CGColorRef ALBMoreLayerFill(CALayer *layer) {
    return [layer isKindOfClass:CAShapeLayer.class] ? ((CAShapeLayer *)layer).fillColor : layer.backgroundColor;
}
static void ALBMoreSetLayerFill(CALayer *layer, CGColorRef fill) {
    if ([layer isKindOfClass:CAShapeLayer.class]) ((CAShapeLayer *)layer).fillColor = fill;
    else layer.backgroundColor = fill;
}
/* 行的卡片底：抖音直接加在行里的一层（不是视图的层），填充不透明、够一块卡片大（contentShapeLayer） */
static CALayer *ALBMoreCardLayer(UIView *view, UIView *cell, NSUInteger depth) {
    if (!view || depth > 3 || ALBPillOursView(view)) return nil;
    for (CALayer *layer in view.layer.sublayers) {
        if ([layer.delegate isKindOfClass:UIView.class] || layer.hidden) continue;
        CGColorRef fill = ALBMoreLayerFill(layer);
        if (!fill || CGColorGetAlpha(fill) < 0.5) continue;
        CGRect r = [layer.superlayer convertRect:ALBMoreLayerRect(layer) toLayer:cell.layer];
        if (r.size.width < 100 || r.size.height < 24) continue;
        return layer;
    }
    for (UIView *sub in view.subviews) {
        CALayer *found = ALBMoreCardLayer(sub, cell, depth + 1);
        if (found) return found;
    }
    return nil;
}

static void ALBMoreCommentSheetSyncFrom(UIView *view);
/* 3.11.47：行里套着一条横向列表（选人那一排头像）：不盖玻璃按钮（盖上了里面的头像点不到、也横滑不动） */
static NSUInteger gALBMoreNestedRowsSkipped;
static BOOL ALBMoreRowHasNestedList(UIView *cell) {
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:cell.subviews];
    for (NSUInteger i = 0; i < queue.count && i < 160; i++) {
        UIView *v = queue[i];
        if (ALBPillOursView(v)) continue;
        if (([v isKindOfClass:UICollectionView.class] || [v isKindOfClass:UITableView.class]) && v.bounds.size.width > 60) return YES;
        [queue addObjectsFromArray:v.subviews];
    }
    return NO;
}
/* 一行：卡片矩形上下各让 3 一块胶囊（圆角交给胶囊），卡片底的填充去掉、同一层的细分割线藏起来 */
static void ALBMoreCommentRowSync(UIView *cell) {
    if (!cell.window || !ALBMoreGlassReady()) return;
    CGSize s = cell.bounds.size;
    BOOL want = ALBMoreSheetsOn() && !cell.hidden && s.width >= 120 && s.height >= 30 && s.height <= 140 && ALBMoreInCommentPanel(cell);
    if (want && ALBMoreRowHasNestedList(cell)) { want = NO; gALBMoreNestedRowsSkipped++; }
    ALBMoreCommentSheetSyncFrom(cell);       // 外壳的布局回调不一定经过：行这一路带着整块底一起同步
    if (!want) { ALBPillApply(cell, nil, NULL); return; }
    UIView *content = [cell isKindOfClass:UICollectionViewCell.class] ? ((UICollectionViewCell *)cell).contentView : cell;
    ALBMoreCard *card = objc_getAssociatedObject(cell, &kALBMoreCardKey);
    CALayer *layer = card.layer;
    if (!layer.superlayer) {
        layer = ALBMoreCardLayer(cell, cell, 0);
        card = [ALBMoreCard new];
        card.layer = layer;
        card.dividers = [NSHashTable weakObjectsHashTable];
        objc_setAssociatedObject(cell, &kALBMoreCardKey, card, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    CGRect frame = CGRectInset(cell.bounds, 16, 3);
    if (layer) {
        CGRect r = CGRectInset([layer.superlayer convertRect:ALBMoreLayerRect(layer) toLayer:cell.layer], 0, 3);
        if (r.size.width >= 100 && r.size.height >= 20) frame = r;
    }
    ALBPillSpec *spec = [ALBPillSpec new];
    spec.frame = frame;
    spec.sources = ALBPillSourcesInRect(cell, content, cell, frame);
    spec.style = ALBMoreStyle(cell);
    spec.area = @"评论分享面板的行";
    ALBPillApply(cell, spec, ALBMoreCommentRowSync);
    if (!ALBPillStateOf(cell) || !layer) return;
    CGColorRef fill = ALBMoreLayerFill(layer);
    if (!card.saved) {
        card.saved = YES;
        card.fill = fill ? CGColorRetain(fill) : NULL;
        gALBMoreCardsCleared++;
    }
    if (fill && CGColorGetAlpha(fill) > 0.001) ALBMoreSetLayerFill(layer, UIColor.clearColor.CGColor);
    for (CALayer *sibling in layer.superlayer.sublayers) {
        if (sibling == layer || sibling.hidden || [sibling.delegate isKindOfClass:UIView.class]) continue;
        CGRect r = ALBMoreLayerRect(sibling);
        if (r.size.height < 1.5 && r.size.width >= 100) {
            sibling.hidden = YES;
            [card.dividers addObject:sibling];
            gALBMoreDividers++;
        }
    }
    __weak UIView *weakCell = cell;
    ALBPillOnRemove(cell, @"card", ^{
        UIView *c = weakCell;
        ALBMoreCard *saved = c ? objc_getAssociatedObject(c, &kALBMoreCardKey) : nil;
        if (saved.saved && saved.layer) ALBMoreSetLayerFill(saved.layer, saved.fill);
        for (CALayer *divider in saved.dividers.allObjects) divider.hidden = NO;
        if (c) objc_setAssociatedObject(c, &kALBMoreCardKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    });
}

/* 分区的底（AWEBaseListSectionBackgroundView，一组行后面那张卡片）：评论的分享面板里藏起来（每行已经是自己的胶囊） */
static char kALBMoreSectionKey;
static void ALBMoreSectionBackgroundSync(UIView *view) {
    if (!view.window || !NSThread.isMainThread) return;
    NSNumber *saved = objc_getAssociatedObject(view, &kALBMoreSectionKey);
    BOOL want = ALBMoreSheetsOn() && AWEPluginFeaturesEnabled() && ALBMoreInCommentPanel(view);
    if (want) {
        if (!saved) {
            objc_setAssociatedObject(view, &kALBMoreSectionKey, @(view.layer.opacity), OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            if (!gALBMoreSections) gALBMoreSections = [NSHashTable weakObjectsHashTable];
            [gALBMoreSections addObject:view];
            gALBMoreSectionHidden++;
        }
        if (view.layer.opacity != 0) view.layer.opacity = 0;
    } else if (saved) {
        view.layer.opacity = saved.floatValue;
        objc_setAssociatedObject(view, &kALBMoreSectionKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
}

#pragma mark - 分享面板

/* 关闭键：它自己换成系统玻璃按钮配置（胶囊、无内边距），图是原来的图去掉白 / 灰圆底——按下去玻璃和叉一起变形（
   “official” 做法）。原来的配置、图、深浅记着，关掉还原 */
@interface ALBMoreCloseState : NSObject
@property (nonatomic, strong) UIButtonConfiguration *configuration;
@property (nonatomic, strong) UIImage *image;
@property (nonatomic, assign) UIUserInterfaceStyle style;
@property (nonatomic, strong) UIImage *ours;          // 换上去的图（配置里还是它＝还是这里的配置）
@end
@implementation ALBMoreCloseState
@end
static char kALBMoreCloseKey;
static void ALBMoreCloseRestore(UIButton *button) {
    ALBMoreCloseState *state = button ? objc_getAssociatedObject(button, &kALBMoreCloseKey) : nil;
    if (!state) return;
    button.configuration = state.configuration;
    if (!state.configuration && state.image) [button setImage:state.image forState:UIControlStateNormal];
    button.overrideUserInterfaceStyle = state.style;
    objc_setAssociatedObject(button, &kALBMoreCloseKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBMoreCloseApply(UIButton *button, UIUserInterfaceStyle style) {
    if (@available(iOS 26.0, *)) {
        ALBMoreCloseState *state = objc_getAssociatedObject(button, &kALBMoreCloseKey);
        if (!state) {
            state = [ALBMoreCloseState new];
            state.configuration = button.configuration;
            state.image = button.currentImage ?: button.imageView.image ?: button.configuration.image;
            state.style = button.overrideUserInterfaceStyle;
            objc_setAssociatedObject(button, &kALBMoreCloseKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
            gALBMoreCloseButtons++;
        }
        if (!state.image) return;                 // 读不到原来的图：不换（不然就是一颗空玻璃）
        if (!state.ours || button.configuration.image != state.ours) {
            UIButtonConfiguration *configuration = [UIButtonConfiguration glassButtonConfiguration];
            configuration.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
            configuration.contentInsets = NSDirectionalEdgeInsetsZero;
            state.ours = ALBPlateless(state.image);
            configuration.image = state.ours;
            button.configuration = configuration;
        }
        if (button.overrideUserInterfaceStyle != style) button.overrideUserInterfaceStyle = style;
    }
}
static UIButton *ALBMoreFindClose(UIView *root) {
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < queue.count && i < 300; i++) {
        UIView *v = queue[i];
        if (ALBPillOursView(v) || [v isKindOfClass:UICollectionView.class]) continue;
        if ([v isKindOfClass:UIButton.class] && [v.accessibilityLabel isEqualToString:@"关闭"] &&
            v.bounds.size.width <= 48 && v.bounds.size.height <= 48) return (UIButton *)v;
        [queue addObjectsFromArray:v.subviews];
    }
    return nil;
}

/* 3.11.45：外壳控制器
   （AWESharePanelContainerViewController）出现 / 布局时也同步一次（只挂内容控制器的话，第一次出现时内容控制器还没布局，
   而之后它不一定再布局）；底色槽就是内容控制器的视图，前提是它不透明。这里另外：内容视图本身不是不透明的那张底，就往里
   （外壳内容层、内容视图两层以内）找一张整宽、不透明的底；以及每次同步记下为什么套上 / 没套上（诊断【更多玻璃】） */
static BOOL ALBMoreCanBePlate(UIView *view);
static NSString *gALBMoreShareLast, *gALBMoreShareTree;
static NSUInteger gALBMoreShareSyncs, gALBMoreShareFallbackPlates;
static void ALBMoreDumpTree(UIView *view, UIView *ref, NSUInteger depth, NSUInteger maxDepth, NSMutableArray<NSString *> *lines) {
    if (!view || depth > maxDepth || lines.count >= 70) return;
    CGRect r = [view convertRect:view.bounds toView:ref];
    UIColor *bg = view.backgroundColor;
    NSString *effect = [view isKindOfClass:UIVisualEffectView.class] ? NSStringFromClass([((UIVisualEffectView *)view).effect class] ?: NSNull.class) : nil;
    [lines addObject:[NSString stringWithFormat:@"%@%@ (%.0f,%.0f,%.0f×%.0f)%@%@%@%@", [@"" stringByPaddingToLength:depth * 2 withString:@" " startingAtIndex:0],
        NSStringFromClass(view.class), r.origin.x, r.origin.y, r.size.width, r.size.height,
        bg ? [NSString stringWithFormat:@" 底α=%.2f", CGColorGetAlpha(bg.CGColor)] : @"",
        view.layer.cornerRadius > 0 ? [NSString stringWithFormat:@" r%.0f", view.layer.cornerRadius] : @"",
        effect ? [@" effect=" stringByAppendingString:effect] : @"", view.hidden ? @" 隐藏" : @""]];
    if ([view isKindOfClass:UICollectionView.class] || [view isKindOfClass:UITableView.class]) return;
    for (UIView *sub in view.subviews) ALBMoreDumpTree(sub, ref, depth + 1, maxDepth, lines);
}
static UIView *ALBMoreSharePlate(UIViewController *content, UIView *shell) {
    UIView *main = content.view;
    UIColor *color = main.backgroundColor;
    if (ALBBackingTaken(main) || (color && CGColorGetAlpha(color.CGColor) >= 0.99)) return main;
    CGFloat width = main.bounds.size.width, height = main.bounds.size.height;
    NSMutableArray<NSArray *> *queue = [NSMutableArray array];
    UIView *shellHost = [shell isKindOfClass:UIVisualEffectView.class] ? ((UIVisualEffectView *)shell).contentView : shell;
    if (shellHost && shellHost != main) [queue addObject:@[shellHost, @0]];
    [queue addObject:@[main, @0]];
    for (NSUInteger i = 0; i < queue.count && i < 120; i++) {
        UIView *v = queue[i][0];
        NSUInteger depth = [queue[i][1] unsignedIntegerValue];
        if (v != main && v != shellHost && ALBMoreCanBePlate(v) && !v.hidden) {
            UIColor *c = v.backgroundColor;
            BOOL taken = ALBBackingTaken(v) && objc_getAssociatedObject(v, &kALBMorePanelGlassKey);
            if (taken || (c && CGColorGetAlpha(c.CGColor) >= 0.99 && v.bounds.size.width >= width * 0.9 && v.bounds.size.height >= height * 0.5)) {
                gALBMoreShareFallbackPlates++;
                return v;
            }
        }
        if (depth < 2 && ![v isKindOfClass:UICollectionView.class]) for (UIView *sub in v.subviews) [queue addObject:@[sub, @(depth + 1)]];
    }
    return nil;
}

/* AWESharePanelViewController（内容控制器）：它的视图就是底色槽——底去掉、最底下挂玻璃（上圆角跟外壳同心）；关闭键 */
static void ALBMoreShareSync(UIViewController *content) {
    if (!content.isViewLoaded || !content.view.window || !NSThread.isMainThread) return;
    gALBMoreShareSyncs++;
    UIView *main = content.view;
    NSString *names = ALBMoreControllers(main);
    BOOL inLongPress = [names rangeOfString:@"LongPress"].location != NSNotFound;
    BOOL want = ALBMoreSheetsOn() && AWEPluginFeaturesEnabled() && [names rangeOfString:@"SharePanel"].location != NSNotFound && !inLongPress;
    UIButton *close = ALBMoreFindClose(main);
    UIViewController *container = content.parentViewController;
    while (container && [NSStringFromClass(container.class) rangeOfString:@"SharePanelContainer"].location == NSNotFound) container = container.parentViewController;
    UIView *shell = container ? ALBMoreShell(container) : nil;
    UIView *slot = want ? ALBMoreSharePlate(content, shell) : main;
    if (!gALBMoreShareTree && container.isViewLoaded) {
        NSMutableArray<NSString *> *lines = [NSMutableArray array];
        ALBMoreDumpTree(container.view, container.view, 0, 5, lines);
        gALBMoreShareTree = [lines componentsJoinedByString:@"\n"];
    }
    UIColor *mainColor = main.backgroundColor;
    NSString *(^record)(NSString *) = ^NSString *(NSString *result) {
        return [NSString stringWithFormat:@"%@（内容视图 %@ 底α=%.2f；底板 %@；外壳 %@ r%.0f；长按面板里=%@；控制器：%@）", result,
            NSStringFromClass(main.class), mainColor ? CGColorGetAlpha(mainColor.CGColor) : -1.0,
            slot ? NSStringFromClass(slot.class) : @"没找到", shell ? NSStringFromClass(shell.class) : @"-", shell.layer.cornerRadius,
            inLongPress ? @"是" : @"否", names.length > 220 ? [names substringToIndex:220] : names];
    };
    if (!want) {
        ALBMorePanelRestore(slot);
        ALBMoreCloseRestore(close);
        gALBMoreShareLast = record(@"不套（开关关、不在分享面板或在长按面板里）");
        return;
    }
    if (!gALBMoreShareContents) gALBMoreShareContents = [NSHashTable weakObjectsHashTable];
    [gALBMoreShareContents addObject:content];
    ALBMoreObserveScene(main);
    CGFloat radius = shell.layer.cornerRadius > 0 ? shell.layer.cornerRadius : 20;     // 上圆角跟外壳同心，最少外壳的（没有就 20）
    BOOL applied = NO;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if (slot) applied = ALBMorePanelApply(slot, -radius);
    [CATransaction commit];
    if (close) ALBMoreCloseApply(close, ALBMoreStyle(close));
    UIVisualEffectView *glass = slot ? objc_getAssociatedObject(slot, &kALBMorePanelGlassKey) : nil;
    gALBMoreShareLast = record(applied ? [NSString stringWithFormat:@"套上了（玻璃 %@ %@）", glass.effect ? NSStringFromClass(glass.effect.class) : @"效果还没装",
                                          NSStringFromCGRect(glass.frame)]
                                       : (slot ? @"找到底板但没套上（底不是不透明的，也没套过）" : @"没套：内容视图不是不透明的，往里也没找到整宽不透明的底"));
}
static UIViewController *ALBMoreShareContentOf(UIViewController *controller, NSUInteger depth) {
    if (!controller || depth > 4) return nil;
    if ([NSStringFromClass(controller.class) isEqualToString:@"AWESharePanelViewController"]) return controller;
    if ([controller isKindOfClass:UINavigationController.class]) return ALBMoreShareContentOf(((UINavigationController *)controller).topViewController, depth + 1);
    for (UIViewController *child in controller.childViewControllers) {
        UIViewController *found = ALBMoreShareContentOf(child, depth + 1);
        if (found) return found;
    }
    return nil;
}

/* 功能格子（AWESharePanelFunctionCell，图标 56）：图标那一圈是玻璃，图里自带的白 / 灰圆底去掉，图标视图自己的底色不画 */
static void ALBMoreShareFunctionSync(UIView *cell) {
    if (!cell.window || !ALBMoreGlassReady()) return;
    BOOL want = ALBMoreSheetsOn() && !cell.hidden && ALBMoreInPanels(cell);
    UIView *content = [cell isKindOfClass:UICollectionViewCell.class] ? ((UICollectionViewCell *)cell).contentView : cell;
    UIView *icon = want ? ALBMoreGetter(cell, @"imageView") : nil;
    if (![icon isKindOfClass:UIImageView.class] || !icon.superview || icon.hidden || icon.bounds.size.width < 30) { ALBPillApply(cell, nil, NULL); return; }
    CGRect r = [icon.superview convertRect:icon.frame toView:cell];
    ALBPillSpec *spec = [ALBPillSpec new];
    spec.frame = r;
    spec.sources = ALBPillSourcesInRect(cell, content, cell, CGRectInset(r, -3, -3));
    spec.destain = YES;
    spec.style = ALBMoreStyle(cell);
    spec.area = @"分享面板功能";
    ALBPillApply(cell, spec, ALBMoreShareFunctionSync);
}

/* 选了联系人以后出来的输入层（AWEIMShareImpl.ShareAdditionTextView）：不透明的底 / 毛玻璃去掉、顶上那条细线藏起来，垫一块不按压的
   玻璃（它压在第三行功能图标上，只去底会透出来）；评论的分享面板里它落在输入白垫的玻璃上，只去底不垫 */
@interface ALBMoreOverlayState : NSObject
@property (nonatomic, strong) UIColor *color;
@property (nonatomic, assign) BOOL saved;
@property (nonatomic, strong) UIVisualEffect *effect;
@property (nonatomic, assign) BOOL effectSaved;
@property (nonatomic, strong) UIVisualEffectView *glass;
@property (nonatomic, weak) UIView *divider;
@end
@implementation ALBMoreOverlayState
@end
static char kALBMoreOverlayKey;
static void ALBMoreOverlayRestore(UIView *overlay) {
    ALBMoreOverlayState *state = objc_getAssociatedObject(overlay, &kALBMoreOverlayKey);
    if (!state) return;
    [state.glass removeFromSuperview];
    if (state.saved) overlay.backgroundColor = state.color;
    if (state.effectSaved && [overlay isKindOfClass:UIVisualEffectView.class]) ((UIVisualEffectView *)overlay).effect = state.effect;
    state.divider.hidden = NO;
    objc_setAssociatedObject(overlay, &kALBMoreOverlayKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}
static void ALBMoreOverlaySync(UIView *overlay) {
    if (!overlay.window || !NSThread.isMainThread) return;
    CGSize s = overlay.bounds.size;
    BOOL want = ALBMoreSheetsOn() && AWEPluginFeaturesEnabled() && !overlay.hidden && s.width >= 100 && s.height >= 30 && ALBMoreInPanels(overlay);
    if (!want) { ALBMoreOverlayRestore(overlay); return; }
    ALBMoreOverlayState *state = objc_getAssociatedObject(overlay, &kALBMoreOverlayKey);
    if (!state) {
        state = [ALBMoreOverlayState new];
        objc_setAssociatedObject(overlay, &kALBMoreOverlayKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!gALBMoreOverlayViews) gALBMoreOverlayViews = [NSHashTable weakObjectsHashTable];
        [gALBMoreOverlayViews addObject:overlay];
    }
    UIColor *color = overlay.backgroundColor;
    if (color && CGColorGetAlpha(color.CGColor) >= 0.99) {
        if (!state.saved) { state.saved = YES; state.color = color; gALBMoreOverlays++; }
        overlay.backgroundColor = UIColor.clearColor;
    }
    if ([overlay isKindOfClass:UIVisualEffectView.class] && ((UIVisualEffectView *)overlay).effect) {
        if (!state.effectSaved) { state.effectSaved = YES; state.effect = ((UIVisualEffectView *)overlay).effect; }
        ((UIVisualEffectView *)overlay).effect = nil;
    }
    UIView *host = [overlay isKindOfClass:UIVisualEffectView.class] ? ((UIVisualEffectView *)overlay).contentView : overlay;
    if (!state.divider) {
        for (UIView *sub in host.subviews) {
            CGFloat height = sub.bounds.size.height;
            if (height > 0.1 && height < 1.5 && sub.bounds.size.width >= 200 && CGRectGetMinY(sub.frame) < 16) { state.divider = sub; break; }
        }
    }
    if (state.divider && !state.divider.hidden) state.divider.hidden = YES;
    BOOL inComment = ALBMoreInCommentPanel(overlay);
    if (inComment || (!state.saved && !state.effectSaved)) { [state.glass removeFromSuperview]; return; }
    if (!state.glass) state.glass = ALBMoreMakeGlass();
    ALBMoreGlassPlace(state.glass, host, host.bounds, ALBMoreStyle(overlay), 0, 0);
}

/* 输入层里的“发送”（ShareAdditionTextButton，408×44 圆角 8）：玻璃＝按钮那么大、圆角跟按钮（没有就 8）；原来是红底的（或者
   标题是“发送”）用红色染的玻璃，字画在玻璃里 */
static void ALBMoreSendSync(UIView *button) {
    if (!button.window || !ALBMoreGlassReady()) return;
    CGSize s = button.bounds.size;
    BOOL want = ALBMoreSheetsOn() && !button.hidden && button.alpha > 0.01 && s.width >= 40 && s.height >= 28 && s.height <= 64 && ALBMoreInPanels(button);
    if (!want) { ALBPillApply(button, nil, NULL); return; }
    NSMutableArray<UIView *> *sources = [NSMutableArray array];
    for (UIView *sub in ALBChatPillSources(button, ALBPillOf(button))) {
        // 按钮配置自己画的底（满幅、类名带 Background）不画进去：颜色交给染色的玻璃
        BOOL fill = [NSStringFromClass(sub.class) rangeOfString:@"Background"].location != NSNotFound &&
                    sub.bounds.size.width >= s.width - 2 && sub.bounds.size.height >= s.height - 2;
        if (!fill) [sources addObject:sub];
    }
    ALBPillSpec *spec = [ALBPillSpec new];
    spec.frame = button.bounds;
    spec.sources = sources;
    spec.radius = button.layer.cornerRadius > 0.5 ? button.layer.cornerRadius : 8;
    spec.autoTint = YES;
    if ([button isKindOfClass:UIButton.class]) {
        UIButtonConfiguration *configuration = ((UIButton *)button).configuration;
        UIColor *native = configuration.baseBackgroundColor ?: configuration.background.backgroundColor;
        UIColor *tint = ALBPillSaturatedPlate(native, button.traitCollection);
        NSString *title = ((UIButton *)button).currentTitle ?: button.accessibilityLabel ?: @"";
        if (!tint && !ALBPillStateOf(button).saved && [title rangeOfString:@"发送"].location != NSNotFound && [title rangeOfString:@"建群"].location == NSNotFound)
            tint = [UIColor colorWithRed:0.9961 green:0.1725 blue:0.3333 alpha:1];
        if (tint) spec.tint = tint;
    }
    spec.style = ALBMoreStyle(button);
    spec.area = @"面板发送键";
    ALBPillApply(button, spec, ALBMoreSendSync);
}

#pragma mark - 评论的分享面板的整块底

/* 外壳 contentView 里那张满幅不透明底换玻璃（上圆角跟外壳同心）；选了人以后，列表里罩住输入层的那块白垫也换玻璃。3.11.44：深浅跟系统 */
static UIViewController *ALBMoreCommentSheetOf(UIView *view) {
    for (UIResponder *r = view.nextResponder; r; r = r.nextResponder)
        if ([r isKindOfClass:UIViewController.class] && [NSStringFromClass(r.class) rangeOfString:@"CommentLongPressPanelSheet"].location != NSNotFound)
            return (UIViewController *)r;
    return nil;
}
static BOOL ALBMoreCanBePlate(UIView *view) {
    return ![view isKindOfClass:UIScrollView.class] && ![view isKindOfClass:UIVisualEffectView.class] && ![view isKindOfClass:UIControl.class] &&
           ![view isKindOfClass:UIImageView.class] && ![view isKindOfClass:UILabel.class] && !ALBPillOursView(view);
}
/* 3.11.46 用户：“评论区分享面板，选择某一个人分享之后，中间部分有一个半透明的东西”。选了人以后抖音在操作行上面盖一张
   不透明的白垫（从联系人那一排下面一直到底，底部是“分享此刻的想法 / 发送”那层）——原来就是用它把操作行整块盖掉。
   3.11.44 起这张垫整张换成了玻璃，玻璃是透的，下面的操作行就隔着它露出来，成了中间那块半透明的东西。现在：玻璃只铺在
   输入层那一段，垫上面那一段只去底；被它盖住的那几行（转发评论到日常、复制、收藏、举报……）藏起来（和抖音原来一样看不到），
   取消选择、输入层收起来时放回来。 */
static NSHashTable<UIView *> *gALBMoreHiddenRows;
static NSString *gALBMoreCommentPadLast;
static void ALBMoreCommentRowsRestore(void) {
    for (UIView *row in gALBMoreHiddenRows.allObjects) if (row.alpha < 0.01) row.alpha = 1;
    [gALBMoreHiddenRows removeAllObjects];
}
static void ALBMoreCommentPadApply(UIView *pad, UIView *overlay, UIViewController *list) {
    CGRect overlayRect = [overlay convertRect:overlay.bounds toView:pad];
    UIColor *color = pad.backgroundColor;
    BOOL taken = ALBBackingTaken(pad);
    if (!taken && !(color && CGColorGetAlpha(color.CGColor) >= 0.99)) return;
    UIVisualEffectView *glass = objc_getAssociatedObject(pad, &kALBMorePanelGlassKey);
    ALBBackingClear(pad);
    if (!glass) {
        glass = ALBMoreMakeGlass();
        objc_setAssociatedObject(pad, &kALBMorePanelGlassKey, glass, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!gALBMorePanelSlots) gALBMorePanelSlots = [NSHashTable weakObjectsHashTable];
        [gALBMorePanelSlots addObject:pad];
        gALBMorePanelGlass++;
    }
    glass.autoresizingMask = UIViewAutoresizingNone;
    CGRect area = CGRectIntersection(pad.bounds, overlayRect);
    if (CGRectIsNull(area) || CGRectIsEmpty(area)) area = pad.bounds;
    ALBMoreGlassPlace(glass, pad, area, ALBMoreStyle(pad), 0, 0);
    // 垫上面、输入层以上被盖住的行：藏起来
    if (!gALBMoreHiddenRows) gALBMoreHiddenRows = [NSHashTable weakObjectsHashTable];
    NSUInteger hidden = 0;
    CGFloat coverTop = 0, coverBottom = CGRectGetMinY(overlayRect);
    UIView *root = list.viewIfLoaded;
    NSMutableArray<UIView *> *queue = root ? [NSMutableArray arrayWithObject:root] : [NSMutableArray array];
    for (NSUInteger i = 0; i < queue.count && i < 300; i++) {
        UIView *v = queue[i];
        if (v == pad || v == overlay || [v isDescendantOfView:pad] || [v isDescendantOfView:overlay]) continue;
        if ([v isKindOfClass:UICollectionViewCell.class]) {
            CGRect r = [v convertRect:v.bounds toView:pad];
            if (CGRectGetMinY(r) >= coverTop - 1 && CGRectGetMaxY(r) <= coverBottom + 1 && r.size.height > 8) {
                if (v.alpha > 0.01) v.alpha = 0;
                [gALBMoreHiddenRows addObject:v];
                hidden++;
            } else if ([gALBMoreHiddenRows containsObject:v] && v.alpha < 0.01) {
                v.alpha = 1;
                [gALBMoreHiddenRows removeObject:v];
            }
            continue;
        }
        [queue addObjectsFromArray:v.subviews];
    }
    gALBMoreCommentPadLast = [NSString stringWithFormat:@"垫 %@ 输入层 %@ 玻璃铺 %@ 藏了 %lu 行",
        NSStringFromCGRect([pad convertRect:pad.bounds toView:nil]), NSStringFromCGRect([overlay convertRect:overlay.bounds toView:nil]),
        NSStringFromCGRect(area), (unsigned long)hidden];
}

static void ALBMoreCommentSheetSync(UIViewController *sheet) {
    if (!sheet.isViewLoaded || !sheet.view.window || !NSThread.isMainThread) return;
    BOOL want = ALBMoreSheetsOn() && AWEPluginFeaturesEnabled();
    UIView *shell = ALBMoreShell(sheet);
    UIView *host = [shell isKindOfClass:UIVisualEffectView.class] ? ((UIVisualEffectView *)shell).contentView : sheet.view;
    UIView *slot = nil;
    for (UIView *sub in host.subviews) {
        if (ALBBackingTaken(sub) && objc_getAssociatedObject(sub, &kALBMorePanelGlassKey)) { slot = sub; break; }
        if (!ALBMoreCanBePlate(sub)) continue;
        UIColor *color = sub.backgroundColor;
        if (!color || CGColorGetAlpha(color.CGColor) < 0.99) continue;
        if (fabs(sub.bounds.size.width - host.bounds.size.width) > 1 || fabs(sub.bounds.size.height - host.bounds.size.height) > 1) continue;
        slot = sub;
        break;
    }
    // 列表控制器里的输入层和它后面那块白垫
    UIViewController *list = nil;
    for (UIViewController *child in sheet.childViewControllers)
        if ([NSStringFromClass(child.class) rangeOfString:@"CommentLongPressPanelCollectionViewController"].location != NSNotFound) { list = child; break; }
    UIView *overlay = nil, *pad = nil;
    Class overlayClass = objc_getClass("AWEIMShareImpl.ShareAdditionTextView");
    for (UIView *sub in list.viewIfLoaded.subviews) if (overlayClass && [sub isKindOfClass:overlayClass]) { overlay = sub; break; }
    for (UIView *sub in list.viewIfLoaded.subviews) {
        if (sub == overlay) continue;
        if (ALBBackingTaken(sub) && objc_getAssociatedObject(sub, &kALBMorePanelGlassKey)) { pad = sub; break; }
        if (!overlay || !ALBMoreCanBePlate(sub)) continue;
        UIColor *color = sub.backgroundColor;
        if (!color || CGColorGetAlpha(color.CGColor) < 0.99) continue;
        if (CGRectGetMinY(sub.frame) > CGRectGetMinY(overlay.frame) + 1 || CGRectGetMaxY(sub.frame) < CGRectGetMaxY(overlay.frame) - 1 ||
            sub.bounds.size.width < overlay.bounds.size.width - 1) continue;
        pad = sub;
        break;
    }
    if (!want) {
        ALBMorePanelRestore(slot);
        ALBMorePanelRestore(pad);
        ALBMoreCommentRowsRestore();
        return;
    }
    if (!gALBMoreCommentSheets) gALBMoreCommentSheets = [NSHashTable weakObjectsHashTable];
    [gALBMoreCommentSheets addObject:sheet];
    ALBMoreObserveScene(sheet.view);
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if (slot) ALBMorePanelApply(slot, -(shell.layer.cornerRadius > 0 ? shell.layer.cornerRadius : 20));
    if (pad) {
        if (overlay && !overlay.hidden && overlay.alpha > 0.01) ALBMoreCommentPadApply(pad, overlay, list);
        else { ALBMorePanelRestore(pad); ALBMoreCommentRowsRestore(); }
    } else if (gALBMoreHiddenRows.count) ALBMoreCommentRowsRestore();
    [CATransaction commit];
}
static void ALBMoreCommentSheetSyncFrom(UIView *view) {
    UIViewController *sheet = ALBMoreCommentSheetOf(view);
    if (sheet) ALBMoreCommentSheetSync(sheet);
}

#pragma mark - 图文详情页：只有底部评论框

/* 里面有能打字的框：不盖（盖上以后点不进去） */
static BOOL ALBMoreHasEditableText(UIView *view) {
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:view.subviews];
    for (NSUInteger i = 0; i < queue.count && i < 60; i++) {
        UIView *v = queue[i];
        if ([v isKindOfClass:UITextField.class] && ((UITextField *)v).enabled) return YES;
        if ([v isKindOfClass:UITextView.class] && ((UITextView *)v).editable) return YES;
        [queue addObjectsFromArray:v.subviews];
    }
    return NO;
}
/* 3.11.46 用户：“我可能需要抓一下评论区未打开时那个区域有一条线，再加上玻璃会有一个边缘导致它重叠在一起，看着累赘”。
   图文页底栏（FullPageFeedBottomBarView）顶边抖音画了一条细线，评论框的玻璃胶囊自己又有一圈边。这里：底栏顶边那条细线
   （高不到 1.5pt、宽过六成、贴着底栏顶边）藏起来；第一次见到时把底栏整棵树记下来（诊断【更多玻璃】）。 */
static NSString *gALBMoreNoteBarTree, *gALBMoreNoteLineLast;
static NSUInteger gALBMoreNoteLinesHidden;
static void ALBMoreNoteBarClean(UIView *input) {
    UIView *bar = nil;
    for (UIView *v = input.superview; v && !bar; v = v.superview)
        if ([NSStringFromClass(v.class) containsString:@"FullPageFeedBottomBarView"]) bar = v;
    if (!bar) bar = input.superview;
    if (!bar.window) return;
    if (!gALBMoreNoteBarTree) {
        NSMutableArray<NSString *> *lines = [NSMutableArray array];
        UIView *dumpRoot = bar.superview ?: bar;
        ALBMoreDumpTree(dumpRoot, bar.window, 0, 4, lines);
        gALBMoreNoteBarTree = [lines componentsJoinedByString:@"\n"];
    }
    CGRect barRect = [bar convertRect:bar.bounds toView:nil];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:bar.subviews];
    if (bar.superview) [queue addObjectsFromArray:bar.superview.subviews];
    for (NSUInteger i = 0; i < queue.count && i < 200; i++) {
        UIView *v = queue[i];
        if (v == bar || v.hidden) continue;
        if (![v isKindOfClass:UIVisualEffectView.class] && [v isDescendantOfView:bar]) [queue addObjectsFromArray:v.subviews];
        if ([v isKindOfClass:UILabel.class] || [v isKindOfClass:UIControl.class] || [v isKindOfClass:UIVisualEffectView.class] ||
            [v isKindOfClass:UIImageView.class]) continue;
        CGRect r = [v convertRect:v.bounds toView:nil];
        BOOL thin = r.size.height > 0 && r.size.height <= 1.5;
        BOOL wide = r.size.width >= barRect.size.width * 0.6;
        BOOL nearTop = fabs(CGRectGetMinY(r) - CGRectGetMinY(barRect)) <= 4.0 || fabs(CGRectGetMaxY(r) - CGRectGetMinY(barRect)) <= 2.0;
        UIColor *bg = v.backgroundColor;
        if (!thin || !wide || !nearTop || !((bg && CGColorGetAlpha(bg.CGColor) > 0.02) || v.layer.backgroundColor)) continue;
        v.hidden = YES;
        gALBMoreNoteLinesHidden++;
        gALBMoreNoteLineLast = [NSString stringWithFormat:@"%@ %@", NSStringFromClass(v.class), NSStringFromCGRect(r)];
    }
}
/* FullPageFeedBottomBarView.CommentInputView（212×40）：一块胶囊（原来的灰底去掉）；用户：图文页别的都不动 */
static void ALBMoreNoteInputSync(UIView *input) {
    if (!input.window || !ALBMoreGlassReady()) return;
    if (ALBMoreNoteOn()) ALBMoreNoteBarClean(input);
    CGSize s = input.bounds.size;
    BOOL editable = ALBMoreHasEditableText(input);
    if (editable && !ALBPillStateOf(input)) gALBMoreEditableSkips++;
    BOOL want = ALBMoreNoteOn() && !input.hidden && input.alpha > 0.01 && s.width >= 80 && s.height >= 24 && s.height <= 60 && !editable;
    if (!want) { ALBPillApply(input, nil, NULL); return; }
    ALBPillSpec *spec = [ALBPillSpec new];
    spec.frame = input.bounds;
    spec.sources = ALBChatPillSources(input, ALBPillOf(input));
    spec.area = @"图文页评论框";
    ALBPillApply(input, spec, ALBMoreNoteInputSync);
}

#pragma mark - 装钩子、设置改了、诊断

static void ALBMoreGlassOptionsChanged(void) {
    for (UIView *owner in gALBChatPillOwners.allObjects) if (ALBPillStateOf(owner)) ALBPillRunSyncer(owner);
    for (UIView *view in gALBMoreSections.allObjects) ALBMoreSectionBackgroundSync(view);
    for (UIView *overlay in gALBMoreOverlayViews.allObjects) ALBMoreOverlaySync(overlay);
    for (UIViewController *content in gALBMoreShareContents.allObjects) ALBMoreShareSync(content);
    for (UIViewController *sheet in gALBMoreCommentSheets.allObjects) ALBMoreCommentSheetSync(sheet);
    if (!ALBMoreSheetsOn()) for (UIView *slot in gALBMorePanelSlots.allObjects) ALBMorePanelRestore(slot);
}

/* 系统深浅一换（场景的特征变化），装过的全部按新的深浅重做 */
static __weak UIWindowScene *gALBMoreObservedScene;
static void ALBMoreObserveScene(UIView *view) {
    UIWindowScene *scene = view.window.windowScene;
    if (!scene || scene == gALBMoreObservedScene) return;
    gALBMoreObservedScene = scene;
    if (@available(iOS 17.0, *)) {
        [scene registerForTraitChanges:@[UITraitUserInterfaceStyle.class]
                           withHandler:^(UIWindowScene *changed, __unused UITraitCollection *previous) {
            gALBMoreStyleFlips++;
            gALBMoreStyleLast = changed.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark ? @"深色" : @"浅色";
            ALBMoreGlassOptionsChanged();
        }];
    }
}

static void ALBMoreGlassInstall(void) {
    if (AWEOperatingSystemMajorVersion() < 26) return;
    // 评论的分享面板
    Class row = ALBMoreClass(@[@"CommentLongPressPanelNormalBaseCell", @"AWECommentLongPressPanelSwiftImpl.CommentLongPressPanelNormalBaseCell",
                                @"AWECommentLongPressPanelSwiftImpl.CommentLongPressPanelNormalCell"]);
    if (row) {
        ALBMoreHookView(row, ALBMoreCommentRowSync);
        ALBMoreHook(row, @"setHighlighted:", ALBMoreHookBool, ^(id object) { ALBMoreCommentRowSync((UIView *)object); });
    }
    Class sheet = ALBMoreClass(@[@"AWECommentLongPressPanelSwiftImpl.CommentLongPressPanelSheet"]);
    if (sheet) {
        ALBMoreAfter after = ^(id object) { ALBMoreCommentSheetSync((UIViewController *)object); };
        ALBMoreHook(sheet, @"viewDidLayoutSubviews", ALBMoreHookVoid, after);
        ALBMoreHook(sheet, @"viewWillAppear:", ALBMoreHookBool, after);
    }
    ALBMoreHookView(ALBMoreClass(@[@"AWEBaseListSectionBackgroundView"]), ALBMoreSectionBackgroundSync);
    // 分享面板
    Class shareContent = ALBMoreClass(@[@"AWESharePanelViewController"]);
    if (shareContent) {
        ALBMoreAfter after = ^(id object) { ALBMoreShareSync((UIViewController *)object); };
        ALBMoreHook(shareContent, @"viewDidLayoutSubviews", ALBMoreHookVoid, after);
        ALBMoreHook(shareContent, @"awe_themeReload", ALBMoreHookVoid, after);
    }
    // 3.11.45：外壳控制器出现 / 布局时也同步
    Class shareContainer = ALBMoreClass(@[@"AWESharePanelContainerViewController"]);
    if (shareContainer) {
        ALBMoreAfter after = ^(id object) {
            UIViewController *content = ALBMoreShareContentOf((UIViewController *)object, 0);
            if (content) ALBMoreShareSync(content);
        };
        ALBMoreHook(shareContainer, @"viewWillAppear:", ALBMoreHookBool, after);
        ALBMoreHook(shareContainer, @"viewDidLayoutSubviews", ALBMoreHookVoid, after);
        ALBMoreHook(shareContainer, @"viewDidAppear:", ALBMoreHookBool, after);
    }
    Class function = ALBMoreClass(@[@"AWESharePanelFunctionCell"]);
    if (function) {
        ALBMoreHookView(function, ALBMoreShareFunctionSync);
        ALBMoreHook(function, @"updateImageViewWithViewModel:", ALBMoreHookObject, ^(id object) { ALBMoreShareFunctionSync((UIView *)object); });
    }
    ALBMoreHookView(ALBMoreClass(@[@"AWEIMShareImpl.ShareAdditionTextView"]), ALBMoreOverlaySync);
    ALBMoreHookView(ALBMoreClass(@[@"_TtC14AWEIMShareImplP33_419E925875FC93479048AF7B3AAD3DB623ShareAdditionTextButton"]), ALBMoreSendSync);
    // 图文详情页：只有评论框
    ALBMoreHookView(ALBMoreClass(@[@"_TtCC27AWEKnowledgeRichContentImpl25FullPageFeedBottomBarView16CommentInputView"]), ALBMoreNoteInputSync);
    [[NSNotificationCenter defaultCenter] addObserverForName:@"ALB5.OptionsChanged" object:nil queue:NSOperationQueue.mainQueue
                                                  usingBlock:^(__unused NSNotification *note) { ALBMoreGlassOptionsChanged(); }];
}

/* 诊断（ALB5Settings.h 的【更多玻璃】） */
NSString *ALBBindGlassReport(void) {
    UIWindowScene *scene = gALBMoreObservedScene;
    NSString *now = !scene ? @"还没打开过面板" : (scene.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark ? @"深色" : @"浅色");
    return [NSString stringWithFormat:@"%@；3.11.47 评论长按面板里套着横向列表的行不盖玻璃 %lu 次；3.11.46 评论分享面板选人后：%@；图文页底栏顶边细线藏 %lu 条（最近 %@）\n图文页底栏结构（第一次见到时）：\n%@\n3.11.45 分享面板：同步 %lu 次，往里找到底板 %lu 次；最近一次：%@；评论长按面板深色模式下锁浅色 %lu 次\n分享面板结构（第一次打开时）：\n%@\n3.11.44（深浅全部跟系统＝%@；系统深浅切换 %lu 次，最近一次 %@）：钩上 %@；没找到 %@；面板整块底换玻璃 %lu 张；关闭键换玻璃按钮 %lu 个；评论分享面板分区底藏 %lu 个、卡片底去掉 %lu 块、分割线藏 %lu 次；输入层换玻璃 %lu 个；评论框里有能打字的框没盖 %lu 次",
        ALBChatReportPart(), (unsigned long)gALBMoreNestedRowsSkipped, gALBMoreCommentPadLast ?: @"还没选过人", (unsigned long)gALBMoreNoteLinesHidden, gALBMoreNoteLineLast ?: @"-",
        gALBMoreNoteBarTree ?: @"还没打开过图文作品", (unsigned long)gALBMoreShareSyncs, (unsigned long)gALBMoreShareFallbackPlates, gALBMoreShareLast ?: @"还没打开过分享面板",
        (unsigned long)gALBMoreCommentLightLocks, gALBMoreShareTree ?: @"还没打开过分享面板",
        now, (unsigned long)gALBMoreStyleFlips, gALBMoreStyleLast ?: @"-",
        gALBMoreHooked.count ? [gALBMoreHooked.array componentsJoinedByString:@"、"] : @"-",
        gALBMoreMissing.count ? [gALBMoreMissing.array componentsJoinedByString:@"、"] : @"无",
        (unsigned long)gALBMorePanelGlass, (unsigned long)gALBMoreCloseButtons,
        (unsigned long)gALBMoreSectionHidden, (unsigned long)gALBMoreCardsCleared, (unsigned long)gALBMoreDividers,
        (unsigned long)gALBMoreOverlays, (unsigned long)gALBMoreEditableSkips];
}
