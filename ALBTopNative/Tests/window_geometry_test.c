#include "../ALBCompactGeometry.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    // Seven host routes, only two visible: the native bar must own just the
    // last two items, rather than scrolling to the cut end of a 7-item platter.
    assert(ALB6TopWindowStart(7, 2, 0, 6, 0) == 5);
    assert(ALB6TopWindowStart(7, 2, 5, 5, -1) == 4);
    assert(ALB6TopWindowStart(7, 2, 4, 5, 1) == 5);
    assert(ALB6TopWindowStart(7, 1, 0, 6, 1) == 6);
    assert(ALB6TopWindowStart(2, 4, 7, 1, 1) == 0);
    assert(ALB6TopWindowStart(0, 4, 7, -1, 0) == 0);
    for (long total=1;total<=20;total++) for(long shown=1;shown<=4;shown++)
      for(long selected=0;selected<total;selected++) for(long direction=-1;direction<=1;direction++) {
        long visible=shown<total?shown:total;
        long start=ALB6TopWindowStart(total,visible,total/2,selected,direction);
        assert(start>=0 && start+visible<=total);
        assert(selected>=start && selected<start+visible);
        assert(ALB6TopWindowStart(total,visible,start,selected,0)==start);
      }
    XHSQ100Fit fit=XHSQ100FitNativePlatter(148,44,250,218,64);
    assert(fit.valid && fabs(fit.scale-44.0/64)<1e-10);
    assert(fabs((fit.width-32)*fit.scale-148)<1e-10);
    assert(!XHSQ100FitNativePlatter(148,44,250,218,0).valid);
    // Compact and expanded top modules must have one baseline even with
    // asymmetric side gaps, nonzero container origins and rapid reversals.
    double scales[] = {1, .78, .45, .78, 1};
    for (unsigned i=0; i<sizeof(scales)/sizeof(scales[0]); i++) {
        ALB7TopPoint left = ALB7TopModuleCenter(237, 84, 54, scales[i]);
        ALB7TopPoint right = ALB7TopModuleCenter(237, 84, 398, scales[i]);
        ALB7TopPoint group = ALB7TopModuleCenter(237, 84, 237, scales[i]);
        assert(left.y == right.y && right.y == group.y);
        assert(group.x == 237);
        assert(fabs((group.x-left.x)/(right.x-group.x)-183.0/161.0) < 1e-10);
        ALB7TopPoint again = ALB7TopModuleCenter(237, 84, 54, scales[i]);
        assert(again.x == left.x && again.y == left.y);
    }
    puts("top native window, platter and shared baseline geometry: PASS");
}
