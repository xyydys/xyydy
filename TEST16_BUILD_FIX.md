# TEST16 build fix

The TEST15 Apple-SDK typecheck exposed two real compile errors:

1. `UIView.animate(changes:completion:)` was not a UIKit overload and resolved toward SwiftUI. TEST16 explicitly calls `UIKit.UIView.animate(withDuration:...)`, using `CATransaction.animationDuration()` rather than a hard-coded plugin speed.
2. `objc_getAssociatedObject` / `objc_setAssociatedObject` require `UnsafeRawPointer` keys. TEST15 incorrectly passed `Selector`. TEST16 uses stable file-scope `UInt8` keys and their addresses.

This is a compile-fix pass. It does not claim device behavior is validated until the Xcode27 workflow completes and the dylib is tested on-device.
