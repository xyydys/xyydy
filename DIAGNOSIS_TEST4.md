# TEST4 定位依据与诊断

本轮只有用户的现象反馈和截图，没有 TEST3 的新运行日志。以下分为源码事实和待真机验证的推断。

## 源码事实

TEST3 在 iOS 26 使用独立 UIWindow，Swift 因跨窗口转而绑定自己的空 UICollectionView。iOS 27 使用同窗口，但把插件 UITabBarController 作为宿主 child；用户提供的 1.6m 特意避免这种嵌套，并直接观察真实根列表的 pan。TEST4 对齐这三处结构；不再只调整判向阈值。

TEST3 设置和顶栏底层明确使用 UIGlassEffect Regular。Clear 不会因为 UIView.backgroundColor 被设为 clear 而自动生效。TEST4 改 effect 的真实 style，清除 tint，并减少其他背景覆盖。

原底栏已有 AWEApplyEventDrivenTransparentContrast：根据低分辨率背景亮度选择黑／白前景，再更新图标。它不是 UIColor.labelColor 自动判断视频背景。TEST4 使用同一类取样和阈值思路，但分组、设置和面板分别取自己的区域。所取画面是合成画面近似值，不保证复杂纹理的每个像素都具有最佳对比度。

清屏时 TEST3 的菜单仍在原头像所在的 stack 中，而且每次布局都清空菜单签名。TEST4 将清屏入口放到窗口直接子层；正常头像菜单仅过滤命中插件菜单按钮的祖先单击触摸。

## 收缩日志

| 关键字 | 意义 |
|---|---|
| SAME_WINDOW_DETACHED_ATTACH | 已把系统底栏视图挂入源窗口，没有嵌套控制器。 |
| ROOT_PAN_INSTALLED | 已向某个真实根列表的原生 pan 添加观察 target。 |
| ROOT_PAN_BEGIN | 收到拖动；同窗口信息在此处记录。 |
| ROOT_PAN_BLOCKED | 当前功能／可见性／设置／路由门禁阻止处理，附具体布尔值。 |
| ROOT_PAN_SOURCE_REJECTED | 列表与当前页面、可见范围或窗口不匹配。 |
| MINIMIZE_BIND_PENDING | 尚无同窗口的真实源列表，不会用空代理冒充成功。 |
| MINIMIZE_BIND | 显示 realFeed、sameWindow 和独立 driver。 |
| MINIMIZE_ROOT | 显示实际观察对象是否与期望根列表匹配。 |
| ROOT_PAN_COMMIT | 上／下滑已识别并提交；这不代表 UIKit 动画成功。 |
| MINIMIZE_RESULT | requested 是请求，measured 来自左侧系统 platter 的实际宽度。unknown 是无法测量，不能算成功。 |

如仍失败，连续操作「上滑—下滑—点左边小球」后，用原设置页的诊断导出入口导出；无需反复换灵敏度。真实收缩仍依赖 UIKit 运行时，私有兼容调用有存在性检查，但本地无法替代设备验证。
