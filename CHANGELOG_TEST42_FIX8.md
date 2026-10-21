# 2.0.10 TEST42 FIX8

Built on FIX7 (`claude/test42-fix2-bugfixes`). The auth47 export from the FIX7
build drove every change.

## Center-ball compact scheme deleted (user request)
- Removed `ALBCenterMorphSurface.swift`, `ALBCenterMorphGeometry.swift` and their
  test and CI step.
- Removed all centered paths from the controller, the ObjC capture, lease and
  cover exports (`ALB24`/`ALB27`/`ALB32`/`ALB33`/`ALB36`/`ALB37`/`ALB42`) and
  the settings rows.
- `ALB15CompactStyle()` is now always native.
- The Repro helpers and the iOS 27 bottom material helper moved to
  `ALBBottomSupport.swift`.
- Checks that guarded the deleted contracts now assert that the scheme stays
  gone. Every unrelated guard is kept.

## Native collapse overshoot
- Log: the first collapse starts from rest. Every later collapse was issued
  about 0.55 s after an expand, while UIKit's ~0.8 s minimize spring was still
  running. Reversing that spring overshoots, then springs back. The first
  collapse never overshot.
- A reversal now waits for UIKit's own animation to finish, using the
  `CATransaction` completion wrapped around the request (event-driven). A
  bounded 1.2 s staleness guard applies. A reversal withdrawn meanwhile is
  never issued.
- CI's extracted-queue fixture adds both scenarios.

## Double-tap panel Share / Favorite
- FIX7 log: `AWEFeedVideoButton` has no control handler (`handlers=0`) and
  overrides `touchesEnded:withEvent:`. Douyin fires the action from its own
  touch handling. The physical tap works because the glass button mirrors the
  real touch into it.
- For right-column actions the panel now delivers one tap (began, then ended)
  at the button's centre to those same touch methods. It is cancel-safe on
  exception.
- New once-per-class `RIGHT_TOUCH_OWNER` diagnostic: the button's methods and
  object ivar types, for tracing if still unresolved.

## Top group
- The FIX6/FIX7 separated glass/capsule build is removed. The native UITabBar
  group (the XHS port) is shown again, with its native selection lens and
  capsule.
- New `TOP_GLASS_STRUCTURE` special collection in the ordinary export, once per
  bar. It records class chains, layer classes, filters and glass/effect/material
  properties of the native top bar. This shows where iOS 26.6 draws that glass,
  since it has no platter `glassEffect` (that exists on iOS 27).
- `TOP_CLEAR_COVERAGE` is written without a repro recording too.
- Category badge hiding from FIX7 is kept.
