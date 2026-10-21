#!/bin/sh
# Phone fallback: packaging/source presence only; the compiler remains authoritative.
set -eu
cd "$(dirname "$0")/.."
alb_package=$(awk -F ': ' '$1=="Package" {print $2}' control)
alb_version=$(awk -F ': ' '$1=="Version" {print $2}' control)
[ "$alb_package" = com.xyy.awemeliquidbar ] && [ "$alb_version" = 2.3 ] || exit 2
alb_sources=$(awk -F ' = ' '$1=="AwemeLiquidBar_FILES" {print $2}' Makefile)
for alb_source in $alb_sources ALBUnifiedMaterial.h ALBMaterialPolicy.h AwemeLiquidBar-Bridging-Header.h; do
    [ -s "$alb_source" ] || { echo "缺少源文件：$alb_source" >&2; exit 2; }
done
echo "基础打包检查通过；未安装 Python，完整源码测试留待开发环境运行。"
