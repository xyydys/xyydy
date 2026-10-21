# 2.3 MATERIAL2：分析、适配与留存

## 本次确认的源码原因

1. 原工程的材质入口各自独立：顶部分组依赖默认 UITabBar provider；两侧按钮使用 Clear 配置；底栏既有 Objective-C 替换，又有 Swift 替换。名称相近并不意味着实际配方相同。
2. 原 iOS 27 查找路径依赖 glassEffect/effect owner。工程已有设备导出记录指出 UIKit._UITabBarItemPlatterView 不提供该入口，owners=0。该版本即使保存了 Clear 开关，也可能没有替换任何材质。旧容器 spacing 修改不能解决材质问题，本次已删除。
3. 顶部在 iOS 27 读取底栏上次缓存的风格，在 iOS 26 则继承应用；Clear 按钮也继承应用。抖音固定浅色窗口与首帧/切页时序会使视频页出现浅色玻璃。本次直接用当前页面与所选来源判断，且在同一目标 trait 环境内构造材质，避免工厂提前继承抖音的浅色环境。
4. 原收缩在约 0.30 秒取消 outstanding 状态，并在 CATransaction completion 后强制无动画重排。外部 transaction 的结束未必等于 UIKit 弹簧结束，可能允许过早反向请求或在结束时跳回布局。源码证据支持修复这两个时序问题；不能仅凭源码证明设备上的全部过度收缩均由此造成。

## 统一材质与开关

唯一材质工厂为 ALBUnifiedMaterial.m。Regular 使用 UIGlassEffect style 0，Clear 使用 style 1；Clear 始终 tintColor=nil。系统的玻璃透明度/色调偏好继续由 UIKit 处理，不用 view.alpha 模拟透明度，也不因“降低透明度”开启而直接换成普通 blur。

选项包含：统一策略总开关；Regular/Clear（iOS 26 另有保留底栏模式）；跟随系统、跟随抖音、固定浅色、固定深色；首页/朋友视频页保护；Regular 的无色调/动态/白/黑/蓝及强度。色调选项只在 Regular 生效；Clear 与 iOS 26 保留底栏模式禁用该组控件。

旧 bottom.clearMode 在没有新模式记录时迁移为 Clear。没有记录时 iOS 27 默认 Regular、iOS 26 默认保留底栏原生材质。切换新模式同时保留旧开关值供旧版读取。关闭统一策略后移除自有替代渲染器、恢复租用的背景层/效果及原生按钮背景。

顶部中间分组、两侧按钮、底栏主分组与辅助搜索进入同一策略；右侧操作、头像菜单源、消息页玻璃按钮、弹幕入口、快捷面板及设置/评论外层玻璃也接入。设置内用于阅读的普通磨砂区块、用户选择的评论磨砂模式、UIKit 原生选择透镜和系统菜单内部仍由各自原生用途控制。

外观优先级：视频页深色保护 → 所选外观来源。保护只针对首页及真正的朋友视频流；第二栏商城、商城路由及网格不会套用该例外。设置/评论面板作为独立面板使用所选来源。要在首页测试固定浅色，先关闭视频页保护。

## iOS 26 最难部分的处理

保留底栏模式不会改写现有底栏配方。以底栏可读取的最宽主分组作为参考，匹配顶部/按钮对应渲染层的类别，同步 filters/backgroundFilters/compositingFilter 和已有底色；不拷贝 layer 几何、transform、mask 或触摸对象。

两侧按钮仍是原 UIButton，保持原配置的内容、菜单、primary action、targets 和 configurationUpdateHandler。只更新背景与外观，不用小 UITabBar 替换按钮，不移动按钮/手势进 effect contentView。按压/高亮时推迟配置修改。

系统内部结构不同可能无法读取或匹配底栏配方，此时使用原生 Clear 按钮/明确的 Clear effect 回退，状态分别标为 native-clear-button-fallback 或 native26-clear-effect-fallback。这些标记不等于“已确认与底栏相同”。精确一致性仍须在目标 26 设备上对比。

## iOS 27 的真实材质入口

