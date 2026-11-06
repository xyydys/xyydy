/* 3.11.45 三段式底栏 ↑ 键的快捷跳转：消息通知、特别关注、我的订单、观看历史。
   用户：“右侧那个箭头就是快捷跳转到对应页面……选项里就是之前抓到的那几处页面，消息通知，特别关注，我的订单，观看历史”。

   3.11.37 的采集里这几页只有观看历史带着地址（BDXViewController → config.schemaParam.originURL＝aweme://lynxview/?…
   channel=watch_history…），另外三页（AWEIMFansAndInteractMergeViewController、AWESkylightUnreadListViewController、
   IESECOrderListViewController）一条“://”都没有，抖音是直接新建控制器推进去的。所以：
   · 每次这几页出现（viewDidAppear:）记下：地址（找得到的话）、控制器自己的简单变量（字符串 / 数字）、是推进来的还是弹出来的，
     并留着这一页（本次启动里）。
   · 跳转时按顺序试：① 有地址 → 交给抖音自己打开（应用代理 application:openURL:options:，不行再走系统 openURL，自己的地址
     系统直接交回抖音）；② 本次启动里留着的那一页 → 推回当前导航（弹出来的按原样弹）；③ 设置里开了“试着新建” → 新建同一个类、
     抄回记下的简单变量再推进去（试验：可能空白或打不开）；都不行 → 提示先自己打开一次。
   · 诊断【快捷跳转】：每一页见过几次、记到了什么、最近一次跳转走的哪一条、抖音注册了哪些地址前缀。
   Included by Tweak.x after ALBSidePalette.h; hooks installed at run time (ALBQuickJumpInstall). */

static NSArray<NSString *> *ALBQJTitles(void) { return @[@"快捷跳转", @"消息通知", @"特别关注", @"我的订单", @"观看历史"]; }
static NSArray<NSString *> *ALBQJSymbols(void) { return @[@"arrow.up.circle.fill", @"bell", @"star.circle", @"bag", @"clock.arrow.circlepath"]; }
static NSArray<NSString *> *ALBQJClassNames(void) {
    return @[@"", @"AWEIMFansAndInteractMergeViewController", @"AWESkylightUnreadListViewController", @"IESECOrderListViewController", @"BDXViewController"];
}
static NSString *ALBQJKey(NSInteger kind) { return ALB5Key([NSString stringWithFormat:@"jump.%ld", (long)kind]); }

static NSMutableDictionary<NSNumber *, UIViewController *> *gALBQJKept;
static NSMutableDictionary<NSNumber *, NSNumber *> *gALBQJSeen;
static NSMutableDictionary<NSNumber *, NSString *> *gALBQJResult;
static NSMutableArray<NSString *> *gALBQJLynxSeen;
static NSString *gALBQJHooked;

static NSString *ALBQJShort(NSString *text, NSUInteger limit) {
    if (![text isKindOfClass:NSString.class]) return @"-";
    return text.length > limit ? [[text substringToIndex:limit] stringByAppendingString:@"…"] : text;
}

/* 控制器身上能找到的地址（都在 @try 里；找不到的键抖音会抛 NSUnknownKeyException） */
static NSString *ALBQJURLFrom(UIViewController *vc) {
    for (NSString *path in @[@"config.schemaParam.originURL", @"config.schemaParam.url", @"schemaParam.originURL", @"config.originURL",
                             @"originURL", @"schema", @"schemaURL", @"schemeURL", @"routeURL", @"openURL", @"pageURL", @"url"]) {
        id value = nil;
        @try { value = [vc valueForKeyPath:path]; } @catch (__unused NSException *e) { value = nil; }
        NSString *text = [value isKindOfClass:NSURL.class] ? [(NSURL *)value absoluteString] : ([value isKindOfClass:NSString.class] ? value : nil);
        if ([text containsString:@"://"]) return text;
    }
    return nil;
}

