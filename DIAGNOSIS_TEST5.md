# TEST5 定位与修改

TEST4 首页/朋友页同时存在滚动驱动连接及窗口/原生 pan 观察。本版将这两页排除，改用 currentIndexWillChange/currentIndexDidChange 回调及作品 ID 去重。真实设备是否还有其他卡顿源尚未测量。

底栏默认背景 appearance 与首页强制 Dark 是已定位到的额外色调来源；本版改为透明 appearance 与系统风格。显式自定义深色仍保留。

设置悬浮容器的周期背景取样/前景刷新已移除；本版没有帧率测量结果。

旧代码未整体重写，授权、下载、右侧已生效的触摸基础保留。新代码分为 ALB5Options、ALB5BottomGeometry、ALB5RightLayout、ALB5Settings、ALBTopNative 与 AudioMeter。
