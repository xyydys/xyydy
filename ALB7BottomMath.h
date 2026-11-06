#pragma once
#include <math.h>

typedef struct { double x,y,w,h; } ALB7Rect;
// Geometry constraints distinguish a real left tab surface + search circle
// from individual tab items, the selection lens, or a full-width carrier.
static inline double ALB7SurfacePairScore(ALB7Rect left, ALB7Rect right, double barWidth) {
    if (barWidth < 100 || left.w < barWidth*.30 || left.w < left.h*1.6 ||
        left.h < 30 || left.h > 120 || right.h < 30 || right.h > 120 ||
        right.w < right.h*.65 || right.w > right.h*1.45 ||
        left.x+left.w > right.x+3 || right.x+right.w < barWidth*.65 ||
        fabs((left.y+left.h*.5)-(right.y+right.h*.5)) > 14 ||
        fabs(left.h-right.h)>18) return -1;
    double span=right.x+right.w-left.x;
    if (span < barWidth*.58 || span > barWidth+12) return -1;
    return span - fabs(left.h-right.h)*3 - fabs(right.w-right.h)*2;
}
static inline int ALB8FeedCompactPolicy(int next,int timerMode,int currentlyCompact,int nextExpands) {
    return timerMode ? (next && currentlyCompact && !nextExpands) : next;
}

// Settled outer-row identity, not movement inside a photo work. Feed versions
// that reuse one row still report a transition if its aweme changes and the
// outer table has supplied a vertical user direction.
static inline int ALB8CommittedWorkDirection(long oldSection,long oldRow,long section,long row,
                                            int identitiesKnown,int sameIdentity,int userDirection) {
    if(identitiesKnown && sameIdentity)return 0;
    if(section!=oldSection)return section>oldSection?1:-1;
    if(row!=oldRow)return row>oldRow?1:-1;
    return identitiesKnown ? (userDirection>0?1:(userDirection<0?-1:0)) : 0;
}

static inline double ALB9TransitionProgress(double start,int compact,double elapsed) {
    double t=fmin(1,fmax(0,elapsed/.42));
    return start+((compact?1:0)-start)*t*t*(3-2*t);
}

static inline double ALB10AnimationProgress(double start,int compact,double elapsed,double duration) {
    if(duration<=0)return compact?1:0;
    double t=fmin(1,fmax(0,elapsed/duration));
    return start+((compact?1:0)-start)*t*t*(3-2*t);
}
