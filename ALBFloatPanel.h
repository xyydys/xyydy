/* 三段式底栏的悬浮面板：✦（mode 0）＝双击面板那些功能的卡片，↑（mode 1）＝快捷跳转的四页。
   · 一整块圆角玻璃（Clear＋深色色调 float.tint），左右各让 12，底边离底栏 float.gap；太高时卡片区滚动。
   · 头部：自己的头像、名字、简介；齿轮＝打开插件设置整页；调色盘＝卡片区换成“外观”（相册壁纸、恢复玻璃、每行几个、
     深色色调、背景透明度 float.clarity）。壁纸存在 Library/Application Support/AwemeLiquidBar/float-wallpaper.jpg。
   · 卡片：半透明黑底，宽:高＝1.618:1，每行 2–4 个（float.columns），文字白色；音量、亮度左右滑调节；✦ 的卡片长按拖动排序
     （float.order）。动作走 ALBPaletteRun，跳转走 ALBDockHostQuickJump。点面板外面关（ALBOutsidePress），再点同一个键也关。
   Included by Tweak.x after ALBSidePalette.h. */

#import <PhotosUI/PhotosUI.h>

static NSString *ALBFloatWallpaperPath(void) {
    NSString *base = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).firstObject;
    NSString *dir = [base stringByAppendingPathComponent:@"AwemeLiquidBar"];
    [NSFileManager.defaultManager createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    return [dir stringByAppendingPathComponent:@"float-wallpaper.jpg"];
}
static NSInteger ALBFloatColumns(void) {
    NSInteger index = [NSUserDefaults.standardUserDefaults integerForKey:ALB5Key(@"float.columns")];
    NSArray<NSNumber *> *values = @[@3, @2, @4];
    return values[(NSUInteger)MIN(MAX(index, 0), 2)].integerValue;
}
static NSArray<NSNumber *> *ALBFloatOrder(void) {
    id saved = [NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"float.order")];
    NSArray<NSNumber *> *base = ALB28PaletteActionOrder();
    NSMutableArray<NSNumber *> *order = [NSMutableArray array];
    for (id v in [saved isKindOfClass:NSArray.class] ? saved : @[]) {
        if ([v isKindOfClass:NSNumber.class] && [base containsObject:v] && ![order containsObject:v]) [order addObject:v];
    }
    for (NSNumber *v in base) if (![order containsObject:v]) [order addObject:v];
    return order;
}
static NSUInteger gALBFloatOpens, gALBFloatRuns, gALBFloatMoves, gALBFloatSlides;
static NSString *gALBFloatLast;
static void ALBFloatOpenSettingsFull(UIWindow *window);
/* ↑ 的快捷跳转（ALBQuickJump.h） */
NSString *ALBDockHostQuickJumpTitle(NSInteger kind);
NSString *ALBDockHostQuickJumpSymbol(NSInteger kind);
BOOL ALBDockHostQuickJumpReady(NSInteger kind);
BOOL ALBDockHostQuickJump(NSInteger kind);
static NSUInteger gALBJumpPanelOpens, gALBJumpPanelPicks;
static NSUInteger gALBFloatPickTaps, gALBFloatPickShown, gALBFloatPickResults, gALBFloatPickWrites, gALBFloatSettingsOpens;
static NSString *gALBFloatPickLast;
/* 深色色调（玻璃染黑的浓度，有壁纸时是压在壁纸上的黑）、背景透明度（壁纸的透明度） */
static CGFloat ALBFloatTint(void) {
    id v = [NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"float.tint")];
    return MIN(MAX(v ? [v doubleValue] : 0.22, 0.0), 0.8);
}
static CGFloat ALBFloatClarity(void) {
    id v = [NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"float.clarity")];
    return MIN(MAX(v ? [v doubleValue] : 0.0, 0.0), 0.9);
}

/* 每一项的标题、图标、这件作品上能不能用（和双击面板 buildRows 同一套规则） */
@interface ALBFloatTexts : NSObject
@property (nonatomic, copy) NSArray<NSString *> *titles, *symbols;
@property (nonatomic, strong) NSMutableIndexSet *available;
@property (nonatomic, strong) UIImage *authorAvatar;
@end
@implementation ALBFloatTexts
@end
static ALBFloatTexts *ALBFloatTextsFor(id aweme, UIView *stack) {
    BOOL album = aweme && AWEAlbumImagesForAweme(aweme).count > 0;
    BOOL likeKnown = NO, liked = stack ? ALB8PaletteLikeState(stack, &likeKnown) : NO;
    id author = AWEFirstSafeValue(aweme, @[@"author", @"authorModel", @"user"]);
    NSString *nickname = AWEStringFromValue(AWEFirstSafeValue(author, @[@"nickname", @"nickName", @"name"]));
    BOOL mainLive = !album && aweme && ALBMSMainIsLive(aweme);
    NSUInteger albumCount = album ? AWEAlbumImagesForAweme(aweme).count : 0;
    ALBMSKind currentKind = album ? ALBMSAlbumCurrentKind(aweme) : ALBMSKindVideo;
    NSUInteger lives = album ? ALBMSAlbumLiveCount(aweme) : 0;
    BOOL mixed = lives > 0;
    if (album && !mixed) for (id image in AWEAlbumImagesForAweme(aweme)) if (ALBMSAlbumEntry(image).kind == ALBMSKindVideo) { mixed = YES; break; }
    BOOL favoriteKnown = NO, favorited = NO;
    NSArray<UIView *> *nativeActions = stack ? ALB33AllHomeActionButtons(stack) : @[];
    for (UIView *action in nativeActions) if (ALB33PanelActionKind(action, nativeActions) == AWEHomeActionKindFavorite) {
        UIImageView *image = objc_getAssociatedObject(action, &kAWEHomeActionOriginalImageViewKey);
        favoriteKnown = YES; favorited = AWEHomeActionIsSelected(action, AWEHomeActionKindFavorite, image); break;
    }
    ALBFloatTexts *t = [ALBFloatTexts new];
    t.titles = @[nickname.length ? nickname : @"作者主页", album ? @"保存当前图片" : (mainLive ? @"保存实况" : @"保存当前视频"),
        album ? @"保存图集为视频" : @"复制作品链接", (likeKnown && liked) ? @"取消点赞" : @"点赞作品", @"保存音频", @"打开评论区", @"复制完整文案",
        @"音量", @"亮度", @"保存所有图片", (favoriteKnown && favorited) ? @"取消收藏" : @"收藏作品", @"转发作品", @"关注作者",
        gAWEStableCleanScreenEnabled ? @"退出清屏" : @"一键清屏", currentKind == ALBMSKindVideo ? @"保存当前视频" : @"保存当前实况", @"保存所有实况", @"保存所有类型图片"];
    t.symbols = @[@"person.crop.circle", @"square.and.arrow.down", album ? @"film" : @"link", (likeKnown && liked) ? @"heart.fill" : @"heart",
        @"music.note", @"text.bubble", @"doc.on.doc", @"speaker.wave.2.fill", @"sun.max.fill", @"photo.on.rectangle.angled",
        (favoriteKnown && favorited) ? @"star.fill" : @"star", @"arrowshape.turn.up.right", @"person.badge.plus",
        gAWEStableCleanScreenEnabled ? @"eye" : @"eye.slash", currentKind == ALBMSKindVideo ? @"video" : @"livephoto", @"livephoto", @"square.stack.3d.down.right"];
    t.available = [NSMutableIndexSet indexSet];
    for (NSUInteger i = 0; i < t.titles.count; i++) {
        if (!ALB5Bool([NSString stringWithFormat:@"panel.action.%lu", (unsigned long)i], YES)) continue;
        if (i == 9 && !(album && albumCount > 1)) continue;
        if (i == 14 && !(album && (currentKind == ALBMSKindLive || currentKind == ALBMSKindVideo))) continue;
        if (i == 15 && !(album && albumCount > 1 && lives > 0)) continue;
        if (i == 16 && !(album && albumCount > 1 && mixed)) continue;
        [t.available addIndex:i];
    }
    if (stack) {
        UIView *host = AWEFindViewByExactRuntimeClass(stack, @"AWEPlayInteractionUserProfilePictureView", 0);
        t.authorAvatar = AWEBestProfileAvatarImageView(host).image;
    }
    return t;
}

