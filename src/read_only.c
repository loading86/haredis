#include "read_only.h"

ReadState* createReadState()
{
    ReadState* rs = zmalloc(sizeof(ReadState));
    rs->index = 0;
    rs->requestCtx = sdsempty();
}

ReadState* dupReadState(const ReadState* rs)
{
    ReadState* new_rs = zmalloc(sizeof(ReadState));
    new_rs->index = rs->index;
    new_rs->requestCtx = sdsdup(rs->requestCtx);
    return new_rs;
}

void freeReadState(ReadState* rs)
{
    sdsfree(rs->requestCtx);
    zfree(rs);
}