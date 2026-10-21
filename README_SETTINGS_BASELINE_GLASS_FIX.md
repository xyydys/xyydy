# 2.0 auth47 — Settings baseline + Comment-style Liquid Glass

This build keeps the current runtime feature code, including RightNativePressFix and top-button scale/margin support, but resets only the settings page back to the original 2.0 SettingsRefactor structure.

Settings-only changes:
- Restores the original right-side Done/checkmark and original navigation layout. No left Close button.
- Restores the original section order: Basic, Bottom, Gestures, Top + Right, Messages + Comments, Icons, Other, Diagnostics/Authorization.
- Keeps LiquidBar master switch at the very top.
- Keeps a single unified Icon Center for bottom/top/right icons + PNG mode.
- Top/Right material option is no longer a separate Right section. The legacy pure-clear toggle is shown only on iOS 26 as `顶部/右侧按钮纯透明材质（iOS 26）`; iOS 27 does not expose the redundant toggle.
- The gesture-opened half sheet mounts a real runtime `UIGlassEffect` using the same creation/materialization path as CommentGlassDYKiller tint mode: mount the UIVisualEffectView first, then assign the effect on the next run loop.
- The half-sheet appearance is resolved from the live presenting Douyin window/controller before presentation, so dark mode uses the dark system glass branch instead of starting as a Light sheet.
- Clears only sheet/presentation wrapper backgrounds needed for the glass to sample the live Douyin page underneath.
- The original navigation title + Done/checkmark float above a system thin-material blur that fades to transparent below the top edge; there is no opaque blank navigation strip.
- Native pushed Settings entry keeps Douyin's ordinary opaque settings colours.

No right-column geometry, bottom-bar runtime, download, comment runtime, gesture runtime, authorization, or native press forwarding was rolled back.

## Right-side click regression restore

The four right-side Liquid Glass controls keep the real interactive system glass path. On completed tap, the plugin now resolves the current native `AWEFeedVideoButton` directly underneath the glass slot before forwarding the action. This avoids stale recycled-cell bindings without changing right-side geometry or material.
