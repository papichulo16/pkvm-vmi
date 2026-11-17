#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include "user_msgq.h"
#include "print.h"

void* msgq_malloc(size_t s) {

  void* p = malloc(s);

  if (!p) {

    print_err("malloc error", 0);
    exit(-1);
  }

  return p;
}

char* msgq_deq(msgq_t* q) {

  char* ret; 

  if (q->head == q->tail)
    return NULL;

  ret = q->queue[q->head];
  q->head = (q->head + 1) % q->qsize;
  sem_post(q->sem);

  return ret; 
}

void msgq_enq(msgq_t* q, char* data) {

  sem_wait(q->sem);

  q->queue[q->tail] = strdup(data);
  q->tail = (q->tail + 1) % q->qsize;
}

msgq_t* msgq_init(size_t size) {

  msgq_t* q = msgq_malloc(sizeof(msgq_t)); 

  q->qsize = size;
  q->queue = msgq_malloc(sizeof(q->queue) * size);
  q->head = 0;
  q->tail = 0;
  q->sem = msgq_malloc(sizeof(sem_t));

  sem_init(q->sem, 0, size);

  return q;
}

void msgq_free(msgq_t* q) {

  sem_destroy(q->sem);

  free(q->sem);
  free(q->queue);
  free(q);
}

