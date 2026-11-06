/* 3.11.45 专项诊断（诊断导出的最后一段 ALB45DiagnosticsReport）。
   · 别人主页简介区：用户：“鉴于别人的主页过于乱，我需要一个将我截取的区域收起的功能，你加入对那个区域的诊断，然后这次抓取完，
     下次做一个收起的按钮。它原本就有一个按钮是用来展开这里的更多简介的，直接借用它，把它挪到粉丝数这些的右列”。
     别人主页（AWEUserDetailViewController）出现 1 秒、2.5 秒后各记一次（导出时还开着就当场再记一次）：获赞 / 关注 / 粉丝那一排、
     简介、标签那一排和 ∨ 展开键、商品橱窗 / 直播动态 / 群聊那一排——每个字在屏幕上的位置；它们共同的父视图往下的整棵树
     （类名、位置、字、按钮、手势）；以及简介区里像“展开”键的小按钮。
   · 团购、我（收藏）页的底部：底下 220 点里所有整宽的视图（类名、位置、底色、模糊、归属控制器），看是谁在底栏后面垫着。
     团购频道布局时、我页内层列表布局时各记一次（每 3 秒最多一次），导出时在那一页就当场再记。
   · 另外把这一版的几份报告收在一起：保存视频 / 实况、双击面板、单双列、全屏、评论区不缩、三段式底栏下标修正。
   Included by Tweak.x after ALBQuickJump.h. */

extern NSString *AWECommentFreezeReport(void);

static NSString *ALB45TextOf(UIView *v) {
    NSString *text = nil;
    if ([v isKindOfClass:UILabel.class]) text = ((UILabel *)v).text ?: ((UILabel *)v).attributedText.string;
    else if ([v isKindOfClass:UIButton.class]) text = [(UIButton *)v currentTitle] ?: [(UIButton *)v currentAttributedTitle].string;
    else if ([v isKindOfClass:UITextView.class]) text = ((UITextView *)v).text;
    else {
        NSString *cls = NSStringFromClass(v.class);
        if ([cls containsString:@"Label"]) {
            @try { id t = [v valueForKey:@"text"]; if ([t isKindOfClass:NSString.class]) text = t; } @catch (__unused NSException *e) {}
            if (!text.length) {
                @try { id a = [v valueForKey:@"attributedText"]; if ([a isKindOfClass:NSAttributedString.class]) text = [(NSAttributedString *)a string]; }
                @catch (__unused NSException *e) {}
            }
        }
    }
    text = [text stringByReplacingOccurrencesOfString:@"\n" withString:@"⏎"];
    return text.length ? text : nil;
}
static NSString *ALB45Short(NSString *text, NSUInteger limit) {
    if (!text) return @"";
    return text.length > limit ? [[text substringToIndex:limit] stringByAppendingString:@"…"] : text;
}
static NSString *ALB45Rect(CGRect r) { return [NSString stringWithFormat:@"(%.0f,%.0f %.0f×%.0f)", r.origin.x, r.origin.y, r.size.width, r.size.height]; }
static NSString *ALB45Extras(UIView *v) {
    NSMutableArray<NSString *> *parts = [NSMutableArray array];
    if ([v isKindOfClass:UIControl.class]) {
        UIControl *c = (UIControl *)v;
        NSMutableArray<NSString *> *actions = [NSMutableArray array];
        for (id target in c.allTargets) {
            for (NSString *a in [c actionsForTarget:target forControlEvent:UIControlEventTouchUpInside] ?: @[])
                [actions addObject:[NSString stringWithFormat:@"%@.%@", NSStringFromClass([target class]), a]];
        }
        [parts addObject:actions.count ? [@"按钮→" stringByAppendingString:[actions componentsJoinedByString:@","]] : @"按钮"];
    }
    if (v.gestureRecognizers.count) {
        NSMutableArray<NSString *> *g = [NSMutableArray array];
        for (UIGestureRecognizer *r in v.gestureRecognizers) if (r.enabled) [g addObject:NSStringFromClass(r.class)];
        if (g.count) [parts addObject:[@"手势 " stringByAppendingString:[g componentsJoinedByString:@","]]];
    }
    if (v.accessibilityLabel.length && ![v isKindOfClass:UILabel.class]) [parts addObject:[@"读作 " stringByAppendingString:ALB45Short(v.accessibilityLabel, 20)]];
    if ([v isKindOfClass:UIImageView.class] && ((UIImageView *)v).image) {
        CGSize s = ((UIImageView *)v).image.size;
        [parts addObject:[NSString stringWithFormat:@"图 %.0f×%.0f", s.width, s.height]];
    }
    if ([v.nextResponder isKindOfClass:UIViewController.class]) [parts addObject:[@"属于 " stringByAppendingString:NSStringFromClass(v.nextResponder.class)]];
    if (v.alpha < 0.99) [parts addObject:[NSString stringWithFormat:@"α%.2f", v.alpha]];
    return parts.count ? [@" " stringByAppendingString:[parts componentsJoinedByString:@" "]] : @"";
}

