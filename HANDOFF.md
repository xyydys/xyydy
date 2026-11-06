# AwemeLiquidBar 交接说明（截至 3.11.53）

写给接手的人（或下一个对话）。读完这一份就能接着做，不需要翻之前的对话。

---

> 换一个 AI 接手：先读 `AI_HANDOFF.md`（精简的当前状态和已排除方案），本文件是逐版本的详细记录。

## 0. 现状一句话

- 最新代码：**3.11.53**（常开开关“播放 → 隐藏状态栏” `statusBar.hide`；清屏刷新状态栏走 UIKit 自己的实现）。
- 3.11.52（清屏状态栏多路藏＋窗口诊断；头像“不展开菜单”每次点都进主页；点赞特效删兼容模式；默认最高画质；快捷跳转补子页面）。
- 3.11.51（设置精简：退役开关固定成用户现在的值，三段式底栏只剩“深色色调” `dock.darkTint`、玻璃固定深色只让图标随背后亮度切；右下角音乐补发点按 `ALBFaceTapOnTouchUp`；快捷跳转用页面自己的初始化 `ALBQJMakePage`、新建结果单独存；含 3.11.50 的一键清屏）。
- 3.11.50（没编译过，并入 3.11.51：一键清屏：插件顶栏原地时也跟着藏 `AWEStableCleanSyncTopChrome`；状态栏补钩新出现的页面类 `AWEStableCleanCheckStatusBar`；诊断【一键清屏】）。
- 3.11.49（玻璃透明度分开：顶栏 `top.glassAlpha`、三段式底栏 `dock.glassAlpha`，主页统一滑块删、展开菜单不跟；点赞特效按作品 ID 认；快捷跳转新建只抄配置变量；三段式底栏图标渐变；头像展开菜单开关 `avatar.menu`；右下角音乐放行原生点按＋按音乐 ID 兜底；第0.51节；未实机验证）。3.11.48 用户实测：主页玻璃透明度不知道管哪；展开菜单不要再跟透明度；点赞特效识别不到（双击 / 三击 / 双击面板 / 新面板）；快捷跳转重启后三页空白；底栏图标变色生硬；要头像菜单开关；右下角音乐点了没反应（很久了）。
- 3.11.48（整理版：设置分类、名字简化、说明删掉；清透开关＋黑色染色合成“玻璃透明度”滑块 `glass.alpha`；↑ 改成悬浮面板 mode 1；单双列是左上角菜单里侧边栏下面两项；团购外层列表也补到底；Tweak.x 开头的逐版本说明换成文件分工表；评论区文件改名 `CommentGlass.xm`；参考插件的名字全部去掉；第0.50节；未实机验证）。3.11.47 用户实测：别的问题没了；单双列图块还是不要，放菜单里当普通项；↑ 系统菜单迟钝（诊断：按下 2 次、弹出 0 次、0.8 秒后改自绘 2 次）；团购还有问题。
- 3.11.47（评论区一整块玻璃＋输入框系统模糊＋键盘弹起输入区薄模糊、纯透明并进色调、圆角滑块删；评论长按面板格子自己选中；↑ 系统菜单（UIButton.menu，预览＝整块胶囊，0.8 秒没弹出自动换自绘）＋自绘卡片 tag 冲突修；角标照抄系统角标规格；红点绑真实未读；展开＝收缩倒放；悬浮面板“外观”（相册壁纸、float.tint、float.clarity、每行几个）、白字、卡片 0.40、齿轮进设置整页；团购信息流 setFrame 里补到页底；点赞特效只认视频点赞键 / 真双击、一次手势一次；单双列图块＋蓝色对勾；别人主页诊断多钩两个类；第0.49节；未实机验证）。3.11.46 用户实测：评论玻璃和输入区分成两块有拼缝、纯透明没法读；↑ 自绘菜单叠行；调色盘选不了相册；团购底部还在；点赞特效进收藏页狂放；角标乱跳、红点没绑信息、展开不是收缩倒放；单双列选中块难看；评论长按面板按钮点不了。
- 3.11.46（✦ 悬浮面板 `ALBFloatPanel.h`；↑ 菜单自己画 `ALBDockQuickMenu`；双击面板按下就关 `ALBOutsidePress.h`；底栏格子宽固定＋居中；评论输入区按参考做法 做成玻璃＋键盘弹起圆角；评论分享面板选人后的白垫；提示条 Clear＋字随背后亮暗；第0.48节；未实机验证）。3.11.45 用户实测：快捷跳转不生效（诊断：菜单 0 次打开）；双击面板只能上滑关；评论输入区不透明 / 磨砂有问题；底栏藏项后变长、不居中；评论分享面板选人后中间半透明；提示条老是切深浅。
- 3.11.45（三段式底栏项目可设＋✦ 双击面板＋↑ 快捷跳转菜单、玻璃换深浅不再变不透明、下标修正；按参考做法 保存最高画质视频和实况；双击面板清屏 / 点外面关 / 上下滑关 / 分区打开；评论区色调用 Clear、输入区背景三选一、作品不缩按参考做法 重做；评论长按面板锁浅色；分享面板外壳也同步＋诊断；左上角单双列；全屏不碰聊天页、搜索进来的作品页照常全屏；别人主页 / 团购 / 收藏 / 快捷跳转专项诊断，第0.47节；未实机验证）。3.11.44 用户实测：底栏右两块切深浅后不透明、图标颜色迟钝；评论长按面板要锁浅色；评论区色调不生效、输入区和键盘区要不透明或模糊；分享面板还不是玻璃；作品不缩完全失效；团购 / 收藏底部、单双列入口、全屏影响聊天页 / 搜索没覆盖。
- 3.11.44（聊天页玻璃全部撤回；分享面板、评论长按面板、评论区面板深浅跟系统，评论区加色调浓度；消息页顶部按钮改系统 Regular＋系统深浅；三段式底栏右两块和系统胶囊同一份玻璃＋诊断，第0.46节；未实机验证）。3.11.43 用户实测：聊天页顶底背景没清掉（要求整个撤回）；评论长按面板深色模式下是浅色玻璃；评论区浅色模式下也是深色；分享面板颜色不一致；底栏右两块动不动变深；消息页顶部三颗每次由深变浅。
- 3.11.43（`ALBMoreGlass.h` 按参考做法 的几何重做、玻璃 Regular 定死深浅、面板玻璃挂进面板自己的底里，第0.45节；未实机验证）。3.11.42 用户实测：太乱；面板拖动时玻璃停在原地；分享面板按钮白变黑；加号面板格子不一致；要求按参考做法。
- 3.11.42（剩下的区域一次做完：聊天顶部标题、表情面板、评论长按面板、分享面板、图文详情页，`ALBMoreGlass.h`；胶囊改成按说明装 `ALBPillSpec`，修 3.11.41 胶囊第二次布局画空；`ALBSheetGlass.xm` 认评论长按 / 分享面板，第0.44节；未实机验证）。3.11.41 用户没测，直接要求一次做完。
- 3.11.41（聊天页铺开：胶囊玻璃按钮 `ALBChatPillButton`、输入框底换玻璃、加号面板 face、返回箭头居中，第0.43节；未实机验证）。3.11.40 用户实测：有效（按钮跟着变形）；返回键图标偏。
- 3.11.40（聊天页按钮改用右侧的系统栏按钮 face `ALBChatFace.h`，删掉 `ALBBindGlass.xm`；三段式底栏头像优先读设置页那张，第0.42节；未实机验证）。3.11.39 用户实测：还是不变形，要求直接用头像 / 音乐那套。
- 3.11.39（`ALBBindGlass.xm` 重写：按压基准比对＋插件自己按压保底、深浅跟页面、挨着的按钮合成胶囊，第0.41节；未实机验证）。3.11.38 用户实测：模块圆角问题解决；聊天输入栏图标不跟随、玻璃一进来是浅的、右边三颗太挤。
- 3.11.38（长按面板模块圆角改成提交前补上；新 `ALBBindGlass.xm`：聊天输入栏的“真玻璃”试点，第0.40节；未实机验证）。3.11.37 用户实测：采集好用，补抓了第 8–16 段；模块圆角离屏回来仍先旧后新。
- 3.11.37（只改探针采集：先问系统命中测试、加“拆开”、记按钮外的文字 / YYLabel / 页面标题 / 深一层跳转字符串；对按参考做法 源码列出了要补抓的区域，第0.39节；未实机验证）。3.11.36 用户实测：采集能用了，抓回 7 段。
- 3.11.36（修 3.11.35 采集模式闪退；模块圆角在 setPath 时当场换，第0.38节）。3.11.35 用户实测：一选“采集”就闪退、之后每次启动都闪退；模块圆角离屏回来要再变一次。
- 3.11.35（长按面板模块圆角按 CAShapeLayer 路径改；探针加“采集”模式 `ALBProbeCapture.h`，第0.37节；未实机验证）。3.11.34 用户实测：长按面板玻璃成功。
- 3.11.34（长按面板背景换成评论区同款玻璃 `ALBSheetGlass.xm`，第0.36节；未实机验证，3.11.33 也还没实测）。
- 3.11.33（三段式底栏用液态底栏的图标/标题/强调色、视频页按背后亮度探针切深浅；并入点赞特效 `ALBAmbience.xm`＋动画时长，第0.35节；未实机验证）。用户对 3.11.32 的结论：右侧和面板“就这样吧”（先深后浅的问题收尾）。
- 3.11.32：封面估值按位置学到的压暗比例校准、放宽找封面；`dock.followEdge` 改默认关，第0.34节。3.11.31 已实测：先深后浅的视频少了、基本不影响；底栏登记无效且失去清透（见 0.34）。
- 3.11.24：新玻璃直接到位（第0.26节）。上一版 3.11.23 用户实测：没有变化（诊断：右侧加速236、面板加速2480，但右侧“完成后保留”的39个没处理；面板全部加速仍先深→深色来自读数到达前，不是动画）。
- 3.11.23 的记录：**3.11.23**（原生色调动画验证版，第0.25节）。3.11.22共存已被用户拒绝，撤回。3.11.21 用户已确认关闭面板后右侧不再变深；3.11.20 已收到实机日志。基线 **3.11.19**（整理版，第 0.21 节），iOS 26.1 SDK 头文件下做过语法检查（这一版没改 Swift），**还没在手机上跑过**。上一次实测是 3.11.18（用户：不行，要求删掉并整理）。
- 历史 **3.11.0** 实测记录（第 0.3 节）：开了“清透玻璃”后三段式底栏、右侧、面板都正常（用户原话“错怪你了，清透开了之后正常”——3.11.0 默认是常规玻璃＝那个深色调；参考工程里用户开着清透）。剩下的问题和 3.11.1 的改法见第 0.3 节。
- 3.11.0 按用户要求做了三件事（第 0.1 节）：并入用户自己的实验工程的“三段式收缩底栏”作为可选的新底栏；右侧按钮和双击面板改用它的玻璃画法；删掉右侧和面板以前所有画法（E2、最上层镜像、登记配方、取色）和探针、对照玻璃。
- 原来的液态底栏仍是默认，代码没动。顶栏的三个玻璃开关保留（只管顶栏）。
- 当前下一步：只围绕真正首帧匹配推进，包含每次换视频和新面板。共存、旧状态保留、固定浅色或延迟显示不算达成。3.11.23先验证系统色调动画是否被捕获且是否解释了实际深色。

测试环境：iPhone 16 Pro Max，iOS 26.6，抖音（版本号没记录）。
本机编译不了（这边没有 Theos 环境），一律推到 GitHub 用 Actions 编译。

## 0.55 3.11.53（用户：“隐藏状态栏 dy[y]y 应该也有吧，你看要不直接搬过来”）

- 看了它的源码：它的“隐藏系统顶栏”只有 6 个 `%hook` 的 `prefersStatusBarHidden` 返回 YES（AWEFeedRootViewController、IESLiveAudienceViewController、AWEAwemeDetailTableViewController、AWEAwemeHotSpotTableViewController、AWEFullPageFeedNewContainerViewController、AFDPureModePageContainerViewController），没有别的手段。这 6 个钩子插件早就有（清屏用），一样。它能藏住是因为常开：页面出现时 UIKit 就按它定；清屏是运行中途切换，要靠刷新请求让 UIKit 重新问，这一步在抖音里没生效。
- 搬成常开开关：播放 → 隐藏状态栏（`statusBar.hide`，默认关），6 个钩子 `gAWEStableCleanScreenEnabled || ALBStatusBarAlwaysHidden()`。切换时立刻请求刷新；设置页关掉时 UIKit 也会重新问一遍。
- 清屏的刷新：`AWEStableCleanUIKitSetNeedsIMP` 先看 `UIViewController` 的 `setNeedsStatusBarAppearanceUpdate` 实现在不在 UIKitCore；不在（被抖音交换过）就在 `UIViewController` 的方法里找名字以 `…etNeedsStatusBarAppearanceUpdate` 结尾、实现在 UIKitCore 的那个直接调。诊断窗口段多“刷新请求=UIKit 自己的 / 被换过…”；【一键清屏】开头多“隐藏状态栏=开/关”。

## 0.54 3.11.52（用户编译 3.11.51 后，带三份诊断：“好像就剩状态栏清不掉，两次我都单独抓了。音乐图标正常，头像点击进入别人主页有点问题，好像不能点击第二次，只能点一次回来再点就不行。点赞特效还是会被例如双击面板之类的手势唤醒。除了那个禁止上灵动岛，我还需要加一个那个默认高画质播放视频的功能，可以参考 dy[y]y 开源的实现”）

- **状态栏**：诊断【一键清屏】进入 0.5 秒后状态栏仍显示，按 `childViewControllerForStatusBarHidden` 算到 `AWENormalModeTabBarController → AWEBaseRootNavigationController → AWEFeedRootViewController`，它要藏、类也钩上了，补请求 18 次无效。所以决定者不是算出来的那个，或请求被吞。3.11.52 同时上几条路：
  - 私有 `_effectiveStatusBarHiddenViewController`（有才调）认出的那个也钩上并请求；
  - 请求时再直接调 `UIViewController` 自己的 `setNeedsStatusBarAppearanceUpdate` IMP（绕过容器改写）；
  - 清屏时 `UIViewController` 基类的 `prefersStatusBarHidden` 也钩上（没自己改写的页面都说要藏）；
  - Info.plist `UIViewControllerBasedStatusBarAppearance=NO` 时改用 `setStatusBarHidden:withAnimation:`，退出还原；`%hook UIApplication` 挡清屏中的应用级“显示”。
  - 诊断：0.5 秒快照多一段窗口列表（类、层级、隐藏、整屏、根页面、要不要藏、`_canAffectStatusBarAppearance`、UIKit 认的决定者），还有“按页面管=是/否”“应用级藏”“挡下应用级显示 N 次”。还不行就看这段。
- **头像（展开菜单关）只能点一次**：头像玻璃铺着栏按钮表面，触摸转交，`TouchUpInside` 时有时无（第一次常有）；抖音头像的手势在 `AWEPlayInteractionUserProfilePictureView` 上，不是这颗按钮的上层，看不到。改：`ALBFaceTapOnTouchUp(slot.menuButton)`，`ALBAvatarTapTarget` 直接 `AWETriggerQuickMenuDestinationView`（0.8 秒去重），去掉 3.11.51 的“0.5 秒后页面没变才进”。菜单开着时补发的 TouchUpInside 进 `ALBAvatarTapTarget` 直接返回。诊断【头像点按】。
- **点赞特效被双击面板手势唤醒**：诊断“最近触发：双击（兼容）”。兼容模式把任何双击都当点赞。删掉（`CompatMode()` 固定 NO，设置行、特效页开关、报告字段都去掉）；按作品认的引擎（`setUserDigged:`）照旧。
- **默认最高画质**（新开关“播放 → 默认最高画质”，`playback.highestQuality`，默认开）：照它的做法 `%hook AWEVideoModel`：`playURL` 换成 `AWEVideoBSModel.bitrate` 最高那档的 `playAddr`，`bitrateModels` 只留那档。组 `ALBHighQualityGroup`，ctor 里类在才 init。插件自己读全部档位（`AWEVideoVariantsForVideo`、`ALBMSVideo` 保存视频挑编码）时 `gALBHQBypass`（每线程计数）绕过。诊断【默认最高画质】。
- **快捷跳转**：3.11.51 诊断——我的订单按 `initWithContext:`（新建 `IESECOrderListContext`、抄回 5 个变量）有内容（字 7、格子 10）；特别关注 `init` 字 4、格子 2；消息通知 `initWithLandingTab:0` 空白（子页面 0）。它的 `fansViewController` / `interactViewController` 是可写属性，由创建方塞入。3.11.52：进页时记下可写的子页面属性（类名＋简单变量，`ALBQJCaptureChildren`），新建后还空着就各建一个塞回（Swift 类不建）。诊断每页多“子页面属性”。
- 没验证。要的诊断：清屏后等 1 秒退出马上导出（看窗口列表）；先进一次消息通知，重启后用 ↑ 跳消息通知，停 2 秒导出。

## 0.53 3.11.51（用户编译 3.11.49 后，带两份诊断：“这很多地方的设置都已经是没有必要的，淘汰掉就行。你比如什么展开就是收缩的倒放，这本来就是这么做的，为什么还要给个开关呢？还有这个什么直接到位……这都是之前淘汰过的方案。整个三段式底栏玻璃就一个深色色调需要自定义，其余的根本就不需要，都是走默认的。按压时融合，这个也是做到了的，根本就不需要开关。开关现在基本只需要对一些功能和间距去用。你说的随亮度切换预知，如果是根据图标的话，那它还有点用……玻璃锁定深色的根本就不需要这个东西。还有顶栏的玻璃已经是栏按钮了，它也不需要去调整浓度……需要的只是双击面板跟右侧按钮，而且是默认打开就行了……然后就是一键清屏不成功的问题和快捷跳转不成功的问题我都给你抓了，还有音乐图标点击不生效的问题”）

**规矩补充：开关只给功能和间距；玻璃相关的一律走固定值，不要再加“试验”开关。**

- 设置删掉的行（值固定在用户现在的状态）：顶栏 `top.glassAlpha`（不再用，顶栏是系统栏按钮的玻璃）、`top.e2`（固定开）、`glass.lifted`（固定关）、`glass.inBarWindow`（固定关）；三段式 `dock.expandReverse`（固定开）；玻璃一节 `glass.directTone`（固定开，`ALBGlassSeed.h` 本来就只管右侧和面板）——第 6 节留空占位，不再接到各页（`ALB311OptionPages`、`openDockOptions`）；三段式外观 `dock.adaptiveInk`（固定开）、`dock.lightAt`（固定 0.85）、`dock.lightTint`（不用了）、`dock.fuse`（固定开）、`dock.followEdge`（固定关）。
- 三段式底栏：`dock.glassAlpha` → `dock.darkTint`“深色色调”（0–0.6，`kALBDockPrefClear` 改指它；`ALB25MaterialDockTint` 第一次读时由旧透明度换算存下）；`ALB25MaterialMotionClear` 固定 YES。`ALBDockGlassStyle` 视频页固定深色；新增 `ALBDockInkStyle`（视频页按背后亮度深 / 浅），`applyInk` 用它解析颜色并给自建各项挂上这个深浅（玻璃组、底栏本身仍是深色）。玻璃种类键去掉了“浅”。
- **右下角音乐（3.11.49 诊断“点了 0 次”）**：音乐玻璃（`ALBArtworkGlassButton`）铺着系统栏按钮表面（`ALBBarFace.h`），触摸落在表面的控件上、各阶段转交给玻璃按钮（`ALBFaceTouchAdopt`），UIKit 对转交的触摸不发 TouchUpInside，所以玻璃上挂的桥（`activate:` → `ALBForwardOriginalView` 转给抖音的 `AWEMusicCoverButton`）和 `ALBMusicTapWatcher` 从没跑过。修：音乐玻璃 `ALBFaceTapOnTouchUp`（同 3.11.19 双击面板）；桥的 `activate:` 同一序号第二次进来当新的一次（转交的触摸没有 TouchDown，`delivered` 不重置，否则只有第一次能送到）。诊断“最近”写转给了哪个控件（`gALBNativeLastForward`）、页面出没出来。
- 头像“展开菜单”关掉时的 `ALBAvatarTapTarget`：0.5 秒后页面没变才替它进主页（抖音自己的点按手势通常已经进了，免得进两次）。
- **快捷跳转（3.11.49 诊断：新建那次被后面“推回留着的那一页”的成功盖掉了；初始化：消息通知 `-initWithLandingTab:`、我的订单 `-init -initWithContext:`、特别关注只有 `init`）**：`ALBQJMakePage` 有 `initWithLandingTab:` 就用它（参数按方法签名的类型，值取记下的 `_landingTab`）；有 `initWithContext:` 且记到过 context 就重建 context（`ALBQJCaptureContext`：名字带 context 的变量；字典原样记，对象记类名＋简单变量，Swift 类不建）；都没有才 `init`；之后照旧只抄配置类变量。`objc_msgSend` 按 `ns_consumed` / `ns_returns_retained` 的函数指针调，ARC 计数对。新建结果（用的初始化、1.5 秒后字 / 格子 / 子页面数）存在 `jump.<n>.recreate`，重启后也在。诊断每页多一行：新建、context、这一类声明的属性（下一步新建要设哪些）。
- 一键清屏：3.11.49 诊断确认 `顶栏E2=开 顶栏放到最上层=关`（原地），正是 3.11.50 修的情况（见 0.52）。
- 没验证。要的诊断：装好后先各进一次 我的订单 / 特别关注 / 消息通知（记下 context 和属性），重启，不进这几页直接用 ↑ 各跳一次，停 2 秒再导出；音乐点一次停 2 秒导出；清屏后翻几条、退出马上导出。

## 0.52 3.11.50（没编译过，并入 3.11.51）（用户编译 3.11.49 时：“你还得修一个一键清屏，清理不掉顶栏元素和状态栏的问题。状态栏是偶尔清理不掉，顶栏是一直清理不掉，现在应该不需要把它放到最上层了吧，我感觉。会不会是这个影响的，忘了”）

- **顶栏一直清不掉**：`ALB3TopChromeView`（插件的顶栏）是抖音 `AWEFeedTopBarContainer` 的兄弟视图（`ALB3PlaceTopChromeInPlace` 插在它上面），不是子视图。清屏 `AWEStableCleanSuppressView` 只把容器 `layer.opacity` 设 0，插件顶栏不受影响。插件顶栏的显示由 `ALB3TopContainerShowing`（已经包含 `!gAWEStableCleanScreenEnabled`）决定，但原地时只在容器 `layoutSubviews / setHidden / setAlpha / setFrame / setCenter / setTransform` 时调 `ALB3MirrorTopChrome`，进清屏改 `layer.opacity` 不触发任何一个，所以一直显示。`glass.lifted` 开时 `ALB37Sync` 每帧都跑，清得掉；用户的设置是关（原地），所以“一直”。不是“放到最上层”导致的，正相反。
  - 修：`AWEStableCleanSyncTopChrome()`（`gAWEHomeFeedTopContainer` 有插件顶栏就 `ALB3MirrorTopChrome`），在 `AWEStableApplyCleanScreenGlobal` 末尾（进入、每次补清都会走）和退出时 `AWEStableCleanRestoreAll` 之后各调一次；树扫描碰到 `AWEFeedTopBarContainer` 时也对它调一次（别的频道的顶栏）。用的是 `hidden`（`ALB3PlaceTopChromeInPlace` / `ALB37SyncChrome` 自己的逻辑），不对玻璃设 opacity。
- **状态栏偶尔清不掉**：`AWEStableEnsureStatusBarHookForClass` 按类换 `prefersStatusBarHidden`，但只在进清屏和退出时走一遍控制器树。之后翻到别的类型的作品（图集 / 直播 / 广告的页面类不同），新页面类没钩，UIKit 问到它时返回 NO，状态栏回来。
  - 修：`AWEStableCleanRequestStatusBarRefreshController` 也顺着 `childViewControllerForStatusBarHidden` 走、每个控制器只走一次、返回新钩的类数；`AWEStableCleanCheckStatusBar()` 在 `AWEScheduleCleanScreenReapply` 每一次（立即＋0.055/0.16/0.36/0.8 秒）都先只钩不请求，新钩了类或 `statusBarManager.statusBarHidden` 为 NO 时再整棵 `setNeedsStatusBarAppearanceUpdate`（没新钩时最多 0.5 秒一次）。补清多了 0.8 秒那一次。
- 诊断【一键清屏】：`AWEStableCleanReport()`——在不在清屏、进过几次、顶栏显示 / 藏着（原地 / 最上层，按父视图 `AwemeLiquidBar.GlassLayer` 认）、状态栏、`glass.lifted`、补藏次数、进清屏后才钩上的类数、最近一次补藏时 UIKit 问的控制器链最后三个（它的 `prefersStatusBarHidden`、类钩没钩）、进清屏 0.5 秒后的快照。
- 没验证。还清不掉时要：清屏、翻几条，再退出清屏马上导出诊断（进清屏 0.5 秒后的快照和补藏记录会留着）。

## 0.51 3.11.49（用户看了 3.11.48：“你现在这个玻璃的透明度调整，我根本就不知道它在调整哪一块的玻璃。还有展开菜单，我现在已经不需要它变透明了，展开什么样就是什么样……它不需要再跟随玻璃透明度去降低了，因为根本就衔接不上。我是说设置要简化……按道理来说现在每个地方的玻璃色调浓度都是单独的吧，你单列出来的那个感觉不管任何地方了……一级页面那个地方应该是不管任何东西的了？你确认一下。第二点，点赞特效识别不到……我有双击点赞、三击点赞、双击面板点赞、新面板点赞……第三点，快捷跳转页面……每次重启之后除了观看历史能够跳转，其余三个都跳转到未知页面，根本就啥也不显示……另外，三段式底栏图标变色……0.15 秒读一次……它的变化是突然的，你能不能增加一个逐渐变化的过程？另外，头像是否展开菜单，也直接给一个设置项。还有右下角音乐图标现在的点击也不生效”）

**确认（统一滑块管了什么）**：iOS 26 上右侧按钮、双击面板走 `ALB25MaterialFollowBar()`（底栏自己的玻璃），`ALB25MaterialMotionClear` 根本不读；统一滑块只影响三段式底栏的视频页材质（`ALBDockGlassUsesClear`）、顶栏的视频页色调（`ALBToneApplyTop`）、展开菜单（`menu.followGlass` 开着时 `ALBToneStyleMenus`）。

**做了什么**
- 玻璃透明度：`ALB25AlphaFor(name, legacyOn)`；`ALB25MaterialTopAlpha`（`top.glassAlpha`，顶栏选项第一行）→ `LiftedClear`、`TopDarkTint`、`ALB25MaterialApplyTone`（只给顶栏用）、lifted 玻璃；`ALB25MaterialDockAlpha`（`dock.glassAlpha`，三段式底栏外观第一行）→ `MotionClear`、`DarkTint`、三段式底栏（`kALBDockPrefClear`）。没存过时先按 3.11.48 的 `glass.alpha`，再按更早的开关和 `glass.darkTint`。主页“玻璃”卡片删；选项第 6 节只剩“新玻璃直接到位”。
- 展开菜单：`ALBToneMenuFollow()` 恒 NO，设置项删。
- 点赞特效（`ALBAmbience.xm`）：`OnDiggChanged(model, old, now)`；`ALBFXArmLike(model)`（ALBPaletteRun 第 3 项、三击点赞前调用）2.5 秒内这一条变成已赞就放；否则要 1.5 秒内碰过、碰的视图在视频上（`InVideoArea`，不再因为类名带 Comment 拒绝）且 `ALBFXAwemeIDForView(碰的视图)`＝`ALBFXAwemeIDForModel(变成已赞的模型)`；一次触摸一次、0.8 秒内多余的不算。ID 两个函数在 Tweak.x（只往上找格子上下文和控制器，不退到最上面那一页）。
- 快捷跳转：`ALBQJIvarIsConfig`（名字带 enter / landing / from / source / noticetype / tab / scene / type / channel，且不带 has / is / state / status / render / request / shown / visi / current / selected 等）才抄回；新建后 1.5 秒数字和格子，诊断写“像是空白”；诊断列出三页类自己的 init / 工厂方法。“重启后也能跳（新建页面，试验）”。
- 三段式底栏：`applyInkFaded`（图标、标题、分隔线、头像、系统底栏的 `_UITabButton` 各挂 0.35 秒 CATransition fade 再换色）；`materialChanged` 的 style 动画 0.4 秒；`lumaUpdateRate` 0.2、防抖 0.2。
- 头像：`avatar.menu`（右侧按钮卡片“头像展开菜单”）；关掉时 menuButton 去掉 menu / primary action，`ALBAvatarTapTarget` 点一下 `AWETriggerQuickMenuDestinationView(profileHost)`。弱引用用 `ALBWeakRef`。
- 右下角音乐：`ALBArtworkGlassButton -gestureRecognizerShouldBegin:` 放行绑定的原生点按（UIControl 默认挡外层单击手势）；桥的 `gestureRecognizerShouldBegin:` 对作品图玻璃也放行；没送到时 `kALBNativeFallbackKey` 兜底；另挂 `ALBMusicTapWatcher`：1 秒后最上面那一页没变就 `snssdk1128://music/detail/<id>`。诊断【右下角音乐】。

**下一版先看**
- 【右下角音乐】放行次数、兜底次数、最近一条（音乐页是不是抖音自己打开的）。
- 【快捷跳转】新建后“1.5 秒后：字 N、格子 N”，空白的话按诊断里列出的初始化方法新建。
- 【点赞特效】“按作品认”的拦下原因。

## 0.50 3.11.48（整理版。用户看了 3.11.47，附诊断：“单双列切换，要不你直接改成跟现在的展开菜单平级的字也行吧，你就放在那个侧边栏下面放一个单列放一个双列得了……那个快捷跳转面板也做成跟那个新面板一样的形式就行。你现在的那个太迟钝了，还不如直接用成功的那个。然后团购还是有问题。别的问题好像没了。把代码和整个插件的设置整理一下，该分类的分类一下，功能名称全部简化，补充说明解释的部分全部删掉。把代码里和参考插件相关的全部删掉，替换掉，替换成正常的类名，一个都不要剩，设置里也不要出现任何相关的东西。清透玻璃全部改成玻璃透明度。我要做这一版结束前的整理了”）

**诊断里看到的（3.11.47）**
- ↑：“系统菜单：按下 2 次、弹出 0 次、收起 2 次、没弹出改自绘 2 次”——UIButton.menu 在底栏里也弹不出来（和 3.11.45 的 UIControl 一样），0.8 秒后才换自绘，所以“迟钝”。
- 团购（导出时设置页开着、频道在屏幕外）：外层 `AWEGrouponC2CollectionView` y150 高 723（到 873，差 83 到屏幕底），里面信息流已经补到父视图底（“补 36，页高 724”，722 次）；外层没补，所以底栏后面还是外层容器 `AWEGrouponC2ContainerView` 的 #F8F8F8。
- 角标照系统：量到了（左 10.3 顶 -17.2 高 18.3 留白 7.3 字号 12.x）。

**做了什么**
- 单双列：`ALBColumnMenuItems`（两个普通 UIAction，图标 rectangle.portrait / square.grid.2x2，当前那项 state On），接在 `ALB3TopMenuElements` 的“侧边栏”后面；图块、蓝色对勾那套删了。
- ↑：`ALBFloatPanel` 加 `mode`（0＝✦ 功能卡片，1＝快捷跳转四页）；`ALBFloatPanelOpen(source, mode)`（同一种开着就关，另一种开着就换）；`ALBDockHostOpenQuickJumpPanel`（ALBDockCommon.h 导出，ALBQuickJump.h 实现）。跳转卡片：没准备好的半透明，点了先关面板 0.2 秒后 `ALBDockHostQuickJump`，不能拖动排序。ALBDock.m 删掉 `ALBDockMenuItem`、`ALBDockArrowButton`、`ALBDockQuickMenu`、`arrowMenuElements` 等，`dock.menuStyle` 删。
- 团购：`ALBGrouponFillFrame`（原 ALB47）同时钩 `AWEGrouponC2CollectionView`（`ALBGrouponOuterFillGroup`）和 `AWEGrouponLifeFeedsCollectionView`（`ALBGrouponFeedFillGroup`）；父视图下面的纯色空视图（类就是 UIView、没有子视图）不挡；下内边距只在 `adjustedContentInset` 不够时才补（外层系统会自己加底部安全区）。
- 玻璃透明度：`ALB25MaterialGlassAlpha()`（`glass.alpha`，0–1；没存过时按 `glass.clear` / `glass.motionClear` 和 `glass.darkTint` 换算一次存下）；`ALB25MaterialLiftedClear` / `ALB25MaterialMotionClear`＝值 >0；`ALB25MaterialDarkTint`＝0.45×(1−值)；顶栏的 lifted 玻璃也用它（原来写死 0.28）。三段式底栏 `kALBDockPrefClear` 改成 `glass.alpha`，`ALBDockGlassUsesClear` 读 `ALB25MaterialMotionClear()`。主设置页“玻璃”卡片一个滑块（松手发 ALB5.OptionsChanged）。
- 设置：`ALB5OptionSections` 重写（名字全短；第 10–13 节是三段式底栏的外观 / 尺寸 / 头像 / 悬浮面板，三段式底栏页＝3、10–13、6）；各节说明（titleForFooter）、动作项的小字、功能排序页 / 项目页 / 图标中心 / 底栏按钮页的说明删掉。主页卡片重排（见 README）；“顶栏玻璃（试验）”卡片拆掉（前三项在顶栏选项页里，后两项挪到消息页）。
- 代码：Tweak.x 开头 450 行逐版本说明换成文件分工表；评论区文件改名 `CommentGlass.xm`（Makefile 同步）；所有参考插件的名字、函数名、对它的代称从代码、设置、文档里去掉；“清透”字样换成“透明”；悬浮面板删掉不用的菜单代码和亮度探针属性。

**下一版先看**
- 团购：【团购底部】里“列表补到父视图底”的最近一条是不是外层 `AWEGrouponC2CollectionView`，有没有“下面有 X，不补”。
- 玻璃透明度：滑块调到 0 时顶栏、右侧、双击面板、三段式底栏是不是都回到系统玻璃。

## 0.49 3.11.47（用户看了 3.11.46，附 7 张图（评论常驻态有拼缝；键盘弹起时输入框不透明白底；评论面板浅色；键盘弹起时玻璃上圆角、评论透出来；↑ 菜单四行叠在一起；电话 App 的 经典 / 统一 菜单（蓝色对勾圈）；我们的单双列菜单（单列带选中块））、诊断、一段底栏录屏：“细枝末节的地方有点多，一条条改吧。首先就是纯透明模式肯定是几乎没有可读性，直接合并到色调模式里面。然后这个玻璃有问题啊……整个评论区从上到下应该是一整块玻璃，然后底部的那个输入长条我让它悬浮就行，除此之外不要有任何分界线之类的，然后那个悬浮条的材质换成系统的高斯模糊……输入框弹起时让除了输入框之外的区域也变成系统的高斯模糊，只不过两者稍微区分一下……不需要圆角了，圆角没办法弄，太丑了……第二个就是这个快捷跳转页面的展开菜单完全乱的没眼看，如果你要变成刚刚讨论的新的的话，就做尝试。第三个是……那个新面板没办法点调色盘选取相册壁纸背景，另外给它也加一个调节色调强弱的选项和调节背景透明度的选项吧，只调节深色色调就行，因为文字肯定固定成白色，然后略微增加一下每个模块的那个不透明度，然后给面板里面增加一个快捷打开设置全页的功能……第四个是团购底部的背景还是没有清掉。第五个是点赞特效……会疯狂跳特效，你把它确确实实绑定到点赞触发上……第六个是未读的这个消息角标它乱跳……还有右上角的红点，你有绑定真实的信息吗？……以及展开动画好像没有完全对应收缩动画去倒放。然后单双列……按图里的这种打蓝色对勾来实现……别人主页，我没有抓到那个……箭头的展开，它只有一个文字的展开。你看信息够不够吧？不够的话，不做的也行……然后评论区的长按面板那些按钮点不了”）

