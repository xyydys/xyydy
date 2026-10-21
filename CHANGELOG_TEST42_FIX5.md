# 2.0.10 TEST42 FIX5

基于 FIX4（0a84507）。依据用户 FIX4 真机截图和反馈。

## 顶栏中间 Clear（FIX4 未生效的原因与修正）

- 原因：FIX4 拦截 `UIVisualEffectView setEffect:`，但浮动 UITabBar 平台的材质不经过它，而是平台视图自身的 `glassEffect` 属性（v10/v11 文档记录的 DYKiller 发现，项目曾用它在 iOS 27 底栏成功实现 Clear）。所以中间分组保持原材质。
- 修改：删除 FIX4 的 effect view 钩子。顶部分组原生 UITabBar 的 `layoutSubviews` 中，对其原生表面内类名含 `Platter` 且拥有 `glassEffect` 的视图，原位替换为项目唯一的 Clear 配方（`AWEMakeUntintedClearGlassEffect`，保留 interactive，无 tint）。不进入 LiquidLens / SelectionLens / SelectedContent / DestOut / Portal / Reparenting 子树，选中透镜及动画不变。已安装的 effect 按对象识别，不重复写入；UIKit 重建时下一次布局替换。同一布局轮次内完成，无延时。
- 左右按钮保持各自的 UIKit Clear 按钮玻璃，TEST42 的材质复制仍已移除。
- 录制时 `top.material.clear.coverage` 给出 `platter-glassEffect-clear` 或 `no-platter-glassEffect-found`。

## 底栏中间球旁的移动胶囊（FIX3 回归）

- 截图显示静止时球旁有随页面位置变化的空胶囊：这是切换页面后 UIKit 新建的选中透镜。FIX3 为保留长按透镜，在收缩终点完全停止捕获新透镜，连这种透镜也放过了。
- 修改：恢复 TEST42 的新透镜捕获；只有在中间球被实际按住期间（零延时按压观察，不取消触摸）首次出现的渲染节点保持原生；展开动画开始前全部捕获。

## 左按钮延迟

- 静态源码不能确定原因。新增仅录制时、最多 80 条的 `top.left.trace`：每次顶栏刷新的结果（未入窗/过小、容器不可见、找到/未找到左入口）以及抖音左入口视图的入窗和布局时刻、alpha/hidden/opacity。

## 双击面板

- FIX3 只加了诊断，匹配逻辑未改，所以仍失败属预期。需要录制中点击收藏、转发得到的 `panel.action.unresolved` 记录后才能按稳定标识修正。

## 未改

收藏首进修正、双击面板诊断逻辑、底栏原生收缩逻辑。
