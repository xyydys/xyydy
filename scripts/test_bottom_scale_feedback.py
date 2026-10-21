#!/usr/bin/env python3
"""Run extracted scale arithmetic against a changing child-inset model.

The production Swift expressions are used by the C harness. This verifies
anchor stability and repeat-call idempotence, not UIKit or device behavior.
"""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

from check_logos_boundaries import mask_literals

root = Path(__file__).resolve().parents[1]
swift = (root / "AwemeAlbumMorphBar.swift").read_text()
tweak = (root / "Tweak.x").read_text()


def body(source, signature):
    start = source.index("{", source.index(signature))
    masked = mask_literals(source)
    depth = 0
    for index in range(start, len(source)):
        depth += (masked[index] == "{") - (masked[index] == "}")
        if depth == 0:
            return source[start + 1:index]
    raise AssertionError("Unclosed method: " + signature)


scale = body(swift, "private func applyStableVerticalOffset()")
offset = body(swift, "func setVerticalOffset(_ offset:")
layout = body(swift, "override func viewDidLayoutSubviews()")
host = body(tweak, "static BOOL AWEIOS27EnsureNativeSameWindowHost(UIWindow *sourceWindow) {")
alignment = body(tweak, "static void AWEAlignAlbumMorphOverlayToOriginalBar(")

# The ownership checks make the mathematical invariant below meaningful: the
# inset must come from the untransformed parent, never the transformed child.
scale_code = re.sub(r"//[^\n]*", "", scale)
assert "view.safeAreaInsets" not in scale_code
assert 'canvas.accessibilityIdentifier == "AwemeLiquidBar.BottomScaleCanvas"' in scale_code
assert "let host = canvas.superview" in scale_code
assert "canvas.transform = target" in scale_code
assert "view.transform = target" not in scale_code
assert "let bottomInset = host.safeAreaInsets.bottom" in scale_code
assert "view.transform.ty - clamped" not in offset
assert "let changed = abs(verticalOffset - clamped) > 0.01" in offset
assert "guard changed else { return }" in offset
assert "defer { bottomLayoutInProgress = false }" in layout
assert "UIViewAutoresizingNone" in host
assert "if (!AWEFrameEqualEnough(chrome.bounds, host.bounds))" in host
assert "if (!AWEFrameEqualEnough(host.frame, root.bounds))" in host
assert "fabs(chrome.center.x-center.x) > 0.01" in host
assert "gAWEAlbumMorphController.view.bounds =" not in alignment
overlay = body(tweak, "static BOOL AWEEnsureAlbumMorphOverlayWindow(UIWindow *sourceWindow) {")
assert "overlay.rootViewController = gAWEAlbumMorphController" not in overlay
assert overlay.count("overlay.rootViewController = [ALB33BottomOverlayController new]") == 2
canvas_host = tweak.split("@implementation AWEIOS27NativeBarHostView",1)[1].split("@end",1)[0]
assert "canvas.bounds=self.bounds" in canvas_host and "canvas.center=center" in canvas_host
assert "canvas.frame=" not in canvas_host and "chrome.frame=" not in canvas_host


pivot = re.search(r"let pivotY = ([^\n]+)", scale).group(1)
translation = re.search(r"ty:\s*([^\n]+)\)\s*\n", scale).group(1)
pivot = pivot.replace("host.bounds.height", "height")
translation = translation.replace("host.bounds.midY", "(height / 2.0)")
assert re.fullmatch(r"[\w\s.()+*/-]+", pivot + translation)

fixture = r"""
#include <assert.h>
#include <math.h>
#include <stdio.h>
static double production_translation(double scale, double verticalOffset,
                                     double height, double bottomInset) {
    double pivotY = PIVOT;
    return TRANSLATION;
}
int main(void) {
    const double heights[] = {667, 812, 896, 956};
    const double insets[] = {0, 20, 21, 34, 59};
    const double offsets[] = {-24, 0, 12, 40, 80};
    unsigned traces = 0;
    for (int si = 0; si <= 60; si++) {
        double scale = 0.55 + si * 0.01;
        for (int hi = 0; hi < 4; hi++) for (int bi = 0; bi < 5; bi++)
        for (int oi = 0; oi < 5; oi++) {
            double h = heights[hi], b = insets[bi], off = offsets[oi];
            double last = 1e30;
            unsigned writes = 0;
            // Native layout may report a different safe area for the scaled
            // child on each pass. The parent's safe area stays constant.
            for (int pass = 0; pass < 100; pass++) {
                double childInset = (pass % 3) * 17.25;
                (void)childInset;
                double ty = production_translation(scale, off, h, b);
                if (fabs(last - ty) > 0.01) { last = ty; writes++; }
                double pivot = h - b - 30;
                double renderedPivot = (pivot - h / 2.0) * scale + h / 2.0 + ty;
                assert(fabs(renderedPivot - (pivot + off)) < 1e-9);
            }
            assert(writes == 1);
            // Host geometry changes, such as rotation, use the new inset on
            // the next pass without accumulating translation from last time.
            double next = production_translation(scale, off, h, b + 8);
            assert(fabs((next - last) + (1 - scale) * 8) < 1e-9);
            traces++;
        }
    }
    puts("Bottom scale: 6100 anchor/idempotence traces (610000 model passes); no UIKit/device test.");
    assert(traces == 6100);
    return 0;
}
""".replace("PIVOT", pivot).replace("TRANSLATION", translation)
compiler = shutil.which("clang") or shutil.which("cc")
if not compiler:
    raise SystemExit("A C compiler is required for the extracted scale test")
with tempfile.TemporaryDirectory(prefix="alb-scale-feedback-") as folder:
    source = Path(folder) / "scale.c"
    binary = Path(folder) / "scale-test"
    source.write_text(fixture)
    subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                    str(source), "-o", str(binary), "-lm"], check=True)
    subprocess.run([str(binary)], check=True)
