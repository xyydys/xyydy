# TEST17 — native runtime guard fixes

See DIAGNOSIS_TEST17.md for evidence and limitations. This revision changes no material file and does not resurrect the TEST10 custom collapse animator.

1. Action-only top carriers leave native item selection to their controller. Their provider fitting has a finite recheck budget; recursive layout is rejected before super.
2. Native minimize binding uses one public API on the resident proxy only. Private observer rebuilding and deferred forced refresh are removed.
3. Explicit minimized requests are coalesced outside the originating layout/lifecycle callback and cannot synchronously reenter. Stale generations cannot overwrite newer requests.
4. Layout refresh performs observation only; search artwork updates use a publication cache rather than the native getter's image identity.
5. Both top and bottom plugin-owned chrome are excluded from host-route lifecycle recovery. Native hit routing and menu dispatch keep their reentrancy barrier active through completion.
6. Version, push branch, Actions artifact and settings identify TEST17. Deployment target remains iOS 15.0; the iOS 17.4 primary-action guard remains.
