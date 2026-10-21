#include <assert.h>
#include <stdio.h>
#include "../ALBMaterialPolicy.h"
int main(void) {
    assert(ALBMaterialResolveMode(26, 0, false, false) == ALBMaterialNative26);
    assert(ALBMaterialResolveMode(27, 0, false, false) == ALBMaterialRegular);
    assert(ALBMaterialResolveMode(26, 0, false, true) == ALBMaterialClear);
    assert(ALBMaterialResolveMode(27, 0, false, true) == ALBMaterialClear);
    assert(ALBMaterialResolveMode(27, 2, true, false) == ALBMaterialRegular);
    assert(ALBMaterialResolveMode(27, 0, true, true) == ALBMaterialRegular);
    assert(ALBMaterialResolveMode(26, 99, true, false) == ALBMaterialNative26);
    unsigned cases = 0;
    for (int preference = 0; preference <= 3; preference++)
    for (int system = 1; system <= 2; system++)
    for (int app = 1; app <= 2; app++)
    for (int feed = 0; feed <= 1; feed++)
    for (int protect = 0; protect <= 1; protect++) {
        int actual = ALBMaterialResolveStyle(preference, system, app, feed, protect);
        if (feed && protect) assert(actual == 2);
        else if (preference == ALBAppearanceSystem) assert(actual == system);
        else if (preference == ALBAppearanceApp) assert(actual == app);
        else assert(actual == (preference == ALBAppearanceLight ? 1 : 2));
        cases++;
    }
    assert(ALBMaterialResolveStyle(0, 0, 0, false, false) == 1);
    assert(ALBShouldWaitForNativeSettle(0, false));
    assert(ALBShouldWaitForNativeSettle(.11, false));
    assert(!ALBShouldWaitForNativeSettle(.12, false));
    assert(ALBShouldWaitForNativeSettle(1.7, true));
    assert(!ALBShouldWaitForNativeSettle(1.8, true));
    printf("Material policy: %u appearance combinations, preference migration and transition boundaries passed.\n", cases);
    return 0;
}
