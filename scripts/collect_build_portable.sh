#!/bin/sh
# Extract a package as data only. No dpkg install, uicache or process restart.
set -eu
cd "$(dirname "$0")/.."
alb_package=$(ls -t packages/*.deb | head -n 1)
[ -f "$alb_package" ] || { echo "没有生成 deb" >&2; exit 2; }
mkdir -p out
cp "$alb_package" out/
if command -v dpkg-deb >/dev/null 2>&1; then
    alb_stage=$(mktemp -d "${TMPDIR:-/tmp}/alb-package.XXXXXX")
    trap 'rm -rf "$alb_stage"' EXIT HUP INT TERM
    dpkg-deb --extract "$alb_package" "$alb_stage"
    alb_binary=$(find "$alb_stage" -type f -name AwemeLiquidBar.dylib | head -n 1)
    [ -f "$alb_binary" ] || { echo "deb 中没有 AwemeLiquidBar.dylib" >&2; exit 2; }
    cp "$alb_binary" out/AwemeLiquidBar.dylib
else
    echo "deb 已生成；缺少 dpkg-deb，不能单独导出 dylib。" >&2
fi
echo "已生成：$alb_package（out 中保留副本）；仅编译打包。"
