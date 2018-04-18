#ifndef  __STORAGE__
#define  __STORAGE__
#include "protocol.h"
typedef struct memoryStorage
{
    persistentState* pState;
    snapshotMetaData* ssmd;
    list* entries;
}memoryStorage;

memoryStorage* newMemoryStorage()
{
    memoryStorage* ms = zmalloc(sizeof(memoryStorage));
    ms->pState = NULL;
    ms->ssmd = NULL;
    raftEntry* entry = createRaftEntry();
    ms->entries = listCreate();
    listAddNodeTail(ms->entries, entry);
    return ms;
}

persistentState* getPersistentState(memoryStorage* ms)
{
    return ms->pState;
}

void setPersistentState(memoryStorage* ms, persistentState* ps)
{
    if(ms->pState != NULL)
    {
        zfree(ms->pState);
    }
    ms->pState = ps;
}

confState* getconfState(memoryStorage* ms)
{
    return ms->ssmd->cs;
}

#endif // ! __STORAGE__