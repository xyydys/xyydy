#ifndef ALB_REPRO_CAPTURE_SIGNAL_H
#define ALB_REPRO_CAPTURE_SIGNAL_H

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#ifdef __cplusplus
extern "C" {
#endif

FOUNDATION_EXPORT BOOL ALBReproIsRecording(void);
FOUNDATION_EXPORT BOOL ALBReproIsEnabled(void);
FOUNDATION_EXPORT void ALBReproRecordEvent(NSString *name, NSDictionary<NSString *, id> *payload);
FOUNDATION_EXPORT void ALBReproScheduleSnapshot(NSString *reason, NSTimeInterval delay);
FOUNDATION_EXPORT void ALBReproRecordTouch(UITouch *touch, UIEvent *event);
FOUNDATION_EXPORT void ALBReproSetEnabled(BOOL enabled);
FOUNDATION_EXPORT void ALBReproRestoreIfEnabled(void);
FOUNDATION_EXPORT void ALBReproConfigure(NSDictionary<NSString *, id> *(^context)(void),
                                       void (^exitCleanScreen)(void));

#ifdef __cplusplus
}
#endif
#endif
