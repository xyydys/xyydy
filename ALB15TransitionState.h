#pragma once
#include <stdint.h>
// Pure coordination, not an animation engine. UIKit alone owns the trajectory
// and duration. A completion is accepted only for the current native request.
typedef struct { uint64_t generation; int target; int moving; } ALB15TransitionState;
static inline ALB15TransitionState ALB15StateMake(void) {
    ALB15TransitionState value={0,0,0};return value;
}
static inline uint64_t ALB15StateBegin(ALB15TransitionState *state,int compact) {
    state->generation++;state->target=compact?1:0;state->moving=1;return state->generation;
}
static inline int ALB15StateComplete(ALB15TransitionState *state,uint64_t token,int compact) {
    if(token!=state->generation || state->target!=(compact?1:0) || !state->moving)return 0;
    state->moving=0;return 1;
}
static inline void ALB15StateInvalidate(ALB15TransitionState *state) {
    state->generation++;state->moving=0;
}
