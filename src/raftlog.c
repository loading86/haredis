#include <stdlib.h>
#include <assert.h>
#include "raftlog.h"
#include "zmalloc.h"
raftLog* newRaftLog(memoryStorage* ms)
{
    raftLog* raft_log = zmalloc(sizeof(raftLog));
    raft_log->ms = ms;
    raft_log->uns = createUnstable();
    uint64_t first_index = storageFirstIndex(ms);
    uint64_t last_index = storageLastIndex(ms);
    raft_log->uns->offset = last_index + 1;
    raft_log->commited = first_index - 1;
    raft_log->applied = raft_log->commited;
    return raft_log;
}

TermResult termOf(raftLog* raftlog, uint64_t index)
{
    TermResult result;
    result.err = StorageOk;
    uint64_t dummy_index = firstIndex(raftlog) - 1;
    if(index < dummy_index || index > lastIndex(raftlog))
    {
        result.term = 0;       
        return result;
    }
    uint64_t t = unstableMaybeTerm(raftlog->uns, index);
    if(t != UINT64_MAX)
    {
        result.term = t;
        return result;
    }
    result = getStorageTermOf(raftlog->ms, index);
    if(result.err ==  StorageOk)
    {
        return result;
    }
    if(result.err ==  ErrCompacted || result.err ==  ErrUnavailable)
    {
        result.term = 0;
        return result;
    }
    assert(false);
}

bool matchTerm(raftLog* raftlog, uint64_t term, uint64_t index)
{
    TermResult result = termOf(raftlog, index);
    if(result.err != StorageOk)
    {
        return false;
    }
    return result.term == term;
}

uint64_t lastIndex(raftLog* raftlog)
{
    uint64_t index = unstableMaybeLastIndex(raftlog->uns);
    if(index != UINT64_MAX)
    {
        return index;
    }

    return storageLastIndex(raftlog->ms);
}

uint64_t firstIndex(raftLog* raftlog)
{
    uint64_t index = unstableMaybeFirstIndex(raftlog->uns);
    if(index != UINT64_MAX)
    {
        return index;
    }

    return storageFirstIndex(raftlog->ms);
}


uint64_t findConflict(raftLog* raftlog, list* entries)
{
    listIter li;
    listRewind(entries,&li);
    raftEntry* entry;
    //uint64_t last_index = lastIndex(raftlog);
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        entry = ln->value;
        if(!matchTerm(raftlog, entry->term, entry->index))
        {
            return entry->index;
        }
    }
    return 0;    
}

void commitTo(raftLog* raftlog, uint64_t commited)
{
    assert(commited > raftlog->commited);
    raftlog->commited = commited;
}

uint64_t maybeAppendEntries(raftLog* raftlog, uint64_t pre_term, uint64_t pre_index, uint64_t commited, list* entries)
{
    if(!matchTerm(raftlog, pre_term, pre_index))
    {
        return UINT64_MAX;
    }
    uint64_t new_last_index = pre_index + listLength(entries);
    uint64_t conflict_index = findConflict(raftlog, entries);
    assert(conflict_index > raftlog->commited);
    uint64_t offset = conflict_index - pre_index - 1;
    while(offset > 0)
    {
        listDelNode(entries, listFirst(entries));
        offset--;
    }
    append(raftlog, entries);
    commitTo(raftlog, new_last_index < commited ? new_last_index : commited);
    return new_last_index;
} 

uint64_t append(raftLog* raftlog, list* entries)
{
    if(listLength(entries) == 0)
    {
        return lastIndex(raftlog);
    }
    raftEntry* raft_entry = listFirst(entries)->value;
    uint64_t after = raft_entry->index - 1;
    assert(after < raftlog->commited);
    unstableTruncateAndAppend(raftlog->uns, entries);
    return lastIndex(raftlog);
}


void stableSnapTo(raftLog* raftlog, uint64_t index)
{
    unstableStableSnapTo(raftlog->uns, index);
}

void stableTo(raftLog* raftlog, uint64_t index, uint64_t term)
{
    unstableStableTo(raftlog->uns, index, term);
}

bool isUpToDate(raftLog* raftlog, uint64_t last_index, uint64_t term)
{
    return term > lastTerm(raftlog) || (term == lastTerm(raftlog) && last_index >= lastIndex(raftlog));
}

