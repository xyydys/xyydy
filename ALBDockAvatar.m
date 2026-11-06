#import "ALBDockAvatar.h"
#import "ALBDockCommon.h"
#import <PhotosUI/PhotosUI.h>
#import <objc/message.h>

static NSString * const kSrcKey = @"AwemeLiquidBar.dock.avatarSource";   // 0 无 1 页面抓取 2 账号 3 手动
static UIImage *gImage;
static BOOL gLoaded;
static BOOL gFetching;

static NSString *ALBDockAvatarPath(void) {
    NSString *dir = [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES).firstObject
                     stringByAppendingPathComponent:@"AwemeLiquidBar"];
    [[NSFileManager defaultManager] createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    return [dir stringByAppendingPathComponent:@"dock-avatar.png"];
}

static id ALBDockSafeGet(id obj, NSString *key) {
    if (!obj) { return nil; }
    SEL s = NSSelectorFromString(key);
    if (![obj respondsToSelector:s]) { return nil; }
    @try { return [obj valueForKey:key]; } @catch (__unused NSException *e) { return nil; }
}

static NSString *ALBDockURLFromValue(id v) {
    if ([v isKindOfClass:[NSString class]] && [(NSString *)v hasPrefix:@"http"]) { return v; }
    if ([v isKindOfClass:[NSURL class]]) { return [(NSURL *)v absoluteString]; }
    for (NSString *k in @[@"originURLList", @"URLList", @"urlList", @"url_list"]) {
        id list = ALBDockSafeGet(v, k);
        if ([list isKindOfClass:[NSArray class]]) {
            for (id u in (NSArray *)list) {
                if ([u isKindOfClass:[NSString class]] && [(NSString *)u hasPrefix:@"http"]) { return u; }
            }
        }
    }
    return nil;
}

static NSString *ALBDockAvatarURLFromUser(id user) {
    for (NSString *k in @[@"avatarLarger", @"avatarMedium", @"avatar300x300", @"avatarThumb",
                          @"avatarLargeURL", @"avatarURL", @"avatarUrl", @"avatar"]) {
        NSString *u = ALBDockURLFromValue(ALBDockSafeGet(user, k));
        if (u) { return u; }
    }
    return nil;
}

@implementation ALBDockAvatar

+ (UIImage *)image {
    if (!gLoaded) {
        gLoaded = YES;
        gImage = [UIImage imageWithContentsOfFile:ALBDockAvatarPath()];
    }
    // 3.11.40：用户：“三段式底栏读取头像一直是错误的，插件设置本来就能读取头像”。以前“页面抓取 / 账号单例”抓到的
    // 图存进缓存后排在设置页那张前面，抓错了（别人的主页、别的账号对象）就一直错。现在：手动选的最优先；其次是本插件
    // 已经读到的本人头像（设置页个人卡片那张，每次现取，晚到或换账号会通知球刷新）；两样都没有才用抓来的缓存
    NSInteger source = [[NSUserDefaults standardUserDefaults] integerForKey:kSrcKey];
    if (gImage && source == 3) { return gImage; }
    UIImage *own = ALBDockHostOwnAvatar();
    return own ?: gImage;
}

+ (void)storeImage:(UIImage *)img source:(NSInteger)src {
    if (!img) { return; }
    NSInteger cur = [[NSUserDefaults standardUserDefaults] integerForKey:kSrcKey];
    [self image];   // 先读一次缓存文件
    if (gImage && src < cur) { return; }   // 不用低优先级来源覆盖高优先级
    gImage = img;
    gLoaded = YES;
    [UIImagePNGRepresentation(img) writeToFile:ALBDockAvatarPath() atomically:YES];
    [[NSUserDefaults standardUserDefaults] setInteger:src forKey:kSrcKey];
    dispatch_async(dispatch_get_main_queue(), ^{
        [[NSNotificationCenter defaultCenter] postNotificationName:ALBDockAvatarChangedNotification object:nil];
    });
}

+ (void)setImage:(UIImage *)image { [self storeImage:image source:3]; }

+ (void)presentPickerFromViewController:(UIViewController *)controller {
    if (!controller) { return; }
    PHPickerConfiguration *cfg = [[PHPickerConfiguration alloc] init];
    cfg.filter = [PHPickerFilter imagesFilter];
    cfg.selectionLimit = 1;
    PHPickerViewController *picker = [[PHPickerViewController alloc] initWithConfiguration:cfg];
    picker.delegate = (id<PHPickerViewControllerDelegate>)self;
    [controller presentViewController:picker animated:YES completion:nil];
}

