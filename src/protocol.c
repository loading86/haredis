#include "protocol.h"
#include "zmalloc.h"
#include <stdlib.h>
raftEntry* createRaftEntry()
{
    raftEntry* entry =  zmalloc(sizeof(raftEntry));
    entry->data = sdsempty();
    entry->term = 0;
    entry->index = 0;
    entry->entryType = EntryNormal;
    return entry;
}

void freeRaftEntry(raftEntry* entry)
{
    if(entry == NULL)
    {
        return;
    }
    sdsfree(entry->data);
    zfree(entry);
}

raftEntry* dupRaftEntry(const raftEntry* entry)
{
    if(entry == NULL)
    {
        return NULL;  
    }
    raftEntry* new_entry = createRaftEntry();
    sdsfree(new_entry->data);
    new_entry->data = sdsdup(new_entry->data);
    new_entry->term = new_entry->term;
    new_entry->index = new_entry->index;
    new_entry->entryType = new_entry->entryType;
    return new_entry;
}

snapshotMetaData* createSnapshotMetaData()
{
    snapshotMetaData* ssmd = zmalloc(sizeof(snapshotMetaData));
    ssmd->cs->peers = listCreate();
    ssmd->cs->learners = listCreate();
    ssmd->lastLogIndex = 0;
    ssmd->lastLogTerm = 0;
    return ssmd;
}

void freeSnapshotMetaData(snapshotMetaData* ssmd)
{
    if(ssmd == NULL)
    {
        return;
    }
    if(ssmd->cs->peers != NULL)
    {
        listRelease(ssmd->cs->peers);
    }
    if(ssmd->cs->learners != NULL)
    {
        listRelease(ssmd->cs->learners);
    }   
    zfree(ssmd);
}

snapshotMetaData* dupSnapshotMetaData(const snapshotMetaData* ssmd)
{
    if(ssmd == NULL)
    {
        return NULL;  
    }
    snapshotMetaData* new_ssmd = zmalloc(sizeof(snapshotMetaData));
    new_ssmd->cs->peers = listDup(ssmd->cs->peers);
    new_ssmd->cs->learners = listDup(ssmd->cs->learners);
    new_ssmd->lastLogIndex = ssmd->lastLogIndex;
    new_ssmd->lastLogTerm = ssmd->lastLogTerm;
    return new_ssmd;
}

snapshot* createSnapShot()
{
    snapshot* ss = zmalloc(sizeof(snapshot));
    ss->metaData = createSnapshotMetaData();
    ss->data = sdsempty();
    return ss;
}

void freeSnapShot(snapshot* ss)
{
    if(ss == NULL)
    {
        return;
    }
    freeSnapshotMetaData(ss->metaData);
    sdsfree(ss->data);
    zfree(ss);
}

snapshot* dupSnapShot(const snapshot* ss)
{
    if(ss == NULL)
    {
        return NULL;    
    }
    snapshot* new_ss = zmalloc(sizeof(snapshot));
    new_ss->metaData = dupSnapshotMetaData(ss->metaData);
    new_ss->data = sdsdup(ss->data);
    return new_ss;
}


raftMessage* createRaftMessage()
{
    raftMessage* msg = zmalloc(sizeof(raftMessage));
    msg->type = MessageProp;
    msg->from = 0;
    msg->to = 0;
    msg->term = 0;
    msg->preLogIndex = 0;
    msg->preLogTerm = 0;
    msg->commited = 0;
    msg->entries = listCreate();
    msg->ss = createSnapShot();
    msg->reject = false;
    msg->lastMatchIndex = 0;
    msg->context = sdsempty();
    listSetDupMethod(msg->entries, (void* (*)(void*))dupRaftEntry);
    listSetFreeMethod(msg->entries, (void (*)(void*))freeRaftEntry);
    return msg;
}

void freeRaftMessage(raftMessage* msg)
{   
    if(msg == NULL)
    {
        return;
    }
    listRelease(msg->entries);
    freeSnapShot(msg->ss);
    sdsfree(msg->context);
    zfree(msg);
}

raftMessage* dupRaftMessage(const raftMessage* msg)
{
    if(msg == NULL)
    {
        return NULL;  
    }
    raftMessage* new_msg = zmalloc(sizeof(raftMessage));
    new_msg->type = msg->type;
    new_msg->from = msg->from;
    new_msg->to = msg->to;
    new_msg->term = msg->term;
    new_msg->preLogIndex = msg->preLogIndex;
    new_msg->preLogTerm = msg->preLogTerm;
    new_msg->commited = msg->commited;
    new_msg->entries = listDup(msg->entries);
    new_msg->ss = dupSnapShot(msg->ss);
    new_msg->reject = msg->reject;
    new_msg->lastMatchIndex = msg->lastMatchIndex;
    new_msg->context = sdsdup(msg->context);
    listSetDupMethod(new_msg->entries, (void* (*)(void*))dupRaftEntry);
    listSetFreeMethod(new_msg->entries, (void (*)(void*))freeRaftEntry);
    return new_msg;
}