bool maybeCommit(raftLog* raftlog, uint64_t max_index, uint64_t term)
{
    if(max_index > raftlog->commited)
    {
        TermResult result  = termOf(raftlog, max_index);
        if(zeroTermOnErrCompacted(result.term, result.err) == term)
        {
            commitTo(raftlog, max_index);
            return true;
        }
    }
    return false;
}

void restoreSnapshotMD(raftLog* raftlog, snapshotMetaData* ssmd)
{
    raftlog->commited = ssmd->lastLogIndex;
    unstableRestore(raftlog->uns, ssmd);
}

list* unstableEntries(raftLog* raftlog)
{
    if(listLength(raftlog->uns->entries) == 0)
    {
        return NULL;
    }
    return listDup(raftlog->uns->entries);
}

list* nextEnts(raftLog* raftlog)
{
    uint64_t first_index = firstIndex(raftlog);
    uint64_t offset = raftlog->applied + 1 > first_index ? raftlog->applied + 1 : first_index;
    if(raftlog->commited + 1 > offset)
    {
        EntriesResult result = slice(raftlog, offset, raftlog->commited + 1, UINT64_MAX );
        assert(result.err == StorageOk);
        return result.entries;
    }
    return NULL;
}

bool hasNextEnts(raftLog* raftlog)
{
    uint64_t first_index = firstIndex(raftlog);
    uint64_t offset = raftlog->applied + 1 > first_index ? raftlog->applied + 1 : first_index;
    return raftlog->commited + 1 > offset;
}

uint64_t lastTerm(raftLog* raftlog)
{
    uint64_t last_index = lastIndex(raftlog);
    TermResult result = termOf(raftlog, last_index);
    assert(result.err == StorageOk);
    return result.term;
}

EntriesResult slice(raftLog* raftlog, uint64_t lo, uint64_t hi, uint64_t max_size)
{
    EntriesResult result;
    result.err = StorageOk;
    result.entries = NULL;
    StorageError err = raftLogMustCheckOutOfBounds(raftlog, lo, hi);
    if(err != StorageOk)
    {
        result.err = err;
        return result;
    }
    if(lo == hi)
    {
        return result;
    }
    // result.entries = listCreate();
    // listSetFreeMethod(result.entries, freeRaftEntry);
    if(lo < raftlog->uns->offset)
    {
        uint64_t upper = hi < raftlog->uns->offset ? hi : raftlog->uns->offset;
        result = getStorageEntries(raftlog->ms, lo, upper, max_size);
        if(result.err == ErrCompacted)
        {
            return result;
        }else if(result.err == ErrUnavailable)
        {
            assert(false);
        }else if(result.err != StorageOk)
        {
            assert(false);
        }
        if(listLength(result.entries) < upper - lo)
        {
            return result;
        }
    }
    if(hi > raftlog->uns->offset)
    {
        uint64_t lower = lo > raftlog->uns->offset ? lo : raftlog->uns->offset;
        list* unstable_ents = unstableSlice(raftlog->uns, lower, hi);;
        if(result.entries == NULL)
        {
            result.entries = unstable_ents;
        }else{
            listJoin(result.entries, unstable_ents);
        }
    }
    return result;
}

StorageError raftLogMustCheckOutOfBounds(raftLog* raftlog, uint64_t lo, uint64_t hi)
{
    assert(lo <= hi);
    uint64_t fi = firstIndex(raftlog);
    if(lo < fi)
    {
        return ErrCompacted;
    }
    uint64_t length = lastIndex(raftlog) + 1 - fi;
    assert(lo >= fi && hi <= fi + length);
    return StorageOk;
}

uint64_t zeroTermOnErrCompacted(uint64_t term, StorageError err)
{
    if(err == StorageOk)
    {
        return term;
    }
    if(err == ErrCompacted)
    {
        return 0;
    }
    assert(false);
    return 0;
}

EntriesResult entriesOfLog(raftLog* raftlog, uint64_t index, uint64_t max_size)
{
    EntriesResult result;
    if( index > lastIndex(raftlog))
    {
        result.err = StorageOk;
        result.entries = NULL;
        return result;
    }
    return slice(raftlog, index, lastIndex(raftlog) + 1, max_size);
}

