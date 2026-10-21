#include "../ALBNativeState.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(ALB15EncodingIsBool("B")); assert(ALB15EncodingIsBool("c"));
    assert(ALB15EncodingIsBool("rB")); assert(ALB15EncodingIsBool("rnNc"));
    const char *bad[]={NULL,"","@","q","i","v","^B","B16","b1","r", "C"};
    for (unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i) assert(!ALB15EncodingIsBool(bad[i]));
    assert(ALB15ClassifyNativeGeometry(300,56,0)==0);
    assert(ALB15ClassifyNativeGeometry(56,56,300)==1);
    assert(ALB15ClassifyNativeGeometry(56,56,0)==-1); // One-item expanded bar.
    assert(ALB15ClassifyNativeGeometry(90,56,300)==-1); // Still transitioning.
    assert(ALB15ClassifyNativeGeometry(NAN,56,300)==-1);
    assert(ALB15ClassifyNativeGeometry(56,INFINITY,300)==-1);
    assert(ALB15ClassifyNativeGeometry(0,56,300)==-1);
    for (int i=0;i<10000;i++) {
        double h=40+(i%32), w=h;
        assert(ALB15ClassifyNativeGeometry(w,h,h*4)==1);
        assert(ALB15ClassifyNativeGeometry(w,h,0)==-1);
        assert(ALB15ClassifyNativeGeometry(h*4,h,0)==0);
    }
    puts("native-state predicates: BOOL ABI checks, unknown/single-item/transition/expanded/compact/invalid geometry passed");
}
