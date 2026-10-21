#include "../ALB7BottomMath.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    ALB7Rect left={20,880,326,62}, search={360,880,62,62};
    assert(ALB7SurfacePairScore(left,search,440)>0);
    assert(ALB7SurfacePairScore((ALB7Rect){20,880,62,62},search,440)<0); // individual item
    assert(ALB7SurfacePairScore((ALB7Rect){0,873,440,83},search,440)<0); // common ancestor
    assert(ALB7SurfacePairScore(left,(ALB7Rect){360,830,62,62},440)<0); // wrong row
    assert(ALB7SurfacePairScore(left,(ALB7Rect){280,880,62,62},440)<0); // overlapping lens
    assert(ALB7SurfacePairScore((ALB7Rect){18,720,258,56},(ALB7Rect){290,720,56,56},390)>0);
    double pivot=(left.x+search.x+search.w)*.5;
    for(double scale=.55;scale<1.16;scale+=.05) {
        double min=ALB7ScaledCoordinate(left.x,pivot,scale);
        double max=ALB7ScaledCoordinate(search.x+search.w,pivot,scale);
        assert(fabs((min+max)*.5-pivot)<1e-9);
        assert(fabs(max-min-(search.x+search.w-left.x)*scale)<1e-9);
    }
    // Native owner with its own affine scale/translation. Thousands of
    // alternating video commits / nested layout completions must return the
    // SAME absolute transform; even a perturbed model value is never a base.
    ALB8Affine native={.98,.02,-.02,.98,3,-2},rendered=native;
    for(int cycle=0;cycle<2000;cycle++) {
        double scale=(cycle%3==0)?1.0:.67;
        double sx=(cycle%2)?scale:scale*(62.0/326.0);
        rendered=ALB8ComposeAbsoluteScale(native,sx,scale,-158,0);
        for(int layout=0;layout<128;layout++) {
            rendered.a+=1e-7; // UIKit/animation model adjustment defeats ==.
            rendered=ALB8ComposeAbsoluteScale(native,sx,scale,-158,0);
            assert(fabs(rendered.a-native.a*sx)<1e-12);
            assert(fabs(rendered.d-native.d*scale)<1e-12);
            assert(fabs(rendered.tx-(-155))<1e-12);
        }
        rendered=ALB8ComposeAbsoluteScale(native,1,1,0,0);
        assert(fabs(rendered.a-native.a)<1e-12 && fabs(rendered.d-native.d)<1e-12);
    }
    for(int n=0;n<1000;n++) {
        double configured=(n%2)?1:.67;
        double fallback=ALB8FallbackAbsoluteScale(configured,1);
        // No accepted/failed private UIKit call can multiply this by another
        // 0.72. Every cycle returns to the same expanded native baseline.
        assert(fabs(fallback-configured*.72)<1e-12);
        rendered=ALB8ComposeAbsoluteScale(native,fallback,fallback,0,0);
        assert(fabs(rendered.a-native.a*configured*.72)<1e-12);
        double expanded=ALB8FallbackAbsoluteScale(configured,0);
        assert(fabs(expanded-configured)<1e-12);
    }
    assert(ALB9TransitionProgress(0,1,0)==0);
    assert(ALB9TransitionProgress(1,0,0)==1);
    assert(ALB9TransitionProgress(0,1,.42)==1);
    assert(ALB9TransitionProgress(1,0,.42)==0);
    double partial=ALB9TransitionProgress(0,1,.15);
    assert(partial>0 && partial<1);
    assert(ALB9TransitionProgress(partial,0,0)==partial); // interrupt without snapping
    for(unsigned n=0;n<1000;n++) {
        double elapsed=.42*n/999.0;
        double collapse=ALB10AnimationProgress(0,1,elapsed,.42);
        double expand=ALB10AnimationProgress(1,0,elapsed,.42);
        assert(fabs(collapse+expand-1)<1e-12); // exact reverse geometry
        assert(collapse>=0 && collapse<=1 && expand>=0 && expand<=1);
    }
    double mid=ALB10AnimationProgress(0,1,.18,.42);
    assert(ALB10AnimationProgress(mid,0,0,.42*mid)==mid);
    assert(ALB10AnimationProgress(mid,0,.42*mid,.42*mid)==0);
    puts("bottom geometry: native pair, pivot, 256000 idempotent layouts, reverse trajectory and interrupted animation passed");
}
