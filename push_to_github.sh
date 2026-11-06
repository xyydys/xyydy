#!/bin/bash
# 3.11.53: makes the repository branch an exact copy of this project folder.
# Every tracked file that is not part of the project (the retired 2.x top
# and bottom sources, old gates, notes, tests) is removed from the branch.
set -euo pipefail
project_dir="$(cd "$(dirname "$0")" && pwd)"
repo_dir="/var/mobile/awemelg-import"
remote_url="git@github.com:xyydys/awemelg.git"
branch_name="main"
if [ ! -d "$repo_dir/.git" ]; then git clone "$remote_url" "$repo_dir"; fi
cd "$repo_dir"
if [ -n "$(git status --porcelain)" ]; then
  echo "仓库目录有未提交的改动，请先提交或备份：$repo_dir" >&2
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
  git checkout -b "$branch_name"
fi
if git show-ref --verify --quiet "refs/remotes/origin/$branch_name"; then
  git merge --ff-only "origin/$branch_name"
fi
# Remove tracked files that are no longer part of the project.
git ls-files -z | while IFS= read -r -d '' file; do
  if [ ! -e "$project_dir/$file" ]; then git rm -q --cached -- "$file"; rm -f -- "$file"; fi
done
# Copy every project file (no build products).
( cd "$project_dir" && find . -type f \
    ! -path './.git/*' ! -path './.theos/*' ! -path './packages/*' ! -path './out/*' \
    ! -name '*.deb' ! -name '.DS_Store' -print ) | while IFS= read -r file; do
  mkdir -p "$(dirname "$file")"
  cp "$project_dir/$file" "$file"
done
find . -type d -empty ! -path './.git/*' -delete 2>/dev/null || true
chmod +x build.sh build_no_install.sh scripts/preflight.py layout/DEBIAN/postinst push_to_github.sh
git add -A -- .
if ! git diff --cached --quiet; then
  git commit -m "3.11.53: always-hide status bar switch, clean screen status bar refresh via UIKit, plus 3.11.52"
fi
git push -u origin "$branch_name"
echo "推送完成：$branch_name。到 GitHub Actions 下载 AwemeLiquidBar-3.11.53-rootless。"
