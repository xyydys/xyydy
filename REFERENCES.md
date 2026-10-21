# 参考

- Apple：Build a UIKit app with the new design（WWDC25）。原生 tab bar minimization、玻璃控件和滚动边缘的公开设计／接口说明。https://developer.apple.com/videos/play/wwdc2025/284/
- Apple：Adopting Liquid Glass。https://developer.apple.com/documentation/technologyoverviews/adopting-liquid-glass
- cilicili：播放设置的透明 List、导航栏和玻璃 sheet 结构参考，UIKit 版本在本工程独立实现。参考提交 `c61b0d33966c5d8e87ea20527b8ba48c702c10c1`：https://github.com/Rone89/cilicili/blob/c61b0d33966c5d8e87ea20527b8ba48c702c10c1/bili/Sources/Features/Player/BiliPlayerViewContent.swift
- 你提供的 `VVebo_LiquidGlass_IconWarp_Solution.md`：UIKit 真实触摸、configuration 内容层和原业务 sender 的处理经验。
- 你提供的 `1.0.56c16` 工程：恢复 `prominentGlassButtonConfiguration` / `glassButtonConfiguration` 带色按钮的原实现。

## TEST2 使用的 Apple 接口

- [tabBarMinimizeBehavior](https://developer.apple.com/documentation/uikit/uitabbarcontroller/tabbarminimizebehavior)：保留 UIKit 控制器的收缩行为；独立驱动属于插件集成方式，Apple 文档并未保证注入宿主下的效果。
- [prefersScrollingExpandsWhenScrolledToEdge](https://developer.apple.com/documentation/uikit/uisheetpresentationcontroller/prefersscrollingexpandswhenscrolledtoedge)：半高 sheet 中先滚动内容的 UIKit 选项。
- [presentationContentInteraction](https://developer.apple.com/documentation/swiftui/view/presentationcontentinteraction(_:))：系统 sheet 的滚动优先策略。
- [presentationBackground](https://developer.apple.com/documentation/swiftui/view/presentationbackground(_:))：系统 sheet 的透明背景。
- [UIGlassEffect.Style.clear](https://developer.apple.com/documentation/uikit/uiglasseffect/style/clear)：设置页 Clear 玻璃。

独立滚动驱动参考用户提供的 `AwemeLiquidMinimizeLab_1.6m_true_k_gesture_merge_local_iOS15(2).zip`，没有引入独立重构版的底栏视图实现。

## TEST3

- [UIGlassEffect.Style.regular](https://developer.apple.com/documentation/uikit/uiglasseffect/style/regular)：本轮顶栏及设置背景使用的系统材质。
- [UIButton.Configuration.glass](https://developer.apple.com/documentation/uikit/uibutton/configuration-swift.struct/glass())：消息页使用普通玻璃按钮配置。
- [selectedSegmentTintColor](https://developer.apple.com/documentation/uikit/uisegmentedcontrol/selectedsegmenttintcolor)：顶栏选中胶囊的颜色入口。
- 用户提供的 1.6m 工程：ALBindFeed、ALBindShadowDriver、ALRefreshNativeObserverRoot、ALRefreshScrollPocketInteraction、ALNativeDriverImpulse。私有观察器兼容调用来自该实测工程，不属于 Apple 官方承诺的注入方案。

上面的 TEST2 sheet/Clear 资料仅为历史；本版普通设置入口已经改成 UIKit 悬浮面板。

## TEST4

- [UIGlassEffect](https://developer.apple.com/documentation/uikit/uiglasseffect) 与 [Clear style](https://developer.apple.com/documentation/uikit/uiglasseffect/style/clear)：明确设置实际 glass effect 的材质，保留 interactive 行为。
- [tabBarMinimizeBehavior](https://developer.apple.com/documentation/uikit/uitabbarcontroller/tabbarminimizebehavior)：系统原生收缩策略。注入宿主、隔离滚动驱动和观察根刷新是本插件及用户实验工程的集成逻辑，不是 Apple 文档保证的方案。
- [DYKiller DKGlassTabBar.xm](https://github.com/HierarchThurs/DYKiller/blob/f907c87b4a4d4039f5d3df370ca8e27028cd2ce1/src/Features/TabBar/DKGlassTabBar.xm)：研究材质与前景分离、Clear effect 与 platter 的关系。本轮未复制其实现；保留原底栏的材质实现，只更换顶部分组和设置的 effect、快捷按钮配置。
- 用户 1.6m 工程的 detached mounting、普通透明页面、真实根列表 pan observer 与 selected→feed／owner→driver 绑定，是本次收缩集成的对照。

TEST3 的 Regular 顶栏／设置描述仅供历史对照，TEST4 两者已经改为 Clear；消息页仍使用 Regular。

## TEST8

- Apple UIButton 原生 glass/clearGlass 配置（材料切换保留 UIButton 原生触摸）：https://developer.apple.com/documentation/uikit/uibutton/configuration-swift.struct/glass()
- 原生 clearGlass：https://developer.apple.com/documentation/uikit/uibutton/configuration-swift.struct/clearglass()
- 控制器管理的 Liquid Glass tab 结构：https://developer.apple.com/videos/play/wwdc2025/284/
- 本次底栏材料基准：用户已在设备确认有效的 TEST7；继续沿用所上传 1.0.58 v9 的页面外观与透明内容结构。

## TEST9

- Apple MPNowPlayingInfoCenter.nowPlayingInfo: https://developer.apple.com/documentation/mediaplayer/mpnowplayinginfocenter/nowplayinginfo
- audioMotion-analyzer: https://github.com/hvianna/audioMotion-analyzer — 径向频谱参考项目（JavaScript / AGPL-3.0），仅查看项目说明；本次未移植其代码、未新增运行依赖。波形调整是对本工程已有频谱数据的独立角度映射与软压缩。