/* 控制器自己（到 UIViewController 为止）的变量：字符串 / 数字 / 布尔记值，别的只记类名（诊断用） */
static NSDictionary<NSString *, id> *ALBQJIvars(UIViewController *vc, NSMutableArray<NSString *> *described) {
    NSMutableDictionary<NSString *, id> *values = [NSMutableDictionary dictionary];
    for (Class cls = object_getClass(vc); cls && cls != UIViewController.class && cls != UIResponder.class; cls = class_getSuperclass(cls)) {
        NSString *clsName = NSStringFromClass(cls);
        if ([clsName hasPrefix:@"NSKVONotifying_"] || [clsName hasPrefix:@"UI"]) continue;
        unsigned int count = 0;
        Ivar *ivars = class_copyIvarList(cls, &count);
        for (unsigned int i = 0; i < count && described.count < 60; i++) {
            const char *name = ivar_getName(ivars[i]), *type = ivar_getTypeEncoding(ivars[i]);
            if (!name || !type) continue;
            NSString *ivarName = @(name), *typeText = @(type);
            ptrdiff_t offset = ivar_getOffset(ivars[i]);
            uint8_t *base = (uint8_t *)(__bridge void *)vc;
            if (type[0] == '@') {
                BOOL valueClass = [typeText containsString:@"NSString"] || [typeText containsString:@"NSNumber"];
                if (valueClass) {
                    id value = object_getIvar(vc, ivars[i]);
                    if ([value isKindOfClass:NSString.class] || [value isKindOfClass:NSNumber.class]) {
                        values[ivarName] = value;
                        [described addObject:[NSString stringWithFormat:@"%@=%@", ivarName, ALBQJShort([value description], 60)]];
                    } else {
                        [described addObject:[NSString stringWithFormat:@"%@=nil", ivarName]];
                    }
                } else {
                    NSString *declared = typeText.length > 3 ? [typeText substringWithRange:NSMakeRange(2, typeText.length - 3)] : @"id";
                    [described addObject:[NSString stringWithFormat:@"%@:%@", ivarName, declared]];
                }
            } else if (type[0] == 'B' || type[0] == 'c') {
                BOOL v = *(BOOL *)(base + offset);
                values[ivarName] = @{@"t": @"B", @"v": @(v)};
                [described addObject:[NSString stringWithFormat:@"%@=%d", ivarName, v]];
            } else if (type[0] == 'q' || type[0] == 'Q' || type[0] == 'l' || type[0] == 'L') {
                long long v = *(long long *)(base + offset);
                values[ivarName] = @{@"t": @"q", @"v": @(v)};
                [described addObject:[NSString stringWithFormat:@"%@=%lld", ivarName, v]];
            } else if (type[0] == 'i' || type[0] == 'I') {
                int v = *(int *)(base + offset);
                values[ivarName] = @{@"t": @"i", @"v": @(v)};
                [described addObject:[NSString stringWithFormat:@"%@=%d", ivarName, v]];
            }
        }
        free(ivars);
    }
    return values;
}

/* 3.11.49 用户：“每次重启之后除了观看历史能够跳转，其余三个都跳转到未知页面，根本就啥也不显示”。新建页面时原来把记下的所有
   简单变量都抄回去了，里面有页面自己的状态（_hasShownInteractPage=1、_VCState、_firstScreenRenderStatus=3、_refreshing…），
   新页面以为自己已经显示 / 渲染过，就不再加载，一片空白。现在只抄“从哪进来、落在哪一页”这类配置（名字里带 enter / landing /
   from / source / noticeType / tab / scene / type 的），状态类的一律不抄。 */
static BOOL ALBQJIvarIsConfig(NSString *name) {
    NSString *n = name.lowercaseString;
    for (NSString *bad in @[@"has", @"is", @"did", @"should", @"state", @"status", @"refresh", @"render", @"request", @"animat",
                            @"trigger", @"shown", @"visi", @"appear", @"ready", @"mount", @"loading", @"current", @"selected", @"count"])
        if ([n containsString:bad]) return NO;
    for (NSString *good in @[@"enter", @"landing", @"from", @"source", @"noticetype", @"tab", @"scene", @"type", @"channel"])
        if ([n containsString:good]) return YES;
    return NO;
}
static NSUInteger gALBQJRestoredIvars;
/* configOnly：页面本身只抄配置类的（3.11.49）；3.11.51 新建的 context 对象整个抄（它本来就是配置） */
static NSUInteger ALBQJRestoreIvars(id vc, NSDictionary<NSString *, id> *values, BOOL configOnly) {
    NSUInteger restored = 0;
    if (![values isKindOfClass:NSDictionary.class]) return 0;
    for (Class cls = object_getClass(vc); cls && cls != UIViewController.class && cls != NSObject.class; cls = class_getSuperclass(cls)) {
        unsigned int count = 0;
        Ivar *ivars = class_copyIvarList(cls, &count);
        for (unsigned int i = 0; i < count; i++) {
            const char *name = ivar_getName(ivars[i]), *type = ivar_getTypeEncoding(ivars[i]);
            id saved = name ? values[@(name)] : nil;
            if (!saved || !type || (configOnly && !ALBQJIvarIsConfig(@(name)))) continue;
            restored++;
            uint8_t *base = (uint8_t *)(__bridge void *)vc;
            ptrdiff_t offset = ivar_getOffset(ivars[i]);
            if (type[0] == '@' && ([saved isKindOfClass:NSString.class] || [saved isKindOfClass:NSNumber.class])) {
                object_setIvarWithStrongDefault(vc, ivars[i], saved);
            } else if ([saved isKindOfClass:NSDictionary.class]) {
                NSString *t = saved[@"t"];
                NSNumber *v = saved[@"v"];
                if ([t isEqualToString:@"B"] && (type[0] == 'B' || type[0] == 'c')) *(BOOL *)(base + offset) = v.boolValue;
                else if ([t isEqualToString:@"q"] && (type[0] == 'q' || type[0] == 'Q' || type[0] == 'l' || type[0] == 'L')) *(long long *)(base + offset) = v.longLongValue;
                else if ([t isEqualToString:@"i"] && (type[0] == 'i' || type[0] == 'I')) *(int *)(base + offset) = v.intValue;
            }
        }
        free(ivars);
    }
    return restored;
}

