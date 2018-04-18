#ifndef __RAFT__
#define __RAFT__
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "adlist.h"
#include "raftlog.h"
#include "node_progress.h"

struct raft;
typedef void (*stepFunc)(struct raft* r, raftMessage* msg);
typedef void (*tickFunc)(struct raft* r);
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
    uint32_t electionElapsed;
    uint32_t electionRandomTimeout;
    uint32_t heartbeatTick;
    uint32_t heartbeatElapsed;
    bool checkQuorum;
    uint64_t maxSizePerMsg;
    uint64_t maxInflightMsg;   
    list* peersProgress;
    list* votes;
    raftLog* raftlog;
    bool pendingConf;
    stepFunc step;
    tickFunc tick;
}raft;

raft* newRaft(raftConfig* cfg);

void resetRaftTerm(raft* r, uint64_t term);

void becomeFollower(raft* r, uint64_t term, uint64_t leader);

void becomeCandidate(raft* r);

void becomeLeader(raft* r);

raftNodeProgress* getProgress(raft* r, uint8_t id);

void stepFollower(struct raft* r, raftMessage* msg);
void stepCandidate(struct raft* r, raftMessage* msg);
void stepLeader(struct raft* r, raftMessage* msg);
void tickElection(struct raft* r);
void tickHeartbeat(struct raft* r);

void appendEntry(raft* r, raftEntry* entry);

#endif // !__RAFT__
