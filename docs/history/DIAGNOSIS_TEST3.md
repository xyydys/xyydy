# 本轮问题依据

来自用户这次导出的 TEST2 日志，时间为日志原始时间：

- 02:27:30.171 出现 `INDEPENDENT_PAN compact=1`。不是没有识别到上滑。
- 02:27:30.248 出现 `INDEPENDENT_DRIVER_COMMIT compact=true`；后续反复出现同向 pan，却没有对应 commit。
- 源码确认：TEST2 在发出请求时马上设置 `compactMode=true`，之后同向请求被提前返回。日志的 COMMIT 只能证明 offset 被写入，不能证明系统呈现已改变。
- TEST2 的实际绑定是代理 collectionView → 代理 collectionView；真实首页没有接入。1.6m 还包含“真实页面绑定、独立 driver、根观察器及 pocket 刷新”，此前未完整移植。
- 02:28:54.334 出现 `NATIVE_GLASS_CONTROL_FALLBACK sent=0 source=AWEFeedVideoButton`。不能把这次备用 UIControl 转发视为成功。
- 源码确认：TEST2 头像按钮在原图的局部父视图中，但清屏代码只豁免右栏的直接子按钮，因此祖先被禁用；退出时又只更新菜单而未重新安置头像按钮。
- TEST2 顶栏把一个完整 UITabBar 的内部 platter 缩放到 44pt 外框，存在内部测量与裁切耦合。TEST3 恢复 TEST1 现有控件尺寸。

新的 MINIMIZE_RESULT 读取真实 platter 的宽度变化；如果当前 iOS 私有视图名称变动，就报告 unknown。触摸日志只报告事件到达，不自动宣称点赞、收藏或页面跳转已经完成。

1.6m 中对右侧 Search/Edit 独立工具栏的布局不适用于本工程现有 UISearchTab，因此没有引入那套外观。原底栏的搜索由其 UIKit 控件继续呈现。
