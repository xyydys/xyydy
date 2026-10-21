# TEST19 — 匹配实际二进制后的右侧布局诊断

## 1. 匹配对象与证据边界

输入为 `Aweme-2026-09-25-062238.ips` 与用户上传的实际 Actions 产物 `AwemeLiquidBar-2.0.10-TEST18-Xcode27-rootless.zip`。不是用源码 ZIP 文件名猜测版本。

- 崩溃映像与 Mach-O 的 arm64e LC_UUID 均为 `AB32436E-767A-3656-B1D7-7647E6EE2544`。
- 产物中的 dylib SHA-256 为 `29c7882a5de3d4458cbf78998e97063447517f361e5ff72c91f8e935ae1a8f06`，与 `build_identity.json` 相同。
- 使用同一产物的 `01_AwemeLiquidBar-arm64e.linkmap`，没有使用 arm64 的地址表。
- 构建记录版本为 `2.0.10-test18`，提交为 `be1e9023b46b0feaceb64c5696ad92757517ba31`。
- 当前源码 ZIP 中存在的 34 项构建清单文件均哈希一致，包括 Makefile 编译清单的全部 9 个源文件。构建清单还有 9 项不在源码 ZIP 内，未计入上述一致性数量。

Link map 给出链接后的函数地址范围，不含完整 dSYM 的源代码行信息，也没有独立 UUID。这里以同包产物的出处、二进制 UUID、清单和源码哈希联合核对。未把这张地址表用于此前 UUID 为 `188cc59b…` 的旧日志。

## 2. 四个插件地址均已映射

| 崩溃 imageOffset | 函数起点 | 函数内偏移 | 映射结果 |
|---|---|---|---|
| `0x16F8C` | `0x16F50` | `60` / `0x3C` | `ALBIsOwnedView` |
| `0x780AC` | `0x770B0` | `4092` / `0xFFC` | `ALB5ApplyRightLayout` |
| `0x6FAD4` | `0x6FA30` | `164` / `0xA4` | `ALB6EndRightGeometry` |
| `0x4EEB8` | `0x4EC14` | `676` / `0x2A4` | Logos 的 `AWEElementStackView.layoutSubviews` hook |

四项均落在对象文件 `[17] Tweak.x…o` 的有效符号范围内。右侧布局头文件由 `Tweak.x` 包含，所以其静态函数也属于这个对象文件。

调用方向为：

```text
Core Animation 布局 / 事务提交
  → AWEElementStackView.layoutSubviews（插件 hook）
    → ALB6EndRightGeometry
      → ALB5ApplyRightLayout
        → ALBIsOwnedView
          → objc_getAssociatedObject / Objective-C 关联对象表查找
```

终止仍为 `FRONTBOARD / 0x8BADF00D / scene-update`，允许时间 10.00 秒；日志记录应用 CPU 时间为 10.458 秒。采样证明终止时主线程正在右侧布局路径，不证明这 10 秒全在同一个函数，也不证明关联对象表发生死锁。

## 3. 源码中确认存在的问题

TEST18 的 `ALB6BeginRightGeometry` 在每次最外层布局前调用 `ALB5RestoreRightLayout`，将所有已修改行的 transform 写回原生值。

原生 `layoutSubviews` 执行后，`ALB6EndRightGeometry` 再调用 `ALB5ApplyRightLayout`，后者再次还原，并重新计算、写入缩放和移位。

因此，即使原生位置、设置和最后视觉结果完全不变，非单位缩放仍会产生：

```text
原生矩阵 → 插件矩阵 → 下一轮原生矩阵 → 下一轮插件矩阵 ……
```

此外，`ALB10RightLocalLayout` 连单个点赞按钮的局部布局也包进了还原/重投影流程；原生重复设置相同 frame/bounds/center 时也会进入此流程。TEST18 的重入锁只能阻止同一调用栈里的嵌套进入，无法阻止函数返回后的下一轮布局。

