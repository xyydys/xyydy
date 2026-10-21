# 2.0.10 TEST22

Baseline: the TEST21 source whose runtime the user reports no longer freezes.

- Restore exactly one top-search TouchUpInside receiver on the native icon wrapper. Original search recognizers stay with their original business owner; remove the old top-wrapper gesture lease.
- Make the original menu and search owners equally routing-only rather than leaving the original search background visible beneath the native carrier. Keep the shared material recipe unchanged.
- Native selection and a passive end-of-touch observer deliver the same physical tap once. The observer neither recognizes nor prevents/cancels native touch; drag/cancel/long-touch fallback is rejected.
- Add top.sideSelectionCapsule (default YES) in both top-settings entrances. OFF affects only recognized idle selection decorations, not the native selected tab or the platter. Restore cached decoration opacity before native press. Unrecognized trees stay untouched.
- Expand minimization dispatch to already-owned UIKit bar providers/minimization objects. Read only bounded object ivars; accept exact BOOL setter signatures; keep UIKit's duration and layout. No synthetic scroll, manual circle, geometry morph or busy-loop fallback.
- Diagnostics show live native state, candidate endpoint method signatures, request acceptance, top action targets and recognized idle-lens count.
- Preserve TEST19/21 right-side geometry, watchdog protection, bottom Swift scheduling, material source and authorization core.

Known limitation: this revision does NOT implement a separate single centre-avatar native minimization layout. The existing avatar option still changes the resident native search item's artwork. Native provider availability and actual collapse need device verification; successful dispatch is not proof of a visible transition.
