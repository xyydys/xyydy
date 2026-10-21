#include "../ALBScrollIntent.h"
#include "../ALB7BottomMath.h"
#include <assert.h>
#include <stdio.h>

/* Exercise the root-scroll adapter against the preserved baseline policy.
   These sequences model a pan and the later settled-item event separately. */
static int settle_feed(int next, int timerMode, int compact, int nextExpands,
                       int *timerCollapsed) {
    int idleOwner = timerMode && *timerCollapsed && compact;
    int target = ALB8FeedCompactPolicy(next,timerMode,compact,idleOwner && nextExpands);
    *timerCollapsed = idleOwner && target;
    return target;
}

static void feed_owner_sequences(void) {
    int timerOwner=0;
    int compact=1; /* Upward root pan just collapsed the bar. Timer is enabled. */
    for(int i=0;i<5;i++) {
        compact=settle_feed(1,1,compact,1,&timerOwner);
        assert(compact==1 && timerOwner==0); /* Settled next must not reverse it. */
    }
    compact=settle_feed(0,1,compact,1,&timerOwner);
    assert(compact==0 && timerOwner==0); /* Previous expands a manual ball. */
    compact=settle_feed(1,1,compact,1,&timerOwner);
    assert(compact==0 && timerOwner==0); /* Auto/duplicate next does not collapse. */

    compact=1; timerOwner=1; /* A genuine idle deadline produced this ball. */
    compact=settle_feed(1,1,compact,1,&timerOwner);
    assert(compact==0 && timerOwner==0); /* The timer-specific preference applies. */
    compact=settle_feed(1,1,compact,1,&timerOwner);
    assert(compact==0 && timerOwner==0);

    compact=1; timerOwner=1;
    compact=settle_feed(1,1,compact,0,&timerOwner);
    assert(compact==1 && timerOwner==1); /* Preference off preserves an idle ball. */
    compact=settle_feed(0,1,compact,0,&timerOwner);
    assert(compact==0 && timerOwner==0);

    compact=0; timerOwner=1; /* Failed idle request must not confer ownership. */
    compact=settle_feed(1,1,compact,1,&timerOwner);
    assert(compact==0 && timerOwner==0);
    compact=settle_feed(1,0,compact,1,&timerOwner);
    assert(compact==1 && timerOwner==0); /* Ordinary feed mode still collapses. */
}

int main(void) {
    ALBPanState s;
    ALBPanBegin(&s,0,0);
    assert(ALBPanUpdate(&s,0,-12,0,-200,10)==1);
    assert(ALBPanUpdate(&s,0,-40,0,-200,10)==0);
    /* Finger reverses while translation is STILL negative: expand immediately. */
    assert(ALBPanUpdate(&s,0,-28,0,200,10)==-1);
    assert(ALBPanUpdate(&s,0,-14,0,200,10)==0);
    assert(ALBPanUpdate(&s,0,-27,0,-200,10)==1);
    ALBPanBegin(&s,0,0);
    assert(ALBPanUpdate(&s,24,-3,300,-40,10)==0);
    assert(ALBPanUpdate(&s,25,-40,0,-300,10)==0);
    ALBPanBegin(&s,0,0);
    assert(ALBPanUpdate(&s,0,-8,0,-20,10)==0);
    assert(ALBPanUpdate(&s,0,-17,0,-20,10)==0);
    assert(ALBPanUpdate(&s,0,-20,0,-20,10)==1);
    assert(ALBPanUpdate(&s,NAN,-40,0,-200,10)==0);
    ALBPanBegin(&s,0,0);
    assert(ALBPanUpdate(&s,0,12,0,200,10)==-1);
    /* 1.6m's actual 4pt / 80pt/s fast threshold, including in-gesture reversal. */
    ALBPanBegin(&s,0,0);
    assert(ALBPanUpdate(&s,0,-3,0,-80,4)==0);
    assert(ALBPanUpdate(&s,0,-4,0,-80,4)==1);
    assert(ALBPanUpdate(&s,0,-20,0,-120,4)==0);
    assert(ALBPanUpdate(&s,0,-16,0,80,4)==-1);
    ALBPanBegin(&s,0,0);
    assert(ALBPanUpdate(&s,6,0,100,0,4)==0);
    assert(ALBPanUpdate(&s,6,-20,0,-120,4)==0);
    for(int next=0;next<=1;next++) for(int timer=0;timer<=1;timer++)
        for(int compact=0;compact<=1;compact++) for(int expand=0;expand<=1;expand++) {
            int result=ALB8FeedCompactPolicy(next,timer,compact,expand);
            if(!next)assert(!result);
            else if(!timer)assert(result);
            else if(expand)assert(!result);
            else assert(result==compact);
        }
    assert(ALB8CommittedWorkDirection(0,4,0,5,1,0,1)==1);
    assert(ALB8CommittedWorkDirection(0,5,0,4,1,0,-1)==-1);
    assert(ALB8CommittedWorkDirection(0,4,0,4,1,1,1)==0); // horizontal photo move
    assert(ALB8CommittedWorkDirection(0,4,0,9,1,1,1)==0); // reload relocates same aweme
    assert(ALB8CommittedWorkDirection(0,4,0,4,1,0,1)==1); // new work in recycled row
    assert(ALB8CommittedWorkDirection(0,4,0,4,1,0,-1)==-1);
    assert(ALB8CommittedWorkDirection(0,4,0,4,0,0,1)==0); // insufficient evidence
    assert(ALB8CommittedWorkDirection(0,4,0,5,0,0,1)==1); // row is authoritative without ID
    int compact=1; // A timer already produced a ball.
    for(int i=0;i<1000;i++) {
        compact=ALB8FeedCompactPolicy(1,1,compact,1);
        assert(!compact); // next expands, a duplicate next commit stays open
        compact=ALB8FeedCompactPolicy(0,1,compact,1);
        assert(!compact);
        compact=1; // next independent idle deadline
    }
    feed_owner_sequences();
    puts("scroll intent: directions, reversal, dedup and manual-vs-idle feed ownership passed");
}
