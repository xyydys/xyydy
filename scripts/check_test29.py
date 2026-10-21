#!/usr/bin/env python3
"""Source contracts for the requested regressions; not an Apple SDK/device test."""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
s=(r/'ALBBottomSupport.swift').read_text()
t=(r/'Tweak.x').read_text()
g=(r/'ALB5BottomGeometry.m').read_text()
settings=(r/'ALBSettingsGlass.h').read_text()
def need(ok,msg):
    if not ok: raise SystemExit('TEST29: '+msg)
# TEST42 FIX8 (user request): the centered compact scheme is deleted. Only the
# native bottom material helper remains; it never replaces material by default.
need(not (r/'ALBCenterMorphSurface.swift').exists() and not (r/'ALBCenterMorphGeometry.swift').exists(),
     'centered compact renderer returned')
need('snapshotViewAfterScreenUpdates' not in g and 'ALB27CaptureCenterChrome' not in g,
     'native bar capture returned')
need('ALB27SnapshotForeground' not in g and 'ALB33BeginForegroundLease' not in g, 'orphaned center snapshot/lease code')
need('ALBBottomMaterialMode' not in s, 'duplicate material owner returned')
need('ALBMaterialApplyBar(nativeBar, YES);' in t, 'unified bottom material missing')
need('openMaterialOptions:' in t, 'material mode selector missing')
slot=t.split('@implementation AWEHomeTopSlotView',1)[1].split('@end',1)[0]
hit=slot.split('- (UIView *)hitTest:',1)[1].split('- (void)setHidden:',1)[0]
need(hit.index('AWELeaseSharedSearchGestureToView(')<hit.index('[super hitTest:'),
     'search recognizer is bound after hit testing')
need('event.allTouches' not in hit,'first hit still requires a Began touch')
need('top.remove-old-lease' not in t,'layout unbinds the working search recognizer')
need('sheetProgress' not in settings and 'dragSheet:' not in settings,'fake half-sheet is still active')
for token in ['UIModalPresentationPageSheet','UISheetPresentationControllerDetent.mediumDetent',
              'UISheetPresentationControllerDetent.largeDetent','sheet.prefersScrollingExpandsWhenScrolledToEdge=NO']:
    need(token in settings, 'native 1.0.56 presentation missing: '+token)
need('UIBlurEffectStyleSystemThickMaterial' not in settings,'opaque section material returned')
need('ALBClearGlassEffect(NO)' in settings,'settings clear background removed')
shop=t.split('static void AWEBottomV21PrepareShop(',1)[1].split('static void AWEBottomV22ClearShopNativeBarFast',1)[0]
need('AWEBottomV16ClearShopBottomCovers(window, reason)' in shop,'active mall path skips cover cleanup')
route=t.split('static void AWEExtendRouteContentBehindBottomBar(',1)[1].split('static void AWEClearCachedBottomCarrier',1)[0]
need(route.index('ALB28PrepareGridBottom')<route.index('if (route == AWEBottomRouteNone) return;'),
     'unknown grid class cannot reach actual-cell detection')
need('overlap>40' in t,'waterfall detector still requires identical row tops')
print('TEST29 native-material / foreground / search / sheet / background source contracts passed; no SDK/device rendering claim.')
