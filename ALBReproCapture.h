#pragma once
// 2.2 (user request): the floating reproduction-capture overlay is removed.
// Only the everyday "交互诊断日志" remains. This keeps the recorder's C API as
// an inert implementation: it never records, never shows a window and never
// writes files, so every existing call site compiles unchanged and costs one
// branch. ALBReproCaptureSignal.h keeps the declarations.
#import "ALBReproCaptureSignal.h"

BOOL ALBReproIsRecording(void) { return NO; }
BOOL ALBReproIsEnabled(void) { return NO; }
void ALBReproRecordEvent(NSString *name, NSDictionary<NSString *,id> *payload) { (void)name; (void)payload; }
void ALBReproScheduleSnapshot(NSString *reason, NSTimeInterval delay) { (void)reason; (void)delay; }
void ALBReproRecordTouch(UITouch *touch, UIEvent *event) { (void)touch; (void)event; }
void ALBReproSetEnabled(BOOL enabled) {
    (void)enabled;
    // Clear the old preference so a previous build's overlay never returns.
    [NSUserDefaults.standardUserDefaults removeObjectForKey:@"ALB.repro.enabled"];
}
void ALBReproRestoreIfEnabled(void) { ALBReproSetEnabled(NO); }
void ALBReproConfigure(NSDictionary<NSString *,id> *(^context)(void), void (^exitCleanScreen)(void)) {
    (void)context; (void)exitCleanScreen;
}
