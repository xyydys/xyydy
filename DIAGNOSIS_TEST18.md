# TEST18 — 基于两份 .ips 的诊断

## 1. 日志证据（不是源码推断）

| 项目 | 054301.ips | 054501.ips |
|---|---|---|
| 宿主 | Aweme 39.9.0 (399017) | Aweme 39.9.0 (399017) |
| 系统 | iPhone OS 26.6 (23G71) | iPhone OS 26.6 (23G71) |
| Exception | EXC_CRASH / SIGKILL | EXC_CRASH / SIGKILL |
| Termination | FRONTBOARD / 0x8BADF00D | FRONTBOARD / 0x8BADF00D |
| Watchdog event | scene-update | scene-update |
| 时限 | 10.00 秒 wall-clock | 10.00 秒 wall-clock |
| 进程可见性 | Background | Background |
| 应用 CPU 时间 | 9.872 秒 | 8.312 秒 |
| thermal 状态 | fair | serious |

两份 `AwemeLiquidBar.dylib` 均为 arm64e，UUID 为 `188cc59b-d8c7-3ded-add1-db4ce1447ecb`，映像大小 983040 字节。无法仅凭这些字段认定 TEST16/TEST17；文件名里的时间也不能确定插件源码版本。

第一份主线程调用链（从当前执行位置往调用者）：

```text
CFAllocatorDeallocate / __CFStringDeallocate / _CFRelease
AwemeLiquidBar.dylib +411904
AwemeLiquidBar.dylib +502928
AwemeLiquidBar.dylib +490344
AwemeLiquidBar.dylib +456296
AwemeLiquidBar.dylib +322944
UIKit UIView layer layout
CA::Layer::perform_update_ / update_if_needed_
CA::Layer::layout_and_display_if_needed
CA::Context::commit_transaction / CA::Transaction::commit
```

第二份主线程调用链：

```text
-[NSArray countByEnumeratingWithState:objects:count:]
AwemeLiquidBar.dylib +697300
AwemeLiquidBar.dylib +665064
-[_UISystemBackgroundView setFrame:]
-[UIButtonConfigurationVisualProvider _layoutContent]
-[UIButtonConfigurationVisualProvider layoutSubviews]
-[UIButton layoutSubviews]
UIView layer layout / CA layer update / CA transaction commit
```

这能确认场景更新超时，且采样时主线程正经过插件与系统按钮布局代码。不能确认这 10 秒全部时间都花在栈顶函数，更不能把 CFRelease 当成内存损坏的证明。第二份有发热状态，但记录的终止理由不是热保护终止。

## 2. 源码中明确存在的高风险路径

TEST17 的 `CommentGlassDYKiller.xm` 对所有 UIView 的 setFrame 生效。只要评论媒体保护功能开启，它就在每次 setFrame 中搜索 responder/controller 祖先链；找到播放器控制器且传入 frame.x > 0.01 时，直接返回，不调用原始 setFrame。这一分支甚至在评论未显示时也执行。

播放器控制器下并非只有视频画面，普通按钮及其原生背景也可能位于该子树。因而“有播放器祖先”不能证明目标视图本身是应该固定的媒体画面。系统内部背景获得正常正 X 坐标时也可能被拦截，违反正常布局预期。这是源码可以确定的作用域错误，与第二份堆栈暴露出的调用路径相符；但没有匹配 UUID 的二进制/地址表，不能把 +697300 / +665064 直接命名为这两个具体源码函数。

同时存在全局 UIView.layoutSubviews 子树扫描、布局阶段同步 drawViewHierarchy、右侧 setFrame/setBounds 同步再布局槽位、重复插入已有底座等工作。它们可增加布局工作或形成再次请求布局的路径。Apple《Auto Layout Guide: Changing Constraints》的 Custom Layouts 节专门提醒：布局期间视图层级暂时不一致，跨子树失效和反复布局可造成反馈循环。

## 3. 本轮修正与取舍

- 全局几何钩子只处理评论可见、应用活跃、主线程且有媒体基准快照的对象；其余调用原样且恰好一次交给原始实现。旧的“任何播放器后代正 X 坐标都拦截”分支删除。
- 每个 UIView 的布局扫描删除。顶部黑条检查移到实际交互控制器，最多两层 / 96 节点 / 0.12 秒节流；黑色/几何判定仍沿用原逻辑。进入评论之前的广域横移防缩小不再保留，需在真机检查评论过渡，这是明确取舍而非静默假称功能全保留。
- 亮度快照在布局中只读缓存，下一轮主队列最多排一项；截图中保持重入锁、应用非活跃时停止。首次没有快照时沿用现有颜色回退，不改变玻璃配置。缓存采样并不等于保证每帧动态颜色完全一致。
- 右侧堆栈在原始 layoutSubviews 前设重入保护。setFrame/setBounds 只调用系统实现，不在这些 setter 内嵌套布局槽位。无变化不重复写几何，不反复重新插入已在第一层的底座。右侧位置计算文件及原生按压交互文件不变。
- 原生标签墨色未变化时不重新排整组顶栏，避免每次相同 setTextColor 都安排工作。
- 完整保留已确认正确的顶部原生载体和底栏材质/状态驱动代码，不新增视觉模式，不做手绘收缩兜底。

## 4. 二进制匹配与后续定位

构建流程导出所打包 dylib 的 UUID、源码哈希与各架构 link map。它们必须与对应崩溃日志的映像 UUID 一致后才能映射函数地址；TEST18 的 map 不能拿去映射这两份旧二进制报告。link map 也不能代替包含行信息的完整 dSYM。

本地没有 Apple SDK / Xcode / Theos 或用户设备，未完成 iOS 编译与真机运行。不宣称所有卡死已经消除，不宣称底栏收缩、顶栏左侧显示延迟和右侧最终位置已通过验收。

参考：Apple Developer Documentation, “Addressing watchdog terminations”；Apple Auto Layout Guide, “Changing Constraints / Custom Layouts”；Theos Documentation, “Variables”。原始 .ips 未包含在本工程内。
