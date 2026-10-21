#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")" && pwd)"
repo_dir="/var/mobile/awemelg-import"
remote_url="git@github.com:xyydys/awemelg.git"
branch_name="main"
if [ ! -d "$repo_dir/.git" ]; then git clone "$remote_url" "$repo_dir"; fi
cd "$repo_dir"
if [ -n "$(git status --porcelain)" ]; then
  echo "AwemeLiquidBar-Fixes-Git 有未提交的改动，请先提交或备份。" >&2
  exit 1
fi
if [ "$(git remote get-url origin)" != "$remote_url" ]; then
  echo "此目录的 origin 不是预期仓库，已停止。" >&2
  exit 1
fi
git fetch origin
if git show-ref --verify --quiet "refs/heads/$branch_name"; then
  git checkout "$branch_name"
elif git show-ref --verify --quiet "refs/remotes/origin/$branch_name"; then
  git checkout -b "$branch_name" "origin/$branch_name"
else
  git checkout -b "$branch_name" origin/main
fi
if git show-ref --verify --quiet "refs/remotes/origin/$branch_name"; then
  git merge --ff-only "origin/$branch_name"
fi
files=(
  CHANGELOG_TEST42_FIX6.md CHANGELOG_TEST42_FIX7.md CHANGELOG_TEST42_FIX8.md CHANGELOG_TEST42_FIX9.md CHANGELOG_2.2.md AUTH_UPDATE_2.2_auth50.txt ALBNativeCarrierState.m scripts/check_native_state.py
  CHANGELOG_TEST42_FIX5.md CHANGELOG_TEST42_FIX4.md CHANGELOG_TEST42_FIX3.md scripts/check_test43.py
  CHANGELOG_TEST42_FIX2.md
  CHANGELOG_TEST42_FIX1.md validation/test42_preservation.json validation/test42_fix1_results.json
  CHANGELOG_TEST42.md scripts/check_test42.py ALBTopMaterial.h Tests/right_music_anchor_test.c
  CHANGELOG_TEST38.md scripts/check_test38.py
  CHANGELOG_TEST37_FIX1.md validation/test37_fix1_results.json validation/test37_fix1_audit.json
  CHANGELOG_TEST37_REPAIR.md validation/test37_repair_results.json validation/test37_repair_audit.json scripts/check_test37.py
  CHANGELOG_TEST36_REPAIR.md validation/test36_repair_results.json validation/test36_repair_audit.json scripts/check_test36.py
  CHANGELOG_TEST35_REPAIR.md validation/test35_repair_results.json validation/test35_repair_audit.json scripts/check_test35.py
  CHANGELOG_TEST34_REPAIR.md validation/test34_repair_results.json validation/test34_repair_audit.json scripts/check_test34.py scripts/test_profile_scroll_preservation.py
  CHANGELOG_TEST33_REPAIR.md validation/test33_repair_results.json validation/test33_repair_audit.json scripts/check_test33.py
  CHANGELOG_TEST32_REPAIR.md validation/test32_repair_results.json validation/test32_repair_audit.json
  CHANGELOG_TEST31_SETTINGS.md validation/test31_settings_results.json validation/test31_settings_audit.json
  CHANGELOG_TEST30_CAPTURE_FIX1.md validation/test30_capture_fix1_results.json validation/test30_capture_fix1_audit.json
  ALBReproCapture.h ALBReproCaptureSignal.h REPRO_CAPTURE_GUIDE.md CHANGELOG_TEST30_CAPTURE.md
  scripts/check_repro_capture.py Tests/Support/repro_capture_compile.mm
  validation/test30_capture_results.json validation/test30_capture_audit.json
  CHANGELOG_TEST29.md validation/test29_results.json validation/test29_preservation_audit.json
  scripts/check_test29.py Tests/right_tail_anchor_test.c
  ALBTopNative/ALB23LensPolicy.h Tests/test23_regression_test.c
  scripts/check_test23.py scripts/check_test24.py scripts/test_top_lens_syntax.py
  CHANGELOG_TEST23.md DIAGNOSIS_TEST23.md validation/test23_preservation_audit.json validation/test23_results.json
  CHANGELOG_TEST24.md DIAGNOSIS_TEST24.md validation/test24_preservation_audit.json validation/test24_results.json
  CHANGELOG_TEST27.md validation/test27_results.json
  CHANGELOG_TEST28.md DIAGNOSIS_TEST28.md validation/test28_results.json validation/test28_preservation_audit.json
  CHANGELOG_TEST28_FIX1.md validation/test28_fix1_results.json
  scripts/test_bottom_scale_feedback.py ALBDanmakuGlass.h
  ALB22NativeEndpoint.h ALB22NativeMethodPolicy.h
  ALB8Profile.h ALBDownloadJob.h ALBDownloadState.h ALBNativeState.h
  ALB5Options.h ALB5BottomGeometry.m ALB7BottomMath.h ALB5Settings.h ALB5RightLayout.h
  ALBTopNative/ALBTopNativeGroup.h ALBTopNative/ALBTopNativeGroup.m ALBTopNative/ALBCompactGeometry.h ALBTopNative/ALBNativeBarSurface.h
  Tweak.x AwemeAlbumMorphBar.swift ALBBottomSupport.swift AwemeLiquidBar-Bridging-Header.h
  MessageTopGlassV3.xm CommentGlassDYKiller.xm
  ALBNativeInteraction.h ALBSettingsGlass.h ALBSidePalette.h ALBAlbumVideoExporter.h ALBScrollIntent.h ALBNativeMinimize.h ALBGlassMaterials.h ALBRootScroll.h ALBClearContrast.h ALBAvatarMenuTouch.h
  WCLGEdgeEffect.h WCLGEdgeEffect.m WCLGBackdropLayer.h WCLGBackdropLayer.m WCLGEdgeGradientTables.h WCLGEdgeGradientTables.m
  THIRD_PARTY_TEST5.md Makefile control AwemeLiquidBar.plist build.sh build_no_install.sh
  scripts/check_logos_boundaries.py scripts/test_logos_boundary.py scripts/preflight.sh scripts/check_native_contract.py scripts/check_primary_action_availability.py scripts/check_runtime_guards.py scripts/check_watchdog_guards.py scripts/check_right_persistence.py scripts/check_right_header_syntax.sh scripts/symbolicate_linkmap.py scripts/collect_build.py Tests/native_state_test.c Tests/scroll_intent_test.c Tests/right_layout_test.c Tests/right_persistence_test.c Tests/Support/right_layout_declarations.m Tests/Support/math.h Tests/bottom_geometry_test.c Tests/download_state_test.c ALBTopNative/Tests/window_geometry_test.c
  layout/DEBIAN/postinst .github/workflows/build.yml
  CHANGELOG_TEST22.md DIAGNOSIS_TEST22.md
  scripts/check_test22.py scripts/test_native_endpoint_syntax.py Tests/native_endpoint_policy_test.c validation/test22_preservation_audit.json validation/test22_results.json
  README.md CHANGELOG_TEST21.md DIAGNOSIS_TEST21.md CHANGELOG_TEST20.md DIAGNOSIS_TEST20.md CHANGELOG_TEST15.md DIAGNOSIS_TEST15.md CHANGELOG_TEST16.md DIAGNOSIS_TEST16.md CHANGELOG_TEST17.md DIAGNOSIS_TEST17.md CHANGELOG_TEST18.md DIAGNOSIS_TEST18.md CHANGELOG_TEST19.md DIAGNOSIS_TEST19.md VALIDATION.md REFERENCES.md push_to_github.sh
  validation/test21_preservation_audit.json validation/test20_preservation_audit.json validation/current_results.json validation/source_sha256.json validation/preservation_audit.json validation/wave_removal.json validation/test16_preservation_audit.json validation/test17_preservation_audit.json validation/test18_preservation_audit.json validation/test19_preservation_audit.json
)
# Retire only generated validation files known to the TEST9 baseline.
git rm --ignore-unmatch -- "validation/compiled_header_sha256.json"
git rm --ignore-unmatch -- "validation/compile_input_stability.json"
git rm --ignore-unmatch -- "validation/download_audit.json"
git rm --ignore-unmatch -- "validation/geometry_tests.json"
git rm --ignore-unmatch -- "validation/push_script.json"
git rm --ignore-unmatch -- "validation/compile_results.json"
git rm --ignore-unmatch -- "validation/swift_syntax.json"
git rm --ignore-unmatch -- "validation/scroll_intent.json"
git rm --ignore-unmatch -- "validation/material_audit.json"
git rm --ignore-unmatch -- "validation/feed_policy_result.json"
git rm --ignore-unmatch -- "validation/build_flags.json"
git rm --ignore-unmatch -- "validation/bridge_header.json"
for file in "${files[@]}"; do
  mkdir -p "$(dirname "$file")"
  cp "$project_dir/$file" "$file"
done
chmod +x build.sh build_no_install.sh scripts/preflight.sh layout/DEBIAN/postinst push_to_github.sh
# The TEST2 SwiftUI wrapper is superseded by the UIKit floating container.
if git ls-files --error-unmatch ALBSettingsPresentation.swift >/dev/null 2>&1; then
  git rm -- ALBSettingsPresentation.swift
fi
git rm -r --ignore-unmatch -- AudioMeter
git add -- "${files[@]}"
if ! git diff --cached --quiet; then
  git commit -m "2.0.10 TEST42 FIX2: remove misplaced nested Swift background method"
fi
git push -u origin "$branch_name"
echo "推送完成：original-fixes-test42-fix2。到 Actions 下载 AwemeLiquidBar-2.0.10-TEST42-FIX2-Xcode27-rootless。"
