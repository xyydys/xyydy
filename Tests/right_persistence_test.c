/* Production geometry helpers, exercised with a deterministic layer model.
   This is not a UIKit/device test. */
#define ALB6_RIGHT_LAYOUT_GEOMETRY_ONLY 1
#include "../ALB5RightLayout.h"
#include <assert.h>
#include <stdio.h>

static const ALB6RightAffine identity={1,0,0,1,0,0};
static ALB6RightAffine inverse(ALB6RightAffine t) {
    double d=t.a*t.d-t.b*t.c;
    assert(fabs(d)>1e-8);
    ALB6RightAffine r={t.d/d,-t.b/d,-t.c/d,t.a/d,0,0};
    r.tx=-(r.a*t.tx+r.c*t.ty);r.ty=-(r.b*t.tx+r.d*t.ty);return r;
}
static ALB6RightPoint actual(ALB6RightPoint p,ALB6RightAffine local,
    ALB6RightPoint position,ALB6RightAffine parent) {
    p=ALB6RightAffinePoint(p,local); p.x+=position.x;p.y+=position.y;
    return ALB6RightAffinePoint(p,parent);
}
static void near(ALB6RightPoint a,ALB6RightPoint b) {
    assert(fabs(a.x-b.x)<1e-7 && fabs(a.y-b.y)<1e-7);
}
static unsigned persistent(ALB6RightAffine parent,ALB6RightAffine native,
    ALB6RightPoint position,ALB6RightPoint pivot,double shift,double scale,unsigned cycles) {
    ALB6RightAffine live=native;
    unsigned writes=0;int projected=0;
    for(unsigned n=0;n<cycles;n++) {
        ALB6RightPoint p=actual((ALB6RightPoint){0,0},live,position,parent);
        ALB6RightPoint x=actual((ALB6RightPoint){1,0},live,position,parent);
        ALB6RightPoint y=actual((ALB6RightPoint){0,1},live,position,parent);
        if(projected) {
            p=ALB19RightUnmapPoint(p,pivot,shift,scale);
            x=ALB19RightUnmapPoint(x,pivot,shift,scale);
            y=ALB19RightUnmapPoint(y,pivot,shift,scale);
        }
        ALB6RightAffine desired=ALB6RightMappedTransform(p,x,y,inverse(parent),position,pivot,shift,scale);
        if(ALB19RightAffineChanged(live,desired)) {live=desired;writes++;}
        projected=1;
        ALB6RightPoint test={-17,38};
        near(actual(test,live,position,parent),
            ALB6RightMapPoint(actual(test,native,position,parent),pivot,shift,scale));
    }
    return writes;
}
int main(void) {
    const ALB6RightAffine parent={1.12,.21,-.17,.82,11,-37};
    const ALB6RightAffine native={.97,-.12,.11,.98,4,-3};
    const ALB6RightPoint pivot={37,420},position={34,250};
    const double scales[]={.5,.65,1,1.25};
    const double shifts[]={0,74,148};
    for(unsigned i=0;i<4;i++) for(unsigned j=0;j<3;j++) {
        unsigned writes=persistent(parent,native,position,pivot,shifts[j],scales[i],100000);
        assert(writes==((i==2 && j==0)?0u:1u));
    }
    assert(persistent(identity,identity,position,pivot,0,1,100000)==0);
    // All six-role hidden masks preserve native coordinates under inversion.
    const double centers[]={40,110,184,258,332,414};
    for(unsigned mask=0;mask<64;mask++) for(unsigned role=0;role<6;role++) {
        double shift=ALB8RightReflowShift(centers,6,mask,role);
        ALB6RightPoint p={37,centers[role]};
        near(ALB19RightUnmapPoint(ALB6RightMapPoint(p,pivot,shift,.65),pivot,shift,.65),p);
    }
    // A native child and its glass may live under different projected roots.
    // Undo source map, apply destination map, then inverse destination: native
    // stack coordinates must survive (no cumulative scale or vertical drift).
    for(unsigned i=0;i<10000;i++) {
        ALB6RightPoint p={(double)(i%100), (double)(i%777)};
        ALB6RightPoint a=ALB6RightMapPoint(p,pivot,74,.65);
        ALB6RightPoint n=ALB19RightUnmapPoint(a,pivot,74,.65);
        ALB6RightPoint q=ALB6RightMapPoint(n,(ALB6RightPoint){20,600},82,1.25);
        near(ALB19RightUnmapPoint(q,(ALB6RightPoint){20,600},82,1.25),p);
    }
    // TEST38 horizontal music alignment is also exactly reversible and
    // independent of the native transform/scale.
    const double xOffsets[]={-19.5,0,13.25};
    for(unsigned a=0;a<3;a++) for(unsigned b=0;b<4;b++) {
        ALB6RightPoint p={51.0,327.0};
        ALB6RightPoint q=ALB6RightMapPointOffset(p,pivot,xOffsets[a],74,scales[b]);
        near(ALB19RightUnmapPointOffset(q,pivot,xOffsets[a],74,scales[b]),p);
    }

    // Old restore/apply transaction dirties every sequential layout pass;
    // a recursion-depth flag is zero again at the beginning of each pass.
    unsigned old_writes=0;
    ALB6RightAffine projected={.65,0,0,.65,10,120},live=projected;
    for(unsigned i=0;i<1000;i++) {
        if(ALB19RightAffineChanged(live,identity)) {live=identity;old_writes++;}
        if(ALB19RightAffineChanged(live,projected)) {live=projected;old_writes++;}
    }
    assert(old_writes==2000);
    puts("right persistence: 1.3M stable cycles, 64 hide masks, cross-root inverse maps passed; old restore/apply negative control stays dirty");
    return 0;
}
