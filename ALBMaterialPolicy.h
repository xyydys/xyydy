#pragma once
#include <stdbool.h>
// Persisted values. Native26 retains the verified bottom provider on iOS 26.
enum { ALBMaterialRegular = 0, ALBMaterialClear = 1, ALBMaterialNative26 = 2 };
enum { ALBAppearanceSystem = 0, ALBAppearanceApp = 1,
       ALBAppearanceLight = 2, ALBAppearanceDark = 3 };
static inline int ALBMaterialResolveMode(int os, int stored, bool hasStored,
                                        bool legacyClear) {
    if (hasStored && stored >= 0 && stored <= 2)
        return stored == ALBMaterialNative26 && os >= 27 ? ALBMaterialRegular : stored;
    return legacyClear ? ALBMaterialClear : (os == 26 ? ALBMaterialNative26 : ALBMaterialRegular);
}
// UIKit Light=1, Dark=2. Feed protection has precedence over appearance choices.
static inline int ALBMaterialResolveStyle(int preference, int systemStyle,
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
