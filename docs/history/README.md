# AwemeLiquidBar 2.0 auth47 — V3 Right Tap + Opaque Settings

Recovery build with two deliberate constraints:

- Home Like / Comment / Favorite / Share use the exact V3 pass-through touch ownership path. The real Douyin `AWEFeedVideoButton` remains the business touch owner. No later gesture/hit-test/forwarding experiment is layered on top. Press/stretch fidelity is explicitly secondary to reliable native tapping.
- The settings UI is fully opaque. The modal sheet, navigation controller, scroll background, cards and settings text buttons use ordinary opaque UIKit grouped surfaces. Settings contain no active `UIGlassEffect`, `UIVisualEffectView`, clear background, translucent navigation bar or custom glass-sheet code.

The original right-side geometry and all non-settings runtime features from the V3RightTap branch are otherwise unchanged.

## Validation locks

`scripts/preflight.sh` now rejects settings transparency/glass regressions and rejects reintroduction of the later right-action gesture experiment.

---

# Right Native Press Fix

This revision keeps the SettingsRefactor + TopControls scope, but corrects one regression: the Home right-side Like / Comment / Favorite / Share glass controls are real interactive UIKit Liquid Glass buttons again. The v18 pass-through `beginTracking:` mirror is no longer used for these four controls. UIKit owns their real touch-down/move/up sequence; only the completed Douyin business action is forwarded through the existing exact-native path. No custom scale animation is added, and right-rail geometry remains untouched.

# AwemeLiquidBar 2.0 auth47 — Settings Refactor + Top Controls

This branch deliberately drops the experimental bottom/right scaling, right-rail hiding, spacing and vertical-position controls. Those mutations interfered with the native Liquid Glass press/stretch behavior and Douyin's own action-stack geometry.

Kept layout customization is limited to the two Home top buttons:

- independent left/right button scale (70%–130%);
- independent left/right horizontal margin (24–96 pt);
- the middle category capsule remains native-size;
- bottom bar and right action rail geometry are untouched.

The settings page is reorganized into separate **顶部** and **右侧** cards. Compile fixes for the configured-image Swift bridge, descendant helper and triple-tap selector declarations are also included.

Built from the user-validated v18 real-touch/search line, while retaining the
v19 avatar-menu and bounded comment-keyboard background cleanup.

## 2.0 scope

- Settings are reorganized into coherent cards: 基础 / 底栏 / 手势 / 顶部与右侧 / 消息与评论 / 图标 / 其他 / 诊断.
- Typography uses role-based sizes with modest safe-area-width scaling; ordinary labels are no longer blanket-shrunk, and pill/button titles are forced to one line with local shrink-to-fit.
- 图标中心 is a single entry with bottom/top/right sections.
- Removed settings/share page material experiments, bottom text-only mode, chat-input capsule/separation cosmetics, and the highest-quality playback preference/hook.
- Gesture mode 2: double tap opens comments; triple tap toggles Like through Douyin's native interaction-controller path.
- Settings title is `LiquidBar · 2.0`; package/log/export versions are 2.0.
- Home/Friends parity, v18 right-action real touch, top/bottom Search lease, iOS 27 pure-transparent bottom platter, Favorites no-jump, commerce fullscreen suppression, and independent message top/bottom blur distances are retained.
- Dark-mode matching follows DYYY's native-settings principle: inherit Douyin's app-local trait and copy dynamic page/card colours from the native Settings host rather than forcing physical system appearance.
- No new permanent timer/display-link is introduced; removed v19 share/settings-material scans and obsolete playback/chat cosmetic paths reduce unnecessary work.

See `V2_0_SCOPE.txt` for the exact scope.

# Page-fixed Liquid Glass v9

This build retires the iOS 27 video/background luminance-adaptive experiment. On iOS 26/27, Home is a permanent **Dark Liquid Glass** material island; other root pages follow Douyin/app Light/Dark appearance. No iOS 26/27 luminance display link, 4x4 material probe, feed-item luminance burst, or per-button clear-glass contrast screenshot participates in Home rendering.

Message Menu/Search/Add remain at Douyin's native positions. Home right-side actions and top Search now use real interactive system-glass controls; iOS 26 bottom Search uses the direct native `UISearchTab` path. Hidden native top category owners are removed from hit-testing while the custom category group is active. See `PAGE_FIXED_GLASS_V9.txt` for the validation matrix.

# iOS 27 v7 note

This build keeps the **same-window native UITabBarController host on iOS 27+**, but fixes the remaining semantic Light/Dark problem explicitly. iOS 27 system Clear/Tinted/custom-transparency remains owned by UIKit; a low-frequency composited-screen luminance sampler only chooses the Light or Dark trait branch. The sampled style is shared by the bottom bar, registered home glass buttons, and message-page glass buttons. A two-sample hysteresis and median of eight patches reduce flicker from captions and bright avatars. iOS 26 keeps the prior architecture.

Key automatic log records: `LUMA_CAPTURE`, `LUMA_PENDING`, `LUMA_COMMIT`, `LUMA_STABLE`, and `NATIVE_STATE mode=IOS27_LUMINANCE_ADAPTIVE_V7`.

# AwemeLiquidBar 1.0.58 auth47 release

## 1.0.58 release