+ (void)picker:(PHPickerViewController *)picker didFinishPicking:(NSArray<PHPickerResult *> *)results {
    [picker dismissViewControllerAnimated:YES completion:nil];
    NSItemProvider *ip = results.firstObject.itemProvider;
    if (![ip canLoadObjectOfClass:[UIImage class]]) { return; }
    [ip loadObjectOfClass:[UIImage class] completionHandler:^(id<NSItemProviderReading> obj, NSError *err) {
        if (![(NSObject *)obj isKindOfClass:[UIImage class]]) { return; }
        UIImage *img = (UIImage *)obj;
        // 缩到 240px 方图，省内存
        CGFloat side = MIN(img.size.width, img.size.height);
        if (side < 1) { return; }
        CGRect crop = CGRectMake((img.size.width - side) / 2, (img.size.height - side) / 2, side, side);
        UIGraphicsImageRenderer *r = [[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(240, 240)];
        UIImage *out = [r imageWithActions:^(UIGraphicsImageRendererContext *ctx) {
            CGFloat k = 240.0 / side;
            [img drawInRect:CGRectMake(-crop.origin.x * k, -crop.origin.y * k, img.size.width * k, img.size.height * k)];
        }];
        dispatch_async(dispatch_get_main_queue(), ^{ [ALBDockAvatar setImage:out]; });
    }];
}

+ (void)clear {
    gImage = nil;
    [[NSFileManager defaultManager] removeItemAtPath:ALBDockAvatarPath() error:nil];
    [[NSUserDefaults standardUserDefaults] setInteger:0 forKey:kSrcKey];
    [[NSNotificationCenter defaultCenter] postNotificationName:ALBDockAvatarChangedNotification object:nil];
    [self refreshFromAccount];
}

#pragma mark 路径 1：账号单例

+ (NSString *)probeAccountURL {
    NSArray *classes = @[@"AWEUserService", @"AWEUserManager", @"AWEAccountManager", @"AWEProfileUserService",
                         @"AWEUserModelManager", @"AWEIMUserService", @"TTAccount", @"BDAccount",
                         @"AWEPassportService", @"IESAccountService", @"AWECurrentUserManager"];
    NSArray *singletons = @[@"sharedInstance", @"sharedManager", @"sharedAccount", @"defaultService",
                            @"sharedService", @"defaultManager", @"shared", @"currentManager"];
    NSArray *userKeys = @[@"currentUser", @"curUser", @"currentUserModel", @"loginUser", @"user", @"userModel"];
    for (NSString *cn in classes) {
        Class c = NSClassFromString(cn);
        if (!c) { continue; }
        for (NSString *sn in singletons) {
            SEL s = NSSelectorFromString(sn);
            if (![c respondsToSelector:s]) { continue; }
            id inst = nil;
            @try { inst = ((id (*)(id, SEL))objc_msgSend)(c, s); } @catch (__unused NSException *e) { inst = nil; }
            if (!inst) { continue; }
            NSString *u = ALBDockAvatarURLFromUser(inst);
            if (u) { return u; }
            for (NSString *uk in userKeys) {
                u = ALBDockAvatarURLFromUser(ALBDockSafeGet(inst, uk));
                if (u) { return u; }
            }
        }
    }
    return nil;
}

+ (void)refreshFromAccount {
    if (gFetching) { return; }
    if (ALBDockHostOwnAvatar()) { return; }     // 3.11.40：设置页那张有了就不再去猜账号对象
    NSString *url = [self probeAccountURL];
    if (!url) { return; }
    NSURL *u = [NSURL URLWithString:url];
    if (!u) { return; }
    gFetching = YES;
    [[[NSURLSession sharedSession] dataTaskWithURL:u completionHandler:^(NSData *data, NSURLResponse *r, NSError *err) {
        UIImage *img = data ? [UIImage imageWithData:data] : nil;
        // 3.11：回主线程再写（gImage 只在主线程读写）
        dispatch_async(dispatch_get_main_queue(), ^{
            gFetching = NO;
            if (img) { [self storeImage:img source:2]; }
        });
    }] resume];
}

#pragma mark 路径 2：自己「我」页面上的头像视图

static UIViewController *ALBDockOwningVC(UIView *v) {
    UIResponder *r = v;
    while (r) {
        r = r.nextResponder;
        if ([r isKindOfClass:[UIViewController class]]) { return (UIViewController *)r; }
    }
    return nil;
}

static BOOL ALBDockNameHasAvatar(UIView *v) {
    UIView *p = v;
    for (int i = 0; i < 4 && p; i++, p = p.superview) {
        if ([NSStringFromClass(p.class) rangeOfString:@"Avatar" options:NSCaseInsensitiveSearch].location != NSNotFound) { return YES; }
    }
    return NO;
}

static UIImageView *ALBDockFindAvatarView(UIView *root, int depth) {
    if (depth > 40 || root.hidden || root.alpha < 0.05) { return nil; }
    if ([root isKindOfClass:[UIImageView class]]) {
        UIImageView *iv = (UIImageView *)root;
        CGSize s = iv.bounds.size;
        if (iv.image && s.width >= 56 && fabs(s.width - s.height) < 2 && ALBDockNameHasAvatar(iv)) {
            NSString *vc = NSStringFromClass(ALBDockOwningVC(iv).class);
            if ([vc rangeOfString:@"Profile"].location != NSNotFound ||
                [vc rangeOfString:@"UserHome"].location != NSNotFound ||
                [vc rangeOfString:@"Mine"].location != NSNotFound) {
                return iv;
            }
        }
    }
    for (UIView *sub in root.subviews) {
        UIImageView *f = ALBDockFindAvatarView(sub, depth + 1);
        if (f) { return f; }
    }
    return nil;
}

+ (void)scanWindow:(UIWindow *)window nativeBarVisible:(BOOL)barVisible {
    // 原生底栏可见 = 停在一级 tab 页；这时看到的个人主页只可能是自己的
    if (!window || !barVisible) { return; }
    if ([self image] || ALBDockHostOwnAvatar()) { return; }   // 3.11.40：设置页那张有了就不再抓页面
    UIImageView *iv = ALBDockFindAvatarView(window, 0);
    if (iv.image) { [self storeImage:iv.image source:1]; }
}

@end
