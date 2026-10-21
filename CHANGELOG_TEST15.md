# TEST15 / TEST10 baseline

- 移除底栏自制逐帧收缩、拉伸变方风险路径及压住 UIKit 的禁用动作事务。
- 接入同一个 UIKit 控制器的原生 minimized 请求；加入签名验证、异常记录、真实状态观察及不假冒原生的失败处理。
- 头像只替换现存 UISearchTab 的图标；保留真实 native hit，修复辅助搜索位可能在 UITabBar 之外时的命中和手势租借。
- 设置新增两档原生内容样式；两档均采用系统原生位置，没有独立居中头像球模式。
- 顶栏两侧与中间共用 ALBTopNativeGroup / 透明集合页 / 原生 TabBar 材质路径；无 clear/tinted 近似。
- 保留 TEST10 底栏材质入口、auth47、右侧布局、滚动策略和下载模块；实际 SHA-256 审计随包提供。
- 更新 control、版本显示、诊断、推送分支、Actions 产物及预检；新增 BOOL ABI/几何观测测试。
