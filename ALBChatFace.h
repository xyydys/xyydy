/* 3.11.44 胶囊（系统玻璃按钮，图＝原来的内容，按下去和玻璃一起变形）——分享面板、评论长按面板、图文页评论框共用这一套。
   Included by Tweak.x right after ALBBarFace.h (same compilation unit: uses its static helpers — ALBFaceTapOnTouchUp 等).

   用户（3.11.44）：“聊天页整个液态玻璃都撤回吧，不要了……撤销聊天页的玻璃覆盖”。3.11.40–3.11.43 聊天页的全部玻璃
   （输入栏 / 顶栏按钮的栏按钮 face、群功能栏 / 粉丝群 / “按住 说话”的胶囊、输入框长底、加号面板格子、顶栏排法、表情栏）
   全部拿掉：钩子、设置项（bind.chatInput）、代码都删了，聊天页回到抖音原样。留下的只是胶囊这套机制本身（“变形机制保留”）。
   设置「更多玻璃」→ bind.sheets（分享面板与评论长按面板）、bind.note（图文页评论框）。 */

/* 3.11.42：评论长按面板 / 分享面板、图文详情页各有自己的开关（「更多玻璃」） */
static BOOL ALBMoreSheetsOn(void) { return AWEOperatingSystemMajorVersion() >= 26 && ALB5Bool(@"bind.sheets", YES); }
static BOOL ALBMoreNoteOn(void) { return AWEOperatingSystemMajorVersion() >= 26 && ALB5Bool(@"bind.note", YES); }
static UIImage *ALBPlateless(UIImage *image);
static void ALBPillImageChanged(UIImageView *imageView);
static NSMutableOrderedSet<NSString *> *gALBPillClasses;

#pragma mark - 3.11.41 / 3.11.42 胶囊：系统玻璃按钮，图＝原来的内容

/* 栏按钮（face）的玻璃只有圆和“按字宽的胶囊”两种，长条、带图标又带字的小胶囊放不进去。这些用系统玻璃按钮（UIButton 的
   玻璃配置，右侧 3.11.0–3.11.6 用过的那种，系统按压时按钮自己的图片跟着一起变形）：按钮的图片＝原来内容（图标＋字）
   原样画成的一张图，原来的内容藏起来（layer.opacity 0），原来的底色去掉（记着）。形状：胶囊。

   3.11.42：同一套给所有地方用（评论长按面板、分享面板、图文详情页；3.11.44 起聊天页不用了），每处给一份说明（ALBPillSpec）：
   · host：玻璃按钮放在哪（默认就是 owner；owner 自己要画进去的——图片视图、文字——放到它上面第一个接触摸的视图里）
   · frame：多大（host 坐标）；sources：画进去、藏起来的原视图；plate：垫一层原来的底色（红色的“发送”“关注”）；
     destain：去掉图里的白 / 灰圆底（分享面板的功能图标）；center：图里的内容按墨迹居中（小图标）；style：深浅（0＝看字色）
   · 触摸：先到玻璃按钮（系统按压），每个阶段转给原来的视图：落在原内容里的按钮（UIControl）给它，别的给 owner（格子 →
     列表的选中，“按住 说话” → 它自己的触摸）；原内容上的点按 / 长按手势搬到玻璃按钮上（原内容藏起来以后点不到），拿掉时
     搬回去；owner 和它上面的手势（长按说话、列表滑动）是玻璃按钮的祖先，照样看得到。按钮只认自己跟踪的点按，和双击面板一样补一次
   · 列表里：从玻璃按钮上开始滑照样能滑（UIScrollView 默认不替按钮取消触摸，钩子里对这里的玻璃按钮放行）
   修了 3.11.41 的一个错：藏起来的原内容 alpha 读出来是 0，下一次布局被当成“看不见”漏掉，图就画空了。
   3.11.43：玻璃换成系统自己的按钮配置（Regular；有颜色的用 prominent 染色），深浅由 overrideUserInterfaceStyle 定死；
   可以固定圆角（发送键 8）；位置、大小由 ALBMoreGlass.h 算。 */
@interface ALBPillSpec : NSObject
@property (nonatomic, weak) UIView *host;
@property (nonatomic, assign) CGRect frame;
@property (nonatomic, copy) NSArray<UIView *> *sources;
@property (nonatomic, strong) UIColor *plate;
@property (nonatomic, assign) BOOL autoTint;         // 3.11.43：原来的底色是彩色（红）：用这个颜色染的玻璃（发送键）
@property (nonatomic, strong) UIColor *tint;         // 染色玻璃（prominent glass）
@property (nonatomic, assign) CGFloat radius;        // 0＝胶囊；>0 固定圆角
@property (nonatomic, weak) UIControl *tapControl;   // 点在胶囊里、底下又不是别的按钮时，算这颗按钮的点按（返回＋未读数那块 → 返回键）
@property (nonatomic, assign) BOOL destain;
@property (nonatomic, assign) BOOL center;
@property (nonatomic, assign) UIUserInterfaceStyle style;
@property (nonatomic, copy) NSString *area;
@end
@implementation ALBPillSpec
@end

typedef void (*ALBPillSyncFn)(UIView *owner);

@interface ALBChatPillButton : UIButton
@property (nonatomic, weak) UIView *owner;
@property (nonatomic, weak) UIView *touchTarget;
@property (nonatomic, weak) UIControl *touchControl;     // 认这次点按的按钮：目标自己，或玻璃按钮外面最近的那个按钮
@property (nonatomic, assign) BOOL cellTap;              // 3.11.47：这一下点在列表的格子上（自己替列表选中）
@end

static char kALBChatPillKey, kALBChatPillStateKey, kALBPillSyncerKey, kALBPillPendingKey;
static NSUInteger gALBChatPillInstalled, gALBChatPillRemoved, gALBChatPillRendered;
static NSUInteger gALBPillTouches, gALBPillControlTargets, gALBPillGesturesMoved, gALBPillTapSent, gALBPillScrollPass;
static NSUInteger gALBPillLive;
static NSUInteger gALBPillCellSelects, gALBPillCellRefused;
static NSString *gALBPillCellLast;
static NSHashTable<UIView *> *gALBChatPillOwners;
static NSMutableDictionary<NSString *, NSNumber *> *gALBPillAreas;

@interface ALBChatPillState : NSObject
@property (nonatomic, strong) UIColor *background;
@property (nonatomic, assign) CGColorRef layerBackground;
@property (nonatomic, assign) BOOL saved;
@property (nonatomic, strong) UIColor *contentBackground;
@property (nonatomic, assign) BOOL contentSaved;
@property (nonatomic, strong) NSHashTable<UIView *> *hidden;                         // 藏起来的原内容（opacity 0）
@property (nonatomic, strong) NSMapTable<UIView *, NSNumber *> *opacity;             // 它们原来的 opacity
@property (nonatomic, strong) NSMapTable<UIGestureRecognizer *, UIView *> *moved;   // 搬到玻璃按钮上的手势 → 原来的视图
@property (nonatomic, strong) NSMutableDictionary<NSString *, dispatch_block_t> *undo; // 各处自己的还原（卡片底、分割线）
@property (nonatomic, copy) NSString *signature;
@property (nonatomic, copy) NSString *area;
@property (nonatomic, strong) UIColor *configTint;      // 现在这颗玻璃按什么做的（变了重做配置）
@property (nonatomic, assign) CGFloat configRadius;
@property (nonatomic, assign) BOOL configured;
@property (nonatomic, weak) UIControl *tapControl;
@end
@implementation ALBChatPillState
- (void)dealloc { if (_layerBackground) CGColorRelease(_layerBackground); }
@end

