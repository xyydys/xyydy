#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../ALB22NativeMethodPolicy.h"
#include "../ALBTopNative/ALB23LensPolicy.h"
#define ALB6_RIGHT_LAYOUT_GEOMETRY_ONLY 1
#include "../ALB5RightLayout.h"
static void abi_test(void) {
    ALB23NativeBoolABI abi;
    // Exact encodings copied from the user's TEST22 runtime diagnostic.
    assert(ALB23ParseNativeBoolABI("v20@0:8B16",3,true,&abi));
    assert(abi.result=='v' && abi.value=='B' && abi.arguments==3);
    assert(ALB23ParseNativeBoolABI("B16@0:8",2,false,&abi));
    assert(abi.result=='B');
    assert(ALB23ParseNativeBoolABI("v24@0:8B16B20",4,true,&abi));
    assert(abi.animated=='B');
    assert(ALB23ParseNativeBoolABI("v24@0:8c16B20",4,true,&abi));
    assert(abi.value=='c' && abi.animated=='B');
    assert(ALB23ParseNativeBoolABI("v24@0:8B16c20",4,true,&abi));
    assert(ALB23ParseNativeBoolABI("v24@0:8c16c20",4,true,&abi));
    assert(ALB23ParseNativeBoolABI("c16@0:8",2,false,&abi));
    assert(ALB23ParseNativeBoolABI("v@:B",3,true,&abi));
    assert(ALB23ParseNativeBoolABI("rv20r@0:8nB16",3,true,&abi));
    assert(ALB23ParseNativeBoolABI("v+20@+0:+8B+16",3,true,&abi));
    const char *bad[]={"v24@0:8q16","v24@0:8i16","v24@0:8@16","v24@0:8@?16", "v24@0:8^B16", "v24@0:8{BOOL=c}16", "v20@?0:8B16", "v20@0:8B16junk", "v20@0:8B16B20", "v20@0:8", "B20@0:8B16", "v20#0:8B16", "v@::B", "v20@0:8B+", "",NULL};
    for (unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i)
        assert(!ALB23ParseNativeBoolABI(bad[i],3,true,&abi));
    assert(!ALB23ParseNativeBoolABI("B16@0:8",3,false,&abi));
    assert(!ALB23ParseNativeBoolABI("B16@0:8B16",2,false,&abi));
    assert(!ALB23ParseNativeBoolABI("@?16@0:8",2,false,&abi));
    assert(!ALB23ParseNativeBoolABI("v20@0:8B16",4,true,&abi));
    assert(!ALB23ParseNativeBoolABI("v20@0:8B16",3,true,NULL));
    assert(ALB22NativeBarClassName("UIKit._UITabBarVisualProvider_Floating"));
    assert(!ALB22NativeBarClassName("UIKit.UIScrollView"));
}
static void lens_test(void) {
    const char *parent="_UILiquidLensView";
    const char *portal="_UIPortalView";
    const char *selected="_TtCC5UIKit32_UITabBarVisualProvider_FloatingP33_3C6E5A7AE2316B749C88F887559DAAB619SelectedContentView";
    const char *sdf="_TtCE5UIKitCSo17_UILiquidLensViewP33_4C400BD973F5E4E0B779D1A21A7AEB277SDFView";
    const char *glass="_TtCE5UIKitCSo17_UILiquidLensViewP33_4C400BD973F5E4E0B779D1A21A7AEB2714ClearGlassView";
    assert(!ALB23LensBackdropClass(parent));assert(ALB23LensContentClass(parent));
    assert(!ALB23LensBackdropClass(portal));assert(ALB23LensContentClass(portal));
    assert(!ALB23LensBackdropClass(selected));assert(ALB23LensContentClass(selected));
    assert(!ALB23LensBackdropClass("UIImageView"));assert(ALB23LensContentClass("UIImageView"));
    assert(!ALB23LensBackdropClass("UIKit._UITabBarPlatterView"));
    assert(!ALB23LensBackdropClass("_UITabButton"));assert(ALB23LensContentClass("_UITabButton"));
    assert(ALB23LensBackdropClass(sdf));assert(ALB23LensBackdropClass(glass));
    // A portal is protected even without a descendant UIImageView. This is the
    // case the TEST22 subtree heuristic missed. The view traversal checks it.
    assert(!ALB23LensContentClass(sdf));
    assert(!ALB23LensBackdropClass("UIVisualEffectView"));
    assert(!ALB23LensBackdropClass(NULL));assert(ALB23LensContentClass(NULL));
}
static void equal(double a,double b) {assert(fabs(a-b)<1e-8);}
static void spacing_test(void) {
    const double examples[][4]={{100,172,246,328},{0,70,142,220},{10,30,120,160},{-300,-250,-160,-70},{0,74,148,222}};
    const double scales[]={.5,.75,1,1.25};
    for(unsigned j=0;j<sizeof(examples)/sizeof(*examples);++j) {
        const double *c=examples[j];double s[4];
        assert(ALB23RightEqualActionShifts(c,s));equal(s[0],0);equal(s[3],0);
        double pitch=(c[3]-c[0])/3;
        for(unsigned k=0;k<4;++k) {
            double pivot=c[3]+25, scale=scales[k];
            double progress=pivot+40; // the progress bar is never written
            for(unsigned i=0;i<4;++i) {
                ALB6RightPoint icon={30,c[i]},number={30,c[i]+23+i};
                ALB6RightPoint a=ALB6RightMapPoint(icon,(ALB6RightPoint){30,pivot},s[i],scale);
                ALB6RightPoint b=ALB6RightMapPoint(number,(ALB6RightPoint){30,pivot},s[i],scale);
                equal(b.y-a.y,(number.y-icon.y)*scale);
                if(i) {
                    ALB6RightPoint prev=ALB6RightMapPoint((ALB6RightPoint){30,c[i-1]},(ALB6RightPoint){30,pivot},s[i-1],scale);
                    equal(a.y-prev.y,pitch*scale);
                }
                ALB6RightPoint raw=ALB19RightUnmapPoint(a,(ALB6RightPoint){30,pivot},s[i],scale);
                equal(raw.y,c[i]);
                if(i==3) {
                    ALB6RightPoint old=ALB6RightMapPoint(number,(ALB6RightPoint){30,pivot},0,scale);
                    equal(progress-b.y,progress-old.y);
                }
            }
        }
        // Native-coordinate reads yield the same correction on repeated layouts.
        double transformed[4];for(unsigned i=0;i<4;i++)transformed[i]=c[i]+s[i];
        for(unsigned tick=0;tick<10000;tick++) {
            double recovered[4],again[4];
            for(unsigned i=0;i<4;i++)recovered[i]=transformed[i]-s[i];
            assert(ALB23RightEqualActionShifts(recovered,again));
            for(unsigned i=0;i<4;i++){equal(again[i],s[i]);equal(recovered[i]+again[i],transformed[i]);}
        }
    }
    double out[4];
    assert(!ALB23RightEqualActionShifts((double[]){0,0,20,30},out));
    assert(!ALB23RightEqualActionShifts((double[]){0,10,9,30},out));
    assert(!ALB23RightEqualActionShifts((double[]){0,10,NAN,30},out));
    assert(!ALB23RightEqualActionShifts((double[]){0,10,20,INFINITY},out));
    assert(!ALB23RightEqualActionShifts(NULL,out));
    assert(!ALB23RightEqualActionShifts((double[]){0,10,20,30},NULL));
}
int main(void){abi_test();lens_test();spacing_test();puts("TEST23: raw runtime ABI / portal-safe policy / row-spacing invariants passed (C only, no UIKit).");}