#pragma mark - 卡片

@interface ALBFloatCard : UICollectionViewCell <UIGestureRecognizerDelegate>
@property (nonatomic, strong) UIView *plate, *fill;
@property (nonatomic, strong) UIImageView *icon;
@property (nonatomic, strong) UILabel *title;
@property (nonatomic, assign) NSInteger action;
@property (nonatomic, assign) BOOL slider, usable;
@property (nonatomic, assign) CGFloat level, panStart;
@property (nonatomic, strong) UIPanGestureRecognizer *slide;
@property (nonatomic, copy) void (^onLevel)(CGFloat level);
@end

@implementation ALBFloatCard
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        _plate = [UIView new];
        _plate.backgroundColor = [UIColor colorWithWhite:0 alpha:0.40];
        _plate.layer.cornerCurve = kCACornerCurveContinuous;
        _plate.clipsToBounds = YES;
        _plate.userInteractionEnabled = NO;
        [self.contentView addSubview:_plate];
        _fill = [UIView new];
        _fill.backgroundColor = [UIColor colorWithWhite:1 alpha:0.20];
        _fill.hidden = YES;
        [_plate addSubview:_fill];
        _icon = [UIImageView new];
        _icon.contentMode = UIViewContentModeScaleAspectFit;
        _icon.tintColor = UIColor.whiteColor;
        [_plate addSubview:_icon];
        _title = [UILabel new];
        _title.textColor = UIColor.whiteColor;
        _title.textAlignment = NSTextAlignmentCenter;
        _title.adjustsFontSizeToFitWidth = YES;
        _title.minimumScaleFactor = 0.7;
        [_plate addSubview:_title];
        _slide = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(alb_slid:)];
        _slide.delegate = self;
        [self.contentView addGestureRecognizer:_slide];
        self.isAccessibilityElement = YES;
    }
    return self;
}
- (void)layoutSubviews {
    [super layoutSubviews];
    CGRect b = self.contentView.bounds;
    self.plate.frame = b;
    self.plate.layer.cornerRadius = MIN(18, b.size.height * 0.26);
    CGFloat h = b.size.height, w = b.size.width;
    CGFloat iconSide = MIN(26, h * 0.34), small = w < 96;
    self.title.font = [UIFont systemFontOfSize:small ? 11.5 : 13.5 weight:UIFontWeightSemibold];
    CGFloat titleH = ceil(self.title.font.lineHeight);
    CGFloat total = iconSide + 6 + titleH;
    CGFloat y0 = (h - total) / 2.0;
    self.icon.frame = CGRectMake((w - iconSide) / 2.0, y0, iconSide, iconSide);
    self.title.frame = CGRectMake(6, y0 + iconSide + 6, w - 12, titleH);
    self.fill.frame = CGRectMake(0, 0, w * MIN(MAX(self.level, 0), 1), h);
}
- (void)setHighlighted:(BOOL)highlighted {
    [super setHighlighted:highlighted];
    [UIView animateWithDuration:0.18 delay:0 options:UIViewAnimationOptionBeginFromCurrentState | UIViewAnimationOptionAllowUserInteraction animations:^{
        self.plate.transform = highlighted && self.usable && !self.slider ? CGAffineTransformMakeScale(0.95, 0.95) : CGAffineTransformIdentity;
        self.plate.backgroundColor = [UIColor colorWithWhite:0 alpha:highlighted && self.usable && !self.slider ? 0.52 : 0.40];
    } completion:nil];
}
- (void)setLevel:(CGFloat)level {
    _level = MIN(MAX(level, 0), 1);
    self.fill.frame = CGRectMake(0, 0, self.plate.bounds.size.width * _level, self.plate.bounds.size.height);
    if (self.slider) self.accessibilityValue = [NSString stringWithFormat:@"%.0f%%", _level * 100];
}
- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gesture {
    if (gesture != self.slide) return YES;
    if (!self.slider || !self.usable) return NO;
    CGPoint v = [self.slide velocityInView:self];
    return fabs(v.x) > fabs(v.y);
}
- (void)alb_slid:(UIPanGestureRecognizer *)pan {
    if (pan.state == UIGestureRecognizerStateBegan) { self.panStart = self.level; gALBFloatSlides++; }
    if (pan.state != UIGestureRecognizerStateBegan && pan.state != UIGestureRecognizerStateChanged) return;
    CGFloat width = MAX(1, self.plate.bounds.size.width);
    CGFloat next = self.panStart + [pan translationInView:self].x / width;
    CGFloat before = self.level;
    self.level = next;
    if (fabs(before - self.level) > 0.0001 && self.onLevel) self.onLevel(self.level);
    if ((before < 1 && self.level >= 1) || (before > 0 && self.level <= 0)) [[UIImpactFeedbackGenerator.alloc initWithStyle:UIImpactFeedbackStyleLight] impactOccurred];
}
- (void)accessibilityIncrement { if (self.slider) { self.level += 0.05; if (self.onLevel) self.onLevel(self.level); } }
- (void)accessibilityDecrement { if (self.slider) { self.level -= 0.05; if (self.onLevel) self.onLevel(self.level); } }
@end

#pragma mark - 面板

@interface ALBFloatPanel : UIView <UICollectionViewDataSource, UICollectionViewDelegateFlowLayout, PHPickerViewControllerDelegate>
@property (nonatomic, strong) UIVisualEffectView *glass;
@property (nonatomic, strong) UIImageView *wallpaper, *avatar;
@property (nonatomic, strong) UILabel *name, *bio;
@property (nonatomic, strong) UIButton *paletteButton, *gearButton;
@property (nonatomic, strong) UIView *shade;                 // 3.11.47：有壁纸时压在壁纸上的黑（深色色调）
@property (nonatomic, strong) UIView *editor;                // 3.11.47：外观（点调色盘换出来）
@property (nonatomic, strong) UIButton *pickButton, *resetButton;
@property (nonatomic, strong) UISegmentedControl *columnsControl;
@property (nonatomic, strong) UISlider *tintSlider, *claritySlider;
@property (nonatomic, strong) UILabel *tintLabel, *clarityLabel;
@property (nonatomic, assign) BOOL editing;
@property (nonatomic, strong) UICollectionView *grid;
@property (nonatomic, strong) MPVolumeView *volumeView;
@property (nonatomic, weak) UISlider *systemVolume;
@property (nonatomic, strong) ALBOutsidePress *press;
@property (nonatomic, strong) NSMutableArray<NSNumber *> *items;
@property (nonatomic, strong) ALBFloatTexts *texts;
@property (nonatomic, weak) UIView *stack, *source;
@property (nonatomic, strong) id model;
@property (nonatomic, weak) AWEPlayInteractionViewController *controller;
@property (nonatomic, assign) CGRect anchor;
@property (nonatomic, assign) CGFloat bottomLimit;
@property (nonatomic, assign) BOOL dismissing, picking;
@property (nonatomic, assign) NSInteger mode;              // 0＝功能卡片（✦） 1＝快捷跳转（↑）
@end

