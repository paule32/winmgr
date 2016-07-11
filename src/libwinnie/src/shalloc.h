#ifndef SHALLOC_H_
#define SHALLOC_H_

#include <cstring>

#define POOL_SIZE 16777216
#define SHMNAME "/winnie.shm"

bool init_shared_memory();
void destroy_shared_memory();

void *sh_malloc(size_t bytes);
void sh_free(void *ptr);

void *get_pool();

#endif // SHALLOC_H_
