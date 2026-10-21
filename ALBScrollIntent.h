#pragma once
#include <stdbool.h>
#include <math.h>
#include <stddef.h>

/* UIKit-free policy. Finger coordinates, not contentOffset or native bar state. */
typedef struct {
    double lastX, lastY, travel;
    int direction, committed;
    bool horizontal;
} ALBPanState;

static inline void ALBPanBegin(ALBPanState *s, double x, double y) {
    *s = (ALBPanState){ .lastX = x, .lastY = y };
}
/* Returns +1 collapse, -1 expand, 0 no transition. Reversal uses incremental travel. */
static inline int ALBPanUpdate(ALBPanState *s, double x, double y,
                               double vx, double vy, double threshold) {
    if (!isfinite(x) || !isfinite(y) || !isfinite(vx) || !isfinite(vy)) return 0;
    double dx = x - s->lastX, dy = y - s->lastY;
    s->lastX = x; s->lastY = y;
    if (s->horizontal) return 0;
    if (!s->committed && fabs(x) > threshold && fabs(x) > fabs(y) * 1.35) {
        s->horizontal = true; return 0;
    }
    if (fabs(dy) < 0.1 || fabs(dx) > fabs(dy) * 1.5) return 0;
    int direction = dy < 0 ? 1 : -1;
    if (direction != s->direction) { s->direction = direction; s->travel = 0; }
    s->travel += fabs(dy);
    double required = fabs(vy) >= 80 && fabs(vy) >= fabs(vx) ? threshold : threshold * 1.8;
    if (s->travel < required || s->committed == direction) return 0;
    s->committed = direction;
    return direction;
}

