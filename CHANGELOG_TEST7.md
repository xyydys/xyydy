# AwemeLiquidBar 2.0.7 TEST7

基于 TEST6 完整工程修复；材质参考本次上传的 `AwemeLiquidBar_1.0.58_auth47_PageFixedGlass_v9_project(1).zip`。不要求清空全部偏好。

## 手机推送到 Xcode 云端

ZIP 放到 `/var/mobile` 后执行：

```bash
cd /var/mobile && mkdir -p AwemeLiquidBar_2.0.7_TEST7 && unzip -oq AwemeLiquidBar_2.0.7_TEST7_project.zip -d /var/mobile/AwemeLiquidBar_2.0.7_TEST7 && bash /var/mobile/AwemeLiquidBar_2.0.7_TEST7/push_to_github.sh
```

分支 `original-fixes-test7`。Actions 产物 `AwemeLiquidBar-2.0.7-TEST7-Xcode27-rootless`。手机只解压和推送；云端继续使用既有 Xcode27 runner 和 Theos 构建规则。

## 本次修改

- **底栏收缩与缩放**：日志已证实作品切换抵达 Swift，保留该观察方式。去掉“必须找到两块 TabBarItemPlatterView”才能执行的条件，按原生材料分支及几何位置寻找左右底座，兼容旧工程记录的 `_UITabBarPlatterView`。找到左右底座后沿用同一搜索玻璃移到中心，左栏收进去；没有识别到独立搜索时恢复原生收缩尝试，并提供原生底栏整体缩小兜底，不再静默不动。底栏自己的 layoutSubviews 也恢复并重新应用当前几何，防止只布局底栏时被系统覆盖。
- **v9 材质上下文**：恢复 v9 透明 UICollectionViewController 页面结构与页面外观策略；取消 TEST6 从右侧按钮反复读取 Light trait 的路径。iOS26+ 顶栏和底栏均不额外安装默认/透明 appearance 背景，保留 UIKit 原生材料、透镜和交互。没有新建另一块圆球玻璃，也不添加微型取色探针。
- **评论隐藏闪回**：用户隐藏状态独立于布局变换，布局期间不恢复可见。过滤对应原生按钮和行容器的透明度更新；设置打开时隐藏仍有效。
- **头像缩小**：替换图使用原头像实际尺寸，取消固定44pt。头像、音乐及右侧按钮在布局时用 bounds/center，不在原生回弹变形期间写 frame；保留原生触摸路由和按压形变。
- **顶栏高度**：三个模块使用共同高度基线；两侧原生按钮的尺寸布局独立于按压变换，关闭中间分组时两侧也保持当前收缩状态。
- **设置窗口**：整体从2/3调整为0.82，文字、控件、玻璃一起放大；对应上传截图右边缘约835px，接近右侧图标左缘。抖音设置入口的全屏仍保留。

## 材质与收缩说明

上传的 v9 源码实际采用：首页 Dark 外观分支，其他页面跟随抖音外观；系统的 Clear/Tinted 选择仍由原生材料处理。“Dark 外观”不等于强制有色背景。TEST7恢复这份源码策略，不把它描述成实时读取视频亮度。顶栏目前是独立原生 UITabBar，底栏是 Photos 形式的 UITabBarController；未进行设备逐像素对照，不能承诺两者视觉完全相同。

首页/朋友页仍按作品变化驱动，不受“其他页面随滚动收缩”开关影响；已有“按停留时间收缩”设置继续保留。点击缩小区域展开。中心头像需要先访问“我”读取本人头像，未读到时保留搜索图标。

圆球及左右玻璃融合依赖设备真实原生层级。若未识别独立搜索底座，退回原生/整体缩小，诊断会明确记录兜底，不能把兜底称作中心圆球已成功。

## 验证与诊断

12个 Objective-C/C/C++ 单元通过实际arm64对象编译；Swift原文语法、C桥接、Theos参数检查通过。四组纯C测试覆盖滚动方向、右侧坐标、顶栏共同基线、底栏表面匹配与缩放中心。

没有本机 Apple Swift 类型检查、最终 dylib 链接或真机验证；完整构建交由既有 Xcode27 Actions。

正常诊断导出新增 `BOTTOM_GEOMETRY`（实际底座类、尺寸、配对/兜底）和包含 geometry 的 `COLLAPSE_STATE`。设置“布局与动画 → 诊断”也可查看。这次诊断输出包含 TEST7 版本，避免此前 SESSION_START 旧版本字符串误导。