/* 3.11.51：页面的 context（我的订单 -initWithContext:）：字典原样记；对象记类名和它自己的简单变量。重建时按这个新建一个。 */
static NSDictionary *ALBQJCaptureContext(UIViewController *vc) {
    for (Class cls = object_getClass(vc); cls && cls != UIViewController.class; cls = class_getSuperclass(cls)) {
        unsigned int count = 0;
        Ivar *ivars = class_copyIvarList(cls, &count);
        id value = nil;
        NSString *ivarName = nil;
        for (unsigned int i = 0; i < count && !value; i++) {
            const char *name = ivar_getName(ivars[i]), *type = ivar_getTypeEncoding(ivars[i]);
            if (!name || !type || type[0] != '@' || ![@(name).lowercaseString containsString:@"context"]) continue;
            value = object_getIvar(vc, ivars[i]);
            ivarName = @(name);
        }
        free(ivars);
        if (!value) continue;
        if ([value isKindOfClass:NSDictionary.class]) {
            BOOL plist = NO;
            @try { plist = [NSPropertyListSerialization propertyList:value isValidForFormat:NSPropertyListBinaryFormat_v1_0]; }
            @catch (__unused NSException *e) { plist = NO; }
            return plist ? @{@"ivar": ivarName, @"dict": value} : @{@"ivar": ivarName, @"class": NSStringFromClass([value class]), @"note": @"字典里有存不下的值"};
        }
        NSMutableArray<NSString *> *described = [NSMutableArray array];
        NSDictionary *simple = ALBQJIvars((UIViewController *)value, described);
        return @{@"ivar": ivarName, @"class": NSStringFromClass([value class]), @"ivars": simple ?: @{},
                 @"described": [described componentsJoinedByString:@"，"]};
    }
    return nil;
}
static id ALBQJRebuildContext(NSDictionary *saved, NSString **note) {
    if (![saved isKindOfClass:NSDictionary.class]) { if (note) *note = @"没记到 context"; return nil; }
    if ([saved[@"dict"] isKindOfClass:NSDictionary.class]) { if (note) *note = @"context＝记下的字典"; return [saved[@"dict"] mutableCopy]; }
    NSString *name = saved[@"class"];
    Class cls = [name isKindOfClass:NSString.class] ? NSClassFromString(name) : Nil;
    // Swift 类（名字带点或 _Tt）没有 ObjC 的 init 可调，不碰
    if (!cls || [name hasPrefix:@"_Tt"] || [name containsString:@"."] || ![cls instancesRespondToSelector:@selector(init)]) {
        if (note) *note = [NSString stringWithFormat:@"context 类 %@ 建不了", name ?: @"-"];
        return nil;
    }
    id object = [[cls alloc] init];
    NSUInteger restored = ALBQJRestoreIvars(object, saved[@"ivars"], NO);
    if (note) *note = [NSString stringWithFormat:@"context＝新建 %@、抄回 %lu 个变量", name, (unsigned long)restored];
    return object;
}

/* 3.11.52：子页面属性。3.11.51 的诊断：消息通知按 -initWithLandingTab: 新建后 1.5 秒字 0 个、格子 0 个、子页面 0 个；它的
   fansViewController / interactViewController 是可写属性——是创建它的地方塞进去的，页面自己不建。进这一页时记下这些属性里
   是哪个类（只记 ObjC 类；Swift 类的 init 可能直接崩，不碰），新建后它们还空着就各建一个、抄回配置变量、塞回去。 */
static NSArray<NSDictionary *> *ALBQJCaptureChildren(UIViewController *vc) {
    NSMutableArray<NSDictionary *> *children = [NSMutableArray array];
    unsigned int count = 0;
    objc_property_t *properties = class_copyPropertyList(vc.class, &count);
    for (unsigned int i = 0; i < count && children.count < 4; i++) {
        const char *attrs = property_getAttributes(properties[i]);
        NSString *attributes = attrs ? @(attrs) : @"";
        NSArray<NSString *> *parts = [attributes componentsSeparatedByString:@","];
        if (![attributes hasPrefix:@"T@"] || [parts containsObject:@"R"]) continue;
        NSString *name = @(property_getName(properties[i]));
        id value = nil;
        @try { value = [vc valueForKey:name]; } @catch (__unused NSException *e) { value = nil; }
        if (![value isKindOfClass:UIViewController.class]) continue;
        NSMutableArray<NSString *> *described = [NSMutableArray array];
        NSDictionary *simple = ALBQJIvars((UIViewController *)value, described) ?: @{};
        [children addObject:@{@"name": name, @"class": NSStringFromClass([value class]), @"ivars": simple}];
    }
    free(properties);
    return children;
}
static NSUInteger ALBQJFillChildren(UIViewController *page, NSArray *children, NSMutableArray<NSString *> *notes) {
    NSUInteger made = 0;
    for (NSDictionary *child in [children isKindOfClass:NSArray.class] ? children : @[]) {
        NSString *name = child[@"name"], *className = child[@"class"];
        if (![name isKindOfClass:NSString.class] || ![className isKindOfClass:NSString.class]) continue;
        id current = nil;
        @try { current = [page valueForKey:name]; } @catch (__unused NSException *e) { continue; }
        if (current) { [notes addObject:[NSString stringWithFormat:@"%@ 已有", name]]; continue; }
        Class cls = NSClassFromString(className);
        if (![cls isSubclassOfClass:UIViewController.class] || [className hasPrefix:@"_Tt"] || [className containsString:@"."]) {
            [notes addObject:[NSString stringWithFormat:@"%@=%@ 建不了", name, className]];
            continue;
        }
        UIViewController *made1 = nil;
        @try {
            made1 = [[cls alloc] init];
            ALBQJRestoreIvars(made1, child[@"ivars"], YES);
            [page setValue:made1 forKey:name];
        } @catch (__unused NSException *e) { made1 = nil; }
        if (made1) { made++; [notes addObject:[NSString stringWithFormat:@"%@=新建 %@", name, className]]; }
        else [notes addObject:[NSString stringWithFormat:@"%@=%@ 出错", name, className]];
    }
    return made;
}

