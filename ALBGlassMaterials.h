#pragma once
#import "ALBUnifiedMaterial.h"
static UIVisualEffect *ALBClearGlassEffect(BOOL interactive) { return ALBMaterialMakeEffect(interactive); }
static void ALBRefreshClearGlass(UIVisualEffectView *view) { ALBMaterialApplyEffectView(view, NO); }
