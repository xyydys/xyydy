#ifndef ALB_GLASS_SEED_H
#define ALB_GLASS_SEED_H

/* Included once, after ALBGlassImmediate.h, by Tweak.x.

   3.11.24 新玻璃直接到位 ("glass.directTone", default on; iOS 26).

   Why a new right-column / panel glass starts dark: every face, its edge and
   its glass button are Dark (3.11.10: on a Light edge the light glass is the
   whiter kind). UIKit's adaptive glass starts a new or rebuilt glass from
   that style and only turns it to the see-through light kind when the
   luminance it is handed crosses its threshold. That luminance reaches the
   glass through -_materialDidReceiveLuma: (UIView, and the glass group view
   that overrides it), one reading per luma update (0.25 s), and a rebuilt
   backdrop's readings walk toward the real value over several readings
   (3.11.13: 0.62, 0.70, 0.77 → light): about one second of dark. The top
   chrome never rebuilds, so it never shows it.

   iOS 26.1 UIKit (decompiled): -[UIView _materialDidReceiveLuma:] only
   hands the value to its superview; the glass group view
   (GlassGroupLayerView) overrides it and applies every reading inside a
   spring (response 0.5, damping 1). So a reading starts at a view near the
   backdrop and bubbles up to the glass group.

   This file sits on that entry, for the right column and the double-tap
   panel only (the top chrome is counted for reference, the bottom bar and
   everything else pass straight through):
   - a new or rebuilt glass is handed a start before its first frame,
     without animation: the panel, the right column's settled readings at
     that height (or their median when they agree); the right column after it
     was hidden (panel, Douyin's covers), its own last settled reading of the
     same video;
   - while that glass's own readings are still walking from where a rebuilt
     backdrop starts toward the start it was handed, the start is kept (they
     only catch up), then its own settled reading takes over;
   - every finished walk is measured; once the walk is known on this device
     (its first reading predicts where it settles, checked on finished
     walks), a new glass's reading is turned into where it will settle (a
     swiped-in video too, from the reading it had before);
   - the first value a new glass is handed is applied without animation.
   UIKit's own glass, readings, threshold and light/dark decision are kept:
   nothing is drawn over the glass, delayed, locked or tinted.

   3.11.25: the user's 3.11.24 diagnostic: hook installed, 598 readings in
   four minutes, every one at UISDFBackdropView outside the right column and
   the panel (the bottom bar's glass), none for the right column, the panel or
   the top chrome. Bar-item glass registered on an edge does not take its
   light/dark from that entry; it follows the edge's pocket
   (ScrollEdgeEffectView: its BackdropView's -backdropLayer:didChangeLuma:
   readings, its lumaStyle; 3.10.5 one shared edge flips every glass on it,
   3.11.10 the edge's style decides the material, 3.11.23 the pocket's
   luminance layer turns with the glass). The same start / hold / prediction
   now also runs on that pocket entry, for our own edges only; the pocket's
   state (-[UIScrollView _lumaStyleForEdge:]) is sampled around each
   appearance, and readings that are not ours record where they come from.

   3.11.26: the 3.11.25 diagnostic: the pocket's readings arrive at their
   true value right away (no walk to catch up with), and the user's report:
   after the panel, Douyin's long-press menu or a profile the right column
   stays dark until the background really changes; closing the comments it
   goes dark and comes back by itself. The lifecycle log of all three
   versions: while the column is hidden its pocket's backdrops leave the
   edge (EB[]); the same ones come back, and every time at the pocket's
   starting level (the third backdrop's opacity 0.85; 0.60 / 0.10 once it
   has looked at a reading). A pocket that comes back still holds the last
   reading its BackdropView took (currentLuma, Double?, nil after init) and
   the last luma it looked at itself (previousEffectiveLuma); readings about
   the same as those change nothing, so it keeps the starting level until
   the background moves enough (the comments: the video grows back). A new
   pocket (the panel's) has neither and looks at its first reading. So when
   a pocket of ours comes back (its didMoveToWindow, after its own subviews
   did theirs): both fields go back to nil, as UIKit's init leaves them
   (only when the field is laid out as a Double? and holds nil or a
   luminance; otherwise it is never written), and the pocket is handed its
   own last reading again (same video) or the start waiting on its button,
   without animation, right then and shortly after; for its first second
   every reading is looked at as new. The equal-value hand-over of 3.11.25
   (which only refreshed the held reading) is gone. Each comeback is traced
   (levels, the two fields, the pocket's lumaStyle) for the diagnostic.

   3.11.28: the 3.11.27 diagnostic: right after the hand-over the pocket is
   still at 0.85 with its evaluated luma nil; it decides within 0.1 s. And
   the right column's pocket comes back 0.06–0.12 s after the column is shown
   again, while its glass is built again at once. So: (1) after every
   hand-over (and every reading in a pocket's first second) the pocket is
   laid out now and the one-shot timers made while it was handed fire now
   (ALBSeedDeliverAndSettle; which step decided it is counted); (2) the
   panel hides the right column underneath UIKit's visibility change
   (ALB5RightLayout.h) and fades its own group in from / out to 0.012
   (ALBSidePalette.h), so neither glass loses its pocket or is built again
   on the way; (3) the panel's windows end when it starts closing.

   3.11.29: the 3.11.28 diagnostic: the pocket decides on its layout (2126
   times "after layout", 0 timers captured); no quiet hide happened (UIKit
   26's _backing_setAlpha is a direct function: ALB5RightLayout.h now sets
   the layer's opacity); a hidden pocket's backdrop reported 47 readings
   down to 0.00 (not drawn = black). The user: a single video no longer goes
   dark; the panel still takes a moment; a video swiped in still goes dark
   to light. A new pocket starts dark because it has not been measured yet,
   and the first measurement comes one luma period (0.25 s) after its first
   frame. So: readings taken while our glass is not drawn never reach the
   pocket; from a pocket's (re)entry until its first reading on screen its
   backdrop reports luma at 60 per second (CABackdropLayer lumaUpdateRate,
   only when UIKit's value reads as 4/s or 0.25 s), then UIKit's rate again.

   3.11.30: the 3.11.29 diagnostic: the panel hide is now quiet (1108/1108,
   no fallback); UIKit's rate read 0.25 and was set to 1/60, but 3.11.17
   already found the interval cannot go below 0.25 s: withdrawn. The user:
   the panel is right from its first frame on some videos, not on others
   (it is when it got a start: "起步26/26"; not when it got none:
   "起步0/26", panel on the left, right-column readings too far apart); a
   video swiped in still goes dark to light (its cell's pockets enter off
   screen and have never measured anything). Nothing can measure a place
   before it is drawn, so: the panel gets the luma of small filterless
   backdrop probes at its positions (measured by the system the same way,
   continuously); a cell about to come on screen gets this position's last
   reading of this video, else the luma of the cell's own cover picture
   behind each button, handed to its pockets while off screen; a first
   reading of 0 after such a value is the capture not being ready and is
   not handed on.

   3.11.31: the 3.11.30 diagnostic: the probes work (3916 readings, 434
   panel starts; the user: the panel is now rarely dark); the cover search
   took Douyin's pause icon (a cell-sized view with a small centred image),
   so no cover value was ever given (88 values, all "seen before"). Now only
   a picture that fills its view and is large enough counts, and a video no
   one has measured starts from its position's typical brightness of late
   (median of the last 15 first readings there): most readings at most
   positions fall in the middle level, which the default dark start never
   matches.

   3.11.32: the 3.11.31 diagnostic: 252 values before the first frame (165
   cover, 85 typical, 2 seen); covers read far brighter than the pocket then
   did (Douyin darkens the picture under the buttons): each position now
   learns the pocket-to-cover ratio from its own pairs (every position's
   until it has two, 0.5 before any). 94 cells had no cover: a small blurred
   preview (32×60) and a photo post (440×587 of the cell) now count too. */

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#import <objc/message.h>
#import <math.h>
#import <string.h>
#import <stdlib.h>

typedef void (*ALBSeedLumaIMP)(id, SEL, double);
typedef void (*ALBSeedLayoutIMP)(id, SEL);

#define ALB_SEED_MAX_CLASSES 12
static Class gALBSeedClasses[ALB_SEED_MAX_CLASSES];
static ALBSeedLumaIMP gALBSeedOriginals[ALB_SEED_MAX_CLASSES];
static NSUInteger gALBSeedClassCount, gALBSeedLayoutHooks;
static BOOL gALBSeedInstallAttempted;
static NSString *gALBSeedStatus = @"未安装";
static NSMutableArray<NSString *> *gALBSeedHookNames;
// Deliveries nested inside one (UIKit passing a value on) pass straight through.
static NSUInteger gALBSeedDepth;
static CFTimeInterval gALBSeedT0;
static NSUInteger gALBSeedSerial, gALBSeedToken;
static NSUInteger gALBSeedReadsRight, gALBSeedReadsPanel, gALBSeedReadsTop, gALBSeedReadsOther;
static NSUInteger gALBSeedFreshRight, gALBSeedFreshPanel, gALBSeedVideoRight;
static NSUInteger gALBSeedInjectRight, gALBSeedInjectPanel, gALBSeedInjectLayout;
static NSUInteger gALBSeedFirstInstant, gALBSeedHeld, gALBSeedPredicted, gALBSeedExceptions;
static NSUInteger gALBSeedPanelStarts, gALBSeedPanelNoStart, gALBSeedRightPending;
static NSMutableSet<NSString *> *gALBSeedOurClasses, *gALBSeedOtherClasses;
static NSMutableArray<NSString *> *gALBSeedEvents;
static NSMutableArray<NSDictionary *> *gALBSeedSamples;
// 3.11.25: the edge pocket's luminance entry.
typedef void (*ALBSeedPocketIMP)(id, SEL, id, double);
static Class gALBSeedPocketClass, gALBSeedPocketViewClass;
static ALBSeedPocketIMP gALBSeedPocketOriginal;
static NSString *gALBSeedPocketStatus = @"未装";
static NSUInteger gALBSeedPocketReadsRight, gALBSeedPocketReadsPanel, gALBSeedPocketReadsOther;
static NSMutableArray<NSString *> *gALBSeedOtherChains;
// 3.11.26: a pocket of ours that comes back looks at its reading again.
static Class gALBSeedBlurClass;
static NSUInteger gALBSeedEntryHooks;
static NSString *gALBSeedEntryHook = @"未装";
// Where the two Double? fields sit (-1 none, -2 refused: never written again);
// the *At copies stay for reading in the diagnostic.
static ptrdiff_t gALBSeedCurrentLumaOffset = -1, gALBSeedPreviousLumaOffset = -1;
static ptrdiff_t gALBSeedCurrentLumaAt = -1, gALBSeedPreviousLumaAt = -1;
static BOOL gALBSeedOffsetsLooked;
static NSString *gALBSeedOffsetsText = @"未查";
static NSUInteger gALBSeedEntries, gALBSeedReplays, gALBSeedFreshReads;
static NSUInteger gALBSeedForgotBackdrop, gALBSeedForgotPocket, gALBSeedForgetRefused;
static CFTimeInterval gALBSeedLastTraceAt;
static NSUInteger gALBSeedTraceSerial;
#define ALB_SEED_ENTRY_WINDOW 1.0
// 3.11.27: the latest end of any owner's snap window (ALBGlassImmediate.h
// looks at it first), and when the panel last let the right column back.
static CFTimeInterval gALBSeedSnapUntil, gALBSeedRightRestoredAt;
static NSUInteger gALBSeedSnapWindows, gALBSeedAppliedNow;
#define ALB_SEED_SNAP_WINDOW 1.5
// 3.11.28: the 3.11.27 diagnostic: right after the hand-over the pocket has
// not decided yet (its evaluated luma still nil, level still 0.85); it
// decides within 0.1 s. While we hand a pocket a value, the one-shot
// timers created (its settle timer, if it is one) are kept; after it, the
// pocket is laid out and, if still undecided, they fire now.
static NSMutableArray<NSTimer *> *gALBSeedCapturedTimers;
static NSUInteger gALBSeedTimerHooks, gALBSeedTimersCaptured, gALBSeedTimersFired;
static NSUInteger gALBSeedSettled[5];   // decided: while handed, after layout, after timers, not yet, unknown
static NSUInteger gALBSeedHiddenReads;  // right-column pocket readings while the panel hides the column
static double gALBSeedHiddenLow = NAN, gALBSeedHiddenHigh = NAN;
// 3.11.29: a backdrop that is not drawn (its glass hidden) reports luma 0
// (3.11.28: 47 readings 0.00–0.78 while the panel hid the column); those
// readings are not the background and never reach our pockets. And a new
// or returning pocket's backdrop reports luma faster (CABackdropLayer
// lumaUpdateRate) until its first reading on screen.
static NSUInteger gALBSeedDroppedUnseen, gALBSeedBoosts, gALBSeedBoostEnds, gALBSeedBoostSkipped;
static double gALBSeedRateSeen = NAN, gALBSeedRateSet = NAN;
static NSMutableArray<NSString *> *gALBSeedFirstReads;
static double gALBSeedUnseenLow = NAN, gALBSeedUnseenHigh = NAN;
/* 3.11.30: brightness that exists before a glass is first drawn.
   - Each right-column button position's last reading per video (a video
     swiped back to, or its cell built again).
   - A video swiped in: the luma of the cell's own full-cell picture (the
     cover Douyin shows until the video plays) behind each button.
   - The panel: small backdrop layers without filters (they draw exactly
     what is behind them) at the panel's positions, read by the system's
     own luma measurement.
   Each value is only handed to the system's pocket, which decides the
   level itself; the first real reading decides again. */
static NSMutableDictionary<NSString *, NSNumber *> *gALBSeedVideoLuma;
static NSUInteger gALBSeedFeedChecks, gALBSeedFeedSeeds, gALBSeedFeedFromCache, gALBSeedFeedFromCover;
static NSUInteger gALBSeedCoverMissing, gALBSeedZeroDropped;
static NSMutableArray<NSString *> *gALBSeedEstimatePairs, *gALBSeedCoverNotes;
static CFTimeInterval gALBSeedFeedCheckedAt;
// 3.11.31: each right-column position's first readings of recent videos
// (any video), whose median starts a video no one has measured yet.
static NSMutableDictionary<NSString *, NSMutableArray<NSNumber *> *> *gALBSeedPositionPrior;
static NSUInteger gALBSeedFeedFromPrior;
// 3.11.32: the 3.11.31 diagnostic: cover estimates read much brighter than
// the pocket then did (0.55/0.12, 0.78/0.14, 0.61/0.22: Douyin darkens the
// lower part of the picture under the buttons, and the pocket sees that).
// Each position learns how much darker the pocket reads than the cover there
// (median of reading/cover over its last pairs); every position's pairs
// together until it has its own.
static NSMutableDictionary<NSString *, NSMutableArray<NSNumber *> *> *gALBSeedCoverRatios;
static NSMutableArray<NSNumber *> *gALBSeedCoverRatiosAll;

/* A button wearing a face (right column, panel icon or pill) or a panel
   slider: its settled reading and a start waiting for its next new glass. */
@interface ALBSeedFace : NSObject
@property(nonatomic) BOOL hasStable;
@property(nonatomic) double stable;
@property(nonatomic) CFTimeInterval stableAt;
@property(nonatomic, copy) NSString *stableVideo;
@property(nonatomic) double pending;
@property(nonatomic) CFTimeInterval pendingAt, pendingUntil;
@property(nonatomic) NSUInteger pendingToken;
@property(nonatomic, copy) NSString *pendingSource;
@property(nonatomic) CFTimeInterval injectedAt;
@property(nonatomic, copy) NSString *video;
@property(nonatomic) CFTimeInterval videoAt;
@property(nonatomic) CFTimeInterval snapUntil;   // 3.11.27: its glass's tone changes finish at once until then
@property(nonatomic, copy) NSString *coverVideo;  // 3.11.30: the video its incoming pockets were given a value for
@property(nonatomic) CFTimeInterval coverCheckedAt;
@end
@implementation ALBSeedFace
@end

/* One view UIKit hands luminance to. */
@interface ALBSeedReceiver : NSObject
@property(nonatomic) NSInteger kind;            // 0 other, 1 right, 2 panel, 3 top chrome
@property(nonatomic) BOOL pocket;               // an edge pocket's backdrop (3.11.25), not a glass
@property(nonatomic, weak) UIView *owner;
@property(nonatomic) NSUInteger serial;
@property(nonatomic) CFTimeInterval classifiedAt;
@property(nonatomic, strong) NSHashTable *backdrops;
@property(nonatomic) BOOL hadBackdrops;
@property(nonatomic) BOOL hasLast;
@property(nonatomic) double last;
@property(nonatomic) CFTimeInterval lastAt;
@property(nonatomic, copy) NSString *lastVideo;
@property(nonatomic) NSInteger epoch;           // 0 none, 1 new/rebuilt glass, 2 same glass, another video, 3 first reading after its button got a start
@property(nonatomic) CFTimeInterval epochAt, epochRef;
@property(nonatomic) double start;              // where the walk starts (epoch 2), NAN when unknown
@property(nonatomic) double seed;               // the start handed to the glass, NAN when none
@property(nonatomic, copy) NSString *seedSource;
@property(nonatomic) NSUInteger injectedToken;
@property(nonatomic) BOOL injectedNow;
@property(nonatomic) BOOL instantDone;
@property(nonatomic) CFTimeInterval firstAfter;
@property(nonatomic, strong) NSMutableArray<NSNumber *> *readings;
@property(nonatomic, strong) NSMutableArray<NSNumber *> *given;
@property(nonatomic) CFTimeInterval entryAt;    // a pocket of ours came back (3.11.26), 0 never
@property(nonatomic) BOOL boosted;              // (3.11.29, withdrawn in 3.11.30)
@property(nonatomic) double rateBefore;
@property(nonatomic) NSUInteger boostReads;
@property(nonatomic) double estimate;           // 3.11.30: value handed before it was ever seen (NAN none)
@property(nonatomic, copy) NSString *estimateSource;
@property(nonatomic, copy) NSString *estimateVideo;
@property(nonatomic) NSUInteger realReads;      // readings since that estimate
@property(nonatomic, copy) NSString *priorVideo; // 3.11.31: the video whose first reading went into the position's history
@property(nonatomic) double estimateRaw;         // 3.11.32: the cover's own luma behind it (before the ratio), NAN none
@property(nonatomic) NSUInteger traceID;
@property(nonatomic, strong) NSMutableArray<NSString *> *trace;
@end
@implementation ALBSeedReceiver
@end

