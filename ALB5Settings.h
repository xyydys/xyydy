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
    self.title = @"功能启用与排序";
    self.orderedActions = [ALB28PaletteActionOrder() mutableCopy];
    self.tableView.backgroundColor = UIColor.clearColor;
    self.tableView.allowsSelectionDuringEditing = NO;
    self.tableView.editing = YES;
    ALBInstallSettingsGlass(self);
}
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return self.orderedActions.count;
}
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    return @"开关控制显示，拖动右侧把手排序。保存全部图片仅在图集作品显示；保存当前内容、图集视频等项目会按当前作品类型显示。";
}
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)path {
    NSInteger action = self.orderedActions[path.row].integerValue;
    NSArray *titles = @[@"作者主页", @"保存当前图片／视频", @"图集视频／复制链接", @"点赞", @"保存音频", @"评论", @"复制文案", @"音量", @"亮度", @"保存全部图片", @"收藏视频", @"转发", @"关注作者"];
    NSArray *symbols = @[@"person.crop.circle", @"square.and.arrow.down", @"link", @"heart", @"music.note", @"bubble.right", @"doc.on.doc", @"speaker.wave.2", @"sun.max", @"photo.on.rectangle.angled", @"star", @"arrowshape.turn.up.right", @"person.badge.plus"];
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
static NSArray *ALB5OptionSections(void) { return @[
@{@"title":@"底栏与收缩",@"rows":@[@{@"key":@"bottom.scale",@"title":@"底栏整体缩放",@"kind":@"range",@"default":@(1),@"low":@(0.55),@"high":@(1.15)},@{@"key":@"collapse.expandOnDown",@"title":@"手指下滑／返回上一条时展开",@"kind":@"bool",@"default":@YES}]},
@{@"title":@"顶栏",@"rows":@[@{@"key":@"top.compactScale",@"title":@"顶栏整体比例",@"kind":@"range",@"default":@(0.78),@"low":@(0.45),@"high":@(1)},@{@"key":@"top.nearGroup",@"title":@"两侧按钮靠近分组",@"kind":@"bool",@"default":@NO},@{@"key":@"top.leftGap",@"title":@"左侧间距",@"kind":@"range",@"default":@(14),@"low":@(0),@"high":@(80)},@{@"key":@"top.rightGap",@"title":@"右侧间距",@"kind":@"range",@"default":@(14),@"low":@(0),@"high":@(80)}]},
@{@"title":@"快捷面板",@"rows":@[@{@"key":@"panel.customTextColor",@"title":@"固定文字颜色（关闭自动黑白）",@"kind":@"bool",@"default":@NO},@{@"key":@"panel.textColor",@"title":@"自定义文字颜色",@"kind":@"color",@"default":@(0)},@{@"key":@"panel.inset",@"title":@"填充边距",@"kind":@"range",@"default":@(3),@"low":@(1),@"high":@(6)},@{@"key":@"panel.volumeColor",@"title":@"音量填充颜色",@"kind":@"color",@"default":@(0)},@{@"key":@"panel.brightnessColor",@"title":@"亮度填充颜色",@"kind":@"color",@"default":@(0)},@{@"key":@"panel.fillAlpha",@"title":@"进度填充透明度",@"kind":@"range",@"default":@(0.26),@"low":@(0.05),@"high":@(0.65)},@{@"key":@"panel.iconFill",@"title":@"图标添加同色浅填充",@"kind":@"bool",@"default":@NO},@{@"key":@"panel.breathe",@"title":@"图标呼吸",@"kind":@"choice",@"default":@[@"关闭",@"中心呼吸",@"上浮呼吸"]},@{@"key":@"panel.breathePeriod",@"title":@"呼吸周期（秒）",@"kind":@"range",@"default":@(2.6),@"low":@(1),@"high":@(8)}]},
@{@"title":@"右侧布局",@"rows":@[@{@"key":@"right.scale",@"title":@"右侧整体缩放（含音乐）",@"kind":@"range",@"default":@(1),@"low":@(0.5),@"high":@(1.25)},@{@"key":@"right.musicGap",@"title":@"音乐与上方按钮间距（移动上方）",@"kind":@"range",@"default":@(12),@"low":@(0),@"high":@(80)},@{@"key":@"right.hide.avatar",@"title":@"隐藏头像",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.like",@"title":@"隐藏点赞",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.comment",@"title":@"隐藏评论",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.favorite",@"title":@"隐藏收藏",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.share",@"title":@"隐藏转发",@"kind":@"bool",@"default":@NO},@{@"key":@"right.hide.music",@"title":@"隐藏音乐",@"kind":@"bool",@"default":@NO}]}]; }

@implementation ALB5OptionsController
- (instancetype)init { return [super initWithStyle:UITableViewStyleInsetGrouped]; }
- (instancetype)initWithSections:(NSIndexSet *)sections title:(NSString *)title { if ((self=[self init])) { self.visibleSections=sections; self.title=title; } return self; }
- (NSArray *)optionSections { NSArray *sections=ALB5OptionSections(); return self.visibleSections ? [sections objectsAtIndexes:self.visibleSections] : sections; }
- (void)viewDidLoad { [super viewDidLoad]; if(!self.title)self.title=@"布局与动画"; self.navigationItem.rightBarButtonItem=[[UIBarButtonItem alloc] initWithTitle:@"诊断" style:UIBarButtonItemStylePlain target:self action:@selector(copyAdaptationStatus)]; self.tableView.backgroundColor=UIColor.clearColor; ALBInstallSettingsGlass(self); }
- (void)copyAdaptationStatus {
    NSMutableString *text=[NSMutableString stringWithFormat:@"LiquidBar TEST37 FIX1\niOS %@\n%@\n%@\n%@",UIDevice.currentDevice.systemVersion,ALB6CollapseDiagnostics(),ALB6BottomGeometryStatus(gAWEAlbumMorphController.tabBar),[gAWEAlbumMorphController nativeCollapseDiagnostics] ?: @"等待原生滚动源"];
    if (gAWEAlbumMorphController.viewIfLoaded.window) {
        [text appendFormat:@"\n当前原生状态=%d\n收缩入口清单=%@",
            ALB15NativeMinimizedState(gAWEAlbumMorphController),
            ALB22DescribeMinimizeCapabilities(ALB22OwnedMinimizeTargets(gAWEAlbumMorphController))];
    }
    for(UIView *container in gALB5TopContainers.allObjects) {
        AWETopLiquidGroupView *group=objc_getAssociatedObject(container,&kAWETopLiquidGroupViewKey);
        if(group.window)[text appendFormat:@"\n顶栏：%@",group.nativeGroup.diagnosticSummary];
        for (UIView *wrapper in ALB8TopModuleHost(container).subviews) {
            if (![wrapper isKindOfClass:AWEHomeTopSlotView.class]) continue;
            AWEHomeTopSlotView *slot = (AWEHomeTopSlotView *)wrapper;
            UIButton *button = slot.topKind == AWEHomeTopActionKindSearch ? slot.systemGlassButton : slot.hitButton;
            if (slot.window && !slot.hidden && button)
                [text appendFormat:@"\n侧按钮 kind=%ld class=%@ alpha=%.3f menu=%d targets=%@ gestures=%lu", (long)slot.topKind,
                    NSStringFromClass(button.class), button.alpha, button.showsMenuAsPrimaryAction,
                    [button actionsForTarget:slot forControlEvent:UIControlEventTouchUpInside],
                    (unsigned long)button.gestureRecognizers.count];
        }
    }
    UIPasteboard.generalPasteboard.string=text;
    UIAlertController *alert=[UIAlertController alertControllerWithTitle:@"适配状态已复制" message:text preferredStyle:UIAlertControllerStyleAlert];
    [alert addAction:[UIAlertAction actionWithTitle:@"好" style:UIAlertActionStyleDefault handler:nil]];
    [self presentViewController:alert animated:YES completion:nil];
}
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView { return [self optionSections].count; }
- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section { return [[self optionSections][section][@"rows"] count]; }
- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section { return [self optionSections][section][@"title"]; }
- (NSString *)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    NSString *title=[self optionSections][section][@"title"];
    if([title isEqual:@"底栏与收缩"])return @"仅保留系统原生收缩：左侧小球与右侧搜索由 UIKit 原生动画完成；上滑／下一条收缩，下滑／上一条展开。";
    if([title isEqual:@"顶栏"])return @"两侧按钮改用独立原生搜索位承载各自图标，不再用选中的普通标签，也不再隐藏透镜或图标。保留现有共享玻璃材质入口；最终外观须真机核对。";
    if([title isEqual:@"快捷面板"])return @"功能开关与顺序在“功能启用与排序”调整。固定文字颜色可避免画面变化时反复切换黑白；图标的图案、颜色、大小和透明度仍在图标中心调整。";
    return @"隐藏后上方元素依次下移补位。整体缩放以转发数字中心为基点；音乐单独缩放。";
}
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
    [NSUserDefaults.standardUserDefaults setDouble:sender.value forKey:ALB5Key(sender.accessibilityIdentifier)];
    for(UIView *v=sender;v;v=v.superview)if([v isKindOfClass:UITableViewCell.class]) { ((UITableViewCell *)v).detailTextLabel.text=[NSString stringWithFormat:@"%.2f",sender.value];break; }
}
- (void)commit:(id)sender {
    [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
    // Geometry has its own bounded update path; rebuilding every icon/tab
    // after this slider re-entered native layout and caused the scale hang.
    if ([sender isKindOfClass:UISlider.class] &&
        [((UISlider *)sender).accessibilityIdentifier isEqualToString:@"bottom.scale"]) return;
    AWERefreshAllConfiguredIcons();
}
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)path {
    NSDictionary *row=[self optionSections][path.section][@"rows"][path.row];NSString *kind=row[@"kind"],*key=row[@"key"];
    if([kind isEqual:@"choice"]) {
        UIAlertController *alert=[UIAlertController alertControllerWithTitle:row[@"title"] message:nil preferredStyle:UIAlertControllerStyleAlert];
        NSArray *options=row[@"default"];
        for(NSUInteger i=0;i<options.count;i++)[alert addAction:[UIAlertAction actionWithTitle:options[i] style:UIAlertActionStyleDefault handler:^(UIAlertAction *a) {
            [NSUserDefaults.standardUserDefaults setInteger:i forKey:ALB5Key(key)];[self.tableView reloadData];
            if ([key isEqualToString:@"collapse.nativeStyle"])
                [NSNotificationCenter.defaultCenter postNotificationName:@"ALB5.OptionsChanged" object:nil];
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
