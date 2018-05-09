#include "storage.h"
#include <assert.h>

memoryStorage* newMemoryStorage()
{
    memoryStorage* ms = zmalloc(sizeof(memoryStorage));
    ms->pState->term = 0;
    ms->pState->commited = 0;
    ms->pState->voteFor = 0;
    ms->ssmd = createSnapshotMetaData();
    raftEntry* entry = createRaftEntry();
    ms->entries = listCreate();
    listSetFreeMethod(ms->entries, freeRaftEntry);
    listAddNodeTail(ms->entries, entry);
    return ms;
}

hardState getHardState(memoryStorage* ms)
{
    return *(ms->pState);
}

void setHardState(memoryStorage* ms, hardState* ps)
{
    ms->pState = ps;
}

confState* getConfState(memoryStorage* ms)
{
    return dupConfState(ms->ssmd->cs);
}

uint64_t storageLastIndex(memoryStorage* ms)
{
    raftEntry *ent = listFirst(ms->entries)->value;
    return ent->index + listLength(ms->entries) - 1;
}

uint64_t storageFirstIndex(memoryStorage* ms)
{
    raftEntry *ent = listFirst(ms->entries)->value;
    return ent->index + 1;
}

EntriesResult getStorageEntries(memoryStorage* ms, uint64_t lo, uint64_t hi, uint64_t max_size)
{
    EntriesResult result;
    uint64_t offset = storageFirstIndex(ms) - 1;
    if(lo <= offset)
    {
        result.err = ErrCompacted;
        return result;
    }
    assert(hi <= storageLastIndex(ms) + 1);
    if(listLength(ms->entries) == 1)
    {
        result.err = ErrUnavailable;
        return result;
    }
    result.entries = listCreate();
    uint64_t lower = lo - offset;
    uint64_t upper = hi - offset;
    listNode* node = listIndex(ms->entries, lower);
    uint64_t count = upper - lower;
    while(count > 0)
    {
        incRaftEntryRefCnt()node->value;
        raftEntry* ent = node->value;
        listAddNodeTail(result.entries, ent);
        node = listNextNode(node);
        count--;
    }
    result.err = StorageOk;
    return result;
}

TermResult getStorageTermOf(memoryStorage* ms, uint64_t index)
{
    TermResult result;
    uint64_t offset = storageFirstIndex(ms) - 1;
    if(index < offset)
    {
        result.err = ErrCompacted;
        return result;
    }
    if(index - offset >= listLength(ms->entries))
    {
        result.err = ErrUnavailable;
        return result;
    }
    listNode* node = listIndex(ms->entries, index-offset);
    raftEntry* ent = node->value;
    result.term = ent->term;
    result.err = StorageOk;
    return result;
}

snapshotMetaData* getStorageSnapshotMD(memoryStorage* ms)
{
    return dupSnapshotMetaData(ms->ssmd);
}

StorageError ApplySnapshot(memoryStorage* ms, snapshotMetaData* ssmd)
{
    uint64_t local_ms_index = ms->ssmd->lastLogIndex;
    uint64_t snap_index = ssmd->lastLogIndex;
    if(local_ms_index >= snap_index)
    {
        return ErrSnapOutOfDate;
    }
    freeSnapshotMetaData(ms->ssmd);
    ms->ssmd = dupSnapshotMetaData(ssmd);
    listEmpty(ms->entries);
    raftEntry* entry = createRaftEntry();
    entry->index = ssmd->lastLogIndex;
    entry->term = ssmd->lastLogTerm;
    listAddNodeTail(ms->entries, entry);
    return StorageOk;
}

StorageError Compact(memoryStorage* ms, uint64_t compact_index)
{
    uint64_t offset = storageFirstIndex(ms) - 1;
    if(compact_index <= offset)
    {
        return ErrCompacted;
    }
    uint64_t last_index = storageLastIndex(ms);
    assert(compact_index <= last_index);
    uint64_t i = compact_index - offset;

    listNode* node = listIndex(ms->entries, i-offset);
    raftEntry* ent = node->value;

    raftEntry* entry = createRaftEntry();
    entry->index = ent->index;
    entry->term = ent->term;  

    uint64_t count = i + 1;
    while(count > 0)
    {
        listDelNode(ms->entries, listFirst(ms->entries));
        count--;
    }
    listAddNodeHead(ms->entries, entry);
    return StorageOk;
}

StorageError AppendEntriesToStorage(memoryStorage* ms, list* ents)
{
    if(listLength(ents) == 0)
    {
        return StorageOk;
    }
    list* copy_entries = listDup(ents);
    uint64_t first = storageFirstIndex(ms);
    listNode* node = listFirst(ms->entries);
    raftEntry* import_first_ent = node->value;
    uint64_t last = import_first_ent->index + listLength(copy_entries) - 1;
    if(last < first)
    {
        return StorageOk;
    }
    if(first > import_first_ent->index)
    {
        uint64_t count = first - import_first_ent->index;
        while(count > 0)
        {
            listDelNode(copy_entries, listFirst(copy_entries));
            count--;
        }       
    }
    uint64_t offset = storageFirstIndex(ms) - 1 - import_first_ent->index;
    if(listLength(ms->entries) >= offset)
    {
        uint64_t count =  listLength(ms->entries) - offset;
        while(count > 0)
        {
            listDelNode(ms->entries, listLast(ms->entries));
            count--;
        }
        listJoin(ms->entries, copy_entries);        
    }else
    {
        assert(false);
    }
    return StorageOk;
}