#!/usr/bin/env python3
"""Compile the four edited lens methods, with real iPhoneOS headers on Xcode.
On Linux use declaration stubs. Never claims UIKit rendering/event validation.
"""
from pathlib import Path
import json, subprocess, tempfile
from test_logos_boundary import resolve_compiler
from test_native_endpoint_syntax import DECLARATIONS
from check_logos_boundaries import mask_literals
R=Path(__file__).resolve().parents[1]
def method(text, signature):
    start=text.index(signature); brace=text.index('{',start)
    clean=mask_literals(text);depth=0
    for end in range(brace,len(clean)):
        depth+=(clean[end]=='{')-(clean[end]=='}')
        if not depth:return text[start:end+1]
    raise ValueError(signature)
EXTRA=r'''
typedef double CFTimeInterval;
CFTimeInterval CACurrentMediaTime(void);
@interface CALayer : NSObject
@property(nonatomic) float opacity;
@end
@interface UIView (LensDeclarations)
@property(nonatomic,readonly) UIView *superview;
@property(nonatomic,readonly) NSArray<UIView *> *subviews;
@property(nonatomic,readonly) CALayer *layer;
- (BOOL)isKindOfClass:(Class)cls;
@end
@interface UIControl : UIView @end
@interface UIImageView : UIView @end
@interface UILabel : UIView @end
@interface NSNumber : NSObject
+ (instancetype)numberWithFloat:(float)value;
@property(readonly) float floatValue;
@end
@interface NSArray (LensDeclarations)
- (id)objectAtIndexedSubscript:(NSUInteger)index;
@end
@interface NSMutableArray (LensDeclarations)
+ (instancetype)arrayWithObject:(id)object;
- (void)addObjectsFromArray:(NSArray *)objects;
@end
@interface NSString (LensDeclarations)
@property(readonly) const char *UTF8String;
@end
@interface NSMapTable<KeyType, ObjectType> : NSObject
+ (instancetype)weakToStrongObjectsMapTable;
@property(readonly) id<NSFastEnumeration> keyEnumerator;
- (ObjectType)objectForKey:(KeyType)key;
- (void)setObject:(ObjectType)object forKey:(KeyType)key;
- (void)removeAllObjects;
@end
'''
FIXTURE=r'''
@interface ALB23TopLensFixture : UIControl
@property(nonatomic) BOOL actionOnly;
@property(nonatomic) BOOL keepsSingleSelectionCapsule;
@property(nonatomic) BOOL isSelectionTracking;
@property(nonatomic,strong) UITabBar *nativeBar;
@property(nonatomic,strong) NSMapTable<UIView *,NSNumber *> *idleLensOpacities;
@property(nonatomic) CFTimeInterval nextIdleLensScan;
- (void)restoreIdleActionLenses;
- (void)updateIdleActionLensVisibility;
- (NSArray<NSString *> *)suppressedBackdropClassNames;
@end
'''
def main():
    cmd,sdk,mode=resolve_compiler()
    top=(R/'ALBTopNative/ALBTopNativeGroup.m').read_text()
    bodies='\n'.join(method(top,s) for s in [
        '- (NSArray<NSString *> *)suppressedBackdropClassNames {',
        '- (void)restoreIdleActionLenses {', '- (void)updateIdleActionLensVisibility {'])
    prefix=('#import <UIKit/UIKit.h>\n#import <QuartzCore/QuartzCore.h>\n#import <objc/runtime.h>\n'
        if mode=='iphoneos' else DECLARATIONS+EXTRA)
    text=prefix+'\n#include "ALBTopNative/ALB23LensPolicy.h"\n'+FIXTURE+'\n@implementation ALB23TopLensFixture\n'+bodies+'\n@end\n'
    results=[]
    with tempfile.TemporaryDirectory(prefix='alb23-lens-') as d:
        p=Path(d)/'lens.m';p.write_text(text)
        for arch in ('arm64','arm64e'):
            flags=['-x','objective-c','-fobjc-arc','-fobjc-exceptions','-fblocks','-fsyntax-only','-Wall','-Wextra','-Werror','-I',str(R)]
            flags+=['-target',f'{arch}-apple-ios15.0','-isysroot',sdk] if mode=='iphoneos' else ['-fobjc-runtime=macosx-10.15']
            c=subprocess.run(cmd+flags+[str(p)],capture_output=True,text=True,timeout=30)
            results.append({'arch_label':arch,'exit_code':c.returncode,'stderr':c.stderr,'passed':c.returncode==0})
    print(json.dumps({'scope':'edited lens methods only','compile_mode':mode,'apple_sdk_compile':mode=='iphoneos',
        'runtime_test':False,'results':results},indent=2))
    return 0 if all(x['passed'] for x in results) else 1
if __name__=='__main__':raise SystemExit(main())
