/* 视频（最高画质）与实况的保存。

   · 选址：视频在 bitrateModels 里去掉 bytevc2（相册解不了）后按分辨率、码率从高到低，
     再接 h264URL、playURL 兜底；每个候选里的镜像地址逐个重试，带水印参数的地址不用。
   · 图集里的一项：clipType 4 带视频＝视频；带 clipVideo 又有图＝实况（图＋视频配成一对）；否则是图片。
     非图集作品是动图视频（animatedImageVideoInfo）＝实况（封面＋视频）。
   · 实况配对：照片写进 Apple MakerNote 的 17 号键、视频写 QuickTime 的 content identifier 和
     still-image-time，同一个 UUID，相册才认成实况。
   · 视频没有音轨时补背景音乐（直通合成，不重新编码）。
   · 图片（实况的封面、图集里的图）先用插件自己的那个地址（AWEImageURLForModel，画质正常、不占空间），取不到再用作品自带的；
     WebP 转 JPEG。
   · 下载、取消、提示用插件自己那套（ALBDownloadJob、AWEShowDownloadToastForJob）：全部下载、配对完再一次写进相册，
     中途取消不会留下半套。
   Included by Tweak.x right after AWECopyCompleteTextForAweme (uses its static helpers). */

#import <AVFoundation/AVFoundation.h>
#import <ImageIO/ImageIO.h>
#import <CoreMedia/CoreMedia.h>

typedef NS_ENUM(NSInteger, ALBMSKind) { ALBMSKindVideo = 0, ALBMSKindImage = 1, ALBMSKindLive = 2 };
typedef NS_ENUM(NSInteger, ALBMSWhat) {
    ALBMSWhatMain = 0,          // 非图集：视频（最高画质）或实况
    ALBMSWhatCurrentLive = 1,   // 图集：当前这一项（实况 / 视频 / 图片，按它是什么存）
    ALBMSWhatAllLive = 2,       // 图集：所有实况
    ALBMSWhatAllTypes = 3,      // 图集：所有项，图片、实况、视频都存
};

@interface ALBMSCandidate : NSObject
@property (nonatomic, copy) NSArray<NSString *> *urls;
@property (nonatomic, copy) NSString *tag;
@end
@implementation ALBMSCandidate
@end
static ALBMSCandidate *ALBMSMakeCandidate(NSArray<NSString *> *urls, NSString *tag) {
    ALBMSCandidate *c = [ALBMSCandidate new];
    c.urls = urls ?: @[];
    c.tag = tag ?: @"";
    return c;
}

@interface ALBMSItem : NSObject
@property (nonatomic, assign) ALBMSKind kind;
@property (nonatomic, copy) NSArray<ALBMSCandidate *> *primary;   // 视频 / 图片 / 实况的照片
@property (nonatomic, copy) NSArray<ALBMSCandidate *> *paired;    // 实况的视频
@property (nonatomic, copy) NSArray<ALBMSCandidate *> *music;     // 视频缺音轨时补的背景音乐
@end
@implementation ALBMSItem
@end
static ALBMSItem *ALBMSMakeItem(ALBMSKind kind, NSArray<ALBMSCandidate *> *primary) {
    ALBMSItem *item = [ALBMSItem new];
    item.kind = kind;
    item.primary = primary ?: @[];
    item.paired = @[];
    item.music = @[];
    return item;
}
static NSString *ALBMSKindName(ALBMSKind kind) {
    return kind == ALBMSKindVideo ? @"视频" : (kind == ALBMSKindLive ? @"实况" : @"图片");
}

/* 诊断：最近 10 次保存（ALB5Settings.h【保存】） */
static NSMutableArray<NSString *> *gALBMSLog;
static NSUInteger gALBMSRuns, gALBMSSaved, gALBMSFailed;
static void ALBMSNote(NSString *line) {
    if (!line.length) return;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!gALBMSLog) gALBMSLog = [NSMutableArray array];
        [gALBMSLog addObject:line];
        while (gALBMSLog.count > 10) [gALBMSLog removeObjectAtIndex:0];
        AWEInteractionDiagnosticWrite(@"MEDIA_SAVE %@", line);
    });
}

#pragma mark - 选址

static id ALBMSValue(id object, NSString *key) {
    if (!object || !key.length) return nil;
    @try {
        id value = [object valueForKey:key];
        return value == NSNull.null ? nil : value;
    } @catch (__unused NSException *exception) { return nil; }
}
static NSArray<NSString *> *ALBMSStrings(id value) {
    if (![value isKindOfClass:NSArray.class]) return @[];
    NSMutableArray<NSString *> *out = [NSMutableArray array];
    for (id item in value) if ([item isKindOfClass:NSString.class] && [item length]) [out addObject:item];
    return out;
}
static NSDictionary *ALBMSURLModel(id model) {
    if (!model) return nil;
    NSArray<NSString *> *urls = ALBMSStrings(ALBMSValue(model, @"originURLList"));
    if (!urls.count) return nil;
    NSMutableDictionary *dict = [@{@"urls": urls} mutableCopy];
    id key = ALBMSValue(model, @"URLKey");
    if ([key isKindOfClass:NSString.class]) dict[@"key"] = key;
    id width = ALBMSValue(model, @"imageWidth"), height = ALBMSValue(model, @"imageHeight");
    if ([width isKindOfClass:NSNumber.class]) dict[@"w"] = width;
    if ([height isKindOfClass:NSNumber.class]) dict[@"h"] = height;
    return dict;
}
static NSDictionary *ALBMSVideo(id video) {
    if (!video) return nil;
    NSMutableDictionary *dict = [NSMutableDictionary dictionary];
    NSMutableArray *gears = [NSMutableArray array];
    gALBHQBypass++;   // 3.11.52：默认最高画质只留一档，这里要全部档位
    id models = ALBMSValue(video, @"bitrateModels");
    gALBHQBypass--;
    for (id model in [models isKindOfClass:NSArray.class] ? models : @[]) {
        NSDictionary *addr = ALBMSURLModel(ALBMSValue(model, @"playAddr"));
        if (!addr) continue;
        NSMutableDictionary *gear = [@{@"addr": addr} mutableCopy];
        id codec = ALBMSValue(model, @"isH265"), bitrate = ALBMSValue(model, @"bitrate"), name = ALBMSValue(model, @"gearName");
        if (codec) gear[@"codec"] = codec;
        if (bitrate) gear[@"bitrate"] = bitrate;
        if ([name isKindOfClass:NSString.class]) gear[@"gear"] = name;
        [gears addObject:gear];
    }
    dict[@"bitrates"] = gears;
    NSDictionary *h264 = ALBMSURLModel(ALBMSValue(video, @"h264URL"));
    NSDictionary *play = ALBMSURLModel(ALBMSValue(video, @"playURL"));
    NSDictionary *cover = ALBMSURLModel(ALBMSValue(video, @"coverURL"));
    if (h264) dict[@"h264"] = h264;
    if (play) dict[@"play"] = play;
    if (cover) dict[@"cover"] = cover;
    return dict;
}

