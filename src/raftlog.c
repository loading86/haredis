#include <stdlib.h>
#include <assert.h>
#include "raftlog.h"
#include "zmalloc.h"
raftLog* newRaftLog(snapshotMetaData* ssmd, list* entries)
{
    raftLog* raft_log = zmalloc(sizeof(raftLog));
    raft_log->persistSnapshotMD = ssmd;
    raft_log->persistEntries = entries;
    raft_log->unstableEntries = NULL;
    raft_log->commited = (uint64_t)listIndex(entries, 0)->value;
    raft_log->applied = raft_log->commited;
    raft_log->unstableOffset = (uint64_t)listIndex(entries, 0)->value + listLength(entries);
    return raft_log;
}

uint64_t termOf(list* entries, uint64_t index)
{
    listIter li;
    listRewind(entries,&li);
    raftEntry* entry;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        entry = ln->value;
        if (entry->index == index) {
            return entry->term;
        }
    }
    return UINT64_MAX;
}

bool matchTerm(raftLog* raftlog, uint64_t term, uint64_t index)
{
    uint64_t local_log_term = termOf(raftlog->unstableEntries, index);
    if(local_log_term < UINT64_MAX)
    {
        if(local_log_term == term)
        {
            return true;
        }
        return false;
    }
    if(raftlog->unstableSnapshotMD != NULL)
    {
        if(raftlog->unstableSnapshotMD->lastLogIndex == index)
        {
            return term == raftlog->unstableSnapshotMD->lastLogTerm;
        }
    }
    local_log_term = termOf(raftlog->persistEntries, index);
    if(local_log_term < UINT64_MAX)
    {
        if(local_log_term == term)
        {
            return true;
        }
    }
    return false;
}

uint64_t lastIndex(raftLog* raftlog)
{
    if(raftlog->unstableEntries != NULL)
    {
        return raftlog->unstableOffset + listLength(raftlog->unstableEntries);
    }
    if(raftlog->unstableSnapshotMD != NULL)
    {
        return raftlog->unstableSnapshotMD->lastLogIndex;
    }
    return (uint64_t)listIndex(raftlog->persistEntries, 0)->value + listLength(raftlog->persistEntries);

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
        offset++;
    }
    append(raftlog, entries);
    commitTo(raftlog, new_last_index < commited ? new_last_index : commited);
    return new_last_index;
} 

void append(raftLog* raftlog, list* entries)
{
    if(listFirst(entries) != NULL)
    {
        return;
    }
    raftEntry* raft_entry = listFirst(entries)->value;
    uint64_t after = raft_entry->index - 1;
    assert(after < raftlog->commited);
    truncateAndAppend(raftlog, entries);
}

void truncateAndAppend(raftLog* raftlog, list* entries)
{
    raftEntry* raft_entry = listFirst(entries)->value;
    uint64_t after = raft_entry->index;
    if(after == raftlog->unstableOffset + listLength(raftlog->unstableEntries))
    {
        listJoin(raftlog->unstableEntries, entries);
    }else if(after <= raftlog->unstableOffset)
    {
        listRelease(raftlog->unstableEntries);
        raftlog->unstableEntries = entries;
        raftlog->unstableOffset = after;
    }else
    {
        int unstable_len = listLength(raftlog->unstableEntries);
        int preserve = after - raftlog->unstableOffset;
        while(unstable_len > preserve)
        {
            listDelNode(raftlog->unstableEntries, listLast(raftlog->unstableEntries));
            --unstable_len;
        }
        listJoin(raftlog->unstableEntries, entries);
    }
}

void stableSnapTo(raftLog* raftlog, uint64_t index)
{
    if(raftlog->unstableSnapshotMD == NULL)
    {
        return;
    }
    if(raftlog->unstableSnapshotMD->lastLogIndex != index)
    {
        return;
    }
    if(raftlog->unstableSnapshotMD->cs->peers != NULL)
    {
        listRelease(raftlog->unstableSnapshotMD->cs->peers);
    }
    if(raftlog->unstableSnapshotMD->cs->learners != NULL)
    {
        listRelease(raftlog->unstableSnapshotMD->cs->learners);
    }   
    zfree(raftlog->unstableSnapshotMD);
    raftlog->unstableSnapshotMD = NULL;
}

void restoreSnapshotMD(raftLog* raftlog, snapshotMetaData* ssmd)
{
    raftlog->unstableOffset = ssmd->lastLogIndex + 1;
    listRelease(raftlog->unstableEntries);
    raftlog->unstableEntries = NULL;
    raftlog->unstableSnapshotMD = ssmd;
}