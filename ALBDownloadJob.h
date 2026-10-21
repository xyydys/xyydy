#ifndef ALB_DOWNLOAD_JOB_H
#define ALB_DOWNLOAD_JOB_H
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>
#include "ALBDownloadState.h"

/* Each operation owns one ephemeral session and one UUID directory. We never
   purge NSURLCache.sharedURLCache, the app's cache, or another download. */
@interface ALBDownloadJob : NSObject {
    ALBDownloadState _downloadState;
}
@property(nonatomic, strong, readonly) NSURLSession *session;
@property(nonatomic, strong, readonly) NSURL *temporaryDirectory;
@property(nonatomic, copy) void (^cancellationHandler)(void);
@property(nonatomic, readonly) ALBDownloadState downloadState;
- (NSURLSessionDownloadTask *)downloadTaskWithRequest:(NSURLRequest *)request
    completion:(void (^)(NSURL *, NSURLResponse *, NSError *))completion;
- (NSURLSessionDataTask *)dataTaskWithRequest:(NSURLRequest *)request
    completion:(void (^)(NSData *, NSURLResponse *, NSError *))completion;
- (void)startTask:(NSURLSessionTask *)task;
- (NSURL *)newTemporaryURLWithExtension:(NSString *)extension;
- (NSURL *)moveDownloadedFile:(NSURL *)location extension:(NSString *)extension;
- (BOOL)beginCommit;
- (BOOL)cancel;
- (BOOL)finishKeepingFiles:(BOOL)keep;
- (void)cleanTemporaryFiles;
@end

@implementation ALBDownloadJob
- (instancetype)init {
    if ((self = [super init])) {
        _downloadState = ALBDownloadActive;
        NSString *name = [@"ALB_download_" stringByAppendingString:NSUUID.UUID.UUIDString];
        _temporaryDirectory = [NSURL fileURLWithPath:[NSTemporaryDirectory()
            stringByAppendingPathComponent:name] isDirectory:YES];
        NSURLSessionConfiguration *configuration = NSURLSessionConfiguration.ephemeralSessionConfiguration;
        configuration.URLCache = nil;
        configuration.HTTPCookieStorage = NSHTTPCookieStorage.sharedHTTPCookieStorage;
        configuration.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;
        configuration.HTTPMaximumConnectionsPerHost = 3;
        _session = [NSURLSession sessionWithConfiguration:configuration];
    }
    return self;
}
- (ALBDownloadState)downloadState { @synchronized(self) { return _downloadState; } }
- (NSURLSessionDownloadTask *)downloadTaskWithRequest:(NSURLRequest *)request
    completion:(void (^)(NSURL *, NSURLResponse *, NSError *))completion {
    @synchronized(self) {
        if (_downloadState != ALBDownloadActive) return nil;
        return [self.session downloadTaskWithRequest:request completionHandler:completion];
    }
}
- (NSURLSessionDataTask *)dataTaskWithRequest:(NSURLRequest *)request
    completion:(void (^)(NSData *, NSURLResponse *, NSError *))completion {
    @synchronized(self) {
        if (_downloadState != ALBDownloadActive) return nil;
        return [self.session dataTaskWithRequest:request completionHandler:completion];
    }
}
- (void)startTask:(NSURLSessionTask *)task {
    @synchronized(self) {
        if (_downloadState == ALBDownloadActive) [task resume];
        else [task cancel];
    }
}
- (NSURL *)newTemporaryURLWithExtension:(NSString *)extension {
    @synchronized(self) {
        if (_downloadState != ALBDownloadActive) return nil;
        NSString *ext = extension.lowercaseString;
        NSCharacterSet *invalid = NSCharacterSet.alphanumericCharacterSet.invertedSet;
        if (!ext.length || ext.length > 8 || [ext rangeOfCharacterFromSet:invalid].location != NSNotFound) ext = @"bin";
        if (![NSFileManager.defaultManager createDirectoryAtURL:self.temporaryDirectory
            withIntermediateDirectories:YES attributes:nil error:nil]) return nil;
        return [self.temporaryDirectory URLByAppendingPathComponent:
            [NSString stringWithFormat:@"%@.%@", NSUUID.UUID.UUIDString, ext]];
    }
}
- (NSURL *)moveDownloadedFile:(NSURL *)location extension:(NSString *)extension {
    @synchronized(self) {
        if (_downloadState != ALBDownloadActive || !location) return nil;
        NSURL *target = [self newTemporaryURLWithExtension:extension];
        if (target && [NSFileManager.defaultManager moveItemAtURL:location toURL:target error:nil]) return target;
        return nil;
    }
}
- (BOOL)beginCommit { @synchronized(self) { return ALBDownloadBeginCommit(&_downloadState); } }
- (BOOL)cancel {
    void (^handler)(void) = nil;
    @synchronized(self) {
        if (!ALBDownloadCancel(&_downloadState)) return NO;
        handler = self.cancellationHandler;
        self.cancellationHandler = nil;
    }
    [self.session invalidateAndCancel];
    if (handler) handler();
    [self cleanTemporaryFiles];
    return YES;
}
- (BOOL)finishKeepingFiles:(BOOL)keep {
    @synchronized(self) {
        if (!ALBDownloadFinish(&_downloadState)) return NO;
        self.cancellationHandler = nil;
    }
    [self.session finishTasksAndInvalidate];
    if (!keep) [self cleanTemporaryFiles];
    return YES;
}
- (void)cleanTemporaryFiles {
    @synchronized(self) {
        // temporaryDirectory is generated internally and never reassigned.
        [NSFileManager.defaultManager removeItemAtURL:self.temporaryDirectory error:nil];
    }
}
@end

/* The visible progress toast is bound to this exact job. Older jobs may finish
   normally but cannot replace or cancel a newer operation's toast. */
static ALBDownloadJob *gALB8DownloadToastJob;
static ALBDownloadJob *ALB8BeginDownloadJob(void) {
    ALBDownloadJob *job = [ALBDownloadJob new];
    @synchronized(ALBDownloadJob.class) { gALB8DownloadToastJob = job; }
    return job;
}
static BOOL ALB8JobOwnsToast(ALBDownloadJob *job) {
    @synchronized(ALBDownloadJob.class) { return job == gALB8DownloadToastJob; }
}
#endif
