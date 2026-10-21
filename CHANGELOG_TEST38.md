# 2.0.10 TEST38

基于 TEST37 FIX1，仅处理本轮指定范围：

- 收缩头像始终位于普通前景层，不再挂入原生搜索 platter/lens 私有渲染树；原生搜索玻璃与点击仍保留在其下方。
- Profile/Favorites 路由复用原有 native tab-bar carrier 背景清理，只清背景/Effect，不改收藏列表 frame、offset 或滚动交互。
- 顶栏左右按钮不再强制 tinted `glassButtonConfiguration`，恢复调用者已有的 untinted UIKit glass 分支；按钮、菜单、搜索转发、按压行为和几何均未改。
- 右侧 `right.scale` 改为包含音乐在内的整列缩放；移除独立 `right.musicScale` UI/逻辑。
- 新增“转发与音乐间距微调”（-40…80），数值按屏幕最终距离补偿。
- 音乐中心 X 对齐到动作列，并显式保持音乐 artwork 在玻璃按钮内居中；水平修正进入持久投影层，原生 press/stretch/deformation transform 继续叠加。

验证为源码/SDK-free 几何测试；仍需真机确认最终 UIKit 渲染。
