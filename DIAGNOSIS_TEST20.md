# TEST20 编译诊断与修复范围

## 1. 截图对应的具体位置

TEST19 的 `Tweak.x:21084`：

```objc
[objc_getAssociatedObject(alb6Stack, &layoutInProgressKey) boolValue]) { %orig; return; }
```

原始源码的括号数量是平衡的；仅检查原文件括号平衡，不能发现这次故障。

当前 Theos Logos 上游 `bin/logos.pl` 的 `%orig` 分支以 `range($patchStart, length($line))` 定义替换范围，替换覆盖该指令到行尾的内容。因此，这一行尾部的 `return; }` 会随替换丢失。Clang 在后面的 `traitCollectionDidChange:` 处发现“函数内又定义函数”，随后在 `setHidden:`、`setAlpha:`、其他 hook 方法处连续报错。这与用户截图的第一条及后续错误顺序相符。

上游源码依据：https://github.com/theos/logos/blob/master/bin/logos.pl （`%orig` 分支，`range($patchStart, length($line))`）

语法说明：https://theos.dev/docs/logos-syntax （`%orig` 调用原方法）

这不是给源文件再加一个多余的 `}`。源文件原本平衡，修复是在保留所有语句的前提下，把不能与 `%orig` 共行的后续内容移到独立行。

## 2. 修复

```objc
[objc_getAssociatedObject(alb6Stack, &layoutInProgressKey) boolValue]) {
    %orig;
    return;
}
```

实际工程保留邻近代码的原缩进。本次没有改变应用活跃状态判断、布局重入锁、原生方法调用次数或 TEST19 的右侧几何函数。

## 3. 检查与证据边界

`check_logos_boundaries.py` 扫描三份输入共 272 个实际 `%orig` 调用。TEST19 仅第 21084 行触发该项检查；修复后的三份输入通过。

`test_logos_boundary.py` 从生产源码提取 `layoutSubviews` 和紧接着的 `traitCollectionDidChange:`，仅模拟上游 `%orig` 行尾替换与函数签名生成，使用最小 Objective-C 声明执行 Clang 检查。两个目标均为 iOS 15，架构为 arm64 与 arm64e。修复后的两项通过；旧行负对照的两项均复现截图中的 `function definition is not allowed here`。

以上局部测试没有运行真实 Logos，也没有 Apple SDK，不等同于完整构建。当前环境直接下载 Logos 工具失败。Actions 已增加对真实 Logos 三份展开输出的结构检查；之后仍由原本的 Theos `make package` 执行完整编译。

**没有完整 Xcode 构建结果或 TEST19/20 真机结果，不能宣称运行时卡死已消除。** TEST19 之前无法生成安装包，所以右侧持久变换修复尚未得到设备验证。本次仅解除已定位的编译阻断。

## 4. 不变内容

除 `Tweak.x` 的换行与三处版本文字，所有既有 `.h/.m/.xm/.swift` 文件和 C 测试文件均与交付的 TEST19 逐字相同。`Tweak.x` 逆转本次换行与版本文字后，其 SHA-256 必须回到 TEST19 的原值。详见 `validation/test20_preservation_audit.json`。

新增文件仅用于验证及说明，构建配置只增加检查、更新版本、分支及产物名。最低部署目标、架构、优化选项、材质参数和授权表未改。
