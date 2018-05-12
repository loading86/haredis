#ifndef __RAFT_PROGRESS__
#define __RAFT_PROGRESS__
#include "protocol.h"


typedef struct inflights
{
    uint64_t size;
    list* buffer;
}inflights;

inflights* newInflights(uint64_t size);

void freeInflights(inflights* inf);

void freeFirstOneInflight(inflights* inf);

void resetInflights(inflights* inf);

bool isInflightsFull(inflights* inf);

void addInflight(inflights* inf, uint64_t index);

void removeInflights(inflights* inf, uint64_t index);

typedef enum NodeState
{
    NodeStateProb,
    NodeStateReplicate,
    NodeStateSnapshot
}NodeState;

typedef struct raftNodeProgress
{
    uint8_t id;
    NodeState state;
    uint64_t match;
    uint64_t next;
    bool paused;
    inflights* ins;
    uint64_t pendingSnapshotIndex;
    bool active;
}raftNodeProgress;

raftNodeProgress* newRaftNodeProgress(uint8_t id, uint64_t inflights_size);

void freeRaftNodeProgress(raftNodeProgress* node);

void resetRaftNodeProgress(raftNodeProgress* node, NodeState state);

void becomeProbe(raftNodeProgress* node);

void becomeReplicate(raftNodeProgress* node);

void becomeSnaphot(raftNodeProgress* node, uint64_t pending_snapshot_index);

bool maybeUpdate(raftNodeProgress* node, uint64_t match_index);

void optimisticUpdate(raftNodeProgress* node, uint64_t last_sent_index);

bool maybeDecrTo(raftNodeProgress* node, uint64_t reject_index, uint64_t last_index);

bool canSend(raftNodeProgress* node);

bool shouldAbortSnapshot(raftNodeProgress* node);

void abortSnapshot(raftNodeProgress* node);

void resumeProgress(raftNodeProgress* node);

void pauseProgress(raftNodeProgress* node);









#endif // ! __RAFT_PROGRESS__