static char kALBSeedReceiverKey, kALBSeedFaceKey;

static BOOL ALBSeedEnabled(void) {
    static BOOL cached;
    static CFTimeInterval at;
    CFTimeInterval now = CACurrentMediaTime();
    if (at > 0 && now - at < 1.0) return cached;
    at = now;
    // 3.11.51：开关删了，固定开（本来就只管右侧按钮和双击面板）
    cached = AWEPluginFeaturesEnabled() && ALB25MaterialFollowBar();
    return cached;
}

static void ALBSeedNote(NSString *line) {
    if (!gALBSeedEvents) gALBSeedEvents = [NSMutableArray array];
    if (gALBSeedEvents.count >= 24) [gALBSeedEvents removeObjectAtIndex:0];
    [gALBSeedEvents addObject:[NSString stringWithFormat:@"+%.2fs %@", MAX(0, CACurrentMediaTime() - gALBSeedT0), line]];
}

static NSString *ALBSeedList(NSArray<NSNumber *> *values) {
    NSMutableArray *parts = [NSMutableArray array];
    for (NSNumber *value in values) {
        if (parts.count >= 7) { [parts addObject:@"…"]; break; }
        [parts addObject:[NSString stringWithFormat:@"%.2f", value.doubleValue]];
    }
    return [parts componentsJoinedByString:@" "];
}

/* The glass an edge of ours (an empty scroll view right under it,
   ALBFollowSyncEdge) belongs to. */