static ALBChatPillState *ALBPillStateOf(UIView *owner) { return owner ? objc_getAssociatedObject(owner, &kALBChatPillStateKey) : nil; }
static ALBChatPillButton *ALBPillOf(UIView *owner) { return owner ? objc_getAssociatedObject(owner, &kALBChatPillKey) : nil; }

/* 看得见：没藏；alpha 是这里设的 0（藏起来画进玻璃里的）也算 */
static BOOL ALBPillVisible(UIView *view, ALBChatPillState *state) {
    if (!view || view.hidden) return NO;
    if (view.alpha > 0.01) return YES;
    return state && [state.hidden containsObject:view];
}
static BOOL ALBPillOursView(UIView *view) {
    return [view isKindOfClass:ALBChatPillButton.class] || [view.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar."];
}

/* 各处自己的还原（拿掉胶囊时跑一次） */
static void ALBPillOnRemove(UIView *owner, NSString *key, dispatch_block_t undo) {
    ALBChatPillState *state = ALBPillStateOf(owner);
    if (!state || !key || !undo || state.undo[key]) return;
    if (!state.undo) state.undo = [NSMutableDictionary dictionary];
    state.undo[key] = [undo copy];
}

static void ALBPillRunSyncer(UIView *owner) {
    NSValue *value = owner ? objc_getAssociatedObject(owner, &kALBPillSyncerKey) : nil;
    ALBPillSyncFn fn = (ALBPillSyncFn)value.pointerValue;
    if (fn) fn(owner);
}
/* 点完：选中换了（群功能栏的字变亮 / 变暗、点赞变红）——自己和同一排的稍后各重画一次 */
static void ALBPillResyncSoon(UIView *owner, BOOL siblings) {
    if (!owner) return;
    __weak UIView *weakOwner = owner;
    for (NSNumber *delay in @[@0.15, @0.6]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay.doubleValue * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            UIView *o = weakOwner;
            if (!o) return;
            if (siblings)
                for (UIView *sibling in o.superview.subviews.copy)
                    if (sibling != o && ALBPillStateOf(sibling)) ALBPillRunSyncer(sibling);
            if (ALBPillStateOf(o)) ALBPillRunSyncer(o);
        });
    }
}
/* 图换了（头像到了、点赞变红）：这一轮结束时重画一次（同一拍里多次只算一次） */
static void ALBPillResyncLater(UIView *owner) {
    if (!owner || objc_getAssociatedObject(owner, &kALBPillPendingKey)) return;
    objc_setAssociatedObject(owner, &kALBPillPendingKey, @YES, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    __weak UIView *weakOwner = owner;
    dispatch_async(dispatch_get_main_queue(), ^{
        UIView *o = weakOwner;
        if (!o) return;
        objc_setAssociatedObject(o, &kALBPillPendingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        ALBPillRunSyncer(o);
    });
}

#pragma mark 触摸

/* 落点下原内容里最深的按钮（UIControl；藏起来的 alpha 不看） */
static UIView *ALBPillControlAt(UIView *view, CGPoint point, UIView *space, NSUInteger depth) {
    for (UIView *sub in view.subviews.reverseObjectEnumerator) {
        if (sub.hidden || !sub.userInteractionEnabled || ALBPillOursView(sub)) continue;
        if (![sub pointInside:[space convertPoint:point toView:sub] withEvent:nil]) continue;
        UIView *deeper = depth < 6 ? ALBPillControlAt(sub, point, space, depth + 1) : nil;
        if (deeper) return deeper;
        if ([sub isKindOfClass:UIControl.class] && ((UIControl *)sub).enabled) return sub;
    }
    return nil;
}
static UIView *ALBPillTouchTarget(ALBChatPillButton *pill, UITouch *touch) {
    UIView *owner = pill.owner, *space = pill.superview;
    ALBChatPillState *state = ALBPillStateOf(owner);
    if (!touch || !space) return owner;
    CGPoint point = [touch locationInView:space];
    for (UIView *source in state.hidden.allObjects) {
        if (source == owner || !source.userInteractionEnabled) continue;
        if (![source pointInside:[space convertPoint:point toView:source] withEvent:nil]) continue;
        UIView *control = ALBPillControlAt(source, point, space, 0);
        if (!control && [source isKindOfClass:UIControl.class] && ((UIControl *)source).enabled) control = source;
        if (control) return control;
    }
    return owner;
}
/* 按钮类的目标：它自己没发 TouchUpInside 就补一次（同双击面板 ALBFaceTouchesEnded）；抬手在玻璃里（外扩 16）才算 */
static void ALBPillTapFallback(UIControl *control, UITouch *touch, UIView *pill) {
    ALBFaceTapState *tap = objc_getAssociatedObject(control, &kALBFaceTapKey);
    if (!tap || tap.began <= 0 || !touch) return;
    CFTimeInterval began = tap.began;
    tap.began = 0;
    UIView *area = pill ?: control;
    if (!CGRectContainsPoint(CGRectInset(area.bounds, -16.0, -16.0), [touch locationInView:area])) return;
    __weak UIControl *weakControl = control;
    dispatch_async(dispatch_get_main_queue(), ^{
        UIControl *live = weakControl;
        if (!live.window || !live.enabled || !live.userInteractionEnabled || tap.sent >= began) return;
        gALBPillTapSent++;
        [live sendActionsForControlEvents:UIControlEventTouchUpInside];
    });
}

/* 3.11.47 用户：“评论区的长按面板那些按钮点不了”。那块面板每一行是列表的格子，整行盖了一颗玻璃按钮：触摸先到玻璃按钮，
   再转给格子（touchesBegan / Ended），可列表的“选中”认的是落在格子里的触摸，转过去的不认，于是哪一行都点不动。现在点在
   格子上（底下又不是别的按钮、也没搬过来的手势）时不转了，抬手在玻璃里就自己替列表选中这一格：先问 shouldSelect，
   再 selectItem＋didSelect（列表的代理是抖音的 ListKit 适配器，会转给那一行自己的处理）。 */
static BOOL ALBPillIsListCell(UIView *view) {
    return [view isKindOfClass:UICollectionViewCell.class] || [view isKindOfClass:UITableViewCell.class];
}
static BOOL ALBPillSelectOwnerCell(UIView *owner) {
    UIView *list = owner.superview;
    while (list && ![list isKindOfClass:UICollectionView.class] && ![list isKindOfClass:UITableView.class]) list = list.superview;
    if ([list isKindOfClass:UICollectionView.class] && [owner isKindOfClass:UICollectionViewCell.class]) {
        UICollectionView *cv = (UICollectionView *)list;
        NSIndexPath *ip = [cv indexPathForCell:(UICollectionViewCell *)owner];
        id<UICollectionViewDelegate> d = cv.delegate;
        if (!ip) { gALBPillCellRefused++; gALBPillCellLast = @"格子不在列表里"; return NO; }
        if ([d respondsToSelector:@selector(collectionView:shouldSelectItemAtIndexPath:)] && ![d collectionView:cv shouldSelectItemAtIndexPath:ip]) {
            gALBPillCellRefused++; gALBPillCellLast = [NSString stringWithFormat:@"列表不让选 %ld-%ld", (long)ip.section, (long)ip.item]; return NO;
        }
        [cv selectItemAtIndexPath:ip animated:NO scrollPosition:UICollectionViewScrollPositionNone];
        if ([d respondsToSelector:@selector(collectionView:didSelectItemAtIndexPath:)]) [d collectionView:cv didSelectItemAtIndexPath:ip];
        gALBPillCellSelects++;
        gALBPillCellLast = [NSString stringWithFormat:@"%@ 第 %ld-%ld 格", NSStringFromClass(d.class), (long)ip.section, (long)ip.item];
        return YES;
    }
    if ([list isKindOfClass:UITableView.class] && [owner isKindOfClass:UITableViewCell.class]) {
        UITableView *tv = (UITableView *)list;
        NSIndexPath *ip = [tv indexPathForCell:(UITableViewCell *)owner];
        id<UITableViewDelegate> d = tv.delegate;
        if (!ip) { gALBPillCellRefused++; return NO; }
        if ([d respondsToSelector:@selector(tableView:willSelectRowAtIndexPath:)]) ip = [d tableView:tv willSelectRowAtIndexPath:ip];
        if (!ip) { gALBPillCellRefused++; return NO; }
        [tv selectRowAtIndexPath:ip animated:NO scrollPosition:UITableViewScrollPositionNone];
        if ([d respondsToSelector:@selector(tableView:didSelectRowAtIndexPath:)]) [d tableView:tv didSelectRowAtIndexPath:ip];
        gALBPillCellSelects++;
        gALBPillCellLast = [NSString stringWithFormat:@"%@ 第 %ld-%ld 行", NSStringFromClass(d.class), (long)ip.section, (long)ip.row];
        return YES;
    }
    gALBPillCellRefused++;
    gALBPillCellLast = @"没找到列表";
    return NO;
}

@implementation ALBChatPillButton
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesBegan:touches withEvent:event];
    UIView *target = ALBPillTouchTarget(self, touches.anyObject);
    self.touchTarget = target;
    gALBPillTouches++;
    ALBChatPillState *cellState = ALBPillStateOf(self.owner);
    self.cellTap = target == self.owner && ALBPillIsListCell(self.owner) && !cellState.tapControl && cellState.moved.count == 0;
    if (self.cellTap) { self.touchTarget = nil; self.touchControl = nil; return; }
    // 目标是自己也画进玻璃里的图片视图 / 文字（图文页顶栏）：触摸顺着响应链往上，外面要是有按钮（包着图标的 UIButton），
    // 点按算它的。别的（格子、按住说话）不往外找，免得给外面不相干的按钮补发
    UIControl *control = [target isKindOfClass:UIControl.class] ? (UIControl *)target : nil;
    if (!control && target == self.owner) control = ALBPillStateOf(self.owner).tapControl;
    BOOL ownerDrawn = target == self.owner && [ALBPillStateOf(self.owner).hidden containsObject:self.owner];
    NSUInteger depth = 0;
    for (UIView *v = self.superview; !control && ownerDrawn && v && depth < 4; v = v.superview, depth++)
        if ([v isKindOfClass:UIControl.class]) control = (UIControl *)v;
    self.touchControl = control;
    if (control) {
        if (control != self.owner) gALBPillControlTargets++;
        ALBFaceTapOnTouchUp((UIButton *)control);
        ALBFaceTapState *tap = objc_getAssociatedObject(control, &kALBFaceTapKey);
        tap.began = CACurrentMediaTime();
    }
    [target touchesBegan:touches withEvent:event];
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesMoved:touches withEvent:event];
    if (self.cellTap) return;
    [self.touchTarget touchesMoved:touches withEvent:event];
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesEnded:touches withEvent:event];
    if (self.cellTap) {
        self.cellTap = NO;
        UITouch *touch = touches.anyObject;
        UIView *owner = self.owner;
        if (touch && owner && CGRectContainsPoint(CGRectInset(self.bounds, -16.0, -16.0), [touch locationInView:self])) {
            dispatch_async(dispatch_get_main_queue(), ^{ if (owner.window) ALBPillSelectOwnerCell(owner); });
        }
        ALBPillResyncSoon(self.owner, YES);
        return;
    }
    UIView *target = self.touchTarget;
    UIControl *control = self.touchControl;
    self.touchTarget = nil;
    self.touchControl = nil;
    [target touchesEnded:touches withEvent:event];
    if (control) ALBPillTapFallback(control, touches.anyObject, self);
    ALBPillResyncSoon(self.owner, YES);
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [super touchesCancelled:touches withEvent:event];
    self.cellTap = NO;
    UIView *target = self.touchTarget;
    UIControl *control = self.touchControl;
    self.touchTarget = nil;
    self.touchControl = nil;
    ALBFaceTapState *tap = control ? objc_getAssociatedObject(control, &kALBFaceTapKey) : nil;
    tap.began = 0;
    [target touchesCancelled:touches withEvent:event];
}
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    UIView *view = gesture.view;
    if (view && view != self && ![view isDescendantOfView:self]) return YES;
    if ([ALBPillStateOf(self.owner).moved objectForKey:gesture]) return YES;     // 搬过来的（原内容上的点按 / 长按）
    return [super gestureRecognizerShouldBegin:gesture];
}
@end