static __weak ALBFloatPanel *gALBFloatPanel;

@implementation ALBFloatPanel

- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.backgroundColor = UIColor.clearColor;
        self.accessibilityIdentifier = @"AwemeLiquidBar.FloatPanel";
        _glass = [[UIVisualEffectView alloc] initWithEffect:nil];
        _glass.userInteractionEnabled = YES;
        _glass.clipsToBounds = NO;
        [self addSubview:_glass];
        _wallpaper = [UIImageView new];
        _wallpaper.contentMode = UIViewContentModeScaleAspectFill;
        _wallpaper.clipsToBounds = YES;
        _wallpaper.layer.cornerCurve = kCACornerCurveContinuous;
        _wallpaper.userInteractionEnabled = NO;
        [_glass.contentView addSubview:_wallpaper];
        _shade = [UIView new];
        _shade.userInteractionEnabled = NO;
        _shade.backgroundColor = UIColor.blackColor;
        _shade.layer.cornerCurve = kCACornerCurveContinuous;
        _shade.clipsToBounds = YES;
        [_glass.contentView addSubview:_shade];
        _avatar = [UIImageView new];
        _avatar.contentMode = UIViewContentModeScaleAspectFill;
        _avatar.clipsToBounds = YES;
        _avatar.tintColor = UIColor.whiteColor;
        [_glass.contentView addSubview:_avatar];
        _name = [UILabel new];
        _name.font = [UIFont systemFontOfSize:20 weight:UIFontWeightBold];
        [_glass.contentView addSubview:_name];
        _bio = [UILabel new];
        _bio.font = [UIFont systemFontOfSize:13 weight:UIFontWeightRegular];
        [_glass.contentView addSubview:_bio];
        _paletteButton = [UIButton buttonWithType:UIButtonTypeSystem];
        UIButtonConfiguration *cfg = nil;
        if (@available(iOS 26.0, *)) cfg = [UIButtonConfiguration glassButtonConfiguration];
        else cfg = [UIButtonConfiguration filledButtonConfiguration];
        cfg.image = [UIImage systemImageNamed:@"paintpalette.fill" withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:19 weight:UIImageSymbolWeightSemibold]];
        cfg.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
        _paletteButton.configuration = cfg;
        _paletteButton.accessibilityLabel = @"面板外观：壁纸、色调、透明度";
        [_paletteButton addTarget:self action:@selector(alb_toggleEditor) forControlEvents:UIControlEventTouchUpInside];
        [_glass.contentView addSubview:_paletteButton];
        _gearButton = [UIButton buttonWithType:UIButtonTypeSystem];
        UIButtonConfiguration *gear = nil;
        if (@available(iOS 26.0, *)) gear = [UIButtonConfiguration glassButtonConfiguration];
        else gear = [UIButtonConfiguration filledButtonConfiguration];
        gear.image = [UIImage systemImageNamed:@"gearshape.fill" withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:18 weight:UIImageSymbolWeightSemibold]];
        gear.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
        gear.baseForegroundColor = UIColor.whiteColor;
        _gearButton.configuration = gear;
        _gearButton.accessibilityLabel = @"打开插件设置（整页）";
        if (@available(iOS 26.0, *)) _gearButton.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;
        [_gearButton addTarget:self action:@selector(alb_openSettings) forControlEvents:UIControlEventTouchUpInside];
        [_glass.contentView addSubview:_gearButton];
        UICollectionViewFlowLayout *layout = [UICollectionViewFlowLayout new];
        layout.minimumInteritemSpacing = 10;
        layout.minimumLineSpacing = 10;
        _grid = [[UICollectionView alloc] initWithFrame:CGRectZero collectionViewLayout:layout];
        _grid.backgroundColor = UIColor.clearColor;
        _grid.showsVerticalScrollIndicator = NO;
        _grid.dataSource = self;
        _grid.delegate = self;
        _grid.clipsToBounds = YES;
        [_grid registerClass:ALBFloatCard.class forCellWithReuseIdentifier:@"card"];
        UILongPressGestureRecognizer *move = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(alb_moved:)];
        move.minimumPressDuration = 0.38;
        [_grid addGestureRecognizer:move];
        [_glass.contentView addSubview:_grid];
        _volumeView = [[MPVolumeView alloc] initWithFrame:CGRectMake(-10000, 0, 160, 44)];
        _volumeView.userInteractionEnabled = NO;
        _volumeView.accessibilityElementsHidden = YES;
        [self addSubview:_volumeView];
        [self alb_buildEditor];
        [self alb_applyInk:NO];     // 3.11.47：文字固定白色
        [self alb_reloadWallpaper];
    }
    return self;
}

#pragma mark 3.11.47 外观

static UIButton *ALBFloatPlainButton(NSString *title, NSString *symbol) {
    UIButtonConfiguration *cfg = [UIButtonConfiguration filledButtonConfiguration];
    cfg.title = title;
    cfg.image = [UIImage systemImageNamed:symbol withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:14 weight:UIImageSymbolWeightSemibold]];
    cfg.imagePadding = 6;
    cfg.baseBackgroundColor = [UIColor colorWithWhite:0 alpha:0.40];
    cfg.baseForegroundColor = UIColor.whiteColor;
    cfg.cornerStyle = UIButtonConfigurationCornerStyleCapsule;
    cfg.titleTextAttributesTransformer = ^NSDictionary<NSAttributedStringKey, id> *(NSDictionary<NSAttributedStringKey, id> *attrs) {
        NSMutableDictionary *m = [attrs mutableCopy];
        m[NSFontAttributeName] = [UIFont systemFontOfSize:14 weight:UIFontWeightSemibold];
        return m;
    };
    UIButton *b = [UIButton buttonWithConfiguration:cfg primaryAction:nil];
    return b;
}

static UILabel *ALBFloatSmallLabel(void) {
    UILabel *l = [UILabel new];
    l.font = [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold];
    l.textColor = UIColor.whiteColor;
    return l;
}

