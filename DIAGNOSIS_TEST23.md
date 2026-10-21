# TEST23 修改说明与验证边界

## 1. 用户日志已经说明：请求触发了，方法也存在

本轮依据是用户粘贴的 TEST22 诊断和截图，没有把旧 .ips 或其他版本的地址当成本轮证据。诊断已输出“请求=收缩”，同时列出：

```text
UITabBar _setMinimized: [v20@0:8B16]
UITabBar _isMinimized [B16@0:8]
```

因此，本轮不再修改“下一条作品触发收缩”的业务开关，也不继续猜测系统没有这些方法。

TEST22 的执行路径与诊断枚举路径不同：执行先走对象的 `respondsToSelector`、NSMethodSignature 和 NSInvocation；诊断直接读取类方法表。旧代码把前述路径中每一种拒绝都汇总为“没有可用入口”。现有诊断不足以区分到底是哪一步拒绝或异常，不能断言是某个特定系统行为。

TEST23 将执行与诊断统一到实际 Method：检查参数个数，并解析完整类型编码（包括类型后的栈偏移），再按其确切的 `B`/`c` 类型调用。优先执行用户设备已确认存在的 `UITabBar._setMinimized:`；一旦派发成功不继续尝试其他入口，不额外设置速度。状态由同一个 bar 的 `_isMinimized` 回读，UIKit 自己管理过渡。

新的状态文本包括入口名称、调用前状态和调用后状态。状态值变化不是已经真机验收动画形状的证明；原生过渡的实际显示仍需设备验证。ABI 不匹配或异常会输出具体原因，不再混称“没有入口”。

## 2. 图标消失：不能把整层 lens 当作纯背景

TEST22 允许将 `_UILiquidLensView.layer.opacity` 设为 0，只检查其子树中是否有 UIControl、UILabel 或带图像的 UIImageView。用户诊断中还存在 `_UIPortalView`，它是必须保护的内容路径；图标的源视图并不一定是被隐藏层的普通子视图。

TEST23 不再把 lens 父层或选中内容容器作为可隐藏对象。仅识别 lens 内部名称明确的 ClearGlassView、SDFView、SDFElementView，并进一步排除其自身/子树存在 portal、图标、文字、按钮或内容容器的情况。也不触碰底座 platter。只有明确不含内容的背景绘制子层才保存和修改透明度。

关闭开关不替换图标、不复制一份固定图标遮掩问题、不清空原生选择状态、不重设原生变换。原生触摸到达时恢复被保存的装饰透明度，以保留原生按压材质；图标并不是按压时才恢复，因为其可见性没有被这条路径修改。

保守边界：遇到未识别或含内容的内部结构会保留它。若设备上没有可分离的背景子层，胶囊可能仍可见，但不会再次通过隐藏父层把图标一起清除。实际选择背景是否完全消失尚无本地真机证据。

诊断新增 `suppressionPolicy` 与 `suppressedBackdropClasses`，可核对究竟改了哪些背景类，而不是只看一个隐藏数量。

## 3. 四个操作按钮的等距约束

令当前原生坐标中的点赞、评论、收藏、分享图标中心为 y0、y1、y2、y3；先保留已有隐藏行重排结果，再使用：

```text
pitch = (y3 - y0) / 3
点赞偏移 = 0
评论偏移 = y0 + pitch - y1
收藏偏移 = y0 + 2*pitch - y2
分享偏移 = 0
```

只平移两个中间行，行内图标与数字保持原来的相对位置。原有整列缩放继续按相同分享锚点执行，因此分享一行和进度条之间的距离不因这次等距校正而改变；点赞与头像原来的距离也保留。不修改进度条代码。

四行缺失、图标还没绑定、任一行被设置隐藏、坐标不合理时，暂不等距校正，保持既有布局。重复布局读取的是已有映射还原后的原生坐标，不将上次偏移累加进下一次。

## 4. 没有改动的关键实现

`ALB5Options.h`、`ALBTopNative/ALBNativeBarSurface.h`、`AwemeAlbumMorphBar.swift`、`ALBNativeInteraction.h`、`ALBNativeMinimize.h`、评论/场景稳定性修复和授权核心均保持。`ALB5BottomGeometry.m` 只把旧的 TEST17 诊断文字改为 TEST23。

所有逐字节未变文件及明确修改的文件均列在 `validation/test23_preservation_audit.json`。并非整个右侧布局文件都未改；它新增了整行等距偏移，旧持久变换算法和去重保护保留。

## 5. 已做与未做的验证

`validation/test23_results.json` 记录本地检查。完整类型编码回归使用本次日志的 `v20@0:8B16`、`B16@0:8`；还检查 c/B 混合参数、错误参数个数、整型/指针/block 等拒绝路径。等距测试覆盖多个缩放倍率、分享与进度条锚点、图标数字间隔以及多轮坐标还原。

声明桩编译只验证提取代码的语法，不提供 UIKit 运行效果；Linux 上的 arm64/arm64e 标签不能视为 Apple 两架构真实构建。Swift 仅进行语法解析。没有执行真实 UIKit 私有方法，也没有运行真实 Logos 预处理器或完整 Xcode/Theos 编译。Actions 保留 iPhoneOS SDK 显式选择和真实全源码编译步骤。

居中单头像球仍未实现；本轮不通过自制移位或缩放把搜索位头像伪装成居中球。

## Runtime API 参考

实际 Method 与带参数类型的消息调用采用 Apple Objective-C runtime API；这些文档不承诺上述 UIKit 私有接口的跨版本行为：

- https://developer.apple.com/documentation/objectivec/class_getinstancemethod(_:_:)
- https://developer.apple.com/documentation/objectivec/objc_msgsend

UIKit 原生最小化概念参考：

- https://developer.apple.com/documentation/uikit/uitabbarcontroller/tabbarminimizebehavior
