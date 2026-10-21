# 2.0.10 TEST42 FIX7

Built on FIX6 (`claude/test42-fix2-bugfixes`). Device feedback on FIX6 and the
auth47 diagnostic export drove each change.

## Double-tap panel Share / Favorite
- Device log: identity is now correct (boundKind 3 = 收藏, 4 = 分享), but the
  forward stopped at `stage=no-business-target`. The button's tap recognizer
  carries only telemetry targets, and `allTargets` has no TouchUpInside target.
- The physical tap works because our glass button mirrors the real touch into
  the native AWEFeedVideoButton, whose UIControl tracking dispatches its
  handlers. Those handlers are UIAction or nil-target actions, which
  `allTargets` does not list.
- Forward now enumerates handlers with `enumerateEventHandlers:` (telemetry
  excluded) and dispatches with `sendActionsForControlEvents:`. A bounded
  ancestor-recognizer route (right column only, below the element stack) runs
  before accessibility.
- The panel miss record now includes handler count, events and whether the
  class overrides touch, tracking or sendAction.

## Center compact mode (group shrink)
- The left group is one rigid transform of its platter. Its native children
  (glyphs, titles, badges, selection lens) are no longer hidden behind copies;
  they travel with the platter. This removes the reveal "jump" at the end of
  expansion.
- The glass platter does not dim with alpha on device, so the group now
  shrinks to 5% (`finalGroupScale` 0.05) and is hidden at the endpoint.
- Artwork copies no longer carry a gradient mask, which forced offscreen
  rendering on every frame.

## Top middle group (separated Clear build)
- At rest the selection is a plain filled capsule, not glass. The Clear glass
  lens appears only while the selection travels, then settles into the fill.
  In-flight state is bounded by the spring's own duration.
- The visible window follows the selection (the native `ALB6TopWindowStart`
  policy), so entering on 推荐 shows 推荐.
- Titles use the native fixed 14 pt semibold, never auto-shrunk.
- Douyin category badge and red-dot views near the hidden titles are hidden with
  the existing restorable indicator marker. The Clear group no longer covers
  them.

## Not changed
- Native (non-center) collapse overshoot: the export had no native-mode
  collapse, and the code shows no geometry write on that path. A repro capture
  in native mode is needed.
