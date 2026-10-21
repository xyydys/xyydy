#pragma once
#include <stdbool.h>
#include <string.h>
static inline bool ALB23EndsWith(const char *name, const char *suffix) {
    if (!name || !suffix) return false;
    size_t n = strlen(name), s = strlen(suffix);
    return n >= s && strcmp(name + n - s, suffix) == 0;
}
// Names observed in the supplied iOS 26.6 diagnostic. Scope is additionally
// restricted to descendants of this side button's native selection lens.
static inline bool ALB23LensBackdropClass(const char *name) {
    if (!name || !strstr(name, "UILiquidLensView")) return false;
    return ALB23EndsWith(name,"ClearGlassView") || ALB23EndsWith(name,"SDFView") ||
        ALB23EndsWith(name,"SDFElementView");
}
static inline bool ALB23LensContentClass(const char *name) {
    if (!name) return true;
    return strstr(name,"PortalView") || strstr(name,"ContentView") ||
        strstr(name,"TabButton") || strstr(name,"ImageView") ||
        strstr(name,"Label") || strcmp(name,"_UILiquidLensView") == 0;
}
