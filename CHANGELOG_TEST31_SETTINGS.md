# TEST31 SETTINGS

## 范围与证据

用户确认全屏为 Clear，半屏观感不是 Clear；本版不把之前一次白色容器的过渡记录认定为持续全屏不透明。
设置半屏与全屏日志指向同一个原生 sheet、同一个外层玻璃。外层玻璃尺寸随 detent 正确更新，但没有设置自己的 cornerConfiguration，layer.cornerRadius 始终为 0。它只被外层裁切，光边无法据此保证与实际面板圆角一致。
未确认半屏独有第二层材质，不扩大视图遍历或清除范围。

## 代码

ALBSettingsGlass.h：
1. 给自有原生设置玻璃添加 UIKit container-concentric 的独立四角配置，使用父容器的形状；不设置固定半径，也不更改系统 sheet 的裁切/圆角。
2. 在首次 viewDidAppear 和 sheet selected-detent 回调中，按 detent + bounds 去重，只有 medium 重绑一次现有 Clear 材质。此项针对初建时全窗口尺寸到原生半屏尺寸的变化，原因仍待真机确认；large 不触发刷新。
3. 专项采集增加玻璃/容器 effectiveCornerRadii，便于直接对照光边与裁切轮廓。

ALBGlassMaterials.h、底栏、顶部按钮、清屏、评论、商城、双列代码保持原样。Tweak.x 只更新版本标签。

## 检查范围

本地源码检查与改动范围审计见 validation/test31_settings_results.json 和 validation/test31_settings_audit.json。
实际 iPhone SDK 类型/可用性检查由现有 Tests/Support/repro_capture_compile.mm 覆盖整个设置实现，在 Xcode 27 Actions 上执行。本地没有 Apple SDK，未宣称编译通过或真机通过。

真机关注：首次打开半屏、半→全→半，四角光边与面板轮廓，顶部标题滚动渐隐及分区磨砂。

## API 依据

Apple UICornerConfiguration 文档（Objective-C）：https://developer.apple.com/documentation/uikit/uicornerconfiguration-c.class
使用 configurationWithRadius: 与 containerConcentricRadius；没有沿用此前不存在的 .fixed 配置工厂。

## 新上传日志的保留结论

- 双列：已识别 AWEFeedNewDoubleColumnViewController 并切换为 double-column，不能归因为入口未识别。
- 清屏：多次命中 UIButton、hasMenu=1 且收到触摸，但未见菜单呈现；紧急恢复可退出。后续应检查命中后的菜单触发链。
- 商城：内容高度873、窗口956，底部83点是待核对的容器空隙。
- 0评论：专项扫描54个节点、清理0个，记录内footer已透明；尚未捕获可明确归因的残留表情背景。
- 新底栏：完成回调均被接受，但通用快照全触及1100节点上限，不能把未记录的子视图当成不存在。
这些项目本版仅分析，不宣称已修复。