**诊断 / 录屏里看到的（3.11.46）**
- 录屏（底栏一条，10.8 秒）：展开结束那一帧“23”从小一号跳成大一号、位置也挪——动画里画的是自建角标（9pt 粗体、高 14、位置 w/2+3），结束后系统底栏接管，画的是系统角标。红点在收缩态一直亮（`dock.redDot` 只是开关，没读任何未读）。
- 【评论区不缩到顶部】3.11.46 输入区：装了 1 块、输入框玻璃 1 块、面板玻璃截断 122 次；最近一次输入区“常驻态 圆角 0”——键盘弹起状态从来没认出来（输入区不一定变高 / 顶出面板，只是被抬离面板底边），所以键盘弹起时那张图里还是输入框白底。
- 【团购底部】`AWEGrouponLifeFeedsCollectionView` y319 高 601（到 920），它的父视图 `AWEGrouponLifeFeedsPageView` y319 高 724；差 123≈系统底栏留的位置；外层 `AWEGrouponC2CollectionView` 到 956、下内缩调整后 127。3.11.3 的“量空缺改 frame”对这条在格子里的列表抢不过（两次写回就判“抢不过”放弃）。
- 【点赞特效】最近触发“双击点赞（模型）”——模型钩子（`setUserDigged:` 0→1）＋“1.5 秒内碰过屏幕”，进收藏 / 喜欢页的那一下点击之后列表批量填“已赞”，一个个都算；“喜欢”页签的读屏文字也会被 `LikeButtonHit` 当成点赞键。
- 【长按面板】是视频长按面板（DUXBasicSheet），评论长按面板这次没记到；代码里评论长按面板每一行（格子）盖一颗 `ALBChatPillButton`，触摸转给格子以后列表的选中不认。
- 【别人主页简介区】见过 0 次（只钩了 `AWEUserDetailViewController`）。

**做了什么**
- 评论区（`CommentGlass.xm`）：`AWECommentGlassMode` 存的 0 当 1；设置页材质只剩 色调 / 磨砂，圆角滑块和 `commentInputRadiusChanged:` 删掉（`AwemeLiquidBarCommentInputRadiusV1` 不再读）。`AWECommentSyncPanelTrim` 只把面板玻璃还原成整块。`AWECommentSyncInputGlass` 重写：常驻态输入区不垫东西，只给输入框垫 `systemMaterial` 模糊（`AWECommentBarGlassView`，圆角跟抖音写的）；键盘弹起（`gAWECommentKeyboardUp`／输入区高 >120／顶出面板／离面板底 >40）时输入区整块垫 `systemThinMaterial`、直角。新钩 `CommentInputContainerView layoutSubviews`（`AWECommentInputLayoutGroup`）和键盘四个通知补同步；输入区不在控制器视图里时到窗口里找；输入框找不到时顺着能打字的视图往上找胶囊（`AWECommentInputFieldByText`）。诊断换成 3.11.47 的一组计数＋“最近一次键盘弹起时”。
- 评论长按面板（`ALBChatFace.h`、`ALBMoreGlass.h`）：玻璃按钮的 owner 是列表格子、点下去没有落在别的按钮上、也没搬手势时，不再转触摸给格子，抬手在玻璃里就 `ALBPillSelectOwnerCell`（shouldSelect → selectItem → didSelect；表格同理）。行里套着横向列表（选人那一排）的不盖玻璃（`ALBMoreRowHasNestedList`）。
- 三段式底栏（`ALBDock.m`）：`ALBDockArrowButton`（UIButton，`menu`＝`UIDeferredMenuElement` 每次现做，`showsMenuAsPrimaryAction`，高亮 / 收回预览＝`midGroup`，`willDisplay/willEnd` 计数，按下 0.8 秒没弹出→`openCustomQuickMenu`），盖在 ↑ 上，`dock.menuStyle`＝1 时藏起来走自绘。自绘卡片行的子视图 tag 改 101–103。`measureNativeBadges`：在系统底栏按钮里找类名带 Badge 的视图，量左边 / 顶边相对图标中心、高、字体、留白、底色，`applyBadgeLeft:` 写到自建项（量不到的项和单独键用第一份）；`syncNative` 角标变了 0.25 秒后量一次。`hasUnread`＝主栏几项＋单独键有角标（nil 以外，空串＝小红点），`cachedRedDot`＝开关 && 有未读。`dock.expandReverse`（默认开）：展开时长＝`kCollapseT`、球的曲线 `1-P(1-u)`、`applyExpandAt` 直接调 `applyCollapseAt:(seg-t)`（x0 换成目标侧）。
- ✦ 悬浮面板（`ALBFloatPanel.h`）：调色盘不弹菜单，切到“外观”（`alb_toggleEditor`）；相册从窗口根控制器往上找（跳过正在收起的 / 类名带 Menu 的），弹出 / 选回 / 写入都记；`float.tint`（默认 0.22，Clear＋黑染色，有壁纸时 `shade` 一层黑）、`float.clarity`（壁纸 alpha＝1−值）；文字固定白、玻璃 `overrideUserInterfaceStyle` 深色；卡片 0.40；齿轮 `ALBFloatOpenSettingsFull`（`AWELiquidSettingsController`＋`ALBSettingsNativeNavigationController`，只给 large 档）。
- 团购（`Tweak.x` `ALB47GrouponFeedGroup`）：`AWEGrouponLifeFeedsCollectionView` 的 `setFrame:`／`layoutSubviews`：铺满父视图宽、底下空 20–200 且没有别的视图→高补到父视图底，下内边距至少补同样多。开关 `bottom.grouponFill`。
- 点赞特效（`ALBAmbience.xm`）：`InVideoArea`（响应链里有播放互动层 / 信息流 / 播放器 / 作品详情，没有 Comment）；点赞键必须在视频上；模型引擎也记真双击（两次按下 <0.35 秒、<60pt、在视频上）；`OnDiggChanged` 只认 1.2 秒内的点赞键或真双击，0.8 秒内、同一个手势只放一次；诊断加拦下次数。
- 单双列（`Tweak.x` `ALBColumnTileImageFor`）：68 宽的图：图标（无外框）＋名字＋对勾圈，浅 / 深各一张进 `UIImageAsset`，`AlwaysOriginal`，菜单项标题留空、状态一律 Off、读屏用 accessibilityLabel。
- 别人主页：`ALB45DiagnosticsInstall` 多钩 `AWEUserHomeViewControllerV2`、`AWEUserHomeViewController`（不是导航栈根、或是弹出来的才算），诊断写钩上了哪些、认到的类。收起功能这版不做（没有带箭头展开的样本）。

**下一版先看**
- ↑：系统菜单弹没弹出来（“弹出 N 次”还是“没弹出改自绘”）；弹出来的话看是不是从胶囊长出来。
- 角标：“角标照系统：量了 N 次（左 / 顶 / 高 / 字号）”；录屏看交接那一帧还跳不跳。
- 评论：键盘弹起时“最近一次键盘弹起时”那一行（输入区位置、输入框找到没有）；“没找到输入框”次数。
- 评论长按面板：“替列表选中 / 选不了”次数；选不了的原因。
- 团购：“信息流补到那一页的底 N 次”和“下面有东西不补”的原因。
- 设置页（整页入口之后要调整）。

## 0.48 3.11.46（用户看了 3.11.45，附 3 张截图（评论分享面板选了人以后中间那块半透明；三段式底栏藏了两项、关了胶囊后整组贴左；参考的悬浮面板：头像＋名字＋简介、右上角调色盘、两张深色半透明卡片“复制链接 / 获取作品数据”），附诊断：“快捷跳转页面不生效，双击面板打开时，点任意地方不能关闭，只能上滑关闭。评论区那两处不透明或者磨砂改的时候有点问题，直接按参考做法 吧，做成玻璃，但是键盘弹起时的那个输入框后面那块大的背景，我需要一个圆角自定义。以及我可能需要抓一下评论区未打开时那个区域有一条线，再加上玻璃会有一个边缘导致它重叠在一起，看着累赘。三段式底栏去掉按钮之后做法有点问题，按道理来说它不会莫名其妙变长才对……而且……去掉两个按钮的胶囊之后，它们两个就不居中了……评论区分享面板，选择某一个人分享之后，中间部分有一个半透明的东西……星星更改为呼出一个悬浮面板……跟底栏有一定的距离……半透明的圆角矩形，长宽比例遵循黄金分割比……右上角的调色盘按钮用来给这个背景更换壁纸。默认……clear 的，可以选择每列放 2 到 4 个功能卡片，音量和亮度支持左右滑动调节……长按某一个模块时，可以让它拖动重新排列……我们自己插件的那个弹窗提示……直接就用纯透按钮，保证内部的字体能够自适应背景切换黑白”）

**诊断里看到的（3.11.45）**
- 快捷跳转：四页都“见过、留着这一页”，但“最近一次跳转 -”；底栏“菜单打开过 0 次”→ UIControl 的 `showsMenuAsPrimaryAction` 在这里没弹出来。观看历史的地址是 `aweme://lynxview/?…channel=watch_history…`，抖音注册的地址前缀里没有 aweme，有 snssdk1128（跳的时候换前缀）。
- 双击面板：“点面板外关闭 2 次、上下滑关闭 4 次”——窗口上的 `UITapGestureRecognizer` 大多数时候被挡（UIControl 会拒绝祖先视图上的单击手势，抖音的点按也会先认）。
- 评论不缩：`videoDidShrink` 不执行 8 次、图文/实况挡 5 次，setFrame 钉回过一次挪开的（128,2,183×363 → 0,0,440×956）——有效。
- 三段式底栏下标：修正 8972 次（下标换算在跑）。我 / 收藏底部：最里面的收藏列表已经撑到底（0,191,440×765），它外面的 `AWEListKitTabContentCollectionView`（0,155,440×727，到 882）会裁——下一版看【我 / 收藏底部】里新加的“裁”标记确认。
- 团购：`AWEGrouponLifeFeedsCollectionView` 到 920，下面 36pt 是容器的 #F8F8F8；`UIView (0,237,440×853)`、`AWEGrouponTabListViewController (0,277,440×807)` 都超出屏幕底。用户这次没提，没动。
- 单双列：替用户点了首页但前后都认成单列（诊断偏好里是别的插件的键）。3.11.46 认法多一条“看得见的双列控制器”，偏好过滤掉别的插件的键。

**✦ 悬浮面板（`ALBFloatPanel.h`）**：见文件头。`ALBPaletteRun`（`ALBSidePalette.h`）是从双击面板的 `actionTapped:` 拆出来的，两边共用；标题 / 图标 / 出现条件 `ALBFloatTextsFor` 抄 `buildRows`。键：`float.columns`（0/1/2＝每行 3/2/4）、`float.order`、`float.gap`；壁纸 `Library/Application Support/AwemeLiquidBar/float-wallpaper.jpg`。头部名字 / 简介 `ALB8OwnName` / `ALB8OwnBio`（`ALB8AcceptOwnProfile` 现在也存 signature）。`ALBDockHostOpenPalette` 改成 `ALBFloatPanelToggle`。

**按下就关（`ALBOutsidePress.h`）**：`UILongPressGestureRecognizer`，`minimumPressDuration 0`、不取消触摸、`canBePreventedByGestureRecognizer:` NO；点按类（`UITapGestureRecognizer` 或类名带 Tap / Click）要等它失败、不同时；其他（拖动、滚动）同时。按下就调 onClose；目标是它自己；`retire` 在触摸进行中会等结束再摘（不然抖音的点按会在被摘掉时认出来）。双击面板和悬浮面板都用它。

**↑ 菜单（`ALBDock.m` `ALBDockQuickMenu`）**：整屏透明接点层（按下就关）＋一张系统 Regular 玻璃卡片，从中栏胶囊的 frame / 圆角弹簧到目标（宽 236、每行 52）；行按下有底色；选了以后关再跳。`ALBDockMenuItem` 的系统菜单关掉了。

**底栏几何（`framesForSide:`）**：格子宽＝`dock.tabWidth`（≥30 才算，0＝自动＝默认排法排满时的四分之一）；主栏＝格子×项数＋8；整组宽＝主栏＋gapA＋(中栏＋gapB)＋单独键，在 W 里居中；放不下时主栏收到放得下。

**评论区（`CommentGlass.xm`）**：3.11.45 的“透明 / 不透明 / 系统模糊”整段删了（`AWECommentInputBackdropMode`、垫模糊、还原原底）。新的 `AWECommentSyncInputGlass`：输入区容器垫 `AWECommentBarGlassView`（它的 layoutSubviews 里重算面板截断，同参考实现）、输入框垫一块（清掉它的底色，圆角跟抖音写的）；回复态＝容器高过 120 或顶到面板上面，上面两个角＝`AwemeLiquidBarCommentInputRadiusV1`（默认 18）。`AWECommentSyncPanelTrim`：输入区玻璃在渲染、满宽、到面板底，且不是“回复态＋圆角>0”时，面板玻璃截到它顶边（顶到面板上面就藏）。`AWECommentHideInputHairlines`：输入区里和它兄弟里贴着它顶边、高≤1.5、宽≥六成、有底色的视图藏起来。面板玻璃 `borderWidth` 0。材质 `AWECommentApplyGlassMaterial` 和面板同一套。诊断【评论区不缩到顶部】里多了输入区玻璃那一段。设置 → 评论区：圆角滑条替掉三段选择。

**评论分享面板选人后（`ALBMoreGlass.h` `ALBMoreCommentPadApply`）**：垫的玻璃只铺在输入层（`ShareAdditionTextView`）那一段，垫上面被盖住的 cell（在垫里、底在输入层顶以上）alpha 0，记在 `gALBMoreHiddenRows`，输入层收起 / 关功能时还原。

**图文页底栏（`ALBMoreNoteBarClean`）**：往上找 `FullPageFeedBottomBarView`，它和它父视图里贴着它顶边的细线藏起来；第一次见到时记树（诊断【更多玻璃】“图文页底栏结构”）。

**提示条（Tweak.x `AWEShowDownloadToastForJob`）**：玻璃固定 `UIGlassEffectStyleClear`（不再 `ALB25MaterialFollowEffect`、不再描边、不再 `ALBMaterialApplyEffectView`），里面垫一个 `ALBLumaProbe`（`ALBLumaProbe.h`，tracksLuma），亮于 0.62 字和普通图标变黑、暗于 0.54 变白。

**留到下一版**：别人主页收起按钮（这次诊断还是 0 次，要先打开一次别人的主页再导出）；快捷跳转免打开；团购位移；收藏页外层裁剪（看“裁”标记）。

## 0.47 3.11.45（用户看了 3.11.44，附 6 张截图（评论区＋键盘的白输入框、评论区＋键盘透明、底部“期待你的评论”输入条、电话 App 的 经典/统一 筛选菜单、别人主页的简介 / 标签 / 商品橱窗那一块、整个主页）：“三段式底栏另外两个不透明是切换深浅色模式导致的……评论区的长按转发……只能锁浅色……给两个区域恢复不透明，或者改为系统高斯模糊……分享面板那个背景还是没有变成玻璃，在参考实现里找一下……三段式底栏的快捷跳转……星星图标……打开双击面板……箭头……快捷跳转……让它点击之后变成展开菜单……消息通知，特别关注，我的订单，观看历史……可以只保留右侧的单独按钮……默认的图标顺序……首页的右边应该是拍摄按钮……支持排序……选择右侧那个单独按键表示哪个功能……某一个 type 直接隐藏掉……单独的那个按钮……图标变为头像，支持自定义内边距……评论区玻璃色调的调节功能不生效……三段式底栏的图标颜色切换比较迟钝……把参考实现的下载视频和实况的功能搬过来……点赞作品和收藏作品……保存所有实况，保存当前实况，保存所有图片，保存当前图片，保存所有类型图片……一键清屏……没有碰到对应的玻璃按钮胶囊就可以触发关闭……上下滑动也可以触发关闭并直接滑到下一个视频……上区域双击打开双击面板，下区域双击打开评论区……团购的那个底部背景的位移……收藏页面背景不能隐藏……左上角展开菜单增加一个切换单双列的区域……全屏状态会影响到聊天页里面的视频 UI 无法置底……应用内通知……搜索进入的一些视频……打开评论区时作品不会收缩到顶部这个开关现在已经完全失效了……加入诊断整个重做……别人的主页……加入对那个区域的诊断……不要分节去做，一次性做完所有的诊断和调整。实在不确定做不到的可以保留到下次再做”）

**三段式底栏（`ALBDock.m`、`ALBDockCommon.h`、`ALBDockMaterial.m`、`ALB5Settings.h` 的 `ALBDockItemsController`、`ALBQuickJump.h`）**
- 项目：五项都是“项目”（`ALBDockType` 0 首页 1 消息 2 拍摄 3 我 4 朋友，`self.tabs` 下标＝项目，五个都建）。新键：`dock.order`（默认 [0,2,1,3,4]）、`dock.hidden`、`dock.singleType`（默认 4）、`dock.mid`（默认开）、`dock.arrow`（0 菜单 1–4 直接跳）、`dock.singleAvatar`、`dock.avatarInset`（0–14，默认 4）、`dock.jumpRecreate`（默认关）。`ALBDockMainTypes()`＝顺序去掉单独键、去掉隐藏（全藏留第一项）。左边时倒序（默认倒过来正好是原来的 我 消息 拍摄 首页）。
- 几何（`framesForSide:`）：格子宽＝四项排满时的宽（不显示中栏时把中栏和 gapB 让给主栏）；主栏少几项按格子宽变短；整组贴单独键那侧的边距；不显示中栏时主栏 ↔ 单独键用 gapA。`ALBDockGeo` 的 `tabX/rank` 五格，只排主栏里的；收缩 / 展开、系统底栏的项（tag＝项目）、实测图标位置（`measuredTabX` 五格）都按主栏项目。中栏不显示时 `parkMid`（挪出屏幕，不切 hidden / effect）、动画里不动它。
- 单独键：`performType:`（拍摄走 TapPlus，其余 TapTitle）；角标跟着那一项；显示头像时 `singleItem.alpha=0`、头像常显，点击由球的手势转成 `singleTapped`。
- ✦：`ALBDockHostOpenPalette(source)`（Tweak.x / `ALBQuickJump.h`）——找屏幕上的右侧按钮栈 → 往上找 `AWEPlayInteractionViewController` → `ALBPresentSidePaletteAt(controller, 点)`（`ALBSidePalette.h` 拆出来的按位置打开）。
- ↑：`ALBDockMenuItem`（`ALBDockItem` 子类，UIControl 自带菜单 `showsMenuAsPrimaryAction`），高亮 / 收回预览指向整块 `midGroup`（`UITargetedPreview`＋胶囊 `visiblePath`），菜单开着时不计闲置。直接跳模式换成那一页的符号。
- 快捷跳转（`ALBQuickJump.h`）：四页 `AWEIMFansAndInteractMergeViewController`、`AWESkylightUnreadListViewController`、`IESECOrderListViewController`、`BDXViewController`（URL 含 watch_history）的 `viewDidAppear:`（`ALBMoreHook`）记下：地址（`config.schemaParam.originURL` 等一串键路径）、自己的简单变量（字符串 / 数字 / 标量）、推进来还是弹出来、导航栈、弹出者，存 `ALB5Key(jump.N)`，并留着这一页（本次启动）。跳：地址 → 应用代理 `application:openURL:options:`（前缀没注册就换成 snssdk1128），不行系统 `openURL`；留着的页面 → 还在导航里就退回去，否则推进当前导航 / 原样弹出；`dock.jumpRecreate` 开 → `[[cls alloc] init]`＋抄回简单变量（试验）。**没 hook init**：ARC 下用 block 换 init 族方法会把 +1 / consumed 语义搞错。诊断【快捷跳转】。
- 换深浅变不透明：`ALBDockGlassGroup` 记 `materialKey`（clear|浅|浓度 / native / regular），深浅只改 override（外面 0.2 秒动画），种类变了才换 effect（清透之间动画，别的先 nil 再装、不带动画）；`ALBDockMaterial.m` 系统深浅一变就发 `ALBDockSystemStyleChangedNotification`（不管在不在视频页），底栏 0.05 / 0.45 秒后 `rematerialize`（nil → 新），动画中就标记、结束补。诊断“项目（3.11.45）”行里有换种类 / 整只重装次数。
- 图标颜色：`applyInk` 把系统底栏的 `unselectedItemTintColor` 写成按当前深浅解析好的颜色（不再是动态色），自建项重设；探针 `lumaUpdateRate` 0.15、防抖 0.15 秒。
- **下标修正（Tweak.x `AWECurrentMainTabIndex`）**：没有液态底栏控制器时原来直接用抖音的 `selectedIndex`，它不算拍摄格（消息 2、我 3）→ 三段式底栏下 `==4`（我 / 收藏的底部处理）从来不成立、`==3`（消息页挡全屏）指到了“我”。现在三段式底栏下按 `ALBDockBridgeSelectedTitle()` 换算（0.1 秒缓存）。诊断【三段式底栏下标】。这可能就是“收藏页背景藏不掉”的原因。

**下载（`ALBMediaSave.h`，按参考做法 `Features/MediaSave`）**：见文件头。入口 `ALBMSSave(aweme, what, source)`：Main（非图集视频 / 动图＝实况）、CurrentLive（图集当前项）、AllLive、AllTypes。图片候选第一个是插件自己的地址（`AWEImageURLForModel`）。双击面板 14/15/16 和头像菜单（`ALBSaveMenuElements`，`UIDeferredMenuElement` 现取）接入；1 号非图集改走它。诊断【保存视频 / 实况】。

**双击面板（`ALBSidePalette.h`）**：13 一键清屏、14–16 保存；点面板外（窗口点按，`cancelsTouchesInView`、其他点按要等它失败）或面板空隙关；窗口上下拖开始就关（不拦，抖音照样翻到下一条）；第 4 种双击模式“面板 / 评”。顺序默认 `[0,1,14,9,15,16,2,3,10,11,12,13,4,5,6,7,8]`。

**评论区（`CommentGlass.xm`）**
- 色调：浓度 > 0 时“色调”也用 Clear＋染色（`gAWECommentTintClearUses`）。
- 输入区背景 `AwemeLiquidBarCommentInputBackdropV1`（0 透明 1 不透明 2 系统模糊，默认 2）：不透明＝输入区整块不进去底（`gAWECommentCoverSkip`）、按记下的原色还回去、零状态底还原、键盘上那条（`AWECommentMiniEmoticonPanelView`）不去底；模糊＝去底后垫 `UIBlurEffectStyleSystemMaterial`（tag 26082045，深浅跟场景）。输入区容器的原底色现在也记着。
- 作品不缩 `AwemeLiquidBarCommentFreezeV1`（默认开）：老的“评论打开前记媒体位置、挡缩小”停用（`AWECommentMediaGuardEnabled` 恒 NO）。新的按参考做法：全局 `setFrame:` 里下一个响应者是 `AWEDPlayerViewController_Merge` 的视图，主窗口里原点挪开或比父视图小就写成原点 0、至少父视图大（只放大不缩小，和全屏撑高不冲突）；`willDisplay` / `viewDidLayoutSubviews` 补钉；`videoDidShrink` 不执行（渐变 α 拉回 1）；图文 / 实况 `updateShrinkState:insets:animated:(animationDuration:)` 要缩不调原方法（类名三个候选，都没有就在类名带 RichContent 的类里按方法找）；老的图片 / 实况内容视图缩小形变挡掉也接到这个开关；HUD 顶黑条（原来只在玻璃开着时藏）开关开着也藏。评论打开 0.6 秒后扫一遍主窗口像媒体的大视图（类名、位置、形变、归属）。诊断【评论区不缩到顶部】。
- 设置 → 评论区：“底部输入区和键盘上那条的背景”三段、“打开评论区时作品不缩到顶部”开关。

**评论长按面板 / 分享面板（`ALBMoreGlass.h`）**：`ALBMoreStyle` 在评论长按面板里一律浅色。分享面板：外壳 `AWESharePanelContainerViewController` 的 `viewWillAppear:` / `viewDidLayoutSubviews` / `viewDidAppear:` 也同步；`ALBMoreSharePlate`：内容视图不透明就是它，否则在外壳内容层 / 内容视图两层内找整宽、高过一半、不透明的底；不再用 `ALBSheetKindForView==2` 排除（只看控制器名有没有 LongPress）。诊断【更多玻璃】：同步次数、最近一次结果和原因、第一次打开时外壳整棵树（5 层）。

**左上角单双列（Tweak.x `ALBColumn*`）**：入口＝在首页再点首页（三段式 `ALBDockBridgeTapTitle(@"首页")`，液态 `AWEForwardToOriginalButton(gAWELastTabBar,0)`——绕开 `HOME_RESELECT_BLOCKED`）。现在的模式：`ALB32BottomVisibleRouteRoot(window, AWEBottomRouteGrid)` 或首页里 `AWEBottomRouteForWindow==Grid`＝双列。菜单最上面一个 inline、`UIMenuElementSizeMedium` 的两块（手画的手机图）。新键 `bottom.homeReselectColumns`（液态底栏放行首页再点首页，默认关）。诊断【单双列】：前后模式、抖音偏好里像单双列的键前后的值。

**全屏（Tweak.x `AWEInternalFullscreenActive` 等）**：最上面页面（0.25 秒缓存 `AWEFSTopRouteKind`）属于聊天（类名 AWEIM / IESIM / IMChat…，或弹出者是）→ 不全屏；`AWEInternalFSLayoutInteractionController`、`AWEFSLayoutKnownPlayerController` 各自再认 `AWEFSViewIsInIMContext`。搜索路线：最上面是作品页（`AWEFSControllerNameLooksDetail`，去掉 CommentInput）时放行。诊断【全屏】。

**专项诊断（`ALB45Diagnostics.h`，导出最后一段）**：别人主页（`AWEUserDetailViewController` 出现 1 / 2.5 秒后、导出时）——获赞那排到作品页签（或商品橱窗那排）之间的区域、里面每个字的位置、像展开键的小可点视图、获赞和橱窗共同父视图往下的树（类、位置、字、按钮动作、手势、归属）；团购（`AWEGrouponC2ContainerView` 布局时）和我页（内层列表布局时）底下 220 点里的整宽视图（底色、模糊、滚动内缩、归属）；收齐保存、双击面板、单双列、全屏、评论区不缩、下标修正的报告。

**留到下一版**：别人主页的收起按钮（等这次的区域诊断）；快捷跳转不用先打开（等诊断里的变量和导航栈）；团购底部位移（等【团购底部】）。

## 0.46 3.11.44（用户看了 3.11.43，附 3 张截图：“聊天页顶部和底部的背景没处理掉，评论区这个长按的面板深浅色不匹配，它是浅色玻璃的，但是我现在是深色模式。包括整个评论区色调也有问题。原本的时候，浅色模式下评论区就是浅色的，现在我系统在浅色模式下，评论区也是深色的，这导致根本看不清字儿……把这些没用的玻璃覆盖都删了吧，聊天页整个液态玻璃都撤回吧……分享面板颜色都不一样……按参考实现直接搬就行，玻璃颜色也直接搬，评论区分享面板应该只需要匹配深色模式就行。图文页那个框保留……三段式底栏……为什么给它改坏了，现在右边两个动不动就变成了深色调。以及……为什么现在每切到消息这一整页，都是由深变浅……这他妈是个固定的页面……这版本就只做转发面板，评论区长按面板，整个评论区的面板恢复……浅色是浅色玻璃，深色是深色玻璃。如果能调节这个色调的透明度的话更好。然后变形机制保留……撤销聊天页的玻璃覆盖”）

**聊天页撤回**
- Tweak.x：删了 `AWEIMInputVCBadgeButton` / `AWEIMInputVCUnhighlightedBadgeButton` / `AWEIMMessageVoipButton` / `AWEIMMessageBaseVCMoreButton` / `AWEIMMessageVCNavigationUnreadView` 的钩子、`ALBChatZoneGroup` / `ALBChatFansChipGroup` / `ALBChatPanelCellGroup` 三组和 ctor 里的 `%init`、`IESIMGrowingTextView` 和 `AWEIMOptimizeAudioInputTouchView` 的 `layoutSubviews`（只留 3.11.39 以前就有的 `didMoveToWindow`，底栏可见度那套）、对应的 @interface。`UIImageView setImage:` 里改成直接 `ALBPillImageChanged`（胶囊在用）。
- `ALBChatFace.h`：face 那段（`ALBChatFaceSync` 等）、`ALBChatPillSync`、输入框长底、加号格子、`ALBChatPageStyle`、`ALBPillInkStyle`、`ALBChatIvar` 删了；胶囊机制、去白底、去底（`ALBBacking*`）留着。胶囊没指定深浅时用 `ALBSceneStyle`。诊断去掉聊天页的计数。
- `ALBMoreGlass.h`：聊天顶栏、表情页签 / 搜索、表情 / 加号面板卡片、加号格子钩子删了。设置「更多玻璃」只剩 `bind.sheets`、`bind.note`。

**分享面板 / 评论长按面板**
- 参考实现（`SharePanelGlass.xm`）：`ShareStyleForView`＝窗口场景深浅；`UIGlassEffect` Regular、tint nil（清透才染）、interactive；整块底的玻璃上圆角 `containerConcentricRadiusWithMinimum:外壳圆角（没有就 20）`；场景 `registerForTraitChanges` 时全部重做。评论面板的底它只在浅色底（亮度≥0.76、饱和度≤0.14）时换。
- 3.11.43 的错：整块底按原来底色的亮度（`ALBMoreToneOf`）、胶囊按往上找到的那块底、输入层按自己的底色，三处各判各的；评论长按面板深色模式下那张底读出来是浅的 → 浅色玻璃。
- 3.11.44：`ALBMoreStyle(view)`＝`ALBSceneStyle`，整块底、关闭键、功能图标、输入层、发送键、评论面板每一行全部用它；`ALBMoreGlassPlace` 的 top<0＝同心；`ALBMoreObserveScene` 在场景上挂 trait 监听，变了跑 `ALBMoreGlassOptionsChanged`；`ALBPillApply` 的签名加了 `host.traitCollection.userInterfaceStyle`（抖音的字色是动态色，换深浅时要重画）。评论面板的底照旧任何不透明满幅底都换（不像 参考实现 只换浅色底），深浅跟系统。诊断【更多玻璃】写“深浅全部跟系统＝深/浅；系统深浅切换 N 次”。

**评论区面板（`CommentGlass.xm`）**
- 之前（至少 3.11.25 起）`AWECommentMakeGlassEffectForMode` 直接 `return ALBMaterialMakeEffect(YES)`（清透＋`glass.darkTint` 黑，后面的原始代码成了死代码），`AWECommentEnsurePanelGlass` 和 `AWECommentSync` 再 `ALBMaterialApplyEffectView`（`ALBMaterialStyle` 恒为深色）→ 系统浅色时评论区也是深色。
- 3.11.44：恢复 1.0.58 的做法——0 纯透明＝`UIGlassEffectStyleClear`、1 色调＝`UIGlassEffectStyleRegular`、2 磨砂＝`UIBlurEffectStyleSystemMaterial`；三种的 override 都＝`AWECommentSceneStyleForView`；材质 / 深浅 / 浓度任何一样变了效果重做（签名 `mode|style|tint` 存在 `kAWECommentGlassModeAppliedKey`）。色调浓度 `AwemeLiquidBarCommentGlassTintAlphaV1`（默认 0，0–0.8，5% 一档）：玻璃的 `tintColor`＝浅色白 / 深色黑 × 浓度；磨砂铺在 `contentView.backgroundColor`。设置 → 评论区卡片加“色调浓度（0＝原样）”滑块（`commentGlassTintChanged:`，开关关掉时禁用）。

**消息页顶部三颗（`MessageTopGlassV3.xm`）**
- 原因：`AWEMessageRegularGlassConfiguration` 是 `ALBMaterialButtonConfiguration`（装右侧那份玻璃：清透＋染色或登记配方，会按背后亮度翻），`AWEMessageApplyAdaptiveStyleToGlassButton` 是 `ALBMaterialApplyButton`（定死深色）。新出现的玻璃没读数时系统当暗，约 0.25 秒后第一次读数到才变浅 → 每次进来先深后浅。
- 3.11.44：配置改成系统 `glassButtonConfiguration`，override＝按钮所在场景的深浅（没有就 `AWEMessageResolvedAdaptiveInterfaceStyle`），玻璃键固定 `system-regular/3.11.44`（旧配置换掉一次）；深浅变化写探针日志 `MESSAGE_GLASS_STYLE`。

**三段式底栏右边两块（`ALBDock.m`、`ALBDockMaterial.m`、`ALBDockHooks.x`）**
- 底栏文件从 3.11.33 起没改过；3.11.33 加的背后亮度探针让视频页的深浅会切。左边是系统 `UITabBar` 的胶囊（没开清透＝UIKit 原厂玻璃，`nativeBar.overrideUserInterfaceStyle` 跟着设），右边两块是自建 `UIGlassEffect` Regular（开清透＝Clear＋染色，系统胶囊也 KVC 换成同一份）。深浅档一直是同一个，所以怀疑两点：① 没开清透时，自建 Regular 的深色比原厂胶囊的深色重得多（3.11.0 用户就说过常规玻璃＝那个深色调）；② 开清透时，UIKit 重建胶囊（换深浅、换页、重新布局）会把 `glassEffect` 换回原厂，而“装过”的记号还在，之前只在换深浅 0.2 秒后核对一次（而且读回来是副本，指针永远不等）。
- 3.11.44：① 第一次见到系统胶囊时把原厂 `glassEffect`（清透下用 kOrigKey 存的那只）复制一份记着（`gALBDockNativeGlass`），没开清透时 `ALBDockGlassGroup -applyMaterial` 用它的副本；② `-verifyNativePlatterGlass`：同步定时器（0.35 秒）里，清透时读胶囊玻璃的 `tintColor` 和应有的比（`ALBDockSameTint`），不同就当被换回原厂、重装；重装后读回来还对不上＝读回的副本不带染色，停止核对（免得每 0.35 秒重装一次）。
- 诊断“三段式底栏”：“玻璃同源”（原厂玻璃的类 / style / 可按压、自建三块用原厂同款几次、清透时被换回再装几次及核对状态、胶囊 / 中栏 / 朋友球现在的效果类）、“深浅”（应为、系统底栏设的、胶囊实际、中栏 / 朋友球设的和实际）、“深浅变化”（`ALBDockMaterialLog`，最近 12 条：系统深浅、背后亮度、页面、读数、四块玻璃→深/浅、读到原厂玻璃、被换回再装）；系统底栏结构树每个效果视图后加“深浅=设的/实际”。
- 下一步：用户实测还有右两块和左边不一样，就要诊断看是哪种（效果类不同 / 深浅不同 / 被换回次数）。

