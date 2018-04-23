#ifndef __RAFTLOG__
#define __RAFTLOG__
#include "protocol.h"
#include "log_unstable.h"
#include "storage.h"
typedef struct raftLog 
{
    unstable* uns;
    memoryStorage* ms;
    uint64_t commited;
    uint64_t applied;
}raftLog;

raftLog* newRaftLog(memoryStorage* ms);

TermResult termOf(raftLog* raftlog, uint64_t index);

bool matchTerm(raftLog* raftlog, uint64_t term, uint64_t index);

uint64_t lastIndex(raftLog* raftlog);

uint64_t firstIndex(raftLog* raftlog);

uint64_t findConflict(raftLog* raftlog, list* entries);

void commitTo(raftLog* raftlog, uint64_t commited);

uint64_t maybeAppendEntries(raftLog* raftlog, uint64_t pre_term, uint64_t pre_index, uint64_t commited, list* entries);

uint64_t append(raftLog* raftlog, list* entries);


void stableSnapTo(raftLog* raftlog, uint64_t index);

void stableTo(raftLog* raftlog, uint64_t index, uint64_t term);

bool isUpToDate(raftLog* raftlog, uint64_t last_index, uint64_t term);

bool maybeCommit(raftLog* raftlog, uint64_t max_index, uint64_t term);

void restoreSnapshotMD(raftLog* raftlog, snapshotMetaData* ssmd);

list* unstableEntries(raftLog* raftlog);

list* nextEnts(raftLog* raftlog);

bool hasNextEnts(raftLog* raftlog);

uint64_t lastTerm(raftLog* raftlog);

EntriesResult slice(raftLog* raftlog, uint64_t lo, uint64_t hi, uint64_t max_size);

StorageError raftLogMustCheckOutOfBounds(raftLog* raftlog, uint64_t lo, uint64_t hi);

uint64_t zeroTermOnErrCompacted(uint64_t term, StorageError err);

EntriesResult entriesOfLog(raftLog* raftlog, uint64_t index, uint64_t max_size);



#endif // !__RAFTLOG__
