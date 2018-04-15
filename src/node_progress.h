#ifndef __RAFT_PROGRESS__
#define __RAFT_PROGRESS__
#include "protocol.h"


typedef struct inflights
{
    uint64_t size;
    list* buffer;
}inflights;

inflights* newInflights(uint64_t size);

void freeInflights(inflights* inf);

void resetInflights(inflights* inf);

bool isInflightsFull(inflights* inf);

void addInflight(inflights* inf, uint64_t index);

void removeInflights(inflights* inf, uint64_t index);


#endif // ! __RAFT_PROGRESS__