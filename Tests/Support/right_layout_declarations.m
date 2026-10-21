/* Declaration-only fixture, not Apple's SDK and not a runtime implementation. */
typedef signed char BOOL;
typedef unsigned long NSUInteger;
typedef long NSInteger;
typedef double CGFloat;
typedef long dispatch_once_t;
#define YES ((BOOL)1)
#define NO ((BOOL)0)
#define nil ((id)0)
#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))
#define CGFLOAT_MAX __DBL_MAX__
#define NSNotFound ((NSUInteger)-1)
#define OBJC_ASSOCIATION_RETAIN_NONATOMIC 1
void dispatch_once(dispatch_once_t *,void (^)(void));
typedef struct {CGFloat x,y;} CGPoint;
typedef struct {CGFloat width,height;} CGSize;
typedef struct {CGPoint origin;CGSize size;} CGRect;
typedef struct {CGFloat a,b,c,d,tx,ty;} CGAffineTransform;
typedef struct {NSUInteger location,length;} NSRange;
static const CGPoint CGPointZero={0,0};
CGPoint CGPointMake(CGFloat,CGFloat);
CGRect CGRectMake(CGFloat,CGFloat,CGFloat,CGFloat);
CGFloat CGRectGetWidth(CGRect);CGFloat CGRectGetHeight(CGRect);
CGFloat CGRectGetMinX(CGRect);CGFloat CGRectGetMinY(CGRect);
CGFloat CGRectGetMaxX(CGRect);CGFloat CGRectGetMaxY(CGRect);
CGFloat CGRectGetMidX(CGRect);CGFloat CGRectGetMidY(CGRect);
CGAffineTransform CGAffineTransformMake(CGFloat,CGFloat,CGFloat,CGFloat,CGFloat,CGFloat);
CGAffineTransform CGAffineTransformConcat(CGAffineTransform,CGAffineTransform);
CGAffineTransform CGAffineTransformInvert(CGAffineTransform);
@protocol NSCopying @end
@protocol NSFastEnumeration
- (NSUInteger)countByEnumeratingWithState:(void *)state objects:(__unsafe_unretained id *)buffer count:(NSUInteger)len;
@end
__attribute__((objc_root_class)) @interface NSObject
+ (id)new;+ (id)alloc;+ (Class)class;- (Class)class;
- (BOOL)isKindOfClass:(Class)c;
@end
@class NSArray,NSCharacterSet;
@interface NSString:NSObject<NSCopying>
@property(readonly) NSUInteger length;
- (NSString *)stringByAppendingString:(NSString *)s;
- (BOOL)isEqualToString:(NSString *)s;
- (BOOL)containsString:(NSString *)s;
- (NSRange)rangeOfCharacterFromSet:(NSCharacterSet *)s;
@end
@interface NSNumber:NSObject
+ (NSNumber *)numberWithBool:(BOOL)v;
+ (NSNumber *)numberWithUnsignedLong:(unsigned long)v;
+ (NSNumber *)numberWithUnsignedInteger:(NSUInteger)v;
+ (NSNumber *)numberWithDouble:(double)v;
- (NSUInteger)unsignedIntegerValue;- (BOOL)boolValue;- (double)doubleValue;
@end
@interface NSArray<__covariant ObjectType>:NSObject<NSFastEnumeration>
@property(readonly) NSUInteger count;
+ (instancetype)arrayWithObjects:(const id [])objects count:(NSUInteger)count;
- (ObjectType)objectAtIndexedSubscript:(NSUInteger)n;
- (BOOL)containsObject:(ObjectType)o;
- (NSUInteger)indexOfObject:(ObjectType)o;
@end
@interface NSMutableArray<ObjectType>:NSArray<ObjectType>
+ (instancetype)array;+ (instancetype)arrayWithObject:(ObjectType)o;
+ (instancetype)arrayWithArray:(NSArray<ObjectType> *)a;
- (void)addObject:(ObjectType)o;- (void)addObjectsFromArray:(NSArray<ObjectType> *)a;
@end
@interface NSDictionary<__covariant KeyType,__covariant ObjectType>:NSObject<NSFastEnumeration>
@property(readonly) NSArray<KeyType> *allKeys;
@property(readonly) NSArray<ObjectType> *allValues;
- (ObjectType)objectForKeyedSubscript:(KeyType)k;
@end
@interface NSMutableDictionary<KeyType,ObjectType>:NSDictionary<KeyType,ObjectType>
+ (instancetype)dictionary;
- (void)setObject:(ObjectType)o forKeyedSubscript:(KeyType)k;
- (void)removeObjectForKey:(KeyType)k;
@end
@interface NSMutableSet<ObjectType>:NSObject
+ (instancetype)set;- (void)addObject:(ObjectType)o;- (BOOL)containsObject:(ObjectType)o;
@end
@interface NSHashTable<ObjectType>:NSObject
+ (instancetype)weakObjectsHashTable;- (void)addObject:(ObjectType)o;
@property(readonly) NSArray<ObjectType> *allObjects;
@end
@interface NSCharacterSet:NSObject
+ (instancetype)decimalDigitCharacterSet;
@end
@interface NSAttributedString:NSObject
@property(readonly) NSString *string;
@end
@interface NSThread:NSObject
+ (BOOL)isMainThread;
@end
@interface UIApplication:NSObject
+ (instancetype)sharedApplication;
@property(readonly) NSInteger applicationState;
@end
enum {UIApplicationStateActive=0};
@interface CATransaction:NSObject
+ (void)begin;+ (void)commit;+ (void)setDisableActions:(BOOL)b;
@end
@interface CALayer:NSObject
@property CGPoint position,anchorPoint;
@property CGAffineTransform affineTransform;
@end
@interface UIView:NSObject
@property UIView *superview,*window;
@property(readonly) NSArray<UIView *> *subviews;
@property CALayer *layer;
@property CGAffineTransform transform;
@property CGRect bounds,frame;
@property CGPoint center;
@property BOOL hidden,userInteractionEnabled;
@property CGFloat alpha;
- (BOOL)isDescendantOfView:(UIView *)v;
- (CGPoint)convertPoint:(CGPoint)p toView:(UIView *)v;
- (CGRect)convertRect:(CGRect)p toView:(UIView *)v;
@end
@interface UIButton:UIView @end
@interface UIImageView:UIView @end
@interface UILabel:UIView
@property NSAttributedString *attributedText;
@property NSString *text;
@end
@interface AWEHomeActionSlotView:UIView
@property NSInteger actionKind;
@property UIView *originalView;
@property UIImageView *sourceImageView;
@property UIButton *systemGlassButton;
@end
@interface AWEHomeProfileGlassView:UIView
@property UIView *profileHost;
@property UIImageView *avatarImageView;
@property UIButton *menuButton;
@end
@interface ALBArtworkLease:NSObject
@property UIImageView *source;
@end
@interface ALBNativeActionBridge:NSObject
@property UIView *source;
@end
id objc_getAssociatedObject(id,const void *);
void objc_setAssociatedObject(id,const void *,id,int);
Class NSClassFromString(NSString *);
NSString *NSStringFromClass(Class);
BOOL ALB5Bool(NSString *,BOOL);
CGFloat ALB5Number(NSString *,CGFloat,CGFloat,CGFloat);
BOOL AWEPluginFeaturesEnabled(void);
BOOL ALBIsOwnedView(UIView *);
BOOL AWESameStyleStableAnchorClass(UIView *);
BOOL AWEViewHasSameStyleMarker(UIView *);
BOOL AWESameStyleIsRightActionStackV23(UIView *);
UIView *AWEFindViewByExactRuntimeClass(UIView *,NSString *,NSUInteger);
UIView *AWEFindCurrentRightActionStackAcrossWindows(void);
static BOOL gAWEStableCleanScreenEnabled;
static char kALBArtworkLeaseKey,kALBNativeActionBridgeKey,kAWEHomeActionStackSlotsKey;
static char kAWEProfileGlassSlotKey,kAWESameStyleStackPlateKey;
UIImageView *ALBMusicArtwork(UIView *root);
#include "../../ALB5RightLayout.h"