- (void)alb_buildEditor {
    UIView *editor = [UIView new];
    editor.alpha = 0;
    editor.hidden = YES;
    self.editor = editor;
    self.pickButton = ALBFloatPlainButton(@"从相册选壁纸", @"photo");
    [self.pickButton addTarget:self action:@selector(alb_pickTapped) forControlEvents:UIControlEventTouchUpInside];
    [editor addSubview:self.pickButton];
    self.resetButton = ALBFloatPlainButton(@"恢复液态玻璃", @"drop");
    [self.resetButton addTarget:self action:@selector(alb_resetWallpaper) forControlEvents:UIControlEventTouchUpInside];
    [editor addSubview:self.resetButton];
    self.columnsControl = [[UISegmentedControl alloc] initWithItems:@[@"每行 2 个", @"每行 3 个", @"每行 4 个"]];
    self.columnsControl.selectedSegmentTintColor = [UIColor colorWithWhite:1 alpha:0.30];
    self.columnsControl.backgroundColor = [UIColor colorWithWhite:0 alpha:0.30];
    [self.columnsControl setTitleTextAttributes:@{NSForegroundColorAttributeName: UIColor.whiteColor,
        NSFontAttributeName: [UIFont systemFontOfSize:13 weight:UIFontWeightSemibold]} forState:UIControlStateNormal];
    [self.columnsControl addTarget:self action:@selector(alb_columnsChanged) forControlEvents:UIControlEventValueChanged];
    [editor addSubview:self.columnsControl];
    self.tintLabel = ALBFloatSmallLabel();
    [editor addSubview:self.tintLabel];
    self.tintSlider = [UISlider new];
    self.tintSlider.minimumValue = 0;
    self.tintSlider.maximumValue = 0.8;
    self.tintSlider.minimumTrackTintColor = UIColor.whiteColor;
    [self.tintSlider addTarget:self action:@selector(alb_tintChanged) forControlEvents:UIControlEventValueChanged];
    [editor addSubview:self.tintSlider];
    self.clarityLabel = ALBFloatSmallLabel();
    [editor addSubview:self.clarityLabel];
    self.claritySlider = [UISlider new];
    self.claritySlider.minimumValue = 0;
    self.claritySlider.maximumValue = 0.9;
    self.claritySlider.minimumTrackTintColor = UIColor.whiteColor;
    [self.claritySlider addTarget:self action:@selector(alb_clarityChanged) forControlEvents:UIControlEventValueChanged];
    [editor addSubview:self.claritySlider];
    [self.glass.contentView addSubview:editor];
    [self alb_syncEditor];
}

- (void)alb_syncEditor {
    NSArray<NSNumber *> *values = @[@2, @3, @4];
    self.columnsControl.selectedSegmentIndex = (NSInteger)[values indexOfObject:@(ALBFloatColumns())];
    self.tintSlider.value = (float)ALBFloatTint();
    self.claritySlider.value = (float)ALBFloatClarity();
    self.tintLabel.text = [NSString stringWithFormat:@"深色色调 %.0f%%", ALBFloatTint() * 100];
    BOOL hasWallpaper = self.wallpaper.image != nil;
    self.clarityLabel.text = hasWallpaper ? [NSString stringWithFormat:@"背景透明度 %.0f%%", ALBFloatClarity() * 100] : @"背景透明度（选了壁纸才有）";
    self.claritySlider.enabled = hasWallpaper;
    self.claritySlider.alpha = hasWallpaper ? 1 : 0.4;
    self.resetButton.enabled = hasWallpaper;
    self.resetButton.alpha = hasWallpaper ? 1 : 0.45;
}

- (CGFloat)alb_editorHeight { return 40 + 12 + 32 + 14 + 22 + 30 + 8 + 22 + 30; }

- (void)alb_layoutEditorIn:(CGRect)area {
    self.editor.frame = area;
    CGFloat w = area.size.width, y = 0, half = floor((w - 10) / 2.0);
    self.pickButton.frame = CGRectMake(0, y, half, 40);
    self.resetButton.frame = CGRectMake(w - half, y, half, 40);
    y += 40 + 12;
    self.columnsControl.frame = CGRectMake(0, y, w, 32);
    y += 32 + 14;
    self.tintLabel.frame = CGRectMake(4, y, w - 8, 22);
    y += 22;
    self.tintSlider.frame = CGRectMake(4, y, w - 8, 30);
    y += 30 + 8;
    self.clarityLabel.frame = CGRectMake(4, y, w - 8, 22);
    y += 22;
    self.claritySlider.frame = CGRectMake(4, y, w - 8, 30);
}

- (void)alb_toggleEditor {
    self.editing = !self.editing;
    [self alb_syncEditor];
    if (self.editing) { self.editor.hidden = NO; }
    UIButtonConfiguration *cfg = self.paletteButton.configuration;
    cfg.image = [UIImage systemImageNamed:self.editing ? @"checkmark" : @"paintpalette.fill"
                        withConfiguration:[UIImageSymbolConfiguration configurationWithPointSize:19 weight:UIImageSymbolWeightSemibold]];
    self.paletteButton.configuration = cfg;
    self.paletteButton.accessibilityLabel = self.editing ? @"完成，回到功能卡片" : @"面板外观：壁纸、色调、透明度";
    [self alb_relayoutAnimated:YES];
    [UIView animateWithDuration:0.22 animations:^{
        self.editor.alpha = self.editing ? 1 : 0;
        self.grid.alpha = self.editing ? 0 : 1;
    } completion:^(__unused BOOL finished) {
        if (!self.editing) self.editor.hidden = YES;
        self.grid.userInteractionEnabled = !self.editing;
    }];
}

- (void)alb_columnsChanged {
    NSArray<NSNumber *> *values = @[@2, @3, @4];
    NSInteger seg = self.columnsControl.selectedSegmentIndex;
    if (seg < 0 || seg >= (NSInteger)values.count) return;
    // float.columns 存的是设置页里的选项下标（[3, 2, 4]）
    NSInteger stored = (NSInteger)[@[@3, @2, @4] indexOfObject:values[(NSUInteger)seg]];
    [NSUserDefaults.standardUserDefaults setInteger:stored forKey:ALB5Key(@"float.columns")];
    [self.grid.collectionViewLayout invalidateLayout];
    [self.grid reloadData];
}

- (void)alb_tintChanged {
    [NSUserDefaults.standardUserDefaults setDouble:self.tintSlider.value forKey:ALB5Key(@"float.tint")];
    self.tintLabel.text = [NSString stringWithFormat:@"深色色调 %.0f%%", ALBFloatTint() * 100];
    [self alb_applyBackground];
}

- (void)alb_clarityChanged {
    [NSUserDefaults.standardUserDefaults setDouble:self.claritySlider.value forKey:ALB5Key(@"float.clarity")];
    self.clarityLabel.text = [NSString stringWithFormat:@"背景透明度 %.0f%%", ALBFloatClarity() * 100];
    [self alb_applyBackground];
}

/* 玻璃：Clear＋染黑（深色色调）；有壁纸：壁纸按透明度淡出，上面压一层黑（深色色调） */
- (void)alb_applyBackground {
    CGFloat tint = ALBFloatTint();
    BOOL hasWallpaper = self.wallpaper.image != nil;
    self.wallpaper.alpha = hasWallpaper ? 1.0 - ALBFloatClarity() : 0;
    self.shade.alpha = hasWallpaper ? tint : 0;
    self.shade.hidden = !hasWallpaper;
    if (!self.glass.window && !self.glass.effect) return;
    if (@available(iOS 26.0, *)) {
        UIGlassEffect *effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
        effect.interactive = NO;
        effect.tintColor = tint > 0.004 ? [UIColor colorWithWhite:0 alpha:tint] : nil;
        self.glass.effect = effect;
    } else {
        self.glass.effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterialDark];
    }
}

