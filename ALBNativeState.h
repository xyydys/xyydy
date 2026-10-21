#pragma once
// Small portable predicates used by the UIKit bridge; no animation machinery.
#include <stdbool.h>
#include <string.h>
#include <math.h>
static inline bool ALB15EncodingIsBool(const char *type) {
    if (!type) return false;
    while (*type && strchr("rnNoORV",*type)) ++type;
    return (*type=='B' || *type=='c') && type[1]=='\0';
}
// -1 is unknown. A square alone is NOT proof of minimization: a bar with one
// regular tab can already be square while expanded. Require a wider baseline.
static inline int ALB15ClassifyNativeGeometry(double width,double height,double expandedWidth) {
    if (!isfinite(width) || !isfinite(height) || width<20 || height<20) return -1;
    if (width>=height*2.0) return 0;
    if (isfinite(expandedWidth) && expandedWidth>=height*2.0 &&
        width<=height*1.35 && width<=expandedWidth*.70) return 1;
    return -1;
}
