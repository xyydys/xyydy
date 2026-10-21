# TEST42 FIX2 — Swift 方法作用域修复

截图中的三个编译错误来自同一个错误副本：`settleForBackground(compact:)` 被插入到嵌套的 `NativePiece` 内，因此其中的 `stopTick`、`finished` 和 `apply(progress:)` 无法访问外层实例。

本版删除 NativePiece 内的错误副本，保留 ALBCenterMorphSurface 外层已有的正确方法和控制器调用。运行时源码仅修复这一处，顶栏材质、头像比例、音乐对齐及收藏处理均保持 TEST42 实现。

新增作用域回归检查，要求：方法恰有一份、直接属于 ALBCenterMorphSurface、NativePiece 内不得调用这三个外层动画成员。使用 FIX1 错误源码验证该检查确实会失败，再用修复源码验证通过。

完整本地静态预检通过。截图说明前面的云端静态检查和原生载体检查已通过；本地没有 Swift 编译器及 Apple SDK，本版仍需云端完成 Swift 类型检查和完整构建，未宣称已经编译成功。