优先原位替换可用的 public glass effect view 或 ABI 检查通过的 glassEffect 属性。只有已识别的纯背景渲染层才进行渲染交接：隐藏其完整背景绘制，安装一个 public UIGlassEffect renderer。不会同时保留两层玻璃。

UIView 所属层一般排除；仅放行已知、不可交互、且不含控件/文字/图片/透镜的 backdrop。选择透镜、SelectedContent、selection、DestOut 和 portal 子树排除。替代 renderer 不接收触摸，使用原 carrier 的 bounds 与 capsule cornerConfiguration，跟随其原生形变。无安全入口则标为 unresolved-native-renderer，保留原绘制，不伪报成功。

## 收缩与缩放

底栏仍由 UIKit 原生 minimize endpoint 动画；原外层 scale canvas 保持唯一缩放来源，未对内部 platter、透镜或图标附加缩放。删除动画结束强制 layoutIfNeeded 和 0.30 秒提前清除请求的代码。

反向请求等待实际 bounds/position/transform 动画结束；短暂起始等待避免 UIKit 尚未添加动画时误判完成，1.8 秒上限防止无法读取或持续动画永久阻塞。监测只发生在一次收缩请求期间，不启用常驻轮询。保留 request/native-settled/native-settle-timeout 的原生几何日志。

## 留存与真机核验

“查看 / 复制材质状态”展示实际背景 effect 描述、覆盖路径、已解析的外观、按钮交互与菜单状态。sharedMaterial 同时进入现有复现状态导出，开启原有诊断后可以保留。系统滑块具体数值无公开读取入口，不能从 JSON 自动证明其位置；截图应同时记录手动测试标签。

| 设备/场景 | 要检查的结果 |
| --- | --- |
| iOS 27，Regular，系统最透明/最着色，深浅两种 | 三处采用同一 Regular；最透明仍允许有扩散 |
| iOS 27，Clear，系统最透明/最着色，深浅两种 | 无自定义 tint；系统最着色时一定程度不透明符合已有实验，不能判为开关无效 |
| 系统浅色，首页/朋友，保护开启 | 分组、两侧按钮、底栏均为 Dark；无缓存等待 |
| 保护关闭，固定浅/深；消息/个人/商城 | 跟随选项；商城及网格不误用朋友规则 |
| iOS 26，保留底栏 | 对比顶部/两侧与原底栏；读取是否为 bottom-native-recipe，回退要单独记下 |
| 左菜单点击、长按、取消、展开/关闭；右搜索 | 原入口与原生按钮菜单、点击持续有效 |
| 缩放 0.55/0.75/1.0/1.15，收缩/展开/快速反向 | 无重复缩放、过早反向或结束重排；结合 native-settled 日志 |
| 总开关关闭/再开启；切页、前后台 | 原背景正确恢复，重新接入后无叠加玻璃 |

授权 auth50 区域保留原 SHA-256；其它业务、下载、滚动/触摸转发、布局几何的保留范围由历史门禁和 material2_changes.json 共同记录。旧验证文档保留作历史参考，以本说明及当前版本验证记录为准。

## 本地验证的实际范围

通过完整 Python preflight；64 组外观策略与偏好迁移/动画边界 C 测试；6100 组缩放锚点/幂等轨迹；原生端点/状态、底栏与顶部几何、滚动方向测试。通过源码括号、Logos 边界与交互所有权门禁。

本环境没有 Clang、Swift、Apple SDK 或设备，未执行 iOS 类型检查、完整 Theos 编译、提取 Swift 调度测试或真机外观/菜单动画测试。CI 已加入 ALBUnifiedMaterial.m 的 arm64/arm64e SDK 检查，并保留 Swift 类型检查与完整打包；这是本次适配实现，不是已通过真机验收的版本。

原生 API 参考：[UIGlassEffect](https://developer.apple.com/documentation/uikit/uiglasseffect)、[UIBackgroundConfiguration](https://developer.apple.com/documentation/uikit/uibackgroundconfiguration-c.class)、[UICornerConfiguration](https://developer.apple.com/documentation/uikit/uicornerconfiguration-c.class)。