#pragma mark - 选址：排序

static long long ALBMSInteger(id value) { return [value respondsToSelector:@selector(longLongValue)] ? [value longLongValue] : 0; }
static NSArray<NSString *> *ALBMSCleanURLs(NSArray *urls) {
    NSMutableArray<NSString *> *out = [NSMutableArray array];
    for (id value in [urls isKindOfClass:NSArray.class] ? urls : @[]) {
        if (![value isKindOfClass:NSString.class]) continue;
        NSString *url = value;
        if (!url.length || [url containsString:@"watermark=1"] || [out containsObject:url]) continue;
        [out addObject:url];
    }
    return out;
}
static NSString *ALBMSCodecOfKey(NSString *key) {
    if (![key isKindOfClass:NSString.class]) return nil;
    for (NSString *codec in @[@"bytevc2", @"bytevc1", @"h264"])
        if ([key containsString:[NSString stringWithFormat:@"_%@_", codec]]) return codec;
    return nil;
}
static NSString *ALBMSPathOf(NSString *url) {
    NSString *path = [NSURL URLWithString:url].path ?: url;
    return path.lowercaseString;
}
static BOOL ALBMSImageURLUsable(NSString *url) {
    if ([ALBMSPathOf(url) hasSuffix:@".image"]) return NO;
    return ![url containsString:@"dy-water"] && ![url containsString:@"-download-webp"];
}
static NSInteger ALBMSImageRank(NSString *url) {
    NSString *ext = ALBMSPathOf(url).pathExtension;
    if ([ext isEqualToString:@"jpeg"] || [ext isEqualToString:@"jpg"]) return 0;
    if ([ext isEqualToString:@"heic"] || [ext isEqualToString:@"heif"] || [ext isEqualToString:@"png"]) return 1;
    if ([ext isEqualToString:@"webp"]) return 2;
    return 3;
}
static NSArray<NSString *> *ALBMSSortedImageURLs(NSArray *urls) {
    NSMutableArray<NSString *> *usable = [NSMutableArray array];
    for (NSString *url in ALBMSCleanURLs(urls)) if (ALBMSImageURLUsable(url)) [usable addObject:url];
    NSMutableArray<NSString *> *sorted = [NSMutableArray array];
    for (NSInteger rank = 0; rank <= 3; rank++)
        for (NSString *url in usable) if (ALBMSImageRank(url) == rank) [sorted addObject:url];
    return sorted;
}
/* 插件自己的那个地址排第一（画质正常、不占空间），再接无水印列表和普通列表 */
static NSArray<ALBMSCandidate *> *ALBMSImageCandidates(NSString *ours, NSArray *urls, NSArray *free) {
    NSMutableArray<ALBMSCandidate *> *out = [NSMutableArray array];
    if (ours.length) [out addObject:ALBMSMakeCandidate(@[ours], @"插件原图")];
    NSArray<NSString *> *freeURLs = ALBMSSortedImageURLs(free);
    if (freeURLs.count) [out addObject:ALBMSMakeCandidate(freeURLs, @"watermark_free")];
    NSArray<NSString *> *listURLs = ALBMSSortedImageURLs(urls);
    if (listURLs.count)
        [out addObject:ALBMSMakeCandidate(listURLs, [NSString stringWithFormat:@"url_list.%@", ALBMSPathOf(listURLs.firstObject).pathExtension])];
    return out;
}
static NSArray<ALBMSCandidate *> *ALBMSPlainCandidates(NSArray *urls, NSString *tag) {
    NSArray<NSString *> *clean = ALBMSCleanURLs(urls);
    return clean.count ? @[ALBMSMakeCandidate(clean, tag)] : @[];
}
static long long ALBMSAddrArea(NSDictionary *addr, id gear) {
    long long area = ALBMSInteger(addr[@"w"]) * ALBMSInteger(addr[@"h"]);
    if (area > 0) return area;
    long long side = ALBMSInteger(gear);
    return side * side;
}
/* is_h265：0 H.264，1 bytevc1（标准 HEVC），2 bytevc2（相册解不了，不要）；分辨率、码率从高到低 */
static NSArray<ALBMSCandidate *> *ALBMSVideoCandidates(NSDictionary *video) {
    NSMutableArray<ALBMSCandidate *> *out = [NSMutableArray array];
    NSMutableSet<NSString *> *seen = [NSMutableSet set];
    void (^add)(NSDictionary *, NSString *) = ^(NSDictionary *addr, NSString *tag) {
        NSArray<NSString *> *urls = ALBMSCleanURLs(addr[@"urls"]);
        if (!urls.count || [seen containsObject:urls.firstObject]) return;
        [seen addObject:urls.firstObject];
        [out addObject:ALBMSMakeCandidate(urls, tag)];
    };
    NSMutableArray<NSDictionary *> *gears = [NSMutableArray array];
    for (id value in [video[@"bitrates"] isKindOfClass:NSArray.class] ? video[@"bitrates"] : @[]) {
        if (![value isKindOfClass:NSDictionary.class]) continue;
        NSDictionary *gear = value;
        NSDictionary *addr = [gear[@"addr"] isKindOfClass:NSDictionary.class] ? gear[@"addr"] : nil;
        long long codec = ALBMSInteger(gear[@"codec"]);
        if (!addr || codec < 0 || codec > 1) continue;
        if ([ALBMSCodecOfKey(addr[@"key"]) isEqualToString:@"bytevc2"]) continue;
        [gears addObject:gear];
    }
    [gears sortWithOptions:NSSortStable usingComparator:^NSComparisonResult(NSDictionary *a, NSDictionary *b) {
        long long areaA = ALBMSAddrArea(a[@"addr"], a[@"gear"]), areaB = ALBMSAddrArea(b[@"addr"], b[@"gear"]);
        if (areaA != areaB) return areaA > areaB ? NSOrderedAscending : NSOrderedDescending;
        long long rateA = ALBMSInteger(a[@"bitrate"]), rateB = ALBMSInteger(b[@"bitrate"]);
        if (rateA != rateB) return rateA > rateB ? NSOrderedAscending : NSOrderedDescending;
        return NSOrderedSame;
    }];
    for (NSDictionary *gear in gears) {
        NSDictionary *addr = gear[@"addr"];
        add(addr, [NSString stringWithFormat:@"%@ %lldx%lld %@", ALBMSInteger(gear[@"codec"]) == 1 ? @"bytevc1" : @"h264",
                   ALBMSInteger(addr[@"w"]), ALBMSInteger(addr[@"h"]), [gear[@"gear"] isKindOfClass:NSString.class] ? gear[@"gear"] : @""]);
    }
    NSDictionary *h264 = [video[@"h264"] isKindOfClass:NSDictionary.class] ? video[@"h264"] : nil;
    if (h264) add(h264, [NSString stringWithFormat:@"play_addr_h264 %lldx%lld", ALBMSInteger(h264[@"w"]), ALBMSInteger(h264[@"h"])]);
    NSDictionary *play = [video[@"play"] isKindOfClass:NSDictionary.class] ? video[@"play"] : nil;
    NSString *playCodec = ALBMSCodecOfKey(play[@"key"]);
    if (play && ![playCodec isEqualToString:@"bytevc2"])
        add(play, [NSString stringWithFormat:@"play_addr %@ %lldx%lld", playCodec ?: @"unknown", ALBMSInteger(play[@"w"]), ALBMSInteger(play[@"h"])]);
    return out;
}

