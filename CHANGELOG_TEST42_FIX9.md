# 2.0.10 TEST42 FIX9

## Top group Clear (experiment, setting "中间分组 Clear 材质（实验）", default on)
- FIX8's `TOP_GLASS_STRUCTURE` shows that iOS 26.6 exposes no glassEffect or
  effect view on the native top group. The glass is drawn in layers UIKit
  owns without a view.
- Nagram (Telegram) does not modify UIKit's bar. It draws its own
  `UIGlassEffect(style: .clear)` and sets the tint alpha to 0.
- The same idea here: only view-less material layers (`_UIMultiLayer`,
  backdrop layers, layers with background filters) directly under the top
  `UITabBar` and its platter are hidden. One untinted public Clear glass sits
  at the back of the platter, as a capsule.
- The native lens, selection capsule, tab buttons and labels are view-backed
  and untouched.
- Turning the setting off restores every hidden layer and removes the glass.
- Diagnostics: `TOP_LAYER_TREE` (once per bar) and `TOP_CLEAR_GROUP` (which
  layers were hidden).
