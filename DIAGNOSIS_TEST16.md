# TEST16 报错定位

用户截图中的首个编译错误位于 TEST15 的 `ALBTopNative/ALBTopNativeGroup.m:941`：

```text
'performPrimaryAction' is only available on iOS 17.4 or newer
[-Werror,-Wunguarded-availability-new]
deployment target is iOS 15.0.0
```

TEST15 在异步派发块中直接调用 `[live performPrimaryAction]`，而 Makefile 的目标是 `iphone:clang:latest:15.0`。此次扫描全部活动 Objective-C/Logos/Swift 源码，该直接方法调用仅一处。

TEST16 在同一 dispatch block 内用 `@available(iOS 17.4, *)` 保护该调用。菜单主动作完成后 return；普通按钮及较低版本不执行该新 API，保留 TouchUpInside 事件路径。不能把低版本事件兜底说成已证明与新版本菜单展开完全等价。

没有修改 nativeGroup 的创建、材质、尺寸、控制器、按压、布局代码，也没有修改底栏状态驱动。底栏材质文件 ALB5Options.h、Swift 状态驱动和 Makefile 与 TEST15 逐字一致；Tweak.x、ALB5BottomGeometry.m、ALB5Settings.h 仅版本标签变化。

局部回归测试实际运行：arm64/iOS15、arm64e/iOS15 和 arm64/iOS17.4 检查通过；移除 guard 的负对照如预期触发相同的 availability 错误。这个测试用最小声明模拟 SDK 的可用性标注，不是完整 UIKit 编译。

Actions 已添加真实 Apple SDK 的整文件预检查，但当前 Linux 环境没有 Xcode、Apple SDK 或 Theos，未在此处执行云端构建、链接、打包或真机验收。
