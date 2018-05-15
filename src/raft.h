#ifndef __RAFT__
#define __RAFT__
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "adlist.h"
#include "dict.h"
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
    uint64_t maxInflightMsgs;
    uint64_t applied;
    memoryStorage* storage;
}raftConfig;

typedef enum NodeStateType
{
    NodeStateFollower,
    NodeStateCandidate,
    NodeStateLeader
}NodeStateType;

typedef struct voteInfo 
{
    uint8_t id;
    bool granted;
}voteInfo;

bool matchVoteInfo(voteInfo* a, voteInfo* b);

typedef struct raft
{
    uint8_t id;
    uint8_t leader;
    uint64_t term;
    uint8_t voteFor;
    NodeStateType state;
    uint32_t electionTimeout;
    uint32_t electionElapsed;
    uint32_t electionRandomTimeout;
    uint32_t heartbeatTimeout;
    uint32_t heartbeatElapsed;
    bool checkQuorum;
    uint64_t maxSizePerMsg;
    uint64_t maxInflightMsgs;   
    dict* peers;
    dict* votes;
    list* msgs;
    raftLog* raftlog;
    bool pendingConf;
    stepFunc step;
    tickFunc tick;
    list* readStates;
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

int numOfPendingConf(list* ents);

int pollRaft(raft* r, uint64_t id, bool v);

int quorum(raft* r);

bool Step(raft* r, raftMessage* msg);

bool restoreSnapshot(raft* r, snapshot* ss);

void restoreNode(raft* r, list* nodes);

bool maybeCommitRaft(raft* r);

bool checkQuorumActive(raft* r);

void campaign(raft* r);

void broadCastAppend(raft* r);

void sendHeartBeat(raft* r, uint64_t to);

void broadcastHeartbeat(raft *r);
#endif // !__RAFT__