- (void)alb_resetWallpaper {
    [NSFileManager.defaultManager removeItemAtPath:ALBFloatWallpaperPath() error:nil];
    [self alb_reloadWallpaper];
    [self alb_syncEditor];
}

- (void)alb_openSettings {
    gALBFloatSettingsOpens++;
    UIWindow *window = self.window;
    [self dismiss];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.28 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        ALBFloatOpenSettingsFull(window);
    });
}

- (void)alb_applyInk:(BOOL)darkInk {
    darkInk = NO;     // 3.11.47 用户：“文字肯定固定成白色”
    UIColor *ink = darkInk ? UIColor.blackColor : UIColor.whiteColor;
    self.name.textColor = ink;
    self.bio.textColor = [ink colorWithAlphaComponent:0.7];
    self.avatar.tintColor = ink;
    if (@available(iOS 26.0, *)) self.paletteButton.overrideUserInterfaceStyle = darkInk ? UIUserInterfaceStyleLight : UIUserInterfaceStyleDark;
    UIButtonConfiguration *cfg = self.paletteButton.configuration;
    cfg.baseForegroundColor = ink;
    self.paletteButton.configuration = cfg;
}

- (void)alb_reloadWallpaper {
    UIImage *image = [UIImage imageWithContentsOfFile:ALBFloatWallpaperPath()];
    self.wallpaper.image = image;
    self.wallpaper.hidden = image == nil;
    [self alb_applyBackground];
}

/* 等这一下点击结束（按钮的抬手）再弹相册，弹的控制器跳过正在收起的 / 菜单的 */
- (void)alb_pickTapped {
    gALBFloatPickTaps++;
    self.picking = YES;
    __weak typeof(self) weakSelf = self;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.15 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        [weakSelf alb_pickWallpaper];
    });
}

- (void)alb_pickWallpaper {
    PHPickerConfiguration *config = [[PHPickerConfiguration alloc] init];
    config.filter = [PHPickerFilter imagesFilter];
    config.selectionLimit = 1;
    PHPickerViewController *picker = [[PHPickerViewController alloc] initWithConfiguration:config];
    picker.delegate = self;
    picker.modalPresentationStyle = UIModalPresentationFormSheet;
    UIWindow *window = self.window ?: AWEActiveMainWindow();
    UIViewController *top = window.rootViewController;
    while (top.presentedViewController && !top.presentedViewController.isBeingDismissed &&
           ![NSStringFromClass(top.presentedViewController.class) containsString:@"Menu"]) top = top.presentedViewController;
    if (!top || !top.viewIfLoaded.window) {
        self.picking = NO;
        gALBFloatPickLast = @"没找到能弹相册的页面";
        AWEShowDownloadToast(@"相册打不开：没找到能弹出的页面");
        return;
    }
    self.picking = YES;
    __weak typeof(self) weakSelf = self;
    gALBFloatPickLast = [NSString stringWithFormat:@"从 %@ 弹相册", NSStringFromClass(top.class)];
    [top presentViewController:picker animated:YES completion:^{
        gALBFloatPickShown++;
        (void)weakSelf;
    }];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        if (!picker.viewIfLoaded.window && !picker.isBeingPresented && picker.presentingViewController == nil) {
            ALBFloatPanel *panel = weakSelf;
            panel.picking = NO;
            gALBFloatPickLast = [NSString stringWithFormat:@"从 %@ 弹相册没弹出来", NSStringFromClass(top.class)];
        }
    });
}

- (void)picker:(PHPickerViewController *)picker didFinishPicking:(NSArray<PHPickerResult *> *)results {
    [picker dismissViewControllerAnimated:YES completion:nil];
    __weak typeof(self) weakSelf = self;
    gALBFloatPickResults += results.count ? 1 : 0;
    NSItemProvider *provider = results.firstObject.itemProvider;
    if (![provider canLoadObjectOfClass:UIImage.class]) {
        self.picking = NO;
        gALBFloatPickLast = results.count ? @"选回来的不是图片" : @"没选（取消）";
        return;
    }
    [provider loadObjectOfClass:UIImage.class completionHandler:^(id<NSItemProviderReading> object, __unused NSError *error) {
        UIImage *image = [(NSObject *)object isKindOfClass:UIImage.class] ? (UIImage *)object : nil;
        NSData *data = nil;
        if (image) {
            CGFloat side = 1600, scale = MIN(1, side / MAX(image.size.width, image.size.height));
            CGSize size = CGSizeMake(round(image.size.width * scale), round(image.size.height * scale));
            UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat preferredFormat];
            format.scale = 1;
            UIImage *small = [[[UIGraphicsImageRenderer alloc] initWithSize:size format:format] imageWithActions:^(__unused UIGraphicsImageRendererContext *c) {
                [image drawInRect:CGRectMake(0, 0, size.width, size.height)];
            }];
            data = UIImageJPEGRepresentation(small, 0.86);
        }
        dispatch_async(dispatch_get_main_queue(), ^{
            ALBFloatPanel *panel = weakSelf;
            panel.picking = NO;
            if (data) {
                BOOL ok = [data writeToFile:ALBFloatWallpaperPath() atomically:YES];
                if (ok) gALBFloatPickWrites++;
                gALBFloatPickLast = ok ? [NSString stringWithFormat:@"壁纸已写入（%.0f KB）", data.length / 1024.0] : @"壁纸写入失败";
                AWEShowDownloadToast(ok ? @"面板壁纸已更换" : @"壁纸保存失败");
            } else {
                gALBFloatPickLast = @"图片读不出来";
            }
            [panel alb_reloadWallpaper];
            [panel alb_syncEditor];
        });
    }];
}

#pragma mark 内容

- (void)alb_loadContent {
    self.avatar.image = ALB8OwnAvatar() ?: [UIImage systemImageNamed:@"person.crop.circle.fill"];
    self.name.text = ALB8OwnName();
    self.bio.text = ALB8OwnBio();
    if (self.mode == 1) {
        self.items = [@[@1, @2, @3, @4] mutableCopy];
        [self.grid reloadData];
        return;
    }
    self.texts = ALBFloatTextsFor(self.model, self.stack);
    NSMutableArray<NSNumber *> *items = [NSMutableArray array];
    for (NSNumber *n in ALBFloatOrder()) if ([self.texts.available containsIndex:n.unsignedIntegerValue]) [items addObject:n];
    self.items = items;
    [self.grid reloadData];
}

- (CGSize)alb_cardSize {
    NSInteger columns = ALBFloatColumns();
    CGFloat width = CGRectGetWidth(self.bounds) - 32;
    CGFloat card = floor((width - 10 * (columns - 1)) / columns);
    return CGSizeMake(card, round(card / 1.618));
}

