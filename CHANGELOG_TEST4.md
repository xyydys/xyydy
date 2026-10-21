# 2.0.4 TEST4

- 同窗口、非嵌套的系统底栏挂载应用到 iOS 26 和 27；不再让 iOS 26 回落到空列表作为收缩来源。
- 普通透明占位页替代 80 个空白 cell 的 UICollectionViewController。
- ALBRootScroll 观察已有 pan，首次安装扫描已缓存页面，根列表后续挂入窗口也会注册；按当前页和窗口过滤。
- 独立 driver 仍负责向 UIKit 提交收缩意图，真实列表 contentOffset 不被改写。收到手势、成功绑定、实际几何变化分别记录。
- 设置、顶栏底层和侧面快捷面板改为无染色 Clear；消息页继续 Regular。
- ALBClearContrast 按局部背景更新文字和快捷面板无强调色图标。所有消费者共享低分辨率快照，最多约每秒 2 次；不可见时停止取样，不存盘、不上传图片。
- 单个可见分类的选中胶囊可关闭。
- 滑条改用真实 UIButton 跟踪；修正为 beginTrackingWithTouch:withEvent: 等 Objective-C 选择子，旧的 beginTracking:withEvent: 不是 UIKit 的跟踪回调。
- 清屏菜单扩大区域并离开原头像的手势祖先链；菜单签名只在业务状态变化时刷新，避免每次布局重建已打开菜单。
- 正常头像菜单也隔离自身触摸与祖先原生单击；其余触摸继续交给原代理。

已确认可用的右侧四按钮桥接、消息页文件和下载业务保持不变。真机视觉和交互结果仍需 TEST4 安装后确认。