/* 图集里的一项 */
static ALBMSItem *ALBMSAlbumEntry(id image) {
    NSArray<ALBMSCandidate *> *images = ALBMSImageCandidates(AWEImageURLForModel(image),
        ALBMSStrings(ALBMSValue(image, @"urlList")), ALBMSStrings(ALBMSValue(image, @"watermarkFreeUrlList")));
    NSDictionary *clip = ALBMSVideo(ALBMSValue(image, @"clipVideo"));
    NSArray<ALBMSCandidate *> *clipVideo = clip ? ALBMSVideoCandidates(clip) : @[];
    if (clipVideo.count && ALBMSInteger(ALBMSValue(image, @"type")) == 4) return ALBMSMakeItem(ALBMSKindVideo, clipVideo);
    if (clipVideo.count && images.count) {
        ALBMSItem *item = ALBMSMakeItem(ALBMSKindLive, images);
        item.paired = clipVideo;
        return item;
    }
    return images.count ? ALBMSMakeItem(ALBMSKindImage, images) : nil;
}
static NSUInteger ALBMSAlbumCurrentIndex(id aweme, NSUInteger count) {
    if (!count) return 0;
    long long current = ALBMSInteger(ALBMSValue(aweme, @"currentImageIndex"));
    long long index = current > 0 ? current - 1 : 0;      // currentImageIndex 从 1 开始；没看过的作品是 0
    return (NSUInteger)MIN(MAX(index, 0LL), (long long)count - 1);
}
/* 非图集：视频（带补音乐），动图视频＝实况 */
static ALBMSItem *ALBMSMainItem(id aweme) {
    if (AWEAlbumImagesForAweme(aweme).count) return nil;
    NSDictionary *video = ALBMSVideo(ALBMSValue(aweme, @"video"));
    NSArray<ALBMSCandidate *> *videos = video ? ALBMSVideoCandidates(video) : @[];
    if (!videos.count) return nil;
    if (ALBMSValue(aweme, @"animatedImageVideoInfo") != nil) {
        NSDictionary *cover = [video[@"cover"] isKindOfClass:NSDictionary.class] ? video[@"cover"] : nil;
        NSArray<ALBMSCandidate *> *coverCandidates = ALBMSImageCandidates(nil, cover[@"urls"], nil);
        if (coverCandidates.count) {
            ALBMSItem *item = ALBMSMakeItem(ALBMSKindLive, coverCandidates);
            item.paired = videos;
            return item;
        }
    }
    ALBMSItem *item = ALBMSMakeItem(ALBMSKindVideo, videos);
    NSDictionary *music = ALBMSURLModel(ALBMSValue(ALBMSValue(aweme, @"music"), @"playURL"));
    item.music = ALBMSPlainCandidates(music[@"urls"], @"music");
    return item;
}
static NSArray<ALBMSItem *> *ALBMSItemsFor(id aweme, ALBMSWhat what) {
    if (!aweme) return @[];
    if (what == ALBMSWhatMain) {
        ALBMSItem *item = ALBMSMainItem(aweme);
        return item ? @[item] : @[];
    }
    NSArray *images = AWEAlbumImagesForAweme(aweme);
    if (!images.count) return @[];
    NSMutableArray<ALBMSItem *> *out = [NSMutableArray array];
    if (what == ALBMSWhatCurrentLive) {
        ALBMSItem *item = ALBMSAlbumEntry(images[ALBMSAlbumCurrentIndex(aweme, images.count)]);
        if (item) [out addObject:item];
        return out;
    }
    for (id image in images) {
        ALBMSItem *item = ALBMSAlbumEntry(image);
        if (!item) continue;
        if (what == ALBMSWhatAllLive && item.kind != ALBMSKindLive) continue;
        [out addObject:item];
    }
    return out;
}
/* 菜单 / 面板标题用 */
static NSUInteger ALBMSAlbumLiveCount(id aweme) {
    NSUInteger n = 0;
    for (id image in AWEAlbumImagesForAweme(aweme)) {
        ALBMSItem *item = ALBMSAlbumEntry(image);
        if (item.kind == ALBMSKindLive) n++;
    }
    return n;
}
static ALBMSKind ALBMSAlbumCurrentKind(id aweme) {
    NSArray *images = AWEAlbumImagesForAweme(aweme);
    if (!images.count) return ALBMSKindImage;
    ALBMSItem *item = ALBMSAlbumEntry(images[ALBMSAlbumCurrentIndex(aweme, images.count)]);
    return item ? item.kind : ALBMSKindImage;
}
static BOOL ALBMSMainIsLive(id aweme) { return ALBMSMainItem(aweme).kind == ALBMSKindLive; }

#pragma mark - 文件格式

