# AwemeLiquidBar 2.3 · auth51 · MATERIAL2

以原始 2.3 auth50 工程包重建。沿用 2.3 的材质、顶栏、底栏、按钮和收缩实现；本次增加一条普通授权，将一条已有普通授权升级为特殊底栏兼容权限，保留原有两条特殊权限。现在共 51 条普通授权记录，其中 3 条同时拥有特殊底栏权限。

特殊用户入口：LiquidBar → 底栏 → **显示液态底栏**。关闭后移除本插件底栏、搜索触摸代理及底栏监听，恢复保存的抖音原生底栏背景、子视图和交互，停止继续清除原生底栏背景，供其它底栏插件接管。重新打开会从原生宿主重建本插件底栏。普通用户沿用原有底栏行为，此开关仅对特殊用户显示。

原有绑定凭据与开关设置继续使用。安装后重新打开抖音和设置面板即可读取新增权限。授权摘要、已有授权、两条旧特殊权限及回退范围的校验见 `validation/auth51_changes.json`；当前验证说明见 [VALIDATION_2.3_auth51.txt](VALIDATION_2.3_auth51.txt)。

本工程以附件中的 2.2 SHARED_MATERIAL1 为基线。统一插件所创建的液态玻璃材质、外观来源及设置，并修复原生收缩请求与动画完成之间的重排问题。先前测试材质配方/结论与本次原始截图保留在 docs/reference。完整分析、适配边界与真机检查方法见 [MATERIAL2_说明.md](MATERIAL2_说明.md)。

设置入口：LiquidBar → 底栏 → **统一液态玻璃 · 材质与深浅色**。

- iOS 27：Regular / 无色调 Clear；深浅色来源与 Regular 色调选择。
- iOS 26：默认保留现有底栏，尝试同步可读取的原生渲染配方；兼容回退会写入实际状态。
- 首页 / 朋友视频流默认保持深色，商城、网格、消息、个人页和设置/评论面板按所选来源处理。
- 保留原按钮、菜单、目标与事件、原生选择透镜；材质同步不重建按钮或迁移触摸对象。

将 ZIP 放到 `/var/mobile` 后，一行编译：

```sh
cd /var/mobile && unzip -o AwemeLiquidBar_2.3_auth51_MATERIAL2_project.zip -d AwemeLiquidBar_2.3_auth51 && cd AwemeLiquidBar_2.3_auth51 && chmod +x build.sh && ./build.sh
```

需要现有 Theos 与 SDK 26 或更新。默认单任务编译，执行 `make clean`、`make package -j1`；仅编译打包。没有 Python 的手机使用基础检查与 deb 提取脚本，产物放在 `packages/` 和 `out/`。

已通过源码门禁、材质策略 C 测试和可运行的回归测试。本环境无 Swift、Clang、Apple SDK 或真机，未完成 iOS 编译、Swift 调度测试或实际外观/菜单动画验证。CI 保留真实 iPhone SDK 类型检查与完整构建。
