#include "raft.h"
#include "rand.h"
#include <assert.h>
//#include "server.h"
raft* newRaft(raftConfig* cfg)
{
    raftLog* log = newRaftLog(cfg->storage);
    hardState hs = getHardState(cfg->storage);
    confState *cs = getConfState(cfg->storage);
    list* peers = cfg->peers;
    if(len(cs->peers) > 0)
    {
        if(len(peers) > 0)
        {
            assert(false);
        }
        peers = cs->peers;
    }
    raft* r = zmalloc(sizeof(raft));
    r->id = cfg->id;
    r->leader = 0;
    r->maxSizePerMsg = cfg->maxSizePerMsg;
    r->maxInflightMsgs = cfg->maxInflightMsgs;
    r->peers = listCreate();
    r->votes = listCreate();
    //listSetFreeMethod(free); todo
    r->electionTimeout = cfg->electionTick;
    r->heartbeatTimeout = cfg->heartbeatTick;
    r->checkQuorum = cfg->checkQuorum;
    r->msgs = listCreate();
    listSetFreeMethod(r->msgs, freeRaftMessage);
    listSetDupMethod(r->msgs, dupRaftMessage);
    r->readStates = listCreate();
    listSetFreeMethod(r->readStates, freeReadIndex);
    listSetDupMethod(r->readStates, dupReadIndex);
    listIter li;  
    listRewind(cfg->peers,&li);
    uint8_t peer;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        peer = (uint8_t)ln->value;
        raftNodeProgress* pr = newRaftNodeProgress(peer, r->maxInflightMsgs);
        pr->next = 1;
        listAddNodeTail(r->peers, pr);
    }
    becomeFollower(r, r->term, 0);
    return r;

}

void resetRaftTerm(raft* r, uint64_t term)
{
    if(r->term != term)
    {
        r->term = term;
        r->voteFor = 0;
    }
    r->leader = 0;
    r->electionElapsed = 0;
    r->heartbeatElapsed = 0;
    r->electionRandomTimeout = r->electionTick+ redisLrand48() % r->electionTick;
    listEmpty(r->votes);
    listIter li;
    listRewind(r->peersProgress,&li);
    raftNodeProgress* progress;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        progress = ln->value;
        resetRaftNodeProgress(progress, NodeStateProb);
        progress->next = lastIndex(r->raftlog) + 1;
        if (progress->id == r->id) {
            return progress->match = lastIndex(r->raftlog);
        }
    }    
}

void becomeFollower(raft* r, uint64_t term, uint64_t leader)
{
    r->step = stepFollower;
    resetRaftTerm(r, term);
    r->tick = tickElection;
    r->leader = leader;
    r->state = NodeStateFollower;
}


void becomeCandidate(raft* r)
{
    assert(r->state != NodeStateLeader);
    r->step = stepLeader;
    resetRaftTerm(r, r->term + 1);
    r->tick = tickElection;
    r->voteFor = r->id;
    r->state = NodeStateCandidate;
}

void becomeLeader(raft* r)
{
    assert(r->state != NodeStateFollower);
    r->step = stepLeader;
    resetRaftTerm(r, r->term);
    r->tick = r->heartbeatTick;
    r->leader = r->id;
    r->state = NodeStateLeader;
    raftEntry* entry = createRaftEntry();
    appendEntry(r, entry);
}


raftNodeProgress* getProgress(raft* r, uint8_t id)
{
    listIter li;
    listRewind(r->peersProgress,&li);
    raftNodeProgress* progress;
    listNode* ln;
    while ((ln = listNext(&li)) != NULL) {
        progress = ln->value;
        if (progress->id == id) {
            return progress;
        }
    }
    return NULL;
}