- (CGRect)alb_targetFrame {
    UIWindow *window = self.window;
    CGFloat ww = CGRectGetWidth(window.bounds), wh = CGRectGetHeight(window.bounds);
    CGFloat width = MIN(ww - 24, 520);
    NSInteger columns = ALBFloatColumns();
    CGFloat card = floor((width - 32 - 10 * (columns - 1)) / columns), cardH = round(card / 1.618);
    NSUInteger rows = (self.items.count + (NSUInteger)columns - 1) / (NSUInteger)columns;
    CGFloat gridH = rows ? rows * cardH + (rows - 1) * 10 : 0;
    if (self.editing) gridH = MAX(gridH, [self alb_editorHeight]);
    CGFloat height = 16 + 58 + 16 + gridH + 16;
    CGFloat top = window.safeAreaInsets.top + 12;
    CGFloat bottom = self.bottomLimit > 0 ? self.bottomLimit : wh - window.safeAreaInsets.bottom - 90;
    height = MIN(height, bottom - top);
    return CGRectMake(round((ww - width) / 2.0), bottom - height, width, height);
}

- (void)layoutSubviews {
    [super layoutSubviews];
    CGRect b = self.bounds;
    self.glass.frame = b;
    CGFloat radius = MIN(38, b.size.height / 2.0);
    if (@available(iOS 26.0, *)) self.glass.cornerConfiguration = [UICornerConfiguration configurationWithUniformRadius:[UICornerRadius fixedRadius:radius]];
    else { self.glass.layer.cornerRadius = radius; self.glass.clipsToBounds = YES; }
    self.wallpaper.frame = b;
    self.wallpaper.layer.cornerRadius = radius;
    self.shade.frame = b;
    self.shade.layer.cornerRadius = radius;
    self.avatar.frame = CGRectMake(16, 16, 58, 58);
    self.avatar.layer.cornerRadius = 29;
    self.paletteButton.frame = CGRectMake(b.size.width - 16 - 46, 22, 46, 46);
    self.gearButton.frame = CGRectMake(CGRectGetMinX(self.paletteButton.frame) - 8 - 46, 22, 46, 46);
    CGFloat textX = CGRectGetMaxX(self.avatar.frame) + 12, textW = CGRectGetMinX(self.gearButton.frame) - 10 - textX;
    self.name.frame = CGRectMake(textX, 20, textW, 26);
    self.bio.frame = CGRectMake(textX, 48, textW, 20);
    CGFloat gridY = 16 + 58 + 16;
    self.grid.frame = CGRectMake(16, gridY, b.size.width - 32, MAX(0, b.size.height - gridY - 16));
    [self alb_layoutEditorIn:CGRectMake(16, gridY, b.size.width - 32, [self alb_editorHeight])];
    CGSize content = self.grid.collectionViewLayout.collectionViewContentSize;
    self.grid.scrollEnabled = content.height > self.grid.bounds.size.height + 1;
    self.volumeView.frame = CGRectMake(-10000, 0, 160, 44);
}

- (void)alb_relayoutAnimated:(BOOL)animated {
    [self.grid.collectionViewLayout invalidateLayout];
    CGRect target = [self alb_targetFrame];
    void (^apply)(void) = ^{ self.frame = target; [self layoutIfNeeded]; };
    if (animated) [UIView animateWithDuration:0.32 delay:0 usingSpringWithDamping:0.86 initialSpringVelocity:0 options:UIViewAnimationOptionBeginFromCurrentState animations:apply completion:nil];
    else apply();
    [self.grid reloadData];
}

#pragma mark 卡片区

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section { return (NSInteger)self.items.count; }

- (CGSize)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout *)layout sizeForItemAtIndexPath:(NSIndexPath *)indexPath {
    return [self alb_cardSize];
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath {
    ALBFloatCard *card = [collectionView dequeueReusableCellWithReuseIdentifier:@"card" forIndexPath:indexPath];
    NSUInteger i = self.items[(NSUInteger)indexPath.item].unsignedIntegerValue;
    if (self.mode == 1) {
        NSInteger kind = (NSInteger)i;
        card.action = kind;
        card.slider = NO;
        card.usable = YES;
        card.contentView.alpha = ALBDockHostQuickJumpReady(kind) ? 1 : 0.5;
        card.icon.image = [UIImage systemImageNamed:ALBDockHostQuickJumpSymbol(kind) withConfiguration:
            [UIImageSymbolConfiguration configurationWithPointSize:22 weight:UIImageSymbolWeightMedium]];
        card.title.text = ALBDockHostQuickJumpTitle(kind);
        card.accessibilityLabel = card.title.text;
        card.accessibilityTraits = UIAccessibilityTraitButton;
        card.fill.hidden = YES;
        card.level = 0;
        card.onLevel = nil;
        return card;
    }
    card.action = (NSInteger)i;
    card.slider = ALBPaletteIsSlider((NSInteger)i);
    card.usable = card.slider || self.model != nil;
    card.contentView.alpha = card.usable ? 1 : 0.42;
    NSString *title = i < self.texts.titles.count ? self.texts.titles[i] : @"";
    NSString *symbol = i < self.texts.symbols.count ? self.texts.symbols[i] : @"circle";
    NSString *role = [NSString stringWithFormat:@"palette.%lu", (unsigned long)i];
    UIImage *image = [UIImage systemImageNamed:AWEConfiguredSymbolName(role, symbol) withConfiguration:
        [UIImageSymbolConfiguration configurationWithPointSize:22 weight:UIImageSymbolWeightMedium]];
    if (i == 0 && self.texts.authorAvatar) {
        UIImage *avatar = self.texts.authorAvatar;
        image = [[[[UIGraphicsImageRenderer alloc] initWithSize:CGSizeMake(52, 52)] imageWithActions:^(__unused UIGraphicsImageRendererContext *c) {
            [[UIBezierPath bezierPathWithOvalInRect:CGRectMake(0, 0, 52, 52)] addClip];
            [avatar drawInRect:CGRectMake(0, 0, 52, 52)];
        }] imageWithRenderingMode:UIImageRenderingModeAlwaysOriginal];
    }
    card.icon.image = image;
    card.title.text = title;
    card.accessibilityLabel = title;
    card.fill.hidden = !card.slider;
    card.accessibilityTraits = card.slider ? UIAccessibilityTraitAdjustable : UIAccessibilityTraitButton;
    __weak typeof(self) weakSelf = self;
    if (i == 7) {
        card.level = AVAudioSession.sharedInstance.outputVolume;
        card.onLevel = ^(CGFloat level) { [weakSelf alb_setVolume:level]; };
    } else if (i == 8) {
        card.level = UIScreen.mainScreen.brightness;
        card.onLevel = ^(CGFloat level) { UIScreen.mainScreen.brightness = level; };
    } else {
        card.level = 0;
        card.onLevel = nil;
    }
    return card;
}

- (void)alb_setVolume:(CGFloat)level {
    UISlider *slider = self.systemVolume;
    if (!slider) {
        [self.volumeView layoutIfNeeded];
        NSMutableArray<UIView *> *queue = [NSMutableArray arrayWithArray:self.volumeView.subviews];
        for (NSUInteger i = 0; i < queue.count && i < 32; i++) {
            if ([queue[i] isKindOfClass:UISlider.class]) { slider = (UISlider *)queue[i]; break; }
            [queue addObjectsFromArray:queue[i].subviews];
        }
        self.systemVolume = slider;
    }
    if (slider) { [slider setValue:level animated:NO]; [slider sendActionsForControlEvents:UIControlEventValueChanged]; }
}

- (BOOL)collectionView:(UICollectionView *)collectionView shouldHighlightItemAtIndexPath:(NSIndexPath *)indexPath {
    if (self.mode == 1) return YES;
    NSUInteger i = self.items[(NSUInteger)indexPath.item].unsignedIntegerValue;
    return !ALBPaletteIsSlider((NSInteger)i);
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath {
    [collectionView deselectItemAtIndexPath:indexPath animated:NO];
    NSInteger i = self.items[(NSUInteger)indexPath.item].integerValue;
    if (self.mode == 1) {
        gALBJumpPanelPicks++;
        [self dismiss];
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.2 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{ ALBDockHostQuickJump(i); });
        return;
    }
    if (ALBPaletteIsSlider(i)) return;
    if (!self.model) { AWEShowDownloadToast(@"这一页没有视频，回到视频上再用"); return; }
    UIView *stack = self.stack;
    id model = ALB8PaletteModelForStack(stack);
    if (!stack.window || !model || ![AWEAwemeIdentity(model) isEqualToString:AWEAwemeIdentity(self.model)]) {
        [self dismiss];
        AWEShowDownloadToast(@"作品已经切换，请重新打开面板");
        return;
    }
    AWEPlayInteractionViewController *controller = ALB8PaletteControllerOwnsStack(self.controller, stack, model) ? self.controller : nil;
    gALBFloatRuns++;
    gALBFloatLast = [NSString stringWithFormat:@"%@（%ld）", self.texts.titles[(NSUInteger)i], (long)i];
    [self dismiss];
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.12 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        ALBPaletteRun(i, stack, model, controller, @"悬浮面板");
    });
}

