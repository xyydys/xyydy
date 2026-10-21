#!/bin/sh
# SDK-free declarations only. A pass is not an Apple SDK or full tweak build.
set -eu
cd "$(dirname "$0")/.."
for arch in arm64 arm64e; do
  "${CLANG:-clang}" -target "${arch}-apple-ios15.0" -fobjc-arc -fblocks \
    -fsyntax-only -Wall -Wextra -Werror -Wno-unused-function -Wno-unused-variable \
    -I Tests/Support Tests/Support/right_layout_declarations.m
  echo "$arch right header declaration-fixture syntax passed (no Apple SDK)."
done
