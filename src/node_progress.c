#include "node_progress.h"
#include <stdlib.h>
#include "zmalloc.h"
inflights* newInflights(uint64_t size)
{
    inflights* inf = zmalloc(sizeof(inflights));
    inf->size = size;
    inf->buffer = listCreate();
    return inf;
}

void freeInflights(inflights* inf)
{
    if(inf == NULL)
    {
        return;
    }
    listRelease(inf->buffer);
    zfree(inf);
}

void resetInflights(inflights* inf)
{
    listEmpty(inf->buffer);
}

bool isInflightsFull(inflights* inf)
{
    return listLength(inf->buffer) == inf->size;
}

void addInflight(inflights* inf, uint64_t index)
{
    if(isInflightsFull(inf))
    {
        return;
    }
    listAddNodeTail(inf->buffer, (void*)index);
}

void removeInflights(inflights* inf, uint64_t index)
{
    listNode* ln = listFirst(inf->buffer);
    while(ln != NULL)
    {
        uint64_t inflight_index = (uint64_t)ln->value;
        if(index >= inflight_index)
        {
            listDelNode(inf->buffer, ln);
            ln = listFirst(inf->buffer);
        }else
        {
            return;
        }
    }
}


raftNodeProgress* newRaftNodeProgress(uint64_t inflights_size)
{
    raftNodeProgress* node = zmalloc(sizeof(raftNodeProgress));
    node->state = NodeStateProb;
    node->match = 0;
    node->next = 1;
    node->paused = false;
    node->ins = newInflights(inflights_size);
    node->pendingSnapshotIndex = 0;
    node->active = false;
    return node;
}

void freeRaftNodeProgress(raftNodeProgress* node)
{
    if(node == NULL)
    {
        return;
    }
    freeInflights(node->ins);
    zfree(node);
}

void resetRaftNodeProgress(raftNodeProgress* node, NodeState state)
{
    node->paused = false;
    node->pendingSnapshotIndex = 0;
    node->state = state;
    resetInflights(node->ins);
}

void becomeProbe(raftNodeProgress* node)
{
    if(node->state == NodeStateSnapshot)
    {
        uint64_t pending_snapshot_index = node->pendingSnapshotIndex;
        resetRaftNodeProgress(node, NodeStateProb);
        node->next = node->match + 1 > pending_snapshot_index + 1 ? node->match + 1 : pending_snapshot_index + 1;
    }else
    {
        resetRaftNodeProgress(node, NodeStateProb);
        node->next = node->match + 1;
    }
}

void becomeReplicate(raftNodeProgress* node)
{
    resetRaftNodeProgress(node, NodeStateReplicate);
    node->next = node->match + 1;
}

void becomeSnaphot(raftNodeProgress* node, uint64_t pending_snapshot_index)
{
    resetRaftNodeProgress(node, NodeStateSnapshot);
    node->pendingSnapshotIndex = pending_snapshot_index;
}

bool maybeUpdate(raftNodeProgress* node, uint64_t match_index)
{
    bool updated = false;
    if(match_index > node->match)
    {
        node->match = match_index;
        updated = true;
        node->paused = false;
    }

    if(match_index + 1 > node->next)
    {
        node->next = match_index + 1;
    }
    return updated;
}

void optimisticUpdate(raftNodeProgress* node, uint64_t last_sent_index)
{
    node->next = last_sent_index + 1;
}

bool maybeDecrTo(raftNodeProgress* node, uint64_t reject_index, uint64_t last_index)
{
    if(node->state == NodeStateReplicate)
    {
        if(reject_index <= node->match)
        {
            return false;
        }
        node->next = node->match + 1;
        return true;
    }

    if(node->next - 1 != reject_index)
    {
        return false;
    }

    node->next = reject_index < last_index + 1 ? reject_index : last_index + 1;
    if(node->next < 1)
    {
        node->next = 1;
    }
    node->paused = false;
    return true;
}

bool canSend(raftNodeProgress* node)
{
    if(node->state == NodeStateProb)
    {
        return node->paused;
    }else if (node->state == NodeStateReplicate)
    {
        return isInflightsFull(node->ins);
    }
    return true;
}


bool shouldAbortSnapshot(raftNodeProgress* node)
{
    return node->state == NodeStateSnapshot && node->pendingSnapshotIndex <= node->match;
}

void abortSnapshot(raftNodeProgress* node)
{
    node->pendingSnapshotIndex = 0;
}
