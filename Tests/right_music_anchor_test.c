#define ALB6_RIGHT_LAYOUT_GEOMETRY_ONLY 1
#include "../ALB5RightLayout.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    unsigned cases=0;
    for (int i=50;i<=125;i++) for (int gap=0;gap<=80;gap+=4)
    for (int last=0;last<5;last++) {
        double scale=i/100.0, musicY=730, musicX=378, axisX=374;
        double rowBottom=last*70+340, musicTop=musicY-25.08;
        ALB6RightPoint pivot={axisX,musicY};
        double shift=ALB42RightUpperGapShift(musicTop,rowBottom,gap,scale);
        ALB6RightPoint music=ALB6RightMapPointOffset((ALB6RightPoint){musicX,musicY},pivot,
            (axisX-musicX)*scale,0,scale);
        assert(fabs(music.y-musicY)<1e-9 && fabs(music.x-axisX)<1e-9);
        ALB6RightPoint upper=ALB6RightMapPointOffset((ALB6RightPoint){370,rowBottom},pivot,
            (axisX-370)*scale,shift,scale);
        ALB6RightPoint top=ALB6RightMapPoint((ALB6RightPoint){axisX,musicTop},pivot,0,scale);
        assert(fabs(top.y-upper.y-gap)<1e-9 && fabs(upper.x-axisX)<1e-9);
        // Decreasing the gap moves the upper column DOWN, music stays fixed.
        double closer=ALB42RightUpperGapShift(musicTop,rowBottom,gap-1,scale);
        assert(closer>shift);
        cases++;
    }
    printf("Right music anchor / screen gap / hidden-tail alternatives: %u cases passed.\n",cases);
}
