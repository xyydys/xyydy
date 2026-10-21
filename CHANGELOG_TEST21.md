# 2.0.10 TEST21 — iPhoneOS sysroot preflight fix

TEST20 failed before Theos installation because `scripts/test_logos_boundary.py` invoked Clang with an iOS 15 target triple while Clang still inherited the macOS 27.0 sysroot. The source code had not reached the real build step.

TEST21 changes only the boundary-regression compiler selection plus version/build metadata. On Xcode/macOS CI the regression now resolves the active `iphoneos` SDK with `xcrun`, resolves Clang from the same Xcode toolchain, and passes the iPhoneOS SDK explicitly with `-isysroot`. CI requires that iPhoneOS mode with `ALB_REQUIRE_IPHONEOS=1`; on non-Apple development hosts it performs a host-only syntax fallback and labels that result as not being an Apple SDK compile.

The TEST20 runtime implementation, including the TEST19 right-side persistence fix, top/bottom glass material implementation, native collapse driver and touch routing, is otherwise retained.