**图文页评论框**：没动。**长按面板（ALBSheetGlass.xm）、右侧、液态底栏**：没动。

## 0.45 3.11.43（用户看了 3.11.42，附 5 张截图：“这都是些什么乱七八糟的玩意儿？我说的，你对尺寸没有把握，就直接用 参考实现 的方案，只不过把玻璃换成 regular，然后让它调整过的那些图标能够跟随玻璃变形就行。虽然质感上现在没有错，可是这莫名其妙的这些背景滑动或者下拉时又不跟随玻璃，而是跟随面板，后面的那块玻璃停在那个地方……我感觉大致的图标位移还是需要的呀……图文作品这个要么就只保留底部的那个输入框是液态玻璃就可以了，别的都不动了。聊天页的坐标数据就用 参考实现 的那个默认，也就是头像不局中的那个就行。给消息未读数加一个胶囊……转发页我点起来的时候好多按钮都是白变黑……想把它降下来的时候，它玻璃停在那里不动……评论区的分享面板也是，还有那个聊天加号展开，就不太行”）

**教训**：拿不准的尺寸、位置不要自己拍；参考实现 源码在手（scratchpad/ext/dyk，公开项目 HierarchThurs/参考实现），照它的几何做，只把“下面垫会按压的玻璃”换成“图标画进系统玻璃按钮”。面板的玻璃要挂在随面板移动的那张底里（它的做法），不要挂在毛玻璃视图旁边。玻璃深浅定死，别用跟着背后亮度自己翻的材质。

**3.11.43**：
- `ALBChatFace.h`：胶囊的玻璃改成系统配置 `glassButtonConfiguration`（有颜色时 `prominentGlassButtonConfiguration`＋`baseBackgroundColor`），`spec.radius`（固定圆角，`cornerStyle` fixed＋`background.cornerRadius`）、`spec.tint` / `autoTint`（原底色饱和度 ≥0.35）、`spec.tapControl`（落点下不是别的按钮时这颗认点按，补发 TouchUpInside 的范围按玻璃外扩 16）。配置变了（染色、圆角）重做并重画。`ALBBackingClear/Restore`（同 ChatGlassStore 的 clearBackingOfView）。加号格子：去掉 3.11.41 的 face，清 `_backgroundColorView`/`_backgroundContainer`/`_blurView`（没有这几个成员就清 50–96 见方、有底色或毛玻璃的块）。顶栏里的按钮（往上有类名带 NavigationBar 的）不穿 face。删 `ALBChatAdoptHit`、`ALBChatFaceSyncBack`。
- `ALBMoreGlass.h` 重写：
  - 聊天顶栏（ChatTopDefaultLayout）：顶栏＝标题往上第一个类名带 NavigationBar/NaviBar 的抖音视图，carrier＝它的 `contentView`；行高＝carrier 高（>56 时取 44、以标题中线为中）。元素：类名带 NavigationUnreadView 的（返回＋未读）、`AWEIMMessageListNaviBarTitleView`（`avatarStackView`/`titleContainerStackView`，没有就排列子视图 0/1）、标题右边的 UIControl（≤60）。返回的内容＝返回键图里不透明像素的范围、两端按行高取圆端（ALBTopRoundEnds），有未读数（`unreadCountView` 或带字的子视图）并上数字；宽：返回＝内容宽，头像 / 按钮＝行高，名字＝内容宽＋16（最少行高，最多剩下的宽）；x 从 8 起、间距 8，右边按钮整体贴右。每个原视图 transform 平移到槽位中心，祖先和宿主关裁边（记着）。胶囊：返回＋未读（宿主＝未读视图，原内容＝它的子视图，数字的底去掉，`tapControl`＝返回键）、头像 / 名字（宿主＝标题视图，原内容＝那一栈）、按钮（宿主＝按钮，原内容＝按钮的子视图，墨迹居中）。顶栏的类在第一次见到时运行时挂 layoutSubviews（有状态才同步）和 hitTest（落在胶囊里的点给胶囊——胶囊比原按钮大、挪了位置）。触发：标题 layoutSubviews / didMoveToWindow、未读视图（Tweak.x 钩子改调 `ALBMoreTopBarSyncFrom`）、顶栏布局。
  - 表情页签、搜索（ChatEmojiTabGlass）：选中垫白 12%（深）/ 18%（浅），搜索右移 4；深浅＝页面。
  - 表情 / 加号面板（ChatInputGlass）：`AWEIMEmoticonPanelContainerView`、`AWEIMPlusPanelView`（layoutSubviews / didMoveToWindow / setHidden: / updatePanelVisible:）：容器和面板里满宽（≥99%）的底去掉；玻璃挂在面板自己里面最底下（卡片＝左右、下各内缩 8，上 22、下 `containerConcentricRadiusWithMinimum:22`）；加号面板挂 mask（下圆角＝屏幕圆角−8）；输入页父视图里的 `AWEUIDynamicBlurEffect` 去掉，每 0.4 秒看屏幕上还有没有卡片，没有就还原。`AWEIMChatPanelCollectionViewCell __configCellBackgroundColorWithColor:` 也清格子。
  - 分享面板（SharePanelGlass）：`AWESharePanelViewController` viewDidLayoutSubviews / awe_themeReload：它的视图不透明就去底、最底下挂玻璃（autoresizing 跟着它，上圆角＝外壳 DUX 圆角、最少 20，下直角），深浅＝原底色亮度（≥0.6 浅），记在视图上，面板里的胶囊都按它（`ALBMorePanelTone`）；关闭键（accessibilityLabel“关闭”，≤48）自己换 `glassButtonConfiguration`（胶囊、无内边距、图＝原图去白底，读不到原图不换），原配置 / 图 / 深浅记着。功能格子：图标一圈胶囊、去白底。输入层：去底、藏顶上细线、垫玻璃（评论的分享面板里只去底）。发送键：圆角跟按钮（没有 8），原底色饱和或标题“发送”（不含“建群”）→ 红色染的玻璃（#FE2C55），按钮配置自己画的满幅 Background 子视图不画。联系人不动。在多功能插件长按面板里（控制器名带 LongPress 或 `ALBSheetKindForView`==2）不动。
  - 评论的分享面板：`AWECommentLongPressPanelSwiftImpl.CommentLongPressPanelSheet` viewDidLayoutSubviews / viewWillAppear:，行布局时也带一次：外壳（sheet.view 下的 DUX）contentView 里满幅不透明的底 → 同上；列表控制器（CommentLongPressPanelCollectionViewController）里输入层露出来时，罩住它的那块白垫 → 玻璃（直角）。行：同 3.11.42（卡片矩形上下让 3、填充去掉、细线藏），深浅＝面板（找不到用系统）。分区底照样藏。
  - 图文详情页：只剩 `CommentInputView`（关注、四个按钮、顶栏扫描删掉）。
- `ALBSheetGlass.xm` 回到 3.11.41，另外控制器名带 SharePanel（且不带 LongPress）的不碰；加 `ALBSheetKindForView`（套了玻璃的长按面板里返回 2）。
- 设置三行的说明改了（默认都开）。

**要看的诊断**（【更多玻璃】）：“聊天顶栏同步 N 次、现在 M 块（行高 X：返回+未读 头像 名字 按钮类名…）、点在胶囊上 K 次、挂钩的顶栏类”；“表情 / 加号面板卡片 N 张”；“面板整块底换玻璃 N 张”；“关闭键换玻璃按钮 N 个”；“没找到”的类。顶栏没排：看挂钩的顶栏类是不是空（39.9 的导航类名不带 NavigationBar），或者“现在 0 块”。

## 0.44 3.11.42（用户：“这分开做的意思是什么呢，你觉得我编译测试起来特别容易是吧有必要分开吗”）

**教训**：用户编译测试一次成本很高（推 GitHub、等 Actions、装、测）。能一起做的不要拆版本；拿不准的地方加诊断，而不是留到下一版。

**3.11.42**：
- `ALBChatFace.h` 胶囊改成按说明装：`ALBPillSpec`（host / frame / sources / plate / autoPlate / destain / center / style / area）→ `ALBPillApply(owner, spec, syncer)`；spec 为 nil 就拿掉。`ALBChatPillSync` 还是原来那几处（群功能栏、粉丝群小胶囊、按住说话）的默认说明。
  - **修了 3.11.41 的错**：原内容用 `layer.opacity = 0` 藏起来以后 `alpha` 读出来也是 0，`ALBChatPillSources` 下一次布局把它们当看不见滤掉 → 签名变了 → 画成空图。现在 `ALBPillVisible`：自己藏的（`state.hidden`）照样算看得见；原来的 opacity 记在 `state.opacity`，画的时候用它、拿掉时还原它。
  - 画：裁成胶囊；`plate` 先铺（`autoPlate`：原来的底色饱和度 ≥0.35 才画，alpha ≤0.9，“发送”“关注”的红）；`destain` 时图片视图自己按 contentMode 画、图过 `ALBPlateless`（外圈最多的不透明中性色＝底，从四边连通抠掉，抠掉 ≥10% 才算，结果挂在原图上）、不画它的底色；`center` 时按墨迹居中（`ALBInkOffset`）。画的时候关动画（`performWithoutAnimation`＋`CATransaction setDisableActions`）。
  - 深浅：spec 给的 > 字色（`ALBPillInkStyle`：浅字 → 深玻璃，深字 → 浅玻璃） > 页面（`ALBChatPageStyle`）。
  - 触摸：`ALBPillTouchTarget` 先找落点下原内容里的 UIControl（藏着的 alpha 不看），没有就 owner；owner 自己也画进去的（图片视图、文字），往外 4 层找包着它的 UIControl 认点按。UIControl 都用 `ALBFaceTapOnTouchUp` 那套：自己没发 TouchUpInside 就补一次。原内容上的点按 / 长按手势（类名不以下划线开头）搬到玻璃按钮上（`state.moved`），拿掉时搬回去。抬手后 0.15 / 0.6 秒自己和同一排重画（`ALBPillResyncSoon`）。
  - 滚动：`UIScrollView touchesShouldCancelInContentView:` 钩子（UICollectionView / UITableView 自己另写的也钩）对 `ALBChatPillButton` 和聊天页的 face 返回 YES——不然从玻璃按钮上开始滑列表滑不动（UIControl 默认不让取消）。右侧、双击面板的 face 不受影响。
  - `UIImageView setImage:`：有胶囊在屏幕上时（`gALBPillLive`）往上 6 层找装了胶囊的，下一拍重画一次。
  - face 分处开关（`ALBChatFaceMarkArea`：sheets / note）、去底（分享关闭键）；`ALBChatAdoptHit`：没类可钩的按钮只换这一个对象的运行时子类（hitTest→`ALBFaceAdoptHit`、layoutSubviews→`ALBChatFaceSync`）。
- 新 `ALBMoreGlass.h`（Tweak.x 在 `ALBChatFace.h` 之后 include；ctor 里 `ALBMoreGlassInstall()` 运行时挂钩：类存在、方法存在才钩，原方法跑完再同步；没找到的类记在诊断里）：
  - 聊天顶部标题 `AWEIMMessageListNaviBarTitleView`：按墨迹（字按实际字宽）一块胶囊，高 ≥32。
  - 表情面板 `AWEIMEmoticonTabCollectionViewCell`（layoutSubviews / didMoveToWindow / setHighlighted: / setSelected: / `__refreshSelectedUI`）：左右各让 2；选中（`userSelected`）垫 16% 白（浅页 7% 黑）。`AWEIMEmoticonPanelBoxView`：`_horizontalStackView` 里除页签以外的入口一颗圆，直径＝页签高，墨迹居中。只在聊天页（响应链控制器名带 AWEIMMessage / IMChat、不带 Comment）。
  - 评论长按面板（控制器名带 CommentLongPressPanel）：行 `CommentLongPressPanelNormalBaseCell`（找不到用 AWECommentLongPressPanelSwiftImpl 的两个名字）：卡片底＝行里（3 层内）不是视图的层、填充 alpha ≥0.5、≥100×24；胶囊＝卡片矩形上下各让 3（没有卡片：行内缩 16、3）；卡片填充清掉（每次）、同层细线（高 <1.5、宽 ≥100）藏起来，拿掉时还原。`AWEBaseListSectionBackgroundView`：opacity 0。
  - 联系人 `AWEIMShareImpl.ShareContactListTargetCell`（评论长按 / 分享面板里）：上半部最大的近正方形（34–84）＝头像，胶囊外扩 ≤4。
  - 分享面板（控制器名带 SharePanel）：`AWESharePanelViewController` viewDidLayoutSubviews / awe_themeReload → 找 accessibilityLabel“关闭”的 UIButton（≤48）→ face（去底、只换命中）。`AWESharePanelFunctionCell`：`imageView` 那块（没有就最大的方块）一颗圆、`destain`。`AWEIMShareImpl.ShareAdditionTextView`：不透明底 / effect 去掉，最底下垫不按压的玻璃（深浅跟原底）。`...ShareAdditionTextButton`（采集到的完整类名）：胶囊、`autoPlate`。
  - 图文详情页：`FullPageFeedRelationButton`（三个名字试）胶囊 `autoPlate`；`FullPageFeedBottomBarView.CommentInputView` 胶囊（里面有能打字的框就不盖）；`InteractionItemView` 每个一块（左右让 2）。顶栏没有类：从关注键 / 评论框往上找页面（控制器名带 Knowledge / FullPage / RichContent 的最上面那个，没有就第一个差不多整屏高的），每页 0.5 秒最多扫一次，扫窗口顶 150（不进滚动视图）：近正方形图 18–30 → 36 的圆（墨迹居中）、30–48 → 外扩 4、头像右边同一行最近的字 → 胶囊（字宽 ＋20，高 ≥32）。它们自己画进玻璃，玻璃放在往上第一个接触摸的视图（裁边又小于 44×40、身上没手势的跳过）。
  - 设置改了：屏幕上的胶囊、face、分区底、输入层各同步一次。
- `ALBSheetGlass.xm`：控制器名带 CommentLongPressPanel（以前随“Comment”一起不碰）或 SharePanel 的面板也套玻璃，开关是 `bind.sheets`；认不出外框（没有“点空白处关闭”那层）时，认得出这两个名字就用往上第一个圆角视图（没有就毛玻璃视图自己）。`ALBSheetClearPlates`：毛玻璃视图的父视图里，和面板差不多宽（≥90%）、高 ≥40、在面板里的不透明底（≥0.9）记着、清掉，第一块的亮度 ≥0.6 → 浅色常规玻璃（`overrideUserInterfaceStyle` Light），否则还是评论区那种深色；布局时再清（抖音会设回来），还原时放回。`ALBSheetToneForView` 给面板里的胶囊用。评估时机多一个 0.8 秒。
- 设置「更多玻璃」：`bind.chatInput`（聊天页，含标题、表情面板）、`bind.sheets`、`bind.note`，都默认开；设置页卡片按钮改名“聊天页、评论长按与分享面板、图文页”。
- 聊天顶栏右边几颗**不合并**：语音 / 视频那几颗的长按、拖动手势挂在抖音自己的按钮上，一块玻璃盖住几颗就只能把触摸转给一颗，手势收不到；要合并得在 `ALB3Chrome.swift` 加多按钮的栏（一个导航栏放几颗栏按钮），每颗还是各自的控件，以后用户要再做。

**要看的诊断**（【更多玻璃】）：胶囊装上 N 次后面括号里每处的数量；“没找到”列出这个抖音版本里不存在的类（39.9 和参考实现的 40.8 类名有出入）；“胶囊按下 / 给原内容里的按钮 / 补发动作 / 搬过来的手势 / 列表放行”；“去白底 N 张（不是那种图 M 张）”；【长按面板】里“评论长按 / 分享面板 认出 N 次、去掉整块底 M 块”。

**下一步**：等用户实测。如果某处点不动：看是不是手势在原内容的父视图上（搬手势只搬原内容自己和下面的）；某处深浅反了：面板看【长按面板】去底的亮度，胶囊看字色；某处没装上：看“没找到”的类名，用采集工具抓 39.9 的真实类名。

## 0.43 3.11.41（用户实测 3.11.40，附群聊截图：“有效的，别的地方也做吧，左上角返回，如果你没做的话，应该就是有点冲突，他现在也有玻璃，而且图标错开了，玻璃大小形状，如果你觉得抓到的数据还拿不准的话，参考参考实现，因为我不知道你说的只做了这些，这次正式做的时候会不会处理掉，比如现在输入框这个区域的这条长背景”）

**3.11.41（`ALBChatFace.h`＋Tweak.x hook）**：
- `ALBChatCenteredImage`：模板图按不透明像素（alpha>24）的范围在原画布里平移居中（偏差 ≥1.5px 才画），结果挂在原图上缓存；face 的图标都过一遍。`ALBChatFaceIcon` 往下两层找最大的图，带回图标边长；≥40 的近正方形按钮（加号面板 72）图标取它自己的边长（≤60%）。
- `ALBChatPillButton`（UIButton，配置＝`ALB25MaterialMotionButtonConfigurationTone(NO)`，capsule，图片＝原内容画成的一张图）：touches 四个阶段先 super（系统按压）再转给 owner；`gestureRecognizerShouldBegin:` 对祖先的手势一律 YES；抬手后 0.15 秒同一排有胶囊的重画（选中变化）。`ALBChatPillSync(owner)`：不在窗口里不动；底色记着去掉（cell 的 contentView 底色也清）；原内容（cell 的 contentView 子视图 / 其余是自己的子视图）签名（字＋颜色、图指针、大小位置、底色透明度）变了才 `renderInContext` 重画（画时临时 opacity 1）；原内容 opacity 0；胶囊铺满 owner、最上层；深浅＝`ALBChatPageStyle`。用在：`AWEIMOptimizeAudioInputTouchView`（已有 hook 加 layoutSubviews）、`AWEIMGroupImpl.GroupZoneEntryItemCell`（Swift 类，group `ALBChatZoneGroup` 占位名 `ALBGroupZoneEntryItemCell`）、`AWEIMInputActionBarFansGroupEntryViewCollectionViewCell`（group）。
- `ALBChatPanelCellSync`：`AWEIMChatPanelCollectionViewCell`（group：layoutSubviews、didMoveToWindow、configWithModel:、addBlurView）里 ≥40 的 UIButton 穿 face。
- `ALBChatFieldSync`（`IESIMGrowingTextView` 加 layoutSubviews，didMoveToWindow 里也调）：往上 3 层（各展开一层）找和输入框对齐（±6）、半透明（0.02–0.95）的块：去底色（图片视图则藏），按住说话那类跳过；输入框里最宽的 UIScrollView 当文字区，左右 ±6，再让开附近输入栏按钮（外扩 4＋4）；高＝输入框高−4（32–120），圆角 min(20, 半高)；非 interactive 的 Regular 玻璃插在输入框下面。
- face / 胶囊在不在窗口时都不动（不再拆了重装）。

**下一步**：顶栏标题（头像＋群名＋公开：胶囊，内容会异步加载，要多重画几次）、表情面板页签（选中态）、评论长按面板逐行和分享面板（参考 参考实现：分享评论面板每行胶囊上下各缩 3、头像一圈、功能格圆钮、关闭键）、图文详情页；右边几颗合成一条胶囊（多按钮栏）；需要时把聊天顶栏底色去掉、消息铺到栏下。

## 0.42 3.11.40（用户：“还是不变形，不是，我不是跟你说过了吗？那头像怎么做的？难道你忘了吗？音乐图标怎么做的？你忘了吗？那个做法是用不到这里吗？而且消息页面这个的顶部那三个按钮我也做了呀。你用那种做法直接开工就行了呗，难道还缺什么东西吗？还有三段式底栏的那个读取头像一直是错误的，插件设置本来就能读取头像，把那个修正一下。”）

**教训（写给接手的人）**：要“图标、文字跟着玻璃一起变形”，就用右侧那套——真正的系统栏按钮（`ALB3BarGlassButton`，`ALBBarFace.h`），内容是这颗栏按钮自己的图片 / 标题（头像、音乐封面是先画成一张图再交给它，`ALBInstallNativeArtwork` / `ALBFaceNoteSpec` preserve）。在按钮下面垫 `UIVisualEffectView` 玻璃、内容用 `_UIPortalView` 放进去（3.11.38/39 `ALBBindGlass.xm`）带不动：系统按压的形变在玻璃材质内部，图层几何读不到（诊断 6/7 “没看到变形”）。已删。

**3.11.40**：
- 新 `ALBChatFace.h`（Tweak.x 在 `ALBBarFace.h` 之后 include，用它的 static 函数；Tweak.x 里新 hook：`AWEIMInputVCBadgeButton`、`AWEIMInputVCUnhighlightedBadgeButton`、`AWEIMMessageVoipButton`、`AWEIMMessageBaseVCMoreButton` 的 layoutSubviews / didMoveToWindow / hitTest，`AWEIMMessageVCNavigationUnreadView` 同样三个（同步里面那颗返回 UIButton）；`UIImageView setImage:` 钩子里加 `ALBChatFaceIconChanged`）。抖音按钮当 owner：`ALBFaceNoteSpec`（图：按钮子视图里最大的 UIImageView 的图，否则 currentImage；模板图 preserve=NO 用系统墨色，彩图 preserve=YES；fixedPoint 22 / 20，长按钮 高−12）→ 新建 face（`kALBFaceKey`＋`kALBChatFaceKey`，`ALBFaceTapOnTouchUp`）→ `ALBFaceApplySpec` / `ALBFaceSyncStyle` → `ALBChatFaceLayout`（face 最上层；近正方形 ≤40 外扩 4（≥28）或 5，否则同大；`ALBFaceHideButtonParts`、`ALBFaceSyncActions`（`ALBFaceTouchAdopt`）、`ALBFollowSyncEdge`）。hitTest 结果过 `ALBFaceAdoptHit`。条件：`bind.chatInput` 开、iOS ≥26、`ALBBarFacesWanted()`、插件启用、按钮在窗口里可见；否则拿掉（还原按钮）。诊断【更多玻璃】改为 face 的统计。
- 删 `ALBBindGlass.xm`（Makefile）、`bind.chatGroup` 设置；“按住 说话”恢复原样。
- `ALBDockAvatar.m`：`+image` 顺序改为 手动（source 3）> `ALBDockHostOwnAvatar()`（＝`ALB8OwnAvatar`，设置页个人卡片那张）> 缓存；本人头像有了就不再 `refreshFromAccount` / `scanWindow`。

**下一步**：用户确认聊天页按钮变形、点击正常后：带字的长条（按住说话、群功能栏、粉丝群小胶囊）用双击面板的文字胶囊 face（`ALBFaceNoteSpec` title）；右边几颗合成一条胶囊（一个导航栏放几颗栏按钮，系统会让它们共用一块玻璃，需在 `ALB3Chrome.swift` 加多按钮版）；再铺评论长按面板、分享面板、图文详情页。

## 0.41 3.11.39（用户实测 3.11.38，附截图和诊断：“不跟随，而且玻璃的取色有问题。它进去的时候明明是深色页面，但是玻璃全部是浅色的，需要我长按之后它才切换回深色。另外，我感觉右侧这三个按钮没有必要用三块玻璃吧，感觉有点太挤了，用一块胶囊也行，长按面板圆角问题没有了，现在就是不变形的问题”）

诊断：【长按面板】设路径时当场换 0、**设路径时还没挂上 87 次→提交前补上 18 次**、回调加不上 0 → 3.11.38 的判断对，问题解决。【更多玻璃】传送门有、flex 入口有；装上 8 次；按压 7 次：自己跟着动 0、插件跟随 0、**没看到变形 6**。玻璃结构（玻璃入口读数）：`ALBBindGlassView › UIKit._GlassGroupView › GlassGroupLayerView › _UIMaterialDefinitionView › UISDFBackdropView`。参照：系统底栏的选中透镜 `_UILiquidLensView`（身上装 `_UIFlexInteraction`）里面自己放了一个 `_UIPortalView`——系统也是靠传送门把内容放进会变形的透镜。

**3.11.39（`ALBBindGlass.xm` 重写，只这一个文件和设置）**：
- 共用的 `ALBBindUnit`（玻璃、传送门、按压）；单颗 `ALBBindState`（宿主里面），胶囊 `ALBBindGroup`（父视图里）。
- 按压：按下（钩宿主 touchesBegan）时把玻璃除内容层外每一层（≤10 层深、≤300 个）的窗口位置记成基准；CADisplayLink 每帧：玻璃本身 / 内容层有 transform 或玻璃大小变了 → 内容自己跟着（不动）；否则找与基准偏差最大的层（≥玻璃 30% 大），把它的位移和宽高比例换算到 holder（以玻璃中心为准：T = (C.mid − H) + s·(H − B.mid)）；0.08 秒内一层都没变 → `startOwnPress`：整块玻璃 transform 弹到 ownScale（单颗圆 1.10、长条 1.03、胶囊 1.05 并往按到的那颗偏 ≤4pt），松手弹回。诊断：每次结果、按压时玻璃里各层的动画键、玻璃里 / 按钮上类名含 Flex 的交互和手势。玻璃位置改用 bounds＋center 设（不碰 transform）。
- 深浅：`overrideUserInterfaceStyle` = 页面深浅（`ALBBindPageStyle`：往上 12 层第一块不透明底色的亮度 <0.5 为深，找不到用系统的）。
- 胶囊（`bind.chatGroup` 默认开）：同一父视图、同一排（中线差 ≤6）、宽 16–64 高 16–44、间距 ≤20 的两颗以上按钮 → 一块非 interactive 玻璃插在它们最低的那个下面，范围＝它们的并集左右各外扩 6、上下外扩到最高 44，圆角＝半高；每颗按钮整个用传送门放进去；成员各自的单颗玻璃拿掉；不再成排时拆掉胶囊、其余成员重新布局。
- 设置第 9 节加“挨着的几颗按钮合成一条胶囊”。

**下一步**：看诊断里按压走的是“插件跟随”还是“插件自己按”，以及按压时的动画键——如果系统形变能读到就只留跟随；读不到就只留插件按压（并去掉玻璃的 interactive，免得系统和插件各弹一次）。然后铺开（顺序同 0.40）。

## 0.40 3.11.38（用户发来第 8–16 段：“有的我感觉可以忽略吧，你像弹幕设置这种地方要玻璃干球啊……这次应该都抓到了，你开工的时候记得注意一下。我感觉他们的那些玻璃好像就是点了一下，有时候会莫名其妙动的那种，然后边缘又跟圆角不匹配……另外自己的长按面板，实际上我们已经做了，而且已经比它的那个效果要好了，就是之前的圆角离屏会失效，进来后又会再次变回设置的圆角，这个问题还是有”）

**补采（3.11.37 工具）**：
- 8 评论长按面板：整块（`_UIVisualEffectContentView` 440×532 @y424）→ 拆开：`ShareContactListTargetCell` 68×95 ×6（头像 56 r28、名字 PingFang-Medium 12、火花 `AWEIMStreakFlameView`）、`CommentLongPressPanelNormalCell` 440×52 ×6（y 587/639/691 一组，755 单独，819/871 一组；图标 20×20 @x32（第一行 24 r12），文字 SFUI 16 #161823 @x64，举报 #FE2C55）。分组灰底 `AWEBaseListSectionBackgroundView`（不是格子，拆不出来）。
- 9 弹幕设置面板（`AWEDanmakuSettingsPanelView` 440×551）：用户说不做。
- 10 分享面板（`AWESharePanelContainerViewController`，`DUXVisualEffectView` 440×334 r20 @y622）：关闭 `UIButton` 28×28 @(396,637)、头像排 6 个、功能格 `AWESharePanelFunctionCell` 68×81 两排（转发到日常 / 推荐 / 私信 / 分享至群 / 帮上热门 / 分享链接；一起看视频 / 生成图片 / 更多分享 / 加桌面伙伴 / 举报 / 播放反馈）。
- 11 点头像后的输入层：`AWEIMShareImpl.ShareTextView` 342×68、发送 `ShareAdditionTextButton` 408×44 r8、头像图 56×56 r4。
- 12 群聊输入区：`IESIMTextViewInternal` 251×44（在 `IESIMGrowingTextView` 424×44 r8 里）、`AWEIMInputVCBadgeButton` 30×30（语音 @x310 / 表情 @x352 / 玩法 @x15 r15）、`AWEIMInputVCUnhighlightedBadgeButton` 30×30 更多 @x394、按住说话 `AWEIMOptimizeAudioInputTouchView` 424×44 r8 底 15% 白、粉丝群小胶囊 `AWEIMInputActionBarFansGroupEntryViewCollectionViewCell` r8 底 15% 白（YYLabel：晚上好 / 比心 / 群聊AI / [赞] / [捂脸]）。
- 13 表情面板（输入栏上移到 y≈531）：搜索表情 `AWEIMInputVCBadgeButton`、发送 `AWEIMInputVCUnhighlightedBadgeButton` 54×34 r17、页签 52×38（`AWEIMEmoticonDefaultEmoticonTabCollectionViewCell` / `AWEIMEmoticonTabCollectionViewCell` / `AWEIMEmoticonInteractiveTabCollectionViewCell`）、表情格 68×68、面板底 #161616。
- 14 加号面板：`AWEIMChatPanelCollectionViewCell` 72×100（按钮 72×72 r16 里一层 `AWEUIDynamicBlurEffect` 模糊、图标 36、名字 SFUI 14 在下面）：相册 / 拍摄 / 红包 / 位置 / 我的喜欢 / 接龙 / 打卡 / 文件。
- 15 图文详情页（`AWEKnowledgeRichContentImpl.FullPageFeedTabListViewController`）：返回 24、头像 36 r18、名字、`FullPageFeedRelationButton` 64×28 r4「已关注」、搜索 24、底部 `FullPageFeedBottomBarView.CommentInputView` 212×40 r20、`InteractionItemView` 52×41 ×4（772 / 29 / 87 / 100）。
- 16 页面：特别关注 `AWESkylightUnreadListViewController`（`_vcEnterForm=homepage_follow`）、观看历史 `BDXViewController`（`config → BDXSchemaParam._originURL = aweme://lynxview/?…channel=watch_history…`）、订单 `IESECOrderListViewController`（`_pageName=order_list_page`）、新关注我的 / 互动消息 `AWEIMFansAndInteractMergeViewController`。

**3.11.38**：
- `ALBSheetGlass.xm`：setPath 钩子认不出（图层没挂上、或所在视图不在窗口里）而路径大小像模块底（≥70×30、不超过屏宽−8）的，记进弱表；本次 `CATransaction` 提交前（`+addCommitHandler:forPhase:1`，私有，WebKit 同用；加不上就 `dispatch_async` 主线程）再用 `ALBSheetShapeInPanel` 认，认出来 `ALBSheetApplyShape`。在窗口里又不在面板里的直接放过。诊断加“设路径时还没挂上 / 提交前补上 / 回调加不上”。
- 新 `ALBBindGlass.xm`（Makefile 加入；设置第 9 节“更多玻璃”，`bind.chatInput` 默认开；主页“更多玻璃”卡片 → `openBindOptions:`；诊断【更多玻璃】`ALBBindGlassReport`）：宿主 `AWEIMInputVCBadgeButton` / `AWEIMInputVCUnhighlightedBadgeButton`（圆钮：≤40 近正方形 → 每边外扩 6 的圆，否则宿主大小＋宿主圆角）、`AWEIMOptimizeAudioInputTouchView`（长条：宿主大小＋宿主圆角）。玻璃 `ALBBindGlassView`（`UIGlassEffect` Regular，interactive，`userInteractionEnabled=NO`，`hitTest` 返回 nil，`_flexInteractionGestureView` 返回宿主——另一个插件就是这样让玻璃在按到按钮时拉伸的）插在宿主 index 0（抖音也往最底下塞时最多让 20 次）。宿主每个看得见的子视图一个 `_UIPortalView`（`hidesSourceView`、`matchesAlpha`，不 matchesPosition / Transform）放在玻璃内容层里的 holder 上，位置按源视图同步。宿主底色：≥0.5 不透明的变成玻璃染色，淡的直接去掉，都记着（`setBackgroundColor:` 钩子替抖音记着），关掉还原。按压（钩宿主的 touchesBegan/Ended/Cancelled）期间 CADisplayLink：玻璃本身或内容层有形变 → “内容自己跟着动”；否则在玻璃 3 层内的层里找形变最大的（至少玻璃一半大），把它相对静止时的位移和宽高比例抄到 holder（松手后再跟 0.9 秒）；诊断记每次按压结果。
- 没动：玻璃配方、底栏、右侧、顶栏、评论区、长按面板背景。

**下一步**：等用户实测——模块圆角是否还先旧后新（看诊断“提交前补上”）；聊天输入栏的按钮按下去图标是否跟着变形（看“更多玻璃”按压结果）、点击 / 长按是否正常、边缘是否对齐。没问题就按同样办法铺：聊天顶栏（返回＋未读、标题、通话、更多；需要把顶栏底色去掉）、群功能栏、粉丝群小胶囊、输入框（不按压的静态玻璃）、表情页签、加号面板格子（替换它的模糊层）、评论长按面板逐行（面板背景也要套上长按面板那套玻璃）、分享面板（头像加一圈、功能格的圆、关闭键）、图文详情页。聊天页要让玻璃看得出来，还得让消息铺到输入栏下面（另一个插件的做法：输入区悬浮、消息从下面经过）。

## 0.39 3.11.37（用户发来 7 段采集 JSON：“你看一下抓到的够不够吧，应该要跟那个 参考实现 的原项目对照，能补齐就行，如果哪个部分缺了的话，记得告诉我，我再去抓一遍。”）

