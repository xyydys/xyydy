# 2.0.10 TEST20 — Logos 行尾截断编译修复

运行源码只改 `Tweak.x`：将 `AWEElementStackView.layoutSubviews` 快速返回分支中的 `%orig; return; }` 分成三行，保留原来的条件、原生调用、return 和括号；另更新三处版本文字。

TEST19 其余运行源码逐字不变，包括右侧持久变换修复、顶栏/底栏材质、底栏原生收缩驱动、按压处理与授权。

新增 `check_logos_boundaries.py`：扫描全部三份 Logos 输入，禁止 `%orig` 后在同一行继续执行语句或闭合代码块；在 Actions 中检查真实 Logos 展开产物的函数边界。

新增 `test_logos_boundary.py`：提取生产方法，按当前上游行尾替换行为建立局部编译用例。arm64、arm64e 的修复后用例通过；重新拼回旧行的负对照均复现 `function definition is not allowed here`。

版本、推送分支和 Actions 产物统一为 TEST20。未完成完整 Apple SDK 编译或真机运行，不宣称卡死、右侧位置或收缩表现已通过验收。
