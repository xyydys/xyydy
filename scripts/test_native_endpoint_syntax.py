#!/usr/bin/env python3
"""Syntax-check production endpoint bridge; declarations-only off Apple hosts.

Uses the same explicit iPhoneOS toolchain selection as the TEST21 regression.
This does not invoke any UIKit selector or prove that a device supports a route.
"""
from pathlib import Path
import json, subprocess, tempfile
from test_logos_boundary import resolve_compiler
ROOT=Path(__file__).resolve().parents[1]
DECLARATIONS=r'''
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
typedef signed char BOOL;
typedef unsigned long NSUInteger;
typedef long NSInteger;
typedef struct { NSUInteger location, length; } NSRange;
#define YES ((BOOL)1)
#define NO ((BOOL)0)
#define nil ((id)0)
#define NSNotFound (~(NSUInteger)0)
#define __unused __attribute__((unused))
#define OBJC_ASSOCIATION_RETAIN_NONATOMIC 1
#define NSCaseInsensitiveSearch 1
@class NSString, NSMethodSignature, NSEnumerator;
@protocol NSFastEnumeration
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(id *)buffer count:(NSUInteger)len;
@end
__attribute__((objc_root_class)) @interface NSObject
+ (instancetype)new;
+ (Class)class;
- (Class)class;
- (BOOL)respondsToSelector:(SEL)selector;
- (NSMethodSignature *)methodSignatureForSelector:(SEL)selector;
- (id)copy;
@end
@interface NSString : NSObject
+ (instancetype)stringWithFormat:(NSString *)format, ...;
+ (instancetype)stringWithUTF8String:(const char *)text;
@property(readonly) NSString *lowercaseString;
- (NSRange)rangeOfString:(NSString *)pattern;
- (BOOL)isEqualToString:(NSString *)other;
- (NSRange)rangeOfString:(NSString *)pattern options:(NSUInteger)options;
@end
@interface NSArray<__covariant ObjectType> : NSObject <NSFastEnumeration>
@property(readonly) NSUInteger count;
+ (instancetype)arrayWithObjects:(const id [])objects count:(NSUInteger)count;
- (NSUInteger)indexOfObjectIdenticalTo:(id)object;
- (NSString *)componentsJoinedByString:(NSString *)separator;
@end
@interface NSMutableArray<ObjectType> : NSArray<ObjectType>
+ (instancetype)array;
+ (instancetype)arrayWithObjects:(id)first, ...;
- (void)addObject:(ObjectType)object;
@end
@interface NSHashTable<ObjectType> : NSObject
+ (instancetype)weakObjectsHashTable;
@property(readonly) ObjectType anyObject;
- (void)addObject:(ObjectType)object;
@end
@interface NSException : NSObject
@property(readonly) NSString *name;
@property(readonly) NSString *reason;
@end
@interface NSMethodSignature : NSObject
@property(readonly) NSUInteger numberOfArguments;
@property(readonly) const char *methodReturnType;
- (const char *)getArgumentTypeAtIndex:(NSUInteger)index;
@end
@interface NSInvocation : NSObject
+ (instancetype)invocationWithMethodSignature:(NSMethodSignature *)signature;
@property(strong) id target;
@property SEL selector;
- (void)setArgument:(void *)argument atIndex:(NSInteger)index;
- (void)invoke;
- (void)getReturnValue:(void *)value;
@end
@interface UIView : NSObject
@property(readonly) id window;
@end
@interface UITabBar : UIView @end
@interface UIViewController : NSObject
@property(readonly) UIView *viewIfLoaded;
- (BOOL)isKindOfClass:(Class)cls;
@end
@interface UITabBarController : UIViewController
@property(readonly) UITabBar *tabBar;
@end
@interface NSThread : NSObject
+ (BOOL)isMainThread;
@end
@interface UIDevice : NSObject
+ (instancetype)currentDevice;
@property(readonly) NSString *systemVersion;
@end
typedef struct objc_ivar *Ivar;
typedef struct objc_method *Method;
const char *class_getName(Class cls);
const char *class_getImageName(Class cls);
Ivar class_getInstanceVariable(Class cls,const char *name);
const char *ivar_getTypeEncoding(Ivar ivar);
const char *ivar_getName(Ivar ivar);
Ivar *class_copyIvarList(Class cls,unsigned *count);
id object_getIvar(id owner,Ivar ivar);
Class class_getSuperclass(Class cls);
Method *class_copyMethodList(Class cls,unsigned *count);
Class object_getClass(id object);
Method class_getInstanceMethod(Class cls,SEL selector);
unsigned method_getNumberOfArguments(Method method);
id objc_msgSend(id object,SEL selector,...);
SEL method_getName(Method method);
const char *method_getTypeEncoding(Method method);
id objc_getAssociatedObject(id owner,const void *key);
void objc_setAssociatedObject(id owner,const void *key,id value,int policy);
NSString *NSStringFromClass(Class cls);
NSString *NSStringFromSelector(SEL selector);
SEL NSSelectorFromString(NSString *name);
'''
EXTERNALS='''
BOOL AWEIsAlbumMorphSystemBar(UITabBar *bar);
void AWEAlbumMorphProbeWrite(NSString *format, ...);
'''
def main():
    cmd,sdk,mode=resolve_compiler()
    source=(ROOT/'Tweak.x').read_text()
    begin=source.index('static NSMutableArray<NSString *> *gALB23NativeAttemptNotes;')
    end=source.index('static void AWEApplyAlbumMorphOriginalSuppression(',begin)
    bridge=source[begin:end]
    endpoint=(ROOT/'ALB22NativeEndpoint.h').read_text()
    rows=[]
    with tempfile.TemporaryDirectory(prefix='alb-native-endpoint-') as temporary:
        fixture=Path(temporary)/'bridge.m'
        if mode=='iphoneos':
            prefix='#import <UIKit/UIKit.h>\n#import <objc/runtime.h>\n#import <objc/message.h>\n#include "ALB22NativeEndpoint.h"\n'
        else:
            endpoint='\n'.join(line for line in endpoint.splitlines() if not line.startswith(('#import','#pragma once')))
            prefix=DECLARATIONS+'\n'+endpoint+'\n'
        fixture.write_text(prefix+EXTERNALS+bridge)
        for arch in ('arm64','arm64e'):
            flags=['-x','objective-c','-fobjc-arc','-fobjc-exceptions','-fblocks','-fsyntax-only',
                   '-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-unused-variable','-I',str(ROOT)]
            if mode=='iphoneos': flags+=['-target',f'{arch}-apple-ios15.0','-isysroot',sdk]
            else: flags+=['-fobjc-runtime=macosx-10.15']
            result=subprocess.run(cmd+flags+[str(fixture)],capture_output=True,text=True,timeout=30)
            rows.append({'arch_label':arch,'exit_code':result.returncode,'passed':result.returncode==0,'stderr':result.stderr})
    print(json.dumps({'scope':'production endpoint header plus invocation/request/readback functions',
        'compile_mode':mode,'apple_sdk_compile':mode=='iphoneos','runtime_test':False,
        'note':'Non-Apple host passes use Foundation declarations, not an Apple ABI/platform compile.', 'results':rows},indent=2))
    return 0 if all(row['passed'] for row in rows) else 1
if __name__=='__main__': raise SystemExit(main())
