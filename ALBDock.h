//
//  ALBDock.h —— 抖音三段式液态玻璃底栏
//
//  布局（与录屏一致）：
//    单独键在右： [ 首页 消息 拍摄 我 ] [ ✦ | ↑ ] ( 👥 )
//    单独键在左： ( 👥 ) [ ✦ | ↑ ] [ 我 消息 拍摄 首页 ]
//
//  触发：
//    静置 N 秒        → 三段式收缩成圆键（停在中间），点圆键展开
//    屏幕左半边上滑    → 收缩一次，再展开成「单独键在左」
//    屏幕右半边上滑    → 收缩一次，再展开成「单独键在右」
//

#import <UIKit/UIKit.h>

typedef NS_ENUM(NSInteger, ALBDockSide) {
    ALBDockSideRight = 0,
    ALBDockSideLeft  = 1,
};

@interface ALBDockView : UIView

@property (nonatomic, assign, readonly) ALBDockSide side;
@property (nonatomic, assign, readonly) BOOL collapsed;
@property (nonatomic, assign, readonly) BOOL transitioning;
@property (nonatomic, assign) NSTimeInterval idleDelay;   // 默认 5s，<=0 关闭

- (void)noteInteraction;
- (void)swipeUpFromLeft:(BOOL)left;
- (void)collapseWithCompletion:(void (^)(void))done;
- (BOOL)containsDockPoint:(CGPoint)pointInSelf;
// 2.6.0 探测用：底栏当前几何 + 系统底栏内部结构（只读）
- (NSString *)debugReport;
// 3.11.1 取样用（只读）：三块玻璃此刻的范围（窗口坐标），看不见时 CGRectNull
- (CGRect)backdropRectInWindow;
// 3.11.44：同步定时器每次核对一下系统胶囊的玻璃还是不是插件装的那只（UIKit 重建胶囊时会换回原厂的），被换掉就再装
- (void)verifyNativePlatterGlass;

@end