/* 3.11.51：按页面自己的初始化方法新建。3.11.49 的诊断：消息通知只有 -initWithLandingTab:，我的订单 -init / -initWithContext:；
   之前一律 [[cls alloc] init]，页面自己的初始化（子页面、请求参数）没走到。参数按方法签名的类型传。 */
typedef id __attribute__((ns_returns_retained)) (*ALBQJInitObjFn)(id __attribute__((ns_consumed)), SEL, id);
typedef id __attribute__((ns_returns_retained)) (*ALBQJInitLLFn)(id __attribute__((ns_consumed)), SEL, long long);
typedef id __attribute__((ns_returns_retained)) (*ALBQJInitIntFn)(id __attribute__((ns_consumed)), SEL, int);
typedef id __attribute__((ns_returns_retained)) (*ALBQJInitBoolFn)(id __attribute__((ns_consumed)), SEL, BOOL);
static id ALBQJInitWith(Class cls, SEL sel, id object, long long number) {
    Method method = class_getInstanceMethod(cls, sel);
    if (!method || method_getNumberOfArguments(method) != 3) return nil;
    char type[64] = {0};
    method_getArgumentType(method, 2, type, sizeof(type));
    const char *t = type;
    while (*t && strchr("rnNoORV", *t)) t++;
    switch (*t) {
        case '@': return ((ALBQJInitObjFn)objc_msgSend)([cls alloc], sel, object);
        case 'q': case 'Q': case 'l': case 'L': return ((ALBQJInitLLFn)objc_msgSend)([cls alloc], sel, number);
        case 'i': case 'I': case 's': case 'S': return ((ALBQJInitIntFn)objc_msgSend)([cls alloc], sel, (int)number);
        case 'B': case 'c': case 'C': return ((ALBQJInitBoolFn)objc_msgSend)([cls alloc], sel, (BOOL)(number != 0));
        default: return nil;
    }
}
static long long ALBQJSavedNumber(NSDictionary *ivars, NSString *name, long long fallback) {
    id value = [ivars isKindOfClass:NSDictionary.class] ? ivars[name] : nil;
    if ([value isKindOfClass:NSDictionary.class]) value = value[@"v"];
    return [value isKindOfClass:NSNumber.class] ? [value longLongValue] : fallback;
}
static UIViewController *ALBQJMakePage(NSDictionary *record, NSString **how) {
    Class cls = NSClassFromString(record[@"class"]);
    if (![cls isSubclassOfClass:UIViewController.class]) { if (how) *how = @"类不在"; return nil; }
    NSDictionary *ivars = [record[@"ivars"] isKindOfClass:NSDictionary.class] ? record[@"ivars"] : @{};
    UIViewController *page = nil;
    NSString *used = nil;
    SEL landing = NSSelectorFromString(@"initWithLandingTab:"), context = NSSelectorFromString(@"initWithContext:");
    if (!page && [cls instancesRespondToSelector:landing]) {
        long long tab = ALBQJSavedNumber(ivars, @"_landingTab", 0);
        page = ALBQJInitWith(cls, landing, @(tab), tab);
        if (page) used = [NSString stringWithFormat:@"-initWithLandingTab:%lld", tab];
    }
    if (!page && [cls instancesRespondToSelector:context] && record[@"context"]) {
        NSString *note = nil;
        id made = ALBQJRebuildContext(record[@"context"], &note);
        if (made) page = ALBQJInitWith(cls, context, made, 0);
        if (page) used = [NSString stringWithFormat:@"-initWithContext:（%@）", note];
        else used = note;
    }
    if (!page) {
        page = [[cls alloc] init];
        used = used ? [used stringByAppendingString:@"；改用 -init"] : @"-init";
    }
    NSUInteger restored = page ? ALBQJRestoreIvars(page, ivars, YES) : 0;
    gALBQJRestoredIvars += restored;
    NSMutableArray<NSString *> *notes = [NSMutableArray array];
    if (page && record[@"children"]) ALBQJFillChildren(page, record[@"children"], notes);
    if (how) *how = [NSString stringWithFormat:@"%@，抄回配置变量 %lu 个%@", used, (unsigned long)restored,
                     notes.count ? [NSString stringWithFormat:@"；子页面：%@", [notes componentsJoinedByString:@"、"]] : @""];
    return page;
}

