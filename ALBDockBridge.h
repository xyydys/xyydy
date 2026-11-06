//
//  ALBDockBridge.h —— 和抖音原生底栏（AWENormalModeTabBar）打交道的唯一出口
//
//  做法：切页不写 selectedIndex（会被抖音的 override 吞掉），
//  而是调用抖音自己的按钮点击回调，连同它自己的按钮和手势一起传过去，走和真实点按完全相同的路径。
//

#import <UIKit/UIKit.h>

#define ALBDockBridgeChangedNotification @"ALBDockNativeChanged"

FOUNDATION_EXPORT void ALBDockBridgeSetNativeBar(UIView *bar);
FOUNDATION_EXPORT UIView *ALBDockBridgeNativeBar(void);

// title：首页 / 朋友 / 消息 / 我；返回是否真的转发出去了
FOUNDATION_EXPORT BOOL ALBDockBridgeTapTitle(NSString *title);
FOUNDATION_EXPORT BOOL ALBDockBridgeTapPlus(void);

// 当前选中页的标题（首页 / 朋友 / 消息 / 我），取不到为 nil
FOUNDATION_EXPORT NSString *ALBDockBridgeSelectedTitle(void);

// 角标：nil = 没有；@"" = 纯红点；其他 = 文案（如 99+）
FOUNDATION_EXPORT NSString *ALBDockBridgeBadgeForTitle(NSString *title);

// 合并到下一轮主队列再通知（底栏布局、角标变化都会触发，一轮最多发一次）
FOUNDATION_EXPORT void ALBDockBridgeScheduleChanged(void);