#pragma mark - 别人主页简介区

static NSString *gALB45Profile;
static NSUInteger gALB45ProfileCaptures;
static __weak UIViewController *gALB45ProfileController;

static UIView *ALB45CommonAncestor(UIView *a, UIView *b) {
    if (!a) return b;
    if (!b) return a;
    NSMutableSet<NSValue *> *chain = [NSMutableSet set];
    for (UIView *v = a; v; v = v.superview) [chain addObject:[NSValue valueWithNonretainedObject:v]];
    for (UIView *v = b; v; v = v.superview) if ([chain containsObject:[NSValue valueWithNonretainedObject:v]]) return v;
    return nil;
}

static void ALB45DumpRegion(UIView *view, UIWindow *window, CGRect region, NSUInteger depth, NSMutableArray<NSString *> *lines) {
    if (!view || depth > 9 || lines.count >= 160 || view.hidden) return;
    CGRect r = [view convertRect:view.bounds toView:window];
    if (!CGRectIntersectsRect(r, region) && depth > 0) return;
    NSString *text = ALB45TextOf(view);
    [lines addObject:[NSString stringWithFormat:@"%@%@ %@%@%@", [@"" stringByPaddingToLength:depth * 2 withString:@" " startingAtIndex:0],
        NSStringFromClass(view.class), ALB45Rect(r), text ? [NSString stringWithFormat:@" “%@”", ALB45Short(text, 40)] : @"", ALB45Extras(view)]];
    if ([view isKindOfClass:UILabel.class]) return;
    for (UIView *sub in view.subviews) ALB45DumpRegion(sub, window, region, depth + 1, lines);
}

