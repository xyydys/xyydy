# 修改定位

- `Tweak.x`：滚动登记、收缩开关及事件连接；原生分组控件；设置布局与玻璃按钮恢复；双击模式接入。现有业务动作、下载函数和 auth47 授权判定沿用。
- `AwemeAlbumMorphBar.swift`：恢复 `.onScrollDown` 和 content scroll view 绑定；去除手动整栏缩放；保留可见性遮罩及用户垂直偏移；增加收缩后的几何日志。
- `ALBNativeInteraction.h`：原动作 sender 转发、原手势层级内的真实触摸、配置图像层中的头像／唱片图像、可恢复的原图像显示状态。
- `MessageTopGlassV3.xm`：启用原有原生按钮代理路径；系统外观刷新；移除运行路径中的模拟高亮长按识别器。
- `ALBSettingsGlass.h`：共用玻璃背景、分区配色、原生 sheet detent 和导航栏设置。
- `ALBSidePalette.h`：触发点定位、两侧布局、原生玻璃圆钮与胶囊、音量／亮度连续填充及触感、业务动作连接。
- `ALBAlbumVideoExporter.h`：图集逐张下载与实际视频编码、结果保存。
- `Makefile`：增加 MediaPlayer、AVFoundation、ImageIO、CoreMedia、CoreVideo 框架。
- `build_no_install.sh` / `scripts/collect_build.py`：SDK 检查，构建后从对应版本的 deb 中提取准确的 dylib；保留已有 THEOS 路径。
- `scripts/preflight.sh`：旧的“设置必须不透明、右侧必须使用手势租借”的锁与本次要求冲突，已替换成源文件、版本及打包完整性检查，完整编译仍由 Theos 执行。

收缩时不重写 tab bar 的宽高、不用统一缩放冒充原生小球。公开滚动接口是主路径；保留少量已存在的 UIKit tab-bar 专用运行时收缩 selector 作为兼容请求。日志把请求是否被接收与实际控件几何分开记录，仍需要设备验证。

顶部分组使用系统分段控件的原生选择行为；它不是从截图量出私有透镜参数后复刻的自绘组件。双击面板按给定截图比例独立实现，当前版本没有宣称像素级复刻成功。