typedef NS_ENUM(NSInteger, ALBMSFileType) {
    ALBMSFileUnknown, ALBMSFileJPEG, ALBMSFilePNG, ALBMSFileGIF, ALBMSFileWebP, ALBMSFileHEIF, ALBMSFileVVIC, ALBMSFileVideo,
};
static NSString *ALBMSBrand(const uint8_t *bytes) {
    return [[NSString alloc] initWithBytes:bytes length:4 encoding:NSASCIIStringEncoding] ?: @"";
}
static ALBMSFileType ALBMSSniff(NSURL *file) {
    NSFileHandle *handle = [NSFileHandle fileHandleForReadingFromURL:file error:nil];
    NSData *head = [handle readDataOfLength:256];
    [handle closeFile];
    if (head.length < 12) return ALBMSFileUnknown;
    const uint8_t *b = head.bytes;
    if (b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF) return ALBMSFileJPEG;
    if (b[0] == 0x89 && b[1] == 'P' && b[2] == 'N' && b[3] == 'G') return ALBMSFilePNG;
    if (b[0] == 'G' && b[1] == 'I' && b[2] == 'F') return ALBMSFileGIF;
    if (!memcmp(b, "RIFF", 4) && !memcmp(b + 8, "WEBP", 4)) return ALBMSFileWebP;
    if (memcmp(b + 4, "ftyp", 4)) return ALBMSFileUnknown;
    uint32_t boxSize = ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
    NSUInteger end = MIN((NSUInteger)boxSize, head.length);
    NSMutableArray<NSString *> *brands = [NSMutableArray arrayWithObject:ALBMSBrand(b + 8)];
    for (NSUInteger offset = 16; offset + 4 <= end; offset += 4) [brands addObject:ALBMSBrand(b + offset)];
    if ([brands containsObject:@"vvic"]) return ALBMSFileVVIC;
    NSSet *heif = [NSSet setWithArray:@[@"heic", @"heix", @"hevc", @"hevx", @"heim", @"heis", @"mif1", @"msf1", @"avif"]];
    NSSet *video = [NSSet setWithArray:@[@"isom", @"iso2", @"iso4", @"iso5", @"iso6", @"mp41", @"mp42", @"avc1", @"qt  ", @"M4V ", @"dash", @"3gp4", @"3gp5"]];
    if ([heif containsObject:brands.firstObject]) return ALBMSFileHEIF;
    if ([video containsObject:brands.firstObject]) return ALBMSFileVideo;
    for (NSString *brand in brands) {
        if ([heif containsObject:brand]) return ALBMSFileHEIF;
        if ([video containsObject:brand]) return ALBMSFileVideo;
    }
    return ALBMSFileUnknown;
}
static NSString *ALBMSFileTypeName(ALBMSFileType type) {
    switch (type) {
        case ALBMSFileJPEG: return @"jpeg";
        case ALBMSFilePNG: return @"png";
        case ALBMSFileGIF: return @"gif";
        case ALBMSFileWebP: return @"webp";
        case ALBMSFileHEIF: return @"heif";
        case ALBMSFileVVIC: return @"vvic";
        case ALBMSFileVideo: return @"video";
        case ALBMSFileUnknown: break;
    }
    return @"unknown";
}
static BOOL ALBMSIsImageType(ALBMSFileType type) {
    return type == ALBMSFileJPEG || type == ALBMSFilePNG || type == ALBMSFileGIF || type == ALBMSFileWebP || type == ALBMSFileHEIF;
}
/* 按文件头给扩展名（AVFoundation 与相册按扩展名认本地文件的容器） */
static NSString *ALBMSFileExtension(NSURL *file) {
    switch (ALBMSSniff(file)) {
        case ALBMSFileJPEG: return @"jpg";
        case ALBMSFilePNG: return @"png";
        case ALBMSFileGIF: return @"gif";
        case ALBMSFileWebP: return @"webp";
        case ALBMSFileHEIF: return @"heic";
        case ALBMSFileVVIC: return nil;
        default: break;
    }
    NSFileHandle *handle = [NSFileHandle fileHandleForReadingFromURL:file error:nil];
    NSData *head = [handle readDataOfLength:12];
    [handle closeFile];
    if (head.length < 12) return nil;
    const uint8_t *b = head.bytes;
    if (!memcmp(b + 4, "ftyp", 4)) {
        NSString *brand = ALBMSBrand(b + 8);
        if ([brand isEqualToString:@"M4A "] || [brand isEqualToString:@"M4B "]) return @"m4a";
        return [brand isEqualToString:@"qt  "] ? @"mov" : @"mp4";
    }
    if (!memcmp(b, "ID3", 3) || (b[0] == 0xFF && (b[1] & 0xE0) == 0xE0)) return @"mp3";
    return nil;
}
static NSString *ALBMSFourCC(FourCharCode code) {
    char chars[5] = { (char)(code >> 24), (char)(code >> 16), (char)(code >> 8), (char)code, 0 };
    return [NSString stringWithUTF8String:chars] ?: @"";
}
/* 视频轨是 avc1 / hvc1 / hev1 才能进相册 */
static BOOL ALBMSVideoPlayable(NSURL *file, NSString **codec) {
    AVURLAsset *asset = [AVURLAsset URLAssetWithURL:file options:nil];
    AVAssetTrack *track = [asset tracksWithMediaType:AVMediaTypeVideo].firstObject;
    id description = track.formatDescriptions.firstObject;
    if (!description) { if (codec) *codec = @"none"; return NO; }
    NSString *name = ALBMSFourCC(CMFormatDescriptionGetMediaSubType((__bridge CMFormatDescriptionRef)description));
    if (codec) *codec = name;
    return [@[@"avc1", @"hvc1", @"hev1"] containsObject:name];
}
static BOOL ALBMSVideoHasAudio(NSURL *file) {
    return [[AVURLAsset URLAssetWithURL:file options:nil] tracksWithMediaType:AVMediaTypeAudio].count > 0;
}
static NSString *ALBMSVideoCheck(NSURL *file) {
    ALBMSFileType type = ALBMSSniff(file);
    if (type != ALBMSFileVideo) return [NSString stringWithFormat:@"不是视频（%@）", ALBMSFileTypeName(type)];
    NSString *codec = nil;
    return ALBMSVideoPlayable(file, &codec) ? nil : [NSString stringWithFormat:@"编码 %@ 无法存入相册", codec];
}
static NSString *ALBMSImageCheck(NSURL *file) {
    ALBMSFileType type = ALBMSSniff(file);
    return ALBMSIsImageType(type) ? nil : [NSString stringWithFormat:@"格式 %@ 无法存入相册", ALBMSFileTypeName(type)];
}
/* WebP 转 JPEG（相册不收 WebP）；其余原样 */
static NSURL *ALBMSPhotoFile(NSURL *file, NSURL *directory, NSString **why) {
    ALBMSFileType type = ALBMSSniff(file);
    if (type != ALBMSFileWebP) return file;
    CGImageSourceRef source = CGImageSourceCreateWithURL((__bridge CFURLRef)file, NULL);
    CGImageRef image = source ? CGImageSourceCreateImageAtIndex(source, 0, NULL) : NULL;
    NSURL *output = [directory URLByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingPathExtension:@"jpg"]];
    CGImageDestinationRef destination = image ? CGImageDestinationCreateWithURL((__bridge CFURLRef)output, CFSTR("public.jpeg"), 1, NULL) : NULL;
    BOOL ok = NO;
    if (destination) {
        CGImageDestinationAddImage(destination, image, (__bridge CFDictionaryRef)@{(__bridge NSString *)kCGImageDestinationLossyCompressionQuality: @0.92});
        ok = CGImageDestinationFinalize(destination);
        CFRelease(destination);
    }
    if (image) CGImageRelease(image);
    if (source) CFRelease(source);
    if (!ok && why) *why = @"WebP 转 JPEG 失败";
    return ok ? output : nil;
}