/* 3.11.51：新建那一次的结果单独存（重启后也留着；3.11.49 的诊断里它被后面“推回留着的那一页”的成功盖掉了） */
static NSString *ALBQJRecreateKey(NSInteger kind) { return ALB5Key([NSString stringWithFormat:@"jump.%ld.recreate", (long)kind]); }
static void ALBQJNoteRecreate(NSInteger kind, NSDictionary *fields) {
    NSMutableDictionary *now = [[NSUserDefaults.standardUserDefaults dictionaryForKey:ALBQJRecreateKey(kind)] mutableCopy] ?: [NSMutableDictionary dictionary];
    [now addEntriesFromDictionary:fields];
    @try { [NSUserDefaults.standardUserDefaults setObject:now forKey:ALBQJRecreateKey(kind)]; } @catch (__unused NSException *e) {}
}

/* 这一页是第几种（观看历史只认 channel=watch_history 的 Lynx 页） */
static NSInteger ALBQJKindFor(UIViewController *vc, NSString **urlOut) {
    NSArray<NSString *> *names = ALBQJClassNames();
    for (NSInteger kind = 1; kind <= 4; kind++) {
        Class cls = NSClassFromString(names[(NSUInteger)kind]);
        if (!cls || ![vc isKindOfClass:cls]) continue;
        NSString *url = ALBQJURLFrom(vc);
        if (urlOut) *urlOut = url;
        if (kind == 4) {
            if (url) {
                if (!gALBQJLynxSeen) gALBQJLynxSeen = [NSMutableArray array];
                NSString *line = ALBQJShort(url, 160);
                if (![gALBQJLynxSeen containsObject:line]) [gALBQJLynxSeen addObject:line];
                while (gALBQJLynxSeen.count > 6) [gALBQJLynxSeen removeObjectAtIndex:0];
            }
            if (![url containsString:@"watch_history"]) return 0;
        }
        return kind;
    }
    return 0;
}

static void ALBQJPageAppeared(UIViewController *vc) {
    if (!vc.isViewLoaded || !vc.view.window) return;
    NSString *url = nil;
    NSInteger kind = ALBQJKindFor(vc, &url);
    if (kind <= 0) return;
    if (!gALBQJSeen) gALBQJSeen = [NSMutableDictionary dictionary];
    gALBQJSeen[@(kind)] = @(gALBQJSeen[@(kind)].unsignedIntegerValue + 1);
    UINavigationController *nav = vc.navigationController;
    BOOL modal = !nav || (nav.viewControllers.firstObject == vc && nav.presentingViewController != nil);
    NSMutableArray<NSString *> *described = [NSMutableArray array];
    NSDictionary *ivars = ALBQJIvars(vc, described);
    NSMutableArray<NSString *> *stack = [NSMutableArray array];
    for (UIViewController *c in nav.viewControllers) [stack addObject:NSStringFromClass(c.class)];
    NSMutableDictionary *record = [NSMutableDictionary dictionary];
    record[@"class"] = NSStringFromClass(vc.class);
    if (url) record[@"url"] = url;
    record[@"modal"] = @(modal);
    record[@"ivars"] = ivars;
    record[@"described"] = [described componentsJoinedByString:@"，"];
    record[@"stack"] = [stack componentsJoinedByString:@" › "];
    record[@"presenting"] = vc.presentingViewController ? NSStringFromClass(vc.presentingViewController.class) : @"-";
    record[@"at"] = @([NSDate date].timeIntervalSince1970);
    NSDictionary *context = ALBQJCaptureContext(vc);
    if (context) record[@"context"] = context;
    NSArray *children = ALBQJCaptureChildren(vc);
    if (children.count) record[@"children"] = children;
    @try { [NSUserDefaults.standardUserDefaults setObject:record forKey:ALBQJKey(kind)]; }
    @catch (__unused NSException *e) {
        [record removeObjectForKey:@"ivars"]; [record removeObjectForKey:@"context"]; [record removeObjectForKey:@"children"];
        @try { [NSUserDefaults.standardUserDefaults setObject:record forKey:ALBQJKey(kind)]; } @catch (__unused NSException *e2) {}
    }
    if (!gALBQJKept) gALBQJKept = [NSMutableDictionary dictionary];
    gALBQJKept[@(kind)] = (modal && nav) ? nav : vc;
}