void stepFollower(struct raft* r, raftMessage* msg)
{
    switch(msg->type)
    {
        case MessageBeat:
            broadcastHeartbeat();
            return;
        case MessageProp:
            assert(listLength(msg->entries) != 0);
            raftNodeProgress* pr = getProgress(r, r->id);
            if(pr == NULL)
            {
                return;
            }
            listNode* n = listFirst(msg->entries);
            while(n != NULL)
            {
                raftEntry* ent = n->value;
                if(ent->type == EntryConfChange)
                {
                    if(r->pendingConf)
                    {
                        ent->type = EntryNormal;
                    }
                    r->pendingConf = true;
                }               
            }
            appendEntries(r, msg->entries);
            broadCastAppend(r);
            return;
        case MessageReadIndex:
            if(quorum(r) > 1)
            {
                TermResult res = termOf(r, r->raftlog->commited);
                uint64_t t = zeroTermOnErrCompacted(res.term, res.err);
                if(t != r.term)
                {
                    return;
                }
                if(msg->from == 0 || msg->from == r->id)
                {
                    ReadState *rs = createReadState();
                    rs->index = r->raftlog->commited;
                    raftEntry* ent = listFirst(msg->entries)->value;
                    rs->requestCtx = sdsdup(ent->data);
                    listAddNodeTail(r->readStates, rs);
                }else 
                {
                    raftMessage* m = createRaftMessage();
                    m->to = msg->from;
                    m->type = MessageReadIndexResp;
                    m->index = r->raftlog->commited;
                    listRelease(m->entries);
                    m->entries = listDup(msg->entries);
                    sendMsg(r, m);
                }
            }else 
            {
                ReadState *rs = createReadState();
                rs->index = r->raftlog->commited;
                raftEntry* ent = listFirst(msg->entries)->value;
                rs->requestCtx = sdsdup(ent->data);
                listAddNodeTail(r->readStates, rs);
            }
            return;
    }

    raftNodeProgress* pr = getProgress(msg->from);
    if(pr == NULL)
    {
        return;
    }
    
    switch(msg->type)
    {
        case MessageAppResp:
            pr->active = true;
            if(msg->reject)
            {
                if(maybeDecrTo(pr, msg->preLogIndex, msg->lastMatchIndex))
                {
                    if(pr->state == NodeStateReplicate)
                    {
                        becomeProbe(pr);
                    }
                }
                sendAppend(msg->from);
            }else
            {
                
            }
    }

}

void stepCandidate(struct raft* r, raftMessage* msg)
{
    MessageType vote_resp_type = MessageVoteResp;
    switch(msg->type)
    {
        case 
    }
}

void stepLeader(struct raft* r, raftMessage* msg)
{

}

void tickElection(struct raft* r)
{

}

void tickHeartbeat(struct raft* r)
{

}

void appendEntry(raft* r, raftEntry* entry)
{

}

void appendEntries(raft* r, list* entries)
{
    uint64_t last_index = lastIndex(r->raftlog);
    listNode* n = listFirst(msg->entries);
    uint64_t off = 1;
    while(n != NULL)
    {
        raftEntry* ent = n->value;
        ent->term = r->term;
        ent->index = last_index + off;
        off++;
    }
    append(r->raftlog, entries);
    raftNodeProgress* pr = getProgress(r->id);
    maybeUpdate(pr, lastIndex(r->raftlog));
    maybeCommit(r);
}


uint64_t quorum(raft* r)
{
    return listLength(r->peers)/2 + 1;
}

void sendMsg(raft* r, raftMessage* msg)
{
    msg->from = r->id;
    if(msg->type == MessageVote || msg->vote == MessageVoteResp)
    {
        if(msg->term == 0)
        {
            assert(false);
        }
    }else 
    {
        if(msg->type != 0)
        {
            assert(false);
        }
        if(msg->type != MessageProp && msg->type != MessageReadIndex)
        {
            msg->term = r->term;
        }
    }
    listAddNodeTail(r->msgs, msg);
}

void sendAppend(raft* r, uint64_t to)
{
    raftNodeProgress* pr = getProgress(r, to);
    if(!canSend(pr))
    {
        return;
    }
    raftMessage* msg = createRaftMessage();
    msg->to = to;
    TermResult term_res = termOf(r->raftlog, pr->next - 1);
    EntriesResult entries_res = entriesOfLog(r->raftlog, pr->next, UINT64_MAX);
    if(term_res.err != StorageOk || entries_res.err != StorageOk)
    {
        if(!pr->active)
        {
            return;
        }
        msg->type = MessageSnap;
        //TODO
    }else
    {
        msg->type = MessageApp;
        msg->preLogIndex = pr->next - 1;
        msg->preLogTerm = term_res.term;
        msg->entries = entries_res.entries;
        msg->commited = r->raftlog->commited;
        int len = listLength(msg->entries);
        if(len != 0)
        {
            if(pr->state == NodeStateReplicate)
            {
                raftEntry* ent = listLast(msg->entries)->value;
                uint64_t last_index = ent->index;
                optimisticUpdate(pr, last_index);
                addInflight(pr->ins, last_index);
            }else if(pr->state == NodeStateProb)
            {

            }else
            {
                asseet(false);
            }
        }

    }
    sendMsg(r, msg);
}