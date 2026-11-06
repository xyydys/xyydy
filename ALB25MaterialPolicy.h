#pragma once
#include <stdbool.h>
// 2.5 MATERIAL3 persisted values.
//   Regular  0: UIGlassEffect Regular. Bars keep UIKit's own glass unless a tint is chosen.
//   Clear    1: UIGlassEffect Clear, black tint only in Dark (tinted Clear recipe).
//   Native26 2: retired 2.3 value, read as Regular (Regular already keeps the native bar).
//   Nagram   3: UIGlassEffect Clear, never a tint (Telegram/Nagram's recipe).
enum { ALB25MaterialRegular = 0, ALB25MaterialClear = 1, ALB25MaterialNative26 = 2, ALB25MaterialNagram = 3 };
enum { ALBAppearanceSystem = 0, ALBAppearanceApp = 1,
       ALBAppearanceLight = 2, ALBAppearanceDark = 3 };
// Button glass source (glass.buttonMaterial, 2.4.2):
//   0 the system glass button configuration (its own private material),
//   1 the same configuration with the bottom bar's real glass material
//     (UIBackgroundConfiguration _material = the platter's _background),
//   2 the platters' view-level glass on the button view (experiment).
// 2.4.1's "configuration + UIGlassEffect" (also 1) is retired.
enum { ALBButtonSourceFactory = 0, ALBButtonSourceBarMaterial = 1, ALBButtonSourceViewGlass = 2 };
static inline int ALBButtonSourceResolve(int stored, bool hasStored) {
    if (hasStored && stored >= ALBButtonSourceFactory && stored <= ALBButtonSourceViewGlass) return stored;
    return ALBButtonSourceBarMaterial;
}

// iOS 27: Clear (the user's target). iOS 26: Regular, i.e. the untouched
// native bar that follows the system transparency setting.
static inline int ALB25MaterialDefaultMode(int os) {
    return os >= 27 ? ALB25MaterialClear : ALB25MaterialRegular;
}
// stored    : glass.material (2.4)
// legacy    : glass.mode (2.3), whose 0/2 were mostly untouched defaults
// legacyClear: bottom.clearMode (<= 2.2)
static inline int ALB25MaterialResolveMode(int os, int stored, bool hasStored,
                                         int legacy, bool hasLegacy, bool legacyClear) {
    if (hasStored && (stored == ALB25MaterialRegular || stored == ALB25MaterialClear ||
                      stored == ALB25MaterialNagram)) return stored;
    if (hasStored && stored == ALB25MaterialNative26) return ALB25MaterialRegular;
    if (hasLegacy) return legacy == ALB25MaterialClear ? ALB25MaterialClear : ALB25MaterialDefaultMode(os);
    if (legacyClear) return ALB25MaterialClear;
    return ALB25MaterialDefaultMode(os);
}
static inline bool ALB25MaterialModeIsClear(int mode) {
    return mode == ALB25MaterialClear || mode == ALB25MaterialNagram;
}
// A bar keeps the glass UIKit gave it when the plug-in would not change it.
static inline bool ALB25MaterialBarKeepsNative(bool enabled, int mode, bool hasTint) {
    return !enabled || (mode == ALB25MaterialRegular && !hasTint);
}
// Black tint alpha for Clear in Dark (UIKit Dark = 2). Clear ignores
// overrideUserInterfaceStyle for darkness; only a tint darkens it.
static inline double ALB25MaterialClearTintAlpha(int mode, int style, double darkAlpha) {
    if (mode != ALB25MaterialClear || style != 2) return 0;
    return darkAlpha < 0 ? 0 : (darkAlpha > .6 ? .6 : darkAlpha);
}
// The default appearance source. 2.4/2.4.1 used the system (screen) style,
// which is Dark on a Dark-mode phone while Douyin's pages are white.
enum { ALBAppearanceDefault = ALBAppearanceApp };
// 2.4.3: a Clear mode draws Clear only while a video feed is on screen;
// every other page draws System Regular (UIKit's own bar glass, shadow and
// depth; Clear on white pages was flat and too bright).
static inline int ALB25MaterialEffectiveMode(int mode, bool clearOnlyOnVideo, bool videoContext) {
    if (ALB25MaterialModeIsClear(mode) && clearOnlyOnVideo && !videoContext) return ALB25MaterialRegular;
    return mode;
}
// UIKit Light=1, Dark=2. Feed protection has precedence over appearance choices.
static inline int ALB25MaterialResolveStyle(int preference, int systemStyle,
    int appStyle, bool videoFeed, bool protectFeed) {
    if (videoFeed && protectFeed) return 2;
    if (preference == ALBAppearanceLight) return 1;
    if (preference == ALBAppearanceDark) return 2;
    int result = preference == ALBAppearanceApp ? appStyle : systemStyle;
    return result == 2 ? 2 : 1;
}
static inline bool ALBShouldWaitForNativeSettle(double elapsed, bool geometryAnimating) {
    return elapsed < 1.8 && (elapsed < 0.12 || geometryAnimating);
}