static void ALB45CaptureProfile(UIViewController *controller, NSString *when) {
    UIView *root = controller.viewIfLoaded;
    UIWindow *window = root.window;
    if (!window) return;
    gALB45ProfileCaptures++;
    CGFloat wh = CGRectGetHeight(window.bounds);
    NSMutableArray<NSDictionary *> *texts = [NSMutableArray array];
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < queue.count && i < 3000; i++) {
        UIView *v = queue[i];
        if (v.hidden || v.alpha < 0.01) continue;
        CGRect r = [v convertRect:v.bounds toView:window];
        if (CGRectGetMinY(r) > wh * 0.85 || CGRectGetMaxY(r) < 0) continue;
        NSString *text = ALB45TextOf(v);
        if (text) [texts addObject:@{@"v": v, @"t": text, @"r": [NSValue valueWithCGRect:r]}];
        if (![v isKindOfClass:UILabel.class]) [queue addObjectsFromArray:v.subviews];
    }
    UIView *likes = nil, *showcase = nil, *tabs = nil;
    CGFloat likesY = -1;
    for (NSDictionary *item in texts) {
        NSString *t = item[@"t"];
        CGRect r = [item[@"r"] CGRectValue];
        if (!likes && [t containsString:@"获赞"]) { likes = item[@"v"]; likesY = CGRectGetMinY(r); }
    }
    for (NSDictionary *item in texts) {
        NSString *t = item[@"t"];
        CGRect r = [item[@"r"] CGRectValue];
        if (likesY >= 0 && CGRectGetMinY(r) < likesY) continue;
        if (!showcase && ([t containsString:@"商品橱窗"] || [t containsString:@"橱窗"] || [t containsString:@"直播动态"] || [t containsString:@"群聊"])) showcase = item[@"v"];
        if (!tabs && ([t isEqualToString:@"作品"] || [t hasPrefix:@"作品 "]) && CGRectGetMinY(r) > likesY + 20) tabs = item[@"v"];
    }
    CGFloat top = likesY >= 0 ? likesY - 6 : 0;
    CGFloat bottom = tabs ? CGRectGetMinY([tabs convertRect:tabs.bounds toView:window]) : wh * 0.7;
    if (showcase) bottom = MAX(bottom, CGRectGetMaxY([showcase convertRect:showcase.bounds toView:window]) + 4);
    CGRect region = CGRectMake(0, top, CGRectGetWidth(window.bounds), MAX(40, bottom - top));
    NSMutableArray<NSString *> *textLines = [NSMutableArray array];
    for (NSDictionary *item in texts) {
        CGRect r = [item[@"r"] CGRectValue];
        if (!CGRectIntersectsRect(r, region)) continue;
        UIView *v = item[@"v"];
        [textLines addObject:[NSString stringWithFormat:@"“%@” %@ %@%@", ALB45Short(item[@"t"], 40), NSStringFromClass(v.class), ALB45Rect(r),
            [v isKindOfClass:UILabel.class] ? [NSString stringWithFormat:@" 行数%ld", (long)((UILabel *)v).numberOfLines] : @""]];
        if (textLines.count >= 50) break;
    }
    // 简介区里像“展开”的小按钮：可点（控件或带手势）、不超过 48×48、在区域里
    NSMutableArray<NSString *> *expanders = [NSMutableArray array];
    NSMutableArray<UIView *> *scan = [NSMutableArray arrayWithObject:root];
    for (NSUInteger i = 0; i < scan.count && i < 3000 && expanders.count < 12; i++) {
        UIView *v = scan[i];
        if (v.hidden || v.alpha < 0.01) continue;
        [scan addObjectsFromArray:v.subviews];
        CGRect r = [v convertRect:v.bounds toView:window];
        if (!CGRectIntersectsRect(r, region) || r.size.width > 48 || r.size.height > 48 || r.size.width < 6) continue;
        BOOL tappable = [v isKindOfClass:UIControl.class] || v.gestureRecognizers.count > 0;
        NSString *t = ALB45TextOf(v) ?: v.accessibilityLabel;
        BOOL looks = [t containsString:@"展开"] || [t containsString:@"更多"] || [t containsString:@"∨"] || [t containsString:@"收起"];
        if (!tappable && !looks) continue;
        [expanders addObject:[NSString stringWithFormat:@"%@ %@ “%@”%@", NSStringFromClass(v.class), ALB45Rect(r), ALB45Short(t, 20), ALB45Extras(v)]];
    }
    UIView *ancestor = ALB45CommonAncestor(likes, showcase ?: tabs);
    NSMutableArray<NSString *> *tree = [NSMutableArray array];
    if (ancestor) ALB45DumpRegion(ancestor, window, CGRectInset(region, 0, -4), 0, tree);
    NSMutableArray<NSString *> *chain = [NSMutableArray array];
    for (UIView *v = ancestor; v && chain.count < 8; v = v.superview) [chain addObject:NSStringFromClass(v.class)];
    gALB45Profile = [NSString stringWithFormat:@"%@（%@）区域 %@；获赞=%@ 橱窗那排=%@ 作品页签=%@；共同父视图链 %@\n字：\n%@\n像展开键的：%@\n结构：\n%@",
        when, NSStringFromClass(controller.class), ALB45Rect(region), likes ? @"找到" : @"没找到", showcase ? @"找到" : @"没找到", tabs ? @"找到" : @"没找到",
        chain.count ? [chain componentsJoinedByString:@" › "] : @"-",
        textLines.count ? [textLines componentsJoinedByString:@"\n"] : @"（区域里没有字）",
        expanders.count ? [expanders componentsJoinedByString:@"；"] : @"没找到",
        tree.count ? [tree componentsJoinedByString:@"\n"] : @"（没找到共同父视图）"];
}

