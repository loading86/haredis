#ifndef  __LOG_UNSTABLE__
#define  __LOG_UNSTABLE__
#include "protocol.h"
typedef struct unstable 
{
    snapshotMetaData* ssmd;
    list* entries;
    uint64_t offset;
}unstable;

uint64_t unstableMaybeFirstIndex(unstable* u);

uint64_t unstableMaybeLastIndex(unstable* u);

uint64_t unstableMaybeTerm(unstable* u, uint64_t index);

void unstableStableTo(unstable* u, uint64_t index, uint64_t term);

void unstableStableSnapTo(unstable* u, uint64_t index);

void unstableRestore(unstable* u, snapshotMetaData* ssmd);


void unstableTruncateAndAppend(unstable* u, list* entries);


list* unstableSlice(unstable* u, uint64_t lo, uint64_t hi);

void mustCheckOutOfBounds(unstable* u, uint64_t lo, uint64_t hi);

#endif // ! __LOG_UNSTABLE__