#pragma mark - 实况配对

static NSString *const kALBMSStillImageTimeKey = @"com.apple.quicktime.still-image-time";
static BOOL ALBMSWritePairedPhoto(NSURL *image, NSURL *output, NSString *identifier) {
    CGImageSourceRef source = CGImageSourceCreateWithURL((__bridge CFURLRef)image, NULL);
    if (!source) return NO;
    NSMutableDictionary *props = [CFBridgingRelease(CGImageSourceCopyPropertiesAtIndex(source, 0, NULL)) mutableCopy] ?: [NSMutableDictionary dictionary];
    NSMutableDictionary *maker = [props[(__bridge NSString *)kCGImagePropertyMakerAppleDictionary] mutableCopy] ?: [NSMutableDictionary dictionary];
    maker[@"17"] = identifier;
    props[(__bridge NSString *)kCGImagePropertyMakerAppleDictionary] = maker;
    CGImageDestinationRef destination = CGImageDestinationCreateWithURL((__bridge CFURLRef)output, CFSTR("public.jpeg"), 1, NULL);
    BOOL ok = NO;
    if (destination) {
        CGImageDestinationAddImageFromSource(destination, source, 0, (__bridge CFDictionaryRef)props);
        ok = CGImageDestinationFinalize(destination);
        CFRelease(destination);
    }
    CFRelease(source);
    return ok;
}
static AVMetadataItem *ALBMSContentIdentifierItem(NSString *identifier) {
    AVMutableMetadataItem *item = [AVMutableMetadataItem metadataItem];
    item.keySpace = AVMetadataKeySpaceQuickTimeMetadata;
    item.key = AVMetadataQuickTimeMetadataKeyContentIdentifier;
    item.value = identifier;
    return item;
}
static AVAssetWriterInputMetadataAdaptor *ALBMSStillImageAdaptor(AVAssetWriter *writer) {
    NSArray *specs = @[@{
        (__bridge NSString *)kCMMetadataFormatDescriptionMetadataSpecificationKey_Identifier:
            [NSString stringWithFormat:@"mdta/%@", kALBMSStillImageTimeKey],
        (__bridge NSString *)kCMMetadataFormatDescriptionMetadataSpecificationKey_DataType:
            (__bridge NSString *)kCMMetadataBaseDataType_SInt8,
    }];
    CMFormatDescriptionRef description = NULL;
    CMMetadataFormatDescriptionCreateWithMetadataSpecifications(kCFAllocatorDefault, kCMMetadataFormatType_Boxed,
                                                                (__bridge CFArrayRef)specs, &description);
    if (!description) return nil;
    AVAssetWriterInput *input = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeMetadata outputSettings:nil sourceFormatHint:description];
    CFRelease(description);
    if (![writer canAddInput:input]) return nil;
    [writer addInput:input];
    return [AVAssetWriterInputMetadataAdaptor assetWriterInputMetadataAdaptorWithAssetWriterInput:input];
}
static void ALBMSWritePairedMovie(NSURL *video, NSURL *output, NSString *identifier, void (^done)(NSString *error)) {
    AVURLAsset *asset = [AVURLAsset URLAssetWithURL:video options:nil];
    NSError *error = nil;
    AVAssetReader *reader = [AVAssetReader assetReaderWithAsset:asset error:&error];
    AVAssetWriter *writer = reader ? [AVAssetWriter assetWriterWithURL:output fileType:AVFileTypeQuickTimeMovie error:&error] : nil;
    if (!reader || !writer) { done(error.localizedDescription ?: @"无法读取实况视频"); return; }
    writer.metadata = @[ALBMSContentIdentifierItem(identifier)];
    NSMutableArray<NSArray *> *pairs = [NSMutableArray array];
    BOOL hasVideo = NO;
    for (AVAssetTrack *track in asset.tracks) {
        if (![track.mediaType isEqualToString:AVMediaTypeVideo] && ![track.mediaType isEqualToString:AVMediaTypeAudio]) continue;
        AVAssetReaderTrackOutput *outputTrack = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:track outputSettings:nil];
        CMFormatDescriptionRef hint = (__bridge CMFormatDescriptionRef)track.formatDescriptions.firstObject;
        AVAssetWriterInput *input = [AVAssetWriterInput assetWriterInputWithMediaType:track.mediaType outputSettings:nil sourceFormatHint:hint];
        input.expectsMediaDataInRealTime = NO;
        if ([track.mediaType isEqualToString:AVMediaTypeVideo]) input.transform = track.preferredTransform;
        if (![reader canAddOutput:outputTrack] || ![writer canAddInput:input]) continue;
        [reader addOutput:outputTrack];
        [writer addInput:input];
        [pairs addObject:@[outputTrack, input]];
        hasVideo = hasVideo || [track.mediaType isEqualToString:AVMediaTypeVideo];
    }
    if (!hasVideo) { done(@"实况视频没有视频轨"); return; }
    AVAssetWriterInputMetadataAdaptor *adaptor = ALBMSStillImageAdaptor(writer);
    if (![writer startWriting] || ![reader startReading]) {
        done(writer.error.localizedDescription ?: reader.error.localizedDescription ?: @"实况视频写入失败");
        return;
    }
    [writer startSessionAtSourceTime:kCMTimeZero];
    if (adaptor) {
        AVMutableMetadataItem *still = [AVMutableMetadataItem metadataItem];
        still.keySpace = AVMetadataKeySpaceQuickTimeMetadata;
        still.key = kALBMSStillImageTimeKey;
        still.value = @(-1);
        still.dataType = (__bridge NSString *)kCMMetadataBaseDataType_SInt8;
        CMTimeRange range = CMTimeRangeMake(CMTimeMake(0, 1000), CMTimeMake(200, 3000));
        [adaptor appendTimedMetadataGroup:[[AVTimedMetadataGroup alloc] initWithItems:@[still] timeRange:range]];
        [adaptor.assetWriterInput markAsFinished];
    }
    dispatch_group_t group = dispatch_group_create();
    for (NSArray *pair in pairs) {
        AVAssetReaderTrackOutput *source = pair[0];
        AVAssetWriterInput *input = pair[1];
        dispatch_queue_t queue = dispatch_queue_create("awemeliquidbar.mediasave.livephoto", DISPATCH_QUEUE_SERIAL);
        dispatch_group_enter(group);
        __block BOOL finished = NO;
        [input requestMediaDataWhenReadyOnQueue:queue usingBlock:^{
            while (!finished && input.readyForMoreMediaData) {
                CMSampleBufferRef buffer = reader.status == AVAssetReaderStatusReading ? [source copyNextSampleBuffer] : NULL;
                BOOL appended = buffer && [input appendSampleBuffer:buffer];
                if (buffer) CFRelease(buffer);
                if (!appended) {
                    finished = YES;
                    [input markAsFinished];
                    dispatch_group_leave(group);
                }
            }
        }];
    }
    dispatch_group_notify(group, dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        if (reader.status == AVAssetReaderStatusFailed) {
            [writer cancelWriting];
            done(reader.error.localizedDescription ?: @"实况视频读取失败");
            return;
        }
        [writer finishWritingWithCompletionHandler:^{
            done(writer.status == AVAssetWriterStatusCompleted ? nil : (writer.error.localizedDescription ?: @"实况视频写入失败"));
        }];
    });
}
static void ALBMSMakeLivePhoto(NSURL *image, NSURL *video, NSURL *directory, void (^completion)(NSURL *photo, NSURL *movie, NSString *error)) {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY, 0), ^{
        NSString *identifier = NSUUID.UUID.UUIDString;
        NSURL *photo = [directory URLByAppendingPathComponent:[identifier stringByAppendingPathExtension:@"jpg"]];
        NSURL *movie = [directory URLByAppendingPathComponent:[identifier stringByAppendingPathExtension:@"mov"]];
        if (!ALBMSWritePairedPhoto(image, photo, identifier)) { completion(nil, nil, @"实况封面写入失败"); return; }
        ALBMSWritePairedMovie(video, movie, identifier, ^(NSString *error) {
            completion(error ? nil : photo, error ? nil : movie, error);
        });
    });
}
/* 视频没有音轨：直通合成背景音乐（截到视频时长） */
static void ALBMSComposeAudio(NSURL *video, NSURL *audio, NSURL *output, void (^done)(NSURL *result, NSString *error)) {
    AVURLAsset *videoAsset = [AVURLAsset URLAssetWithURL:video options:nil];
    AVURLAsset *audioAsset = [AVURLAsset URLAssetWithURL:audio options:nil];
    AVAssetTrack *videoTrack = [videoAsset tracksWithMediaType:AVMediaTypeVideo].firstObject;
    AVAssetTrack *audioTrack = [audioAsset tracksWithMediaType:AVMediaTypeAudio].firstObject;
    if (!videoTrack || !audioTrack) { done(nil, @"缺少可合成的轨道"); return; }
    AVMutableComposition *composition = [AVMutableComposition composition];
    CMTimeRange range = CMTimeRangeMake(kCMTimeZero, videoAsset.duration);
    AVMutableCompositionTrack *v = [composition addMutableTrackWithMediaType:AVMediaTypeVideo preferredTrackID:kCMPersistentTrackID_Invalid];
    AVMutableCompositionTrack *a = [composition addMutableTrackWithMediaType:AVMediaTypeAudio preferredTrackID:kCMPersistentTrackID_Invalid];
    NSError *error = nil;
    [v insertTimeRange:range ofTrack:videoTrack atTime:kCMTimeZero error:&error];
    CMTime audioDuration = CMTimeMinimum(videoAsset.duration, audioAsset.duration);
    if (!error) [a insertTimeRange:CMTimeRangeMake(kCMTimeZero, audioDuration) ofTrack:audioTrack atTime:kCMTimeZero error:&error];
    if (error) { done(nil, error.localizedDescription); return; }
    v.preferredTransform = videoTrack.preferredTransform;
    AVAssetExportSession *session = [AVAssetExportSession exportSessionWithAsset:composition presetName:AVAssetExportPresetPassthrough];
    session.outputURL = output;
    session.outputFileType = AVFileTypeQuickTimeMovie;
    [session exportAsynchronouslyWithCompletionHandler:^{
        BOOL ok = session.status == AVAssetExportSessionStatusCompleted;
        done(ok ? output : nil, ok ? nil : (session.error.localizedDescription ?: @"合成失败"));
    }];
}

