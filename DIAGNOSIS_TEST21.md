# TEST21 build diagnosis

## Observed failure

The GitHub Actions run stopped in `Static preflight before dependencies` before Theos installation and before `make package`.

The failing regression reported, for both arm64 and arm64e:

`clang: error: using sysroot for 'macOS 27.0' but targeting 'arm64-apple-ios15.0'`

This was a test-harness/toolchain mismatch, not a new Tweak.x compiler error. The regression selected a bare `clang`, supplied an iOS target triple, but did not select the iPhoneOS SDK. Xcode therefore paired the iOS target with its default macOS sysroot and correctly rejected the invocation.

## Repair

`scripts/test_logos_boundary.py` now:

1. Detects `xcrun` on Apple CI.
2. Resolves `xcrun --sdk iphoneos --show-sdk-path`.
3. Resolves Clang from the active Xcode toolchain with `xcrun --sdk iphoneos --find clang`.
4. Compiles arm64 and arm64e boundary fixtures with `-target <arch>-apple-ios15.0 -isysroot <iphoneos-sdk>`.
5. CI sets `ALB_REQUIRE_IPHONEOS=1`, so it fails rather than silently falling back if iPhoneOS toolchain resolution ever breaks.
6. Uses a clearly-labelled host-only syntax fallback only on non-Apple local development hosts.

The existing workflow still performs the separate real `logos.pl` preprocessing step after Theos is installed and then performs the actual rootless package build.

## Scope

No runtime layout/material/minimize logic was changed for this repair. Tweak.x changes are limited to TEST21 version text; the functional source remains TEST20-equivalent.
