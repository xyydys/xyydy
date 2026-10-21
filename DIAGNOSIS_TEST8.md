# TEST8 修复依据

| 问题 | 源码路径 | 处理 |
|---|---|---|
| 底栏反复缩小 | ALB5BottomGeometry.m / Swift 布局回调 / UITabBar hook | 原始变换保持独立；嵌套布局只在最外层恢复和应用 |
| 隐藏后补位闪烁、头像间距错误 | ALB5RightLayout.h | 角色结构缓存与可见性分离；图标中心间距；原生属性写入前恢复布局 |
| 顶栏材料不同 | ALBTopNativeGroup.m | 控制器托管原生 tab 和透明 collection 内容，禁止写入自定义材料 appearance |
| 两侧按钮各自收缩 | ALB8TopModuleHost | 单一父容器、44pt 统一基准，不更改子按钮按压变换 |
| 面板挡滑动/操作旧作品 | ALBSidePalette.h | 空白区域穿透、作品提交后更新、动作前核对可见作品 |
| 自动变色慢 | ALBClearContrast.h | 逐行迟滞判定，一次采样供所有行使用，滚出第一行不影响剩余行 |
| 清屏无法退出 | ALBAvatarMenuTouch.h | 原头像位置的独立菜单按钮，不检查已被清屏隐藏的原生祖先 |
| 取消下载无入口 | ALBDownloadJob.h / Tweak.x | 提示绑定任务、真实取消、独占目录、状态机隔离完成回调 |

设备专属私有层级仍可能随抖音版本变化。没有日志或实机结果时，不能把源码检查描述为真机结论。
