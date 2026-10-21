#ifndef ALB_DOWNLOAD_STATE_H
#define ALB_DOWNLOAD_STATE_H
#include <stdbool.h>
/* Serialized by ALBDownloadJob; no UI or file-system state in this policy. */
typedef enum {
    ALBDownloadActive = 0,
    ALBDownloadCommitting,
    ALBDownloadFinished,
    ALBDownloadCancelled
} ALBDownloadState;
static inline bool ALBDownloadBeginCommit(ALBDownloadState *state) {
    if (*state != ALBDownloadActive) return false;
    *state = ALBDownloadCommitting; return true;
}
static inline bool ALBDownloadCancel(ALBDownloadState *state) {
    if (*state != ALBDownloadActive) return false;
    *state = ALBDownloadCancelled; return true;
}
static inline bool ALBDownloadFinish(ALBDownloadState *state) {
    if (*state != ALBDownloadActive && *state != ALBDownloadCommitting) return false;
    *state = ALBDownloadFinished; return true;
}
#endif
