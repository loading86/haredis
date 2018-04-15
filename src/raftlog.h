#ifndef __RAFTLOG__
#define __RAFTLOG__
#include "protocol.h"

typedef struct raftLog 
{
    list* unstableEntries;
    uint64_t unstableOffset;
    list* persistEntries;
    snapshotMetaData* persistSnapshotMD;
    snapshotMetaData* unstableSnapshotMD;
    uint64_t commited;
    uint64_t applied;
}raftLog;

raftLog* newRaftLog(snapshotMetaData* ssmd, list* entries);
bool matchTerm(raftLog* raftlog, uint64_t term, uint64_t index);
uint64_t lastIndex(raftLog* raftlog);
uint64_t findConflict(raftLog* raftlog, list* entries);
void commitTo(raftLog* raftlog, uint64_t commited);
uint64_t maybeAppendEntries(raftLog* raftlog, uint64_t pre_term, uint64_t pre_index, uint64_t commited, list* entries);
void append(raftLog* raftlog, list* entries);
void truncateAndAppend(raftLog* raftlog, list* entries);
uint64_t termOf(list* entries, uint64_t index);
void stableSnapTo(raftLog* raftlog, uint64_t index);
void restoreSnapshotMD(raftLog* raftlog, snapshotMetaData* ssmd);



#endif // !__RAFTLOG__