/* 原内容上的点按 / 长按手势搬到玻璃按钮上（原内容 alpha 0，点不到了） */
static void ALBPillMoveGestures(ALBChatPillButton *pill, ALBChatPillState *state, NSArray<UIView *> *sources) {
    if (!state.moved) state.moved = [NSMapTable weakToWeakObjectsMapTable];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:sources];
    for (NSUInteger i = 0; i < queue.count && i < 80; i++) {
        UIView *view = queue[i];
        if ([view isKindOfClass:UIScrollView.class] || ALBPillOursView(view)) continue;
        for (UIGestureRecognizer *gesture in view.gestureRecognizers.copy) {
            if (![gesture isKindOfClass:UITapGestureRecognizer.class] && ![gesture isKindOfClass:UILongPressGestureRecognizer.class]) continue;
            // 只搬抖音自己的（系统的点按 / 长按两个公开类，或抖音的子类）；系统内部的（_ 开头、UI 开头的文字选择那些）不动
            NSString *name = NSStringFromClass(gesture.class);
            if ([name hasPrefix:@"_"]) continue;
            if ([name hasPrefix:@"UI"] && ![name isEqualToString:@"UITapGestureRecognizer"] && ![name isEqualToString:@"UILongPressGestureRecognizer"]) continue;
            [state.moved setObject:view forKey:gesture];
            [pill addGestureRecognizer:gesture];
            gALBPillGesturesMoved++;
        }
        [queue addObjectsFromArray:view.subviews];
    }
}
static void ALBPillRestoreGestures(ALBChatPillState *state) {
    for (UIGestureRecognizer *gesture in state.moved.keyEnumerator.allObjects) {
        UIView *view = [state.moved objectForKey:gesture];
        if (view) [view addGestureRecognizer:gesture];
    }
    [state.moved removeAllObjects];
}