static void ALBQuickJumpInstall(void) {
    static BOOL installed;
    NSMutableArray<NSString *> *hooked = [NSMutableArray array];
    BOOL missing = NO;
    static NSMutableSet<NSString *> *done;
    if (!done) done = [NSMutableSet set];
    for (NSString *name in ALBQJClassNames()) {
        if (!name.length || [done containsObject:name]) { if (name.length) [hooked addObject:name]; continue; }
        Class cls = NSClassFromString(name);
        if (!cls) { missing = YES; continue; }
        if (ALBMoreHook(cls, @"viewDidAppear:", ALBMoreHookBool, ^(id object) { ALBQJPageAppeared((UIViewController *)object); })) {
            [done addObject:name];
            [hooked addObject:name];
        }
    }
    gALBQJHooked = hooked.count ? [hooked componentsJoinedByString:@"、"] : @"无";
    // 有的类在后加载的库里：3 秒后再挂一次
    if (missing && !installed) {
        installed = YES;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ ALBQuickJumpInstall(); });
    }
}

#pragma mark - 打开

static NSArray<NSString *> *ALBQJRegisteredSchemes(void) {
    NSMutableArray<NSString *> *schemes = [NSMutableArray array];
    for (NSDictionary *type in NSBundle.mainBundle.infoDictionary[@"CFBundleURLTypes"] ?: @[]) {
        if (![type isKindOfClass:NSDictionary.class]) continue;
        for (NSString *s in type[@"CFBundleURLSchemes"] ?: @[]) if ([s isKindOfClass:NSString.class]) [schemes addObject:s.lowercaseString];
    }
    return schemes;
}

static BOOL ALBQJOpenURL(NSString *text, NSString **how) {
    NSURL *url = [NSURL URLWithString:text];
    if (!url.scheme.length) return NO;
    NSArray<NSString *> *schemes = ALBQJRegisteredSchemes();
    if (![schemes containsObject:url.scheme.lowercaseString] && [schemes containsObject:@"snssdk1128"]) {
        NSURLComponents *c = [NSURLComponents componentsWithURL:url resolvingAgainstBaseURL:NO];
        c.scheme = @"snssdk1128";
        if (c.URL) url = c.URL;
    }
    UIApplication *app = UIApplication.sharedApplication;
    id<UIApplicationDelegate> delegate = app.delegate;
    if ([delegate respondsToSelector:@selector(application:openURL:options:)]) {
        BOOL ok = NO;
        @try { ok = [delegate application:app openURL:url options:@{}]; } @catch (__unused NSException *e) { ok = NO; }
        if (ok) { if (how) *how = [NSString stringWithFormat:@"地址→应用代理（%@）", url.scheme]; return YES; }
    }
    [app openURL:url options:@{} completionHandler:nil];
    if (how) *how = [NSString stringWithFormat:@"地址→系统 openURL（%@）", url.scheme];
    return YES;
}

static UIViewController *ALBQJTopController(void) {
    UIWindow *window = AWEActiveMainWindow();
    return AWETopViewControllerFrom(window.rootViewController);
}

static BOOL ALBQJShowController(UIViewController *page, BOOL modal, NSString **how) {
    if (!page) return NO;
    UIViewController *top = ALBQJTopController();
    if (!top) return NO;
    UINavigationController *container = [page isKindOfClass:UINavigationController.class] ? (UINavigationController *)page : nil;
    UIViewController *inner = container ? container.viewControllers.firstObject : page;
    // 还在某个导航里（用户没退出来）：退回到它
    UINavigationController *owner = inner.navigationController;
    if (!container && owner && owner.view.window && [owner.viewControllers containsObject:inner]) {
        [owner popToViewController:inner animated:YES];
        if (how) *how = @"退回到还开着的那一页";
        return YES;
    }
    if (page.presentingViewController || page.viewIfLoaded.window) { if (how) *how = @"这一页已经开着"; return YES; }
    if (page.parentViewController) return NO;
    UINavigationController *nav = [top isKindOfClass:UINavigationController.class] ? (UINavigationController *)top : top.navigationController;
    if (!modal && nav && !container) {
        [nav pushViewController:page animated:YES];
        if (how) *how = @"推进当前导航";
        return YES;
    }
    UIViewController *shown = container ?: [[UINavigationController alloc] initWithRootViewController:page];
    if (!container) ((UINavigationController *)shown).navigationBarHidden = YES;
    shown.modalPresentationStyle = UIModalPresentationFullScreen;
    [top presentViewController:shown animated:YES completion:nil];
    if (how) *how = @"弹出";
    return YES;
}

BOOL ALBDockHostQuickJumpReady(NSInteger kind) {
    if (kind < 1 || kind > 4) return NO;
    NSDictionary *record = [NSUserDefaults.standardUserDefaults dictionaryForKey:ALBQJKey(kind)];
    if ([record[@"url"] length]) return YES;
    if (gALBQJKept[@(kind)]) return YES;
    return record && ALB5Bool(@"dock.jumpRecreate", NO);
}
NSString *ALBDockHostQuickJumpTitle(NSInteger kind) { return ALBQJTitles()[(NSUInteger)MIN(MAX(kind, 0), 4)]; }
NSString *ALBDockHostQuickJumpSymbol(NSInteger kind) { return ALBQJSymbols()[(NSUInteger)MIN(MAX(kind, 0), 4)]; }