这是从源码确认的重复几何写入路径，也是与本次已符号化调用链相符的主要修复目标；单份看门狗快照仍不足以断言它是应用所有卡顿的唯一原因。Apple 的 Auto Layout Guide 也指出，在布局阶段再次使布局失效可能形成反馈循环。

## 4. TEST19 的处理

### 持久保留显示变换

`ALB6BeginRightGeometry` 只记录嵌套深度，不再还原矩阵，也不再用禁止动画的事务包住宿主布局。普通按钮的局部布局直接执行原逻辑，不再触发外层变换重置。

`ALB5ApplyRightLayout` 复用现有变换记录，比较目标矩阵与 CALayer 当前模型矩阵。相同结果不调用 setter。只有设置回到原样、功能关闭、对象解绑或角色移除时才还原对应对象，不再每轮全量还原。

### 读取坐标不需要改动视图

新增 `ALB19RightConvertPoint/Rect`：仍由 UIKit 完成实际坐标转换，再仅对插件自身的投影做数学逆变换；目标坐标系也有插件投影时，进行对应换算。这样可在已缩放界面中读取原生坐标，不临时移动或缩放视图。

头像、四个操作按钮及音乐底座的定位读取同步改用这套方法，避免把已经缩放的坐标再次作为原生输入，造成累积偏移。

### 实际几何变化与原生事件保留

对已有变换记录的原生行，相同 frame/bounds/center/transform 写入不重复触发还原流程；真正变化仍交给原生 setter 后更新投影。没有关闭点赞、评论、收藏、分享、头像菜单或音乐按钮。

场景非活跃时暂停右侧插件附加重排。`AWEElementStackView.layoutSubviews` 的原始实现仍执行；重入时也保留原始调用。没有新增计时器、逐帧动画或定时布局重试。

## 5. 保留项与未解决项

以下文件与 TEST18 逐字相同：底栏材质 `ALB5Options.h`、顶栏原生载体及表面实现、底栏 `AwemeAlbumMorphBar.swift`、`ALB5BottomGeometry.m`、原生收缩入口 `ALBNativeMinimize.h`、共享材质、设置参数及评论模块等。完整清单见 `validation/test19_preservation_audit.json`。

`ALBNativeInteraction.h` 只修改四处几何辅助调用。将这四处逆向还原后，该文件 SHA-256 与 TEST18 完全相同，证明原生触摸转发、玻璃配置和图片配置等其余实现未改动。授权核心也通过独立哈希检查。

本次不宣称底栏无法收缩、顶栏左按钮延迟出现或两套独立原生收缩外形已解决。没有拿另一种玻璃材质替换已确认正确的材质。

## 6. 已完成的本地验证

- 全部 7 组 C 测试通过，其中 6 组为既有测试，新增 1 组右侧持久投影测试。
- 新增模型测试包含 130 万次稳定输入循环、64 种隐藏组合及跨投影坐标换算。相同输入首次应用后不继续写矩阵；旧还原/重套模式的负对照在 1,000 轮中产生 2,000 次写入。
- 完整右侧头文件通过 arm64 / arm64e、iOS 15 目标的声明桩语法检查。声明桩不是 Apple SDK，不包含 UIKit 实现。
- Swift 语法解析、既有 8 组模拟队列测试、API 可用性局部检查、源码保留检查、Python/Shell 语法检查通过。

没有完整 Xcode/Apple SDK 构建、UIKit 集成运行或真机视觉验收。本地模型验证不能替代真实宿主布局。工程不包含重新标名的 TEST18 dylib，也不包含已编译的 TEST19 deb/dylib。

## 7. 复现地址定位

项目附带的脚本直接读取 Mach-O UUID 并拒绝不匹配的崩溃映像：

```bash
python3 scripts/symbolicate_linkmap.py Aweme-2026-09-25-062238.ips AwemeLiquidBar.dylib 01_AwemeLiquidBar-arm64e.linkmap
```

参考：Apple Auto Layout Guide, “Changing Constraints / Custom Layouts”：
https://developer.apple.com/library/archive/documentation/UserExperience/Conceptual/AutolayoutPG/ModifyingConstraints.html
