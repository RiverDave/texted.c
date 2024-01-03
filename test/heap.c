#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct heap {

  size_t size; // in bytes
};

#define HEAP_INIT { 0 }

struct heap H = HEAP_INIT;

bool *mem_alloc(size_t size) {

  if (H.size < size) {
    return NULL;
  }

  H.size += size;
  return EXIT_SUCCESS; 
}

void mem_free(void *ptr) { assert(false); }

int main(void) {

  void *mem = mem_alloc(sizeof(char) * 10);
  printf("H.size = %zu\n", H.size);
  mem_free(mem);


  return 0;
}
