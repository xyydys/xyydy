# 2.2 shared-material repair

- Bottom bar keeps the native iOS 27 material when **Clear is off**. No synthetic alpha/tint material is installed, so UIKit/system transparency remains authoritative.
- `bottom.clearMode` still installs the project's single untinted `UIGlassEffectStyleClear` recipe.
- The top category group now uses the **same material policy** as the bottom bar. It remains a compact native `UITabBar`; only its geometry is smaller.
- On iOS 27, the top native bar receives the same resolved Light/Dark trait used by the bottom bar.
- Removed the always-Clear top path. Top Clear now follows `bottom.clearMode` instead of being forced on.
- Removed the `top.bodyGlass` experiment and its extra `UIVisualEffectView`/native-layer hiding path, preventing double-glass stacking.
- Changing the bottom Clear switch immediately reapplies the material to both bottom and top bars.

No top height, corner geometry, native selected lens, `SelectedContent`, touch tracking, or press/stretch interaction code is changed.
