#!/usr/bin/env python3
"""Current production-source invariants; explicitly NOT UIKit verification."""
from pathlib import Path
import re
r=Path(__file__).resolve().parents[1]
sw=(r/'AwemeAlbumMorphBar.swift').read_text();top=(r/'ALBTopNative/ALBTopNativeGroup.m').read_text()
t=(r/'Tweak.x').read_text();root=(r/'ALBRootScroll.h').read_text();settings=(r/'ALB5Settings.h').read_text()
bind=(r/'ALBNativeMinimize.h').read_text();comment=(r/'CommentGlassDYKiller.xm').read_text()
def need(ok,msg):
    if not ok: raise SystemExit('TEST24 contract: '+msg)
need('AWEAlbumMorphRequestNativeMinimized(self,' in sw,'native direction has no UIKit endpoint')
need('setContentScrollView:feed' not in bind and 'ALBBindMinimizeSources(' not in sw,'foreign list binding returned')
need('setContentOffset' not in bind and 'setDelegate' not in bind and 'dispatch_after' not in bind,'observer mutates host or adds retry loop')
need('UISearchTab' in top and 'top.action.parking' in top,'sides are not native auxiliary actions')
need('initWithViewControllerProvider:' in top and 'fitAuxiliaryActionToSize:' in top,'auxiliary construction/fit absent')
start=top.index('- (void)updateIdleActionLensVisibility {');end=top.index('- (BOOL)handleNativeItemSelection:',start)
need('opacity =' not in top[start:end] and '.hidden=' not in top[start:end],'selected lens hidden again')
need('top.sideSelectionCapsule' not in settings,'obsolete capsule option still shown')
need('ALB15CompactStyle()!=0 || !ALB5Bool(@"collapse.timer"' in root,'timer leaks into native-left mode')
# FIX8: expand-on-leave belonged to the deleted centered mode only.
need('ALB8ExpandOnLeaveFeed()' not in sw,'expand-on-leave leaks into native-left mode')
need('UIKit' in bind and 'sourceScrollView' in sw,'missing native driver')
need('HOME_RESELECT_BLOCKED' in t,'Home reselect still forwarded')
need('ALB24ColumnMenuRow' not in t and 'definitions.count+1' not in t,'removed column row returned')
need('button.menu.children.count == definitions.count' in t,'stable menu signature no longer matches its actions')
need('kALB24MediaFeedLeaseKey' in comment and '[current isEqual:savedIndex]' in comment,'comment restore not tied to same item')
need('gALB24CommentWindowSize' in comment and 'generation != gAWECommentMediaSnapshotGeneration' in comment,'comment recovery lacks generation/rotation guard')
need('ALB24ClearNativeBottomBackdrops(window,route)' in t and 'AWEBottomRouteGrid' in t,'bottom backdrop route missing')
print('TEST24 production-source invariants passed; not an iOS build, native-animation test, or device test.')