**7 段里有什么**（抖音 39.9.0，iOS 26.6）：
1. 评论区长按面板（`AWECommentLongPressPanelSwiftImpl.CommentLongPressPanelSheet` › `CommentLongPressPanelCollectionViewController`）：用户点了 3 行，3 次都被框成整块 `DUXVisualEffectView` 440×532（圆角 20，effect 无，浅色面板、字 #161823，“举报”#FE2C55）。整块里文字 / 图标坐标是全的：行 `CommentLongPressPanelNormalCell` 440×52，图标 20×20 在 x=32，文字在 x=64；分组灰底 `AWEBaseListSectionBackgroundView`（三行一组 440×156）；“分享评论给”一排 `AWEIMShareImpl.ShareContactListTargetCell` 头像 56×56＋名字。缺逐行的胶囊。
2. 群聊页：顶栏（`AWEIMMessageVCNavigationUnreadView` 57×24、`AWEIMMessageListNaviBarTitleView` 277×36 里头像＋群名＋人数＋“公开”、右边 r12 的小块带 12×12 图标）、群功能栏 `AWEIMGroupImpl.GroupZoneEntryItemCell` 72×26（聊天 / 公告 / 置顶 / 收藏 / 设置，只有字）、输入框上方粉丝群入口 `AWEIMInputActionBarFansGroupEntryViewCollectionViewCell` r8（字是 YYLabel，被当成了图标）。输入区（语音键 x≈40、右边键 x≈337、输入框）点了 37 次，全框成 `UIKit._UITabBarContainerWrapperView` 440×83（y=873）。
3. 私聊输入区（`IESIMInputViewController`）：`AWEIMInputVCBadgeButton` 30×30（语音输入 / 表情 / 玩法 r15）、`AWEIMInputVCUnhighlightedBadgeButton`（更多面板，`IESIMInputPlusComponent plusBtnClicked:`）、`IESIMGrowingTextView` 424×44 r8；图标是模板图 30×30、白 0.9。
4. 加号面板 `AWEIMPlusPanelView`：`AWEIMChatPanelCollectionViewCell` 72×100 → `UIButton` 72×72 r16 → 图标 36×36；名字在按钮外面，没记到。
5. 私聊顶栏 `AWENavigationBar` 440×106：未读数＋返回、标题（头像 36 r18、名字、`AWEIMStreakFlameView` 火花）、`AWEIMMessageVoipButton` 24、`AWEIMMessageBaseVCMoreButton` 24。
6–7. 页面：`BDXViewController`（我 › 个人页 › Lynx 容器）、`IESECOrderListViewController`（`_pageName=order_list_page`，经 `AWEECMallFullScreenViewController`）、`AWEUserHomeViewControllerV2`（`_enterFrom=message`）、`AWEIMFansAndInteractMergeViewController`（消息页）、`AWESkylightUnreadListViewController`（首页，`_vcEnterForm=homepage_follow`）。没有一条“://”，也没有标题。
- 7 段里没有任何别的插件的视图 / 玻璃（采集时 参考实现 应该没开；也不需要开，见下）。

**两个工具问题**：① iOS 26 的标签栏 / 导航控制器在最前面盖着透明的悬浮栏容器（`_UIFloatingBarContainerView`、`FloatingBarHostingView<FloatingBarContainer>`、`_UITabBarContainerWrapperView`），触摸会穿过，但 3.11.35 按几何取“最前面那层”先拿到它们——第 1、2 段的错框都是这个。② `ALBCaptureModule` 只认主程序，抖音的类都在包里的 AwemeCore.framework，全记成“AwemeCore”，“旁边的玻璃”把不是“抖音”的都算成别的插件（输入框、关闭层被列成玻璃）。

**参考实现 对照**：源码是公开的（GitHub `HierarchThurs/参考实现`，0.7.1，2026-11-05 提交，README 写“DY测试版本 40.8.0”）。用户是 39.9.0，类名有出入（例：它的 `AWEIMNavigationUnreadView` ↔ 39.9.0 的 `AWEIMMessageVCNavigationUnreadView`），以采集为准。它的玻璃开关都是三档：关闭 / 模糊玻璃（系统 Regular）/ 清透玻璃（Clear）。除去评论区本身（用户要单独重做）、首页顶栏和底栏（我们有自己的）、它自己的设置页和提示面板，它盖的区域：

| 区域 | 它怎么做（都是在原生视图后面垫玻璃，图标文字不动） | 关键类（40.8.0） | 我们采到没有 |
|---|---|---|---|
| 作品长按菜单、弹幕设置面板 | 外壳 `DUXVisualEffectView` 的模糊换玻璃，行卡片常态底色去掉 | `AWEVideoLongPressPanelViewController`、`AWELongPressPanelSettingCell`、`AWEModernLongPressPanelUIConfig`、`AWEDanmakuSettingsPanelView(Controller)` | 背景 3.11.34 已做；逐行没采；弹幕设置面板没采 |
| 分享面板（含分享评论面板＝评论长按、分享成功提示） | 卡片底色、关闭键、第三行圆钮、点头像后的输入层、发送键、表情栏；评论长按每行一块胶囊 | `AWESharePanelViewController`、`AWESharePanelContainerViewController`、`AWESharePanelFunctionCell`、`AWEIMShareImpl.ShareAdditionTextView`、`AWEIMShareInputEmoticonToolBarView`、`CommentLongPressPanelNormalBaseCell`、`AWEIMBottomShareTipsView` | 评论长按只有整块；分享面板没采 |
| 聊天顶栏、群功能栏 | 返回 / 头像名称 / 通话 / 菜单各一块，群功能栏每项小胶囊；可选顶部渐变模糊 | `AWEIMMessageListNaviBar*Component`、`AWEIMMessageListNaviBarTitleView`、`GroupZoneEntryBarView/ItemCell` | 私聊、群聊都采到了 |
| 聊天输入区 | 输入框 / 按住说话为胶囊，独立按钮圆形，发送键粉色玻璃；输入区下移贴底 | `IESIMInputViewController`、`IESIMInputLayoutComponent`、`IESIMGrowingTextView`、`AWEIMOptimizeAudioInputTouchView` | 私聊按钮采到；群聊全框错；按住说话、发送键没采 |
| 表情 / 加号面板 | 面板为悬浮卡片；表情面板搜索圆钮、页签胶囊 | `AWEIMEmoticonPanelContainerView/BoxView`、`AWEIMEmoticonTabCollectionViewCell`、`AWEIMEmoticonFixedTab`、`AWEIMEmoticonThemesCollectionView`、`AWEIMPlusPanelView`、`AWEIMChatPanelCollectionViewCell` | 加号面板采到（缺名字）；表情面板没采 |
| 表情提示芯片、最近照片条、回到最新 / 新消息提示 | 每个芯片、每张照片格、圆钮 / 胶囊各一块 | `AWEIMInputActionBarEntryItemCollectionViewCell`、`AWEIMQuickPhotoPickerView`、`AWEIMAssetListCollectionViewCell`、`AWEIMPhotoAddMoreCollectionViewCell`、`AWEIMBottomNewMessageTipsImageView` | 没采（群聊采到的粉丝群入口是同一位置的另一种） |
| 小火人挂件 | 挪到输入框中间 | `AWEIMStreakPendantView` | 没采（可选） |
| 应用内通知横幅 | 白卡片换玻璃，回复键用系统玻璃按钮，角标正圆 | `AWEInnerNotificationContainerView`、`AWEInnerPushCommonView` | 没采（一闪而过，做的时候自动记） |
| 收藏提示 | 换玻璃，保留原生按钮 | `AFDSnackBar`、`AFDSnackBarCustomView`、`AWEPlayInteractionSnackBar`、`AWEFavoriteNoticeView` | 没采（同上） |
| 图文详情页 | 返回、头像、用户名、关注、搜索、底部输入框、互动按钮悬浮玻璃 | `AWEUGCKMPImpl.FullPageFeedKMPTopBarView/BottomBarView`、`AWEKnowledgeRichContentImpl.FullPageFeedCommentInputView`（DanceUI 画的） | 没采 |
| 清屏按钮、长按倍速提示 | 玻璃外观，保留原生内容和触摸（它只在自己的玻璃底栏开着时做） | `AFDRoundRectangleButton/BoxView`、`AFDPureModePageContainerViewController`、`AFDFastSpeedView` | 没采（倍速提示要按住才出现） |

**3.11.37（只改探针，`ALBProbeCapture.h`、`ALBBackgroundProbe.h`）**：
- `captureAt:`：从最前面的抖音窗口起问 `-hitTest:withEvent:`（nil 事件；插件窗口不问；窗口把触摸留给自己就停）；在候选（前到后、深到浅）里取第一个是命中视图本身或它子孙的；都不是（触摸范围比自己大的按钮）就用命中视图本身；命中不到才按几何，跳过 `ALBCapturePassThrough`（类名含 FloatingBar / _UITabBarContainer / UITransitionView / UIDropShadowView / UILayoutContainerView / UIViewControllerWrapperView / UINavigationTransitionView）。每项 `hitTest`、`origin`。新建项抽成 `addCaptureItemFrom:point:atPoint:hit:origin:level:`。
- “拆开”（`captureSplitTapped`，第一排第 5 个）：`ALBCaptureSplit` 广度遍历最后一项胶囊的子孙（≤900 个视图、≤40 项），取最外层 `ALBCaptureCapsuleScore`==2、≥16×16、在屏幕内的；里面还套着滚动列表的（`ALBCaptureHasInnerList`，4 层内）继续往里；按 y 再 x 排序，各建一项（level 0）。原来那一整块保留。
- `ALBCaptureNearbyTexts`：往上 3 层内最近的格子（没有就父视图，且面积 ≤ 胶囊 3 倍）里、胶囊外的文字，≤8 条 → `textsNearby`。`ALBCaptureCustomLabelText`：类名含 Label、不是 UILabel / 输入框，读 `attributedText` / `text`（@try）→ 文字（kind 自绘文字）。
- `ALBCaptureModule`：路径在 `/<App>.app/` 下且不是 .dylib 的算“抖音”；“旁边的玻璃”只算 `UIVisualEffectView`、类名带 Glass、或模块既不是抖音也不是系统的。
- 页面：`visibleTitles`（顶部 y<170 的 UILabel，字号大、靠中间优先，≤4）；`ALBCaptureRouteScan` 再往里一层：strong / copy、类型是抖音自己的类（非系统、非视图、非控制器）的属性，直接读背后的变量（不调 getter），同样找字符串；KVO 子类用 `[object class]`。
- 探针以外没改任何东西。

**下一步**：用户用 3.11.37 补采（清单在 README 3.11.37 和给用户的回复里），够了以后开始做“真玻璃”：iOS 26 用 Regular，按右侧音乐 / 头像的做法把原生图标、文字和玻璃绑在一起；参考实现 的源码只用来对照它盖了哪些区域和 40.8.0 的类名。通知横幅、收藏提示、倍速提示这类一闪而过的，实现时先加自动记录（出现时记结构到诊断），不靠手点。快捷跳转：先看补采里 `visibleTitles` / 深一层的跳转字符串，还不够就记打开页面时走的路由（需要另加诊断）。

## 0.38 3.11.36（用户：“不行，新工具好像劲儿用大了，会导致 app 闪退，再次启用也是；抖音自带的那个模块的圆角更改之后，如果离屏再回来，就又得变一次”；附 .ips）

闪退记录：主线程 `EXC_BAD_ACCESS`“Thread stack size exceeded due to excessive recursion”；栈：`UIAlertController` 选项回调（设置页 choice 行）→ `postNotificationName`（`ALB5.OptionsChanged`）→ 插件里的观察者 → 同一个函数反复调用自己（偏移 0xd83f8 → 0xd83d0）。原因：`ALBBackgroundProbe.h` 里我写了 `- (void)setCapturePaused:`，和属性 `capturePaused` 自动生成的设置方法同名（覆盖了它），方法体里 `self.capturePaused = paused` → 无限递归。`probe.mode`=1 存着，所以每次启动 `ALBProbeUpdateFloatingUI` → `syncCaptureMode` → 同样闪退。

**3.11.36**：
- 方法改名 `applyCapturePaused:`（属性用自动生成的设置方法）；全工程扫了一遍“自定义 setX: 里又写 self.x =”的写法，没有别的。
- `ALBSheetGlass.xm`：`%group ALBSheetShapeGroup` hook `CAShapeLayer -setPath:`（iOS 26 才 init）：`gALBSheetShapeHook`（`sheet.glass`＋插件启用＋`sheet.moduleRadiusOn`，设置变化时同步）开着、主线程、图层挂在视图上、往上 7 层内有类名含 `LongPressPanel` 的视图时，用 `ALBSheetModulePath`（从抖音这次设的原路径判断哪几个角圆，≥70×30、≤窗口宽−8）换成新半径的路径再交给原方法，并记下原路径/我们的路径/半径（关掉开关时定时器那条路还原）。`ALBSheetApplyShape` 也记半径：圆角设置变了从记着的原路径重做（3.11.35 改半径要等抖音重设才生效）；去掉了“有填充或描边”的条件（设路径时颜色可能还没设）。定时器 0.2→0.5 秒兜底。诊断“设路径时当场换 N 次”。

## 0.37 3.11.35（用户：“成功了，就是抖音自己的那个长按里的模块，不知道能不能一起把圆角改了”；下一步要给更多地方加液态玻璃：参考另一个插件（参考实现）覆盖的区域（评论区以外，评论区要单独重新做）——长按、聊天页、评论区里的长按页等；它只是给背景换/加了一块玻璃，玻璃本身能按压拉伸，但上面的图标文字不跟着动（“散架”），而且一直用清透（用户反感这个名字）。要做的是：用它覆盖的那些按钮/胶囊的位置，探针读出上面实际的图标、文字层，像右侧音乐、头像那样和真正的玻璃绑在一起；iOS 26 用 regular 不用 clear，iOS 27 以后再说。这次先升级探针去抓图标和文字层；自动抓不全就手动点：打开后只有探针收触摸、屏蔽系统点按反馈，点一个框一个表示命中，点完一段打包一段，可以临时关掉切页面继续抓；另外要抓一些快捷跳转页面，给那颗双按钮胶囊（三段式底栏中栏 ✦ / ↑）跳转用）

背景探针（3.11.34 起记控制器）：长按面板控制器 `AWEVideoLongPressPanelViewController` › `AWELongPressPanelTableViewController`，外层 `DUXBasicSheet`（在 `AWENormalModeTabBarController` 上弹出）；玻璃已套上（我们的 `UIVisualEffectView` 圆角 34.1＝用户调的面板圆角，`DUXVisualEffectView` 子图层只剩 1）。点到的模块：`AWELongPressPanelSettingCell`(440×52) › contentView › `UIView`(x16 408×52，圆角 0，无底色)；同 cell 里一层 440×52 的 `UIView` 带两个 `CAShapeLayer`(frame 0)——模块的灰底是 CAShapeLayer 路径画的，3.11.34 的模块圆角只看视图圆角，没改到。另：我们的 `ALBFXPassWindow`（点赞特效窗口）在探针里排最前，它的 hitTest 返回 nil，不影响触摸。

**3.11.35**：
- `ALBSheetGlass.xm`：`ALBSheetApplyShape`——contentView 里每个视图的非视图 `CAShapeLayer`：路径包围盒 ≥70×30 且 ≤ 面板宽−8、有填充或描边（alpha≥0.02）；四个角各从角点沿对角线往里 0.5 步走，进入路径的深度 ≥1.5（≈圆角 5 以上）算圆角；换成 `bezierPathWithRoundedRect:byRoundingCorners:`（同样那几个角、新半径，≤短边一半），原路径存关联对象，比较用 `CGPathEqualToPath`（CAShapeLayer 可能存拷贝）；抖音换了路径就把新的当原样。`setDisableActions` 免隐式动画。定时器 0.4→0.2 秒，`DUXVisualEffectView layoutSubviews` 时也补。诊断多“其中路径画的 N 个”。
- 探针采集（`ALBProbeCapture.h` 新，被 `ALBBackgroundProbe.h` 包含；控制器方法在 `ALBBackgroundProbe.h` 末尾 “Capture (3.11.35)”）：`probe.mode`=1（设置“探针用途”，或规则页“采集”按钮）进入；进入时如果设置页开着就先暂停。采集中 `handlesPoint` 全部 YES（覆盖层接点按），暂停时只接按钮和工具条；`stopLink` 采集中不停（线框每帧跟随）。点一下：`ALBProbeCollect` 全部应用窗口，取最前面的非插件、面积 <60% 窗口的视图，往上取链（到 60% 窗口为止，≤14 层），默认胶囊＝第一个 UIControl/cell/类名像按钮（Button|Btn|Cell|Item|Capsule|Pill|Chip|Tab|Entrance|Bubble|Action|Option）的，否则第一个带点击/长按手势或有圆角且有底色/毛玻璃的，否则点到的是图片/文字就取它父视图。胶囊里（≤300 视图）记图标（有图的 UIImageView、动画类名、≤120 的自绘内容；非视图图层：CATextLayer、≤120 带内容的图层、≤60 的 CAShapeLayer）、文字（UILabel、CATextLayer）、玻璃（UIVisualEffectView）；胶囊同级里覆盖它一半以上的 UIVisualEffectView/非抖音模块/类名带 Glass 的、往上 3 层内的 UIVisualEffectView 也记成玻璃（用来看另一个插件的玻璃挂在哪）。每个视图：类、模块（`class_getImageName`：抖音/系统/文件名）、窗口与胶囊坐标、圆角、底色、控制器链、`allTargets`+TouchUpInside 的 action、手势类、规则定位。“记下页面”：根控制器沿 presented → 标签页选中 → 导航栈顶走到底，每层类/标题/栈/子控制器；最上面一层（和它在窗口里的子控制器）非系统类里类型为 NSString/NSURL/NSDictionary 的 ivar，值含“://”或名字像 schema/url/route/page/enter/source/scene/path/link 的记下（≤24 条）。打包：剪贴板＋`Documents/AwemeLiquidBar_Capture_<段>.json`，段号存 `AwemeLiquidBar.probe.captureStage`，打包后清空。

**下一步**：等用户发采集 JSON，按每个胶囊的图标/文字层做“真玻璃按钮”（像右侧音乐、头像：玻璃按钮自己的图就是那个图标，一起变形），iOS 26 Regular；页面 JSON 用来做中栏胶囊的快捷跳转。

## 0.36 3.11.34（用户用背景探针点了多功能插件的长按面板：“看能不能直接做成液态玻璃的背景”；补充：“它取代了抖音原来长按弹出的面板菜单，里面有抖音原来的分享等功能，加上它自己的解析下载……只需要改造它的背景。每一个模块对应的那个灰色半透明的不用动，或者改它的圆角也行，现在的圆角有点大，整个背景的圆角也跟评论区一样支持自定义”）

