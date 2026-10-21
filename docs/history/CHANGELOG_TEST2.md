# 2.0.2 TEST2 变更定位

- `ALBNativeInteraction.h`：原手势的真实触摸观察、委托适配、单次动作去重、UIControl 安全回退、解绑恢复、图像局部层级。
- `MessageTopGlassV3.xm`：排除整个代理子树、缓存菜单来源、使用原图像及其显示尺寸、重入保护、离开页面取消按压。
- `Tweak.x`：头像／音乐局部挂载、头像扫描排除自建视图、按压期间不关闭右侧按钮交互；顶部分组使用 UITabBar 并沿用底栏实际玻璃；独立 pan 的方向识别；设置呈现入口。移除为收缩而在全局 setContentOffset 中捕捉抖音滚动视图的部分。
- `ALBScrollIntent.h`：手指增量方向策略；同次拖动反向不必回到起点，横向拖动不触发。
- `AwemeAlbumMorphBar.swift`：复用原有透明代理 collectionView 作为独立滚动驱动；绑定后通过短序列 contentOffset 变化请求系统动画；左侧收缩球点击展开。
- `ALBSettingsGlass.h`：Clear 材质、可访问性回退、半屏滚动配置。
- `ALBSettingsPresentation.swift`：系统 sheet 的透明背景及内容优先滚动；仅呈现容器为 SwiftUI，原 UIKit 设置内容保留。
- Actions：仍用原仓库 `xcode-27` runner；新增 `original-fixes-test2` 触发分支、方向测试和新版本产物名。

未增加持续定时器、逐帧截图取色或可见的材质探针。底栏原有材质函数保持不变。消息页的图像复用不等同于迁移整个原 UIImageView；它复用宿主提供的图像，由 UIKit 按钮的内容层负责按压形变。
