# TEST28 FIX1：菜单编译与中心玻璃叠层修复

基线：已交付的 TEST28 完整工程。保留该版设置、评论、背景、动作面板、缩放及消息按钮等功能。本轮未调用网站搜索。

## 编译错误

用户截图中两处有效错误均在 ALBTopNativeIconButton 的菜单生命周期回调：SDK 要求重写后直接调用父类，但原实现通过 UIButton 的运行时 IMP 间接调用，编译器无法识别。现改为直接调用 super，保留原有菜单状态、generation 和关闭 completion 恢复顺序。没有关闭编译警告。

云端 top carrier 的 arm64/arm64e SDK 语法检查新增 `-Werror=objc-missing-super-calls`，在安装打包依赖前拦截同类问题。

## 玻璃叠层

旧结构在进度 1 时仍把 primaryGlass 和 searchGlass 完全重叠，其中 primary 不可交互。搜索玻璃按压变形时，下面的主玻璃会继续保持静态圆。截图中的灰圆与拉伸玻璃分离，和这个结构一致。由于 TEST28 编译尚未成功，不能把截图当作 TEST28 新材质路径已运行的证据。

FIX1 从动画创建时明确使用同一种 `UIGlassEffect(style: .clear)`，两侧 `tintColor=nil`、`isInteractive=true`。取消复制无法确认样式的内部 effect，避免把 Regular/色调参数带进新容器。原生展开底栏仍由 UIKit 管理；不重排其内部视图，不声称逐像素复用了系统私有材质。

搜索圆完整进入等高主胶囊后，搜索玻璃隐藏，主胶囊继续缩成唯一的交互圆。展开与中途反转沿同一几何条件恢复搜索，不做 effect 切换。因两者同高，搜索圆的圆心位于胶囊两端圆心连线内，隐藏搜索不会缩小外轮廓。

头像和最终点击按钮从第一帧就属于存活的主玻璃，头像以最终球心减当前胶囊原点定位；不在终点换父视图。未融合时搜索有独立透明按钮，隐藏后明确退出命中路径。图标、标题和选中胶囊的位移渐隐、0.32 秒时长、圆径减 20 的头像及轻微末端回弹保留。

## 验证边界

本机执行的源码/哈希/边界、布局保护、缩放数学和 shell 语法检查记录在 `validation/test28_fix1_results.json`。Swift 几何测试补充了被隐藏圆的轮廓覆盖和反向状态检查，留在云端执行。

本机没有 Apple SDK、Swift 编译器和 iPhone；没有完成修复版的 Xcode 编译或实际玻璃融合/按压测试。云端完整构建和真机渲染仍需验证，不能将源码检查视为构建成功。

## 推送

```bash
cd /var/mobile && unzip -o AwemeLiquidBar_2.0.10_TEST28_FIX1_project.zip -d AwemeLiquidBar_2.0.10_TEST28_FIX1 && cd AwemeLiquidBar_2.0.10_TEST28_FIX1 && chmod +x push_to_github.sh && ./push_to_github.sh
```

分支：`original-fixes-test28-fix1`。Actions 产物：`AwemeLiquidBar-2.0.10-TEST28-FIX1-Xcode27-rootless`。此工程未代为推送到 GitHub。
