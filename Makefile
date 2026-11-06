ARCHS = arm64 arm64e
TARGET = iphone:clang:latest:15.0
THEOS_PACKAGE_SCHEME = rootless
FINALPACKAGE = 1
DEBUG = 0

include $(THEOS)/makefiles/common.mk

TWEAK_NAME = AwemeLiquidBar
AwemeLiquidBar_FILES = Tweak.x ALBDockHooks.x ALBDock.m ALBDockBridge.m ALBDockMaterial.m ALBDockAvatar.m ALB3Chrome.swift AwemeAlbumMorphBar.swift ALBBottomSupport.swift ALBUnifiedMaterial.m ALB25Material.m ALB5BottomGeometry.m ALBNativeCarrierState.m CommentGlass.xm MessageTopGlassV3.xm ALBAmbience.xm ALBSheetGlass.xm WCLGEdgeEffect.m WCLGBackdropLayer.m WCLGEdgeGradientTables.m
AwemeLiquidBar_CFLAGS = -O2 -fvisibility=hidden -fobjc-arc -Wno-deprecated-declarations -Wno-unused-function -Wno-unused-variable -Wno-nullability-completeness
AwemeLiquidBar_SWIFTFLAGS = -O -swift-version 5 -parse-as-library
AwemeLiquidBar_SWIFT_BRIDGING_HEADER = AwemeLiquidBar-Bridging-Header.h
AwemeLiquidBar_FRAMEWORKS = UIKit QuartzCore CoreGraphics CoreFoundation Foundation Photos PhotosUI MediaPlayer AVFoundation ImageIO CoreMedia CoreVideo AudioToolbox
# An address map for every architecture, for symbolicating device logs.
AwemeLiquidBar_LDFLAGS = -Wl,-x -Wl,-map,$(THEOS_OBJ_DIR)/AwemeLiquidBar-$(THEOS_CURRENT_ARCH).linkmap

# ---- Obfuscation (Arkari, a modern OLLVM fork) ---------------------------
# Engaged only when ALB_OBFUSCATE=1 AND the C compiler is the Arkari clang
# from ghcr.io/xyydys/arkari-ios-toolchain. The -irobf* options are Arkari
# driver flags; Apple's clang would reject them, so the stock macOS build
# (build.yml never sets ALB_OBFUSCATE) is left byte-for-byte unchanged.
#
# Chosen scope — a balance between protection and the fact that this is a
# per-frame UI tweak running inside Douyin:
#   -irobf        turn the obfuscation framework on (no pass by itself)
#   -irobf-cse    C string literal encryption
#   -irobf-cie    integer constant encryption
#   -irobf-indgv  indirect global-variable references (hide addresses)
#   -irobf-indbr  indirect branches (encrypted jump targets)
# Deliberately left OFF (the two most expensive passes, costly on hot UI
# paths); opt in with ALB_OBFUSCATE_HEAVY=1 when protection matters more
# than a few frames:
#   -irobf-fla    control-flow flattening
#   -irobf-icall  indirect calls
# The three Swift sources are compiled by the Swift toolchain, not Arkari
# clang, so they are not IR-obfuscated.
ifeq ($(ALB_OBFUSCATE),1)
AwemeLiquidBar_CFLAGS += -irobf -irobf-cse -irobf-cie -irobf-indgv -irobf-indbr
ifeq ($(ALB_OBFUSCATE_HEAVY),1)
AwemeLiquidBar_CFLAGS += -irobf-fla -irobf-icall
endif
endif

include $(THEOS_MAKE_PATH)/tweak.mk
