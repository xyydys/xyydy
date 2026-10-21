# 2.0.10 TEST37 REPAIR

本版从已交付的 TEST36 修改。没有撤回到整个 2.0.9 工程。

## 底栏

- 搜索玻璃保留采集到的原生宽、高和纵向位置，只平移；主栏只改变宽度。取消把搜索宽度强行写成主栏高度的计算。
- 原生选中系统的普通 Content、SelectedContent、Lens、DestOut 作为完整前景组保留展开布局；收缩开始隐藏这一组的渲染。停止替换它们的原生 mask、停止清除原生动画，不再修改原生图标叶子的 hidden/alpha。
- 前景组暂停内部 layout，玻璃材质本体仍使用原来实例。展开结束先解除布局冻结，执行一次展开状态的原生布局，再恢复前景；不会主动把 selectedTab 设置为第一个 tab。
- 收缩终点把主栏设置为 hidden，将主栏及选中透镜从可见玻璃中移除，保留原生搜索玻璃。展开第一帧恢复主栏。新增的头像只有 UIImageView，没有新建玻璃或透镜。
- 搜索只遮挡公开的图像/文字叶子，保留真正的搜索按钮及其按压子树；头像位于原生搜索玻璃的前景，按两边坐标换算大小，目标直径为搜索可见区域短边的 90%。
- 朋友、消息角标在原位快速渐隐，不参与向中心的移动；按同一进度反向恢复，完全展开前约 35 ms 开始出现。其他图标/标题仍按同一收缩进度移动、按接近中心的距离渐隐。选中胶囊到完全展开才恢复。
- 没有改动默认底栏材质，也没有在收缩过程中创建 Clear/Regular 替代效果；原有 Clear 模式仍需显式启用。

## 顶栏两侧

恢复 2.0.9 TEST9 的 AWEHomeTopSlotView、普通 UIButton 创建、直接绑定 UIMenu、原始搜索手势租借/归还和业务转发实现。源码逐段指纹比对。

移除 ALBTopNativeIconButton、空菜单锚点、手工 performPrimaryAction 和自制菜单返回预览路径。菜单源就是原生玻璃按钮本身，由 UIKit 处理展开时的源按钮和收回动画。

材质配置使用 regular 玻璃，对齐当前中间顶栏的材质族及明暗上下文。中间顶栏自身的材质实现保留。本地无法验证 iOS 26/27 的实际光边、菜单色调和触摸表现。

## 收藏底部

TEST36 只解除裁切，不能让 UICollectionView 为 bounds 外区域生成更多 cell。本版只延伸 AWETabContentItemContainerCell 内的竖向列表：高度增加 Δ，同时 contentInset.bottom 增加 Δ，保持 contentSize.height - bounds.height + bottomInset 不变。

排除外层 ProfileStick、ListKitTabContent 分页器、其他横向/嵌套列表；不改变顶端 inset、contentSize 或吸顶外层坐标。不在拖动/减速时改几何，不恢复旧的全局 contentOffset 拦截或滚动刷新扫描。所有写入可恢复，原有裁切恢复也保留。

## 验证范围

- 执行本机可运行的预检、回归契约、范围保护与提取 C 算术测试；详细结果在 validation/test37_repair_results.json。
- 顶栏关键段与 TEST9 比对；Tweak.x 请求范围外的源码与 TEST36 比对；auth47、Swift 的原生请求队列及其他未修改源文件有校验。
- CI 仍要求 iPhone SDK 的 Objective-C/Swift 检查、Swift 几何测试和 Theos arm64/arm64e 完整构建。没有取消实际编译门禁。
- 本机没有 clang、swiftc、Xcode 或 iPhone，不能把源码检查描述为 iOS 编译通过、透镜实机消失或帧率达标。完整工程须经随包 Actions 构建，安装后的渲染和交互仍需真机确认。
