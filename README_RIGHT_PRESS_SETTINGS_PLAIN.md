# 2.0 auth47 — right press passthrough + plain Settings

This build focuses on the interaction root cause instead of forwarding taps.

## Right Like / Comment / Favorite / Share
- Douyin's real `AWEFeedVideoButton` remains the physical hit-test and business-action owner.
- The visible Liquid Glass overlay is permanently non-interactive.
- Restores the previously proven 1.0.50a press-feedback mechanism: a zero-duration `UILongPressGestureRecognizer` is installed on the real Douyin button with `cancelsTouchesInView = NO`, no touch delays, and simultaneous recognition allowed.
- That recognizer changes only the visual glass button's highlighted/stretch transform. It never forwards the business action.
- The later `beginTracking/continueTracking/endTracking` touch mirror is disabled so it cannot temporarily make the overlay interactive.

Result: the original Douyin touch/recognizer chain and the visible Liquid Glass deformation run in parallel instead of competing for hit testing.

## Settings
- Restored to ordinary UIKit grouped Settings appearance.
- No settings-page `UIGlassEffect`, transparent glass backdrop, glass cards, or custom sheet material.
- Keeps the original right-side Done/checkmark and existing settings/features.
