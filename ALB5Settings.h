/* New options use their own namespace; original authorization and preferences are unchanged. */
@interface ALB5OptionsController : UITableViewController <UIColorPickerViewControllerDelegate>
@property(nonatomic,copy) NSString *colorKey;
@property(nonatomic,copy) NSIndexSet *visibleSections;
- (instancetype)initWithSections:(NSIndexSet *)sections title:(NSString *)title;
- (NSArray *)optionSections;
@end

@interface ALBPaletteActionsController : UITableViewController
@property(nonatomic, strong) NSMutableArray<NSNumber *> *orderedActions;
@end

@implementation ALBPaletteActionsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"功能与排序";
    self.orderedActions = [ALB28PaletteActionOrder() mutableCopy];
    self.tableView.backgroundColor = UIColor.clearColor;
    self.tableView.allowsSelectionDuringEditing = NO;
    self.tableView.editing = YES;
    ALBInstallSettingsGlass(self);
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.orderedActions.count;
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    NSInteger action = self.orderedActions[path.row].integerValue;
    NSArray *titles = @[@"作者主页", @"保存当前图片／视频", @"图集视频／复制链接", @"点赞作品", @"保存音频", @"评论", @"复制文案", @"音量", @"亮度", @"保存所有图片", @"收藏作品", @"转发", @"关注作者", @"一键清屏", @"保存当前实况", @"保存所有实况", @"保存所有类型图片"];
    NSArray *symbols = @[@"person.crop.circle", @"square.and.arrow.down", @"link", @"heart", @"music.note", @"bubble.right", @"doc.on.doc", @"speaker.wave.2", @"sun.max", @"photo.on.rectangle.angled", @"star", @"arrowshape.turn.up.right", @"person.badge.plus", @"eye.slash", @"livephoto", @"livephoto", @"square.stack.3d.down.right"];
    UITableViewCell *cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:nil];
    cell.textLabel.text = titles[action];
    cell.textLabel.font = [UIFont systemFontOfSize:14];
    cell.imageView.image = [UIImage systemImageNamed:symbols[action]];
    cell.imageView.tintColor = UIColor.labelColor;
    cell.showsReorderControl = YES;
    ALBSettingsInstallCellFrost(cell);
    UISwitch *toggle = [UISwitch new];
    toggle.tag = action;
    toggle.on = ALB5Bool([NSString stringWithFormat:@"panel.action.%ld", (long)action], YES);
    toggle.accessibilityLabel = titles[action];
    [toggle addTarget:self action:@selector(actionChanged:) forControlEvents:UIControlEventValueChanged];
    cell.editingAccessoryView = toggle;
    return cell;
}
- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)path {
    return UITableViewCellEditingStyleNone;
}
- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)path { return NO; }
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)path { return YES; }
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)source toIndexPath:(NSIndexPath *)destination {
    NSNumber *action = self.orderedActions[source.row];
    [self.orderedActions removeObjectAtIndex:source.row];
    [self.orderedActions insertObject:action atIndex:destination.row];
    [NSUserDefaults.standardUserDefaults setObject:self.orderedActions forKey:ALB5Key(@"panel.actionOrder")];
    [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
}
- (void)actionChanged:(UISwitch *)sender {
    NSString *key = [NSString stringWithFormat:@"panel.action.%ld", (long)sender.tag];
    [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:ALB5Key(key)];
    [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
}
@end
/* 3.11.45：三段式底栏的项目——五项（首页 消息 拍摄 我 朋友）的顺序、隐藏、单独键是哪一项（ALBDockCommon.h 读同样的键） */
static NSArray<NSNumber *> *ALBDockEditorOrder(void) {
    id saved=[NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"dock.order")];
    NSMutableArray<NSNumber *> *order=[NSMutableArray array];
    for(id v in [saved isKindOfClass:NSArray.class]?saved:@[]) {
        if(![v isKindOfClass:NSNumber.class])continue;
        NSInteger t=[v integerValue];
        if(t>=0 && t<5 && ![order containsObject:@(t)])[order addObject:@(t)];
    }
    for(NSNumber *t in @[@0,@2,@1,@3,@4])if(![order containsObject:t])[order addObject:t];
    return order;
}
@interface ALBDockItemsController : UITableViewController
@property(nonatomic,strong) NSMutableArray<NSNumber *> *order;
@property(nonatomic,strong) NSMutableSet<NSNumber *> *hiddenTypes;
@property(nonatomic,assign) NSInteger singleType;
@end
@implementation ALBDockItemsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
+ (NSArray<NSString *> *)names { return @[@"首页",@"消息",@"拍摄（发布）",@"我",@"朋友"]; }
+ (NSArray<NSString *> *)symbols { return @[@"house",@"message",@"plus.circle",@"person.crop.circle",@"person.2"]; }
- (void)viewDidLoad {
    [super viewDidLoad];
    self.title=@"三段式底栏项目";
    self.order=[ALBDockEditorOrder() mutableCopy];
    self.hiddenTypes=[NSMutableSet set];
    id hidden=[NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"dock.hidden")];
    for(id v in [hidden isKindOfClass:NSArray.class]?hidden:@[])if([v isKindOfClass:NSNumber.class])[self.hiddenTypes addObject:@([v integerValue])];
    id single=[NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"dock.singleType")];
    self.singleType=single ? MIN(4,MAX(0,[single integerValue])) : 4;
    self.tableView.backgroundColor=UIColor.clearColor;
    self.tableView.allowsSelectionDuringEditing=YES;
    self.tableView.editing=YES;
    self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"恢复默认" style:UIBarButtonItemStylePlain target:self action:@selector(resetItems)];
    ALBInstallSettingsGlass(self);
}
- (void)save {
    [NSUserDefaults.standardUserDefaults setObject:self.order forKey:ALB5Key(@"dock.order")];
    NSArray *hidden=[self.hiddenTypes.allObjects sortedArrayUsingSelector:@selector(compare:)];
    [NSUserDefaults.standardUserDefaults setObject:hidden forKey:ALB5Key(@"dock.hidden")];
    [NSUserDefaults.standardUserDefaults setInteger:self.singleType forKey:ALB5Key(@"dock.singleType")];
    [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
}
- (void)resetItems {
    for(NSString *k in @[@"dock.order",@"dock.hidden",@"dock.singleType"])[NSUserDefaults.standardUserDefaults removeObjectForKey:ALB5Key(k)];
    self.order=[@[@0,@2,@1,@3,@4] mutableCopy]; [self.hiddenTypes removeAllObjects]; self.singleType=4;
    [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
    [self.tableView reloadData];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return 2; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return section==0 ? 5 : (NSInteger)self.order.count; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return section==0 ? @"单独键" : @"主栏"; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleValue1 reuseIdentifier:nil];
    cell.textLabel.font=[UIFont systemFontOfSize:14];
    ALBSettingsInstallCellFrost(cell);
    NSInteger type=path.section==0 ? path.row : self.order[(NSUInteger)path.row].integerValue;
    cell.textLabel.text=[ALBDockItemsController names][(NSUInteger)type];
    cell.imageView.image=[UIImage systemImageNamed:[ALBDockItemsController symbols][(NSUInteger)type]];
    cell.imageView.tintColor=UIColor.labelColor;
    if(path.section==0) {
        cell.editingAccessoryType=type==self.singleType ? UITableViewCellAccessoryCheckmark : UITableViewCellAccessoryNone;
        cell.accessoryType=cell.editingAccessoryType;
        return cell;
    }
    cell.showsReorderControl=YES;
    cell.selectionStyle=UITableViewCellSelectionStyleNone;
    if(type==self.singleType) {
        cell.detailTextLabel.text=@"单独键";
        cell.editingAccessoryView=nil;
    } else {
        UISwitch *toggle=[UISwitch new];
        toggle.tag=type;
        toggle.on=![self.hiddenTypes containsObject:@(type)];
        toggle.accessibilityLabel=cell.textLabel.text;
        [toggle addTarget:self action:@selector(toggleChanged:) forControlEvents:UIControlEventValueChanged];
        cell.editingAccessoryView=toggle;
    }
    return cell;
}
- (void)toggleChanged:(UISwitch *)sender {
    NSNumber *type=@(sender.tag);
    if(sender.on)[self.hiddenTypes removeObject:type];
    else {
        NSUInteger shown=0;
        for(NSNumber *t in self.order)if(t.integerValue!=self.singleType && ![self.hiddenTypes containsObject:t] && ![t isEqual:type])shown++;
        if(shown==0) { [sender setOn:YES animated:YES]; AWEShowDownloadToast(@"主栏至少保留一项"); return; }
        [self.hiddenTypes addObject:type];
    }
    [self save];
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    [tableView deselectRowAtIndexPath:path animated:YES];
    if(path.section!=0 || path.row==self.singleType)return;
    self.singleType=path.row;
    [self.hiddenTypes removeObject:@(path.row)];
    [self save];
    [tableView reloadData];
}
- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)path { return UITableViewCellEditingStyleNone; }
- (BOOL)tableView:(UITableView *)tableView shouldIndentWhileEditingRowAtIndexPath:(NSIndexPath *)path { return NO; }
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)path { return path.section==1; }
- (NSIndexPath *)tableView:(UITableView *)tableView targetIndexPathForMoveFromRowAtIndexPath:(NSIndexPath *)source toProposedIndexPath:(NSIndexPath *)proposed {
    if(proposed.section==1)return proposed;
    return [NSIndexPath indexPathForRow:0 inSection:1];
}
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)source toIndexPath:(NSIndexPath *)destination {
    NSNumber *type=self.order[(NSUInteger)source.row];
    [self.order removeObjectAtIndex:(NSUInteger)source.row];
    [self.order insertObject:type atIndex:(NSUInteger)destination.row];
    [self save];
}
@end

