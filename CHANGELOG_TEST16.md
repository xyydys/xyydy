# 2.0.10 TEST16 — 编译兼容修复

基线：本次对话中的 `AwemeLiquidBar_2.0.10_TEST15_Native_project.zip`。

- 为顶栏动作按钮的 `performPrimaryAction` 增加 `if (@available(iOS 17.4, *))`，位于实际执行该方法的 dispatch block 内。
- 成功派发原生菜单主动作后立即 return，防止同次操作再次发 TouchUpInside；低版本不调用新接口。
- 最低部署目标仍为 iOS 15.0，编译警告策略未放宽。
- 添加提取源码块的局部可用性编译测试及负对照；Actions 提前用真实 SDK 检查整个顶栏 `.m`。
- 包、设置页、诊断、分支、工作流及产物名称统一为 TEST16。

未改：底栏材质、原生收缩/展开请求、头像玻璃路径、顶栏两侧与中间的载体/材质/布局、右侧布局、授权。TEST15 未实现两套独立原生收缩外形的限制不变。

本包不是已编译或真机验收的版本。已运行的检查及其实际输出见 validation/current_results.json。
