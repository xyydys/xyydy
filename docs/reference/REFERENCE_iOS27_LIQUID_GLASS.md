# iOS 27 玻璃材质移植参考

核心桥在 `AGLGlassMaterial.h`，只依赖 UIKit 与 Objective-C runtime，适用于旧 Theos SDK 构建。

```objc
#import "AGLGlassMaterial.h"
UIVisualEffectView *glassView = [[UIVisualEffectView alloc] initWithEffect:nil];
glassView.effect = AGLMakeSystemGlass(AGLGlassStyleRegular, nil, YES);
// 普通玻璃 + 显式色调
UIColor *tint = [UIColor colorWithRed:0.12 green:0.55 blue:1.0 alpha:0.35];
glassView.effect = AGLMakeSystemGlass(AGLGlassStyleRegular, tint, YES);
// 纯 Clear
glassView.effect = AGLMakeSystemGlass(AGLGlassStyleClear, nil, YES);
```

参数样式为 Regular=0、Clear=1。保持 visual effect view 的 alpha 为 1；不覆盖白色或黑色半透明背景来模拟系统透明度。普通玻璃以 nil tint 作为系统设置验证基准。显式 tint 的 alpha 只代表色调强度，不能当作系统透明度值。

正式 UI 若要继承父视图外观，用 overrideUserInterfaceStyle=Unspecified；该值会继承父视图，父视图可能受 App 自己的深浅色设置影响。本实验的“跟随”采用 UIWindow.screen.traitCollection.userInterfaceStyle 作为来源，只覆盖实验面板，同时记录 screen / host / view，必须在目标设备用系统外观切换验证。

返回前台时重新建立相同 effect，用于观察系统 Liquid Glass 设置是否影响它。系统透明度最终由系统合成器处理，本工程不读取其数值。外观不同不自动判定为成功，需保持同背景做人工比较。

纯 Clear 使用 nil tint，避免普通玻璃的实验色调残留。返回 Regular 后复用已保存的色调与强度。

每次确认参数后保存；拖动色调滑杆时限频到约 30 次/秒，松手后记录最终参数，避免连续写磁盘。隐藏面板后不做材质刷新，前台轮询只比较屏幕 trait。

SDK 与设备系统是两件事：旧 SDK 编译的 tweak 可在新系统运行时找到公开新类；旧系统没有该类时仅提供标记明确的 UIBlurEffect fallback。是否真正遵从 iOS 27 系统透明度、是否存在宿主 App 的兼容模式影响，必须由设备上的实验确认。
