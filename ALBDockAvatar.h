//
//  ALBDockAvatar.h —— 三段式收缩底栏：收缩后球上的「本人账号」头像
//
//  来源，先到先用，拿到就缓存到 Library/Caches/AwemeLiquidBar/dock-avatar.png：
//    0. 本插件自己已经读到的本人头像（设置页个人卡片那张）
//    1. 运行时探测抖音账号单例（currentUser → avatarThumb.originURLList）
//    2. 停在自己的「我」页（原生底栏可见 = 根页面）时，从头像视图上抓图
//    3. 设置里手动从相册选
//

#import <UIKit/UIKit.h>

#define ALBDockAvatarChangedNotification @"ALBDockAvatarChanged"

@interface ALBDockAvatar : NSObject
+ (UIImage *)image;                         // 可能为 nil
+ (void)setImage:(UIImage *)image;          // 写缓存并广播
+ (void)refreshFromAccount;                 // 路径 1
+ (void)scanWindow:(UIWindow *)window nativeBarVisible:(BOOL)barVisible;   // 路径 2
+ (void)clear;
+ (void)presentPickerFromViewController:(UIViewController *)controller;   // 路径 3：从相册选
@end
