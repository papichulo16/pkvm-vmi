#pragma once

#ifndef INSP_MSGQ_H
#define INSP_MSGQ_H

#include <semaphore.h>

typedef struct msgq msgq_t;

struct msgq {

  size_t qsize;
  char** queue;

  size_t head;
  size_t tail;

  sem_t* sem;
};

char* msgq_deq(msgq_t* q); // non blocking
void msgq_enq(msgq_t* q, char* data); // should call strdup on data

msgq_t* msgq_init(size_t size);
void msgq_free(msgq_t* q);

#endif

