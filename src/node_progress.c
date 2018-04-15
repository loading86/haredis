#include "node_progress.h"
#include <stdlib.h>
#include "zmalloc.h"
inflights* newInflights(uint64_t size)
{
    inflights* inf = zmalloc(sizeof(inflights));
    inf->size = size;
    inf->buffer = listCreate();
    return inf;
}

void freeInflights(inflights* inf)
{
    if(inf == NULL)
    {
        return;
    }
    listRelease(inf->buffer);
    zfree(inf);
}

void resetInflights(inflights* inf)
{
    listEmpty(inf->buffer);
}

bool isInflightsFull(inflights* inf)
{
    return listLength(inf->buffer) == inf->size;
}

void addInflight(inflights* inf, uint64_t index)
{
    if(isInflightsFull(inf))
    {
        return;
    }
    listAddNodeTail(inf->buffer, (void*)index);
}

void removeInflights(inflights* inf, uint64_t index)
{
    listNode* ln = listFirst(inf->buffer);
    while(ln != NULL)
    {
        uint64_t inflight_index = (uint64_t)ln->value;
        if(index >= inflight_index)
        {
            listDelNode(inf->buffer, ln);
            ln = listFirst(inf->buffer);
        }else
        {
            return;
        }
    }
}