/* 列表从玻璃按钮上开始滑：UIScrollView 默认不替 UIControl 取消触摸（滑不动），这里的玻璃按钮放行 */
static BOOL ALBPillScrollShouldPass(UIView *view) {
    NSUInteger depth = 0;
    for (UIView *v = view; v && depth < 8; v = v.superview, depth++)
        if ([v isKindOfClass:ALBChatPillButton.class]) return YES;
    return NO;
}
static void ALBPillHookScrollCancel(Class cls) {
    SEL sel = @selector(touchesShouldCancelInContentView:);
    Method method = cls ? class_getInstanceMethod(cls, sel) : NULL;
    if (!method) return;
    IMP original = method_getImplementation(method);
    IMP replacement = imp_implementationWithBlock(^BOOL(UIScrollView *me, UIView *view) {
        if (ALBPillScrollShouldPass(view)) { gALBPillScrollPass++; return YES; }
        return ((BOOL (*)(id, SEL, UIView *))original)(me, sel, view);
    });
    if (!class_addMethod(cls, sel, replacement, method_getTypeEncoding(method))) method_setImplementation(method, replacement);
}
static void ALBPillInstallScrollHook(void) {
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        ALBPillHookScrollCancel(UIScrollView.class);
        Method base = class_getInstanceMethod(UIScrollView.class, @selector(touchesShouldCancelInContentView:));
        for (Class cls in @[UICollectionView.class, UITableView.class]) {
            Method own = class_getInstanceMethod(cls, @selector(touchesShouldCancelInContentView:));
            if (own && own != base) ALBPillHookScrollCancel(cls);     // 自己另写了一份的才再钩
        }
    });
}

#pragma mark 画

