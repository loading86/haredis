#ifndef  __READ_ONLY_H__
#define __READ_ONLY_H__
#include <inttypes.h>
#include "sds.h"
typedef struct ReadState
{
    uint64_t index;
    sds requestCtx;
}ReadState;

ReadState* createReadState();

ReadState* dupReadState(const ReadState* rs);

void freeReadState(ReadState* rs);




#endif // ! __READ_ONLY_H__