- (BOOL)collectionView:(UICollectionView *)collectionView canMoveItemAtIndexPath:(NSIndexPath *)indexPath { return self.mode == 0; }

- (void)collectionView:(UICollectionView *)collectionView moveItemAtIndexPath:(NSIndexPath *)source toIndexPath:(NSIndexPath *)destination {
    NSNumber *moved = self.items[(NSUInteger)source.item];
    [self.items removeObjectAtIndex:(NSUInteger)source.item];
    [self.items insertObject:moved atIndex:(NSUInteger)destination.item];
    // 只排看得见的这几项：在完整顺序里，把看得见的位置按新顺序重新填一遍
    NSMutableArray<NSNumber *> *full = [ALBFloatOrder() mutableCopy];
    NSSet *visible = [NSSet setWithArray:self.items];
    NSUInteger next = 0;
    for (NSUInteger k = 0; k < full.count; k++) {
        if ([visible containsObject:full[k]] && next < self.items.count) full[k] = self.items[next++];
    }
    [NSUserDefaults.standardUserDefaults setObject:full forKey:ALB5Key(@"float.order")];
    gALBFloatMoves++;
}

- (void)alb_moved:(UILongPressGestureRecognizer *)press {
    if (self.mode != 0) return;
    UICollectionView *grid = self.grid;
    CGPoint p = [press locationInView:grid];
    switch (press.state) {
        case UIGestureRecognizerStateBegan: {
            NSIndexPath *path = [grid indexPathForItemAtPoint:p];
            if (!path || ![grid beginInteractiveMovementForItemAtIndexPath:path]) return;
            [[UIImpactFeedbackGenerator.alloc initWithStyle:UIImpactFeedbackStyleMedium] impactOccurred];
            UICollectionViewCell *cell = [grid cellForItemAtIndexPath:path];
            [UIView animateWithDuration:0.2 animations:^{ cell.transform = CGAffineTransformMakeScale(1.06, 1.06); }];
            break;
        }
        case UIGestureRecognizerStateChanged: [grid updateInteractiveMovementTargetPosition:p]; break;
        case UIGestureRecognizerStateEnded: [grid endInteractiveMovement]; break;
        default: [grid cancelInteractiveMovement]; break;
    }
    if (press.state == UIGestureRecognizerStateEnded || press.state == UIGestureRecognizerStateCancelled) {
        for (UICollectionViewCell *cell in grid.visibleCells) cell.transform = CGAffineTransformIdentity;
    }
}

#pragma mark 出现 / 关

- (void)alb_presentInWindow:(UIWindow *)window {
    if (self.mode == 1) gALBJumpPanelOpens++; else gALBFloatOpens++;
    [window addSubview:self];
    [self alb_loadContent];
    self.frame = [self alb_targetFrame];
    [self layoutIfNeeded];
    // 从 ✦ 的位置长出来
    CGPoint from = CGPointMake(CGRectGetMidX(self.anchor), CGRectGetMidY(self.anchor));
    CGPoint center = self.center;
    CGFloat sx = MAX(0.08, CGRectGetWidth(self.anchor) / MAX(1, self.bounds.size.width));
    self.transform = CGAffineTransformConcat(CGAffineTransformMakeScale(sx, sx),
        CGAffineTransformMakeTranslation(from.x - center.x, from.y - center.y));
    self.alpha = 0;
    [UIView animateWithDuration:UIAccessibilityIsReduceMotionEnabled() ? 0 : 0.42 delay:0 usingSpringWithDamping:0.84 initialSpringVelocity:0
                        options:UIViewAnimationOptionAllowUserInteraction animations:^{
        self.transform = CGAffineTransformIdentity;
        self.alpha = 1;
    } completion:nil];
    self.glass.overrideUserInterfaceStyle = UIUserInterfaceStyleDark;   // 3.11.47：白字，玻璃按深色来
    [self alb_applyBackground];
    if (!self.glass.effect) {
        if (@available(iOS 26.0, *)) {
            UIGlassEffect *effect = [UIGlassEffect effectWithStyle:UIGlassEffectStyleClear];
            effect.interactive = NO;
            CGFloat tint = ALBFloatTint();
            effect.tintColor = tint > 0.004 ? [UIColor colorWithWhite:0 alpha:tint] : nil;
            self.glass.effect = effect;
        } else {
            self.glass.effect = [UIBlurEffect effectWithStyle:UIBlurEffectStyleSystemUltraThinMaterialDark];
        }
    }
    __weak typeof(self) weakSelf = self;
    ALBOutsidePress *press = [ALBOutsidePress installOnWindow:window];
    press.insideControl = ^BOOL(UITouch *touch) {
        ALBFloatPanel *panel = weakSelf;
        if (!panel || panel.dismissing) return YES;
        if (panel.picking) return YES;
        UIView *v = touch.view;
        if ([v isDescendantOfView:panel] || (panel.source && [v isDescendantOfView:panel.source])) return YES;
        // 系统菜单 / 相册（不在面板里）：点上面不算外面
        NSUInteger steps = 0;
        for (UIView *up = v; up && steps < 40; up = up.superview, steps++) {
            NSString *name = NSStringFromClass(up.class);
            if ([name containsString:@"ContextMenu"] || [name containsString:@"MorphingPlatter"] || [name containsString:@"_UIMenu"]) return YES;
        }
        return NO;
    };
    press.onClose = ^{ [weakSelf dismiss]; };
    self.press = press;
    [NSNotificationCenter.defaultCenter addObserver:self selector:@selector(alb_levelsChanged) name:UIScreenBrightnessDidChangeNotification object:nil];
}

