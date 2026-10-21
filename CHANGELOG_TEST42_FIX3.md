# 2.0.10 TEST42 FIX3

基线：origin/main `6243271`（2.0.10 TEST42 FIX2 稳定版）。未合入 PROBE1/PROBE2、Placement-A、WIDTH-B1、安全区/宽度实验、W1/W2/W3 几何重构、载体回读实验或任何 minprobe 运行代码。

## 1. 收藏页首次进入底部背景

- 根因：底部延伸只在两处执行：内层列表自身 `layoutSubviews`，以及 `AWEProfileBaseContainerViewController viewDidAppear` 的全量扫描（从作品返回时触发）。首次进入时，新建的收藏列表在 ListKit 分页器仍把它放在屏外或尚未入窗时完成布局，被可见性判断拒绝；随后分页器滑入只触发分页器布局，列表自身 bounds 不变，不会再布局。
- 修改：`AWEListKitTabContentCollectionView -layoutSubviews` 在分页器布局时，对其可见 `AWETabContentItemContainerCell` 内的列表执行原有 `ALB36ProfileViewportDidLayout` / `ALB37ExtendProfileInnerList`（`ALB43RevealVisibleProfilePages`）。无延时、无轮询；内层列表查找有上限并按 cell 缓存。

## 2. 顶栏冷启动与左右材质

- 中间分组的玻璃来自 UIKit 浮动 `UITabBar` 平台（源码不设置任何 effect）。左右是 TEST9 的 UIButton（`clearGlassButtonConfiguration`），TEST42 再从中间分组复制一个 effect 覆盖到按钮的 effect view。
- 冷启动根因：左右按钮只在自身布局时同步材质；材质来源要等中间分组的原生 UITabBar 完成布局后才存在，因此最初几帧显示按钮自身配方，之后才变化。另外按钮配置被 UIKit 重算（按压、状态、外观变化）后，要到下一次布局才重新套用。
- 修改：顶部原生 UITabBar 布局后立即同步左右材质（`ALB43SyncTopSideMaterialFromNativeBar`）；`ALB42TopMaterialButton -updateConfiguration` 后立即重新套用；来源查找跳过隐藏/透明的 effect view（不可见的栏背景不能作为材质来源）。录制时记录一次 `top.material.sync`（来源/目标类、层级和 effect 描述）。
- 未完全解决：UIButton 不能承载 UITabBar 平台的原生玻璃。完全相同的原生构造只有 `ALBTopNativeGroup` 的 action 位（TEST15–36 使用过），但会失去 TEST37 恢复的左按钮 UIButton 原生菜单展开/收回。需要用户决定取舍；`top.material.sync` 记录可确认当前复制的具体来源。

## 3. 中间头像球长按残留胶囊

- 所属：UIKit 的选中/按压透镜（LiquidLens）。TEST42 的 `reapplyNativeGeometry` 会把控制器布局中新建的任何透镜渲染节点纳入前景租约（图层隐藏、布局冻结、几何锁定，并拦截 UIKit 自身的 `setHidden:`）。
- 根因：长按中间球时 UIKit 新建按压透镜（承载按下项内容，包括搜索内容中的头像），随即被上述租约捕获：不能拉伸，松手后 UIKit 也无法隐藏，于是成为静止残留胶囊。TEST42 之前该透镜保持原生，和头像一起按压形变。
- 修改：收缩终点静止时（`progress == 1` 且无动画）不再捕获新建透镜，由 UIKit 完整负责；从终点开始展开前（`animate(to:)`）先捕获，保持 TEST42 在过渡期间的隐藏规则。不隐藏额外视图，不新增玻璃。

## 4. 双击面板转发/收藏

- 现状：面板从 `AWEFeedVideoButton` 列中按已绑定种类→文字证据→（恰好四个时）位置识别。点赞/评论匹配失败时还能回退到控制器 `performLikeAction`/`performCommentAction`，转发/收藏没有回退，因此只有这两项表现为失败。
- 静态源码无法唯一确定当前抖音的收藏/转发运行时标识，因此不盲改匹配。新增仅面板使用、只读、有上限的诊断：任一面板动作转发失败时记录 `panel.action.unresolved`（录制中）和 `PANEL_ACTION_UNRESOLVED`（诊断文件）：每个 `AWEFeedVideoButton` 的类、父类、位置、可见性、已绑定种类、文字证据、无障碍标识、手势/控件的 target.selector；以及右侧列中其它可交互的非 `AWEFeedVideoButton` 视图和插件槽位绑定。

## 验证

本地：完整预检、`check_test43.py`（4 个反例）、Logos 边界、原生端点语法、11 个 C 测试。无 Swift 编译器、Apple SDK 或设备；Swift 类型检查与完整构建由 Xcode 27 CI 完成。
