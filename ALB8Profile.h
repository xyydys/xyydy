#pragma once
/* Read only the signed-in account service. Never infer the viewer from an
   author's avatar in the playing feed. Private selectors are signature checked. */
#ifdef __cplusplus
extern "C" {
#endif
void ALB5SetOwnAvatar(UIImage *image);
UIImage *ALB8OwnAvatar(void);
void ALB8SetOwnIdentity(NSString *uid);
#ifdef __cplusplus
}
#endif
static NSString * const ALB8ProfileDidChange = @"AwemeLiquidBar.OwnProfileChanged";
static NSString *ALB8ProfileIdentity, *ALB8ProfilePhotoURL;
static NSURLSessionDataTask *ALB8ProfilePhotoTask;
static CFTimeInterval ALB8ProfileLastAttempt;
static id ALB8ProfileRead(id object, NSString *name) {
    if (!object) return nil;
    @try {
        SEL selector=NSSelectorFromString(name);
        NSMethodSignature *signature=[object methodSignatureForSelector:selector];
        if (![object respondsToSelector:selector] || signature.numberOfArguments!=2 || signature.methodReturnType[0]!='@') return nil;
        return ((id(*)(id,SEL))objc_msgSend)(object,selector);
    } @catch (__unused NSException *exception) { return nil; }
}
static id ALB8ProfileFirst(id object, NSArray<NSString *> *keys) {
    for (NSString *key in keys) {
        id value=[object isKindOfClass:NSDictionary.class] ? object[key] : ALB8ProfileRead(object,key);
        if(value && value!=NSNull.null)return value;
    }
    return nil;
}
static NSString *ALB8ProfileString(id object) {
    if([object isKindOfClass:NSString.class])return object;
    if([object isKindOfClass:NSNumber.class])return [object stringValue];
    return nil;
}
static NSString *ALB8ProfileURL(id value, NSUInteger depth) {
    if (!value || depth>4)return nil;
    NSString *string=[value isKindOfClass:NSURL.class] ? [value absoluteString] : ALB8ProfileString(value);
    if ([string hasPrefix:@"https://"] || [string hasPrefix:@"http://"]) return string;
    if([value isKindOfClass:NSArray.class])for(id item in value) { NSString *url=ALB8ProfileURL(item,depth+1);if(url)return url; }
    for(NSString *key in @[@"originURLList",@"urlList",@"URLList",@"url_list",@"url",@"URL"]) {
        id child=ALB8ProfileFirst(value,@[key]);if(child && child!=value) { NSString *url=ALB8ProfileURL(child,depth+1);if(url)return url; }
    }
    return nil;
}
static NSString *ALB8OwnName(void) {
    return [NSUserDefaults.standardUserDefaults stringForKey:ALB5Key(@"profile.ownName")] ?: @"我的抖音";
}
static void ALB8AcceptOwnProfile(id user) {
    NSString *identity=ALB8ProfileString(ALB8ProfileFirst(user,@[@"uid",@"userID",@"userId",@"secUID",@"secUid"]));
    if(!identity.length)return;
    NSString *previous=ALB8ProfileIdentity ?: [NSUserDefaults.standardUserDefaults stringForKey:ALB5Key(@"profile.ownID")];
    BOOL changed=previous.length && ![previous isEqual:identity];
    if(changed) { [ALB8ProfilePhotoTask cancel]; ALB8ProfilePhotoTask=nil; ALB8ProfilePhotoURL=nil; }
    ALB8SetOwnIdentity(identity);
    ALB8ProfileIdentity=identity;
    [NSUserDefaults.standardUserDefaults setObject:identity forKey:ALB5Key(@"profile.ownID")];
    NSString *name=ALB8ProfileString(ALB8ProfileFirst(user,@[@"nickname",@"nickName",@"name"]));
    if(name.length)[NSUserDefaults.standardUserDefaults setObject:name forKey:ALB5Key(@"profile.ownName")];
    else if(changed)[NSUserDefaults.standardUserDefaults removeObjectForKey:ALB5Key(@"profile.ownName")];
    id photo=ALB8ProfileFirst(user,@[@"avatarMedium",@"avatarLarger",@"avatarThumb",@"avatar",@"avatar_medium",@"avatar_thumb"]);
    if([photo isKindOfClass:UIImage.class]) { ALB5SetOwnAvatar(photo); }
    NSString *url=ALB8ProfileURL(photo,0);
    [NSNotificationCenter.defaultCenter postNotificationName:ALB8ProfileDidChange object:nil];
    if(!url.length || ([url isEqual:ALB8ProfilePhotoURL] && (ALB8ProfilePhotoTask || ALB8OwnAvatar())))return;
    [ALB8ProfilePhotoTask cancel];ALB8ProfilePhotoURL=url;
    NSMutableURLRequest *request=[NSMutableURLRequest requestWithURL:[NSURL URLWithString:url]
        cachePolicy:NSURLRequestReturnCacheDataElseLoad timeoutInterval:15];
    ALB8ProfilePhotoTask=[NSURLSession.sharedSession dataTaskWithRequest:request completionHandler:^(NSData *data,NSURLResponse *response,NSError *error) {
        NSInteger status=[response isKindOfClass:NSHTTPURLResponse.class]?[(NSHTTPURLResponse *)response statusCode]:0;
        UIImage *image=(!error && status>=200 && status<300 && data.length<=8*1024*1024)?[UIImage imageWithData:data]:nil;
        dispatch_async(dispatch_get_main_queue(), ^{
            if(![identity isEqual:ALB8ProfileIdentity] || ![url isEqual:ALB8ProfilePhotoURL])return;
            ALB8ProfilePhotoTask=nil;
            if(image) { ALB5SetOwnAvatar(image);[NSNotificationCenter.defaultCenter postNotificationName:ALB8ProfileDidChange object:nil]; }
            else ALB8ProfilePhotoURL=nil;
        });
    }];
    [ALB8ProfilePhotoTask resume];
}
static void ALB8RefreshOwnProfile(void) {
    if(!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{ALB8RefreshOwnProfile();});return; }
    if(CACurrentMediaTime()-ALB8ProfileLastAttempt<2)return;
    ALB8ProfileLastAttempt=CACurrentMediaTime();
    for(NSString *className in @[@"AWEUserService",@"ACCUserService",@"AWEAccountService",@"IESAccountService",@"AWEUserManager"]) {
        Class cls=NSClassFromString(className);if(!cls)continue;
        NSMutableArray *services=[NSMutableArray arrayWithObject:cls];
        for(NSString *getter in @[@"sharedService",@"sharedInstance",@"sharedManager",@"defaultService"]) {
            id service=ALB8ProfileRead(cls,getter);if(service)[services addObject:service];
        }
        for(id service in services) {
            id user=ALB8ProfileFirst(service,@[@"currentUser",@"currentUserModel",@"currentLoginUser",@"loginUser",@"loggedInUser"]);
            if(ALB8ProfileString(ALB8ProfileFirst(user,@[@"uid",@"userID",@"userId",@"secUID",@"secUid"])).length) { ALB8AcceptOwnProfile(user);return; }
        }
    }
}
/* Own-tab-only fallback when a host build exposes its model on its controller
   rather than an account singleton. Caller supplies the verified Me tab. */
