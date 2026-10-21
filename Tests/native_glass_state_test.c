#include "../ALB15TransitionState.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    ALB15TransitionState s=ALB15StateMake();
    uint64_t first=ALB15StateBegin(&s,1),second=ALB15StateBegin(&s,0),third=ALB15StateBegin(&s,1);
    assert(!ALB15StateComplete(&s,first,1)); // old collapse must not hide new content
    assert(!ALB15StateComplete(&s,second,0)); // old expand must not reveal both bars
    assert(ALB15StateComplete(&s,third,1));
    assert(!ALB15StateComplete(&s,third,1)); // one-shot completion
    uint64_t beforeReset=ALB15StateBegin(&s,0);ALB15StateInvalidate(&s);
    assert(!ALB15StateComplete(&s,beforeReset,0));
    for(int i=0;i<100000;i++) {
        uint64_t a=ALB15StateBegin(&s,i&1);
        uint64_t b=ALB15StateBegin(&s,!(i&1));
        assert(!ALB15StateComplete(&s,a,i&1));
        assert(ALB15StateComplete(&s,b,!(i&1)));
    }
    puts("native glass: interrupted/reversed/reset/stale/one-shot completion checks passed");
}
