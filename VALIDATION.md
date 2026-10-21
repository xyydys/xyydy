# 当前版本 TEST30 CAPTURE FIX1

本轮以 `validation/test30_capture_fix1_results.json`、`validation/test30_capture_fix1_audit.json` 为准。修正用户 SDK 日志报告的类型/可用性错误，新增零评论表情背景采集。真实 SDK 和完整构建仍需 Actions，本机未完成。以下为历史记录。

# 当前版本 TEST30 CAPTURE

本轮以 `validation/test30_capture_results.json`、`validation/test30_capture_audit.json` 和 `CHANGELOG_TEST30_CAPTURE.md` 为准。此版采集 TEST29 的真机故障，未宣称已修复。本地没有 Apple SDK/Swift/测试手机。以下为历史记录。

# 当前版本 TEST29

本轮以 `validation/test29_results.json`、`validation/test29_preservation_audit.json` 和 `CHANGELOG_TEST29.md` 为准。源检查和 C 几何回归通过；本机无 Swift/Apple SDK/测试手机，未完成 iOS 编译与真机验证。以下均为历史记录。

# 当前版本 TEST28 FIX1

本轮以 `validation/test28_fix1_results.json`、`validation/test28_preservation_audit.json` 和 `CHANGELOG_TEST28_FIX1.md` 为准。TEST28 原报告保留为历史记录；FIX1 没有在本机完成 Apple SDK 编译或设备验证。以下为历史版本记录。

# 当前版本 TEST27 FIX1

本次检查以 `validation/test27_results.json` 和 `CHANGELOG_TEST27.md` 为准。以下内容属于原 TEST24 及更早版本的历史记录，不代表本次完成了其编译或真机测试。

# TEST24 validation

Environment: Linux Clang + Swift. No Apple SDK, Theos, actual UIKit runtime or device. Full Xcode/Theos build **not run**.

The machine-readable current results are in `validation/test24_results.json`. All 20 recorded command-level checks passed: source/package and current-mode contracts, source-extracted queue logic (8 mock dispatcher scenarios), the retained watchdog frame-gate fixture (48 combinations plus ordinary pass-through), right-side persistence/spacing tests, 9 C test executables, Clang declaration-stub regressions, Swift syntax parse, and shell syntax.

The 9 C executables include legacy policy/math regressions; passing them does not demonstrate the new UIKit behavior. The legacy lens model test is not used as evidence that auxiliary controls render correctly.

`validation/test24_preservation_audit.json` binds unchanged source files and deliberate edits to the actual uploaded TEST23 source bytes. Material, right-layout and authorization invariants remain checked. Previous TESTxx reports are historical only and do not describe this build.

CI still resolves the iPhoneOS SDK explicitly, checks the entire top carrier with the real SDK for arm64/arm64e at iOS 15 deployment target, executes real Logos preprocessing with generated-method boundary checks, then performs the complete Theos package build. No warnings-as-errors gate was disabled to hide failures.

Not verified: acceptance of actual host scroll binding by UIKit, visual material equality, touch/stretch, real column routes, centered-glass fusion, comment-video restoration on-device, host bottom backgrounds, runtime performance and watchdog stability.
