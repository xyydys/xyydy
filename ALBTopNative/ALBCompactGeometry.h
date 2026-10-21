#ifndef XHSQ100_COMPACT_GEOMETRY_H
#define XHSQ100_COMPACT_GEOMETRY_H
#include <math.h>
#include <stdbool.h>
typedef struct { bool valid; double scale, width; } XHSQ100Fit;
typedef struct { double x, y; } ALB7TopPoint;
static inline ALB7TopPoint ALB7TopModuleCenter(double centerX, double centerY,
    double naturalX, double scale) {
    // Do not derive the next pose from an already scaled frame or center.
    // Both side controls and the group have exactly this same baseline.
    if (!isfinite(scale) || scale <= 0) scale = 1;
    ALB7TopPoint result = {centerX + (naturalX - centerX) * scale, centerY};
    return result;
}
static inline XHSQ100Fit XHSQ100FitNativePlatter(double targetWidth, double targetHeight,
    double barWidth, double platterWidth, double platterHeight) {
    XHSQ100Fit f = {false, 1.0, targetWidth};
    if (!isfinite(targetWidth) || !isfinite(targetHeight) || !isfinite(barWidth) ||
        !isfinite(platterWidth) || !isfinite(platterHeight) || targetWidth <= 0 ||
        targetHeight <= 0 || platterWidth <= 0 || platterHeight <= 0) return f;
    double scale = targetHeight / platterHeight;
    double margin = barWidth - platterWidth;
    if (scale < 0.35 || scale > 1.6 || margin < -1 || margin > barWidth * 0.55) return f;
    f.valid = true;
    f.scale = scale;
    f.width = targetWidth / scale + fmax(0.0, margin);
    return f;
}
// Pure route-window policy, also used by the portable geometry regression test.
// direction advances by one slot only after selection reaches that window edge.
static inline long ALB6TopWindowStart(long total, long visible, long start,
    long selected, long direction) {
    if (total <= 0 || visible <= 0) return 0;
    if (visible > total) visible = total;
    long maximum = total - visible;
    if (start < 0) start = 0;
    if (start > maximum) start = maximum;
    if (selected >= 0 && selected < total) {
        if (selected < start) start = selected;
        if (selected >= start + visible) start = selected - visible + 1;
        if (visible > 1 && direction > 0 && selected == start + visible - 1 && start < maximum) start++;
        else if (visible > 1 && direction < 0 && selected == start && start > 0) start--;
    }
    return start;
}
#endif