#pragma mark - 下载（候选和镜像逐个重试）

static dispatch_queue_t ALBMSQueue(void) {
    static dispatch_queue_t queue;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ queue = dispatch_queue_create("awemeliquidbar.mediasave", DISPATCH_QUEUE_SERIAL); });
    return queue;
}
typedef void (^ALBMSFetched)(NSURL *file, ALBMSCandidate *candidate, NSString *error);
static void ALBMSFetchAt(ALBDownloadJob *job, NSArray<ALBMSCandidate *> *candidates, NSUInteger ci, NSUInteger ui,
                         NSString *accept, NSString *(^validate)(NSURL *), NSString *lastError, ALBMSFetched completion) {
    if (job.downloadState != ALBDownloadActive) { completion(nil, nil, @"已取消"); return; }
    if (ci >= candidates.count) { completion(nil, nil, lastError ?: @"没有可用地址"); return; }
    ALBMSCandidate *candidate = candidates[ci];
    if (ui >= candidate.urls.count) { ALBMSFetchAt(job, candidates, ci + 1, 0, accept, validate, lastError, completion); return; }
    NSString *urlString = candidate.urls[ui];
    void (^next)(NSString *) = ^(NSString *error) { ALBMSFetchAt(job, candidates, ci, ui + 1, accept, validate, error, completion); };
    NSMutableURLRequest *request = AWEMediaRequest(urlString);
    if (!request) { next(@"地址无效"); return; }
    if (accept.length) [request setValue:accept forHTTPHeaderField:@"Accept"];
    NSURLSessionDownloadTask *task = [job downloadTaskWithRequest:request completion:^(NSURL *location, NSURLResponse *response, NSError *error) {
        NSInteger status = [response isKindOfClass:NSHTTPURLResponse.class] ? ((NSHTTPURLResponse *)response).statusCode : 0;
        NSString *failure = nil;
        NSURL *file = nil;
        if (job.downloadState != ALBDownloadActive) failure = @"已取消";
        else if (error) failure = error.localizedDescription ?: @"下载失败";
        else if (status < 200 || status >= 300) failure = [NSString stringWithFormat:@"HTTP %ld", (long)status];
        else if (!(file = [job moveDownloadedFile:location extension:@"bin"])) failure = @"无法写入临时文件";
        dispatch_async(ALBMSQueue(), ^{
            if (job.downloadState != ALBDownloadActive) { completion(nil, nil, @"已取消"); return; }
            NSString *reason = failure;
            NSURL *named = file;
            if (!reason) {
                NSNumber *size = nil;
                [file getResourceValue:&size forKey:NSURLFileSizeKey error:nil];
                NSString *extension = size.longLongValue > 0 ? ALBMSFileExtension(file) : nil;
                NSURL *renamed = extension ? [[file URLByDeletingPathExtension] URLByAppendingPathExtension:extension] : nil;
                if (renamed && [NSFileManager.defaultManager moveItemAtURL:file toURL:renamed error:nil]) named = renamed;
                reason = size.longLongValue > 0 ? (validate ? validate(named) : nil) : @"文件为空";
            }
            if (reason) {
                if (named) [NSFileManager.defaultManager removeItemAtURL:named error:nil];
                if ([reason isEqualToString:@"已取消"]) { completion(nil, nil, reason); return; }
                next(reason);
                return;
            }
            completion(named, candidate, nil);
        });
    }];
    if (!task) { completion(nil, nil, @"已取消"); return; }
    [job startTask:task];
}
static void ALBMSFetch(ALBDownloadJob *job, NSArray<ALBMSCandidate *> *candidates, NSString *accept,
                       NSString *(^validate)(NSURL *), ALBMSFetched completion) {
    ALBMSFetchAt(job, candidates, 0, 0, accept, validate, nil, completion);
}

