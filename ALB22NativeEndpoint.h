#pragma once
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#include "ALB22NativeMethodPolicy.h"
#include <stdlib.h>

// TEST22 probes only endpoints already OWNED by this plugin's resident native
// bar. No global view walk, new controllers, forced layout or scroll rebinding.
static char kALB22NativeEndpointKey;
static NSString *gALB22NativeRequestStatus = @"尚无收缩请求";
static NSString *gALB22LastUnsupportedClasses;
static NSString *gALB22LastUnsupportedDescription;

static BOOL ALB22IsUIKitBarEndpoint(id object) {
    if (!object || !ALB22NativeBarClassName(class_getName([object class]))) return NO;
    const char *image = class_getImageName([object class]);
    return image && (strstr(image, "/UIKitCore.framework/") || strstr(image, "/UIKit.framework/"));
}
static id ALB22ExistingObjectIvar(id owner, const char *name) {
    if (!owner) return nil;
    Ivar ivar = class_getInstanceVariable([owner class], name);
    if (!ivar || !ALB22ObjectType(ivar_getTypeEncoding(ivar))) return nil;
    // Reading a known object ivar avoids KVC fallback and lazy provider creation.
    @try { return object_getIvar(owner, ivar); }
    @catch (__unused NSException *exception) { return nil; }
}
static void ALB22AppendOwnedEndpoint(NSMutableArray *targets, id candidate) {
    if (targets.count >= 8 || !ALB22IsUIKitBarEndpoint(candidate) ||
        [targets indexOfObjectIdenticalTo:candidate] != NSNotFound) return;
    [targets addObject:candidate];
}
static void ALB22AppendOwnedIvarEndpoints(NSMutableArray *targets, id root) {
    // Ivar spelling also changes across SDKs. Inspect names/types on this
    // existing owner only, then accept just UIKit tab/minimization objects.
    NSUInteger inspected = 0;
    Class cls = [root class];
    for (NSUInteger depth = 0; cls && depth < 4 && inspected < 64 && targets.count < 8;
         cls = class_getSuperclass(cls), ++depth) {
        unsigned count = 0;
        Ivar *ivars = class_copyIvarList(cls, &count);
        for (unsigned i = 0; i < count && inspected < 64 && targets.count < 8; ++i, ++inspected) {
            const char *rawName = ivar_getName(ivars[i]);
            if (!rawName || !ALB22ObjectType(ivar_getTypeEncoding(ivars[i]))) continue;
            NSString *name = [NSString stringWithUTF8String:rawName].lowercaseString;
            BOOL related = [name rangeOfString:@"minimiz"].location != NSNotFound ||
                ([name rangeOfString:@"provider"].location != NSNotFound &&
                 [name rangeOfString:@"tabbar"].location != NSNotFound);
            if (!related) continue;
            @try { ALB22AppendOwnedEndpoint(targets, object_getIvar(root, ivars[i])); }
            @catch (__unused NSException *exception) {}
        }
        free(ivars);
    }
}
static NSArray *ALB22OwnedMinimizeTargets(UITabBarController *owner) {
    if (!owner) return @[];
    NSMutableArray *targets = [NSMutableArray arrayWithObjects:owner, owner.tabBar, nil];
    // Modern providers implement some selectors themselves rather than
    // forwarding them through UITabBarController or UITabBar.
    for (id root in @[owner, owner.tabBar]) {
        ALB22AppendOwnedEndpoint(targets, ALB22ExistingObjectIvar(root, "_visualProvider"));
        ALB22AppendOwnedEndpoint(targets, ALB22ExistingObjectIvar(root, "_tabBarVisualProvider"));
    }
    NSArray *providerRoots = [targets copy];
    for (id root in providerRoots) {
        ALB22AppendOwnedIvarEndpoints(targets, root);
        ALB22AppendOwnedEndpoint(targets, ALB22ExistingObjectIvar(root, "_minimizationController"));
        ALB22AppendOwnedEndpoint(targets, ALB22ExistingObjectIvar(root, "_minimizeController"));
        ALB22AppendOwnedEndpoint(targets, ALB22ExistingObjectIvar(root, "_minimizationCoordinator"));
    }
    return targets;
}
static void ALB22RememberNativeEndpoint(UITabBarController *owner, id target, NSString *selector, BOOL compact) {
    NSHashTable *weakTarget = [NSHashTable weakObjectsHashTable];
    [weakTarget addObject:target];
    objc_setAssociatedObject(owner, &kALB22NativeEndpointKey, weakTarget, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    gALB22NativeRequestStatus = [NSString stringWithFormat:@"请求=%@；入口=%@ %@；已派发，实际状态待原生回读",
        compact ? @"收缩" : @"展开", NSStringFromClass([target class]), selector];
}
static NSArray *ALB22NativeReadbackTargets(UITabBarController *owner) {
    NSHashTable *weakTarget = objc_getAssociatedObject(owner, &kALB22NativeEndpointKey);
    id target = weakTarget.anyObject;
    NSMutableArray *targets = [NSMutableArray array];
    if (target) [targets addObject:target];
    if ([targets indexOfObjectIdenticalTo:owner] == NSNotFound) [targets addObject:owner];
    if ([targets indexOfObjectIdenticalTo:owner.tabBar] == NSNotFound) [targets addObject:owner.tabBar];
    return targets;
}
static NSString *ALB22DescribeMinimizeCapabilities(NSArray *targets) {
    NSMutableArray<NSString *> *classes = [NSMutableArray array];
    for (id target in targets) [classes addObject:NSStringFromClass([target class])];
    NSString *key = [classes componentsJoinedByString:@","];
    if ([gALB22LastUnsupportedClasses isEqualToString:key]) return gALB22LastUnsupportedDescription;
    NSMutableArray<NSString *> *lines = [NSMutableArray array];
    for (id target in targets) {
        NSUInteger emitted = 0;
        Class cls = [target class];
        for (NSUInteger depth = 0; cls && depth < 4 && emitted < 12; cls = class_getSuperclass(cls), ++depth) {
            unsigned count = 0;
            Method *methods = class_copyMethodList(cls, &count);
            for (unsigned i = 0; i < count && emitted < 12; ++i) {
                NSString *name = NSStringFromSelector(method_getName(methods[i]));
                if ([name rangeOfString:@"minimiz" options:NSCaseInsensitiveSearch].location == NSNotFound) continue;
                const char *types = method_getTypeEncoding(methods[i]);
                [lines addObject:[NSString stringWithFormat:@"%@ %@ [%s]", NSStringFromClass(cls), name, types ?: "?"]];
                emitted++;
            }
            free(methods);
        }
    }
    gALB22LastUnsupportedClasses = key;
    gALB22LastUnsupportedDescription = [NSString stringWithFormat:@"对象=%@；候选方法（仅读取，未调用）=%@",
        key, lines.count ? [lines componentsJoinedByString:@" | "] : @"没有发现"];
    return gALB22LastUnsupportedDescription;
}