/* 3.11.47：3.11.46 诊断“见过 0 次”——别人的主页不一定是 AWEUserDetailViewController（我页用的 AWEUserHomeViewControllerV2，
   别人的主页可能也是它）。现在几个候选类都钩上，别人主页＝不是导航栈最底下那一页（我页是 tab 的根）；钩上了哪些写进诊断 */
static NSMutableArray<NSString *> *gALB45ProfileHooked;
static NSString *gALB45ProfileSeenClass;
static void ALB45ProfileAppeared(UIViewController *controller) {
    NSString *name = NSStringFromClass(controller.class);
    BOOL detail = [name hasPrefix:@"AWEUserDetailViewController"];
    UINavigationController *nav = controller.navigationController;
    BOOL pushed = nav && nav.viewControllers.firstObject != controller;
    if (!detail && !(pushed || controller.presentingViewController)) return;
    gALB45ProfileSeenClass = name;
    gALB45ProfileController = controller;
    __weak UIViewController *weak = controller;
    for (NSNumber *delay in @[@1.0, @2.5]) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay.doubleValue * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            UIViewController *live = weak;
            if (live.viewIfLoaded.window) ALB45CaptureProfile(live, [NSString stringWithFormat:@"出现 %.1f 秒后", delay.doubleValue]);
        });
    }
}

#pragma mark - 团购、我（收藏）页的底部

static NSString *gALB45GrouponBand, *gALB45ProfileBand;
static CFTimeInterval gALB45GrouponAt, gALB45ProfileBandAt;

