ARCHS = arm64 arm64e
TARGET = iphone:clang:latest:15.0
THEOS_PACKAGE_SCHEME = rootless
FINALPACKAGE = 1
DEBUG = 0

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = AwemeLiquidBar
AwemeLiquidBar_FILES = ALBUnifiedMaterial.m ALB5BottomGeometry.m ALBTopNative/ALBTopNativeGroup.m Tweak.x AwemeAlbumMorphBar.swift ALBBottomSupport.swift CommentGlassDYKiller.xm MessageTopGlassV3.xm WCLGEdgeEffect.m WCLGBackdropLayer.m WCLGEdgeGradientTables.m ALBNativeCarrierState.m
AwemeLiquidBar_CFLAGS = -O2 -fvisibility=hidden -fobjc-arc -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable -Wno-nullability-completeness
AwemeLiquidBar_SWIFTFLAGS = -O -swift-version 5 -parse-as-library
AwemeLiquidBar_SWIFT_BRIDGING_HEADER = AwemeLiquidBar-Bridging-Header.h
AwemeLiquidBar_FRAMEWORKS = UIKit QuartzCore CoreGraphics CoreFoundation Foundation SwiftUI Photos MediaPlayer AVFoundation ImageIO CoreMedia CoreVideo AudioToolbox
# Preserve an address map for every architecture without changing optimization,
# stripping, authorization or the runtime material/animation implementation.
AwemeLiquidBar_LDFLAGS = -Wl,-x -Wl,-map,$(THEOS_OBJ_DIR)/AwemeLiquidBar-$(THEOS_CURRENT_ARCH).linkmap

include $(THEOS_MAKE_PATH)/tweak.mk
