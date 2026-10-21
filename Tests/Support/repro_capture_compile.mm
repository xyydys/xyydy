// Compile the exact collector implementation as ObjC++, like generated Tweak.x.
// This fixture is syntax/type/availability checking against the real SDK only;
// it is not a UI, persistence, or touch-routing test.
#import "../../ALBReproCapture.h"

// Include the real settings implementation too: its capture helper reads the
// same UISheetPresentationController properties. Declare only plugin-owned
// dependencies; UIKit types and availability still come from the real SDK.
#import <objc/runtime.h>
UIVisualEffect *ALBClearGlassEffect(BOOL interactive);
void ALBRefreshClearGlass(UIVisualEffectView *view);
UIUserInterfaceStyle AWESystemInterfaceStyle(void);
BOOL ALB5Bool(NSString *key, BOOL fallback);
void ALB5ScheduleIdleCollapse(void);
BOOL ALBMaterialEnabled(void);
UIUserInterfaceStyle ALBMaterialStyle(UIView *view, BOOL feedEligible);
static BOOL gAWESettingsVisible;
#import "../../ALBSettingsGlass.h"