#pragma mark - 一次保存

@interface ALBMSReady : NSObject
@property (nonatomic, assign) ALBMSKind kind;
@property (nonatomic, strong) NSURL *file;     // 视频 / 照片
@property (nonatomic, strong) NSURL *paired;   // 实况的视频
@end
@implementation ALBMSReady
@end

static NSString *const kALBMSImageAccept = @"image/heic,image/heif,image/webp,image/jpeg,image/*;q=0.8,*/*;q=0.5";

static void ALBMSPrepare(ALBDownloadJob *job, ALBMSItem *item, void (^done)(ALBMSReady *ready, NSString *error, NSString *detail)) {
    NSURL *directory = job.temporaryDirectory;
    if (item.kind == ALBMSKindVideo) {
        ALBMSFetch(job, item.primary, nil, ^NSString *(NSURL *file) { return ALBMSVideoCheck(file); },
                   ^(NSURL *file, ALBMSCandidate *candidate, NSString *error) {
            if (error) { done(nil, error, nil); return; }
            ALBMSReady *ready = [ALBMSReady new];
            ready.kind = ALBMSKindVideo;
            ready.file = file;
            if (!item.music.count || ALBMSVideoHasAudio(file)) { done(ready, nil, candidate.tag); return; }
            ALBMSFetch(job, item.music, nil, nil, ^(NSURL *audio, __unused ALBMSCandidate *c, NSString *audioError) {
                if (audioError) { done(ready, nil, [candidate.tag stringByAppendingString:@" 无音轨"]); return; }
                NSURL *output = [directory URLByAppendingPathComponent:[NSUUID.UUID.UUIDString stringByAppendingPathExtension:@"mov"]];
                ALBMSComposeAudio(file, audio, output, ^(NSURL *merged, __unused NSString *mergeError) {
                    if (merged) ready.file = merged;
                    done(ready, nil, [candidate.tag stringByAppendingString:merged ? @" 已补音轨" : @" 补音轨失败"]);
                });
            });
        });
        return;
    }
    if (item.kind == ALBMSKindLive) {
        ALBMSFetch(job, item.primary, kALBMSImageAccept, ^NSString *(NSURL *file) { return ALBMSImageCheck(file); },
                   ^(NSURL *image, ALBMSCandidate *imageCandidate, NSString *imageError) {
            if (imageError) { done(nil, imageError, nil); return; }
            ALBMSFetch(job, item.paired, nil, ^NSString *(NSURL *file) { return ALBMSVideoCheck(file); },
                       ^(NSURL *video, ALBMSCandidate *videoCandidate, NSString *videoError) {
                if (videoError) { done(nil, videoError, nil); return; }
                NSString *detail = [NSString stringWithFormat:@"%@ + %@", imageCandidate.tag, videoCandidate.tag];
                ALBMSMakeLivePhoto(image, video, directory, ^(NSURL *photo, NSURL *movie, NSString *pairError) {
                    if (pairError) { done(nil, pairError, detail); return; }
                    ALBMSReady *ready = [ALBMSReady new];
                    ready.kind = ALBMSKindLive;
                    ready.file = photo;
                    ready.paired = movie;
                    done(ready, nil, detail);
                });
            });
        });
        return;
    }
    ALBMSFetch(job, item.primary, kALBMSImageAccept, ^NSString *(NSURL *file) { return ALBMSImageCheck(file); },
               ^(NSURL *file, ALBMSCandidate *candidate, NSString *error) {
        if (error) { done(nil, error, nil); return; }
        NSString *why = nil;
        NSURL *photo = ALBMSPhotoFile(file, directory, &why);
        if (!photo) { done(nil, why ?: @"图片转换失败", candidate.tag); return; }
        ALBMSReady *ready = [ALBMSReady new];
        ready.kind = ALBMSKindImage;
        ready.file = photo;
        done(ready, nil, candidate.tag);
    });
}

static NSString *ALBMSSuccessText(NSArray<ALBMSReady *> *ready, NSUInteger failed) {
    if (ready.count == 1 && !failed) {
        ALBMSKind kind = ready.firstObject.kind;
        return kind == ALBMSKindLive ? @"实况已保存到相册" : (kind == ALBMSKindVideo ? @"视频已保存（最高画质）" : @"图片已保存");
    }
    if (!failed) return [NSString stringWithFormat:@"已保存 %lu 项", (unsigned long)ready.count];
    return [NSString stringWithFormat:@"已保存 %lu 项，失败 %lu 项", (unsigned long)ready.count, (unsigned long)failed];
}

