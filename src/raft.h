#ifndef __RAFT__
#define __RAFT__
#include <stdint.h>
#include <stdbool.h>

typedef void (*stepFunc)(raft* r, raftMessage* msg);
typedef void (*tickFunc)(raft* r);
typedef struct raftConfig
{
    uint8_t id;
    uint32_t electionTick;
    uint32_t heartbeatTick;
    bool checkQuorum;
    list* peers;
    uint64_t maxSizePerMsg;
    uint64_t maxInflightMsg;
    uint64_t applied;
    raftLog* raftlog;
}raftConfig;

typedef enum NodeStateType
{
    NodeStateFollower,
    NodeStateCandidate,
    NodeStateLeader
}NodeStateType;

typedef struct raft
{
    uint8_t id;
    uint8_t leader;
    uint64_t term;
    uint8_t voteFor;
    NodeStateType state;
    uint32_t electionTick;
    uint32_t elactionElapsed;
    uint32_t electionRandomTimeout;
    uint32_t heartbeatTick;
    uint32_t heartbeatElapsed;
    bool checkQuorum;
    uint64_t maxSizePerMsg;
    uint64_t maxInflightMsg;   
    list* peersProgress;
    raftLog* raftlog;
    bool pendingConf;
    stepFunc step;
    tickFunc tick;
}raft;

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
    r->peersProgress = listEmpty();
    listRewind(cfg->peers,&li);
    uint8_t peer;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        peer = (uint8_t)ln->value;
        raftNodeProgress* pr = newRaftNodeProgress(peer, r->maxInflightMsg);
        pr->next = 1;
        listAddNodeTail(r->peersProgress, pr);
    }
    becomeFollower(r, 0);
    return r;
}

void becomeFollower(raft* r, uint64_t term)
{
    
}
raftNodeProgress* getProgress(raft* r, uint8_t id)
{
    listIter li;
    listRewind(entries,&li);
    raftNodeProgress* progress;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        progress = ln->value;
        if (progress->id == id) {
            return progress
        }
    }
    return NULL;
}
#endif // !__RAFT__