static NSString *ALB45BottomBand(UIWindow *window, NSString *why) {
    if (!window) return @"-";
    CGFloat ww = CGRectGetWidth(window.bounds), wh = CGRectGetHeight(window.bounds);
    NSMutableArray<NSString *> *lines = [NSMutableArray array];
    NSMutableArray<NSArray *> *queue = [NSMutableArray arrayWithObject:@[window, @0]];
    for (NSUInteger i = 0; i < queue.count && i < 4000 && lines.count < 70; i++) {
        UIView *v = queue[i][0];
        NSUInteger depth = [queue[i][1] unsignedIntegerValue];
        if (v.hidden || v.alpha < 0.01) continue;
        CGRect r = [v convertRect:v.bounds toView:window];
        if (CGRectGetMaxY(r) < wh - 220 || CGRectGetMinY(r) > wh) continue;
        for (UIView *sub in v.subviews) [queue addObject:@[sub, @(depth + 1)]];
        if (r.size.width < ww * 0.5 || depth == 0) continue;
        UIColor *bg = v.backgroundColor;
        CGFloat alpha = bg ? CGColorGetAlpha(bg.CGColor) : 0;
        CGColorRef layerBG = v.layer.backgroundColor;
        if (!bg && layerBG) alpha = CGColorGetAlpha(layerBG);
        NSString *effect = [v isKindOfClass:UIVisualEffectView.class] ? NSStringFromClass([((UIVisualEffectView *)v).effect class] ?: NSNull.class) : nil;
        BOOL interesting = alpha > 0.01 || effect || [v isKindOfClass:UIScrollView.class] || v.layer.contents ||
                           [v.nextResponder isKindOfClass:UIViewController.class] || [v.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar"];
        if (!interesting) continue;
        NSString *color = @"";
        if (alpha > 0.01) {
            CGFloat cr = 0, cg = 0, cb = 0, ca = 0;
            UIColor *c = bg ?: [UIColor colorWithCGColor:layerBG];
            if (![c getRed:&cr green:&cg blue:&cb alpha:&ca]) { CGFloat w = 0; [c getWhite:&w alpha:&ca]; cr = cg = cb = w; }
            color = [NSString stringWithFormat:@" 底#%02X%02X%02X α%.2f", (int)round(cr * 255), (int)round(cg * 255), (int)round(cb * 255), alpha];
        }
        NSString *scroll = @"";
        if ([v isKindOfClass:UIScrollView.class]) {
            UIScrollView *sv = (UIScrollView *)v;
            scroll = [NSString stringWithFormat:@" 内容高%.0f 下内缩%.0f(调整后%.0f)", sv.contentSize.height, sv.contentInset.bottom, sv.adjustedContentInset.bottom];
        }
        [lines addObject:[NSString stringWithFormat:@"%lu %@ %@%@%@%@%@%@%@", (unsigned long)depth, NSStringFromClass(v.class), ALB45Rect(r), color,
            effect ? [@" effect=" stringByAppendingString:effect] : @"", scroll, v.layer.contents ? @" 有图" : @"", v.clipsToBounds ? @" 裁" : @"",
            [v.nextResponder isKindOfClass:UIViewController.class] ? [@" 属于 " stringByAppendingString:NSStringFromClass(v.nextResponder.class)] : @""]];
    }
    return [NSString stringWithFormat:@"%@（%@，底栏=%@）\n%@", why, [NSDate date].description,
        ALBDockModeActive() ? @"三段式" : @"液态", lines.count ? [lines componentsJoinedByString:@"\n"] : @"（底下没有整宽的视图）"];
}

static void ALB45NoteGrouponLayout(UIWindow *window) {
    CFTimeInterval now = CACurrentMediaTime();
    if (!window || now - gALB45GrouponAt < 3.0) return;
    gALB45GrouponAt = now;
    __weak UIWindow *weak = window;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.4 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        UIWindow *w = weak;
        if (w && gALB33GrouponContainer.window == w) gALB45GrouponBand = ALB45BottomBand(w, @"团购频道布局后");
    });
}
static void ALB45NoteProfileGridLayout(UIView *grid) {
    CFTimeInterval now = CACurrentMediaTime();
    UIWindow *window = grid.window;
    if (!window || now - gALB45ProfileBandAt < 3.0) return;
    gALB45ProfileBandAt = now;
    __weak UIWindow *weak = window;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.4 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        UIWindow *w = weak;
        if (w && AWECurrentMainTabIndex() == 4) gALB45ProfileBand = ALB45BottomBand(w, @"我页（含收藏）内层列表布局后");
    });
}

static void ALB45DiagnosticsInstall(void) {
    gALB45ProfileHooked = [NSMutableArray array];
    for (NSString *name in @[@"AWEUserDetailViewController", @"AWEUserHomeViewControllerV2", @"AWEUserHomeViewController"]) {
        Class cls = NSClassFromString(name);
        if (cls && ALBMoreHook(cls, @"viewDidAppear:", ALBMoreHookBool, ^(id object) { ALB45ProfileAppeared((UIViewController *)object); }))
            [gALB45ProfileHooked addObject:name];
    }
}