- Top liquid category group and left quick menu can coexist; the old hard exclusivity was removed.
- Message-page left menu keeps Douyin's proven native hit geometry so taps work reliably; right-side controls remain inset.
- Fullscreen media protection is independent from Comment Glass. With Fullscreen Playback enabled, opening/closing comments no longer shrinks a fullscreen video merely because Comment Glass is off.
- Comment Glass adds a 16–40 pt panel corner-radius control (default 22 pt).
- Settings layout is condensed and reordered by bottom bar → top controls → message page → global material/diagnostics; verbose helper paragraphs are no longer rendered.
- auth47 authorization table (47 unique masked digests).


Release baseline: v22, with the user-validated shop, message-page and same-style
stability preserved.

## Release changes

- Comment glass is simplified to two modes only: **Pure Clear** and **Tint**.
  Tint follows Douyin's own app/window Light/Dark appearance. All comment
  video/avatar/corner probe selection, text-color rewriting, attributed-string
  rewriting and icon-color scanning have been removed.
- Message page returns to the WCG **blur-only** edge effect. The opacity/fade
  mask is removed completely. A new **Message edge blur distance** slider
  (60–180 pt, default 118 pt) controls the blur reach without modifying scroll
  geometry. Message header Menu/Search/Add controls are moved inward together
  with Douyin's real artwork/hit owners, so plate and icon stay aligned.
- Top menu/search are moved inward together with their original Douyin owners.
  Turning top glass off restores Douyin's original controls and positions.
- Top search no longer disappears merely because bottom search is enabled.
  A separate **Hide top-right search button** switch now controls it.
- The top liquid category group and the left quick-menu can coexist.
- Top category text reuses Douyin's native text colors instead of forcing white
  or plug-in-selected foreground colors.
- The validated v22 shop behavior, same-style/music glass stability, profile
  bottom cleanup and photo/live-photo geometry behavior are intentionally left
  unchanged.

## Build

Run `scripts/preflight.sh` before pushing to the Xcode 27 GitHub Actions
builder. This package does not auto-install.


## 1.0.58 release-fix (auth46)
- Removed the false SameStyle glass on the wide caption stack; only the right action rail may own the music glass.
- Message header menu now moves its real interaction owner; AWEIMSlotIntrinsicView is suppressed at lifecycle level.
- Comment input/emoji strip clears translucent outer backdrop while retaining the rounded input field.
- Restored two masked authorization digests (46 total); no plaintext account identifiers are stored.
## 1.0.58 pre-release fullscreen comment-dismiss correction

Before the first public 1.0.58 build, the native-comment/fullscreen path was
corrected again. Comment controllers no longer suspend fullscreen geometry when
comment glass is disabled, and a short generation-guarded post-dismiss recovery
restores only media hosts that were truly fullscreen before opening comments.
This targets the 956→873 late relayout that can expose an 83pt black strip after
closing native comments.


## 1.0.58 auth47 authorization update
- Added 1 authorized member as a salted SHA-256 digest split into a fresh XOR mask/value pair.
- Authorization rows: 46 → 47.
- No plaintext account identifier is stored in the project.

## v15.1 compilefix
- Compile-only correction in `MessageTopGlassV3.xm`: the edge-blur diagnostic log now uses the existing `topDistance` / `bottomDistance` variables instead of the removed legacy `distance` identifier.
- No runtime behavior or v15 feature logic changed.

## v16 exact action-forward repair

Based on the user-tested v15.1 / PageFixedGlass v9 line. This revision leaves the
native Liquid Glass button geometry and UIKit press/stretch configuration untouched.
It replaces only the final business-action forwarding step for Home right actions
and Top/Bottom Search with exact forwarding through Douyin's original recognizer,
original target and original selector. It also adds early top-category label
suppression and a missing-search-icon recovery guard.

New diagnostic markers: `EXACT_NATIVE_CANDIDATE`, `EXACT_NATIVE_FORWARD`,
`ALBUM_MORPH_SEARCH_EXACT`, `TOP_SEARCH_VISUAL_REPAIR`.


## v18 real-touch lease + Home/Friends parity

- Based on the v17/PageFixedGlass-v9 selective-fix line.
- Home and Friends now share the same bottom-bar and Home chrome visual policy; Shop is excluded from the Friends rule. When the iOS 27 Clear Glass switch is enabled, Home + Friends + Profile use the clear platter path.
- Right-side Like/Comment/Favorite/Share no longer synthesize Douyin business actions. Their original `AWEFeedVideoButton` remains the real UIKit touch owner; the visual Liquid Glass control only mirrors the same physical touch for press/stretch feedback.
- Top and bottom Search share Douyin's one real Search recognizer through a bounded touch-time lease instead of calling its private target while the recognizer is still Possible.
- Keeps the v17 clear-platter implementation, iOS 27 vertical-offset geometry correction, compact settings typography, Favorites no-jump repair, commerce fullscreen suppression, split message top/bottom blur distance, and auth47.

Run `scripts/preflight.sh` before pushing.

## Zero-comment emoji-row transparency fix
When a post has zero comments, Douyin can mount the composer/emoji row through an empty-state footer and an additional UITextEffectsWindow sibling backdrop. This build adds a geometry-scoped cleanup for those carriers and re-applies it during the mini-emoticon panel's delayed materialization, without changing the right-button experiment or settings page.
