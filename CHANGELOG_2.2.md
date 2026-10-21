# AwemeLiquidBar 2.2 (auth50)

Built on TEST42 FIX9 (`claude/test42-fix2-bugfixes`).

## Authorization
- 3 accounts added as salted SHA-256 digests with fresh XOR masks
  (47 -> 50 rows, plus their fast tags).
- All previous entries and the authorization logic are unchanged.
- The authorization-core hash lock is updated to match.

## Top group
- The failed FIX9 experiment (hiding UIKit material layers) is removed.
- iOS 26: the middle native group is no longer pinned to the bottom bar's
  Light/Dark. Like the side Clear buttons, it is left Unspecified, so UIKit
  adapts its glass to the content behind it.
- iOS 27: the existing top platter `glassEffect` Clear (ALB43) applies. On
  iOS 27 the platter exposes `glassEffect`.

## Bottom bar Clear mode (iOS 27)
- The switch had been a no-op since TEST6: `AWEApplyIOS27UntintedClearPlatter`
  had been reduced to an empty function.
- The v10/v11 DYKiller path is restored: Clear glassEffect on the floating
  platter and search views, plus the shared glass container spacing.
- With the switch on, every page uses Clear. With it off, the originals are
  restored and the normal style policy applies.
- Toggling the switch relayouts the bar.

## Native minimize under bottom scale
- Symptom: after a native collapse or expand, the bar ends larger by exactly
  the scale factor. The initial layout is correct at any scale.
- Once UIKit's own animation completes (the request's `CATransaction`
  completion), the bar takes one normal layout pass. This only happens when
  the scale is not 1.
- New `NATIVE_SCALE_TRACE` collection records the canvas, root, bar and its
  children, in local and window geometry, at request, animation-complete,
  +0.30 s and settled. Comparing a scale=1 run with a scaled run shows where
  they diverge.

## Version
- 2.2; artifact `AwemeLiquidBar-2.2-auth50-Xcode27-rootless`.

## 2.2 update 2
- The Friends feed (second tab, when it is not Shop) now keeps Home's Dark
  bottom material in Light mode. Previously only Home did, so Friends showed
  a light bar.
- The floating reproduction-capture overlay is removed at the user's request.
  - Its settings switch and window classes are gone.
  - The recorder C API is an inert stub, so existing call sites compile and do
    nothing, and it clears the old enabled preference.
  - The everyday interaction diagnostic log and its export are unchanged.
- iOS 27 Clear: on device the top group and the Clear bottom mode stay tinted.
  - New `BOTTOM_CLEAR_COVERAGE` and `TOP_CLEAR_COVERAGE_IOS27` collection is
    written once per bar and state.
  - It records every nearby glass-bearing view: class and superclass, whether
    it exposes `glassEffect` or is an effect view, and the effect's class,
    style, tint and interactive flag.
  - This shows where iOS 27.x now keeps that material.

## 2.2 update 3: iOS 27 Clear (device export, iOS 27.0 24A5380h)
- The export had no `BOTTOM_CLEAR_COVERAGE`, `TOP_CLEAR_COVERAGE` or
  `TOP_CLEAR_COVERAGE_IOS27` record at all.
  - Both Clear paths searched only the bar's own subtree: two ancestors up for
    the bottom bar, the bar itself for the top.
  - The coverage writer skipped empty results.
  - So on iOS 27.0 nothing was found and nothing was replaced. The floating
    glass is no longer inside that subtree.
- Glass owners are now located in the bar's window by position:
  - a platter, search or tab-bar `glassEffect` owner, or
  - a glass effect view with a TabBar ancestor,
  - overlapping the bar area, excluding lens and selection renderers.
- Owners are cached per bar. A re-scan happens only when the cache breaks,
  and at most every 0.5 s.
- The same helper handles the top native group (always Clear on iOS 27) and
  the bottom bar (the Clear switch). Effect views and `glassEffect` owners
  keep their originals for restore.
- It is triggered from our own controller and group layout as well, not only
  from `UITabBar -layoutSubviews`.
- Coverage is written once per bar and state. If nothing is found after 20
  passes it is still written, with the bar's ancestor chain.

## 2.2 update 4 (iOS 27.0 export 2)
- Window-wide scan result: `owners=0` for both bars. On iOS 27.0 there is no
  `glassEffect` owner and no glass effect view anywhere near the bottom or top
  bar, so replacing an effect cannot work there. Only the structure remains
  unknown.
- New `BOTTOM_CLEAR_COVERAGE_STRUCTURE` and `TOP_CLEAR_COVERAGE_IOS27_STRUCTURE`
  records. They list up to 70 views with class chain, layer class, sublayer
  classes and filters, and glass, effect or style properties.
- Top group appearance: with the iOS 26 rule (Unspecified) the group stayed
  light on iOS 27.0. It is pinned to the bottom bar's style again on iOS 27
  (Dark on Home and Friends). iOS 26 keeps the adaptive rule.

## 2.2 update 5: top group body glass (user-supplied DYKiller write-up)
- The write-up's structure: a static body glass (`UIGlassEffect`, tint nil,
  `interactive = NO`, no touches) sits behind the native tabs. Selection lens
  and SelectedContent keep their own interaction, and nothing is rebuilt.
- The top group now follows that. `UIVisualEffectView` body glass at the
  back of the group, with a capsule shape.
  - Style comes from the same setting as the side buttons: Regular when tinted
    glass is on, Clear otherwise.
  - The view is left Unspecified so UIKit adapts it, like those buttons.
- The device dumps show the native platter draws its own glass in view-less
  UIKit layers (no `glassEffect` owner, no glass effect view). Only view-less
  layers directly under the bar and platter are hidden, so two glasses do not
  stack.
  - Nothing view-backed is touched.
  - Lens, SelectedContent, ContentView, DestOut and tab buttons are skipped
    entirely.
- Setting: 顶栏 → "中间分组独立玻璃（实验）", default on. Turning it off restores
  every hidden layer and removes the glass.
- `TOP_BODY_GLASS` records the style and which layers were hidden.

## 2.2 update 6 (iOS 27.0 export 3: top group overlap)
- The dump shows the top platter is `UIKit._UITabBarItemPlatterView`.
  - All five of its sublayers are view-backed.
  - No material layer exists without a view, so `TOP_BODY_GLASS hidden=0`.
  - The body glass therefore stacked on top of the native glass, which is the
    overlap in the screenshot.
- The body glass is now off by default. The setting stays as an experiment.
- Style fix: the trait override is now also set on the group's `UITabBar`
  itself, the way the bottom bar does. Before, only the group, controller and
  controller view were pinned, so the platter kept the window's light traits.
- The glass-owner scan no longer skips the small rail `UIScrollView` the top
  bar sits under. Only large scroll views (the feed) are skipped.
- New `..._PLATTER_API` record. It lists the glass, effect, material, backdrop,
  tint, clear and style properties, methods and ivars on the platter class
  chain, since no `glassEffect` selector answered on 27.0.