- (void)alb_levelsChanged {
    for (ALBFloatCard *card in self.grid.visibleCells) {
        if (card.action == 8 && card.slide.state == UIGestureRecognizerStatePossible) card.level = UIScreen.mainScreen.brightness;
    }
}

- (void)dismiss {
    if (self.dismissing) return;
    self.dismissing = YES;
    if (gALBFloatPanel == self) gALBFloatPanel = nil;
    [self.press retire];
    self.press = nil;
    [NSNotificationCenter.defaultCenter removeObserver:self];
    self.userInteractionEnabled = NO;
    CGPoint from = CGPointMake(CGRectGetMidX(self.anchor), CGRectGetMidY(self.anchor));
    CGPoint center = self.center;
    CGFloat sx = MAX(0.08, CGRectGetWidth(self.anchor) / MAX(1, self.bounds.size.width));
    [UIView animateWithDuration:UIAccessibilityIsReduceMotionEnabled() ? 0 : 0.24 delay:0 options:UIViewAnimationOptionCurveEaseIn animations:^{
        self.transform = CGAffineTransformConcat(CGAffineTransformMakeScale(sx, sx),
            CGAffineTransformMakeTranslation(from.x - center.x, from.y - center.y));
        self.alpha = 0.012;
    } completion:^(__unused BOOL finished) { [self removeFromSuperview]; }];
}

@end

/* ✦（mode 0）／↑（mode 1）：打开；同一种已开着就关，另一种开着就换。不在视频上也能开，只是视频的动作暂时用不了 */
static BOOL ALBFloatPanelOpen(UIView *source, NSInteger mode) {
    ALBFloatPanel *open = gALBFloatPanel;
    if (open.window && !open.dismissing) {
        [open dismiss];
        if (open.mode == mode) return YES;
    }
    UIWindow *window = source.window ?: AWEActiveMainWindow();
    if (!window) return NO;
    UIWindow *main = AWEActiveMainWindow();
    UIView *stack = ALBFindVisibleActionStack(main.rootViewController.view);
    id model = stack ? ALB8PaletteModelForStack(stack) : nil;
    Class interaction = NSClassFromString(@"AWEPlayInteractionViewController");
    AWEPlayInteractionViewController *controller = nil;
    for (UIResponder *r = stack; r && interaction; r = r.nextResponder) {
        if ([r isKindOfClass:interaction]) { controller = (AWEPlayInteractionViewController *)r; break; }
    }
    ALBFloatPanel *panel = [[ALBFloatPanel alloc] initWithFrame:CGRectZero];
    panel.mode = mode;
    panel.stack = stack;
    panel.model = model;
    panel.controller = controller;
    panel.source = source;
    CGFloat gap = MIN(MAX([NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"float.gap")] ?
        [NSUserDefaults.standardUserDefaults doubleForKey:ALB5Key(@"float.gap")] : 14.0, 0), 60);
    if (source.window) {
        panel.anchor = [source convertRect:source.bounds toView:window];
        UIView *group = source.superview ?: source;
        panel.bottomLimit = CGRectGetMinY([group convertRect:group.bounds toView:window]) - gap;
    } else {
        CGRect wb = window.bounds;
        panel.anchor = CGRectMake(CGRectGetMidX(wb) - 20, CGRectGetMaxY(wb) - 120, 40, 40);
        panel.bottomLimit = CGRectGetMaxY(wb) - window.safeAreaInsets.bottom - 80 - gap;
    }
    gALBFloatPanel = panel;
    [panel alb_presentInWindow:window];
    return YES;
}

/* 3.11.47：插件设置整页（大尺寸的页面，不是半屏；同抖音设置里那个入口用的控制器） */
static void ALBFloatOpenSettingsFull(UIWindow *window) {
    if (gAWESettingsVisible) return;
    window = window ?: AWEActiveMainWindow();
    UIViewController *top = AWETopViewControllerFrom(window.rootViewController);
    if (!top) return;
    if ([top isKindOfClass:[AWELiquidSettingsController class]] ||
        [top.navigationController.topViewController isKindOfClass:[AWELiquidSettingsController class]]) return;
    AWELiquidSettingsController *settings = [AWELiquidSettingsController new];
    UINavigationController *navigation = [[ALBSettingsNativeNavigationController alloc] initWithRootViewController:settings];
    navigation.modalPresentationStyle = UIModalPresentationPageSheet;
    navigation.overrideUserInterfaceStyle = ALBMaterialStyle(navigation.view, NO);
    if (@available(iOS 15.0, *)) {
        UISheetPresentationController *sheet = navigation.sheetPresentationController;
        sheet.detents = @[UISheetPresentationControllerDetent.largeDetent];
        sheet.selectedDetentIdentifier = UISheetPresentationControllerDetentIdentifierLarge;
        sheet.prefersGrabberVisible = YES;
        if ([navigation isKindOfClass:ALBSettingsNativeNavigationController.class])
            sheet.delegate = (ALBSettingsNativeNavigationController *)navigation;
    }
    ALBInstallSettingsGlass(navigation.topViewController);
    [top presentViewController:navigation animated:YES completion:nil];
}

static NSString *ALBFloatPanelReport(void) {
    return [NSString stringWithFormat:@"打开 %lu 次、点卡片 %lu 次（最近 %@）、拖动排序 %lu 次、滑动调节 %lu 次；每行 %ld 个；壁纸=%@；离底栏 %.0f；碰外面关 %lu 次（其中滑动 %lu 次，最近一次按在 %@）；"
        "深色色调 %.0f%%、背景透明度 %.0f%%；选壁纸 点了 %lu 次、相册弹出 %lu 次、选回 %lu 次、写入 %lu 次（最近：%@）；齿轮打开设置 %lu 次；↑ 快捷跳转面板 打开 %lu 次、点了 %lu 次",
        (unsigned long)gALBFloatOpens, (unsigned long)gALBFloatRuns, gALBFloatLast ?: @"-", (unsigned long)gALBFloatMoves,
        (unsigned long)gALBFloatSlides, (long)ALBFloatColumns(),
        [NSFileManager.defaultManager fileExistsAtPath:ALBFloatWallpaperPath()] ? @"有" : @"没有（液态玻璃）",
        [NSUserDefaults.standardUserDefaults objectForKey:ALB5Key(@"float.gap")] ? [NSUserDefaults.standardUserDefaults doubleForKey:ALB5Key(@"float.gap")] : 14.0,
        (unsigned long)gALBOutsidePressCloses, (unsigned long)gALBOutsidePressSwipes, gALBOutsidePressLastHit ?: @"-",
        ALBFloatTint() * 100, ALBFloatClarity() * 100, (unsigned long)gALBFloatPickTaps, (unsigned long)gALBFloatPickShown,
        (unsigned long)gALBFloatPickResults, (unsigned long)gALBFloatPickWrites, gALBFloatPickLast ?: @"-", (unsigned long)gALBFloatSettingsOpens,
        (unsigned long)gALBJumpPanelOpens, (unsigned long)gALBJumpPanelPicks];
}