NSString *ALB45DiagnosticsReport(void) {
    UIWindow *window = AWEActiveMainWindow();
    // 导出时还在那一页：当场再记一次
    UIViewController *profile = gALB45ProfileController;
    if (profile.viewIfLoaded.window) ALB45CaptureProfile(profile, @"导出时");
    if (gALB33GrouponContainer.window) gALB45GrouponBand = ALB45BottomBand(gALB33GrouponContainer.window, @"导出时在团购");
    else if (AWECurrentMainTabIndex() == 4 && window) gALB45ProfileBand = ALB45BottomBand(window, @"导出时在我页");
    NSMutableString *text = [NSMutableString string];
    [text appendFormat:@"【保存视频 / 实况】%@", ALBMediaSaveReport()];
    [text appendFormat:@"\n【双击面板】点面板外关闭 %lu 次、上下滑关闭 %lu 次（3.11.46 按下就关：一共 %lu 次，最近一次按在 %@）；“面板 / 评”模式上区打开面板 %lu 次、下区打开评论 %lu 次",
        (unsigned long)gALBPaletteOutsideCloses, (unsigned long)gALBPaletteSwipeCloses, (unsigned long)gALBOutsidePressCloses,
        gALBOutsidePressLastHit ?: @"-", (unsigned long)gALBPaletteZoneOpens, (unsigned long)gALBPaletteZoneComments];
    [text appendFormat:@"\n【✦ 悬浮面板（3.11.46）】%@", ALBFloatPanelReport()];
    [text appendFormat:@"\n【单双列】%@", ALBColumnReport()];
    [text appendFormat:@"\n【全屏】%@", AWEFSReport()];
    [text appendFormat:@"\n【评论区不缩到顶部】%@", AWECommentFreezeReport()];
    [text appendFormat:@"\n【三段式底栏下标】三段式=%@ 抖音自己的下标=%ld 换算后=%ld（修正过 %lu 次）",
        ALBDockModeActive() ? @"是" : @"否", (long)AWECurrentOriginalMainTabIndex(), (long)AWECurrentMainTabIndex(), (unsigned long)gALB45DockIndexFixes];
    [text appendFormat:@"\n【别人主页简介区】钩上 %@；最近一次认到的页面类 %@；见过 %lu 次：%@",
        gALB45ProfileHooked.count ? [gALB45ProfileHooked componentsJoinedByString:@"、"] : @"一个都没有", gALB45ProfileSeenClass ?: @"-",
        (unsigned long)gALB45ProfileCaptures, gALB45Profile ?: @"还没打开过别人的主页（打开一次，停 3 秒再导出）"];
    [text appendFormat:@"\n【右下角音乐】点了 %lu 次；放行抖音的点按 %lu 次；没送到时兜底 %lu 次（成功 %lu）；点完音乐页没出来 %lu 次；最近 %@",
        (unsigned long)gALBMusicTaps, (unsigned long)gALBNativeGestureAllowed, (unsigned long)gALBNativeFallbackRuns,
        (unsigned long)gALBNativeFallbackOK, (unsigned long)gALBMusicNoPage, gALBMusicLast ?: @"-"];
    [text appendFormat:@"\n【默认最高画质】%@；换成最高档地址 %lu 次、档位只留最高 %lu 次", ALB5Bool(@"playback.highestQuality", YES) ? @"开" : @"关",
        (unsigned long)gALBHQPicked, (unsigned long)gALBHQNarrowed];
    [text appendFormat:@"\n【头像点按】展开菜单=%@；不展开时点了 %lu 次；最近 %@", ALB5Bool(@"avatar.menu", YES) ? @"开" : @"关",
        (unsigned long)gALBAvatarTaps, gALBAvatarLast ?: @"-"];
    [text appendFormat:@"\n【一键清屏】%@", AWEStableCleanReport()];
    [text appendFormat:@"\n【团购底部】%@", gALB45GrouponBand ?: @"还没进过团购频道（进去停 3 秒再导出）"];
    [text appendFormat:@"\n【我 / 收藏底部】%@", gALB45ProfileBand ?: @"还没进过我页（进去切到收藏，停 3 秒再导出）"];
    return text;
}
