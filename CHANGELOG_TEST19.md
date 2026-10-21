# 2.0.10 TEST19 — 右侧持久变换修正

基于实际 TEST18 arm64e 二进制、同包 link map 与 062238.ips。

运行逻辑修改仅涉及 `ALB5RightLayout.h`、`Tweak.x` 的右侧调用点及 `ALBNativeInteraction.h` 的四处几何辅助调用。删除普通布局的还原/重套循环；使用持久记录与输出差异检查；新增无写入的坐标换算、相同原生几何输入去重、非活跃场景门控及过期角色记录释放。

底栏/顶栏材质、底栏收缩驱动、原生触摸处理、授权核心保留。其他主要变化为版本号、诊断报告、测试和构建辅助脚本。

新增：`Tests/right_persistence_test.c`、声明桩语法检查、`scripts/check_right_persistence.py`、`scripts/symbolicate_linkmap.py`。新增测试已接入工作流；现有 UUID/link map 导出继续保留。

版本号、设置页、会话日志、推送分支和 Actions 产物统一为 TEST19。

尚未完成完整 Xcode 编译和真机验证；未宣称底栏收缩及顶栏左按钮延迟问题已解决。详见 `DIAGNOSIS_TEST19.md` 与 `VALIDATION.md`。
