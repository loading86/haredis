#ifndef __RAFT_PROTOCOL_H
#define __RAFT_PROTOCOL_H
#include <stdint.h>
#include <stdbool.h>
#include "sds.h"
#include "adlist.h"
typedef enum EntryType
{
    EntryNormal = 1,
    EntryConfChange
}EntryType;

typedef struct raftEntry
{
    uint64_t term;
    uint64_t index;
    EntryType entryType;
    sds data;
}raftEntry;

typedef enum MessageType
{
    MessageProp = 1,
    MessageHeartBeat,
    MessageHeartBeatResp,
    MessageApp,
    MessageAppResp,
    MessageVote,
    MessageVoteResp,
    MessageReadIndex,
    MessageReadIndexResp,
    MessageSnap
}MessageType;

typedef struct snapshotMetaData
{
    list* peers;
    list* learners;
    uint64_t lastLogIndex;
    uint64_t lastLogTerm; 
}snapshotMetaData;

typedef struct snapshot
{
    snapshotMetaData* metaData;
    sds data;
}snapshot;

typedef struct raftMessage
{
    MessageType type;
    uint8_t from;
    uint8_t to;
    uint64_t term;
    uint64_t preLogIndex;
    uint64_t preLogTerm;
    uint64_t commited;
    list* entries;
    snapshot* ss;
    bool reject;
    uint64_t lastMatchIndex;
    sds context;
}raftMessage;

typedef struct persistentState 
{
    uint64_t commited;
    uint64_t term;
    uint8_t voteFor;
}persistentState;

typedef enum ConfChangeType
{
    ConfChangeAddPeer = 1,
    ConfChangeAddLearner,
    ConfChangeRemoveNode,
}ConfChangeType;

typedef struct ConfChange
{
    ConfChangeType type;
    uint64_t nodeID;
}ConfChange;












raftEntry* createRaftEntry();
void freeRaftEntry(raftEntry* entry);

raftEntry* dupRaftEntry(const raftEntry* entry);

snapshotMetaData* createSnapshotMetaData();

void freeSnapshotMetaData(snapshotMetaData* ssmd);

snapshotMetaData* dupSnapshotMetaData(const snapshotMetaData* ssmd);

snapshot* createSnapShot();

void freeSnapShot(snapshot* ss);

snapshot* dupSnapShot(const snapshot* ss);


raftMessage* createRaftMessage();

void freeRaftMessage(raftMessage* msg);

raftMessage* dupRaftMessage(const raftMessage* msg);

#endif //  