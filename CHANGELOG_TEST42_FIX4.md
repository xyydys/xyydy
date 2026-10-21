# 2.0.10 TEST42 FIX4 — 顶栏统一 Clear

基于 TEST42 FIX3（586f996）。收藏首进、中间头像球按压透镜、双击面板诊断逻辑未改。

## 目标

左按钮 Clear + 中间分组 Clear + 右按钮 Clear。TEST37 的左右 UIButton 交互层（左菜单展开/收回形变、原生按压拉伸、右侧搜索、命中与几何）不变。

## 修改

- 移除 TEST42 的材质复制：`ALB42TopMaterialButton` 只保留为普通 UIButton 子类，不再查找、复制或同步中间分组的 effect；左右按钮保持各自原有的 UIKit `clearGlassButtonConfiguration`。
- 中间分组：凡是位于 `ALBTopNativeGroup` 内、effect 为 `UIGlassEffect` 的 `UIVisualEffectView`（即原生浮动 UITabBar 平台自身的背景玻璃），其 effect 替换为项目已有的 Clear 配方（`AWEMakeUntintedClearGlassEffect`：`UIGlassEffect` style Clear、无 tint、保留原 interactive）。这是替换同一个 effect view 的材质，不叠加第二层。
- 排除：祖先链中出现 LiquidLens、SelectionLens、SelectedContent、DestOut、Portal、Reparenting 的视图一律不改，因此选中透镜、其玻璃与动画仍由 UIKit 负责。
- 时机：在 UIKit 调用 `setEffect:` 时直接替换参数，或在该视图 `didMoveToWindow` 时替换，标准材质不会先显示；UIKit 重建 effect 或视图时各自的事件会再次触发；已替换的对象用关联标记识别，不在布局中重复写入。无延时、无重试。
- 冷启动旧入口：抖音顶栏容器进入窗口、由隐藏变显示、alpha 从 0 变可见时，同步执行一次现有的顶栏刷新（绑定并隐藏原入口），原有排程刷新仍保留给晚到的入口。
- 录制时记录 `top.material.clear`（每次替换的视图类、祖先链、原 effect）与一次 `top.material.clear.coverage`（中间分组中已 Clear / 未替换的玻璃视图数、effect 视图类型、非 effect 的玻璃类名）。若 iOS 26 没有可替换的 `UIGlassEffect`，coverage 显示 `no-replaceable-UIGlassEffect-found`，此时不会使用任何假模糊或染色替代。

## 验证

本地预检、`check_test43.py`（含 Clear 排除项与配方复用反例）、`check_test42.py`（改为断言材质复制已移除）。实机效果需确认。
