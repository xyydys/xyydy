#include <assert.h>
#include "../ALBDownloadState.h"
int main(void) {
    ALBDownloadState state = ALBDownloadActive;
    assert(ALBDownloadCancel(&state));
    assert(!ALBDownloadCancel(&state));
    assert(!ALBDownloadBeginCommit(&state));
    assert(!ALBDownloadFinish(&state)); // cancelled late completion cannot report success
    state = ALBDownloadActive;
    assert(ALBDownloadBeginCommit(&state));
    assert(!ALBDownloadCancel(&state)); // photo commit is not falsely advertised as cancellable
    assert(ALBDownloadFinish(&state));
    assert(!ALBDownloadFinish(&state));
    assert(!ALBDownloadCancel(&state));
    state = ALBDownloadActive;
    assert(ALBDownloadFinish(&state)); // network failure or audio export can terminate directly
    assert(!ALBDownloadBeginCommit(&state));
    // Independent operation states: cancelling A cannot change B.
    ALBDownloadState a = ALBDownloadActive, b = ALBDownloadActive;
    assert(ALBDownloadCancel(&a));
    assert(ALBDownloadBeginCommit(&b));
    assert(a == ALBDownloadCancelled && b == ALBDownloadCommitting);
    return 0;
}
