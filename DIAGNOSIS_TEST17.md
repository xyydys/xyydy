# TEST17：源码证据与运行时未确认事项

## 输入证据

用户对 TEST16 的反馈是“疯狂闪退，卡死，无法使用”。本轮没有 .ips、线程堆栈、看门狗报告、JetsamEvent 或实际运行视频。不能确认具体异常类型，也不能确认崩溃发生于顶栏还是底栏。以下是能够从所给源码确认的冲突路径，不是已经由设备堆栈证实的完整根因。

## 可见风险及对应修正

### 顶栏单槽选中态冲突
TEST16 为 ALBTopNativeIconButton 设置 actionOnly=YES、selectedIndex=0、keepsSingleSelectionCapsule=NO。hidesSelection 因此为 YES，layoutSubviews 每次会尝试将 nativeBar.selectedItem 改为 nil，而控制器仍拥有选中的 UITab/代理页。这形成两个选中状态来源。TEST17 的 action-only 分支保留原生控制器选择，不再直接写 selectedItem。

### 顶栏布局反馈
TEST16 的控制器 didFinishLayout 会在检测到子层几何改变后请求父层布局，父层 fitNativeBarToSize 又同步触发多个原生 layoutIfNeeded。外层重入锁原先在 super.layoutSubviews 之后才启用。TEST17 提前启用锁、finally 释放，并对同一真实外部尺寸/外观世代设置两次拟合上限，阻止子层几何噪声无限续发拟合。

### 底栏布局 → 监听/收缩 → 布局
TEST16 的 queueNativeRefresh 会在 viewDidLayoutSubviews 后调用 prepareIndependentDriver，并可重试原生收缩请求。ALBNativeMinimize.h 同时设置多个滚动入口并直接调用 _forceUpdateScrollViewIfNecessary、_childViewController:updatedObservedScrollView:、_updateAndObserveScrollView:viewController: 以及 scroll-pocket 私有更新，随后追加两次延时强刷。这不是单纯读取状态。
TEST17 删除私有强刷，仅用公开 setContentScrollView:forEdge: 绑定属于当前代理页的滚动视图。真实宿主 feed 不再被接入透明控制器的私有观察关系。作品/手势原有意图输入保持不变，最终收缩动画仍由原生控制器执行。

### 原生请求重入与陈旧回调
TEST16 同步发起原生请求，只在返回后 accepted=YES 才能阻止同目标重复事件。TEST17 在入队时去重；每轮主线程处理最新目标；原生调用有 Swift/Objective-C 双层重入保护。布局回调只读，不作为重试源。延后观察带 generation 校验，不会覆盖新目标。

### 图标身份与生命周期反馈
TEST16 用 search.image 的对象地址判断是否重写原生图标。对象身份不是语义变更的充分证据，特别是原生 provider 可能返回解析后的对象时。TEST17 只在插件已发布目标发生变化时写入图标。
同时，底栏代理页属于插件 chrome，不是抖音的真实页面。TEST17 将它们与已有顶栏代理一样排除于全局宿主出现/消失恢复回调之外；没有关闭抖音真实页面的生命周期处理。

## 未改变与未完成

未修改底栏材质、右侧布局、授权核心、作品切换策略；没有增加 CADisplayLink 或自定义收缩几何。顶栏两侧仍复用同一 ALBTopNativeGroup 与共用材质入口。两套独立原生收缩外形未在此次修复中完成。

这些修正尚无真实 Xcode 完整构建或 iPhone 运行结果。mock 队列测试覆盖生产 Swift 请求调度源码，不模拟 UIKit 内部、玻璃渲染、内存或 iOS 异常。需要真机测试和崩溃日志才能关闭该运行时问题。