探针（点在 312.7, 389.7，面板顶边下 8 点）：最上面是另一个插件的透明全屏窗口 `JDSBNotificationWindow`（这一点是空的）；面板在 `AWEMaskWindow`：`UITransitionView` ┬ `DUXAccessibilityDismissView`（全屏）└ `UIView`（y382、h956、圆角20、裁边，用户选中的这一层）→ `UIView`(574) ┬ `UIView`(透明) └ `DUXVisualEffectView`(574，`_UIVisualEffectBackdropView`＋`_UIVisualEffectSubview` 背景 #1C1C1C 73%＋contentView：3% 黑的全尺寸视图、顶部 16 高的条、面板内容)。即抖音自己的底部弹出组件，整块背景只有这一层毛玻璃。

**3.11.34**：`ALBSheetGlass.xm`（新，Makefile 已加；`%group` 只在 `DUXVisualEffectView` 类存在时 init）：
- 入口：`DUXVisualEffectView` `didMoveToWindow`（0/0.1/0.35 秒各认一次）。认：整宽、底边贴屏幕底、高≥120；往上 3 层内第一个有圆角且裁边的外框，外框的父视图里有 `DUXAccessibilityDismissView`；面板里的控制器（响应链往上＋内容里每个视图的 nextResponder）和视图类名（≤400 个）有带 `LongPress` 的才套用（`sheet.allSheets` 开则任何底部弹出面板），带 `Comment` 的控制器不碰。**类名没实测**：抖音长按面板现在叫什么没拿到（探针当时不记控制器），所以诊断“【长按面板】最近：控制器[…] 长按=是/否 → 套用/不碰”要看。
- 套用：记下原效果、`effect=nil`（`setEffect:` 钩子在 active 时把抖音新设的效果记成原效果、照样设 nil）；在它下面（同一父视图）插一块 `UIVisualEffectView`，tag 26113401，效果 `ALBMaterialMakeEffect(NO)`（评论区同款：`glass.lifted`＋`glass.clear` 时 Clear＋28% 黑），`ALBMaterialApplyEffectView` 深色，0.65 白 20% 描边，先装再异步给效果（同评论区）；外框和玻璃圆角 `sheet.radius`（8–44，默认 20），玻璃只圆上两角；`layoutSubviews` 钩子跟随 frame。还原：玻璃移除、外框圆角/maskedCorners 还原、效果还原。
- 模块圆角 `sheet.moduleRadiusOn`（默认关）/`sheet.moduleRadius`（4–24，默认 12）：contentView 里（≤600）宽≥70、高≥40、宽≤面板宽−8、原圆角≥6、背景色或图层背景 alpha 0.02–0.9（或是 UIVisualEffectView）、不是 UILabel/UIImageView 的视图；原圆角存关联对象，关掉还原；面板在窗口上时 0.4 秒定时器补。诊断列出改到的类和尺寸。
- 设置第 8 节“长按面板”（主页卡片“长按面板”→`openSheetOptions:`）；`ALB5.OptionsChanged` 时屏幕上的面板按新设置重来。诊断“【长按面板】”＝`ALBSheetGlassReport()`。
- 背景探针：每个视图多记 `controllers`（响应链上最多 3 个控制器）。

**下一步**：看诊断“长按面板”一行的控制器名和“套用”；没套上就按控制器名改判断（或用户先开 `sheet.allSheets` 试）。

## 0.35 3.11.33（用户：“右侧和面板这些其余部分的按钮就这样吧。三段式底栏看能不能让它的图标颜色跟液态底栏的一样去自适应深浅色。然后整个的图标能够和液态底栏一样的功能的 type 全部用现在的液态底栏所设置的图标，标题的名字也是。另外那两个功能键暂时搁置。然后把之前做的这个点赞特效插件合并进去，设置里也增加对应到参数调整，不过我看它没有对应的动画持续时间是长还是短，看可不可以加一个控制项”；附件 AwemeAmbience_TEST6_src.zip）

**右侧 / 面板**：用户收尾，不再改。

**三段式底栏（`ALBDock.m`、`ALBDockCommon.h`、`ALBDockMaterial.m` 没改、Tweak.x 两个导出）**
- 图标/标题：液态底栏的项是 `AwemeAlbumMorphBar.swift tabForItemKey`：首页 `newbar.home.normal`(photo.on.rectangle.angled)、第二项 `newbar.friends|shop.normal`、发布 `newbar.publish.normal`(plus)、消息 `newbar.messages.normal`(message)、我 `newbar.profile.normal`(person.crop.circle)，标题写死“首页/朋友(商城)/发布/消息/我”，图由 `AWEConfiguredSymbolName`（图标页的覆盖）给。底栏：`ALBDockTabSymbols()`/`ALBDockFriendsSymbol()` 经 Tweak.x 导出的 `ALBDockHostConfiguredSymbol(key,fallback)` 取同一套；显示标题 `ALBDockDisplayTitles()`＝首页/消息/发布/我。**和抖音底栏对话（选中页、角标、点击转发）仍用抖音自己的 `ALBDockTabTitles()`（拍摄）**；量系统底栏规格时按显示标题找 label。中栏 ✦ / ↑ 没动（用户：搁置）。朋友球固定用 friends 的符号（它永远点“朋友”）。没跟“隐藏标题”（量规格依赖标题，另做）。
- 图标页改动：`AWERefreshAllConfiguredIcons` 发 `ALBDock.IconsChanged`（每次改设置都会发），底栏 `iconsChanged` 比符号变了才 `reloadSymbol:` 和重建系统底栏的项（动画中则 `itemsPending`，结束补）。
- 选中色：`ALBDockSelectedTint()`＝`dock.accent`（默认开）时 Tweak.x 导出的 `ALBDockHostAccentColor()`（`AWEStoredAccentColor`，液态底栏 `updateAccentColor` 用的同一个），否则抖音红；自建项 `applyTint`、系统底栏 `tintColor`。
- 深浅：3.11.1 起底栏就有“视频页背后亮→浅色”整条路（`ALBDockMaterialNoteBackdrop` → `ALBDockSceneIsLight` → `ALBDockGlassStyle` Light、清透白色染色、`ALBDockInk` 黑），**3.11.19 删截图取样后没人再喂它**（`gALBDockLight` 恒 NO，所以视频页底栏永远深）。3.11.33 用探针喂：`ALBDockLumaProbe`（无滤镜 `CABackdropLayer`，tracksLuma、lumaUpdateRate 0.25、scale 0.25、opacity 0.02，同 `ALBSeedLumaProbe`），垫在 `ALBDockView` 最底下、整宽×底栏高、在 barCenterY；`backdropLuma:`：只在视频页且 `dock.adaptiveInk` 开；第一次读数直接定（中间带取近的一边）；之后读数越过阈值（>`dock.lightAt` 浅，<阈值−0.08 深）则 0.3 秒后看最新读数仍在那边才换（以 `ALBDockSceneIsLight()` 为当前值）；离开视频页 `lumaState=-1`（材质那边也会忘掉浅）。关掉开关＝`NoteBackdrop(NO)`。不截图、不改玻璃以外的东西；“已经排除”里的截图取样不是这个。
- 浅色时清透的白色染色改成底栏自己的 `dock.lightTint`（默认 0，0–0.4；`ALBDockMakeGlassTone` 里 light 用它，深色仍 `ALB25MaterialToneTint(NO)`；系统胶囊签名里也换成它）。理由：“已经排除”表里 3.11.1–3.11.9 的插件浅色玻璃“比系统的更白”被否，而 3.11.1 时用户自己把浅色染色调成 0——所以默认只换黑图标、玻璃仍透。这和被排除的做法不同处：不截图（系统读数），只用在系统管不到的底栏玻璃上（3.11.31 实测 UIGlassEffect 不是口袋元素），右侧/面板仍是系统自己切。
- 设置（三段式页）：`dock.adaptiveInk`(YES)、`dock.lightAt`(0.62，0.3–0.9，滑块不取整)、`dock.lightTint`(0)、`dock.accent`(YES)；说明改写。诊断“三段式底栏”多两行：图标符号、选中色；背后亮度（读数、多久前、次数、换边次数、阈值、探针在不在窗口）。
- 未知：探针读数和系统口袋的尺度是否一致（面板探针当口袋起步用是对的，说明同尺度）；默认阈值 0.62 是猜的，看用户诊断再调。

**点赞特效（`ALBAmbience.xm`，新，Makefile 已加）**
- 来源：用户的独立测试版 TEST6（`Tweak.xm`，com.zry.awemeambience）。逻辑原样：识别＝hook `AWEAwemeModel`/`AWEAwemeStatisticsModel` 的 `setUserDigged:`，0→1 且 1.5 秒内摸过屏幕；兼容模式靠 `%hook UIApplication sendEvent:`（Tweak.x 也 hook 了它，两处各自链上，互不影响）；特效画在 `UIWindowLevelAlert+10` 的穿透窗口；悬浮球面板在 +20 的窗口。
- 改动：键 `ALB5Key(@"fx.*")`（style3→`fx.style`），`MigrateStandaloneKeys` 第一次抄 `AwemeAmbience_*`；窗口类改名 `ALBFXPassWindow`/`ALBFXPanelWindow`；`FireAt` 先看 `ALBDockHostPluginActive()`（授权＋总开关）和 `StandalonePresent()`（独立版的类在→不放，诊断写明）；悬浮球默认关、开了才建窗口（`ApplyBallPreference`，`ALB5.OptionsChanged` 时也调）；三指双击唤球也要插件启用；`HandleEvent` 单指只在 Began 读偏好。
- 动画时长 `fx.duration`（0.5–2.5）：`FireAt` 取一次放进 `gFXTime`，`TS(t)` 乘到所有时间——`Spawn`（动画组、朝向关键帧的 duration/beginTime）、`KF`、`Twinkle`（周期和延迟）、`Spin`（时长，转的总角度不变）、`Flip`、`HueCycle`、`HueGS`、`EdgeAura` 的旋转、`RemoveLater`（按特效时间轴换算）、`Emit`（`speed=1/T`，停发也乘）。轨迹采样（`Sample`/`Ballistic`）用的是原始秒数（空间形状不变）。即整段等比例慢放/快放。浮球面板也加了“动画时长”滑块。
- 设置：第 7 节“点赞特效”（`ALB5OptionSections` 末尾追加，原 0–6 号不变），主页卡片“点赞特效”→`openFXOptions:`；行：总开关、样式(choice 13)、时长、密度(0.4–2)、喷射(choice 3)、按钮触发、双击触发、边缘光、震动、兼容、悬浮球、预览(action `fx.preview` → `ALBFXPreview()`)。改样式/喷射/时长/密度后 `OptionsChanged` 自动放一次。诊断“【点赞特效】”＝`ALBFXReport()`。`ALBFXPreview`/`ALBFXReport` 在 .xm 里 `extern "C"`。
- 语法检查：.xm 以 objective-c++ 过了 iOS 26.1 SDK（0 错 0 警告）。

**下一步**：看用户实测底栏深浅切得对不对（诊断“背后亮度”读数 vs 阈值）、特效是否正常识别（诊断“识别引擎”）。中栏两个功能键等用户再说。

## 0.34 3.11.32（用户发来 3.11.31 诊断：“会深变浅的视频比例减少了，看还有没有优化的可能吧，基本不怎么影响了；三段式底栏改变不成功，且原本清透会透现在也不透了”）

3.11.31 诊断：滑入前检查419 给值252（看过2 封面165 常见85）没找到封面94；首个0读数不交62。封面行：找到 `UIImageView 440×956 图360×640 resizeAspectFill`、`AWEAllScreenAdaptionVideoFirstFrameView 440×956 图360×640 resize`（首帧视图）；没找到时候选有 `UIImageView 440×956 图3×3`、`图32×60`、`UIImageView 440×587 图1920×2560`（图文）。估值与真实：常见 8 对 6 同档；封面 4 对 3 异档且都偏亮（0.55/0.12、0.78/0.14、0.61/0.22、0.40/0.34）。三段式底栏：“边深 登记4/4 口袋未出现”——`UIGlassEffect` 视图注册为边缘元素后 UIKit 不建口袋（推断：只有带 `_background` 玻璃的栏按钮/平台才是口袋元素；底栏常处于收起态，nativeBar 挪出屏幕）；`dock.followEdge` 开时清透被关→底栏只是不透了。

**3.11.32**：
- `ALBGlassSeed.h`：封面估值＝封面亮度×比例；比例＝该位置“首个真实读数÷封面亮度”（夹在 0.05–2）最近 12 对的中位数（≥2 对），否则全部位置最近 40 对的中位数（≥3 对），否则 0.5；来源文字“封面本位/全体/默认×比例”，配对时显示“(图x)”。`ALBSeedLayerIsPicture`：铺满类 gravity、≥16×16 像素（去掉 3.11.31 的 ≥120 和 25% 条件）；`ALBSeedPictureLayer`：覆盖 cell ≥50%，取像素最多的。
- `dock.followEdge` 默认 NO（`ALBDockCommon.h` 默认表、`ALB37GlassLayer.h`、设置行标题改为“试验…默认关”）；三段式页说明改写。

**下一步**：看“估值与真实”里封面对的同档比例。底栏若还要和右侧/面板一致：需把底栏的块改成栏按钮 face（像面板文字胶囊 `ALBPaletteFacePill` 那样），会失去自建玻璃的融合动画——先问用户要不要。

## 0.33 3.11.31（用户发来 3.11.30 诊断：“面板似乎极少会出现深色了，右侧按钮还是会大概率出现深色再变成较浅；另外三段式底栏似乎一直漏掉了……色调很深……如果能和面板以及右侧一致的话更省心，就不用区分页面选择要不要色调了”）

3.11.30 诊断：面板探针 读数3916 用上434（面板起步 443 颗、无可用 1）；滑入前检查502 给值88（看过88 封面0）；“封面图：AWEAwemePlayVideoPauseIcon 440×956 图158×186 center”——选中的是暂停图标层（contentsGravity center），`ALBSeedPictureLuma` 对 center 返回 NAN，所以封面一次没给上。估值与真实（看过）：12 对里 4 对异档。探针现在：左图标[0.66 0.66 0.66 0.67 0.52 0.27] 左文字[0.61 0.67 0.55 0.66 0.50 0.25] 右文字[0.62 0.53 0.58 0.53 0.47 0.22]；右侧当前 0.83/0.83/0.83/0.44/0.22——多数在中档。三段式底栏：其他读数链里有 `ALBDockPassEffectView<ALBDockView<ALBDockOverlayView`（底栏的 UIGlassEffect 走玻璃自己的亮度入口），没登记任何边；`glass.motionClear` 默认关→视频页常规玻璃＋固定 Dark＝深色调。

**3.11.31**：
- `ALBGlassSeed.h`：`ALBSeedLayerIsPicture`——contents 是 CGImage、contentsGravity 是 resizeAspectFill/resizeAspect/resize、图 ≥120×120 像素且至少一边 ≥ 视图像素的 25%；没找到时 `ALBSeedNoteCellPictures` 列出图片层的像素尺寸和铺法。位置常见亮度：每个右侧位置（cell 内 midY/8）记最近 15 个视频的第一次屏幕上读数（`priorVideo` 防重复），≥3 个时取中位数；滑入前没有“看过”和封面时用它（来源“常见”）。
- `ALB37GlassLayer.h`：`ALBDockHostFollowEdge()`（`dock.followEdge` 默认开、插件开、iOS 26）、`ALBDockHostEdgeStyle()`（＝`ALB311TopEdgeStyle`：视频页 Dark，其他页页面样式）、`ALBDockHostSyncEdge(dock, glass)`：底栏层（`ALBDockOverlayView`）里、底栏视图正下方一块窗口大小的 `ALBFollowMakeProxy`（`AwemeLiquidBar.DockEdge`，不抢顶栏边的 index 0），四块玻璃（mainGroup/midGroup/singleGroup/nativeBar）各自 `ALB3RegisterEdge(view, edge, 4)`；关掉时注销并移除边。
- `ALBDockCommon.h`：`ALBDockGlassUsesClear()` 在跟随时为 NO；`ALBDockGlassStyle()` 跟随时取 `ALBDockHostEdgeStyle()`；偏好 `dock.followEdge`（默认 YES，进 ALBDockPrefDefaults，改了走 materialChanged）。`ALBDock.m`：`syncEdge` 在 layoutSubviews、didMoveToWindow、materialChanged 里调。
- 设置：三段式页新增“和右侧、双击面板一样的玻璃”；清透说明注明此项开着时底栏不用。诊断末尾“三段式底栏玻璃（dock.followEdge=…）：边深/浅 登记n/4 层…”。

**下一步看**：“滑入前…封面/常见”次数、“封面：”行（找到的层或候选列表）、“估值与真实”同档比例；用户切视频是否还先深。底栏：登记 4/4、口袋层档位是否随背景变（0.85/0.60/0.10），用户看底栏是否和右侧/面板一样；若 UIGlassEffect 视图登记后不随口袋变（仍深）：说明只有栏按钮玻璃吃口袋，下一步要把底栏的块改成栏按钮 face（工作量大）。

## 0.32 3.11.30（用户发来 3.11.29 诊断：“部分画面双击面板可以做到启动就是目标状态，部分不行，切换视频还是会由深变浅”）

3.11.29 诊断：面板藏右侧 藏1108 显1108 退回普通0（图层透明度法生效）；看不见时的读数不交 682；`lumaUpdateRate` 原 0.25 → 设 1/60（**3.11.17 早已证明最小 0.25 s，设小无效——本次撤回**）。面板：“起步26/26”（右侧样本一致→中位）时用户看到启动即目标态；“起步0/26”（面板在左、右侧样本 0.25–0.66 / 0.20–0.62 等相差 >0.3）时不行。右侧离屏 cell（y-348、y1564、y-9896…）进窗“当场无值”、整 1.2 s 读空。

**3.11.30**（`ALBGlassSeed.h`，Tweak.x 的 `AWEFeedTableView -setContentOffset:` 调 `ALBGlassSeedFeedScrolled`）：
- 撤掉 3.11.29 的 `ALBSeedBoost`（不再调用）。
- 记忆：右侧每次屏幕上的读数按“视频|按钮在 cell 内 midY/8”存（≤600 条）；`ALBSeedReplayValue` 在“自己上次”之后查它（来源“看过”）。
- 滑入前：列表滚动时（每 50 ms 至多一次）对屏幕外上下 0.75 屏内的右侧按钮（每颗每 0.3 s 至多查一次视频，同一视频只做一次）：它的口袋还没量过这个视频→值＝看过的读数，否则封面（cell 子树里 layer.contents 是 CGImage、覆盖 cell ≥70% 面积的最大一层，按 contentsGravity 的 aspectFill/aspect/resize 映射，每张封面先缩成宽 36 像素的小图（缓存在该层上，图换了才重做），按钮外扩 6pt 的区域在小图上求 Rec.709 平均）→ 记 `estimate`，清字段 + 交值 + 当场定档 + ApplyNow + 开瞬时窗口。
- 交了估值的口袋，前两次读数里的 0（<0.005）不交（采集未就绪）。第一次真实读数记“来源估x/实y 同档/异档”（档：<0.3、<0.88、其余）。
- 面板探针：`ALBSeedLumaProbe`（layerClass=CABackdropLayer，tracksLuma=YES、lumaUpdateRate=0.25、scale=0.25、opacity 0.02、不接触摸、无障碍隐藏，代理方法 `backdropLayer:didChangeLuma:` 记值）；宿主 `AwemeLiquidBar.LumaProbes` 插在窗口 root 视图之上（面板在它之上）；按面板几何（同 ALBSidePalette layoutSubviews 的 top/bottom/scale）放左图标列、左文字列、右文字列各 6 段，共 18 个；右侧口袋有读数时确保存在。面板打开：每个 owner 先取所在列最近一段探针（5 s 内有读数）的值，没有再按原规则（右侧中位/就近）。
- 诊断新增一行“出现前的亮度”和“封面：…”“探针现在：…”。

**下一步看**：探针读数是否 >0、面板“用上”次数，用户看面板是否都一步到位；“滑入前 给值/封面/没找到封面”，封面行（找到的是哪一层、图多大）；“估值与真实”同档比例；用户切视频是否还由深变浅。若封面没找到：看“没找到封面图：…”列出的层，改找法。若估值常异档：换算法（加权/区域）或改用别的来源。

## 0.31 3.11.29（用户发来 3.11.28 诊断：“除了打开双击面板还需要一段时间匹配，单个视频右侧已经不会变深了，但是上下滑动切换视频还是会由深变浅……为啥它非得从深色启动，哪怕从终态启动也行，那个值看着是固定的”）

3.11.28 诊断：口袋当场定档 交时已定0 布局后定2126 计时器后定0 仍未定0；计时器钩子4 截到0——口袋在 layoutSubviews 里定档，强制布局即可，延后不是 NSTimer。面板藏右侧：藏0 显0——`_backing_setAlpha:` 在 26.x 是直接函数（26.1 反编译里 `-[UIView _backing_setAlpha:]` 把参数当 self 用：`*(alpha + 176)`），`class_getInstanceMethod` 取不到，退回了 setAlpha。藏着时右侧口袋读数 47（0.00–0.78）：不被绘制的 backdrop 报 0。面板在左时起步 0/26（右侧样本 0.20–0.97 不一致），首个读数 0.25–0.31 s 后才来。口袋三个档：0.85（亮度≈0/未测）、0.60（约 0.4–0.85）、0.10（约 0.9 以上）。离屏 cell 进窗无读数；屏上第一次读数有时是 0（采集未就绪），下一次 0.25 s 后才是真值。`CABackdropLayer`（26 头文件）有 `lumaUpdateRate`、`tracksLuma`、`tracksLumaWhileHidden`、`lumaSubrect`、`updateRate`。

**3.11.29**：
- `ALB5RightLayout.h` `ALB7RightSetAlpha` quiet：`[CATransaction setDisableActions:YES]` 下直接设 `view.layer.opacity`，读回 `view.alpha` 一致才算（中间层的视图读不回来→恢复并退回 setAlpha，计“退回普通”）。
- `ALBGlassSeed.h` `ALBSeedPocketReceive` 重写：我方口袋若按钮链上隐藏/opacity<0.01（`ALBSeedViewShown`）→ 读数不交（计数），其余同 3.11.28。
- `ALBSeedBoost/Unboost`：口袋（重新）进窗口时读 PocketBlur 层 `lumaUpdateRate`：1–30 视为每秒次数→设 60；0–1 视为间隔秒→设 1/60；其他不改（计跳过）。屏幕上（按钮与窗口相交）收到读数且 >0.001（或第 4 次）后恢复原值，记“首个读数（距回来）”。

**下一步看**：“亮度上报加快：原 x → y”是否 4 或 0.25；“首个读数”是否从 0.25 s 降到几十毫秒；面板打开、上下滑是否还看得见深→浅；“面板藏右侧…藏/显/退回普通”是否非 0；“看不见时的读数不交”数。若加快无效（首读仍 0.25 s）：CA 的首读间隔可能由别的属性（`updateRate`）决定，或要等首帧渲染后统计。

## 0.30 3.11.28（用户发来 3.11.27 诊断：“还是不能一步到位”）

3.11.27 诊断：瞬时窗口生效（玻璃五项：右侧 26、面板 2552 全部瞬时；口袋 opacity 755 瞬时）。但跟踪行“当场补 → 当场后 层隐/0.85 读1.0000 评空”：交值后（含 updatePropertiesIfNeeded、玻璃 layoutIfNeeded）口袋仍是 0.85、previousEffectiveLuma 仍空，0.1 秒采样时已定（0.10/0.60）——口袋的判定是延后的（`lumaUpdateSettleTimer`@480，8 字节，26.0 头文件里是非 ObjC 类型）。“距面板关后右侧恢复 0.068/0.117/0.061/0.070s”：右侧口袋在列重新显示后 0.06–0.12 秒才回来，而玻璃本体 B 在恢复当下就换了新对象。窗口里还把面板关闭时的动画（口袋 opacity 0.88 秒、玻璃色调 0.16 秒）变成了瞬时——不该动。

UIKit 26.1 反编译：`-[UIView setAlpha:]` 只有跨过 0.01 时才在 performWithoutAnimation 里调 `_notifySubtreeVisibilityDidChange:`，对子树里每个响应 `_viewVisibilityDidChange` 的 interaction 发通知（`_UIScrollEdgeEffectViewInteraction` 有此方法），并调视图的 `_visibilityDidChange`；`_isEffectivelyHidden` = hidden 或某标志或 alpha<0.01；`_backing_alpha` 读 layer（或中间层）opacity，`_backing_setAlpha:` 只改值不通知。

**3.11.28**：
- `ALB5RightLayout.h`：租约加 `quiet`；`ALB7RightQuietHide(role)`＝面板造成的隐藏（`gALB8PaletteRightHidden` 且不是用户 `right.hide.*`）且 `glass.directTone` 开；`ALB7RightSetAlpha(view, alpha, quiet)` 用 `_backing_setAlpha:`（运行时核对参数类型 d，没有就退回 setAlpha）。Suppress/Prune/Release 都走它。
- `ALBSidePalette.h`：`palette.group.alpha` 从 0.012 淡入；dismiss 淡出到 0.012 再移除（`glass.directTone` 关时仍是 0）；dismiss 开头调 `ALBGlassSeedPanelClosing`（清掉面板里所有 face 的 snapUntil）。
- `ALBGlassSeed.h`：`ALBSeedDeliverAndSettle(blur, send)`：交值期间截 NSTimer 一次性工厂（`scheduledTimerWithTimeInterval:repeats:block:`、`timerWithTimeInterval:repeats:block:`、两个 target/selector 版本；只在 `gALBSeedCapturedTimers` 非空且主线程时记，不改行为）；交完若口袋未定→`setNeedsLayout`+`layoutIfNeeded`（不带动画）→仍未定且截到计时器→`fire`。补值和头一秒读数都走它；跟踪行补值后注明“交时已定/布局后定/计时器后定/仍未定/不明”。诊断：各步计数、计时器钩子/截到/触发数、面板藏右侧不触发显隐的藏/显次数、藏着时右侧口袋读数及范围。

**下一步看**：“口袋当场定档”哪一项多（若都是“仍未定”且计时器截到 0：延后不是 NSTimer，可能是 dispatch_after/Task，需另想）；“当场后”是否已是终值档位；生命周期里面板打开期间右侧 EB 是否一直在（不再 EB[]）、B 恢复时是否不再换号；“藏着时右侧口袋读数”有无、范围是否像真实亮度（若出现 0 之类说明不渲染时报假值）。用户看到：面板关闭后右侧、面板打开是否一步到位；长按菜单/主页返回是否改善。

## 0.29 3.11.27（用户发来 3.11.26 诊断：“任何情况下都能快速恢复正常，但仍然存在那个恢复过程；能不能直接跳过 0.85 变成对的最终值”）

3.11.26 诊断：回来钩子“模糊层+口袋”；字段 `currentLuma@424+16`、`previousEffectiveLuma@464+16`，拒写 0；回来 1237 次、补给 2148 次、清读数 2788、清评估 1754。口袋读数是 1/64 档（0.7969=51/64、0.8906=57/64），BackdropView 存的 currentLuma 是 1/31 档（0.8065=25/31、0.9032=28/31）——它把读数量化到 32 档再比较。回来时（PocketBlur 的 didMoveToWindow，口袋自己的之前）currentLuma 已是空、previousEffectiveLuma 已是 0.0000（隐藏期间系统把有效亮度置 0，即起步深档 0.85）。当场补值后 0.1 秒内档位已是 0.60（读 0.8065）或 0.10（读 0.9032 以上）。离屏的 cell（y<0 或 >屏高）进窗时无值，留在 0.85，等滑进来第一次读数。`_lumaStyleForEdge:` 在 26.6 上不存在（“无此方法”）。字段布局：ScrollEdgeEffectView[552] backgroundCapture@408 pocketMask@416 pocketMaskedBlur@424 luminanceAdjustment@432 … previousEffectiveLuma@464 lumaUpdateSettleTimer@480 effect@488 edge@496 reducedTransparency@504 isOwningViewEffectivelyHidden@505 blurAlpha@512 …；BackdropView[440] lumaDidChangeHandler@408 currentLuma@424。生命周期：右侧恢复后第一次采样 EB 第三层已是 0.10；B（玻璃本体 Backdrop）每次恢复都是新对象；面板 B 会出现一对（#60/#61）约 0.25 秒后剩一个。色调动画瞬时：匹配 0（3.11.24 起只在交值那一刻生效）。

**判断**：锁深已解决；现在看到的“恢复过程”是玻璃跟随口袋时系统加的 0.5 秒色调动画（3.11.23 诊断记录：`filters.glassBackground.inputFaceColorMatrixBlack/White`、`inputFaceOpacity`，0.5 秒，含完成后保留的），在交值之后才加，所以 3.11.24 起的“交值那一刻瞬时”碰不到。

**3.11.27**：
- `ALBGlassSeed.h`：`ALBSeedFace.snapUntil`；`ALBSeedStartSnap(owner)` 开 1.5 秒窗口（口袋回来新记一次、口袋第一次读数（含早先离屏进窗的）、同一口袋换视频、面板关后右侧恢复（每颗右侧按钮）、面板打开（每个图标/胶囊））。`ALBGlassSeedSnapActive/ALBGlassSeedSnapping/ALBGlassSeedPocketLayerOwner` 给 `ALBGlassImmediate.h` 用。口袋回来当场补值后 `ALBSeedApplyNow`：在不带动画里对口袋和它那颗玻璃调 `updatePropertiesIfNeeded`（iOS 26 公开 API，有才调）和玻璃 `layoutIfNeeded`；跟踪行加“当场后 …”和“距面板关后右侧恢复 x 秒”。
- `ALBGlassImmediate.h`：五项色调路径除了交值那一刻，也在所属按钮的窗口内加速到瞬时（`ALBGlassImmediateOwner` 现在同时返回按钮）；新增：CABackdropLayer 上 `opacity` 或 `filters.*` 动画，若该层在插件自己的边的口袋里且按钮在窗口内，同样瞬时。动画组、叠加/重复/自定义时序、几何动画不动。报告列“窗口里见到的动画”（路径×次数）。

**下一步看**：用户是否还看得到过程；诊断最后一节“玻璃五项色调…（其中在回来窗口里 n）”“口袋档位 匹配/瞬时”“窗口里见到的动画”；跟踪行“当场后”档位是否已是终值。若仍有一下深：看是否是回来前玻璃已重建（B 换号）那几帧——下一步考虑 `_UIViewGlass setAdaptiveInitialLuminance:`（26.1 反编译有此方法）给玻璃起步亮度。

## 0.28 3.11.26（用户发来 3.11.25 诊断；“基本一样，大多数情况下深色不动屏幕就一直深；双击面板、关长按菜单、从别人主页出来都锁死深色；关评论区先深但自己能刷新到正常值”）

3.11.25 诊断：口袋入口装上，口袋读数右侧730、面板295、其他669；接收类 `_TtCC5UIKit20ScrollEdgeEffectView10PocketBlur`（PocketBlur 继承 BackdropView）。口袋读数首读+0.00s 就是真值、连续几次几乎相同（0.95 0.95 0.95），读数爬升模型判定“读数一来就接近真值”（保持/预测从未触发）。面板起步 22/130（右侧中位要求一致≤0.3、面板在左时无就近）。`_lumaStyleForEdge:` 输出“-”（26.6 编码不是 `=fffdqq}`）。

生命周期记录（3.11.23/24/25 三份一致）：右侧被隐藏（alpha 0）约 0.2 秒后 EB[]（口袋的三个 Backdrop 层整个离开边）；恢复后同一批对象（#9/#10/#11）回来，第三层 opacity 每次都是 0.85；看过亮度后变 0.60 或 0.10（面板新口袋 0.85→0.60 约 0.34 秒）。3.11.25 那份里右侧动作口袋整段一直 0.85。

反编译：`ScrollEdgeEffectView.BackdropView` ivar `currentLuma`（Double?，init 写 `*v9=0; v9[8]=1` 即 nil）+ `lumaDidChangeHandler`；`backdropLayer:didChangeLuma:` 调 Swift `sub_188FF7A70`（看不到实现）。iOS 26.0 `_UIScrollPocket` ivar 有 `previousEffectiveLuma`、`lumaUpdateSettleTimer`、`lumaTrackingToken`、`observableState`（`_isEffectivelyHidden`、`_lumaStyle`）。`-[UIView _didMoveFromWindow:toWindow:]`：先递归子视图，最后才调自己的 `didMoveToWindow`。PocketBlur、LuminanceAdjustment、ScrollEdgeEffectView 都重写了 `didMoveToWindow`。

**判断**：口袋回来时视觉档位回到起步（0.85），但 BackdropView 的 `currentLuma` 和口袋的 `previousEffectiveLuma` 还留着离开前的值；之后的读数与之相同或差得少，被当作“没变化”，不重新判断 → 锁深色直到背景变化够大。关评论区：视频从缩小变回全屏，亮度大变 → 自己恢复。面板口袋是新的（两字段都是 nil）→ 只是先深后浅。3.11.25 在口袋布局时交“和原来一样的值”只是把同样的值再写一遍，没用。

**3.11.26**（`ALBGlassSeed.h`）：
- 钩 `PocketBlur -didMoveToWindow`（名字含 PocketBlur、是入口类子类）和口袋视图 `ScrollEdgeEffectView -didMoveToWindow`（优先精确名 `_TtC5UIKit20ScrollEdgeEffectView`；去掉 3.11.25 的口袋 layoutSubviews 钩）。只管插件自己的边（往上 4 层内有 `AwemeLiquidBar.GlassEdge`）。
- 回来时（`ALBSeedBlurEntered`）：记 `entryAt`；口袋级钩子（子视图都处理完之后）当场：`ALBSeedForget`（两个字段清成 nil）+ 交值（`ALBSeedReplayValue`：回来后的新读数 > 按钮上等着的起步 > 自己上次读数且同一视频）不带动画；0（下一轮，若还没新读数）、0.2、0.5 秒再交一次。
- 字段：`class_copyIvarList` 找 `currentLuma`（入口类）和 `previousEffectiveLuma`（口袋类），要求 8 字节对齐、到下一个 ivar 9–16 字节、类型不是对象/指针；写前每次核对内容：tag=1 且 payload=0（nil）或 tag=0 且 payload 是有限值、0 或 1e-6 以上且在 [-0.05,1.05]；不符合则该字段永久不写（计“拒写”）。
- 口袋入口：读数原样转发（不再保持/预测）；回来后 1 秒内（或新口袋第一次读数起 1 秒内）每次先清两个字段再转发、不带动画。没有回来钩子时退化为“停 1.5 秒以上后的第一次读数”。
- 面板打开：只对拿到起步的按钮，把它们已建好的口袋当场按“回来”处理（清字段+交起步）；之后才建的口袋在 didMoveToWindow 时拿起步。右侧恢复仍留“自己上次”起步，口袋回来时交。
- 诊断：每次回来（全局 2 秒内只跟一次）记一行“回来时 层a/隐/b 读x 评y 式… | 当场补… | 读…@… | 0.1s … | 1.2s …”（层＝口袋里 CABackdropLayer 的 opacity，第三个是档位；读＝currentLuma；评＝previousEffectiveLuma；式＝`_lumaStyleForEdge:`，按实际编码解析，>16 字节且不全是浮点才调用，读进 128 字节缓冲）；报告列出右侧每颗口袋现在的状态、样式编码、口袋视图/BackdropView/PocketBlur 的 ivar 布局（名字@偏移）。

**下一步看**：字段那行是否“currentLuma@x+16 previousEffectiveLuma@y+16”且拒写 0；回来那行“回来时”第三层 0.85、当场补之后 0.1s/0.3s 是否变成 0.60/0.10；用户是否还锁深色。若字段被清、交了值但档位仍 0.85：可能是玻璃/口袋的 Observation 只在值变化时通知，或口袋在 effectivelyHidden 期间丢弃——看 lumaStyle 是否已变而档位没变，再定。若档位要等 0.3 秒才变：口袋自己的 settle 计时，再看 `lumaUpdateSettleTimer`。

## 0.27 3.11.25（用户发来 3.11.24 诊断：“看下有用不”）

3.11.24 诊断（约 4 分钟）：钩子“已装 UIView、GlassGroupLayerView（布局1）”。读数：右侧0、面板0、顶栏0、其他598，其他接收类只有 `_TtC5UIKitP33_ACD4A08F4BE9D00246F2A9C24A80CA8817UISDFBackdropView`。面板打开 8 次，“右侧样本0 起步0/26 当场给0”，右侧恢复“留起步0颗”。色调动画瞬时：右侧0、面板0。

**结论**：`_materialDidReceiveLuma:` 不服务于右侧、面板、顶栏的导航栏按钮玻璃（598 次大概率是底栏 UITabBar 的玻璃）。登记在边上的栏按钮玻璃的深浅跟着边缘口袋（`ScrollEdgeEffectView`，iOS 26.0 叫 `_UIScrollPocket`）：它的 BackdropView 收 `backdropLayer:didChangeLuma:` 读数，算出 `lumaStyle`（`-[UIScrollView _lumaStyleForEdge:]` 返回 `{float,float,float,double,long long,long long}`），还有 `lumaUpdateSettleTimer`、`previousEffectiveLuma`、`preferredUserInterfaceStyle`。`UIScrollEdgeEffect` 有 `_setPreferredUserInterfaceStyle:`、`_setFixedUserInterfaceStyle:`（未用）。3.11.24 实际对右侧和面板没有任何作用（没有注入、没有改值）。

**3.11.25**（`ALBGlassSeed.h`）：
- 新钩子：边缘口袋 BackdropView 的 `backdropLayer:didChangeLuma:`（按类名找：含 ScrollEdgeEffectView/_UIScrollPocket 且含 BackdropView、自带该方法、签名 v@:@d），和口袋视图自己的 `layoutSubviews`（名字以 ScrollEdgeEffectView 结尾或 `_UIScrollPocket`）。进同一套“起步 / 保持 / 预测 / 首值不带动画 / 停了交回”逻辑；送值走 `objc_msgSend`（口袋：`backdropLayer:didChangeLuma:` 传它自己的 layer）。
- 归属：往上找时遇到插件自己的边（`AwemeLiquidBar.GlassEdge` 的 UIScrollView）→ 找同级里 `kALBFollowProxyKey` 指向它的那颗玻璃 → 再按玻璃归属；另外在按钮内部任何位置的视图也认右侧/面板按钮。
- 起步只交给口袋（不再塞进玻璃组：3.11.24 证明玻璃组从没收过我们按钮的读数，硬塞可能改样子）；玻璃组布局钩只在它自己收过读数时才动。
- 右侧口袋隔 0.6 秒以上再读：视频变了＝换作品（epoch 2），没变＝重新出现（epoch 3，起步＝自己上次稳定读数）。
- 诊断：面板打开/右侧恢复后 0–1.5 秒读 7 次 `_lumaStyleForEdge:`（编码必须是 `=fffdqq}` 才读，读进 128 字节的缓冲）；报告里“右侧口袋现在”；不属于插件的读数记 4 条来源视图链；读数分“玻璃入口/边缘口袋”两组计数。

**下一步看**：边缘口袋读数右侧/面板是否>0；“面板首图标口袋 0.00s=… 1.50s=…”六个值里哪个在翻（推测第5个是深浅、第1–3个是亮度/阈值）、何时翻、和交进去的起步值有没有关系；“新玻璃·首帧前已给”的条数；用户看到的变化。若口袋收到起步值后 lumaStyle 仍按原时间才翻，说明口袋有自己的稳定计时（settle timer），下一步看 `_setFixedUserInterfaceStyle:` 或口袋起步值。

## 0.26 3.11.24（用户：真的无法理解为什么非要从深色换回来；问深色是不是读了抖音的图层；要求“仍随背景适配，但出现时就匹配”）

用户 3.11.23 实测：没有变化。诊断：钩子装上，右侧匹配275加速236、面板匹配2480加速2480、跳过39（全是“完成后保留”的0.5秒色调动画）。生命周期记录：右侧恢复时本体Backdrop换号，边的Backdrop(EB#11)透明度0.85→0.60约0.25–0.45秒后变化；面板每颗首次布局后同样。

**深色起步的来源（回答用户）**：不是抖音的图层。插件自己把 face、边、玻璃按钮都设成 Dark（3.11.10：浅色边上的“变浅”是更白的另一种）；iOS 26 自适应玻璃新建/重建时从自己的风格起步，等亮度读数超过阈值才变浅；读数0.25秒一次、重新开始时逐步爬升（3.11.13：0.62→0.70→0.77）。顶栏从不重建所以没有；纯 Clear 是固定材质没有自适应所以没有。

**关键新事实（iOS 26.1 反编译 EthanArbuckle/iPhone18-3_26.1_23B85_Restore）**：
- `-[UIView _materialDidReceiveLuma:]` 只做 `[self.superview _materialDidReceiveLuma:luma]`。
- `GlassGroupLayerView` 重写它：创建弹簧（response 0.5、damping 1.0），在弹簧动画里应用这次亮度。整个 UIKitCore 头文件里只有这两个实现。
- `_UIViewGlass setAdaptiveInitialLuminance:` / `setAdaptiveFixedLuminance:` 都是写同一个“自适应”枚举的不同 case（起步/固定），是配置层的，3.11.15/3.11.18 没有真正装到平台玻璃上。
- 3.11.23 加速了动画却无变化 → 深色主要来自读数到达之前和读数爬升，不是动画本身。

**3.11.24 做法（`ALBGlassSeed.h`，Tweak.x 在 ALBGlassImmediate.h 后 include；`%ctor` 里 `ALBGlassSeedInstall()`）**：
- 安装：hook `UIView` 和 UIKitCore 里所有自己实现 `_materialDidReceiveLuma:` 的 UIView 子类（按类名含 Glass/Material/Backdrop 筛，`objc_copyClassNamesForImage`，不发消息），每个类一个 block IMP 保存各自原实现；这些子类的 `layoutSubviews` 也 hook（布局后补给起步）。
- 分类：读数最先到达的视图往上找 `ALB3BarGlassButton`（穿在右侧/面板按钮上）或 `ALBPaletteSlider`；顶栏计数作参照；底栏、其他原样通过。嵌套（往上交）的调用 `gALBSeedDepth` 直接放行。
- “重新开始”：新视图、它的 CABackdropLayer 全部换新（弱表比对）、按钮刚被给了起步后的第一次读数（epoch 3）；右侧同一视图隔0.6秒以上再读且视频 id 变了＝换作品（epoch 2，起点＝它之前的读数）。
- 起步：面板打开（`ALBPresentSidePalette`，隐藏右侧之前取右侧可见按钮的稳定读数，布局和淡入设置之后 `ALBGlassSeedPanelOpened`）：右侧读数彼此相差≤0.3 用中位，否则面板在右侧时按高度就近（≤140pt），否则不给；右侧恢复（`ALB8SetPaletteRightHidden(NO)` → `ALBGlassSeedRightRestored`）和抖音盖住后重建（`ALBSeedAfterLayout` 发现玻璃组 backdrop 全换新且同一视频）：用它自己上次稳定读数。起步存在按钮上 1.2 秒；当场给已建好的玻璃组（`performWithoutAnimation` + `gALBSeedApplying`），没建好的在它布局时或第一次读数时用。
- 读数阶段：模型未验证时，读数在“新层起点(默认0.5)”和起步值之间就交起步值（保持），稳定后交真值；读数停了0.8秒交回最后真值（`ALBSeedScheduleRelease`）。第一次交的值不带动画。
- 本机学习爬升：样本＝epoch1/2 的完整读数序列（稳定或停了才算）；r_n = s + (T−s)·g_n，g_n 来自起点已知的换作品样本，新层起点 s 来自不同终值的新玻璃样本对；用首读预测终值，≥6个可检验且命中≥75%才“已验证”，之后用读数直接预测终值（钳在0–1、离读数不超0.45）。首读就接近真值（g1≥0.9）＝原值模式，只做不带动画。
- `ALBGlassImmediate.h`：只在 `gALBSeedApplying>0`（给起步/首值那一刻）把五项色调动画变瞬时，“完成后保留”的也处理；平时一律不碰。设置 `glass.immediateTone` 删掉，新设置 `glass.directTone`（默认开）。
- 诊断【新玻璃直接到位】：钩子类、各类读数次数、接收类名（本插件/其他）、重新开始次数、起步给了几颗/当场/布局补、保持/预测次数、学到的 g_n 与 s 与命中、右侧当前读数、最近18条（每次新玻璃：起步值和来源、首读延迟、读[…]、给[…]、稳定/到时）。

独立审查（提交前）：未发现闪退风险；按意见改了：走完的读数段一律把系统自己的读数交回（不会停在保持值上），超 3 秒或停 0.6 秒的段在下一次读数时结束；设置关掉时入口完全直通；开关状态缓存 1 秒、视频 id 缓存 10 秒（暂停后第一次读数会重新查）；布局钩先看是不是本插件的玻璃再读设置；`gALBSeedApplying`/`gALBSeedDepth` 用 @finally 配平；插件自己交值一律走 `objc_msgSend`（各钩子直通到正确实现）；钩子类上限 12、GlassGroupLayerView 优先；“不是本插件”的判断 2 秒后可重判。

**下一步看诊断**：右侧/面板读数次数是否>0（=0 说明读数不经过这个入口，钩子不在路径上）；本插件接收类是什么；“读[…]”有没有爬升、从哪里起；“给[…]”是否保持了起步；首读延迟多少；用户看到的是否仍先深（若读数和给的值都对而仍先深，说明还有入口之外的深色来源，如口袋的 lumaStyle）。

验证：自写 Logos→ObjC 转换（`%hook`→静态函数、`%orig`→同签名函数指针，行号基本不变）后 clang 18 + iOS 26.1 SDK / arm64 / iOS15 / ARC / blocks 语法检查零错误；`-Wall -Wextra -Wconversion` 下新文件零警告、改动文件警告数与 3.11.23 相同；preflight、bash -n、禁用前缀检查通过。未实机运行。

## 0.25 3.11.23（用户明确拒绝共存：要消除每次换视频/新面板的启动深色）

- 撤回3.11.22推广行为，恢复原有面板/右侧互斥显隐。3.11.21“不再变深”只证明不隐藏可绕过一次恢复，不是用户需求的完成。
- 代码重审：可见的右侧/面板icon/pill由Swift导航栏item glass绘制，复制底栏玻璃的UIButton底座实际被隐藏；slider才直接用configuration。没有发现每次布局重装材质或登记；顶栏/底栏主胶囊实际gaussianBlur+colorMatrix与右侧glassBackground并非同一renderer，不能仅凭_UIViewGlass描述相同就称机制相同。
- 公开原生分析证据：GlassGroupLayerView._materialDidReceiveLuma发起0.5响应spring；DesignLibrary.Configuration.adaptive(animatable:)的NO标记被updateState消费；ObjC未找到对应setter，不能硬写Swift布局。UIKit CASupportsAnimationForKey列出5个面部色调属性。
- 新ALBGlassImmediate.h：只拦实际有glassBackground且归属本插件右侧/面板的CABackdropLayer，精确5个面部色调属性；安全动画copy后提高speed，保留duration/物理参数。重复、累加、复杂时序、不移除、组等不改。原方法一次，保留回调；完成回调会因播放加速提前。总开关与glass.immediateTone(默认YES)控制，诊断有真实命中和跳过。顶底栏不匹配，不改透明度/染色/取样/模型配方，不做截图遮罩共存延迟显示。材质色调的按压渐变可能加快，几何按压保持。
- 关键边界：原生可能使用in-process presentation modifier绕过该hook；零命中不代表没有动画。即使命中也只消除过渡，不能使迟到的背景模型提前。这是针对性验证，不是首帧已修复声明。
- 真机直接测切换视频/面板首开/其他恢复，仍深时复制普通诊断；不再要求90秒大录制。未实机验证，不瞎报成功。
- 本轮检查：实际Logos转换完整Tweak.x后，clang19 + iOS26.1 SDK / arm64 / iOS15 deployment / ARC / blocks语法检查零警告零错误；preflight、bash -n、diff --check、版本与禁用前缀检查通过。ALB5RightLayout.h和ALBSidePalette.h与3.11.20基线逐字节相同（恢复原显隐），受保护Swift/材质/顶底栏/触摸转发文件未改。独立审查通过；未实机运行。

## 0.24 3.11.22（3.11.21 用户：不会变深了；附16.1秒记录）

- 记录83条全部保留，两次面板 +2.54～4.59、+5.84～9.15 秒打开/关闭，右侧动作一直 O2/F12/E13/I14/G7/B8/EB9/10/11；另一个代表 O3/F21/E22/I23/G16/B17 同样保持。+10.81 进设置、+12.89 退出期间两颗也无变化，只有顶栏登记滚动目标换号。用户视觉反馈补齐日志不能读到的实际深浅：不再变深。
- `ALB5RightLayout.h` 删除临时 gKeepRight；用正式 `panel.hideRight`（默认NO）过滤面板自己的隐藏请求，默认行为等价于已测对照开启。用户 right.hide.* 照旧。新设置在双击面板页，开启可恢复旧的整列隐藏，但其深色恢复问题可能重现。
- `ALB5Settings.h` 移除临时诊断开关，只保留手动录制；更新说明，不自动记录/不重复让用户采集此项。`ALBGlassLifecycle.h` Scene/Report 改为正常隐藏设置状态，记录器仍只读。
- 新建或现有面板按可见性选项设置AX modal，OptionsChanged 对当前未关闭的面板同步隐藏/AX。普通触摸及动作转发未改，未改玻璃材质、透明度预设或登记，顶底栏与Swift未改。
- 本轮只推广已测试策略，尚未尝试把评论/菜单/主页隐藏强制改掉；也没有缓存或预热面板。面板自身新建的深色首帧和其余场景仍不能宣布解决。下一步若需继续，须寻找既保持真正玻璃状态、又不让不该显示的控件露出的办法，不重复已失败的亮度/起步/遮罩方案。
- 验证：完整Tweak.x实际Logos转换后，clang19 + iOS26.1 SDK / arm64 / iOS15 deployment target / ARC / blocks语法检查零警告零错误；preflight、bash -n、diff --check、版本标识和禁用前缀检查通过；确认Swift、玻璃材质、顶底栏及触摸转发文件与3.11.21未改。未本地链接deb或实机运行整合版。

## 0.23 3.11.21（3.11.20 实机记录；用户补充双指长按打开插件设置不会让右侧变深）

证据与限制：
- 右侧动作 O2/F12/E13/I14/G7 多次面板开关后不变，隐藏时 o累=0，本体 B 变空，恢复后 B 换号，例如 +43.23 B189、+48.51 B226、+58.04 B263、+62.73 B300。独立边 EB9/10/11 恢复仍同编号。已排除这些场景下外层 face/proxy/interaction 被重新创建；没有排除原对象内部状态停止/重置。
- 音乐例外：现有逻辑在打开面板时不隐藏 music，日志该代表在这段没有同样的换层事件。进入设置时，动作的 B315 保持，唱片有独立 settings 隐藏，不能说整列都活着。
- +64–67 秒顶栏也 hidden 后恢复且部分 B 换号，但仍有 B108 延续。探针按类名只取前三个 Backdrop 候选，可能含临时层，不可把任意 B 换号等同玻璃深浅或根因。顶栏 O4/O90/O100 也发生过换样，“顶栏从不重建”的历史绝对说法须撤回。
- 422 条仅保留最后240条，最早可见 +33.51 秒，前面评论/主页详细过程已覆盖。本轮不再要求四场景长录，改为十几秒的单场景对照。

本版改动：
- `ALB5RightLayout.h` 新 `gALBGlassPaletteKeepRight` 默认NO、仅进程内临时状态；`ALB8SetPaletteRightHidden` 只在它开时跳过面板请求的整列隐藏。`gALB8PaletteRightHidden` 仍表示实际隐藏，用户 `right.hide.*` 生效且补位逻辑照旧。
- 诊断新增按钮，切换后自动开始现有录制；再点或重启抖音还原。停止录制不关闭对照。`ALBGlassLifecycle.h` 的 Scene/Report 包含状态；Mark/采样器/容量不变。
- 面板对照开启时 `accessibilityViewIsModal=YES`，防止 VoiceOver 激活后方右侧。普通触摸原本就由面板全窗关闭按钮/组截获；关闭流程交互照旧。
- 只验证关闭面板后的右侧，尚未修复面板首开深色，也不能推出评论/菜单/主页全部解决。材质、透明度预设、系统登记、Swift、顶底栏渲染未改。未缓存面板、未用tiny-alpha/遮罩或系统读数修改。

操作：设置→玻璃深浅→诊断→“对照：面板保留右侧（当前关）”；关设置，在同一亮画面左侧开关面板两次，每次关后等两秒；立即“结束记录并复制”，附上右侧是否仍深的观察。

验证：完整 `Tweak.x` 经实际 Logos（internal generator）转换后，clang19 + iOS26.1 SDK / arm64 / iOS15 deployment target / ARC / blocks 语法检查零警告零错误；preflight、bash -n、diff --check、禁用前缀扫描通过。Swift、材质、顶底栏和触摸转发实现与3.11.20一致。未本地链接deb或实机运行。

## 0.22 3.11.20（用户希望消除右侧在关闭评论、长按菜单、主页返回，以及双击面板打开时的深色起步）

- 审查确认 `ALBFollowSyncEdge` 只在 mode 改变时登记，不是每次 layout 都重登记。面板每次 alloc 新对象，`buildRows` 无条件重建全部行，dismiss 设 alpha=0 后移出窗口。右侧打开面板时，role 根 alpha=0，face+edge 都在其子树里。
- 这些只证明对象生命周期/祖先可见性差异；并未证明系统读数器在隐藏时必然重启。评论、长按菜单可能只是压暗背景；单独把 proxy 常驻不保证保住 face 内玻璃状态。没有尝试 tiny-alpha 保活、缓存面板或常驻边池，避免一次改多处再来回试。
- 新增 `ALBGlassLifecycle.h`，在 `ALBGlassTone.h` 后 include。设置诊断手动开始 90 秒只读录制、手动结束并复制；自动结束只停录，不覆盖剪贴板。固定容量、有限代表对象、稳定实例编号、记录窗口/祖先隐藏与 alpha/屏内状态/实际注册目标/内部玻璃和 backdrop 层身份。录制外不采样，不截屏、不 hook 亮度回调、不 force layout、不更改样式与交互。日志只在内存；关联编号是“首次观察时间”而非创建时刻。
- `ALBGlassLifecycleMark` 在面板创建前/首次布局后/关闭动画前/移出窗口后，以及右侧面板隐藏状态变更后标记。90 秒内可依次复现评论、长按、主页返回、双击面板两次；每次返回停两秒，再到设置“结束记录并复制”。
- 日志不能直接读到系统最终玻璃深浅，也不能证明真实采样是否暂停或亮度滞回；必须结合用户观察。根据 face/edge/内部玻璃是否重建，才决定下一版最小实验。仍保留 3.11.19 面板点击兜底，顶底栏渲染无改动。
- 独立审查发现原工程 `ALB9RightReadAlpha` 会在隐藏租约期间向调用者返回原生 alpha，单读视图 getter 会漏掉右侧隐藏。本版诊断已同时记录祖先 view.alpha 与模型 layer.opacity 累积、视图/层 hidden，恢复判据读层 opacity；这只是只读取证，不改租约或其渲染。完整样本输出仅用稳定编号，最多256项类名图例；重复短查压成“状态同前”，避免导出重复长文本。
- 本轮检查环境：本地取得 clang 19、iOS 26.1 SDK 和 Theos Logos；将完整 `Tweak.x` 用 Logos `generator=internal` 转成 Objective-C 后，以 arm64 / iOS 15 deployment target 做 `-fobjc-arc -fblocks -fsyntax-only` 检查。Swift 未改，未做本地 deb 链接/打包或手机运行；最终仍需 Actions 构建和手机验证。静态 preflight 和推送脚本 `bash -n` 均通过。

## 0.21 3.11.19（用户实测 3.11.18：不行；“删掉吧，该整理的整理，就保持当前最终能达到和底栏顶栏一个效果的做法就行，其余测试方案及相关无用代码全部删掉，各类阈值调节也删掉，修复双击面板无法按压生效的问题”；问首页/朋友页锁定深色会不会干扰初始状态）

最终做法（不再有开关，iOS 26）：右侧、双击面板（图标、文字胶囊、两条滑条）、提示胶囊＝底栏自己的玻璃（`ALB25MaterialFollowBar` 只看系统版本）＋系统栏按钮 face（`ALBBarFace.h`，`ALBBarFacesWanted` 只看系统版本和 Swift 类）＋每颗自己的深色边（`ALBFollowSyncEdge` 只剩 mode 0/1，共享边 mode 2 删了）＋系统切深浅（`ALBFollowSystemDecides` ＝ `ALB25MaterialFollowBar`）＋系统栏按钮自己的按压（`ALBFaceTouchAdopt`，`ALBFaceNativePress` 删了）。

删掉的：
- `ALBGlassTone.h` 从 1449 行剩约 340 行：截图取样（`ALBToneCaptureWindow`、各种 `*Luma` 读法、`ALBToneSample`、`ALBToneSamplePanelNow`）、插件自己的深浅（`ALBToneEnabled`/`ALBToneLightForView`/`ALBToneHasSample`/`ALBToneThreshold`/`ALBToneSyncButton`/`ALBToneSyncPanel`/面板每行深浅）、亮度回调钩子和记录（`ALBLumaHookClass`、`ALBEdgeTrackers`、`ALBEdgeTunePanel`/`ALBEdgeTuneAll`）、3.11.18 起步亮度整节、所有探针（`ALBToneTopButtonProbe`、`ALBToneSystemLumaProbe`、`ALBEdgeInsideProbe`、`ALBLumaTrackersIn`、`ALBWindowLumaProbe`、`ALBRegisteredScroll`、`ALBStartLumaReport`、`ALBRightEdgeRateSample`）。剩：是不是视频页（含导出给 `ALBDockMaterial.m` 的 `ALBDockHostNonVideoChannel`）、顶栏的玻璃配方（`ALBToneApplyTop`，深色，`glass.clear` 照旧）、菜单跟随（`menu.followGlass`，染色下限固定 0.35）、精简诊断（`ALBFollowReport`/`ALBToneReport`/`ALBGrouponReport`）。
- `ALB25Material.m/.h`：3.11.18 的 `ALB25MaterialGlassWithStartLuminance` 等四个函数；`ALB25MaterialLightTint` 固定 0.12（只在插件自己切浅色时用过）。
- Tweak.x：右侧 `ALBRGToneStyle` 固定深色，`ALBRGRetone` 删；提示胶囊只走“深色＋自己的边”；头像菜单固定按 0.5 亮度判断（3.11.0 的规则，原来插件深浅关着时就是这样）。
- 设置“玻璃深浅”只剩 `glass.darkTint`（标题改成只管顶栏清透和三段式底栏清透）和 `menu.followGlass`。

双击面板按了不生效：面板按钮是普通 UIButton，动作挂在它自己的 TouchUpInside 上；face 的控件把触摸转交给它（`[owner touchesBegan:…]` 等），UIControl 对转交来的触摸不一定会发 TouchUpInside（右侧的按钮自己重写了触摸方法交给抖音，所以不受影响）。`ALBFaceTapOnTouchUp`（`ALBPaletteFaceIcon`/`ALBPaletteFacePill` 里打标）：按钮上挂一个只记录的 TouchUpInside 目标；控件的 touchesEnded 转交后，若触摸在按钮范围（外扩 16 pt）内结束、按钮在这次触摸开始后没有自己发出 TouchUpInside，下一个主线程循环补发一次 `sendActionsForControlEvents:TouchUpInside`（按钮不可用、不在窗口里不补；触摸被取消不补）。按钮自己发了就不补，不会重复。诊断“面板补发动作N”是补发次数；双击面板一栏“按抬起发动作x/y”是打了标的按钮数，“不可点”是 `enabled=NO` 的按钮数（`setSourceActionsEnabled:` 在没找到当前视频时会禁用）。

首页/朋友页锁定深色：查了代码，锁在 `ALB33` 页面风格里（视频流 → `gAWEAlbumMorphController`、它的 view、tabBar 和 iOS 27 以下的底栏窗口 `overrideUserInterfaceStyle = Dark`），只作用于液态底栏自己，而底栏正是秒切的那个。右侧和面板在抖音主窗口里，深色是插件自己设的（3.11.10 起每条边、每个 face 都是深色，顶栏在视频页也是深色），跟这把锁无关；顶栏同样深色起步却没有那 1 秒，差别是右侧/面板的玻璃会新建或重新出现。结论：不是它。

## 0.20 3.11.18（用户实测 3.11.17：好像无效，附诊断；用户选“试起步亮度的正确用法”，读亮度用“底栏/顶栏读数或固定预设值，像清透那样替换掉深色状态”）

诊断：`lumaUpdateRate 是不改：读数间隔，但不能比0.25秒更短（设0.6后间隔变长）`（只设过测试那 1 层）；`面板第一颗的系统墨色[0.06秒读不到]`；按压 `按下13 抬起13 取消0`（点击好了）；`_UIViewGlass` 接口只有 `setAdaptiveInitialLuminance:`/`setAdaptiveStyle:`，没有 getter，描述是 Swift 值 `_Glass(... variant2: ... frost: Frost(base: automatic) ...)`（不含起步亮度字段）；CABackdropLayer 有 `allowsFilteredLuma`。面板在左侧：边的 PocketBlur 和元素一样大（40×40、153×28）。记录里“面=”0.31→0.41→0.50→0.64（0.25 秒一次），之后画面变暗跟着降。

结论：读数器不能更快；玻璃的深浅变化 App 看不到，是系统画玻璃时自己做的；3.11.15 的起步亮度设在了 `_background` 交出来的拷贝上。顶栏/底栏的读数是顶部一条（抖音顶部渐变，平时偏暗），拿来当面板中间的起步会大多是深色，所以按用户的另一个选项用固定预设。

3.11.18：
- 删 3.11.17 的 `ALBLumaRateProbe`、`glass.fastLuma`、`ALBPanelProbeOpen`/`ALBFaceInkState`。读数钩子仍只记录。
- `ALB25Material.m`：`ALB25MaterialGlassWithStartLuminance`（复制＋设起步亮度）、`ALB25MaterialEffectWithStartLuminance`（取 `UIGlassEffect.glass` → 上面 → `effectWithGlass:`，interactive 照原来）、`ALB25MaterialConfigurationWithStartLuminance`（配置的 `_material` → 上面 → `ALBInstallMaterial`）、`ALB25MaterialStartLuminanceTest`（诊断：设 0.9 后描述有没有变、经过 effect 读回还在不在）。
- `ALBGlassTone.h` “3.11.18 新玻璃的起步亮度”：栏按钮里 KVC `glassEffect` 是 UIGlassEffect 的视图（缓存，2 秒重找），`ALBStartLumaOwner` 写回带起步亮度的效果；记下装上的对象（连读两次不同＝getter 每次给新对象，之后不再判断）和系统原来的；系统换回自己的才再装，10 秒内最多 3 次，然后放手。面板：`ALBPresentSidePalette` 在 `ALBToneSamplePanelNow` 之后（画出来之前）`ALBStartLumaPanel`，tick 里补后建的；右侧：`ALBEdgeTuneAll` 每个 tick（所有格子，含屏幕外）；滑条：`ALBPaletteSlider` init 里 `ALBStartLumaConfiguration`。开关关掉（或插件自己切深浅、栏按钮关）→ `ALBStartLumaRestoreAll` 放回原来的。
- **下一步看**：诊断“新玻璃起步亮度”的测试——“设0.9后玻璃的值没变”说明这个值不在玻璃的数据里，系统画的时候拿不到，这条路也就到头了；“变了…且装进效果后还在”而面板仍先深，说明系统重画玻璃时不用它。还要看“装回”数是否大于 0、装在什么视图上、有没有“系统换回/放弃”。

## 0.19 3.11.17（用户实测 3.11.16：遮罩太低级；栏按钮本身不该有延迟，顶栏底栏都是秒切，怀疑是没删干净的代码让它一开始是系统默认色调而不是登记后的状态，要查代码；面板点不动，右侧偶尔点不动）

查代码的结论（`ALBGlassTone.h` “3.11.12-3.11.17” 注释）：
- **顶栏**：三段式下是窗口大小的一条自己的边（`ALB37GlassLayer.h` `ALB311TopOwnEdge`，在 `ALBDockOverlayView` 最底下），液态底栏下是底栏页面的滚动视图；整个生命周期只有这一条边，系统在边上读背后亮度的读数器从不重来。
- **右侧、面板**：每块玻璃一条自己的空滚动视图边（`ALBFollowSyncEdge` 模式 1，边 2）。面板每次打开都是新边；右侧在面板打开时被 `ALB7RightSuppress` 设成透明度 0（边一起不画、读数器停）、翻页时下一格在屏幕外，回来时读数器重新开始。
- **读数器重新开始**：从 0.5 起步，每 0.25 秒朝真实亮度走一部分。3.11.13 记录的面板 0.62、0.70、0.77（0.77 时变浅）＝0.5 往 1.0 每次走约 0.22–0.25。这就是“先深约 1 秒”。顶栏启动时也有，只是只有一次，之后只在画面变化时滞后（被翻页动画盖住）。
- 不是“没登记”：面板的栏按钮在第一次布局里就同步登记了（`ALBFaceLayout`→`ALBFollowSyncEdge`），按钮自己的旧玻璃也被藏着；深的那一下是登记以后读数器还没读到位。
- 3.11.13–3.11.16 改的都是读数器“出来以后”的东西：交给代理的读数（3.11.14，没效果）、玻璃的起步亮度（3.11.15）、盖一层浅色（3.11.16）。

3.11.17：
- 删掉 3.11.16 遮罩（`ALBPanelVeil*`、`panel.openVeil*`）、3.11.15 起步亮度（`ALBSeedFace`、`ALBFaceGlassObjects`、`glass.seedStart`）、3.11.12–3.11.15 盖住保持（`ALBFaceWatchCover`、`gALBLumaHold`、离开 App 的观察者）；系统切深浅时面板打开不截图。
- `glass.fastLuma`（默认开）：`ALBEdgeTrackers` 给插件自己边里所有 tracksLuma 的背景层（含亮度调整层）设 `lumaUpdateRate`（记下原值，关掉开关就还原）；右侧每条边最多 1.5 秒一次（`ALBEdgeTuneAll`，屏幕外相邻格子也设，翻进来时已经是快的），面板每次 tick 都重设（没变不写）。值的含义由 `ALBLumaRateProbe` 测：第一个回调的读数器先设 0.05，之后两次间隔 <0.15 秒＝“间隔”；四次 ≥0.2 秒就再设 0.6，两次间隔 ≥0.45 秒＝“间隔但不能短于 0.25 秒”（不改，设大只会更慢），三次 0.15–0.45 秒＝“每次走多远”；3.5 秒内判断不了、或它安静了 1.5 秒就还原并换一个再测（最多 6 次）；结果按 iOS 版本存在 `NSUserDefaults` 的 `AwemeLiquidBar.lumaUpdateRateKind`（“不改”不存）。间隔：面板 0.05、右侧 0.08；走多远：面板 1.0、右侧 0.75。读数只记录（钩子照旧原样传下去），顶栏的边只打标签不设。
- 点不动：`ALBFaceTouchAdopt` 以前只在按钮布局时（`ALBFaceSyncActions`）给栏按钮的控件换上转交触摸的子类；系统之后建/重建的控件接不到。3.11.15 打开面板前的同步截图（`drawViewHierarchyInRect`）碰巧让面板再布局一次，3.11.16 去掉截图后面板的控件就没接上。现在 `ALBFaceAdoptHit`：命中落在栏按钮的控件（或它里面）时，当场接上并返回控件本身；`ALBSidePalette hitTest:` 和右侧 `ALBFaceFilterHit` 都走它。
- 诊断：“新玻璃读亮度加快”（开关、测出的含义和依据、设过几层、右侧一条边的读数器个数和频率、最新读数）、“面板第一颗的系统墨色”（`ALBPanelProbeOpen`：打开后 1.6 秒内每 0.05 秒看一次第一颗图标栏按钮里文字/图标的深浅或墨色，记录变化时刻；“读不到”说明翻色不在 App 这边）、亮度回调记录（读数快了以后每个标签 0.02 的变化或 0.5 秒记一次）。
- **下一步看**：诊断里“lumaUpdateRate 是…”测出了什么；亮度回调记录里“面=”从 0.5 到 0.77 用了多久（原来约 0.5 秒）；“面板第一颗的系统墨色”几秒变浅。如果读数快了但玻璃变浅还是慢，说明玻璃自己还有一段固定的过渡，下一步就只剩“不新建边”（面板复用一套一直在读的边）这条路。

## 0.18 3.11.16（用户实测 3.11.15：可能深色时间短了一点，但默认打开肯定还是深色；“哪怕打开的时候纯透明或者浅透明都行”，太迟钝了）

没有新诊断。判断：系统玻璃新建/重新出现时先深后浅是系统自己的（3.11.12 换新边就让右侧每次先深，说明深浅跟着边的状态；3.11.13–3.11.15 改读数、设起步亮度都没能改掉起步）。用户接受“打开时透明或浅透明”，所以改成**盖住**而不是改它。另外 3.11.15 的 `ALBSeedPanelAtOpen` 在面板出现前同步截图（`drawViewHierarchyInRect`，3.11.10 记录里一次 107 毫秒），每次打开都晚约 0.1 秒——用户说的“迟钝”很可能有它一份。

3.11.16（`ALBGlassTone.h` “3.11.16 开场浅色遮罩”，开关 `panel.openVeil` 默认开，`panel.openVeilTime` 0.9 秒、`panel.openVeilAlpha` 0.6）：
- 只在系统切深浅（`ALBFollowSystemDecides`）且用系统栏按钮时。`ALBPresentSidePalette` 里 `ALBToneSamplePanelNow` 之后、组淡入之前调 `ALBPanelVeilOpen`。
- 每行的图标脸和文字胶囊脸：在脸（`ALB3BarGlassButton`）**上面**加一个容器（不接触摸、无障碍隐藏），按 `contentFrame` 画白色圆/胶囊（浓度），再按按钮的 spec（`kALBFaceSpecKey`：image/point/regular/alpha/fill/inset/preserve，或 title）把图标或文字画一遍，系统墨色画成黑色、自己的颜色不变。脸本身一点没动（不改 alpha、不是它的祖先、不在它的边后面），系统在下面照常起步、变浅。滑条：在内容画布（`imageView`）里最底下插一层同形状的浅色，名字和数值先黑色，揭开时交叉淡回原来的颜色；这期间 `ALBToneApplyPanelInk` 要给名字的颜色记下来，揭开时用（`ALBVeilHoldsSliderInk`）。
- 哪几行：打开时 `ALBPanelVeilPrior` = 右侧可见按钮的边 2.5 秒内最新读数的中位数，≥0.5 就全部盖上（不截图）；0.06 秒后 `ALBVeilRefine` 截一次窗口，`ALBPanelRowLumas`（每行胶囊外侧），≥0.55 没盖的补盖（0.1 秒淡入），<0.45 盖着的揭开（0.12 秒），≥0.7 记为“明确亮”；同一张图做 3.11.15 的起步亮度（`ALBSeedPanelRows`，开关仍是 `glass.seedStart`）。
- 揭开（每 0.1 秒一次 `ALBVeilTick`，主循环 common 模式的计时器）：脸下面的系统墨色读到“浅”（`ALBFaceLooksLight`：控件里第一个可见的、非原色的图片或文字，深浅＝浅或墨色亮度 <0.35）连续两次且已过 0.25 秒→0.25 秒淡出；否则到 `panel.openVeilTime`（读得到墨色、这行明确亮、墨色还是深：再多等 0.5 秒）→0.3 秒淡出。面板 `hitTest`（触摸）→全部 0.12 秒揭开。面板关了→收尾。
- 诊断“开场浅色遮罩”：第几次、先验读数和颗数、开场盖了几行、修正（用时、每行亮度、补盖/去掉）、变浅/到时/按下各揭开几块、收尾原因、第一块的系统墨色变化时间和读的是什么视图。**下一步看这里**：如果“第一块的系统墨色”一直是“读不到”或一直“深”而玻璃其实已经浅了，说明系统的翻色不在 App 这边，只能按时间揭开；如果有“x.xx秒浅”，那就是系统真正变浅的时刻。
- 没做：右侧在关掉长按菜单、评论区、面板后的先深（用户这次只说面板打开；同一套遮罩可以用在“面板关掉后右侧重新出现”上，那一刻插件自己知道）。

## 0.17 3.11.15（用户实测 3.11.14：没用；用户选“试系统玻璃的起步亮度”）

用户：没用，一样；诊断真的有用吗；为什么底栏经历类似场景不会莫名变深，面板和右侧会。

结论（这次说清楚了）：3.11.13–3.11.14 改的是 `ScrollEdgeEffectView` 的 BackdropView/PocketBlur 收到的 `backdropLayer:didChangeLuma:` 读数，面板起步交 1.0 都没效果——**玻璃自己的深浅不看这些读数**（`ALB25Material.m` 头部 2.4 的笔记：亮度钩子碰不到这块玻璃；2.4.4 改玻璃自身自适应后栏图标颜色卡住）。底栏在三段式＋清透下是插件固定清透深色，不走系统自动深浅；顶栏走系统的但从不离开屏幕。右侧（面板打开时插件隐藏它；长按菜单、评论区时抖音盖住它）和面板（每次新建）都是“新建/重新出现”的系统玻璃：推断从深色起步、约 0.7 以上才切浅。

3.11.15（试验，`glass.seedStart` 默认开）：
- 去掉 3.11.13–3.11.14 对读数的修改和 `tracksLumaWhileHidden`；钩子只记录（`ALBLumaNote` 存每层最新读数）。`ALBEdgeTrackers` 只标 `lumaSubrect` 非空的层。
- `ALBSeedFace(face, luma)`：栏按钮里所有 `_background` 是 `_UIViewGlass`（不含 Group）的视图的玻璃对象，`setAdaptiveInitialLuminance:`（≥0.5 给 1.0，否则读数）；玻璃对象表 2 秒刷新一次；同值不重设。
- 面板：`ALBToneSamplePanelNow` 开头（系统切时）`ALBSeedPanelAtOpen`：截一次窗口（12%），每行 `ALBToneBesideLuma`，设这一行图标和胶囊的玻璃。之后 tick 里按各自边的读数每秒更新。
- 右侧：tick 里（`ALBEdgeTuneAll`）每秒按各自边的最新读数（2 秒内的）更新；`gALBLumaHold`（盖住，第一次看到就算；露出 0.35 秒后放开；翻页放开；离开 App 开始）期间不更新。
- 诊断“起步亮度（试验）”：示例右侧按钮的玻璃对象类名、能否设、给了、读回（KVC `adaptiveInitialLuminance`）、边读数；上次面板的行数、玻璃数、用时、每行亮度。
- 如果没效果：下一步是用设好起步亮度的玻璃副本替换平台视图的 `_background`（让它重新起步），风险是动到系统栏按钮的按压和分组。

## 0.16 3.11.14（用户实测 3.11.13，附诊断和两张截图）

用户（很生气）：3.11.13 把右侧按钮改坏了（截图：赞、评、藏、转全变成“•••”）。“方形遮罩”指的是双击面板图标里自己设计的浅色、有内边距的内衬（`panel.iconFill`），原来圆的现在方的，和头像变方同理。只要解决：按钮下面明明没有深色背景，长按菜单、评论区、双击面板等动作之后玻璃却变深。

诊断“亮度回调记录”（3.11.13 加的）：右侧每 0.25 秒一次；被盖住期间 0.25–0.27，露出后 0.55–0.62（用户看到的是深色）；面板开后 0.15 秒就有第一次读数 0.62，接着 0.70、0.77 才变浅。结论：读数不慢，是系统的滞回——从深切回浅要约 0.7 以上，从浅保持只要更低；抖音在弹层下压暗画面（0.25）让系统切深，之后的 0.55–0.62 不够它切回来。新边从深开始，同理要过 0.7。

3.11.14：
- 撤回 3.11.13 的导航栏收窄（`ALB3BarGlassButton` 图标 160、文字 `max(240,宽/比例+80)`；92 点时系统把项目收进了溢出“•••”）、边的 contentFrame 尺寸和圆角裁剪、`ALBFaceLayout` 里的 `layoutIfNeeded`、`tracksLuma` 关开重读。
- 内衬：`layoutFill` 也调用 `unclipAncestors`（从 `layoutOverlay` 抽出来的循环）。
- `ALBLumaHookClass`：`backdropLayer:didChangeLuma:` 的替换实现交给系统 `ALBLumaAdjust` 的结果。只处理标了 `kALBLumaKindKey` 的层（`ALBEdgeTrackers` 给右侧边标 1、面板边标 2），其余原样。1：`gALBLumaHold` 期间交“保持前 0.75 秒的读数”（≥0.55 时至少 0.85）；2：层第一次读数起 1 秒内，读数 ≥0.55 交 1.0。`ALBFaceWatchCover`：第一次看到盖住就保持（`ALBLumaStartHold`），没盖住 0.35 秒后放开，保持中 feed 的 contentOffset 变了 40 点以上立即放开（翻页）；`UIApplicationWillResignActiveNotification` 也开始保持。面板布局后 `ALBEdgeTunePanel` 立即标记（下一轮主循环和 0.08 秒后）。
- 诊断：“遮挡时保持：现在保持中/读实际、上次、保持交出次数、面板起步次数”；记录里改过的读数显示为“右=0.25→0.85”。
- 未做：切换视频时新一格的短暂深色（用户这次没要求）。

## 0.15 3.11.13（用户实测 3.11.12，附诊断）

用户：太多场景都是先深色再正常（关长按菜单、关评论区、关双击面板，双击面板打开本身也是），好像除了顶栏底栏都有问题；双击面板那个较浅的颜色遮罩是方形的。诊断：重新登记过 21 次；新边里有 `ScrollEdgeEffectView(透)`、`PocketBlur(隐)`、`LuminanceAdjustment`、`ScrollEdgeEffectView.BackdropView`，都 `tracksLuma`、频率 0.25，**亮度矩形 {{-20.9,0},{167.3,46}}**（=系统栏按钮里 160 点宽的导航栏×46/44，从玻璃左 21 点到右 100 点）；全窗口 26 个跟踪层，没有 `_UILumaTrackingBackdropView`。3.11.11 时右侧的边里是空的（那时的边建在按钮还没尺寸时）。

3.11.13：
- 去掉换边重新登记（新边从深色开始是系统行为）。`ALBEdgeTuneAll`（tick 里，每条边最多 1.5 秒一次）：右侧所有栏按钮的边、面板栏按钮和滑条的边里的跟踪层设 `tracksLumaWhileHidden=YES`；`ALBFaceNudgeEdges`：露出 0.3 秒后、回到前台 0.4 秒后，右侧各边的跟踪层 `tracksLuma` 关、下一轮主循环再开（推断：重新开始读会立刻给一次当前亮度，不改系统已有的判断，不会先深）。
- `ALB3BarGlassButton` 的导航栏宽度：图标 92（原 160），文字 `max(宽/比例,100)+48`（原 `max(240,宽/比例+80)`）。
- `ALBFollowSyncEdge` 模式 1：边的 frame＝栏按钮的 `contentFrame`（真正的玻璃），`cornerRadius`＝短边一半，裁剪。`ALBFaceLayout` 先 `[face layoutIfNeeded]`。
- 诊断：`ALBLumaHookClass` 给跟踪层的 delegate 类接上 `backdropLayer:didChangeLuma:`（只认 `v32@0:8@16d24`，记录后原样传下去），给右侧一颗（“右”）、面板第一颗（“面”）、顶栏（“顶”）的层打标签，记录 48 条“时间 标签=亮度”，以及“盖住/露出/重读/面板开”事件。下一步看：面板开到第一条“面=”隔多久；露出后重读有没有带来新的“右=”。

## 0.14 3.11.12（用户实测 3.11.11，附诊断和四张截图）

用户：点按、拉伸都没问题；图标大小和变色都没问题（诊断 `墨色=系统 旧透明玻璃开关=开`，证实 3.11.11 的推断）。问题：
1. 切换视频时有一下深色，像打开双击面板那样但更快更轻。
2. 本来对的玻璃在呼出长按菜单之类后被锁在深色、色调较重，只有视频背景自己变化才恢复（截图：图文作品白底上右侧四颗是重的深灰；过一会儿同一张图上是浅的）。
3. 提示胶囊出现过一瞬间是圆角矩形。
诊断：`按压=系统栏按钮自己的（接管149个控件 按下12 抬起10 取消2）`；示例 `亮度跟踪[0个 频率{}] 边内部[空（0个子视图）]`——右侧系统栏按钮里和它们自己的边里都找不到跟踪亮度的背景层（顶栏那条整屏大小的边里有 `ScrollEdgeEffectView`/`PocketBlur`/`LuminanceAdjustment`，tracksLuma 频率 0.25）。

3.11.12：
- 2：`ALBFaceWatchCover`（在 `ALBToneTick` 里，每 0.25 秒）：找一颗中心在屏幕内的右侧系统栏按钮，看上层窗口（同级排在后面或更高级）的 hitTest 是否接走这个点、主窗口 hitTest 是否落在它自己的按钮里；连续两次（0.5 秒）盖住才算盖住，“盖住→没盖住”后 0.3 秒再确认一次，`ALBFaceRefreshEdges`：每颗有登记的栏按钮去掉登记、去掉旧的空滚动视图，下一轮主循环 `ALBFollowSyncEdge` 建新边、新登记（新登记会重新判断，面板就是这样）。`UIApplicationDidBecomeActiveNotification` 后 0.4 秒也做一次。开关 `glass.refreshAfterCover`（默认开）。推断的原因：被盖住时系统读到暗→深；盖子走后它只在背后亮度变化时才更新。代价：每次弹层关掉后右侧先深约 1 秒。
- 1：没改。新出现那一格的栏按钮先显示这一格之前的深浅，背后变化后才更新。要去掉它（和面板的 1 秒）得找到系统读亮度的地方再“播种”（`_UILumaTrackingBackdropView unpauseAfterSeedingWithLumaLevel:`、`_UIViewGlass setAdaptiveInitialLuminance:`）。诊断加“全窗口：跟踪亮度的背景层/跟踪视图（暂停数）/例”，以及栏按钮里所有层（不只一层子层）的“亮度跟踪[个数 频率 背景层数]”。
- 3：提示胶囊 `cornerConfiguration = capsule`，布局后 `cornerRadius = 高/2`（两行时高过 56 点，固定 28 点圆角成了圆角矩形）。

## 0.13 3.11.11（用户实测 3.11.10，三段式底栏，附诊断和截图）

用户：除双击面板要过一下才切换外，别的玻璃色调、透明度都没问题了（顶栏也对了：诊断 `登记=UIScrollView/上 边深`，证实“边的深浅决定材质”）；面板真正切换后文字颜色也跟着。问题：右侧和面板丢了按压拉伸扭曲，只剩微微放大；右侧按钮变小（截图对比：玻璃大小已回到 3.11.4，**字形**从约 22 点变成约 16 点）；图标中心“自动”颜色不生效、一直黑色（截图：深色玻璃上黑色图标）；无专辑时音乐把玻璃套在“听抖音”的小耳机图上（截图：约 29 点的玻璃套在耳机上，后面是抖音 44 点的灰色空唱片）。诊断：`边内部[空（0个子视图）]`（自己的小边里没有边缘效果视图）；顶栏的边里有 `ScrollEdgeEffectView`、`PocketBlur`、`LuminanceAdjustment`，`tracksLuma` 频率 0.25。

3.11.11：
- 按压：`glass.facePress`（默认开）。`ALBFaceSyncActions` 让栏按钮接触摸、给一个空动作；`ALBFaceTouchAdopt` 把栏按钮的控件换成运行时子类 `ALBFaceTouch_<原类名>`（`-class` 返回原类），四个 touches 方法先走原实现再原样转给玻璃按钮的 touches 方法（按下时记住是哪个按钮，弱引用）；`gestureRecognizerShouldBegin:` 对挂在上层的手势一律 YES。`ALBFaceSetPressed` 在这时不再放大。诊断“按压=系统栏按钮自己的（接管 n 个控件 按下/抬起/取消次数）”。**风险**：如果点按又不灵，先看抬起/取消次数；关掉开关即回到 3.11.10。
- 字形：`ALB3BarGlassButton.updateContent`，`customImage.isSymbolImage` 时用 `SymbolConfiguration(pointSize:weight:scale:)` 直接配置（右侧 Medium、面板 Regular）。
- 自动颜色：`AWEConfigureHomeActionSlot` 里 `AWEHomeTransparentGlassEnabled()` 那一支在 `ALBBarFacesWanted()` 时不走。未用诊断确认用户那个旧值是开的（推断：这是“自动”能变成固定黑色的唯一来源）；诊断示例加了“墨色、旧透明玻璃开关”。
- 音乐：`ALBMusicArtwork` 找到的图边长 < 34 点就当没有封面（走原来的无封面路径：44 点按封面位置，放在 stack 最底下）。
- 没做：面板约 1 秒才切换（新登记的通病）。诊断加了“亮度跟踪[个数 频率]”。

## 0.12 3.11.10（用户实测 3.11.9，附诊断和两张截图，三段式底栏）

用户：面板长条材质不对，顶栏材质不对，右侧头像按钮和音乐按钮无玻璃，但点击能展开菜单。诊断：`面板登记12/24`、`每行文字=白黑白白白白白白黑黑黑黑`、`用时106.9毫秒 跳过：上次太慢，暂停中`、示例 `控件 36×36 玻璃 48.2×48.2（_UINavigationBarPlatterAnimationView）链[_UIButtonBarButton36>ItemWrapperView36>PlatterItemView裁36>SubviewContainerView44>PlatterGlassView裁44>PlatterContentView44]`、顶栏 `登记=UIScrollView/上`（3.11.8 的边，在 ALBDockOverlayView 里）。截图：面板图标（当时被误登记）是右侧那种通透的玻璃，文字胶囊和滑条是更白、更不透的浅色；顶栏在偏暗的背景上是白色浅玻璃。

结论和改法：
- **系统栏按钮量错了玻璃**：`glassFrame` 取了最大的候选（按压动画的 48.2 点），真正的玻璃是 `PlatterGlassView`（44）。3.11.7 截图里方块头像外的玻璃圈＝方块的 1.21 倍（44/36＝1.22，48.2/36＝1.34）；3.11.9 右侧玻璃比 3.11.4 小约 10%。所以每颗系统栏按钮只有设定的 91%，头像/音乐的图（画布＝玻璃设定大小）盖住了整块玻璃。改：`glassFrame(around:in:ownGlass:)`，系统栏按钮（`ALB3BarGlassButton.measureGlass`）传 true，优先 `PlatterGlassView`；顶栏（`ALB3TopChromeView`）照旧。
- **边的深浅决定材质**（推断，四次实测都对得上）：看对的——液态底栏下的顶栏（登记在底栏那一页，`AWEAlbumMorphOverlayWindow` 视频页 Dark）、3.11.7 起右侧（边在 Dark 的按钮里）、3.11.9 面板图标（误登记，边在 Dark 的面板里）；看错的——3.11.4/3.11.5 右侧（边在抖音的视图里，主窗口 Light：“长时间白色浅色调、透明度不高”）、三段式下顶栏（feed、顶栏下、ALBDockOverlayView 里，都在 Light 的主窗口）、3.11.9 面板胶囊（插件直接设 Light，不登记）。即：系统从边的深浅出发并偏向它；深色边上的“变浅”是通透的那种，浅色边/插件设的 Light 是更白的另一种。改：`ALBFollowMakeProxy` 设 `overrideUserInterfaceStyle = Dark`；三段式顶栏的边按 `ALB311TopEdgeStyle`（视频页 Dark，其他页 `ALB33PageAppearanceStyle`，同液态那一页）。诊断里登记后面带“边深/边浅”，下次可以直接核对。
- **面板**：跟右侧同一规则（`glass.adaptive`）。关：图标、胶囊的系统栏按钮各自登记在自己的深色边上（`ALBFaceLayout` 只在插件切时给栏按钮打不登记标记），两条滑条（玻璃按钮）也登记；墨色交给系统（`ALBPaletteInkIsPlain`：自动颜色的黑白→nil；用户设的颜色、点赞粉/收藏黄照旧）；滑条名字用 labelColor；`ALBFaceSyncStyle` 系统切时忽略行深浅。不取样（`ALBToneSample` 只在 tones 时读面板；`ALBToneSamplePanelNow` 只在 tones 时取；tick 关着就返回）。开：3.11.9 原样。面板按钮自己永远不登记（3.11.9 图标第一次布局就登记、没人撤）。
- **提示胶囊**：系统切时 Dark＋自己的深色边，不取样；插件切时同 3.11.9。
- 已知：新登记要约 1 秒才判断深浅（面板、提示胶囊一出现时偏深）。诊断“边内部[…]”列出系统放在边上的口袋/亮度跟踪（`_UILumaTrackingBackdropView` 的亮度级、暂停，`CABackdropLayer` 的 tracksLuma、lumaUpdateRate），给下一步（播种初始亮度或提高频率）用。
- 没做：团购。

## 0.11 3.11.9（用户选择）

用户选：双击面板＝“系统栏玻璃，插件按背后切”（每颗图标、每条胶囊各自，打开第一帧就对，阈值可调）；提示胶囊＝“和面板同一种”。

3.11.9：
- 面板：`ALB5PaletteIcon` 的 `refreshArtwork` 末尾 `ALBPaletteFaceIcon`（图标图、颜色、点数、透明度、填充、内边距记成规格，`ALBFaceNoteSpec`），文字胶囊 `ALBPaletteFacePill`（标题、文字颜色、对齐：面板在右＝靠右、在左＝靠左），都打 `kALBFollowNoEdgeKey`（永不登记，`ALBFollowSyncEdge` 和 `ALBFaceLayout` 都认）；滑条不铺。图标 `setHighlighted:` 有按下效果。
- 每一行的深浅：`ALBToneNotePanelRows` 在 `glass.followBar` 开时读胶囊旁边（`ALBToneBesideLuma`，面板在右读左边、在左读右边）、按 `glass.lightThreshold`＋0.08 防抖；`ALBToneApplyPanelInk` 用同一结果定文字颜色，并把这一行图标和胶囊的栏按钮设为 Light/Dark（`kALBFaceStyleKey` → `ALBFaceSyncStyle`；没有栏按钮的，如滑条，设按钮自己的 `overrideUserInterfaceStyle`）。打开时 `ALBToneSamplePanelNow` 在淡入前读一次；开着时每 0.5 秒（即使“插件按阈值切”关着也读面板）。
- 提示胶囊：`ALB25MaterialFollowEffect()`（`UIGlassEffect +effectWithGlass:` 装底栏/顶栏读到的玻璃，没有时 Regular），出现时 `AWECreateLowResolutionBackdropSnapshot` 读胶囊位置 3×3 中位亮度，按阈值定 Light/Dark。
- **没做**：滑条换成栏按钮；图标呼吸动画在栏按钮上不再有。

## 0.10 3.11.8（用户实测 3.11.7，附诊断和截图）

用户：**玻璃对了**（右侧系统栏按钮）。问题：点击不生效；头像变成方的；三段式下顶栏仍和液态底栏下不同；双击面板打开第一秒是偏深的色调（不知道该用哪种）；提示胶囊（`AWEShowDownloadToast`）也不知道该用哪种。诊断：系统栏按钮 27 颗、登记 27，示例 `控件 36×36 玻璃 48.2×48.2（_UINavigationBarPlatterAnimationView）登记=UIScrollView/左 风格=2/2`；顶栏 `登记=UIScrollView/上`（3.11.7 的自己的边）。截图：头像是圆玻璃里一个小的圆角方块照片。

3.11.8：
- 栏按钮**不接触摸**（`ALBFaceSyncActions` 关掉交互、去掉动作和菜单；手势旁观者和控件中继已删）。推断的失效原因：系统控件在触摸下会挡住祖先上的单击手势（UIControl 对 `gestureRecognizerShouldBegin` 的处理），而抖音右侧的点按是这种手势（动作桥把它挂在玻璃按钮或抖音按钮上）。触摸回到 3.11.6 的路：玻璃按钮自己的 touches 交给抖音、动作桥、头像菜单在按钮上（`ALBRGMenuBegins` 照常）。按下效果：两个按钮类的 `setHighlighted:` → `ALBFaceSetPressed`（栏按钮控件高亮＋玻璃 1.08 倍弹簧）。
- 头像方块：`ALB3BarGlassButton.layoutOverlay` 把图放进控件（约 36 点），控件在 iOS 26.6 裁剪。现在控件到导航栏之间、比玻璃小的视图都关掉裁剪；`diagnosticsText` 加“链[类名 裁/罩 宽度…]”。
- 三段式下顶栏的边：放进 `ALBDockOverlayView`（主窗口最上面那层）的最底下、和它一样大——相对顶栏的位置同液态底栏那一页（在一切抖音内容上面）；没有这层时才放在顶栏正下方。
- **双击面板、提示胶囊**：等用户选（见下一节回复里的问题）。面板“第一秒偏深”是登记的通病：面板每次打开都是新的，系统要约一秒才判断出深浅。

## 0.9 3.11.7（用户：两种底栏都试过，三段式下顶栏不对、液态下右侧不对）

用户选择：右侧和面板改成**真正的系统栏按钮**（和顶栏两侧同一种）；两种底栏都要能用。结论依据：3.11.5 在液态底栏下已把底栏的玻璃对象装进右侧玻璃按钮，用户看仍不对——“按钮里装同一块玻璃”做不出顶栏那种效果；顶栏两侧是导航栏按钮，玻璃、按压、文字翻色都是系统栏自己做的。

3.11.7（`ALBBarFace.h`，新文件，Tweak.x 在 `ALBNativeInteraction.h` 后 include）：
- `glass.barFaces`（默认开，需 `glass.followBar`）：右侧每颗玻璃 UIButton（`ALBRightGlassButton` 四颗、`ALBArtworkGlassButton` 头像／音乐、底板）里铺一颗 `ALB3BarGlassButton`（ALB3Chrome.swift，3.0 时代的单项导航栏玻璃按钮，之前一直没用），铺满、在最上面；按钮自己的子视图全部隐藏（每次布局隐藏，摘掉时还原）。内容由穿衣处记下（`ALBFaceNoteIcon`／`ALBFaceNotePlate`）：保留原色的图（头像、音乐的画布图）→ `setOriginalImage`，图标 → 模板图（用户颜色或系统颜色，系统颜色时跟玻璃翻）；底板 → 透明 1 点图（只有玻璃）。
- 登记：栏按钮登记在自己身下的边（`ALBFollowSyncEdge(face)`，边在按钮里、栏按钮下面）；按钮本身不再登记（`ALBFollowSyncEdge` 遇到有栏按钮的按钮＝不登记）。“插件按阈值切”开时栏按钮的深浅＝按钮的深浅（`ALBFaceSyncStyle`），不登记。
- 触摸：点中栏按钮的控件（系统自己的按压）；`AWEFeedVideoButton` 的命中钩子调用的是玻璃按钮的 `hitTest:`，所以会落到栏按钮里。按钮上挂一个只看不管的手势 `ALBFaceTouchMirror`（不识别、不取消、不延迟），触摸开始时给按钮发 `TouchDown`（动作桥计数），并把同一组触摸交给 `nativeTouchSource`（和 `ALBRightGlassButton` 自己的 touches 方法一样）；栏按钮的动作发 `TouchUpInside`。抖音挂在祖先上的手势照样收到（栏按钮是它们的后代）。按钮上落在圆形玻璃外的触摸不归任何人（`ALBFaceFilterHit`）。会在点按时弹菜单的按钮（头像，`showsMenuAsPrimaryAction`）把菜单交给栏按钮（菜单从栏按钮长出来，像左上角菜单），`ALBRGMenuBegins` 不再被调用。
- 三段式底栏下 `top.e2`：`ALB311TopOwnEdge`——顶栏正下方一块窗口大小的空滚动视图，顶栏登记在它的上边（原来退回视频流）。
- 独立审查（提交前）改过的：触摸的“结束”改由栏按钮的控件自己报告（`ALBFaceControlRelay`：抬起＝touchesEnded，被取消＝touchesCancelled），否则被别的手势取走的触摸会以“结束”交给抖音、动作可能触发两次；控件没报告时事件结束后补一个取消。`AWEFeedVideoButton` 命中钩子：玻璃按钮返回空（圆形玻璃外的角）时回到抖音自己的命中，不再落到视频上；不再把按钮的可交互状态复制给栏按钮（新按钮绑定动作前是不可交互的）。`AWEEnforceExplicitForeground` 不再把有栏按钮的玻璃按钮自己的图标显示出来。栏按钮只在穿衣签名变了时重画（原来每次刷新都是新的 UIImage 对象，会每轮重渲染、重测量）。栏按钮被按着时玻璃按钮读作“在按”（`ALB3BarGlassButton.faceTracking`，两个按钮类的 `isTracking`），原有的“按着时不改”全部生效。头像菜单设好后立刻交给栏按钮，每次布局也同步。
- **没做**：双击面板（图标、文字胶囊可以用 `ALB3BarGlassButton` 的 fillColor／setTitle，滑条要另想）。
- **风险**：触摸和菜单走了新路；每颗自己的边可能仍偏容易变浅（这是系统的判断，阈值不可调；不行就开“插件按阈值切”或关“各算各的”）；每颗栏按钮是一个 UINavigationBar，比按钮重。

## 0.8 3.11.6（用户实测 3.11.5 后：“应该还是不行”，附一份诊断）

诊断：用户这次在**三段式底栏**下（`三段式底栏：在主窗口`）。没有液态底栏可读（`底栏读取=unseen(深无/浅无)`），顶栏的系统玻璃也没读（`顶栏读取=无`：`ALB25MaterialCaptureRegisteredGlass` 只在 `right.registeredGlass` 开时读，用户关着），所以“用底栏自己的玻璃”退回了常规配方：`顶栏同款：…玻璃=recipe`，【右侧】`smoothness: 0.0`。登记仍成功（右侧 6/6）。顶栏在三段式下 `top.e2` 退回登记到视频流（`登记=AWEFeedTableView/上`）。

3.11.6：`glass.followBar` 开时也读顶栏的系统玻璃（分组平台的 `_UIViewGlass`，和底栏同一种，smoothness 6；两侧圆按钮的材质不是 `_UIViewGlass`，读取时被跳过），三段式下右侧和面板就装它。诊断列出 `_UIViewGlass initWithVariant:smoothness:` 等、`_UILumaTrackingBackdropView` 几个调用的参数类型编码（只读），为“自己造一块 smoothness 6 的玻璃”或“用系统的亮度跟踪”做准备。

**待证实**：两块玻璃的描述只差 smoothness（6 vs 0），换成底栏／顶栏的玻璃后看起来是否真的不同，要用户看；3.5 时用户看出过区别。

## 0.7 3.11.5（用户实测 3.11.4 后，附诊断和三张截图）

用户反馈：右侧按钮大部分时间是白色、透明度不高的浅色，只有很暗的背景才变深（“想让它变深的阈值太高”）；深浅两种的透明度都明显不是底栏那种；菜单能和按钮同色；右侧确实是各切各的；双击面板显然不是底栏同款。

诊断：`顶栏同款：右侧登记17/17（自己的边17）`、`登记=UIScrollView/左`——登记成功、系统在切。但 `玻璃成分=常规 实际=recipe`：用户的 `right.glassSource`＝2，`ALBInstallRegisteredEffect` 按它装了 UIKit 的常规配方。【右侧】玻璃 `smoothness: 0.0`，底栏和顶栏的是 `6.0`。3.5 的笔记早说过：常规配方不跟系统“透明／着色”，底栏实体化的玻璃才跟。

3.11.5：
- “用底栏自己的玻璃”一律装底栏读到的那块（`ALBFollowSourceGlass`：先深色、再浅色读数，没有时顶栏的，再没有才配方），不再经过 `right.glassSource`；签名带 `ALB25MaterialFollowGlassKey()`。
- 谁来切：`glass.adaptive`（设置名改为“插件按背后亮度和下面的阈值切深浅”）。关＝系统（`ALBFollowSystemDecides`，各自的边，同 3.11.4）；开＝插件按阈值切（3.11.3 的整窗取样，读按钮左边），玻璃仍是底栏的，覆盖风格 Light/Dark，不登记（系统的决定会盖过插件的）。
- 独立审查（提交前）改过的：关掉“插件按阈值切”时清掉每颗按钮和面板记着的深浅和亮度（再打开时从新读数开始，不会先用几分钟前的浅色）；面板材质签名变了时 `setNeedsLayout`（边跟着系统／插件切换）；顶栏玻璃读取在“用底栏自己的玻璃”开着时不受 `right.glassSource`＝2 限制（作为后备）；插件切成浅色时优先用底栏的浅色读数。
- 推测（未证实）：系统切得太容易变浅，可能和玻璃本身有关（配方玻璃 vs 底栏玻璃），换成底栏玻璃后也许就好；不好就用“插件按阈值切”。

## 0.6 3.11.4（用户实测 3.11.3 后）

用户实测：**顶栏成功**（设置：`top.e2` 开＝“顶栏玻璃与底栏一致”，`glass.clear` 关＝不用清透）。原话：不管从哪方面都太完美，深浅跟随、自己的文字颜色切换，和底栏一样；只有左上角展开菜单比按钮稍不透明（可以忽略）。诊断：顶栏是 UIKit 自己的栏玻璃（分组 `_UITabBarPlatterView` regular、无染色、smoothness 6，两侧是导航栏按钮 `_UINavigationBarPlatterGlassView`），`登记=UICollectionView/上`（液态底栏那一页的滚动视图）。顶栏在主窗口、登记的滚动视图在底栏窗口——**原地、跨窗口登记可行，没有卡白**（0.5 节里“会量到自己”的担心不成立，至少对顶栏）。用户这次仍关着 `glass.adaptive`，3.11.3 的取样没被测到。

用户要求：右侧和双击面板全部换成顶栏这种，**每颗按钮、每条胶囊各算各的**（背景差别大时一起切会突兀）。做不到的话再修清透模式的头像菜单锁深色、延迟。

3.11.4：
- `glass.followBar`（默认开，`ALB25MaterialFollowBar`）：`ALB25MaterialMotionButtonConfigurationTone` 改为工厂玻璃按钮配置＋`ALBInstallRegisteredEffect`（底栏自己实体化的玻璃，跟系统“透明／着色”；没读到时顶栏的、再没有时 UIKit Regular）——3.10 的 E / E2 用的就是它。我们的深浅（`ALBToneLightForView`）对右侧和面板一律不起作用，覆盖风格固定 Dark（E2 也是固定 Dark 照样会被系统翻浅）。
- 每块玻璃自己的边（`ALBFollowSyncEdge`，Tweak.x，`ALB3RegisterEdge` 后面）：玻璃正下方同一个父视图里放一个空的、不可点、不滚动、`scrollsToTop` 关、边缘效果全隐藏的 `UIScrollView`（`AwemeLiquidBar.GlassEdge`），同 frame，玻璃登记到它的左边（2，同 3.10.5 的 own edge）。`glass.followOwnEdge` 关＝右侧共用液态底栏那一页的右边（8），一起切（三段式下没有这页，退回自己的边）。右侧玻璃类 `ALBRightGlassButton` / `ALBArtworkGlassButton` 的 setFrame/setCenter/setBounds/didMoveToSuperview 同步这块边；面板在 `layoutSubviews` 末尾同步每颗图标和胶囊。原来把玻璃钉在最底层的两处（右侧四颗在抖音按钮里、拍同款底板）改用 `ALBFollowAtBottom`（把它身下的边算作“最底层”），避免两边轮流抢 index 0。
- 头像菜单：开着时 `ALBToneStyleMenus` 不碰头像菜单（系统菜单玻璃）；菜单的深浅由 `ALBRGMenuBegins` 照旧按菜单打开那一刻头像背后的小图定（自适应开时用阈值，关时 0.5）——菜单不是边缘元素，否则会跟着按钮固定 Dark。
- 独立审查（提交前）改过的：头像、音乐封面那处“玻璃必须紧贴在抖音的图上面”（`ALBPlaceArtworkButton`）也承认中间夹着自己的边（`ALBFollowRightAbove`），否则每次布局两边来回换层；玻璃签名带上底栏玻璃的键（`ALB25MaterialRegisteredSourceKey`），先建好右侧、后读到底栏玻璃或改了“透明／着色”时会重建一次；插件关掉时每颗右侧玻璃都撤掉登记和那块边；共用一条边时只在底栏那一页换了时才重新登记。
- 面板每行文字：开着时即使 `glass.adaptive` 关也取样（读这一行显示出来的样子，含玻璃，所以系统把玻璃翻浅后文字会跟着变黑；3.11 以前就是“系统翻玻璃＋取样定文字”这一组合）。
- **没做**：右侧图标颜色跟玻璃翻（图标是抖音的图，固定白色；系统只翻默认颜色的系统符号）。如果系统把右侧玻璃翻浅了，白图标会不清楚——下一步可以用“读显示出来的玻璃”定黑白（同面板文字）。
- **风险**：3.10.x 时“每块自己的边”的顶栏比共用边更容易翻白；E2 玻璃每次重新出现会先深后浅几秒。这两条在原地按钮上会不会出现要实测。
- 团购（这次在团购页、已下滑）：外层 `AWEGrouponC2CollectionView` 被抖音改回 y150-873（偏移 218），里层 `AWEGrouponLifeFeedsCollectionView` 被抖音跟着滑动重新定位、改高度（y148-837，顶部时是 y277-878）——里层不是跟着外层平移的，3.11.3 的“按外层在顶部量”量不准，没延长。分类下面那条是 `UICollectionReusableView y150-191` 白色、吸顶的分区头（滑下去后才吸在那里），清掉的话卡片会从分类后面透出来。改框架这条路和抖音一直在打架，下一步要换做法（这一版没动）。

## 0.5 3.11.3（用户实测 3.11.2 后，附一份诊断）

用户反馈：右侧、双击面板、展开菜单、团购、顶栏“照旧”；别的页面色调重、不随背景切换、锁深色。用户的总结：底栏最完美（该透时透、该不透时不透），顶栏和右侧做不到；“它就是最简单的系统材质跟随背景变化”。

诊断（视频页）证明的事：
- `深浅自适应=关`：用户把总开关关了，按设计全部固定深色。
- 但开着也没用：唯一一次取样，右侧五颗亮度全是 **0.00**、顶栏 0.00（用时 25.7 毫秒、画了 3 块）。3.11.2 一块块对视频自己的视图 `drawViewHierarchyInRect`，**离开窗口这样画，视频层画不出来**（Tweak.x 7589 行的注释早写过：视频/Metal 层用 renderInContext 画不出来）。3.11.1 画根视图同理（视频是黑的，读到的只是按钮和玻璃）。而头像菜单一直用的 `AWECreateLowResolutionBackdropSnapshot`（对**窗口**画，渲染服务器出图，含视频）——用户 3.11.1 说过“展开菜单能变”，3.11 以前双击面板每行文字黑白也是它，都好用。
- 顶栏左按钮结构“没找到左按钮”：两侧按钮是 `ALB3PassThroughNavigationBar` 的导航栏按钮，不是 `ALB3BarGlassButton`，探针找错了类。
- 团购：`AWEGrouponC2CollectionView` y150-956 已延长，但真正的内容列表 `AWEGrouponLifeFeedsCollectionView`（在外层列表的格子里）y277-878 **没延长**——回到顶部时底栏后面露出的就是它下面的底色。
- 顶栏现在登记在视频流（`登记=AWEFeedTableView/上`，`top.e2` 关）：非视频频道（团购、商城）背后不是视频流，系统量不到，深浅卡住——这就是“别的页面锁深色”。

3.11.3 改的：
- 取样改成对主窗口画（`ALBToneCaptureWindow`：和 `AWECreateLowResolutionBackdropSnapshot` 同一个调用——对窗口 `drawViewHierarchyInRect`，这里 12%）。窗口里有我们自己的玻璃，所以每块玻璃读旁边：右侧、面板读左边 16/25/34 点（3.11.1 的读法），顶栏读玻璃行下面 10/22 点（`rowCenterY`/`contentHeight`），三段式底栏读它上面 8–24 点（底栏覆盖层在主窗口里），面板每行文字读这一行上下边 14%/86%（含玻璃，3.11 以前的读法）。页面上没有要读的玻璃时不画。诊断加“整窗平均亮度”（0.00＝又没画出来）。
- 团购：延长时也在找到的列表里面找嵌套的竖向整宽列表；所有列表的缺口都按“外层列表都在顶部时”的位置量（`contentOffset.y + adjustedContentInset.top` 累加），外层在动时不量。
- 独立审查（提交前）改过的：面板在左半边打开时左边读不到，改读右边，面板自己的深浅不再用它自己的行（含它自己的玻璃，会卡住）；“太慢暂停”对翻页停稳那一次和面板打开那一次也有效（超过 25 毫秒停 3 秒，超过 60 毫秒停 8 秒）；三段式底栏读它上面 16–32 点（原来 8 点会读到它自己的边）；团购里层列表：去掉父视图裁剪也按“外层在顶部”量、只占它下面的空地（它下面在外层列表里有别的东西就不延长）、抖音的格子立刻改回两次就不再延长（不拉锯）、往里找时不进找到的列表、最多 5 层。
- 诊断：顶栏导航栏左半边的视图树（含 `_background`、glassEffect、层）；系统亮度跟踪类（`_UILumaTrackingBackdropView`、`CABackdropLayer` 里带 statistic/luma 的方法名，只列名字）。

关于“跟底栏一样”（结论写给下一个人）：
- 系统自己切深浅的前提是**登记**到某个滚动视图的边（第 7 节：对照玻璃 E 登记到底栏页的 `UICollectionView` 才和底栏一模一样；不登记的玻璃不会自己切）。底栏完美，是因为它登记在自己那一页的滚动视图上、而且**在这个滚动视图的上面**（同一个窗口里，系统量背后亮度时不会量到底栏自己）。
- 原地的玻璃（主窗口里）登记到底栏窗口的滚动视图，系统量的那一片里会有它自己：玻璃一变白，量到的就更亮——“卡在白色”的可能来源（推断，没证实）。E2 放进底栏窗口没有这个问题，但带来了逐帧同步、重新出现先深后浅、菜单弹不出等一串问题（3.10.x 历史）。
- 所以 3.11.3 继续走“自己读背后、阈值可调”这条路，先把读不到视频这个根本问题修掉。顶栏在非视频页可以让用户先试 `top.e2`（顶栏与底栏一致）——不用编译，看非视频频道是否跟着页面变。

## 0.4 3.11.2（用户实测 3.11.1 后，附两份诊断和截图）

实测数据（诊断）：
- 用户自己调的值：阈值 0.40、深色染色 0.11、浅色染色 0.00、顶栏放到最上层＝关（顶栏原地）、菜单跟随＝开。
- **取样用时 115–142 毫秒**（3.11.1 画整个根视图，里面全是玻璃）→ 主线程每次卡一下。3.11.2 只画视频画面本身（`ALBToneMediaViews`：从抖音右侧按钮列往上到视频格，每一层在它下面的兄弟视图，不含按钮、玻璃、顶栏），用时显示在诊断。超过 25 毫秒停 3 秒。
- 顶栏：分组胶囊的玻璃确实换成了我们的（诊断里分组平台的 `_Glass` 是 clear＋0.105 黑 / clear＋白 0），但**两侧按钮没有任何 KVC glassEffect 视图**（“玻璃1处”只有分组），所以两侧还是 UIKit 自己的自适应玻璃、容易变白。3.11.2 在顶栏里额外处理“系统玻璃按钮”（配置的私有材质，和右侧同一办法），并在诊断里列出左按钮的结构（“顶栏左按钮结构”）：哪个视图有 glassEffect / `_background` 玻璃（含 adaptiveStyle）/ 取样层 / 按钮配置。**下一步按这份结构定两侧按钮的改法。**
- 团购：延长后的列表 `AWEGrouponC2CollectionView` 外层被抖音改回 y150-873 后没有再延长（灰底 #F8F8F8 是 `AWEGrouponC2ContainerView` 自己的底色）；延长时出现 `UIView y115-203` 白色带（不延长时没有）＝用户说的“分类下面那条背景”。3.11.2：列表 layoutSubviews 时发现被改回就立刻再延长（每秒最多 4 次），并清掉这条白带（`ALB34ClearGrouponBand`，插件关掉时还原）。
- 收藏页底部背景在三段式底栏下失效：内容延伸到底栏后面的处理都只认 `gAWEAlbumMorphVisible`（液态底栏可见）。3.11.2 换成 `ALBBottomBarFloating()`＝液态底栏可见或三段式底栏在屏幕上。
- 双击面板文字自动黑白失效：3.11.0 起是固定白色/整块面板一个深浅。3.11.2 恢复每行各自按背后亮暗（0.46/0.54 防抖，同 3.3），取样同上。
- 菜单：跟随按钮时先出现系统菜单的深色玻璃、过一会才换——3.11.1 是在菜单出现后 0–0.6 秒才换。3.11.2 在窗口加子视图后 0.6 秒内、每次 Core Animation 提交前换（第一帧就是按钮同款）；并有“展开菜单染色下限”`menu.minTint`（默认 0.35），用户把浅色染色调到 0 时菜单文字也能看清。
- 独立审查（3.11.2 提交前）改过的：频道判断每秒最多一次且滑动中不做（路由判断很重）；顶栏“系统玻璃按钮”只按配置换、不再往按钮里面钻（避免两处同时写），并按材质“成分”认“已经是我们的”（不再每秒重写）；团购白带只在本频道列表已延长时清、只认纯白、整 0.3 秒一次、不碰卡片；取样跳过也算一次（不会每 0.25 秒白找一遍）；找视频画面只停在占满屏幕的格子；面板正在拖音量／亮度时不取样；从个人页、搜索打开的视频（含全屏弹出）右侧和面板也取样（底栏、顶栏仍只在首页视频页）。
- 用户要求：清透只用在视频页（直播、竖屏视频），其他页面（商城、团购这类大片纯色）用系统玻璃跟随系统；主页作品/喜欢/收藏列表他也想要清透，但承认难以界定。3.11.2：`ALBToneVideoContext()`＝首页/朋友（三段式底栏下用它自己的页面判断）且不是团购/经验/商城/网格/个人页这类频道（`ALB25MaterialNonVideoChannel` + 团购容器 + 路由）；非视频页底栏和顶栏都用系统玻璃，不取样。主页的作品列表暂时也算非视频页。

## 0.3 3.11.1（用户实测 3.11.0 后的四件事）

用户反馈（3.11.0，两种底栏都试过）：
1. **顶栏**在不算很亮的背景上也变成接近纯白（两种底栏都这样）。原因：顶栏是 UIKit 自己的栏玻璃（`ALB3Chrome.swift`，导航栏按钮＋分组 UITabBar），抬到最上层后不登记，UIKit 自己按背后亮度切浅色，很容易变白；`glass.clear`（“最上层顶栏用清透玻璃”）其实只管设置页和提示条的玻璃，从来没碰过顶栏。
2. 右侧、面板、三段式底栏在纯白背景上也锁死深色清透——要“浅色清透”，阈值可调。
3. 两个展开菜单（头像、左上角）的透明度跟随按钮：先修好深浅锁死，再试整体更透明。
4. 右侧四颗玻璃到数字的距离，只有转发大一点。
5. 团购频道：底部透明时顶部分类下面出现一条底色；回到顶部时底部不透明、分类下没有那条底色。

3.11.1 的做法：
- **`ALBGlassTone.h`（新）**：右侧每颗玻璃、双击面板、三段式底栏（视频页）、顶栏各有深浅两种：深＝Clear＋黑色染色（`glass.darkTint`，默认 0.28），浅＝Clear＋白色染色（`glass.lightTint`，默认 0.12）、图标变黑；没开清透时是常规玻璃只切 Light/Dark。背后亮度＞`glass.lightThreshold`（默认 0.70）变浅，低于阈值−0.08 变回深（防抖）。`glass.adaptive` 关＝全部固定深色（3.11.0）。
- 取样：主窗口根视图（或盖在上面的全屏弹出页）的 12% 小图（`drawViewHierarchyInRect`，不编码、不存文件），只在静止时取：翻页停稳立刻一次，之后 3 秒内每 0.75 秒一次，再往后每 2.5 秒一次；滑动、三段式底栏动画、头像菜单、设置页打开时不取；单次超过 12 毫秒就停 3 秒（诊断里有用时）。定时器 0.25 秒、跑在 common 模式（才看得到“正在滑”），滑动中只做不费事的补换。三段式底栏、最上层的顶栏、双击面板在根视图外面（窗口层），小图里正好是它们背后的内容；右侧玻璃在小图里，所以取它左边一小块（离玻璃 16–34 点、单个像素，避开自己的边和上下的数字）。三段式底栏只在视频页取样，离开视频页就忘掉“亮”。面板打开时先取一次再淡入（不会先深再变）。
- 独立审查（3.11.1 提交前）改过的：右侧取样位置（原来一圈会碰到数字和自己的边）、底栏非视频页的残留浅色、顶栏换玻璃跳过选中胶囊整棵子树并防“每次读都是新对象”、顶栏每轮都补（原地也补）、菜单显示时不碰顶栏、先改深浅再写玻璃、重换后列自己的签名同步（不再重建第二次）、系统胶囊换深浅后 0.2 秒核对一次、顶栏玻璃不再被复制给消息页顶部按钮。
- 右侧：`AWEConfigureAdaptiveSystemGlassIconButton` 的签名和配置带上深浅（`ALB25MaterialMotionButtonConfigurationTone`），深浅变了 `ALBRGRetone` 0.25 秒换玻璃（按着或菜单期间不换，下一轮补）；`ALBRGDarkStyle` 改名 `ALBRGToneStyle`。头像菜单的深浅＝按钮当前的深浅（取样过的话），否则沿用截图取样（阈值同设置；自适应关时用 0.5）。
- 三段式底栏：`ALBDockMaterialNoteBackdrop` → `ALBDockGlassStyle` 视频页亮时 Light、`ALBDockMakeGlassTone` 白色染色；系统胶囊（applyNativePlatterGlass）同样换。
- 顶栏（`glass.clear` 开，默认开）：`ALB25MaterialApplyTone` 把顶栏里所有 UIKit 玻璃（KVC `glassEffect` 有值的视图、带玻璃的 UIVisualEffectView；选中胶囊/透镜类除外）换成 Clear＋当前深浅的染色，顶栏和按钮的 `overrideUserInterfaceStyle` 跟着深浅；`glass.clear` 关＝还原 UIKit 自己的玻璃（3.11.0 的样子）。最上层时每帧同步里最多 10 次/秒补一次（UIKit 重建按钮玻璃时）。**没验证**：导航栏按钮的玻璃能不能用 KVC 换（分组 UITabBar 的平台可以，2.5 起就这么换底栏）；诊断“【玻璃深浅】顶栏：…玻璃N处 换M”会说明。
- 菜单（试验 `menu.followGlass`，默认关）：全局 `%hook UIWindow didAddSubview`，开着时菜单出现后 0/0.06/0.15/0.3/0.6 秒把菜单里的玻璃换成按钮同款（头像菜单：头像按钮的深浅；左上角：顶栏的深浅且菜单在左上区域）；抖音自己的菜单不动。
- 右侧数字间距：四颗图标玻璃以“赞”的玻璃到它数字的距离为准，其余每颗按自己的数字（`ALB32RightTailLabel`，取文字上沿）对齐到同样距离（最多挪 12 点）；没有数字或没有赞时仍对准图标中心。诊断“右侧玻璃与数字间距”列出各自居中时的距离和挪了多少。
- 团购：**没改**，只加了诊断“【团购】”（列表的位置、偏移、是否被延长，底部和上部的底色），要用户在频道顶部和往下滑后各复制一次诊断。
- 设置：新的选项节“玻璃深浅”（第 6 节：自适应、阈值、深色染色、浅色染色、菜单跟随），挂在右侧、双击面板、三段式底栏、顶栏四个选项页下面，右侧卡片也有入口。

## 0.1 3.11.0 做了什么

**参考工程**：用户自己的实验工程（底栏三段式收缩＋右侧玻璃）。它的类名前缀**不许出现**在本工程和回复里（用户要求）。并入时所有名字都换成了 `ALBDock…`，偏好键换成本插件命名空间里的 `dock.*`，通知名、缓存文件名也换了。查找命令：`grep -rnE 'S[J]{2}|s[j]{2}' .`，应为 0。

**新底栏（可选，`bottom.style`=1，重启生效）**
- 文件：`ALBDock.m/.h`（底栏本体，逻辑照搬）、`ALBDockBridge.m/.h`（转发点击到抖音 `AWENormalModeTabBar` 的控制器、读选中页和角标）、`ALBDockMaterial.m`（视频页深色＋清透开关，其他页跟系统深浅）、`ALBDockAvatar.m/.h`（球上的本人头像：先用插件已读到的 `ALB8OwnAvatar`，再探测账号、扫“我”页、相册选择）、`ALBDockCommon.h`（玻璃、偏好、颜色）、`ALBDockHooks.x`（接入，独立 Logos 文件，`%group ALBDockGroup` 只在 `ALBDockModeActive()` 时 `%init`）。
- 选了它：`Tweak.x` 的 `AWEBottomBarDisplayEnabled()` 返回 NO，走插件原有的“释放底栏”路径（液态底栏不安装、抖音原底栏还原），再由 `ALBDockHooks.x` 把抖音原底栏透明度钉成 0、不吃点击（**只在插件已授权且总开关开时**，`ALBDockHostPluginActive`；总开关关掉或授权失效时把抖音自己要的透明度和点击还回去，`ALBDockApplyNativeBlank`，0.35 秒定时器也会检查），在主窗口最上面放一层穿透覆盖层装底栏，全屏挂一个只观察的上滑手势。
- 和本插件的接口（Tweak.x 导出）：`ALBDockModeActive`、`ALBDockStyleStored`、`ALBDockHostVerticalOffset`（“底栏垂直位置”，正值向下，0＝原工程位置；默认值 3）、`ALBDockHostAllowsDock(nativeBar)`（插件总开关、清屏、插件设置页打开、弹出页面盖住抖音底栏——和液态底栏同一个判断 `AWEAlbumMorphNativeBarCoveredByPresentedSurface`；窗口加子视图时立刻判断一次）、`ALBDockHostPluginActive`、`ALBDockHostOwnAvatar`。改垂直位置时设置页发 `ALBDock.VerticalOffsetChanged`。
- 偏好：设置页统一发 `ALB5.OptionsChanged`，`ALBDock.m` 的 `optionsChanged` 比对 `dock.*`（和 `glass.motionClear`）哪几项变了，再按原工程的 `prefsChanged:` 逐项处理。滑块松手生效（原工程是拖动实时；设置页开着时底栏本来就让开），`dock.*` 除速度外按整数存（同原工程）。
- 球上的头像：`+image` 现取（手动／账号／抓取的优先，没有才用 `ALB8OwnAvatar`，不存进缓存），插件的 `AwemeLiquidBar.OwnProfileChanged` 会转成 `ALBDockAvatarChanged` 让球刷新；账号头像下载完回主线程再写。
- 诊断里有“三段式底栏：…”一段（原工程的只读几何报告 `debugReport`）。
- 没带过来：中间动效胶囊及其参数、流星雨、悬浮设置按钮、探测、原工程的右侧玻璃模块（画法并进了插件自己的右侧）、头像测试菜单、自绘按压形变（用户说会导致异常位移）、展开液滴开关（原工程底栏里没用到）。

**右侧按钮（两种底栏都一样）**
- 仍是 2.5.3 起插件自己的原地按钮（`AWEHomeActionSlotView` 的 `ALBRightGlassButton`、头像 `AWEHomeProfileGlassView.menuButton`、音乐 plate，都是系统玻璃 UIButton，图标是配置里的图，所以按压时图标和玻璃一起原生拉伸扭曲——用户要保留的“图标变形机制”），右侧缩放、补位（`ALB5RightLayout.h`）照旧。
- 画法换成参考工程的：`AWEConfigureAdaptiveSystemGlassIconButton` / `…PlateButton`（只有右侧在用）现在只做一件事：`ALB25MaterialMotionButtonConfiguration()`（系统玻璃按钮自己的 Regular 玻璃；`glass.motionClear` 开时装 Clear＋28% 黑），固定 Dark（`ALBRGDarkStyle`），不登记、不进注册表（`AWEAdaptiveGlassButtonRegistry` 现在是空的，`ALB25MaterialApplyButton` 不再碰右侧）、不取色。
- 位置：玻璃按钮放进抖音自己的按钮（`ALBBindOriginalGlassAction`），右侧四颗（标识 `AwemeLiquidBar.HomeAction.NativeButton`）**插在最底层**（弹幕发送玻璃仍在最上面，同 3.10）（index 0，在抖音的数字、角标、动画下面；点击仍由 `AWEFeedVideoButton` 的 hitTest 先交给玻璃）。圆心＝抖音图标的中心（`AWEConfigureHomeActionSlot` / `AWELayoutBoundHomeActionSlots`，去掉了原来的 −4），直径 `right.glassSize`（默认 48），可 `right.glassOffsetY` 上下微调。
- 头像、音乐：直径＝抖音头像／封面的边长（取整）＋2×`right.glassRing`（默认 4）；图画在和玻璃一样大的画布正中（`ALBInstallNativeArtwork` 多了 canvas 参数）。布局后 0.05 秒量一次 UIKit 真正画图的那一层，偏离中心超过 1/3 点就用内边距推回（每张图最多 4 次，`ALBArtworkMeasure`）。
- 头像菜单（`ALBNativeInteraction.h` 末尾 `ALBRGMenuBegins/Ends`）：菜单开始时截一张小图，头像外圈 8 点中位亮度 >0.5 用浅色，否则深色；菜单期间任何配置和深浅都不改（`ALBRGMenuActive`，20 秒兜底过期；源图换了＝格子已复用，标记立即清掉）；结束后 0.3 秒动画回到深色（超过 20 秒的菜单结束时也回）。
- 删除：E2（底栏窗口里的玻璃、自己的边、按背景取色、角标贴图、按压镜像 `ALB39*`）、3.7–3.9 镜像、右侧注册到视频流（`ALB32RegisterRightRail` 现为空函数）、`right.e2` / `glass.ownEdge` / `glass.sampleBehind` / `right.followDouyin` / `right.registeredGlass`（对右侧）/ `right.glassSource`（对右侧）、玻璃对照、`ALB311GlassLog.h`、`ALB312Probe.h`。

**双击面板**：回到抖音的窗口（不再进底栏窗口），每颗图标、每条胶囊 `ALBPaletteGlassConfiguration()`＝同一种玻璃，整块面板 Dark，文字白色（`panel.customTextColor` 开时用自定义色），不再截屏取色、不登记。

**顶栏**：`ALB37GlassLayer.h` 重写为只剩顶栏：`glass.lifted`、`glass.inBarWindow`、`top.e2`（三段式底栏下没有液态底栏的滚动视图，`top.e2` 自动退回登记到视频流）。`top.e2` 开、顶栏在原地时，run loop 观察者每 0.5 秒重新登记一次（原来靠右侧镜像让观察者转起来，现在用 `gALB37TopE2InPlace`）。

## 0.2 3.11.0 没验证、最可能出问题的地方

1. **右侧玻璃在最底层**：抖音按钮里如果有不透明的背景子视图，玻璃会被挡住（参考工程同样放在 index 0，它看起来是好的）。
2. **48 的玻璃和数字的距离**：圆心对准图标后，玻璃下沿比原来低约 6 点，可能压到数字上沿（数字画在玻璃上面）。用户可调直径、上下微调。
3. **头像、音乐居中**：靠整数画布＋测量校正；测量找不到画图的层时不校正（不会出错，只是不校正）。
4. **头像菜单**：菜单从深色头像玻璃长出、按背景切深浅；收起后回深色有 0.3 秒过渡。
5. **三段式底栏**：和插件其它依赖液态底栏控制器的功能（收缩手势、底栏标题、未读角标设置、iOS 27 同窗口宿主）在这个模式下都不起作用；抖音在朋友页、详情页隐藏自己的底栏时，三段式底栏也跟着隐藏（参考工程原样）。插件设置页、相册、操作表等弹出页面盖住底栏时它让开（参考工程没有这条，是 3.11 接入时加的）；抖音自己的评论区如果不是系统弹出页面，则和参考工程一样不让开。
6. 面板文字固定白色：清透开关打开且背景很亮时可能看不清（参考工程同样固定白色）。

---

## 历史（3.10.x E2 时期，3.11 已删除对应代码，只留结论）

### H1 3.10.11 的专项探针结论（3.10.12 据此改；E2 已在 3.11 删除）

- **位置没问题**：滑动中和静止时，E2 玻璃和抖音自己按钮的差都在 ±0.2 点内，和抖音数字的距离恒定（18.8–19.1 点）。玻璃已经贴着抖音走。
- **深色不是主窗口画的**：原地按钮画玻璃 0 次（菜单期间按设计的除外）；主窗口右侧“其他玻璃”9 个，全是插件自己：头像菜单期间原地按钮的干净玻璃（按设计），以及 `AWEHomeProfileGlassView`（类名带 Glass 的空容器，探针误报，3.10.12 已改成只认真正画玻璃/模糊的视图）。抖音没有画深色玻璃。
- **深色是 E2 玻璃自己**：右侧和面板的玻璃在底栏窗口里，`overrideUserInterfaceStyle` 固定为底栏风格（记录里一直是 2＝深），唯一能让它变浅的是登记在它下面那块看不见的空 `UIScrollView`（自己的边）上的系统机制。玻璃每次重新出现（新视频、关评论区后、头像菜单后——菜单期间玻璃是隐藏的、双击面板——每次打开都新建），这个机制都从深色重新开始，几秒后才到对的样子。头像菜单**收起动画中**看到的是原地按钮那颗**不登记**的普通系统玻璃，用户说它“透明度跟目标差不多”，收起完成换成 E2 玻璃就变深——这是关键证据。
- 3.10.10 的“只调透明度”当时没有出现“菜单后变深”的抱怨，3.10.11 撤回后又出现，和上面一致（隐藏→显示会重来）。面板先深再变在两版都有，因为面板每次新建，和“只调透明度”无关。3.10.11 撤回它的理由（怀疑它让面板发深）站不住。
- **3.10.12 的改动**（开关 `glass.sampleBehind`，默认开）：右侧和面板的 E2 玻璃不再登记到任何边；深浅在玻璃出现的那一刻读它身后抖音画面（`AWECreateLowResolutionBackdropSnapshot`，所有玻璃和面板共用一张，`ALB313Backdrop`）的亮度来定：明显亮（>0.62）才浅、明显暗（<0.48）才深，第一次 0.58 为界；视频流停住时每 0.5 秒再读一次，滑动中不读（只在出现时读一张）。面板在布局完、淡入之前读一次（原来第一次读取发生在布局之前，行的尺寸是 0）。关掉开关就是 3.10.11 的登记做法。
- **未验证的风险**：不登记的玻璃在 3.6 对照里“和底栏不一样”（G、G2）；浅色时白色图标对比度低（第 5.5 节，登记的玻璃变浅时也一样）。用户如果觉得样子不对，先关这个开关对比。
- **卡顿（3.10.12 已改）**：抖音的标记（关注“+”、“拍同款”）原来每 0.1 秒整张重画（`drawViewHierarchyInRect`），现在只在内容变化或有动画时重画，否则每秒一次；3.10.7 的玻璃深浅记录（每 0.5 秒截抖音窗口）只在探针打开时运行；每帧的让开原因文字只在探针打开时拼。另外：抖音会把 5 条左右视频的整列都留在窗口里（记录里 A–E），每帧要同步约 30 颗玻璃，这是结构性开销，还没动。
- 记录里还有：双击面板打开时右侧整列被插件按设计隐藏（`ALB8SetPaletteRightHidden`），所以面板开关一次右侧玻璃会隐藏再出现；“导航转场中”在滑动时出现 27 次，原因待查。

### H2 3.10.10 的回退事故（规则仍然有效，必须记住）

- **用户实测 3.10.10**：原本基本没问题的左上角展开菜单动画被破坏（展开常异常、收起延迟出现）；头像菜单和左上角一样卡、圆角每次不一样、不像系统的；双击面板打开后变成默认深色、半天切不回来；右侧又卡又乱，和视频运动速度不一致。
- **原因（从用户发来的诊断看）**：
  - 专项探针默认开着，**截了 84 张图**。每张都在主线程把主窗口和底栏窗口整个 `drawViewHierarchyInRect` 一遍，再编码 PNG、写文件，时机正好是每次翻页停稳后 0.05/0.35/0.8/1.45 秒、每个菜单动画进行到 0.3/0.9 秒（左上角菜单也算），以及菜单结束后的头像。菜单期间还每帧遍历菜单视图树，读私有属性，并让 `CADisplayLink` 一直开着驱动同步。左上角菜单的代码完全没改，是被这些主线程卡顿拖坏的。
  - 右侧玻璃“让开只调透明度”（`right.asideByAlpha`）：看不见的玻璃和它们的边一直留在底栏窗口里，和双击面板同一个窗口。这是面板卡在深色最可能的原因（未单独验证）。
  - 视频格进入窗口时同步做整列刷新：在滑动途中增加主线程开销。
- **3.10.11 已撤回**：探针默认关；打开后不截图、不驱动逐帧刷新、菜单最多每秒看 4 次、不读实体玻璃（`ALB25MaterialRealizedGlassText` 不再逐帧调用）；图片居中改成手动点一次采一次（关掉设置、静止 1.2 秒后）。“只调透明度”整个删掉，让开恢复隐藏。进入窗口的同步刷新删掉。
- **规则**：任何探针、日志在**有东西在动或菜单在显示时**都不能做重活（渲染截图、编码图片、深度遍历、反射调用），也不能为了记录去驱动逐帧刷新。截图只能在用户明确要求、静止时做一次。
- 3.10.10 那次记录里有一条有用的发现：原地按钮画玻璃 0 次，但**主窗口右侧出现过 8 个其他玻璃/模糊视图**——可能就是“旧深色玻璃”的真正来源，需要那次的 zip 才知道是哪些（用户如果没杀抖音进程，可以在装 3.10.11 前从 3.10.10 导出）。
- 那次记录还显示：新建玻璃 42 次，其中 30 次是因为原按钮被释放（抖音/插件重建了按钮，不只是复用格子）；滑动中让开的原因主要是“超出视频流”（按钮部分在屏幕外，正常）、“导航转场中”33 次（原因待查）、“原按钮看不见:祖先隐藏(AWEFeedViewCell)”10 次。

### H3 严重 bug S1–S3（用户在 3.10.9 上报告；3.11 换了画法，要在 3.11 上重新确认）

用户原话的意思：一直被一个东西干扰，旧的深色玻璃始终清理不掉，每次都被拉进来。用户明确要求过删掉一切旧色调玻璃的东西。

#### S1 翻页时玻璃先消失、再出现旧深色玻璃、再延迟变正常；数字间距错位
- **现象**（3.10.9 截图）：滑动过程中右侧玻璃全部消失，只剩抖音的数字和关注“+”；新视频快停稳时出现深色玻璃，过一会儿才变成理想的透明度；这段时间里玻璃和抖音数字的间距也是乱的（数字压在玻璃下沿）。用户要的是：玻璃一直存在、跟着视频滑动，新视频出现时就自己建好。
- **读代码确定的原因（3.10.9）**：
  1. 视频格离开屏幕时 `ALB37NoteRightButtons` 把这一格的 E2 玻璃**全部销毁**，`ALB37ReleaseMirror` 还把旧配方（`glassless NO`）还给原地按钮。抖音复用这个格子放下一条视频时，每颗玻璃都是新建的（新玻璃、新的边），先按默认（深）画，过一会儿系统才判断出该有的深浅——这就是“先深后正常”的最可能来源。
  2. `ALB37ShowMirror` 用 `barWindow.windowScene == source.window.windowScene` 判断，离屏时 `source.window` 是 nil，判断失败，原地按钮在屏幕外被装回旧玻璃。
  3. 玻璃让开时用 `hidden`，连同它自己的边（透明 UIScrollView）一起隐藏；再显示时深浅判断从头开始（推断，探针会证实或否定）。
  4. 只换图（新头像、点赞状态）也整份重建玻璃配置，每次放进一个新的玻璃对象副本（`ALB25MaterialInstallRegisteredEffect` 里 `ALBGlassCopy`）。
  5. 3.10.9 起玻璃位置跟的是**插件原地按钮**的画面位置；原地按钮如果被抖音的布局动画带着动，玻璃就和抖音的数字错开。
- **3.10.10 已改（3.10.11 现状）**：离屏不销毁（复用）；离屏时不动原地按钮的状态；E2 开着时销毁也不还旧玻璃；只换图时沿用原来的玻璃对象；玻璃位置跟抖音自己的按钮（开关 `right.followDouyin`，默认开）。~~让开只调透明度~~、~~进入窗口立刻刷新~~：3.10.11 已撤回（第 0.4 节）。
- **独立审查找到的另一个原因（已改）**：`presentationLayer` 是**上一次提交**的画面。同步在 CA 提交之前跑，而 UIKit 滑动是每帧直接写模型值，所以 3.10.5–3.10.9 用 presentation 定位时，玻璃（和角标贴图）在滑动中永远落后一帧——快速上滑时玻璃偏低，正好是“数字压在玻璃下沿”。现在只有在抖音的按钮或它的上层真有 Core Animation 动画（位置/尺寸/变换/转场）时才用 presentation，否则用模型值（`ALB312GeometryAnimating`）。探针里每帧会标“抖音在动画/原按钮在动画”。
- **还不知道**：滑动过程中玻璃为什么让开（哪个条件）。探针每帧记原因，见“滑动中玻璃让开的原因”。

#### S2 头像菜单是旧的深色；收起后头像玻璃深色、延迟恢复
- **现象**：弹出的菜单不管什么背景都是深色；菜单收起后头像玻璃是深色，过一会儿恢复。左上角菜单基本正常（大部分时候收回不变色），可作对照。
- **读代码确定的原因（3.10.8/3.10.9）**：`ALB310SourceMenuBegins` 先按背后亮度设好深浅，然后调用 `ALB25MaterialBeginButtonMenuSession` → `ApplyButtonNow` → `ALBApplyStyle`，把按钮的深浅改回视频流固定的 Dark，并装上旧配方，之后才换干净玻璃。菜单从一颗 Dark 的按钮长出来，所以永远深色。另外原地按钮还登记在视频流的右边缘上（“登记到视频流的玻璃会卡在白色或深色”）。收起后变深：头像的 E2 玻璃在整个菜单期间是 `hidden`，再显示时同 S1 第 3 条。
- **3.10.10 已改**：先冻结（新函数 `ALB25MaterialFreezeButtonForMenu`，不先套任何配方），再换干净系统玻璃，深浅**最后**设；菜单结束后原地按钮**继续冻结、继续穿着菜单玻璃**，直到 E2 玻璃真正显示出来的那一帧才恢复原来的配置和深浅（最多 1.5 秒），不会出现两者都没有、或旧玻璃的帧；菜单期间配置被别处换过（比如关注后头像图变了）就保留新的；E2 开着时原地按钮不再登记为视频流的边缘元素；让开用透明度（同 S1）。

#### S3 头像、音乐图片和玻璃不居中（边距不一致，偏左上）
- 见第 5.2 节的历史。3.10.10 没改算法，只加探针：图片那一层在玻璃里的位置、玻璃单独渲染时画到的范围和重心、整棵图层树（含阴影、变换、滤镜）、以及屏幕截图（3 倍）。拿到 PNG 后可以直接量。

#### （3.10.10–3.10.12 的专项探针已在 3.11 删除）
- 入口：设置 → 右侧布局 → 最上面“专项探针（3.10.12）”。**默认关**，先打开“记录专项探针”，正常操作后点“导出”。zip 里是 `专项探针.txt`；点过“采集一次图片居中”才会有 `img/`。
- `专项探针.txt` 开头有【怎么读】【自动结论】，接着是【此刻每颗玻璃】、每一段翻页/菜单的逐帧记录、图片居中、全部事件、图片列表。
- 判断旧深色从哪来：
  - 【自动结论】里“原地按钮画出玻璃 / 被装上旧配方 / 主窗口右侧其他玻璃”任何一个不为 0 → 旧色来自主窗口（原地按钮或抖音自己），看对应事件（含调用栈）和截图的“主窗口”一格。
  - 全为 0、但“显示后玻璃自报变了”很多 → 深色是 E2 玻璃刚显示时的重新判断；看 ▤ 玻璃结构的变化找出是哪个值在变。
- 调用栈里的 `ALB+0x…` 用 Actions 产物里的 `AwemeLiquidBar-arm64e.linkmap`（或 arm64）查函数名。
- 截图（只有手动采集图片居中时才有）三格：主窗口（抖音＋原地按钮，准确）｜底栏窗口（E2 玻璃，背后内容可能缺，日志写了不透明比例）｜叠加（近似）。

---

## 1. 编译与交付

- 把工程打包成 zip（不带 `.git`），用户放到 `/var/mobile` 后执行：

  ```sh
  cd /var/mobile && unzip -oq AwemeLiquidBar_X_project.zip -d AwemeLiquidBar_X && cd AwemeLiquidBar_X && bash push_to_github.sh
  ```

- 推送脚本会让 `main` 分支和工程目录完全一致。之后在 Actions 里下载 `AwemeLiquidBar-X-rootless`。
- 版本号要一起改的地方（全局替换旧版本号）：
  - `control`（Version、Description）
  - `push_to_github.sh`（第 2 行注释、提交信息、最后的提示）
  - `.github/workflows/build.yml`（name、artifact 名）
  - `Tweak.x`（开头注释、设置标题 `LiquidBar · X`、诊断导出头、`entry.detail`、`SESSION_START build=`、`@"build":`）
  - `ALB5Settings.h`（诊断第一行 `LiquidBar X`）
  - `README.md`（标题和命令）
- 静态检查：`python3 scripts/preflight.py`，检查 Makefile 里的源文件、Logos 配对、Swift 桥接是否一致。
- 语法检查（3.11 起覆盖全部 ObjC/Logos 源文件）：从 GitHub `xybp888/iOS-SDKs` 稀疏检出 `iPhoneOS26.1.sdk` 的头文件，用本机 clang 18：`clang -target arm64-apple-ios15.0 -isysroot SDK -iframework SDK/System/Library/SubFrameworks -fobjc-arc -fsyntax-only -x objective-c -I. 文件`。`.x/.xm` 先用自写的小转换脚本（`%hook` → 分类、每个被 hook 的方法生成同签名的函数指针给 `%orig` 用、`%c`/`%init`/`%ctor`/`%group` 原样替换，行号不变）转成 `.m` 再查。这边不能运行外部 Logos 工具。
- 新增的 `.h` 只能被 `Tweak.x` include 一次（`Makefile` 只编译 `AwemeLiquidBar_FILES` 里列出的文件）。新的 `.m`/`.x` 要加进 `AwemeLiquidBar_FILES`；`.m` 也加进 `build.yml` 的 ObjC 语法检查列表。
- 3.11 新增框架：`PhotosUI`（三段式底栏的相册选头像）。

---

## 2. 文件

| 文件 | 内容 |
|---|---|
| `ALBDock*.m/.h`、`ALBDockHooks.x` | 3.11 三段式收缩底栏（第 0.1 节）。 |
| `Tweak.x` 里 `AWEConfigureAdaptiveSystemGlassIconButton` 一带 | 右侧玻璃的画法、几何（`ALBRG*`）。 |
| `ALBNativeInteraction.h` | 原地按钮类 `ALBRightGlassButton` / `ALBArtworkGlassButton`、动作转发、头像／音乐的图（画布＋居中测量）、头像菜单的深浅（`ALBRGMenuBegins/Ends`）。 |
| `ALB25Material.m/.h` | 玻璃配方。3.11 新增 `ALB25MaterialMotionButtonConfiguration`（右侧、面板用）；其余配方现在只给消息页顶部等别处用。 |
| `ALB5RightLayout.h` | 插件对右侧整列的布局：位移、缩放写在 `view.transform` 上（缩放、补位、音乐间距）。 |
| `ALBSidePalette.h` | 双击面板。 |
| `ALB37GlassLayer.h` | 只剩顶栏：最上层／底栏窗口／`top.e2`。 |
| `ALB3Chrome.swift` | 顶栏；`ALB3Edge`（登记）。 |
| `ALB5Settings.h` | 选项页的行、诊断文本。 |

---

## 3. 现在的做法

见第 0.1 节。要点：右侧和面板的玻璃全在抖音自己的视图树里，跟着抖音的视图走，不需要任何逐帧同步；整个插件不再有给右侧、面板用的 run loop 观察者、显示链接或定时截图。

---

## 4. 设置开关（键 / 默认）

| 键 | 默认 | 作用 |
|---|---|---|
| `bottom.style` | 0 | 0 液态底栏，1 三段式收缩底栏（重启生效） |
| `dock.idle` / `dock.speed` / `dock.redDot` | 5 / 1.0 / 关 | 三段式：静置收缩秒数、动画速度、收缩红点 |
| `dock.barH` / `dock.midW` / `dock.gapA` / `dock.gapB` | 62 / 74 / 8 / 12 | 三段式：高度、胶囊宽、两个间距 |
| `dock.fuse` | 固定开（3.11.51 设置删了） | 三段式：系统底栏放进融合容器 |
| `dock.darkTint` | 从 3.11.49 的 `dock.glassAlpha` 换算（0.45×(1−值)，0 时 0.28） | 3.11.51：三段式底栏只剩“深色色调”，视频页固定 Clear＋这个黑色染色；玻璃固定深色，背后亮度只切图标黑白（阈值固定 0.85 / 0.77） |
| `right.glassSize` / `right.glassRing` / `right.glassOffsetY` | 48 / 4 / 0 | 右侧四颗直径、头像音乐外圈、四颗上下微调 |
| `glass.darkTint` | 0.28 | 旧的黑色染色，只在换算时读 |
| `menu.followGlass` | 关 | 头像菜单、左上角菜单换成按钮同款玻璃（染色下限固定 0.35） |
| `right.scale` / `right.musicGap` / `right.hide.*` | 1 / 12 / 关 | 右侧缩放、音乐间距、隐藏（未变） |
| `top.e2` | 固定开（3.11.51，用户一直开着） | 顶栏原地登记到液态底栏的滚动视图（三段式下用自己的边） |
| `glass.lifted` | 固定关（3.11.51） | 顶栏放到最上层 |
| `glass.inBarWindow` | 固定关（3.11.51） | 最上层顶栏放进液态底栏的窗口 |
| `glass.clear` / `top.glassAlpha` | 不再用（3.11.51） | 顶栏没有透明度，用系统栏按钮自己的玻璃 |
| `right.registeredGlass` / `right.glassSource` | 开 / 0 | 现在只影响消息页顶部按钮等仍用登记配方的地方 |

已删除（3.11 起不读）：`right.e2`、`panel.e2`、`glass.ownEdge`、`glass.sampleBehind`、`right.followDouyin`、`probe312.*`、`glass.bench`。
3.11.19 删除（不再读，最终做法写死）：`glass.followBar`、`glass.barFaces`、`glass.facePress`、`glass.followOwnEdge`（都等于开）、`glass.adaptive`（等于关）、`glass.lightThreshold`、`glass.lightTint`、`menu.minTint`、`glass.startLuma`、`glass.startLumaValue`。

主设置页：底栏卡片最上面是“底栏样式”和“三段式收缩底栏：收缩、尺寸与头像”；右侧按钮卡片只剩“隐藏、补位与整体缩放”、清透开关和头像菜单相关；顶栏的玻璃试验开关挪到了“顶栏玻璃（试验）”卡片。选项页：第 3 节“三段式收缩底栏”，第 4 节“右侧布局”。

---

## 5. 没解决的问题（3.10.x 时的记录；3.11 换了画法，要在 3.11 上重新确认哪些还在）

每一项都写了：现象、已经试过什么、目前的判断、哪些还没验证。3.11 里和 E2 有关的嫌疑（逐帧同步、跨窗口、登记）都已不存在。

### 5.1 右侧整列上下抖（最优先）

- **现象**：3.10.8 的录屏里，不翻视频时，整列（连同抖音自己的点赞数等）上下抖动约 ±7 点，玻璃跟着抖。翻页时，玻璃和数字、关注“+”之间的距离也会跳。
- **试过**：
  - 3.10.5：用父视图左上角的 presentation 差值跟踪。直播头像一跳就晃，已撤回。
  - 3.10.9：改成取按钮自身 presentation 中心，并逐帧记录（**未测**）。
- **目前判断**：抖的是整列本身，不是玻璃自己乱跳。是谁在动它不知道。嫌疑：
  - `ALB5RightLayout.h` 反复重算并写 transform；
  - 底栏收缩带动抖音调整布局；
  - E2 的某些写入（mask、glassless 触发 `ALB25MaterialApplyButton` 改按钮配置 → 布局 → 右侧布局重算）形成循环。
- **没拿到**：每帧是谁改了列的位置；E2 关掉时还抖不抖。

### 5.2 头像、音乐的图片在玻璃里偏左上

- **现象**：图片离玻璃左边、上边的距离比右边、下边小。3.10.8 的截图里大约差 0.5–1 点。
- **试过**：
  - 3.10.3：画在和玻璃一样大的画布正中。
  - 3.10.6：改用独立的图片视图。居中了，但图片不再跟着玻璃扭曲，已撤回。
  - 3.10.7：在按钮的图层里找到画这张图的那一层并测量，结果在 0.3 点以内，“居中”。
  - 3.10.9：玻璃和画布都取整到偶数像素，中心取整到整像素（**未测**）。
- **目前判断**：
  - 已确认一处 1/3 点的取整误差：画布 41.35 点被画成 41.67 点。
  - 但 3.10.7 测到的图层位置是居中的，画面却不居中。所以剩下的偏差可能出在玻璃本身的绘制上：`isFlexible` 的形变、透镜、高光，或者玻璃的 SDF 层和按钮边界不一致。
- **没拿到**：玻璃那一层（`CASDFLayer` / `CABackdropLayer` / `_UIMultiLayer`）在玻璃按钮里的实际 frame；把这颗玻璃单独渲染出来的像素图。

### 5.3 直播头像

- **现象**：
  - 3.10.7：直播圈跳动时玻璃上下左右扭。
  - 3.10.8：头像具体哪里不对，用户没有描述，只说“头像你改了啥”。
- **已知**：直播头像这一组里，抖音有 `AFDColorRingView`（55×55 的直播圈）、`AWEFeedLiveMarkView`（37×37）、`UIImageView`（37×37 的头像）。3.10.4–3.10.7 把这些整个画成贴图盖在玻璃上，所以看到的是抖音的头像，不是玻璃的。3.10.8 起只贴小于按钮 0.6 倍的视图。抖音的直播圈本身还在抖音窗口里，透过玻璃边能看到。
- **没拿到**：抖音的头像容器中心和插件原地按钮（`slot.menuButton`）的中心是否重合；直播动画加在哪个视图上；3.10.8 以后用户看到的头像到底什么样。

### 5.4 深浅不统一（顶栏、底栏、右侧）

- **现象**：
  - 顶栏（`top.e2` 开）背后稍亮就翻成浅色，成了一大块不透明的白。开了每块玻璃自己的边时更容易翻（3.10.8 已让顶栏不用自己的边）。
  - 底栏在同样的画面上一直是深色。
- **已有数据**（3.10.7 记录）：
  - 底栏的 `ClearGlassView` 风格几乎一直是 2（深）；
  - 顶栏在背后亮度约 0.48–0.84 时翻成 1（浅）；
  - 右侧玻璃报告的风格永远是 2/2，因为插件强制用了底栏的风格。玻璃内部的自适应状态没读到：记录器在 `ALB310Face` 里没找到任何玻璃子视图。
- **目前判断**：
  - 系统按“登记的那条边”判断深浅。共用一条边时一起翻，各自一条边时各算各的。这是从现象推断的，没有文档证实。
  - 每个元素背后的区域不同（顶部、底部、右侧），所以各自判断时结果自然不同。
- **用户要的**：统一。但规则还没定：全部跟底栏，还是各自判断。
- **没拿到**：
  - 玻璃按钮内部哪个对象、哪个值代表“现在是浅色还是深色”；
  - 系统翻转的阈值；
  - `UIScrollEdgeElementContainerInteraction` 是按边还是按元素决定的。

### 5.5 玻璃上的图标不跟着玻璃翻色

- **现象**：3.10.6 的截图里，玻璃翻成浅色，白色的图标就看不见了。
- **已知**：苹果的说法是，只有系统符号用默认颜色时才会跟着玻璃一起翻；插件给的图标是固定颜色或原色图片，不会翻。
- 没处理。

### 5.6 顶部分组栏的选中胶囊偶尔不见

- 原因不明。还没确认关掉 `top.e2` 后是否仍会发生。

### 5.7 菜单在底栏窗口里弹不出来

- **现象**：3.10.1 时，顶栏搬进底栏窗口后，左上角菜单和头像玻璃自己的菜单都弹不出来。
- **现在的绕法**：菜单从抖音窗口里的原地按钮弹。
- 原因没查。
- 同一版顶栏搬进底栏窗口后两侧按钮贴边、边距失效，也没查。

### 5.8 头像菜单

- 3.10.7 时：菜单是深色，收起后头像深灰一阵才恢复。原因是原地按钮弹菜单时用的是旧的登记配方，那套配方固定是深色。
- 3.10.8 改成弹菜单时换干净的系统玻璃。3.10.9 实测仍然深色——见第 0.5 节 S2（深浅被 `ALBApplyStyle` 改回 Dark），3.10.10 已改顺序。

### 5.9 角标贴图

- 每秒 10 次的截图，会动的角标看起来一顿一顿。
- 0.6 倍面积的挑选规则只在几种视频上看过，可能挑错或漏挑。

---

## 6. 还没拿到的数据

3.10.10–3.10.12 的专项探针和 3.10.7 的玻璃深浅记录都在 3.11 删除了（用户要求删掉右侧的旧做法；探针本身也拖慢过画面）。以后需要数据时重新加，遵守历史 H2 的规则：有东西在动或菜单在显示时不做重活，默认关。可能还需要的：
1. 右侧整列逐帧位置（5.1 的抖动是谁在动）。
2. 头像、音乐的图在玻璃里实际画到的位置（3.11 自动测量的结果，可在诊断里输出 `kALBArtworkShiftKey` 的累计校正量）。
3. 三段式底栏和抖音底栏的显示／隐藏时机（朋友页、详情页）。

---

## 7. 已经确认的结论（不用再测）

- **对照玻璃 E** 和底栏一模一样。E 的做法：放在底栏窗口最上层、登记到底栏页的透明 `UICollectionView` 左边缘、深浅取 tab bar 的风格。
- **E2** = E 加上自己接收触摸：外观和底栏一样，有原生按压（3.9.2 验证）。
- **触摸只能真接**：把触摸转交给一个不是命中视图的玻璃，不会产生原生按压（3.9.3 的 E3 验证）。
- **结构不能代替登记**：放在底栏那一页里、但不登记的玻璃（G、G2），和底栏不一样。
- **登记到视频流的玻璃**会卡在白色或深色；不登记的玻璃不会自己切换深浅。
- **老的登记配方**（`ALBInstallRegisteredEffect`）不管背后多亮都是深色。3.10.x 的深色闪烁、深色菜单都来自它。
- **系统菜单弹出时**会铺一层全屏透明层。3.10.x 曾把它当成遮挡，导致右侧玻璃全部让开、露出旧的深色玻璃。3.10.7 已修。
- **系统 portal 不能跨窗口**。

---

## 8. 已知限制（目前是有意这样）

- 长按右侧按钮只算单击；头像菜单在点按时弹出（`showsMenuAsPrimaryAction`）。
- 双击面板开着时，点液态底栏会直接切页（面板跟着关闭；3.10.2–3.10.12 面板在底栏窗口里时是先收起面板）。
- 三段式底栏只在重启后切换。

---

## 9. 待清理（等稳定后一起做）

- 参考工程的名字：3.11 已全部换掉（代码、README），`grep -rnE 'S[J]{2}|s[j]{2}' .` 为 0。git 提交历史里可能还有，zip 不带历史。
- 老开关和老代码：`glass.lifted` / `glass.inBarWindow` / `glass.clear` / `top.e2` 现在只管顶栏；`right.registeredGlass` / `right.glassSource` 和 `ALB25Material.m` 里的登记配方、`MessageTopGlassV3.xm` 还在用。
- 3.11 后没人用的小函数：`ALB3EnclosingFeedScrollView`、`ALBObserveClearContrastRows`、`AWERefreshAutomaticContrastForButton`、`AWERegisterAdaptiveGlassButton`、`AWEApplyResolvedGlassStyleToAdaptiveButton` 一族（注册表为空）。
- 设置页开关名太乱，用户已经分不清每个开关是干什么的。等定型后重新命名、合并。

## 10. 用户偏好和约束

- 回复用简体中文，要详细；技术名词尽量用中文解释。
- 要编好的 deb：通过上面的推送脚本走 Actions 编译。**不要自己推送**，只给 zip 和那一行命令。
- 用户的额度有限，每一版都要有把握。不确定的地方先加探针拿数据，不要对着截图和录屏去猜着改。
- 顶栏和底栏不能回退。液态底栏目前完美，不要动；三段式底栏除改名、偏好、垂直位置外也不要改它的逻辑。
- 回复和代码里不要出现参考工程的类名前缀（见第 9 节的查找命令）。
- 参考工程：用户自己的实验工程 LiquidGlassLab（0.4.0）。在实验工程里，右侧和底栏完全一致，顶栏对白色比较敏感。