static void ALBMSRun(NSArray<ALBMSItem *> *items, NSString *source) {
    if (!NSThread.isMainThread) { dispatch_async(dispatch_get_main_queue(), ^{ ALBMSRun(items, source); }); return; }
    gALBMSRuns++;
    if (!items.count) {
        AWEShowDownloadToast(@"没有可保存的内容");
        ALBMSNote([NSString stringWithFormat:@"[%@] 没有可保存的内容", source ?: @""]);
        return;
    }
    ALBDownloadJob *job = ALB8BeginDownloadJob();
    BOOL single = items.count == 1;
    NSString *noun = single ? ALBMSKindName(items.firstObject.kind) : [NSString stringWithFormat:@"%lu 项", (unsigned long)items.count];
    AWEShowDownloadToastForJob([NSString stringWithFormat:@"正在下载%@%@", noun, single && items.firstObject.kind == ALBMSKindVideo ? @"（最高画质）" : @""], job);
    NSMutableArray<ALBMSReady *> *readyList = [NSMutableArray array];
    __block NSUInteger failed = 0;
    __block NSString *lastError = nil;
    __block void (^step)(NSUInteger);
    __block __weak void (^weakStep)(NSUInteger);
    void (^commit)(void) = ^{
        if (job.downloadState != ALBDownloadActive) return;
        if (!readyList.count) {
            gALBMSFailed += failed;
            ALB8FinishDownload(job, [NSString stringWithFormat:@"保存失败：%@", lastError ?: @"未知原因"]);
            return;
        }
        BOOL needsLive = NO;
        for (ALBMSReady *ready in readyList) needsLive = needsLive || ready.kind == ALBMSKindLive;
        if (needsLive && ![PHAssetCreationRequest supportsAssetResourceTypes:@[@(PHAssetResourceTypePhoto), @(PHAssetResourceTypePairedVideo)]]) {
            ALB8FinishDownload(job, @"系统不支持写入实况");
            return;
        }
        if (![job beginCommit]) return;
        AWEShowDownloadToastForJob(@"正在写入相册", job);
        NSArray<ALBMSReady *> *list = readyList.copy;
        [[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
            for (ALBMSReady *ready in list) {
                PHAssetCreationRequest *request = [PHAssetCreationRequest creationRequestForAsset];
                if (ready.kind == ALBMSKindVideo) {
                    [request addResourceWithType:PHAssetResourceTypeVideo fileURL:ready.file options:nil];
                } else if (ready.kind == ALBMSKindLive) {
                    [request addResourceWithType:PHAssetResourceTypePhoto fileURL:ready.file options:nil];
                    [request addResourceWithType:PHAssetResourceTypePairedVideo fileURL:ready.paired options:nil];
                } else {
                    [request addResourceWithType:PHAssetResourceTypePhoto fileURL:ready.file options:nil];
                }
            }
        } completionHandler:^(BOOL success, NSError *error) {
            if (success) { gALBMSSaved += list.count; gALBMSFailed += failed; }
            ALBMSNote([NSString stringWithFormat:@"[%@] 写入相册 %@ %lu 项%@", source ?: @"", success ? @"成功" : @"失败",
                       (unsigned long)list.count, success ? @"" : [@" " stringByAppendingString:error.localizedDescription ?: @""]]);
            ALB8FinishDownload(job, success ? ALBMSSuccessText(list, failed) : @"无法写入相册，请检查照片权限");
        }];
    };
    step = ^(NSUInteger index) {
        if (job.downloadState != ALBDownloadActive) return;
        if (index >= items.count) { dispatch_async(dispatch_get_main_queue(), commit); return; }
        ALBMSItem *item = items[index];
        if (!single) {
            dispatch_async(dispatch_get_main_queue(), ^{
                AWEShowDownloadToastForJob([NSString stringWithFormat:@"正在下载 %lu/%lu（%@）", (unsigned long)index + 1,
                                            (unsigned long)items.count, ALBMSKindName(item.kind)], job);
            });
        }
        ALBMSPrepare(job, item, ^(ALBMSReady *ready, NSString *error, NSString *detail) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (ready) [readyList addObject:ready];
                else { failed++; lastError = error; }
                ALBMSNote([NSString stringWithFormat:@"[%@] %@ %@ %@", source ?: @"", ALBMSKindName(item.kind),
                           error ? @"失败" : @"下载好", error ?: (detail ?: @"")]);
                void (^go)(NSUInteger) = weakStep;
                if (go) go(index + 1);
            });
        });
    };
    weakStep = step;
    AWERequestPhotoAddPermission(^(BOOL granted) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if (!granted) { ALB8FinishDownload(job, @"请允许添加照片后重试"); step = nil; return; }
            step(0);
        });
    });
    // step 由下载回调一路持有到结束；结束（写完 / 取消 / 失败）后放掉
    job.cancellationHandler = ^{ dispatch_async(dispatch_get_main_queue(), ^{ step = nil; }); };
}

static void ALBMSSave(id aweme, ALBMSWhat what, NSString *source) {
    NSArray<ALBMSItem *> *items = ALBMSItemsFor(aweme, what);
    AWEInteractionDiagnosticWrite(@"MEDIA_SAVE_REQUEST what=%ld model=%@ items=%lu source=%@", (long)what,
                                  AWEAwemeIdentity(aweme), (unsigned long)items.count, source ?: @"");
    if (!items.count) {
        if (what == ALBMSWhatMain) AWEShowDownloadToast(@"未找到可下载的视频资源");
        else if (what == ALBMSWhatAllLive || what == ALBMSWhatCurrentLive) AWEShowDownloadToast(@"当前作品没有实况");
        else AWEShowDownloadToast(@"没有可保存的内容");
        return;
    }
    ALBMSRun(items, source);
}

static NSString *ALBMediaSaveReport(void) {
    return [NSString stringWithFormat:@"视频最高画质、实况配对；发起 %lu 次、存进相册 %lu 项、失败 %lu 项；最近：%@",
            (unsigned long)gALBMSRuns, (unsigned long)gALBMSSaved, (unsigned long)gALBMSFailed,
            gALBMSLog.count ? [gALBMSLog componentsJoinedByString:@"；"] : @"-"];
}