static BOOL ALB8CaptureOwnProfileController(UIViewController *root) {
    NSMutableArray *queue=[NSMutableArray array];if(root)[queue addObject:root];
    for(NSUInteger i=0;i<queue.count && i<32;i++) {
        UIViewController *controller=queue[i];
        NSString *name=NSStringFromClass(controller.class).lowercaseString;
        if(([name containsString:@"profile"] || [name containsString:@"userdetail"] || [name containsString:@"mine"]) && ![name containsString:@"otheruser"]) {
            id user=ALB8ProfileFirst(controller,@[@"userModel",@"currentUser",@"user"]);
            if(ALB8ProfileString(ALB8ProfileFirst(user,@[@"uid",@"userID",@"userId",@"secUID",@"secUid"])).length) { ALB8AcceptOwnProfile(user);return YES; }
        }
        [queue addObjectsFromArray:controller.childViewControllers];
    }
    return NO;
}
static void ALB8StartOwnProfileUpdates(void) {
    ALB8RefreshOwnProfile();
    for(NSNumber *delay in @[@3,@8,@20])dispatch_after(dispatch_time(DISPATCH_TIME_NOW,(int64_t)(delay.doubleValue*NSEC_PER_SEC)),dispatch_get_main_queue(),^{ALB8RefreshOwnProfile();});
    [NSNotificationCenter.defaultCenter addObserverForName:UIApplicationDidBecomeActiveNotification object:nil queue:NSOperationQueue.mainQueue usingBlock:^(__unused NSNotification *note) { ALB8RefreshOwnProfile(); }];
}
