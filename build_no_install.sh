#!/bin/sh
# Build only; never installs to a phone or removes the Theos toolchain.
set -eu
cd "$(dirname "$0")"
: "${THEOS:=$HOME/theos}"
export THEOS
if [ ! -f "$THEOS/makefiles/common.mk" ]; then
    echo "未找到 Theos：请先设置 THEOS 为你的 Theos 目录。" >&2
    exit 2
fi
if command -v xcrun >/dev/null 2>&1; then
    sdk_version=$(xcrun --sdk iphoneos --show-sdk-version)
    sdk_major=${sdk_version%%.*}
    if [ "$sdk_major" -lt 26 ]; then
        echo "当前 iPhoneOS SDK 为 $sdk_version；需要 SDK 26 或更新。" >&2
        exit 2
    fi
fi
chmod 755 layout/DEBIAN/postinst scripts/preflight.sh
if command -v python3 >/dev/null 2>&1; then
    sh scripts/preflight.sh
else
    sh scripts/preflight_portable.sh
fi
make clean
make package -j"${JOBS:-1}" "$@"
if command -v python3 >/dev/null 2>&1; then
    python3 scripts/collect_build.py
else
    sh scripts/collect_build_portable.sh
fi
