#define ALB6_RIGHT_LAYOUT_GEOMETRY_ONLY 1
#include "../ALB5RightLayout.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    const double centers[]={40,110,184,258,332,414};
    // Deliberately different count offsets/font heights: using icon centers
    // or row bounds instead of count bottoms must fail these cases.
    const double countBottom[]={69,144,222,294,375,436};
    unsigned cases=0;
    for (unsigned mask=0;mask<64;mask++) {
        if (!(mask&(1u<<4))) continue;
        int last=-1;
        for(unsigned i=1;i<4;i++) if(!(mask&(1u<<i))) last=(int)i;
        if(last<0) continue;
        double reference=ALB29RightReferenceShift(centers,6,mask,4,384,437,(mask&(1u<<5))!=0);
        double shift=ALB8RightReflowShift(centers,6,mask,(unsigned)last);
        double correction=ALB29RightCountCorrection(countBottom[4],reference,countBottom[last],shift);
        ALB6RightPoint pivot={37,368+reference};
        for (unsigned step=0;step<5;step++) {
            double scale=.55+step*.175;
            ALB6RightPoint expected=ALB6RightMapPoint((ALB6RightPoint){37,countBottom[4]},pivot,reference,scale);
            ALB6RightPoint actual=ALB6RightMapPoint((ALB6RightPoint){37,countBottom[last]},pivot,shift+correction,scale);
            assert(fabs(actual.y-expected.y)<1e-8);
            assert(actual.y<470); // never force the count onto the progress bar
            // A second pass first unmaps the old projection; no accumulating gap.
            ALB6RightPoint native=ALB19RightUnmapPoint(actual,pivot,shift+correction,scale);
            assert(fabs(native.y-countBottom[last])<1e-8);
            cases++;
        }
    }
    // Share-visible reference is unchanged, whether music exists or is hidden.
    assert(ALB29RightReferenceShift(centers,6,0,4,384,437,0)==0);
    assert(ALB29RightReferenceShift(centers,6,1u<<5,4,384,437,1)==53);
    // Native visibility and configured hiding are independent. A retained
    // Favorite slot must not be chosen when Like is the final rendered row.
    const unsigned actionMask=(1u<<1)|(1u<<2)|(1u<<3);
    assert(ALB32RightLastActionIndex(6,actionMask,1u<<4,0)==3);
    assert(ALB32RightLastActionIndex(6,actionMask,1u<<4,1u<<3)==2);
    assert(ALB32RightLastActionIndex(6,actionMask,(1u<<4)|(1u<<2),1u<<3)==1);
    assert(ALB32RightLastActionIndex(6,actionMask,1u<<4,actionMask)==-1);
    for(unsigned nativeHidden=0;nativeHidden<64;nativeHidden++) {
        int last=ALB32RightLastActionIndex(6,actionMask,1u<<4,nativeHidden);
        if(last<0)continue;
        double shift=ALB8RightReflowShift(centers,6,1u<<4,(unsigned)last);
        double correction=ALB29RightCountCorrection(countBottom[4],0,countBottom[last],shift);
        for(unsigned step=0;step<5;step++) {
            double scale=.55+step*.175;
            ALB6RightPoint pivot={37,368};
            ALB6RightPoint expected=ALB6RightMapPoint((ALB6RightPoint){37,countBottom[4]},pivot,0,scale);
            ALB6RightPoint actual=ALB6RightMapPoint((ALB6RightPoint){37,countBottom[last]},pivot,shift+correction,scale);
            assert(fabs(actual.y-expected.y)<1e-8);
            cases++;
        }
    }
    printf("hidden-share count anchoring: %u visibility/scale cases passed\n",cases);
}
