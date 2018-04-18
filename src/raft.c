#include "raft.h"
#include "rand.h"
#include <assert.h>
//#include "server.h"
raft* newRaft(raftConfig* cfg)
{
    raft* r = zmalloc(sizeof(raft));
    r->id = cfg->id;
    r->electionTick = cfg->electionTick;
    r->heartbeatTick = cfg->heartbeatTick;
    r->electionElapsed = 0;
    r->heartbeatElapsed = 0;
    r->leader = 0;
    r->voteFor = 0;
    r->maxSizePerMsg = cfg->maxSizePerMsg;
    r->maxInflightMsg = cfg->maxInflightMsg;
    r->raftlog = cfg->raftlog;
    listIter li;
    r->peersProgress = listCreate();
    r->votes = listCreate();
    listRewind(cfg->peers,&li);
    uint8_t peer;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        peer = (uint8_t)ln->value;
        raftNodeProgress* pr = newRaftNodeProgress(peer, r->maxInflightMsg);
        pr->next = 1;
        listAddNodeTail(r->peersProgress, pr);
    }
    becomeFollower(r, r->term, 0);
    return r;
}

void resetRaftTerm(raft* r, uint64_t term)
{
    if(r->term != term)
    {
        r->term = term;
        r->voteFor = 0;
    }
    r->leader = 0;
    r->electionElapsed = 0;
    r->heartbeatElapsed = 0;
    r->electionRandomTimeout = r->electionTick+ redisLrand48() % r->electionTick;
    listEmpty(r->votes);
    listIter li;
    listRewind(r->peersProgress,&li);
    raftNodeProgress* progress;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        progress = ln->value;
        resetRaftNodeProgress(progress, NodeStateProb);
        progress->next = lastIndex(r->raftlog) + 1;
        if (progress->id == r->id) {
            return progress->match = lastIndex(r->raftlog);
        }
    }    
}

void becomeFollower(raft* r, uint64_t term, uint64_t leader)
{
    r->step = stepFollower;
    resetRaftTerm(r, term);
    r->tick = tickElection;
    r->leader = leader;
    r->state = NodeStateFollower;
}


void becomeCandidate(raft* r)
{
    assert(r->state != NodeStateLeader);
    r->step = stepLeader;
    resetRaftTerm(r, r->term + 1);
    r->tick = tickElection;
    r->voteFor = r->id;
    r->state = NodeStateCandidate;
}

void becomeLeader(raft* r)
{
    assert(r->state != NodeStateFollower);
    r->step = stepLeader;
    resetRaftTerm(r, r->term);
    r->tick = r->heartbeatTick;
    r->leader = r->id;
    r->state = NodeStateLeader;
    raftEntry* entry = createRaftEntry();
    appendEntry(r, entry);
}


raftNodeProgress* getProgress(raft* r, uint8_t id)
{
    listIter li;
    listRewind(r->peersProgress,&li);
    raftNodeProgress* progress;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        progress = ln->value;
        if (progress->id == id) {
            return progress;
        }
    }
    return NULL;
}


void stepFollower(struct raft* r, raftMessage* msg)
{

}

void stepCandidate(struct raft* r, raftMessage* msg)
{

}

void stepLeader(struct raft* r, raftMessage* msg)
{

}

void tickElection(struct raft* r)
{

}

void tickHeartbeat(struct raft* r)
{

}

void appendEntry(raft* r, raftEntry* entry)
{

}