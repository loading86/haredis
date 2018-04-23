#include "log_unstable.h"
#include <stdio.h>
#include <assert.h>
unstable* createUnstable()
{
    unstable* uns = zmalloc(sizeof(unstable));
    uns->ssmd = createSnapshotMetaData();
    uns->entries = listCreate();
    listSetFreeMethod(uns->entries, freeRaftEntry);
    uns->offset = 0;
    return uns;
}

uint64_t unstableMaybeFirstIndex(unstable* u)
{
    if(u->ssmd != NULL)
    {
        return u->ssmd->lastLogIndex + 1;
    }
    return UINT64_MAX;
}

uint64_t unstableMaybeLastIndex(unstable* u)
{
    if(listLength(u->entries) != 0)
    {
        return u->offset + listLength(u->entries) - 1;
    }
    if(u->ssmd != NULL)
    {
        return u->ssmd->lastLogIndex;
    }
    return UINT64_MAX;    
}

uint64_t unstableMaybeTerm(unstable* u, uint64_t index)
{
    if(index < u->offset)
    {
        if(u->ssmd == NULL)
        {
            return UINT64_MAX;
        }
        if(u->ssmd->lastLogIndex == index)
        {
            return u->ssmd->lastLogTerm;
        }
        return UINT64_MAX;
    }
    uint64_t last = unstableMaybeLastIndex(u);
    if(UINT64_MAX == last)
    {
        return UINT64_MAX;
    }
    if(index > last)
    {
        return UINT64_MAX;
    }
    raftEntry* entry = listIndex(u->entries, index - u->offset)->value;
    return entry->term;
}

void unstableStableTo(unstable* u, uint64_t index, uint64_t term)
{
    uint64_t real_term = unstableMaybeTerm(u, index);
    if(real_term == UINT64_MAX)
    {
        return;
    }
    if(real_term == term && index >= u->offset)
    {
        uint64_t lower = index + 1 - u->offset;
        while(lower > 0)
        {
            listDelNode(u->entries, listFirst(u->entries));
            u->offset = index + 1;
        }
    }
}

void unstableStableSnapTo(unstable* u, uint64_t index)
{
    if(u->ssmd != NULL && u->ssmd->lastLogIndex == index)
    {
        freeSnapshotMetaData(u->ssmd);
        u->ssmd = NULL;
    }
}

void unstableRestore(unstable* u, snapshotMetaData* ssmd)
{
    u->offset = ssmd->lastLogIndex + 1;
    listEmpty(u->entries);
    if(u->ssmd != NULL)
    {
        freeSnapshotMetaData(u->ssmd);
    }
    u->ssmd = ssmd;
}


void unstableTruncateAndAppend(unstable* u, list* entries)
{
    raftEntry* entry = listFirst(u->entries)->value;
    uint64_t after = entry->index;
    if(after == u->offset + listLength(u->entries))
    {
        listJoin(u->entries, entries);
    }else if(after <= u->offset)
    {
        u->offset = after;
        listEmpty(u->entries);
        u->entries = entries;
    }else
    {
        list* slice = unstableSlice(u, u->offset, after);
        listEmpty(u->entries);
        listJoin(u->entries, entries);
    }
}


list* unstableSlice(unstable* u, uint64_t lo, uint64_t hi)
{
    mustCheckOutOfBounds(u, lo, hi);
    list* new_list = listCreate();
    uint64_t lower = lo - u->offset;
    uint64_t upper = hi - u->offset;
    listNode* node = listIndex(u->entries, lower);
    uint64_t count = upper - lower;
    while(count > 0)
    {
        raftEntry* ent = dupRaftEntry(node->value);
        listAddNodeTail(new_list, ent);
        node = listNextNode(node);
        count--;
    }
    return new_list;
}

void mustCheckOutOfBounds(unstable* u, uint64_t lo, uint64_t hi)
{
    assert(lo <= hi);
    uint64_t upper = u->offset + listLength(u->entries);
    assert(lo >= u->offset && hi <= upper);
}


