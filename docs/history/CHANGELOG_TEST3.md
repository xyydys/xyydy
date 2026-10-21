# 2.0.3 TEST3 代码定位

- `MessageTopGlassV3.xm`：Clear → Regular 工厂；其余消息功能保持 TEST2 路径。
- `Tweak.x` 顶部分类区域：恢复 TEST1 控件和尺寸，仅调整基底／选中材质。
- `ALBGlassMaterials.h`：顶栏、设置使用的 Regular UIGlassEffect；系统减少透明度时使用系统材质。
- `ALBNativeInteraction.h`：右侧真实触摸序列、原识别器共存及头像裁切恢复。
- `Tweak.x` 清屏区域：透明菜单入口回到右栏；离开清屏重新绑定头像。
- `ALBNativeMinimize.h`：来自 1.6m 的页面／驱动分离与根观察器刷新。私有调用全部限定本插件控制器并检查 selector，延时绑定有世代校验。
- `AwemeAlbumMorphBar.swift`：单独 shadow driver；不用 requested 状态冒充系统已经收缩；原底栏图标、材质、布局逻辑保留。
- `ALBSettingsGlass.h`：单层 UIKit 悬浮容器；移除 TEST2 的 SwiftUI 双层 sheet 文件。
- `Tests/scroll_intent_test.c`：补 4pt 阈值、同次拖动反向与横滑排除验证。

`docs/history` 是旧版记录，不作为本版行为说明。
