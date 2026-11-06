#pragma once
#include <stdbool.h>
// The bottom bar's next native minimize request waits for UIKit's own
// geometry animation (bounded, never longer than 1.8 s).
static inline bool ALBShouldWaitForNativeSettle(double elapsed, bool geometryAnimating) {
    return elapsed < 1.8 && (elapsed < 0.12 || geometryAnimating);
}
