#!/usr/bin/env python3
"""Exercise the production profile bottom-only policy with changing header state.

The C helpers are extracted verbatim from Tweak.x. Integration guards verify
the TEST35 profile-route exclusion as well as nested pan protection for
other routes. The old arithmetic remains covered for non-profile callers. This does
not simulate UIKit's sticky-header implementation or claim a device test.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

from check_logos_boundaries import mask_literals

root = Path(__file__).resolve().parents[1]
tweak = (root / "Tweak.x").read_text()


def function(signature):
    start = tweak.index(signature)
    opening = tweak.index("{", start)
    masked = mask_literals(tweak)
    depth = 0
    for index in range(opening, len(tweak)):
        depth += (masked[index] == "{") - (masked[index] == "}")
        if depth == 0:
            return tweak[start:index + 1]
    raise AssertionError("Unclosed function: " + signature)


frame = function("static CGRect ALB34ProfileExtendedFrame(")
inset = function("static UIEdgeInsets ALB34ProfileReleasedInset(")
busy = function("static BOOL ALB34ProfileScrollBusy(")
clip = function("static BOOL ALB36ProfileClipEligible(")
compensation = function("static CGFloat ALB37CompensatedBottomInset(")
adjust = function("static BOOL AWEBottomV15AdjustView(")
route = function("static void AWEExtendRouteContentBehindBottomBar(")
assert "ALB34ProfileExtendedFrame(before, gap)" in adjust
assert "ALB34ProfileReleasedInset(scroll.contentInset," in adjust
assert "ALB34ProfileScrollChainBusy(view)" in adjust
assert "kAWEBottomV15BaseFrameKey" not in tweak
assert "CGRect desired = base" not in adjust
assert "UIEdgeInsets inset = baseInset" not in adjust
assert "if (!CGPointEqualToPoint(scroll.contentOffset, offset)) scroll.contentOffset = offset;" in adjust
assert route.index("profileScrollBusy = YES") < route.index("for (UIView *v in candidates)")
assert route.index("if (profileScrollBusy) break;") < route.index("AWEBottomV15AdjustView(v,")
assert "ALB32BottomViewIsVisible(v, window)" in route
# Restore the actual previously working no-jump gate, including idle callbacks.
assert adjust.index("if (route == AWEBottomRouteProfile) return NO;") < adjust.index("CGRect wr =")
profile_branch = route.index("if (route == AWEBottomRouteProfile) {")
profile_return = route.index("return;", profile_branch)
assert route.index("ALB24ClearNativeBottomBackdrops(window,route)") < profile_branch < profile_return < route.index("NSMutableArray<UIView *> *queue")
assert route.index("AWEBottomV22ClearShopNativeBarFast", profile_branch, profile_return) < route.index("ALB36RevealProfileBottom", profile_branch, profile_return)
assert "%hook UIScrollView\n" not in tweak
assert '@"scroll.scan"' not in tweak
assert '@"profile.collection.layout"' not in tweak and '@"profile.layout"' not in tweak
reveal = function("static void ALB36RevealProfileBottom(")
chain = function("static void ALB36OpenProfileClipChain(")
assert 'ALB36RevealProfileBottom(window)' in route and 'ALB36OpenProfileClipChain' in reveal
for forbidden in ['.frame =', '.frame=', '.bounds =', '.bounds=', 'contentOffset', 'contentInset',
                  'contentSize', 'reloadData', 'scrollTo', 'setNeedsLayout', 'layoutIfNeeded']:
    assert forbidden not in reveal + chain, forbidden
specific = function("static void AWEExtendScrollViewBehindBottomBar(")
assert "if (route == AWEBottomRouteNone || route == AWEBottomRouteProfile) return;" in specific

inner = function("static void ALB37ExtendProfileInnerList(")
identity = function("static BOOL ALB37IsInnerProfileGrid(")
assert 'AWETabContentItemContainerCell' in identity
assert '@"ProfileStick"' in identity and '@"ListKitTabContent"' in identity
assert 'list.pagingEnabled' in identity and 'UICollectionViewScrollDirectionVertical' in identity
assert inner.index('ALB34ProfileScrollChainBusy(list)') < inner.index('list.contentInset=inset')
assert 'inset.bottom=bottom' in inner and 'frame.size.height=height' in inner
assert 'setContentOffset:CGPointZero' not in inner and 'scrollTo' not in inner
assert 'contentSize=' not in inner and 'inset.top=' not in inner

fixture = r"""
#include <assert.h>
#include <math.h>
#include <stdio.h>
#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))
typedef int BOOL;
typedef double CGFloat;
typedef struct { double x,y; } CGPoint;
typedef struct { double width,height; } CGSize;
typedef struct { CGPoint origin; CGSize size; } CGRect;
#define CGRectGetMinX(r) ((r).origin.x)
#define CGRectGetMaxX(r) ((r).origin.x+(r).size.width)
#define CGRectGetMaxY(r) ((r).origin.y+(r).size.height)
typedef struct { double top,left,bottom,right; } UIEdgeInsets;
typedef enum { UIGestureRecognizerStatePossible, UIGestureRecognizerStateBegan,
 UIGestureRecognizerStateChanged, UIGestureRecognizerStateEnded,
 UIGestureRecognizerStateCancelled, UIGestureRecognizerStateFailed
} UIGestureRecognizerState;
PRODUCTION
int main(void) {
    unsigned traces = 0;
    const double gaps[] = {-83, 0, 29, 30, 60, 83, 135, 136};
    // Native header collapse/expansion changes origin and top inset while the
    // collection is far down. Each cleanup must use that live state, even if
    // its saved bottom reservation was sampled at the top of the page.
    for (int pass=0; pass<100; ++pass) {
        CGRect current = {{pass*.25, 312-pass*3.0}, {390+pass*.5, 560+pass}};
        UIEdgeInsets live = {pass*3.0, pass*.25, 34, pass*.125};
        for (unsigned i=0; i<sizeof(gaps)/sizeof(gaps[0]); ++i) {
            double gap=gaps[i];
            CGRect next=ALB34ProfileExtendedFrame(current,gap);
            assert(next.origin.x==current.origin.x);
            assert(next.origin.y==current.origin.y);
            assert(next.size.width==current.size.width);
            double delta=(gap>=30 && gap<=135)?gap:0;
            assert(next.size.height==current.size.height+delta);
            CGRect repeated=ALB34ProfileExtendedFrame(next,delta?0:gap);
            assert(repeated.size.height==next.size.height);
            UIEdgeInsets released=ALB34ProfileReleasedInset(live,34,83);
            assert(released.top==live.top && released.left==live.left && released.right==live.right);
            assert(released.bottom==-49);
            released=ALB34ProfileReleasedInset(released,34,83);
            assert(released.top==live.top && released.bottom==-49);
            UIEdgeInsets untouched=ALB34ProfileReleasedInset(live,0,0);
            assert(untouched.bottom==live.bottom && untouched.top==live.top);
            ++traces;
        }
    }
    // Either member of the outer sticky/inner favorites pair can own the
    // gesture. No geometry pass is allowed just because the other is idle.
    for (int a=0; a<48; ++a) for (int b=0; b<48; ++b) {
        int ta=a&1, da=(a>>1)&1, ca=(a>>2)&1, pa=a/8;
        int tb=b&1, db=(b>>1)&1, cb=(b>>2)&1, pb=b/8;
        int blocked=ALB34ProfileScrollBusy(ta,da,ca,pa) ||
                    ALB34ProfileScrollBusy(tb,db,cb,pb);
        int expected=ta||da||ca||pa==1||pa==2||tb||db||cb||pb==1||pb==2;
        assert(blocked==expected);
        ++traces;
    }
    assert(traces==3104);
    // Full-width Favorites viewport ending at the native bar: eligible at
    // different header positions, independent of scroll direction/offset.
    CGSize window={440,956};
    for (int top=-600;top<=380;top+=20) {
        CGRect viewport={{0,top},{440,873-top}};
        assert(ALB36ProfileClipEligible(viewport,window));
    }
    assert(!ALB36ProfileClipEligible((CGRect){{0,830},{146,126}},window)); // thumbnail
    assert(!ALB36ProfileClipEligible((CGRect){{0,800},{440,40}},window)); // category row
    assert(!ALB36ProfileClipEligible((CGRect){{440,0},{440,873}},window)); // offscreen tab
    assert(!ALB36ProfileClipEligible((CGRect){{0,0},{440,760}},window)); // unrelated panel
    assert(!ALB36ProfileClipEligible((CGRect){{0,0},{440,1200}},window)); // overflowing content
    assert(!ALB36ProfileClipEligible((CGRect){{0,NAN},{440,873}},window));
    // The real compensation helper preserves scroll limits through repeated
    // acquisition, native bottom-inset changes and full release. Header/top
    // state is independent and must not enter this arithmetic.
    unsigned compensationCases=0;
    for (int height=240;height<=960;height+=120) {
        for (int content=100;content<=4000;content+=130) {
            for (int delta=0;delta<=145;delta+=29) {
                double nativeBottom=34;
                double newBottom=ALB37CompensatedBottomInset(nativeBottom,nativeBottom,0,delta);
                assert(newBottom==34+delta);
                double top=-200; // dynamic sticky header state
                double before=MAX(-top,content-height+nativeBottom);
                double after=MAX(-top,content-(height+delta)+newBottom);
                assert(before==after);
                assert(ALB37CompensatedBottomInset(newBottom,newBottom,delta,delta)==newBottom);
                assert(ALB37CompensatedBottomInset(newBottom,newBottom,delta,0)==nativeBottom);
                // App replaced bottom reservation while lease was active.
                double changed=ALB37CompensatedBottomInset(17,34+delta,delta,delta);
                assert(changed==17+delta);
                compensationCases++;
            }
        }
    }
    printf("Profile: 3104 retained helper traces, 56 clip cases, %u compensated-inner-scroll cases passed; no UIKit/device test.\n",compensationCases);
    return 0;
}
""".replace("PRODUCTION", "\n".join([frame, inset, busy, clip, compensation]))

compiler = shutil.which("clang") or shutil.which("cc")
if not compiler:
    raise SystemExit("A C compiler is required for the extracted profile test")
with tempfile.TemporaryDirectory(prefix="alb-profile-scroll-") as folder:
    source = Path(folder) / "profile.c"
    binary = Path(folder) / "profile-test"
    source.write_text(fixture)
    subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                    str(source), "-o", str(binary), "-lm"], check=True)
    subprocess.run([str(binary)], check=True)
