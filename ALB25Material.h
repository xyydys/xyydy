#pragma once
#import <UIKit/UIKit.h>
#include "ALB25MaterialPolicy.h"
NS_ASSUME_NONNULL_BEGIN
/// Resolved ALB25MaterialRegular / ALB25MaterialClear / ALB25MaterialNagram.
FOUNDATION_EXPORT NSInteger ALB25MaterialMode(void);
FOUNDATION_EXPORT BOOL ALB25MaterialEnabled(void);
/// Enabled and Clear or Nagram (on this page): every plug-in glass uses UIGlassEffect Clear.
FOUNDATION_EXPORT BOOL ALB25MaterialUsesClearStyle(void);
/// Clear is wanted on the bottom bar and the top group, and this OS has a
/// writable platter material (iOS 27) or the iOS 26 renderer experiment is on.
FOUNDATION_EXPORT BOOL ALB25MaterialBarsWantClear(void);
/// The bottom bar's target page is a video feed (Home/Friends) or not. Clear
/// modes draw Clear only on a video feed unless that option is turned off.
FOUNDATION_EXPORT void ALB25MaterialNoteVideoContext(BOOL video);
FOUNDATION_EXPORT NSUInteger ALB25MaterialGeneration(void);
FOUNDATION_EXPORT UIUserInterfaceStyle ALB25MaterialStyle(UIView * _Nullable view, BOOL feedEligible);
FOUNDATION_EXPORT void ALB25MaterialInstallResolvers(BOOL (^feed)(void), UIUserInterfaceStyle (^app)(void));
/// Uses the current trait collection's style for the Clear Dark tint.
FOUNDATION_EXPORT UIVisualEffect * _Nullable ALB25MaterialMakeEffect(BOOL interactive);
FOUNDATION_EXPORT UIButtonConfiguration *ALB25MaterialButtonConfiguration(void);
FOUNDATION_EXPORT void ALB25MaterialApplyButton(UIButton *button, BOOL feedEligible);
/// 2.5: a native carrier draws this button's glass (and icon); the button
/// keeps only the touch, menu and actions and draws nothing itself.
FOUNDATION_EXPORT void ALB25MaterialSetButtonCarried(UIButton *button, BOOL carried);
FOUNDATION_EXPORT BOOL ALB25MaterialButtonIsCarried(UIButton *button);
/// 2.5.1: UIKit morphs a glass UIButton into its menu and back; an empty
/// (carried) button gives that morph nothing. For one menu session the
/// button gets its own glass back, applied now (outside the press guard),
/// and its material is then frozen until the session ends. Returns what the
/// button holds afterwards ("ownGlass" NO: the caller must not lease it).
FOUNDATION_EXPORT NSDictionary *ALB25MaterialBeginButtonMenuSession(UIButton *button);
FOUNDATION_EXPORT void ALB25MaterialEndButtonMenuSession(UIButton *button);
FOUNDATION_EXPORT BOOL ALB25MaterialButtonInMenuSession(UIButton * _Nullable button);
/// Applies the material now even while UIKit tracks the button: a carried
/// button is emptied, an uncarried one gets its glass.
FOUNDATION_EXPORT void ALB25MaterialApplyButtonNow(UIButton *button, BOOL feedEligible);
/// Read-only probe helpers (top-left menu probe).
FOUNDATION_EXPORT NSDictionary *ALB25MaterialButtonProbeState(UIButton * _Nullable button);
FOUNDATION_EXPORT NSString * _Nullable ALB25MaterialViewGlassSummary(UIView * _Nullable view);
/// The material audit this file keeps for a surface (path, mode, style), or nil.
FOUNDATION_EXPORT NSDictionary * _Nullable ALB25MaterialAudit(UIView * _Nullable view);
FOUNDATION_EXPORT void ALB25MaterialApplyEffectView(UIVisualEffectView *view, BOOL feedEligible);
FOUNDATION_EXPORT void ALB25MaterialApplyBar(UITabBar *bar, BOOL bottom);
FOUNDATION_EXPORT void ALB25MaterialRefresh(void);
FOUNDATION_EXPORT NSDictionary *ALB25MaterialSnapshot(void);
/// One-shot structure probe (bars, platters, glass owners, layers, button
/// factories, private glass selectors). Main thread. Never changes material.
FOUNDATION_EXPORT NSDictionary *ALB25MaterialProbeReport(void);
/// 3.2: the right column and double-tap panel draw the top chrome's
/// registered Dark glass (setting "right.registeredGlass", on by default since 3.3).
FOUNDATION_EXPORT BOOL ALB25MaterialRegisteredGlassEnabled(void);
/// 3.7 "glass.lifted": feed glass drawn as plain view glass, unregistered.
FOUNDATION_EXPORT BOOL ALB25MaterialGlassLifted(void);
/// 3.9: the Clear recipe for lifted glass ("glass.clear").
FOUNDATION_EXPORT BOOL ALB25MaterialLiftedClear(void);
FOUNDATION_EXPORT UIVisualEffect *ALB25MaterialLiftedEffect(BOOL interactive);
FOUNDATION_EXPORT BOOL ALB25MaterialInstallLiftedGlass(UIButtonConfiguration * _Nullable configuration);
/// 3.11: the right column's and the panel's glass (the collapsing dock's
/// video glass): Regular, or Clear + 28 % black with "glass.motionClear";
/// plain, never registered, Dark set on the view by the caller.
FOUNDATION_EXPORT BOOL ALB25MaterialMotionClear(void);
/// 玻璃透明度（0＝系统玻璃，越大越透）：顶栏、三段式底栏各一个
FOUNDATION_EXPORT CGFloat ALB25MaterialGlassAlpha(void);
FOUNDATION_EXPORT CGFloat ALB25MaterialTopAlpha(void);
FOUNDATION_EXPORT CGFloat ALB25MaterialDockAlpha(void);
FOUNDATION_EXPORT CGFloat ALB25MaterialTopDarkTint(void);
FOUNDATION_EXPORT UIVisualEffect *ALB25MaterialMotionEffect(BOOL interactive);
FOUNDATION_EXPORT UIButtonConfiguration *ALB25MaterialMotionButtonConfiguration(void);
/// 3.11.1: the same glass in a tone: dark (black "glass.darkTint") or light
/// (white "glass.lightTint") with "glass.motionClear"; Regular is untinted
/// and takes its tone from the caller's light/dark override.
FOUNDATION_EXPORT CGFloat ALB25MaterialDarkTint(void);
FOUNDATION_EXPORT CGFloat ALB25MaterialLightTint(void);
FOUNDATION_EXPORT UIColor *ALB25MaterialToneTint(BOOL light);
FOUNDATION_EXPORT UIVisualEffect *ALB25MaterialMotionEffectTone(BOOL interactive, BOOL light);
FOUNDATION_EXPORT UIButtonConfiguration *ALB25MaterialMotionButtonConfigurationTone(BOOL light);
// 3.11.4 (3.11.19: always on iOS 26): the right column and the
// double-tap panel draw the top chrome's way: the bottom bar's own realized
// glass (it follows the system's 透明／着色) in the glass button, each glass
// registered with an edge of its own (Tweak.x ALBFollowSyncEdge), so UIKit
// turns it light or dark by what is behind it, as it does the bars. The
// plug-in's Clear recipe and its own tones are not used for them then.
FOUNDATION_EXPORT BOOL ALB25MaterialFollowBar(void);
/// Which glass that is right now ("b:…" the bottom bar's, "t:…" the top
/// chrome's, "recipe"): a glass button built under another key is rebuilt.
FOUNDATION_EXPORT NSString *ALB25MaterialFollowGlassKey(void);
/// 3.11.9: that glass as a visual effect (UIGlassEffect +effectWithGlass:),
/// UIKit's Regular when there is none (the status toast).
FOUNDATION_EXPORT UIVisualEffect *ALB25MaterialFollowEffect(void);
/// 3.11.1: UIKit's own glass under `root` (bar items, a tab bar's platter, a
/// menu's platters) as the Clear recipe in a tone; `clear` NO restores it.
/// Visits at most `budget` views. Returns a short 诊断 text.
FOUNDATION_EXPORT NSString *ALB25MaterialApplyTone(UIView * _Nullable root, BOOL clear, BOOL light, CGFloat minTint, NSUInteger budget);
/// 3.11.2: the Home channel on screen plays no video (团购, 经验 …), or nil.
FOUNDATION_EXPORT NSString * _Nullable ALB25MaterialNonVideoChannel(void);
/// Reads the realized glass of the top chrome's bar item inside `source`.
FOUNDATION_EXPORT void ALB25MaterialCaptureRegisteredGlass(UIView * _Nullable source);
/// Reads the bottom bar's realized platter glass (2.5.3's "bar-material"
/// source for the right column). Never changes the bar.
FOUNDATION_EXPORT void ALB25MaterialCaptureBarGlass(UITabBar * _Nullable bar);
/// 3.5 "right.glassSource": 0 the bottom bar's own glass (default), 1 the top
/// chrome's own glass, 2 UIKit's Regular recipe (3.2-3.4).
FOUNDATION_EXPORT NSInteger ALB25MaterialRegisteredGlassSource(void);
/// Puts a copy of the chosen glass into a glass button configuration's
/// private material. NO (configuration untouched) when none is read yet.
FOUNDATION_EXPORT BOOL ALB25MaterialInstallRegisteredGlass(UIButtonConfiguration * _Nullable configuration);
/// What ALB25MaterialInstallRegisteredGlass puts in right now (source and
/// glass); a button configured under another key is rebuilt by its owner.
FOUNDATION_EXPORT NSString *ALB25MaterialRegisteredGlassKey(void);
/// 3.10.10: the registered glass's key whatever "glass.lifted" is (E2 glass).
FOUNDATION_EXPORT NSString *ALB25MaterialRegisteredSourceKey(void);
/// Exactly what the registered right-column buttons get (with the Regular
/// recipe when that is chosen or nothing is read yet).
FOUNDATION_EXPORT BOOL ALB25MaterialInstallRegisteredEffect(UIButtonConfiguration * _Nullable configuration);
/// 诊断: what one glass is made of (class, size, style, glass description).
FOUNDATION_EXPORT NSString *ALB25MaterialGlassReport(UIView * _Nullable root, BOOL preferRound);
FOUNDATION_EXPORT NSString *ALB25MaterialBottomGlassReport(UITabBar * _Nullable bar);
FOUNDATION_EXPORT NSString *ALB25MaterialRegisteredGlassStatus(void);
FOUNDATION_EXPORT NSString *ALB25MaterialGlassRuntimeNames(void);
/// 3.6 诊断: the bottom bar's main capsule, the view holding the realized
/// glass inside `root`, and a full report of one glass (description,
/// sampling layers, ancestors that change what reaches it).
FOUNDATION_EXPORT UIView * _Nullable ALB25MaterialBottomPlatter(UITabBar * _Nullable bar);
FOUNDATION_EXPORT UIView * _Nullable ALB25MaterialGlassOwner(UIView * _Nullable root, BOOL preferRound);
FOUNDATION_EXPORT NSString *ALB25MaterialGlassDeepReport(UIView * _Nullable view);
NS_ASSUME_NONNULL_END
