#ifndef  __STORAGE__
#define  __STORAGE__
#include "protocol.h"

typedef enum StorageError
{
    StorageOk,
    ErrCompacted,
    ErrSnapOutOfDate,
    ErrUnavailable,
    ErrSnapshotTemporarilyUnavailable
}StorageError;

typedef struct memoryStorage
{
    hardState* pState;
    snapshotMetaData* ssmd;
    list* entries;
}memoryStorage;

memoryStorage* newMemoryStorage();

hardState getHardState(memoryStorage* ms);

void setHardState(memoryStorage* ms, hardState* ps);

confState* getConfState(memoryStorage* ms);

uint64_t storageLastIndex(memoryStorage* ms);

uint64_t storageFirstIndex(memoryStorage* ms);

typedef struct EntriesResult
{
    list* entries;
    StorageError err;
}EntriesResult;

EntriesResult getStorageEntries(memoryStorage* ms, uint64_t lo, uint64_t hi, uint64_t max_size);

typedef struct TermResult
{
    uint64_t term;
    StorageError err;
}TermResult;

TermResult getStorageTermOf(memoryStorage* ms, uint64_t index);

snapshotMetaData* getStorageSnapshotMD(memoryStorage* ms);

StorageError ApplySnapshot(memoryStorage* ms, snapshotMetaData* ssmd);

StorageError Compact(memoryStorage* ms, uint64_t compact_index);

StorageError AppendEntriesToStorage(memoryStorage* ms, list* ents);

#endif // ! __STORAGE__