static CGFloat ALBPillLuma(UIColor *color, UITraitCollection *traits, CGFloat *alpha, CGFloat *saturation) {
    if (!color) return -1;
    UIColor *resolved = traits ? [color resolvedColorWithTraitCollection:traits] : color;
    CGFloat r = 0, g = 0, b = 0, a = 0;
    if (![resolved getRed:&r green:&g blue:&b alpha:&a]) {
        CGFloat w = 0;
        if (![resolved getWhite:&w alpha:&a]) return -1;
        r = g = b = w;
    }
    if (alpha) *alpha = a;
    CGFloat hi = MAX(r, MAX(g, b)), lo = MIN(r, MIN(g, b));
    if (saturation) *saturation = hi > 0.001 ? (hi - lo) / hi : 0;
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

/* 原来的底色是彩色（“发送”“关注”的红）：当作底画进去（灰的、白的半透明底不画） */
static UIColor *ALBPillSaturatedPlate(UIColor *color, UITraitCollection *traits) {
    CGFloat alpha = 0, saturation = 0;
    if (!color || ALBPillLuma(color, traits, &alpha, &saturation) < 0 || alpha < 0.3 || saturation < 0.35) return nil;
    UIColor *resolved = traits ? [color resolvedColorWithTraitCollection:traits] : color;
    return [resolved colorWithAlphaComponent:MIN(alpha, 0.9)];
}

/* 去掉图里贴着边的中性底（白 / 灰的圆底、方底）：外圈最多的那种不透明中性色＝底，从四边往里把连着的这种颜色（和透明）
   都抠掉，中间的图标留下。不是这种图（彩色圆、照片）原样。结果记在图上 */
static char kALBPlatelessKey;
static UIImage *ALBPlatelessCompute(UIImage *image) {
    CGImageRef cg = image.CGImage;
    size_t w = cg ? CGImageGetWidth(cg) : 0, h = cg ? CGImageGetHeight(cg) : 0;
    if (w < 8 || h < 8 || w > 1024 || h > 1024) return nil;
    size_t stride = w * 4;
    NSMutableData *data = [NSMutableData dataWithLength:stride * h];
    uint8_t *px = data.mutableBytes;
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef ctx = CGBitmapContextCreate(px, w, h, 8, stride, space, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if (!ctx) return nil;
    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cg);
    size_t ix = w / 5, iy = h / 5;
    NSUInteger histogram[64] = {0}, band = 0;
    for (size_t y = 0; y < h; y++)
        for (size_t x = 0; x < w; x++) {
            if (x >= ix && x < w - ix && y >= iy && y < h - iy) continue;
            band++;
            const uint8_t *p = px + y * stride + x * 4;
            if (p[3] < 230) continue;
            uint8_t hi = MAX(p[0], MAX(p[1], p[2])), lo = MIN(p[0], MIN(p[1], p[2]));
            if (hi - lo > 16) continue;
            histogram[p[0] >> 2]++;
        }
    NSUInteger best = 0;
    for (NSUInteger i = 1; i < 64; i++) if (histogram[i] > histogram[best]) best = i;
    if (!band || histogram[best] * 4 < band) { CGContextRelease(ctx); return nil; }
    int low = (int)(best * 4) - 8, high = (int)(best * 4) + 11;
    NSMutableData *visitedData = [NSMutableData dataWithLength:w * h];
    NSMutableData *queueData = [NSMutableData dataWithLength:w * h * sizeof(uint32_t)];
    uint8_t *visited = visitedData.mutableBytes;
    uint32_t *queue = queueData.mutableBytes;
    size_t head = 0, tail = 0, removed = 0;
    BOOL (^plate)(size_t) = ^BOOL(size_t index) {
        const uint8_t *p = px + (index / w) * stride + (index % w) * 4;
        if (p[3] < 10) return YES;
        int r = p[0] * 255 / p[3], g = p[1] * 255 / p[3], b = p[2] * 255 / p[3];
        int hi = MAX(r, MAX(g, b)), lo = MIN(r, MIN(g, b));
        return hi - lo <= 18 && r >= low && r <= high;
    };
    for (size_t y = 0; y < h; y++)
        for (size_t x = 0; x < w; x++) {
            if (x && y && x + 1 < w && y + 1 < h) continue;
            size_t index = y * w + x;
            if (plate(index)) { visited[index] = 1; queue[tail++] = (uint32_t)index; }
        }
    while (head < tail) {
        size_t index = queue[head++], x = index % w, y = index / w;
        uint8_t *p = px + y * stride + x * 4;
        if (p[3]) { removed++; memset(p, 0, 4); }
        size_t next[4] = { x ? index - 1 : index, x + 1 < w ? index + 1 : index, y ? index - w : index, y + 1 < h ? index + w : index };
        for (int n = 0; n < 4; n++) {
            if (visited[next[n]]) continue;
            visited[next[n]] = 1;
            if (plate(next[n])) queue[tail++] = (uint32_t)next[n];
        }
    }
    UIImage *result = nil;
    if (removed * 10 >= w * h) {
        CGImageRef out = CGBitmapContextCreateImage(ctx);
        if (out) {
            result = [[UIImage imageWithCGImage:out scale:image.scale orientation:image.imageOrientation]
                      imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
            CGImageRelease(out);
        }
    }
    CGContextRelease(ctx);
    return result;
}
static NSUInteger gALBPlatelessDone, gALBPlatelessSkipped;
static UIImage *ALBPlateless(UIImage *image) {
    if (!image || image.isSymbolImage || image.renderingMode == UIImageRenderingModeAlwaysTemplate) return image;
    id cached = objc_getAssociatedObject(image, &kALBPlatelessKey);
    if (cached) return cached == (id)NSNull.null ? image : cached;
    UIImage *result = ALBPlatelessCompute(image);
    if (result) gALBPlatelessDone++; else gALBPlatelessSkipped++;
    objc_setAssociatedObject(image, &kALBPlatelessKey, result ?: (id)NSNull.null, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return result ?: image;
}

/* 图里不透明部分的中心偏了多少（像素） */
static BOOL ALBInkOffset(CGImageRef cg, CGFloat *dx, CGFloat *dy) {
    size_t w = cg ? CGImageGetWidth(cg) : 0, h = cg ? CGImageGetHeight(cg) : 0;
    if (!w || !h || w > 1024 || h > 1024) return NO;
    NSMutableData *data = [NSMutableData dataWithLength:w * h];
    CGContextRef ctx = CGBitmapContextCreate(data.mutableBytes, w, h, 8, w, NULL, (CGBitmapInfo)kCGImageAlphaOnly);
    if (!ctx) return NO;
    CGContextDrawImage(ctx, CGRectMake(0, 0, w, h), cg);
    CGContextRelease(ctx);
    const uint8_t *px = data.bytes;
    size_t minX = w, minY = h, maxX = 0, maxY = 0;
    for (size_t y = 0; y < h; y++)
        for (size_t x = 0; x < w; x++)
            if (px[y * w + x] > 24) {
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
    if (minX > maxX || minY > maxY) return NO;
    *dx = w / 2.0 - (minX + maxX + 1) / 2.0;
    *dy = h / 2.0 - (minY + maxY + 1) / 2.0;
    return YES;
}
static UIImage *ALBPillCenterInk(UIImage *image) {
    CGFloat dx = 0, dy = 0;
    if (!image || !ALBInkOffset(image.CGImage, &dx, &dy) || (fabs(dx) < 1.5 && fabs(dy) < 1.5)) return image;
    CGFloat scale = image.scale > 0 ? image.scale : 1;
    UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat preferredFormat];
    format.scale = scale;
    format.opaque = NO;
    return [[[UIGraphicsImageRenderer alloc] initWithSize:image.size format:format]
        imageWithActions:^(__unused UIGraphicsImageRendererContext *context) {
            [image drawAtPoint:CGPointMake(dx / scale, dy / scale)];
        }];
}

/* 图片视图按它的 contentMode 画（去底的时候用：不画它的底色，图去掉圆底） */
static void ALBPillDrawImageView(UIImageView *view, CGRect rect) {
    UIImage *image = ALBPlateless(view.image);
    if (!image) return;
    if (image.renderingMode == UIImageRenderingModeAlwaysTemplate || image.isSymbolImage) {
        UIColor *tint = [(view.tintColor ?: UIColor.labelColor) resolvedColorWithTraitCollection:view.traitCollection];
        image = [image imageWithTintColor:tint renderingMode:UIImageRenderingModeAlwaysOriginal];
    }
    CGSize s = image.size;
    if (s.width < 1 || s.height < 1 || rect.size.width < 1 || rect.size.height < 1) return;
    CGRect target = rect;
    UIViewContentMode mode = view.contentMode;
    if (mode == UIViewContentModeScaleAspectFit || mode == UIViewContentModeScaleAspectFill) {
        CGFloat k = mode == UIViewContentModeScaleAspectFit ? MIN(rect.size.width / s.width, rect.size.height / s.height)
                                                            : MAX(rect.size.width / s.width, rect.size.height / s.height);
        CGSize d = CGSizeMake(s.width * k, s.height * k);
        target = CGRectMake(CGRectGetMidX(rect) - d.width / 2, CGRectGetMidY(rect) - d.height / 2, d.width, d.height);
    } else if (mode == UIViewContentModeCenter) {
        target = CGRectMake(CGRectGetMidX(rect) - s.width / 2, CGRectGetMidY(rect) - s.height / 2, s.width, s.height);
    }
    [image drawInRect:target];
}

/* 原来内容原样画成一张图（frame 那么大，裁成胶囊；画的时候临时把藏起来的放回原来的 opacity，同一拍里放回去，屏幕上看不到） */
static UIImage *ALBPillRender(ALBPillSpec *spec, UIView *host, UIColor *plate, ALBChatPillState *state) {
    CGRect frame = spec.frame;
    CGSize size = frame.size;
    if (size.width < 1 || size.height < 1 || !host) return nil;
    UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat preferredFormat];
    format.opaque = NO;
    __block UIImage *image = nil;
    [UIView performWithoutAnimation:^{
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        image = [[[UIGraphicsImageRenderer alloc] initWithSize:size format:format]
            imageWithActions:^(UIGraphicsImageRendererContext *context) {
                CGContextRef c = context.CGContext;
                CGFloat corner = spec.radius > 0 ? MIN(spec.radius, MIN(size.width, size.height) / 2) : MIN(size.width, size.height) / 2;
                UIBezierPath *shape = [UIBezierPath bezierPathWithRoundedRect:CGRectMake(0, 0, size.width, size.height)
                                                                 cornerRadius:corner];
                [shape addClip];
                if (plate) { [plate setFill]; [shape fill]; }
                for (UIView *view in spec.sources) {
                    if (!view.superview) continue;
                    float opacity = view.layer.opacity;
                    NSNumber *original = [state.opacity objectForKey:view];
                    view.layer.opacity = original ? original.floatValue : (opacity > 0 ? opacity : 1);
                    CGContextSaveGState(c);
                    CGRect r = [view.superview convertRect:view.frame toView:host];
                    r.origin.x -= frame.origin.x;
                    r.origin.y -= frame.origin.y;
                    if (spec.destain && [view isKindOfClass:UIImageView.class]) {
                        CGContextSetAlpha(c, view.layer.opacity);
                        ALBPillDrawImageView((UIImageView *)view, r);
                        for (UIView *sub in view.subviews) {
                            if (sub.hidden || ALBPillOursView(sub)) continue;
                            CGContextSaveGState(c);
                            CGContextTranslateCTM(c, r.origin.x + sub.frame.origin.x, r.origin.y + sub.frame.origin.y);
                            [sub.layer renderInContext:c];
                            CGContextRestoreGState(c);
                        }
                    } else if (CGAffineTransformIsIdentity(view.transform)) {
                        CGContextTranslateCTM(c, r.origin.x, r.origin.y);
                        [view.layer renderInContext:c];
                    } else {
                        CGPoint center = [view.superview convertPoint:view.center toView:host];
                        CGContextTranslateCTM(c, center.x - frame.origin.x, center.y - frame.origin.y);
                        CGContextConcatCTM(c, view.transform);
                        CGContextTranslateCTM(c, -view.bounds.size.width / 2, -view.bounds.size.height / 2);
                        [view.layer renderInContext:c];
                    }
                    CGContextRestoreGState(c);
                    view.layer.opacity = opacity;
                }
            }];
        [CATransaction commit];
    }];
    gALBChatPillRendered++;
    if (spec.center) image = ALBPillCenterInk(image);
    return [image imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
}

/* 内容签名：字、图（换了就重画） */
static void ALBChatPillSignaturePart(UIView *view, NSMutableString *sig, NSUInteger depth) {
    if (depth > 4 || sig.length > 900) return;
    if ([view isKindOfClass:UILabel.class]) [sig appendFormat:@"L%@#%@|", ((UILabel *)view).text ?: @"", ((UILabel *)view).textColor ?: @""];
    else if ([view isKindOfClass:UIImageView.class]) [sig appendFormat:@"I%p|", (__bridge void *)((UIImageView *)view).image];
    else {
        id text = nil;
        if ([NSStringFromClass(view.class) containsString:@"Label"] && [view respondsToSelector:@selector(attributedText)]) {
            @try { text = [view valueForKey:@"attributedText"]; } @catch (__unused NSException *e) { text = nil; }
        }
        if ([text isKindOfClass:NSAttributedString.class]) [sig appendFormat:@"Y%@|", ((NSAttributedString *)text).string];
    }
    [sig appendFormat:@"%.0fx%.0f@%.0f,%.0f/%.2f%@;", view.bounds.size.width, view.bounds.size.height, view.center.x, view.center.y,
        view.backgroundColor ? CGColorGetAlpha(view.backgroundColor.CGColor) : 0, view.hidden ? @"h" : @""];
    for (UIView *sub in view.subviews) ALBChatPillSignaturePart(sub, sig, depth + 1);
}

#pragma mark 装上 / 拿掉

/* 3.11.43：用户：“把玻璃换成 regular”。系统自己的玻璃按钮配置（Regular；有颜色的用 prominent，按那个颜色染），不再用
   右侧那份会跟着背后亮度自己翻深浅的材质（分享面板一按、一滑就白变黑）。深浅由 overrideUserInterfaceStyle 定死 */
static UIButtonConfiguration *ALBPillConfiguration(UIColor *tint, CGFloat radius) {
    UIButtonConfiguration *configuration = nil;
    if (@available(iOS 26.0, *)) {
        configuration = tint ? [UIButtonConfiguration prominentGlassButtonConfiguration] : [UIButtonConfiguration glassButtonConfiguration];
    }
    if (!configuration) configuration = ALB25MaterialMotionButtonConfigurationTone(NO);
    if (!configuration) return nil;
    if (tint) configuration.baseBackgroundColor = tint;
    if (radius > 0) {
        configuration.cornerStyle = UIButtonConfigurationCornerStyleFixed;
        UIBackgroundConfiguration *background = configuration.background;
        background.cornerRadius = radius;
        configuration.background = background;
    } else {
        configuration.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
    }
    configuration.contentInsets = NSDirectionalEdgeInsetsZero;
    configuration.imagePadding = 0;
    return configuration;
}
/* 系统外观（深浅都按它） */
static UIUserInterfaceStyle ALBSceneStyle(UIView *view) {
    UIUserInterfaceStyle style = view.window.windowScene.traitCollection.userInterfaceStyle;
    if (style == UIUserInterfaceStyleUnspecified) style = view.traitCollection.userInterfaceStyle;
    return style == UIUserInterfaceStyleDark ? UIUserInterfaceStyleDark : UIUserInterfaceStyleLight;
}

static void ALBChatPillRemove(UIView *owner) {
    ALBChatPillButton *pill = ALBPillOf(owner);
    ALBChatPillState *state = ALBPillStateOf(owner);
    [pill removeFromSuperview];
    for (UIView *view in state.hidden.allObjects) {
        NSNumber *original = [state.opacity objectForKey:view];
        if (view.layer.opacity == 0) view.layer.opacity = original ? original.floatValue : 1;
    }
    if (state.saved) {
        owner.backgroundColor = state.background;
        owner.layer.backgroundColor = state.layerBackground;
    }
    if (state.contentSaved && [owner isKindOfClass:UICollectionViewCell.class])
        ((UICollectionViewCell *)owner).contentView.backgroundColor = state.contentBackground;
    if (state) ALBPillRestoreGestures(state);
    for (dispatch_block_t undo in state.undo.allValues) undo();
    objc_setAssociatedObject(owner, &kALBChatPillKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(owner, &kALBChatPillStateKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    objc_setAssociatedObject(owner, &kALBPillSyncerKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    if (pill && gALBPillLive) gALBPillLive--;
    gALBChatPillRemoved++;
}

/* 每次同步：玻璃按钮在 host 最上面、spec.frame 那么大；内容变了重画；原内容藏、原底色去。spec 为 nil＝拿掉 */
static void ALBPillApply(UIView *owner, ALBPillSpec *spec, ALBPillSyncFn syncer) {
    if (!owner || !NSThread.isMainThread) return;
    ALBChatPillButton *pill = ALBPillOf(owner);
    ALBChatPillState *state = ALBPillStateOf(owner);
    UIView *host = spec.host ?: owner;
    if (!spec || !host.window || spec.frame.size.width < 8 || spec.frame.size.height < 8) {
        if (pill || state) ALBChatPillRemove(owner);
        return;
    }
    if (!state) {
        state = [ALBChatPillState new];
        state.hidden = [NSHashTable weakObjectsHashTable];
        state.opacity = [NSMapTable weakToStrongObjectsMapTable];
        objc_setAssociatedObject(owner, &kALBChatPillStateKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    state.area = spec.area;
    state.tapControl = spec.tapControl;
    if (!pill) {
        UIButtonConfiguration *configuration = ALBPillConfiguration(nil, 0);
        if (!configuration) return;
        pill = [ALBChatPillButton buttonWithType:UIButtonTypeCustom];
        pill.owner = owner;
        pill.accessibilityIdentifier = @"AwemeLiquidBar.ChatPill";
        pill.configuration = configuration;
        pill.adjustsImageWhenHighlighted = NO;
        pill.backgroundColor = UIColor.clearColor;
        objc_setAssociatedObject(owner, &kALBChatPillKey, pill, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!gALBChatPillOwners) gALBChatPillOwners = [NSHashTable weakObjectsHashTable];
        [gALBChatPillOwners addObject:owner];
        gALBChatPillInstalled++;
        gALBPillLive++;
        if (!gALBPillAreas) gALBPillAreas = [NSMutableDictionary dictionary];
        NSString *area = spec.area ?: @"其它";
        gALBPillAreas[area] = @(gALBPillAreas[area].unsignedIntegerValue + 1);
        if (!gALBPillClasses) gALBPillClasses = [NSMutableOrderedSet orderedSet];
        if (gALBPillClasses.count < 24)
            [gALBPillClasses addObject:[NSString stringWithFormat:@"胶囊 %@ %@ %.0f×%.0f", area, NSStringFromClass(owner.class),
                spec.frame.size.width, spec.frame.size.height]];
        if ([owner isKindOfClass:UIControl.class]) ALBFaceTapOnTouchUp((UIButton *)owner);
        ALBPillInstallScrollHook();
    }
    if (spec.tapControl) ALBFaceTapOnTouchUp((UIButton *)spec.tapControl);
    if (syncer) objc_setAssociatedObject(owner, &kALBPillSyncerKey, [NSValue valueWithPointer:(const void *)syncer], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    NSArray<UIView *> *sources = spec.sources ?: @[];
    BOOL ownerIsSource = [sources indexOfObjectIdenticalTo:owner] != NSNotFound;
    // 原来的底色（格子的 15% 白、选中的那块、按住说话的 15% 白、发送键的红）：记着，去掉
    if (!ownerIsSource) {
        BOOL showsColor = (owner.backgroundColor && CGColorGetAlpha(owner.backgroundColor.CGColor) > 0.01) ||
                          (owner.layer.backgroundColor && CGColorGetAlpha(owner.layer.backgroundColor) > 0.01);
        if (showsColor) {
            if (!state.saved) {
                state.saved = YES;
                state.background = owner.backgroundColor;
                state.layerBackground = owner.layer.backgroundColor ? CGColorRetain(owner.layer.backgroundColor) : NULL;
            }
            owner.backgroundColor = UIColor.clearColor;
        }
        UIView *content = [owner isKindOfClass:UICollectionViewCell.class] ? ((UICollectionViewCell *)owner).contentView : nil;
        if (content.backgroundColor && CGColorGetAlpha(content.backgroundColor.CGColor) > 0.01) {
            if (!state.contentSaved) { state.contentSaved = YES; state.contentBackground = content.backgroundColor; }
            content.backgroundColor = UIColor.clearColor;
        }
    }
    UIColor *plate = spec.plate;
    UIColor *tint = spec.tint;
    if (!tint && spec.autoTint && state.saved)
        tint = ALBPillSaturatedPlate(state.background ?: (state.layerBackground ? [UIColor colorWithCGColor:state.layerBackground] : nil), owner.traitCollection);
    if (tint) tint = [tint colorWithAlphaComponent:1];
    // 玻璃的做法变了（染不染色、圆角）：配置重做，图也重画
    BOOL tintChanged = (tint || state.configTint) && ![tint isEqual:state.configTint];
    if (!state.configured || tintChanged || fabs(state.configRadius - spec.radius) > 0.01) {
        UIButtonConfiguration *configuration = ALBPillConfiguration(tint, spec.radius);
        if (configuration) {
            pill.configuration = configuration;
            if (tint) pill.tintColor = tint;
            state.configTint = tint;
            state.configRadius = spec.radius;
            state.configured = YES;
            state.signature = nil;
        }
    }
    // 内容：变了就重画成玻璃按钮的图
    // 3.11.44：系统深浅也算（抖音的字色是随深浅变的动态色，描述里看不出来）
    NSMutableString *sig = [NSMutableString stringWithFormat:@"%.1fx%.1f|%@|%d%d|r%.1f|t%ld|", spec.frame.size.width, spec.frame.size.height,
                            plate ?: @"-", spec.destain, spec.center, spec.radius, (long)host.traitCollection.userInterfaceStyle];
    for (UIView *view in sources) {
        CGRect r = [view.superview convertRect:view.frame toView:host];
        [sig appendFormat:@"@%.0f,%.0f:", r.origin.x - spec.frame.origin.x, r.origin.y - spec.frame.origin.y];
        ALBChatPillSignaturePart(view, sig, 0);
    }
    for (UIView *view in sources)
        if (![state.opacity objectForKey:view]) [state.opacity setObject:@(view.layer.opacity > 0 ? view.layer.opacity : 1) forKey:view];
    if (![sig isEqualToString:state.signature] || !pill.configuration.image) {
        UIImage *image = ALBPillRender(spec, host, plate, state);
        if (image) {
            UIButtonConfiguration *configuration = pill.configuration ?: ALBPillConfiguration(tint, spec.radius);
            configuration.title = nil;
            configuration.subtitle = nil;
            configuration.image = image;
            configuration.imagePadding = 0;
            configuration.contentInsets = NSDirectionalEdgeInsetsZero;
            configuration.baseForegroundColor = nil;
            pill.configuration = configuration;
            state.signature = sig;
        }
    }
    // 不再画的放回来，要画的藏起来
    for (UIView *view in state.hidden.allObjects) {
        if ([sources indexOfObjectIdenticalTo:view] != NSNotFound) continue;
        NSNumber *original = [state.opacity objectForKey:view];
        if (view.layer.opacity == 0) view.layer.opacity = original ? original.floatValue : 1;
        [state.hidden removeObject:view];
    }
    for (UIView *view in sources) {
        if (view.layer.opacity != 0) view.layer.opacity = 0;
        [state.hidden addObject:view];
    }
    if (pill.superview != host) [host addSubview:pill];
    else if (host.subviews.lastObject != pill) [host bringSubviewToFront:pill];
    if (!CGRectEqualToRect(pill.frame, spec.frame)) pill.frame = spec.frame;
    UIUserInterfaceStyle style = spec.style;
    if (style == UIUserInterfaceStyleUnspecified) style = ALBSceneStyle(host);     // 3.11.44：没指定就跟系统
    if (pill.overrideUserInterfaceStyle != style) pill.overrideUserInterfaceStyle = style;
    ALBPillMoveGestures(pill, state, sources);
}

#pragma mark 原内容

/* 原来的内容：格子是 contentView 里的，别的是自己的子视图（不含玻璃按钮和插件自己的视图） */
static NSArray<UIView *> *ALBChatPillSources(UIView *owner, UIView *pill) {
    ALBChatPillState *state = ALBPillStateOf(owner);
    UIView *root = [owner isKindOfClass:UICollectionViewCell.class] ? ((UICollectionViewCell *)owner).contentView
                 : ([owner isKindOfClass:UITableViewCell.class] ? ((UITableViewCell *)owner).contentView : owner);
    NSMutableArray<UIView *> *out = [NSMutableArray array];
    for (UIView *sub in root.subviews) {
        if (sub == pill || ALBPillOursView(sub) || !ALBPillVisible(sub, state)) continue;
        if (sub.bounds.size.width < 1 || sub.bounds.size.height < 1) continue;
        [out addObject:sub];
    }
    return out;
}

/* 落在 rect（space 坐标）里的原内容：整个在里面的就是它，跨出去的往下一层找 */
static void ALBPillCollectInRect(UIView *root, UIView *space, CGRect rect, ALBChatPillState *state, NSMutableArray<UIView *> *out, NSUInteger depth) {
    for (UIView *sub in root.subviews) {
        if (ALBPillOursView(sub) || !ALBPillVisible(sub, state)) continue;
        if (sub.bounds.size.width < 1 || sub.bounds.size.height < 1) continue;
        CGRect r = [sub.superview convertRect:sub.frame toView:space];
        if (!CGRectIntersectsRect(r, rect)) continue;
        if (CGRectContainsRect(CGRectInset(rect, -2, -2), r)) [out addObject:sub];
        else if (depth < 4 && sub.subviews.count) ALBPillCollectInRect(sub, space, rect, state, out, depth + 1);
    }
}
static NSArray<UIView *> *ALBPillSourcesInRect(UIView *owner, UIView *root, UIView *space, CGRect rect) {
    NSMutableArray<UIView *> *out = [NSMutableArray array];
    ALBPillCollectInRect(root, space, rect, ALBPillStateOf(owner), out, 0);
    return out;
}

/* UIImageView setImage 钩子（每张图都过这里）：有胶囊在屏幕上时才往上找（自己或往上 6 层里装了胶囊的那个），这一轮结束重画 */
static void ALBPillImageChanged(UIImageView *imageView) {
    if (!gALBPillLive || !imageView.window) return;
    NSUInteger depth = 0;
    for (UIView *v = imageView; v && depth < 7; v = v.superview, depth++) {
        if ([v isKindOfClass:ALBChatPillButton.class]) return;          // 玻璃按钮自己的图
        if (objc_getAssociatedObject(v, &kALBChatPillStateKey)) { ALBPillResyncLater(v); return; }
    }
}

#pragma mark - 3.11.43 去底（记着原来的，关掉时还原）

@interface ALBBacking : NSObject
@property (nonatomic, strong) UIColor *color;
@property (nonatomic, strong) UIVisualEffect *effect;
@property (nonatomic, assign) BOOL opaque;
@end
@implementation ALBBacking
@end
static char kALBBackingKey;
static NSHashTable<UIView *> *gALBBackingViews;
static NSUInteger gALBBackingCleared;

/* 底色、毛玻璃效果、不透明标记去掉；抖音再设的颜色当作以后还原的值 */
static void ALBBackingClear(UIView *view) {
    if (!view) return;
    ALBBacking *backing = objc_getAssociatedObject(view, &kALBBackingKey);
    if (!backing) {
        backing = [ALBBacking new];
        backing.color = view.backgroundColor;
        backing.opaque = view.opaque;
        objc_setAssociatedObject(view, &kALBBackingKey, backing, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        if (!gALBBackingViews) gALBBackingViews = [NSHashTable weakObjectsHashTable];
        [gALBBackingViews addObject:view];
        gALBBackingCleared++;
    }
    UIColor *color = view.backgroundColor;
    if (color && CGColorGetAlpha(color.CGColor) > 0.001) {
        backing.color = color;
        view.backgroundColor = UIColor.clearColor;
    }
    if (view.opaque) { backing.opaque = YES; view.opaque = NO; }
    if ([view isKindOfClass:UIVisualEffectView.class] && ((UIVisualEffectView *)view).effect) {
        backing.effect = ((UIVisualEffectView *)view).effect;
        ((UIVisualEffectView *)view).effect = nil;
    }
}
static void ALBBackingRestore(UIView *view) {
    ALBBacking *backing = view ? objc_getAssociatedObject(view, &kALBBackingKey) : nil;
    if (!backing) return;
    UIColor *now = view.backgroundColor;
    if (backing.color && (!now || CGColorGetAlpha(now.CGColor) <= 0.001)) view.backgroundColor = backing.color;
    if (backing.opaque) view.opaque = YES;
    if (backing.effect && [view isKindOfClass:UIVisualEffectView.class] && !((UIVisualEffectView *)view).effect)
        ((UIVisualEffectView *)view).effect = backing.effect;
    objc_setAssociatedObject(view, &kALBBackingKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    [gALBBackingViews removeObject:view];
}
static BOOL ALBBackingTaken(UIView *view) { return view && objc_getAssociatedObject(view, &kALBBackingKey) != nil; }

/* 诊断（ALB5Settings.h 的【更多玻璃】，ALBMoreGlass.h 的 ALBBindGlassReport 接着往下写各处的） */
static NSString *ALBChatReportPart(void) {
    NSUInteger pills = 0;
    for (UIView *owner in gALBChatPillOwners.allObjects) if (owner.window && objc_getAssociatedObject(owner, &kALBChatPillKey)) pills++;
    NSMutableArray<NSString *> *areas = [NSMutableArray array];
    for (NSString *area in gALBPillAreas) [areas addObject:[NSString stringWithFormat:@"%@ %@", area, gALBPillAreas[area]]];
    return [NSString stringWithFormat:@"聊天页=已撤回（3.11.44 起不碰） 分享面板与评论长按面板=%@ 图文页评论框=%@；胶囊装上 %lu 次（%@）、拿掉 %lu 次、重画 %lu 次、现在屏幕上 %lu 个；胶囊按下 %lu 次（其中给原内容里的按钮 %lu 次）、补发动作 %lu、搬过来的手势 %lu 个、列表从胶囊上开始滑放行 %lu 次；去白底 %lu 张（不是那种图 %lu 张）；去掉的底 %lu 块；装过：%@；3.11.47 点在列表格子上替列表选中 %lu 次、选不了 %lu 次（最近 %@）",
        ALB5Bool(@"bind.sheets", YES) ? @"开" : @"关", ALB5Bool(@"bind.note", YES) ? @"开" : @"关",
        (unsigned long)gALBChatPillInstalled, areas.count ? [areas componentsJoinedByString:@"、"] : @"-",
        (unsigned long)gALBChatPillRemoved, (unsigned long)gALBChatPillRendered, (unsigned long)pills,
        (unsigned long)gALBPillTouches, (unsigned long)gALBPillControlTargets, (unsigned long)gALBPillTapSent,
        (unsigned long)gALBPillGesturesMoved, (unsigned long)gALBPillScrollPass,
        (unsigned long)gALBPlatelessDone, (unsigned long)gALBPlatelessSkipped, (unsigned long)gALBBackingCleared,
        gALBPillClasses.count ? [gALBPillClasses.array componentsJoinedByString:@"、"] : @"还没装上过（打开分享面板再导出）",
        (unsigned long)gALBPillCellSelects, (unsigned long)gALBPillCellRefused, gALBPillCellLast ?: @"-"];
}
