/* Tests the actual coordinate-mapping helpers used by ALB5RightLayout.h. */
#define ALB6_RIGHT_LAYOUT_GEOMETRY_ONLY 1
#include "../ALB5RightLayout.h"
#include <assert.h>
#include <stdio.h>
static ALB6RightPoint add(ALB6RightPoint a,ALB6RightPoint b) { return (ALB6RightPoint){a.x+b.x,a.y+b.y}; }
static ALB6RightAffine inverse(ALB6RightAffine t) {
    double determinant=t.a*t.d-t.b*t.c;
    ALB6RightAffine r={t.d/determinant,-t.b/determinant,-t.c/determinant,t.a/determinant,0,0};
    r.tx=-(r.a*t.tx+r.c*t.ty);r.ty=-(r.b*t.tx+r.d*t.ty);return r;
}
static void equal(ALB6RightPoint a,ALB6RightPoint b) { assert(fabs(a.x-b.x)<1e-8);assert(fabs(a.y-b.y)<1e-8); }
static ALB6RightPoint actual(ALB6RightPoint local,ALB6RightAffine view,
    ALB6RightPoint layerPosition,ALB6RightAffine parent) {
    return ALB6RightAffinePoint(add(ALB6RightAffinePoint(local,view),layerPosition),parent);
}
static void test(ALB6RightAffine parent,ALB6RightAffine base,ALB6RightPoint position,
    ALB6RightPoint pivot,double shift,double scale) {
    ALB6RightPoint anchor=actual((ALB6RightPoint){0,0},base,position,parent);
    ALB6RightPoint ux=actual((ALB6RightPoint){1,0},base,position,parent);
    ALB6RightPoint uy=actual((ALB6RightPoint){0,1},base,position,parent);
    ALB6RightAffine mapped=ALB6RightMappedTransform(anchor,ux,uy,inverse(parent),position,pivot,shift,scale);
    const ALB6RightPoint points[]={{0,0},{-23,-23},{23,23},{-23,35},{0,46}};
    for(unsigned i=0;i<sizeof(points)/sizeof(points[0]);i++) {
        ALB6RightPoint expected=ALB6RightMapPoint(actual(points[i],base,position,parent),pivot,shift,scale);
        equal(actual(points[i],mapped,position,parent),expected);
    }
    /* Identity is reversible even with a non-identity native transform. */
    ALB6RightAffine restored=ALB6RightMappedTransform(anchor,ux,uy,inverse(parent),position,pivot,0,1);
    for(unsigned i=0;i<sizeof(points)/sizeof(points[0]);i++)
        equal(actual(points[i],restored,position,parent),actual(points[i],base,position,parent));
}
int main(void) {
    const ALB6RightAffine identity={1,0,0,1,0,0};
    ALB6RightAffine rotatedParent={cos(.27)*1.3,sin(.27)*1.3,-sin(.27)*1.3,cos(.27)*1.3,11,-37};
    ALB6RightAffine scaledParent={.8,0,0,1.2,-70,29};
    ALB6RightAffine native={cos(-.13)*.9,sin(-.13)*.9,-sin(-.13)*.9,cos(-.13)*.9,4,-3};
    ALB6RightPoint position={34,250},pivot={37,420};
    test(identity,identity,position,pivot,0,1);
    test(identity,identity,position,pivot,74,.6);
    test(rotatedParent,native,position,pivot,74,.6);
    test(scaledParent,native,position,pivot,148,1.25);
    test(rotatedParent,identity,position,pivot,0,.5);
    test(scaledParent,identity,position,pivot,74,1);
    /* After music is hidden, the share-count pivot moves down with its row.
       Scaling must keep that final pivot fixed. */
    ALB6RightPoint movedPivot={37,494};
    equal(ALB6RightMapPoint(pivot,movedPivot,74,.5),movedPivot);
    /* Different native row heights must not change the visible icon spacing.
       Avatar->Like 70, then standard action gaps 74, final music gap 82. */
    const double centers[]={40,110,184,258,332,414};
    const unsigned noComment=1u<<2;
    assert(ALB8RightReflowShift(centers,6,noComment,0)==70);
    assert(ALB8RightReflowShift(centers,6,noComment,1)==74);
    assert(ALB8RightReflowShift(centers,6,noComment,3)==0);
    double avatarY=centers[0]+ALB8RightReflowShift(centers,6,noComment,0);
    double likeY=centers[1]+ALB8RightReflowShift(centers,6,noComment,1);
    assert(likeY-avatarY==74); /* icon centers, not row-bounds centers */
    assert(ALB8RightReflowShift(centers,6,1u<<5,4)==82);
    assert(ALB8RightReflowShift(centers,6,(1u<<2)|(1u<<5),0)==144);
    assert(ALB8RightReflowShift(centers,6,1u<<0,1)==0);
    /* 10k layout cycles always map from the same native coordinates. */
    for (unsigned n=0;n<10000;n++) {
        double shift=ALB8RightReflowShift(centers,6,noComment,0);
        ALB6RightPoint mapped=ALB6RightMapPoint((ALB6RightPoint){37,centers[0]},pivot,shift,.65);
        equal(mapped,ALB6RightMapPoint((ALB6RightPoint){37,110},pivot,0,.65));
    }
    for(unsigned n=0;n<10000;n++) {
        double nativeAlpha=1;
        double hiddenPresentation=0;
        (void)hiddenPresentation;
        assert(!ALB10RightAlphaChanged(nativeAlpha,1));
    }
    assert(ALB10RightAlphaChanged(1,.5));
    assert(!ALB10RightAlphaChanged(.5,.5001));
    puts("right layout: coordinate transforms + icon-slot packing + repeated cycles passed");
    return 0;
}