BOOL ALBDockHostQuickJump(NSInteger kind) {
    if (kind < 1 || kind > 4) return NO;
    if (!gALBQJResult) gALBQJResult = [NSMutableDictionary dictionary];
    NSString *title = ALBQJTitles()[(NSUInteger)kind], *how = nil;
    NSDictionary *record = [NSUserDefaults.standardUserDefaults dictionaryForKey:ALBQJKey(kind)];
    BOOL modal = [record[@"modal"] boolValue];
    BOOL ok = NO;
    @try {
        if ([record[@"url"] length]) ok = ALBQJOpenURL(record[@"url"], &how);
        if (!ok && gALBQJKept[@(kind)]) ok = ALBQJShowController(gALBQJKept[@(kind)], modal, &how);
        if (!ok && record[@"class"] && ALB5Bool(@"dock.jumpRecreate", NO)) {
            NSString *made = nil;
            UIViewController *page = ALBQJMakePage(record, &made);
            ALBQJNoteRecreate(kind, @{@"at": @([NSDate date].timeIntervalSince1970), @"init": made ?: @"-",
                                      @"check": @"（还没看：1.5 秒内离开了这一页）"});
            if (page) {
                ok = ALBQJShowController(page, modal, &how);
                if (ok) {
                    how = [NSString stringWithFormat:@"新建（%@）→%@", made ?: @"-", how ?: @""];
                    // 1.5 秒后看一眼新页面有没有内容（诊断用：空白的话下一版换别的新建方式）
                    __weak UIViewController *weakPage = page;
                    NSInteger k = kind;
                    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                        UIViewController *live = weakPage;
                        if (!live.viewIfLoaded.window) return;
                        NSUInteger labels = 0, cells = 0;
                        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:live.view];
                        for (NSUInteger i = 0; i < queue.count && i < 800; i++) {
                            UIView *v = queue[i];
                            if (v.hidden || v.alpha < 0.01) continue;
                            if ([v isKindOfClass:UILabel.class] && ((UILabel *)v).text.length) labels++;
                            if ([v isKindOfClass:UICollectionViewCell.class] || [v isKindOfClass:UITableViewCell.class]) cells++;
                            [queue addObjectsFromArray:v.subviews];
                        }
                        NSString *check = [NSString stringWithFormat:@"1.5 秒后：字 %lu 个、格子 %lu 个%@；子页面 %lu 个", (unsigned long)labels,
                                           (unsigned long)cells, cells == 0 && labels < 4 ? @"，像是空白" : @"",
                                           (unsigned long)live.childViewControllers.count];
                        NSString *was = gALBQJResult[@(k)] ?: @"";
                        gALBQJResult[@(k)] = [was stringByAppendingFormat:@"（%@）", check];
                        ALBQJNoteRecreate(k, @{@"check": check});
                    });
                }
            }
        }
    } @catch (NSException *e) {
        ok = NO;
        how = [NSString stringWithFormat:@"出错：%@", e.reason ?: e.name];
    }
    gALBQJResult[@(kind)] = [NSString stringWithFormat:@"%@ %@", ok ? @"成功" : @"没打开", how ?: (record ? @"记到了这一页但没有地址，本次启动也还没打开过" : @"还没见过这一页")];
    if (!ok) AWEShowDownloadToast([NSString stringWithFormat:@"先自己打开一次「%@」，之后就能直接跳", title]);
    return ok;
}

/* 三段式底栏的 ✦ / ↑：悬浮面板（ALBFloatPanel.h；已开着就关） */
BOOL ALBDockHostOpenPalette(UIView *source) {
    return ALBFloatPanelOpen(source, 0);
}
BOOL ALBDockHostOpenQuickJumpPanel(UIView *source) {
    return ALBFloatPanelOpen(source, 1);
}

