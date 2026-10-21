/* Sequential image download / bounded-memory silent slideshow export. */
#import <AVFoundation/AVFoundation.h>

@interface ALBAlbumVideoExporter : NSObject
@property(nonatomic, copy) NSArray<NSString *> *urls;
@property(nonatomic, strong) ALBDownloadJob *downloadJob;
@property(nonatomic, strong) AVAssetWriter *writer;
@property(nonatomic, strong) AVAssetWriterInput *input;
@property(nonatomic, strong) AVAssetWriterInputPixelBufferAdaptor *adaptor;
@property(nonatomic, strong) NSURL *fileURL;
@property(nonatomic, strong) dispatch_queue_t queue;
@property(nonatomic) NSUInteger frame;
@property(nonatomic) BOOL downloading;
@property(nonatomic) BOOL finished;
@property(nonatomic) CVPixelBufferRef pixels;
- (void)drain;
- (void)cancel;
- (void)finishWithError:(NSString *)message;
@end
static ALBAlbumVideoExporter *gALBAlbumExporter;

@implementation ALBAlbumVideoExporter
- (void)dealloc { if (_pixels) CVPixelBufferRelease(_pixels); }
- (void)cancel {
    dispatch_async(self.queue, ^{
        self.finished = YES;
        [self.writer cancelWriting];
        if (self.pixels) { CVPixelBufferRelease(self.pixels); self.pixels = NULL; }
        // A writer may still have held an open file when the tap removed the
        // job directory. Clean again after it has stopped, within this job only.
        [self.downloadJob cleanTemporaryFiles];
        dispatch_async(dispatch_get_main_queue(), ^{
            if (gALBAlbumExporter == self) gALBAlbumExporter = nil;
        });
    });
}
- (void)finishWithError:(NSString *)message {
    if (self.finished) return;
    self.finished = YES;
    if (message) [self.writer cancelWriting];
    if (self.pixels) { CVPixelBufferRelease(self.pixels); self.pixels = NULL; }
    dispatch_async(dispatch_get_main_queue(), ^{
        if (self.downloadJob.downloadState == ALBDownloadActive) {
            if (message) ALB8FinishDownload(self.downloadJob, message);
            else ALB8SaveDownloadedFiles(self.downloadJob, @[self.fileURL], YES, @"图集视频已保存（每张 3 秒）");
        } else if (self.downloadJob.downloadState == ALBDownloadCancelled) {
            [self.downloadJob cleanTemporaryFiles];
        }
        if (gALBAlbumExporter == self) gALBAlbumExporter = nil;
    });
}
- (BOOL)preparePixels:(UIImage *)image {
    if (!image || !image.CGImage) return NO;
    CVPixelBufferRef buffer = NULL;
    CVReturn result = CVPixelBufferPoolCreatePixelBuffer(kCFAllocatorDefault, self.adaptor.pixelBufferPool, &buffer);
    if (result != kCVReturnSuccess || !buffer) return NO;
    CVPixelBufferLockBaseAddress(buffer, 0);
    CGColorSpaceRef space = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(CVPixelBufferGetBaseAddress(buffer), 720, 1280, 8,
        CVPixelBufferGetBytesPerRow(buffer), space, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst);
    CGColorSpaceRelease(space);
    if (!context) { CVPixelBufferUnlockBaseAddress(buffer, 0); CVPixelBufferRelease(buffer); return NO; }
    CGContextSetRGBFillColor(context, 0, 0, 0, 1);
    CGContextFillRect(context, CGRectMake(0, 0, 720, 1280));
    CGFloat scale = MIN(720.0 / MAX(1, image.size.width), 1280.0 / MAX(1, image.size.height));
    CGSize size = CGSizeMake(image.size.width * scale, image.size.height * scale);
    CGContextTranslateCTM(context, 0, 1280); CGContextScaleCTM(context, 1, -1);
    UIGraphicsPushContext(context);
    [image drawInRect:CGRectMake((720-size.width)*0.5, (1280-size.height)*0.5, size.width, size.height)];
    UIGraphicsPopContext();
    CGContextRelease(context);
    CVPixelBufferUnlockBaseAddress(buffer, 0);
    self.pixels = buffer;
    return YES;
}
- (void)loadImageAtIndex:(NSUInteger)index {
    if (self.downloading || self.finished || self.downloadJob.downloadState != ALBDownloadActive) return;
    self.downloading = YES;
    NSMutableURLRequest *request = AWEMediaRequest(self.urls[index]);
    if (!request) { [self finishWithError:@"图集图片地址无效"]; return; }
    request.timeoutInterval = 30;
    NSURLSessionDataTask *task = [self.downloadJob dataTaskWithRequest:request completion:^(NSData *data, NSURLResponse *response, NSError *error) {
        dispatch_async(self.queue, ^{
            if (self.finished || self.downloadJob.downloadState != ALBDownloadActive) return;
            self.downloading = NO;
            NSInteger status = [response isKindOfClass:NSHTTPURLResponse.class] ? ((NSHTTPURLResponse *)response).statusCode : 0;
            // Decode a thumbnail: even a very large original must not become
            // an unbounded full-resolution bitmap during a slideshow export.
            CGImageSourceRef source = (!error && data.length && status < 400) ? CGImageSourceCreateWithData((__bridge CFDataRef)data, NULL) : NULL;
            CGImageRef thumb = source ? CGImageSourceCreateThumbnailAtIndex(source, 0, (__bridge CFDictionaryRef)@{
                (__bridge NSString *)kCGImageSourceCreateThumbnailFromImageAlways:@YES,
                (__bridge NSString *)kCGImageSourceThumbnailMaxPixelSize:@1280,
                (__bridge NSString *)kCGImageSourceCreateThumbnailWithTransform:@YES}) : NULL;
            UIImage *image = thumb ? [UIImage imageWithCGImage:thumb] : nil;
            if (thumb) CGImageRelease(thumb);
            if (source) CFRelease(source);
            if (![self preparePixels:image]) { [self finishWithError:@"图集图片读取失败，未生成不完整视频"]; return; }
            [self drain];
        });
    }];
    [self.downloadJob startTask:task];
}
- (void)drain {
    if (self.finished || self.downloading || self.downloadJob.downloadState != ALBDownloadActive) return;
    while (self.input.readyForMoreMediaData && !self.finished && self.downloadJob.downloadState == ALBDownloadActive) {
        if (self.writer.status == AVAssetWriterStatusFailed) { [self finishWithError:@"视频编码失败"]; return; }
        NSUInteger index = self.frame / 90;
        if (index >= self.urls.count) {
            [self.writer endSessionAtSourceTime:CMTimeMake(self.frame, 30)];
            [self.input markAsFinished];
            self.downloading = YES; // also prevents re-entry while finalizing
            [self.writer finishWritingWithCompletionHandler:^{
                dispatch_async(self.queue, ^{ [self finishWithError:self.writer.status == AVAssetWriterStatusCompleted ? nil : @"视频封装失败"]; });
            }];
            return;
        }
        if (!self.pixels) { [self loadImageAtIndex:index]; return; }
        if (![self.adaptor appendPixelBuffer:self.pixels withPresentationTime:CMTimeMake(self.frame, 30)]) {
            [self finishWithError:@"写入视频帧失败"]; return;
        }
        self.frame++;
        if (self.frame % 90 == 0) { CVPixelBufferRelease(self.pixels); self.pixels = NULL; }
    }
}
@end

