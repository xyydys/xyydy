# 检查记录

完成：

- build.sh、build_no_install.sh、scripts/static_check.sh 的 sh 语法检查。
- 公开 UIGlassEffect 工厂与属性的运行时调用路径复核。
- Regular / Clear 路径、Clear 去色调与控件禁用、返回 Regular 保留参数的源码复核。
- 跟随模式屏幕来源、面板范围外观覆盖、重入保护、返回前台刷新逻辑复核。
- 原生类不存在时清楚标记普通模糊。
- 新色调选项的旧配置索引迁移；保存最终滑杆值。
- 构建仅 clean / package -j1；没有安装、重启或联网取 SDK 步骤，没有 Python 依赖。
- 只注入 com.ss.iphone.ugc.Aweme；不存在抖音视图 / 布局 hook。
- 现场记录是用户手动触发，JSON 不将接口成功伪装为画面验证成功。
- ZIP 完整性和参考清单哈希核验。

未完成：

当前环境没有 Theos、iOS SDK、clang 或 iOS 设备，因此未生成 deb / dylib，未进行真实 Objective-C 编译、抖音注入、按钮点击或画面验证。

手机上需要已有 Theos + clang + make；build.sh 会进行无 Python 的源码路径检查，然后构建。运行后以面板实际材质状态和 README 中的对照步骤验证，不把本检查文件视为真机成功证明。
