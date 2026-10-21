# TEST30 CAPTURE FIX1

修复截图中的采集器编译错误，并增加零评论表情背景采集。

- `UISheetPresentationController.preferredCornerRadius` 是 `CGFloat`，统一用 `@(sheet.preferredCornerRadius)` 写入诊断字典；采集器及设置页采集各修复一处。
- `UISheetPresentationControllerDetent.identifier` 加 iOS 16 判断；iOS 15 记录该属性不可用，保留 detent 数量和 selectedDetent 等已有信息。最低部署版本仍为 iOS 15。
- SDK 检查包含真实采集器与整份设置实现头，不再遗漏设置采集的同类错误。
- 浮窗增加第 11 项“0 条评论 / 表情栏残留背景”；评论模块记录既有背景清理的证据、命中区域及清理前后状态。没有可靠评论数量时明确记录 unknown。
- 不修改零评论背景清理算法，也不更换底栏、顶栏和设置材质；本版仍用于采集定位。

操作：评论关闭时选择新项目 → 打开 0 条评论的视频评论区 → 保持表情栏/残留背景可见并标记 → 必要时展开表情面板再次标记 → 结束并保存 → 导出 JSONL。

用户提供的上版日志显示底栏 Swift 类型检查已通过，采集器检查报错；这不是本 FIX1 已编译的证明。本地仅能完成源码、边界及既有 C 分支检查；本 FIX1 的真实 SDK 编译和完整打包仍由 Actions 执行。