static void ALBExportAlbumVideo(id aweme) {
    if (gALBAlbumExporter) { AWEShowDownloadToast(@"上一段图集视频仍在导出"); return; }
    NSArray *images = AWEAlbumImagesForAweme(aweme);
    if (!images.count) { AWEShowDownloadToast(@"当前作品不是图集"); return; }
    NSMutableArray *urls = [NSMutableArray array];
    for (id image in images) {
        NSString *url = AWEImageURLForModel(image);
        if (!url.length) { AWEShowDownloadToast(@"部分图片地址缺失，无法生成完整图集视频"); return; }
        [urls addObject:url];
    }
    ALBAlbumVideoExporter *job = [ALBAlbumVideoExporter new];
    job.urls = urls;
    job.downloadJob = ALB8BeginDownloadJob();
    job.queue = dispatch_queue_create("com.xyy.awemeliquidbar.album-export", DISPATCH_QUEUE_SERIAL);
    job.fileURL = [job.downloadJob newTemporaryURLWithExtension:@"mp4"];
    if (!job.fileURL) { ALB8FinishDownload(job.downloadJob, @"无法创建视频临时目录"); return; }
    NSError *error = nil;
    job.writer = [[AVAssetWriter alloc] initWithURL:job.fileURL fileType:AVFileTypeMPEG4 error:&error];
    if (!job.writer) { ALB8FinishDownload(job.downloadJob, @"无法创建视频文件"); return; }
    job.input = [AVAssetWriterInput assetWriterInputWithMediaType:AVMediaTypeVideo outputSettings:@{
        AVVideoCodecKey:AVVideoCodecTypeH264, AVVideoWidthKey:@720, AVVideoHeightKey:@1280,
        AVVideoCompressionPropertiesKey:@{AVVideoAverageBitRateKey:@4000000, AVVideoExpectedSourceFrameRateKey:@30}}];
    job.input.expectsMediaDataInRealTime = NO;
    job.adaptor = [AVAssetWriterInputPixelBufferAdaptor assetWriterInputPixelBufferAdaptorWithAssetWriterInput:job.input
        sourcePixelBufferAttributes:@{(id)kCVPixelBufferPixelFormatTypeKey:@(kCVPixelFormatType_32BGRA),
            (id)kCVPixelBufferWidthKey:@720, (id)kCVPixelBufferHeightKey:@1280,
            (id)kCVPixelBufferCGImageCompatibilityKey:@YES, (id)kCVPixelBufferCGBitmapContextCompatibilityKey:@YES}];
    if (![job.writer canAddInput:job.input]) { [job.writer cancelWriting]; ALB8FinishDownload(job.downloadJob, @"当前设备不支持此视频编码"); return; }
    [job.writer addInput:job.input];
    if (![job.writer startWriting]) { [job.writer cancelWriting]; ALB8FinishDownload(job.downloadJob, @"无法启动视频编码"); return; }
    [job.writer startSessionAtSourceTime:kCMTimeZero];
    gALBAlbumExporter = job;
    __weak ALBAlbumVideoExporter *weakJob = job;
    job.downloadJob.cancellationHandler = ^{ [weakJob cancel]; };
    AWEShowDownloadToastForJob(@"正在生成图集视频（每张 3 秒，无音轨）", job.downloadJob);
    [job.input requestMediaDataWhenReadyOnQueue:job.queue usingBlock:^{ [weakJob drain]; }];
}