static NSArray *ALB5OptionSections(void) { return @[
@{@"title":@"底栏收缩",@"rows":@[@{@"key":@"bottom.style",@"title":@"底栏样式",@"kind":@"choice",@"default":@[@"液态底栏",@"三段式底栏"]},@{@"key":@"collapse.expandOnDown",@"title":@"下滑时展开",@"kind":@"bool",@"default":@YES},@{@"key":@"bottom.sixTabs",@"title":@"解除 5 个按钮上限",@"kind":@"bool",@"default":@YES},@{@"key":@"bottom.homeReselectColumns",@"title":@"再点首页切换单双列",@"kind":@"bool",@"default":@NO},@{@"key":@"bottom.grouponFill",@"title":@"团购信息流铺满",@"kind":@"bool",@"default":@YES}]},
@{@"title":@"顶栏",@"rows":@[@{@"key":@"top.groupScale",@"title":@"分组比例",@"kind":@"range",@"default":@(1),@"low":@(0.5),@"high":@(1.2)},@{@"key":@"top.slotWidth",@"title":@"分组单项宽度",@"kind":@"range",@"default":@(76),@"low":@(44),@"high":@(120)},@{@"key":@"top.sideScale",@"title":@"左右按钮比例",@"kind":@"range",@"default":@(1),@"low":@(0.6),@"high":@(1.3)},@{@"key":@"top.sideMargin",@"title":@"左右按钮边距",@"kind":@"range",@"default":@(16),@"low":@(0),@"high":@(40)},@{@"key":@"top.leftMenu",@"title":@"左上角展开菜单",@"kind":@"bool",@"default":@YES},@{@"key":@"top.keepSingle",@"title":@"单个分组显示选中胶囊",@"kind":@"bool",@"default":@YES},@{@"key":@"top.nearGroup",@"title":@"两侧按钮靠近分组",@"kind":@"bool",@"default":@NO},@{@"key":@"top.leftGap",@"title":@"左侧间距",@"kind":@"range",@"default":@(14),@"low":@(0),@"high":@(80)},@{@"key":@"top.rightGap",@"title":@"右侧间距",@"kind":@"range",@"default":@(14),@"low":@(0),@"high":@(80)}]},
@{@"title":@"双击面板",@"rows":@[@{@"key":@"panel.customTextColor",@"title":@"固定文字颜色",@"kind":@"bool",@"default":@NO},@{@"key":@"panel.textColor",@"title":@"文字颜色",@"kind":@"color",@"default":@(0)},@{@"key":@"panel.inset",@"title":@"填充边距",@"kind":@"range",@"default":@(3),@"low":@(1),@"high":@(6)},@{@"key":@"panel.volumeColor",@"title":@"音量颜色",@"kind":@"color",@"default":@(0)},@{@"key":@"panel.brightnessColor",@"title":@"亮度颜色",@"kind":@"color",@"default":@(0)},@{@"key":@"panel.fillAlpha",@"title":@"填充透明度",@"kind":@"range",@"default":@(0.26),@"low":@(0.05),@"high":@(0.65)},@{@"key":@"panel.iconFill",@"title":@"图标浅填充",@"kind":@"bool",@"default":@NO},@{@"key":@"panel.breathe",@"title":@"图标呼吸",@"kind":@"choice",@"default":@[@"关闭",@"中心呼吸",@"上浮呼吸"]},@{@"key":@"panel.breathePeriod",@"title":@"呼吸周期（秒）",@"kind":@"range",@"default":@(2.6),@"low":@(1),@"high":@(8)}]},
@{@"title":@"三段式底栏",@"rows":@[@{@"key":@"bottom.style",@"title":@"底栏样式",@"kind":@"choice",@"default":@[@"液态底栏",@"三段式底栏"]},@{@"key":@"dock.idle",@"title":@"自动收缩（秒）",@"kind":@"range",@"default":@(5),@"low":@(2),@"high":@(30)},@{@"key":@"dock.speed",@"title":@"动画速度",@"kind":@"range",@"default":@(1),@"low":@(0.5),@"high":@(2)},@{@"key":@"dock.items",@"title":@"项目",@"kind":@"action"},@{@"key":@"dock.mid",@"title":@"中间胶囊",@"kind":@"bool",@"default":@YES},@{@"key":@"dock.arrow",@"title":@"↑ 键",@"kind":@"choice",@"default":@[@"快捷跳转面板",@"消息通知",@"特别关注",@"我的订单",@"观看历史"]},@{@"key":@"dock.jumpRecreate",@"title":@"重启后也能跳（新建页面，试验）",@"kind":@"bool",@"default":@NO}]},
@{@"title":@"右侧按钮",@"rows":@[@{@"key":@"right.glassSize",@"title":@"玻璃直径",@"kind":@"range",@"default":@(48),@"low":@(36),@"high":@(64)},@{@"key":@"right.glassRing",@"title":@"头像／音乐外圈",@"kind":@"range",@"default":@(4),@"low":@(0),@"high":@(12)},@{@"key":@"right.glassOffsetY",@"title":@"玻璃上下微调",@"kind":@"range",@"default":@(0),@"low":@(-8),@"high":@(8)},@{@"key":@"right.scale",@"title":@"整体缩放",@"kind":@"range",@"default":@(1),@"low":@(0.5),@"high":@(1.25)},@{@"key":@"right.musicGap",@"title":@"音乐上方间距",@"kind":@"range",@"default":@(12),@"low":@(0),@"high":@(80)},@{@"key":@"right.hide.avatar",@"title":@"隐藏头像",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.like",@"title":@"隐藏点赞",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.comment",@"title":@"隐藏评论",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.favorite",@"title":@"隐藏收藏",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.share",@"title":@"隐藏转发",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.music",@"title":@"隐藏音乐",@"kind":@"bool",@"default":@NO}]},
@{@"title":@"背景探针",@"rows":@[@{@"key":@"probe.enabled",@"title":@"显示探针按钮",@"kind":@"bool",@"default":@NO},@{@"key":@"probe.mode",@"title":@"探针用途",@"kind":@"choice",@"default":@[@"检查背景",@"采集"]}]},
@{@"title":@"玻璃",@"rows":@[]},   // 3.11.51：这一节的开关删了（新玻璃直接到位固定开，只管右侧按钮和双击面板）；留着占位，后面各节的序号不变
@{@"title":@"点赞特效",@"rows":@[@{@"key":@"fx.enabled",@"title":@"点赞特效",@"kind":@"bool",@"default":@YES},@{@"key":@"fx.style",@"title":@"样式",@"kind":@"choice",@"default":@[@"心动",@"樱花",@"星光",@"流星",@"礼花",@"泡泡",@"烟花",@"智能光",@"雪花",@"萤火",@"表情雨",@"玫瑰",@"随机"]},@{@"key":@"fx.duration",@"title":@"时长",@"kind":@"range",@"default":@(1),@"low":@(0.5),@"high":@(2.5)},@{@"key":@"fx.intensity",@"title":@"密度",@"kind":@"range",@"default":@(1),@"low":@(0.4),@"high":@(2)},@{@"key":@"fx.spray",@"title":@"喷射位置",@"kind":@"choice",@"default":@[@"随机多点",@"两侧对称",@"点赞处"]},@{@"key":@"fx.trigButton",@"title":@"点赞按钮触发",@"kind":@"bool",@"default":@YES},@{@"key":@"fx.trigDouble",@"title":@"双击触发",@"kind":@"bool",@"default":@YES},@{@"key":@"fx.edgeGlow",@"title":@"边缘光晕",@"kind":@"bool",@"default":@YES},@{@"key":@"fx.haptic",@"title":@"震动",@"kind":@"bool",@"default":@YES},@{@"key":@"fx.ball",@"title":@"悬浮球",@"kind":@"bool",@"default":@NO},@{@"key":@"fx.preview",@"title":@"预览",@"kind":@"action"}]},
@{@"title":@"长按面板",@"rows":@[@{@"key":@"sheet.glass",@"title":@"玻璃背景",@"kind":@"bool",@"default":@YES},@{@"key":@"sheet.radius",@"title":@"面板圆角",@"kind":@"range",@"default":@(20),@"low":@(8),@"high":@(44)},@{@"key":@"sheet.moduleRadiusOn",@"title":@"改模块圆角",@"kind":@"bool",@"default":@NO},@{@"key":@"sheet.moduleRadius",@"title":@"模块圆角",@"kind":@"range",@"default":@(12),@"low":@(4),@"high":@(24)},@{@"key":@"sheet.allSheets",@"title":@"其他底部面板也用（试验）",@"kind":@"bool",@"default":@NO}]},
@{@"title":@"分享面板",@"rows":@[@{@"key":@"bind.sheets",@"title":@"分享面板玻璃",@"kind":@"bool",@"default":@YES},@{@"key":@"bind.note",@"title":@"图文页评论框玻璃",@"kind":@"bool",@"default":@YES}]},
@{@"title":@"三段式底栏外观",@"rows":@[@{@"key":@"dock.darkTint",@"title":@"深色色调",@"kind":@"range",@"default":@(0.28),@"low":@(0),@"high":@(0.6)},@{@"key":@"dock.accent",@"title":@"选中用强调色",@"kind":@"bool",@"default":@YES},@{@"key":@"dock.redDot",@"title":@"未读红点",@"kind":@"bool",@"default":@NO}]},
@{@"title":@"三段式底栏尺寸",@"rows":@[@{@"key":@"dock.barH",@"title":@"底栏高度",@"kind":@"range",@"default":@(62),@"low":@(40),@"high":@(62)},@{@"key":@"dock.tabWidth",@"title":@"项目宽度（0 自动）",@"kind":@"range",@"default":@(0),@"low":@(0),@"high":@(110)},@{@"key":@"dock.midW",@"title":@"胶囊宽度",@"kind":@"range",@"default":@(74),@"low":@(44),@"high":@(110)},@{@"key":@"dock.gapA",@"title":@"主栏与胶囊间距",@"kind":@"range",@"default":@(8),@"low":@(0),@"high":@(30)},@{@"key":@"dock.gapB",@"title":@"胶囊与单独键间距",@"kind":@"range",@"default":@(12),@"low":@(0),@"high":@(30)},@{@"key":@"dock.resetGeometry",@"title":@"恢复默认尺寸",@"kind":@"action"}]},
@{@"title":@"三段式底栏头像",@"rows":@[@{@"key":@"dock.singleAvatar",@"title":@"单独键显示头像",@"kind":@"bool",@"default":@NO},@{@"key":@"dock.avatarInset",@"title":@"头像内边距",@"kind":@"range",@"default":@(4),@"low":@(0),@"high":@(14)},@{@"key":@"dock.avatarPick",@"title":@"从相册选头像",@"kind":@"action"},@{@"key":@"dock.avatarReload",@"title":@"重新读取头像",@"kind":@"action"}]},
@{@"title":@"悬浮面板",@"rows":@[@{@"key":@"float.columns",@"title":@"每行卡片",@"kind":@"choice",@"default":@[@"3 个",@"2 个",@"4 个"]},@{@"key":@"float.gap",@"title":@"离底栏距离",@"kind":@"range",@"default":@(14),@"low":@(0),@"high":@(40)},@{@"key":@"float.tint",@"title":@"深色色调",@"kind":@"range",@"default":@(0.22),@"low":@(0),@"high":@(0.8)},@{@"key":@"float.clarity",@"title":@"背景透明度",@"kind":@"range",@"default":@(0),@"low":@(0),@"high":@(0.9)}]}]; }

/* 3.11: the option pages' action rows. */
extern NSString *ALBDockDiagnostics(void);
/* 3.11.33: the like effect (ALBAmbience.xm). */
extern void ALBFXPreview(void);
extern NSString *ALBFXReport(void);
/* 3.11.34: the long-press panel's glass (ALBSheetGlass.xm). */
extern NSString *ALBSheetGlassReport(void);
/* 3.11.44: the share panel, the comment share sheet and the note page's comment box (ALBMoreGlass.h, in Tweak.x). */
extern NSString *ALBBindGlassReport(void);
/* 3.11.45: quick jumps, media saving and the special diagnostics (Tweak.x and its headers). */
extern NSString *ALBQuickJumpReport(void);
extern NSString *ALB45DiagnosticsReport(void);
static void ALB5OptionAction(NSString *key, UIViewController *host, UIView *source) {
    (void)source;
    if ([key isEqualToString:@"fx.preview"]) { ALBFXPreview(); return; }
    if ([key isEqualToString:@"dock.resetGeometry"]) {
        for (NSString *name in @[@"dock.barH", @"dock.midW", @"dock.gapA", @"dock.gapB"])
            [NSUserDefaults.standardUserDefaults removeObjectForKey:ALB5Key(name)];
        [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
        if ([host isKindOfClass:UITableViewController.class]) [((UITableViewController *)host).tableView reloadData];
        AWEShowDownloadToast(@"已恢复默认尺寸");
    } else if ([key isEqualToString:@"dock.items"]) {
        UIViewController *editor = [ALBDockItemsController new];
        if (host.navigationController) [host.navigationController pushViewController:editor animated:YES];
        else [host presentViewController:[[UINavigationController alloc] initWithRootViewController:editor] animated:YES completion:nil];
    } else if ([key isEqualToString:@"dock.avatarPick"]) {
        [ALBDockAvatar presentPickerFromViewController:host];
    } else if ([key isEqualToString:@"dock.avatarReload"]) {
        [ALBDockAvatar clear];
        AWEShowDownloadToast(@"正在重新读取头像");
    }
}

static NSString *ALB5AdaptationText(void) {
    NSMutableString *text=[NSMutableString stringWithFormat:@"LiquidBar 3.11.53\niOS %@\n%@\n%@\n%@",UIDevice.currentDevice.systemVersion,ALB6CollapseDiagnostics(),ALB6BottomGeometryStatus(gAWEAlbumMorphController.tabBar),[gAWEAlbumMorphController nativeCollapseDiagnostics] ?: @"等待原生滚动源"];
    if (gAWEAlbumMorphController.viewIfLoaded.window) {
        [text appendFormat:@"\n当前原生状态=%d\n收缩入口清单=%@",
            ALB15NativeMinimizedState(gAWEAlbumMorphController),
            ALB22DescribeMinimizeCapabilities(ALB22OwnedMinimizeTargets(gAWEAlbumMorphController))];
    }
    if (gAWEAlbumMorphController.viewIfLoaded)
        [text appendFormat:@"\n%@", [gAWEAlbumMorphController nativeMoreRouteDiagnostics] ?: @""];
    ALB3TopChromeView *topChrome = ALB3TopChromeForContainer(gAWEHomeFeedTopContainer);
    if (topChrome.window) [text appendFormat:@"\n顶栏：%@", topChrome.diagnosticsText];
    // 3.6: everything that decides how each glass renders, side by side.
    [text appendFormat:@"\n%@\n%@", ALB37Status(), ALB25MaterialRegisteredGlassStatus()];
    [text appendFormat:@"\n%@", ALBDockDiagnostics()];
    [text appendFormat:@"\n【点赞特效】%@", ALBFXReport()];
    [text appendFormat:@"\n【长按面板】%@", ALBSheetGlassReport()];
    [text appendFormat:@"\n【更多玻璃】%@", ALBBindGlassReport()];
    [text appendFormat:@"\n【快捷跳转】%@", ALBQuickJumpReport()];
    [text appendFormat:@"\n%@", ALB45DiagnosticsReport()];
    [text appendFormat:@"\n右侧与双击面板玻璃：%@ 直径=%.0f 外圈=%.0f 上下=%.1f",
        ALB25MaterialFollowBar() ? @"顶栏同款（底栏的玻璃、每颗自己的边，深浅由系统切）"
        : @"三段式底栏同款（不登记、透明＋深色色调）", ALB5Number(@"right.glassSize", 48, 36, 64),
        ALB5Number(@"right.glassRing", 4, 0, 12), ALB5Number(@"right.glassOffsetY", 0, -8, 8)];
    // 3.11.1
    UIView *rightStack = ALBFindVisibleActionStack(AWEActiveMainWindow().rootViewController.view);
    [text appendFormat:@"\n右侧玻璃与数字间距（各自居中时）：%@", ALBRGGapReport(rightStack)];
    [text appendFormat:@"\n【玻璃深浅】%@", ALBToneReport()];
    [text appendFormat:@"\n【新玻璃直接到位】%@", ALBGlassSeedReport()];
    [text appendFormat:@"\n【玻璃恢复记录】%@", ALBGlassLifecycleReport()];
    [text appendFormat:@"\n【顶栏】%@", ALB37Status()];
    [text appendFormat:@"\n【%@】", ALBGrouponReport()];
    UITabBar *bottomBar = gAWEAlbumMorphController.tabBar;
    UIView *platter = ALB25MaterialBottomPlatter(bottomBar);
    [text appendFormat:@"\n【底栏胶囊】%@", platter ? ALB25MaterialGlassDeepReport(platter) : @"不在屏幕上"];
    UIView *topGlass = topChrome.window ? ALB25MaterialGlassOwner(topChrome, YES) : nil;
    [text appendFormat:@"\n【顶栏按钮】%@ 登记=%@", topGlass ? ALB25MaterialGlassDeepReport(topGlass) : @"不在屏幕上",
        topChrome.window ? ALB35RegistrationText(topChrome) : @"-"];
    UIButton *rightGlass = ALB35VisibleRightGlass();
    [text appendFormat:@"\n【右侧】%@ 登记=%@", rightGlass ? ALB25MaterialGlassDeepReport(rightGlass) : @"不在屏幕上（回到首页视频再点诊断）",
        rightGlass ? ALB35RegistrationText(rightGlass) : @"-"];
    [text appendFormat:@"\n玻璃接口：%@", ALB25MaterialGlassRuntimeNames()];
    return text;
}
/* Waits until the plug-in's settings are closed, then 3 s, then copies. */
static void ALB38ScheduleDiagnostics(void) {
    static NSUInteger generation;
    NSUInteger mine = ++generation;
    __block NSUInteger polls = 0;
    __block void (^poll)(void);
    __block __weak void (^weakPoll)(void);
    poll = ^{
        if (mine != generation || ++polls > 240) return;
        UIWindow *window = AWEActiveMainWindow();
        if (!window || ALB36SettingsShowing(window)) {
            void (^again)(void) = weakPoll;
            if (again) dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), again);
            return;
        }
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
            if (mine != generation) return;
            UIPasteboard.generalPasteboard.string = ALB5AdaptationText();
            AWEShowDownloadToast(@"诊断已复制，直接粘贴发给我");
        });
    };
    weakPoll = poll;
    static void (^keep)(void);
    keep = poll;
    poll();
}
@implementation ALB5OptionsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (instancetype)initWithSections:(NSIndexSet *)sections title:(NSString *)title { if ((self=[self init])) { self.visibleSections=sections; self.title=title; } return self; }
- (NSArray *)optionSections { NSArray *sections=ALB5OptionSections(); return self.visibleSections ? [sections objectsAtIndexes:self.visibleSections] : sections; }
- (void)viewDidLoad { [super viewDidLoad]; if(!self.title)self.title=@"选项"; self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"诊断" style:UIBarButtonItemStylePlain target:self action:@selector(copyAdaptationStatus)]; self.tableView.backgroundColor=UIColor.clearColor; ALBInstallSettingsGlass(self); }
- (void)copyAdaptationStatus {
    // 3.8.2: the bench and the lifted glass only draw (and only have their
    // sampling layers) with the settings closed, so they can be read later.
    UIAlertController *sheet=[UIAlertController alertControllerWithTitle:@"诊断" message:@"需要排查玻璃恢复问题时，手动开始记录，关闭设置并复现，随后立即回来“结束记录并复制”。90 秒为最长时限，不必等待到期。记录只保存在内存，不改变玻璃样式。" preferredStyle:UIAlertControllerStyleAlert];
    [sheet addAction:[UIAlertAction actionWithTitle:@"开始记录玻璃恢复（90 秒）" style:UIAlertActionStyleDefault handler:^(__unused UIAlertAction *action) {
        ALBGlassLifecycleStart();
        AWEShowDownloadToast(@"已开始记录：复现后尽快结束复制，90秒为最长时限");
    }]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"结束记录并复制" style:UIAlertActionStyleDefault handler:^(__unused UIAlertAction *action) {
        ALBGlassLifecycleStop();
        UIPasteboard.generalPasteboard.string=ALB5AdaptationText();
        AWEShowDownloadToast(@"诊断已复制，直接粘贴发给我");
    }]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"关闭设置后采集" style:UIAlertActionStyleDefault handler:^(__unused UIAlertAction *action) {
        ALB38ScheduleDiagnostics();
        AWEShowDownloadToast(@"关掉设置、回到首页视频，3 秒后自动复制诊断");
    }]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"立即复制" style:UIAlertActionStyleDefault handler:^(__unused UIAlertAction *action) {
        NSString *text=ALB5AdaptationText();
        UIPasteboard.generalPasteboard.string=text;
        UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"适配状态已复制" message:text preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
        [self presentViewController:alert animated:YES completion:nil];
    }]];
    [sheet addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];
    [self presentViewController:sheet animated:YES completion:nil];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return [self optionSections].count; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return [[self optionSections][section][@"rows"] count]; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return [self optionSections][section][@"title"]; }
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    NSDictionary *row=[self optionSections][path.section][@"rows"][path.row];
    UITableViewCell *cell=[[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:nil];
    cell.textLabel.text=row[@"title"];cell.textLabel.font=[UIFont systemFontOfSize:13];
    ALBSettingsInstallCellFrost(cell);cell.selectionStyle=UITableViewCellSelectionStyleNone;
    NSString *key=row[@"key"],*kind=row[@"kind"];
    if([kind isEqual:@"bool"]) {
        UISwitch *v=[UISwitch new];v.on=ALB5Bool(key,[row[@"default"] boolValue]);v.accessibilityIdentifier=key;
        [v addTarget:self action:@selector(toggle:) forControlEvents:UIControlEventValueChanged];cell.accessoryView=v;
    } else if([kind isEqual:@"range"]) {
        UISlider *v=[[UISlider alloc] initWithFrame:CGRectMake(0,0,112,36)];v.minimumValue=[row[@"low"] floatValue];v.maximumValue=[row[@"high"] floatValue];
        v.value=ALB5Number(key,[row[@"default"] doubleValue],v.minimumValue,v.maximumValue);v.accessibilityIdentifier=key;
        cell.detailTextLabel.text=[NSString stringWithFormat:@"%.2f",v.value];
        [v addTarget:self action:@selector(slide:) forControlEvents:UIControlEventValueChanged];
        [v addTarget:self action:@selector(commit:) forControlEvents:UIControlEventTouchUpInside|UIControlEventTouchUpOutside|UIControlEventTouchCancel];cell.accessoryView=v;
    } else if([kind isEqual:@"action"]) {
        cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;cell.selectionStyle=UITableViewCellSelectionStyleDefault;
    } else if([kind isEqual:@"choice"]) {
        NSArray *options=row[@"default"];NSInteger value=(NSInteger)ALB5Number(key,[key isEqual:@"collapse.nativeStyle"] ? 1 : 0,0,options.count-1);
        cell.detailTextLabel.text=options[value];cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;
    } else { cell.accessoryType=UITableViewCellAccessoryDisclosureIndicator;cell.imageView.image=[UIImage systemImageNamed:@"circle.fill"];cell.imageView.tintColor=ALB5Color(key,[key isEqualToString:@"panel.textColor"] ? UIColor.whiteColor : UIColor.systemTealColor); }
    BOOL timer=ALB15CompactStyle()==0 && ALB5Bool(@"collapse.timer",NO);
    if ((ALB15CompactStyle()!=0 && ([key isEqual:@"collapse.timer"] || [key isEqual:@"bottom.expandOnLeaveFeed"])) || (([key isEqual:@"collapse.delay"] || [key isEqual:@"collapse.nextExpands"]) && !timer)) {
        cell.userInteractionEnabled=NO; cell.contentView.alpha=.4;cell.accessoryView.alpha=.4;
    }
    return cell;
}
- (void)toggle:(UISwitch *)sender { [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:ALB5Key(sender.accessibilityIdentifier)];[self commit:sender]; [self.tableView reloadData]; }
- (void)slide:(UISlider *)sender {
    double value=sender.value;
    // 3.11：三段式底栏的秒数和尺寸按整数存（同原工程），动画速度保留小数
    NSString *key=sender.accessibilityIdentifier;
    if([key hasPrefix:@"dock."] && ![key isEqualToString:@"dock.speed"] && ![key isEqualToString:@"dock.darkTint"])value=round(value);
    [NSUserDefaults.standardUserDefaults setDouble:value forKey:ALB5Key(key)];
    for(UIView *v=sender;v;v=v.superview)if([v isKindOfClass:UITableViewCell.class]) { ((UITableViewCell *)v).detailTextLabel.text=[NSString stringWithFormat:@"%.2f",value];break; }
}
- (void)commit:(id)sender {
    [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
    AWERefreshAllConfiguredIcons();
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    NSDictionary *row=[self optionSections][path.section][@"rows"][path.row];NSString *kind=row[@"kind"],*key=row[@"key"];
    if([kind isEqual:@"action"]) {
        [tableView deselectRowAtIndexPath:path animated:YES];
        ALB5OptionAction(key, self, [tableView cellForRowAtIndexPath:path]);
        return;
    }
    if([kind isEqual:@"choice"]) {
        UIAlertController *alert=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleAlert];
        NSArray *options=row[@"default"];
        for(NSUInteger i=0;i<options.count;i++)[alert addAction:[UIAlertAction actionWithTitle:options[i] style:UIAlertActionStyleDefault handler:^(UIAlertAction *a) {
            [NSUserDefaults.standardUserDefaults setInteger:i forKey:ALB5Key(key)];[self.tableView reloadData];
            if ([key isEqualToString:@"collapse.nativeStyle"])
                [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
            else if ([key isEqualToString:@"bottom.style"])
                AWEShowDownloadToast((NSInteger)i == ALBDockModeActive() ? @"底栏样式未变" : @"底栏样式已保存，重启抖音后生效");
            else [self commit:nil];
        }]];
        [alert addAction:[UIAlertAction actionWithTitle:@"取消" style:UIAlertActionStyleCancel handler:nil]];[self presentViewController:alert animated:YES completion:nil];
    } else if([kind isEqual:@"color"]) {
        self.colorKey=key;UIColorPickerViewController *picker=[UIColorPickerViewController new];picker.delegate=self;
        picker.selectedColor=ALB5Color(key,[key isEqualToString:@"panel.textColor"] ? UIColor.whiteColor : UIColor.systemTealColor);[self presentViewController:picker animated:YES completion:nil];
    }
}
- (void)colorPickerViewControllerDidSelectColor:(UIColorPickerViewController *)picker {
    CGFloat r,g,b,a;if([picker.selectedColor getRed:&r green:&g blue:&b alpha:&a])
        [NSUserDefaults.standardUserDefaults setObject:@[@(r),@(g),@(b),@(a)] forKey:ALB5Key(self.colorKey)];
}
- (void)colorPickerViewControllerDidFinish:(UIColorPickerViewController *)picker { [self.tableView reloadData];[self commit:nil]; }
@end