static UIView *ALBSeedGlassForEdge(UIView *edge) {
    if (![edge.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.GlassEdge"]) return nil;
    for (UIView *sibling in edge.superview.subviews)
        if (sibling != edge && objc_getAssociatedObject(sibling, &kALBFollowProxyKey) == edge) return sibling;
    return nil;
}

/* 1 right column, 2 panel (a face on a panel button, or a slider), 3 top
   chrome, 0 anything else. A view inside one of our edges (its pocket)
   belongs to that edge's glass. */
static NSInteger ALBSeedClassify(UIView *view, UIView **ownerOut) {
    static Class faceClass, rightClass, artworkClass, sliderClass, topClass;
    static BOOL looked;
    if (!looked) {
        looked = YES;
        faceClass = NSClassFromString(@"ALB3BarGlassButton");
        rightClass = NSClassFromString(@"ALBRightGlassButton");
        artworkClass = NSClassFromString(@"ALBArtworkGlassButton");
        sliderClass = NSClassFromString(@"ALBPaletteSlider");
        topClass = NSClassFromString(@"ALB3TopChromeView");
    }
    NSUInteger depth = 0;
    for (UIView *cursor = view; cursor && depth < 48; cursor = cursor.superview, depth++) {
        if (topClass && [cursor isKindOfClass:topClass]) return 3;
        if ([cursor isKindOfClass:UITabBar.class]) return 0;
        if (sliderClass && [cursor isKindOfClass:sliderClass]) { *ownerOut = cursor; return 2; }
        if ([cursor isKindOfClass:UIScrollView.class]) {
            UIView *glass = ALBSeedGlassForEdge(cursor);
            if (glass && glass != view) return ALBSeedClassify(glass, ownerOut);
            continue;
        }
        if (faceClass && [cursor isKindOfClass:faceClass]) {
            UIView *button = cursor.superview;
            if (![button isKindOfClass:UIButton.class] || (UIView *)ALBFaceFor(button) != cursor) continue;
            if ((rightClass && [button isKindOfClass:rightClass]) || (artworkClass && [button isKindOfClass:artworkClass])) {
                *ownerOut = button; return 1;
            }
            if ([button.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar.SidePalette."]) { *ownerOut = button; return 2; }
            return 0;
        }
        // A glass hosted somewhere else inside one of our buttons.
        if ((rightClass && [cursor isKindOfClass:rightClass]) || (artworkClass && [cursor isKindOfClass:artworkClass])) {
            *ownerOut = cursor; return 1;
        }
        if ([cursor isKindOfClass:UIButton.class] && [cursor.accessibilityIdentifier hasPrefix:@"AwemeLiquidBar.SidePalette."]) {
            *ownerOut = cursor; return 2;
        }
    }
    return 0;
}

/* Where a reading that is not ours comes from (诊断, first few). */
static void ALBSeedNoteOtherChain(UIView *view) {
    if (!gALBSeedOtherChains) gALBSeedOtherChains = [NSMutableArray array];
    if (gALBSeedOtherChains.count >= 4) return;
    NSMutableArray<NSString *> *parts = [NSMutableArray array];
    NSUInteger depth = 0;
    for (UIView *cursor = view; cursor && depth < 12; cursor = cursor.superview, depth++) {
        NSString *name = NSStringFromClass(cursor.class);
        if (name.length > 26) name = [name substringFromIndex:name.length - 26];
        [parts addObject:name];
    }
    NSString *chain = [parts componentsJoinedByString:@"<"];
    for (NSString *seen in gALBSeedOtherChains) if ([seen isEqualToString:chain]) return;
    [gALBSeedOtherChains addObject:chain];
}

static ALBSeedReceiver *ALBSeedReceiverFor(UIView *view) {
    ALBSeedReceiver *state = objc_getAssociatedObject(view, &kALBSeedReceiverKey);
    // (A glass that reported before its face was worn is looked at again.)
    if (state && (state.kind == 3 || state.owner ||
                  (state.kind == 0 && CACurrentMediaTime() - state.classifiedAt < 2.0))) return state;
    if (!view.window) return state;
    UIView *owner = nil;
    NSInteger kind = ALBSeedClassify(view, &owner);
    if (!state) {
        state = [ALBSeedReceiver new];
        state.serial = ++gALBSeedSerial;
        state.start = NAN;
        state.seed = NAN;
        state.estimate = NAN;
        state.estimateRaw = NAN;
        objc_setAssociatedObject(view, &kALBSeedReceiverKey, state, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    state.kind = kind;
    state.owner = owner;
    state.pocket = gALBSeedPocketClass && [view isKindOfClass:gALBSeedPocketClass];
    state.classifiedAt = CACurrentMediaTime();
    return state;
}

static ALBSeedFace *ALBSeedFaceFor(UIView *owner, BOOL create) {
    if (!owner) return nil;
    ALBSeedFace *face = objc_getAssociatedObject(owner, &kALBSeedFaceKey);
    if (!face && create) {
        face = [ALBSeedFace new];
        face.pending = NAN;
        objc_setAssociatedObject(owner, &kALBSeedFaceKey, face, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return face;
}

/* 3.11.27: from now on, for 1.5 s, the tone changes of this owner's glass
   and its pocket's level finish at once (ALBGlassImmediate.h). */
static void ALBSeedStartSnap(UIView *owner) {
    if (!owner) return;
    ALBSeedFace *face = ALBSeedFaceFor(owner, YES);
    CFTimeInterval until = CACurrentMediaTime() + ALB_SEED_SNAP_WINDOW;
    if (face.snapUntil < until) { face.snapUntil = until; gALBSeedSnapWindows++; }
    if (gALBSeedSnapUntil < until) gALBSeedSnapUntil = until;
}

static BOOL ALBGlassSeedSnapActive(void) {
    return gALBSeedSnapUntil > CACurrentMediaTime() && ALBSeedEnabled();
}

static BOOL ALBGlassSeedSnapping(UIView *owner) {
    if (!owner) return NO;
    ALBSeedFace *face = objc_getAssociatedObject(owner, &kALBSeedFaceKey);
    return face && face.snapUntil > CACurrentMediaTime();
}

/* The video a right-column button belongs to (its cell's own model). */
static NSString *ALBSeedVideo(UIView *owner, BOOL fresh) {
    if (!owner.window) return nil;
    ALBSeedFace *face = ALBSeedFaceFor(owner, YES);
    CFTimeInterval now = CACurrentMediaTime();
    // A button's video only changes while it is off screen (no readings then):
    // the first reading after a pause looks again (fresh).
    if (!fresh && face.video && now - face.videoAt < 10.0) return face.video;
    id model = nil;
    @try { model = ALB8PaletteModelForStack(owner); } @catch (__unused NSException *exception) { model = nil; }
    face.video = model ? AWEAwemeIdentity(model) : nil;
    face.videoAt = now;
    return face.video;
}

static NSArray<CALayer *> *ALBSeedBackdrops(UIView *view) {
    static Class backdropClass;
    if (!backdropClass) backdropClass = NSClassFromString(@"CABackdropLayer");
    NSMutableArray<CALayer *> *found = [NSMutableArray array];
    if (!backdropClass || !view.layer) return found;
    NSMutableArray<CALayer *> *queue = [NSMutableArray arrayWithObject:view.layer];
    for (NSUInteger i = 0; i < queue.count && i < 96 && found.count < 4; i++) {
        CALayer *layer = queue[i];
        if ([layer isKindOfClass:backdropClass]) [found addObject:layer];
        NSArray<CALayer *> *sublayers = layer.sublayers;
        if (sublayers.count && queue.count < 96) [queue addObjectsFromArray:sublayers];
    }
    return found;
}

/* The glass's backdrops are all new (UIKit tore them down while it was
   hidden and built them again). `remember` keeps the current ones. */
static BOOL ALBSeedBackdropsRebuilt(ALBSeedReceiver *state, NSArray<CALayer *> *now, BOOL remember) {
    BOOL overlap = NO;
    for (CALayer *layer in now) if ([state.backdrops containsObject:layer]) { overlap = YES; break; }
    BOOL rebuilt = state.hadBackdrops && now.count && !overlap;
    if (remember && now.count) {
        NSHashTable *table = [NSHashTable weakObjectsHashTable];
        for (CALayer *layer in now) [table addObject:layer];
        state.backdrops = table;
        state.hadBackdrops = YES;
    }
    return rebuilt;
}

#pragma mark - The walk, measured on this device

/* A restarted glass's readings r1, r2, … go from where they start (s) to
   where they settle (T): r_n = s + (T - s)·g_n, the same g_n for every walk
   whatever its shape (each reading closing a share of the gap, or an
   average over a window). g_n from finished walks whose start is known (the
   glass's previous reading when another video slid in), s for a rebuilt
   backdrop from walks that settle on different values; trusted once the
   first reading of the finished walks predicts where they settled. */
#define ALB_SEED_STEPS 6
typedef struct {
    NSInteger samples, informative, tested, hits;
    double g[ALB_SEED_STEPS];
    double s;
    BOOL validated, raw;
} ALBSeedModelInfo;
static ALBSeedModelInfo gALBSeedModel;
static BOOL gALBSeedModelDirty = YES;

static double ALBSeedMedian(NSMutableArray<NSNumber *> *values) {
    if (!values.count) return NAN;
    [values sortUsingSelector:@selector(compare:)];
    NSUInteger count = values.count;
    return count % 2 ? values[count / 2].doubleValue
        : (values[count / 2 - 1].doubleValue + values[count / 2].doubleValue) / 2.0;
}

static ALBSeedModelInfo ALBSeedModelNow(void) {
    if (!gALBSeedModelDirty) return gALBSeedModel;
    gALBSeedModelDirty = NO;
    ALBSeedModelInfo info;
    memset(&info, 0, sizeof info);
    info.s = NAN;
    NSMutableArray<NSMutableArray<NSNumber *> *> *steps = [NSMutableArray array];
    for (NSUInteger k = 0; k < ALB_SEED_STEPS; k++) [steps addObject:[NSMutableArray array]];
    NSMutableArray<NSDictionary *> *fresh = [NSMutableArray array];
    for (NSDictionary *sample in gALBSeedSamples) {
        NSArray<NSNumber *> *r = sample[@"r"];
        if (r.count < 2) continue;
        info.samples++;
        double target = r.lastObject.doubleValue;
        if ([sample[@"e"] integerValue] != 2) { [fresh addObject:sample]; continue; }
        double from = [sample[@"st"] doubleValue];
        if (fabs(target - from) < 0.12) continue;
        for (NSUInteger k = 0; k + 1 < r.count && k < ALB_SEED_STEPS; k++)
            [steps[k] addObject:@((r[k].doubleValue - from) / (target - from))];
    }
    // Where a rebuilt backdrop's readings start.
    double g1 = steps[0].count >= 3 ? ALBSeedMedian([steps[0] mutableCopy]) : NAN;
    if (!isfinite(g1) || g1 >= 0.95) {
        NSMutableArray<NSNumber *> *pairs = [NSMutableArray array];
        NSUInteger first = fresh.count > 24 ? fresh.count - 24 : 0;
        for (NSUInteger a = first; a < fresh.count; a++) for (NSUInteger b = a + 1; b < fresh.count; b++) {
            NSArray<NSNumber *> *ra = fresh[a][@"r"], *rb = fresh[b][@"r"];
            double ta = ra.lastObject.doubleValue, tb = rb.lastObject.doubleValue;
            if (fabs(ta - tb) < 0.2) continue;
            double g = (ra.firstObject.doubleValue - rb.firstObject.doubleValue) / (ta - tb);
            if (g > 0.02 && g < 1.2) [pairs addObject:@(g)];
        }
        if (pairs.count >= 3) g1 = ALBSeedMedian(pairs);
    }
    if (isfinite(g1) && g1 < 0.95) {
        NSMutableArray<NSNumber *> *starts = [NSMutableArray array];
        for (NSDictionary *sample in fresh) {
            NSArray<NSNumber *> *r = sample[@"r"];
            double s = (r.firstObject.doubleValue - r.lastObject.doubleValue * g1) / (1.0 - g1);
            if (s > -0.3 && s < 1.3) [starts addObject:@(s)];
        }
        if (starts.count >= 3) info.s = ALBSeedMedian(starts);
    }
    if (isfinite(info.s)) for (NSDictionary *sample in fresh) {
        NSArray<NSNumber *> *r = sample[@"r"];
        double target = r.lastObject.doubleValue;
        if (fabs(target - info.s) < 0.12) continue;
        for (NSUInteger k = 0; k + 1 < r.count && k < ALB_SEED_STEPS; k++)
            [steps[k] addObject:@((r[k].doubleValue - info.s) / (target - info.s))];
    }
    for (NSUInteger k = 0; k < ALB_SEED_STEPS; k++)
        info.g[k] = steps[k].count >= 3 ? ALBSeedMedian(steps[k]) : NAN;
    // Does the first reading predict where a walk settles?
    if (isfinite(info.g[0]) && info.g[0] >= 0.1) for (NSDictionary *sample in gALBSeedSamples) {
        NSArray<NSNumber *> *r = sample[@"r"];
        if (r.count < 2) continue;
        double target = r.lastObject.doubleValue;
        double from = [sample[@"e"] integerValue] == 2 ? [sample[@"st"] doubleValue] : info.s;
        if (!isfinite(from) || fabs(target - from) < 0.12) continue;
        info.informative++;
        double estimate = from + (r.firstObject.doubleValue - from) / info.g[0];
        info.tested++;
        if (fabs(estimate - target) <= 0.08) info.hits++;
    }
    info.validated = info.tested >= 6 && info.hits * 4 >= info.tested * 3;
    info.raw = (isfinite(info.g[0]) && info.g[0] >= 0.9) || (!isfinite(info.g[0]) && isfinite(g1) && g1 >= 0.9);
    gALBSeedModel = info;
    return info;
}

static void ALBSeedRecordWalk(ALBSeedReceiver *state) {
    // Only walks whose start is clear: a new or rebuilt glass, another video.
    if (state.readings.count < 2 || (state.epoch != 1 && state.epoch != 2)) return;
    if (!gALBSeedSamples) gALBSeedSamples = [NSMutableArray array];
    double start = isfinite(state.start) ? state.start : -1;
    [gALBSeedSamples addObject:@{@"e": @(state.epoch), @"st": @(start), @"r": [state.readings copy]}];
    if (gALBSeedSamples.count > 60) [gALBSeedSamples removeObjectAtIndex:0];
    gALBSeedModelDirty = YES;
}

#pragma mark - One walk

static void ALBSeedBeginEpoch(ALBSeedReceiver *state, ALBSeedFace *face, NSInteger epoch, double start, CFTimeInterval now) {
    state.epoch = epoch;
    state.epochAt = now;
    state.epochRef = now;
    state.start = start;
    state.seed = NAN;
    state.seedSource = nil;
    state.readings = [NSMutableArray array];
    state.given = [NSMutableArray array];
    state.instantDone = NO;
    state.injectedNow = NO;
    state.firstAfter = -1;
    if (epoch == 2) return;
    if (epoch == 1) { if (state.kind == 1) gALBSeedFreshRight++; else gALBSeedFreshPanel++; }
    if (!face) return;
    if (face.pendingUntil > now && isfinite(face.pending)) {
        state.seed = face.pending;
        state.seedSource = face.pendingSource;
        state.injectedNow = face.injectedAt >= face.pendingAt;
        state.epochRef = face.pendingAt;
    } else if (state.kind == 1 && face.hasStable && now - face.stableAt < 300 && face.stableVideo.length &&
               [face.stableVideo isEqualToString:ALBSeedVideo(state.owner, YES) ?: @""]) {
        state.seed = face.stable;
        state.seedSource = @"自己上次";
    }
}

static void ALBSeedEndEpoch(ALBSeedReceiver *state, BOOL settled) {
    if (settled) ALBSeedRecordWalk(state);
    // A pocket's comeback has its own trace (3.11.26).
    if (state.pocket) { state.epoch = 0; return; }
    NSString *what = state.epoch == 2 ? @"换作品" : (state.epoch == 3 ? @"给起步后" : @"新玻璃");
    if (state.injectedNow) what = [what stringByAppendingString:@"·首帧前已给"];
    NSString *start = isfinite(state.seed)
        ? [NSString stringWithFormat:@" 起步%.2f(%@)", state.seed, state.seedSource ?: @"?"]
        : (isfinite(state.start) ? [NSString stringWithFormat:@" 之前%.2f", state.start] : @" 无起步");
    NSString *first = state.firstAfter >= 0 ? [NSString stringWithFormat:@" 首读+%.2fs", state.firstAfter] : @"";
    ALBSeedNote([NSString stringWithFormat:@"%@#%lu %@%@%@ 读[%@] 给[%@] %@",
        [NSString stringWithFormat:@"%@%@", state.kind == 1 ? @"右侧" : @"面板", state.pocket ? @"口袋" : @"玻璃"],
        (unsigned long)state.serial, what, start, first,
        ALBSeedList(state.readings), ALBSeedList(state.given), settled ? @"已稳定" : @"到时放手"]);
    state.epoch = 0;
}

/* What the glass gets for this reading of a walk. */
static double ALBSeedDecide(ALBSeedReceiver *state, double luma, NSString **how) {
    ALBSeedModelInfo model = ALBSeedModelNow();
    // Readings that arrive already settled need nothing but no animation.
    if (model.raw) { *how = @"原值"; return luma; }
    NSUInteger k = state.readings.count - 1;
    double from = state.epoch == 2 ? state.start : (state.epoch == 1 ? model.s : NAN);
    if (model.validated && k < ALB_SEED_STEPS && isfinite(from) && isfinite(model.g[k])) {
        double g = model.g[k];
        if (g >= 0.97) { *how = @"原值"; return luma; }
        if (g >= 0.1) {
            double estimate = from + (luma - from) / g;
            estimate = MIN(1.0, MAX(0.0, estimate));
            estimate = MIN(luma + 0.45, MAX(luma - 0.45, estimate));
            *how = @"预测";
            return estimate;
        }
    }
    double seed = state.seed;
    if (isfinite(seed) && state.epoch != 2) {
        double rebuiltFrom = isfinite(model.s) ? model.s : 0.5;
        double low = MIN(rebuiltFrom, seed), high = MAX(rebuiltFrom, seed);
        if (high - low >= 0.04 && luma > low && luma < high) { *how = @"保持起步"; return seed; }
    }
    *how = @"原值";
    return luma;
}

static void ALBSeedDeliver(id receiver, SEL selector, double value, ALBSeedLumaIMP original, BOOL instant) {
    if (!instant) { original(receiver, selector, value); return; }
    gALBSeedApplying++;
    @try { [UIView performWithoutAnimation:^{ original(receiver, selector, value); }]; }
    @finally { gALBSeedApplying--; }
}

/* Our own hand-over: through the runtime (whatever class the view is), with
   every hook passing straight through. */
static void ALBSeedSend(UIView *view, double value, BOOL instant) {
    ALBSeedReceiver *state = objc_getAssociatedObject(view, &kALBSeedReceiverKey);
    BOOL pocket = state.pocket || (gALBSeedPocketClass && [view isKindOfClass:gALBSeedPocketClass]);
    SEL selector = NSSelectorFromString(pocket ? @"backdropLayer:didChangeLuma:" : @"_materialDidReceiveLuma:");
    CALayer *layer = view.layer;
    void (^send)(void) = ^{
        if (pocket) ((void (*)(id, SEL, id, double))objc_msgSend)(view, selector, layer, value);
        else ((void (*)(id, SEL, double))objc_msgSend)(view, selector, value);
    };
    gALBSeedDepth++;
    @try {
        if (instant) {
            gALBSeedApplying++;
            @try { [UIView performWithoutAnimation:send]; }
            @finally { gALBSeedApplying--; }
        } else {
            send();
        }
    } @catch (__unused NSException *exception) {
        gALBSeedExceptions++;
    } @finally {
        gALBSeedDepth--;
    }
}

/* A held or predicted value stands until the next reading; when the
   readings stop (UIKit only reports a change, so they stop once settled),
   the glass gets its own last reading back, as UIKit would have left it. */
static void ALBSeedScheduleRelease(UIView *view, ALBSeedReceiver *state) {
    __weak UIView *weakView = view;
    __weak ALBSeedReceiver *weakState = state;
    CFTimeInterval readingAt = state.lastAt, epochAt = state.epochAt;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.8 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        UIView *live = weakView;
        ALBSeedReceiver *liveState = weakState;
        if (!live || !liveState || !liveState.epoch || liveState.epochAt != epochAt || liveState.lastAt != readingAt) return;
        double last = liveState.last;
        ALBSeedEndEpoch(liveState, liveState.readings.count >= 2);
        ALBSeedFace *face = ALBSeedFaceFor(liveState.owner, NO);
        if (face) { face.hasStable = YES; face.stable = last; face.stableAt = CACurrentMediaTime(); face.stableVideo = liveState.lastVideo; }
        if (!live.window || gALBSeedDepth) return;
        ALBSeedSend(live, last, NO);
    });
}

#pragma mark - A pocket that comes back looks again (3.11.26)

static BOOL ALBSeedViewShown(UIView *view);
static void ALBSeedRemember(UIView *owner, NSString *video, double luma);
static double ALBSeedRemembered(UIView *owner, NSString *video);
static void ALBSeedNotePrior(UIView *owner, double luma);
static void ALBSeedNoteCoverRatio(UIView *owner, double cover, double reading);
static void ALBSeedEnsureProbes(UIWindow *window);

/* An ivar UIKit keeps as Double? (8 bytes, then a tag byte: 1 = nil): its
   offset when the class's layout leaves room for exactly that (9 to 16
   bytes up to the next ivar, not an object or pointer), else -1. */
static ptrdiff_t ALBSeedOptionalDoubleIvar(Class cls, const char *name, NSMutableArray<NSString *> *notes) {
    for (Class owner = cls; owner && owner != UIView.class && owner != NSObject.class; owner = class_getSuperclass(owner)) {
        unsigned int count = 0;
        Ivar *ivars = class_copyIvarList(owner, &count);
        ptrdiff_t offset = -1, upper = (ptrdiff_t)class_getInstanceSize(owner);
        const char *type = NULL;
        for (unsigned int i = 0; i < count; i++) {
            const char *ivarName = ivar_getName(ivars[i]);
            if (ivarName && !strcmp(ivarName, name)) { offset = ivar_getOffset(ivars[i]); type = ivar_getTypeEncoding(ivars[i]); }
        }
        if (offset >= 0)
            for (unsigned int i = 0; i < count; i++) {
                ptrdiff_t other = ivar_getOffset(ivars[i]);
                if (other > offset && other < upper) upper = other;
            }
        if (ivars) free(ivars);
        if (offset < 0) continue;
        BOOL pointer = type && type[0] && strchr("@#^*:", type[0]);
        BOOL fits = !pointer && offset >= (ptrdiff_t)sizeof(void *) && offset % 8 == 0 &&
            upper - offset >= 9 && upper - offset <= 16;
        [notes addObject:[NSString stringWithFormat:@"%s@%td+%td%@", name, offset, upper - offset, fits ? @"" : @"(不像Double?)"]];
        return fits ? offset : -1;
    }
    [notes addObject:[NSString stringWithFormat:@"%s无", name]];
    return -1;
}

static void ALBSeedLookOffsets(void) {
    if (gALBSeedOffsetsLooked || !gALBSeedPocketClass) return;
    gALBSeedOffsetsLooked = YES;
    NSMutableArray<NSString *> *notes = [NSMutableArray array];
    gALBSeedCurrentLumaOffset = gALBSeedCurrentLumaAt = ALBSeedOptionalDoubleIvar(gALBSeedPocketClass, "currentLuma", notes);
    if (gALBSeedPocketViewClass)
        gALBSeedPreviousLumaOffset = gALBSeedPreviousLumaAt =
            ALBSeedOptionalDoubleIvar(gALBSeedPocketViewClass, "previousEffectiveLuma", notes);
    else
        [notes addObject:@"口袋类无"];
    gALBSeedOffsetsText = [notes componentsJoinedByString:@" "];
}

/* The field as it is now: NO when its bytes cannot be a Double? holding nil
   or a luminance (an object pointer read as a double is far below 1e-6). */
static BOOL ALBSeedReadOptional(id object, ptrdiff_t offset, BOOL *isNil, double *value) {
    if (!object || offset < 0) return NO;
    const uint8_t *field = (const uint8_t *)(__bridge void *)object + offset;
    double payload;
    memcpy(&payload, field, sizeof payload);
    uint8_t tag = field[8];
    if (tag == 1) { *isNil = YES; *value = NAN; return payload == 0; }
    if (tag != 0 || !isfinite(payload)) return NO;
    if (payload != 0 && (fabs(payload) < 1e-6 || payload < -0.05 || payload > 1.05)) return NO;
    *isNil = NO;
    *value = payload;
    return YES;
}

/* Back to nil, as UIKit's init leaves it. 1 cleared, 0 nothing to do, -1
   refused (that field is never written again). */
static int ALBSeedForgetOptional(id object, ptrdiff_t *offset) {
    if (!object || *offset < 0) return 0;
    BOOL isNil = NO;
    double value = NAN;
    if (!ALBSeedReadOptional(object, *offset, &isNil, &value)) {
        *offset = -2;
        gALBSeedForgetRefused++;
        return -1;
    }
    if (isNil) return 0;
    uint8_t *field = (uint8_t *)(__bridge void *)object + *offset;
    memset(field, 0, 8);
    field[8] = 1;
    return 1;
}

static NSString *ALBSeedFieldText(id object, ptrdiff_t offset) {
    if (!object || offset < 0) return @"-";
    BOOL isNil = NO;
    double value = NAN;
    if (ALBSeedReadOptional(object, offset, &isNil, &value))
        return isNil ? @"空" : [NSString stringWithFormat:@"%.4f", value];
    const uint8_t *field = (const uint8_t *)(__bridge void *)object + offset;
    NSMutableString *hex = [NSMutableString stringWithString:@"0x"];
    for (int i = 0; i < 9; i++) [hex appendFormat:@"%02x", field[i]];
    return hex;
}

static BOOL ALBSeedIsBlur(UIView *view) {
    if (gALBSeedBlurClass) return [view isKindOfClass:gALBSeedBlurClass];
    return gALBSeedPocketClass && [view isKindOfClass:gALBSeedPocketClass];
}

static BOOL ALBSeedInOurEdge(UIView *view) {
    NSUInteger depth = 0;
    for (UIView *cursor = view.superview; cursor && depth < 4; cursor = cursor.superview, depth++)
        if ([cursor.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.GlassEdge"]) return YES;
    return NO;
}

static UIView *ALBSeedPocketViewOf(UIView *blur) {
    if (!gALBSeedPocketViewClass) return nil;
    NSUInteger depth = 0;
    for (UIView *cursor = blur.superview; cursor && depth < 3; cursor = cursor.superview, depth++)
        if ([cursor isKindOfClass:gALBSeedPocketViewClass]) return cursor;
    return nil;
}

static UIScrollView *ALBSeedEdgeOf(UIView *blur) {
    NSUInteger depth = 0;
    for (UIView *cursor = blur.superview; cursor && depth < 5; cursor = cursor.superview, depth++)
        if ([cursor isKindOfClass:UIScrollView.class]) return (UIScrollView *)cursor;
    return nil;
}

/* 3.11.27: the button (or slider) whose edge pocket a layer belongs to:
   the layer's view, inside a pocket view, inside one of our edges. */
static UIView *ALBGlassSeedPocketLayerOwner(CALayer *layer, NSUInteger *kind) {
    if (!gALBSeedPocketViewClass) return nil;
    UIView *view = nil;
    CALayer *cursor = layer;
    for (NSUInteger i = 0; cursor && i < 8 && !view; i++, cursor = cursor.superlayer)
        if ([cursor.delegate isKindOfClass:UIView.class]) view = (UIView *)cursor.delegate;
    if (!view) return nil;
    BOOL inPocket = NO;
    UIView *edge = nil;
    NSUInteger depth = 0;
    for (UIView *step = view; step && depth < 6 && !edge; step = step.superview, depth++) {
        if (!inPocket) inPocket = [step isKindOfClass:gALBSeedPocketViewClass];
        else if ([step.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.GlassEdge"]) edge = step;
    }
    UIView *glass = edge ? ALBSeedGlassForEdge(edge) : nil;
    if (!glass) return nil;
    UIView *owner = nil;
    NSInteger found = ALBSeedClassify(glass, &owner);
    if ((found != 1 && found != 2) || !owner) return nil;
    if (kind) *kind = (NSUInteger)found;
    return owner;
}

/* 3.11.27: right after a hand-over in the pocket's comeback, the pocket
   and the glass of its edge take it now (their pending property updates and
   layout), without animation, so the frame being built already shows it. */
static void ALBSeedApplyNow(UIView *blur) {
    UIScrollView *edge = ALBSeedEdgeOf(blur);
    UIView *glass = edge ? ALBSeedGlassForEdge(edge) : nil;
    if (!glass) return;
    UIView *pocket = ALBSeedPocketViewOf(blur);
    SEL update = NSSelectorFromString(@"updatePropertiesIfNeeded");
    gALBSeedDepth++;
    gALBSeedApplying++;
    @try {
        [UIView performWithoutAnimation:^{
            if (pocket && [pocket respondsToSelector:update]) ((void (*)(id, SEL))objc_msgSend)(pocket, update);
            if ([glass respondsToSelector:update]) ((void (*)(id, SEL))objc_msgSend)(glass, update);
            [glass layoutIfNeeded];
        }];
        gALBSeedAppliedNow++;
    } @catch (__unused NSException *exception) {
        gALBSeedExceptions++;
    } @finally {
        gALBSeedApplying--;
        gALBSeedDepth--;
    }
}

static void ALBSeedCaptureTimer(NSTimer *timer, BOOL repeats) {
    if (!NSThread.isMainThread || !gALBSeedCapturedTimers || repeats || !timer) return;
    if (gALBSeedCapturedTimers.count >= 8) return;
    [gALBSeedCapturedTimers addObject:timer];
    gALBSeedTimersCaptured++;
}

/* Its evaluated luma is set (it has decided); `known` NO when it cannot be
   read. */
static BOOL ALBSeedPocketDecided(UIView *pocket, BOOL *known) {
    BOOL isNil = NO;
    double value = NAN;
    *known = NO;
    if (!pocket || gALBSeedPreviousLumaAt < 0 || !ALBSeedReadOptional(pocket, gALBSeedPreviousLumaAt, &isNil, &value)) return NO;
    *known = YES;
    return !isNil;
}

static NSString *ALBSeedSettleName(NSInteger step) {
    switch (step) {
        case 0: return @"交时已定";
        case 1: return @"布局后定";
        case 2: return @"计时器后定";
        case 3: return @"仍未定";
        default: return @"不明";
    }
}

/* `send` hands one of our pockets a reading; then the pocket decides now:
   its layout, then the one-shot timers created while it was handed (all
   without animation). 0 decided while handed, 1 after its layout, 2 after
   the timers, 3 not yet, 4 cannot tell. */
static NSInteger ALBSeedDeliverAndSettle(UIView *blur, void (^send)(void)) {
    NSMutableArray<NSTimer *> *captured = [NSMutableArray array];
    NSMutableArray<NSTimer *> *outer = gALBSeedCapturedTimers;
    gALBSeedCapturedTimers = captured;
    @try { send(); }
    @finally { gALBSeedCapturedTimers = outer; }
    UIView *pocket = ALBSeedPocketViewOf(blur);
    BOOL known = NO;
    NSInteger step = 4;
    if (ALBSeedPocketDecided(pocket, &known)) step = 0;
    else if (pocket) {
        step = known ? 3 : 4;
        gALBSeedDepth++;
        gALBSeedApplying++;
        @try {
            [UIView performWithoutAnimation:^{ [pocket setNeedsLayout]; [pocket layoutIfNeeded]; }];
            if (ALBSeedPocketDecided(pocket, &known)) step = 1;
            else if (captured.count) {
                [UIView performWithoutAnimation:^{
                    for (NSTimer *timer in captured)
                        if (timer.valid) { [timer fire]; gALBSeedTimersFired++; }
                }];
                if (ALBSeedPocketDecided(pocket, &known)) step = 2;
            }
        } @catch (__unused NSException *exception) {
            gALBSeedExceptions++;
        } @finally {
            gALBSeedApplying--;
            gALBSeedDepth--;
        }
    }
    gALBSeedSettled[MIN(4, MAX(0, step))]++;
    return step;
}

/* -[UIScrollView _lumaStyleForEdge:] of an edge of ours (诊断). Read only
   when the method returns a flat struct of plain numbers larger than 16
   bytes and not all floats (so it comes back through memory, never
   registers), into a buffer larger than it; each field by the encoding. */
typedef struct { unsigned char bytes[128]; } ALBSeedStyleBuffer;
static NSString *gALBSeedStyleEncoding = @"未查";
static NSString *ALBSeedPocketStyleText(UIScrollView *edge) {
    static int usable = -1;
    static char fields[24];
    SEL selector = NSSelectorFromString(@"_lumaStyleForEdge:");
    if (usable < 0) {
        usable = 0;
        Method method = class_getInstanceMethod(UIScrollView.class, selector);
        char type[160] = {0};
        if (method && method_getNumberOfArguments(method) == 3) method_getReturnType(method, type, sizeof type);
        gALBSeedStyleEncoding = type[0] ? [NSString stringWithUTF8String:type] ?: @"?" : @"无此方法";
        const char *equals = strchr(type, '=');
        BOOL flat = type[0] == '{' && equals != NULL, mixed = NO;
        NSUInteger count = 0;
        for (const char *c = equals ? equals + 1 : type; flat && *c && *c != '}'; c++) {
            if (!strchr("fdqQiIlLsScCB", *c) || count + 1 >= sizeof fields) { flat = NO; break; }
            if (count && *c != fields[0]) mixed = YES;
            fields[count++] = *c;
        }
        fields[count] = 0;
        if (flat && count && (mixed || count > 4)) {
            NSUInteger size = 0, align = 0;
            @try { NSGetSizeAndAlignment(type, &size, &align); } @catch (__unused NSException *exception) { size = 0; }
            usable = size > 16 && size <= sizeof(ALBSeedStyleBuffer) ? 1 : 0;
        }
    }
    if (!usable || ![edge isKindOfClass:UIScrollView.class]) return @"-";
    ALBSeedStyleBuffer buffer;
    memset(&buffer, 0, sizeof buffer);
    buffer = ((ALBSeedStyleBuffer (*)(id, SEL, unsigned long))objc_msgSend)(edge, selector, 2);
    NSMutableArray<NSString *> *parts = [NSMutableArray array];
    size_t offset = 0;
    for (const char *c = fields; *c; c++) {
        size_t size = strchr("dqQ", *c) ? 8 : (strchr("fiIlL", *c) ? 4 : (strchr("sS", *c) ? 2 : 1));
        offset = (offset + size - 1) / size * size;
        if (offset + size > sizeof buffer.bytes) break;
        const unsigned char *bytes = buffer.bytes + offset;
        if (*c == 'f') { float v; memcpy(&v, bytes, 4); [parts addObject:[NSString stringWithFormat:@"%.3f", v]]; }
        else if (*c == 'd') { double v; memcpy(&v, bytes, 8); [parts addObject:[NSString stringWithFormat:@"%.3f", v]]; }
        else if (size == 8) { long long v; memcpy(&v, bytes, 8); [parts addObject:[NSString stringWithFormat:@"%lld", v]]; }
        else if (size == 4) { int v; memcpy(&v, bytes, 4); [parts addObject:[NSString stringWithFormat:@"%d", v]]; }
        else if (size == 2) { short v; memcpy(&v, bytes, 2); [parts addObject:[NSString stringWithFormat:@"%d", v]]; }
        else [parts addObject:[NSString stringWithFormat:@"%d", bytes[0]]];
        offset += size;
    }
    return [parts componentsJoinedByString:@","];
}

/* One pocket now: its backdrops' opacity (the third one is its level:
   0.85 start, 0.60 / 0.10 after a look), the two fields, its lumaStyle. */
static NSString *ALBSeedPocketStateText(UIView *blur) {
    UIView *pocket = ALBSeedPocketViewOf(blur);
    NSMutableArray<NSString *> *levels = [NSMutableArray array];
    for (CALayer *layer in ALBSeedBackdrops(pocket ?: blur))
        [levels addObject:layer.hidden ? @"隐" : [NSString stringWithFormat:@"%.2f", layer.opacity]];
    return [NSString stringWithFormat:@"层%@ 读%@ 评%@ 式%@",
        levels.count ? [levels componentsJoinedByString:@"/"] : @"无",
        ALBSeedFieldText(blur, gALBSeedCurrentLumaAt),
        pocket ? ALBSeedFieldText(pocket, gALBSeedPreviousLumaAt) : @"无口袋",
        ALBSeedPocketStyleText(ALBSeedEdgeOf(blur))];
}

static void ALBSeedTrace(ALBSeedReceiver *state, NSString *line) {
    if (state.trace && state.trace.count < 28) [state.trace addObject:line];
}

/* Both fields back to nil for one blur and its pocket. */
static void ALBSeedForget(UIView *blur) {
    ALBSeedLookOffsets();
    if (ALBSeedForgetOptional(blur, &gALBSeedCurrentLumaOffset) > 0) gALBSeedForgotBackdrop++;
    UIView *pocket = ALBSeedPocketViewOf(blur);
    if (pocket && ALBSeedForgetOptional(pocket, &gALBSeedPreviousLumaOffset) > 0) gALBSeedForgotPocket++;
}

/* What a pocket that came back is handed: a reading it took since, else the
   start waiting on its button, else its own last reading of the same video
   (right column; the panel's pockets never come back). */
static double ALBSeedReplayValue(ALBSeedReceiver *state, NSString **source) {
    CFTimeInterval now = CACurrentMediaTime();
    if (state.hasLast && state.lastAt >= state.entryAt) { *source = @"新读数"; return state.last; }
    ALBSeedFace *face = ALBSeedFaceFor(state.owner, NO);
    if (face && face.pendingUntil > now && isfinite(face.pending)) { *source = face.pendingSource ?: @"起步"; return face.pending; }
    if (state.kind == 1) {
        NSString *video = ALBSeedVideo(state.owner, YES);
        if (state.hasLast && now - state.lastAt < 300 && video.length && [video isEqualToString:state.lastVideo ?: @""]) {
            *source = @"自己上次";
            return state.last;
        }
        // 3.11.30: this position of this video was seen before (another cell).
        double remembered = ALBSeedRemembered(state.owner, video);
        if (isfinite(remembered)) { *source = @"看过"; return remembered; }
    }
    *source = nil;
    return NAN;
}

static BOOL ALBSeedReplay(UIView *blur, ALBSeedReceiver *state, NSString *when) {
    NSString *source = nil;
    double value = ALBSeedReplayValue(state, &source);
    if (!isfinite(value)) { ALBSeedTrace(state, [NSString stringWithFormat:@"%@无值", when]); return NO; }
    ALBSeedForget(blur);
    NSInteger step = ALBSeedDeliverAndSettle(blur, ^{ ALBSeedSend(blur, value, YES); });
    gALBSeedReplays++;
    ALBSeedTrace(state, [NSString stringWithFormat:@"%@补%.4f(%@)%@", when, value, source, ALBSeedSettleName(step)]);
    return YES;
}

/* The comeback, traced (one at a time, at most every 2 s): before anything
   is handed, then at 0.1 / 0.3 / 0.6 / 1.2 s, with the hand-overs and the
   readings in between, as one line. */
static void ALBSeedStartTrace(UIView *blur, ALBSeedReceiver *state) {
    CFTimeInterval now = CACurrentMediaTime();
    if (now - gALBSeedLastTraceAt < 2.0) return;
    gALBSeedLastTraceAt = now;
    ALBSeedLookOffsets();
    NSUInteger traceID = ++gALBSeedTraceSerial;
    state.traceID = traceID;
    UIWindow *window = blur.window;
    CGFloat y = window && state.owner ? CGRectGetMidY([state.owner convertRect:state.owner.bounds toView:window]) : -1;
    state.trace = [NSMutableArray arrayWithObject:[NSString stringWithFormat:@"y%.0f 回来时 %@", y, ALBSeedPocketStateText(blur)]];
    __weak UIView *weakBlur = blur;
    __weak ALBSeedReceiver *weakState = state;
    NSArray<NSNumber *> *times = @[@0.1, @0.3, @0.6, @1.2];
    for (NSNumber *time in times) {
        BOOL last = time == times.lastObject;
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(time.doubleValue * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            UIView *live = weakBlur;
            ALBSeedReceiver *liveState = weakState;
            if (!live || !liveState || liveState.traceID != traceID || !liveState.trace) return;
            [liveState.trace addObject:[NSString stringWithFormat:@"%.1fs %@", time.doubleValue, ALBSeedPocketStateText(live)]];
            if (!last) return;
            ALBSeedNote([NSString stringWithFormat:@"%@口袋#%lu %@", liveState.kind == 1 ? @"右侧" : @"面板",
                (unsigned long)liveState.serial, [liveState.trace componentsJoinedByString:@" | "]]);
            liveState.trace = nil;
        });
    }
}

/* Shortly after it came back, again (the first one only when no reading
   came since): whatever its own pocket did to itself after the first
   hand-over, the last one is looked at as new. */
static void ALBSeedScheduleReplays(UIView *blur, ALBSeedReceiver *state) {
    __weak UIView *weakBlur = blur;
    __weak ALBSeedReceiver *weakState = state;
    CFTimeInterval entryAt = state.entryAt;
    for (NSNumber *delay in @[@0, @0.2, @0.5]) {
        void (^replay)(void) = ^{
            UIView *live = weakBlur;
            ALBSeedReceiver *liveState = weakState;
            if (!live || !liveState || liveState.entryAt != entryAt || !live.window || gALBSeedDepth || !ALBSeedEnabled()) return;
            if (!ALBSeedViewShown(liveState.owner)) return;
            if (delay.doubleValue == 0 && liveState.hasLast && liveState.lastAt >= entryAt) return;
            @try { ALBSeedReplay(live, liveState, [NSString stringWithFormat:@"%.1fs", delay.doubleValue]); }
            @catch (__unused NSException *exception) { gALBSeedExceptions++; }
        };
        if (delay.doubleValue == 0) dispatch_async(dispatch_get_main_queue(), replay);
        else dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay.doubleValue * NSEC_PER_SEC)), dispatch_get_main_queue(), replay);
    }
}

/* The rate UIKit gave the backdrop is about one reading per 0.25 s (the
   readings' spacing in every diagnostic): 4 when it is per second, 0.25
   when it is seconds apart. Only those two readings of it are changed: to
   60 per second / 1/60 s. */
static void ALBSeedBoost(UIView *blur, ALBSeedReceiver *state) {
    if (state.boosted) return;
    CALayer *layer = blur.layer;
    SEL getter = NSSelectorFromString(@"lumaUpdateRate"), setter = NSSelectorFromString(@"setLumaUpdateRate:");
    if (![layer respondsToSelector:getter] || ![layer respondsToSelector:setter]) { gALBSeedBoostSkipped++; return; }
    double rate = ((double (*)(id, SEL))objc_msgSend)(layer, getter);
    if (!isfinite(gALBSeedRateSeen)) gALBSeedRateSeen = rate;
    double fast = NAN;
    if (rate >= 1.0 && rate <= 30.0) fast = 60.0;
    else if (rate > 0.0 && rate < 1.0) fast = 1.0 / 60.0;
    if (!isfinite(fast)) { gALBSeedBoostSkipped++; return; }
    ((void (*)(id, SEL, double))objc_msgSend)(layer, setter, fast);
    gALBSeedRateSet = fast;
    state.rateBefore = rate;
    state.boosted = YES;
    state.boostReads = 0;
    gALBSeedBoosts++;
}

static void ALBSeedUnboost(UIView *blur, ALBSeedReceiver *state) {
    if (!state.boosted) return;
    state.boosted = NO;
    CALayer *layer = blur.layer;
    SEL setter = NSSelectorFromString(@"setLumaUpdateRate:");
    if ([layer respondsToSelector:setter]) ((void (*)(id, SEL, double))objc_msgSend)(layer, setter, state.rateBefore);
    gALBSeedBoostEnds++;
}

/* A blur of one of our pockets came (back) into a window, or its pocket
   did (then after every subview of the pocket did its own; `replayNow`
   hands the value over right then, before the frame). */
static void ALBSeedBlurEntered(UIView *blur, BOOL replayNow) {
    if (gALBSeedDepth || !blur.window || !ALBSeedIsBlur(blur) || !ALBSeedInOurEdge(blur) || !ALBSeedEnabled()) return;
    ALBSeedReceiver *state = ALBSeedReceiverFor(blur);
    // A glass that was not worn yet when it was last looked at.
    if (state && state.kind == 0 && !state.owner) { state.classifiedAt = 0; state = ALBSeedReceiverFor(blur); }
    if (!state || (state.kind != 1 && state.kind != 2) || !state.owner) return;
    state.pocket = YES;
    CFTimeInterval now = CACurrentMediaTime();
    // (The blur and then its pocket report the same comeback.)
    if (now - state.entryAt >= 0.08) {
        state.entryAt = now;
        gALBSeedEntries++;
        ALBSeedStartSnap(state.owner);
        ALBSeedStartTrace(blur, state);
        if (state.kind == 1 && gALBSeedRightRestoredAt > 0 && now - gALBSeedRightRestoredAt < 2.0)
            ALBSeedTrace(state, [NSString stringWithFormat:@"距面板关后右侧恢复%.3fs", now - gALBSeedRightRestoredAt]);
        ALBSeedScheduleReplays(blur, state);
    }
    if (replayNow && ALBSeedReplay(blur, state, @"当场")) {
        ALBSeedApplyNow(blur);
        if (state.trace) ALBSeedTrace(state, [NSString stringWithFormat:@"当场后 %@", ALBSeedPocketStateText(blur)]);
    }
}

/* didMoveToWindow of a pocket view: every blur in it, if the pocket is one
   of ours. */
static void ALBSeedPocketMoved(UIView *pocket) {
    if (!pocket.window || gALBSeedDepth || !gALBSeedPocketClass || !ALBSeedInOurEdge(pocket) || !ALBSeedEnabled()) return;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:pocket.subviews];
    for (NSUInteger i = 0; i < queue.count && i < 24; i++) {
        UIView *view = queue[i];
        if (ALBSeedIsBlur(view)) { ALBSeedBlurEntered(view, YES); continue; }
        [queue addObjectsFromArray:view.subviews];
    }
}

/* The blurs of one button (or slider): inside it and inside its edge. */
static NSArray<UIView *> *ALBSeedBlursOf(UIView *owner) {
    NSMutableArray<UIView *> *found = [NSMutableArray array];
    if (!owner || !gALBSeedPocketClass) return found;
    NSMutableArray<UIView *> *roots = [NSMutableArray arrayWithObject:owner];
    UIView *glass = [owner isKindOfClass:UIButton.class] ? ((UIView *)ALBFaceFor(owner) ?: owner) : owner;
    UIView *edge = objc_getAssociatedObject(glass, &kALBFollowProxyKey);
    if (edge && ![edge isDescendantOfView:owner]) [roots addObject:edge];
    for (UIView *root in roots) {
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:root];
        for (NSUInteger i = 0; i < queue.count && i < 240; i++) {
            UIView *view = queue[i];
            if (view != root && ALBSeedIsBlur(view)) { [found addObject:view]; continue; }
            [queue addObjectsFromArray:view.subviews];
        }
    }
    return found;
}

static double ALBSeedHandleOurs(UIView *view, ALBSeedReceiver *state, double luma, BOOL *instant, BOOL *release) {
    CFTimeInterval now = CACurrentMediaTime();
    BOOL right = state.kind == 1;
    if (state.pocket) { if (right) gALBSeedPocketReadsRight++; else gALBSeedPocketReadsPanel++; }
    else if (right) gALBSeedReadsRight++; else gALBSeedReadsPanel++;
    if (!gALBSeedOurClasses) gALBSeedOurClasses = [NSMutableSet set];
    if (gALBSeedOurClasses.count < 8) [gALBSeedOurClasses addObject:NSStringFromClass(view.class)];
    UIView *owner = state.owner;
    ALBSeedFace *face = ALBSeedFaceFor(owner, YES);

    // A walk whose readings had stopped settled on its last one; one that is
    // too old ends.
    if (state.epoch && (now - state.lastAt >= 0.6 || now - state.epochAt > 3.0))
        ALBSeedEndEpoch(state, state.readings.count >= 2 && now - state.lastAt >= 0.6);

    // Has this glass started over? (New, its backdrops built again, or the
    // first reading since its button was given a start.)
    BOOL rebuilt = ALBSeedBackdropsRebuilt(state, ALBSeedBackdrops(view), YES);
    BOOL started = face.pendingUntil > now && isfinite(face.pending) && state.lastAt < face.pendingAt;
    if (!state.hasLast || rebuilt || (started && !state.epoch)) {
        if (state.epoch) ALBSeedEndEpoch(state, NO);
        BOOL restarted = !state.hasLast || rebuilt;
        ALBSeedBeginEpoch(state, face, restarted ? 1 : 3, restarted ? NAN : state.last, now);
    } else if (right && !state.epoch && now - state.lastAt >= 0.6 && state.lastVideo.length) {
        NSString *video = ALBSeedVideo(owner, YES);
        if (video.length && ![video isEqualToString:state.lastVideo]) {
            gALBSeedVideoRight++;
            ALBSeedBeginEpoch(state, face, 2, state.last, now);
        } else if (video.length && state.pocket) {
            // Shown again (Douyin's covers, the panel): an edge pocket starts
            // its readings over without being rebuilt; its own last settled
            // reading of this video is the start.
            ALBSeedBeginEpoch(state, face, 3, state.last, now);
        }
    }

    double value = luma;
    if (state.epoch) {
        if (state.readings.count == 0) state.firstAfter = now - state.epochRef;
        NSUInteger previous = state.readings.count;
        [state.readings addObject:@(luma)];
        NSString *how = @"原值";
        value = ALBSeedDecide(state, luma, &how);
        if ([how isEqualToString:@"预测"]) gALBSeedPredicted++;
        else if ([how isEqualToString:@"保持起步"]) gALBSeedHeld++;
        if (!state.instantDone) {
            *instant = YES;
            state.instantDone = YES;
            gALBSeedFirstInstant++;
        }
        BOOL settled = previous >= 2 && fabs(luma - state.readings[previous - 1].doubleValue) < 0.012;
        BOOL ending = settled || state.readings.count >= 10 || now - state.epochAt > 3.0;
        // A walk that ends hands the glass its own reading.
        if (ending) value = luma;
        [state.given addObject:@(value)];
        if (ending) ALBSeedEndEpoch(state, settled);
        else if (fabs(value - luma) > 0.02) *release = YES;
    }

    state.hasLast = YES;
    state.last = luma;
    state.lastAt = now;
    if (right) state.lastVideo = ALBSeedVideo(owner, NO);
    if (face && !state.epoch) {
        face.hasStable = YES;
        face.stable = luma;
        face.stableAt = now;
        face.stableVideo = right ? state.lastVideo : nil;
    }
    return value;
}

static void ALBSeedReceive(id receiver, SEL selector, double luma, ALBSeedLumaIMP original) {
    if (!original) return;
    if (gALBSeedDepth || !NSThread.isMainThread || !isfinite(luma) || ![receiver isKindOfClass:UIView.class] ||
        !ALBSeedEnabled()) {
        original(receiver, selector, luma);
        return;
    }
    gALBSeedDepth++;
    double value = luma;
    BOOL instant = NO, release = NO;
    ALBSeedReceiver *ours = nil;
    @try {
        ALBSeedReceiver *state = ALBSeedReceiverFor((UIView *)receiver);
        if (state.kind == 1 || state.kind == 2) {
            ours = state;
            value = ALBSeedHandleOurs((UIView *)receiver, state, luma, &instant, &release);
        } else if (state.kind == 3) {
            gALBSeedReadsTop++;
        } else {
            gALBSeedReadsOther++;
            if (!gALBSeedOtherClasses) gALBSeedOtherClasses = [NSMutableSet set];
            if (gALBSeedOtherClasses.count < 8) [gALBSeedOtherClasses addObject:NSStringFromClass([receiver class])];
            ALBSeedNoteOtherChain((UIView *)receiver);
        }
    } @catch (__unused NSException *exception) {
        gALBSeedExceptions++;
        value = luma;
        instant = NO;
    }
    if (!isfinite(value)) value = luma;
    @try {
        ALBSeedDeliver(receiver, selector, value, original, instant);
        if (release && ours) ALBSeedScheduleRelease((UIView *)receiver, ours);
    } @finally {
        gALBSeedDepth--;
    }
}

/* Rough level band of a luma, from the diagnostics (0.85 / 0.60 / 0.10). */
static NSInteger ALBSeedBand(double luma) {
    return luma < 0.3 ? 0 : (luma < 0.88 ? 1 : 2);
}

/* The edge pocket's entry (3.11.25). 3.11.26: the reading itself always
   goes through as it came (they arrive at their true value); our own
   pockets' readings are counted and remembered, and in a pocket's first
   second after it came back (or its very first reading) each one is looked
   at as new, without animation. */
static void ALBSeedPocketReceive(id receiver, SEL selector, id layer, double luma, ALBSeedPocketIMP original) {
    if (!original) return;
    if (gALBSeedDepth || !NSThread.isMainThread || !isfinite(luma) || ![receiver isKindOfClass:UIView.class] ||
        !ALBSeedEnabled()) {
        original(receiver, selector, layer, luma);
        return;
    }
    gALBSeedDepth++;
    BOOL fresh = NO, drop = NO;
    ALBSeedReceiver *ours = nil;
    CFTimeInterval now = CACurrentMediaTime();
    @try {
        ALBSeedReceiver *state = ALBSeedReceiverFor((UIView *)receiver);
        if (state.kind == 1 || state.kind == 2) {
            state.pocket = YES;
            ours = state;
            if (state.kind == 1 && gALB8PaletteRightHidden) {
                gALBSeedHiddenReads++;
                gALBSeedHiddenLow = isfinite(gALBSeedHiddenLow) ? MIN(gALBSeedHiddenLow, luma) : luma;
                gALBSeedHiddenHigh = isfinite(gALBSeedHiddenHigh) ? MAX(gALBSeedHiddenHigh, luma) : luma;
            }
            // 3.11.29: its glass is not drawn (hidden, alpha 0 up the chain):
            // this reading is not the background.
            drop = state.owner && !ALBSeedViewShown(state.owner);
            if (drop) {
                gALBSeedDroppedUnseen++;
                gALBSeedUnseenLow = isfinite(gALBSeedUnseenLow) ? MIN(gALBSeedUnseenLow, luma) : luma;
                gALBSeedUnseenHigh = isfinite(gALBSeedUnseenHigh) ? MAX(gALBSeedUnseenHigh, luma) : luma;
            }
            // 3.11.30: a pocket handed a value before it was ever seen: its
            // first reading of 0 is the capture not being ready (3.11.28: 0,
            // then the real one a period later), not the background.
            if (!drop && isfinite(state.estimate) && state.realReads < 2 && luma < 0.005) {
                drop = YES;
                state.realReads++;
                gALBSeedZeroDropped++;
            }
            if (state.kind == 1 && !drop) ALBSeedEnsureProbes(state.owner.window);
        } else {
            gALBSeedPocketReadsOther++;
        }
        if (ours && !drop) {
            BOOL first = !ours.hasLast;
            CFTimeInterval before = ours.lastAt, epochBefore = ours.epochAt;
            BOOL ignoredInstant = NO, ignoredRelease = NO;
            (void)ALBSeedHandleOurs((UIView *)receiver, ours, luma, &ignoredInstant, &ignoredRelease);
            // A pocket's very first reading (also one that came into the
            // window long before, off screen: a cell further down the feed),
            // another video in the same pocket (3.11.27); without the comeback
            // hooks, the first reading after a long pause (hidden: no readings).
            BOOL anotherVideo = ours.epoch == 2 && ours.epochAt != epochBefore;
            BOOL inWindow = ours.entryAt > 0 && now - ours.entryAt < ALB_SEED_ENTRY_WINDOW;
            if ((first && !inWindow) || (anotherVideo && !inWindow) ||
                (!gALBSeedEntryHooks && !first && now - before >= 1.5)) {
                ours.entryAt = now;
                gALBSeedEntries++;
                ALBSeedStartSnap(ours.owner);
            }
            if (ours.entryAt > 0 && now - ours.entryAt < ALB_SEED_ENTRY_WINDOW) {
                fresh = YES;
                gALBSeedFreshReads++;
                ALBSeedTrace(ours, [NSString stringWithFormat:@"读%.4f@%.2fs", luma, now - ours.entryAt]);
            }
        }
    } @catch (__unused NSException *exception) {
        gALBSeedExceptions++;
        fresh = NO;
        drop = NO;
    }
    @try {
        if (drop) {
            // Not handed on: the pocket keeps what it had while it was seen.
        } else if (fresh) {
            ALBSeedForget((UIView *)receiver);
            gALBSeedApplying++;
            @try {
                // 3.11.28: decided now too.
                (void)ALBSeedDeliverAndSettle((UIView *)receiver, ^{
                    [UIView performWithoutAnimation:^{ original(receiver, selector, layer, luma); }];
                });
            }
            @finally { gALBSeedApplying--; }
        } else {
            original(receiver, selector, layer, luma);
        }
        if (ours && !drop) {
            // 3.11.30: how the value handed before it was seen compares.
            if (isfinite(ours.estimate)) {
                if (!gALBSeedEstimatePairs) gALBSeedEstimatePairs = [NSMutableArray array];
                if (gALBSeedEstimatePairs.count >= 12) [gALBSeedEstimatePairs removeObjectAtIndex:0];
                [gALBSeedEstimatePairs addObject:[NSString stringWithFormat:@"%@%@估%.2f/实%.2f%@", ours.estimateSource ?: @"?",
                    isfinite(ours.estimateRaw) ? [NSString stringWithFormat:@"(图%.2f)", ours.estimateRaw] : @"",
                    ours.estimate, luma, ALBSeedBand(ours.estimate) == ALBSeedBand(luma) ? @"同档" : @"异档"]];
                // 3.11.32: how much darker the pocket read than the cover here.
                if (isfinite(ours.estimateRaw)) ALBSeedNoteCoverRatio(ours.owner, ours.estimateRaw, luma);
                ours.estimate = NAN;
                ours.estimateRaw = NAN;
            }
            ours.realReads++;
            // This position of this video, for when it is seen again.
            UIView *owner = ours.owner;
            UIWindow *window = owner.window;
            if (ours.kind == 1 && window && ours.lastVideo.length &&
                CGRectIntersectsRect([owner convertRect:owner.bounds toView:window], window.bounds)) {
                ALBSeedRemember(owner, ours.lastVideo, luma);
                // 3.11.31: this video's first reading here, into the position's history.
                if (![ours.priorVideo isEqualToString:ours.lastVideo]) {
                    ours.priorVideo = ours.lastVideo;
                    ALBSeedNotePrior(owner, luma);
                }
            }
        }
    } @finally {
        gALBSeedDepth--;
    }
}

#pragma mark - Handing a start before the first frame

static BOOL ALBSeedViewShown(UIView *view) {
    if (!view.window) return NO;
    NSUInteger depth = 0;
    for (UIView *cursor = view; cursor && depth < 64; cursor = cursor.superview, depth++)
        if (cursor.hidden || cursor.layer.opacity < 0.01f) return NO;
    return YES;
}

/* `value` to one glass group now, without animation. The walk itself is
   kept by the view its readings start at (its button's start was set just
   before, ALBSeedSetPending): this only makes the start the glass's first
   frame. Only a glass UIKit has built (its backdrops exist); one that is not
   built yet gets it from its next layout or its first reading. */
static BOOL ALBSeedInject(UIView *view, double value, NSUInteger token) {
    if (!isfinite(value) || gALBSeedDepth) return NO;
    ALBSeedReceiver *state = ALBSeedReceiverFor(view);
    if (!state || (state.kind != 1 && state.kind != 2) || !state.owner) return NO;
    // Already given this start and still on the same backdrops (cheap).
    if (token && state.injectedToken == token)
        for (CALayer *layer in state.backdrops.allObjects) if (layer.superlayer) return NO;
    if (!ALBSeedViewShown(state.pocket ? state.owner : view)) return NO;
    NSArray<CALayer *> *backdrops = ALBSeedBackdrops(view);
    if (!backdrops.count) return NO;
    // Once per start, unless UIKit built this glass again after it got it.
    BOOL rebuilt = ALBSeedBackdropsRebuilt(state, backdrops, YES);
    if (token && state.injectedToken == token && !rebuilt) return NO;
    state.injectedToken = token;
    ALBSeedSend(view, value, YES);
    ALBSeedFaceFor(state.owner, YES).injectedAt = CACurrentMediaTime();
    if (state.kind == 1) gALBSeedInjectRight++; else gALBSeedInjectPanel++;
    return YES;
}

/* `owner` (a right-column or panel button, or a slider): the next new glass
   in it starts from `value` (kept 1.2 s). */
static void ALBSeedSetPending(UIView *owner, double value, NSString *source, NSUInteger token) {
    ALBSeedFace *face = ALBSeedFaceFor(owner, YES);
    CFTimeInterval now = CACurrentMediaTime();
    face.pending = value;
    face.pendingAt = now;
    face.pendingUntil = now + 1.2;
    face.pendingToken = token;
    face.pendingSource = source;
}

/* After UIKit lays out a glass group view: a start waiting on its button
   (panel just opened, right column just shown again), or, for the right
   column, a glass UIKit has just built again (it was hidden: panel,
   Douyin's covers) of the same video it showed: its own last settled
   reading. */
static void ALBSeedAfterLayout(UIView *view) {
    if (gALBSeedDepth || !gALBSeedClassCount) return;
    ALBSeedReceiver *state = ALBSeedReceiverFor(view);
    if (!state || (state.kind != 1 && state.kind != 2) || !state.owner || !ALBSeedEnabled()) return;
    // Only a glass group that is handed readings itself (3.11.24: ours never was).
    if (!state.hasLast) return;
    ALBSeedFace *face = ALBSeedFaceFor(state.owner, NO);
    if (!face) return;
    CFTimeInterval now = CACurrentMediaTime();
    if (face.pendingUntil > now && isfinite(face.pending)) {
        if (ALBSeedInject(view, face.pending, face.pendingToken)) gALBSeedInjectLayout++;
        return;
    }
    if (state.kind != 1) return;
    // Cheap first: one of the backdrops it had is still attached → not rebuilt.
    for (CALayer *layer in state.backdrops.allObjects) if (layer.superlayer) return;
    NSArray<CALayer *> *backdrops = ALBSeedBackdrops(view);
    if (!backdrops.count) return;   // hidden: nothing built now
    // Remember them (the first time only that); all new → built again.
    if (!ALBSeedBackdropsRebuilt(state, backdrops, YES)) return;
    if (!face.hasStable || now - face.stableAt > 300) return;
    NSString *video = ALBSeedVideo(state.owner, YES);
    if (!video.length || ![video isEqualToString:face.stableVideo ?: @""]) return;
    NSUInteger token = ++gALBSeedToken;
    ALBSeedSetPending(state.owner, face.stable, @"自己上次", token);
    if (ALBSeedInject(view, face.stable, token)) gALBSeedInjectLayout++;
}

#pragma mark - Events

static NSArray<UIButton *> *ALBSeedRightButtons(void) {
    NSMutableArray<UIButton *> *buttons = [NSMutableArray arrayWithArray:ALBRGButtons().allObjects];
    for (UIButton *button in ALBArtworkButtons().allObjects)
        if (![buttons containsObject:button]) [buttons addObject:button];
    return buttons;
}

#pragma mark - Brightness before the first frame (3.11.30)

static UIView *ALBSeedCellOf(UIView *view) {
    for (UIView *cursor = view.superview; cursor; cursor = cursor.superview)
        if ([cursor isKindOfClass:UITableViewCell.class] || [cursor isKindOfClass:UICollectionViewCell.class]) return cursor;
    return nil;
}

/* One right-column position of one video: the video and the button's
   height inside its cell (8 pt steps). */
static NSString *ALBSeedPositionKey(UIView *owner, NSString *video) {
    UIView *cell = owner && video.length ? ALBSeedCellOf(owner) : nil;
    if (!cell) return nil;
    CGRect frame = [owner convertRect:owner.bounds toView:cell];
    return [NSString stringWithFormat:@"%@|%.0f", video, round(CGRectGetMidY(frame) / 8.0)];
}

static void ALBSeedRemember(UIView *owner, NSString *video, double luma) {
    NSString *key = ALBSeedPositionKey(owner, video);
    if (!key || !isfinite(luma)) return;
    if (!gALBSeedVideoLuma) gALBSeedVideoLuma = [NSMutableDictionary dictionary];
    if (gALBSeedVideoLuma.count >= 600 && !gALBSeedVideoLuma[key]) [gALBSeedVideoLuma removeAllObjects];
    gALBSeedVideoLuma[key] = @(luma);
}

static double ALBSeedRemembered(UIView *owner, NSString *video) {
    NSString *key = ALBSeedPositionKey(owner, video);
    NSNumber *value = key ? gALBSeedVideoLuma[key] : nil;
    return value ? value.doubleValue : NAN;
}

/* 3.11.31: one right-column position, whatever the video. */
static NSString *ALBSeedSlotKey(UIView *owner) {
    UIView *cell = owner ? ALBSeedCellOf(owner) : nil;
    if (!cell) return nil;
    CGRect frame = [owner convertRect:owner.bounds toView:cell];
    return [NSString stringWithFormat:@"%.0f", round(CGRectGetMidY(frame) / 8.0)];
}

static void ALBSeedNotePrior(UIView *owner, double luma) {
    NSString *key = ALBSeedSlotKey(owner);
    if (!key || !isfinite(luma)) return;
    if (!gALBSeedPositionPrior) gALBSeedPositionPrior = [NSMutableDictionary dictionary];
    NSMutableArray<NSNumber *> *values = gALBSeedPositionPrior[key];
    if (!values) {
        if (gALBSeedPositionPrior.count >= 40) [gALBSeedPositionPrior removeAllObjects];
        values = [NSMutableArray array];
        gALBSeedPositionPrior[key] = values;
    }
    [values addObject:@(luma)];
    if (values.count > 15) [values removeObjectAtIndex:0];
}

/* The median of this position's recent first readings (at least 3). */
static double ALBSeedPrior(UIView *owner) {
    NSString *key = ALBSeedSlotKey(owner);
    NSArray<NSNumber *> *values = key ? gALBSeedPositionPrior[key] : nil;
    if (values.count < 3) return NAN;
    return ALBSeedMedian([values mutableCopy]);
}

static void ALBSeedNoteCoverRatio(UIView *owner, double cover, double reading) {
    if (!isfinite(cover) || !isfinite(reading) || cover < 0.03) return;
    double ratio = MIN(2.0, MAX(0.05, reading / cover));
    NSString *key = ALBSeedSlotKey(owner);
    if (key) {
        if (!gALBSeedCoverRatios) gALBSeedCoverRatios = [NSMutableDictionary dictionary];
        NSMutableArray<NSNumber *> *values = gALBSeedCoverRatios[key];
        if (!values) {
            if (gALBSeedCoverRatios.count >= 40) [gALBSeedCoverRatios removeAllObjects];
            values = [NSMutableArray array];
            gALBSeedCoverRatios[key] = values;
        }
        [values addObject:@(ratio)];
        if (values.count > 12) [values removeObjectAtIndex:0];
    }
    if (!gALBSeedCoverRatiosAll) gALBSeedCoverRatiosAll = [NSMutableArray array];
    [gALBSeedCoverRatiosAll addObject:@(ratio)];
    if (gALBSeedCoverRatiosAll.count > 40) [gALBSeedCoverRatiosAll removeObjectAtIndex:0];
}

/* How much darker the pocket reads than the cover at this position: its own
   pairs (2 or more), else every position's (3 or more), else 0.5 (the
   3.11.31 pairs ran about a third to a half). */
static double ALBSeedCoverRatio(UIView *owner, NSString **basis) {
    NSString *key = ALBSeedSlotKey(owner);
    NSArray<NSNumber *> *own = key ? gALBSeedCoverRatios[key] : nil;
    if (own.count >= 2) { *basis = @"本位"; return ALBSeedMedian([own mutableCopy]); }
    if (gALBSeedCoverRatiosAll.count >= 3) { *basis = @"全体"; return ALBSeedMedian([gALBSeedCoverRatiosAll mutableCopy]); }
    *basis = @"默认";
    return 0.5;
}

static void ALBSeedCoverNote(NSString *line) {
    if (!gALBSeedCoverNotes) gALBSeedCoverNotes = [NSMutableArray array];
    if (gALBSeedCoverNotes.count >= 4) return;
    for (NSString *seen in gALBSeedCoverNotes) if ([seen isEqualToString:line]) return;
    [gALBSeedCoverNotes addObject:line];
}

/* The cell's own picture covering (most of) it: a layer whose contents is
   an image (an image view showing the cover). */
/* 3.11.31: the 3.11.30 diagnostic found AWEAwemePlayVideoPauseIcon (a
   cell-sized view showing a 158×186 icon in its centre): only a picture
   that fills its view (stretched or aspect fit / fill) and is large enough
   to be a frame counts. */
static BOOL ALBSeedLayerIsPicture(CALayer *layer, CGFloat screenScale) {
    id contents = layer.contents;
    if (!contents || CFGetTypeID((__bridge CFTypeRef)contents) != CGImageGetTypeID()) return NO;
    NSString *gravity = layer.contentsGravity;
    if (![gravity isEqualToString:kCAGravityResizeAspectFill] && ![gravity isEqualToString:kCAGravityResizeAspect] &&
        ![gravity isEqualToString:kCAGravityResize]) return NO;
    CGImageRef image = (__bridge CGImageRef)contents;
    CGFloat width = CGImageGetWidth(image), height = CGImageGetHeight(image);
    CGFloat viewWidth = MAX(1, CGRectGetWidth(layer.bounds) * screenScale), viewHeight = MAX(1, CGRectGetHeight(layer.bounds) * screenScale);
    // 3.11.32: a small blurred preview (32×60) is as good for a mean as a
    // full frame; 3×3 and the like are placeholders or overlay masks.
    (void)viewWidth; (void)viewHeight;
    return width >= 16 && height >= 16;
}

static CALayer *ALBSeedPictureLayer(UIView *cell) {
    CGFloat cellArea = CGRectGetWidth(cell.bounds) * CGRectGetHeight(cell.bounds);
    if (cellArea < 1) return nil;
    CGFloat screenScale = cell.window.screen.scale ?: UIScreen.mainScreen.scale;
    CALayer *best = nil;
    CGFloat bestArea = 0;
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:cell];
    for (NSUInteger i = 0; i < queue.count && i < 800; i++) {
        UIView *view = queue[i];
        if (ALBSeedLayerIsPicture(view.layer, screenScale)) {
            CGRect shown = CGRectIntersection([view convertRect:view.bounds toView:cell], cell.bounds);
            CGFloat area = CGRectIsNull(shown) ? 0 : CGRectGetWidth(shown) * CGRectGetHeight(shown);
            // 3.11.32: half the cell will do (a photo post: 440×587 of 440×956);
            // among those, the one with the most pixels.
            CGImageRef image = (__bridge CGImageRef)view.layer.contents;
            CGFloat pixels = (CGFloat)CGImageGetWidth(image) * (CGFloat)CGImageGetHeight(image);
            if (area >= cellArea * 0.5 && pixels > bestArea) { best = view.layer; bestArea = pixels; }
        }
        [queue addObjectsFromArray:view.subviews];
    }
    return best;
}

/* Diagnostic: what the cell has, when no picture was found. */
static void ALBSeedNoteCellPictures(UIView *cell) {
    if (gALBSeedCoverNotes.count >= 4) return;
    NSMutableArray<NSString *> *parts = [NSMutableArray array];
    CGFloat cellArea = MAX(1, CGRectGetWidth(cell.bounds) * CGRectGetHeight(cell.bounds));
    NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:cell];
    for (NSUInteger i = 0; i < queue.count && i < 800 && parts.count < 6; i++) {
        UIView *view = queue[i];
        CGRect frame = [view convertRect:view.bounds toView:cell];
        if (view != cell && CGRectGetWidth(frame) * CGRectGetHeight(frame) >= cellArea * 0.3) {
            id contents = view.layer.contents;
            BOOL picture = contents && CFGetTypeID((__bridge CFTypeRef)contents) == CGImageGetTypeID();
            NSString *kind = !contents ? @"无内容" : (picture
                ? [NSString stringWithFormat:@"图%zu×%zu/%@", CGImageGetWidth((__bridge CGImageRef)contents),
                    CGImageGetHeight((__bridge CGImageRef)contents), view.layer.contentsGravity]
                : @"其他");
            NSString *name = NSStringFromClass(view.class);
            if (name.length > 28) name = [name substringFromIndex:name.length - 28];
            if (![kind isEqualToString:@"无内容"] || [view isKindOfClass:UIImageView.class])
                [parts addObject:[NSString stringWithFormat:@"%@ %.0f×%.0f %@%@", name, CGRectGetWidth(frame), CGRectGetHeight(frame),
                    kind, view.hidden ? @"(隐)" : @""]];
        }
        [queue addObjectsFromArray:view.subviews];
    }
    ALBSeedCoverNote([NSString stringWithFormat:@"没找到封面图：%@ %@", NSStringFromClass(cell.class),
        parts.count ? [parts componentsJoinedByString:@"；"] : @"大块视图里没有图片内容"]);
}

/* One small copy of a cover (decoded once per picture), so every button of
   the cell is read from it without touching the full image again. */
@interface ALBSeedThumbnail : NSObject
@property(nonatomic) uintptr_t source;
@property(nonatomic, strong) NSData *pixels;     // RGBA, width × height
@property(nonatomic) NSUInteger width, height;
@end
@implementation ALBSeedThumbnail
@end
static char kALBSeedThumbnailKey;

static ALBSeedThumbnail *ALBSeedThumbnailFor(CALayer *picture) {
    CGImageRef image = (__bridge CGImageRef)picture.contents;
    ALBSeedThumbnail *thumbnail = objc_getAssociatedObject(picture, &kALBSeedThumbnailKey);
    if (thumbnail && thumbnail.source == (uintptr_t)image) return thumbnail;
    size_t width = CGImageGetWidth(image), height = CGImageGetHeight(image);
    if (!width || !height) return nil;
    NSUInteger tw = 36, th = (NSUInteger)MIN(96.0, MAX(8.0, round(36.0 * height / width)));
    NSMutableData *pixels = [NSMutableData dataWithLength:tw * th * 4];
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(pixels.mutableBytes, tw, th, 8, tw * 4, space,
        kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
    CGColorSpaceRelease(space);
    if (!context) return nil;
    CGContextSetInterpolationQuality(context, kCGInterpolationLow);
    // (A bitmap context's first row is the top of the image drawn upright.)
    CGContextDrawImage(context, CGRectMake(0, 0, tw, th), image);
    CGContextRelease(context);
    thumbnail = [ALBSeedThumbnail new];
    thumbnail.source = (uintptr_t)image;
    thumbnail.pixels = pixels;
    thumbnail.width = tw;
    thumbnail.height = th;
    objc_setAssociatedObject(picture, &kALBSeedThumbnailKey, thumbnail, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return thumbnail;
}

/* The average luma of the picture behind `rect` (in the picture layer's
   coordinates), as it is shown (aspect fill / fit / stretch). */
static double ALBSeedPictureLuma(CALayer *picture, CGRect rect) {
    CGImageRef image = (__bridge CGImageRef)picture.contents;
    size_t width = CGImageGetWidth(image), height = CGImageGetHeight(image);
    if (!width || !height) return NAN;
    CGSize size = picture.bounds.size;
    if (size.width < 1 || size.height < 1) return NAN;
    NSString *gravity = picture.contentsGravity;
    CGRect shown;
    if ([gravity isEqualToString:kCAGravityResizeAspectFill] || [gravity isEqualToString:kCAGravityResizeAspect]) {
        CGFloat scale = [gravity isEqualToString:kCAGravityResizeAspectFill]
            ? MAX(size.width / width, size.height / height) : MIN(size.width / width, size.height / height);
        shown = CGRectMake((size.width - width * scale) / 2, (size.height - height * scale) / 2, width * scale, height * scale);
    } else if ([gravity isEqualToString:kCAGravityResize]) {
        shown = CGRectMake(0, 0, size.width, size.height);
    } else {
        return NAN;
    }
    CGRect area = CGRectIntersection(rect, shown);
    if (CGRectIsNull(area) || CGRectGetWidth(area) < 1 || CGRectGetHeight(area) < 1) return NAN;
    ALBSeedThumbnail *thumbnail = ALBSeedThumbnailFor(picture);
    if (!thumbnail) return NAN;
    // The area in thumbnail pixels (at least one).
    double tw = thumbnail.width, th = thumbnail.height;
    NSInteger x0 = (NSInteger)floor((area.origin.x - shown.origin.x) / shown.size.width * tw);
    NSInteger y0 = (NSInteger)floor((area.origin.y - shown.origin.y) / shown.size.height * th);
    NSInteger x1 = (NSInteger)ceil((CGRectGetMaxX(area) - shown.origin.x) / shown.size.width * tw);
    NSInteger y1 = (NSInteger)ceil((CGRectGetMaxY(area) - shown.origin.y) / shown.size.height * th);
    x0 = MAX(0, MIN((NSInteger)tw - 1, x0)); y0 = MAX(0, MIN((NSInteger)th - 1, y0));
    x1 = MAX(x0 + 1, MIN((NSInteger)tw, x1)); y1 = MAX(y0 + 1, MIN((NSInteger)th, y1));
    const uint8_t *bytes = thumbnail.pixels.bytes;
    double total = 0;
    NSUInteger count = 0;
    for (NSInteger y = y0; y < y1; y++)
        for (NSInteger x = x0; x < x1; x++) {
            const uint8_t *pixel = bytes + ((NSUInteger)y * thumbnail.width + (NSUInteger)x) * 4;
            if (pixel[3] < 8) continue;
            double alpha = pixel[3] / 255.0;
            total += (0.2126 * pixel[0] + 0.7152 * pixel[1] + 0.0722 * pixel[2]) / 255.0 / alpha;
            count++;
        }
    return count ? MIN(1.0, MAX(0.0, total / count)) : NAN;
}

/* The cover's luma behind one right-column button of a cell not on screen. */
static double ALBSeedCoverLuma(UIView *owner) {
    UIView *cell = ALBSeedCellOf(owner);
    if (!cell) { ALBSeedCoverNote(@"按钮不在 cell 里"); return NAN; }
    CALayer *picture = ALBSeedPictureLayer(cell);
    if (!picture) { gALBSeedCoverMissing++; ALBSeedNoteCellPictures(cell); return NAN; }
    UIView *pictureView = [picture.delegate isKindOfClass:UIView.class] ? (UIView *)picture.delegate : nil;
    if (!pictureView) return NAN;
    CGImageRef image = (__bridge CGImageRef)picture.contents;
    ALBSeedCoverNote([NSString stringWithFormat:@"封面图：%@ %.0f×%.0f 图%zu×%zu %@", NSStringFromClass(pictureView.class),
        CGRectGetWidth(pictureView.bounds), CGRectGetHeight(pictureView.bounds), CGImageGetWidth(image), CGImageGetHeight(image),
        picture.contentsGravity]);
    CGRect rect = CGRectInset([owner convertRect:owner.bounds toView:pictureView], -6, -6);
    return ALBSeedPictureLuma(picture, rect);
}

/* The feed list moved (AWEFeedTableView -setContentOffset:): right-column
   buttons of a cell about to come on screen whose pockets have not measured
   this video yet get a value now, while they are still off screen: this
   position's last reading of this video if it was seen, else the cover's
   luma behind it. Once per button and video. */
static void ALBGlassSeedFeedScrolled(UIScrollView *table) {
    if (!NSThread.isMainThread || !gALBSeedPocketClass || !table.window || !ALBSeedEnabled()) return;
    CFTimeInterval now = CACurrentMediaTime();
    if (now - gALBSeedFeedCheckedAt < 0.05) return;
    gALBSeedFeedCheckedAt = now;
    @try {
        UIWindow *window = table.window;
        CGRect screen = window.bounds;
        CGRect near = CGRectInset(screen, 0, -CGRectGetHeight(screen) * 0.75);
        for (UIButton *button in ALBSeedRightButtons()) {
            if (button.window != window || ![button isDescendantOfView:table]) continue;
            CGRect frame = [button convertRect:button.bounds toView:window];
            if (CGRectIntersectsRect(frame, screen) || !CGRectIntersectsRect(frame, near)) continue;
            ALBSeedFace *face = ALBSeedFaceFor(button, YES);
            // Each button's video looked up at most every 0.3 s while it waits.
            if (now - face.coverCheckedAt < 0.3) continue;
            face.coverCheckedAt = now;
            NSString *video = ALBSeedVideo(button, YES);
            if (!video.length || [face.coverVideo isEqualToString:video]) continue;
            face.coverVideo = video;
            gALBSeedFeedChecks++;
            NSMutableArray<UIView *> *unseen = [NSMutableArray array];
            for (UIView *blur in ALBSeedBlursOf(button)) {
                ALBSeedReceiver *state = ALBSeedReceiverFor(blur);
                if (!state || state.kind != 1) continue;
                if (state.hasLast && [state.lastVideo isEqualToString:video]) continue;
                [unseen addObject:blur];
            }
            if (!unseen.count) continue;
            NSString *source = @"看过";
            double luma = ALBSeedRemembered(button, video);
            if (isfinite(luma)) gALBSeedFeedFromCache++;
            double coverRaw = NAN;
            if (!isfinite(luma)) {
                coverRaw = ALBSeedCoverLuma(button);
                if (isfinite(coverRaw)) {
                    NSString *basis = nil;
                    double ratio = ALBSeedCoverRatio(button, &basis);
                    luma = MIN(1.0, MAX(0.0, coverRaw * ratio));
                    source = [NSString stringWithFormat:@"封面%@×%.2f", basis, ratio];
                    gALBSeedFeedFromCover++;
                }
            }
            // 3.11.31: no one has measured this video: this position's
            // typical brightness of late.
            if (!isfinite(luma)) {
                luma = ALBSeedPrior(button);
                source = @"常见";
                if (isfinite(luma)) gALBSeedFeedFromPrior++;
            }
            if (!isfinite(luma)) continue;
            ALBSeedStartSnap(button);
            for (UIView *blur in unseen) {
                ALBSeedReceiver *state = ALBSeedReceiverFor(blur);
                state.estimate = luma;
                state.estimateRaw = coverRaw;
                state.estimateSource = source;
                state.estimateVideo = video;
                state.realReads = 0;
                ALBSeedForget(blur);
                (void)ALBSeedDeliverAndSettle(blur, ^{ ALBSeedSend(blur, luma, YES); });
                ALBSeedApplyNow(blur);
            }
            gALBSeedFeedSeeds++;
        }
    } @catch (__unused NSException *exception) {
        gALBSeedExceptions++;
    }
}

/* Panel probes: a backdrop layer without filters draws exactly what is
   behind it (nearly transparent on top), and the system measures its luma
   like a pocket's. One per row of the panel's left icon column, left text
   column and right text column (the right icon column sits on the right
   column's own glass, whose readings serve there). */
static NSUInteger gALBSeedProbeReads, gALBSeedProbeSeeds, gALBSeedProbeHosts;
@interface ALBSeedLumaProbe : UIView
@property(nonatomic) double luma;
@property(nonatomic) CFTimeInterval lumaAt;
@property(nonatomic) NSInteger column;
@end
@implementation ALBSeedLumaProbe
+ (Class)layerClass {
    Class backdrop = NSClassFromString(@"CABackdropLayer");
    return backdrop ?: CALayer.class;
}
- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        _luma = NAN;
        self.userInteractionEnabled = NO;
        self.isAccessibilityElement = NO;
        self.accessibilityElementsHidden = YES;
        CALayer *layer = self.layer;
        // The pocket's own settings: luma every 0.25 s (its BackdropView's
        // value), captured at a quarter resolution.
        @try {
            if ([layer respondsToSelector:NSSelectorFromString(@"setTracksLuma:")]) [layer setValue:@YES forKey:@"tracksLuma"];
            if ([layer respondsToSelector:NSSelectorFromString(@"setLumaUpdateRate:")]) [layer setValue:@0.25 forKey:@"lumaUpdateRate"];
            if ([layer respondsToSelector:NSSelectorFromString(@"setScale:")]) [layer setValue:@0.25 forKey:@"scale"];
        } @catch (__unused NSException *exception) {}
        layer.opacity = 0.02f;
    }
    return self;
}
- (void)backdropLayer:(CALayer *)layer didChangeLuma:(double)luma {
    if (!isfinite(luma) || !NSThread.isMainThread) return;
    self.luma = luma;
    self.lumaAt = CACurrentMediaTime();
    gALBSeedProbeReads++;
}
@end

static __weak UIView *gALBSeedProbeHost;
static void ALBSeedEnsureProbes(UIWindow *window) {
    if (!window || !NSThread.isMainThread || !ALBSeedEnabled() || !NSClassFromString(@"CABackdropLayer")) return;
    UIView *host = gALBSeedProbeHost;
    if (host && host.superview == window && CGSizeEqualToSize(host.bounds.size, window.bounds.size)) return;
    [host removeFromSuperview];
    CGSize size = window.bounds.size;
    if (size.width < 100 || size.height < 300) return;
    host = [[UIView alloc] initWithFrame:window.bounds];
    host.userInteractionEnabled = NO;
    host.accessibilityElementsHidden = YES;
    host.accessibilityIdentifier = @"AwemeLiquidBar.LumaProbes";
    // The panel's own geometry (ALBSidePalette -layoutSubviews).
    CGFloat width = size.width, height = size.height;
    CGFloat top = MIN(MAX(window.safeAreaInsets.top + 54, 100), height * 0.25);
    CGFloat bottom = MIN(MAX(window.safeAreaInsets.bottom + 64, 90), height * 0.20);
    CGFloat scale = MIN(1.05, MAX(0.82, width / 393.0));
    CGFloat side = 38 * scale, pill = 146 * scale, gap = 9 * scale;
    CGFloat columnX[3] = {12, 12 + side + gap, width - 12 - side - gap - pill};
    CGFloat columnW[3] = {side, pill, pill};
    NSInteger rows = 6;
    CGFloat rowHeight = MAX(1, (height - top - bottom) / rows);
    for (NSInteger column = 0; column < 3; column++)
        for (NSInteger row = 0; row < rows; row++) {
            ALBSeedLumaProbe *probe = [[ALBSeedLumaProbe alloc] initWithFrame:
                CGRectMake(columnX[column], top + row * rowHeight, columnW[column], rowHeight)];
            probe.column = column;
            [host addSubview:probe];
        }
    UIView *root = window.rootViewController.viewIfLoaded;
    if (root.superview == window) [window insertSubview:host aboveSubview:root];
    else [window addSubview:host];
    gALBSeedProbeHost = host;
    gALBSeedProbeHosts++;
}

/* The probe measuring where `owner` (a panel icon or text) is, NAN when
   none has a fresh reading there. */
static double ALBSeedProbeLuma(UIView *owner, BOOL right, BOOL text) {
    UIView *host = gALBSeedProbeHost;
    if (!host || !owner.window || host.window != owner.window) return NAN;
    NSInteger column = right ? (text ? 2 : -1) : (text ? 1 : 0);
    if (column < 0) return NAN;
    CGFloat y = CGRectGetMidY([owner convertRect:owner.bounds toView:host]);
    CFTimeInterval now = CACurrentMediaTime();
    ALBSeedLumaProbe *best = nil;
    CGFloat bestDistance = 1e9;
    for (ALBSeedLumaProbe *probe in host.subviews) {
        if (![probe isKindOfClass:ALBSeedLumaProbe.class] || probe.column != column) continue;
        if (!isfinite(probe.luma) || now - probe.lumaAt > 5.0) continue;
        CGFloat distance = fabs(CGRectGetMidY(probe.frame) - y);
        if (distance < bestDistance) { bestDistance = distance; best = probe; }
    }
    return best && bestDistance <= CGRectGetHeight(best.frame) ? best.luma : NAN;
}

/* Before the panel hides the right column: its buttons' settled readings by
   height ({midY, luma} in window points). */
static NSArray<NSArray<NSNumber *> *> *ALBGlassSeedRightSamples(UIWindow *window) {
    NSMutableArray<NSArray<NSNumber *> *> *samples = [NSMutableArray array];
    if (!window || (!gALBSeedClassCount && !gALBSeedPocketClass) || !ALBSeedEnabled()) return samples;
    CFTimeInterval now = CACurrentMediaTime();
    for (UIButton *button in ALBSeedRightButtons()) {
        if (button.window != window || !ALBSeedViewShown(button)) continue;
        CGRect frame = [button convertRect:button.bounds toView:window];
        if (!CGRectIntersectsRect(frame, window.bounds)) continue;
        ALBSeedFace *face = ALBSeedFaceFor(button, NO);
        if (!face.hasStable || now - face.stableAt > 20) continue;
        NSString *video = ALBSeedVideo(button, YES);
        if (!video.length || ![video isEqualToString:face.stableVideo ?: @""]) continue;
        [samples addObject:@[@(CGRectGetMidY(frame)), @(face.stable)]];
    }
    return samples;
}

/* The panel was just built and laid out: every icon, pill and slider gets
   a start (the right column's reading at its height when the panel is
   beside it, or their median when they agree), handed to the pockets UIKit
   has built (as a new reading) and kept for the ones it builds next. */
static void ALBGlassSeedPanelOpened(UIView *paletteView, NSArray<NSArray<NSNumber *> *> *rightSamples) {
    if (!ALBSeedEnabled() || (!gALBSeedClassCount && !gALBSeedPocketClass) || ![paletteView isKindOfClass:ALBSidePalette.class]) return;
    ALBSidePalette *palette = (ALBSidePalette *)paletteView;
    UIWindow *window = palette.window;
    if (!window) return;
    @try {
        NSMutableArray<NSNumber *> *lumas = [NSMutableArray array];
        double low = 1, high = 0;
        for (NSArray<NSNumber *> *sample in rightSamples) {
            double luma = sample[1].doubleValue;
            [lumas addObject:@(luma)];
            low = MIN(low, luma); high = MAX(high, luma);
        }
        double median = ALBSeedMedian(lumas);
        BOOL agree = rightSamples.count >= 2 && high - low <= 0.3;
        BOOL beside = palette.anchorFraction.x > 0.5;
        NSUInteger token = ++gALBSeedToken, started = 0, handed = 0;
        NSMutableArray<UIView *> *owners = [NSMutableArray arrayWithArray:palette.icons ?: @[]];
        [owners addObjectsFromArray:palette.pills ?: @[]];
        NSUInteger iconCount = palette.icons.count;
        for (NSUInteger index = 0; index < owners.count; index++) {
            UIView *owner = owners[index];
            ALBSeedStartSnap(owner);
            double start = NAN;
            NSString *source = nil;
            // 3.11.30: measured right there by a probe.
            start = ALBSeedProbeLuma(owner, beside, index >= iconCount);
            if (isfinite(start)) { source = @"探针"; gALBSeedProbeSeeds++; }
            else if (agree) { start = median; source = @"右侧中位"; }
            else if (beside && rightSamples.count) {
                CGFloat y = CGRectGetMidY([owner convertRect:owner.bounds toView:window]);
                CGFloat best = 141;
                for (NSArray<NSNumber *> *sample in rightSamples) {
                    CGFloat distance = fabs(sample[0].doubleValue - y);
                    if (distance < best) { best = distance; start = sample[1].doubleValue; source = @"右侧就近"; }
                }
            }
            if (!isfinite(start)) { gALBSeedPanelNoStart++; continue; }
            ALBSeedSetPending(owner, start, source, token);
            started++;
            // Pockets already built get it now, as new (3.11.26); the ones
            // UIKit builds next get it when they come into the window.
            for (UIView *blur in ALBSeedBlursOf(owner)) {
                NSUInteger before = gALBSeedReplays;
                ALBSeedBlurEntered(blur, YES);
                if (gALBSeedReplays > before) handed++;
            }
        }
        gALBSeedPanelStarts += started;
        gALBSeedInjectPanel += handed;
        ALBSeedNote([NSString stringWithFormat:@"面板打开 %@ 右侧样本%lu%@ 起步%lu/%lu 当场给%lu",
            beside ? @"在右" : @"在左", (unsigned long)rightSamples.count,
            rightSamples.count ? [NSString stringWithFormat:@"(%.2f–%.2f 中位%.2f)", low, high, median] : @"",
            (unsigned long)started, (unsigned long)owners.count, (unsigned long)handed]);
    } @catch (__unused NSException *exception) {
        gALBSeedExceptions++;
    }
}

/* The panel let the right column show again: every button of the video it
   showed keeps its own last settled reading as the start of the glass UIKit
   is about to build again. */
static void ALBGlassSeedRightRestored(void) {
    if (!ALBSeedEnabled() || (!gALBSeedClassCount && !gALBSeedPocketClass)) return;
    @try {
        CFTimeInterval now = CACurrentMediaTime();
        gALBSeedRightRestoredAt = now;
        NSUInteger token = ++gALBSeedToken, count = 0;
        for (UIButton *button in ALBSeedRightButtons()) {
            if (!button.window) continue;
            // Its glass is built again as soon as it is shown (3.11.27).
            ALBSeedStartSnap(button);
            ALBSeedFace *face = ALBSeedFaceFor(button, NO);
            if (!face.hasStable || now - face.stableAt > 300) continue;
            NSString *video = ALBSeedVideo(button, YES);
            if (!video.length || ![video isEqualToString:face.stableVideo ?: @""]) continue;
            ALBSeedSetPending(button, face.stable, @"自己上次", token);
            count++;
        }
        gALBSeedRightPending += count;
        // Its pockets come back right after this (ALB5ApplyRightLayout, then
        // UIKit puts them in again): ALBSeedBlurEntered hands it over.
        ALBSeedNote([NSString stringWithFormat:@"右侧恢复 留起步%lu颗", (unsigned long)count]);
    } @catch (__unused NSException *exception) {
        gALBSeedExceptions++;
    }
}

/* 3.11.28: the panel starts closing: its glasses' windows end, so their
   fade-out is drawn as UIKit makes it. */
static void ALBGlassSeedPanelClosing(UIView *paletteView) {
    if (!paletteView) return;
    @try {
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithObject:paletteView];
        for (NSUInteger i = 0; i < queue.count && i < 1500; i++) {
            UIView *view = queue[i];
            ALBSeedFace *face = objc_getAssociatedObject(view, &kALBSeedFaceKey);
            if (face) face.snapUntil = 0;
            [queue addObjectsFromArray:view.subviews];
        }
    } @catch (__unused NSException *exception) {
        gALBSeedExceptions++;
    }
}

#pragma mark - Install

static Method ALBSeedOwnMethod(Class cls, SEL selector) {
    unsigned int count = 0;
    Method *methods = class_copyMethodList(cls, &count);
    Method found = NULL;
    for (unsigned int i = 0; i < count; i++) if (method_getName(methods[i]) == selector) { found = methods[i]; break; }
    free(methods);
    return found;
}

static BOOL ALBSeedIsViewClass(Class cls) {
    for (Class cursor = cls; cursor; cursor = class_getSuperclass(cursor)) if (cursor == UIView.class) return YES;
    return NO;
}

static BOOL ALBSeedHookLuma(Class cls, SEL selector) {
    if (gALBSeedClassCount >= ALB_SEED_MAX_CLASSES) return NO;
    Method method = ALBSeedOwnMethod(cls, selector);
    if (!method || method_getNumberOfArguments(method) != 3) return NO;
    char result[16] = {0}, argument[16] = {0};
    method_getReturnType(method, result, sizeof result);
    method_getArgumentType(method, 2, argument, sizeof argument);
    if (result[0] != 'v' || argument[0] != 'd') return NO;
    ALBSeedLumaIMP original = (ALBSeedLumaIMP)method_getImplementation(method);
    if (!original) return NO;
    NSUInteger slot = gALBSeedClassCount;
    gALBSeedClasses[slot] = cls;
    gALBSeedOriginals[slot] = original;
    IMP replacement = imp_implementationWithBlock(^(id receiver, double luma) {
        ALBSeedReceive(receiver, selector, luma, original);
    });
    method_setImplementation(method, replacement);
    gALBSeedClassCount++;
    return YES;
}

static void ALBSeedHookLayout(Class cls) {
    if (cls == UIView.class) return;
    SEL selector = @selector(layoutSubviews);
    Method method = ALBSeedOwnMethod(cls, selector);
    if (!method || method_getNumberOfArguments(method) != 2) return;
    ALBSeedLayoutIMP original = (ALBSeedLayoutIMP)method_getImplementation(method);
    if (!original) return;
    IMP replacement = imp_implementationWithBlock(^(id receiver) {
        original(receiver, selector);
        if (!NSThread.isMainThread) return;
        @try { ALBSeedAfterLayout((UIView *)receiver); }
        @catch (__unused NSException *exception) { gALBSeedExceptions++; }
    });
    method_setImplementation(method, replacement);
    gALBSeedLayoutHooks++;
}

/* didMoveToWindow of a pocket view (after every view inside it did its
   own) or of a blur (when only the blur comes back): a pocket of ours came
   back (3.11.26). */
static BOOL ALBSeedHookMove(Class cls, BOOL pocketLevel) {
    SEL selector = @selector(didMoveToWindow);
    Method method = ALBSeedOwnMethod(cls, selector);
    if (!method || method_getNumberOfArguments(method) != 2) return NO;
    ALBSeedLayoutIMP original = (ALBSeedLayoutIMP)method_getImplementation(method);
    if (!original) return NO;
    method_setImplementation(method, imp_implementationWithBlock(^(id receiver) {
        original(receiver, selector);
        if (!NSThread.isMainThread) return;
        @try {
            if (pocketLevel) ALBSeedPocketMoved((UIView *)receiver);
            else ALBSeedBlurEntered((UIView *)receiver, NO);
        } @catch (__unused NSException *exception) {
            gALBSeedExceptions++;
        }
    }));
    return YES;
}

/* NSTimer's one-shot factories (3.11.28): a timer made while we hand one
   of our pockets a value is kept (ALBSeedCaptureTimer); otherwise the call
   is untouched. */
typedef NSTimer *(*ALBSeedTimerBlockIMP)(id, SEL, NSTimeInterval, BOOL, id);
typedef NSTimer *(*ALBSeedTimerTargetIMP)(id, SEL, NSTimeInterval, id, SEL, id, BOOL);
static void ALBSeedHookTimers(void) {
    Class meta = object_getClass(NSTimer.class);
    if (!meta) return;
    for (NSString *name in @[@"scheduledTimerWithTimeInterval:repeats:block:", @"timerWithTimeInterval:repeats:block:"]) {
        SEL selector = NSSelectorFromString(name);
        Method method = ALBSeedOwnMethod(meta, selector);
        if (!method || method_getNumberOfArguments(method) != 5) continue;
        char result[8] = {0}, interval[8] = {0}, repeats[8] = {0}, block[8] = {0};
        method_getReturnType(method, result, sizeof result);
        method_getArgumentType(method, 2, interval, sizeof interval);
        method_getArgumentType(method, 3, repeats, sizeof repeats);
        method_getArgumentType(method, 4, block, sizeof block);
        if (result[0] != '@' || interval[0] != 'd' || (repeats[0] != 'B' && repeats[0] != 'c') || block[0] != '@') continue;
        ALBSeedTimerBlockIMP original = (ALBSeedTimerBlockIMP)method_getImplementation(method);
        if (!original) continue;
        method_setImplementation(method, imp_implementationWithBlock(^NSTimer *(id cls, NSTimeInterval seconds, BOOL again, id work) {
            NSTimer *timer = original(cls, selector, seconds, again, work);
            ALBSeedCaptureTimer(timer, again);
            return timer;
        }));
        gALBSeedTimerHooks++;
    }
    for (NSString *name in @[@"scheduledTimerWithTimeInterval:target:selector:userInfo:repeats:",
                             @"timerWithTimeInterval:target:selector:userInfo:repeats:"]) {
        SEL selector = NSSelectorFromString(name);
        Method method = ALBSeedOwnMethod(meta, selector);
        if (!method || method_getNumberOfArguments(method) != 7) continue;
        char result[8] = {0}, interval[8] = {0}, target[8] = {0}, action[8] = {0}, info[8] = {0}, repeats[8] = {0};
        method_getReturnType(method, result, sizeof result);
        method_getArgumentType(method, 2, interval, sizeof interval);
        method_getArgumentType(method, 3, target, sizeof target);
        method_getArgumentType(method, 4, action, sizeof action);
        method_getArgumentType(method, 5, info, sizeof info);
        method_getArgumentType(method, 6, repeats, sizeof repeats);
        if (result[0] != '@' || interval[0] != 'd' || target[0] != '@' || action[0] != ':' || info[0] != '@' ||
            (repeats[0] != 'B' && repeats[0] != 'c')) continue;
        ALBSeedTimerTargetIMP original = (ALBSeedTimerTargetIMP)method_getImplementation(method);
        if (!original) continue;
        method_setImplementation(method, imp_implementationWithBlock(^NSTimer *(id cls, NSTimeInterval seconds, id aim, SEL work, id userInfo, BOOL again) {
            NSTimer *timer = original(cls, selector, seconds, aim, work, userInfo, again);
            ALBSeedCaptureTimer(timer, again);
            return timer;
        }));
        gALBSeedTimerHooks++;
    }
}

/* The edge pocket's backdrop view (its luminance entry), the pocket view and
   its blur, by class name: ScrollEdgeEffectView (iOS 26.1+) or
   _UIScrollPocket (26.0). */
static void ALBSeedInstallPocket(const char **names, unsigned int count) {
    SEL entry = NSSelectorFromString(@"backdropLayer:didChangeLuma:");
    NSMutableArray<Class> *family = [NSMutableArray array];
    Class pocketView = Nil, pocketViewExact = Nil;
    for (unsigned int i = 0; names && i < count; i++) {
        const char *name = names[i];
        BOOL member = strstr(name, "ScrollEdgeEffectView") || strstr(name, "_UIScrollPocket");
        if (!member) continue;
        Class cls = objc_getClass(name);
        if (!cls || !ALBSeedIsViewClass(cls)) continue;
        [family addObject:cls];
        size_t length = strlen(name);
        if ((length >= 20 && !strcmp(name + length - 20, "ScrollEdgeEffectView")) || !strcmp(name, "_UIScrollPocket")) {
            if (!pocketView) pocketView = cls;
            if (!strcmp(name, "_TtC5UIKit20ScrollEdgeEffectView") || !strcmp(name, "_UIScrollPocket")) pocketViewExact = cls;
            continue;
        }
        if (gALBSeedPocketClass || !strstr(name, "BackdropView")) continue;
        Method method = ALBSeedOwnMethod(cls, entry);
        if (!method || method_getNumberOfArguments(method) != 4) continue;
        char result[16] = {0}, first[16] = {0}, second[16] = {0};
        method_getReturnType(method, result, sizeof result);
        method_getArgumentType(method, 2, first, sizeof first);
        method_getArgumentType(method, 3, second, sizeof second);
        if (result[0] != 'v' || first[0] != '@' || second[0] != 'd') continue;
        ALBSeedPocketIMP original = (ALBSeedPocketIMP)method_getImplementation(method);
        if (!original) continue;
        method_setImplementation(method, imp_implementationWithBlock(^(id receiver, id layer, double luma) {
            ALBSeedPocketReceive(receiver, entry, layer, luma, original);
        }));
        gALBSeedPocketClass = cls;
        gALBSeedPocketOriginal = original;
    }
    gALBSeedPocketViewClass = pocketViewExact ?: pocketView;
    // Where a pocket of ours comes back.
    NSMutableArray<NSString *> *hooks = [NSMutableArray array];
    if (gALBSeedPocketClass) {
        for (Class cls in family) {
            if (!strstr(class_getName(cls), "PocketBlur") || ![cls isSubclassOfClass:gALBSeedPocketClass]) continue;
            gALBSeedBlurClass = cls;
            if (ALBSeedHookMove(cls, NO)) [hooks addObject:@"模糊层"];
            break;
        }
        if (gALBSeedPocketViewClass && ALBSeedHookMove(gALBSeedPocketViewClass, YES)) [hooks addObject:@"口袋"];
    }
    gALBSeedEntryHooks = hooks.count;
    gALBSeedEntryHook = hooks.count ? [hooks componentsJoinedByString:@"+"] : @"未装";
    gALBSeedPocketStatus = gALBSeedPocketClass
        ? [NSString stringWithFormat:@"已装（回来时：%@；模糊层类%@）", gALBSeedEntryHook, gALBSeedBlurClass ? @"有" : @"无"]
        : @"未装：找不到边缘口袋的亮度入口";
}

static void ALBGlassSeedInstall(void) {
    if (!NSThread.isMainThread || gALBSeedInstallAttempted) return;
    gALBSeedInstallAttempted = YES;
    gALBSeedT0 = CACurrentMediaTime();
    if (@available(iOS 26.0, *)) {
        SEL selector = NSSelectorFromString(@"_materialDidReceiveLuma:");
        gALBSeedHookNames = [NSMutableArray array];
        NSMutableArray<Class> *subclasses = [NSMutableArray array];
        const char *image = class_getImageName(UIView.class);
        unsigned int count = 0;
        const char **names = image ? objc_copyClassNamesForImage(image, &count) : NULL;
        ALBSeedInstallPocket(names, count);
        if (gALBSeedPocketClass) ALBSeedHookTimers();
        for (unsigned int i = 0; names && i < count && subclasses.count < 40; i++) {
            const char *name = names[i];
            if (!strstr(name, "Glass") && !strstr(name, "Material") && !strstr(name, "Backdrop")) continue;
            Class cls = objc_getClass(name);
            if (!cls || cls == UIView.class || !ALBSeedIsViewClass(cls)) continue;
            if (ALBSeedOwnMethod(cls, selector)) [subclasses addObject:cls];
        }
        if (names) free(names);
        [subclasses sortUsingComparator:^NSComparisonResult(Class a, Class b) {
            BOOL ga = [NSStringFromClass(a) containsString:@"GlassGroupLayerView"];
            BOOL gb = [NSStringFromClass(b) containsString:@"GlassGroupLayerView"];
            return ga == gb ? NSOrderedSame : (ga ? NSOrderedAscending : NSOrderedDescending);
        }];
        if (ALBSeedHookLuma(UIView.class, selector)) [gALBSeedHookNames addObject:@"UIView"];
        for (Class cls in subclasses) {
            if (!ALBSeedHookLuma(cls, selector)) continue;
            NSString *name = NSStringFromClass(cls);
            NSRange range = [name rangeOfString:@"GlassGroupLayerView"];
            [gALBSeedHookNames addObject:range.location != NSNotFound ? @"GlassGroupLayerView" : name];
            ALBSeedHookLayout(cls);
        }
        gALBSeedStatus = gALBSeedClassCount
            ? [NSString stringWithFormat:@"玻璃入口已装 %@（布局%lu）；边缘口袋入口%@", [gALBSeedHookNames componentsJoinedByString:@"、"],
               (unsigned long)gALBSeedLayoutHooks, gALBSeedPocketStatus]
            : [NSString stringWithFormat:@"玻璃入口未装；边缘口袋入口%@", gALBSeedPocketStatus];
    } else {
        gALBSeedStatus = @"未装：需要 iOS 26";
    }
}

#pragma mark - 诊断

static NSString *ALBGlassSeedReport(void) {
    if (!NSThread.isMainThread) return @"需在主线程读取";
    ALBSeedModelInfo model = ALBSeedModelNow();
    NSMutableArray<NSString *> *shape = [NSMutableArray array];
    for (NSUInteger k = 0; k < ALB_SEED_STEPS; k++)
        [shape addObject:isfinite(model.g[k]) ? [NSString stringWithFormat:@"%.2f", model.g[k]] : @"-"];
    NSString *walk = [NSString stringWithFormat:@"每次读数走完的比例[%@] 新层起点%@ 首读预测命中%ld/%ld → %@",
        [shape componentsJoinedByString:@" "],
        isfinite(model.s) ? [NSString stringWithFormat:@"%.2f", model.s] : @"-", (long)model.hits, (long)model.tested,
        model.raw ? @"读数一来就接近真值" : (model.validated ? @"已验证：用读数直接预测最终值" : @"未验证：只用起步值保持")];
    NSMutableString *text = [NSMutableString stringWithFormat:
        @"新玻璃直接到位：%@；钩子：%@\n"
        @"玻璃入口读数：右侧%lu 面板%lu 顶栏(参照)%lu 其他%lu；边缘口袋读数：右侧%lu 面板%lu 其他%lu；本插件的接收类：%@；其他接收类：%@\n"
        @"重新开始：右侧新玻璃%lu 换作品%lu；面板新玻璃%lu\n"
        @"起步：面板给%lu颗(无可用%lu) 当场%lu；右侧恢复留%lu 当场%lu；布局时补给%lu；首值不带动画%lu；保持起步%lu；预测%lu；异常%lu\n"
        @"读数爬升：样本%ld（可检验%ld）%@\n"
        @"口袋回来重看（3.11.26）：回来钩子%@；字段 %@；回来%lu次 补给%lu次 头一秒读数%lu（清读数%lu 清评估%lu 拒写%lu）；回来窗口（3.11.27）%lu次 当场让玻璃跟上%lu次\n"
        @"口袋当场定档（3.11.28）：交时已定%lu 布局后定%lu 计时器后定%lu 仍未定%lu 不明%lu；计时器钩子%lu 截到%lu 提前触发%lu；面板藏右侧不触发系统显隐：藏%lu 显%lu 退回普通%lu；藏着时右侧口袋读数%lu%@\n"
        @"看不见时的读数不交（3.11.29）：%lu%@；亮度上报加快：3.11.30 已撤（3.11.17 已证明读数间隔最小 0.25 秒；系统原值%@）\n"
        @"出现前的亮度（3.11.30）：滑入前检查%lu 给值%lu（看过%lu 封面%lu 常见%lu）没找到封面%lu；首个0读数不交%lu；面板探针：%lu组 读数%lu 用上%lu；估值与真实：%@",
        ALBSeedEnabled() ? @"开" : @"关", gALBSeedStatus,
        (unsigned long)gALBSeedReadsRight, (unsigned long)gALBSeedReadsPanel, (unsigned long)gALBSeedReadsTop, (unsigned long)gALBSeedReadsOther,
        (unsigned long)gALBSeedPocketReadsRight, (unsigned long)gALBSeedPocketReadsPanel, (unsigned long)gALBSeedPocketReadsOther,
        gALBSeedOurClasses.count ? [gALBSeedOurClasses.allObjects componentsJoinedByString:@"、"] : @"无",
        gALBSeedOtherClasses.count ? [gALBSeedOtherClasses.allObjects componentsJoinedByString:@"、"] : @"无",
        (unsigned long)gALBSeedFreshRight, (unsigned long)gALBSeedVideoRight, (unsigned long)gALBSeedFreshPanel,
        (unsigned long)gALBSeedPanelStarts, (unsigned long)gALBSeedPanelNoStart, (unsigned long)gALBSeedInjectPanel,
        (unsigned long)gALBSeedRightPending, (unsigned long)gALBSeedInjectRight, (unsigned long)gALBSeedInjectLayout,
        (unsigned long)gALBSeedFirstInstant, (unsigned long)gALBSeedHeld, (unsigned long)gALBSeedPredicted, (unsigned long)gALBSeedExceptions,
        (long)model.samples, (long)model.informative, walk,
        gALBSeedEntryHook, gALBSeedOffsetsText, (unsigned long)gALBSeedEntries, (unsigned long)gALBSeedReplays,
        (unsigned long)gALBSeedFreshReads, (unsigned long)gALBSeedForgotBackdrop, (unsigned long)gALBSeedForgotPocket,
        (unsigned long)gALBSeedForgetRefused, (unsigned long)gALBSeedSnapWindows, (unsigned long)gALBSeedAppliedNow,
        (unsigned long)gALBSeedSettled[0], (unsigned long)gALBSeedSettled[1], (unsigned long)gALBSeedSettled[2],
        (unsigned long)gALBSeedSettled[3], (unsigned long)gALBSeedSettled[4], (unsigned long)gALBSeedTimerHooks,
        (unsigned long)gALBSeedTimersCaptured, (unsigned long)gALBSeedTimersFired,
        (unsigned long)gALB7RightQuietHides, (unsigned long)gALB7RightQuietShows, (unsigned long)gALB7RightQuietFallbacks,
        (unsigned long)gALBSeedHiddenReads,
        gALBSeedHiddenReads ? [NSString stringWithFormat:@"（%.2f–%.2f）", gALBSeedHiddenLow, gALBSeedHiddenHigh] : @"",
        (unsigned long)gALBSeedDroppedUnseen,
        isfinite(gALBSeedUnseenLow) ? [NSString stringWithFormat:@"（%.2f–%.2f）", gALBSeedUnseenLow, gALBSeedUnseenHigh] : @"",
        @"0.25",
        (unsigned long)gALBSeedFeedChecks, (unsigned long)gALBSeedFeedSeeds, (unsigned long)gALBSeedFeedFromCache,
        (unsigned long)gALBSeedFeedFromCover, (unsigned long)gALBSeedFeedFromPrior, (unsigned long)gALBSeedCoverMissing, (unsigned long)gALBSeedZeroDropped,
        (unsigned long)gALBSeedProbeHosts, (unsigned long)gALBSeedProbeReads, (unsigned long)gALBSeedProbeSeeds,
        gALBSeedEstimatePairs.count ? [gALBSeedEstimatePairs componentsJoinedByString:@" "] : @"-"];
    if (gALBSeedCoverNotes.count) [text appendFormat:@"\n封面：%@", [gALBSeedCoverNotes componentsJoinedByString:@"\n"]];
    // 3.11.31: the three-part dock on its own edge.
    {
        UIScrollView *dockEdge = nil;
        Class overlayClass = NSClassFromString(@"ALBDockOverlayView");
        for (UIView *subview in AWEActiveMainWindow().subviews)
            if (overlayClass && [subview isKindOfClass:overlayClass])
                for (UIView *child in subview.subviews)
                    if ([child.accessibilityIdentifier isEqualToString:@"AwemeLiquidBar.DockEdge"] && [child isKindOfClass:UIScrollView.class])
                        dockEdge = (UIScrollView *)child;
        NSString *dockState = @"无边";
        if (dockEdge) {
            UIView *blur = nil;
            NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:dockEdge.subviews];
            for (NSUInteger i = 0; i < queue.count && i < 40 && !blur; i++) {
                if (ALBSeedIsBlur(queue[i])) blur = queue[i];
                [queue addObjectsFromArray:queue[i].subviews];
            }
            dockState = [NSString stringWithFormat:@"边%@ 登记%lu/4 %@", dockEdge.overrideUserInterfaceStyle == UIUserInterfaceStyleLight ? @"浅" : @"深",
                (unsigned long)gALBDockEdgeRegistered, blur ? ALBSeedPocketStateText(blur) : @"口袋未出现"];
        }
        [text appendFormat:@"\n三段式底栏玻璃（3.11.31，dock.followEdge=%@）：%@", ALBDockHostFollowEdge() ? @"开" : @"关", dockState];
    }
    // The probes now (left icons / left text / right text, top to bottom).
    UIView *probeHost = gALBSeedProbeHost;
    if (probeHost) {
        NSMutableArray<NSString *> *columns = [NSMutableArray array];
        for (NSInteger column = 0; column < 3; column++) {
            NSMutableArray<NSString *> *values = [NSMutableArray array];
            for (ALBSeedLumaProbe *probe in probeHost.subviews)
                if ([probe isKindOfClass:ALBSeedLumaProbe.class] && probe.column == column)
                    [values addObject:isfinite(probe.luma) ? [NSString stringWithFormat:@"%.2f", probe.luma] : @"-"];
            [columns addObject:[values componentsJoinedByString:@" "]];
        }
        [text appendFormat:@"\n探针现在：左图标[%@] 左文字[%@] 右文字[%@]", columns[0], columns[1], columns[2]];
    }
    // The right column on screen now: its buttons' settled readings.
    UIWindow *window = AWEActiveMainWindow();
    NSArray<NSArray<NSNumber *> *> *now = ALBGlassSeedRightSamples(window);
    if (now.count) {
        NSMutableArray<NSString *> *parts = [NSMutableArray array];
        for (NSArray<NSNumber *> *sample in [now sortedArrayUsingComparator:^NSComparisonResult(NSArray *a, NSArray *b) {
            return [a[0] compare:b[0]]; }])
            [parts addObject:[NSString stringWithFormat:@"y%.0f=%.2f", sample[0].doubleValue, sample[1].doubleValue]];
        [text appendFormat:@"\n右侧当前读数：%@", [parts componentsJoinedByString:@" "]];
    }
    // Every right-column pocket on screen now (third level 0.85 = still at
    // its start).
    ALBSeedLookOffsets();
    NSMutableArray<NSArray *> *pockets = [NSMutableArray array];
    for (UIButton *button in ALBSeedRightButtons()) {
        if (button.window != window || !ALBSeedViewShown(button)) continue;
        CGRect frame = [button convertRect:button.bounds toView:window];
        if (!CGRectIntersectsRect(frame, window.bounds)) continue;
        UIView *blur = ALBSeedBlursOf(button).firstObject;
        [pockets addObject:@[@(CGRectGetMidY(frame)), blur ? ALBSeedPocketStateText(blur) : @"没有口袋"]];
    }
    [pockets sortUsingComparator:^NSComparisonResult(NSArray *a, NSArray *b) { return [a[0] compare:b[0]]; }];
    for (NSArray *pocket in pockets)
        [text appendFormat:@"\n右侧口袋现在 y%.0f：%@", [pocket[0] doubleValue], pocket[1]];
    [text appendFormat:@"\n口袋亮度样式编码：%@", gALBSeedStyleEncoding];
    Class layouts[3] = { gALBSeedPocketViewClass, gALBSeedPocketClass, gALBSeedBlurClass };
    for (int c = 0; c < 3; c++) {
        Class cls = layouts[c];
        if (!cls) continue;
        unsigned int count = 0;
        Ivar *ivars = class_copyIvarList(cls, &count);
        NSMutableArray<NSString *> *parts = [NSMutableArray array];
        for (unsigned int i = 0; i < count && i < 40; i++) {
            const char *type = ivar_getTypeEncoding(ivars[i]);
            [parts addObject:[NSString stringWithFormat:@"%s@%td%@", ivar_getName(ivars[i]) ?: "?", ivar_getOffset(ivars[i]),
                type && type[0] ? [NSString stringWithFormat:@"(%.14s)", type] : @""]];
        }
        if (ivars) free(ivars);
        NSString *name = NSStringFromClass(cls);
        if (name.length > 34) name = [name substringFromIndex:name.length - 34];
        [text appendFormat:@"\n字段布局 %@[%zu]：%@", name, class_getInstanceSize(cls), [parts componentsJoinedByString:@" "]];
    }
    if (gALBSeedOtherChains.count)
        [text appendFormat:@"\n其他读数从哪来：\n%@", [gALBSeedOtherChains componentsJoinedByString:@"\n"]];
    if (gALBSeedEvents.count) [text appendFormat:@"\n最近：\n%@", [gALBSeedEvents componentsJoinedByString:@"\n"]];
    [text appendFormat:@"\n%@", ALBGlassImmediateReport()];
    return text;
}

#endif
