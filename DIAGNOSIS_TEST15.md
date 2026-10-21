# TEST15 源码诊断与未验证事项

## 在 TEST10 中确认的代码冲突

ALB5BottomGeometry.m 同时缩放左右 native carrier，左侧非等比压为圆球宽度，改变 alpha 并向中间移动。CADisplayLink 用自定 0.42 秒轨迹驱动，原生 layout 前后又以 CATransaction.setDisableActions 包裹恢复/重施。这是与原生玻璃自身圆角/按压/过渡相冲突的实现路径；仅凭源码不能证明它是每一次方形帧的唯一原因。

底栏 compact hitTest 返回控制器整屏 view，Swift tap recognizer 设置 cancelsTouchesInView=true。物理触摸不能正常到达真实的辅助控件，因此本版移除截获，而不是自行画按压效果。

顶栏中间用 ALBTopNativeGroup；左右却调用 AWEConfigureAdaptiveSystemGlassIconButton，且顶栏 key 被路由到 Clear。两条材质创建路径确实不同。本版直接复用中间类，而不是再对 Clear 调色。

## 当前诊断含义

- NATIVE_REQUEST_API / accepted=1：已成功调用签名匹配的入口，不等于视觉上已经收缩。
- NATIVE_OBSERVED：通过原生 BOOL getter，或有展开宽度基线的左侧 platter 几何观察到状态。
- NATIVE_REQUEST_UNSUPPORTED / EXCEPTION / UNCONFIRMED：不宣称成功，不添加几何补偿。
- 设置页 TEST15 诊断列出原生左侧/辅助位矩形。纯读数，不修改该矩形。

只使用已有控制器和公开滚动绑定结构；最小化手动请求包含私有 selector，因此仍依赖系统版本和设备布局。这里未连接运行中的 iOS，不能确认这些入口在目标机一定可用。两侧菜单的原生 source morph、单槽 TabBar 在 44pt 载体里的行为以及真假材质匹配都需要真机验收。

本包的头像档并不是单独居中球布局，两档只在系统搜索位内容和动作上不同。此限制在设置、README 和交付说明中明确保留。