NSString *ALBQuickJumpReport(void) {
    NSMutableArray<NSString *> *lines = [NSMutableArray array];
    [lines addObject:[NSString stringWithFormat:@"钩上 %@；抖音注册的地址前缀 %@；新建试验=%@", gALBQJHooked ?: @"还没挂",
                      [ALBQJRegisteredSchemes() componentsJoinedByString:@"、"] ?: @"-", ALB5Bool(@"dock.jumpRecreate", NO) ? @"开" : @"关"]];
    for (NSInteger kind = 1; kind <= 4; kind++) {
        NSDictionary *r = [NSUserDefaults.standardUserDefaults dictionaryForKey:ALBQJKey(kind)];
        double age = r ? [NSDate date].timeIntervalSince1970 - [r[@"at"] doubleValue] : -1;
        [lines addObject:[NSString stringWithFormat:@"%@：本次见过 %lu 次%@；记到 %@；地址 %@；%@；导航 %@；弹出它的 %@；变量 %@；最近一次跳转 %@",
            ALBQJTitles()[(NSUInteger)kind], (unsigned long)gALBQJSeen[@(kind)].unsignedIntegerValue,
            gALBQJKept[@(kind)] ? @"（留着这一页）" : @"",
            r ? [NSString stringWithFormat:@"%@（%.0f 分钟前）", r[@"class"], age / 60.0] : @"没有",
            ALBQJShort(r[@"url"], 200), [r[@"modal"] boolValue] ? @"弹出来的" : @"推进来的", ALBQJShort(r[@"stack"], 200),
            r[@"presenting"] ?: @"-", ALBQJShort(r[@"described"], 600), gALBQJResult[@(kind)] ?: @"-"]];
    }
    // 这几页自己的初始化方法（下一版按它们新建）
    for (NSInteger kind = 1; kind <= 3; kind++) {
        Class cls = NSClassFromString(ALBQJClassNames()[(NSUInteger)kind]);
        if (!cls) continue;
        NSMutableArray<NSString *> *inits = [NSMutableArray array];
        for (int meta = 0; meta < 2; meta++) {
            unsigned int count = 0;
            Method *methods = class_copyMethodList(meta ? object_getClass(cls) : cls, &count);
            for (unsigned int i = 0; i < count && inits.count < 16; i++) {
                NSString *name = NSStringFromSelector(method_getName(methods[i]));
                NSString *low = name.lowercaseString;
                if ([low hasPrefix:@"init"] || (meta && ([low containsString:@"create"] || [low containsString:@"controller"] || [low hasPrefix:@"new"])))
                    [inits addObject:[NSString stringWithFormat:@"%@%@", meta ? @"+" : @"-", name]];
            }
            free(methods);
        }
        [lines addObject:[NSString stringWithFormat:@"%@ 的初始化：%@", ALBQJTitles()[(NSUInteger)kind], inits.count ? [inits componentsJoinedByString:@" "] : @"只有 init"]];
        // 3.11.51：新建那一次（重启后也留着）、记下的 context、这一类自己声明的属性（下一步新建要设哪些）
        NSDictionary *again = [NSUserDefaults.standardUserDefaults dictionaryForKey:ALBQJRecreateKey(kind)];
        double againAge = again ? [NSDate date].timeIntervalSince1970 - [again[@"at"] doubleValue] : -1;
        NSDictionary *r = [NSUserDefaults.standardUserDefaults dictionaryForKey:ALBQJKey(kind)];
        NSDictionary *context = [r[@"context"] isKindOfClass:NSDictionary.class] ? r[@"context"] : nil;
        NSMutableArray<NSString *> *props = [NSMutableArray array];
        unsigned int propCount = 0;
        objc_property_t *properties = class_copyPropertyList(cls, &propCount);
        for (unsigned int i = 0; i < propCount && props.count < 40; i++) {
            const char *attrs = property_getAttributes(properties[i]);
            NSString *attributes = attrs ? @(attrs) : @"";
            NSString *typeText = [attributes componentsSeparatedByString:@","].firstObject ?: @"";
            if ([typeText hasPrefix:@"T@\""] && typeText.length > 4) typeText = [typeText substringWithRange:NSMakeRange(3, typeText.length - 4)];
            else if (typeText.length > 1) typeText = [typeText substringFromIndex:1];
            BOOL readonly = [[attributes componentsSeparatedByString:@","] containsObject:@"R"];
            [props addObject:[NSString stringWithFormat:@"%s:%@%@", property_getName(properties[i]), typeText, readonly ? @"(只读)" : @""]];
        }
        free(properties);
        NSMutableArray<NSString *> *childNames = [NSMutableArray array];
        for (NSDictionary *child in [r[@"children"] isKindOfClass:NSArray.class] ? r[@"children"] : @[])
            if ([child isKindOfClass:NSDictionary.class]) [childNames addObject:[NSString stringWithFormat:@"%@=%@", child[@"name"], child[@"class"]]];
        [lines addObject:[NSString stringWithFormat:@"%@ 新建：%@；context：%@；子页面属性：%@；属性 %@",
            ALBQJTitles()[(NSUInteger)kind],
            again ? [NSString stringWithFormat:@"%.0f 分钟前 %@ %@", againAge / 60.0, again[@"init"] ?: @"-", again[@"check"] ?: @""] : @"还没新建过（重启后不先进这一页，直接用 ↑ 跳一次）",
            context ? [NSString stringWithFormat:@"%@=%@ %@", context[@"ivar"] ?: @"-", context[@"dict"] ? @"字典" : (context[@"class"] ?: @"-"),
                       ALBQJShort(context[@"described"] ?: context[@"note"], 300)] : @"没记到（进一次这一页再导出）",
            childNames.count ? [childNames componentsJoinedByString:@" "] : @"没记到",
            props.count ? [props componentsJoinedByString:@" "] : @"无"]];
    }
    [lines addObject:[NSString stringWithFormat:@"新建时抄回的配置变量 %lu 个", (unsigned long)gALBQJRestoredIvars]];
    [lines addObject:[NSString stringWithFormat:@"见过的 Lynx 页地址：%@", gALBQJLynxSeen.count ? [gALBQJLynxSeen componentsJoinedByString:@" ｜ "] : @"-"]];
    return [lines componentsJoinedByString:@"\n"];
}
